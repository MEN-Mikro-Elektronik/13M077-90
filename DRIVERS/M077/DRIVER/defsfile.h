/***********************  I n c l u d e  -  F i l e  ************************
 *  
 *         Name: defsfile.h
 *
 *       Author: win
 *        $Date: 2007/08/15 17:05:59 $
 *    $Revision: 1.2 $
 * 
 *  Description: header file for m77-tty-driver
 *				 
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _DEFSFILE_H
#define _DEFSFILE_H

struct M77dev_struct{
    void  *G_dev;
    MACCESS  G_ma;                /* virtual mapped adress */
    char          *brdName;       /* device name of carrier board */
    unsigned char  slotNo;        /* slot number on carrier board */
    u_int32        irqVector;     /* VME-interrupt line */
    u_int8         irq_cnt;       /* counter for used irqs */
    struct M77chan_struct   *m77chan[4];     /* channel struct */
    struct M77dev_struct    *m77dev_prev;    /* For the linked list */
    struct M77dev_struct    *m77dev_next;
};

struct M77chan_struct{
    void *G_ma;                   /* virtual mapped address  */
    unsigned char *iomem_base;    /* virtual address for register access */
    struct async_struct *info;
    u_int8  channel_nr;           /* Channel-nr. [0-3] */
    u_int8  minor;                /* line, retval from register_serial */
    u_int16 physInt;              /* interface mode */
    u_int8  echo_supress;         /* enable receive line */
    u_int16 ACR;                  /* content of ACR-register */
    struct M77dev_struct     *m77dev;        /* M77 device (modul) */
    struct M77chan_struct    *m77chan_prev;  /* For the linked list */
    struct M77chan_struct    *m77chan_next;
};

struct M77dev_struct *m77struct = 0;

static int InitModuleParm(void);
static int GetM77Chan(u_int8 minor, struct M77chan_struct **m77chan_res);
static int M77PhysIntSet(struct M77chan_struct *m77chan, u_int16 drvMode);

#define M77_SIZE_FIFO            128

/* offset for M77 HW line driver configuration register & ISR register */
#ifdef M77_DCR_REG_BASE
#undef M77_DCR_REG_BASE		/* dont confuse with value from serial_m77.h */
#define M77_DCR_REG_BASE	0x40    /* offset of Driver config. register */
#endif

#define M77_IRQ_REG		0x48

/* equates for M77 configuration/interrupt register */
#define M77_IRQ_CLEAR		0x01
#define M77_IRQ_EN		0x02
#define M77_TX_EN		0x04
#define M77_RX_EN               0x08

/* equates for additional control register */
#define M77_ACR_HD_DRIVER_CTL   0x18  /* line driver ctrl. in HD-mode */

#endif // _DEFSFILE_H
