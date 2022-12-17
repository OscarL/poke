//
// Copyright 2005, Haiku Inc. Distributed under the terms of the MIT license.
// Author(s):
// - Oscar Lesta <oscar@users.berlios.de>.
//

#ifndef _POKE_COMMANDS_H_
#define _POKE_COMMANDS_H_

#if defined(__BEOS__) || defined(__HAIKU__)
	#include <SupportDefs.h>
#else
	#include "SupportDefs.h"
#endif

// In poke_commands_mem.c

void command_db(int argc, uint32 argv[]);
void command_dw(int argc, uint32 argv[]);
void command_dl(int argc, uint32 argv[]);
void command_dm(int argc, uint32 argv[]);
void command_sb(int argc, uint32 argv[]);
void command_sw(int argc, uint32 argv[]);
void command_sl(int argc, uint32 argv[]);

void command_dpb(int argc, uint32 argv[]);
void command_dpw(int argc, uint32 argv[]);
void command_dpl(int argc, uint32 argv[]);
void command_dpm(int argc, uint32 argv[]);
void command_spb(int argc, uint32 argv[]);
void command_spw(int argc, uint32 argv[]);
void command_spl(int argc, uint32 argv[]);

void command_dumpvm(int argc, uint32 argv[]);
void command_dumppm(int argc, uint32 argv[]);


// In poke_commands_ports.c

void command_inb(int argc, uint32 argv[]);
void command_inw(int argc, uint32 argv[]);
void command_inl(int argc, uint32 argv[]);
void command_outb(int argc, uint32 argv[]);
void command_outw(int argc, uint32 argv[]);
void command_outl(int argc, uint32 argv[]);

void command_idxinb(int argc, uint32 argv[]);
void command_idxoutb(int argc, uint32 argv[]);


// In poke_commands_pci.c

void command_pci(int argc, uint32 argv[]);
void command_pcilist(int argc, uint32 argv[]);

void command_cfinb(int argc, uint32 argv[]);
void command_cfinw(int argc, uint32 argv[]);
void command_cfinl(int argc, uint32 argv[]);
void command_cfoutb(int argc, uint32 argv[]);
void command_cfoutw(int argc, uint32 argv[]);
void command_cfoutl(int argc, uint32 argv[]);


#endif	// _POKE_COMMANDS_H_
