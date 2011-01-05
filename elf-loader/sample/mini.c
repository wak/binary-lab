#include <sys/syscall.h>

#define MESSAGE "hello, world!\n"

extern long int syscall(long int sysno, ...);
extern void print(const char *);
extern void print_ulong(unsigned long n);
extern void do_count(void);
extern unsigned long g_count;

void _start(void)
{
	/* CALL syscall@PLT (syscall.so)
	 *   説明:
	 *     PLTとGOTを経由して，rtld.soのruntime_resolve()へ制御が移る．そこで，
	 *     miniのGOTの再配置が行われる．
	 *   目的:
	 *     - rtld.soによるGOTの初期化が正しく行われたか
	 *     - rtld.soによるGOTの（遅延）再配置がうまく動作するか
         *      （次にminiがsyscallを呼ぶまで分からない）
	 */
	syscall(SYS_write, 1, MESSAGE, sizeof(MESSAGE)-1);

	/* CALL print@PLT (print.so)
	 *  説明:
	 *    print.soもまた内部にPLTとGOTを持っている．そして，print()は内部で
	 *    syscall()を呼ぶため，print.soのGOTの再配置がおきる
	 *  目的:
	 *    - 共有ライブラリ内のGOTの初期化が正しく行われたか
	 *    - 共有ライブラリ内のGOTの（遅延）再配置がうまく動作するか
         *      （次にprint.soがsyscallを呼ぶまで分からない）
	 */
	print("hello, world by print()\n");

	/* CALL do_count@PLT (print.so)
	 *   説明:
	 *     do_count()はglobal.soのg_countグローバル変数の値を変更する．
	 *     do_count()は内部でsyscall()関数を呼ぶ．このsyscall()呼び出しは，す
	 *     でにGOTが再配置された後であるため，runtime_resolve()を経由せずに呼
	 *     ばれる．
	 *   目的:
	 *     - 共有ライブラリprint.soからグローバル変数を正しく扱えるか
	 *     - print.soのGOT（syscall用）が正しく再配置を終えたかどうか
	 */
	do_count();

	/* miniでグローバル変数を変更してみる
	 *   目的:
	 *     - mini（実行形式プログラム）がグローバル変数を正しく扱えるか
	 */
	g_count = 0;

	/* miniが変更したグローバル変数をprint.soの関数で表示する
	 *   説明:
	 *     0が表示されれば，miniとprint.soが同じアドレスを参照することがわかる．
	 */
	do_count();

	/* もう一回表示してみる
	 *   説明:
	 *     とくに意味はない．1が表示されればOK．
	 */
	print_ulong(g_count);

	/* CALL syscall@PLT (syscall.so)
	 *   説明:
	 *     miniでのsyscall@PLTのための再配置はすでに終えたため，再配置は発生
	 *     しない．
	 */
	syscall(SYS_exit, 0);
}
