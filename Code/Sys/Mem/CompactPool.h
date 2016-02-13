/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		
**																			**
**	Module:			
**																			**
**	File name:		
**																			**
**	Created by:		8/2/2001 - rjm							                **
**																			**
**	Description:	
**																			**
*****************************************************************************/

#ifndef __SYS_MEM_COMPACTPOOL_H
#define __SYS_MEM_COMPACTPOOL_H


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
/*****************************************************************************
**								   Defines									**
*****************************************************************************/


#ifdef __NOPT_ASSERT__
#define __DEBUG_COMPACTPOOL__
#endif


namespace Mem
{

						


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class CCompactPool
{
public:
						CCompactPool(int item_size, int desired_num_items, char *name = NULL);
						~CCompactPool();

	void *				Allocate();
	void 				Free(void *pFreeItem);
	int					GetMaxUsedItems() {return m_maxUsedItems;}
	int					GetNumUsedItems() {return m_currentUsedItems;}
	int					GetTotalItems() {return m_totalItems;}
	char *				GetName() {return m_name;}
	bool				IsInPool(void *p);
	
private:
	uint8 *				mp_buffer;
	uint8 *				mp_buffer_end;
	int					m_totalItems; // that we have room for
	int					m_itemSize;

	int					m_currentUsedItems;

	uint32 *			mp_freeList;

	char				m_name[64];
	int					m_maxUsedItems;
#ifdef __DEBUG_COMPACTPOOL__
	uint32 *			mp_used_marker_tab;
	
#endif

};




/*
template<class _V>
class StaticCCompactPool
{
public:
	static void			SAllocPool(int size, char *name = NULL) {sp_pool = new CCompactPool<_V>(size, name);}
	static void			SFreePool()	{delete sp_pool;}
	static _V*			SCreate() {return sp_pool->Create();}
	static void			SFree(_V *pItem) {sp_pool->Free(pItem);}

	static int			SGetNumUsedItems() {return sp_pool->GetNumUsedItems();}

protected:
	static CCompactPool<_V> *	sp_pool;
};
*/




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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}

#endif	// __SYS_MEM_CCOMPACTPOOL_H


