#-------------------------------------------------------------------------------
# Android.mk
# 
# Copyright (c) 2013 Mészáros Tamás.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/gpl.html
# 
# Contributors:
#     Mészáros Tamás - initial API and implementation
#-------------------------------------------------------------------------------
TOP_PATH := $(call my-dir)
LOCAL_PATH := $(TOP_PATH)
include $(call all-subdir-makefiles)

LOCAL_PATH := $(TOP_PATH)
include $(CLEAR_VARS)

#LOCAL_C_INCLUDES:= $(LOCAL_PATH)/include
#LOCAL_MODULE    := soundalchemy
#LOCAL_SRC_FILES := jni_main.cpp logs.cpp
#LOCAL_STATIC_LIBRARIES := libllaudio
#LOCAL_LDLIBS := -llog
#
#include $(BUILD_SHARED_LIBRARY)

LOCAL_C_INCLUDES:= $(LOCAL_PATH)/llaudio $(LOCAL_PATH)/include $(LOCAL_PATH)/lib
LOCAL_MODULE    := soundalchemy
LOCAL_SRC_FILES := main.cpp logs.cpp dspserver.cpp clientconnector.cpp message.cpp androidconnector.cpp thread.cpp soundeffect.cpp
LOCAL_STATIC_LIBRARIES := libllaudio  libjsoncpp
LOCAL_LDLIBS := -llog

include $(BUILD_EXECUTABLE)
