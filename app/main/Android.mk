LOCAL_PATH := $(call my-dir)
JNI_SRC_PATH := $(LOCAL_PATH)/src

include $(CLEAR_VARS)

LOCAL_MODULE    := main
LOCAL_SRC_FILES := \
	$(JNI_SRC_PATH)/c_main.c

LOCAL_LDLIBS    := -llog -lEGL -landroid -lGLESv3 

LOCAL_LDFLAGS   := -u ANativeActivity_onCreate

include $(BUILD_SHARED_LIBRARY)


