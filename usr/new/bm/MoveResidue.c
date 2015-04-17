#include "bm.h"
/* Moves the text which has not been completely searched at the end of
* the buffer to the beginning of the buffer
* and returns number of bytes moved */

/*
 * In coordination with the I/O optimization code in Execute, if the Residual
 * is odd in length, we move it to Buffer+1, otherwise to Buffer+0, to make
 * sure that [at least] the next read goes down on an even address.  A pointer
 * to a pointer to the current start of data in the buffer is passed in, that
 * pointer is updated and passed back.
 */

int MoveResidue(DescVec, NPats, Buffer, BuffStartP, BuffEnd)
struct PattDesc *DescVec[];
int NPats;
char *Buffer, **BuffStartP, *BuffEnd;
{
	char *FirstStart, *f, *t, *Residue;
	int ResSize, i;
	FirstStart = BuffEnd;
	/* find the earliest point which has not been
	* completely searched */
	for (i=0; i < NPats; i++) {
		FirstStart = 
			min(FirstStart, DescVec[i]-> Start );
	} /* for */
	/* now scan to the beginning of the line containing the
	* unsearched text */
	for (Residue = FirstStart; *Residue != '\n' &&
	Residue >= *BuffStartP; Residue--);
	if (Residue < *BuffStartP)
		Residue = FirstStart;
	else ++Residue;
	ResSize = (BuffEnd - Residue + 1);
	/* now move the residue to the beginning of
	* the file */
	t = *BuffStartP = ((unsigned) ResSize & 1) ? Buffer+1 : Buffer;
	f = Residue;
	for(i=ResSize;i;--i)
		*t++ = *f++;
	/* now fix the status vectors */
	for (i=0; i < NPats; i++) {
		DescVec[i]->Start -= (Residue - *BuffStartP);
	} /* for */
	return(ResSize);
} /* MoveResidue */
