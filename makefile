## BeOS Generic Makefile v2.2 ##

#########################################
## Please choose *one* of these options:
#########################################
# DONT_USE_LINE_EDITING = TRUE
# USE_READLINE = TRUE
USE_EDITLINE = TRUE
# USE_ECL = TRUE << not yet ready.
#########################################

NAME = ../poke
TYPE = APP

SRCS = \
	 poke_main.c  \
	 poke_commands_mem.c  \
	 poke_commands_pci.c  \
	 poke_commands_ports.c  \
	 poke_io_beos.c  \
	 pc_speaker.c

RSRCS = \
	 poke.rsrc

ifeq ($(DONT_USE_LINE_EDITING), )
	ifeq ($(USE_READLINE), TRUE)
		LIBS = readline history termcap
		LIBPATHS = ./readline
		LOCAL_INCLUDE_PATHS = ./readline 
		SYSTEM_INCLUDE_PATHS = ./readline 
		DEFINES = USE_READLINE READLINE_LIBRARY
	else
		ifeq ($(USE_EDITLINE), TRUE)
			LIBS = editline termcap
			LIBPATHS = ./editline
			LOCAL_INCLUDE_PATHS = ./editline
			SYSTEM_INCLUDE_PATHS = 
			DEFINES = USE_EDITLINE
		else
			LIBS = EditCL
			LIBPATHS = ./EditCL
			LOCAL_INCLUDE_PATHS = ./EditCL
			SYSTEM_INCLUDE_PATHS = 
			DEFINES = USE_ECL
		endif
	endif
else
	LIBPATHS = 
	SYSTEM_INCLUDE_PATHS = 
	DEFINES = DONT_USE_LINE_EDITING
endif

OPTIMIZE = FULL
WARNINGS = ALL
SYMBOLS = FALSE
DEBUGGER = FALSE
COMPILER_FLAGS =
LINKER_FLAGS = -s
APP_VERSION =

## include the makefile-engine
include $(BUILDHOME)/etc/makefile-engine
