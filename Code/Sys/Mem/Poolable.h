/*
	MODULE DESCRIPTION
	
	Poolable.h, RJM, 10/18/2001
*/

#ifndef __SYS_MEM_POOLABLE_H
#define __SYS_MEM_POOLABLE_H

#include <sys/mem/CompactPool.h>
#include <sys/mem/memman.h>

#define DefinePoolableClass(_T)										\
namespace Mem														\
{																	\
	Mem::CCompactPool *Mem::CPoolable< _T >::sp_pool[POOL_STACK_SIZE] = {NULL,NULL};		\
	bool Mem::CPoolable< _T >::s_internallyCreatedPool[POOL_STACK_SIZE] = {false,false};	\
	int Mem::CPoolable< _T >::s_currentPool=0;						\
}																	\


namespace Mem
{


	extern int gHeapPools;

//class CCompactPool;


template <class _T>
class CPoolable
{
	enum EPoolStackSize
	{
		POOL_STACK_SIZE=2
	};
		
public:
	static void							SCreatePool(int num_items, char *name);
	static void							SAttachPool(CCompactPool *pPool);
	static void							SRemovePool();
	static int							SGetMaxUsedItems();
	static int							SGetNumUsedItems();
	static int							SGetTotalItems();
							  													
	static void							SPrintInfo();
	
	static void							SSwitchToNextPool();
	static void							SSwitchToPreviousPool();
	
	static int							SGetCurrentPoolIndex();
	static bool							SPoolExists();
	
	void * operator new (size_t size);										  	
	void operator delete (void * pMem);									   		
protected:																		
	static CCompactPool *				sp_pool[POOL_STACK_SIZE];
	static bool	  						s_internallyCreatedPool[POOL_STACK_SIZE];  	
	static int							s_currentPool;
};




class PoolTest : public CPoolable<PoolTest>
{

public:
	PoolTest();
	~PoolTest();

	int m_monkey;
};




template<class _T>
void *CPoolable<_T>::operator new (size_t size)							
{																
	Dbg_Assert(size == sizeof(_T));								
																
	//printf("in CPoolable operator new\n");

	#ifdef	__NOPT_ASSERT__
	if (gHeapPools)
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
		void *p = ::new char[size];								
		Mem::Manager::sHandle().PopContext();
		return p;
	}
	#endif
	
	if (sp_pool[s_currentPool])	 
	{
		return sp_pool[s_currentPool]->Allocate();								
	}

	return ::new char[size];								
}																
																



template<class _T>
void CPoolable<_T>::operator delete (void * pMem)							
{																
	Dbg_Assert(pMem);  
	#ifdef	__NOPT_ASSERT__
	if (gHeapPools)
	{
		delete [] (char *)pMem;											
		return;
	}
	#endif
	
	// pMem contains no info as to which pool it was allocated from, because that would
	// use up too much memory.
	// So query the pools to see if it is in them.
	
	CCompactPool *p_pool=sp_pool[s_currentPool];
	// Check if it is in the current pool, which it will be most of the time.
	if (p_pool && p_pool->IsInPool(pMem))
	{
		p_pool->Free(pMem);
	}
	else
	{
		// Otherwise try the other one, which exists for saving games to mem card.
		Dbg_MsgAssert(POOL_STACK_SIZE==2,("Only two pool supported at the moment"));
		p_pool=sp_pool[s_currentPool ^ 1];
		if (p_pool && p_pool->IsInPool(pMem))
		{
			p_pool->Free(pMem);
		}
		else
		{
			delete [] (char *)pMem;											
		}
	}		
}																
																



template<class _T>
void CPoolable<_T>::SCreatePool(int num_items, char *name)					
{																
	Dbg_Assert(!sp_pool[s_currentPool]);										
	sp_pool[s_currentPool] = new CCompactPool(sizeof(_T), num_items, name);		
	s_internallyCreatedPool[s_currentPool] = true;
}																
																
template<class _T>
bool CPoolable<_T>::SPoolExists()
{																
	if (sp_pool[s_currentPool])
	{
		return true;
	}
	return false;
}		
 

template<class _T>
void CPoolable<_T>::SAttachPool(CCompactPool *pPool)					
{																
	Dbg_Assert(!sp_pool[s_currentPool]);										
	Dbg_Assert(pPool);										
	sp_pool[s_currentPool] = pPool;		
	s_internallyCreatedPool[s_currentPool] = false;
}																
																



template<class _T>
void CPoolable<_T>::SRemovePool()											
{																
	if (sp_pool[s_currentPool])												
	{															
		if (s_internallyCreatedPool[s_currentPool])
			delete sp_pool[s_currentPool];											
		sp_pool[s_currentPool] = NULL;											
	}															
}																

template<class _T>
int CPoolable<_T>::SGetCurrentPoolIndex()
{
	return s_currentPool;
}

template<class _T>
void CPoolable<_T>::SSwitchToNextPool()
{
	Dbg_MsgAssert(s_currentPool<POOL_STACK_SIZE-1,("Called SSwitchToNextPool with no next pool"));
	++s_currentPool;
}
	
template<class _T>
void CPoolable<_T>::SSwitchToPreviousPool()
{
	Dbg_MsgAssert(s_currentPool,("Called SSwitchToPreviousPool with no previous pool"));
	--s_currentPool;
}

template<class _T>
int CPoolable<_T>::SGetMaxUsedItems()											
{
	return sp_pool[s_currentPool]->GetMaxUsedItems();
}


template<class _T>
int CPoolable<_T>::SGetNumUsedItems()											
{
	return sp_pool[s_currentPool]->GetNumUsedItems();
}

template<class _T>
int CPoolable<_T>::SGetTotalItems()											
{
	return sp_pool[s_currentPool]->GetTotalItems();
}




template<class _T>
void CPoolable<_T>::SPrintInfo()											
{
	printf("pool is at %p\n", sp_pool[s_currentPool]);
}




}

#endif
