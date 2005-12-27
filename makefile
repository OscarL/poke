## BeOS Generic Makefile v2.2 ##

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

RSRCS = \
	 poke.rsrc

# @<-src@
#%}

#LIBS = readline termcap
LIBS = editline termcap

LIBPATHS = ./editline/ /boot/home/Desktop/poke/lib/
SYSTEM_INCLUDE_PATHS = 
LOCAL_INCLUDE_PATHS = ./include

#OPTIMIZE = FULL
#DEFINES = READLINE_LIBRARY

WARNINGS = ALL
SYMBOLS = FALSE
DEBUGGER = FALSE
#COMPILER_FLAGS = -DDONT_USE_LINE_EDIT
COMPILER_FLAGS = -DUSE_BSD_EDITLINE
LINKER_FLAGS = -s
APP_VERSION =

## include the makefile-engine
include $(BUILDHOME)/etc/makefile-engine
