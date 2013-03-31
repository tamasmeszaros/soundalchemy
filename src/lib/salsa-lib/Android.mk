LOCAL_PATH := $(call my-dir)/src

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= control.c cards.c pcm.c pcm_params.c pcm_misc.c hcontrol.c mixer.c hwdep.c timer.c
LOCAL_MODULE := libsalsa
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

include $(BUILD_STATIC_LIBRARY)
