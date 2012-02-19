LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libflowers-jni
LOCAL_SRC_FILES := gl_context.c
LOCAL_LDLIBS    := -landroid -llog -lEGL -lGLESv2

include $(BUILD_SHARED_LIBRARY)
