# Tell Android where to locate source files
# "my-dir" is a macro function which will return the path of the current 

LOCAL_PATH := $(call my-dir)

# Clear contents of the LOCAL_* variables
include $(CLEAR_VARS)

# All the source files to include in this module
LOCAL_SRC_FILES := jni.cpp \
                   canvas.cpp 

#LOCAL_SHARED_LIBRARIES += libffmpeg-prebuild 

# The name of the module
LOCAL_MODULE := libcaffemacchiato

# Compilation flags
LOCAL_CFLAGS := -Werror

# Static libraries to link with
LOCAL_LDLIBS := -lGLESv3 -lEGL

LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog

# Build the module
include $(BUILD_SHARED_LIBRARY)
