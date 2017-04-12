#
# GNU 'posix' gcc/g++ compiler
#

ifeq ($(PLATFORM),posix)
ifeq ($(COMPILER),gnu)
OBJEXT:=.o
LIBEXT:=.a
DLLEXT:=.a
CPPFLAGS:=-Wall
CCFLAGS:=-Wall
EXE_FLAGS:=$(DEFAULT_LIBRARIES) -lm -lpthread

ifeq ($(USE_ELECTRIC_FENCE),1)
EXE_FLAGS+=-lefence
endif

LIB_FLAGS:=-Bdynamic -r
INCLUDEDIRS +=
DEFINES += -D GNU_COMPILER

CC:=gcc
CPP:=g++
LINK:=ld


ifeq ($(MODE),super_debug)
# Debug
CCFLAGS += -c -g
CPPFLAGS += -c -g
EXE_FLAGS += 
endif
ifeq ($(MODE),debug)
# Debug
CCFLAGS += -c -g
CPPFLAGS += -c -g
EXE_FLAGS += 
endif
ifeq ($(MODE),release)
# Release 
CCFLAGS += -c -O2 -g
CPPFLAGS += -c -O2 -g
EXE_FLAGS += 
endif
ifeq ($(MODE),release_w_symbols)
# Release 
CCFLAGS += -c -O2 -g
CPPFLAGS += -c -O2 -g
EXE_FLAGS += 
endif

# COMPILER=gnu
endif
# PLATFORM=posix
endif
