#include <core/defines.h>
#include <sys/mem/CompactPool.h>


#ifdef __DEBUG_COMPACTPOOL__
//#define __REALLY_DEBUG_COMPACTPOOL__


#endif

#ifdef	__NOPT_ASSERT__
#define	REPORT_ON		0		// change this to the address of a black you want to watch
#endif

#ifdef __PLAT_NGC__
extern bool g_mc_hack;
#include "sys/ngc/p_buffer.h"
#endif		// __PLAT_NGC__

namespace Mem
{


CCompactPool::CCompactPool(int item_size, int desired_num_items, char *name)
{
	m_totalItems = desired_num_items;
	m_itemSize = item_size;
	Dbg_MsgAssert(m_itemSize >= 4, ("item size too small (%d)", m_itemSize));

#ifdef __PLAT_NGC__
	if ( g_mc_hack )
	{
		mp_buffer = (uint8*)NsBuffer::alloc( m_totalItems * m_itemSize );
	}
	else
#endif		// __PLAT_NGC__
	{
		mp_buffer = new uint8[m_totalItems * m_itemSize];
	}
	mp_buffer_end = mp_buffer + m_totalItems * m_itemSize;

	m_currentUsedItems = 0;
	m_maxUsedItems = 0;

	// set up free list
	mp_freeList = (uint32 *) mp_buffer;
	uint8 *pItem = mp_buffer;
	for (int i = 0; i < m_totalItems; i++)
	{
		if (i < m_totalItems - 1)
			*((uint32 **) pItem) = (uint32 *) (pItem + m_itemSize);
		else
			*((uint32 **) pItem) = NULL;
		pItem += m_itemSize;
	}

	if (name)
		strcpy(m_name, name);
	else
		strcpy(m_name, "unnamed");

#ifdef __REALLY_DEBUG_COMPACTPOOL__

	m_maxUsedItems = 0;

	int total_marker_blocks = (m_totalItems + 31) >> 5;
	mp_used_marker_tab = new uint32[total_marker_blocks];
	//printf("ZOOPY: %d marker blocks, marker tab at 0x%x\n", total_marker_blocks, mp_used_marker_tab);
	for (int b = 0; b < total_marker_blocks; b++)
		mp_used_marker_tab[b] = 0;
#endif
}




CCompactPool::~CCompactPool()
{
	Dbg_MsgAssert(!m_currentUsedItems, ("pool still contains items"));

#ifdef __PLAT_NGC__
	if ( !g_mc_hack )
#endif		// __PLAT_NGC__
	{
		delete mp_buffer;
	}

#ifdef __REALLY_DEBUG_COMPACTPOOL__
	Ryan("Freeing pool %s, max items used in this pool: %d\n", m_name, m_maxUsedItems);
	delete [] mp_used_marker_tab;
	mp_used_marker_tab = NULL;
	m_maxUsedItems = 0;
#endif
}

   
bool CCompactPool::IsInPool(void *p)
{
	if (p>=mp_buffer && p<mp_buffer_end)
	{
		return true;
	}
	return false;
}			


void * CCompactPool::Allocate()
{
	if (mp_freeList)
	{
		m_currentUsedItems++;

		if (m_currentUsedItems > m_maxUsedItems)
		{
			m_maxUsedItems = m_currentUsedItems;
		}
#ifdef __REALLY_DEBUG_COMPACTPOOL__		

		uint32 marker_num = ((uint32) mp_freeList - (uint32) mp_buffer) / m_itemSize;
		uint32 marker_tab_entry = marker_num >> 5;
		uint32 marker_tab_bit = marker_num - (marker_tab_entry << 5);
		//printf("ZOOPY: allocating using entry %d, bit %d, marker tab at 0x%x\n", marker_tab_entry, marker_tab_bit, mp_used_marker_tab);
		Dbg_MsgAssert(mp_used_marker_tab, ("max used items %d", m_maxUsedItems));
		mp_used_marker_tab[marker_tab_entry] |= (1<<marker_tab_bit);
#endif
	
		//printf("CCompactPool::Allocate(), now %d used items out of %d\n", m_currentUsedItems, m_totalItems);
		
		void *pItem = mp_freeList;
		mp_freeList = (uint32 *) *mp_freeList;
		#ifdef	__NOPT_ASSERT__
		if ((int)pItem == REPORT_ON)
		{
			printf ("++++ CCompactPool::Allocate %p\n",pItem);
			DumpUnwindStack(20,0);
		}
		#endif
		return pItem;
	}

	Dbg_MsgAssert(0, ("Out of %ss (%d max) in their Compact Pool", m_name, m_totalItems));
	
	return NULL;
}



void CCompactPool::Free(void *pFreeItem)
{
	#ifdef	__NOPT_ASSERT__
	if ((int)pFreeItem == REPORT_ON)
	{
		printf ("--- CCompactPool::Free %p\n",pFreeItem);
		DumpUnwindStack(20,0);
	}
	#endif
#ifdef __REALLY_DEBUG_COMPACTPOOL__
	// make sure block is within range
	int pool_item = ((int) ((uint8 *) pFreeItem - mp_buffer)) / m_itemSize;
	Dbg_MsgAssert(pool_item >= 0 && pool_item < m_totalItems, ("item (%d) out of range (%d)", pool_item, m_totalItems));
	Dbg_Assert(!(((uint32) pFreeItem) & 3));
	
	uint32 marker_tab_entry = pool_item >> 5;
	uint32 marker_tab_bit = pool_item - (marker_tab_entry << 5);
	//printf("ZOOPY: freeing using entry %d, bit %d, marker tab at 0x%x\n", marker_tab_entry, marker_tab_bit, mp_used_marker_tab);
	Dbg_Assert(mp_used_marker_tab);
	Dbg_MsgAssert(mp_used_marker_tab[marker_tab_entry] & (1<<marker_tab_bit), ("already freed item from pool"));
	mp_used_marker_tab[marker_tab_entry] &= ~(1<<marker_tab_bit);	
#endif

	m_currentUsedItems--;
	
	//printf("CCompactPool::Free(), now %d used items out of %d\n", m_currentUsedItems, m_totalItems);
	
	*((uint32 **) pFreeItem) = mp_freeList;
	mp_freeList = (uint32 *) pFreeItem;
	
/*
#ifdef __REALLY_DEBUG_COMPACTPOOL__
	uint32 *p_free_next = *((uint32 **) mp_freeList);
	if (p_free_next)
	{
		int pool_item = ((int) ((uint8 *) p_free_next - mp_buffer)) / m_itemSize;
		Dbg_MsgAssert(pool_item >= 0 && pool_item < m_totalItems, ("next item (%d) out of range (%d)", pool_item, m_totalItems));
	}
#endif
*/
}



} // namespace Mem



