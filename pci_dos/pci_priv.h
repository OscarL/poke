// gcc -O3 -c pci_info.cpp
// gcc -O3 -c pci_config.c

/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __PCI_PRIV_H__
#define __PCI_PRIV_H__

#include "OSDefs.h"

#define PCI_CONFIG_ADDRESS	0xCF8	// I/O port for PCI config space address
#define PCI_CONFIG_DATA		0xCFC	// I/O port for PCI config space data

/*
extern device_manager_info *gDeviceManager;
extern pci_device_module_info gPCIDeviceModule;
extern spinlock	gConfigLock;
*/

extern int	gMaxBusDevices;
extern bool gIrqRouterAvailable;

#if __BEOS__
#define PCI_LOCK_CONFIG(cpu_status) \
{ \
	cpu_status = disable_interrupts(); \
	acquire_spinlock(&gConfigLock); \
}

#define PCI_UNLOCK_CONFIG(cpu_status) \
{ \
	release_spinlock(&gConfigLock); \
	restore_interrupts(cpu_status); \
}

#else
	#define cpu_status int
	#define PCI_LOCK_CONFIG(cpu_status)
	#define PCI_UNLOCK_CONFIG(cpu_status)
#endif

#ifdef __cplusplus
extern "C" {
#endif

status_t 	pci_config_init(void);
uint32		pci_read_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size);
void		pci_write_config(uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32	value);
void		*pci_ram_address(const void *physical_address_in_system_memory);

status_t 	pci_io_init(void);
uint8		pci_read_io_8(int mapped_io_addr);
void		pci_write_io_8(int mapped_io_addr, uint8 value);
uint16		pci_read_io_16(int mapped_io_addr);
void		pci_write_io_16(int mapped_io_addr, uint16 value);
uint32		pci_read_io_32(int mapped_io_addr);
void		pci_write_io_32(int mapped_io_addr, uint32 value);

status_t	pci_irq_init(void);
uint8		pci_read_irq(uint8 bus, uint8 device, uint8 function, uint8 pin);
void		pci_write_irq(uint8 bus, uint8 device, uint8 function, uint8 pin, uint8 irq);

#ifdef __cplusplus
}
#endif

#endif	// __PCI_PRIV_H__
