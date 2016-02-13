#ifdef __PLAT_WN32__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

namespace Tmr
{
uint64 GetRenderFrame( void )
{
	return 0;
}

uint64 GetTimeInCPUCycles( void )
{
	return 0;
}
}

namespace Obj
{
// Needed for asserts to compile
bool DebugSkaterScripts=false;

class CObject;

CObject *ResolveToObject(uint32 id, bool ignoreScreenElements)
{
	return NULL;
}
}

namespace Pip
{
void *Load(const char *p_fileName)
{
	return NULL;
}

void Unload(const char *p_fileName)
{
}
}

namespace Mem
{

Manager *Manager::sp_instance = NULL;

Manager::Manager( void )
{
	memset(this,0,sizeof(Manager));
}

void Manager::sSetUp( void )
{
	if ( !sp_instance )
	{
		sp_instance = new Manager;
	}
	else
	{
		Dbg_Warning( "Already Initialized!" );
	}
}

void Manager::sCloseDown( void )
{
}

void Manager::PushContext( Allocator* alloc )
{
}

void Manager::PopContext( void )
{
}

char *Manager::GetContextName()
{
	return "";
}

Mem::Heap *Manager::FirstHeap()
{
	return NULL;
}

Heap* Manager::GetHeap( uint32 whichHeap )
{
	return NULL;
}

Mem::Heap *Manager::NextHeap(Mem::Heap * pHeap)
{
	return NULL;
}

void SetThreadSafe(bool safe)
{
}

void *Malloc( size_t size )
{
	return new char[size];
}

void Free( void *p_mem )
{
	delete [] p_mem;
}

void PopMemProfile()
{
}

void PushMemProfile(char *p_type)
{
}

}

#endif


