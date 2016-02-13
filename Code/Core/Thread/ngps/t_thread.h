/*******************************************************************
*
*    DESCRIPTION: thread/ngps/t_thread.h
*
*    AUTHOR:	JAB
*
*    HISTORY:	
*
*    DATE:9/1/2000
*
*******************************************************************/

/** include files **/
#ifndef __CORE_THREAD_NGPS_T_THREAD_H__
#define __CORE_THREAD_NGPS_T_THREAD_H__

#include <core/support.h>
#include <eekernel.h>

namespace Thread {
	

	typedef int	THREADID;			// Id for use and assigned by application
	typedef int	OSTHREADID;			// Id assigned by the OS.
	typedef unsigned int HTHREAD;	// A value that is easy to determine from the OS
	
	class PerThreadStruct {
	public:
		char		m_cID[16];			// a text name so its easy to 
		THREADID 	m_utid;				// an id that the use can assign
		OSTHREADID	m_osId;				// an id that the OS assigns
		void*		m_pEntry;
		void*		m_pStackBase;
		int			m_iStackSize;
		int			m_iInitialPriority;
		void*		m_pRpcData;			// ptr to rpc specific data.
	};

	//inline void				StartThread( HTHREAD hthr, void* arg = NULL )  { ::StartThread( ((PerThreadStruct*)hthr)->m_osId, arg ); }
	// So you must set up the stack, and the perthreadstruct.
	HTHREAD CreateThread( PerThreadStruct* pPTS );

	/*
	Commentary on data for submodules (eg m_pRpcData):
	Yes there are less data dependent ways of doing this. For example,
	by having an array of pointers and the subsystem requests an index
	that it can use. But the extra work (small tho it is) seems a bit
	pointless when the penalty for hardcoding it is having to recompile
	lots of things if it changes. Not a big deal.
	*/
	extern int dummy;
};

struct PTS{
	char		m_cID[16];			// a text name so its easy to 
	Thread::THREADID 	m_utid;				// an id that the use can assign
	Thread::OSTHREADID	m_osId;				// an id that the OS assigns
	void*		m_pEntry;
	void*		m_pStackBase;
	int			m_iStackSize;
	int			m_iInitialPriority;
	void*		m_pRpcData;			// ptr to rpc specific data.
};

extern "C" Thread::PerThreadStruct _rootThreadStruct;

#endif	// __CORE_THREAD_NGPS_T_THREAD_H__
