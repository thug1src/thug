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
**	File name:		cfuncs.h												**
**																			**
**	Created: 		09/14/2000	-	ksh										**
**																			**
*****************************************************************************/

#ifndef	__SCRIPTING_SKFUNCS_H
#define	__SCRIPTING_SKFUNCS_H

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

bool ScriptCurrentSkaterIsPro(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetGoalsCompleted(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetNextLevelRequirements(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetCurrentSkaterProfile(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCurrentSkaterProfileIs(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAddSkaterProfile(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAddTemporaryProfile(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRememberTemporaryAppearance(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRestoreTemporaryAppearance(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSyncPlayer2Profile(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPreloadModels( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPreloadPedestrians( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPreselectRandomPedestrians( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptReplaceCarTextures( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetSkaterProfileInfo(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetSkaterProfileInfo(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetUIFromPreferences(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetUIFromSkaterProfile(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetPreferencesFromUI(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptResetDefaultAppearance(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptResetDefaultTricks(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptResetDefaultStats(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRandomizeAppearance(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPrintCurrentAppearance(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetNeversoftSkater(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCurrentProfileIsLocked(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptResetSkaters(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetSkaterProfileProperty(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleAlwaysSpecial( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSkaterSpeedGreaterThan( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSkaterSpeedLessThan( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLastScoreLandedGreaterThan( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLastScoreLandedLessThan( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptAnyTotalScoreAtLeast( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptOnlyOneSkaterLeft( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptTotalScoreGreaterThan( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptTotalScoreLessThan( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCurrentScorePotGreaterThan( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCurrentScorePotLessThan( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSkaterGetScoreInfo( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGoalsGreaterThan( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGoalsEqualTo( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptMedalsGreaterThan( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptMedalsEqualTo( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptToggleStats(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptToggleSkaterCamMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetSkaterID( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetCurrentSkaterID( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptResetScore( Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUpdateScore( Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptPlaySkaterCamAnim( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetSkaterCamAnimSkippable( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetSkaterCamAnimShouldPause( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSkaterCamAnimFinished( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSkaterCamAnimHeld( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptKillSkaterCamAnim( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGetSkaterCamAnimParams( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGetCurrentSkaterCamAnimName( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptPlayMovingObjectAnim( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPlayCutscene( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptHasMovieStarted( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptIsMovieQueued( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetMovingObjectAnimSkippable( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetMovingObjectAnimShouldPause( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptMovingObjectAnimFinished( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptMovingObjectAnimHeld( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptKillMovingObjectAnim( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptSetSkaterCamLerpReductionTimer( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptReloadSkaterCamAnim( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSkaterDebugOn(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSkaterDebugOff(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptVibrationIsOn(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptVibrationOn(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptVibrationOff(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAutoKickIsOn(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAutoKickOn(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAutoKickOff(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSpinTapsAreOn(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSpinTapsOn(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSpinTapsOff(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetCurrentProDisplayInfo(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetPlayerAppearance(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetPlayerFacePoints(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetPlayerFacePoints(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetPlayerFaceTexture(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetPlayerFaceOverlayTexture(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptClearPlayerFaceTexture(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPlayerFaceIsValid(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSelectCurrentSkater(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCareerStartLevel(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCareerLevelIs(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetRecordText(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUpdateRecords(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCareerReset(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUnSetGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptJustGotGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUnSetFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptJustGotFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetGlobalFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUnSetGlobalFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetGlobalFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptProfileEquals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptIsCareerMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptClearScoreGoals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetScoreGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEndRun(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptShouldEndRun(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptInitializeSkaters(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEndRunSelected(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAllSkatersAreIdle(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptFirstTrickStarted( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptFirstTrickCompleted( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCalculateFinalScores(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptReinsertSkaters(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUnhookSkaters(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptApplySplitScreenOptions(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptStartCompetition(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptStartCompetitionRun(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEndCompetitionRun(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptIsTopJudge(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPlaceIs(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRoundIs(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEndCompetition(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCompetitionEnded(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptStartHorse(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEndHorse(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptHorseEnded(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptHorseStatusEquals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptStartHorseRun(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEndHorseRun(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSwitchHorsePlayers(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetHorseString(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptIsCurrentHorseSkater(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptApplyToHorsePanelString(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptApplyToSkaterProfile(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptApplyColorToSkaterProfile(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRefreshSkaterColors(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRefreshSkaterScale(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRefreshSkaterVisibility(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRefreshSkaterUV(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAwardStatPoint(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAwardSpecialTrickSlot(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUpdateSkaterStats(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUpdateInitials( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptNewRecord(  Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptTrickOffAllObjects( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGameModeSetScoreDegradation( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGameModeSetScoreAccumulation( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptResetScoreDegradation( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSkaterIsBraking( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptLocalSkaterExists( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptInitSkaterModel(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRefreshSkaterModel(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEditPlayerAppearance(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetCurrentSkaterProfileIndex(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetCustomSkaterName(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptResetScorePot( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptPrintSkaterStats( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptPrintSkaterStats2( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptPrintSkaterPosition( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetSkaterPosition( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetSkaterVelocity( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetStatValue( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetNumStatPointsAvailable( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptUnlockSkater( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetActualCASOptionStruct( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetActualPlayerAppearancePart( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetPlayerAppearancePart( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetPlayerAppearanceColor( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetPlayerAppearanceScale( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetPlayerAppearanceUV( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptFlushDeadObjects( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptBindTrickToKeyCombo( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetKeyComboBoundToTrick( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptUpdateTrickMappings( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetConfigurableTricksFromType( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptTrickIsLocked( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetTrickDisplayText( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetSpecialTrickInfo( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetTrickType( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptGetIndexOfItemContaining( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptForEachSkaterName( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptForEachSkaterProfile( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptResetAllToDefaultStats( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptResetAllToDefaultProfile( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptResetToDefaultProfile( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGetLevelRecords( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptResetComboRecords( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptGetSkaterProfileInfoByName( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSetSkaterProfileInfoByName( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptGetNumberOfTrickOccurrences( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptGetNumSoundtracks( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGetSoundtrackName( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptControllerBoundToDifferentSkater( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptBindControllerToSkater( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptBindFrontEndToController( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptControllerBoundToSkater( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptGetKeyComboArrayFromTrickArray( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptFirstInputReceived( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptVibrateController( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLockCurrentSkaterProfileIndex( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptSetSpecialTrickInfo( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptInterpolateParameters( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptPlaySkaterStream ( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGetTextureFromPath ( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptGetVramUsage ( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptCompositeObjectExists ( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptClearPowerups( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptBroadcastProjectile( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptBroadcastEnterVehicle ( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptGetCollidingPlayerAndTeam( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptLobbyCheckKeyboard( Script::CStruct *pParams, Script::CScript *pScript );


/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace CFuncs

#endif	// __SCRIPTING_SKFUNCS_H

