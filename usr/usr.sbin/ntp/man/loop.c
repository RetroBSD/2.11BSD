/*
***********************************************************************
* Program to simulate NTP phase-lock loop                             *
***********************************************************************
*/
#include <stdio.h>
#include <ctype.h>
#include <math.h>
/*
Parameters
*/
#define FMAX 8                  /* max filter size */
#define DAY 86400               /* one day of seconds */
#define DMAX .128               /* max clock aperture */
#define PI 3.1415926535         /* the real thing */
/*
Function declarations
*/
extern double pow(double, double);
/*
Global declarations
*/
double Kf = 1024;               /* frequency weight */
double Kg = 256;                /* phase weight */
double Kh = 256;                /* compliance weight */
double S = 16;                  /* compliance maximum */
double T = 262144;              /* compliance factor */
double U = 4;                   /* adjustment interval */
double poll = 64;               /* poll interval */
;
double f = 0;                   /* frequency error */
double g = 0;                   /* phase error */
double h = 0;                   /* compliance */
double c = 0;                   /* clock error */
double d;                       /* current filter output */
double filt[FMAX];              /* clock filter */
;
double offset = .01;             /* initial phase offset */
double freq = 10e-6;             /* initial frequency offset */
;
double t;                       /* current time */
double q, a, x, z;              /* temporaries */
int ns = 0;                     /* reset counter */
int n,i;                        /* integer temporary */
/*
Main program
*/
main() {
	n = poll/U; q = 1-1/Kg; q = ((pow(q,n)-1)/(q-1))/Kg;
	ns = 0; x = 0; z = 8;
		for (t = 0; t < 2*DAY; t += poll) {
		for (i = FMAX-1; i > 0; i--) filt[i] = filt[i-1];
		filt[0] = (offset-c-freq*t); d = filt[FMAX-1];
		if (fabs(d) >= DMAX) {
			ns++; c = c+d;
			for (i = 0; i < FMAX; i++) filt[i] = 0;
			printf("Reset %5.0f\n", d*1000);
			}
		a = S-T*fabs(h); if (a<1) a=1;
		h = h+(d-h)/Kh;
		f = f+d/(a*poll);
		g = d*q; c = c+g+(poll/(U*Kf))*f;

		printf("%8.2f%8.2f%8.2f%8.2f%8.2f   %8.2e",
		t/60, offset*1000, d*1000, (f/(U*Kf)+freq)*1e6,
		h*1000, a);
		printf("\n");
		}
	return (0);
	}

