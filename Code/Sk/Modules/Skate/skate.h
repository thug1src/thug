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
**	Module:			SKATE													**
**																			**
**	File name:		modules/skate/skate.h									**
**																			**
**	Created: 		06/07/2000	-	spg										**
**																			**
*****************************************************************************/

#ifndef __MODULES_SKATE_SKATE_H
#define __MODULES_SKATE_SKATE_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/


#include <core/singleton.h>
#include <core/task.h>

#include <gel/module.h>
#include <gel/net/net.h>

#include <gfx/shadow.h> // For EShadowType

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

enum
{
	SKATE_TYPE_UNDEFINED 		= 0,
	SKATE_TYPE_SKATER 			= 1,
	SKATE_TYPE_PED				= 2,
	SKATE_TYPE_CAR				= 3,
	SKATE_TYPE_GAME_OBJ			= 4,
	SKATE_TYPE_BOUNCY_OBJ		= 5,
	SKATE_TYPE_CASSETTE			= 6,
	SKATE_TYPE_ANIMATING_OBJECT	= 7,
	SKATE_TYPE_CROWN			= 8,
	SKATE_TYPE_PARTICLE			= 9,
	SKATE_TYPE_REPLAY_DUMMY		= 10,
	SKATE_TYPE_COMPOSITE		= 11,		// just a temp type, to exclude it from certain checks
	
	// Note: If a new type is added here, please also add it to the switch statement in
	// the CCompositeObject::GetDebugInfo function in gel\object\compositeobject.cpp and also add
	// the name to data\scripts\debugger\debugger_names.q
	// That way the m_type member will appear as a name in the script debugger rather than as
	// an integer.
};

namespace Game
{
    class CGameMode;
	class CGoalManager;
}

namespace GameNet
{
	class PlayerInfo;
}

namespace Obj
{
    class CGapChecklist;
    class CGeneralManager;
	class CMovieManager;
    class CObject;
	class CPlayerProfileManager;
	class CProximManager;
	class CRailManager;
	class CSkater;
	class CSkaterCareer;
	class CSkaterProfile;
    class CTrickChecksumTable;
    class CTrickObjectManager;

#	ifdef TESTING_GUNSLINGER
	class CNavManager;
#	endif
};

namespace Prefs
{
	class Preferences;
};

namespace Records
{
	class CGameRecords;
};

namespace Script
{
    class CScript;
    class CStruct;
};

namespace Mdl
{
    class CCompetition;
    class CGameFlow;
    class CHorse;

// Little structure for storing information about a high score goal
// currently there are just two, the "high score" and "pro score"
// we did previously have an additional "sick score", which may return
// hence, I'm trying to be flexible here, if not totally object oriented...   
struct	SScoreGoal
{
	int				score;					// What score we try to get
	uint32			script;					// what script it triggers
	int				goal;					// what goal we get (can be -1 for none, if you want to do it in script)
};
   
// There is an array of vMAX_SKATERS of these in the Skate class.
// They get set from the controller config menu in the skateshop, and
// get saved out to the memory card. (see ScriptSaveToMemoryCard in cfuncs.cpp)
//
// Brad - the skaterIndex is used to map a controller to a skater.
// This is needed to meet an XBox requirement that whatever controller they
// use to start the game can be used to play.
struct SControllerPreferences
{
	int skaterIndex;
	bool VibrationOn;
	bool AutoKickOn;
	bool SpinTapsOn;
};

// Ken: A small structure for holding info needed by SkipLogic & MovingObj_Update
// to save them having to calculate the same things for billions of objects every frame.
// This structure gets filled in once per frame, and read by SkipLogic & MovingObj_Update
struct SPreCalculatedObjectUpdateInfo
{
	// This is set if it is a multiplayer game or if a cutscene is playing
	bool mDoNotSuspendAnything;
	
	// SkipLogic calculates the distance of the object from this.
	Mth::Vector mActiveCameraPosition;
};

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class Skate : public Module 
{

public:
	
	enum
	{
		vMAX_SKATERS 		= 8,
		vMAX_SCORE_GOALS    = 64,		// Don't really need 32, but just to cover the maximun number of goals
	};
									Skate( void );
	virtual							~Skate( void );
		
	// Cleans up game objects and optionally skaters
	void							Cleanup();
	void 							CleanupForParkEditor();

	
	void							Pause( bool pause = true );
	
	/************************************
	* skater control functions
	*************************************/
	Obj::CSkater* 					GetLocalSkater( void );
	Obj::CSkater*					GetSkater( uint num );
	Obj::CSkater*					GetSkaterById( uint32 id );
	uint32							GetNumSkaters( void );
	int								GetNextUnusedSkaterHeapIndex( bool for_local_skater );
	void		 					GetNextStartingPointData( Mth::Vector* pos, Mth::Matrix* matrix, int obj_id );
	void							HideSkater( Obj::CSkater* skater, bool should_hide = true );
	Obj::CSkater*					add_skater( Obj::CSkaterProfile* pSkaterProfile, bool local_client, int obj_id, int player_num );
	void							remove_skater( Obj::CSkater* skater );	// Remove a specific skater
	int								find_restart_node( int index );
	void							skip_to_restart_point( Obj::CSkater* skater, int node = -1, bool walk = false );
	void							move_to_restart_point( Obj::CSkater* skater );
	
	void							PauseGameFlow( bool paused );
	void							SetTimeLimit( int seconds );
	int								GetTimeLimit( void );

	void							FirstInputReceived();
	
	/************************************
	* game control functions
	*************************************/
	
public:
	// game control
	void							SetGameType(uint32 gameType);
	// Added by Ken, for use by the cassette menu so that it can tell whether it is to be
	// used for career, free skate or session.
	uint32							GetGameType() {return m_requested_game_type;}
	
	void							SetCurrentGameType();

	// returns checksum of level script name
	uint32							GetGameLevel();		  

	// informs the skate module that we want the game in progress to end
	void							RequestGameEnd();
	bool							IsGameInProgress() {return m_gameInProgress;}

	void							SetLaunchingQueuedScripts( void );
	void							ClearLaunchingQueuedScripts( void );
	bool							LaunchingQueuedScripts( void );

	// used to not load objects that were meant to be left
	// out for performance/bandwidth/gameplay reasons
	bool							IsMultiplayerGame( void );
	bool							ShouldBeAbsentNode( Script::CStruct* pNode );
	
	void							OpenLevel(uint32 level_script);
	void							RestartLevel();
	void							ResetLevel();
	void							ResetSkaters(int node = -1, bool walk = false);
	void							ChangeLevel( uint32 level_id );
	void							LaunchGame();
	void							LaunchNetworkGame();

	/************************************
	* network functions					*
	*************************************/
	void							StartServer( void );
	void							AddNetworkMsgHandlers( Net::Client* client, int index );
	void							LeaveServer( void );
	void							SendScoreUpdates( bool final = false );
	void 							ObserveNextSkater( void );
	uint32							GetCheatChecksum( void );
	void							SendCheatList( GameNet::PlayerInfo* player );

	/************************************
	* other functions
	*************************************/

	const char*						GetSkaterDisplayName( int id );
	Obj::CSkaterProfile*			GetProfile( int i );
	Obj::CSkaterProfile*			GetCurrentProfile( Script::CStruct* pParams = NULL );
	Obj::CSkaterCareer*				GetCareer(){return mp_career;} 
	Game::CGameMode*				GetGameMode();
	Game::CGoalManager*				GetGoalManager();
	Obj::CGeneralManager*			GetObjectManager( void );
	Obj::CTrickObjectManager*		GetTrickObjectManager( void );
	Obj::CPlayerProfileManager*		GetPlayerProfileManager( void );
	Obj::CTrickChecksumTable*		GetTrickChecksumTable() { return mp_trickChecksumTable; }

	Prefs::Preferences*				GetSplitScreenPreferences( void );
	float							GetHandicap( int id );

	void 							CheckSkaterCollisions( void );
	
    static Tsk::Task< Skate >::Code   	s_logic_code; 
	static Tsk::Task< Skate >::Code   	s_object_update_code; 
	static Tsk::Task< Skate >::Code   	s_score_update_code; 
	static void	s_object_update( Obj::CObject* object, void* data );
	
private:
			
	void							v_start_cb( void );
	void							v_stop_cb( void );
	
	Tsk::Task< Skate >*				m_logic_task;	
	Tsk::Task< Skate >*				m_object_update_task;	
	Tsk::Task< Skate >*				m_score_update_task;    


	Obj::CSkaterCareer*				mp_career;
	
public:
	Obj::CRailManager*	            mp_railManager;    
	Obj::CProximManager*	        mpProximManager;
	void 							UpdateGameFlow();
	Obj::CRailManager*	            GetRailManager() { return mp_railManager; }

#	ifdef TESTING_GUNSLINGER
	Obj::CNavManager*				mp_navManager;
	Obj::CNavManager*	            GetNavManager() { return mp_navManager; }
#	endif

	void			                Rail_DebugRender();

private:
	void 							DoUpdate();	// called by s_logic

	
//	Obj::CGeneralManager*			mp_obj_manager;
	Lst::Head< Obj::CSkater >		m_skaters;
	    
	bool 							m_recording;
	bool 							m_playing;
	int 							m_fd; 

	bool							m_first_input_received;
	int								m_controller_unplugged_frame_count;

	// Network msg handlers
	void							net_setup( void );
	void							net_shutdown( void );
	bool							should_send_update( GameNet::PlayerInfo* send_player, Obj::CObject* object );

	bool							m_launching_queued_scripts;
	    
	static Net::MsgHandlerCode		handle_cheats;
	static Net::MsgHandlerCode		handle_cheat_list;
	static Net::MsgHandlerCode		handle_projectile;
	static Net::MsgHandlerCode		handle_enter_vehicle_client;
	
	static Net::MsgHandlerCode		handle_anims;
	static Net::MsgHandlerCode		handle_flip_anim;
	static Net::MsgHandlerCode		handle_enter_vehicle_server;
	
	static Net::MsgHandlerCode		handle_msg_relay;
	static Net::MsgHandlerCode		handle_selective_msg_relay;
	static Net::MsgHandlerCode		handle_script_relay;
	static Net::MsgHandlerCode		handle_score_request;
	static Net::MsgHandlerCode		handle_landed_trick;
	static Net::MsgHandlerCode		handle_combo_report;
	static Net::MsgHandlerCode		handle_object_update;
	static Net::MsgHandlerCode		handle_object_update_relay;
	static Net::MsgHandlerCode		handle_player_quit;
	static Net::MsgHandlerCode		handle_disconn_accepted;
	static Net::MsgHandlerCode		handle_kicked;
	static Net::MsgHandlerCode		handle_play_sound;
	static Net::MsgHandlerCode		handle_sparks;
	static Net::MsgHandlerCode		handle_blood;
	static Net::MsgHandlerCode		handle_score_update;
	static Net::MsgHandlerCode		handle_bail_done;
	static Net::MsgHandlerCode		handle_change_level;
	static Net::MsgHandlerCode		handle_run_script;
	static Net::MsgHandlerCode		handle_spawn_run_script;
	static Net::MsgHandlerCode		handle_start_info;
	static Net::MsgHandlerCode		handle_free_trick_objects;
	static Net::MsgHandlerCode		handle_obs_log_trick_objects;
	static Net::MsgHandlerCode		handle_obs_init_graffiti_state;
	static Net::MsgHandlerCode		handle_face_data;
		
	friend bool ScriptSetGameType(Script::CStruct *pParams, Script::CScript *pScript);
	friend bool ScriptInTeamGame(Script::CStruct *pParams, Script::CScript *pScript);
	friend bool ScriptLaunchLevel(Script::CStruct *pParams, Script::CScript *pScript);
    friend bool ScriptSetTimeLimit(Script::CStruct *pParams, Script::CScript *pScript);
	
	// Exactly what it says. This should be the master flag for determining if a game is in progress.
	bool							m_gameInProgress;
	bool							m_levelLoaded;

	bool							m_endRun;

	uint32 							m_gameType;

	// needed so that we can restore the skatercamera
	// after skatercams and cutscenes...
	bool							m_lastFrameWasMovieCamera;


	// Keeps track of all the rules/options for this particular game mode
	Game::CGameMode*				mp_gameMode;

	// Keeps track of all the trick objects in the scene
	Obj::CTrickObjectManager*		mp_trickObjectManager;

	// Keeps track of all the trick names in the game
	Obj::CTrickChecksumTable*		mp_trickChecksumTable;

	// Keeps track of skater appearance, stats, etc.
	Obj::CPlayerProfileManager*		mp_PlayerProfileManager;

	// Keeps track of all the high scores and suchlike
	Records::CGameRecords*			mp_gameRecords;

	// Keeps track of splitscreen preferences
	Prefs::Preferences*				mp_splitscreen_preferences;

	uint32							m_requested_game_type;
	bool							m_levelChanged;

	SScoreGoal						m_ScoreGoal[vMAX_SCORE_GOALS];
	CCompetition*					mp_competition;
	CHorse*							mp_horse;
	Obj::CMovieManager*				mp_movieManager;
	Obj::CMovieManager*				mp_objectAnimManager;

	Gfx::EShadowType				m_shadow_mode;
	
	int								m_time_limit;

public:
	void 							BeepTimer(float time, float beep_time, float beep_speed, const char *p_script_name);

	bool							ShouldAllocateInternetHeap( void );
	bool							ShouldAllocateNetMiscHeap( void );

	uint32							m_requested_level;
	uint32							m_cur_level;
	uint32							m_prev_level;
	
	bool							m_new_record;
		
	void							GetRecordsText(int level);
	void 							UpdateRecords();

	void							SetShadowMode( Gfx::EShadowType shadowMode );
	Gfx::EShadowType				GetShadowMode( void ) const;
	
	Records::CGameRecords*			GetGameRecords() {return mp_gameRecords;}

	Obj::CGapChecklist		*		GetGapChecklist();
	
// Some handy dandy functions for accessing the high score goals 	
	int								GetScoreGoalScore(int n){return m_ScoreGoal[n].score;}
	void							SetScoreGoalScore(int n, int score){m_ScoreGoal[n].score = score;}
	uint32							GetScoreGoalScript(int n){return m_ScoreGoal[n].script;}
	void							SetScoreGoalScript(int n, uint32 script){m_ScoreGoal[n].script = script;}	
	int								GetScoreGoalGoal(int n){return m_ScoreGoal[n].goal;}
	void							SetScoreGoalGoal(int n, uint32 goal){m_ScoreGoal[n].goal = goal;}
	void							ClearScoreGoals() {for (int i =0;i<vMAX_SCORE_GOALS;i++) SetScoreGoalScore(i,0);}

	// for detecting end-of-run
	bool							FirstTrickStarted();
	bool							FirstTrickCompleted();
	bool							SkatersAreIdle();
	bool							EndRunSelected() {return m_endRun;}
	void 							EndRun() {m_endRun = true;}
	void							ClearEndRun() {m_endRun = false;}

	CCompetition*					GetCompetition(){return mp_competition;}
	CHorse*							GetHorse(){return mp_horse;}
	Obj::CMovieManager*				GetMovieManager(){return mp_movieManager;}
	Obj::CMovieManager*				GetObjectAnimManager(){return mp_objectAnimManager;}

	CGameFlow*						mp_gameFlow;
	Game::CGoalManager*				mp_goalManager;
	
	SControllerPreferences			mp_controller_preferences[vMAX_SKATERS];
	int								m_device_server_map[vMAX_SKATERS];
	void 							SetSpinTaps(int index, bool state);
	void							SetAutoKick(int index, bool state);
	void							SetVibration(int index, bool state);
	
	
	void							SetStatOverride(float s) {m_stat_override = s;}
	float							GetStatOverride() {return m_stat_override;}									 

	void							SetDrawRails(bool x) {m_draw_rails = x;}
	bool							GetDrawRails() {return m_draw_rails;}

	void							UpdateSkaterInputHandlers();
private:

	float							m_stat_override;
	bool							m_draw_rails;

	// Ken: A small structure for holding info needed by SkipLogic & MovingObj_Update
	// to save them having to calculate the same things for billions of objects every frame.
	// This structure gets filled in once per frame, and read by SkipLogic & MovingObj_Update
	SPreCalculatedObjectUpdateInfo	m_precalculated_object_update_info;
	void							pre_calculate_object_update_info();
public:
	SPreCalculatedObjectUpdateInfo	*GetPreCalculatedObjectUpdateInfo() {return &m_precalculated_object_update_info;}
	
private:
	DeclareSingletonClass( Skate );
};

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

bool ScriptSetGameType(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptTestGameType(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptTestRequestedGameType(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptRetry(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptLaunchGame(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptChangeLevel(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptResetLevel(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptLaunchLevel(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptRequestLevel(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptInitSkaterHeaps(Script::CStruct* pParams, Script::CScript* pScript);
bool ScriptFillRankingScreen(Script::CStruct* pParams, Script::CScript* pScript);

// GJ:  These are some convenience functions that don't really
// belong in any class, but are of general use to the skate code
// If such a beast existed, it would
Prefs::Preferences*	GetPreferences( uint32 checksum, bool assert_on_fail = true );
Prefs::Preferences*	GetPreferences( Script::CStruct* pParams, bool assert_on_fail = true );

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**						Inline Functions									**
*****************************************************************************/

inline uint32 Skate::GetNumSkaters()
{
	return m_skaters.CountItems();
}

} // namespace Mdl

#endif	// __MODULES_SKATE_SKATE_H

