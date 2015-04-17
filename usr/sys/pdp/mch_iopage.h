/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mch_iopage.h	1.1 (2.10BSD Berkeley) 6/12/88
 */

/*
 *	Access abilities (from seg.h)
 */
#define	RO	02		/* Read only */
#define	RW	06		/* Read and write */

#define	SSR3	0172516		/* Memory Management register 3 */
#define	CCSR	0172540		/* KW11-P Control/Status Register */
#define	CCSB	0172542		/* KW11-P Counter Set Buffer */
#define	SSR0	0177572		/* Memory Management register 0 */
#define	SSR1	0177574		/* Memory Management register 1 */
#define	SSR2	0177576		/* Memory Management register 2 */

#define	STACKLIM 0177774	/* Stack Limit register */
#define	PS	0177776		/* Processor Status register */
#define	PIR	0177772		/* Program Interrupt Request register */

/*
 *	ENABLE/34 registers
 *
 *	All information relevant to the ENABLE/34 is supplied with
 *	the permission of ABLE Computer and may not be disclosed in
 *	any manner to sites not licensed by the University of California
 *	for the Second Berkeley Software Distribution.
 *
 */
#ifdef ENABLE34
#   define	ENABLE_UISA	0163720
#   define	DEC_UISA	0177640
#   ifdef NONSEPARATE
#	define	ENABLE_UDSA	ENABLE_UISA
#	define	DEC_UDSA	DEC_UISA
#   else
#	define	ENABLE_UDSA	0163740
#	define	DEC_UDSA	0177660
#   endif	NONSEPARATE
#   define	ENABLE_KISA0	0163700
#   define	ENABLE_KISA6	0163714
#   define	DEC_KISA0	0172340
#   define	DEC_KISA6	0172354
#   ifdef KERN_NONSEP
#	define	ENABLE_KDSA1	0163702
#	define	ENABLE_KDSA2	0163704
#	define	ENABLE_KDSA5	0163712
#	define	ENABLE_KDSA6	0163714
#	define	DEC_KDSA1	0172342
#	define	DEC_KDSA2	0172344
#	define	DEC_KDSA5	0172352
#	define	DEC_KDSA6	0172354
#   else
#	define	ENABLE_KDSA1	0163762
#	define	ENABLE_KDSA2	0163764
#	define	ENABLE_KDSA5	0163772
#	define	ENABLE_KDSA6	0163774
#	define	DEC_KDSA1	0172362
#	define	DEC_KDSA2	0172364
#	define	DEC_KDSA5	0172372
#	define	DEC_KDSA6	0172374
#   endif	KERN_NONSEP
#   define	ENABLE_SSR4	0163674
#   define	ENABLE_SSR3	0163676
#endif	ENABLE34

/*
 * Supervisor segmentation registers:
 *	SISD: Supervisor Instruction Space Descriptors registers
 *	SDSD: Supervisor Data        Space Descriptors registers
 *	SISA: Supervisor Instruction Space Address     registers
 *	SDSA: Supervisor Data        Space Address     registers
 */
#define	SISD0	0172200
#define	SISD1	0172202
#define	SISD2	0172204
#define	SISD3	0172206
#define	SISD4	0172210
#define	SISD5	0172212
#define	SISD6	0172214
#define	SISD7	0172216

#define	SDSD0	0172220
#define	SDSD1	0172222
#define	SDSD2	0172224
#define	SDSD3	0172226
#define	SDSD4	0172230
#define	SDSD5	0172232
#define	SDSD6	0172234
#define	SDSD7	0172236

#define	SISA0	0172240
#define	SISA1	0172242
#define	SISA2	0172244
#define	SISA3	0172246
#define	SISA4	0172250
#define	SISA5	0172252
#define	SISA6	0172254
#define	SISA7	0172256

#define	SDSA0	0172260
#define	SDSA1	0172262
#define	SDSA2	0172264
#define	SDSA3	0172266
#define	SDSA4	0172270
#define	SDSA5	0172272
#define	SDSA6	0172274
#define	SDSA7	0172276

/*
 * Kernel segmentation registers:
 *	KISD: Kernel Instruction Space Descriptors registers
 *	KDSD: Kernel Data        Space Descriptors registers
 *	KISA: Kernel Instruction Space Address     registers
 *	KDSA: Kernel Data        Space Address     registers
 */
#define	KISD0	0172300
#define	KISD1	0172302
#define	KISD2	0172304
#define	KISD4	0172310
#define	KISD5	0172312
#define	KISD6	0172314
#define	KISD7	0172316

#ifdef KERN_NONSEP
#   define	KDSD0	KISD0
#   define	KDSD5	KISD5
#   define	KDSD6	KISD6
#   define	KDSD7	KISD7
#else
#   define	KDSD0	0172320
#   define	KDSD5	0172332
#   define	KDSD6	0172334
#   define	KDSD7	0172336
#endif

#ifdef ENABLE34
#   define	KISA0	*_KISA0
#else
#   define	KISA0	0172340
#endif
#define	KISA1	0172342
#define	KISA2	0172344
#define	KISA4	0172350
#define	KISA5	0172352
#ifdef ENABLE34
#   define	KISA6	*_KISA6
#else
#   define	KISA6	0172354
#endif
#define	KISA7	0172356

#ifdef KERN_NONSEP
#   define	KDSA0	KISA0
#   ifdef ENABLE34
#	define	KDSA1	*_KDSA1
#	define	KDSA2	*_KDSA2
#	define	KDSA5	*_KDSA5
#	define	KDSA6	*_KDSA6
#   else
#	define	KDSA1	KISA1
#	define	KDSA2	KISA2
#	define	KDSA5	KISA5
#	define	KDSA6	KISA6
#   endif
#   define	KDSA7	KISA7
#else	KERN_NONSEP
#   define	KDSA0	0172360
#   ifdef ENABLE34
#	define	KDSA1	*_KDSA1
#	define	KDSA2	*_KDSA2
#	define	KDSA5	*_KDSA5
#	define	KDSA6	*_KDSA6
#   else
#	define	KDSA1	0172362
#	define	KDSA2	0172364
#	define	KDSA5	0172372
#	define	KDSA6	0172374
#   endif
#   define	KDSA7	0172376
#endif	KERN_NONSEP

/*
 * User segmentation registers:
 *	UISD: User Instruction Space Descriptors registers
 *	UDSD: User Data        Space Descriptors registers
 *	UISA: User Instruction Space Address     registers
 *	UDSA: User Data        Space Address     registers
 */
#ifdef ENABLE34
#   define	UISA	*_UISA
#   define	UDSA	*_UDSA
#else
#   define	UISA	0177640
#   define	UDSA	0177660
#endif
