NETVIS_DEFINES+=-I hlvis -I netvis -D ZHLT_NETVIS -D CXX_IMPORTS
NETVIS:=$(OUTDIR)/netvis$(EXEEXT)
NETVIS_SRCDIR:=netvis
NETVIS_OUTDIR:=$(NETVIS_SRCDIR)/$(OUTDIR)

ifeq ($(PLATFORM),posix)
NETVIS_LIBRARIES=-lccxx -lz
endif

ifeq ($(PLATFORM),win32)
NETVIS_LIBRARIES=ws2_32.lib zlib-fastcall.lib
ifeq ($(MODE),super_debug)
NETVIS_LIBRARIES+=ccxx32d.lib
endif
ifeq ($(MODE),debug)
NETVIS_LIBRARIES+=ccxx32d.lib
endif
ifeq ($(MODE),release)
NETVIS_LIBRARIES+=ccxx32.lib
endif
ifeq ($(MODE),release_w_symbols)
NETVIS_LIBRARIES+=ccxx32.lib
endif
endif

NETVIS_INPUT := \
$(NETVIS_SRCDIR)/SocketThreads.cpp \
$(NETVIS_SRCDIR)/NetvisSession.cpp \
$(NETVIS_SRCDIR)/packet.cpp \
$(HLVIS_SRCDIR)/flow.cpp \
$(HLVIS_SRCDIR)/vis.cpp \
$(COMMON_SRCDIR)/blockmem.cpp \
$(COMMON_SRCDIR)/bspfile.cpp \
$(COMMON_SRCDIR)/cmdlib.cpp \
$(COMMON_SRCDIR)/filelib.cpp \
$(COMMON_SRCDIR)/log.cpp \
$(COMMON_SRCDIR)/mathlib.cpp \
$(COMMON_SRCDIR)/messages.cpp \
$(COMMON_SRCDIR)/scriplib.cpp \
$(COMMON_SRCDIR)/threads.cpp \


NETVIS_OUTPUT := \
$(NETVIS_OUTDIR)/SocketThreads$(OBJEXT) \
$(NETVIS_OUTDIR)/NetvisSession$(OBJEXT) \
$(NETVIS_OUTDIR)/packet$(OBJEXT) \
$(NETVIS_OUTDIR)/flow$(OBJEXT) \
$(NETVIS_OUTDIR)/vis$(OBJEXT) \
$(NETVIS_OUTDIR)/blockmem$(OBJEXT) \
$(NETVIS_OUTDIR)/bspfile$(OBJEXT) \
$(NETVIS_OUTDIR)/cmdlib$(OBJEXT) \
$(NETVIS_OUTDIR)/filelib$(OBJEXT) \
$(NETVIS_OUTDIR)/log$(OBJEXT) \
$(NETVIS_OUTDIR)/mathlib$(OBJEXT) \
$(NETVIS_OUTDIR)/messages$(OBJEXT) \
$(NETVIS_OUTDIR)/scriplib$(OBJEXT) \
$(NETVIS_OUTDIR)/threads$(OBJEXT) \


