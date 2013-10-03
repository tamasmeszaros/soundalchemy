#-------------------------------------------------------------------------------
# Copyright (c) 2013 Mészáros Tamás.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v2.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# 
# Contributors:
#     Mészáros Tamás - initial API and implementation
#-------------------------------------------------------------------------------
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= $(LOCAL_PATH)/llaudio  $(LOCAL_PATH)/drivers/salsa $(LOCAL_PATH)/../../external/include
LOCAL_MODULE    := llaudio
LOCAL_CFLAGS    := -fpermissive -fexceptions
LOCAL_SRC_FILES := lladevicemanager.cpp lladriver.cpp llastream.cpp llaaudiopipe.cpp \
				   lladevice.cpp llaerrorhandler.cpp \
				   drivers/salsa/salsadriver.cpp \
				   drivers/salsa/salsastream.cpp \
				   drivers/salsa/salsadevice.cpp 
				   
LOCAL_LDLIBS += -L$(LOCAL_PATH)/../../external/lib/arm_androideabi -lsalsa
	   
include $(BUILD_SHARED_LIBRARY)
