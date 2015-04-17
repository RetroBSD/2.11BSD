#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bm.h"
int
GetPatFile(PatFile, DescVec)
char *PatFile;
struct PattDesc *DescVec[];
/* read patterns from a file and set up a pattern descriptor vector */
{
	extern char *malloc();
	FILE *PFile;
	struct stat StatBuff;
	int PatSize; /* the number of chars in all the patterns */
	char *PatBuff; /* hold the patterns */
	if (!(PFile = fopen(PatFile,"r"))) {
		fprintf(stderr,"bm: can't open pattern file %s\n",PatFile);
		exit(2);
	} /* if */
	/* find out how big the patterns are */
	if (fstat(fileno(PFile),&StatBuff) == -1) {
		fprintf(stderr,"bm: can't fstat %s\n",PatFile);
		exit(2);
	} /* if */
	PatSize = StatBuff.st_size;
	if (!PatSize) {
		fprintf(stderr,"bm: pattern file is empty\n");
		exit(2);
	} /* if */
	if (!(PatBuff = malloc(PatSize))) {
	       fprintf(stderr,"bm: insufficient memory to store patterns\n");
		exit(2);
	} /* if */
	fread(PatBuff,1,PatSize,PFile); /* get the patterns */
	/* make sure the patterns are null-terminated. We can't have
	* nulls in the patterns */
	if (PatBuff[PatSize-1] == '\n')
		PatBuff[PatSize-1] = '\0';
	else
		PatBuff[PatSize] = '\0';
	return(MkDescVec(DescVec,PatBuff));
} /* GetPatFile */
