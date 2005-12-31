//
// Copyright 2005, Haiku Inc. Distributed under the terms of the MIT license.
// Author(s):
// - Oscar Lesta <oscar@users.berlios.de>.
//

#include "poke_io.h"

#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////

static bool
check_ports_args(int min_argc, int max_argc, int argc, uint32 argv[])
{
	if (argc < min_argc || argc > max_argc) {
		printf("Wrong number of arguments.\n");
		return false;
	}

	if (argv[0] > 0xFFFF) {
		printf("port must be on range 0x00-0xFFFF\n");
		return false;
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Ports I/O

void command_inb(int argc, uint32 argv[])
{
	uint32 count;

	if (!check_ports_args(1, 2, argc, argv))
		return;

	if (argc == 1) {
		uint8 tmp = in_port_8(argv[0]);
		printf("I/O Byte 0x%04lX = 0x%02X\n", argv[0], tmp);
		return;
	}

	if ((argc == 2) && (argv[1] < argv[0])) {
		printf("'last_port' must be greater than the first one\n");
		return;
	}

	count = argv[1] - argv[0] + 1;

	while (count > 0) {
		uint i;
		printf("0x%04X:", (uint16) argv[0]);

		for (i = 0; i < 16; i++) {
			uint8 tmp = in_port_8(argv[0]++);
			printf(" %02X", tmp);
			if (--count == 0)
				break;
		}

		printf("\n");
	}
}


void command_inw(int argc, uint32 argv[])
{
	uint16 tmp;

	if (!check_ports_args(1, 1, argc, argv))
		return;

	tmp = in_port_16(argv[0]);
	printf("I/O Word 0x%04lX = 0x%04X\n", argv[0], tmp);
}


void command_inl(int argc, uint32 argv[])
{
	uint32 tmp;

	if (!check_ports_args(1, 1, argc, argv))
		return;

	tmp = in_port_32(argv[0]);
	printf("I/O Long 0x%04lX = 0x%08lX\n", argv[0], tmp);
}


//------------------------------------------------------------------------------

void command_outb(int argc, uint32 argv[])
{
	if (!check_ports_args(2, 2, argc, argv))
		return;

	out_port_8(argv[0], argv[1]);
}


void command_outw(int argc, uint32 argv[])
{
	if (!check_ports_args(2, 2, argc, argv))
		return;

	out_port_16(argv[0], argv[1]);
}


void command_outl(int argc, uint32 argv[])
{
	if (!check_ports_args(2, 2, argc, argv))
		return;

	out_port_32(argv[0], argv[1]);
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - VGA-Style I/O


// argv[0] = port, argv[1] = first register index, argv[2] = last register index
void command_idxinb(int argc, uint32 argv[])
{
	uint i, index_count = 1;

	if (!check_ports_args(2, 3, argc, argv))
		return;

	if ((argc == 3) && (argv[2] < argv[1])) {
		printf("Last index must be greater than the first one\n");
		return;
	}

	if (argc == 3)
		index_count = argv[2] - argv[1] + 1;

	while (index_count > 0) {
		printf("0x%04X.%02X:", (uint16) argv[0], (uint8) argv[1]);

		for (i = 0; i < 16; i++) {
			uint8 tmp = in_port_indexed(argv[0], argv[1]++);
			printf(" %02X", tmp);
			if (--index_count == 0)
				break;
		}

		printf("\n");
	}
}


// argv[0] = port, argv[1] = register index, argv[2] = value to write

void command_idxoutb(int argc, uint32 argv[])
{
	if (!check_ports_args(3, 3, argc, argv))
		return;

	out_port_indexed(argv[0], argv[1], argv[2]);
}
