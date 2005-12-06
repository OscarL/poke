//
// Copyright 2005, Haiku Inc. Distributed under the terms of the MIT license.
// Author(s):
// - Oscar Lesta <oscar@users.berlios.de>.
//

#ifndef _POKE_IO_H_
#define _POKE_IO_H_

#if defined(__BEOS__)

	#include "poke_io_beos.h"

#elif defined(__WIN32__)

	#include "poke_io_win.h"

#elif defined(__MSDOS__)

	#include "poke_io_dos.h"

#else

	#error "This platform is not supported."

#endif


#endif	// _POKE_IO_H_
