#ifndef _BUFFER_H_
#define _BUFFER_H_

#ifndef __CORE_DEFINES_H
	#include <core/defines.h>
#endif

namespace NsBuffer
{
	void	init	( int size );
	void	reset	( bool clear = false );
	void	begin	( void );
	void	end		( void );

	void *	alloc	( int size );

	// Stat tracking
	int		bytes_used	( void );

	int		get_size	( void );
}

#endif		// _ARAM_H_



