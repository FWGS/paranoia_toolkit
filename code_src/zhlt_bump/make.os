#
# Platform setup/detection
#

ifeq ($(OS),Windows_NT)
COMPILER_BASE:=msvc
COMPILER:=intel
ifeq ($(PROCESSOR_ARCHITECTURE),x86)
PLATFORM:=win32
endif

MKDIR:=mkdir.exe
NUKE:=rm -rf
#SYSTEM_INCLUDE=-I "$(subst ;," -I ",$(INCLUDE))"
DEFINES += -D SYSTEM_WIN32

else

PLATFORM:=posix
COMPILER:=gnu
MKDIR:=mkdir
NUKE:=rm -rf
DEFINES += -D SYSTEM_POSIX

endif


#
# Compile mode (Platform/compiler independent)
#

OUTDIR:=$(MODE)

ifeq ($(MODE),super_debug)
# Debug
LIBDIR:=libd
RSC_FLAGS += -D _DEBUG
DEFINES += -D _DEBUG -D CHECK_HEAP
endif
ifeq ($(MODE),debug)
# Debug
LIBDIR:=libfd
RSC_FLAGS += -D _DEBUG
DEFINES += -D _DEBUG
endif
ifeq ($(MODE),release)
# Release 
LIBDIR:=lib
RSC_FLAGS += -D NDEBUG
DEFINES += -D NDEBUG
endif
ifeq ($(MODE),release_w_symbols)
# Release 
LIBDIR:=libr
RSC_FLAGS += -D NDEBUG
DEFINES += -D NDEBUG
endif

#
# Project settings (typically from make.ctl)
#

ifeq ($(UNICODE),1)
DEFINES += -D UNICODE -D _UNICODE
endif

ifeq ($(PLATFORM),win32)
DEFINES += -D WIN32 -D _WIN32
endif
