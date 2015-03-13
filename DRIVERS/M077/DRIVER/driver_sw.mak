#**************************  M a k e f i l e ********************************
#  
#         Author: nw/ts
#          $Date: 2007/08/14 10:40:51 $
#      $Revision: 1.2 $
#  
#    Description: makefile descriptor for MDIS LL-Driver 
#                      
#---------------------------------[ History ]---------------------------------
#
# $Log: driver_sw.mak,v $
# Revision 1.2  2007/08/14 10:40:51  ts
# module name changed to lx_m77
# used source file depends on $(VERSION_SUFFIX)
#
# Revision 1.2  2007/06/25 17:08:45  ts
# Cosmetic, Log entry for Header added
#
# Revision 1.1  11.06.2003 10:06:05 by kp
# Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

MAK_NAME=lx_m77_sw

MAK_LIBS=

MAK_INCL=$(MEN_INC_DIR)/mdis_com.h     \
         $(MEN_INC_DIR)/men_typs.h     \
         $(MEN_INC_DIR)/oss.h          \
         $(MEN_INC_DIR)/mdis_err.h     \
         $(MEN_INC_DIR)/maccess.h      \
         $(MEN_INC_DIR)/mdis_api.h     \
		 $(MEN_MOD_DIR)/serialP_m77.h  \
		 $(MEN_MOD_DIR)/serial_m77.h   \

MAK_OPTIM=$(OPT_1)

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED \
		   $(SW_PREFIX)MAC_BYTESWAP   \
		   $(SW_PREFIX)ID_SW

MAK_INP1=serial_m77_$(VERSION_SUFFIX)$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
