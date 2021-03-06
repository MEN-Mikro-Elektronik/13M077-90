#**************************  M a k e f i l e ********************************
#  
#         Author: nw/ts
#  
#    Description: makefile descriptor for MDIS LL-Driver 
#                      
#-----------------------------------------------------------------------------
#   Copyright 2003-2019, MEN Mikro Elektronik GmbH
#*****************************************************************************
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

MAK_NAME=lx_m77_sw
# the next line is updated during the MDIS installation
STAMPED_REVISION="13M077-90_01_12-7-g9e77041-dirty_2019-05-28"

DEF_REVISION=MAK_REVISION=$(STAMPED_REVISION)

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
		$(SW_PREFIX)$(DEF_REVISION) \
		   $(SW_PREFIX)MAC_BYTESWAP   \
		   $(SW_PREFIX)ID_SW

MAK_INP1=serial_m77$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
