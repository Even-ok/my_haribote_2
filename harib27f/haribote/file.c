/* ファイル関係 */

#include "bootpack.h"
#include <stdio.h>
#include <string.h>

/**
 * ディスクイメージ内のFATの圧縮をとく
 */
void file_readfat(int *fat, unsigned char *img)
{
	int i, j = 0;
	for (i = 0; i < 2880; i += 2) {
		fat[i + 0] = (img[j + 0]      | img[j + 1] << 8) & 0xfff;
		fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
		j += 3;
	}
	return;
}

/**
 * ファイルを読み込む
 */
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img)
{
	int i;
	for (;;) {
		if (size <= 512) {
			for (i = 0; i < size; i++) {
				buf[i] = img[clustno * 512 + i];
			}
			break;
		}
		for (i = 0; i < 512; i++) {
			buf[i] = img[clustno * 512 + i];
		}
		size -= 512;
		buf += 512;
		clustno = fat[clustno];
	}
	return;
}

/**
 * search file in fileinfo using given name
 */
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max)
{
	int i, j;
	char s[12];
	for (j = 0; j < 11; j++) {
		s[j] = ' ';
	}
	j = 0;
	for (i = 0; name[i] != 0; i++) {
		if (j >= 11) { return 0; /* 見つからなかった */ }
		if (name[i] == '.' && j <= 8) {
			j = 8;
		} else {
			s[j] = name[i];
			if ('a' <= s[j] && s[j] <= 'z') {
				/* 小文字は大文字に直す */
				s[j] -= 0x20;
			}
			j++;
		}
	}
	for (i = 0; i < max; ) {
		if (finfo->name[0] == 0x00) {
			break;
		}
		if ((finfo[i].type & 0x18) == 0) {
			for (j = 0; j < 11; j++) {
				if (finfo[i].name[j] != s[j]) {
					goto next;
				}
			}
			return finfo + i; /* ファイルが見つかった */
		}
		next:
		i++;
	}
	return 0; /* 見つからなかった */
}

/**
 * search my file in my filesystem using name.
 * @name: Ex. "hoge.txt", "foo.hrb"
 * return dinfo addr if it succeeded.
 * return 0 if it failed.
 */
struct MYFILEINFO *myfinfo_search(char *name, struct MYDIRINFO *dinfo, int max)
{
	int i, j;
	char s[12];
	for (j = 0; j < 11; j++) {
		s[j] = ' ';
	}
	j = 0;
	for (i = 0; name[i] != 0; i++) {
		if (j >= 11) {
			debug_print("file was not found in myfinfo_search(): int j is too high.\n");
			return 0; /* 見つからなかった */
		}
		if (name[i] == '.' && j <= 8) {
			j = 8;
		} else {
			s[j] = name[i];
			if ('a' <= s[j] && s[j] <= 'z') {
				/* 小文字は大文字に直す */
				s[j] -= 'a'-'A';
			}
			j++;
		}
	}

	for (i = 0; i < max; ) {
		// ファイル名が無い場合、これ以上先にファイルがないので処理を終了させる。
		if (dinfo->finfo[i].name[0] == 0x00) {
			break;
		}

		/* ファイル名がある場合, finfoのfiletypeの識別をする */
		/* finfoがファイルの場合(今は２種類しかない) */
		if (dinfo->finfo[i].type == 0x20) {
			for (j = 0; j < 11; j++) {
				if (dinfo->finfo[i].name[j] != s[j]) {
					goto next;
				}
			}
			//debug_print("file was found!\n");
			return dinfo->finfo + i; /* ファイルが見つかった */

			/* finfoがディレクトリの場合 */
		}else if(dinfo->finfo[i].type == 0x10){
			// ディレクトリは拡張子がないのでファイル名だけ比較する
			for (j = 0; j < 8; j++) {
				if (dinfo->finfo[i].name[j] != s[j]) {
					goto next;
				}
			}
			//debug_print("directory was found!\n");
			return dinfo->finfo + i; /* ディレクトリが見つかった */
		}
		next:
		i++;
	}

	debug_print("file/directory was not found in myfinfo_search()\n");
	return 0; /* 見つからなかった */
}

/**
 * load file
 */
char *file_loadfile2(int clustno, int *psize, int *fat)
{
	int size = *psize, size2;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char *buf, *buf2;
	buf = (char *) memman_alloc_4k(memman, size);
	file_loadfile(clustno, size, buf, fat, (char *) (ADR_DISKIMG + 0x003e00));
	if (size >= 17) {
		size2 = tek_getsize(buf);
		if (size2 > 0) {	/* tek圧縮がかかっていた */
			buf2 = (char *) memman_alloc_4k(memman, size2);
			tek_decomp(buf, buf2, size2);
			memman_free_4k(memman, (int) buf, size);
			buf = buf2;
			*psize = size2;
		}
	}
	return buf;
}


/**
 * ファイル情報管理領域から、使われていないディレクトリ空間を探し、初期化したのち、その場所を返す
 */
struct MYDIRINFO *get_newdinfo(){
	char s[50];
	struct MYDIRINFO *dinfo = (struct MYDIRINFO *)ROOT_DIR_ADDR;
	struct MYDIRINFO *temp_dinfo;
	struct MYDIRINFO *this_dinfo;
	struct MYDIRINFO *new_dinfo;
	int i = 0, dir_num = -1;

	///* debug code: Viewing behavior of get_newdinfo().
	sprintf(s, "/*** IN FUNCTION get_newdinfo() ***/\n");
	debug_print(s);
	//*/

	// 有効なdinfoを検索/表示する
	for(i=0, temp_dinfo = dinfo; temp_dinfo->this_dir != 0x00000000 ; i++, temp_dinfo++){
		dir_num++;
		this_dinfo = temp_dinfo->this_dir;

		///* debug code: Viewing behavior of get_newdinfo().
		sprintf(s, "dinfo[%d] addr = 0x%08x\n", i, this_dinfo);
		debug_print(s);
		//*/
	}
	///* debug code: Viewing behavior of get_newdinfo().
	sprintf(s, "dinfo[%d]->this_addr = 0x%08x\n", i, temp_dinfo->this_dir);
	debug_print(s);
	this_dinfo = temp_dinfo->this_dir;
	sprintf(s, "max available dir number is %d\n", dir_num);
	debug_print(s);
	sprintf(s, "/*********************************/\n");
	debug_print(s);
	//*/

	/* 新しいディレクトリの初期化 */
	new_dinfo = (dinfo + dir_num + 1);
	sprintf(new_dinfo->name, "");
	new_dinfo->parent_dir = 0;
	new_dinfo->this_dir = 0;

	return new_dinfo;
}

/* ファイルをデータ管理領域からコピーして、コピー先の番地を教える
 * return struct MYFILEDATA: コピー先の番地に格納されているファイルデータ
 * return 0: 失敗
 */
struct MYFILEDATA *myfopen(char *filename, struct MYDIRINFO *dinfo){
	// とりあえずルートディレクトリにあるファイルに対してのみ実行することにする。
	struct MYFILEINFO *finfo = myfinfo_search(filename, dinfo, 224);
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned int mem_addr;
	int i;
	unsigned int *temp_addr;
	int block_count, alloc_size;
	char s[BODY_SIZE + 128];
	if(finfo == 0 || (finfo->type & FTYPE_DIR) != 0){
		/* 該当するファイルがROOTディレクトリに存在しなかった場合
		 * または、該当するファイルがディレクトリであった場合 */
		debug_print("In function myfopen(): file was not found.\n");
		return 0;
	}else{
		/* openのときにはSTAT_OPENEDは確認しない (要検討)*/
		add_status_myfdata(finfo->fdata, STAT_OPENED);
		sprintf(s, "fdata = 0x%08x\n[debug] head.this_fdata = 0x%08x\n[debug] head.this_dir = 0x%08x\n[debug] head.stat = 0x%02x\n",
				finfo->fdata,
				finfo->fdata->head.this_fdata,
				finfo->fdata->head.this_dir,
				finfo->fdata->head.stat);
		debug_print(s);
		sprintf(s, "body = %s[EOF]\n", finfo->fdata->body);
		debug_print(s);

		// 確保するメモリのサイズを計算
		block_count = get_blocknum_myfdata(finfo->fdata);	// 仕様しているブロック数を取得
		alloc_size = block_count * BLOCK_SIZE;		// 全ブロック数の分だけメモリを確保

		/*** debug ***/
		sprintf(s, "alloc_size = 0x%08x\n", alloc_size);
		debug_print(s);
		/*************/

		mem_addr = memman_alloc(memman, alloc_size);

		/* 確保した領域の初期化 */
		temp_addr = (unsigned int *)mem_addr;
		sprintf(s ,"INIT temp_addr = 0x%08x\n", temp_addr);
		for(i = 0; i<alloc_size; i++){
			if(*temp_addr != 0){
				//sprintf(s, "temp_addr = 0x%08x: %d\n", temp_addr, *temp_addr);
				//debug_print(s);
			}
			*temp_addr = 0;
			temp_addr += 1;
		}
		debug_print("initializing memory domain was finished.\n");

		struct MYFILEDATA *opened_fdata = (struct MYFILEDATA *) mem_addr;

		/* 確保したメモリ番地の出力 */
		sprintf(s, "opened fdata addr = 0x%08x\n", opened_fdata);	// 最初はmem_addrと同じ値
		debug_print(s);

		/* メモリ領域のコピー */	// read -> write で実装できる？ -> headの情報が保存されない
		myfcopy(opened_fdata, finfo->fdata);
		add_status_myfdata(opened_fdata, STAT_BUF);	//ステータスビットを追加する

		///* debug: コピーしたメモリ管理領域(ここで色々な作業を行う)
		sprintf(s, "allocated fdata addr = 0x%08x\n", mem_addr);
		debug_print(s);
		//sprintf(s, "allocated fdata length = 0x%08x + 0FFF[byte]\n", alloc_size);
		//debug_print(s);
		//*/
		return opened_fdata;
	}
	return 0;
}

/* データ管理領域の該当ファイルがオープンされていたら、メモリを解放し、status bitのopenedを書き換える。
 * return 0: if success
 * return -1: if failed
 * */
int myfclose(struct MYFILEDATA *opened_fdata){
	// データ領域にある実物のデータをfdataに格納する
	struct MYFILEDATA *fdata =(struct MYFILEDATA *)opened_fdata->head.this_fdata;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	if((fdata->head.stat & STAT_OPENED) == 0){
		/* オープンされていないファイルの場合 */
		debug_print("In function myfclose(): this file data is already closed.\n");
		return -1;	// close失敗
	}else{
		/* オープンされているファイルの場合、実データのステータス変数を変更した後、
		 * 確保していたメモリを解放する */

		/* ステータス変数の変更(open bitを折る) */
		fdata->head.stat &= (STAT_ALL - STAT_OPENED);

		/* 使用していたバッファ領域のメモリ解放 */
		memman_free_fdata(memman, (unsigned int)opened_fdata);

		return 0;	// close成功
	}

	return -1;	// close失敗
}

/* データ管理領域の該当ファイルがセーブ可能ならば、fdata->bodyの内容を保存する
 * return 0: success
 * return -1: failed
 */
int myfsave(struct MYFILEDATA *opened_fdata){
	struct MYFILEDATA *fdata;
	struct MYDIRINFO *dinfo;
	struct MYFILEINFO *finfo;
	char s[1000];	// 1000文字以上のデータの場合どうする？

	/* 元情報の取得 */
	fdata = opened_fdata->head.this_fdata;
	dinfo = fdata->head.this_dir;
	finfo = myfinfo_search(fdata->head.name, dinfo, MAX_FINFO_NUM);

	sprintf(s, "fdata->head.name = %s[EOF]\n", fdata->head.name);
	debug_print(s);
	if(finfo == 0){
		return -1;
	}


	if((fdata->head.stat & STAT_OPENED) == 0){
		/* オープンされていないファイルに対して保存しようとした場合 */
		sprintf(s, "In function myfsave():Can't save because this file data is not opened.\n");
		debug_print(s);
		return -1;	// close失敗
	}else{
		myfread(s, opened_fdata);	// バッファからファイルデータを読み込む
		myfwrite(fdata, s);			// 読み込んだファイルデータを書き込む
		finfo->size = get_size_myfdata(fdata);
		return 0;	// close成功
	}

	return -1;	// close失敗
}

/* データ管理領域の使われていない空間を探し、初期化したのち、その場所を教える
 * return 発見したMYFILEDATAの番地アドレス
 * [CAUTION!]この関数内ではファイルデータ同士のリンクは貼らない
 * Ex. fdata->head.next_fdata = new_fdata;
 */
struct MYFILEDATA *get_newfdata(struct MYFILEDATA *fdata){
	struct MYFILEDATA *root_fdata = (struct MYFILEDATA *) ROOT_DATA_ADDR;
	struct MYFILEDATA *temp_fdata;
	struct MYFILEDATA *new_fdata;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	//char s[100];
	unsigned int mem_addr, alloc_size;
	unsigned int *temp_addr;
	int i=0;

	/*
	sprintf(s, "*** IN FUNCTION get_newfdata() ***\n");
	debug_print(s);
	sprintf(s, "fdata addr = 0x%08x\n", (unsigned int)fdata);
	debug_print(s);
	//*/

	if(fdata->head.stat == STAT_ALL){
		/* 新規ファイル作成(mkfileコマンド)時の処理 */
		//debug_print("get_newfdata() was called by mkfile command.\n");
		goto MKFILE;

	}else if(ROOT_DATA_ADDR <= (unsigned int)fdata && (unsigned int)fdata < LAST_DATA_ADDR ){
		MKFILE:

		/* OPENED bitが閉じている = データ管理領域のけるファイル新規データの取得 */
		/* データ管理領域内のファイルデータに対して、呼び出された場合 */
		/*
		debug_print("Getting new file data in data manage domain.\n");
		sprintf(s, "root_fdata = 0x%08x\n", root_fdata);
		debug_print(s);
		//*/

		temp_fdata = root_fdata;
		while((temp_fdata->head.stat & STAT_VALID) != 0){
			/* データ管理領域を線形的に探索し、空いているデータ領域を探す */
			/* temp_fdataのvalid bitが立っている間 */
			//sprintf(s, "fdata[%d] addr = 0x%08x\n", i, temp_fdata);
			//debug_print(s);
			temp_fdata += 1; // 隣の番地に移動(線形探索なので、検索速度はめちゃくちゃ遅い[要検討])
			i++;
		}
		/* valid bitが0のfile dataが見つかった */
		new_fdata = temp_fdata;
		//sprintf(s, "found invalid fdata[%d] addr = 0x%08x\n", i, new_fdata);
		//debug_print(s);

		/* ファイルデータの初期化 */
		for(i=0; i< BODY_SIZE; i++)new_fdata->body[i] = '\0';
		new_fdata->head.stat = STAT_VALID;	// 初期ステータスはvalidのみ立っている状態
		new_fdata->head.this_fdata = new_fdata;	// 自分の本来の番地を記憶(open時に必要)
		new_fdata->head.this_dir = fdata->head.this_dir;
		new_fdata->head.next_fdata = 0;		// 番兵として使う
	}else{

		/* OPENED bitが立っている = バッファ領域における新規ファイルデータ取得 */
		/* メモリ管理領域にあるファイルの場合、
		 * 新しくメモリ領域を確保し、そこに新しいファイルデータを作成する */
		//debug_print("Getting new file data in buffer domain.\n");

		// 確保するメモリのサイズを計算と確保
		alloc_size = BLOCK_SIZE;	// 確保するサイズを計算
		mem_addr = memman_alloc(memman, alloc_size);

		/* 確保した領域の初期化 */
		temp_addr = (unsigned int *)mem_addr;
		for(i = 0; i<alloc_size; i++){
			if(*temp_addr != 0){
				//sprintf(s, "temp_addr = 0x%08x: %d\n", temp_addr, *temp_addr);
				//debug_print(s);
			}
			*temp_addr = 0;
			temp_addr += 1;
		}
		//debug_print("initlizing memory domain was finished.\n");

		new_fdata = (struct MYFILEDATA *) mem_addr;

		/* debug: コピーしたメモリ管理領域(ここで色々な作業を行う) */
		/* 確保したメモリ番地の出力 */
		//sprintf(s, "new fdata addr = 0x%08x\n", new_fdata);	// 最初はmem_addrと同じ値
		//debug_print(s);
		//*/

		/* ファイルデータの初期化 */
		for(i=0; i< BODY_SIZE; i++)new_fdata->body[i] = '\0';
		new_fdata->head.stat = STAT_VALID | STAT_OPENED | STAT_BUF; // 初期ステータスはvalid, opened, bufが立っている状態
		new_fdata->head.this_fdata = new_fdata;	// 自分の本来の番地を記憶(open時に必要)
		new_fdata->head.this_dir = fdata->head.this_dir;
		new_fdata->head.next_fdata = 0;		// 番兵として使う
	}

	//sprintf(s, "/********************************/\n");
	//debug_print(s);
	return new_fdata;	/* 取得したファイルデータを返す */
}

/**
 * interface to write data into file
 * @param fdata: string data to be written
 * @param str: string data to write
 * return 1 if it succeeded
 * return 0 if it failed
 */
int myfwrite(struct MYFILEDATA *fdata, char *str){
	char s[BODY_SIZE + BODY_SIZE_OFFSET];
	int i,j;
	unsigned int block_num, prev_block_num;
	unsigned int file_size;
	struct MYFILEDATA *debug_fdata;
	struct MYFILEDATA *temp_fdata;
	struct MYFILEDATA *new_fdata;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	/* 初期化処理 */
	i = j = 0;
	block_num = 1;
	prev_block_num = get_blocknum_myfdata(fdata);

	///* debugging
	sprintf(s, "myfwrite() has been called.\n");
	debug_print(s);
	//sprintf(s, "str = %s[EOF]\n", str);
	//debug_print(s);
	//*/

	while(str[i] != '\0'){
		fdata->body[j] = str[i];	// ファイルデータに一文字書き込む
		i++;
		j++;

		if(i == (BODY_SIZE-1) * block_num){	// Ex. (108 * 1)-1 = 107
			/* ブロックサイズの上限に到達した場合 */
			fdata->body[j] = '\0';	// fdata->body[107]の最後にヌル文字('\0')を入力する。
			debug_fdata = fdata;
			if(fdata->head.next_fdata == 0){
				/* ブロック数が足りないときは新しいファイルデータを作り、拡張する */
				new_fdata = get_newfdata(fdata);
				fdata->head.next_fdata = new_fdata;	// 次のデータの番地を格納
				fdata->head.stat |= STAT_CONT;		// ステータスビットのCONTを立てる
			}
			fdata = fdata->head.next_fdata;	// 次のデータに進む
			block_num++;
			sprintf(s, "block moved: 0x%08x -> 0x%08x\n", debug_fdata, fdata);
			debug_print(s);
			j=0;
		}
	}
	fdata->body[j] = str[i];	// fdata->bodyの最後にヌル文字('\0')を入力する。

	if(block_num < prev_block_num){
		/* 元のブロックサイズより、書きこんだブロックサイズが小さい場合、
		 * 使われなくなったブロックを初期化 or 解放する。 */
		debug_print("***IN BLOCK DELETE FUNCTION***\n");

		temp_fdata = fdata->head.next_fdata;	//次のファイルデータを保存。
		/* 末端のファイルデータを修正 */
		fdata->head.stat &= (STAT_ALL - STAT_CONT);	//STAT_CONTビットを折る
		fdata->head.next_fdata = 0;	//次のファイルデータ番地を上書き

		if((fdata->head.stat & STAT_BUF) != 0){	//条件式は要検討！->ステータス変数を作ればもっと簡単にできるはず
			/* バッファ領域にあるファイルデータの場合、続くファイルデータのメモリを解放 */
			debug_print("[Free fdata mode]\n");
			memman_free_fdata(memman, (unsigned int)temp_fdata);
		}else{
			/* データ領域のファイルデータの場合、続くファイルデータを初期化 */
			debug_print("[Init fdata mode]\n");
			while(temp_fdata->head.next_fdata != 0){
				temp_fdata->head.stat = 0;	// 全てのステータスビットを折る
				temp_fdata = temp_fdata->head.next_fdata;
			}
		}
		debug_print("******************************\n");
	}

	file_size = get_size_myfdata(fdata);
	sprintf(s, "In current block, %d/%d is used.\n", file_size, BODY_SIZE);
	debug_print(s);
	return 1;
}

/**
 * interface to read data from file
 * @param str: string data to read
 * @param body: string data to be read
 * return 1: succeeded
 * return 0: failed
 */
int myfread(char *str, struct MYFILEDATA *fdata){
	struct MYFILEDATA *prev_fdata;
	char s[BODY_SIZE + BODY_SIZE_OFFSET];
	int i=0;
	sprintf(s, "myfread() has been called.\n");
	debug_print(s);
	sprintf(str, "");	// strの初期化

	do{ /* 次のファイルデータが存在する間,ファイルデータを読み込み続ける */
		prev_fdata = fdata;
		sprintf(s, "fdata->body[%d] = %s[EOF]\n", i, fdata->body);
		debug_print(s);
		strcat(str, fdata->body);
		fdata = fdata->head.next_fdata;
		i++;
	}while(prev_fdata->head.next_fdata != 0);

	sprintf(s, "read data = %s[EOF]\n", str);
	debug_print(s);
	return 1;
}

/**
 * interface to copy data
 * @param fdata1: file data to be copied
 * @param fdata2: file data to copy
 * return 1: succeeded
 * return 0: failed
 */
int myfcopy(struct MYFILEDATA *fdata1, struct MYFILEDATA *fdata2){
	char s[50];
	int i=0;
	sprintf(s, "myfcopy() has been called.\n");
	debug_print(s);

	for(;;){
		memcpy(fdata1, fdata2, sizeof(struct MYFILEDATA));
		sprintf(s, "fdata->body[%d] was copied.\n", i);
		debug_print(s);
		i++;

		if(fdata2->head.next_fdata == 0){
			break;
		}else{
			/* バッファでしか使われないという前提(要検討) */
			fdata2 = fdata2->head.next_fdata;

			fdata1->head.next_fdata = fdata1 + 1;
			fdata1++;

			//if(fdata1->head.next_fdata == 0){
			//	/* コピーされる側のブロック数が限界を迎えた場合 */
			//	fdata1->head.next_fdata = get_newfdata(fdata1);
			//}
		}
	}

	return 1;
}




/**
 * calculate size of file data, and return a result[byte].
 * @param fdata: file data
 */
unsigned int get_size_myfdata(struct MYFILEDATA *fdata){
	char s[BODY_SIZE + BODY_SIZE_OFFSET];
	unsigned int block_count = 0;
	unsigned int filesize;
	int rest_size = 0;

	/* 連なっているブロックの数を数える */
	while((fdata->head.stat & STAT_CONT) != 0){
		/* ファイルデータに続きがある場合 */
		if(fdata->body[BODY_SIZE-2] != '\0'){
			/* ブロックの最後尾から2番目に文字があるので(1番目はヌル文字), ブロックは満杯と判断 */
			fdata = fdata->head.next_fdata;
			block_count++;
		}else{
			/* ブロックの最後尾に文字がないので, このブロックにEOFがあると判断 */
			break; // 空ファイルだったらカウントしない(バッファ計算用)
		}
	}
	rest_size = get_size_str(fdata->body); // 最後のブロックの文字列サイズを取得

	sprintf(s, "fdata->body = %s\n", fdata->body);
	debug_print(s);
	sprintf(s , "p = %d\n", rest_size);
	debug_print(s);

	filesize = (BODY_SIZE * block_count) + rest_size;
	return filesize;	// (ブロックサイズ×ブロック数) + 余り [byte]
}

/**
 * calculate size of char*, and return a result[byte].
 * @param str: char *
 * return data size of "char *str"
 */
unsigned int get_size_str(char *str){
	int p;
	p=0;
	while(str[p] != '\0') p++;
	return p;	// 単位はバイト
}

/**
 * calculate block number, and return the number.
 * @param fdata
 * return block number
 */
unsigned int get_blocknum_myfdata(struct MYFILEDATA *fdata){
	unsigned int block_num, data_size;
	char s[50];	// for debugging
	data_size = get_size_myfdata(fdata);
	block_num = (data_size / BODY_SIZE) + 1;	// ブロックの数を計算
	sprintf(s, "used block number = %d\n", block_num);
	debug_print(s);
	return block_num;
}

/**
 * ファイルデータにステータスビットを追加する。
 * 追加するステータスビットが既に立っていた場合は失敗する。
 * @param fdata: 追加されるファイルデータ
 * @param stat: 追加したいステータスビット
 * return 1 if it succeeded.
 * return 0 if it failed.
 */
unsigned int add_status_myfdata(struct MYFILEDATA *fdata, unsigned char stat){
	struct MYFILEDATA *temp_fdata, *prev_temp_fdata;
	char s[100];
	unsigned char debug_char;
	temp_fdata = fdata;
	do{
		prev_temp_fdata = temp_fdata;
		if((temp_fdata->head.stat & stat) == stat){
			/* 追加するステータスビットが既に立っていた場合, 失敗*/
			debug_print("***adding status bit is already valid.***\n");
			return 0;
		}
		debug_char = temp_fdata->head.stat;
		temp_fdata->head.stat |= stat;

		///* debug: output status bits */
		sprintf(s, "head.stat is changed: 0x%02x -> 0x%02x\n", debug_char, temp_fdata->head.stat);
		debug_print(s);
		//*/
		temp_fdata = temp_fdata->head.next_fdata;
	}while(prev_temp_fdata->head.next_fdata != 0);

	debug_print("status bit was added correctly.\n");
	return 1;
}
