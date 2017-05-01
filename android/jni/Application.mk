CFLAGS_OPT :=  -O3 -fomit-frame-pointer -funsafe-math-optimizations -ftree-vectorize -fgraphite-identity -floop-interchange -floop-block -funsafe-loop-optimizations -finline-limit=1024
CFLAGS_OPT_ARM := -mthumb -mfpu=neon -mcpu=cortex-a9 -pipe -mvectorize-with-neon-quad -DVECTORIZE_SINCOS
CFLAGS_OPT_ARMv5 :=-march=armv6 -mfpu=vfp -marm -pipe
CFLAGS_OPT_X86 := -mtune=atom -march=atom -mssse3 -mfpmath=sse -funroll-loops -pipe -DVECTORIZE_SINCOS
CFLAGS_HARDFP := -D_NDK_MATH_NO_SOFTFP=1 -mhard-float -mfloat-abi=hard -DLOAD_HARDFP -DSOFTFP_LINK
APPLICATIONMK_PATH = $(call my-dir)
GPG_SDK_PATH := $(call my-dir)/jni/cs-client/3rdparty/gpg-cpp-sdk/android/

XASH3D_CONFIG := $(APPLICATIONMK_PATH)/bs_config.mk

APP_ABI := x86 armeabi-v7a-hard armeabi
APP_MODULES := server #menu gpgs-1
APP_PLATFORM := android-9
APP_STL := gnustl_static
