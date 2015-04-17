#include "bm.h"
#include "Extern.h"
int Search(Pattern,PatLen,Buffer, EndBuff, Skip1, Skip2, Desc)
char Pattern[];
int PatLen;
char Buffer[];
char *EndBuff;
int Skip1[], Skip2[];
struct PattDesc *Desc;
{
	register char *k, /* indexes text */
		*j, /* indexes Pattern */
		*PatBegin; /* register pointing to char
		* before beginning of Pattern */
	register int Skip; /* skip distance */
	char *PatEnd,
	*BuffEnd; /* pointers to last char in Pattern and Buffer */
	BuffEnd = EndBuff;
	PatBegin = Pattern - 1;
	PatEnd = Pattern + PatLen - 1;

	k = Desc->Start;
	Skip = PatLen-1;
	while ( Skip <= (BuffEnd - k) ) {
		j = PatEnd;
		k = k + Skip;
		while (j > PatBegin && *j == *k) {
			--j; --k;
		} /* while */
		if (j< Pattern) {
			/* found it. Start next search
			* just after the pattern */
			Desc -> Start = k + 1 + Desc->PatLen;
			return(1);
		} /* if */
		Skip = max(Skip1[*(unsigned char *)k],Skip2[j-Pattern]);
	} /* while */
	Desc->Start = k;
	return(0);
} /* Search */
