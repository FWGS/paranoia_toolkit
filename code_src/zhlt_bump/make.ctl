ifeq ($(MODE),)
MODE:=release
endif

ifeq ($(MAPS),)
MAPS:=1
endif

ifeq ($(MODE),super_debug)
USE_ELECTRIC_FENCE:=1
endif

LINT:=1
