/*
 * Copyright 2005-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oscar Lesta, oscar.lesta@gmail.com
 */


#include "poke_io.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <FindDirectory.h>

////////////////////////////////////////////////////////////////////////////////

#define MEM8(address)	(*(vuint8 *) (address))
#define MEM16(address)	(*(vuint16*) (address))
#define MEM32(address)	(*(vuint32*) (address))

////////////////////////////////////////////////////////////////////////////////
// The original poke lets you segfault rather easily... we'd like to avoid that:

static bool
mem_args_ok(int min_argc, int max_argc, bool phys_cmd, int argc, uint32 address)
{
	int mem_state = MEM_NOT_MAPPED;

	if (argc < min_argc || argc > max_argc) {
		printf("Wrong number of arguments\n");
		return false;
	}

	mem_state = memory_state_of(address);
	if (mem_state == MEM_UNAVAILABLE) {
		printf("Memory functions not available on this platform.\n");
		return false;
	}

	if (phys_cmd && (mem_state != MEM_NOT_MAPPED)) {
		printf("address 0x%08" B_PRIX32 " is already mapped, use the 'virtual'"
				" version of this command\n", address);
		return false;
	}

	if (!phys_cmd) {
	 	if (mem_state == MEM_NOT_MAPPED) {
			printf("address 0x%08" B_PRIX32 "is not mapped, use the 'physical'"
					" version of this command\n", address);
			return false;
		} else if (mem_state == MEM_PROTECTED) {
			printf("address 0x%08" B_PRIX32 "belongs to a protected memory area\n",
					address);
			return false;
		}
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Virtual Space I/O
// argv[0] = address.

// (On my system)
// SiS300 cursor Reg: 0x2063f000 + 0x8500 = 0x20647500

void command_db(int argc, uint32 argv[])
{
	uint8 tmp;

	if (!mem_args_ok(1, 1, false, argc, argv[0]))
		return;

	tmp = MEM8(argv[0]);
	printf("0x%08" B_PRIX32 " = 0x%02X\n", argv[0], tmp);
}


void command_dw(int argc, uint32 argv[])
{
	uint16 tmp;

	if (!mem_args_ok(1, 1, false, argc, argv[0]))
		return;

	tmp = MEM16(argv[0]);
	printf("0x%08" B_PRIX32 " = 0x%04X\n", argv[0], tmp);
}


void command_dl(int argc, uint32 argv[])
{
	uint32 tmp;

	if (!mem_args_ok(1, 1, false, argc, argv[0]))
		return;

	tmp = MEM32(argv[0]);
	printf("0x%08" B_PRIX32 " = 0x%08" B_PRIX32 "\n", argv[0], tmp);
}

//------------------------------------------------------------------------------

// Display a memory block. argv[0] = address, argv[1] = count.
void command_dm(int argc, uint32 argv[])
{
	if (!mem_args_ok(1, 2, false, argc, argv[0]))
		return;

	if (argc == 1) {
//		printf("Parameter 'count' is missing, using default one (16)\n");
		argv[1] = 16;
	}

	if (argv[1] > 4096) {
		printf("'count' must be <= 4096 (requested: %" B_PRId32 ")\n", argv[1]);
		argv[1] = 4096;
	}

	// Force "count" to next multiple of 16, to ease next code.
	argv[1] = ((argv[1] - 1) & ~15) + 16;

	while (argv[1] > 0) {
		int i;

		printf("0x%08" B_PRIX32 ": ", argv[0]);

		for (i = 0; i < 16; i++)
			printf(" %02X", MEM8(argv[0] + i));

		printf("  ");

		for (i = 0; i < 16; i++) {
			uint8 tmp = MEM8(argv[0] + i);
			// Printable ASCII or dot.
			printf("%c", (tmp >= 32) && (tmp < 127) ? tmp : '.');
		}

		printf("\n");
		argv[0] += 16;
		argv[1] -= 16;
	}
}


//------------------------------------------------------------------------------
// argv[0] = address, argv[1] = value to write.

void command_sb(int argc, uint32 argv[])
{
	if (!mem_args_ok(2, 2, false, argc, argv[0]))
		return;

	MEM8(argv[0]) = argv[1];
}


void command_sw(int argc, uint32 argv[])
{
	if (!mem_args_ok(2, 2, false, argc, argv[0]))
		return;

	MEM16(argv[0]) = argv[1];
}


void command_sl(int argc, uint32 argv[])
{
	if (!mem_args_ok(2, 2, false, argc, argv[0]))
		return;

	MEM32(argv[0]) = argv[1];
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Physical MEM

// (On my system)
// SiS300 cursor reg: 0xdfee0000 + 0x8500

void read_physical_mem(int size, int argc, uint32 argv[])
{
	area_id area = -1;
	uint32 address, offset;

	if (!mem_args_ok(1, 1, true, argc, argv[0]))
		return;

	area = poke_map_physical_mem(argv[0], &address, &offset);
	if (area < B_OK) {
		printf("Error while trying to map physical memory (error: 0x%x)\n", area);
		return;
	}

	if (size > (B_PAGE_SIZE - offset)) {
		printf("Can't read %d bytes (beyond this page boundary)\n", size);
		return;
	}

	switch (size) {
		case 1:	printf("0x%08" B_PRIX32 " = 0x%02" B_PRIX8 "\n", argv[0], MEM8(address)); break;
		case 2:	printf("0x%08" B_PRIX32 " = 0x%04" B_PRIX16 "\n", argv[0], MEM16(address)); break;
		case 4:	printf("0x%08" B_PRIX32 " = 0x%08" B_PRIX32 "\n", argv[0], MEM32(address)); break;
	}

	poke_unmap_physical_mem(area);
}


void command_dpb(int argc, uint32 argv[])
{
	read_physical_mem(1, argc, argv);
}


void command_dpw(int argc, uint32 argv[])
{
	read_physical_mem(2, argc, argv);
}


void command_dpl(int argc, uint32 argv[])
{
	read_physical_mem(4, argc, argv);
}


void command_dpm(int argc, uint32 argv[])
{
	area_id area = -1;
	uint32 address, offset;

	if (!mem_args_ok(1, 2, true, argc, argv[0]))
		return;

	if (argc == 1)
		argv[1] = 16;

	area = poke_map_physical_mem(argv[0], &address, &offset);
	if (area < B_OK) {
		printf("Error while trying to map physical memory\n");
		return;
	}

	if (argv[1] > (B_PAGE_SIZE - offset)) {
		printf("'count' will be limited to %" B_PRId32 "\n", (B_PAGE_SIZE - offset));
		argv[1] = (B_PAGE_SIZE - offset);
	}

	// Force "count" to next multiple of 16, to ease next code.
	argv[1] = ((argv[1] - 1) & ~15) + 16;

	while (argv[1] > 0) {
		int i;

		printf("0x%08" B_PRIX32 ": ", argv[0]);

		for (i = 0; i < 16; i++)
			printf(" %02X", MEM8(address + i));

		printf("  ");

		for (i = 0; i < 16; i++) {
			uint8 tmp = MEM8(address + i);
			// Printable ASCII or dot.
			printf("%c", (tmp >= 32) && (tmp < 127) ? tmp : '.');
		}

		printf("\n");
		address += 16;
		argv[0] += 16;
		argv[1] -= 16;
	}

	poke_unmap_physical_mem(area);
}

//------------------------------------------------------------------------------
// argv[0] = address, argv[1] = value to write.

void write_physical_mem(int size, int argc, uint32 argv[])
{
	area_id area = -1;
	uint32 address, offset;

	if (!mem_args_ok(2, 2, true, argc, argv[0]))
		return;

	area = poke_map_physical_mem(argv[0], &address, &offset);
	if (area < B_OK) {
		printf("Error while trying to map physical memory\n");
		return;
	}

	if (size > (B_PAGE_SIZE - offset)) {
		printf("Can't write %d bytes (beyond this page boundary)\n", size);
		return;
	}

	switch (size) {
		case 1:
			MEM8(address) = (uint8) argv[1];
			printf("0x%08" B_PRIX32 " is now: 0x%02" B_PRIX8 "\n", argv[0], MEM8(address));
		break;
		case 2:
			MEM16(address) = (uint16) argv[1];
			printf("0x%08" B_PRIX32 " is now: 0x%04" B_PRIX16 "\n", argv[0], MEM16(address));
		break;
		case 4:
			MEM32(address) = argv[1];
			printf("0x%08" B_PRIX32 " is now: 0x%08" B_PRIX32 "\n", argv[0], MEM32(address));
		break;
	}

	poke_unmap_physical_mem(area);
}


void command_spb(int argc, uint32 argv[])
{
	write_physical_mem(1, argc, argv);
}


void command_spw(int argc, uint32 argv[])
{
	write_physical_mem(2, argc, argv);
}


void command_spl(int argc, uint32 argv[])
{
	write_physical_mem(4, argc, argv);
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Page Dumps


// argv[0] = start address, argv[1] = number of pages,
// argv[2] = filename's suffix (ie: dumpfile.01, dumpfile.02, etc)

void command_dumpvm(int argc, uint32 argv[])
{
	int i, j, fd, dummy = 0;
	char filename[B_FILE_NAME_LENGTH];

	if (!mem_args_ok(3, 3, false, argc, argv[0]))
		return;

	if (find_directory(B_USER_DIRECTORY, 0, false, filename, dummy) == B_OK) {
		strcat(filename, "/poke_vm_dump.%02d");
		sprintf(filename, filename, argv[2]);
	}
	else
		sprintf(filename, "poke_vm_dump.%02d", (int) argv[2]);

	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		printf("Couldn't create output file, reason: %s.\n", strerror(errno));
		return;
	}

	// ToDo: check for enough free disk space.
	for (i = 0; i < argv[1]; i++) {
		uint8 tmp[B_PAGE_SIZE];

		for (j = 0; j < B_PAGE_SIZE; j++) {
			tmp[j] = MEM8(argv[0] + j);
		}

		write(fd, tmp, B_PAGE_SIZE);
		argv[0] += B_PAGE_SIZE;
	}

	close(fd);
}


// dumppm 0xdfee0000 32 1   // dump all SiS 300 regs (128 KB)

void command_dumppm(int argc, uint32 argv[])
{
	area_id area = -1;
	uint32 address, dummy = 0;
	int fd;
	char filename[B_FILE_NAME_LENGTH];
	uint i, j;

	if (!mem_args_ok(3, 3, true, argc, argv[0]))
		return;

	if (find_directory(B_USER_DIRECTORY, 0, false, filename, dummy) == B_OK) {
		strcat(filename, "/poke_pm_dump.%02d");
		sprintf(filename, filename, argv[2]);
	}
	else
		sprintf(filename, "poke_pm_dump.%02d", (int) argv[2]);

	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		printf("Couldn't create output file, reason: %s.\n", strerror(errno));
		return;
	}

	// ToDo: check for enough free disk space.
	for (i = 0; i < argv[1]; i++)
	{
		uint8 tmp[B_PAGE_SIZE];

		area = poke_map_physical_mem(argv[0] + (i * B_PAGE_SIZE), &address, &dummy);
		if (area < B_OK) {
			printf("Error while trying to map physical memory (page = %d)\n", i);
			break;
		}

		for (j = 0; j < B_PAGE_SIZE; j++) {
			tmp[j] = MEM8(address + j);
		}

		write(fd, tmp, B_PAGE_SIZE);
		address += B_PAGE_SIZE;

		poke_unmap_physical_mem(area);
	}

	close(fd);
}
