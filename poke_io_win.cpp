#include "poke_io.h"
#include "pci_info.h"

// Using DScaler's driver until I find out how to build a driver with GCC/MingW
#include "DScalerDriver.h"

// Finally! we can create WDM (.sys) drivers with GCC+MinGW using this:
// gcc -o driver.sys driver1.c driver2.c -s -shared -Wl,--entry,_DriverEntry@8 -nostartfiles -nostdlib -L. -lntoskrn

#include <stdio.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Poke driver

static TDScalerDriver* sDriver;


void close_poke_driver()
{
 	if (sDriver)	delete sDriver;
	pci_uninit();
}


status_t open_poke_driver()
{
	close_poke_driver();

	sDriver = new TDScalerDriver();
	if ((sDriver != NULL) && (sDriver->LoadDriver() == true))
		return pci_init();

	return B_NO_INIT;
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Ports I/O

uint32 in_port(uint16 port, uint8 size)
{
	status_t status = B_ERROR;
	TDSDrvParam cmd;

	if (sDriver == NULL) return (unsigned)-1;

	cmd.dwAddress = port;
	cmd.dwValue = 0;
	cmd.dwFlags = 0;

	DWORD in_val = 0;
	DWORD tmp = 0;

	switch (size) {
		case 1:
			status = sDriver->SendCommand(IOCTL_DSDRV_READPORTBYTE,
											&cmd, sizeof(cmd),
											&in_val, sizeof(in_val),
                                            &tmp);
		break;

		case 2:
			status = sDriver->SendCommand(IOCTL_DSDRV_READPORTWORD,
											&cmd, sizeof(cmd),
											&in_val, sizeof(in_val),
                                            &tmp);
		break;

		case 4:
			status = sDriver->SendCommand(IOCTL_DSDRV_READPORTDWORD,
											&cmd, sizeof(cmd),
											&in_val, sizeof(in_val),
                                            &tmp);
		break;
	}

//	printf("status = 0x%X, cmd.dwAddress = 0x%X, cmd.dwValue = 0x%X, "
//			"cmd.dwFlags = 0x%X, in_val = 0x%x\n",
//			status, cmd.dwAddress, cmd.dwValue, cmd.dwFlags, in_val);

	if (status != ERROR_SUCCESS/* || cmd.dwFlags != size*/)
		return status;
		printf("Error while reading port.\n");

	return in_val;
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
	TDSDrvParam cmd;

	if (!sDriver) return;

	cmd.dwAddress = port;
	cmd.dwValue = value;
	cmd.dwFlags = value;

	switch (size) {
		case 1:
			sDriver->SendCommand(IOCTL_DSDRV_WRITEPORTBYTE, &cmd, sizeof(cmd));
		break;

		case 2:
			sDriver->SendCommand(IOCTL_DSDRV_WRITEPORTWORD, &cmd, sizeof(cmd));
		break;

		case 4:
			sDriver->SendCommand(IOCTL_DSDRV_WRITEPORTDWORD, &cmd, sizeof(cmd));
		break;
	}

//	printf("cmd.dwAddress = 0x%X, cmd.dwValue = 0x%X, cmd.dwFlags = 0x%X\n",
//			cmd.dwAddress, cmd.dwValue, cmd.dwFlags);

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


uint8
in_port_indexed(uint16 port, uint8 index)
{
	out_port_8(port, index);
	return in_port_8(port + 1);
}


void
out_port_indexed(uint16 port, uint8 index, uint8 value)
{
	out_port_8(port, index);
	out_port_8(port + 1, value);
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - PCI

status_t
poke_read_pci_config(uint8 bus, uint8 device, uint8 function, uint8 offset,
					uint8 size)
{
	status_t result;
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
				printf("read_pci_config: can't read 2 bytes at reg %d (offset 3)"
						"!\n", offset);
			}
		break;

		case 4:
			if ((offset & 3) == 0)
				result = in_port_32(0xCFC);
			else {
				result = B_BAD_VALUE;
				printf("read_pci_config: can't read 4 bytes at reg %d "
						"(offset != 0)!\n", offset);
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
				printf("write_pci_config: can't write 2 bytes at reg %d "
						"(offset 3)!\n", offset);
		break;
		case 4:
			if ((offset & 3) == 0)
				out_port_32(0xCFC, value);
			else
				printf("write_pci_config: can't write 4 bytes at reg %d "
						"(offset != 0)!\n", offset);
		break;
		default:
			printf("write_pci_config: called for %d bytes!!\n", size);
	}
}


status_t poke_get_nth_pci_info(int index, pci_info* pciinfo)
{
	return pci_get_nth_pci_info(index, pciinfo);
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - MemMap

int memory_state_of(uint32 memAddress)
{
	int tmp = IsBadReadPtr((const void*) memAddress, 4);
	if (tmp == 0)
		return MEM_OK;

	if (tmp == 1)
		return MEM_NOT_MAPPED;		// educated guess.

	printf("Can't touch that mem (tmp = %d)\n", tmp);
	// not sure about this. I've no clue about the meaning of the return value.
	return MEM_PROTECTED;
}


// sadly, this is in Ring 0
//void* __cdecl _MapPhysToLinear(const void* phys_addr, uint32 bytes, uint32 flags);


area_id
poke_map_physical_mem(uint32 phys_address, uint32* virtual_address, uint32* off)
{
/*
	void* tmp = _MapPhysToLinear((void*) phys_address, B_PAGE_SIZE, 0);
	if (tmp)
		*virtual_address = tmp;
*/
	TDSDrvParam cmd;

	status_t status = B_ERROR;
	DWORD virtAddress = 0;
	uint32 offset = 0;
	DWORD dummy = 0;

	// Doesn't seems to be necessary (or even correct) for _MapPhysToLinear()
	// [note the difference with BeOS's map_physical_mem()]

//	offset = phys_address & (B_PAGE_SIZE - 1);
//	phys_address -= offset;

	cmd.dwAddress = 0;						// bus number (used by the NT version)
	cmd.dwValue   = (DWORD) phys_address;
	cmd.dwFlags   = (DWORD) B_PAGE_SIZE;	// size

	if (sDriver != NULL)
		status = sDriver->SendCommand(IOCTL_DSDRV_MAPMEMORY, &cmd, sizeof(cmd),
									&virtAddress, sizeof(DWORD), &dummy);

	if ((status != ERROR_SUCCESS)/* || (dummy != sizeof(DWORD))*/) {
		printf("Error maping memory\n");
		return status;
	}

	*virtual_address = (DWORD) (virtAddress + offset);
	*off = offset;
	return B_OK;
}


void
poke_unmap_physical_mem(area_id area)
{
	// This is a NO-OP in the Win9x DScaler driver so... don't bother.
	// OTOH... Rudolf would like to use it under XP so...
	TDSDrvParam cmd;
	cmd.dwAddress = area;
	cmd.dwValue   = B_PAGE_SIZE;

	if (sDriver)
		sDriver->SendCommand(IOCTL_DSDRV_UNMAPMEMORY, &cmd, sizeof(cmd));
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Misc

void snooze(uint32 miliseconds)
{
	uint32 start = GetTickCount();
	while (GetTickCount() <= (start + miliseconds)) {
		;
	}
}


#ifdef __cplusplus
}
#endif
