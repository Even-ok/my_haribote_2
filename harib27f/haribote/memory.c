/* メモリ関係 */

#include "bootpack.h"
#include <stdio.h>
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

/**
 * return area of available memory
 */
unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 386か、486以降なのかの確認 */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* 386ではAC=1にしても自動で0に戻ってしまう */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* キャッシュ禁止 */
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* キャッシュ許可 */
		store_cr0(cr0);
	}

	return i;
}

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/* あき情報の個数 */
	man->maxfrees = 0;		/* 状況観察用：freesの最大値 */
	man->lostsize = 0;		/* 解放に失敗した合計サイズ */
	man->losts = 0;			/* 解放に失敗した回数 */
	return;
}

/* あきサイズの合計を報告 */
unsigned int memman_total(struct MEMMAN *man)
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

/* メモリを確保する関数
 * @param man: メモリマネージャ
 * @param size: データサイズ
 * return 確保したメモリ領域の先頭番地 */
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			/* 十分な広さのあきを発見 */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* free[i]がなくなったので前へつめる */
				man->frees--;
				/* 代入したi以降の値に対してfor文を行う */
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; /* 構造体の代入 */
				}
			}
			return a;
		}
	}
	return 0; /* あきがない */
}

/* メモリ解放関数
 * return 0: 成功終了
 * return -1: 失敗終了*/
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i, j;
	/* まとめやすさを考えると、free[]がaddr順に並んでいるほうがいい */
	/* だからまず、どこに入れるべきかを決める */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* 前がある */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* 前のあき領域にまとめられる */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* 後ろもある */
				if (addr + size == man->free[i].addr) {
					/* なんと後ろともまとめられる */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]の削除 */
					/* free[i]がなくなったので前へつめる */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* 構造体の代入 */
					}
				}
			}
			return 0; /* 成功終了 */
		}
	}
	/* 前とはまとめられなかった */
	if (i < man->frees) {
		/* 後ろがある */
		if (addr + size == man->free[i].addr) {
			/* 後ろとはまとめられる */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* 成功終了 */
		}
	}
	/* 前にも後ろにもまとめられない */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]より後ろを、後ろへずらして、すきまを作る */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* 最大値を更新 */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* 成功終了 */
	}
	/* 後ろにずらせなかった */
	man->losts++;
	man->lostsize += size;
	return -1; /* 失敗終了 */
}

/* メモリを確保する関数(wrapper)
 * @param man: メモリマネージャ
 * @param size: 確保するデータサイズ[4,096 byte]
 * return 確保したメモリ領域の番地アドレス
 * */
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;	// 端数繰り上げ
	a = memman_alloc(man, size);
	return a;
}

/* メモリ解放関数
 * return 0: 成功終了
 * return -1: 失敗終了 */
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}


/* メモリの使用状況を文字列で返す */
unsigned int memman_show(struct CONSOLE *cons, struct MEMMAN *man){
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

/* 与えられたファイルデータから先のファイルデータをメモリ解放する
 * @param man: memory manager
 * @param middle_fdata: 初期化&メモリ解放したい中間ファイルデータ
 * return 1 if it succeeded.
 * return 0 if it failed.
 * */
int memman_free_fdata(struct MEMMAN *memman, unsigned int fdata_addr){
	struct MYFILEDATA *temp_fdata, *prev_temp_fdata;
	int i;
	char s[50];
	unsigned int fdata_size;

	fdata_size = sizeof(struct MYFILEDATA);
	temp_fdata = (struct MYFILEDATA *)fdata_addr;

	do{ /* 次のファイルデータが存在している間 、メモリ解放処理を行う*/
		prev_temp_fdata = temp_fdata;
		i = memman_free(memman, (unsigned int)temp_fdata, fdata_size);
		if(i == -1){
			sprintf(s, "return memman_free() was failed.\n");
			debug_print(s);
			return 0;
		}else{
			sprintf(s, "%d length was released from the addr 0x%08x\n", fdata_size, temp_fdata);
			debug_print(s);
		}
		temp_fdata = temp_fdata->head.next_fdata;
	}while(prev_temp_fdata->head.next_fdata != 0);

	return 1;
}
