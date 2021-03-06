/* asmhead.nas */
struct BOOTINFO { /* 0x0ff0-0x0fff */
	char cyls; /* �u�[�g�Z�N�^�͂ǂ��܂Ńf�B�X�N��ǂ񂾂̂� */
	char leds; /* �u�[�g���̃L�[�{�[�h��LED�̏�� */
	char vmode; /* �r�f�I���[�h  ���r�b�g�J���[�� */
	char reserve;
	short scrnx, scrny; /* ��ʉ𑜓x */
	char *vram;
};
#define ADR_BOOTINFO	0x00000ff0
#define ADR_DISKIMG	0x00100000

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0(void);
void store_cr0(int cr0);
void load_tr(int tr);
void asm_inthandler0c(void);
void asm_inthandler0d(void);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void asm_hrb_api(void);
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
void asm_end_app(void);

/* fifo.c */
struct FIFO32 {
	int *buf;
	int p, q, size, free, flags;
	struct TASK *task;
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize);
int read_picture(int *fat, short *vram, int x, int y); /* 锟斤拷锟斤！ */
#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15

/* dsctbl.c */
struct SEGMENT_DESCRIPTOR {
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};
struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_LDT			0x0082
#define AR_TSS32		0x0089
#define AR_INTGATE32	0x008e

/* int.c */
void init_pic(void);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

/* keyboard.c */
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);
#define PORT_KEYDAT		0x0060
#define PORT_KEYCMD		0x0064

/* mouse.c */
struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};
void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

/* memory.c */
#define MEMMAN_FREES		4090	/* ����Ŗ�32KB */
#define MEMMAN_ADDR			0x003c0000	/* Memory Manager�̃������Ԓn*/
struct FREEINFO {	/* ������� */
	unsigned int addr, size;
};
struct MEMMAN {		/* �������Ǘ� */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);
int memman_free_fdata(struct MEMMAN *memman, unsigned int fdata_addr);

/* sheet.c */
#define MAX_SHEETS		256
struct SHEET {
	unsigned char *buf;
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;
	struct SHTCTL *ctl;
	struct TASK *task;
};
struct SHTCTL {
	unsigned char *vram, *map;
	int xsize, ysize, top;
	struct SHEET *sheets[MAX_SHEETS];
	struct SHEET sheets0[MAX_SHEETS];
};
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

/* timer.c */
#define MAX_TIMER	500
struct TIMER {
	struct TIMER *next;
	unsigned int timeout;
	char flags, flags2;
	struct FIFO32 *fifo;
	int data;
};
struct TIMERCTL {
	unsigned int count, next;
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);
int timer_cancel(struct TIMER *timer);
void timer_cancelall(struct FIFO32 *fifo);

/* mtask.c */
#define MAX_TASKS		1000	/* �ő�^�X�N�� */
#define TASK_GDT0		3		/* TSS��GDT�̉��Ԃ��犄�蓖�Ă�̂� */
#define MAX_TASKS_LV	100
#define MAX_TASKLEVELS	10
struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};
struct TASK {
	int sel, flags; /* sel��GDT�̔ԍ��̂��� */
	int level, priority;
	struct FIFO32 fifo;
	struct TSS32 tss;
	struct SEGMENT_DESCRIPTOR ldt[2];
	struct CONSOLE *cons;
	int ds_base, cons_stack;
	struct FILEHANDLE *fhandle;
	int *fat;
	char *cmdline;
	unsigned char langmode, langbyte1;
};
struct TASKLEVEL {
	int running; /* ���삵�Ă���^�X�N�̐� */
	int now; /* ���ݓ��삵�Ă���^�X�N���ǂꂾ��������悤�ɂ��邽�߂̕ϐ� */
	struct TASK *tasks[MAX_TASKS_LV];
};
struct TASKCTL {
	int now_lv; /* ���ݓ��쒆�̃��x�� */
	char lv_change; /* ����^�X�N�X�C�b�`�̂Ƃ��ɁA���x�����ς����ق����������ǂ��� */
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK tasks0[MAX_TASKS];
};
extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;
struct TASK *task_now(void);
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);

/* window.c */
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void change_wtitle8(struct SHEET *sht, char act);
void make_menu(struct SHEET *sht, int n);//制作菜单图层

/***** file.c *****/
#define ROOT_DIR_ADDR	0x00400000
#define ROOT_DATA_ADDR	0x00500000
#define LAST_DATA_ADDR	0x00600000	// �f�[�^�Ǘ��̈�̏I���+1
#define MAX_FINFO_NUM 50	// �t�@�C�����̍ő�o�^��
#define MAX_NAME_LENGTH 8	// �f�B���N�g����/�t�@�C�����̍ő啶����

/* finfo->fdata->head.char�p�̃r�b�g�z��
 * 0x08, 0x10, 0x20, 0x40, 0x80�͖��g�p  */
#define STAT_ALL	0xFF	/* �r������p */
#define STAT_VALID	0x01	/* valid bit */
#define STAT_CONT	0x02	/* continuous bit */
#define STAT_BUF	0x04	/* buffer bit:�o�b�t�@�̈�ɂ���t�@�C���f�[�^�ł��邱�Ƃ����� */
#define STAT_OPENED	0x08	/* file open����Ă��邩�ǂ���������(�f�t�H���g��0) */

/*�@finfo->type�p�̃r�b�g�z��(����ł�, �f�B���N�g���ƃt�@�C���̂Q��ނ�����ʂ��Ă��Ȃ�)
 * 0x10: �f�B���N�g�����
 * 0x20: �t�@�C�����
 * 0x40: �V�X�e���t�@�C��
 * 0x80: ����ȊO�̏��(�f�B�X�N�̖��O�Ƃ�)*/
#define FTYPE_DIR	0x10
#define FTYPE_FILE	0x20
#define FTYPE_SYS	0x40
#define FTYPE_OTHER	0x80

/* �u���b�N�Ɋւ����` */
#define BLOCK_SIZE 1024	// �t�@�C���f�[�^�̃T�C�Y(default��4,096)
#define BODY_SIZE (BLOCK_SIZE - sizeof(struct HEAD))	//1�u���b�N������̎��f�[�^�̃T�C�Y
#define BODY_SIZE_OFFSET 128	// �]���Ɋm�ۂ���T�C�Y(�����Ă��悢�H)
#define MAX_BLOCK_NUM 50 // �ő�u���b�N��(����ȏ�̃u���b�N�̊m�ۂ͋����Ȃ�)
struct FILEINFO {
	unsigned char name[MAX_NAME_LENGTH], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};

/* my original file information */
struct MYFILEINFO{
	unsigned char name[MAX_NAME_LENGTH], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
	struct MYDIRINFO *child_dir;
	struct MYFILEDATA *fdata;
};

/* my original directory information */
struct MYDIRINFO {
	struct MYFILEINFO finfo[MAX_FINFO_NUM];
	unsigned char name[MAX_NAME_LENGTH];
	struct MYDIRINFO *parent_dir;
	struct MYDIRINFO *this_dir; // ����͕ʂɂȂ��Ă���薳���͂�
};

/* my original file data */
struct HEAD{
	unsigned char stat;
	char name[12];
	struct MYFILEDATA *this_fdata;	// �f�[�^�̈�̃A�h���X
	struct MYDIRINFO *this_dir;		// �ܗL���Ă���f�B���N�g���̃A�h���X
	struct MYFILEDATA *next_fdata;	// ���̃t�@�C���f�[�^(�f�[�^����𒴂��Ă����ꍇ�g�p����)
	// next_data�̃f�t�H���g�̒l��0x0000 0000(�G���hflag bit�������Ă��鎞�Ɠ���)
	/* stat is a bit arguments shown below
	 * valid bit: 0x01
	 * continuous bit: 0x02�@(����������t�@�C���B�����̃t�@�C���f�[�^����\������Ă���)
	 * end file: 0x04 (����Ȃ��H)
	 * opened file 0x08
	 */
};
struct MYFILEDATA{
	struct HEAD head;
	char body[BODY_SIZE];	// ���v�T�C�Y��1024byte�ɂȂ�悤�ɂ���
};

void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
char *file_loadfile2(int clustno, int *psize, int *fat);
struct MYFILEINFO *myfinfo_search(char *name, struct MYDIRINFO *dinfo, int max);
// �t�@�C�����Ǘ��̈悩��A�g���Ă��Ȃ��f�B���N�g����Ԃ�T���A�����������̂��A���̏ꏊ��Ԃ�
struct MYDIRINFO *get_newdinfo();
// �f�[�^�Ǘ��̈�̎g���Ă��Ȃ���Ԃ�T���A���̏ꏊ��������
struct MYFILEDATA *get_newfdata(struct MYFILEDATA *fdata);
// �t�@�C�����f�[�^�Ǘ��̈悩��R�s�[���āA�R�s�[��̔Ԓn��������
struct MYFILEDATA *myfopen(char *filename, struct MYDIRINFO *dinfo);
// �f�[�^�Ǘ��̈�̊Y���t�@�C�����I�[�v������Ă�����A��������������Astatus bit��opened������������B
int myfclose(struct MYFILEDATA *opened_fdata);
// �f�[�^�Ǘ��̈�̊Y���t�@�C�����Z�[�u�\�Ȃ�΁Afdata->body�̓��e��ۑ�����
int myfsave(struct MYFILEDATA *opened_fdata);
int myfwrite(struct MYFILEDATA *fdata, char *str);
int myfread(char *str, struct MYFILEDATA *fdata);
int myfcopy(struct MYFILEDATA *fdata1, struct MYFILEDATA *fdata2);
unsigned int get_size_myfdata(struct MYFILEDATA *fdata);
unsigned int get_size_str(char *str);
unsigned int get_blocknum_myfdata(struct MYFILEDATA *fdata);
unsigned int add_status_myfdata(struct MYFILEDATA *fdata, unsigned char stat);


/* console.c */
#define MAX_CMDLINE	50	// �R�}���h���C���̓��͕�������

struct CONSOLE {
	struct SHEET *sht;
	int cur_x, cur_y, cur_c;
	struct TIMER *timer;
	struct MYDIRINFO *current_dir;
	unsigned int id;
};
struct FILEHANDLE {
	char *buf;
	int size;
	int pos;
};

void debug_print(char *str);
void console_task(struct SHEET *sheet, int memtotal);
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_newline(struct CONSOLE *cons);
void cons_putstr(struct CONSOLE *cons, char *s);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
int cons_putdir(struct CONSOLE *cons);
void get_pathname(char *pathname, struct MYDIRINFO *dinfo);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal);
void cmd_cd(struct CONSOLE *cons, char *cmdline);
struct MYDIRINFO *parse_cdline(struct CONSOLE *cons, char *cdline);
void cd_error(struct CONSOLE *cons, char *message);
void cmd_edit(struct CONSOLE *cons, char *cmdline);
void cmd_mkdir(struct CONSOLE *cons, char *cmdline);
void cmd_mkfile(struct CONSOLE *cons, char *cmdline);
void cmd_show(struct CONSOLE *cons, char *cmdline);
void cmd_fview(struct CONSOLE *cons, char *cmdline);
void cmd_setlog(struct CONSOLE *cons);
void cmd_mem(struct CONSOLE *cons, int memtotal);
void cmd_memmap(struct CONSOLE *cons, int memtotal);
void cmd_log(struct CONSOLE *cons);
void cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline);
void cmd_cls(struct CONSOLE *cons);
void cmd_logcls(struct CONSOLE *cons);
void cmd_test(struct CONSOLE *cons);
void cmd_mkfs(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_fddir(struct CONSOLE *cons);
void cmd_exit(struct CONSOLE *cons, int *fat);
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_langmode(struct CONSOLE *cons, char *cmdline);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
int *inthandler0d(int *esp);
int *inthandler0c(int *esp);
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col);

/* tek.c */
int tek_getsize(unsigned char *p);
int tek_decomp(unsigned char *p, char *q, int size);

/* bootpack.c */
struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);
struct SHEET *open_log(struct SHTCTL *shtctl, unsigned int memtotal);


/*cmos.c*/
#define cmos_index 0x70
#define cmos_data 0x71
#define CMOS_CUR_SEC	0x0
#define CMOS_ALA_SEC	0x1
#define CMOS_CUR_MIN	0x2
#define CMOS_ALA_MIN	0x3
#define CMOS_CUR_HOUR	0x4
#define CMOS_ALA_HOUR	0x5
#define CMOS_WEEK_DAY	0x6
#define CMOS_MON_DAY	0x7
#define CMOS_CUR_MON	0x8
#define CMOS_CUR_YEAR	0x9
#define CMOS_DEV_TYPE	0x12
#define CMOS_CUR_CEN	0x32
#define BCD_HEX(n)	((n >> 4) * 10) + (n & 0xf)

#define BCD_ASCII_first(n)	(((n<<4)>>4)+0x30)
#define BCD_ASCII_S(n)	((n<<4)+0x30)

unsigned int get_hour_hex();
unsigned int get_min_hex();
unsigned int get_sec_hex();
unsigned int get_day_of_month();
unsigned int get_day_of_week();
unsigned int get_mon_hex();
unsigned int get_year();

/*jpeg.c*/
struct DLL_STRPICENV{
	int work[64 * 1024 / 4];
};
struct RGB{
	unsigned char b,g,r,t;
};
int info_JPEG(struct DLL_STRPICENV*env,int *info,int size,unsigned char *fp);
int decode0_JPEG(struct DLL_STRPICENV*env,int size,unsigned char *fp,int b_type,unsigned char *buf,int skip);

//svar.c
//鐢ㄦ埛鎬佺殑绔炰簤鏉′欢
#define VAR_MAX_NUM 100 //瑙勫畾鎿嶄綔绯荤粺鏈€澶氬彧鑳借?剧疆100涓?鍏变韩鍙橀噺
struct SVAR{//鍏变韩鍙橀噺鐨勫悕瀛楋紝鏍囧織浣嶅拰鍐呭?癸紱
	char *name;
	int flag;//涓?0琛ㄧず璇ュ彉閲忓瓨鍦?锛屼负1琛ㄧず璇ュ彉閲忎笉瀛樺湪銆?
	int length;//琛ㄧず鍏变韩鍙橀噺鐨勫唴瀹归暱搴?
	int *content;
};
struct SVARCTL{
	struct SVAR var[VAR_MAX_NUM];
};
void init_Svar(struct MEMMAN *memman);
int var_create(char *name,int length);
int var_read(char *name,int n);
int var_wrt(char *name,int n,int value);
int var_free(char *name);
void avoid_sleep();
int TestAndSet(int *t);
void Tlock();
void unTlock();