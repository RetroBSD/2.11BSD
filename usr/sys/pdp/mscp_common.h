/*
 *	1.3	(2.11BSD) 1999/2/25
 *
 * Definitions common to both MSCP and TMSCP were moved here from tmscp.h.
 * Eventually the MSCP driver and include file will be modified to use these
 * instead of their private versions.
*/

#ifndef	_MSCP_COMMON_H_
#define	_MSCP_COMMON_H_

struct mscp_header {
	u_short	mscp_msglen;	/* length of MSCP packet */
	char	mscp_credits;	/* low 4 bits: credits, high 4 bits: msgtype */
	char	mscp_vcid;	/* virtual circuit id */
};

/*
 * Control message opcodes
*/
#define	M_OP_ABORT	0x01	/* Abort command */
#define	M_OP_GTCMD	0x02	/* Get command status command */
#define	M_OP_GTUNT	0x03	/* Get unit status command */
#define	M_OP_STCON	0x04	/* Set controller characteristics command */
#define	M_OP_ACCNM	0x05	/* Access non volatile memory */
#define	M_OP_SEREX	0x07	/* Serious exceptin message */
#define	M_OP_AVAIL	0x08	/* Available command */
#define	M_OP_ONLIN	0x09	/* Online command */
#define	M_OP_STUNT	0x0a	/* Set unit characteristics command */
#define	M_OP_DTACP	0x0b	/* Determine access paths command */
#define	M_OP_ACCES	0x10	/* Access command */
#define	M_OP_CMPCD	0x11	/* Compare controller data command */
#define	M_OP_ERASE	0x12	/* Erase command */
#define	M_OP_FLUSH	0x13	/* Flush command */
#define M_OP_ERGAP	0x16	/* Erase gap command */
#define	M_OP_COMP	0x20	/* Compare host data command */
#define	M_OP_READ	0x21	/* Read command */
#define	M_OP_WRITE	0x22	/* Write command */
#define	M_OP_WRITM	0x24	/* Write tape mark command */
#define	M_OP_REPOS	0x25	/* Reposition command */
				/* 0x28 thru 0x2f reserved */
#define	M_OP_AVATN	0x40	/* Available attention message */
#define	M_OP_DUPUN	0x41	/* Duplicate unit number attention message */
#define	M_OP_ACPTH	0x42	/* Access path attention message */
#define	M_OP_RWATN	0x43	/* Rewind attention message */
#define	M_OP_END	0x80	/* End message flag */
 
/*
 * Generic command modifiers
 */
#define	M_MD_EXPRS	0x8000		/* Express request */
#define	M_MD_COMP	0x4000		/* Compare */
#define	M_MD_CLSEX	0x2000		/* Clear serious exception */
#define	M_MD_CDATL	0x1000		/* Clear cache data lost exception */
#define	M_MD_SCCHH	0x0800		/* Suppress caching (high speed) */
#define	M_MD_SCCHL	0x0400		/* Suppress caching (low speed) */
#define	M_MD_SECOR	0x0200		/* Suppress error correction */
#define	M_MD_SEREC	0x0100		/* Suppress error recovery */
#define	M_MD_WBKNV	0x0040		/* Write back non volative mem */
#define	M_MD_IMMED	0x0040		/* Immediate completion */
#define	M_MD_WBKVL	0x0020		/* Write back volatile mem */
#define	M_MD_UNLOD	0x0010		/* Unload */
#define	M_MD_REVRS	0x0008		/* reverse */
 
/*
 * Repositioning command modifiers
 */
#define	M_MD_DLEOT	0x0080		/* delete LEOT */
#define	M_MD_OBJCT	0x0004		/* object count */
#define	M_MD_REWND	0x0002		/* rewind */
 
/*
 * Available command modifiers
*/
#define	M_MD_ALLCD	0x0002		/* All class drivers */
#define	M_MD_SPNDW	0x0001		/* Spin down */

/*
 * Flush command modifiers
*/
#define	M_MD_VOLTL	0x0002		/* Volatile memory only */
#define	M_MD_FLENU	0x0001		/* Flush entire unit's memory */

/*
 * Get unit status command modifiers
*/
#define	M_MD_NXUNT	0x0001		/* Get next unit */

/*
 * Online and set unit char command modifiers
*/
#define	M_MD_EXCAC	0x0020		/* Exclusive access */
#define	M_MD_STWRP	0x0004		/* Enable write protect */

/*
 * End message flags
 */
#define	M_EF_ERLOG	0x0020	/* Error log generated */
#define	M_EF_SEREX	0x0010	/* Serious exception */
#define	M_EF_EOT	0x0008	/* End of tape encountered */
#define	M_EF_PLS	0x0004	/* Position lost */
#define	M_EF_DLS	0x0002	/* Cache data lost */
 
/*
 * Controller flags
 */
#define	M_CF_ATTN	0x0080	/* Enable attention messages */
#define	M_CF_MISC	0x0040	/* Enable miscellaneous error log messages */
#define	M_CF_OTHER	0x0020	/* Enable other host's error log messages */
#define	M_CF_THIS	0x0010	/* Enable this host's error log messages */
 
 
/*
 * Unit flags
 */
#define	M_UF_CACH	0x8000		/* Set if caching capable */
#define	M_UF_WRTPH	0x2000		/* Write protect (hardware) */
#define	M_UF_WRTPS	0x1000		/* Write protect (software or volume) */
#define	M_UF_SCCHH	0x0800		/* Suppress caching */
#define	M_UF_EXACC	0x0400		/* Exclusive access */
#define	M_UF_LOADR	0x0200		/* Tape loader present */
#define	M_UF_WRTPD	0x0100		/* Write protect (data safety) */
#define	M_UF_RMVBL	0x0080		/* Removable media */
#define	M_UF_WBKNV	0x0040		/* Write back (enables cache) */
#define	M_UF_VSMSU	0x0020		/* Variable speed mode suppression */
#define	M_UF_VARSP	0x0010		/* Variable speed unit */
#define	M_UF_CACFL	0x0004		/* Cache has been flushed */
#define	M_UF_CMPWR	0x0002		/* Compare writes */
#define	M_UF_CMPRD	0x0001		/* Compare reads */
 
/*
 * Status codes
 */
#define	M_ST_MASK	0x1f		/* Status code mask */
#define	M_ST_SBCOD	0x20		/* sub code multiplier */
#define	M_ST_SBBIT	0x05		/* Sub code starting bit position */

#define	M_ST_SUCC	0x00		/* Success */
#define	M_ST_ICMD	0x01		/* Invalid command */
#define	M_ST_ABRTD	0x02		/* Command aborted */
#define	M_ST_OFFLN	0x03		/* Unit offline */
#define	M_ST_AVLBL	0x04		/* Unit available */
#define	M_ST_MFMTE	0x05		/* Media format error */
#define	M_ST_WRTPR	0x06		/* Write protected */
#define	M_ST_COMP	0x07		/* Compare error */
#define	M_ST_DATA	0x08		/* Data error */
#define	M_ST_HSTBF	0x09		/* Host buffer access error */
#define	M_ST_CNTLR	0x0a		/* Controller error */
#define	M_ST_DRIVE	0x0b		/* Drive error */
#define	M_ST_FMTER	0x0c		/* Formatter error */
#define	M_ST_BOT	0x0d		/* BOT encountered */
#define	M_ST_TAPEM	0x0e		/* Tape mark encountered */
#define	M_ST_RDTRN	0x10		/* Record data truncated */
#define	M_ST_PLOST	0x11		/* Position lost */
#define	M_ST_SEX	0x12		/* Serious exception */
#define	M_ST_LED	0x13		/* LEOT detected */
#define	M_ST_BBR	0x14		/* Bad block replacement complete */
#define	M_ST_IPARM	0x15		/* Invalid parameter */
#define	M_ST_INFO	0x16		/* Informational message, not error */
#define	M_ST_LOADR	0x17		/* Media loader error */
#define	M_ST_DIAG	0x1f		/* Message from internal diagnostic */

/*
 * Success subcodes
*/
#define	M_SC_NORML	0x0000		/* Normal */
#define	M_SC_SDIGN	0x0001		/* Spin down ignored */
#define	M_SC_STCON	0x0002		/* Still connected */
#define	M_SC_DUPUN	0x0004		/* Duplicate unit number */
#define	M_SC_ALONL	0x0008		/* Already online */
#define	M_SC_STONL	0x0010		/* Still online */
#define	M_SC_UNIGN	0x0011		/* Still online/Unload ignored (T) */
#define	M_SC_EOT	0x0020		/* EOT seen */
#ifdef	notnow
#define	M_SC_INREP	0x0020		/* Incomplete replacement (D)
#define	M_SC_IVRCT	0x0040		/* Invalid RCT */
#endif
#define	M_SC_ROVOL	0x0080		/* Read only volume */

/*
 * Invalid command subcodes
*/
#define	M_SC_INVML	0x0000		/* Invalid message length */

/*
 * Data error subcodes
*/
#define	M_SC_LGAP	0x0000		/* Long gap seen */
#define	M_SC_UREAD	0x0007		/* Unrecoverable read err */

/*
 * Unit offline subcodes
*/
#define	M_SC_UNKNO	0x0000		/* Unit unknown */
#define	M_SC_NOVOL	0x0001		/* No volume (turned off) */
#define	M_SC_INOPR	0x0002		/* Unit is inoperative */
#define	M_SC_UDSBL	0x0008		/* Unit disabled */
#define	M_SC_EXUSE	0x0010		/* Unit in use elsewhere */
#define	M_SC_LOADE	0x0020		/* Loader error */

/*
 * Unit available subcodes
*/
#define	M_SC_AVAIL	0x0000		/* Success */
#ifdef	notnow
#define	M_SC_NOMEMB	0x0001		/* No members */
#define	M_SC_ALUSE	0x0020		/* Online to another host */
#endif

/*
 * Write protect subcodes
*/
#define	M_SC_DATL	0x0008		/* Data loss write protected */
#define	M_SC_SOFTW	0x0080		/* Software write protected */
#define	M_SC_HARDW	0x0100		/* Hardware write protected */

/*
 * Invalid parameter sub-codes
*/
#ifdef	notnow
#define	M_SC_IVKLN	0x0001		/* Invalid key length */
#define	M_SC_IVKTP	0x0002		/* Invalid key type */
#define	M_SC_IVKVL	0x0003		/* Invalid key value */

/*
 * Media format error sub-codes
*/
#define M_SC_NO512	0x0005	/* 576 byte sectors on a 512 byte drive	     */
#define	M_SC_UNFMT	0x0006	/* Disk unformatted or FCT corrupted	     */
#define M_SC_RCTBD	0x0008	/* RCT corrupted			     */
#define M_SC_NORBL	0x0009	/* No replacement block available	     */
#define M_SC_MULT	0x000A	/* Multi-copy protection warning	     */

/*
 * Data error sub-codes
 *
 * sub-codes marked (*) may also appear in media format errors
*/
#define M_SC_FRCER	0x0000	/* Forced error (*)			     */
#define M_SC_IVHDR	0x0002	/* Invalid header (*)			     */
#define M_SC_SYNTO	0x0003	/* Data synch timeout (*)		     */
#define M_SC_ECCFL	0x0004	/* Correctable error in ECC field	     */
#define M_SC_UNECC	0x0007	/* Uncorrectable ECC error (*)		     */
#define M_SC_1SECC	0x0008	/* 1 symbol correctable ECC error	     */
#define M_SC_2SECC	0x0009	/* 2 symbol correctable ECC error	     */
#define M_SC_3SECC	0x000a	/* 3 symbol correctable ECC error	     */
#define M_SC_4SECC	0x000b	/* 4 symbol correctable ECC error	     */
#define M_SC_5SECC	0x000c	/* 5 symbol correctable ECC error	     */
#define M_SC_6SECC	0x000d	/* 6 symbol correctable ECC error	     */
#define M_SC_7SECC	0x000e	/* 7 symbol correctable ECC error	     */
#define M_SC_8SECC	0x000f	/* 8 symbol correctable ECC error	     */

/*
 * Host buffer access error sub-codes
 */
#define M_SC_ODDTA	0x0001	/* Odd transfer address			     */
#define M_SC_ODDBC	0x0002	/* Odd byte count			     */
#define M_SC_NXM	0x0003	/* Non-existent memory			     */
#define M_SC_MPAR	0x0004	/* Host memory parity error		     */
#define M_SC_IVPTE	0x0005	/* Invalid Page Table Entry (UQSSP)	     */
#define M_SC_IVBFN	0x0006	/* Invalid buffer name			     */
#define M_SC_BLENV	0x0007	/* Buffer length violation		     */
#define M_SC_ACVIO	0x0008	/* Access violation			     */

/*
 * Controller error sub-codes
 */
#define	M_SC_HDETO	0x0000	/* Host detected controller timeout	     */
#define M_SC_DLATE	0x0001	/* Data late (SERDES) error		     */
#define M_SC_EDCER	0x0002	/* EDC error				     */
#define M_SC_DTSTR	0x0003	/* Data structure error			     */
#define M_SC_IEDC	0x0004	/* Internal EDC error			     */
#define M_SC_LACIN	0x0005	/* LESI adaptor card input error	     */
#define M_SC_LACOU	0x0006	/* LESI adaptor card output error	     */
#define M_SC_LACCB	0x0007	/* LESI adaptor card cable not in place	     */
#define M_SC_OVRUN	0x0008	/* Controller overrun or underrun	     */
#define M_SC_MEMER	0x0009	/* Controller memory error		     */

/*
 * Drive error sub-codes
 */
#define M_SC_CMDTO	0x0001	/* SDI command timed out		     */
#define M_SC_XMSER	0x0002	/* Controller-detected transmission error    */
#define M_SC_MISSK	0x0003	/* Positioner error (mis-seek)		     */
#define M_SC_RWRDY	0x0004	/* Lost read/write ready between transfers   */
#define M_SC_CLKDO	0x0005	/* Drive clock dropout			     */
#define M_SC_RXRDY	0x0006	/* Lost receiver ready between sectors	     */
#define M_SC_DRDET	0x0007	/* Drive-detected error			     */
#define	M_SC_PULSE	0x0008	/* Ctlr-detected pulse/state parity error    */
#define M_SC_PRTCL	0x000a	/* Controller detected protocol error	     */
#define	M_SC_FLINI	0x000b	/* Drive failed initialization		     */
#define	M_SC_IGINI	0x000c	/* Drive ignored initialization		     */
#define	M_SC_RRCOL	0x000d	/* Receiver ready collision		     */

/*
 * Informational event only subcodes
*/
#define M_SC_IQUAL	0x0001	/* Media Quality Log			     */
#define M_SC_ISTAT	0x0002	/* Unload, spin down statistics		     */

#endif /* notnow */

/*
 * Error Log message format codes.  Many of these are ifdef'd out so as to
 * not overload the C preprocessor.  The symbols themselves are not used by
 * the kernel but are very handy to have when deciphering datagrams logged
 * to the console or the messages file.
 */
#define	M_FM_CNTERR	0	/* Controller error */
#define	M_FM_BUSADDR	1	/* Host memory access error */
#define	M_FM_DISKTRN	2	/* Disk transfer error (D) */
#define	M_FM_SDI	3	/* SDI errors */
#define	M_FM_SMLDSK	4	/* Small disk errors */
#define	M_FM_TAPETRN	5	/* Tape transfer error (T) */
#define	M_FM_STIERR	6	/* STI communication or command failure (T) */
#define	M_FM_STIDEL	7	/* STI drive error log (T) */
#define	M_FM_STIFEL	0x8	/* STI formatter error log (T) */
#ifdef	notnow
#define	M_FM_REPLACE	0x9	/* Bad block replacement attempt */
#define	M_FM_LDRERR	0xa	/* Media loader errors */
#define	M_FM_IBMSENSE	0xb	/* Sense data error log (T) */
#endif
 
/*
 * Error Log message flags
 */
#define	M_LF_SUCC	0x80	/* Operation successful */
#define	M_LF_CONT	0x40	/* Operation continuing */
#ifdef	notnow
#define	M_LF_BBR	0x20	/* Bad block replacement attempt */
#define	M_LF_RPLER	0x10	/* Error during replacement */
#define	M_LF_INFO	0x02	/* Informational */
#endif
#define	M_LF_SQNRS	0x01	/* Sequence number reset */
 
/*
 * Tape Format Flag Values
 */
#define	M_TF_MASK	0x00ff	/* Density bits */
#define	M_TF_CODE	0x0100	/* Format code multiplier */
#define	M_TF_800	0x01	/* NRZI 800 bpi */
#define	M_TF_PE		0x02	/* Phase Encoded 1600 bpi */
#define	M_TF_GCR	0x04	/* Group Code Recording 6250 bpi */
#define	M_TF_BLK	0x08	/* Cartridge Block Mode */

#ifdef	notnow
/*
 * Define a few of the common controller and drive types for reference but
 * don't actually force the preprocessor to handle even more defines.
*/
#define	M_CM_UDA50	2
#define	M_CM_TU81	5
#define	M_CM_UDA50A	6
#define	M_CM_TK50	9
#define	M_CM_TQK50	9
#define	M_CM_TK70	14
#define	M_CM_TQK70	14
#define	M_CM_RQDX3	19
#endif

#endif	/* _MSCP_COMMON_H_ */
