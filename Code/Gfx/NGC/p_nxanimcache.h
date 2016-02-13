//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_NxAnimCache.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  5/06/2002
//****************************************************************************

#ifndef	__GFX_P_NXANIMCACHE_H__
#define	__GFX_P_NXANIMCACHE_H__
    
#include <gfx/nxanimcache.h>

namespace Nx
{
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/////////////////////////////////////////////////////////////////////////////////////
//
// Here's a machine specific implementation of the CGeom
    
class CNgcAnimCache : public CAnimCache
{
                                      
public:
						CNgcAnimCache( int lookupTableSize );
	virtual 			~CNgcAnimCache();

private:				// It's all private, as it is machine specific
};

} // Nx

#endif 

