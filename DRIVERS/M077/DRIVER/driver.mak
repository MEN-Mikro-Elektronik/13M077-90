#**************************  M a k e f i l e ********************************
#  
#         Author: nw/ts
#          $Date: 2007/08/14 10:39:48 $
#      $Revision: 1.3 $
#  
#    Description: makefile descriptor for M77 Driver, unswapped
#                      
#---------------------------------[ History ]---------------------------------
# 
# 
# 29.05.2015 ts:  removed 2.4 support, named driver file straight 
#
# ------------ end of CVS control ---------------
# Revision 1.3  2007/08/14 10:39:48  ts
# module name changed to lx_m77
# used source file depends on $(VERSION_SUFFIX)
#
# Revision 1.2  2007/06/25 17:08:45  ts
# Cosmetic, Log entry for Header added
#
# Revision 1.1  11.06.2003 10:06:04 by kp
# Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

MAK_NAME=lx_m77

MAK_LIBS=

MAK_INCL=$(MEN_INC_DIR)/mdis_com.h   \
          $(MEN_INC_DIR)/men_typs.h   \
          $(MEN_INC_DIR)/oss.h        \
          $(MEN_INC_DIR)/mdis_err.h   \
          $(MEN_INC_DIR)/maccess.h    \
          $(MEN_INC_DIR)/mdis_api.h   \
	  $(MEN_MOD_DIR)/serialP_m77.h \
	  $(MEN_MOD_DIR)/serial_m77.h


MAK_OPTIM=$(OPT_1)

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED 

MAK_INP1=serial_m77$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

