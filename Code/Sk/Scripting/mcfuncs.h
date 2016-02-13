/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Scripting												**
**																			**
**	File name:		mcfuncs.h												**
**																			**
**	Created: 		06/10/2002	-	ksh										**
**																			**
*****************************************************************************/

#ifndef	__SCRIPTING_MCFUNCS_H
#define	__SCRIPTING_MCFUNCS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

namespace Script
{
	class CStruct;
	class CScript;
};
	
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace CFuncs
{

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

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

bool ScriptGetMostRecentSave(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetMemCardSpaceAvailable(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetMemCardSpaceRequired(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptMemCardFileExists(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDeleteMemCardFile(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptFormatCard(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCardIsInSlot(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCardIsFormatted(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSaveFailedDueToInsufficientSpace(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetSummaryInfo(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSaveToMemoryCard(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetSectionsToApplyWhenLoading(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLoadFromMemoryCard(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLoadedCustomSkater(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetMemCardDataForUpload(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptClearMemCardDataForUpload(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptNeedToLoadReplayBuffer(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLoadReplayData(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetMemCardDirectoryListing(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetMaxTHPS4FilesAllowed(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSectorSizeOK(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCardIsDamaged(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCardIsForeign(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptBadDevice(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptGetSaveInfo(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCreateTemporaryMemCardPools(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRemoveTemporaryMemCardPools(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSwitchToTempPoolsIfTheyExist(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSwitchToRegularPools(Script::CStruct *pParams, Script::CScript *pScript);


bool SaveDataFile(const char *p_name, uint8 *p_data, uint32 size);
bool LoadDataFile(const char *p_name, uint8 *p_data, uint32 size);

void SetVaultData(uint8 *p_data, uint32 type);

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace CFuncs

#endif	// __SCRIPTING_MCFUNCS_H

