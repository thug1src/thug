//****************************************************************************
//* MODULE:         Sk/Scripting
//* FILENAME:       CFuncs.h
//* OWNER:          Kendall Harrison
//* CREATION DATE:  9/14/2000
//****************************************************************************

#ifndef	__SCRIPTING_CFUNCS_H
#define	__SCRIPTING_CFUNCS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

// for now, include skfuncs.h, so that
// it won't break the existing CPP files
// that include cfuncs.h
//#include <sk/scripting/skfuncs.h>

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

enum {
	TYPE_VUTESTOBJECT = 0x8daad03d,
	VUTESTOBJECT = 0xae96a38,
};


bool	SetActiveCamera(uint32 id, int viewport, bool move_to_current);


/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

bool ScriptDummyCommand(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptAllocateSplitScreenDMA ( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptWait(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptBlock(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPrintEventHandlerTable(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptFormatText(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPrintf(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptScriptAssert(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPrintScriptInfo(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetHackFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptHackFlagIsSet(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPrintStruct(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUnloadAllLevelGeometry(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLoadScene(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLoadCollision(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAddScene(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAddCollision(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUnloadScene(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptQuickReload(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLoadNodeArray(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptReLoadNodeArray(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptParseNodeArray(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetBackgroundColor(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetBSPAmbientColor(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetDFFAmbientColor(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetDFFDirectColor(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetDynamicLightModulationFactor( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetClippingDistances(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetTrivialFarZClip(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEnableFog(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDisableFog(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetFogDistance(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetFogExponent(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetFogColor(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetUVWibbleParams(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEnableExplicitUVWibble(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDisableExplicitUVWibble(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetUVWibbleOffsets(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetWorldSize( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetMovementVelocity( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetRotateVelocity( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptOnReload(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptResetEngine(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleMetrics(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleVRAMViewer(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetVRAMPackContext(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDumpVRAMUsage(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleLightViewer(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCleanup(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptProximCleanup(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLoadQB(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUnloadQB(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetDebugRenderMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleTextureUpload(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleTextureDraw(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptTogglePointDraw(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRenderTest1(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRenderTest2(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptIsZero(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCastToInteger(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptStringToInteger(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptIntegerEquals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptChecksumEquals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptStringEquals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptArrayContains(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetArraySize(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetArrayElement(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAddParams(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGet3DArrayData(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGet2DArrayData(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetNDArrayData(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRemoveComponent(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetScriptString(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetScriptString(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptKenTest1(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptKenTest2(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAppendSuffix(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptFindNearestRailPoint(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetTime(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetDate(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetObNearestScreenCoord(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRandomize(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptResetTimer(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptTimeGreaterThan(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetStartTime(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetElapsedTime(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRemoveParameter(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetRandomValue(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetConfig(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPrintConfig(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGerman(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptFrench(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSpanish(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptItalian(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetNodeName(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetTesterScript(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptKillTesterScript(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptResetStopwatch(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPrintStopwatchTime(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCustomSkaterFilenameDefined(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetCustomSkaterFilename(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetCustomSkaterFilename(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEditingPark(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetParkName(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetParkName(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptParkEditorThemeWasSwitched(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptMenuIsShown(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptMenuIsSelected(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptForEachIn(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSizeOf(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetElement(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetNextArrayElement(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetRandomArrayElement(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPermuteArray(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptApplyChangeGamma( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGetGammaValues( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGotParam(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPreloadModel(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGoto(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGotoPreserveParams(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGotoRandomScript(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleRenderMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetRenderMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetWireframeMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDebugRenderIgnore(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptScreenShot(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDumpShots(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleMetricItem( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptToggleMemMetrics( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptKillAllTextureSplats( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPlayMovie( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLoadAsset( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLoadAnim( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLoadSkeleton( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptUnloadAnim( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptAssManSetReferenceChecksum( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptAssManSetDefaultPermanent( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLoadSound( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPlaySound( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptStopSound( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptStopAllSounds( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetSoundParams( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptIsSoundPlaying( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPlayStream( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptStopStream( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetStreamParams( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptIsStreamPlaying( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetReverb( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLoadTerrainSounds( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetObjectColor( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetSceneColor( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCompressVC( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptNudgeVC( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptFakeLights( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCenterCamera( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetDropoff( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetVolume( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetMusicVolume( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetMusicStreamVolume( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLoadMusicHeader( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLoadStreamHeader( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptStreamIsAvailable( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPlayTrack( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPlayMusicStream( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSkipMusicTrack( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetRandomMode( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGetCurrentTrack( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetMusicMode( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetMusicLooping( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptStopMusic( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPauseMusic( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPauseStream( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLoadStreamFrameAmp( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptFreeStreamFrameAmp( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptAddMusicTrack( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptChangeTrackState(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptTrackEnabled(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptMusicIsPaused(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptClearMusicTrackList( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptIfDebugOn(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptIfDebugOff(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptBootstrap(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCD(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptNotCD(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCasArtist(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptNotCasArtist(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGunslinger(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDoAction(Script::CStruct *pParams, Script::CScript *pScript, int action );
bool ScriptSendFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptClearFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptQueryFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCheckIfAlive( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCheckExistenceFromNodeIndex( int nodeIndex );
bool ScriptFlagException(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptMakeSkaterGoto(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptMakeSkaterGosub(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSpawnSound(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSpawnScript(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSpawnSkaterScript(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptKillSpawnedScript(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPauseSkaters( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptUnPauseSkaters( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPauseSkater( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptUnPauseSkater( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPauseGame( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptUnPauseGame( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptIsGamePaused( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPauseObjects( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptUnPauseObjects( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPauseSpawnedScripts( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptUnPauseSpawnedScripts( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptResetClock( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPadsPluggedIn( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptDoFlash( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetViewMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptStartServer(Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSpawnCrown(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSpawnCompass(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLeaveServer(Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptFindServers(Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptJoinServer(Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetNetworkMode( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetServerMode( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCreateFromNodeIndex( int nodeIndex );
bool ScriptKillFromNodeIndex( int nodeIndex, Script::CScript *pScript );
bool ScriptSetColorFromNodeIndex( int nodeIndex, Script::CStruct *pParams );
bool ScriptShatterFromNodeIndex( int nodeIndex );
bool ScriptSetVisibilityFromNodeIndex( int nodeIndex, bool invisible, int viewport_number = 0 );
bool ScriptNodeExists( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCreate( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCreateFromStructure( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptKill( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptVisible( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptInvisible( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptShatter( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptResetCamera(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetValueFromVolume(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetSliderValue(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetVolumeFromValue(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetIconTexture(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptMemViewToggle(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPreferenceEquals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetPreference(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetPreferenceString(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetPreferencePassword(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetPreferenceChecksum(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptMemPushContext(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptMemPopContext(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptProfileTasks( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptUseNetworkPreferences( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCanChangeDevices( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptNeedToTestNetSetup( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptTestNetSetup( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptConnectToInternet( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCancelConnectToInternet( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCancelLogon( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptDisconnectFromInternet( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptIsOnline( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptStopAllScripts( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetMenuElementText(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptFirstTimeThisIsCalled(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEnableActuators(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptInNetGame(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleNetMetrics(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetSlomo(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetArenaSize(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetParticleSysVisibility( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptTogglePlayerNames( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetCurrentGameType( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptDumpNetMessageStats( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptNotifyBailDone( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptDisplayLoadingScreen( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptHideLoadingScreen( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptObserveNextSkater( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPrintMemInfo( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptDisplayFreeMem( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptAnalyzeHeap( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptMemThreadSafe( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptIsSingleSession(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEnterObserverMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAllowPause(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRefreshServerList(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptStartServerList(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptStopServerList(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptFreeServerList(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPauseGameFlow(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUnpauseGameFlow(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptInFrontEnd(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetFrontEndInactive(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptInSplitScreenGame(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGameModeEquals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetFireballLevel(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptOnServer(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetCurrentLevel(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRestartLevel(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleScores(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptIsTrue(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetLevelLoadScript(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptXTriggered(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUsePad(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptInMultiplayerGame(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleShadowType(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptGameFlow(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptControllerPressed(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetAnalogueInfo(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptControllerDebounce( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptClearCheats(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptBroadcastCheat(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLastBroadcastedCheatWas(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCheatAllowed(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptJoinWithPassword(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSendChatMessage(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptInSlapGame( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetScreenModeFromGameMode( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetScreenMode( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptLoadPendingPlayers( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptLaunchQueuedScripts( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptIsObserving( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSkatersAreReady(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptGetInitialsString( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetInitialsString( Script::CStruct *pParams, Script::CScript *pScript );
bool	ScriptAttachToSkater(  Script::CStruct *pParams, Script::CScript *pScript );
bool	ScriptTryCheatString(  Script::CStruct *pParams, Script::CScript *pScript );
bool	ScriptLevelIs(  Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptStartNetworkLobby( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptObserversAllowed( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptNumPlayersAllowed( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptAutoDNS( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptUsingDefaultMasterServers( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptUsingDHCP( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptInInternetMode( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptEnteringNetGame( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptDeviceChosen( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLoadExecPS2( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptExitDemo( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptGameIsOver( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptSetLevelName( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptDumpHeaps( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptDumpFragments( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptResetPS2( Script::CStruct *pParams, Script::CScript *pScript ); 
bool ScriptResetHD( Script::CStruct *pParams, Script::CScript *pScript ); 
bool ScriptPAL( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptEnglish( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptTimeUp( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptChangeSymbolValue(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDumpScripts( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptResetPS2( Script::CStruct *pParams, Script::CScript *pScript ); 
bool ScriptResetHD( Script::CStruct *pParams, Script::CScript *pScript ); 
bool ScriptPAL( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptEnglish( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptTimeUp( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptLaunchViewer( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptLaunchScriptDebugger( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetViewerModel( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetViewerAnim( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetViewerLODDist( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetViewerObjectID( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptReloadViewerAnim( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptInitHealth( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptResetScore( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptAddRestartsToMenu( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptAddWarpPointsToMenu( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptRunScriptOnObject( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptRunScriptOnComponentType( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptTestName( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptDebounce( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetStatOverride( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptToggleRails( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptToggleRigidBodyDebug( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptCheckForHoles( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptStartLobbyList( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptChatConnect( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptChatDisconnect( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptFillPlayerListMenu( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptOnXbox( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGotoXboxDashboard( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSystemLinkEnabled( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptCreateParticleSystem( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetScript( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptDestroyParticleSystem( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptEmptyParticleSystem( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptParticleExists( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptStructureContains( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetBonePosition( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptMangleChecksums( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptAppendSuffixToChecksum( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptShouldEmitParticles( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptParticlesOn( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptParticlesOff( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptRotateVector( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptIsPS2( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptIsXBOX( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptIsNGC( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptIsWIN32( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetPlatform( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptIsPal( Script::CStruct* pParams, Script::CScript* pScript );


bool ScriptPushMemProfile( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptPopMemProfile( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptTogglePass( Script::CStruct* pParams, Script::CScript* pScript );

bool	ScriptSetScreen( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptGetUpperCaseString( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptGetGameMode( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptStartKeyboardHandler( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptStopKeyboardHandler( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptEnableKeyboard( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptDisableKeyboard( Script::CStruct* pParams, Script::CScript* pScript );
	 
bool ScriptCreateIndexArray( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetRescaledTargetValue( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptMemInitHeap( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptMemDeleteHeap( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptClearStruct( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptAppendStruct( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptScriptExists( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptSpawnSecondControllerCheck( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptStopSecondControllerCheck( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptIsArray( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptUseUserSoundtrack( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptUseStandardSoundtrack( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptDisableReset( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptEnableReset( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptResetToIPL( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptInitAnimCompressTable( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptCreateCompositeObject( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptAutoRail( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptInside( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptSetSpecialBarColors( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetMetrics( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptMoveNode( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetActiveCamera( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptSetColorBufferClear( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetEventHandler( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptClearEventHandler(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptClearEventHandlerGroup(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptOnExceptionRun(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptOnExitRun(Script::CStruct* pParams, Script::CScript* pScript);



bool ScriptSin( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptCos( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptTan( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptASin( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptACos( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptATan( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptShowTracking(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptIsGrind(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptShowCamOffset(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptAddSkaterEarly( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptPreLoadStreamDone( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptStartPreLoadedStream( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptFinishRendering( Script::CStruct* pParams, Script::CScript* pScript );

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace CFuncs

#endif	// __SCRIPTING_CFUNCS_H

