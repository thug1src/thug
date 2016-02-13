//****************************************************************************
//* MODULE:         Sk/Modules/Skate
//* FILENAME:       GoalManager.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  1/24/2001
//****************************************************************************

#ifndef	__MODULES_SKATE_GOALMANAGER_H
#define	__MODULES_SKATE_GOALMANAGER_H

                           
#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#ifndef __SYS_TIMER_H
    #include <sys/timer.h> // For Tmr::Time
#endif
							  
#include <sk/modules/skate/goal.h>

#include <core/list/node.h>
#include <core/lookuptable.h>

#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

namespace Script
{
	class CScript;
	class CStruct;
}

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Game
{
	// script names
	const uint32 vINIT			= 0x398b1b8b;
	const uint32 vUNINIT		= 0xd4e0306a;
	const uint32 vDESTROY		= 0x83b57984;
	const uint32 vACTIVATE		= 0x4999fb52;
	const uint32 vDEACTIVATE	= 0x01c92cac;	
	const uint32 vACTIVE		= 0xb4e103fd;
	const uint32 vINACTIVE		= 0x742cf11f;
	const uint32 vEXPIRED		= 0xd34eb52b;
	const uint32 vSUCCESS		= 0x90ff204d;
	const uint32 vFAIL			= 0x79d8d4b6;
    const uint32 vUNLOCK        = 0x951b4f10;

	const uint32 vBET_SUCCESS	= 0x782bdda1;

    // flag array name - goal_flags
    const uint32 vFLAGS         = 0xcc3c4cc4;

	// number of goal flags allowed
	const uint32 vNUMGOALFLAGS = 10;

    // number of completed goals required to unlock this goal
    const int vUNLOCKED_BY_ANOTHER  = 0x8a3a324e;   // type - unlocked_by_another goal
    const int vPRO_GOAL             = 0xd303a1a3;   // type - pro_goal
	// const int vPRO_CHALLENGE		= 0xe0f96410;	// type - pro_specific_challenge

	// number of streams to search through for goal wait vo
	const int vMAXWAITSTREAMS		= 10;
	const int vMAXWINSTREAMS		= 10;
	const int vMAXMIDGOALSTREAMS	= 10;
	const int vMAXCALLSKATERBYNAMESTREAMS = 5;

	// Size of the local buffer used for replacing trick names in goals
	const uint32	NEW_STRING_LENGTH = 512;

	const int vMAXDIFFICULTYLEVEL = 2;	
	enum GOAL_MANAGER_DIFFICULTY_LEVEL
	{
		GOAL_MANAGER_DIFFICULTY_LOW = 0,
		GOAL_MANAGER_DIFFICULTY_MEDIUM,
		GOAL_MANAGER_DIFFICULTY_HIGH,
	};
	
/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class CGoalManager : public Spt::Class
{
public:
	CGoalManager();
	~CGoalManager();

public:
	bool				CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript );
	uint32				GetGoalIdFromParams( Script::CStruct* pParams, bool assert = true );
	
	bool				AddGoal( uint32 goalId, Script::CStruct* pParams );
	CGoal*				GetGoal( uint32 goalId, bool assert = true );
	CGoal*				GetGoal( Script::CStruct* pParams, bool assert = true );
	CGoal*				GetGoalByIndex( int index );
	bool				ActivateGoal( uint32 goalId, bool dontAssert );
	bool				DeactivateGoal( uint32 goalId );
	bool				WinGoal( uint32 goalId );
	bool				LoseGoal( uint32 goalId );
	bool				RemoveGoal( uint32 goalId );
    void                UnlockGoal( uint32 goalId );
    void                LevelUnload();
    void                LevelLoad();

	void				UpdateFamilyTrees();
	void				FindCircularLinks( uint32 parent, uint32 child );
	
    bool				GoalIsActive( uint32 goalId );
	bool				EditGoal( uint32 goalId, Script::CStruct* pParams );
	bool				SetGoalTimer( uint32 goalId, int time = -1 );
	int					GetNumGoals();
	int					GetNumActiveGoals( bool count_all = false );
	int					GetNumSelectedGoals();
	void				DeactivateAllGoals( bool include_net_goals = false );
	void				UninitializeGoal( uint32 goalId );
	void				UninitializeGoalTree( uint32 start_goal );
	void				UninitializeAllGoals();
	void				DeactivateAllMinigames();
	void				InitializeAllMinigames();
	void				InitializeAllGoals();
	void				InitializeAllSelectedGoals();
	void				DeselectAllGoals();
	void				RemoveAllGoals();
	void				Update();
    bool                HasSeenGoal( uint32 goalId );
    void                PauseAllGoals();
    void                UnPauseAllGoals();
    Tmr::Time           GetGoalTime();
    int                 GetActiveGoal( bool ignore_net_goals = false );
    bool                QuickStartGoal( uint32 goalId, bool dontAssert = false );
	void                RestartLastGoal();
	bool				CanRestartStage();
	void				RestartStage();
    bool                HasWonGoal( uint32 goalId );
	bool				GoalIsSelected( uint32 goalId );
	void				ToggleGoalSelection( uint32 goalId );
	void				CreateGoalLevelObjects( void );
    
    // void                CreateGoalFlag( uint32 goalId, uint32 flag );
    bool                SetGoalFlag( uint32 goalId, uint32 flag, int value );
	bool				GoalFlagSet( uint32 goalId, uint32 flag );
    bool                AllFlagsSet( uint32 goalId );
    
	bool				CreatedGapGoalIsActive(); // Added by Ken
	bool				GetCreatedGoalGap(int gapIndex); // Also added by Ken

    bool                CanRetryGoal();
    Script::CStruct*    GetGoalParams( uint32 goalId );
    bool                CanStartGoal();
    
    bool                NextRaceWaypoint( uint32 goalId );
    bool                NextTourSpot( uint32 goalId );
    void                InitTrickObjects( uint32 goalId );
	bool				ShouldLogTrickObject();
    bool                SetGraffitiMode( int mode );

	void				GotTrickObject( uint32 clusterId, int score );
    void                SetSpecialGoal( uint32 goalId, int set );
	void                CheckTrickText();
//	void                CheckTetrisTricks();
    void                GotCounterObject( uint32 goalId );
    bool                CounterGoalDone( uint32 goalId );
    void                SpendGoalPoints( int num );
    bool                HasGoalPoints( int num );
    int                 GetNumberOfGoalPoints();
	int					GetTotalNumberOfGoalPointsEarned();
	void				ClearGoalPoints();
    void                AddGoalPoint();
    void                AddCash( int amount );
    int                 GetCash( bool get_total = false );
    bool                SpendCash( int amount );
    bool                HasBeatenGoalWithProset( const char* proset_prefix );
	int                 NumGoalsBeatenBy( int obj_id );
	int                 NumGoalsBeatenByTeam( int team_id );
	int                 NumGoalsBeaten();
    int                 NumGoalsBeatenInLevel( int levelNum, bool count_pro_specific_challenges = false );
    void                UnlockAllGoals();
	void				TurnPro();
	bool				CheckMinigameRecord( uint32 goalId, int value );
	void				DeactivateMinigamesWithTimer();
	void				DeactivateCurrentGoal();
	void				SetStartTime( uint32 goalId );
	void				UpdateComboTimer( uint32 goalId );
	void				SetStartHeight( uint32 goalId );
	bool				CheckHeightRecord( uint32 goalId );
	bool				CheckDistanceRecord( uint32 goalId );

	void				PlayGoalStartStream( Script::CStruct* pParams );
	void				StopCurrentStream( uint32 goalId );
	void				PlayGoalWinStream( Script::CStruct* pParams );
	void				PlayGoalWaitStream( uint32 goalId );
	void				PlayGoalStream( Script::CStruct* pParams );

	void				GetGoalAnimations( uint32 goalId, const char* type, Script::CScript* pScript );

	void				PauseGoal( uint32 goalId );
	void				UnPauseGoal( uint32 goalId );

	void				PauseCompetition( uint32 goalId );
	void				UnPauseCompetition( uint32 goalId );

	void				UnBeatAllGoals();
	void				AddViewGoalsList();
	void				AddGoalChoices( bool selected_only );

	bool				GoalIsLocked( uint32 goalId );

	CGoal*				IsInCompetition();

	uint32				GetRandomBettingMinigame();
	bool				EndRunCalled( uint32 goalId );

	uint32				GetBettingGuyId();

	Script::CStruct*	GetGoalManagerParams();

	bool				ReplaceTrickText( uint32 goalId );
	bool				ReplaceTrickText( uint32 goalId, const char* p_old_string, char* p_new_string, uint out_buffer_size );
	void				ReplaceAllTrickText();

	// void				UnlockProSpecificChallenges();
	// bool				ProSpecificChallengesUnlocked();

	int					GetNumberCollected( uint32 goalId );
	int					GetNumberOfFlags( uint32 goalId );

	bool 				IsPro();

	void				ResetGoalFlags( uint32 goalId );

	void				ColorTrickObjects( uint32 goalId, int seqIndex, bool clear = false );

	int					GetNumberOfTimesGoalStarted( uint32 goalId );

	bool				GoalExists( uint32 goalId );

	Script::CStruct*	GetGoalAttemptInfo();

	void				SetCanStartGoal( int value );
	void				CheckTrickObjects();

	uint32				GetLastGoalId() { return m_lastGoal; }
	void				ClearLastGoal( void );

	// void				MarkProSpecificChallengeBeaten( uint32 skater_name, int beaten = 1 );
	// bool				SkaterHasBeatenProSpecificChallenge( uint32 skater_name );

	void				SetEndRunType( uint32 goalId, EndRunType newEndRunType );
	bool				ShouldUseTimer();
	void				SetShouldDeactivateOnExpire( uint32 goalId, bool should_deactivate );
	void				Land();
	void				Bail();
	bool				AwardMinigameCash( uint32 goalId, int amount );
	void				ResetCareer();
	void				AwardAllGoalCash();

	void				RememberLevelStructureName( uint32 level ) { m_levelStructureName = level; }
	uint32				GetLevelStructureName() { return m_levelStructureName; }

	void				SetDifficultyLevel( int level );
	GOAL_MANAGER_DIFFICULTY_LEVEL GetDifficultyLevel( bool career = false );

	void				SetGoalChaptersAndStages();
	bool				AdvanceStage( bool force = false, uint32 just_won_goal_id = 0 );
	int					GetCurrentChapter() { return m_currentChapter; }
	int					GetCurrentStage() { return m_currentStage; }
	void				SetCurrentChapter( int chapter ) { m_currentChapter = chapter; }
	void				SetCurrentStage( int stage ) { m_currentStage = stage; }
	void				RunLastStageScript();
	void				SetSponsor( uint32 sponsor ) { m_sponsor = sponsor; }
	uint32				GetSponsor() { return m_sponsor; }
	void				SetTeamMember( uint32 pro, bool remove = false, bool mark_used = false );
	void				KillTeamMembers( );
	void				SetTeamName( const char* pTeamName );
	Script::CStruct*	GetTeam() { return mp_team; }
	void				HideAllGoalPeds( bool hide );
protected:
	Lst::LookupTable<CGoal>	    m_goalList;
	Script::CStruct*            mp_goalFlags;
    
    uint32                      m_lastGoal;
	uint32						m_lastStage;
    int                         m_graffitiMode;
    int                         m_goalPoints;
	int							m_totalGoalPointsEarned;
    int                         m_cash;
	int							m_totalCash;
    
	int							m_numGoalsBeaten;
	bool 						m_isPro;
	// bool						m_proChallengesUnlocked;
	bool						m_canStartGoal;

	int							m_currentChapter;
	int							m_currentStage;

	uint32						m_sponsor;
	Script::CStruct*			mp_team;

	// Script::CStruct*			mp_proSpecificChallenges;

	uint32						m_levelStructureName;

	Script::CStruct* 			mp_difficulty_levels;
};

CGoalManager*           GetGoalManager();

bool ScriptAddGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRemoveGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptEditGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptActivateGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDeactivateGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptClearLastGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptWinGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLoseGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRemoveAllGoals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDeactivateAllGoals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUninitializeAllGoals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUninitializeGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUpdateAllGoals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptInitializeAllGoals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptInitializeAllSelectedGoals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptHasActiveGoals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGoalIsActive(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetGoalTimer(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptZeroGoalTimer(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetGoalFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptHasSeenGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptPauseAllGoals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUnPauseAllGoals(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRestartLastGoal(Script::CStruct *pParams, Script::CScript *pScript);
// bool ScriptCreateGoalFlag(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGoalFlagSet(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAllFlagsSet(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCanRetryGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetGoalParams(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCanStartGoal(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetCanStartGoal( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptNextRaceWaypoint(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptNextTourSpot(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptInitTrickObjects(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCreateGoalName(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptGetLevelPrefix( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetGraffitiMode(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptSetSpecialGoal(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptGetNumGoalsCompleted(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptGetNumGoalsNeeded(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptHasWonGoal(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptGoalIsSelected(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptToggleGoalSelection(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptGoalsAreSelected(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptGotCounterObject(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptCounterGoalDone(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptAddGoalPoint(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptSpendGoalPoints(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptHasGoalPoints(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptClearGoalPoints(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptGetNumberOfGoalPoints(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptAddCash(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptGetCash(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptSpendCash(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptHasBeatenGoalWithProset(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptGetProsetNotPrefix(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptLevelLoad(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptLevelUnload(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptNumGoalsBeatenInLevel(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptUnlockAllGoals(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptCheckMinigameRecord(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptDeactivateCurrentGoal(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptSetStartTime(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptUpdateComboTimer(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptSetStartHeight(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptCheckHeightRecord(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptCheckDistanceRecord(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptShowGoalPoints(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptHideGoalPoints(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptHidePoints(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptShowPoints(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptTurnPro(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptDeactivateAllMinigames(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptInitializeAllMinigames(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptPauseGoal(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptUnPauseGoal(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptPauseCompetition(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptUnPauseCompetition(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptPlayGoalStartStream(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptPlayGoalWinStream(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptStopCurrentStream(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptPlayGoalWaitStream(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptUnBeatAllGoals(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptAddViewGoalsList(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptGoalIsLocked(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptIsInCompetition(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptGetGoalAnimations(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptPlayGoalStream(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptSetEndRunType(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptEndRunCalled(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptClearEndRun(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptFinishedEndOfRun(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptStartedEndOfRun(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptEndBetAttempt(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptStartBetAttempt(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptBetOffered(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptBetRefused(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptWinBet(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptMoveBettingGuyNow(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptBetAccepted(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptBetIsActive(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptAddMinigameTime(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptAddTempSpecialTrick(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptRemoveTempSpecialTrick(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptGetTrickFromKeyCombo(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptUnlockGoal(Script::CStruct* pParams, Script::CScript* pScript);

bool ScriptQuickStartGoal( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptInitializeGoal( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGoalInitialized( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptAddGoalChoices( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptAddTime( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptReplaceTrickText( Script::CStruct* pParams, Script::CScript* pScript );

// bool ScriptUnlockProSpecificChallenges( Script::CStruct* pParams, Script::CScript* pScript );
// bool ScriptProSpecificChallengesUnlocked( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptGetNumberCollected( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetNumberOfFlags( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptIsPro( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptResetGoalFlags( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptColorTrickObjects( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptGetNumberOfTimesGoalStarted( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptGoalExists( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptGetGoalAttemptInfo( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptGetLastGoalId( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptClearTetrisTricks( Script::CStruct* pParams, Script::CScript* pScript );

// bool ScriptMarkProSpecificChallengeBeaten( Script::CStruct* pParams, Script::CScript* pScript );
// bool ScriptSkaterHasBeatenProSpecificChallenge( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptAnnounceGoalStarted( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptAnnounceGoalExited( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptSetShouldDeactivateOnExpire( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptGetActiveGoalId( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptAwardMinigameCash( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptResetCareer( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptAwardAllGoalCash( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptUpdateFamilyTrees( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptIsLeafNode( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptIsRootNode( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptSuspendGoalPedLogic( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptRememberLevelStructureName( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptSetDifficultyLevel( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetDifficultyLevel( Script::CStruct* pParams, Script::CScript* pScript );

// bool ScriptGetGoalParam( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptRestartStage( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptCanRestartStage( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptSetGoalChaptersAndStages( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptAdvanceStage( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetCurrentChapterAndStage( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetCurrentChapterAndStage( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptRunLastStageScript( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptFilmGoalCheckpoint( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptStartFilming( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptGoalShouldExpire( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptSetSponsor( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetSponsor( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetTeam( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetTeamMember( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptKillTeamMembers( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptSetTeamName( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptUnloadLastFam( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptStopLastSream( Script::CStruct* pParams, Script::CScript* pScript );

bool ScriptHideAllGoalPeds( Script::CStruct* pParams, Script::CScript* pScript );
}

#endif // __MODULES_SKATE_GOALMANAGER_H

