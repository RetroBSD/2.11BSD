/*	bk.h	2.5	6/6/80	*/

#define	BK_BUFSIZE	128		/* buffer size per line */

/*
 * Macro definition of bk.c/bkinput().
 * This is used to replace a call to
 *		(*linesw[tp->t_line].l_input)(c,tp);
 * with
 *
 *		if (tp->t_line == NETLDISC)
 *			BKINPUT(c, tp);
 *		else
 *			(*linesw[tp->t_line].l_input)(c,tp);
 */
#define	BKINPUT(c, tp) { \
	if ((tp)->t_rec == 0) { \
		*(tp)->t_cp++ = c; \
		if (++(tp)->t_inbuf == BK_BUFSIZE || (c) == '\n') { \
			(tp)->t_rec = 1; \
			wakeup((caddr_t)&(tp)->t_rawq); \
		} \
	} \
}
