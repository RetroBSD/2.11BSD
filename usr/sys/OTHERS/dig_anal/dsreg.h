/*
 * DSC System 200 via DSC dma11
 * registers and bits
 */

struct	dsdevice {
	/* dma registers */
	short	dmablr;			/* block length */
	u_short	dmasax;			/* starting address extension */
	u_short	dmasar;			/* starting address */
	u_short	dmacsr;			/* command status */
	short	dmawc;			/* word count */
	u_short	dmaacx;			/* address count extension */
	u_short	dmaac;			/* address count */
	u_short	dmadr;			/* data */
	u_short	dmaiva;			/* interrupt vector */
	u_short	dmacls;			/* clear status */
	u_short	dmaclr;			/* clear device */
	u_short	unused1;

	/* asc registers */
	u_short	ascdis;			/* display register */
	u_short	asccsr;			/* command status register */
	u_short	ascsrt;			/* sample rate register */
	u_short	ascrst;			/* reset */
	u_short	ascseq[16];		/* sequence rams */

	/* converter registers */
	u_short	adc[8];			/* analog to digital converters */
	u_short	dac[8];			/* digital to analog converters */
};

# define bit(x)				((1) << (x))

/*
 * ASC csr bits
 */
# define ASC_RUN	bit(0)		/* run conversion */
# define ASC_HZ20	(0)		/* filter, 20kHz */
# define ASC_HZ10	bit(1)		/* filter, 10kHz */
# define ASC_BYPASS	bit(2)		/* external filter */
# define ASC_HZ05	(bit(1)|bit(2))	/* filter, 5kHz */
# define ASC_RECORD	bit(3)		/* a/d */
# define ASC_PLAY	bit(4)		/* d/a */
# define ASC_BRD	bit(5)		/* broadcast */
# define ASC_MON	bit(6)		/* monitor */
# define ASC_IE		bit(7)		/* interrupt enable */
# define ASC_BA		bit(8)		/* bus abort */
# define ASC_DCN	bit(9)		/* dc not ok */
# define ASC_DNP	bit(12)		/* device not present */
# define ASC_DER	bit(13)		/* device error */
# define ASC_DLT	bit(14)		/* data late */
# define ASC_ERR	bit(15)		/* error */

# define ASC_HZMSK	(bit(1)|bit(2))	/* to turn off all filter bits */

# define ASC_BITS	"\10\01RUN\02HZ10\03BYPASS\04RECORD\05PLAY\06BRD\07MON\10IE\11BA\12DCN\15DNP\16DER\17DLT\20ERR"

/*
 * DMA csr bits
 */
# define DMA_DRF	bit(0)		/* data register full */
# define DMA_AMPS	bit(1)		/* memory port select */
# define DMA_AMPE	bit(2)		/* memory parity error */
# define DMA_XBA	bit(3)		/* external buss abort */
# define DMA_W2M	bit(4)		/* write to memory */
# define DMA_CHN	bit(5)		/* chain */
# define DMA_IE		bit(6)		/* interrupt enable */
# define DMA_BSY	bit(7)		/* busy */
# define DMA_SFL	bit(9)		/* starting address register full */
# define DMA_OFL	bit(10)		/* offline */
# define DMA_XIN	bit(11)		/* external interrupt */
# define DMA_IIN	bit(12)		/* internal interrrupt */
# define DMA_XER	bit(13)		/* external error */
# define DMA_UBA	bit(14)		/* unibus abort */
# define DMA_ERR	bit(15)		/* error */

# define DMA_BITS	"\10\01DRF\02AMPS\03AMPE\04XBA\05W2M\06CHN\07IE\10BSY\12SFL\13OFL\14XIN\15IIN\16XER\17UBA\20ERR"
