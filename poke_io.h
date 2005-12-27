//
// Copyright 2005, Haiku Inc. Distributed under the terms of the MIT license.
// Author(s):
// - Oscar Lesta <oscar@users.berlios.de>.
//

#ifndef _POKE_IO_H_
#define _POKE_IO_H_

#include "SupportDefs.h"
#include "PCI.h"


// Driver access

status_t	open_poke_driver(void);
void		close_poke_driver(void);


// Ports I/O
uint32	in_port(uint16 port, uint8 size);
void	out_port(uint16 port, uint32 value, uint8 size);

uint8	in_port_8 (uint16 port);
uint16	in_port_16(uint16 port);
uint32	in_port_32(uint16 port);

void	out_port_8 (uint16 port, uint8  value);
void	out_port_16(uint16 port, uint16 value);
void	out_port_32(uint16 port, uint32 value);


// Indexed Port I/O

uint8	in_port_indexed (uint16 port, uint8 index);
void	out_port_indexed(uint16 port, uint8 index, uint8 value);


// PCI I/O

uint32	poke_read_pci_config (uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size);
void	poke_write_pci_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value);

status_t	poke_get_nth_pci_info(int index, pci_info* pciinfo);


// Memory access

enum {
	MEM_UNAVAILABLE	= -3,	// can't access the memory (not implemented?)
	MEM_NOT_MAPPED	= -2,
	MEM_PROTECTED	= -1,
	MEM_OK			=  0	// ready to be peeked/poked
};

int		memory_state_of(uint32 memAddress);

area_id	poke_map_physical_mem(uint32 phys_address, uint32* virtual_address, uint32* offset);
void	poke_unmap_physical_mem(area_id area);


#ifdef __INTEL__

void	pc_speaker_play(uint16 freq, uint duration);
void	pc_speaker_on(uint16 freq);
void	pc_speaker_off(void);

#endif

#endif	// _POKE_IO_H_
