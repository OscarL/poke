## BeOS Generic Makefile v2.2 ##

NAME = ../poke
TYPE = APP

#%{
# @src->@ 

SRCS = \
	 poke.c  \
	 poke_commands_mem.c  \
	 poke_commands_pci.c  \
	 poke_commands_ports.c  \
	 poke_io_beos.c  \
	 speaker.c

RSRCS = \
	 Poke.rsrc

# @<-src@ 
#%}

#LIBS = readline termcap
LIBS = editline termcap

#LIBPATHS = ./readline/
LIBPATHS = ./editline/ /boot/home/Desktop/poke/lib/
SYSTEM_INCLUDE_PATHS = 
#LOCAL_INCLUDE_PATHS = ./readline/ ./poke_driver/
LOCAL_INCLUDE_PATHS = ./editline/ ./poke_driver/

#OPTIMIZE = FULL
#DEFINES = READLINE_LIBRARY

WARNINGS = ALL
SYMBOLS = FALSE
DEBUGGER = FALSE
#COMPILER_FLAGS = -DDONT_USE_LINE_EDIT
COMPILER_FLAGS = -DUSE_LINE_EDITING -DDONT_USE_MY_CODE
LINKER_FLAGS = -s
APP_VERSION =

## include the makefile-engine
include $(BUILDHOME)/etc/makefile-engine
