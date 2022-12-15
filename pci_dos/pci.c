////////////////////////////////////////////////////////////////////////////////


void command_inb(int argc, uint32 argv[])
{
	uint32 count = 1, i, cnt = 0;

	if (!check_ports_args(1, 2, argc, argv))
		return;

	if (argc == 2)
		count = argv[1];

	while (count > 0) {
		printf("0x%04X.%02X:", (uint16) argv[0], (uint8) cnt);

		for (i = 0; i < 16; i++) {
			uint8 tmp = in_port_8(argv[0]++);
			printf(" %02X", tmp);
			if (!(--count))
				break;
		}

		printf("\n");
		cnt += 16;
	}
}

// argv[0] = port, argv[1] = count

static void
command_in(int size, int argc, uint32 argv[])
{
	int i, count = 1;
	char tmp[50];

	if (!check_ports_args(1, 2, argc, argv))
		return;

	if (argc == 3) {
		if (argv[2] < argv[1]) {
			printf("'last-port' must be greater than 'port'\n");
			return;
		}
		count = (argv[2] - argv[1] + 1) / size;
	}

	// only one read requested.
	if (count == 1) {
		switch (size) {
			case 1:
				printf("I/O Byte @ 0x%04lX = 0x%02X\n", argv[0], in_port_8(argv[0]);
			break;
			case 2:
				printf("I/O Word @ 0x%04lX = 0x%04X\n", argv[0], in_port_16(argv[0]);
			break;
			case 4:
				printf("I/O Word @ 0x%04lX = 0x%08X\n", argv[0], in_port_32(argv[0]);
			break;

			default:
			break;
		}
		return;
	}

	printf("I/O %s @:\n",
			(size == 4) ? "Longs" : (size == 2) ? "Words" : "Bytes");

	sprintf(tmp, " %s", (size == 4) ? "%08lX" : (size == 2) ? "%04X" : "%02X");

	while (count > 0) {
//		printf("  0x%02lX: ", argv[3]);
		printf("0x%04X.%02X:", (uint16) argv[0], (uint8) cnt);

		for (i = 0; i < (16 / size); i++) {
			printf(tmp, in_port(argv[0] + i, size));
//			argv[3] += size;
			if (!(--count))
				break;
		}

		printf("\n");
	}
}


void command_inb(int argc, uint32 argv[])
{
	command_in(1, argc, argv);
}


void command_inw(int argc, uint32 argv[])
{
	command_in(2, argc, argv);
}


void command_inl(int argc, uint32 argv[])
{
	command_in(4, argc, argv);
}
