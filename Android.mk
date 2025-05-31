LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc

### Lib Name
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
	LOCAL_MODULE := SkyGFXMobile
else
	LOCAL_MODULE := SkyGFXMobile64
endif

### Source Files
LOCAL_CFLAGS = -O3 -mfloat-abi=softfp -mfpu=neon -DNDEBUG -std=c++14
LOCAL_CXXFLAGS = -O3 -mfloat-abi=softfp -mfpu=neon -DNDEBUG -std=c++14
LOCAL_SRC_FILES := main.cpp mod/logger.cpp mod/config.cpp

LOCAL_SRC_FILES += externs.cpp
LOCAL_SRC_FILES += skygfx/colorfilter.cpp skygfx/buildingpipe.cpp \
				   skygfx/shaders.cpp skygfx/shading.cpp \
				   skygfx/vehiclepipe.cpp skygfx/misc.cpp \
				   skygfx/envmap.cpp

### Libraries
LOCAL_LDLIBS += -llog

### Flags
LOCAL_CXXFLAGS += -DNEW_LIGHTING
#LOCAL_CXXFLAGS += -DSTOCHASTIC_TEX # Does not work for some reason. And causes a lag, heavy thing.

include $(BUILD_SHARED_LIBRARY)