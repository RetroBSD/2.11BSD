
/*
 *      Program Name:   nsym.c
 *      Author:  S.M. Schultz
 *
 *      -----------   Modification History   ------------
 *      Version Date            Reason For Modification
 *      1.0     31Oct93         1. Initial release into the public domain.
 *				   Calculating the offsets of the string
 *				   and symbol tables in an executable is
 *				   rather messy and verbose when dealing
 *				   with overlaid objects.  The macros (in
 *				   a.out.h) N_STROFF, N_SYMOFF, etc simply
 *				   call these routines.
 *      --------------------------------------------------              
*/

#include <a.out.h>

off_t
n_stroff(ep)
	register struct xexec *ep;
	{
	off_t	l;

	l = n_symoff(ep);
	l += ep->e.a_syms;
	return(l);
	}

off_t
n_datoff(ep)
	register struct xexec *ep;
	{
	off_t	l;

	l = n_treloc(ep);
	l -= ep->e.a_data;
	return(l);
	}

/*
 * Obviously if bit 0 of the flags word (a_flag) is not off then there's
 * no relocation information present and this routine shouldn't have been
 * called.
*/

off_t
n_dreloc(ep)
	register struct xexec *ep;
	{
	off_t	l;
	register u_short *ov = ep->o.ov_siz;
	register int	i;

	l = (off_t)sizeof (struct exec) + ep->e.a_text + ep->e.a_data;
	if	(ep->e.a_magic == A_MAGIC5 || ep->e.a_magic == A_MAGIC6)
		{
		for	(i = 0; i < NOVL; i++)
			l += *ov++;
		l += sizeof (struct ovlhdr);
		}
	l += ep->e.a_text;
	return(l);
	}

off_t
n_treloc(ep)
	register struct xexec *ep;
	{
	off_t	l;

	l = n_dreloc(ep);
	l -= ep->e.a_text;
	return(l);
	}

off_t
n_symoff(ep)
	register struct xexec *ep;
	{
	register int	i;
	register u_short *ov;
	off_t	sum, l;

	l = (off_t) N_TXTOFF(ep->e);
	sum = (off_t)ep->e.a_text + ep->e.a_data;
	if	(ep->e.a_magic == A_MAGIC5 || ep->e.a_magic == A_MAGIC6)
		{
		for	(ov = ep->o.ov_siz, i = 0; i < NOVL; i++)
			sum += *ov++;
		}
	l += sum;
	if	((ep->e.a_flag & 1) == 0)	/* relocation present? */
		l += sum;
	return(l);
	}
