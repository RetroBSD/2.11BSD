# include	"../../pipes.h"

IIprpipe(buf1)
struct pipfrmt	*buf1;
{
	register int		i;
	register struct pipfrmt	*buf;
	register char		c;

	buf = buf1;
	printf("pipe struct =\t%u\n", buf);
	printf("exec_id = '%c'\tfunc_id = '%c'\terr_id = %d\nhdrstat = %d\tbuf_len = %d\tpbuf_pt = %d\n",
		buf->exec_id, buf->func_id, buf->err_id, buf->hdrstat, buf->buf_len, buf->pbuf_pt);
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
