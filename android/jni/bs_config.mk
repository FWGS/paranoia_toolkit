LOCAL_CFLAGS += $(CFLAGS_OPT) 
LOCAL_CPPFLAGS += -Wno-write-strings -Wno-invalid-offsetof -Wno-conversion-null
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
LOCAL_CFLAGS += $(CFLAGS_OPT_ARM) $(CFLAGS_HARDFP)
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CFLAGS += $(CFLAGS_OPT_ARM) -mfloat-abi=softfp
endif
ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_CFLAGS += $(CFLAGS_OPT_ARMv5)
endif
ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_CFLAGS += $(CFLAGS_OPT_X86)
endif

ifeq ($(NDK_DEBUG),1)
LOCAL_CFLAGS += -ggdb -DDEBUG
else
LOCAL_CFLAGS += -DNDEBUG
endif
