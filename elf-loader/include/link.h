#ifndef LINK_H
#define LINK_H

#include <elf.h>
#include <list.h>

#include <defs.h>

//#define ElfW(type) Elf64_##type

#define __ELF_NATIVE_CLASS 64
#define ElfW(type)	_ElfW (Elf, __ELF_NATIVE_CLASS, type)
#define ELFW(type)	_ElfW (ELF, __ELF_NATIVE_CLASS, type)
#define _ElfW(e,w,t)	_ElfW_1 (e, w, _##t)
#define _ElfW_1(e,w,t)	e##w##t

/* link_map->l_info[]にアクセスするときに使う．*/
#define D_PTR(map, i)  ((map)->l_info[i]->d_un.d_ptr + (map)->l_addr)
#define D_VAL(map, i)  ((map)->l_info[i]->d_un.d_val)
#define D_VALID(map, i)((map)->l_info[i] != NULL)

/* Structure to describe a single list of scope elements.  The lookup
   functions get passed an array of pointers to such structures.  */
struct r_scope_elem
{
	struct link_map **r_list;	  /* Array of maps for the scope.  */
	unsigned int r_nlist;		  /* Number of entries in the scope.  */
};

#ifndef DT_THISPROCNUM
# define DT_THISPROCNUM 0
#endif

typedef uint32_t Elf_Symndx;

/* マップしたファイルの情報を格納する
 *
 * ポインタ変数はすべて，直接参照可能．ElfW(Addr)なメンバへのアクセスは，l_addrを
 * 加算してアクセスする */
struct link_map {
	ElfW(Addr) l_addr;	 /* 共有ライブラリをロードしたベースアドレス */
	char *l_name;		 /* 共有ライブラリの絶対パス名  */
	ElfW(Dyn) *l_ld;	 /* 共有ライブラリのDYNAMICセグメント */
	struct link_map *l_next, *l_prev;    /* Chain of loaded objects.  */

	//struct libname_list *l_libname;	     /* sonameのリストらしい． */

	const ElfW(Phdr) *l_phdr; /* プログラムヘッダテーブルへのポインタ  */
	ElfW(Addr) l_entry;	  /* エントリポイント（Ehdr->e_entry）  */
	ElfW(Half) l_phnum;	  /* プログラムヘッダのエントリ数 */
	ElfW(Half) l_ldnum;	  /* DYNAMICセグメントのエントリ数  */

	/* メモリにマップした開始位置と終了位置．l_addrとは違うことに注意 */
	ElfW(Addr) l_map_start, l_map_end;
	/* 実行可能な領域の終了マップ位置 */
	ElfW(Addr) l_text_end;

	/* DYNAMICセグメントの各要素へのポインタ
	 * 
	 * DT_*という名前をインデックスとして用いている．DT_NUM以下のものであれば
	 * そのままインデックスとして利用可能．それ以外は注意．
	 * 
	 * 0..DT_NUM
	 *     indexed by the processor-independent tags
	 *  ..+DT_THISPROCNUM
	 *     indexed by the tag minus DT_LOPROC
	 *  ..+DT_VERSIONTAGNUM
	 *     DT_VERSIONTAGIDX(tagvalue)
	 *  ..+DT_EXTRANUM
	 *     DT_EXTRATAGIDX(tagvalue)
	 *  ..+DT_VALNUM
	 *     DT_VALTAGIDX(tagvalue)
	 *  ..+DT_ADDRNUM
	 *     DT_ADDRTAGIDX(tagvalue)
	 */
	ElfW(Dyn) *l_info[DT_NUM + DT_THISPROCNUM + DT_VERSIONTAGNUM
			  + DT_EXTRANUM + DT_VALNUM + DT_ADDRNUM];

	struct r_scope_elem l_searchlist;    /* シンボル検索用（使っていないけど） */

	unsigned int l_relocated:1;	/* 再配置したか */
	unsigned int l_contiguous:1;	/* Nonzero if inter-segment holes are
					   mprotected or if no holes are present at
					   all.  */
	unsigned int l_reserved:2;	/* いろいろ使う */

	/* Information used to change permission after the relocations are done.  */
	ElfW(Addr) l_relro_addr;
	size_t l_relro_size;

	/* シンボルハッシュテーブル用  */
	Elf_Symndx l_nbuckets;
	//Elf32_Word l_gnu_bitmask_idxbits;
	//Elf32_Word l_gnu_shift;
	const ElfW(Addr) *l_gnu_bitmask;
	union {
		//const Elf32_Word *l_gnu_buckets;
		const Elf_Symndx *l_chain;
	};
	union {
		//const Elf32_Word *l_gnu_chain_zero;
		const Elf_Symndx *l_buckets;
	};
};

/* link_map構造体のl_infoのための定義．
 * 実装はDT_NUM以下しか使用していないため，出番がない． */
#define L_VERSYMIDX(sym) (DT_NUM + DT_THISPROCNUM	\
			  + DT_VERSIONTAGIDX(sym))
#define L_EXTRAIDX(tag)  (DT_NUM + DT_THISPROCNUM + DT_VERSIONTAGNUM	\
			  + DT_EXTRATAGIDX(dyn->d_tag))
#define L_VALIDX(tag)    (DT_NUM + DT_THISPROCNUM + DT_VERSIONTAGNUM	\
			  + DT_EXTRANUM + DT_VALTAGIDX(tag))
#define L_ADDRIDX(tag)   (DT_NUM + DT_THISPROCNUM + DT_VERSIONTAGNUM	\
			  + DT_EXTRANUM + DT_VALNUM + DT_ADDRTAGIDX(tag))

/* REF: _dl_new_object [dl-object.c]  */
static inline void init_link_map(struct link_map *l)
{
	int i;

	*l = (struct link_map) {
		.l_addr = 0,
		.l_name = NULL,
		.l_ld = NULL,
		.l_next = NULL,
		.l_prev = NULL,
		.l_phdr = NULL,
		.l_entry = 0,
		.l_phnum = 0,
		.l_ldnum = 0,
		.l_relocated = 0,
	};
	l->l_relro_addr = l->l_relro_size = 0;
	l->l_text_end = l->l_map_end = 0;
	l->l_map_start = ~0;
	for (i = 0; i < sizeof(l->l_info) / sizeof(*l->l_info); i++)
		l->l_info[i] = NULL;
	l->l_next = l->l_prev = NULL;
	l->l_reserved = 0;
	l->l_searchlist.r_list = NULL;
	l->l_searchlist.r_nlist = 0;

	l->l_nbuckets = 0;
	l->l_chain = l->l_buckets = NULL;
	//l->l_gnu_bitmask_idxbits = l->l_gnu_shift = 0;
	//l->l_gnu_buckets = l->l_gnu_chain_zero = NULL;
}

#endif
