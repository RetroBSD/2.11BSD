/*
 * Rewritten to look like the more modern versions of gcore on other systems.
 * The -s option for stopping the process was added, the ability to name
 * the core file was added, and to follow changes to the kernel the default 
 * corefile name was changed to be 'pid.core' rather than simply 'core'.
 * Only one process may be gcore'd at a time now however.  4/15/94 - sms.
 *
 * Originally written for V7 Unix, later changed to handle 2.9BSD, later
 * still brought up on 2.10BSD.  Pretty close the 4.XBSD except for
 * handling swapped out processes (it doesn't). - sms
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <stdio.h>
#include <nlist.h>
#include <varargs.h>

#define NLIST	"/vmunix"
#define MEM	"/dev/mem"

struct nlist nl[] = {
	{ "_proc" },
#define	X_PROC	0
	{ "_nproc" },
#define	X_NPROC	1
	{ 0 },
};

	int	mem, cor, nproc, sflag;
	struct	proc	*pbuf;
	char	*corefile, *program_name;

extern	int	optind, opterr;
extern	char	*optarg, *rindex();
extern	off_t	lseek();

main(argc, argv)
	int	argc;
	char	**argv;
{
	register int	pid, c;
	char	fname[32];

	if	(program_name = rindex(argv[0], '/'))
		program_name++;
	else
		program_name = argv[0];

	opterr = 0;
	while	((c = getopt(argc, argv, "c:s")) != EOF)
		{
		switch	(c)
			{
			case	'c':
				corefile = optarg;
				break;
			case	's':
				sflag++;
				break;
			default:
				usage();
				break;
			}
		}
	argv += optind;
	argc -= optind;
	if	(argc != 1)
		usage();
	pid = atoi(argv[0]);
	if	(corefile == 0)
		{
		sprintf(fname, "%d.core", pid);
		corefile = fname;
		}

	openfiles();
	getkvars();
	(void)lseek(mem, (long)nl[X_NPROC].n_value, L_SET);
	(void)read(mem, &nproc, sizeof nproc);
	pbuf = (struct proc *)calloc(nproc, sizeof (struct proc));
	if (pbuf == NULL)
		error("not enough core");
	(void)lseek(mem, (long)nl[X_PROC].n_value, L_SET);
	(void)read(mem, pbuf, nproc * sizeof (struct proc));

	core(pid);
	exit(0);
}

void
core(pid)
	register int	pid;
{
	register int i;
	register struct proc *pp;
	uid_t uid, getuid();
	char ubuf[USIZE*64];

	for (i = 0, pp = pbuf;; pp++) {
		if (pp->p_pid == pid)
			break;
		if (i++ == nproc)
			error("%d: not found", pid);
	}
	if (pp->p_uid != (uid = getuid()) && uid != 0)
		error("%d: not owner", pid);
	if ((pp->p_flag & SLOAD) == 0)
		error("%d: swapped out", pid);
	if (pp->p_stat == SZOMB)
		error("%d: zombie", pid);
	if (pp->p_flag & SSYS)
		error("%d: system process", pid);
	if (sflag && kill(pid, SIGSTOP) < 0)
		warning("%d: could not send stop signal", pid);
	if (lseek(mem, (long)pp->p_addr << 6, L_SET) < 0)
		error("bad mem seek");
	if (read(mem, &ubuf, sizeof ubuf) != sizeof ubuf)
		error("bad mem read");
	if ((cor = open(corefile, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
		perror(corefile);
		exit(1);
	}
	(void)write(cor, &ubuf, sizeof ubuf);
	(void)lseek(mem, (long)pp->p_daddr << 6, L_SET);
	dump(pp->p_dsize);
	(void)lseek(mem, (long)pp->p_saddr << 6, L_SET);
	dump(pp->p_ssize);

	if (sflag && kill(pid, SIGCONT) < 0)
		warning("%d: could not send continue signal", pid);
	(void)close(cor);
}

void
dump(size)
	register u_int size;
{
	register int blocks, i;
	int bytes;
	char buffer[BUFSIZ * 2];

	size <<= 6;
	blocks = size / sizeof (buffer);
	bytes = size % sizeof (buffer);

	for (i = 0; i < blocks; i++) {
		(void)read(mem, buffer, sizeof (buffer));
		(void)write(cor, buffer, sizeof (buffer));
	}
	if (bytes) {
		(void)read(mem, buffer, bytes);
		(void)write(cor, buffer, bytes);
	}
}

void
openfiles()
{
	mem = open(MEM, 0);
	if (mem < 0) {
		perror(MEM);
		exit(1);
	}
}

void
getkvars()
{
	nlist(NLIST, nl);
	if (nl[0].n_type == 0)
		error("%s: no namelist\n", NLIST);
}

void
usage()
	{
	fprintf(stderr, "usage: %s [ -s ] [ -c core ] pid\n", program_name);
	exit(1);
	}

/* VARARGS */
void
error(va_alist)
	va_dcl
	{
	va_list ap;
	register char	*cp;

	(void)fprintf(stderr, "%s: ", program_name);

	va_start(ap);
	cp = va_arg(ap, char *);
	(void)vfprintf(stderr, cp, ap);
	va_end(ap);
	if	(*cp)
		{
		cp += strlen(cp);
		if	(cp[-1] != '\n')
			(void)fputc('\n', stderr);
		}
	exit(1);
	/* NOTREACHED */
	}

/* VARARGS */
void
warning(va_alist)
	va_dcl
	{
	va_list ap;
	register char *cp;

	(void)fprintf(stderr, "%s: warning: ", program_name);

	va_start(ap);
	cp = va_arg(ap, char *);
	(void)vfprintf(stderr, cp, ap);
	va_end(ap);
	if	(*cp)
		{
		cp += strlen(cp);
		if	(cp[-1] != '\n')
			(void)fputc('\n', stderr);
		}
	}
