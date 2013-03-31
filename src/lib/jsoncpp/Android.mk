LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= $(LOCAL_PATH)
LOCAL_CFLAGS    := -fpermissive -fexceptions
LOCAL_SRC_FILES:= src/json_reader.cpp src/json_value.cpp src/json_writer.cpp

LOCAL_MODULE := libjsoncpp
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

include $(BUILD_STATIC_LIBRARY)
