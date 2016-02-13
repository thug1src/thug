/*******************************************************************
*
*    DESCRIPTION: thread/ngps/t_thread.cpp
*
*    AUTHOR:	JAB
*
*    HISTORY:	
*
*    DATE:9/1/2000
*
*******************************************************************/

/** include files **/

#include <core/defines.h>
#include <core/task.h>
#include <eekernel.h>

#include "t_thread.h"

extern "C" int _std_stack[8];
static int* _stack = _std_stack;

extern "C" int _stack_size[8];
extern "C" int _start(void);

Thread::PerThreadStruct _rootThreadStruct =
{
	"ROOT",
	0,
	0,
	_start,				// m_pEntry
	_stack,				// m_pStackBase
	(int)_stack_size,	// m_iStackSize
	0,
	0
};

int Thread::dummy = -1;


								   
typedef	void (*VOID_CB)(void*);		// pointer to function that takes a void pointer
								   


namespace Thread
{
	
	
	
	HTHREAD CreateThread( PerThreadStruct* pPTS )
	{
		
		ThreadParam sParam;
		/*sParam.entry = pPTS->m_pEntry;*/
		sParam.entry = (VOID_CB) (pPTS->m_pEntry);
		sParam.stack = pPTS->m_pStackBase;
		sParam.stackSize = pPTS->m_iStackSize;
		//ps2memset( pPTS->m_pStackBase, 0xca, pPTS->m_iStackSize);
		sParam.initPriority = pPTS->m_iInitialPriority;
		sParam.gpReg = &_gp;
#ifdef __THREAD_DEBUGGING_PRINTS__
		Dbg_Message( "T::CrThread: %s, entry 0x%x, stack 0x%x, size 0x%x\n",pPTS->m_cID, (unsigned int) pPTS->m_pEntry, (unsigned int) pPTS->m_pStackBase, (unsigned int) pPTS->m_iStackSize );
#endif
		pPTS->m_osId = ::CreateThread( &sParam );
		return (HTHREAD) pPTS;
	}
}


