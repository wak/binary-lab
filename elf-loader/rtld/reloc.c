#include <elf.h>
#include <lib.h>
#include <ldsodefs.h>
#include <lookup.h>
#include <link.h>

extern void runtime_resolve(ElfW(Word)) rtld_local;

/* 初期値のGOTを経由してやってくる関数 */
void *got_fixup(struct link_map *l, ElfW(Word) reloc_offset)
{
	Elf64_Addr *got;
	ElfW(Rela) *jmprel, *rela;
	ElfW(Xword) pltrelsz;
	ElfW(Sym) *symtab, *sym;
	ElfW(Word) symidx;
	const char *strtab, *name;
	struct sym_val symval;
	void *jmp;

	/* 手順:
	 *   - 引数から，再配置情報を取得する（DT_JMPRELから取得可能）
	 *   - その再配置情報から，どのシンボルをコールしたのかを調べる
	 *   - そのシンボルはどこにあるのかを探す
	 *   - 見つかったシンボル値をGOTに設定する
	 *   - シンボル値にjmpする
	 */
	MPRINT_START_FMT(LAZYRELOC, "GOT Trampline (%s, off:%u)",
			 l->l_name, reloc_offset);
	got      = (Elf64_Addr *) D_PTR(l, DT_PLTGOT);
	jmprel   = (void *) D_PTR(l, DT_JMPREL);
	pltrelsz = D_VAL(l, DT_PLTRELSZ);
	rela = &jmprel[reloc_offset];

	assert(pltrelsz/sizeof(ElfW(Rela)) > reloc_offset);

	symidx = ELF64_R_SYM(rela->r_info);
	symtab = (void *) D_PTR(l, DT_SYMTAB);
	strtab = (void *) D_PTR(l, DT_STRTAB);

	sym = &symtab[symidx];
	name = &strtab[sym->st_name];

	MPRINTF(LAZYRELOC, "name: %s\n", name);
	if (lookup_symbol(NULL, name, &symval) != 0)
		dprintf_die("symbol %s not found\n", name);
	jmp = (void *) (symval.m->l_addr + symval.s->st_value);
	MPRINTF(LAZYRELOC,
		"symbol found: in %s, symval:%#x, addr:%p\n",
		symval.m->l_name, symval.s->st_value, jmp);
	got[3 + reloc_offset] = (Elf64_Addr) jmp;
	MPRINT_END(LAZYRELOC);

	return jmp;
}
HIDDEN(got_fixup);

static void reloc_rela(struct link_map *l, ElfW(Rela) *rela, unsigned long count)
{
	struct sym_val symval;
	ElfW(Sym) *symtab;
	const char *strtab;
	ElfW(Addr) *got = NULL;
	symtab = (void *) D_PTR(l, DT_SYMTAB);
	strtab = (void *) D_PTR(l, DT_STRTAB);

	if (l->l_info[DT_PLTGOT] != NULL)
		got = (ElfW(Addr) *) D_PTR(l, DT_PLTGOT);

	struct sym_val find_symbol(const struct link_map *skip, const char *name) {
		struct sym_val v;
		if (lookup_symbol(skip, name, &v) != 0)
			dprintf_die("\nsymbol %s not found\n", name);
		return v;
	}
	for (; count-- > 0; rela++) {
		ElfW(Xword) *reloc = (void *) (l->l_addr + rela->r_offset);
		const char *name;
		ElfW(Sym) *sym;

		sym = &symtab[ELF64_R_SYM(rela->r_info)];
		name = strtab + sym->st_name;

		switch (ELF64_R_TYPE(rela->r_info)) {
		case R_X86_64_RELATIVE:
			/* ロードアドレスを加算する */
			MPRINTF(RELOC, "reloc RELATIVE *%p(%lx) += %lx + %lx\n",
				reloc, *reloc, l->l_addr, rela->r_addend);
			*reloc += l->l_addr + rela->r_addend;
			break;
		case R_X86_64_64:
			/* これでいいのかわからない */
			MPRINTF(RELOC, "reloc 64 :%10s", name);
			symval = find_symbol(NULL, name);
			MPRINTF(RELOC, " *%p(%lx) += %lx\n",
				reloc, *reloc, symval.m->l_addr + symval.s->st_value);
			*reloc += symval.m->l_addr + symval.s->st_value;
			break;
		case R_X86_64_JUMP_SLOT:
			/* ロードアドレスを加算する */
			MPRINTF(RELOC, "reloc JUMP_SLOT *%p(%lx) += %lx\n",
				reloc, *reloc, l->l_addr);
			*reloc += l->l_addr;
			break;
		case R_X86_64_GLOB_DAT:
			/* グローバル変数のアドレスをセットする．
			 *
			 * 共有ライブラリでのグローバル変数へのアクセスは，ポイン
			 * タを一つ経由して行われる．そのポインタを設定するのがこ
			 * の再配置種別．そのポインタはどこに入れるのかといえ
			 * ば，.gotセクションらしい（.got.pltでは少なくともない）
			 * */
			/*
			if (got > reloc) {
				dprintf("got   : %p\n", got);
				dprintf("reloc : %p\n", reloc);
				dprintf("OFFSET: %lu\n", ((void*)got - (void*)reloc) / sizeof(ElfW(Addr)));
			}
			*/
			MPRINTF(RELOC, "reloc GLOB_DAT :%s\n", name);
			symval = find_symbol(NULL, name);
			MPRINTF(RELOC, "      *%p(%lx) += %lx + %lx\n",
				reloc, *reloc, symval.m->l_addr, symval.s->st_value);
			*reloc += symval.m->l_addr + symval.s->st_value;
			break;
		case R_X86_64_COPY:
		{
			/* 変数の初期値を共有ライブラリからコピーする．
			 *
			 * GLOB_DATでは，このコピー先の領域のアドレスをセットする
			 * ことに注意．シンボルを探すときには，自分のシンボルが引っ
			 * かからないように注意する． */
			ElfW(Xword) copy;

			MPRINTF(RELOC, "reloc COPY :%s\n", name);
			symval = find_symbol(l, name);
			MPRINTF(RELOC, "      from '%s'\n", symval.m->l_name);
			copy = *(ElfW(Xword) *)(symval.m->l_addr+symval.s->st_value);
			MPRINTF(RELOC, "      *%p = *(%lx + %lx) == (%lx)\n",
				reloc, symval.m->l_addr, symval.s->st_value, copy);
			*reloc = copy;
			break;
		}
		/*
		case R_X86_64_DTPMOD64:
			dprintf("WARNING: ignore DTPMOD64\n");
			break;
		case R_X86_64_TPOFF64:
			dprintf("WARNING: ignore TPOFF64\n");
			break;
		*/
		default:
			dprintf_die("  unknown reloc type (%lu)\n",
				    ELF64_R_TYPE(rela->r_info));
			break;
		}
	}
}

/* REF: _dl_relocate_object [glibc/elf/dl-reloc.c] */
static void relocate_object(struct link_map *l)
{
	MPRINT_START_FMT(RELOC, "RELOCAION (%s)", l->l_name);

	if (l->l_relocated) {
		MPRINTF(RELOC, "already relocated\n");
		return;
	}
	if (D_VALID(l, DT_RELA)) {
		reloc_rela(l, (ElfW(Rela)*) D_PTR(l, DT_RELA),
			   D_VAL(l, DT_RELASZ) / sizeof(ElfW(Rela)));
	}

	if (D_VALID(l, DT_PLTGOT)) {
		Elf64_Addr *got;
		ElfW(Addr) jmprel;
		ElfW(Xword) pltrel, pltrelsz;

		got      = (ElfW(Addr) *) D_PTR(l, DT_PLTGOT);
		jmprel   = D_PTR(l, DT_JMPREL);
		pltrel   = D_VAL(l, DT_PLTREL);
		pltrelsz = D_VAL(l, DT_PLTRELSZ);

		MPRINT_START(RELOC, "GOT & PLT info");
		MPRINTF(RELOC, "GOT address: %p\n", (void *) got);
		MPRINTF(RELOC, "     JMPREL: %#lx (PLT relocs addr. [.rela.plt])\n", jmprel);
		MPRINTF(RELOC, "     PLTREL: %#lx (typeof reloc in PLT)\n", pltrel);
		MPRINTF(RELOC, "   PLTRELSZ: %#lx bytes (sizeof PLT relocs)\n", pltrelsz);
		MPRINT_END(RELOC);
		if (pltrel != DT_RELA)
			dprintf_die("PLTREL type is not DT_RELA. not supported\n");
		reloc_rela(l, (void *) jmprel, pltrelsz / sizeof(ElfW(Rela)));

		/* 動的リンカ依存のGOTの値を設定する
		 *
		 *   plt_0:
		 *       push got[1] <-- プッシュ（動的リンカが設定したもの)
		 *       jmp got[2]  <-- レゾルバへジャンプ
		 *
		 *   print@plt:      <-- print()するとここにくる
		 *       jmp got[3]
		 *       push $0     <-- このPLTの識別番号みたいなもの
		 *       jmp plt_0
		 */
		got[1] = (Elf64_Addr) l;  /* この共有オブジェクトを表すもの */
		got[2] = (Elf64_Addr) &runtime_resolve;

	}
	MPRINT_END(RELOC);
	l->l_relocated = 1;
}
HIDDEN(relocate_object);

void reloc_all(void)
{
	struct link_map *l;

	l = GL(namespace);
	assert(l != NULL);
	while (l->l_next)
		l = l->l_next;
	while (l) {
		relocate_object(l);
		l = l->l_prev;
	}
}
HIDDEN(reloc_all);
