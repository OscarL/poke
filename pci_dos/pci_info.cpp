/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>

#include "PCI.h"

#include "pci_priv.h"
#include "pci_info.h"

#if 0
	#define TRACE(x)	printf x
#else
	#define TRACE(x)
#endif

bool gIrqRouterAvailable = false;
//spinlock gConfigLock = 0;

static PCI *sPCI;

#ifdef __cplusplus
extern "C" {
#endif

status_t
pci_init(void)
{
/*
	if (pci_io_init() != B_OK) {
		TRACE(("PCI: pci_io_init failed\n"));
		return B_ERROR;
	}
*/
	if (pci_config_init() != B_OK) {
		TRACE(("PCI: pci_config_init failed\n"));
		return B_ERROR;
	}
/*
	if (pci_irq_init() != B_OK)
		TRACE(("PCI: IRQ router not available\n"));
	else
*/
		gIrqRouterAvailable = true;

	sPCI = new PCI;

	return B_OK;
}


void
pci_uninit(void)
{
	delete sPCI;
}


long
pci_get_nth_pci_info(long index, pci_info *outInfo)
{
	return sPCI->GetNthPciInfo(index, outInfo);
}

#ifdef __cplusplus
}
#endif


PCI::PCI()
{
	fRootBus.parent = 0;
	fRootBus.bus = 0;
	fRootBus.child = 0;

	DiscoverBus(&fRootBus);

	RefreshDeviceInfo(&fRootBus);
}


PCI::~PCI()
{
}


status_t
PCI::GetNthPciInfo(long index, pci_info *outInfo)
{
	long curindex = 0;
	return GetNthPciInfo(&fRootBus, &curindex, index, outInfo);
}


status_t
PCI::GetNthPciInfo(PCIBus *bus, long *curindex, long wantindex, pci_info *outInfo)
{
	// maps tree structure to linear indexed view
	PCIDev *dev = bus->child;
	while (dev) {
		if (*curindex == wantindex) {
			*outInfo = dev->info;
			return B_OK;
		}
		*curindex += 1;
		if (dev->child && B_OK == GetNthPciInfo(dev->child, curindex, wantindex, outInfo))
			return B_OK;
		dev = dev->next;
	}
	return B_ERROR;
}


void
PCI::DiscoverBus(PCIBus *bus)
{
	TRACE(("PCI: DiscoverBus, bus %u\n", bus->bus));

	for (int dev = 0; dev < gMaxBusDevices; dev++) {
		uint16 vendor_id = pci_read_config(bus->bus, dev, 0, PCI_vendor_id, 2);
		if (vendor_id == 0xffff)
			continue;

		uint8 type = pci_read_config(bus->bus, dev, 0, PCI_header_type, 1);
		int nfunc = (type & PCI_multifunction) ? 8 : 1;
		for (int func = 0; func < nfunc; func++)
			DiscoverDevice(bus, dev, func);
	}
}

void
PCI::DiscoverDevice(PCIBus *bus, uint8 dev, uint8 func)
{
	TRACE(("PCI: DiscoverDevice, bus %u, dev %u, func %u\n", bus->bus, dev, func));

	uint16 device_id = pci_read_config(bus->bus, dev, func, PCI_device_id, 2);
	if (device_id == 0xffff)
		return;

	PCIDev *newdev = CreateDevice(bus, dev, func);

	uint8 base_class = pci_read_config(bus->bus, dev, func, PCI_class_base, 1);
	uint8 sub_class	 = pci_read_config(bus->bus, dev, func, PCI_class_sub, 1);
	if (base_class == PCI_bridge && sub_class == PCI_pci) {
		uint8 secondary_bus = pci_read_config(bus->bus, dev, func, PCI_secondary_bus, 1);
		PCIBus *newbus = CreateBus(newdev, secondary_bus);
		DiscoverBus(newbus);
	}
}


PCIBus *
PCI::CreateBus(PCIDev *parent, uint8 bus)
{
	PCIBus *newbus = new PCIBus;
	newbus->parent = parent;
	newbus->bus = bus;
	newbus->child = 0;

	// append
	parent->child = newbus;

	return newbus;
}

PCIDev *
PCI::CreateDevice(PCIBus *parent, uint8 dev, uint8 func)
{
	TRACE(("PCI: CreateDevice, bus %u, dev %u, func %u:\n", parent->bus, dev, func));

	PCIDev *newdev = new PCIDev;
	newdev->next = 0;
	newdev->parent = parent;
	newdev->child = 0;
	newdev->bus = parent->bus;
	newdev->dev = dev;
	newdev->func = func;

	ReadPciBasicInfo(newdev);

	TRACE(("PCI: vendor 0x%04x, device 0x%04x, class_base 0x%02x, class_sub 0x%02x\n",
		newdev->info.vendor_id, newdev->info.device_id, newdev->info.class_base, newdev->info.class_sub));

	// append
	if (parent->child == 0) {
		parent->child = newdev;
	} else {
		PCIDev *sub = parent->child;
		while (sub->next)
			sub = sub->next;
		sub->next = newdev;
	}
	
	return newdev;
}


uint32
PCI::BarSize(uint32 bits, uint32 mask)
{
	bits &= mask;
	if (!bits)
		return 0;
	uint32 size = 1;
	while (!(bits & size))
		size <<= 1;
	return size;
}

void
PCI::GetBarInfo(PCIDev* dev, uint8 offset, uint32 *address, uint32 *size, uint8 *flags)
{
	uint32 oldvalue = pci_read_config(dev->bus, dev->dev, dev->func, offset, 4);
	pci_write_config(dev->bus, dev->dev, dev->func, offset, 4, 0xffffffff);
	uint32 newvalue = pci_read_config(dev->bus, dev->dev, dev->func, offset, 4);
	pci_write_config(dev->bus, dev->dev, dev->func, offset, 4, oldvalue);

	*address = oldvalue & PCI_address_memory_32_mask;
	if (size)
		*size = BarSize(newvalue, PCI_address_memory_32_mask);
	if (flags)
		*flags = newvalue & 0xf;
}


void
PCI::GetRomBarInfo(PCIDev *dev, uint8 offset, uint32 *address, uint32 *size, uint8 *flags)
{
	uint32 oldvalue = pci_read_config(dev->bus, dev->dev, dev->func, offset, 4);
	pci_write_config(dev->bus, dev->dev, dev->func, offset, 4, 0xfffffffe); // LSB must be 0
	uint32 newvalue = pci_read_config(dev->bus, dev->dev, dev->func, offset, 4);
	pci_write_config(dev->bus, dev->dev, dev->func, offset, 4, oldvalue);

	*address = oldvalue & PCI_rom_address_mask;
	if (size)
		*size = BarSize(newvalue, PCI_rom_address_mask);
	if (flags)
		*flags = newvalue & 0xf;
}


void
PCI::ReadPciBasicInfo(PCIDev *dev)
{
	dev->info.vendor_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_vendor_id, 2);
	dev->info.device_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_device_id, 2);
	dev->info.bus = dev->bus;
	dev->info.device = dev->dev;
	dev->info.function = dev->func;
	dev->info.revision = pci_read_config(dev->bus, dev->dev, dev->func, PCI_revision, 1);
	dev->info.class_api = pci_read_config(dev->bus, dev->dev, dev->func, PCI_class_api, 1);
	dev->info.class_sub = pci_read_config(dev->bus, dev->dev, dev->func, PCI_class_sub, 1);
	dev->info.class_base = pci_read_config(dev->bus, dev->dev, dev->func, PCI_class_base, 1);
	dev->info.line_size = pci_read_config(dev->bus, dev->dev, dev->func, PCI_line_size, 1);
	dev->info.latency = pci_read_config(dev->bus, dev->dev, dev->func, PCI_latency, 1);
	// BeOS does not mask off the multifunction bit, developer must use (header_type & PCI_header_type_mask)
	dev->info.header_type = pci_read_config(dev->bus, dev->dev, dev->func, PCI_header_type, 1);
	dev->info.bist = pci_read_config(dev->bus, dev->dev, dev->func, PCI_bist, 1);
	dev->info.reserved = 0;
}

void
PCI::ReadPciHeaderInfo(PCIDev* dev)
{
	switch (dev->info.header_type & PCI_header_type_mask) {
		case 0:
		{
			// disable PCI device address decoding (io and memory) while BARs are modified
			uint16 pcicmd = pci_read_config(dev->bus, dev->dev, dev->func, PCI_command, 2);
			pci_write_config(dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd & ~(PCI_command_io | PCI_command_memory));

			// get BAR size infos
			GetRomBarInfo(dev, PCI_rom_base, (uint32*)&dev->info.u.h0.rom_base_pci, (uint32*)&dev->info.u.h0.rom_size);
			for (int i = 0; i < 6; i++) {
				GetBarInfo(dev, PCI_base_registers + 4*i,
					(uint32*) &dev->info.u.h0.base_registers_pci[i],
					(uint32*) &dev->info.u.h0.base_register_sizes[i],
					(uint8*) &dev->info.u.h0.base_register_flags[i]);
			}

			// restore PCI device address decoding
			pci_write_config(dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd);

			dev->info.u.h0.rom_base = (ulong)pci_ram_address((void *)dev->info.u.h0.rom_base_pci);
			for (int i = 0; i < 6; i++) {
				dev->info.u.h0.base_registers[i] = (ulong)pci_ram_address((void *)dev->info.u.h0.base_registers_pci[i]);
			}
			
			dev->info.u.h0.cardbus_cis = pci_read_config(dev->bus, dev->dev, dev->func, PCI_cardbus_cis, 4);
			dev->info.u.h0.subsystem_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_subsystem_id, 2);
			dev->info.u.h0.subsystem_vendor_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_subsystem_vendor_id, 2);
			dev->info.u.h0.interrupt_line = pci_read_config(dev->bus, dev->dev, dev->func, PCI_interrupt_line, 1);
			dev->info.u.h0.interrupt_pin = pci_read_config(dev->bus, dev->dev, dev->func, PCI_interrupt_pin, 1);
			dev->info.u.h0.min_grant = pci_read_config(dev->bus, dev->dev, dev->func, PCI_min_grant, 1);
			dev->info.u.h0.max_latency = pci_read_config(dev->bus, dev->dev, dev->func, PCI_max_latency, 1);
			break;
		}

		case 1:
		{
			// disable PCI device address decoding (io and memory) while BARs are modified
			uint16 pcicmd = pci_read_config(dev->bus, dev->dev, dev->func, PCI_command, 2);
			pci_write_config(dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd & ~(PCI_command_io | PCI_command_memory));

			GetRomBarInfo(dev, PCI_bridge_rom_base, (uint32*)&dev->info.u.h1.rom_base_pci);
			for (int i = 0; i < 2; i++) {
				GetBarInfo(dev, PCI_base_registers + 4*i,
					(uint32*) &dev->info.u.h1.base_registers_pci[i],
					(uint32*) &dev->info.u.h1.base_register_sizes[i],
					(uint8*) &dev->info.u.h1.base_register_flags[i]);
			}

			// restore PCI device address decoding
			pci_write_config(dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd);

			dev->info.u.h1.rom_base = (ulong)pci_ram_address((void *)dev->info.u.h1.rom_base_pci);
			for (int i = 0; i < 2; i++) {
				dev->info.u.h1.base_registers[i] = (ulong)pci_ram_address((void *)dev->info.u.h1.base_registers_pci[i]);
			}

			dev->info.u.h1.primary_bus = pci_read_config(dev->bus, dev->dev, dev->func, PCI_primary_bus, 1);
			dev->info.u.h1.secondary_bus = pci_read_config(dev->bus, dev->dev, dev->func, PCI_secondary_bus, 1);
			dev->info.u.h1.subordinate_bus = pci_read_config(dev->bus, dev->dev, dev->func, PCI_subordinate_bus, 1);
			dev->info.u.h1.secondary_latency = pci_read_config(dev->bus, dev->dev, dev->func, PCI_secondary_latency, 1);
			dev->info.u.h1.io_base = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_base, 1);
			dev->info.u.h1.io_limit = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_limit, 1);
			dev->info.u.h1.secondary_status = pci_read_config(dev->bus, dev->dev, dev->func, PCI_secondary_status, 2);
			dev->info.u.h1.memory_base = pci_read_config(dev->bus, dev->dev, dev->func, PCI_memory_base, 2);
			dev->info.u.h1.memory_limit = pci_read_config(dev->bus, dev->dev, dev->func, PCI_memory_limit, 2);
			dev->info.u.h1.prefetchable_memory_base = pci_read_config(dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_limit, 2);
			dev->info.u.h1.prefetchable_memory_limit = pci_read_config(dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_limit, 2);
			dev->info.u.h1.prefetchable_memory_base_upper32 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_base_upper32, 4);
			dev->info.u.h1.prefetchable_memory_limit_upper32 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_limit_upper32, 4);
			dev->info.u.h1.io_base_upper16 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_base_upper16, 2);
			dev->info.u.h1.io_limit_upper16 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_limit_upper16, 2);
			dev->info.u.h1.interrupt_line = pci_read_config(dev->bus, dev->dev, dev->func, PCI_interrupt_line, 1);
			dev->info.u.h1.interrupt_pin = pci_read_config(dev->bus, dev->dev, dev->func, PCI_interrupt_pin, 1);
			dev->info.u.h1.bridge_control = pci_read_config(dev->bus, dev->dev, dev->func, PCI_bridge_control, 2);		
			break;
		}

		default:
			TRACE(("PCI: Header type unknown (%d)\n", dev->info.header_type));
			break;
	}
}


void
PCI::RefreshDeviceInfo(PCIBus *bus)
{
	for (PCIDev *dev = bus->child; dev; dev = dev->next) {
		ReadPciBasicInfo(dev);
		ReadPciHeaderInfo(dev);
		if (dev->child)
			RefreshDeviceInfo(dev->child);
	}
}

////////////////////////////////////////////////////////////////////////////////
//Copyright 2003-2005, Marcus Overhagen. All rights reserved.
// Distributed under the terms of the MIT License.

#include <string.h>

//#include "pci_info.h"
//#include "pci_priv.h"

#define PCI_VERBOSE	1
//#define USE_PCI_HEADER 1

#if USE_PCI_HEADER
#	include "pcihdr.h"
#endif


void print_bridge_info(const pci_info *info, bool verbose);
void print_generic_info(const pci_info *info, bool verbose);
void print_capabilities(const pci_info *info);
void print_info_basic(const pci_info *info, bool verbose);
void get_vendor_info(uint16 vendorID, const char **venShort, const char **venFull);
void get_device_info(uint16 vendorID, uint16 deviceID, const char **devShort, const char **devFull);
const char *get_class_info(uint8 class_base, uint8 class_sub, uint8 class_api);
const char *get_capability_name(uint8 cap_id);


void
print_bridge_info(const pci_info *info, bool verbose)
{
	TRACE(("PCI:   primary_bus %02x, secondary_bus %02x, subordinate_bus %02x, secondary_latency %02x\n",
			info->u.h1.primary_bus, info->u.h1.secondary_bus, info->u.h1.subordinate_bus, info->u.h1.secondary_latency));
	TRACE(("PCI:   io_base %04x%02x, io_limit %04x%02x\n",
			info->u.h1.io_base_upper16, info->u.h1.io_base, info->u.h1.io_limit_upper16, info->u.h1.io_limit));
	TRACE(("PCI:   memory_base %04x, memory_limit %04x\n",
			info->u.h1.memory_base, info->u.h1.memory_limit));
	TRACE(("PCI:   prefetchable memory base %08lx%04x, limit %08lx%04x\n",
		info->u.h1.prefetchable_memory_base_upper32, info->u.h1.prefetchable_memory_base,
		info->u.h1.prefetchable_memory_limit_upper32, info->u.h1.prefetchable_memory_limit));
	TRACE(("PCI:   bridge_control %04x, secondary_status %04x\n",
			info->u.h1.bridge_control, info->u.h1.secondary_status));
	TRACE(("PCI:   interrupt_line %02x, interrupt_pin %02x\n",
			info->u.h1.interrupt_line, info->u.h1.interrupt_pin));
	TRACE(("PCI:   ROM base host %08lx, pci %08lx, size ??\n",
			info->u.h1.rom_base, info->u.h1.rom_base_pci));
	for (int i = 0; i < 2; i++)
		TRACE(("PCI:   base reg %d: host %08lx, pci %08lx, size %08lx, flags %02x\n",
			i, info->u.h1.base_registers[i], info->u.h1.base_registers_pci[i],
			info->u.h1.base_register_sizes[i], info->u.h1.base_register_flags[i]));
}


void
print_generic_info(const pci_info *info, bool verbose)
{
	TRACE(("PCI:   ROM base host %08lx, pci %08lx, size %08lx\n",
			info->u.h0.rom_base, info->u.h0.rom_base_pci, info->u.h0.rom_size));
	TRACE(("PCI:   cardbus_CIS %08lx, subsystem_id %04x, subsystem_vendor_id %04x\n",
			info->u.h0.cardbus_cis, info->u.h0.subsystem_id, info->u.h0.subsystem_vendor_id));
	TRACE(("PCI:   interrupt_line %02x, interrupt_pin %02x, min_grant %02x, max_latency %02x\n",
			info->u.h0.interrupt_line, info->u.h0.interrupt_pin, info->u.h0.min_grant, info->u.h0.max_latency));
	for (int i = 0; i < 6; i++)
		TRACE(("PCI:   base reg %d: host %08lx, pci %08lx, size %08lx, flags %02x\n",
			i, info->u.h0.base_registers[i], info->u.h0.base_registers_pci[i],
			info->u.h0.base_register_sizes[i], info->u.h0.base_register_flags[i]));
}


void
print_capabilities(const pci_info *info)
{
	uint16	status;
	uint8	cap_ptr;
	uint8	cap_id;
	int		i;

	TRACE(("PCI:   Capabilities: "));
	
	status = pci_read_config(info->bus, info->device, info->function, PCI_status, 2);
	if (!(status & PCI_status_capabilities)) {
		TRACE(("(not supported)\n"));
		return;
	}
	
	cap_ptr = pci_read_config(info->bus, info->device, info->function, PCI_capabilities_ptr, 1);
	cap_ptr &= ~3;
	if (!cap_ptr) {
		TRACE(("(empty list)\n"));
		return;
	}

	for (i = 0; i < 48; i++) {
		const char *name;
		cap_id  = pci_read_config(info->bus, info->device, info->function, cap_ptr, 1);
		cap_ptr = pci_read_config(info->bus, info->device, info->function, cap_ptr + 1, 1);
		cap_ptr &= ~3;
		if (i) {
			TRACE((", "));
		}
		name = get_capability_name(cap_id);
		if (name) {
			TRACE(("%s", name));
		} else {
			TRACE(("0x%02x", cap_id));
		}
		if (!cap_ptr)
			break;
	}
	TRACE(("\n"));
}


void
print_info_basic(const pci_info *info, bool verbose)
{
	TRACE(("PCI: bus %2d, device %2d, function %2d: vendor %04x, device %04x, revision %02x\n",
			info->bus, info->device, info->function, info->vendor_id, info->device_id, info->revision));
	TRACE(("PCI:   class_base %02x, class_function %02x, class_api %02x\n",
			info->class_base, info->class_sub, info->class_api));

	if (verbose) {
#if USE_PCI_HEADER
		const char *venShort;
		const char *venFull;
		get_vendor_info(info->vendor_id, &venShort, &venFull);
		if (!venShort && !venFull) {
			TRACE(("PCI:   vendor %04x: Unknown\n", info->vendor_id));
		} else if (venShort && venFull) {
			TRACE(("PCI:   vendor %04x: %s - %s\n", info->vendor_id, venShort, venFull));
		} else {
			TRACE(("PCI:   vendor %04x: %s\n", info->vendor_id, venShort ? venShort : venFull));
		}
		const char *devShort;
		const char *devFull;
		get_device_info(info->vendor_id, info->device_id, &devShort, &devFull);
		if (!devShort && !devFull) {
			TRACE(("PCI:   device %04x: Unknown\n", info->device_id));
		} else if (devShort && devFull) {
			TRACE(("PCI:   device %04x: %s - %s\n", info->device_id, devShort, devFull));
		} else {
			TRACE(("PCI:   device %04x: %s\n", info->device_id, devShort ? devShort : devFull));
		}
#endif
		TRACE(("PCI:   info: %s\n", get_class_info(info->class_base, info->class_sub, info->class_api)));
	}
	TRACE(("PCI:   line_size %02x, latency %02x, header_type %02x, BIST %02x\n",
			info->line_size, info->latency, info->header_type, info->bist));
			
	switch (info->header_type & PCI_header_type_mask) {
		case 0:
			print_generic_info(info, verbose);
			break;
		case 1:
			print_bridge_info(info, verbose);
			break;
		default:
			TRACE(("PCI:   unknown header type\n"));
	}

	print_capabilities(info);
}


void
pci_print_info()
{
	pci_info info;
	for (long index = 0; B_OK == pci_get_nth_pci_info(index, &info); index++) {
		print_info_basic(&info, PCI_VERBOSE);
	}
}


const char *
get_class_info(uint8 class_base, uint8 class_sub, uint8 class_api)
{
	switch (class_base) {
		case PCI_early:
			switch (class_sub) {
				case PCI_early_not_vga:
					return "Not VGA-compatible pre-2.0 PCI specification device";
				case PCI_early_vga:
					return "VGA-compatible pre-2.0 PCI specification device";
				default:
					return "Unknown pre-2.0 PCI specification device";
			}
			
		case PCI_mass_storage:
			switch (class_sub) {
				case PCI_scsi:
					return "SCSI mass storage controller";
				case PCI_ide:
					return "IDE mass storage controller";
				case PCI_floppy:
					return "Floppy disk controller";
				case PCI_ipi:
					return "IPI mass storage controller";
				case PCI_raid:
					return "RAID mass storage controller";
				case PCI_mass_storage_other:
					return "Other mass storage controller";
				default:
					return "Unknown mass storage controller";
			}

		case PCI_network:
			switch (class_sub) {
				case PCI_ethernet:
					return "Ethernet network controller";
				case PCI_token_ring:
					return "Token ring network controller";
				case PCI_fddi:
					return "FDDI network controller";
				case PCI_atm:
					return "ATM network controller";
				case PCI_isdn:
					return "ISDN network controller";
				case PCI_network_other:
					return "Other network controller";
				default:
					return "Unknown network controller";
			}

		case PCI_display:
			switch (class_sub) {
				case PCI_vga:
					switch (class_api) {
						case 0x00:
							return "VGA-compatible display controller";
						case 0x01:
							return "8514-compatible display controller";
						default:
							return "Unknown VGA display controller";
					}
				case PCI_xga:
					return "XGA display controller";
				case PCI_3d:
					return "3D display controller";
				case PCI_display_other:
					return "Other display controller";
				default:
					return "Unknown display controller";
			}

		case PCI_multimedia:
			switch (class_sub) {
				case PCI_video:
					return "Video multimedia device";
				case PCI_audio:
					return "Audio multimedia device";
				case PCI_telephony:
					return "Computer telephony multimedia device";
				case PCI_multimedia_other:
					return "Other multimedia device";
				default:
					return "Unknown multimedia device";
			}

		case PCI_memory:
			switch (class_sub) {
				case PCI_ram:
					return "RAM memory controller";
				case PCI_flash:
					return "Flash memory controller";
				case PCI_memory_other:
					return "Other memory controller";
				default:
					return "Unknown memory controller";
			}

		case PCI_bridge:
			switch (class_sub) {
				case PCI_host:
					return "Host/PCI bridge device";
				case PCI_isa:
					return "PCI/ISA bridge device";
				case PCI_eisa:
					return "PCI/EISA bridge device";
				case PCI_microchannel:
					return "PCI/Micro Channel bridge device";
				case PCI_pci:
					switch (class_api) {
						case 0x00:
							return "PCI/PCI bridge device";
						case 0x01:
							return "Transparent PCI/PCI bridge device";
						default:
							return "Unknown PCI/PCI bridge device";
					}
				case PCI_pcmcia:
					return "PCI/PCMCIA bridge device";
				case PCI_nubus:
					return "PCI/NuBus bridge device";
				case PCI_cardbus:
					return "PCI/CardBus bridge device";
				case PCI_raceway:
					if (class_api & 1)
						return "PCI/RACEway bridge device, end-point mode";
					else
						return "PCI/RACEway bridge device, transparent mode";
				case PCI_bridge_other:
					return "Other bridge device";
				default:
					return "Unknown bridge device";
			}

		case PCI_simple_communications:
			switch (class_sub) {
				case PCI_serial:
					switch (class_api) {
						case PCI_serial_xt:
							return "Generic XT-compatible serial communications controller";
						case PCI_serial_16450:
							return "16450-compatible serial communications controller";
						case PCI_serial_16550:
							return "16550-compatible serial communications controller";
						case 0x03:
							return "16650-compatible serial communications controller";
						case 0x04:
							return "16750-compatible serial communications controller";
						case 0x05:
							return "16850-compatible serial communications controller";
						case 0x06:
							return "16950-compatible serial communications controller";
						default:
							return "Unknown serial communications controller";
					}
				case PCI_parallel:
					switch (class_api) {
						case PCI_parallel_simple:
							return "Simple parallel port communications controller";
						case PCI_parallel_bidirectional:
							return "Bi-directional parallel port communications controller";
						case PCI_parallel_ecp:
							return "ECP 1.x compliant parallel port communications controller";
						case 0x03:
							return "IEEE 1284 parallel communications controller";
						case 0xfe:
							return "IEEE 1284 parallel communications target device";
						default:
							return "Unknown parallel communications controller";
					}										
				case PCI_multiport_serial:
					return "Multiport serial communications controller";
				case PCI_modem:
					switch (class_api) {
						case 0x00:
							return "Generic modem";
						case 0x01:
							return "Hayes-compatible modem, 16450-compatible interface";
						case 0x02:
							return "Hayes-compatible modem, 16550-compatible interface";
						case 0x03:
							return "Hayes-compatible modem, 16650-compatible interface";
						case 0x04:
							return "Hayes-compatible modem, 16750-compatible interface";
						default:
							return "Unknown modem communications controller";
					}
				case PCI_simple_communications_other:
					return "Other simple communications controller";
				default:
					return "Unknown simple communications controller";
			}

		case PCI_base_peripheral:
			switch (class_sub) {
				case PCI_pic:
					switch (class_api) {
						case PCI_pic_8259:
							return "Generic 8259 programmable interrupt controller (PIC)";
						case PCI_pic_isa:
							return "ISA programmable interrupt controller (PIC)";
						case PCI_pic_eisa:
							return "EISA programmable interrupt controller (PIC)";
						case 0x10:
							return "IO advanced programmable interrupt controller (APIC)";
						case 0x20:
							return "IO(x) advanced programmable interrupt controller (APIC)";
						default:
							return "Unknown programmable interrupt controller (PIC)";
					}
				case PCI_dma:
					switch (class_api) {
						case PCI_dma_8237:
							return "Generic 8237 DMA controller";
						case PCI_dma_isa:
							return "ISA DMA controller";
						case PCI_dma_eisa:
							return "EISA DMA controller";
						default:
							return "Unknown DMA controller";
					}
				case PCI_timer:
					switch (class_api) {
						case PCI_timer_8254:
							return "Generic 8254 timer";
						case PCI_timer_isa:
							return "ISA system timers";
						case PCI_timer_eisa:
							return "EISA system timers";
						default:
							return "Unknown timer";
					}
				case PCI_rtc:
					switch (class_api) {
						case PCI_rtc_generic:
							return "Generic real time clock (RTC) controller";
						case PCI_rtc_isa:
							return "ISA real time clock (RTC) controller";
						default:
							return "Unknown real time clock (RTC) controller";
					}
				case PCI_generic_hot_plug:
					return "Generic PCI Hot-Plug controller";
				case PCI_system_peripheral_other:
					return "Other base system peripheral";
				default:
					return "Unknown base system peripheral";
			}

		case PCI_input:
			switch (class_sub) {
				case PCI_keyboard:
					return "Keyboard controller";
				case PCI_pen:
					return "Digitizer (pen) input device";
				case PCI_mouse:
					return "Mouse controller";
				case PCI_scanner:
					return "Scanner controller";
				case PCI_gameport:
					switch (class_api) {
						case 0x00:
							return "Generic gameport controller";
						case 0x10:
							return "Gameport controller";
						default:
							return "Unknown gameport controller";
					}
				case PCI_input_other:
					return "Other input controller";
				default:
					return "Unknown input controller";
			}

		case PCI_docking_station:
			switch (class_sub) {
				case PCI_docking_generic:
					return "Generic docking station";
				case 0x80:
					return "Other type of docking station";
				default:
					return "Unknown docking station";
			}

		case PCI_processor:
			switch (class_sub) {
				case PCI_386:
					return "386 processor";
				case PCI_486:
					return "486 processor";
				case PCI_pentium:
					return "Pentium processor";
				case PCI_alpha:
					return "Alpha processor";
				case PCI_PowerPC:
					return "PowerPC processor";
				case PCI_mips:
					return "MIPS processor";
				case PCI_coprocessor:
					return "Co-processor";
				default:
					return "Unknown processor";
			}

		case PCI_serial_bus:
			switch (class_sub) {
				case PCI_firewire:
					switch (class_api) {
						case 0x00:
							return "Firewire (IEEE 1394) serial bus controller";
						case 0x10:
							return "Firewire (IEEE 1394) OpenHCI serial bus controller";
						default:
							return "Unknown Firewire (IEEE 1394) serial bus controller";
					}
				case PCI_access:
					return "ACCESS serial bus controller";
				case PCI_ssa:
					return "Serial Storage Architecture (SSA) controller";
				case PCI_usb:
					switch (class_api) {
						case PCI_usb_uhci:
							return "USB UHCI controller";
						case PCI_usb_ohci:
							return "USB OHCI controller";
						case 0x80:
							return "Other USB controller";
						case 0xfe:
							return "USB device";
						default:
							return "Unknown USB serial bus controller";
					}
				case PCI_fibre_channel:
					return "Fibre Channel serial bus controller";
				case 0x05:
					return "System Management Bus (SMBus) controller";
				default:
					return "Unknown serial bus controller";
			}

		case PCI_wireless:
			switch (class_sub) {
				case 0x00:
					return "iRDA compatible wireless controller";
				case 0x01:
					return "Consumer IR wireless controller";
				case 0x10:
					return "RF wireless controller";
				case 0x80:
					return "Other wireless controller";
				default:
					return "Unknown wireless controller";
			}

		case PCI_intelligent_io:
			switch (class_sub) {
				case 0x00:
					return "Intelligent IO controller";
				default:
					return "Unknown intelligent IO controller";
			}

		case PCI_satellite_communications:
			switch (class_sub) {
				case 0x01:
					return "TV satellite communications controller";
				case 0x02:
					return "Audio satellite communications controller";
				case 0x03:
					return "Voice satellite communications controller";
				case 0x04:
					return "Data satellite communications controller";
				default:
					return "Unknown satellite communications controller";
			}

		case PCI_encryption_decryption:
			switch (class_sub) {
				case 0x00:
					return "Network and computing encryption/decryption controller";
				case 0x10:
					return "Entertainment encryption/decryption controller";
				case 0x80:
					return "Other encryption/decryption controller";
				default:
					return "Unknown encryption/decryption controller";
			}

		case PCI_data_acquisition:
			switch (class_sub) {
				case 0x00:
					return "DPIO modules (data acquisition and signal processing controller)";
				case 0x80:
					return " Other data acquisition and signal processing controller";
				default:
					return "Unknown data acquisition and signal processing controller";
			}

		case PCI_undefined:
			return "Does not fit any defined class";

		default:
			return "Unknown device class base";
	}
}


const char *
get_capability_name(uint8 cap_id)
{
	switch (cap_id) {
		case PCI_cap_id_reserved:
			return "reserved";
		case PCI_cap_id_pm:
			return "PM";
		case PCI_cap_id_agp:
			return "AGP";
		case PCI_cap_id_vpd:
			return "VPD";
		case PCI_cap_id_slotid:
			return "SlotID";
		case PCI_cap_id_msi:
			return "MSI";
		case PCI_cap_id_chswp:
			return "chswp";
		case PCI_cap_id_pcix:
			return "PCI-X";
		case PCI_cap_id_ldt:
			return "ldt";
		case PCI_cap_id_vendspec:
			return "vendspec";
		case PCI_cap_id_debugport:
			return "DebugPort";
		case PCI_cap_id_cpci_rsrcctl:
			return "cpci_rsrcctl";
		case PCI_cap_id_hotplug:
			return "HotPlug";
		default:
			return NULL;
	}
}


#if USE_PCI_HEADER
void
get_vendor_info(uint16 vendorID, const char **venShort, const char **venFull)
{
	for (int i = 0; i < (int)PCI_VENTABLE_LEN; i++) {
		if (PciVenTable[i].VenId == vendorID) {
			if (0 == strcmp(PciVenTable[i].VenShort, PciVenTable[i].VenFull)) {
				*venShort = PciVenTable[i].VenShort[0] ? PciVenTable[i].VenShort : NULL;
				*venFull = NULL;
			} else {
				*venShort = PciVenTable[i].VenShort[0] ? PciVenTable[i].VenShort : NULL;
				*venFull = PciVenTable[i].VenFull[0] ? PciVenTable[i].VenFull : NULL;
			}
			return;
		}
	}
	*venShort = NULL;
	*venFull = NULL;
}


void
get_device_info(uint16 vendorID, uint16 deviceID, const char **devShort, const char **devFull)
{
	for (int i = 0; i < (int)PCI_DEVTABLE_LEN; i++) {
		if (PciDevTable[i].VenId == vendorID && PciDevTable[i].DevId == deviceID ) {
			*devShort = PciDevTable[i].Chip[0] ? PciDevTable[i].Chip : NULL;
			*devFull = PciDevTable[i].ChipDesc[0] ? PciDevTable[i].ChipDesc : NULL;
			return;
		}
	}
	*devShort = NULL;
	*devFull = NULL;
}
#endif	// USE_PCI_HEADER

