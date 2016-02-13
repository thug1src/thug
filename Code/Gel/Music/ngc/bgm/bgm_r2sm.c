#include <kernel.h>

#define SPU_BLOCK 512

// Using SP will speed things up, but ...

void _BgmRaw2SpuMono( unsigned int *src, unsigned int *dst, unsigned int block )
{
	int i;

	for ( i = 0; i < block; i++ )
	{
		memcpy( (void*)((int)dst+i*SPU_BLOCK*2), (void*)((int)src+i*SPU_BLOCK), SPU_BLOCK );
		memcpy( (void*)((int)dst+i*SPU_BLOCK*2+SPU_BLOCK), (void*)((int)src+i*SPU_BLOCK) , SPU_BLOCK );
	}

	return;
}


