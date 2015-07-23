/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  m77_ioctl.c
 *
 *      \author  thomas.schnuerer@men.de
 *        $Date: 2007/08/13 15:38:15 $
 *    $Revision: 1.3 $
 * 
 *  	 \brief  Verification helper tool to call M45N/M77 specific ioctls
 * 				 for enable/disable of Tristate Mode (M45N) and setting
 *				 physical Interface Modes between RS232/422/485 (M77) 
 *
 *				 Build on Commandline using:
 *				 ppc_60x-gcc -Wall -o m77_ioctl m77_ioctl.c;
 *				 scp m77_ioctl 192.1.1.152:/usr/bin/
 *
 *     Switches: -
 *
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: m77_ioctl.c,v $
 * Revision 1.3  2007/08/13 15:38:15  ts
 * Options m and w removed
 * ioctl Values of M77_PHY_INT_SET adapted to serial_m77.h
 *
 * Revision 1.2  2007/08/06 17:49:05  ts
 * dont allow using wrong argument values for phy mode on M77
 *
 * Revision 1.1  2007/08/03 16:12:34  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "../serial_m77.h"


/***********************************************************************/
/*
 * Display Program usage
 */
void usage(void)
{
	
	printf(" m77_ioctl [hkvm]<-d device> [-t=0/1] [-p=0].\n");	
	printf(" device: UART device /dev/ttyD0 to /dev/ttyDn\n");
	printf("\n");
	printf("Example for M45N specific ioctls (Tristate control):\n");
	printf(" m77_ioctl /dev/ttyDn -t 1    set Chan. n to Tristate\n");
	printf(" m77_ioctl /dev/ttyDn -t 0    set Chan. n to normal Operation\n");
	printf("\n");

	printf("Example for M77 specific ioctls (physical Mode Control):\n");
	printf(" m77_ioctl /dev/ttyDn -p 1    RS422 Halfduplex\n");
	printf(" m77_ioctl /dev/ttyDn -p 2    RS422 Fullduplex\n");
	printf(" m77_ioctl /dev/ttyDn -p 3    RS485 Halfduplex\n");
	printf(" m77_ioctl /dev/ttyDn -p 4    RS485 Fullduplex\n");
	printf(" m77_ioctl /dev/ttyDn -p 7    RS232\n");

	printf("Example for M77 specific ioctls (Echo suppression in HD):\n");
	printf(" m77_ioctl /dev/ttyDn -s 0  suppress echo (DCR[RX_EN] = 0)\n");
	printf(" m77_ioctl /dev/ttyDn -s 1  Enable echo (DCR[RX_EN]   = 1)\n");
	printf("\n");

	printf(" Arguments without Value:\n");
	printf(" -k   program ends after Enter is pressed\n");
	printf(" -v   verbose outputs of whats done\n");
	printf(" -h   help, dumps this usage text\n");
	exit(1);

}



/***********************************************************************/
/*
 * the only main function
 *
 */
int main(int argc, char *argv[]) 
{

	FILE *fd 	= 0;
	int option 	= 0;
	int val 	= 0;
	int retval 	= 0;
	int nverbose = 0;
	int nkeypress = 0;

	/* map given phy mode (equal to definition in serial_m77.h) to a string*/
	char *phyModes[8]={" ", "RS422HD", "RS422FD", "RS485HD", "RS485FD",
					   " ", " ", "RS232" };
	
	if (nverbose) {
		printf( "MEN M45N/M69N/M77 Linux driver ioctl test\n");
	}

	if (argc < 2)
		usage();

	while ((option = getopt(argc, argv, "vhkd:t:p:s:")) >=0 ) {
		switch (option) {

		case 'k':
			nkeypress = 1;
			break;

		case 'v':
			nverbose = 1;
			break;

		case 'h':
			usage();
			break;

		case 'd':
			if (nverbose)
				printf( "Opening UART device %s\n", optarg );

			if ( !(fd = fopen( optarg, "rw"))) {
				printf("*** cant open device %s !\n", optarg);
				exit(1);
			}
			break;

		case 'p':
			val = atoi(optarg);
			if ((val!=1) && (val!=2) && (val!=3) && (val!=4) && (val!=7)) 
			{
				printf("*** unknown PHY Mode '%d'. Use 1,2,3,4 or 7\n",val);
				exit(1);
			}

			if (nverbose)
				printf("Set M77 Physical Mode to '%s'\n",  phyModes[val] );
			retval = ioctl( fileno(fd), M77_PHYS_INT_SET, val );

			break;

		case 't':
			val = atoi(optarg);
			if (nverbose)
				printf("Set M45N Tristate Mode to %d\n", val ? 1 : 0 );
			retval = ioctl( fileno(fd), M45_TIO_TRI_MODE, val );
			break;

		case 's':
			val = atoi(optarg);
			if (nverbose)
				printf("Set M77 Echo Mode to %d\n", val ? 1 : 0);
			retval = ioctl( fileno(fd), M77_ECHO_SUPPRESS, val );
			break;

		case 'm':
			for (val = 0; val < 5; val ++) {
				if (nverbose)
					printf("Set M77 PHY Mode to '%s'\n",  phyModes[val] );
				retval = ioctl( fileno(fd), M77_PHYS_INT_SET, val );
				
				sleep(1);
			}

			break;

		default:
			usage();
		}
		fflush(fd);
	}

	if (retval)
		printf("*** error %d while executing ioctl!", retval);


	if (nkeypress) {
		printf("Hit Enter to stop the program\n");	
		getc(stdin);
	}

	fclose(fd);
	return retval;

};
