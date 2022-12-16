//
// Copyright 2005, Haiku Inc. Distributed under the terms of the MIT license.
// Author(s):
// - Oscar Lesta <oscar@users.berlios.de>.
//

#ifndef _POKE_IO_H_
#define _POKE_IO_H_

#if defined(__BEOS__)
	#include <SupportDefs.h>
	#include <PCI.h>
#else
	#include "SupportDefs.h"
	#include "PCI.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


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
	MEM_UNAVAILABLE	= -3,	// generic error (like when not implemented)
	MEM_NOT_MAPPED	= -2,
	MEM_PROTECTED	= -1,
	MEM_OK			=  0	// ready to be peeked/poked
};

int		memory_state_of(uint32 memAddress);

area_id	poke_map_physical_mem(uint32 phys_address, uint32* virtual_address, uint32* offset);
void	poke_unmap_physical_mem(area_id area);


// Misc

#ifdef __INTEL__
void	pc_speaker_beep(uint16 freq, uint duration);
#define	BEEP()	pc_speaker_beep(1000, 125000)
#endif

#if !defined(__BEOS__)
void	snooze(uint32 microseconds);
#endif


#ifdef __cplusplus
}
#endif

#endif	// _POKE_IO_H_
