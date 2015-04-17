/* v6-- accesses version 6 filesystems and copies files to	*/
/*	version 7 filesystems					*/
/*	Mike Karels, University of California Berkeley		*/
/*			Molecular Biology Department		*/
/*			(mail to virus.mike @BERKELEY)		*/
/*			(415) 642-7359				*/

#include "inode.h"
#include "filsys.h"
#include <signal.h>
#include <stdio.h>

#define	BLKSIZE	512
#define	MAXLINE 80		/* maximum input line length */
#define NARG	3	/* maximum number of arguments */
#define MAXARG	30	/* maximum size of input args */
#define	MAXSIZE	BLKSIZE/sizeof(struct direct)*8	/* max number of  
					* directory entries	*/
#define	DIRSIZE	14

struct filsys sb;	/* superblock */
struct inode dinode, finode;	/* current dir, file inodes */
struct direct 
{
	int	d_ino;
	char	d_name[DIRSIZE];
} dir[MAXSIZE], cur_dir, root = { 1, "/" };
int	dir_sz, fd, fo, imax;
char	*cmd[] = { "ls", "cd", "cp", "cat", "?", "", "q", "cpdir" };




main(argc,argv)
int argc;
char *argv[];
{
	extern struct direct dir[], root, cur_dir;
	extern struct inode dinode;
	extern int fd;
	extern char *cmd[];
	char linein[MAXLINE], arg[NARG][MAXARG];
	int i, count, null();

	if (argc != 2) {
		fprintf(stderr,"Usage: v6 file, where file is block special\n");
		exit(1);
	}
	if ((fd=open(argv[1],0)) < 0) {
		fprintf(stderr,"can't open %s\n",argv[1]);
		exit(1);
	}
	if ((lseek(fd,(long)BLKSIZE,0) < 0) ||
	   (read(fd,&sb,BLKSIZE) < BLKSIZE)) {
		fprintf(stderr,"can't read superblock\n");
		exit(1);
	   }
	imax = BLKSIZE / sizeof(struct inode) * (sb.isize-1);
	if (readdir(&root) < 0) exit(1);
	cur_dir.d_ino = root.d_ino;
	strcpy(cur_dir.d_name,root.d_name);
	signal(SIGINT,SIG_IGN);

	while(1) {
		printf("* ");
		if (getline(linein) == EOF) exit(0);
		for (i=0; i<NARG; i++) arg[i][0] = '\0';
		sscanf(linein,"%s %s %s",arg[0],arg[1],arg[2]);
		for (i = 0; (i<8) && (strcmp(arg[0],cmd[i])); i++)  ;
		switch(i) {
			case 0: ls(arg[1]);
				break;
			case 1: cd(arg[1]);
				break;
			case 2: cp(arg[1],arg[2],1);
				break;
			case 3: cat(arg[1]);
				break;
			case 4: msg();
				break;
			case 5: break;
			case 6: exit(0);
			case 7: cpdir(arg[1]);
				break;
			case 8: if (*arg[0] == '!') {
					signal(SIGINT,null);
					system(linein+1);
					signal(SIGINT,SIG_IGN);
				}
				else fprintf(stderr,"invalid command\n");
				break;
		}
	}
}

getline(linein)		/* reads input line into linein */
char linein[]; {
	int i, c;

	for (i=0; i<MAXLINE-1 && (c=getchar()) != '\n' && c != EOF;
		i++) linein[i] = c;
	if (c == '\n') {
		linein[i] = '\0';
		return(i);
	}
	else if (c == EOF) return(EOF);
	else {
		fprintf(stderr,"input line too long\n");
		fflush(stdin);
		linein[0] = '\0';
		return(0);
	}
}

null() {}		/* trap here on interrupt during system() */

ls(name)	/* list directory 'name', current dir if no name */
char *name; {
	int i, j, flag;
	extern struct direct dir[], cur_dir;
	extern int dir_sz;
	struct direct save_dir;

	flag = 0;
	if (name[0] != '\0') {
		flag = 1;
		save_dir.d_ino = cur_dir.d_ino;
		strcpy(save_dir.d_name,cur_dir.d_name);
		if (cd(name) < 0) return;
	}
	j=0;
	for (i=0; i<dir_sz; i++)
		if (dir[i].d_ino != 0 ) 
			printf("%-14s%c", dir[i].d_name,
				++j%4 ? '\t':'\n');
	if (j%4) putchar('\n');
	if (flag) {
		readdir(&save_dir);
		cur_dir.d_ino = save_dir.d_ino;
		strcpy(cur_dir.d_name,save_dir.d_name);
	}
}
cpdir(arg)	/* copy contents of current directory */
char *arg; {
	int i;
	extern struct direct dir[];
	extern int dir_sz;

	if (arg[0] != '\0') {
		fprintf(stderr,"no arguments allowed for cpdir\n");
		return(-1);
	}
	for (i=0; i<dir_sz; i++)
		if ((dir[i].d_ino != 0) && (dir[i].d_name[0] != '.')) 
			cp(dir[i].d_name,dir[i].d_name,1);
	return(0);
}

cd(name)		/* returns 0 if successful, else -1 */
char *name; {	/* returns to previous directory if unsuccesssful */
	extern struct inode dinode;
	extern struct direct dir[], cur_dir, root;
	extern int dir_sz;
	int i, ino;
	char *c, *rindex();
	struct direct *pdir, *find(), save_dir;

	if ((i=iindex(name,'/')) >= 0) {
		if (i==0) {
			readdir(&root);
			name++;
		}
		if (name[0]=='\0') {
			cur_dir.d_ino = root.d_ino;
			strcpy(cur_dir.d_name,root.d_name);
			return(0);
		}
		if (*((c=rindex(name,'/'))+1) == '\0') *c = '\0';
			/* removes trailing '/' if present */
		while ((i=iindex(name,'/')) != -1) {
			name[i] = '\0';
			if ((pdir=find(name)) == NULL) {
				fprintf(stderr,"can't find %s\n",name);
				readdir(&cur_dir);
				return(-1);
			}
			if (readdir(pdir) < 0) return(-1);
			name += i+1;
		}
	}
	if ((pdir=find(name))==NULL) {
		fprintf(stderr,"can't find %s\n",name);
		readdir(&cur_dir);
		return(-1);
	}
	ino = pdir->d_ino;
	if (readdir(pdir) >= 0) {
		cur_dir.d_ino = ino;
		strcpy(cur_dir.d_name,name);
		return(0);
	}
	else return(-1);
}

iindex(s,c)
char *s, c; {
	int i;

	for (i=0; ; i++) {
		if (s[i] == c) return(i);
		if (s[i] == NULL) return(-1);
	}
}

struct direct *find(name)	/* returns pointer to "name" entry */
char *name; {			/* in dir[], NULL if not found */
	extern struct direct dir[];
	int i;
	extern int dir_sz;

	for (i=0; i<dir_sz; i++)
		if ((strcmp(dir[i].d_name,name) == 0) &&
			(dir[i].d_ino != 0))
			break;
	if (i==dir_sz) return(NULL);
	return(&(dir[i]));
}

#define MODE 0644
#define STDOUT 1

cp(ifile,ofile,mode)	/* copies v6 ifile to v7 ofile if mode 1 */
char *ifile, *ofile;	/* cats ifile if mode 0 */
int mode; {

	extern struct inode finode;
	extern struct direct dir[], cur_dir;
	extern int fd,fo;
	int n, i, j, blk, flag, indir[BLKSIZE/2];
	long size;
	char buf[BLKSIZE], *tname;
	struct direct *pdir, save_dir;

	flag = 0;
	if (ofile[0] == '\0' && mode == 1) {
		fprintf(stderr,"Usage: cp v6file v7file\n");
		goto quit;
	}
	if ((mode==1) && ((fo=open(ofile,0) != -1))) {
		fprintf(stderr,"%s already exists\n",ofile);
		goto quit;
	}
	if (mode == 1)  {
		if ((fo= creat(ofile,MODE)) < 0) {
			fprintf(stderr,"can't create %s\n",ofile);
			goto quit;
		}
	}
	else fo = dup(STDOUT);
	if (iindex(ifile,'/') != -1) {
		flag = 1;
		save_dir.d_ino = cur_dir.d_ino;
		strcpy(save_dir.d_name,cur_dir.d_name);
		tname = rindex(ifile,'/');
		*tname = '\0';
		if (cd(ifile) < 0) return;
		ifile = tname + 1;
	}
	if ((pdir = find(ifile)) == NULL) {
		fprintf(stderr,"can't find %s\n",ifile);
		goto quit;
	}
	if (readi(pdir->d_ino,&finode) < 0) goto quit;
	if (finode.flags & (IFCHR | IFBLK)) {	
		/* is special file or directory */
		fprintf(stderr,"cp: %s not a regular file\n");
		goto quit;
	}
	size = finode.size1 + ((long)(finode.size0 & 0377) << 8);
	if (finode.flags & IFLRG)	/* file is large */
	   for (blk=0; size>0; blk++) {
		if (blk == 7) {
			fprintf(stderr,"%s is huge file, cp incomplete\n",
				ifile);
			goto quit;
		}
		if (readblk(finode.addr[blk],indir) < BLKSIZE)
			goto quit;
		for (i=0; ((i<BLKSIZE/2) && (size > 0)); i++) {
			if (indir[i]==0)
				for (j=0; j<BLKSIZE/2; j++)
				buf[j] = 0;
			else if (readblk(indir[i],buf) < BLKSIZE)
				goto quit;
			n = (size < BLKSIZE) ? size : BLKSIZE;
			if (write(fo,buf,n) != n) {
				if (mode == 1)
					fprintf(stderr,"write error\n");
				else putchar('\n');
				goto quit;
			}
			size -= BLKSIZE;
		}
	   }
	else for (blk=0; size>0 && blk<8; blk++) {
		if (finode.addr[blk]==0)
			for (j=0; j<BLKSIZE/2; j++)
			buf[j] = 0;
		else if (readblk(finode.addr[blk],buf) < BLKSIZE)
			goto quit;
		n = (size<BLKSIZE) ? size : BLKSIZE;
		if (write(fo,buf,n) != n) {
			if (mode == 1) fprintf(stderr,"write error\n");
			else putchar('\n');
			goto quit;
		}
	size -= BLKSIZE;
	}
quit:	if (fo != STDOUT) close(fo);
	if (flag) {
		readdir(&save_dir);
		cur_dir.d_ino = save_dir.d_ino;
		strcpy(cur_dir.d_name,save_dir.d_name);
	}
}

readdir(pdir)		/* reads pdir->d_inode into dinode, then */
struct direct *pdir;	/* reads corresponding directory into dir */
{			/* reads cur_dir on error, then returns -1 */
	extern struct inode dinode;
	extern struct direct dir[], cur_dir;
	extern int dir_sz, fd;
	char buf[BLKSIZE];
	int blk, sz, i, n;
	long size;

	if (pdir->d_ino == 0) return(-1);
	if (readi(pdir->d_ino,&dinode) < 0) return(-1);
	if (!(dinode.flags & IFDIR)) {	/* not a directory */
		fprintf(stderr,"%s not a directory\n",pdir->d_name);
		readdir(&cur_dir);
		return(-1);
	}
	if (dinode.flags & IFLRG) {	/* file is large */
		fprintf(stderr,"Ouch, %s is large\n",pdir->d_name);
		readdir(&cur_dir);
		return(-1);
	}
	size = dinode.size1 + (long)(dinode.size0 & 0377) << 8;
	sz = 0;
	n = BLKSIZE/sizeof(struct direct);
	for (blk=0; size>0 && blk<8; blk++)
		if (dinode.addr[blk] > 0) {
			if (readblk(dinode.addr[blk],&dir[sz])<BLKSIZE)
				return(-1);
			sz += (size<BLKSIZE ? size : BLKSIZE)
				/sizeof(struct direct);
			size -= BLKSIZE;
		}
	dir_sz = sz;
	return(dir_sz);
}

readi(inum,pinode)	/* reads inode inum into *pinode */
int inum;
struct inode *pinode; {
	extern int fd, imax;

	if (inum < 1 || inum > imax) {
		fprintf(stderr,"bad inode number, %d\n",inum);
		return(-1);
	}
	if ((lseek(fd,((long)2*BLKSIZE +
		(long)(inum-1)*(sizeof(struct inode))),0) < 0) ||
	(read(fd,pinode,sizeof(struct inode)) < 0)) {
		fprintf(stderr,"can't read inode %d\n",inum);
		return(-1);
	}
	return(0);
}

readblk(blk,buf)	/* reads block blk into buf */
int blk;
char *buf; {
	extern struct filsys sb;
	extern int fd;
	int n;

	if ((blk < sb.isize-1) || (blk >= sb.fsize)) {
		fprintf(stderr,"bad block number %d\n",blk);
		return(-1);
	}
	if (lseek(fd,(long)(BLKSIZE)*(long)(blk),0) < 0) {
		fprintf(stderr,"seek error, block %d\n",blk);
		return(-1);
	}
	if ((n=read(fd,buf,BLKSIZE)) != BLKSIZE)
		fprintf(stderr,"read error, block %d\n",blk);
	return(n);
}

cat(ifile)		/* prints ifile on terminal, using cp */
char *ifile; {

	int flush();
	signal(SIGINT,flush);
	cp(ifile,NULL,0);	/* mode 0 signals print on tty */
	signal(SIGINT,SIG_IGN);
}

flush() {		/* closes fo to terminate a cat() */
	extern int fo;
	close(fo);
	return;
}

msg() {
	printf("commands:\n");
	printf("\t ls [dir]: list directory contents, current dir default\n");
	printf("\t cd name: change to (v6) directory 'name'\n");
	printf("\t cat name: print (v6) file 'name' on terminal\n");
	printf("\t cp name1 name2: copy (v6) 'name1' to (v7) file 'name2'\n");
	printf("\t cpdir: copy all files in current v6 directory\n");
	printf("\t\tto current v7 directory\n");
	printf("\t q or ^d: quit\n");
}
