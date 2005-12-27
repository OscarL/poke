//
// Copyright 2005, Haiku Inc. Distributed under the terms of the MIT license.
// Author(s):
// - Oscar Lesta <oscar@users.berlios.de>.
//

#include "poke_io.h"

#include "SupportDefs.h"
#include "PCI.h"
#include "pci_info.h"


////////////////////////////////////////////////////////////////////////////////
// (fake) Driver access

status_t open_poke_driver(void)
{
 	return pci_init();
}


void close_poke_driver(void)
{
	pci_uninit();
}


////////////////////////////////////////////////////////////////////////////////
// Ports I/O

// See "4.32 Assembler Instructions with C Expression Operands" on GCC docs for
// help with "built-in" asm.
//
// Basically: ("<instruction> [params]" : [output] : [input] [,] [more_in])
//

uint8 in_port_8(uint16 port)
{
	uint8 value;
	asm volatile("inb %%dx,%%al" : "=a" (value) : "d" (port));
	return value;
}


uint16 in_port_16(uint16 port)
{
	uint16 value;
	asm volatile("inw %%dx,%%ax" : "=a" (value) : "d" (port));
	return value;
}


uint32 in_port_32(uint16 port)
{
	uint32 value;
	asm volatile("inl %%dx,%%eax" : "=a" (value) : "d" (port));
	return value;
}


void out_port_8(uint16 port, uint8 value)
{
	asm volatile("outb %%al,%%dx" : : "a" (value), "d" (port));
}


void out_port_16(uint16 port, uint16 value)
{
	asm volatile("outw %%ax,%%dx" : : "a" (value), "d" (port));
}


void out_port_32(uint16 port, uint32 value)
{
	asm volatile("outl %%eax,%%dx" : : "a" (value), "d" (port));
}


////////////////////////////////////////////////////////////////////////////////
// Indexed Port I/O

uint8 in_port_indexed(uint16 port, uint8 index)
{
	out_port_8(port, index);
	return in_port_8(port + 1);
}


void out_port_indexed(uint16 port, uint8 index, uint8 value)
{
	out_port_8(port, index);
	out_port_8(port + 1, value);
}


////////////////////////////////////////////////////////////////////////////////
// PCI I/O


uint32
poke_read_pci_config(uint8 bus, uint8 device, uint8 function, uint8 offset,
					uint8 size)
{
	uint32 result;
	uint32 tmp = 0x80000000;
	tmp |= (bus << 16 | device << 11 | function << 8 | (offset & ~0x3));

	out_port_32(0xCF8, tmp);

	switch (size) {
		case 1:
			result = in_port_8(0xCFC + (offset & 3));
		break;

		case 2:
			if ((offset & 3) != 3)
				result = in_port_16(0xCFC + (offset & 3));
			else {
				result = B_BAD_VALUE;
				printf("read_pci_config: can't read 2 bytes at reg %d (offset 3)!\n", offset);
			}
		break;

		case 4:
			if ((offset & 3) == 0)
				result = in_port_32(0xCFC);
			else {
				result = B_BAD_VALUE;
				printf("read_pci_config: can't read 4 bytes at reg %d (offset != 0)!\n", offset);
			}
		break;

		default:
			printf("read_pci_config: called for %d bytes!!\n", size);
	}

	return result;
}


void
poke_write_pci_config(uint8 bus, uint8 device, uint8 function, uint8 offset,
						uint8 size, uint32 value)
{
	uint32 tmp = 0x80000000;
	tmp |= (bus << 24 | device << 16 | function << 8 | (offset & ~0x3));

	out_port_32(0xCF8, tmp);

	switch (size) {
		case 1:
			out_port_8(0xCFC + (offset & 3), value);
		break;
		case 2:
			if ((offset & 3) != 3)
				out_port_16(0xCFC + (offset & 3), value);
			else
				printf("write_pci_config: can't write 2 bytes at reg %d (offset 3)!\n", offset);
		break;
		case 4:
			if ((offset & 3) == 0)
				out_port_32(0xCFC, value);
			else
				printf("write_pci_config: can't write 4 bytes at reg %d (offset != 0)!\n", offset);
		break;
		default:
			printf("write_pci_config: called for %d bytes!!\n", size);
	}
}


status_t
poke_get_nth_pci_info(int index, pci_info* pciinfo)
{
	return pci_get_nth_pci_info(index, pciinfo);
}


////////////////////////////////////////////////////////////////////////////////
// Memory access. Empty stubs, because mem commands aren't supported yet in DOS

int memory_state_of(uint32 memAddress)
{
	return MEM_UNAVAILABLE;
}


area_id
poke_map_physical_mem(uint32 phys_address, uint32* virtual_address, uint32* offset)
{
	return B_ERROR;
}


void
poke_unmap_physical_mem(area_id area)
{

}
