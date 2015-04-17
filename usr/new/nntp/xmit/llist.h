/*
** Stuff for fiddlin' linked lists
** Erik E. Fair <fair@ucbarpa.berkeley.edu>
*/

struct llist {
	struct llist	*l_next;
	caddr_t		l_item;
	int		l_len;
};

typedef	struct llist	ll_t;

extern void	l_free();
extern ll_t	*l_alloc();

#ifndef NULL
#define NULL	0
#endif

#define	L_LOOP(p,head)	\
	for(p = &head; p->l_item != (caddr_t)NULL; p = p->l_next)
