#ifndef BOOTPARAMS_H
#define	BOOTPARAMS_H

typedef struct {
	char flags[12];
	unsigned int val;
} ParamItem;

typedef struct {
	ParamItem start;
	//ParamItem cpu_clk;
	ParamItem boot_delay;
	ParamItem serial_sel;
	ParamItem AppRun_addr;
	ParamItem serial_baud;
	ParamItem machine;
	ParamItem run_addr;
	ParamItem root_sel;
	ParamItem tty_sel;
	ParamItem display_sel;
	ParamItem display_mode;
	ParamItem initrd_addr;
	ParamItem initrd_len;
	ParamItem mem_cfg;
	//ParamItem devfs_sel;
	//ParamItem osstor;
	ParamItem user_params;
	char string[128];
	unsigned int bpage[50];
} BootParams;




#define	DEFAULT_USER_PARAMS	" "

#ifdef GLOBAL_PARAMS

//小于等于512个字节,最多保存24个ITEM和128字节用户定义的字符串
BootParams boot_params = {
	{"auto-run", 3},	//0=boot without parameters,1=boot with parameters
	//{"cpuclk",   2},	//0=200M, 1=300M, 2=400M, 3=440M
	{"rundelay", 0},	//0 seconds
	{"serial",   0},	//0=serial port 0, 1=serial port 1
	{"AppRunAddr",   0x32000000},
	{"baudrate", 115200},
	{"machine",  193},
	{"runAddr",  0x30201000},
	{"rootfs",   3},
	{"tty",      0},
	{"displayS",  0},	//0=320*240  1=640*480	2 = 800*600
	{"displayM",  0},	//0= lcd	1=vga	2=tv
	{"initrdA",  0x30200000},
	{"initrdL",  0x02000000},
	{"memsize",  0x04000000},
	//{"devfs",    1},
	//{"ostore",   0},	//0=nand, 1=nor
	{"userpara", sizeof(DEFAULT_USER_PARAMS)},
	DEFAULT_USER_PARAMS,
	{0}
};

//小于等于256字节


#else

extern BootParams boot_params;


#endif

#endif
