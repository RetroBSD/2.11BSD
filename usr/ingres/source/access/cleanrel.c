# include "../ingres.h"
# include "../aux.h"
# include "../access.h"

/*
** CLEANREL --
**	If there are any buffers being used by the relation described
**	in the descriptor struct, flush and zap the buffer.
**	This will force a UNIX disk read the next time the relation
**	is accessed which is useful to get the most up-to-date
**	information from a file that is being updated by another
**	program.
*/

cleanrel(d)
struct descriptor	*d;

{

	return (flush_rel(d, TRUE));	/* flush and reset all pages of this rel */
}
