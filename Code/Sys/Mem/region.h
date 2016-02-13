/*****************************************************************************
**																			**
**			              Neversoft Entertainment	                        **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Sys Library												**
**																			**
**	Module:			Memory Manager (Mem)									**
**																			**
**	Created:		03/20/00	-	mjb										**
**																			**
**	File name:		/sys/mem/region.h   									**
**																			**
*****************************************************************************/

#ifndef	__SYS_MEM_REGION_H
#define	__SYS_MEM_REGION_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Mem
{



/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

class Allocator;

class  Region  : public Spt::Class			
{
	

	friend class Manager;

public :
		
	void*		StartAddr ( void ) const { return mp_start; }
	void*		EndAddr ( void ) const { return mp_end; }

	void		RegisterAllocator( Allocator* alloc );		
	void		UnregisterAllocator( Allocator* alloc );
	void*		Allocate( Allocator* pAlloc, size_t size, bool assert_on_fail = true );
	int			MemAvailable( void );
	int			MinMemAvailable( void ) {return m_min_free;}
	int			TotalSize( void );

	virtual		~Region();

protected :
				Region( void ){};
				Region( void* pStart, void* pEnd );

	void		init ( void* pStart, void* pEnd );
	void*		mp_start;


	int			m_min_free;		// minimum amount of free memory

private :
	
	enum
	{
		vBOTTOM_UP,
		vTOP_DOWN,
		vMAX_ALLOCS
	};

	void*		mp_end;

	Allocator*	m_alloc[vMAX_ALLOCS];

};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  AllocRegion : public  Region 			
{
	

	friend class Manager;

public :
				AllocRegion( size_t size );
	virtual		~AllocRegion( void );

private :


};

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

inline int Region::TotalSize( void )
{
	
		
	return ((int)mp_end - (int)mp_start);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mem

#endif  // __SYS_MEM_REGION_H
