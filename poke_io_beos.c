//
// Copyright 2005, Haiku Inc. Distributed under the terms of the MIT license.
// Author(s):
// - Oscar Lesta <oscar@users.berlios.de>.
//

#include "poke_io.h"

#include <errno.h>
#include <unistd.h>

#include <private/drivers/poke.h>


static int sFD = -1;		// File Descriptor for the poke driver


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Poke driver

void close_poke_driver()
{
	if (sFD) close(sFD);
	sFD = -1;
}


status_t open_poke_driver()
{
	close_poke_driver();
	sFD = open(POKE_DEVICE_FULLNAME, O_RDWR);
	if (sFD < 0)
		return errno;

	return B_OK;
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Ports I/O

uint32 in_port(uint16 port, uint8 size)
{
	status_t status = B_ERROR;
	port_io_args cmd;

	cmd.signature = POKE_SIGNATURE;
	cmd.port = port;
	cmd.size = size;
	cmd.value = 0;

	status = ioctl(sFD, POKE_PORT_READ, &cmd);
	if (status < B_OK)
		return status;

	return cmd.value;
}


uint8 in_port_8(uint16 port)
{
	return (uint8) in_port(port, 1);
}


uint16 in_port_16(uint16 port)
{
	return (uint16) in_port(port, 2);
}


uint32 in_port_32(uint16 port)
{
	return in_port(port, 4);
}


void out_port(uint16 port, uint32 value, uint8 size)
{
	port_io_args cmd;

	cmd.signature = POKE_SIGNATURE;
	cmd.port = port;
	cmd.size = size;
	cmd.value = value;

	if (sFD)	ioctl(sFD, POKE_PORT_WRITE, &cmd);
}


void out_port_8(uint16 port, uint8 value)
{
	out_port(port, value, 1);
}


void out_port_16(uint16 port, uint16 value)
{
	out_port(port, value, 2);
}


void out_port_32(uint16 port, uint32 value)
{
	out_port(port, value, 4);
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Indexed Port I/O

uint8 in_port_indexed(uint16 port, uint8 index)
{
	status_t status = B_ERROR;
	port_io_args cmd;

	cmd.signature = POKE_SIGNATURE;
	cmd.port = port;
	cmd.size = index;
	cmd.value = 0;

	if (sFD)
		status = ioctl(sFD, POKE_PORT_INDEXED_READ, &cmd);

	if (status < B_OK)
		return status;

	return cmd.value;
}


void out_port_indexed(uint16 port, uint8 index, uint8 value)
{
	port_io_args cmd;

	cmd.signature = POKE_SIGNATURE;
	cmd.port = port;
	cmd.size = index;
	cmd.value = value;

	if (sFD) ioctl(sFD, POKE_PORT_INDEXED_WRITE, &cmd);
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - PCI access

uint32
poke_read_pci_config(uint8 bus, uint8 device, uint8 function, uint8 offset,
					uint8 size)
{
	status_t status = B_ERROR;
	pci_io_args cmd;

	cmd.signature = POKE_SIGNATURE;
	cmd.bus       = bus;
	cmd.device    = device;
	cmd.function  = function;
	cmd.size      = size;
	cmd.offset    = offset;
	cmd.value     = 0;

	if (sFD)
		status = ioctl(sFD, POKE_PCI_READ_CONFIG, &cmd);

	if (status < B_OK)
		return status;

	return cmd.value;
}


void
poke_write_pci_config(uint8 bus, uint8 device, uint8 function, uint8 offset,
						uint8 size, uint32 value)
{
	pci_io_args cmd;

	cmd.signature = POKE_SIGNATURE;
	cmd.bus       = bus;
	cmd.device    = device;
	cmd.function  = function;
	cmd.size      = size;
	cmd.offset    = offset;
	cmd.value     = value;

	if (sFD)	ioctl(sFD, POKE_PCI_WRITE_CONFIG, &cmd);
}


status_t
poke_get_nth_pci_info(int index, pci_info* pciinfo)
{
	status_t status = B_ERROR;
	pci_info_args cmd;

	cmd.signature = POKE_SIGNATURE;
	cmd.index     = index;
	cmd.info      = pciinfo;
	cmd.status    = B_ERROR;

	if (sFD)
		status = ioctl(sFD, POKE_GET_NTH_PCI_INFO, &cmd);

	if (status < B_OK)
		return status;

	return cmd.status;
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Memory Access

int memory_state_of(uint32 memAddress)
{
	area_id area = -1;
	area_info info;

	area = area_for((void*) memAddress);
	if (area < 0)
		return MEM_NOT_MAPPED;	// memAddress not in local address space.

	// are B_READ_AREA or B_WRITE_AREA set?
	if (get_area_info(area, &info) != B_OK || (!info.protection))
		return MEM_PROTECTED;

	return MEM_OK;
}


area_id
poke_map_physical_mem(uint32 phys_address, uint32* virtual_address, uint32* off)
{
	status_t status = B_ERROR;
	uint32 offset = 0;
	mem_map_args cmd;

	offset = phys_address & (B_PAGE_SIZE - 1);
	phys_address -= offset;

	cmd.signature = POKE_SIGNATURE;
	cmd.name      = "poke_area";	// forget to name the area and await doom.
	cmd.area      = -1;
	cmd.size      = B_PAGE_SIZE;
	cmd.physical_address = (void*) phys_address;
	cmd.protection = B_READ_AREA | B_WRITE_AREA;
	cmd.flags = B_ANY_KERNEL_ADDRESS;

	if (sFD)
		status = ioctl(sFD, POKE_MAP_MEMORY, &cmd);

	if (status < B_OK)
		return status;

	*virtual_address = (uint32) (cmd.address) + offset;
	*off = offset;
	return cmd.area;
}


void poke_unmap_physical_mem(area_id area)
{
	mem_map_args cmd;

	cmd.signature = POKE_SIGNATURE;
	cmd.area      = area;

	if (sFD)
		ioctl(sFD, POKE_UNMAP_MEMORY, &cmd);
}
