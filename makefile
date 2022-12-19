## Haiku Generic Makefile v2.6 ##

#########################################
USE_EDITLINE = TRUE
#########################################

NAME = ../poke
TYPE = APP

#%{
# @src->@
SRCS = \
	 poke_main.c  \
	 poke_commands_mem.c  \
	 poke_commands_pci.c  \
	 poke_commands_ports.c  \
	 poke_io_beos.c  \
	 pc_speaker.c

RDEFS = poke.rdef

RSRCS = poke.rsrc
# @<-src@
#%}


ifeq ($(USE_EDITLINE), TRUE)
	LIBS = edit
	DEFINES = USE_EDITLINE
else
	LIBS =
	DEFINES = DONT_USE_LINE_EDITING
endif


LIBPATHS =
SYSTEM_INCLUDE_PATHS =
LOCAL_INCLUDE_PATHS =

OPTIMIZE := FULL
WARNINGS = ALL
SYMBOLS :=
DEBUGGER :=
COMPILER_FLAGS =
LINKER_FLAGS =
DEFINES +=
APP_VERSION =

## Include the Makefile-Engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine

