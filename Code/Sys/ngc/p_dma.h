#ifndef _DMA_H_
#define _DMA_H_

#include <dolphin.h>

namespace NsDMA
{
	void		toARAM		( u32 aram, void * p_mram, int size );
	void		toMRAM		( void * p_mram, u32 aram, int size );
};

#endif		// _DMA_H_

