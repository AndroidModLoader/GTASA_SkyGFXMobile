LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc
LOCAL_MODULE    := SkyGfxMobile
LOCAL_SRC_FILES := main.cpp colorfilter.cpp shader.cpp shading.cpp pipeline.cpp effects.cpp plantsurfprop.cpp shadows.cpp rtshadowman.cpp mod/logger.cpp mod/config.cpp
LOCAL_CXXFLAGS = -O3 -mfloat-abi=softfp -mfpu=neon -DNDEBUG
LOCAL_CXXFLAGS += -DNEW_LIGHTING # Nah, too dark...
#LOCAL_CXXFLAGS += -DSTOCHASTIC_TEX # Does not work for some reason. And causes a lag, heavy thing.
LOCAL_C_INCLUDES += ./include
LOCAL_LDLIBS += -llog
include $(BUILD_SHARED_LIBRARY)