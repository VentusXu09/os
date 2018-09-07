
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                   xjbx,2017
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

char FileNames[200]={0};
int FileNumber=0;

struct File{
    int type;
    char filename[10];
    int firstchild,nextsibling;
    int goback;
};

int nowfolder, currentFile; 
struct File fs[100];

/*****************************************************************************
 *                               kernel_main
 *****************************************************************************/
/**
 * jmp from kernel.asm::_start. 
 * 
 *****************************************************************************/
PUBLIC int kernel_main()
{
	disp_str("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	int i, j, eflags, prio;
        u8  rpl;
        u8  priv; /* privilege */

	struct task * t;
	struct proc * p = proc_table;

	char * stk = task_stack + STACK_SIZE_TOTAL;

	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++,t++) {
		if (i >= NR_TASKS + NR_NATIVE_PROCS) {
			p->p_flags = FREE_SLOT;
			continue;
		}

	        if (i < NR_TASKS) {     /* TASK */
                        t	= task_table + i;
                        priv	= PRIVILEGE_TASK;
                        rpl     = RPL_TASK;
                        eflags  = 0x1202;/* IF=1, IOPL=1, bit 2 is always 1 */
			prio    = 15;
                }
                else {                  /* USER PROC */
                        t	= user_proc_table + (i - NR_TASKS);
                        priv	= PRIVILEGE_USER;
                        rpl     = RPL_USER;
                        eflags  = 0x202;	/* IF=1, bit 2 is always 1 */
			prio    = 5;
                }

		strcpy(p->name, t->name);	/* name of the process */
		p->p_parent = NO_TASK;

		if (strcmp(t->name, "INIT") != 0) {
			p->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs */
			p->ldts[INDEX_LDT_C].attr1  = DA_C   | priv << 5;
			p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
		}
		else {		/* INIT process */
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);
			init_desc(&p->ldts[INDEX_LDT_C],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			init_desc(&p->ldts[INDEX_LDT_RW],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
		}

		p->regs.cs = INDEX_LDT_C << 3 |	SA_TIL | rpl;
		p->regs.ds =
			p->regs.es =
			p->regs.fs =
			p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip	= (u32)t->initial_eip;
		p->regs.esp	= (u32)stk;
		p->regs.eflags	= eflags;

		p->ticks = p->priority = prio;

		p->p_flags = 0;
		p->p_msg = 0;
		p->p_recvfrom = NO_TASK;
		p->p_sendto = NO_TASK;
		p->has_int_msg = 0;
		p->q_sending = 0;
		p->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p->filp[j] = 0;

		stk -= t->stacksize;
	}

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

	init_clock();
        init_keyboard();

	restart();

	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}


/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header
{				/* byte offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
	/* 500 */
};

/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar file and store them.
 * 
 * @param filename The tar file.
 *****************************************************************************/
void untar(const char * filename)
{
	printf("[extract `%s'\n", filename);
	int fd = open(filename, O_RDWR);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);
	int i = 0;
	int bytes = 0;

	while (1) {
		bytes = read(fd, buf, SECTOR_SIZE);
		assert(bytes == SECTOR_SIZE); /* size of a TAR file
					       * must be multiple of 512
					       */
		if (buf[0] == 0) {
			if (i == 0)
				printf("    need not unpack the file.\n");
			break;
		}
		i++;

		struct posix_tar_header * phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		char * p = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREAT | O_RDWR | O_TRUNC);
		if (fdout == -1) {
			printf("    failed to extract file: %s\n", phdr->name);
			printf(" aborted]\n");
			close(fd);
			return;
		}
		printf("    %s", phdr->name);
		while (bytes_left) {
			int iobytes = min(chunk, bytes_left);
			read(fd, buf,
			     ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			bytes = write(fdout, buf, iobytes);
			assert(bytes == iobytes);
			bytes_left -= iobytes;
			printf(".");
		}
		printf("\n");
		close(fdout);
	}

	if (i) {
		lseek(fd, 0, SEEK_SET);
		buf[0] = 0;
		bytes = write(fd, buf, 1);
		assert(bytes == 1);
	}

	close(fd);

	printf(" done, %d files extracted]\n", i);
}

/*****************************************************************************
 *                                shabby_shell
 *****************************************************************************/
/**
 * A very very simple shell.
 * 
 * @param tty_name  TTY file name.
 *****************************************************************************/
void shabby_shell(const char * tty_name)
{
	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];
	char arg1[128];
	animation();
	welcome();

	currentFile=0;
	nowfolder=0;
	for(int i=0;i<100;i++)
	    fs[i].type=2;

	strcpy(fs[0].filename, "home");
	fs[0].firstchild=-1;
	fs[0].nextsibling=-1;
	fs[0].type=1;
	
	while (1) {
		write(1, "$ ", 2);
		int r = read(0, rdbuf, 70);
		rdbuf[r] = 0;

		int argc = 0;
		char * argv[PROC_ORIGIN_STACK];
		char * p = rdbuf;
		char * s;
		int word = 0;
		char ch;
		do {
			ch = *p;
			if (*p != ' ' && *p != 0 && !word) {
				s = p;
				word = 1;
			}
			if ((*p == ' ' || *p == 0) && word) {
				word = 0;
				argv[argc++] = s;
				*p = 0;
			}
			p++;
		} while(ch);
		argv[argc] = 0;

		int fd = open(argv[0], O_RDWR);
		if (fd == -1) {
			if (rdbuf[0]) {
				if(strcmp(rdbuf, "createFile") == 0){
				    createFile();
				}
				else if(strcmp(rdbuf, "writeFile") == 0){
				    writeFile();
				}
				else if(strcmp(rdbuf, "readFile") == 0){
				    readFile();
				}
				else if(strcmp(rdbuf, "removeFile") == 0){
				    removeFile();
				}
				else if(strcmp(rdbuf, "createFolder") == 0){
				    createFolder();
				}
				else if(strcmp(rdbuf, "ls") == 0){
				    ls();
				}
				else if(strcmp(rdbuf, "openFolder") == 0){
				    openFolder();
				}
				else if(strcmp(rdbuf, "goback") == 0){
				    goback();
				}
				else if(strcmp(rdbuf, "help") == 0){
				    help();
				}
				else if(strcmp(rdbuf, "animation") == 0){
				    animation();
				}
			}
		}
		else {
			close(fd);
			int pid = fork();
			if (pid != 0) { /* parent */
				int s;
				wait(&s);
			}
			else {	/* child */
				execv(argv[0], argv);
			}
		}
	}

	close(1);
	close(0);
}

void ls(){
	
	printf("%s:\n",fs[nowfolder].filename);
	for(int i=fs[nowfolder].firstchild; i!=-1;i=fs[i].nextsibling){
	    printf("%s\n",fs[i].filename);
	}
}

void goback(){
	nowfolder=fs[nowfolder].goback;
	for(int i=fs[nowfolder].firstchild; fs[i].nextsibling!=-1;i=fs[i].nextsibling)
		currentFile=fs[i].nextsibling;
	//printf("%s\n",fs[currentFile].filename);
}

void openFolder(){
    printf("please put in the FolderName\n");
    char filename[10]={0};
    read(0, filename, 10);
    for(int i=fs[nowfolder].firstchild; i!=-1;i=fs[i].nextsibling){
	if(strcmp(filename, fs[i].filename)==0){
	        nowfolder=i;
		currentFile=i;
	        break;	
	    }
	}
}

void createFolder(){
	printf("please put in the FolderName\n");
	int i=1;
	for(;fs[i].type!=2;i++);
        read(0,fs[i].filename,10); 
	fs[i].firstchild=-1;
	fs[i].nextsibling=-1;
	fs[i].type=1;
	fs[i].goback=nowfolder;
	if(fs[nowfolder].firstchild==-1)
	    fs[currentFile].firstchild=i;
	else
	    fs[currentFile].nextsibling=i;
	currentFile=i;
}

void createFile(){
	printf("please put in the FileName\n");
	int fd;int namefd;
        char filename[10]={0};
	int i=1;
	for(;fs[i].type!=2;i++);
		
        read(0,fs[i].filename,10); 
	
	fs[i].firstchild=-1;
	fs[i].nextsibling=-1;
	fs[i].type=0;
	if(fs[nowfolder].firstchild==-1)
	    fs[currentFile].firstchild=i;
	else
	    fs[currentFile].nextsibling=i;
	currentFile=i;
	//int i, n;
	//const char bufw[] = "abcde";
	//const int rd_bytes = 10;
	//char bufr[rd_bytes];

	//assert(rd_bytes <= strlen(bufw));

	/* create */
	fd = open(fs[currentFile].filename, O_CREAT | O_RDWR);
	if(fd == -1)
	{
		printf("Fail, please check and try again!!\n");
		return;
	}
	if(fd == -2)
	{
		printf("Fail, file exsists!!\n");
		return;
	}
        
	/*int j=0;
	for(int i=FileNumber*10;i<FileNumber*11;i++)
        {
	    FileNames[i]=filename[j];
	    j++;
	}
        namefd=open("spfile++",O_CREAT | O_RDWR);
        if(namefd == -2)
	{
		namefd=open("spfile++",O_RDWR);
	}
        if(namefd==-1)
        {
        printf("Fail, please check and try again!!\n");
		return;
        }
        write(namefd, FileNames, 200);*/
	//assert(fd != -1);
	printl("File created: %s \n", fs[currentFile].filename);
	close(fd);
        

}

void writeFile(){
	printf("please put in the FileName\n");
	int fd;
	int n;
	char filename[10]={0};
        read(0,filename,10);
	char bufw[128]={0};
	read(0,bufw,128);
	fd = open(filename,O_RDWR);
        if(fd == -1)
	{
		printf("Fail, please check and try again!!\n");
		return;
	}
	if(fd == -2)
	{
		printf("Fail, file exsists!!\n");
		return;
	}
	n = write(fd, bufw, 128);
	//printl("%s",bufw);
	//assert(n == strlen(bufw));
	close(fd);
}

void readFile(){
	printf("please put in the FileName\n");
	int fd;
	int n;
	char bufr[128]={0};
	char filename[10]={0};
        read(0,filename,10);
	fd = open(filename,O_RDWR);
		if(fd == -1)
	{
		printf("Fail, please check and try again!!\n");
		return;
	}
	if(fd == -2)
	{
		printf("Fail, file exsists!!\n");
		return;
	}
	n = read(fd, bufr, 128);
	//assert(n == rd_bytes);
	bufr[n] = 0;
	printl("read: %s\n", bufr);
	close(fd);
}

void removeFile(){
	char filename[10]={0};
        read(0,filename,10);
	printf("%s \n", filename);
	printf("%s\n",fs[nowfolder].filename);
	int i=nowfolder;
	if(fs[i].firstchild!=-1)
	    if(strcmp(filename, fs[fs[i].firstchild].filename)==0){
		fs[i].firstchild=fs[fs[i].firstchild].nextsibling;
	    }
        for(int i= fs[nowfolder].firstchild; i!=-1;i=fs[i].nextsibling){
	    if(fs[i].nextsibling!=-1){
		if(strcmp(filename, fs[fs[i].nextsibling].filename)==0)
	            fs[i].nextsibling=fs[fs[i].nextsibling].nextsibling;	
	    }
		
	}
     
	
	/*if (unlink(filename) == 0)
		printf("File removed: %s\n", filename);
	else
		printf("Failed to remove file: %s\n", filename);*/
}


void help()
{
	printf("********************************************************************************\n");
	printf("        name               |                      function                      \n");
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	printf("        animation          |           Play the video\n");
	printf("        ls                 |           List all files in current folder\n");
	printf("        help               |           List all commands\n");
	printf("        fiveinarow         |           Start game fiveinarow\n");
	printf("        moveBox            |           Start game moveBox\n");
	printf("        getDay             |           Start game getDay\n");
        printf("        openFolder         |           Start game getDay\n");
	printf("        calculator         |           Start a calculator\n");
	printf("        createFile         |           Create a file\n");
	printf("        createFolder       |           Create a folder\n");
	printf("        readFile           |           Read a file\n");
	printf("        removeFile         |           Delete a file\n");
	printf("        writefile          |           Edit file, cover the content\n");
	printf("        goback             |           turn to higher level Folder\n");
	printf("********************************************************************************\n");
	
}


/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 * 
 *****************************************************************************/
void Init()
{
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdout == 1);

	printf("Init() is running ...\n");

	/* extract `cmd.tar' */
	untar("/cmd.tar");
			

	char * tty_list[] = {"/dev_tty0"};

	int i;
	for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) {
		int pid = fork();
		if (pid != 0) { /* parent process */
			printf("[parent is running, child pid:%d]\n", pid);
		}
		else {	/* child process */
			printf("[child is running, pid:%d]\n", getpid());
			close(fd_stdin);
			close(fd_stdout);
			
			shabby_shell(tty_list[i]);
			assert(0);
		}
	}

	while (1) {
		int s;
		int child = wait(&s);
		printf("child (%d) exited with status: %d.\n", child, s);
	}

	assert(0);
}


/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	for(;;);
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	for(;;);
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	for(;;);
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

void clear(){
    for(int i=0;i<30;i++)
	printf("\n");
}

void animation(){
int i = 0;
clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n\n");
milli_delay(1000);


clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                   @@@@@\n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n\n");
milli_delay(1000);

clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                           @@@@@@@@@@@@@\n");
printf("                                                                @@@@@@@@\n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n\n");
milli_delay(1000);


clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                 @@@@@@@@@@@@@@@@@@     \n");
printf("                                                      @@@@@@@@@@@@@@@@@@\n");
printf("                                                              @@@       \n");
printf("                                                                @@      \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n\n");
milli_delay(1000);


clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                     @  \n");
printf("                                                                     @@@\n");
printf("                                                                    @@@@\n");
printf("                                                                   @@@@@\n");
printf("                                                                      @@\n");
printf("                                                                     @@ \n");
printf("                                                                   @@@@ \n");
printf("                                                                      @@\n");
printf("                                                                       @\n");
printf("                                                                       @\n");
printf("                                                                        \n");
printf("                                                                       @\n");
printf("                                                                      @@\n");
printf("                                            @@@@@@@@@@@@@@@@@@       @@@\n");
printf("                                                 @@@@@@@@@@@@@@@@@@@@@@@\n");
printf("                                                         @@@            \n");
printf("                                                           @@           \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n\n");
milli_delay(1000);

clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                              @         \n");
printf("                                                              @@@@  @@@@\n");
printf("                                                             @@@@@@@@@@@\n");
printf("                                                            @@@@@@@@@@@@\n");
printf("                                                               @@@@@@@@@\n");
printf("                                                              @@ @@@@@@@\n");
printf("                                                            @@@@   @@@@@\n");
printf("                                                               @@ @@@@@@\n");
printf("                                                                @@@@@@@@\n");
printf("                                                                @@@@@@@@\n");
printf("                                                                 @@  @@@\n");
printf("                                                                @@   @@@\n");
printf("                                                               @@   @@@@\n");
printf("                                     @@@@@@@@@@@@@@@@@@       @@@  @@@@@\n");
printf("                                          @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
printf("                                                  @@@            @@@@@@@\n");
printf("                                                    @@                  \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n\n");
milli_delay(1000);


clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                              @                         \n");
printf("                                              @@@@  @@@@@@@@@@          \n");
printf("                                             @@@@@@@@@@@@@@@ @@@        \n");
printf("                                            @@@@@@@@@@@@                \n");
printf("                                               @@@@@@@@@@@@@            \n");
printf("                                              @@ @@@@@@@@@@@@@          \n");
printf("                                            @@@@   @@@@@@@@@@@@@        \n");
printf("                                               @@ @@@@@@@@@@@@@@@       \n");
printf("                                                @@@@@@@@@@  @@@         \n");
printf("                                                @@@@@@@@@@@@@@@@        \n");
printf("                                                 @@  @@@@@@@@@@@        \n");
printf("                                                @@   @@@@@@@@@@@@@@@@   \n");
printf("                                               @@   @@@@@@@@@@@@@@@@   @\n");
printf("                     @@@@@@@@@@@@@@@@@@       @@@  @@@@@@@@@@@@@@@@@@@@@\n");
printf("                          @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
printf("                                  @@@            @@@@@@@@@@@@@@@@@@@@@@@\n");
printf("                                    @@                    @@@@@@@@@  @@@\n");
printf("                                                          @@@@@@        \n");
printf("                                                                        \n");
printf("                                                                        \n\n");
milli_delay(1000);

clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                      @                                 \n");
printf("                                      @@@@  @@@@@@@@@@                  \n");
printf("                                     @@@@@@@@@@@@@@@ @@@                \n");
printf("                                    @@@@@@@@@@@@                        \n");
printf("                                       @@@@@@@@@@@@@                    \n");
printf("                                      @@ @@@@@@@@@@@@@                  \n");
printf("                                    @@@@   @@@@@@@@@@@@@                \n");
printf("                                       @@ @@@@@@@@@@@@@@@               \n");
printf("                                        @@@@@@@@@@  @@@                 \n");
printf("                                        @@@@@@@@@@@@@@@@                \n");
printf("                                         @@  @@@@@@@@@@@                \n");
printf("                                        @@   @@@@@@@@@@@@@@@@           \n");
printf("                                       @@   @@@@@@@@@@@@@@@@   @ @@@@@@@\n");
printf("             @@@@@@@@@@@@@@@@@@       @@@  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
printf("                  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
printf("                          @@@            @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
printf("                            @@                    @@@@@@@@@  @@@@@@@@@@@\n");
printf("                                                  @@@@@@               @\n");
printf("                                                                        \n");
printf("                                                                        \n\n");
milli_delay(1000);

clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                         @                                              \n");
printf("                         @@@@  @@@@@@@@@@                               \n");
printf("                        @@@@@@@@@@@@@@@ @@@                             \n");
printf("                       @@@@@@@@@@@@                                     \n");
printf("                          @@@@@@@@@@@@@                                 \n");
printf("                         @@ @@@@@@@@@@@@@                               \n");
printf("                       @@@@   @@@@@@@@@@@@@                             \n");
printf("                          @@ @@@@@@@@@@@@@@@                            \n");
printf("                           @@@@@@@@@@  @@@                              \n");
printf("                           @@@@@@@@@@@@@@@@                             \n");
printf("                            @@  @@@@@@@@@@@                             \n");
printf("                           @@   @@@@@@@@@@@@@@@@                        \n");
printf("                          @@   @@@@@@@@@@@@@@@@   @ @@@@@@@@@@@@        \n");
printf("@@@@@@@@@@@@@@@@@@       @@@  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@      \n");
printf("     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   \n");
printf("             @@@            @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  \n");
printf("               @@                    @@@@@@@@@  @@@@@@@@@@@@@@@@@@@@@@  \n");
printf("                                     @@@@@@               @@@@@@@@@ @   \n");
printf("                                                               @        \n");
printf("                                                                        \n\n");
milli_delay(1000);

clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                  @                                                     \n");
printf("                  @@@@  @@@@@@@@@@                                      \n");
printf("                 @@@@@@@@@@@@@@@ @@@                                    \n");
printf("                @@@@@@@@@@@@                                            \n");
printf("                   @@@@@@@@@@@@@                                        \n");
printf("                  @@ @@@@@@@@@@@@@                                      \n");
printf("                @@@@   @@@@@@@@@@@@@                                    \n");
printf("                   @@ @@@@@@@@@@@@@@@                                   \n");
printf("                    @@@@@@@@@@  @@@                                     \n");
printf("                    @@@@@@@@@@@@@@@@                                    \n");
printf("                     @@  @@@@@@@@@@@                                    \n");
printf("                    @@   @@@@@@@@@@@@@@@@                               \n");
printf("                   @@   @@@@@@@@@@@@@@@@   @ @@@@@@@@@@@@               \n");
printf("@@@@@@@@@@@       @@@  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@             \n");
printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@          \n");
printf("      @@@            @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@         \n");
printf("        @@                    @@@@@@@@@  @@@@@@@@@@@@@@@@@@@@@@         \n");
printf("                              @@@@@@               @@@@@@@@@ @          \n");
printf("                                                        @               \n");
printf("                                                                        \n\n");
milli_delay(1000);

clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("       @                                                                \n");
printf("       @@@@  @@@@@@@@@@                                                 \n");
printf("      @@@@@@@@@@@@@@@ @@@                                               \n");
printf("     @@@@@@@@@@@@                                                       \n");
printf("        @@@@@@@@@@@@@                                                   \n");
printf("       @@ @@@@@@@@@@@@@                                                 \n");
printf("     @@@@   @@@@@@@@@@@@@                                               \n");
printf("        @@ @@@@@@@@@@@@@@@                                              \n");
printf("         @@@@@@@@@@  @@@                                                \n");
printf("         @@@@@@@@@@@@@@@@                                               \n");
printf("          @@  @@@@@@@@@@@                                               \n");
printf("         @@   @@@@@@@@@@@@@@@@                                          \n");
printf("        @@   @@@@@@@@@@@@@@@@   @ @@@@@@@@@@@@                          \n");
printf("       @@@  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                        \n");
printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                     \n");
printf("          @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                    \n");
printf("                   @@@@@@@@@  @@@@@@@@@@@@@@@@@@@@@@                    \n");
printf("                   @@@@@@               @@@@@@@@@ @                     \n");
printf("                                             @                          \n");
printf("                                                                        \n\n");
milli_delay(1000);


clear();
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("                                                                        \n");
printf("@@@@@@@                                                                 \n");
printf("@@@@@@@@@                                                               \n");
printf("@@@@@@@@@@@@                                                            \n");
printf("@@@@@@@@@@@@@                                                           \n");
printf("@@@@@@@@@@@@@                                                           \n");
printf(" @@@@@@@@@ @                                                            \n");
printf("      @                                                                 \n");
printf("                                                                        \n\n");
milli_delay(1000);


clear();
}

PUBLIC void welcome()
{

	printf("            ******************************************************\n");
	printf("            *                                                    *\n");
	printf("            *        The Shabby Operating System is running      *\n");
	printf("            *                                                    *\n");
	printf("            ******************************************************\n");
	printf("            *                                                    *\n");
	printf("            *                                                    *\n");
	printf("            *                1552713 Chen Kaixun                 *\n");
	printf("            *                1552737 Ling Yihong                 *\n");
	printf("            *                1552754 Luo Tianyuan                *\n");
	printf("            *                                                    *\n");
	printf("            *                                                    *\n");
	printf("            ******************************************************\n\n");
}

