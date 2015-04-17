  /*  Input/Output Statements */

io:	  io1
		{ endio(); }
	;

io1:	  iofmove ioctl
	| iofmove unpar_fexpr
		{ ioclause(IOSUNIT, $2); endioctl(); }
	| iofctl ioctl
	| read ioctl
		{ doio(NULL); }
	| read ioctl inlist
		{ doio($3); }
	| read infmt SCOMMA inlist
		{ doio($4); }
	| read ioctl SCOMMA inlist
		{ doio($4); }
	| decode ioctlc inlist				/* E-D */
		{ doio($3); }				/* E-D */
	| encode ioctlc					/* E-D */
		{ doio(NULL); }				/* E-D */
	| encode ioctlc outlist				/* E-D */
		{ doio($3); }				/* E-D */
	| write ioctl
		{ doio(NULL); }
	| write ioctl outlist
		{ doio($3); }
	| print
		{ doio(NULL); }
	| print SCOMMA outlist
		{ doio($3); }
	;

iofmove:   fmkwd end_spec in_ioctl
	;

fmkwd:	  SBACKSPACE
		{ iostmt = IOBACKSPACE; }
	| SREWIND
		{ iostmt = IOREWIND; }
	| SENDFILE
		{ iostmt = IOENDFILE; }
	;

iofctl:  ctlkwd end_spec in_ioctl
	;

ctlkwd:	  SINQUIRE
		{ iostmt = IOINQUIRE; }
	| SOPEN
		{ iostmt = IOOPEN; }
	| SCLOSE
		{ iostmt = IOCLOSE; }
	;

infmt:	  unpar_fexpr
		{
		ioclause(IOSUNIT, NULL);
		ioclause(IOSFMT, $1);
		endioctl();
		}
	| SSTAR
		{
		ioclause(IOSUNIT, NULL);
		ioclause(IOSFMT, NULL);
		endioctl();
		}
	;

ioctl:	  SLPAR fexpr SRPAR
		{ ioclause(IOSUNIT, $2); endioctl(); }
	| SLPAR ctllist SRPAR
		{ endioctl(); }
	;

ctllist:  ioclause SCOMMA ioclause
	| ctllist SCOMMA ioclause
	;

ioclause:  fexpr
		{ ioclause(IOSPOSITIONAL, $1); }
	| SSTAR
		{ ioclause(IOSPOSITIONAL, NULL); }
	| nameeq expr
		{ ioclause($1, $2); }
	| nameeq SSTAR
		{ ioclause($1, NULL); }
	;

nameeq:  SNAMEEQ
		{ $$ = iocname(); }
	;

ioctlc:	 SLPAR expr SCOMMA fexpr SCOMMA fexpr SRPAR  	/* E-D */
		{					/* E-D */
		iosetecdc( $2 );			/* E-D */
		ioclause( IOSUNIT, $6 );		/* E-D */
		ioclause(  IOSFMT, $4 );		/* E-D */
		endioctl();				/* E-D */
		}					/* E-D */
	;						/* E-D */

read:	  SREAD end_spec in_ioctl
		{ iostmt = IOREAD; }
	;

write:	  SWRITE end_spec in_ioctl
		{ iostmt = IOWRITE; }
	;

decode:   SDECODE end_spec in_ioctl			/* E-D */
		{ iostmt = IOREAD; }			/* E-D */
	;						/* E-D */
							/* E-D */
encode:   SENCODE end_spec in_ioctl			/* E-D */
		{ iostmt = IOWRITE; }			/* E-D */
	;						/* E-D */

print:	  SPRINT end_spec fexpr in_ioctl
		{
		iostmt = IOWRITE;
		ioclause(IOSUNIT, NULL);
		ioclause(IOSFMT, $3);
		endioctl();
		}
	| SPRINT end_spec SSTAR in_ioctl
		{
		iostmt = IOWRITE;
		ioclause(IOSUNIT, NULL);
		ioclause(IOSFMT, NULL);
		endioctl();
		}
	;

inlist:	  inelt
		{ $$ = mkchain($1,0); }
	| inlist SCOMMA inelt
		{ $$ = hookup($1, mkchain($3,0)); }
	;

inelt:	  lhs
	| SLPAR inlist SCOMMA dospec SRPAR
		{ $$ = mkiodo($4,$2); }
	;

outlist:  uexpr
		{ $$ = mkchain($1, 0); }
	| other
		{ $$ = mkchain($1, 0); }
	| out2
	;

out2:	  uexpr SCOMMA uexpr
		{ $$ = mkchain($1, mkchain($3, 0) ); }
	| uexpr SCOMMA other
		{ $$ = mkchain($1, mkchain($3, 0) ); }
	| other SCOMMA uexpr
		{ $$ = mkchain($1, mkchain($3, 0) ); }
	| other SCOMMA other
		{ $$ = mkchain($1, mkchain($3, 0) ); }
	| out2  SCOMMA uexpr
		{ $$ = hookup($1, mkchain($3, 0) ); }
	| out2  SCOMMA other
		{ $$ = hookup($1, mkchain($3, 0) ); }
	;

other:	  complex_const
	| SLPAR uexpr SCOMMA dospec SRPAR
		{ $$ = mkiodo($4, mkchain($2, 0) ); }
	| SLPAR other SCOMMA dospec SRPAR
		{ $$ = mkiodo($4, mkchain($2, 0) ); }
	| SLPAR out2  SCOMMA dospec SRPAR
		{ $$ = mkiodo($4, $2); }
	;

in_ioctl:
		{ startioctl(); }
	;
