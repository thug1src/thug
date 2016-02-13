#ifndef	__MODULES_SKATE_GOAL_H
#define	__MODULES_SKATE_GOAL_H

                           
#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#ifndef __SYS_TIMER_H
    #include <sys/timer.h> // For Tmr::Time
#endif
							  
#include <sk/modules/skate/goalmanager.h>
#include <sk/modules/skate/goalped.h>

#include <core/list/node.h>
#include <core/lookuptable.h>

#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>


namespace Game
{
	enum EndRunType
	{
		vGOALENDOFRUN,
		vENDOFRUN,
		vNONE,
	};

	enum
	{
		GOAL_INHERITED_FLAGS_ONE_TIMER		= ( 1 << 1  ),
		GOAL_INHERITED_FLAGS_RETRY_STAGE	= ( 1 << 2  ),
	};
	
// *************************************************************
//			Class def
// *************************************************************

class CGoalLink
{
public:
	CGoalLink();
	~CGoalLink();
	
	uint32	   	m_relative;
	CGoalLink*	mp_next;
	bool		m_beaten;
};

class CGoal : public  Lst::Node<CGoal>
{
public:
	CGoal();
	CGoal(Script::CStruct* pParams);
	~CGoal();

public:
	virtual bool		IsExpired();
	bool				IsExpired( bool force_end );
	virtual bool		ShouldExpire();
	
	virtual bool		IsActive();
	virtual bool		CountAsActive(); // eg, minigames don't count
    virtual void		Expire();

	virtual void		LoadSaveData( Script::CStruct* pFlags );
	virtual void		GetSaveData( Script::CStruct* pFlags );

	void				AddChild( uint32 id );
	void				AddParent( uint32 id );
	void				ResetGoalLinks();
	void				DeleteLinks( CGoalLink* p_list );

	void                MarkInvalid();
    bool                IsInvalid(); 
	void				SetTimer();
	void				AddTime( int time );
	void				SetTimer( int time );
	int					GetTimeLeft( void );
	virtual bool		ShouldUseTimer();
	virtual bool		Activate();
	virtual void		SetActive();
	virtual bool		Deactivate( bool force = false, bool affect_tree = true );
	virtual void		SetInactive();
	virtual bool		Win();
	virtual bool		Lose();
	bool				IsProGoal();
	bool				IsNetGoal();

	bool				RunCallbackScript( uint32 script_name );
	Script::CStruct*	GetParams();
	bool				EditParams(Script::CStruct* pParams);
	virtual bool		Update();

    bool                HasSeenGoal();
    void                PauseGoal();
    void                UnPauseGoal();
	virtual bool		IsPaused();
    Tmr::Time           GetTime();
    // void                CreateGoalFlag( uint32 flag );
    bool                SetGoalFlag( uint32 flag, int value );
	bool				GetCreatedGoalGap(int gapIndex);
	
    void                ResetGoalFlags();
	void				CreateGoalFlags( Script::CStruct* pParams );
    bool                GoalFlagEquals( uint32 flag, int value );
    bool                AllFlagsSet();
	bool				GoalFlagSet( uint32 flag );
    void                Init();
	void                Uninit( bool propogate = false, int recurse_level = 0 );
    bool                IsLocked();
    void                UnlockGoal();
    uint32              GetGoalId();
	uint32              GetRootGoalId();
	bool				IsEditedGoal();
	void				GetViewGoalsText( const char** p_view_goals_text );
    virtual bool        HasWonGoal();
	virtual bool        HasWonGoal( int id );
	virtual bool        HasTeamWonGoal( int team_id );
	virtual bool		HasWonStage();
	bool				IsSelected();
	virtual bool		IsInitialized() { return m_isInitialized; }

    bool                NextTourSpot();
    void                InitTrickObjects();

	void				GotTrickObject( uint32 clusterId );
	bool				IsGraffitiGoal();
	bool				ShouldLogTrickObject();
    void                GotGraffitiCluster( uint32 clusterId, int score );
    bool                IsSpecialGoal();
    bool                IsTetrisGoal();
    bool                CheckSpecialGoal();
    
    void                GotCounterObject();
    bool                CounterGoalDone();
    bool                HasProset( const char* proset_prefix );
	bool				CheckMinigameRecord( int value );
	int					GetMinigameRecord();
	bool				IsMinigame();
	bool				IsBettingGame();
	bool				IsBettingGuy();
	virtual bool		CanRetry();
	void				SetStartTime();
	void				UpdateComboTimer();

    void                SetHasSeen();
    virtual void        MarkBeaten();
	virtual void		MarkBeatenBy( int id );
	virtual void		MarkBeatenByTeam( int team_id );
	void				Select();
	void				Deselect();
    void                SetUnlocked();
    void                SetLevelNum( int levelNum );
    int                 GetLevelNum();
	void				SetStartHeight();
	bool				CheckHeightRecord();
	bool				CheckDistanceRecord();

	void 				SetQuickStartFlag();
	void				UnsetQuickStartFlag();

	bool				IsCompetition();

	bool				AllSkatersAtEndOfRun();
	bool				EndRunCalled();
	void				ClearEndRun( Script::CScript* pScript );
	bool				FinishedEndOfRun();
	bool				StartedEndOfRun();
	void				SetEndRunType( EndRunType newEndRunType );

	virtual bool		UnBeatGoal();

	void				NoValidKeyCombos();
	int 				GetRandomIndexFromKeyCombos( Script::CArray* p_KeyCombos );

	bool				AddTempSpecialTrick();
	void				RemoveTempSpecialTrick();

	bool				ReplaceTrickText();

	int					GetNumberCollected();
	int					GetNumberOfFlags();

	void				ColorTrickObjects( int seqIndex, bool clear = false );

	int					GetNumberOfTimesGoalStarted();

	void				GetRewardsStructure( Script::CStruct *p_structure, bool give_cash = true );
	void				UnlockRewardGoals();

	void				SetShouldDeactivateOnExpire( bool should_deactivate );

	bool				IsHorseGoal();
	bool				IsFindGapsGoal();

	// family stuff	
	Game::CGoalLink*	GetChildren() { return mp_children; }
	Game::CGoalLink*	GetParents() { return mp_parents; }

	void				SetInheritableFlags( int recurse_level = 0 );

	Game::CGoalPed*		mp_goalPed;

	// the inhereted flags of any root node
	// in a family branch will be set in the goals of all offspring
	uint32				m_inherited_flags;

	// chatper/stage stuff
	void				SetChapter( int chapter ) { m_chapter = chapter; }
	void				SetStage( int stage ) { m_stage = stage; }
	int					GetChapter() { return m_chapter; }
	int					GetStage() { return m_stage; }

	bool				ShouldRequireCombo();

	void				AppendDifficultyLevelParams();
protected:	
	Script::CStruct*	mp_params;
    Script::CStruct*	mp_goalFlags;
	
	EndRunType			m_endRunType;

	Game::CGoalLink*	mp_children;
	Game::CGoalLink*	mp_parents;
	
	// members stored by goal manager on level unload
    int                 m_levelNum;
	Flags< int >		m_beatenFlags;
	Flags< int >		m_teamBeatenFlags;
	int					m_numTimesLost;
	int					m_numTimesWon;
	int					m_numTimesStarted;
	Tmr::Time			m_elapsedTimeRecord;
	int					m_winScoreRecord;
    Tmr::Time           m_timeLeft;
	int					m_ownerId;
    int                 m_numflags;
    int                 m_flagsSet;
	Tmr::Time			m_elapsedTime;
	int					m_chapter;
	int					m_stage;
    
	bool				m_isInitialized;
	bool                m_hasSeen;
    bool                m_isbeat;
	bool				m_netIsBeat;
	bool				m_isselected;
    bool                m_isLocked;
	bool				m_deactivateOnExpire;
	bool				m_endRunCalled;
	bool				m_active;
    bool                m_invalid;
    bool                m_paused;
	bool				m_isOwned;
    bool                m_unlimitedTime;
};

}	// namespace Game

#endif // __MODULES_SKATE_GOAL_H
