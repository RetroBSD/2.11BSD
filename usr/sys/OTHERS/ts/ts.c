From osmail!egdorf@LANL Thu May  2 12:04:38 1985
Date: Thu, 2 May 85 09:58:16 mdt
From: osmail!egdorf@LANL.ARPA
To: lanl!keith@seismo
Subject: 2.9 ts.c

Keith, here is a diff -c of the original ts.c and my fixes.
The driver has been tested on 11/23, 11/34, and 11/73.
The two remaining areas of bugs are probably in error handling, and
unibus-map stuff (note no tests on any machine with a ubmap).

The driver in the standalone boot DOES seem to work already.

Sorry about the delay in sending this stuff to you, but as you have already
said, Things take longer than expected.

I have my mods for use with the 11/73 if you would like them.
I can send either diffs or source over the net, or I can make a tape.
What would you prefer?

>*** ts.c.orig	Tue Dec 18 15:41:04 1984
>--- ts.c	Sun Feb  3 16:45:31 1985
>***************
>*** 15,20
>  #include <sys/file.h>
>  #include <sys/user.h>
>  #include <sys/tsreg.h>
>  #ifdef	TS_IOCTL
>  #include <sys/mtio.h>
>  #endif
>
>--- 15,21 -----
>  #include <sys/file.h>
>  #include <sys/user.h>
>  #include <sys/tsreg.h>
>+ #include <sys/seg.h>
>  #ifdef	TS_IOCTL
>  #include <sys/mtio.h>
>  #endif
>***************
>*** 37,42
>  	short	sc_resid;
>  	daddr_t	sc_blkno;
>  	daddr_t	sc_nxrec;
>  	struct	ts_cmd	sc_cmd;
>  	struct	ts_sts	sc_sts;
>  	struct	ts_char	sc_char;
>
>--- 38,44 -----
>  	short	sc_resid;
>  	daddr_t	sc_blkno;
>  	daddr_t	sc_nxrec;
>+ 	char	sc_pad[4];	/* allow mod 4 alignment of sc_cmd */
>  	struct	ts_cmd	sc_cmd;
>  	struct	ts_sts	sc_sts;
>  	struct	ts_char	sc_char;
>***************
>*** 150,156
>  		 * preventing further opens from completing by
>  		 * preventing a TS_SENSE from completing.
>  		 */
>! 		tscommand(dev, TS_REV, 0);
>  	sc->sc_openf = 0;
>  }
>  
>
>--- 152,158 -----
>  		 * preventing further opens from completing by
>  		 * preventing a TS_SENSE from completing.
>  		 */
>! 		tscommand(dev, TS_REW, 0);
>  	sc->sc_openf = 0;
>  }
>  
>***************
>*** 243,249
>  		return;
>  	tsunit = TSUNIT(bp->b_dev);
>  	sc = &ts_softc[tsunit];
>! 	tc = &sc->sc_cmd;
>  	/*
>  	 * Default is that last command was NOT a write command;
>  	 * if we do a write command we will notice this in tsintr().
>
>--- 245,251 -----
>  		return;
>  	tsunit = TSUNIT(bp->b_dev);
>  	sc = &ts_softc[tsunit];
>! 	tc = (unsigned)(&sc->sc_cmd) & ~3;
>  	/*
>  	 * Default is that last command was NOT a write command;
>  	 * if we do a write command we will notice this in tsintr().
>***************
>*** 296,301
>  	 */
>  	if ((blkno = sc->sc_blkno) == dbtofsb(bp->b_blkno)) {
>  		tc->c_size = bp->b_bcount;
>  		if ((bp->b_flags & B_READ) == 0)
>  			cmd = TS_WCOM;
>  		else
>
>--- 298,305 -----
>  	 */
>  	if ((blkno = sc->sc_blkno) == dbtofsb(bp->b_blkno)) {
>  		tc->c_size = bp->b_bcount;
>+ 		tc->c_loba = bp->b_un.b_addr;
>+ 		tc->c_hiba = bp->b_xmem;
>  		if ((bp->b_flags & B_READ) == 0)
>  			cmd = TS_WCOM;
>  		else
>***************
>*** 304,310
>  			cmd |= TS_RETRY;
>  		tstab.b_active = SIO;
>  		tc->c_cmd = TS_ACK | TS_CVC | TS_IE | cmd;
>! 		TSADDR->tsdb = &sc->sc_cmd.c_cmd;
>  		return;
>  	}
>  	/*
>
>--- 308,314 -----
>  			cmd |= TS_RETRY;
>  		tstab.b_active = SIO;
>  		tc->c_cmd = TS_ACK | TS_CVC | TS_IE | cmd;
>! 		TSADDR->tsdb = tc;
>  		return;
>  	}
>  	/*
>***************
>*** 327,333
>  	 * Do the command in bp.
>  	 */
>  	tc->c_cmd = TS_ACK | TS_CVC | TS_IE | bp->b_command;
>! 	TSADDR->tsdb = &sc->sc_cmd.c_cmd;
>  	return;
>  
>  next:
>
>--- 331,337 -----
>  	 * Do the command in bp.
>  	 */
>  	tc->c_cmd = TS_ACK | TS_CVC | TS_IE | bp->b_command;
>! 	TSADDR->tsdb = tc;
>  	return;
>  
>  next:
>***************
>*** 444,450
>  		 * Couldn't recover error.
>  		 */
>  #ifdef	UCB_DEVERR
>! 		printf("ts%d: hard error bn%d xs0=%b", TSUNIT(bp->b_dev),
>  		     bp->b_blkno, sc->sc_sts.s_xs0, TSXS0_BITS);
>  		if (sc->sc_sts.s_xs1)
>  			printf(" xs1=%b", sc->sc_sts.s_xs1, TSXS1_BITS);
>
>--- 448,454 -----
>  		 * Couldn't recover error.
>  		 */
>  #ifdef	UCB_DEVERR
>! 		printf("ts%d: hard error bn%d xs0=%b\n", TSUNIT(bp->b_dev),
>  		     bp->b_blkno, sc->sc_sts.s_xs0, TSXS0_BITS);
>  		if (sc->sc_sts.s_xs1)
>  			printf(" xs1=%b\n", sc->sc_sts.s_xs1, TSXS1_BITS);
>***************
>*** 447,453
>  		printf("ts%d: hard error bn%d xs0=%b", TSUNIT(bp->b_dev),
>  		     bp->b_blkno, sc->sc_sts.s_xs0, TSXS0_BITS);
>  		if (sc->sc_sts.s_xs1)
>! 			printf(" xs1=%b", sc->sc_sts.s_xs1, TSXS1_BITS);
>  		if (sc->sc_sts.s_xs2)
>  			printf(" xs2=%b", sc->sc_sts.s_xs2, TSXS2_BITS);
>  		if (sc->sc_sts.s_xs3)
>
>--- 451,457 -----
>  		printf("ts%d: hard error bn%d xs0=%b\n", TSUNIT(bp->b_dev),
>  		     bp->b_blkno, sc->sc_sts.s_xs0, TSXS0_BITS);
>  		if (sc->sc_sts.s_xs1)
>! 			printf(" xs1=%b\n", sc->sc_sts.s_xs1, TSXS1_BITS);
>  		if (sc->sc_sts.s_xs2)
>  			printf(" xs2=%b\n", sc->sc_sts.s_xs2, TSXS2_BITS);
>  		if (sc->sc_sts.s_xs3)
>***************
>*** 449,455
>  		if (sc->sc_sts.s_xs1)
>  			printf(" xs1=%b", sc->sc_sts.s_xs1, TSXS1_BITS);
>  		if (sc->sc_sts.s_xs2)
>! 			printf(" xs2=%b", sc->sc_sts.s_xs2, TSXS2_BITS);
>  		if (sc->sc_sts.s_xs3)
>  			printf(" xs3=%b\n", sc->sc_sts.s_xs3, TSXS3_BITS);
>  #else
>
>--- 453,459 -----
>  		if (sc->sc_sts.s_xs1)
>  			printf(" xs1=%b\n", sc->sc_sts.s_xs1, TSXS1_BITS);
>  		if (sc->sc_sts.s_xs2)
>! 			printf(" xs2=%b\n", sc->sc_sts.s_xs2, TSXS2_BITS);
>  		if (sc->sc_sts.s_xs3)
>  			printf(" xs3=%b\n", sc->sc_sts.s_xs3, TSXS3_BITS);
>  #else
>***************
>*** 547,553
>  {
>  	register struct tsdevice *tsaddr = TSADDR;
>  	struct	ts_softc *sc = &ts_softc[tsunit];
>! 	register struct ts_cmd *tcmd = &sc->sc_cmd;
>  	register struct ts_char *tchar = &sc->sc_char;
>  
>  	if (tsaddr->tssr & (TS_NBA | TS_OFL)) {
>
>--- 551,557 -----
>  {
>  	register struct tsdevice *tsaddr = TSADDR;
>  	struct	ts_softc *sc = &ts_softc[tsunit];
>! 	register struct ts_cmd *tcmd;
>  	register struct ts_char *tchar = &sc->sc_char;
>  
>  	tcmd = (unsigned)(&sc->sc_cmd) & ~3;
>***************
>*** 550,555
>  	register struct ts_cmd *tcmd = &sc->sc_cmd;
>  	register struct ts_char *tchar = &sc->sc_char;
>  
>  	if (tsaddr->tssr & (TS_NBA | TS_OFL)) {
>  		tsaddr->tssr = 0;		/* subsystem initialize */
>  		tswait(tssr);
>
>--- 554,560 -----
>  	register struct ts_cmd *tcmd;
>  	register struct ts_char *tchar = &sc->sc_char;
>  
>+ 	tcmd = (unsigned)(&sc->sc_cmd) & ~3;
>  	if (tsaddr->tssr & (TS_NBA | TS_OFL)) {
>  		tsaddr->tssr = 0;		/* subsystem initialize */
>  		tswait(tssr);
>***************
>*** 553,562
>  	if (tsaddr->tssr & (TS_NBA | TS_OFL)) {
>  		tsaddr->tssr = 0;		/* subsystem initialize */
>  		tswait(tssr);
>- 		if (((u_short) tcmd) & 03) {
>- 			printf("ts%d: addr mod 4 != 0\n", tsunit);
>- 			return (1);
>- 		}
>  		tchar->char_bptr = &sc->sc_sts;
>  		tchar->char_bae = 0;
>  		tchar->char_size = sizeof(struct ts_sts);
>
>--- 558,563 -----
>  	if (tsaddr->tssr & (TS_NBA | TS_OFL)) {
>  		tsaddr->tssr = 0;		/* subsystem initialize */
>  		tswait(tssr);
>  		tchar->char_bptr = &sc->sc_sts;
>  		tchar->char_bae = 0;
>  		tchar->char_size = sizeof(struct ts_sts);
>***************
>*** 570,577
>  		if (tsaddr->tssr & TS_NBA)
>  			return (1);
>  	}
>! 	else
>! 		return(0);
>  }
>  
>  tsread(dev)
>
>--- 571,577 -----
>  		if (tsaddr->tssr & TS_NBA)
>  			return (1);
>  	}
>! 	return(0);
>  }
>  
>  tsread(dev)
>***************
>*** 674,676
>  }
>  #endif	TS_IOCTL
>  #endif	NTS
>
>--- 674,677 -----
>  }
>  #endif	TS_IOCTL
>  #endif	NTS
>+ 


