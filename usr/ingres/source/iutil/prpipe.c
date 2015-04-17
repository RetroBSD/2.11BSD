# include	"../pipes.h"

prpipe(buf1)
struct pipfrmt	*buf1;
{
	register int		i;
	register struct pipfrmt	*buf;
	register char		c;

	buf = buf1;
	printf("pipe struct =\t%u\n", buf);
	printf("exec_id = '");
	xputchar(buf->exec_id);
	printf("'\tfunc_id = '");
	xputchar(buf->func_id);
	printf("'\terr_id = %d\nhdrstat = %d\tbuf_len = %d\tpbuf_pt = %d\n",
		buf->err_id, buf->hdrstat, buf->buf_len, buf->pbuf_pt);
	for (i=0; i < buf->buf_len; i++)
	{
		c = buf->pbuf[i];
		printf("\t%3d", c);
		if (c > ' ')
			printf(" %c", c);
		if (i % 8 == 7)
			putchar('\n');
	}
	putchar('\n');
}
