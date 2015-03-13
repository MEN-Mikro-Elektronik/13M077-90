/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  serial_m77_doc.c
 *
 *      \author  thomas.schnuerer@men.de
 *        $Date: 2007/08/27 10:36:50 $
 *    $Revision: 1.3 $
 *
 *      \brief   User documentation for native Linux driver of
 *				 M45N, M69N and M77
 *
 *     Required: -
 *
 *     \switches -
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: serial_m77_doc.c,v $
 * Revision 1.3  2007/08/27 10:36:50  ts
 * Example for M77 mode/echo passing added
 *
 * Revision 1.2  2007/08/15 11:44:38  ts
 * finalized, typos corrected, examples and troubleshoot section
 * added
 *
 * Revision 1.1  2007/08/13 15:26:54  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

/*! \mainpage Linux native driver for quad and octal UART M-Modules.

    The men_lx_m77(_sw) module serves as a driver for the M-Modules M69N 
	(quad UART with RTS/CTS Handshake lines), M45N (octal UART with handshake 
	lines and additional Tristating functionality), and M77. \n

    The driver supports the Oxford Semiconductor Ox16C954 quad UART which
    is used on all 3 Modules. It is based on the 8250.c driver and
	has been adapted to make use of the special underlaying Hardware, which 
	can be any of the MEN M-Module Carriers. This also means that any standard
	tool for configuring the serial Interfaces can be used, like stty. Driver
	tests have mainly been done with stty, minicom, and the m77_ioctl tool
	for executing the special ioctls of the driver. \n

	\n \section installation Driver build and Installation

	\subsection builddrv Driver building

	The driver package 13m07790.zip can be added to the MDIS Wizard like any
	other driver package, then the Project can be built either as part of an
	ElinOS Project (on MEN PowerPC CPUs) or Selfhosted (most x86 CPUs or
	standard PCs). See the MDIS User Manual for detailled instructions.

	The driver also needs Information about the Carrier boards base address and
	IRQ Information on which the Module is mounted on, so the MDIS kernel 
	drivers from the MDIS System Package are needed. These are built and
	installed automatically when the driver is configured and built using 
	the MDIS Wizard. \n

	\subsection install_mdis creating carrier device prior to driver usage

	Because the driver calls the mdis_open_external_device() Function from the
	MDIS core it is necessary that the Carrier device is instanciated prior
	to loading the driver. This is done via executing the mdis_createdev tool:

\verbatim
	mdis_createdev -b d201_1
\endverbatim

    In this example a D201 carrier board was opened and registered. The 
	Information about which (e.g. cPCI) Slot this Carrier is located at has to
	be configured in the MDIS Wizard. 
	Also the Modules descriptor files have to be available, in the examples 
	below they are m45_1.bin, m69_1.bin and m77_1.bin located in /etc/mdis/. 
	\n
    \subsection sernam Serial Port device nodes and naming 

	A sufficient number of Device node entries in /dev/ must exist too,
    the entries must be named ttyD<nr> and have the major nr. 19.

\verbatim
/#ls -la /dev/ttyD*
crw-rw-r--  1 root root 19, 0 Jan  1 01:06 /dev/ttyD0
crw-rw-r--  1 root root 19, 1 Jun 14  2007 /dev/ttyD1
crw-rw-r--  1 root root 19, 2 Jun 14  2007 /dev/ttyD2
crw-rw-r--  1 root root 19, 3 Jun 14  2007 /dev/ttyD3
crw-rw-r--  1 root root 19, 4 Jun 14  2007 /dev/ttyD4
crw-rw-r--  1 root root 19, 5 Jun 14  2007 /dev/ttyD5
crw-rw-r--  1 root root 19, 6 Jun 14  2007 /dev/ttyD6
crw-rw-r--  1 root root 19, 7 Jun 14  2007 /dev/ttyD7
\endverbatim

    they can be created with the mknod command, in ElinOS this can be included
	in the root filesystem. The device names and major number have been kept 
	separate to be able to coexist with other standard 16550 UARTs that are	
	supported by the regular 8250.c driver.
   
	\n \section ioctls Special ioctl codes supported by the driver

	The following special ioctl Codes which are not part of the serial core
	driver are supported.

	\subsection ioctl_m45 M45N ioctl codes

	The M45N Module can put its outputs to Tristate Mode, but not all of them 
	Separately. The User Manual for the Register Layout of Registers TCR1
	and TCR2 explains how each Channels tristate Mode is controlled:
\verbatim
Code: M45_TIO_TRI_MODE    Arguments: 0 (Tristate off, normal Operation)
                                     1 (Tristate on, UART is idle)
\endverbatim
\n
    \subsection ioctl_m77 M77 ioctl codes

	Beside RS232, the M77 offers differential physical Modes RS422 and RS485.
	The RS422 Mode from the previous M77 Variant is not supported!
\verbatim
Code: M77_ECHO_SUPPRESS  Arguments: 0 (Echo in HD Modes suppressed)
                                    1 (Echo in HD Modes enabled)

Code: M77_PHYS_INT_SET   Arguments:  M77_RS422_HD
                                     M77_RS422_FD
                                     M77_RS485_HD
                                     M77_RS485_FD
                                     M77_RS232
\endverbatim

    See LINUX/DRIVERS/M077/DRIVER/serial_m77.h for their definitions.

	\n \section parameter Module Parameter

    The driver supports the same Parameters as the previous kernel-2.4-only
	driver did. However, the additional support of the Modules M45N and M69N
	makes it necessary to pass the MDIS Descriptor names of the Modules too.

	\subsection available Parameters

	- devName			MDIS Descriptor Module Name, e.g. 'm77_1','m45_2'
	- brdName			MDIS Descriptor Carrier Name, e.g. 'd201_1'
	- slotNo			slot number on carrier board e.g. 0..3 on D201
	- mode				(For M77 only) Interface mode of M77 channels: 
	   - 1 		RS422 Half Duplex
	   - 2 		RS422 Full Duplex 
	   - 3 		RS485 Half Duplex
	   - 4		RS485 Full Duplex
	   - 7		RS232 Full Duplex

	- echo
	  disable/enable receive line of a M77 channel in HD modes

	\subsection Examples For Module loading

	The following examples explain passing the Parameters when loading the
	driver. This can be done either manually with modprobe or a default module
	config file of the used Linux Distribution, e.g. /etc/modules on Ubuntu.

	All driver Parameters are internally arrays, so they are passed comma
	separated. Up to 4 Modules are supported.

	The first example shows driver usage for a M45N which is mounted on a D201
	Carrier board, Module Slot 1.
\verbatim
modprobe men_lx_m77 devName=m45_1 brdName=d201_1 slotNo=1
\endverbatim
	The second example shows driver usage for a M69N and M77 which are 
	mounted on a D201 Carrier board, Slots 0 and 2.
\verbatim
modprobe men_lx_m77 devName=m69_1,m77_1 brdName=d201_1,d201_1 slotNo=0,2
\endverbatim
    So, all Parameters are parsed in array-like order. That means, all Values passed first after each parameter belong together, then the values passed at second and so on. If a M77 is mounted a D201 carrier and another on a second carrier the Parameters look like
\verbatim
modprobe men_lx_m77 devName=m77_1,m77_2 brdName=d201_1,d201_2 slotNo=1,2 mode=1,0,0,0,1,0,0,0 echo=0,0,0,0,0,0,0,0
\endverbatim
So the first 4 Values of mode and echo Parameter are assigned to the first M77, the second 4 Values to the second one and so on.

	The third example shows driver usage for a M077 on a D203 
	(A24 Version used), Slot 2, with Channel 0 and 1 in RS422 HD mode and echo
    enabled and channel 2 and 3 in RS232 Mode:
\verbatim
modprobe men_lx_m77 devName=m77_1 brdName=d203_a24_1 slotNo=2 mode=1,1,7,7 echo=1,1,0,0
\endverbatim
    For M69N and M45N the mode and echo Parameters are ignored.

	\n \section trouble Troubleshooting

	In case of problems with using the driver there are some points that should
	be checked before MEN support should be contacted:
	- Problems regarding module load
	      - does the Version of the built modules match the running kernel? This can happen on Selfhosted build environments with several kernel sources installed in /usr/src/. Execute e.g. the commands 'modinfo men_lx_m77' and 'uname -a' to see if the versions match.			
		  - were the module dependencies updated ? 
		     run a 'depmod' command in case an 'unresolved symbols' error occurs

    - Problems opening a UART channel
	      - were all device nodes /dev/ttyD* properly created (Major number 19) ?
            See \ref sernam
		  - is another process in background blocking the UART? check with 'ps ax' command 
	- Problems setting a non RS232 PHY Mode on M77
	      - was the mode Parameter properly passed to the driver? 
            Mind that the Mode can also be changed at runtime with the given special ioctl calls, see \ref ioctl_m77.
	
	\subsection irqs displayed interrupt number on module load
	the kernel messages like 'ttyD0 at MMIO 0xc9036e00 (irq = 255) is a 16550A' upon loading can be confusing, the IRQ shown here is not the one used for the M-Module, its the one used by the Carrier board the M-Module is mounted on. This number is not known at load time. Instead, use the 'cat /proc/interrupts' command to query the correct interrupt number, it should be equal to the one of the carriers PCI-to-M-Module bridge.

	\subsection loadexample Example of a typical driver load on a D003 PPC CPU
	the following example shows the Messages seen on the driver verification
	system (D003 with D201 carrier on cPCI Slot 4)

\verbatim
/#uname -a
Linux (none) 2.6.15.7-ELinOS-295 #20 PREEMPT Thu Jul 5 11:37:27 CEST 2007 ppc unk
nown unknown GNU/Linux
/#
/#modprobe men_bb_d201_sw
MEN men_oss init_module
MEN men_id_sw init_module
MEN men_desc init_module
MEN men_pld_sw init_module
MEN BBIS Kernel init_module
MEN men_bb_d201_sw init_module
/#
/#modprobe men_mdis_kernel
MEN MDIS Kernel init_module
/#
/#mdis_createdev -b d201_1
create BBIS device d201_1
/#
/#modprobe men_lx_m77_sw devName=m77_1  brdName=d201_1 slotNo=2
MEN M45/69/77 driver build Aug 14 2007 15:06:27
ttyD0 at MMIO 0xc9036e00 (irq = 255) is a 16550A
ttyD1 at MMIO 0xc9036e10 (irq = 255) is a 16550A
ttyD2 at MMIO 0xc9036e20 (irq = 255) is a 16550A
ttyD3 at MMIO 0xc9036e30 (irq = 255) is a 16550A
/#
\endverbatim
\n

    See also the Linux Serial-HOWTO for more info, on www.tldp.org

*/


/** \page dummy
  \menimages
*/
