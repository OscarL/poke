//
// This files eases porting my BeOS apps to other places.
//

#ifndef _SUPPORTDEFS_H_
#define _SUPPORTDEFS_H_

#if defined(__BEOS__)
	#include <SupportDefs.h>
#else

#include <sys/types.h>

typedef char					int8;
typedef unsigned char			uint8;
typedef short					int16;
typedef unsigned short			uint16;
typedef int						int32;
typedef unsigned int			uint32;
typedef unsigned long long		uint64;

typedef volatile char			vint8;
typedef volatile unsigned char	vuint8;
typedef volatile short			vint16;
typedef volatile unsigned short	vuint16;
typedef volatile long			vint32;
typedef volatile unsigned long	vuint32;

typedef unsigned char			uchar;
typedef unsigned short			ushort;
typedef unsigned int			uint;
typedef unsigned long			ulong;
typedef unsigned long			addr;
typedef uint64					bigtime_t;
typedef int32					status_t;

// area_id is an int32 really, but I need it this way for MMIO on Poke for Win.
typedef addr	area_id;

#ifndef __cplusplus
	typedef unsigned char	bool;
	#define	false	0
	#define	true	1
	#ifndef	NULL
		#define	NULL	((void*)0)
	#endif
#endif

#ifndef	_PACKED
	#define _PACKED			__attribute__((packed))
#endif

#define B_PAGE_SIZE			4096
#define PAGE_SIZE			B_PAGE_SIZE
#define	B_FILE_NAME_LENGTH	1024

// Not accurate at all, but real value doesn't matters.
enum {
	B_DEV_ID_ERROR	= -20,
	B_BAD_VALUE		= -10,
	B_TIMEOUT		= - 5,
	B_IO_ERROR		= - 2,
	B_ERROR			= - 1,
	B_OK			=   0
};


#endif	// ! __BEOS__

#endif  // _SUPPORTDEFS_H_
