/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Debug (DBG)			 									**
**																			**
**	File name:		log.cpp			   		 								**
**																			**
**	Created by:		09/25/02	-	ksh										**
**																			**
**	Description:	Logging code											**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <stdio.h>
#include <core/defines.h>
#include <core/support.h>
#include <core/debug.h>


#include <sys/config/config.h>

#ifndef __PLAT_WN32__
#include <gfx/gfxman.h>
#endif // __PLAT_WN32__

#ifdef __PLAT_NGC__
#include <dolphin.h>
#define _output OSReport
#else
#define _output printf
#endif

#ifdef __NOPT_ASSERT__

#ifdef __PLAT_NGPS__

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/
// 128 bytes each (sizeof(SLogEntry)) and they come off the debug heap.
#define MAX_LOG_ENTRIES 20000

extern char _log_info_start[];
extern char _log_info_end[];

namespace Log
{
/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

// Chosen just to make sizeof(SLogEntry) a nice round 128
#define MAX_MESSAGE_LENGTH 107
struct SLogEntry
{
	Tmr::CPUCycles mCPUTime;
	const char *mpSourceFileName;
	const char *mpFunctionName;
	int mLineNumber;
	char mpMessage[MAX_MESSAGE_LENGTH+1];
};
//char p_foo[sizeof(SLogEntry)/0];

struct SLogBufferInfo
{
	// Pointer to the start of the big log buffer, which will be in the debug heap somewhere. 
	SLogEntry *mpBuffer;
	// The total number of entries in the buffer.
	int mTotalEntries;
	// The number of entries written to so far. This starts at 0 when the game starts,
	// and increases till it hits mTotalEntries, then stays at that value.
	int mNumEntries;
	
	// Points to just after the last entry that got added to the buffer.
	// So it may not point to a valid SLogEntry, for example when the buffer is filling
	// up, or when pTop points to the end of pBuffer.
	SLogEntry *mpTop;
};

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**								 Private Data								**
*****************************************************************************/
static bool sInitialised=false;
static SLogBufferInfo *spLogInfo=NULL;
static SLogEntry *spNotALeak=NULL;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							   Public Functions							**
*****************************************************************************/

void Init()
{
	
	if (Config::GotExtraMemory())
	{
		Dbg_MsgAssert(!sInitialised,("Tried to call Log::Init twice"));
		
		Dbg_MsgAssert((uint32)(_log_info_end-_log_info_start) >= sizeof(SLogBufferInfo),("log_info section too small, got=%d, required=%d",(uint32)(_log_info_end-_log_info_start),sizeof(SLogBufferInfo)));
		spLogInfo=(SLogBufferInfo*)_log_info_start;
	
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
		
		spLogInfo->mNumEntries=0;
		spLogInfo->mTotalEntries=MAX_LOG_ENTRIES;
		spLogInfo->mpBuffer=(SLogEntry*)Mem::Malloc(MAX_LOG_ENTRIES*sizeof(SLogEntry));
		spNotALeak=spLogInfo->mpBuffer;
		spLogInfo->mpTop=spLogInfo->mpBuffer;
	
		uint32 *p_long=(uint32*)spLogInfo->mpBuffer;
		Dbg_MsgAssert((sizeof(SLogEntry)&3)==0,("sizeof(SLogEntry) not a multiple of 4"));
		for (uint32 i=0; i<MAX_LOG_ENTRIES*sizeof(SLogEntry)/4; ++i)
		{
			*p_long++=0;
		}
			
		Mem::Manager::sHandle().PopContext();
		
		sInitialised=true;
	}
}

void AddEntry(char *p_fileName, int lineNumber, char *p_functionName, char *p_message)
{
	if (Config::GotExtraMemory())
	{
		Dbg_MsgAssert(sInitialised,("Log::AddEntry called before Log::Init called"));
		
		SLogEntry *p_new=NULL;
		if (spLogInfo->mNumEntries < MAX_LOG_ENTRIES)
		{
			p_new=spLogInfo->mpTop++;
			++spLogInfo->mNumEntries;
		}	
		else
		{
			if (spLogInfo->mpTop >= spLogInfo->mpBuffer+MAX_LOG_ENTRIES)
			{
				spLogInfo->mpTop=spLogInfo->mpBuffer;
			}
			p_new=spLogInfo->mpTop++;
		}
		
		p_new->mCPUTime=Tmr::GetTimeInCPUCycles();
		p_new->mLineNumber=lineNumber;
		p_new->mpFunctionName=p_functionName;
		p_new->mpSourceFileName=p_fileName;
		if (p_message)
		{
			Dbg_MsgAssert(strlen(p_message)<=MAX_MESSAGE_LENGTH,("Log message '%s' too long",p_message));
			strcpy(p_new->mpMessage,p_message);
		}
		else
		{
			p_new->mpMessage[0]=0;
		}	
	}
}

} // namespace Log

#else
namespace Log
{
void Init()
{
}

void AddEntry(char *p_fileName, int lineNumber, char *p_functionName, char *p_message)
{
}
} // namespace Log
#endif // #ifdef __PLAT_NGPS__

#endif	//__NOPT_ASSERT__

