LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc
LOCAL_MODULE    := SkyGfxMobile
LOCAL_SRC_FILES := main.cpp colorfilter.cpp shader.cpp shading.cpp mod/logger.cpp mod/config.cpp
LOCAL_CXXFLAGS = -O2 -mfloat-abi=softfp -DNDEBUG
LOCAL_C_INCLUDES += ./include
LOCAL_CFLAGS += -fPIC
#LOCAL_LDLIBS += -Wl,--no-warn-shared-textrel
LOCAL_LDLIBS += -llog
include $(BUILD_SHARED_LIBRARY)