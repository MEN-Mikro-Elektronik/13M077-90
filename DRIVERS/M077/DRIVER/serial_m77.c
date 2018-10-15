/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  serial_m77_26.c
 *
 *      \author  thomas schnuerer
 *        $Date: 2015/03/06 13:06:07 $
 *    $Revision: 1.21 $
 * 
 *       \brief  Kernel 2.6 driver for M77 modules 
 *               Attention: Instanciate carrier Board MDIS device prior to
 *				 modprobe'ing this driver!
 *
 * Features
 * - All standard baudrates up to 1152000 Baud ( only RS422/485 with > 115200 )
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: serial_m77_26.c,v $
 * Revision 1.21  2015/03/06 13:06:07  ts
 * R: 1. compiler warning about type cast of mapbase
 *    2. RTAI support in MDIS was removed
 * M: 1. added explicit (int) cast of mapbase
 *    2. removed first parameter from call to bbis_open_external_device
 *
 * Revision 1.20  2014/07/29 16:21:25  ts
 * R: compile under 64bit linux (Fedora19) showed warnings
 * M: fixed parameter passing to m_getmodinfo, corrected casting
 *
 * Revision 1.19  2014/07/15 10:40:27  ts
 * R: compile failed under kernel 3.10.11
 * M: apply API change in tty_flip_buffer_push, pass struct tty_port
 *
 * Revision 1.18  2013/10/24 10:23:23  ts
 * R: gcc 4.6 compile for A21 failed because kmalloc/kfree were unknown
 * M: include <linux/slab.h>
 *
 * Revision 1.17  2012/12/19 13:49:59  ts
 * R: DEFINE_SEMAPHORE was renamed to DECLARE_MUTEX from 2.6.35 on
 * M: made define kernel version dependent
 *
 * Revision 1.16  2010/11/08 19:16:46  rt
 * R: 1) Only the first M77 module (per carrier board) works correct.
 * M: 1) Fixed m77_init_devices() to support multiple M77 modules at the
 *       same carrier board.
 *
 * Revision 1.15  2010/11/05 17:37:57  rt
 * R: 1) M77: Half duplex modes not working if echo is off.
 *    2) Not compilable with kernel version 2.6.32.
 *    3) Support more modules per default.
 *    4) Cosmetics.
 * M: 1) register_uarts() function fixed.
 *    2) Added compiler switches for new kernel versions.
 *    3) MAX_MODS_SUPPORTED and MAX_SNGL_UARTS changed.
 *    4.a) Added/changed some debug messages.
 *      b) Renamed some constants.
 *
 * Revision 1.14  2010/02/11 15:46:13  gvarlet
 * R: Linux disabled associated IRQ after 100000 cycles if any other shared interupt is triggered because M77_IrqHandler always returned LL_IRQ_DEV_NOT.
 * M: M77_IrqHandler now return LL_IRQ_DEVICE when M77 receive an IRQ.
 *
 * Revision 1.13  200n8/05/05 18:03:54  ts
 * - replaced all calls to readb()/writeb() to MREAD_D16/MWRITE_D16 because
 *   linux' bytewise native mem-IO functions dont work with VME carriers.
 * - moved all HW access in 4 basic functions serial_in/out, control_in/out
 * - added revision string for version tracking
 *
 * Revision 1.12  2007/08/28 10:56:11  ts
 * at ElinOS 4.1 S1317 use MEN_UART_MAJOR instead CYCLADES_MAJOR
 *
 * Revision 1.11  2007/08/27 10:25:25  ts
 * Echo setting moved to men_uart_startup
 *
 * Revision 1.10  2007/08/15 16:25:29  ts
 * unnecessary member ser_struct removed
 * build problem under SuSE 10.0 fixed
 *
 * Revision 1.9  2007/08/14 16:33:13  ts
 * fixed:
 * reference to tty.flip removed in receive_chars (error on Ubuntu 2.6.17)
 *
 * Revision 1.8  2007/08/14 14:24:24  ts
 * all doxygen conformant function headers added
 *
 * Revision 1.7  2007/08/06 19:34:02  ts
 * work Checkin
 * fixed: Dont set Bit DRV_EN for M77 Modules to 0 again in M77_Irq()
 *
 * Revision 1.6  2007/08/06 11:35:40  ts
 * added struct members for DCR and TCR addresses to
 * be independent from assigned UART line number
 *
 * Revision 1.5  2007/08/03 16:11:05  ts
 * work checkin, ioctl handling for M45n added
 *
 * Revision 1.4  2007/06/29 15:35:31  ts
 * work checkin, added ioctl handler, Handshakes tested (stty)
 *
 * Revision 1.3  2007/06/26 17:14:03  ts
 * first working revision, more unnecessary code removed
 *
 * Revision 1.2  2007/06/25 17:09:36  ts
 * work checkin, compiles and detects Oxford 16C954 Chip
 *
 * Revision 1.1  2007/06/14 14:22:47  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2007 by MEN mikro elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/


#include <linux/moduleparam.h>
#include <linux/serial_core.h>  /* for struct uart_port 	*/
#include <linux/serial_reg.h>
#include <linux/version.h>
#include <linux/serial.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>			/* linked list functions	*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
# include <linux/config.h>
#endif
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include "serial_m77.h"
#include <linux/slab.h>
#include <asm/io.h>

/* MDIS stuff */
#include <MEN/men_typs.h>
#include <MEN/desctyps.h>
#include <MEN/ll_defs.h>
#include <MEN/oss.h>
#include <MEN/maccess.h>
#include <MEN/mk_nonmdisif.h>
#include <MEN/mdis_com.h>
#include <MEN/modcom.h>

/*-----------------------------+
|   DEFINES                    |
+-----------------------------*/


#define Z025_SERIAL_DIFF    KERNEL_VERSION(2,6,14)

#define MM_UARTCLK			18432000	/* 18,432 MHz 				 */
#define	MAX_MODS_SUPPORTED  8			/* up to # Modules supported */

/* This is the total nr. of UARTS, can be e.g. 8xM45N or 8xM77/M69N  */
#define MAX_SNGL_UARTS				64			

#define UART_NAME_PREFIX	"ttyD"		/* ttyD0 to ttyDnn 			 */
#define ARRLEN 	16

#define UART_CAP_FIFO		(1 << 8)	/* UART has FIFO 					*/
#define UART_CAP_EFR		(1 << 9)	/* UART has EFR 					*/
#define UART_CAP_SLEEP		(1 << 10)	/* UART has IER sleep 				*/
#define UART_CAP_AFE		(1 << 11)	/* MCR-based hw flow control 		*/
#define UART_CAP_UUE		(1 << 12)	/* UART needs IER bit 6 set (Xscal) */
#define UART_BUG_QUOT		(1 << 0)	/* UART has buggy quot LSB 			*/
#define UART_BUG_TXEN		(1 << 1)	/* UART has buggy TX IIR status 	*/
#define UART_BUG_NOMSR		(1 << 2)	/* UART has buggy MSR status bits 	*/
#define HIGH_BITS_OFFSET 	((sizeof(long)-sizeof(int))*8)

/*
 * Different Debug Facilities, define Value below as needed in case of
 * Problems
 */
#define M77_DBG_LEVEL	0

#if M77_DBG_LEVEL == 0
#  define M77DBG(x...) 			do { } while (0)
#  define M77DBG2(x...) 		do { } while (0)
#  define M77DBG3(x...) 		do { } while (0)
#elif M77_DBG_LEVEL == 1
#  define M77DBG(x...) 			printk(x)
#  define M77DBG2(x...) 		do { } while (0)
#  define M77DBG3(x...) 		do { } while (0)
#elif M77_DBG_LEVEL == 2
#  define M77DBG(x...) 			printk(x)
#  define M77DBG2(x...) 		printk(x)
#  define M77DBG3(x...) 		do { } while (0)
#elif M77_DBG_LEVEL == 3
#  define M77DBG(x...) 			printk(x)
#  define M77DBG2(x...) 		printk(x)
#  define M77DBG3(x...) 		printk(x)
#endif

#if 0 /* in-ISR debugs, from 8250.c */
#define DEBUG_INTR(fmt...)	printk(fmt)
#else
#define DEBUG_INTR(fmt...)	do { } while (0)
#endif


/*-----------------------------+
|   TYPEDEFS                   |
+-----------------------------*/


/*******************************************************************/
/** This replaces serial_uart_config in include/linux/serial.h
 */
struct men_uart_config {
	const char		*name;
	unsigned short	fifo_size;
	unsigned short	tx_loadsz;
	unsigned char	fcr;
	unsigned int	flags;
};


/*******************************************************************/
/** This is the platform device platform_data structure 
 */
struct plat_men_uart_port {
	unsigned long	iobase;		/* io base address 		*/
	void __iomem	*membase;	/* ioremap cookie or NULL 	*/
	unsigned long	mapbase;	/* resource base 		*/
	unsigned int	irq;		/* interrupt number 		*/
	unsigned int	uartclk;	/* UART clock rate 		*/
	unsigned char	regshift;	/* register shift 		*/
	unsigned char	iotype;		/* UPIO_*	 		*/
	unsigned char	hub6;
        unsigned int	flags;		/* UPF_* flags 			*/
};


/*******************************************************************/
/** The central Oxford 16C950 UART port struct 
 */
struct ox16c954_port {
	struct uart_port	port;
	struct timer_list	timer;			/* "no irq" timer 				*/
	struct list_head	list;			/* ports on this IRQ 			*/
	unsigned short		capabilities;	/* port capabilities 			*/
	unsigned short		bugs;			/* port bugs 					*/
	unsigned int		tx_loadsz;		/* transmit fifo load size 		*/
	unsigned char		acr;			/* Advanced Control Register	*/ 
	unsigned char		ier;
	unsigned char		lcr;
	unsigned char		mcr;
	unsigned char		mcr_mask;		/* mask of user bits 			*/
	unsigned char		mcr_force;		/* mask of forced bits 			*/
	unsigned char		lsr_break_flag;

	/* Additional 16C954 & M-Module maintenance stuff */
	unsigned char		efr;
	unsigned int		type;		/* Type MOD_M45N/MOD_M69N/MOD_M77	*/
	unsigned int		dcrReg;		/* M77:	DCR adress of this Uart		*/
	unsigned int		tcrReg;		/* M45N: TCR adress of this Uart	*/
	unsigned int		tcrBit;		/* M45N: TCR Bit for this Channel	*/
	unsigned int		acrShadow;	/* keep M77 ACR (DTR#) setting		*/
	unsigned int		m77Mode;	/* M77: PHY Mode setting			*/

	/*
	 * We provide a per-port pm hook.
	 */
	void	(*pm)(struct uart_port *port, 
				  unsigned int state, unsigned int old);
};


/*******************************************************************/
/** Maintain central per-M-Module Info about each M45N/M69N/M77
 *  For M77/M69N 4 structs are allocated, for M45N 8
 */
typedef struct uartmod {
    struct list_head head;			/* for linked list			*/
	unsigned int  	modnum;			/* nr. in list 				*/
	unsigned int  	modtype;		/* MOD_M45, MOD_M69 or MOD_M77		*/
	unsigned int  	nrChannels;		/* M77/M69N: 4, M45N: 8			*/
	unsigned int  	irq;			/* (PCI)IRQ of this Modules Carrier	*/
	unsigned int  	line;			/* UART # from registering UART		*/
	unsigned int  	mode[4];		/* M77: PHY mode passed at loading	*/
	unsigned int  	echo[4];		/* M77: echo on/off (HD modes)		*/
	char 		brdName[ARRLEN];	/* carrier name e.g. "D201_1" 		*/
	char 		deviceName[ARRLEN];	/* dev. name e.g. "m45_1" 		*/
	void 		*mdisDev;		/* from mdis_open_external_device 	*/
	void		*memBase;		/* ioremapped address of Module 	*/
  struct uart_port uart;
  struct ox16c954_port *port8250[MAX_SNGL_UARTS];

} UARTMOD_INFO;


/*-----------------------------+
|   MODULE PARAMETERS          |
+-----------------------------*/

static char* brdName[MAX_MODS_SUPPORTED];
static char* devName[MAX_MODS_SUPPORTED];
static int   slotNo[MAX_MODS_SUPPORTED];
static int   mode[MAX_MODS_SUPPORTED*4];
static int   echo[MAX_MODS_SUPPORTED*4];

/* Array Element count at load time */
static int   arr_argc = MAX_MODS_SUPPORTED;

/* 
 * We have to pass the MDIS Device Names for MDIS_FindDevByName, 
 * called within mdis_open_external_device() 
 */
module_param_array(devName, charp, 	&arr_argc, 	0 );
MODULE_PARM_DESC(devName, "MDIS Descriptor Module Name ('m77_1','m45_2' etc)");
module_param_array(brdName, charp, 	&arr_argc, 	0 );
MODULE_PARM_DESC( brdName, 	"MDIS Descriptor Carrier Name, e.g. 'd201_1'");
module_param_array(slotNo, int, &arr_argc, 	0 );
MODULE_PARM_DESC( slotNo, "slot number on carrier board e.g. 0..3 on D201" );
module_param_array(mode, int, 	&arr_argc, 	0 );
MODULE_PARM_DESC( mode, "on M77: phy mode of channel 0-3, e.g. '1,1,7,7'" );
module_param_array(echo, int, &arr_argc, 	0 );
MODULE_PARM_DESC( echo, "on M77: disable / enable Rx feedback in HD modes");

/*-----------------------------+
|   GLOBALS                    |
+-----------------------------*/

/* linked List Anchor */
static struct list_head		G_uartModListHead;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
static DEFINE_SEMAPHORE(serial_sem);
#else 
static DECLARE_MUTEX(serial_sem);
#endif

static const char RCSid[]="$Id: serial_m77_26.c,v 1.21 2015/03/06 13:06:07 ts Exp $";

/*-----------------------------+
|  PROTOTYPES                  |
+-----------------------------*/

static unsigned int men_uart_tx_empty(struct uart_port *port);
static void men_uart_set_mctrl(struct uart_port *port, unsigned int mctrl);
static unsigned int men_uart_get_mctrl(struct uart_port *port);
#if LINUX_VERSION_CODE < Z025_SERIAL_DIFF
static void men_uart_stop_tx(struct uart_port *port, unsigned int tty_stop);
static void men_uart_start_tx(struct uart_port *port, unsigned int tty_start);
#else
static void men_uart_stop_tx(struct uart_port *port);
static void men_uart_start_tx(struct uart_port *port);
#endif

static void men_uart_stop_rx(struct uart_port *port);
static void men_uart_enable_ms(struct uart_port *port);
static void men_uart_break_ctl(struct uart_port *port, int break_state);
static int men_uart_startup(struct uart_port *port);
static void men_uart_shutdown(struct uart_port *port);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
static void men_uart_set_termios(struct uart_port *port,
								 struct termios *termios, 
								 struct termios *old);
#else
static void men_uart_set_termios(struct uart_port *port,
								 struct ktermios *termios, 
								 struct ktermios *old);
#endif

static void men_uart_pm(struct uart_port *port, unsigned int state,
						unsigned int oldstate);
static void men_uart_release_port(struct uart_port *port);
static int men_uart_request_port(struct uart_port *port);
static void men_uart_config_port(struct uart_port *port, int flags);
static const char *men_uart_type(struct uart_port *port);
static int men_uart_verify_port(struct uart_port *port,
								struct serial_struct *ser);
static int men_uart_ioctl(struct uart_port *up, unsigned int cmd, 
						  unsigned long arg);

static int register_uarts(UARTMOD_INFO*);


/*******************************************************************/
/** basic UART Port definitions
 */
static const struct men_uart_config uart_config[] = {
	[PORT_UNKNOWN] = {
		.name		= "unknown",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_8250] = {
		.name		= "8250",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_16450] = {
		.name		= "16450",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_16550] = {
		.name		= "16550",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_16550A] = {
		.name		= "16550A",
		.fifo_size	= 16,
		.tx_loadsz	= 16,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO,
	},
	[PORT_CIRRUS] = {
		.name		= "Cirrus",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_16650] = {
		.name		= "ST16650",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
		.flags		= UART_CAP_FIFO | UART_CAP_EFR | UART_CAP_SLEEP,
	},
	[PORT_16650V2] = {
		.name		= "ST16650V2",
		.fifo_size	= 32,
		.tx_loadsz	= 16,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_01 |
				  UART_FCR_T_TRIG_00,
		.flags		= UART_CAP_FIFO | UART_CAP_EFR | UART_CAP_SLEEP,
	},
	[PORT_16750] = {
		.name		= "TI16750",
		.fifo_size	= 64,
		.tx_loadsz	= 64,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10 |
				  UART_FCR7_64BYTE,
		.flags		= UART_CAP_FIFO | UART_CAP_SLEEP | UART_CAP_AFE,
	},
	[PORT_STARTECH] = {
		.name		= "Startech",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[PORT_16C950] = {	/* this one is for M45/69/77 */
		.name		= "16C950/954",
		.fifo_size	= 128,
		.tx_loadsz	= 128,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO,/* | UART_CAP_EFR | UART_CAP_SLEEP, */
	},
	[PORT_16654] = {
		.name		= "ST16654",
		.fifo_size	= 64,
		.tx_loadsz	= 32,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_01 |
				  UART_FCR_T_TRIG_10,
		.flags		= UART_CAP_FIFO | UART_CAP_EFR | UART_CAP_SLEEP,
	},
	[PORT_16850] = {
		.name		= "XR16850",
		.fifo_size	= 128,
		.tx_loadsz	= 128,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO | UART_CAP_EFR | UART_CAP_SLEEP,
	},
	[PORT_RSA] = {
		.name		= "RSA",
		.fifo_size	= 2048,
		.tx_loadsz	= 2048,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_11,
		.flags		= UART_CAP_FIFO,
	},
	[PORT_NS16550A] = {
		.name		= "NS16550A",
		.fifo_size	= 16,
		.tx_loadsz	= 16,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO | UART_NATSEMI,
	},
	[PORT_XSCALE] = {
		.name		= "XScale",
		.fifo_size	= 32,
		.tx_loadsz	= 32,
		.fcr		= UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10,
		.flags		= UART_CAP_FIFO | UART_CAP_UUE,
	},
};




/*******************************************************************/
/** basic UART Register read function
 *
 * \param up		\IN Oxford 16C954 Port Struct
 * \param offset	\IN Register offset for address
 *
 * \return 			Value read from given Register
 */
static inline unsigned int serial_in(struct ox16c954_port *up, int offset)
{
	unsigned char val = MREAD_D16(up->port.membase, offset << 1 ) & 0x00ff;
	M77DBG3("serial_in: Adr %02x = %02x\n", offset << 1, val );
	return val;
}




/*******************************************************************/
/** basic UART Register write function
 *
 * \param up		\IN Oxford 16C954 Port Struct
 * \param offset	\IN Register offset for address
 * \param value		\IN Value to write to Register
 *
 * \return 			-
 */
static inline void serial_out(struct ox16c954_port *up, int offset,	int value)
{
	M77DBG3("serial_out: wr 0x%02x to adr %02x\n", value, offset<<1);
	MWRITE_D16(up->port.membase, offset << 1, value); 
}


/*******************************************************************/
/** basic UART Register read function
 *
 * \param base		\IN  base control register address to read from
 * \param offset	\IN Register offset for this address
 *
 * \return 			Value read from given Register
 */
static inline unsigned int control_in(char *base, int offset)
{
	unsigned char val = MREAD_D16(base, offset) & 0x00ff;
	M77DBG3("control_in: Adr %02x = %02x\n", offset, val );
	return val;
}



/*******************************************************************/
/** basic UART control Register write function
 *
 * \param up		\IN Oxford 16C954 Port Struct
 * \param offset	\IN Register offset for address
 * \param value		\IN Value to write to Register
 *
 * \return 			-
 */
static inline void control_out(char *base, int offset,	int value)
{
	M77DBG3("control_out: wr 0x%02x to adr %02x\n", value, offset);
	MWRITE_D16(base, offset, value); 
}


/*******************************************************************/
/** Read from 650 compatible Register indexed through EFR
 *
 * \param up		\IN Oxford 16C954 Port Struct
 * \param offset	\IN Register offset for address, to be shifted by 1
 *
 * \return 			Value read from EFR
 */
static unsigned char serial_efr_read(struct ox16c954_port *up, int offset)
{

	unsigned char oldLcr = 0, efr = 0;

	/* 1. store old lcr */
	oldLcr = serial_in(up, UART_LCR);

	/* 2. write access code 0xbf to lcr */
	serial_out(up, UART_LCR, 0xbf );

	/* 3. read desired Register */
	efr = serial_in(up, offset);

	M77DBG3("%s: read 0x%02x from Reg. 0x%02x\n", __FUNCTION__,efr, offset<<1);

	/* 4. restore lcr */
	serial_out(up, UART_LCR, oldLcr );

	return (efr);

}



/*******************************************************************/
/** Write to 650 compatible Register indexed through EFR
 *
 * \param up		\IN Oxford 16C954 Port Struct
 * \param offset	\IN Register offset for address, to be shifted by 1
 * \param value		\IN Value to write to Register
 *
 * \return 			-
 */
static void serial_efr_write(struct ox16c954_port *up, int offset, int value)
{
	
	unsigned char oldLcr = 0;

	M77DBG3("%s: write 0x%02x to Reg 0x%02x\n",
			__FUNCTION__, value, offset << 1);

	/* 1. store old lcr.. */
	oldLcr = serial_in(up, UART_LCR);

	/* 2. write access code 0xbf to lcr */
	serial_out(up, UART_LCR, 0xbf );

	/* 3. write value to desired Register */
	serial_out(up, offset, value);

	/* 4. restore lcr */
	serial_out(up, UART_LCR, oldLcr );

}



/*******************************************************************/
/** Control Software XON/XOFF handshaking in EFR
 *
 * \param up		\IN Oxford 16C954 Port Struct
 * \param en		\IN 0: disable XON/XOFF Flowcontrol else enable it
 *
 * \return 			-
 */
static void set_inband_flowctrl(struct ox16c954_port *up, unsigned char en)
{

	unsigned char efr = serial_efr_read(up, M77_EFR_OFFSET) & 0xF0;
	
	if (en) 
		efr |=0xa;  /* bits xxxx 1010 enable it*/
	
	M77DBG3("set_inband_flowctrl: Setting EFR = 0x%02x\n", efr);
	serial_efr_write(up, M77_EFR_OFFSET, efr);
}


/*******************************************************************/
/** Write to 16C950 Indexed Control Register set
 *
 * \param up		\IN Oxford 16C954 Port Struct
 * \param offset	\IN Register offset for address
 * \param value		\IN Register Value to write (see Datasheet p.20)
 *
 * \return 			-
 */
static void serial_icr_write(struct ox16c954_port *up, int offset, int value)
{
	M77DBG3("%s: write 0x%02x to Reg 0x%02x\n",	__FUNCTION__, value, offset);
	serial_out(up, UART_SCR, offset);
	serial_out(up, UART_ICR, value);
}


/*******************************************************************/
/** Read from an 16C950 Indexed Control Register
 *
 * \param up		\IN Oxford 16C954 Port Struct
 * \param offset	\IN Register offset for address
 *
 * \return 			Value read from indexed Register
 */
static unsigned int serial_icr_read(struct ox16c954_port *up, int offset)
{
	unsigned int value;

	serial_icr_write(up, UART_ACR, up->acr | UART_ACR_ICRRD);
	serial_out(up, UART_SCR, offset);
	value = serial_in(up, UART_ICR);
	serial_icr_write(up, UART_ACR, up->acr );
	M77DBG3("%s: read 0x%02x from Reg. 0x%02x\n",__FUNCTION__, value, offset);
	
	return value;
}


/*******************************************************************/
/** Clear UART FIFOs
 *
 * \param up		\IN Oxford 16C954 Port Struct
 *
 * \return 			-
 */
static inline void men_uart_clear_fifos(struct ox16c954_port *up)
{
	if (up->capabilities & UART_CAP_FIFO) {
		serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO);
		serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO |
			       UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
		serial_out(up, UART_FCR, 0);
	}
}

/*******************************************************************/
/** IER sleep support, Unused in this driver
 *
 * \param up		\IN Oxford 16C954 Port Struct
 * \param sleep		\IN state of sleep mode
 *
 * \return 			-
 */
static inline void men_uart_set_sleep(struct ox16c954_port *up, int sleep)
{
	/* M45N/M69N/M77 never sleep */
	do { } while (0);
}

 
/*******************************************************************/
/** Main serial driver struct 
 */
static struct uart_driver men_uart_reg = {
	.owner			= THIS_MODULE,
	.driver_name	= "serM77",
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
	.devfs_name		= "tts/",  		/* obsolete from 2.6.15 on! */
#endif
	.dev_name		= UART_NAME_PREFIX,
#ifdef CYCLADES_MAJOR
	.major			= CYCLADES_MAJOR,
#else
	.major			= MEN_UART_MAJOR,
#endif
	.minor			= 0,
	.nr				= MAX_SNGL_UARTS,
	.cons			= NULL/* MEN_UART_CONSOLE */,
};


/*******************************************************************/
/** The HW-dependent port operations called by the core driver
 */
static struct uart_ops men_uart_pops = {
	.tx_empty		= men_uart_tx_empty,
	.set_mctrl		= men_uart_set_mctrl,
	.get_mctrl		= men_uart_get_mctrl,
	.stop_tx		= men_uart_stop_tx,
	.start_tx		= men_uart_start_tx,
	.stop_rx		= men_uart_stop_rx,
	.enable_ms		= men_uart_enable_ms,
	.break_ctl		= men_uart_break_ctl,
	.startup		= men_uart_startup,
	.shutdown		= men_uart_shutdown,
	.set_termios	= men_uart_set_termios,
	.pm				= men_uart_pm,
	.type			= men_uart_type,
	.ioctl 			= men_uart_ioctl,
	.release_port	= men_uart_release_port,
	.request_port	= men_uart_request_port,
	.config_port	= men_uart_config_port,
	.verify_port	= men_uart_verify_port,
};

static struct ox16c954_port men_uart_ports[MAX_SNGL_UARTS];


/*******************************************************************/
/** Ioctl function to treat special codes not handled in serial_core.c
 *
 * \param up		\IN highlevel (serial core) Port Struct
 * \param cmd		\IN ioctl type, see serial_m77.h
 * \param arg		\IN ioctl argument, see serial_m77.h
 *
 * \return 			0 or negative error number
 */
static int men_uart_m77phy( struct uart_port *up, 
							unsigned int cmd,
							unsigned long arg)
{
/*     int retVal = 0; */
	unsigned char ch = 0;
	struct ox16c954_port *ox = &men_uart_ports[up->line];

	M77DBG2("%s: line %d ox->type = %d\n", __FUNCTION__, up->line, ox->type );

	switch (cmd) {
		/* 	
		 * IOCTL for M77 Echo suppression 
		 */
    case M77_ECHO_SUPPRESS:
		M77DBG2("M77_ECHO_SUPPRESS:");
		if (ox->type != MOD_M77)
			return -ENOTTY;

		ch = serial_in(ox, ox->dcrReg ) & 0xfe;
		M77DBG2(" 1. read DCR: 0x%02x ", ch );
		if (arg) {
			ch |= M77_RX_EN;	/* enable Receive Line, allowing Echo */
		}

		M77DBG2("2. set DCR %02x at Reg %02x\n", ch, ox->dcrReg << 1 );
		serial_out(ox, ox->dcrReg, ch );
		break;


		/* 	
		 * IOCTL for M77 physical Mode setting
		 */
	case M77_PHYS_INT_SET:
		M77DBG2("ioctl M77_PHYS_INT_SET\n" );
		if (ox->type != MOD_M77)
			return -ENOTTY;
		
		/* Read DCR, ACR and clear out Mode bits DCR[0:2] first */
		ch = serial_in(ox, ox->dcrReg);
		ch &= 0xF8;	/* set desired bits later.. */
		ox->acr = serial_icr_read(ox, UART_ACR);
		M77DBG2("1. DCR=0x%02x ACR=0x%02x ", ch, ox->acr );

		switch (arg) {
		case M77_RS422_HD:
			ch |= M77_RS422_HD;
			M77DBG2("2. set DCR(0x%02x)=%02x(RS422 HD), ", ox->dcrReg<<1, ch);
			ox->acr |= OX954_ACR_DTR;
			ox->acrShadow = ox->acr;
			serial_icr_write(ox, UART_ACR, ox->acr);
			serial_out(ox, ox->dcrReg, ch);
			break;

		case M77_RS422_FD:
			ch |= M77_RS422_FD;
			M77DBG2("2. set DCR(0x%02x)=%02x(RS422 FD), ", ox->dcrReg<<1, ch);
			serial_out(ox, ox->dcrReg, ch);
			ox->acr &= ~OX954_ACR_DTR;
			ox->acrShadow = ox->acr;
			serial_icr_write(ox, UART_ACR, ox->acr);
			break;

		case M77_RS485_HD:
			ch |= M77_RS485_HD;
			M77DBG2("2. set DCR(0x%02x)=%02x(RS485 HD), ", ox->dcrReg<<1, ch);
			ox->acr |= OX954_ACR_DTR;
			ox->acrShadow = ox->acr;
			serial_icr_write(ox, UART_ACR, ox->acr);
			serial_out(ox, ox->dcrReg, ch);
			break;

		case M77_RS485_FD:
			ch |= M77_RS485_FD;
			M77DBG2("2. set DCR(0x%02x)=%02x(RS485 FD), ", ox->dcrReg<<1, ch);
			ox->acr &= ~OX954_ACR_DTR;
			ox->acrShadow = ox->acr;
			serial_icr_write(ox, UART_ACR, ox->acr);
			serial_out(ox, ox->dcrReg, ch);
			break;

		case M77_RS232:
			ch |= M77_RS232;
			M77DBG2("2. set DCR(0x%02x)=%02x(RS232 HD), ", ox->dcrReg<<1, ch);
			ox->acr &= ~OX954_ACR_DTR;
			ox->acrShadow = ox->acr;
			serial_icr_write(ox, UART_ACR, ox->acr);
			serial_out(ox, ox->dcrReg, ch);
			break;
		default:
			return -EINVAL;
		}

		ox->m77Mode = arg;
		M77DBG(" ACR = %02x\n", ox->acr);
		break;

		/* 	
		 * IOCTL for M45N Tristate settings
		 */
	case M45_TIO_TRI_MODE:
		M77DBG2(" ioctl M45_TIO_TRI_MODE " );
		if (ox->type != MOD_M45)
			return -ENOTTY;

		ch = serial_in(ox, ox->tcrReg);		
		M77DBG2(" 1. read TCR: 0x%02x ", ch );
		if (arg)
			ch |=ox->tcrBit;
		else
			ch &=~ox->tcrBit;

		M77DBG2("2. set TCR(0x%02x) = %02x\n", ox->tcrReg << 1, ch );
		serial_out(ox, ox->tcrReg, ch );			
		break;
	}

    return 0; /* retVal; */
}




/*******************************************************************/
/** Main HW dependent Ioctl function
 *
 * \param up		\IN Oxford 16C954 Port Struct
 * \param cmd		\IN ioctl type, see serial_m77.h
 * \param arg		\IN ioctl argument, see serial_m77.h
 *
 * \return 			0 or negative error number
 */
static int men_uart_ioctl(struct uart_port *up, unsigned int cmd, 
						  unsigned long arg)
{
	int retval = 0;

	/* M77DBG("%s: cmd = 0x%x arg = 0x%x ", __FUNCTION__, cmd, arg ); */

	switch (cmd) {

	case M77_PHYS_INT_SET:
	case M77_ECHO_SUPPRESS:
	case M45_TIO_TRI_MODE:
		retval = men_uart_m77phy( up, cmd, arg);
		break;
            
	default:
		retval = -ENOIOCTLCMD;
	}
	return retval;

}


/*******************************************************************/
/** Toplevel 16550 + compatibles autoconfig function
 *
 * \param up		\IN Oxford 16C954 Port Struct
 *
 * \return 			-
 */
static void autoconfig_16550a(struct ox16c954_port *up)
{
	unsigned int id1, id2, id3, rev;
	up->port.type = PORT_16C950;
 	up->capabilities |= UART_CAP_FIFO; 

	serial_out(up, UART_LCR, 0xBF);
	if (serial_in(up, UART_EFR) == 0 ) {
		printk("EFRv2 ");

		/* Everything with an EFR has SLEEP */
		up->capabilities |= UART_CAP_FIFO|UART_CAP_EFR | UART_CAP_SLEEP;

		/* Check for Oxford Semiconductor 16C95x (true for M45N/69N/77) */
		up->acr = 0;	
		up->acrShadow = up->acr;
		
		/* Set Enhanced Mode */
		serial_efr_write(up, UART_EFR, UART_EFR_ECB);
		id1 = serial_icr_read(up, UART_ID1);
		id2 = serial_icr_read(up, UART_ID2);
		id3 = serial_icr_read(up, UART_ID3);
		rev = serial_icr_read(up, UART_REV);
		M77DBG("16c950 ID: %02x:%02x:%02x:%02x ", id1, id2, id3, rev);
		if (id1==0x16 && id2 == 0xC9 && (id3==0x50 || id3==0x52 || id3==0x54)) 
			up->port.type = PORT_16C950;
		
		return;
	}
}


/*******************************************************************/
/** Toplevel Autoconfig function
 *
 * \param up			\IN Oxford 16C954 Port Struct
 * \param probeflags	\IN one of UPF_*, see serial_core.h
 *
 * \brief This is called to determine what type of UART chip this serial 
 *        port is, using: 8250, 16450, 16550, 16550A.
 *
 * \return 			-
 */
static void autoconfig(struct ox16c954_port *up, unsigned int probeflags)
{
	unsigned char status1, scratch, scratch2, scratch3;
	unsigned char save_lcr, save_mcr;
	unsigned long flags;

	if (!up->port.iobase && !up->port.mapbase && !up->port.membase)
		return;

	M77DBG( UART_NAME_PREFIX"%d: autoconf: ", up->port.line);

	/*
	 * We really do need global IRQs disabled here - we're going to
	 * be frobbing the chips IRQ enable register to see if it exists.
	 */
	spin_lock_irqsave(&up->port.lock, flags);

	up->capabilities = 0;
	up->bugs = 0;

	scratch = serial_in(up, UART_IER);
	serial_out(up, UART_IER, 0);

	scratch2 = serial_in(up, UART_IER);
	serial_out(up, UART_IER, 0x0F);

	scratch3 = serial_in(up, UART_IER);
	serial_out(up, UART_IER, scratch);

	if (scratch2 != 0 || scratch3 != 0x0F) {
		/* We failed; there's nothing here */
		M77DBG("IER test failed (%02x, %02x) ",scratch2, scratch3);
		goto out;
	}

	save_mcr = serial_in(up, UART_MCR);
	save_lcr = serial_in(up, UART_LCR);

	/* 
	 * Check to see if a UART is really there.
	 */
	serial_out(up, UART_MCR, UART_MCR_LOOP | 0x0A);
	status1 = serial_in(up, UART_MSR) & 0xF0;
	serial_out(up, UART_MCR, save_mcr);
	if (status1 != 0x90) {
		M77DBG("LOOP test failed (%02x) ", status1);
		goto out;
	}


	/*
	 * We're pretty sure there's a port here.  Lets find out what
	 * type of port it is.  The IIR top two bits allows us to find
	 * out if it's 8250 or 16450, 16550, 16550A or later.  This
	 * determines what we test for next.
	 *
	 * We also initialise the EFR (if any) to zero for later.  The
	 * EFR occupies the same register location as the FCR and IIR.
	 */

	/* Set Enhanced Mode */
	serial_efr_write(up, UART_EFR, UART_EFR_ECB);

	serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO);
	scratch = serial_in(up, UART_IIR) >> 6;

	M77DBG3("iir=%d ", scratch);

	switch (scratch) {
	case 3:
		autoconfig_16550a(up);
		break;
	default:
		printk("*** unknown iir value:=%d (should be 3)", scratch);
		break;
	}

	serial_out(up, UART_LCR, save_lcr);

	if (up->capabilities != uart_config[up->port.type].flags) {

		printk(KERN_WARNING UART_NAME_PREFIX
			   "%d: detected caps %08x should be %08x\n",
			   up->port.line, up->capabilities,
			   uart_config[up->port.type].flags);
	}

	up->port.fifosize = uart_config[up->port.type].fifo_size;
	up->capabilities = uart_config[up->port.type].flags;
	up->tx_loadsz = uart_config[up->port.type].tx_loadsz;

	if (up->port.type == PORT_UNKNOWN)
		goto out;

	/*
	 * Reset the UART.
	 */

	serial_out(up, UART_MCR, save_mcr);
	men_uart_clear_fifos(up);
	(void)serial_in(up, UART_RX);
	if (up->capabilities & UART_CAP_UUE)
		serial_out(up, UART_IER, UART_IER_UUE);
	else
		serial_out(up, UART_IER, 0);

 out:	
	spin_unlock_irqrestore(&up->port.lock, flags);
//	restore_flags(flags);

}


/*******************************************************************/
/** lowlevel TX Stop function, by clearing Threshold IRQ
 *
 * \param p			\IN Oxford 16C954 Port Struct
 *
 * \return 			-
 */
static inline void __stop_tx(struct ox16c954_port *p)
{
	if (p->ier & UART_IER_THRI) {
		p->ier &= ~UART_IER_THRI;
		serial_out(p, UART_IER, p->ier);
	}
}

/*******************************************************************/
/** highlevel TX stop function called by serial_core
 *
 * \param port		\IN highlevel uart_port Struct (as in serial_core)
 *
 * \return 			-
 */
#if LINUX_VERSION_CODE < Z025_SERIAL_DIFF
static void men_uart_stop_tx(struct uart_port *port, unsigned int tty_stop)
#else
static void men_uart_stop_tx(struct uart_port *port)
#endif
{
	struct ox16c954_port *up = (struct ox16c954_port *)port;

	__stop_tx(up);

	/*
	 * We really want to stop the transmitter from sending.
	 */
	if (up->port.type == PORT_16C950) {
		up->acr |= UART_ACR_TXDIS;
		serial_icr_write(up, UART_ACR, up->acr);
		up->acrShadow = up->acr;
	}
}


/*******************************************************************/
/** central transmit function, called in ISR 
 *
 * \param up		\IN Oxford 16C954 Port Struct
 *
 * \return 			-
 */
static inline void transmit_chars(struct ox16c954_port *up)
{

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
	struct circ_buf *xmit = &up->port.info->xmit;
#else
	struct circ_buf *xmit = &up->port.state->xmit;
#endif

	int count;

	if (up->port.x_char) {
		serial_out(up, UART_TX, up->port.x_char);
		up->port.icount.tx++;
		up->port.x_char = 0;
		return;
	}

	if (uart_tx_stopped(&up->port)) {
#if LINUX_VERSION_CODE < Z025_SERIAL_DIFF
		men_uart_stop_tx(&up->port, 0);
#else
		men_uart_stop_tx(&up->port);
#endif
		return;
	}

	if (uart_circ_empty(xmit)) {
		__stop_tx(up);
		return;
	}

	count = up->tx_loadsz;
	do {
		serial_out(up, UART_TX, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		up->port.icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&up->port);

	DEBUG_INTR("THRE ");

	if (uart_circ_empty(xmit))
		__stop_tx(up);
}


/*******************************************************************/
/** transmit start function
 *
 * \param port		\IN highlevel (serial core) Port Struct
 *
 * \return 			-
 */
#if LINUX_VERSION_CODE < Z025_SERIAL_DIFF
static void men_uart_start_tx(struct uart_port *port, unsigned int tty_start)
#else
static void men_uart_start_tx(struct uart_port *port)
#endif
{
	struct ox16c954_port *up = (struct ox16c954_port *)port;

	if (!(up->ier & UART_IER_THRI)) {
		up->ier |= UART_IER_THRI;
		serial_out(up, UART_IER, up->ier);

		if (up->bugs & UART_BUG_TXEN) {
			unsigned char lsr, iir;
			lsr = serial_in(up, UART_LSR);
			iir = serial_in(up, UART_IIR);
			if (lsr & UART_LSR_TEMT && iir & UART_IIR_NO_INT)
				transmit_chars(up);
		}
	}

	/* Re-enable the transmitter if we disabled it. */
	if ( up->acr & UART_ACR_TXDIS) {
		up->acr &= ~UART_ACR_TXDIS;
		serial_icr_write(up, UART_ACR, up->acr);
		up->acrShadow = up->acr;
	}
}


/*******************************************************************/
/** receive stop function
 *
 * \param port		\IN highlevel (serial core) Port Struct
 *
 * \return 			-
 */
static void men_uart_stop_rx(struct uart_port *port)
{
	struct ox16c954_port *up = (struct ox16c954_port *)port;

	up->ier &= ~UART_IER_RLSI;
	up->port.read_status_mask &= ~UART_LSR_DR;
	serial_out(up, UART_IER, up->ier);
}

/*******************************************************************/
/** receive stop function
 *
 * \param port		\IN highlevel uart_port Struct (as in serial_core)
 *
 * \return 			-
 */
static void men_uart_enable_ms(struct uart_port *port)
{
	struct ox16c954_port *up = (struct ox16c954_port *)port;

	/* no MSR capabilities */
	if (up->bugs & UART_BUG_NOMSR)
		return;

	up->ier |= UART_IER_MSI;
	serial_out(up, UART_IER, up->ier);
}

/*******************************************************************/
/** receive chars function, called within ISR
 *
 * \param up			\IN		Oxford 16C954 Port Struct
 * \param status		\OUT	new Value of LSR Register
 * \param regs			\IN		pt_regs parameter from ISR (unused)
 *
 * \return 			-
 */
static inline void
receive_chars(struct ox16c954_port *up, int *status, struct pt_regs *regs)
{

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,27)
	struct tty_struct *tty = up->port.info->tty;
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,31)
	struct tty_struct *tty = up->port.info->port.tty;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32) 
	struct tty_struct *tty = up->port.state->port.tty;
#endif

	unsigned char ch, lsr = *status;
	int max_count = 256;
	char flag;

	do {
		ch = serial_in(up, UART_RX);
		flag = TTY_NORMAL;
		up->port.icount.rx++;

		if (unlikely(lsr & (UART_LSR_BI | UART_LSR_PE |
							UART_LSR_FE | UART_LSR_OE))) {
			/*
			 * For statistics only
			 */
			if (lsr & UART_LSR_BI) {
				lsr &= ~(UART_LSR_FE | UART_LSR_PE);
				up->port.icount.brk++;
				/*
				 * We do the SysRQ and SAK checking
				 * here because otherwise the break
				 * may get masked by ignore_status_mask
				 * or read_status_mask.
				 */
				if (uart_handle_break(&up->port))
					goto ignore_char;
			} else if (lsr & UART_LSR_PE)
				up->port.icount.parity++;
			else if (lsr & UART_LSR_FE)
				up->port.icount.frame++;
			if (lsr & UART_LSR_OE)
				up->port.icount.overrun++;

			/*
			 * Mask off conditions which should be ignored.
			 */
			lsr &= up->port.read_status_mask;

			if (lsr & UART_LSR_BI) {
				DEBUG_INTR("handling break....");
				flag = TTY_BREAK;
			} else if (lsr & UART_LSR_PE)
				flag = TTY_PARITY;
			else if (lsr & UART_LSR_FE)
				flag = TTY_FRAME;
		}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
		if (uart_handle_sysrq_char(&up->port, ch, regs))
#else
		if (uart_handle_sysrq_char(&up->port, ch))
#endif
			goto ignore_char;

		uart_insert_char(&up->port, lsr, UART_LSR_OE, ch, flag);

	ignore_char:
		lsr = serial_in(up, UART_LSR);
	} while ((lsr & UART_LSR_DR) && (max_count-- > 0));
	spin_unlock(&up->port.lock);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)
	tty_flip_buffer_push(tty);
#else 
	tty_flip_buffer_push(tty->port);
#endif
	spin_lock(&up->port.lock);
	*status = lsr;
}

/*******************************************************************/
/** check_modem_status Bits
 *
 * \param up			\IN Oxford 16C954 Port Struct
 *
 * \return 			-
 */
static inline void check_modem_status(struct ox16c954_port *up)
{
	int status;

	status = serial_in(up, UART_MSR);

	if ((status & UART_MSR_ANY_DELTA) == 0)
		return;

	if (status & UART_MSR_TERI)
		up->port.icount.rng++;
	if (status & UART_MSR_DDSR)
		up->port.icount.dsr++;
	if (status & UART_MSR_DDCD)
		uart_handle_dcd_change(&up->port, status & UART_MSR_DCD);
	if (status & UART_MSR_DCTS)
		uart_handle_cts_change(&up->port, status & UART_MSR_CTS);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
	wake_up_interruptible(&up->port.info->delta_msr_wait);
#else
	wake_up_interruptible(&up->port.state->port.delta_msr_wait);
#endif
}


/*******************************************************************/
/** handles the interrupt from one port, within ISR
 *
 * \param up		\IN 	Oxford 16C954 Port Struct
 * \param regs		\IN 	passed from ISR but unused
 *
 * \return 			-
 */
static inline void men_uart_handle_port(struct ox16c954_port *up, 
										struct pt_regs *regs)
{
	unsigned int status = serial_in(up, UART_LSR);

	DEBUG_INTR("status = %x...", status);

	if (status & UART_LSR_DR)
		receive_chars(up, &status, regs);

	check_modem_status(up);

	if (status & UART_LSR_THRE)
		transmit_chars(up);
}



/*****************************************************************************/
/** handles the interrupt from one M-Module
 *
 * \param data		\IN 	void Parameter from MDIS Irq handler
 *
 * \brief The original 8250.c UART interrupt handler is now Registered via
 * mdis_install_external_irq(). Its taylored to match the Requirements
 * of serving the M45N/M69N/M77 Interrupts only. No calls to request_irq() are
 * done within this driver.
 * \return 			-
 */
static int M77_IrqHandler(void *data)
{

 	UARTMOD_INFO *mmod 			= data;
	struct pt_regs *regs = NULL;
	unsigned int i;
	struct ox16c954_port *up;
	unsigned int iir;
	unsigned char cpld_ir_reg;
	unsigned int retcode 		= LL_IRQ_DEV_NOT;
	struct list_head  *pos		= NULL;

	/* skip through the registered M-Modules List */
    list_for_each( pos, &G_uartModListHead ) {

		mmod = list_entry(pos, UARTMOD_INFO, head);

		cpld_ir_reg = MREAD_D16( mmod->memBase, M77_REG_IR ) & 0x00ff;
		/* printk(KERN_ERR "cpld_ir_reg = 0x%02x\n", cpld_ir_reg); */

		if ( cpld_ir_reg & 0x1 ) {
			for (i = 0; i < mmod->nrChannels; i++) {
				up = mmod->port8250[i];				
				iir = serial_in(up, UART_IIR);
				if ( !(iir & UART_IIR_NO_INT) ) {
					spin_lock(&up->port.lock);
					DEBUG_INTR("ISR: UART%d\n", i);
					men_uart_handle_port(up, regs);
					spin_unlock(&up->port.lock);
				}
			}
			/* clear Interrupt */
			control_out( mmod->memBase, M77_REG_IR,	cpld_ir_reg);
			retcode = LL_IRQ_DEVICE;
		}

		/* If its an M45N check the second IR Register at 0xC8 too */
		if (mmod->modtype == MOD_M45) {

			cpld_ir_reg = MREAD_D16(mmod->memBase, M45_REG_IR2 ) & 0x00ff;
			/* printk(KERN_ERR "cpld_ir_reg(2) = 0x%02x\n", cpld_ir_reg); */

			if ( cpld_ir_reg & 0x1 ) {
				for (i = 0; i < mmod->nrChannels; i++) { 
					/* optimal:check 4-7 only */
					up = mmod->port8250[i];
					iir = serial_in(up, UART_IIR);
					if ( !(iir & UART_IIR_NO_INT) ) {
						spin_lock(&up->port.lock);
						men_uart_handle_port(up, regs);
						spin_unlock(&up->port.lock);
					}
				}
				/* clear Interrupt */
				control_out( mmod->memBase, M45_REG_IR2, cpld_ir_reg);
			}
		}
	}
	return(retcode);
}


/*******************************************************************/
/** check if UART transmit Register is empty
 *
 * \param port		\IN highlevel uart_port Struct (as in serial_core)
 *
 * \return 			TIOCSER_TEMT if empty or 0
 */
static unsigned int men_uart_tx_empty(struct uart_port *port)
{

	unsigned long flags;
	unsigned int ret;
	struct ox16c954_port *up;

	up = (struct ox16c954_port *)port;

	spin_lock_irqsave(&up->port.lock, flags);
	ret = serial_in(up, UART_LSR) & UART_LSR_TEMT ? TIOCSER_TEMT : 0;
	spin_unlock_irqrestore(&up->port.lock, flags);

	return ret;
}


/******************************************************************************/
/** return Modem Control Register and return as TIO* Values, for ioctl call
 *
 * \param port		\IN 	Oxford 16C954 Port Struct
 *
 * \return 			encoded MSR Bits
 */
static unsigned int men_uart_get_mctrl(struct uart_port *port)
{
	unsigned char status;
	unsigned int ret;
	struct ox16c954_port *up = (struct ox16c954_port *)port;
	status = serial_in(up, UART_MSR);

	ret = 0;
	if (status & UART_MSR_DCD)
		ret |= TIOCM_CAR;
	if (status & UART_MSR_RI)
		ret |= TIOCM_RNG;
	if (status & UART_MSR_DSR)
		ret |= TIOCM_DSR;
	if (status & UART_MSR_CTS)
		ret |= TIOCM_CTS;
	return ret;
}



/******************************************************************************/
/** set Modem Control Register, called from core ioctl function
 *
 * \param port		\IN 	Oxford 16C954 Port Struct
 * \param mctrl		\IN 	MSR Bits to set
 *
 * \return 			-
 */
static void men_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct ox16c954_port *up = (struct ox16c954_port *)port;
	unsigned char mcr = 0;

	if (mctrl & TIOCM_RTS)
		mcr |= UART_MCR_RTS;

	if (mctrl & TIOCM_DTR)
		mcr |= UART_MCR_DTR;

	if (mctrl & TIOCM_OUT1)
		mcr |= UART_MCR_OUT1;

	if (mctrl & TIOCM_OUT2)
		mcr |= UART_MCR_OUT2;

	if (mctrl & TIOCM_LOOP)
		mcr |= UART_MCR_LOOP;

	mcr = (mcr & up->mcr_mask) | up->mcr_force | up->mcr;

	serial_out(up, UART_MCR, mcr);
}


/******************************************************************************/
/** set Set break control 
 *
 * \param port			\IN 	Oxford 16C954 Port Struct
 * \param break_state	\IN     state to set, -1: set break, != -1:clear
 *
 * \return 			-
 */
static void men_uart_break_ctl(struct uart_port *port, int break_state)
{
	struct ox16c954_port *up = (struct ox16c954_port *)port;
	unsigned long flags;

	spin_lock_irqsave(&up->port.lock, flags);
	if (break_state == -1)
		up->lcr |= UART_LCR_SBC;
	else
		up->lcr &= ~UART_LCR_SBC;
	serial_out(up, UART_LCR, up->lcr);
	spin_unlock_irqrestore(&up->port.lock, flags);
}



/*****************************************************************************/
/** central UART channel startup function, called upon each open to /dev/ttyDx
 *
 * \param port			\IN 	Oxford 16C954 Port Struct
 *
 * \return 				0 or error code
 *
 * \brief	Wakes up and initialize UART. Gets called whenever a process opens
 *          /dev/ttyDx. When passed at module load time the ACR is set such
 *          that for M077
 *
 */
static int men_uart_startup(struct uart_port *port)
{
	struct ox16c954_port *up = (struct ox16c954_port *)port;
	unsigned long flags;
	unsigned char lsr, iir;

	up->capabilities = uart_config[up->port.type].flags;
	up->mcr = 0;

	serial_out(up, 	UART_IER, 	0);
	serial_out(up, 	UART_LCR, 	0);
	serial_icr_write(up, UART_CSR, 	0); /* Reset the UART */

	/* Set Enhanced Mode */
	serial_efr_write(up, UART_EFR, UART_EFR_ECB);

	up->acr = up->acrShadow;
	M77DBG3("%s: up=%p up->type=0x%x up->m77Mode=0x%x up->acr=0x%02x\n", 
			__FUNCTION__, up, up->type, up->m77Mode, up->acr);
	
	if ( up->type == MOD_M77 && \
		 ((up->m77Mode == M77_RS485_HD) || (up->m77Mode == M77_RS422_HD ))) {
		M77DBG3("%s: up->acr = 0x%02x\n", __FUNCTION__, up->acr);		
		up->acr |= 0x18;
		serial_icr_write(up, UART_ACR, up->acr);
	}

	/* Clear FIFO buffers & disable them. Theyre reenabled in set_termios */
	men_uart_clear_fifos(up);

	/* Clear the interrupt registers. */
	(void) serial_in(up, UART_LSR);
	(void) serial_in(up, UART_RX);
	(void) serial_in(up, UART_IIR);
	(void) serial_in(up, UART_MSR);

	/* Now, initialize the UART */
	serial_out(up, UART_LCR, UART_LCR_WLEN8);

	spin_lock_irqsave(&up->port.lock, flags);

	up->port.mctrl |= TIOCM_OUT2;
	men_uart_set_mctrl(&up->port, up->port.mctrl);

	/* quick test to see if we receive an IRQ when we enable the TX irq. */
	serial_out(up, UART_IER, UART_IER_THRI);
	lsr = serial_in(up, UART_LSR);
	iir = serial_in(up, UART_IIR);
	serial_out(up, UART_IER, 0);

	if (lsr & UART_LSR_TEMT && iir & UART_IIR_NO_INT) {
		if (!(up->bugs & UART_BUG_TXEN)) {
			up->bugs |= UART_BUG_TXEN;
			pr_debug("ttyS%d - enabling bad tx status workarounds\n",
				 port->line);
		}
	} else {
		up->bugs &= ~UART_BUG_TXEN;
	}

	spin_unlock_irqrestore(&up->port.lock, flags);

	/*
	 * Finally, enable interrupts.  Note: Modem status interrupts
	 * are set via set_termios(), which will be occurring imminently
	 * anyway, so we don't enable them here.
	 */
	up->ier = UART_IER_RLSI | UART_IER_RDI;
	serial_out(up, UART_IER, up->ier);

	/*
	 * And clear the interrupt registers again for luck.
	 */
	(void) serial_in(up, UART_LSR);
	(void) serial_in(up, UART_RX);
	(void) serial_in(up, UART_IIR);
	(void) serial_in(up, UART_MSR);

	return 0;
}


/******************************************************************************/
/** central UART shutdown function, called upon funal close of port /dev/ttyDx
 *
 * \param port			\IN 	Oxford 16C954 Port Struct
 *
 * \return 				0 or error code
 */
static void men_uart_shutdown(struct uart_port *port)
{

	struct ox16c954_port *up = (struct ox16c954_port *)port;
	unsigned long flags;

	/*
	 * Disable interrupts from this port
	 */
	up->ier = 0;
	serial_out(up, UART_IER, 0);

	spin_lock_irqsave(&up->port.lock, flags);

	
	up->port.mctrl &= ~TIOCM_OUT2;
	
	men_uart_set_mctrl(&up->port, up->port.mctrl);
	spin_unlock_irqrestore(&up->port.lock, flags);

	/*
	 * Disable break condition and FIFOs
	 */
	serial_out(up, UART_LCR, serial_in(up, UART_LCR) & ~UART_LCR_SBC);
	men_uart_clear_fifos(up);


	/*
	 * Read data port to reset things
	 */
	(void) serial_in(up, UART_RX);


}


/******************************************************************************/
/** Calculate Baudrate Divisor
 *
 * \param port			\IN 	Oxford 16C954 Port Struct
 * \param baud			\IN 	Baudrate Value, 300-1152000
 *
 * \return 				Divisor with respect to Clock 18,432 MHz
 */
static unsigned int men_uart_get_divisor(struct uart_port *port, 
										 unsigned int baud)
{
	/* No magic handling needed for M Modules, Clock is always 18,432 MHz */
	return uart_get_divisor(port, baud);
}



/******************************************************************************/
/** function to set changed tty settings
 *
 * \param port			\IN 	highlevel UART Port Struct
 * \param termios		\IN 	termios struct with new settings
 * \param old			\IN 	termios struct to keep old settings(?)
 *
 * \brief this is the central link between higher tty layer and the UART 
 *        Hardware to change all aspects of the Channel like Baudrate, data bit
 *        settings etc. Its called everytime a property of the channel is 
 *		  changed e.g. with stty.
 *
 * \return 				-
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
static void men_uart_set_termios(struct uart_port *port,struct termios *termios, struct termios *old)
#else
static void men_uart_set_termios(struct uart_port *port,struct ktermios *termios, struct ktermios *old)
#endif
{
	unsigned char efr = 0;
	struct ox16c954_port *up = (struct ox16c954_port *)port;
	unsigned char cval, fcr = 0;
	unsigned long flags;
	unsigned int baud, quot;

	M77DBG3("%s: c_iflag = 0x%04x c_cflag = 0x%04x  Settings:\n",
			   __FUNCTION__, termios->c_iflag, termios->c_cflag );

	/* 
	 * Set Word Length: 5..8 bit 
	 */
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		cval = UART_LCR_WLEN5;
		M77DBG3(" - 5 Data Bits\n");
		break;
	case CS6:
		cval = UART_LCR_WLEN6;
		M77DBG3(" - 6 Data Bits\n");
		break;
	case CS7:
		cval = UART_LCR_WLEN7;
		M77DBG3(" - 7 Data Bits\n");
		break;
	default:
	case CS8:
		cval = UART_LCR_WLEN8;
		M77DBG3(" - 8 Data Bits\n");
		break;
	}

	/* 
	 * Set 1 or 2 Stop Bits
	 * Oxford Data Sheet states that its actually 1.5 Stopbits at 5 Databits,
	 * 2 Bits at 6,7,8 Data bits
	 */
	if (termios->c_cflag & CSTOPB){
		cval |= UART_LCR_STOP;
		M77DBG3(" - 2 Stop Bits\n");
	} else {
		M77DBG3(" - 1 Stop Bit\n");
	}


	/* 
	 * Set Parity (none/odd/even)
	 */
	if (termios->c_cflag & PARENB)
		cval |= UART_LCR_PARITY;
	if (!(termios->c_cflag & PARODD))
		cval |= UART_LCR_EPAR;

	/* Report the Parity setting according to LCR[5:3] Data Sheet */

	if(!(cval & UART_LCR_PARITY)){
		M77DBG3(" - No Parity\n");
	} else {
		if( ((cval & 0x38) >> 3) == 1 ) /* Mask Bits 5,4,3:  0x38=0011 1000 */
			M77DBG3(" - Odd Parity\n");

		if( ((cval & 0x38) >> 3) == 3 )
			M77DBG3(" - Even Parity\n");

		if( ((cval & 0x38) >> 3) == 5 )
			M77DBG3(" - Parity forced 1\n");

		if( ((cval & 0x38) >> 3) == 7 )
			M77DBG3(" - Parity forced 0\n");
	}

	/*
	 * Ask the core to calculate the divisor for us.
	 */
	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/16); 
	quot = men_uart_get_divisor(port, baud);
	M77DBG3(" - Baudrate: %d (quot=%d)\n", baud, quot);

	/*
	 * Oxford Semi 952 rev B workaround
	 */
	if (up->bugs & UART_BUG_QUOT && (quot & 0xff) == 0)
		quot ++;

	if (up->capabilities & UART_CAP_FIFO && up->port.fifosize > 1) {
		if (baud < 2400)
			fcr = UART_FCR_ENABLE_FIFO | UART_FCR_TRIGGER_1;
		else
			fcr = uart_config[up->port.type].fcr;
	}

	/*
	 * MCR-based auto flow control.  When AFE is enabled, RTS will be
	 * deasserted when the receive FIFO contains more characters than
	 * the trigger, or the MCR RTS bit is cleared.  In the case where
	 * the remote UART is not using CTS auto flow control, we must
	 * have sufficient FIFO entries for the latency of the remote
	 * UART to respond.  IOW, at least 32 bytes of FIFO.
	 */
	if (up->capabilities & UART_CAP_AFE && up->port.fifosize >= 32) {
		up->mcr &= ~UART_MCR_AFE;
		if (termios->c_cflag & CRTSCTS)
			up->mcr |= UART_MCR_AFE;
	}

	/* Ok, we're now changing the port state. Do it with interrupts disabled. */
	spin_lock_irqsave(&up->port.lock, flags);

	/* Update the per-port timeout. */
	uart_update_timeout(port, termios->c_cflag, baud);

	up->port.read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		up->port.read_status_mask |= UART_LSR_FE | UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		up->port.read_status_mask |= UART_LSR_BI;

	/* Characteres to ignore */
	up->port.ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		up->port.ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		up->port.ignore_status_mask |= UART_LSR_BI;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			up->port.ignore_status_mask |= UART_LSR_OE;
	}

	/* ignore all characters if CREAD is not set */
	if ((termios->c_cflag & CREAD) == 0)
		up->port.ignore_status_mask |= UART_LSR_DR;

	/* CTS flow control flag and modem status interrupts */
	up->ier &= ~UART_IER_MSI;
	if (!(up->bugs & UART_BUG_NOMSR) &&
			UART_ENABLE_MS(&up->port, termios->c_cflag))
		up->ier |= UART_IER_MSI;
	if (up->capabilities & UART_CAP_UUE)
		up->ier |= UART_IER_UUE | UART_IER_RTOIE;
	serial_out(up, UART_IER, up->ier);

	if ( termios->c_cflag & CRTSCTS ) {
		if ( up->type != MOD_M77 ) {

			efr |= UART_EFR_CTS;
			M77DBG3(" - HW Flow Control (RTS/CTS)\n");
		} else {
			/* Dont use RTS/CTS Handshake setting on M77! */
			printk(KERN_INFO "*** Module is a M77 - ignoring Flag CRTSCTS\n");
		}
	}

	/* Apply to Register */
	serial_efr_write(up, UART_EFR, efr );

	/* Inband XON/XOFF Flow Control desired? */
	if (termios->c_iflag & (IXON|IXOFF)) {
		serial_efr_write(up, M77_XON1_OFFSET, 	M77_XON_CHAR );
		serial_efr_write(up, M77_XON2_OFFSET, 	M77_XON_CHAR );        
		serial_efr_write(up, M77_XOFF1_OFFSET, 	M77_XOFF_CHAR );       
 		serial_efr_write(up, M77_XOFF2_OFFSET, 	M77_XOFF_CHAR );       
		M77DBG3(" - SW Flow Control IXON/IXOFF\n");
		set_inband_flowctrl(up, 1 );
 	} else {
		set_inband_flowctrl(up, 0 );
	}
	
	/*  Set Baudrate Divider. M45N/69N/77 uartclk is always 18,432 MHz */
	serial_out(up, UART_LCR, cval | UART_LCR_DLAB);
	serial_out(up, UART_DLL, quot & 0xff);			
	serial_out(up, UART_DLM, quot >> 8);			

	/*
	 * LCR DLAB must be set to enable 64-byte FIFO mode. If the FCR
	 * is written without DLAB set, this mode will be disabled.
	 */
	if (up->port.type == PORT_16750)
		serial_out(up, UART_FCR, fcr);

	serial_out(up, UART_LCR, cval);		/* reset DLAB */

	up->lcr = cval;					    /* Save LCR */
	if (up->port.type != PORT_16750) {

		if (fcr & UART_FCR_ENABLE_FIFO) {
			/* emulated UARTs (Lucent Venus 167x) need two steps */
			serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO);
		}
		serial_out(up, UART_FCR, fcr);		/* set fcr */
	}

	men_uart_set_mctrl(&up->port, up->port.mctrl);
	spin_unlock_irqrestore(&up->port.lock, flags);
}


static void
men_uart_pm(struct uart_port *port, unsigned int state, unsigned int oldstate)
{
	/* No special Powermanagement for our M Modules */
	do { } while (0);

}


static void men_uart_release_port(struct uart_port *port)
{
	/* Not necessary for M-Modules */
}

static int men_uart_request_port(struct uart_port *port)
{
	/* Not necessary for M-Modules */
	return 0;
}


/******************************************************************************/
/** highlevel UART Port config function
 *
 * \param port			\IN 	highlevel UART Port Struct
 * \param flags			\IN 	optional flags to control probing
 *
 * \return 				-
 */
static void men_uart_config_port(struct uart_port *port, int flags)
{
	struct ox16c954_port *up = (struct ox16c954_port *)port;
	int probeflags = ~0;

	if (flags & UART_CONFIG_TYPE)
		autoconfig(up, probeflags);

}


/*****************************************************************************/
/** return next free UART from internal UART Array
 *
 * \param port			\IN 	highlevel UART Port Struct
 *
 * \brief  We are free to use the ttyDxx entries by ourselves 
 * 		   So just return the next free men_uart_ports[i].port.
 * 		   This is simplified from serial8250_find_match_or_unused().
 *
 * \return 				pointer to port struct
 */
static struct 
ox16c954_port *men_uart_find_match_or_unused(struct uart_port *port)
{
	int i;
	/* simply return the first entry which hasnt already a membase entry */
	for (i = 0; i < MAX_SNGL_UARTS; i++) {
		if ( !men_uart_ports[i].port.membase ) {
			M77DBG3("%s: return UART nr. %d\n", __FUNCTION__, i);
			return &men_uart_ports[i];
		}	
	}
	return NULL;
}



static int men_uart_verify_port(struct uart_port *port, 
								struct serial_struct *ser)
{
	/* shouldnt hurt to report us to serial core as always existing */
	return 0;
}



/*****************************************************************************/
/** Function to register each 4 or 8 UART Channels of a M-Module at serial core
 *
 * \param port		\IN    highlevel UART Port Struct
 * \param modtype	\IN    Module ID as in 13m07790.xml
 * \param nrLine	\IN    UART Line number as in /dev/ttyDn
 * \param glob		\OUT   returned OX16C954 UART, stored in UARTMOD_INFO array
 *
 * \return 				0 or negative error number
 */
int men_uart_register_port(struct uart_port *port, 
						   unsigned int modtype,
						   unsigned int nrLine, 
						   struct ox16c954_port **glob )
{
	struct ox16c954_port *uart;
	int ret = -ENOSPC;
	
	/* For M45N: helper for TCRbit-to-Channelnumber mapping */
	unsigned int tcr[8] = {M45_TCR_TRISTATE0, M45_TCR_TRISTATE1, 
						   M45_TCR_TRISTATE2, M45_TCR_TRISTATE2,
						   M45_TCR_TRISTATE3, M45_TCR_TRISTATE3,
						   M45_TCR_TRISTATE4, M45_TCR_TRISTATE4 };

	if (port->uartclk == 0)
		return -EINVAL;

	down( &serial_sem );

	/* take one UART from the global ox16c954_port array */
	uart = men_uart_find_match_or_unused(port);

	if (uart) {
		uart->port.iobase   	= port->iobase;
		uart->port.membase  	= port->membase;
		uart->port.irq      	= port->irq;
		uart->port.uartclk  	= port->uartclk;
		uart->port.fifosize 	= port->fifosize;
		uart->port.regshift 	= port->regshift;
		uart->port.iotype   	= port->iotype;
		uart->port.flags    	= port->flags;
		uart->port.mapbase  	= port->mapbase;

		/* for runtime-ioctl calls remember if we are a M45N, M69N or M77 */
		uart->type				= modtype;

		switch (uart->type) {
		case MOD_M77:
			uart->dcrReg = M77_DCR_REG_BASE + nrLine;
			break;

		case MOD_M45:
			uart->tcrReg = (nrLine < 4) ? M45_TCR1_REG : M45_TCR2_REG;
			uart->tcrBit = tcr[nrLine];	
			break;
		}

#if 0
		printk( KERN_INFO "men_uart_register_port:\n");
		printk( KERN_INFO "uart->port.iobase   = 0x%08x\n",uart->port.iobase);
		printk( KERN_INFO "uart->port.membase  = 0x%08x\n",uart->port.membase);
		printk( KERN_INFO "uart->port.irq      = 0x%02x\n",uart->port.irq);
		printk( KERN_INFO "uart->port.uartclk  = %ld\n",uart->port.uartclk);
		printk( KERN_INFO "uart->port.fifosize = %d\n", uart->port.fifosize );
		printk( KERN_INFO "uart->port.regshift = %d\n", uart->port.regshift );
		printk( KERN_INFO "uart->port.iotype   = 0x%08x\n", uart->port.iotype);
		printk( KERN_INFO "uart->port.flags    = 0x%08x\n",uart->port.flags );
		printk( KERN_INFO "uart->port.mapbase  = 0x%08x\n",uart->port.mapbase);
#endif

		if (port->dev)
			uart->port.dev = port->dev;

		ret = uart_add_one_port(&men_uart_reg, &uart->port);
		
		/* give back this OX16C954 UART for storing in UARTMOD_INFO array */
		*glob = uart;

		if (ret == 0)
			ret = uart->port.line;
	} 

	up(&serial_sem);
	return ret;
}

/*******************************************************************/
/** Remove one serial port
 *
 * \param line		\IN serial line number
 *
 * \return 			-
 */
void men_uart_unregister_port(int line)
{
	struct ox16c954_port *uart = &men_uart_ports[line];
	
	down(&serial_sem);
	uart_remove_one_port( &men_uart_reg, &uart->port);
	uart->port.dev = NULL;
	up(&serial_sem);
}



/*******************************************************************/
/** return Port type name
 *
 * \param port		\IN 	highlevel UART Port Struct
 *
 * \return 			char pointer to string, typically "16C954" here
 */
static const char *men_uart_type(struct uart_port *port)
{
	int type = port->type;

	if (type >= ARRAY_SIZE(uart_config))
		type = 0;
	return uart_config[type].name;
}



/*******************************************************************/
/** Initialize the global Array of Oxford UARTs
 *
 */
static void men_uart_init_ports(void)
{

	unsigned int i;

	for (i = 0; i < MAX_SNGL_UARTS; i++) {
		struct ox16c954_port *up = &men_uart_ports[i];

		up->port.line 		= 	i;
		spin_lock_init(&up->port.lock);

		up->timer.function 	= NULL /* serial8250_timeout */;
		up->mcr_mask 		= ~0;
		up->mcr_force 		= 0;
		up->port.ops 		= &men_uart_pops;
	}
}


/*******************************************************************/
/** Deinitialize all registered M-Modules
 *
 * \return 			-
 */
static void deinit_devices(void)
{

 	UARTMOD_INFO *mmod;
	struct list_head *tmp, *element;
	struct ox16c954_port *up;
	unsigned int i;

	/*
	 * 1. shutdown all UARTs physically 
	 */
	for (i=0; i < MAX_SNGL_UARTS ; i++) {
		if ( men_uart_ports[i].port.membase ) 
		{
			M77DBG3(KERN_INFO "deinit port %d\n", i);
			up = (struct ox16c954_port *)&men_uart_ports[i].port;
			serial_out(up, 	UART_IER, 0);
			serial_out(up, 	UART_LCR, 0);
			serial_icr_write(up, UART_CSR, 0); /* Reset the UART */
			/* unregister the UARTs from the driver subsystem */
			men_uart_unregister_port( i );
		}	
	}

	/*
	 * 2. Unregister everything in the MDIS context (external MDIS devices) 
	 */
	M77DBG2(KERN_INFO "Unregister external MDIS devices\n");
    list_for_each( tmp, &G_uartModListHead ) {
		mmod = list_entry(tmp, UARTMOD_INFO, head);
		if (mmod->mdisDev) {			
			M77DBG2(KERN_INFO "Closing Device %s \n",mmod->deviceName);

			/* clear any left Interrupt & disable them */
			control_out(mmod->memBase, M77_REG_IR, 0x01 );
			control_out(mmod->memBase, M77_REG_IR, 0x00 );

			/* Clear TCR/DCR Registers back to powerup values*/
			if (mmod->modtype == MOD_M77) {
				for (i = M77_DCR_REG_BASE; i < M77_DCR_REG_BASE + 4; i++ )
					control_out(mmod->memBase, i << 1, M77_RS422_HD );
			}

			if (mmod->modtype == MOD_M45) {
				control_out(mmod->memBase, M45_REG_IR2, 		0x01 );
				control_out(mmod->memBase, M45_REG_IR2, 		0x00 );
				control_out(mmod->memBase, M45_TCR1_REG << 1, 	0x00 );
				control_out(mmod->memBase, M45_TCR2_REG << 1, 	0x00 );
			}
			M77DBG2(KERN_INFO "Closing external device %s \n",mmod->deviceName);
			mdis_close_external_dev( mmod->mdisDev );
		}
	}
	/*
	 * 3. kfree kmalloc'ed memory and resources
	 */
	list_for_each_safe(element, tmp, &G_uartModListHead) { // Liste freigeben
		mmod = list_entry(element, UARTMOD_INFO, head);
		M77DBG2(KERN_INFO "Now freeing space for '%s'\n", mmod->deviceName );
		list_del( element );
		kfree( list_entry( element, UARTMOD_INFO, head));
    }

}


/*******************************************************************/
/** Select the M77 phymode
 *
 * \param mod		\IN  per-module struct of M-Module data
 *
 * \return 			-
 */
static void	parse_m77_phyinfo( UARTMOD_INFO *mmod_data, unsigned int idx )
{

    unsigned int j = 0, m77mode=0;
	for ( j = 0; j < 4; j++ ) {

		mmod_data->mode[j] = 0;
		mmod_data->echo[j] = 0;
		m77mode = mode[(idx*4) + j];

		if (m77mode) {
			/* sanity check of passed mode */
			if ( (m77mode == M77_RS422_FD) || (m77mode == M77_RS422_HD) ||
				 (m77mode == M77_RS485_HD) || (m77mode == M77_RS485_FD) ||
				 (m77mode == M77_RS232)) {
				mmod_data->mode[j] = m77mode;
				mmod_data->echo[j] = !!echo[(idx*4) + j];
				M77DBG("mmod_data->mode[%d] = %d  ", j, mmod_data->mode[j] );
				M77DBG("mmod_data->echo[%d] = %d\n", j, mmod_data->echo[j] );
			} else {
				printk(KERN_ERR "** Parameter mode[%d]=%d invalid, ignored\n",
					   j, m77mode );
				mmod_data->mode[j] = 0;
				mmod_data->echo[j] = 0;
			}
		}
	}
}


/*******************************************************************/
/** Register the 4 or 8 UART Channels of this M-Module 
 *
 * \param mod		\IN  per-module struct of M-Module data
 *
 * \return 			0 on success or error code
 */
static int register_uarts(UARTMOD_INFO *mod )
{
	int retval = 0, nrChan = 0 ;
	unsigned char dcr_val = 0;
	unsigned int tmpmode = 0;
	struct ox16c954_port *ox; 
	void *baseAdr;

	switch ( mod->modtype ) {
	case MOD_M45:
		mod->nrChannels = MOD_M45_CHAN_NUM;
		break;
	case MOD_M69:
		mod->nrChannels = MOD_M69_CHAN_NUM;
		break;
	case MOD_M77:
		mod->nrChannels = MOD_M77_CHAN_NUM;
		break;
	default: 
		printk(KERN_ERR "*** invalid M-Module type 0x%04x!\n", mod->modtype );
		return -ENODEV;
	}

	/*
	 *	Enable Interrupts in the additional IR Registers on locations:
	 *	M77:	0x48
	 *	M69N:	0x48
	 *	M45N:	0x48, 0xc8
	 */
	switch ( mod->modtype ) {
	case MOD_M45:
		M77DBG2("Init M45N Registers\n");
		control_out( mod->memBase, M45_REG_IR1, M77_IR_IMASK );
		control_out( mod->memBase, M45_REG_IR2, M77_IR_IMASK );
		break;

	case MOD_M69:
		M77DBG2("Init M69N Register\n");
		control_out( mod->memBase, M69_REG_IR, M77_IR_IMASK );
		break;

	case MOD_M77:
		/* On M77 also enable the galvanic isolated Drivers */
		M77DBG2("Init M77 Register \n");		
		control_out( mod->memBase, M77_REG_IR, M77_IR_IMASK | M77_IR_DRVEN );
		break;
	}


	/*  Register all channels of this M-Module  */
	for ( nrChan = 0; nrChan < mod->nrChannels; nrChan++ ) {
		dcr_val = 0;
		mod->uart.flags 	= UPF_SHARE_IRQ | UPF_BOOT_AUTOCONF;
		mod->uart.uartclk 	= MM_UARTCLK;
		mod->uart.iotype 	= UPIO_MEM;
		mod->uart.irq 		= 0xff; /* dummy Info, unused by this driver */
		mod->uart.regshift 	= 1;	/* already done in serial_in/out */
		mod->uart.fifosize 	= 128;
		mod->uart.type 		= PORT_16C950;

		baseAdr = mod->memBase + (0x10 * nrChan);

		/* correct M45N Adress Gap between chan. 0-3 and 4-7 */
		if ( (mod->modtype == MOD_M45 ) && (nrChan > 3) )
			baseAdr+=0x40;

		/* use same address for mem/mapbase, shown as "MMIO" at loading */
		mod->uart.membase 	= baseAdr;
		mod->uart.mapbase 	= (unsigned long)(unsigned long*)baseAdr;
		
		if ((retval=men_uart_register_port(&mod->uart, mod->modtype, nrChan, 
										   &mod->port8250[nrChan]))<0) {

			printk(KERN_ERR "*** Error during registering UART %d!\n", nrChan);
			return retval;
		} else
			mod->line = retval;

		ox = mod->port8250[nrChan]; /* is valid now */

		/* on M77, also set phy mode and echo and switch it on */
		tmpmode = mod->mode[nrChan];
		if ( mod->modtype == MOD_M77 && tmpmode ) {
			dcr_val |= tmpmode;
			/* echoing only for HD modes! unknown effects at other modes.. */
			if ( (( tmpmode==M77_RS422_HD) || (tmpmode==M77_RS485_HD )) && (mod->echo[nrChan])) 
			{
				dcr_val |= M77_RX_EN;
			}
			control_out(mod->memBase, (M77_DCR_REG_BASE+nrChan) << 1, dcr_val);
			
			/* save M77 mode */
			ox->m77Mode = tmpmode;
		}
	}
	return retval;
}


/*******************************************************************/
/** main enumeration and initialization Function of all Modules
 *
 * \brief
 * The passed module Parameters are parsed and for each given Module
 * and a call to mdis_open_external_dev() is done. For each new 
 * carrier board a call to mdis_install_external_irq() is done because
 * from CPU View the IRQ s of all M-Modules on the same Carrier are
 * equal.
 *
 * \return 			0 on success or negative error code
 */
static int m77_init_devices(void)
{

	u_int32 modtype = 0, devid = 0, devrev = 0, modnr = 0;
	char moddevname[64];
    unsigned int m_idx = 0;
	int retval = 0;
    char device[ARRLEN];
	char prevBrdName[ARRLEN];
	UARTMOD_INFO *mmod_data = NULL;

	memset( device, 0x0, sizeof(device));
	memset( prevBrdName, 0x0, sizeof(prevBrdName));

    INIT_LIST_HEAD( &G_uartModListHead );	

	if ( brdName[m_idx] == NULL ) {
		printk(KERN_ERR 
			   " *** Error: No BBIS device specified, e.g.'brdName=d201_1'\n");
		retval = -EINVAL;
		goto errout;
	}

	/*-----------------------------+
	 |  For each M-Module do..     |
     +-----------------------------*/
    while( devName[m_idx] )  {
		
		strncpy( device, devName[m_idx], ARRLEN-1 );

		/* kmalloc one UARTMOD_INFO struct per M-Module   */	
		if ((mmod_data = kmalloc(sizeof(UARTMOD_INFO), GFP_KERNEL)) == NULL ){
			retval = -ENOMEM;
			goto errout;
		}
		memset( mmod_data, 0x0, sizeof(UARTMOD_INFO) );

		/* store index, devicename, list element etc */
		mmod_data->modnum = m_idx;	
		strncpy( mmod_data->brdName, brdName[m_idx], ARRLEN-1 );
		list_add(&mmod_data->head, &G_uartModListHead );

		if ( slotNo[m_idx] > 3 ) {
			printk(KERN_ERR " *** Error: Slotnumber %d invalid (use 0..3)\n", 
				   slotNo[m_idx]);
			retval = -EINVAL;
			goto errout;
		}

		retval=mdis_open_external_dev( device, 
					       brdName[m_idx],
					       slotNo[m_idx], 
					       MDIS_MA08, 
					       MDIS_MD08, 
					       256, 
					       &mmod_data->memBase,
					       NULL, 
					       &mmod_data->mdisDev );
		
		M77DBG3("called mdis_open_external_dev for '%s' board '%s' slot %d.\n",
				devName[m_idx], brdName[m_idx], slotNo[m_idx] );
		M77DBG3("returnvalue %d. membase=0x%08x\n",retval, mmod_data->memBase);

		if (retval < 0) {
			printk(KERN_ERR "*** open '%s' failed: board %s slot %d!\n",
				   devName[m_idx], brdName[m_idx], slotNo[m_idx] );
			printk(KERN_ERR "hint: forgot mdis_createdev -b %s ?\n",
				   brdName[m_idx]);

			retval 	= -EIO;
			goto errout;
		}

		if ( m_getmodinfo( (unsigned long)((unsigned long*)mmod_data->memBase), 
				   &modtype, &devid, 
				   &devrev, moddevname) == 0) {
			M77DBG3("getmodinfo found: '%s' devID: 0x%08x\n",moddevname,devid);

			modnr = devid & 0xffff;
			mmod_data->modtype = modnr;
			strncpy( mmod_data->deviceName, device, ARRLEN-1 );

			/*  sanity checks against wrong driver insertion */
			if (strncmp("m45" ,devName[m_idx], 3 ) && (modnr == MOD_M45)){
				printk(KERN_ERR "*** Error: passed '%s' but found '%s'\n",
					   devName[m_idx], moddevname );
				retval = -ENODEV;
				goto errout;	
			}
			
			if (strncmp("m69" ,devName[m_idx], 3) && (modnr == MOD_M69)) {
				printk(KERN_ERR "*** Error: passed '%s' but found '%s'\n",
					   devName[m_idx], moddevname );
				retval = -ENODEV;
				goto errout;
			} 
			
			if (strncmp("m77" ,devName[m_idx], 3) && (modnr == MOD_M77)) {
				printk(KERN_ERR "*** Error: passed '%s' but found '%s'\n",
					   devName[m_idx], moddevname );
				retval = -ENODEV;
				goto errout;				
			} 

			if ((modnr != MOD_M45) && (modnr != MOD_M69) &&(modnr != MOD_M77)){
				printk(KERN_ERR "*** Error: unknown Module (0x%04x)!\n", devid);
				retval = -ENODEV;
				goto errout;				
			} 

		} else {
			printk("*** Error identifying M-Module!\n");
			retval = -ENODEV;
			goto errout;
		}
		
		M77DBG("Carrier now %s: Registering new ISR\n", brdName[m_idx] );
		retval = mdis_install_external_irq(	mmod_data->mdisDev,	M77_IrqHandler,
											(void*)mmod_data);
		if ( retval < 0 ) {	
			printk(KERN_ERR "*** install irq error: %d\n", retval);
			retval = -EBUSY;
			goto errout;
		}

		if ( mmod_data->modtype == MOD_M77 ) 		
			parse_m77_phyinfo(mmod_data, m_idx);

		/* Register all UART channels of this M-Module */
		register_uarts(mmod_data);

		retval = mdis_enable_external_irq( mmod_data->mdisDev );
		if ( retval < 0 ) {	
			printk(KERN_ERR "*** enable irq error: %d\n", retval);
			retval = -EBUSY;
			goto errout;
		}
		/* current carrier becomes old one  */
		strncpy(prevBrdName, brdName[m_idx], ARRLEN);

		m_idx++; 

	} /* end while(brdname... */

	/* was any M-Module processed at all ?  */
	if (!m_idx) {
		printk(KERN_ERR 
			   " *** Error: No MDIS Device specified, e.g. 'devName=m45_1'\n");
		retval = -ENODEV;
		goto errout;
	}

	return 0; 
 errout:
	deinit_devices();	
	return retval;
}

/*******************************************************************/
/** module init function
 */
static int __init m77_serial_init(void)
{

	int ret = 0;
	printk(KERN_INFO "MEN M45/69/77 driver version %s\n", RCSid);
	printk(KERN_INFO "Up to %d modules supported\n", MAX_MODS_SUPPORTED );

	/* 2. Init the global Array of 16550(950) like ports */
	men_uart_init_ports();

	/* 3. Register platform Driver */
	ret = uart_register_driver( &men_uart_reg );
	if (ret) {
		printk(KERN_ERR "*** uart_register_driver returned %d\n", ret );
		goto out;
	}

	/* 4. Register the passed UART M-Modules */
	ret = m77_init_devices();

	if (ret) {
		printk(KERN_ERR "*** m77_init_devices returned %d\n", ret );
	}

	if ( ret < 0 ) {		
		uart_unregister_driver(&men_uart_reg);
		return ret;
	} else if ( ret )
		goto unreg;

 out:
	return ret;

 unreg:
	deinit_devices();
	uart_unregister_driver(&men_uart_reg);
	return ret;
}

/**********************************************************************/
/** Driver's cleanup routine
 */
static void __exit m77_serial_cleanup(void)
{
	deinit_devices();
	uart_unregister_driver(&men_uart_reg);
	return;
}

module_init(m77_serial_init);
module_exit(m77_serial_cleanup);

MODULE_LICENSE( "GPL" );
MODULE_DESCRIPTION("MEN M77/M69N/M45N serial driver");
MODULE_AUTHOR("thomas schnuerer<thomas.schnuerer@men.de>");
