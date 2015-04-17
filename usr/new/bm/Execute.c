#include <stdio.h>
#include "bm.h"
#include "Extern.h"
Execute(DescVec, NPats, TextFile, Buffer)
struct PattDesc *DescVec[]; /* pointers to status vectors for the different
	* patterns, including skip tables, position in buffer, etc. */
int NPats; /* number of patterns */
char Buffer[]; /* holds text from file */
int TextFile; /* file to search */
{
	long BuffPos; /* offset of first char in Buffer in TextFile */
	int NRead, /* number of chars read from file */
		NWanted, /* number of chars wanted */
		NAvail, /* number of chars actually read */
		BuffSize, /* number of chars in buffer */
		BuffEx, /* flag to indicate that buffer has been searched */
		ResSize,
		/* number of characters in the last, incomplete line */
		Found, /* flag indicates whether pattern found
		* completely and all matches printed */
		Valid; /* was the match "valid", i.e. if -x used,
		* did the whole line match? */
	char *BuffEnd; /* pointer to last char of last complete line */

	/*
	 * In order to optimize I/O for some machines which impose a severe
	 * penalty for I/O on an odd address, we play a nasty game.  First, we
	 * assume that the Buffer which is passed to us has an even address.
	 * Then whenever we move a buffer residual back to the beginning of
	 * the Buffer for the next read cycle, we actually move it to Buffer +
	 * Odd(Residual) where Odd() is 1 if Residual is odd, zero otherwise.
	 * This causes the the next read to go down on an even address.  We
	 * keep track of the beginning of data in the Buffer with BuffStart.
	 */
	char *BuffStart;

	/* misc working variables */
#ifdef notdef
	int i;
#else !notdef
	register struct PattDesc	*Desc;
	struct PattDesc			**Vec, **LastPatt = DescVec+NPats;
#endif notdef

	/* initialize */
	ResSize = 0;
	Found = 0;
	BuffPos = 0;
	BuffStart = Buffer;
#ifdef notdef
	for (i=0; i < NPats; i++) {
		DescVec[i] -> Success = 0;
		DescVec[i] -> Start = Buffer;
	} /* for */
#else !notdef
	for (Vec=DescVec; Vec < LastPatt; Vec++) {
		Desc = *Vec;
		Desc->Success = 0;
		Desc->Start = Buffer;
	}
#endif notdef
	/* now do the searching */
	do {
		/* first, read a bufferfull and set up the variables */
		/*
		 * Some systems *even* get upset when you ask for an odd read
		 * length - ARGH!
		 */
		NWanted = (int)((unsigned)(MAXBUFF - ResSize) & ~1);
		NRead = 0;
		do {
			/*
			 * BuffStart+ResSize+BRead is even first time through
			 * this loop - afterwards, no guaranties, but for
			 * files this loop never goes more than once ...
			 * Can't do any better.
			 */
			NAvail =
			   read(TextFile,BuffStart + ResSize + NRead, NWanted);
			if (NAvail == -1) {
				fprintf(stderr,
				  "bm: error reading from input file\n");
				exit(2);
			} /* if */
			NRead += NAvail; NWanted -= NAvail;
		} while (NAvail && NWanted);
		BuffEx = 0;
		BuffSize = ResSize + NRead;
		BuffEnd = BuffStart + BuffSize - 1;
		/* locate the end of the last complete line */
		while (*BuffEnd != '\n' && BuffEnd >= BuffStart)
			--BuffEnd;
		if (BuffEnd < BuffStart)
			BuffEnd = BuffStart + BuffSize - 1;
		while (!BuffEx) { /* work through one buffer full */
			BuffEx = 1; /* set it provisionally, then clear
			* it if we find the buffer non-empty */
#ifdef notdef
			for (i=0; i< NPats; i++) {
				if (!DescVec[i]->Success)
				/* if the pattern  has not been found */
					DescVec[i]-> Success =
					Search(DescVec[i]->Pattern,
					DescVec[i]->PatLen, BuffStart, BuffEnd,
					DescVec[i]->Skip1, DescVec[i]->Skip2,
					DescVec[i]);
				if (DescVec[i]->Success){
				/* if a match occurred */
					BuffEx = 0;
					Valid = MatchFound(DescVec[i],BuffPos,
					BuffStart, BuffEnd);
					Found |= Valid;
					if ((sFlag || lFlag) && Found)
						return(0);
				} /* if */
			} /* for */
#else !notdef
			for (Vec=DescVec; Vec < LastPatt; Vec++) {
				Desc = *Vec;
				if (!Desc->Success)
				/* if the pattern  has not been found */
					Desc-> Success =
					Search(Desc->Pattern,
					Desc->PatLen, BuffStart, BuffEnd,
					Desc->Skip1, Desc->Skip2,
					Desc);
				if (Desc->Success){
				/* if a match occurred */
					BuffEx = 0;
					Valid = MatchFound(Desc,BuffPos,
					BuffStart, BuffEnd);
					Found |= Valid;
					if ((sFlag || lFlag) && Found)
						return(0);
				} /* if */
			} /* for */
#endif notdef
		} /* while */
		if(NRead) {
			ResSize = MoveResidue(DescVec,NPats,Buffer,&BuffStart,BuffEnd);
			BuffPos += BuffSize - ResSize;
		} /* if */
	} while (NRead);
	return(!Found);
} /* Execute */
