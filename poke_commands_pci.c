//
// Copyright 2005, Haiku Inc. Distributed under the terms of the MIT license.
// Author(s):
// - Oscar Lesta <oscar@users.berlios.de>.
//

#include "poke_io.h"

#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - PCI Info

// As seen on TV!, err... on "KERN 'BOOT'" syslog (well... almost).
static void dump_pci_info(pci_info* pciinfo)
{
	uint8 i;

	printf("bus %02d device %02d function %02d: vendor 0x%04X device 0x%04X revision 0x%02X\n",
			pciinfo->bus, pciinfo->device, pciinfo->function,
			pciinfo->vendor_id, pciinfo->device_id, pciinfo->revision);

	printf("  class_base = 0x%02x class_function = 0x%02x class_api   = 0x%02x\n",
			pciinfo->class_base, pciinfo->class_sub, pciinfo->class_api);

	printf("  line_size  = 0x%02x latency_timer  = 0x%02x header_type = 0x%02x BIST = 0x%02x\n",
			pciinfo->line_size, pciinfo->latency, pciinfo->header_type,
			pciinfo->bist);

	printf("  interrupt  = 0x%02x interrupt_pin  = 0x%02x min_grant   = 0x%02x max_latency = 0x%02x\n",
			pciinfo->u.h0.interrupt_line, pciinfo->u.h0.interrupt_pin,
			pciinfo->u.h0.min_grant, pciinfo->u.h0.max_latency);

	printf("  rom_base    = 0x%08lX pci = 0x%08lX size = 0x%08lX\n",
			pciinfo->u.h0.rom_base, pciinfo->u.h0.rom_base_pci,
			pciinfo->u.h0.rom_size);

	printf("  cardbus_cis = 0x%08lX subsystem_id = 0x%04X subsystem_vendor_id = 0x%04X\n",
			pciinfo->u.h0.cardbus_cis, pciinfo->u.h0.subsystem_id,
			pciinfo->u.h0.subsystem_vendor_id);

	for (i = 0; i < 6; i++) {
		printf("  base reg %d: host addr 0x%08lX pci 0x%08lX size 0x%08lX flags 0x%02X\n",
				i, pciinfo->u.h0.base_registers[i],
				pciinfo->u.h0.base_registers_pci[i],
				pciinfo->u.h0.base_register_sizes[i],
				pciinfo->u.h0.base_register_flags[i]);
	}
}


void command_pci(int argc, uint32 argv[])
{
	pci_info pciinfo;
	int i = 0, devfound = 0;

	while ((poke_get_nth_pci_info(i, &pciinfo) == B_OK)) {
		i++;

		if (argc == 0) {
			dump_pci_info(&pciinfo);
			continue;
		}

		if (pciinfo.bus			== argv[0] &&
			pciinfo.device		== argv[1] &&
			pciinfo.function	== argv[2])
		{
			dump_pci_info(&pciinfo);
			devfound++;
			break;
		}
	}

	if (argc && !devfound)
		printf("Requested PCI device not found\n");
}


void command_pcilist(int argc, uint32 argv[])
{
	pci_info pciinfo;
	int i = 0;

	while ((poke_get_nth_pci_info(i, &pciinfo) == B_OK)) {
		i++;
		printf("bus %02d device %02d function %02d: vendor 0x%04X device 0x%04X revision 0x%02X\n",
				pciinfo.bus, pciinfo.device, pciinfo.function,
				pciinfo.vendor_id, pciinfo.device_id, pciinfo.revision);
	}
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - PCI Input

// argv[0] = bus, argv[1] = device, argv[2] = function, argv[3] = off_start,
// argv[4] = off_end.

static void pci_config_in(int size, int argc, uint32 argv[])
{
	int i, count = 1;
	char tmp[10];

	if (argc < 4 || argc > 5) {
		printf("Wrong number of arguments.\n");
		return;
	}

	if ((argv[3] & (size - 1)) != 0) {
		printf("'offset' has to be multiple of %d.\n", size);
		return;
	}

	if (argc == 5) {
		if (argv[4] < argv[3]) {
			printf("'last-off' must be greater than 'offset'\n");
			return;
		}

		if ((argv[4] & (size - 1)) != 0) {
			printf("'last-off' has to be multiple of %d.\n", size);
			return;
		}

		count = (argv[4] - argv[3] + 1) / size;
	}

	printf("bus %02d device %02d function %02d:", (int) argv[0], (int) argv[1],
			(int) argv[2]);

	// only one reg requested.
	if (count == 1) {
		printf(" read at 0x%02lX:", argv[3]);
		sprintf(tmp, " %s\n", (size == 4) ? "0x%08lX" : (size == 2) ? "0x%04X" : "0x%02X");
		printf(tmp, poke_read_pci_config(argv[0], argv[1], argv[2], argv[3], size));
		return;
	}

	printf("\n");
	sprintf(tmp, " %s", (size == 4) ? "%08lX" : (size == 2) ? "%04X" : "%02X");

	while (count > 0) {
		printf("  0x%02lX: ", argv[3]);

		for (i = 0; i < (16 / size); i++) {
			printf(tmp, poke_read_pci_config(argv[0], argv[1], argv[2], argv[3], size));
			argv[3] += size;
			if (!(--count))
				break;
		}

		printf("\n");
	}
}


void command_cfinb(int argc, uint32 argv[])
{
	pci_config_in(1, argc, argv);
}


void command_cfinw(int argc, uint32 argv[])
{
	pci_config_in(2, argc, argv);
}


void command_cfinl(int argc, uint32 argv[])
{
	pci_config_in(4, argc, argv);
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - PCI Output

// argv[0] = bus, argv[1] = device, argv[2] = function, argv[3] = offset,
// argv[4] = value.

void pci_config_out(int size, int argc, uint32 argv[])
{
	uint32 read_back;
	char buffer[80];

	if (argc != 5) {
		printf("Wrong number of arguments\n");
		return;
	}

	if ((argv[3] & (size - 1)) != 0) {
		printf("'offset' has to be multiple of %d\n", size);
		return;
	}

	poke_write_pci_config(argv[0], argv[1], argv[2], argv[3], size, argv[4]);

	sprintf(buffer,
			"bus %%02d device %%02d function %%02d offset %%02d: is now %s\n",
			(size == 4) ? "0x%08lX" : (size == 2) ? "0x%04lX" : "0x%02lX");

	read_back = poke_read_pci_config(argv[0], argv[1], argv[2], argv[3], size);
	printf(buffer, argv[0], argv[1], argv[2], argv[3], read_back);
}


void command_cfoutb(int argc, uint32 argv[])
{
	pci_config_out(1, argc, argv);
}


void command_cfoutw(int argc, uint32 argv[])
{
	pci_config_out(2, argc, argv);
}


void command_cfoutl(int argc, uint32 argv[])
{
	pci_config_out(4, argc, argv);
}
