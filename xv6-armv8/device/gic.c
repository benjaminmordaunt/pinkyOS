#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "memlayout.h"
#include "mmu.h"


static volatile uint* gic_base;

#define SGI_TYPE		1
#define PPI_TYPE		2
#define SPI_TYPE		3

#define GICD_CTLR		0x000
#define GICD_TYPER		0x004
#define GICD_IIDR		0x008

#define GICD_IGROUP		0x080
#define GICD_ISENABLE		0x100
#define GICD_ICENABLE		0x180
#define GICD_ISPEND		0x200
#define GICD_ICPEND		0x280
#define GICD_ISACTIVE		0x300
#define GICD_ICACTIVE		0x380
#define GICD_IPRIORITY		0x400
#define GICD_ITARGET		0x800
#define GICD_ICFG		0xC00

#define GICC_CTLR		0x000
#define GICC_PMR		0x004
#define GICC_BPR		0x008
#define GICC_IAR		0x00C
#define GICC_EOIR		0x010
#define GICC_RRR		0x014
#define GICC_HPPIR		0x018

#define GICC_ABPR		0x01C
#define GICC_AIAR		0x020
#define GICC_AEOIR		0x024
#define GICC_AHPPIR		0x028

#define GICC_APR		0x0D0
#define GICC_NSAPR		0x0E0
#define GICC_IIDR		0x0FC
#define GICC_DIR		0x1000 /* v2 only */



#define GICD_REG(o)		(*(uint64 *)(((uint64) gic_base) + o))
#define GICC_REG(o)		(*(uint64 *)(((uint64) gic_base) + 0x10000 + o))

/*  id is m
 *  offset n= m DIV 32
 *  bit    pos = m MOD 32;
 */
static void gicd_set_bit(int base, int id, int bval) {
	int offset = id/32;
	int bitpos = id%32;
	uint rval = GICD_REG(base+4*offset);
	if(bval)
		rval |= 1 << bitpos;
	else
		rval &= ~(1<< bitpos);
	GICD_REG(base+ 4*offset) = rval;
}


void gicc_set_bit(int base, int id, int bval) {
	int offset = id/32;
	int bitpos = id%32;
	uint rval = GICC_REG(base+4*offset);
	if(bval)
		rval |= 1 << bitpos;
	else
		rval &= ~(1<< bitpos);
	GICC_REG(base+ 4*offset) = rval;
}

static int spi2id(int spi)
{
	return spi+32;
}

static void gd_spi_enable(int spi)
{
	int id = spi2id(spi);
	gicd_set_bit(GICD_ISENABLE, id, 1);
}

/* By default, SPI is group 0
 * GIC spec ch4.3.4
 *
 */
static void gd_spi_group0(int spi)
{
	return;
}

/* set target processor
 */
static void gd_spi_target0(int spi)
{
	int id=spi2id(spi);
	int offset = id/4;
	int bitpos = (id%4)*8;
	uint rval = GICD_REG(GICD_ITARGET+4*offset);
	unsigned char tcpu=0x01;
	rval |= tcpu << bitpos;
	GICD_REG(GICD_ITARGET+ 4*offset) = rval;	
}

/* set cfg
 */
static void gd_spi_setcfg(int spi, int is_edge)
{
	int id=spi2id(spi);
	int offset = id/16;
	int bitpos = (id%16)*2;
	uint rval = GICD_REG(GICD_ICFG+4*offset);
	uint vmask=0x03;
	rval &= ~(vmask << bitpos);
	if (is_edge)
		rval |= 0x02 << bitpos;
	GICD_REG(GICD_ICFG+ 4*offset) = rval;	
}

/*
 * TODO: process itype other than SPI
 */
static void gic_dist_configure(int itype, int num)
{
	int spi= num;
	gd_spi_setcfg(spi, 1);
	gd_spi_enable(spi);
	gd_spi_group0(spi);
	gd_spi_target0(spi);
}

/*
 * initial every spi pin 
 */
static void gic_dist_init() 
{
	/* cprintf("Found gic type: 0x%x\n", GICD_REG(GICD_TYPER)); here is 0x4 **/
}

/* HCLIN: the intc does not work until set the priority mask 
 * for a intc
 *   enable int pin, 
 *   set pin trigger type, 
 *   enable int global, 
 *   set mask
 *
 */
static void gic_cpu_init() 
{
	cprintf("gic cpuif type:0x%x\n", GICC_REG(GICC_IIDR));
	GICC_REG(GICC_PMR) = 0x0f; /* priority value 0 to 0xe is supported */
}


/* enable group 0 only
 */
static void gic_enable()
{
	GICD_REG(GICD_CTLR) |= 1;
	GICC_REG(GICC_CTLR) |= 1;
}

/* disable group 0 only
 */
void gic_disable()
{
	GICD_REG(GICD_CTLR) &= ~(uint)1;
	GICD_REG(GICC_CTLR) &= ~(uint)1;
}
/* configure and enable interrupt
 */
static void gic_configure(int itype, int num)
{
	gic_dist_configure(itype, num);
}

void gic_eoi(int intn)
{
	GICC_REG(GICC_EOIR) = spi2id(intn);
}

int gic_getack()
{
	return GICC_REG(GICC_IAR);
}

/* ISR code */
#define NUM_INTSRC		32 // numbers of interrupt source supported

static ISR isrs[NUM_INTSRC];

static void default_isr (struct trapframe *tf, int n)
{
    cprintf ("unhandled interrupt: %d\n", n);
}


void pic_enable (int n, ISR isr)
{
	if(n < NUM_INTSRC) {
		isrs[n] = isr;
	}
}

void isr_init()
{
	int i;
	for (i=0; i< NUM_INTSRC; i++)
		isrs[i] = default_isr;
}
/*
 * This section init gic according to CORTEX A15 reference manual
 * 8.3.1 distributor register and 8.3.2 
 */
void gic_init(void * base)
{
	gic_base = base;
	gic_dist_init();
	gic_cpu_init();
	isr_init();

	gic_configure(SPI_TYPE, PIC_TIMER01);
	gic_configure(SPI_TYPE, PIC_TIMER23);
	gic_configure(SPI_TYPE, PIC_UART0);

	gic_enable();

}

/*
 * dispatch the interrupt
 */
void pic_dispatch (struct trapframe *tp)
{
	int intid, intn;
	intid = gic_getack(); /* iack */
	intn = intid - 32;
	/* TODO: int disable here? **/
	isrs[intn](tp, intn);
	gic_eoi(intn);
}

