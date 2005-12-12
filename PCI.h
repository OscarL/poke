// Stripped down version of Be's PCI.h file.

// Copyright 1993-98, Be Incorporated, All Rights Reserved.

#ifndef _PCI_H_
#define _PCI_H_

#if defined(__BEOS__)
	#include <PCI.h>
#else

#include "SupportDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

// pci device info

typedef struct pci_info {
	ushort	vendor_id;				// vendor id
	ushort	device_id;				// device id
	uchar	bus;					// bus number
	uchar	device;					// device number on bus
	uchar	function;				// function number in device
	uchar	revision;				// revision id
	uchar	class_api;				// specific register interface type
	uchar	class_sub;				// specific device function
	uchar	class_base;				// device type (display vs network, etc)
	uchar	line_size;				// cache line size in 32 bit words
	uchar	latency;				// latency timer
	uchar	header_type;			// header type
	uchar	bist;					// built-in self-test
	uchar	reserved;				// filler, for alignment
	union {
		struct {
			ulong	cardbus_cis;			// CardBus CIS pointer
			ushort	subsystem_id;			// subsystem (add-in card) id
			ushort	subsystem_vendor_id;	// subsystem (add-in card) vendor id
			ulong	rom_base;				// rom base address, viewed from host
			ulong	rom_base_pci;			// rom base addr, viewed from pci
			ulong	rom_size;				// rom size
			ulong	base_registers[6];		// base registers, viewed from host
			ulong	base_registers_pci[6];	// base registers, viewed from pci
			ulong	base_register_sizes[6];	// size of what base regs point to
			uchar	base_register_flags[6];	// flags from base address fields
			uchar	interrupt_line;			// interrupt line
			uchar	interrupt_pin;			// interrupt pin
			uchar	min_grant;				// burst period @ 33 Mhz
			uchar	max_latency;			// how often PCI access needed
		} h0;
		struct {
			ulong	base_registers[2];		// base registers, viewed from host
			ulong	base_registers_pci[2];	// base registers, viewed from pci
			ulong	base_register_sizes[2];	// size of what base regs point to
			uchar	base_register_flags[2];	// flags from base address fields
			uchar	primary_bus;
			uchar	secondary_bus;
			uchar	subordinate_bus;
			uchar	secondary_latency;
			uchar	io_base;
			uchar	io_limit;
			ushort	secondary_status;
			ushort	memory_base;
			ushort	memory_limit;
			ushort  prefetchable_memory_base;
			ushort  prefetchable_memory_limit;
			ulong	prefetchable_memory_base_upper32;
			ulong	prefetchable_memory_limit_upper32;
			ushort	io_base_upper16;
			ushort	io_limit_upper16;
			ulong	rom_base;				// rom base address, viewed from host
			ulong	rom_base_pci;			// rom base addr, viewed from pci
			uchar	interrupt_line;			// interrupt line
			uchar	interrupt_pin;			// interrupt pin
			ushort	bridge_control;
			ushort	subsystem_id;			// subsystem (add-in card) id
			ushort	subsystem_vendor_id;	// subsystem (add-in card) vendor id
		} h1;
		struct {
			ushort	subsystem_id;			// subsystem (add-in card) id
			ushort	subsystem_vendor_id;	// subsystem (add-in card) vendor id
		} h2;
	} u;
} pci_info;


#ifdef __cplusplus
}
#endif

#endif	// ! __BEOS__

#endif	// _PCI_H_
