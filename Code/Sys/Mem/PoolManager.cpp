#include <sys/mem/PoolManager.h>
#include <core/hashtable.h>

//DefinePoolableClass(Lst::HashItem)


namespace Mem 
{


Mem::CCompactPool *		PoolManager::sp_pool[PoolManager::vTOTAL_POOLS] =
{
	NULL
};




void PoolManager::SSetupPool(int poolId, int numItems)
{
	Dbg_MsgAssert(poolId >= 0 && poolId < vTOTAL_POOLS, ("%d not a valid pool", poolId));
	Dbg_MsgAssert(!sp_pool[poolId], ("pool %d already exists", poolId));

	if (poolId == vHASH_ITEM_POOL)
		sp_pool[poolId] = new Mem::CCompactPool(s_get_item_size(poolId), numItems, "HashItem");
}




void PoolManager::SDestroyPool(int poolId)
{
	Dbg_MsgAssert(poolId >= 0 && poolId < vTOTAL_POOLS, ("%d not a valid pool", poolId));
	Dbg_MsgAssert(sp_pool[poolId], ("pool %d doesn't exist", poolId));
	
	delete sp_pool[poolId];
	sp_pool[poolId] = NULL;
}




void *PoolManager::SCreateItem(int poolId)
{
	Dbg_MsgAssert(poolId >= 0 && poolId < vTOTAL_POOLS, ("%d not a valid pool", poolId));
	
	if (sp_pool[poolId])
	{
		return sp_pool[poolId]->Allocate();
	}

	return new char[s_get_item_size(poolId)];
}




void PoolManager::SFreeItem(int poolId, void *pItem)
{
	Dbg_MsgAssert(poolId >= 0 && poolId < vTOTAL_POOLS, ("%d not a valid pool", poolId));
	
	if (sp_pool[poolId])
	{
		sp_pool[poolId]->Free(pItem);
		return;
	}

	delete [] (char *)pItem;
}

// Provide access to the pools for debugging (like reporting usage)
Mem::CCompactPool * PoolManager::SGetPool(int poolId)
{
	return sp_pool[poolId];	
}




int PoolManager::s_get_item_size(int poolId)
{
	if (poolId == vHASH_ITEM_POOL)
		return sizeof(Lst::HashItem<void *>);
	return 0;
}




}

