#ifndef __SYS_MEM_POOLMANAGER_H
#define __SYS_MEM_POOLMANAGER_H

#include <sys/mem/CompactPool.h>

namespace Mem
{


/*
	Used in cases where one can't simply make a class poolable
	by inheriting from CPoolable, for example, where we want
	every HashItem<T> class instance to come off the same
	pool.
*/

class PoolManager
{
public:

	enum
	{
		vHASH_ITEM_POOL =	0,
		vTOTAL_POOLS	=	1,
	};

	static void				SSetupPool(int poolId, int numItems);
	static void				SDestroyPool(int poolId);

	static void *			SCreateItem(int poolId);
	static void				SFreeItem(int poolId, void *pItem);
	static Mem::CCompactPool * 	SGetPool(int poolId);
	

private:

	static int				s_get_item_size(int poolId);

	static Mem::CCompactPool *sp_pool[vTOTAL_POOLS];
};

}

#endif

