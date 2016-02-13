#ifndef	__FILE_PIP_H
#define	__FILE_PIP_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

namespace Script
{
	class CStruct;
	class CScript;
}	

namespace Pip
{
void	LoadPre(const char *p_preFileName);
bool	UnloadPre(const char *p_preFileName);

void*	Load(const char *p_fileName);
void	Unload(const char *p_fileName);
uint32	GetFileSize(const char *p_fileName);

// GJ:  sometimes it's useful to do this
// by checksum, so that we don't have to keep 
// the full filename string hanging around
void	Unload(uint32 fileNameCRC);
uint32	GetFileSize(uint32 fileNameCRC);

const char *GetNextLoadedPre(const char *p_pre_name=NULL);
bool PreFileIsInUse(const char *p_pre_name);

bool ScriptLoadPipPre(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUnloadPipPre(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDumpPipPreStatus(Script::CStruct *pParams, Script::CScript *pScript);


}

#endif

