include $(CLEAR_VARS)
LOCAL_PATH      := $(NDK_PROJECT_PATH)
LOCAL_MODULE    := systrace_unity
LOCAL_CFLAGS    := -Werror

LOCAL_SRC_FILES := src/SystraceUnity.cpp
LOCAL_LDLIBS    += -landroid
LOCAL_LDLIBS    += -llog

include $(BUILD_SHARED_LIBRARY)
