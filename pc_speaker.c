/*
 * Copyright 2005-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oscar Lesta, oscar.lesta@gmail.com
 */


#if defined(__i386__) || defined(__x86_64__)

#include "poke_io.h"


void pc_speaker_on(uint16 freq)
{
	if (freq > 18) {
		uint8 b;
	    freq = (uint16) (1193181 / freq);
		b = in_port_8(0x61);

		if ((b & 0x3) == 0) {
			out_port_8(0x61, b | 0x3); // Speaker ON
			out_port_8(0x43, 0xB6);
		}

		out_port_8(0x42, freq);
		out_port_8(0x42, freq >> 8);
	}
}


void pc_speaker_off(void)
{
	uint16 value;
	value = in_port_8(0x61) & 0xFC; // Speaker OFF
	out_port_8(0x61, value);
}


void pc_speaker_beep(uint16 freq, uint duration)
{
	pc_speaker_on(freq);
	snooze(duration);
	pc_speaker_off();
}

#endif	// #if defined(__i386__) || defined(__x86_64__)
