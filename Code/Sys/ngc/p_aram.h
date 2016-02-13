#ifndef _ARAM_H_
#define _ARAM_H_

#ifndef __CORE_DEFINES_H
	#include <core/defines.h>
#endif

#include <dolphin.h>

namespace NsARAM
{
	typedef enum {
		BOTTOMUP = 0,
		TOPDOWN,
		SKATER0,
		SKATER1,
		SCRIPT,

		NUMHEAPS,
	} HEAPTYPE;

	void	init	( void );
	uint32	alloc	( uint32 size, HEAPTYPE heap = BOTTOMUP );
	void	free	( uint32 p );
	uint32	getSize	( uint32 p );

	uint32	unused	( void );
};

#endif		// _ARAM_H_


