/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate3													**
**																			**
**	Module:			GameNet					 								**
**																			**
**	File name:		GameNet.h												**
**																			**
**	Created by:		02/01/01	-	spg										**
**																			**
**	Description:	Game-Side Network Code 									**
**																			**
*****************************************************************************/

#ifndef __GAMENET_GAMENET_H
#define __GAMENET_GAMENET_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/


#include <gel/net/net.h>
#include <gel/prefs/prefs.h>
#include <gel/inpman.h>

#include <gamenet/gamemsg.h>

#include <sk/objects/skaterflags.h>
#include <sk/modules/skate/skate.h>

#ifdef __PLAT_NGPS__
//#include <cengine/goaceng.h>
#include <ghttp/ghttp.h>
#include <peer/peer.h>
#include <gamenet/ngps/p_ezcommon.h>
#include <gamenet/ngps/p_netcnfif.h>
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

	  
namespace	Obj
{
	class	CSkaterProfile;
	class	CCrown;
    class   CMovingObject;
}

namespace GameNet
{

						

enum
{
	vVERSION_NUMBER = 0x30036,	// version number. High word is the integer, lo word is the decimal fraction
};

enum
{
	vMAX_RANK			=	10,
	vMAX_GAME_CREDIT	=	500,
	vNUM_RANKED_LEVELS	=	12,
	vMAX_SCORE_CREDIT	=	100,
	vMAX_COMBO_CREDIT	=	100,
	vMAX_RATING			= (( vMAX_GAME_CREDIT ) + ( vNUM_RANKED_LEVELS * ( vMAX_SCORE_CREDIT + vMAX_COMBO_CREDIT ))),
};

enum NetworkMode
{
	vNO_NET_MODE,
	vLAN_MODE,
	vINTERNET_MODE
};

enum JoinMode
{
	vJOIN_MODE_PLAY,
	vJOIN_MODE_OBSERVE
};

enum
{
	mSERVER 	= 0x0001,
	mCLIENT 	= 0x0002,
	mINTERNET	= 0x0004,
	mLAN		= 0x0008,
	
	mNETWORK_MODE = mINTERNET | mLAN
};
						 
enum
{
	vMAX_LOCAL_CLIENTS		= 2	// two-player split screen
};

enum
{
	vSKILL_LEVEL_1,
	vSKILL_LEVEL_2,
	vSKILL_LEVEL_3,
	vSKILL_LEVEL_4,
	vSKILL_LEVEL_5,
	
	vSKILL_LEVEL_DEFAULT = vSKILL_LEVEL_3
};

enum
{
	vDEFERRED_JOIN_FRAMES	=	10
};

enum
{
	vSCORE_UPDATE_FREQUENCY	=	2000	// update the score every 2 seconds
};

enum DropReason
{
	vREASON_QUIT,
	vREASON_TIMEOUT,
	vREASON_OBSERVING,
	vREASON_BANNED,
	vREASON_KICKED,
	vREASON_BAD_CONNECTION,
	vREASON_LEFT_OUT
};

// Valid fields of object updates
enum
{
	mUPDATE_FIELD_POS_X			= 0x0001,
	mUPDATE_FIELD_POS_Y 		= 0x0002,
	mUPDATE_FIELD_POS_Z 		= 0x0004,
	mUPDATE_FIELD_ROT_X			= 0x0008,
	mUPDATE_FIELD_ROT_Y			= 0x0010,
	mUPDATE_FIELD_ROT_Z			= 0x0020,
	mUPDATE_FIELD_STATE			= 0x0040,
	mUPDATE_FIELD_FLAGS 		= 0x0080,
	mUPDATE_FIELD_RAIL_NODE		= 0x0100,
};

enum
{
	mSTATE_MASK = 0x07,
	mDOING_TRICK_MASK = 0x80,
};

enum ServerListState
{
	vSERVER_LIST_STATE_WAIT,
	vSERVER_LIST_STATE_INITIALIZE,
	vSERVER_LIST_STATE_SHUTDOWN,
	vSERVER_LIST_STATE_STARTING_MOTD,
	vSERVER_LIST_STATE_GETTING_MOTD,
	vSERVER_LIST_STATE_TRACK_USAGE,
	vSERVER_LIST_STATE_SHOW_MOTD,
	vSERVER_LIST_STATE_RETRY_MOTD,
	vSERVER_LIST_STATE_FAILED_MOTD,
	vSERVER_LIST_STATE_TRACKING_USAGE,
	vSERVER_LIST_STATE_STARTING_LOBBY_LIST,
	vSERVER_LIST_STATE_GETTING_LOBBY_LIST,
	vSERVER_LIST_STATE_GOT_LOBBY_LIST,
	vSERVER_LIST_STATE_FAILED_LOBBY_LIST,
	vSERVER_LIST_STATE_FILL_LOBBY_LIST,
};

enum JoinState
{   
	vJOIN_STATE_WAITING,
	vJOIN_STATE_CONNECTING,
	vJOIN_STATE_JOINING,
	vJOIN_STATE_JOINING_WITH_PASSWORD,
	vJOIN_STATE_TRYING_PASSWORD,
	vJOIN_STATE_GOT_PLAYERS,
	vJOIN_STATE_WAITING_FOR_START_INFO,
	vJOIN_STATE_CONNECTED,
	vJOIN_STATE_REFUSED,
	vJOIN_STATE_FINISHED
};

enum ObserverCommand
{
	vOBSERVER_COMMAND_NONE,
	vOBSERVER_COMMAND_NEXT,
};

enum HostMode
{
	vHOST_MODE_SERVE,
	vHOST_MODE_AUTO_SERVE,
	vHOST_MODE_FCFS,      
};

enum
{
	vSORT_KEY_NAME,
	vSORT_KEY_PING,
	vSORT_KEY_NUMPLAYERS,
	vSORT_KEY_MODE,
	vSORT_KEY_LEVEL,
	vSORT_KEY_SKILL,
	vNUM_SORT_KEYS
};

enum
{
	vTEAM_RED,
	vTEAM_BLUE,
	vTEAM_GREEN,
	vTEAM_YELLOW,
	vMAX_TEAMS,

	vNO_TEAM = -1
};

enum
{
	vNET_THREAD_STACK_SIZE = ( 32 * 1024 )
};

enum
{
	vNUM_CROWN_SPAWN_POINTS	= 10
};

enum
{
	vMAX_MASTER_SERVERS = 10,
	vMAX_NUM_PROSETS = 7,
	vMAX_GOAL_SIZE = 6000,
};

enum
{
	vGAMESPY_PRODUCT_ID = 10160
};

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class LobbyMan;
class ContentMan;
class BuddyMan;
class StatsMan;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  NewPlayerInfo  : public Spt::Class
{
	
	
	public:
		NewPlayerInfo();
		~NewPlayerInfo();
		
	public:
		char					Name[ vMAX_PLAYER_NAME_LEN + 1 ];
		char					ObjID;
		Net::Conn*				Conn;
		unsigned int			AllowedJoinFrame;
		Obj::CSkaterProfile*	mpSkaterProfile;
		int 					Flags;
		int						JumpInFrame;
		int						Team;
		int						Profile;
		int						Rating;
		int						Score;
		uint32					VehicleControlType;
};

class TriggerEvent : public Lst::Node< TriggerEvent >
{   
public:
		TriggerEvent( void ) 
			: Lst::Node< TriggerEvent > ( this ) {}

		int						m_Node;
		int						m_ObjID;
		uint32					m_Script;
};

class LastSentProps
{
public:

	unsigned int	m_LastSkaterUpdateTime[Mdl::Skate::vMAX_SKATERS];	// The last time of update
	char			m_LastSkaterTerrain[Mdl::Skate::vMAX_SKATERS];		// The last terrain of each skater
	char			m_LastSkaterWalking[Mdl::Skate::vMAX_SKATERS];		// The last walking of each skater
	char			m_LastSkaterDriving[Mdl::Skate::vMAX_SKATERS];		// The last driving of each skater
	short			m_LastSkaterPosUpdate[Mdl::Skate::vMAX_SKATERS][3];	// The last pos of each skater
	short			m_LastSkaterRotUpdate[Mdl::Skate::vMAX_SKATERS][3];	// The last pos of each skater
    Flags< int >	m_LastSkaterFlagsUpdate[Mdl::Skate::vMAX_SKATERS];	// The last flags 
																	   	// we sent to each skater
	Flags< int >	m_LastEndRunFlagsUpdate[Mdl::Skate::vMAX_SKATERS];	// The last end run flags 
																	   	// we sent to each skater
	char			m_LastSkaterStateUpdate[Mdl::Skate::vMAX_SKATERS];	// The last state
																		// we sent to each skater
	char			m_LastDoingTrickUpdate[Mdl::Skate::vMAX_SKATERS];	// The last "doing trick"
																		// state we sent to each skater
	char			m_LastDrivingUpdate[Mdl::Skate::vMAX_SKATERS];		// The last "driving"
																		// state we sent to each skater
	sint16			m_LastRailNodeUpdate[Mdl::Skate::vMAX_SKATERS];		// The last rail node sent to each skater
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class PlayerInfo : public Lst::Node< PlayerInfo >
{
public:
	
	friend class Manager;

	enum SkaterFlagMasks
	{
		mTENSE = nBit( Obj::TENSE ),
		// mSWITCH = nBit( Obj::SWITCH ),
		// mLEAN = nBit( Obj::LEAN ),
		// mTENSE_ON_GROUND = nBit( Obj::TENSE_ON_GROUND ),
		mFLIPPED = nBit( Obj::FLIPPED ),
		mVERT_AIR = nBit( Obj::VERT_AIR),
		mTRACKING_VERT = nBit( Obj::TRACKING_VERT ),
		mLAST_POLY_WAS_VERT = nBit( Obj::LAST_POLY_WAS_VERT ),
		mCAN_BREAK_VERT = nBit( Obj::CAN_BREAK_VERT ),
		mCAN_RERAIL = nBit( Obj::CAN_RERAIL ),
		// mSHOULD_END_RUN = nBit( Obj::SHOULD_END_RUN ),
		// mFINISHED_END_OF_RUN = nBit( Obj::FINISHED_END_OF_RUN ),
		mRAIL_SLIDING = nBit( Obj::RAIL_SLIDING ),
		mCAN_HIT_CAR = nBit( Obj::CAN_HIT_CAR ),
		mAUTOTURN = nBit( Obj::AUTOTURN ),
		mIS_BAILING = nBit( Obj::IS_BAILING ),
	};

	enum
	{
		mLOCAL_PLAYER	=	0x00000001,
		mNON_COLLIDABLE	=	0x00000002,	// For use, for example, after skater is knocked down so you don't
									// knock him down over and over
		mOBSERVER		=	0x00000004,
		mPENDING_PLAYER	=	0x00000008,	// Player is observing until the game finishes
		mJUMPING_IN		=	0x00000010,	// Transitioning from pending to playing
		mFULLY_IN		=	0x00000020,	// Player is fully in the game
		mHAS_PLAYER_INFO=	0x00000040,	// Player has fully received initial player info packet(s)
		mKING_OF_THE_HILL=	0x00000080,	// Player is king of the hill
		mRUN_ENDED		=	0x00000100,	// Player's run has ended
		mSERVER_PLAYER	=	0x00000200,	// The player acting as host
		mMARKED_PLAYER	=	0x00000400,	// The player is marked for an operation
		mHAS_RED_FLAG	=	0x00000800,	// The player has the red flag
		mHAS_BLUE_FLAG	=	0x00001000,	// The player has the blue flag
		mHAS_GREEN_FLAG	=	0x00002000,	// The player has the green flag
		mHAS_YELLOW_FLAG=	0x00004000,	// The player has the yellow flag
		mRESTARTING		=	0x00008000,	// The player is restarting
		mSURVEYING		=	0x00010000,	// The player is surveying the playing field

		mHAS_ANY_FLAG = mHAS_RED_FLAG | mHAS_BLUE_FLAG | mHAS_GREEN_FLAG | mHAS_YELLOW_FLAG,
	};
	
	PlayerInfo( int flags );
	
	void			SetSkater( Obj::CSkater* skater );
	void			CopyProfile( Obj::CSkaterProfile* pSkaterProfile );

	void			SetFaceData( uint8* face_data, int size );
	uint8*			GetFaceData( void );
	
	bool			IsLocalPlayer( void );
	bool			IsServerPlayer( void );
	bool			IsObserving( void );
	bool			IsSurveying( void );

	void			MarkAsServerPlayer( void );
	void			MarkAsNotServerPlayer( void );
	void			MarkAsReady( int time );
	void			MarkAsNotReady( int time );
	void			MarkAsRestarting( void );
	void			SetReadyQueryTime( int time );

	void			MarkAsNonCollidable( void );
	void			ClearNonCollidable( void );
	bool			IsNonCollidable( void );

	void			MarkAsFullyIn( void );
	bool			IsFullyIn( void );

	void			MarkAsKing( bool mark );
	bool			IsKing( void );

	bool			HasCTFFlag( void );
	int				HasWhichFlag( void );
	void			TookFlag( int team );
	void			StoleFlag( int team );
	void			CapturedFlag( int team );
	void			RetrievedFlag( void );
	void			ClearCTFState( void );

	bool			IsVulnerable( void );
	void			ResetProjectileVulnerability( void );
	void			SetHitTime( Tmr::Time hit_time );

	bool			IsPendingPlayer( void );

	int				GetLastObjectUpdateID( void );
	void			SetLastObjectUpdateID( int id );
	int				GetMaxObjectUpdates( void );

	int				GetConnHandle( void );
	int				GetSkaterNumber( void );

	Obj::CSkater*	m_Skater;
	Net::Conn*		m_Conn;
	int				m_Score;
	int				m_BestCombo;
	int				m_Profile;
	int				m_Rating;
	int				m_Team;
	char			m_Name[ vMAX_PLAYER_NAME_LEN + 1 ];
	uint32			m_VehicleControlType;
	Obj::CSkaterProfile*	mp_SkaterProfile;							// Appearance information
    
	LastSentProps	m_LastSentProps;

protected:
					~PlayerInfo();
private:
	static	Tsk::Task< PlayerInfo >::Code   	s_observer_logic_code;

	Flags< int >	m_flags;
	int				m_latest_ready_query;
	int				m_jump_in_frame;
	Tmr::Time		m_last_bail_time;
	Tmr::Time		m_last_hit_time;
	int				m_last_object_update_id;
	uint8*			m_face_data;

	Tsk::Task< PlayerInfo >*		m_observer_logic_task;
};

class ServerInfo : public Lst::Node< ServerInfo >
{
	public:
		ServerInfo( void );
		~ServerInfo( void );

		void	AddPlayer( char* name, int rating = 0 );
		char*	GetPlayerName( int index );
		int		GetPlayerRating( int index );
		void	ClearPlayerNames( void );

		char	m_Name[vMAX_SERVER_NAME_LEN + 1];
		int		m_Ip;
		int 	m_Port;
		int		m_Latency;
		int		m_NumPlayers;
		int		m_MaxPlayers;
		int		m_NumObservers;
		int		m_MaxObservers;
		char	m_Level[32];
		char	m_Mode[32];
		bool	m_Password;
		bool	m_GameStarted;
		char	m_SkillLevel;
		int		m_HostMode;
		bool	m_Ranked;
		bool	m_CanDirectConnect;
		bool	m_Listed;
		bool	m_InFocus;
		bool	m_HasBasicInfo;
		bool	m_HasFullInfo;
#ifdef __PLAT_NGPS__
		SBServer m_GServer;
#endif
#ifdef __PLAT_XBOX__
		XNKID		m_XboxKeyId;
		XNKEY		m_XboxKey;
		XNADDR		m_XboxAddr;
#endif // __PLAT_XBOX__

	private:
		Lst::Head< PlayerInfo > m_players;
};

class LobbyInfo : public Lst::Node< LobbyInfo >
{
	public:
		LobbyInfo( void );

		char	m_Name[64];
		int		m_GroupId;
		int		m_NumServers;
		int 	m_MaxServers;
		int		m_MinRating;
		int		m_MaxRating;
		bool	m_Full;
		bool	m_OffLimits;
		bool	m_Official;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  Manager  : public Spt::Class
{
	
public:
	void				UsePreferences( void );

//	Client and Server Access
/////////////////////////////
	Net::Server*		SpawnServer( bool local, bool secure );
	Net::Client*		SpawnClient( bool broadcast, bool local, bool secure, int index );
	Net::Client*		SpawnMatchClient( void );
	void				SpawnAutoServer( void );

	void				ServerShutdown( void );
	void				ClientShutdown( void );
	void				MatchClientShutdown( void );
	void				AutoServerShutdown( void );

	Net::Server*		GetServer( void );
	Net::Client*		GetClient( int index );
	Net::Client*		GetMatchClient( void );

	int					GetJoinIP( void );
	int					GetJoinPrivateIP( void );
	int					GetJoinPort( void );
	void				SetJoinIP( int ip );
	void				SetJoinPrivateIP( int ip );
	void				SetJoinPort( int port );

//	Player Access
/////////////////////////////
	void				LoadPendingPlayers( void );
	int					NumPartiallyLoadedPlayers( void );
	void				DropPartiallyLoadedPlayers( void );
	void				DropPlayer( PlayerInfo* info, DropReason reason );
	void				ResetPlayers( void );
	int					GetMaxPlayers( void );
	int					GetMaxObservers( void );
	void				SetMaxPlayers( int max_players );
	void				SetMaxObservers( int max_observers );
	int					GetNumPlayers( void );
	int					GetNumObservers( void );
	int					GetNumTeams( void );
	int					NumTeamMembers( int team );
	void				CreateTeamFlags( int num_teams );
	void				ClientAddNewPlayer( NewPlayerInfo* new_player );
	void				DeferredNewPlayer( NewPlayerInfo* new_player, 
										   int num_frames = vDEFERRED_JOIN_FRAMES );
	PlayerInfo*			NewPlayer( Obj::CSkater* skater, Net::Conn* conn, int flags = 0 );
	void				AddPlayerToList( PlayerInfo* player );
	void				DestroyPlayer( PlayerInfo* player );
    
	int					GetNextPlayerObjectId( void );
	Obj::CSkater*		GetSkaterByConnection( Net::Conn* conn );          
	PlayerInfo*			FirstPlayerInfo( Lst::Search< PlayerInfo > &sh, bool include_observers = false );
	PlayerInfo*			NextPlayerInfo( Lst::Search< PlayerInfo > &sh, bool include_observers = false );
	PlayerInfo*			GetPlayerByConnection( Net::Conn* conn );          
	PlayerInfo*			GetPlayerByObjectID( unsigned short obj_id );
    
	PlayerInfo*			GetLocalPlayer( void );
	PlayerInfo*			GetServerPlayer( void );
	PlayerInfo*			GetNextPlayerToObserve( void );
	PlayerInfo*			GetCurrentlyObservedPlayer( void );
	PlayerInfo*			GetKingOfTheHill( void );
	
	NewPlayerInfo*		FirstNewPlayerInfo( Lst::Search< NewPlayerInfo > &sh );
	NewPlayerInfo*		NextNewPlayerInfo( Lst::Search< NewPlayerInfo > &sh );
	NewPlayerInfo*		GetNewPlayerInfoByObjectID( unsigned short obj_id );
	void				DestroyNewPlayer( NewPlayerInfo* new_player );

	bool				SendFaceDataToServer( void );
	void				SendFaceDataToPlayer( PlayerInfo* player );

//	Modes and Options
/////////////////////////////

	char*				GetLevelName( bool get_created_park_name = true );
	char*				GetGameModeName( void );
	char*				GetPassword( void );
	int					GetSkillLevel( void );
	void				SetNetworkMode( NetworkMode mode );
    void				SetJoinMode( JoinMode mode );
	JoinMode			GetJoinMode( void );
	void				SetServerMode( bool on );
	HostMode			GetHostMode( void );
	void				SetHostMode( HostMode mode );
	bool				PlayerCollisionEnabled( void );
	bool				ShouldDisplayTeamScores( void );
	Prefs::Preferences*	GetNetworkPreferences( void );
	Prefs::Preferences*	GetTauntPreferences( void );
	uint32				GetNetworkLevelId( void );
	void				SetNetworkGameId( unsigned char id );
	unsigned char		GetNetworkGameId( void );
	const char*			GetNameFromArrayEntry( char* array_name, uint32 checksum );
	int					GetIntFromArrayEntry( char* array_name, uint32 checksum, uint32 field_checksum );
	bool				InNetMode( void );
	bool				InNetGame( void );
	bool				InLanMode( void );
	bool				InInternetMode( void );
	bool				OnServer( void );
	bool				OnClient( void );
	
	void				JoinServer( bool observe_only, unsigned long ip, unsigned short port, int index );
	void				CancelJoinServer( void );
	void				ReattemptJoin( Net::App* client );
	void				ReattemptJoinWithPassword( char* password );
	void				FindServersOnLAN( void );
	void				DisconnectFromServer( void );
	void				CleanupPlayers();
	
	void				RequestObserverMode( void );
	void				ObservePlayer( PlayerInfo* player );
	void				EnterObserverMode( void );
	
	void				ChooseNewServerPlayer( void );
	
	void				StartNetworkLobby( void );
	void				StartNetworkGame( void );
	bool				ShouldStopAtZero( void );
	
	void				ToggleProSet( char bit, uint32 param_id );
//	Script Functions
/////////////////////////////	
    
	static	bool		ScriptJoinTeam(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptSetNumTeams(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptGetNumTeams(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptGetNumPlayersOnTeam(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptGetMyTeam(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptInGroupRoom(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptConnectedToPeer(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptIsHost(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptIsAutoServing(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptChangeLevelPending(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool 		ScriptSetJoinMode(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool 		ScriptSetHostMode(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptChooseServer(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptRetrieveServerInfo(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptDescribeServer(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptFoundServers(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static 	bool		ScriptRemovePlayer(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static 	bool		ScriptAllPlayersAreReady(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptCreatePlayerOptions(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static 	bool		ScriptCancelRemovePlayer(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static 	bool		ScriptKickPlayer(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static 	bool		ScriptBanPlayer(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptStartNetworkGame(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptNetworkGamePending(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptEndNetworkGame(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptSendGameOverToObservers(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptSpawnCrown(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptStartCTFGame(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptEndCTFGame(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptTookFlag(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptCapturedFlag(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptRetrievedFlag(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptHasFlag(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptTeamFlagTaken(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptDisplayFlagBaseWarning(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptAlreadyGotMotd(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptDownloadMotd(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptAlreadySignedIn(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptSignOut(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptJoinServerComplete(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptEnteredNetworkGame(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptJustStartedNetGame(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptChooseAccount(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptCancelJoinServer(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptReattemptJoinServer(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptDropPendingPlayers(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static 	bool		ScriptToggleProSet(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static 	bool		ScriptResetProSetFlags(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptFCFSRequestStartGame(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptFCFSRequestChangeLevel(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptFCFSRequestToggleProSet(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptFCFSRequestToggleGoalSelection(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool 		ScriptHasSignedDisclaimer(Script::CScriptStructure *pParams, Script::CScript *pScript);

	static	bool		ScriptGetNumServersInLobby(Script::CScriptStructure *pParams, Script::CScript *pScript);

	static	bool		ScriptEnterSurveyorMode(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool		ScriptExitSurveyorMode(Script::CScriptStructure *pParams, Script::CScript *pScript);

	//void				RequestMatchmakerConnect( void );
	void				PostGame( void );
	void				SetLevel( uint32 level_id );
	uint32				GetLevelFromPreferences( void );
	int					GetFireballLevelFromPreferences( void );
	uint32				GetGameTypeFromPreferences( void );

	void				ToggleMetrics( void );
	void				ToggleScores( void );
	void				TogglePlayerNames( void );
	bool				ShouldDrawPlayerNames( void );

	void				CreateNetPanelMessage( 	bool broadcast, uint32 message_checksum, char* string_parm_1 = NULL,
												char* string_param_2 = NULL, Obj::CMovingObject* object = NULL, char* properties = NULL,
												bool important = false, int time = 2000 );
	bool				ReadyToPlay( void ) { return m_ready_to_play; }
	void				SetReadyToPlay( bool ready_to_play );
	void				RespondToReadyQuery( void );
		
	bool				ShouldSendScoreUpdates( void );
	void				SetLastScoreUpdateTime( int time );
	int					GetLastScoreUpdateTime( void );
	int					GetTeamScore( int team_id );
	int					GetPlayerScore( int obj_id );

	void				MarkReceivedFinalScores( bool received );
	bool				HaveReceivedFinalScores( void );

	void				SetCurrentLeader( PlayerInfo* player );
	PlayerInfo*			GetCurrentLeader( void );

	void				SetCurrentLeadingTeam( int team );
	int					GetCurrentLeadingTeam( void );

	void				AddSpawnedTriggerEvent( int node, int obj_id, uint32 script );
	void				ClearTriggerEventList( void );
	void				SetProSetFlags( char flags );
	
//	Server List Functions
/////////////////////////////	
	

	void				SetServerListState( ServerListState state );
	void				SetNextServerListState( ServerListState state );
	ServerListState		GetServerListState( void );
	ServerListState		GetNextServerListState( void );

	bool				StartServerList( void );
	void				StopServerList( void );
	void				SortPreviousKey( void );
	void				SortNextKey( void );
	void				SortServerList( void );
#ifdef __PLAT_XBOX__
	bool				ServerAlreadyInList( char* name, XNADDR* xbox_addr );
#else
	bool				ServerAlreadyInList( char* name, int ip );
#endif
	void				RefreshServerList( bool force_refresh = false );
	
	void				SetServerFocus( ServerInfo* server );
	void				ClearServerFocus( void );
	ServerInfo*			GetServerFocus( void );
	int					NumServersListed( void );
	void				AddServerToMenu( ServerInfo* server, int index );
	
	void				ClearServerList( bool refocus = true );
	void				FreeServerList( void );
	ServerInfo*			GetServerInfo( uint32 id );

	

	void				SendChatMessage( char* message );

	void				SetJoinState( JoinState state );
	JoinState			GetJoinState( void );

	void				SetObserverCommand( ObserverCommand command );
	ObserverCommand		GetObserverCommand( void );

	bool				ConnectToInternet( uint32 success_script, uint32 failure_script );
	void				DisconnectFromInternet( uint32 callback_script = 0 );
	void				RemoveModemStateTask( void );
	void				CancelConnectToInternet( void );

	void				RandomizeSkaterStartingPoints( void );
	void				SetSkaterStartingPoints( int* start_points );
	int					GetSkaterStartingPoint( int skater_id );

	void				LaunchQueuedScripts( void );

	Obj::CCrown*		GetCrown( void );
	void				SetCrownSpawnPoint( char spawn_point ); 
	void				CleanupObjects( void );

	void				SendEndOfRunMessage( void );
	bool				HaveSentEndOfRunMessage(  void );
	bool				GameIsOver( void );
	void				MarkGameOver( void );
	void				ClearGameOver( void );

	char*				GetNetThreadStack( void );
	void				SetNetThreadId( int net_thread_id );
	int					GetNetThreadId( void );

	bool				HasCheatingOccurred( void );
	void				ClearCheating( void );
	void				CheatingOccured( void );

	bool				UsingCreatedGoals( void );
	void				LoadGoals( uint32 level );
	uint32				GetGoalsLevel( void );
	char*				GetGoalsData( void );
	int					GetGoalsDataSize( void );
	void				SetGoalsData( char* data, uint32 level, int size );

	LobbyMan*			mpLobbyMan;
	ContentMan*			mpContentMan;
#ifdef __PLAT_XBOX__
	bool				m_XboxKeyRegistered;
	XNKID				m_XboxKeyId;	
	class AuthMan		*mpAuthMan;
	class BuddyMan		*mpBuddyMan;
	class VoiceMan		*mpVoiceMan;
#else
	BuddyMan* 			mpBuddyMan;
	StatsMan*			mpStatsMan;
#endif

private:
	Manager( void );
	~Manager( void );
	
	void				free_all_players( void );
	void				free_all_pending_players( void );
	bool				ok_to_join( int& reason, MsgConnectInfo* connect_msg, Net::Conn* conn );

	Net::Server*	m_server;
	Net::Client*	m_client[vMAX_LOCAL_CLIENTS];
	Net::Client* 	m_match_client;
    
	uint32			m_gamemode_id;
	uint32 			m_level_id;
	unsigned char	m_game_id;
	unsigned char	m_last_end_of_run_game_id;
	bool			m_game_over;
	bool			m_cheating_occurred;

	JoinMode		m_join_mode;
	HostMode		m_host_mode;
	
	bool			m_received_final_scores;

	bool			m_metrics_on;
	bool			m_scores_on;
	bool			m_draw_player_names;

	int				m_skater_starting_points[Mdl::Skate::vMAX_SKATERS];
	int				m_crown_spawn_point;

	Obj::CCrown* 	m_crown;

	static	Tsk::Task< Manager >::Code	s_modem_state_code;
	static	Tsk::Task< Manager >::Code 	s_timeout_connections_code;
	static	Tsk::Task< Manager >::Code 	s_render_metrics_code;
	static	Tsk::Task< Manager >::Code 	s_render_scores_code;
	static	Tsk::Task< Manager >::Code 	s_server_add_new_players_code;
	static	Tsk::Task< Manager >::Code 	s_client_add_new_players_code;
	static 	Tsk::Task< Manager >::Code	s_enter_chat_code;
	static	Tsk::Task< Manager >::Code	s_join_timeout_code;
	static 	Tsk::Task< Manager >::Code	s_server_list_state_code;
	static 	Tsk::Task< Manager >::Code	s_join_state_code;
	static	Tsk::Task< Manager >::Code	s_start_network_game_code;
	static	Tsk::Task< Manager >::Code	s_change_level_code;
    static  Tsk::Task< Manager >::Code	s_auto_refresh_code;
	static  Tsk::Task< Manager >::Code	s_auto_server_code;
	static	Tsk::Task< Manager >::Code	s_ctf_logic_code;
	static Net::MsgHandlerCode			s_handle_disconn_request;
	static Net::MsgHandlerCode			s_handle_find_server;
	static Net::MsgHandlerCode			s_handle_connection;
	static Net::MsgHandlerCode			s_handle_join_request;
	static Net::MsgHandlerCode			s_handle_join_accepted;
	static Net::MsgHandlerCode			s_handle_wait_n_seconds;
	static Net::MsgHandlerCode			s_handle_new_player;
	static Net::MsgHandlerCode			s_handle_game_info;
	static Net::MsgHandlerCode			s_handle_kill_flags;
	static Net::MsgHandlerCode			s_handle_select_goals;
	static Net::MsgHandlerCode			s_handle_change_level;
	static Net::MsgHandlerCode			s_handle_request_level;
	static Net::MsgHandlerCode			s_handle_new_level;
	static Net::MsgHandlerCode			s_handle_level_data;
	static Net::MsgHandlerCode			s_handle_rail_data;
	static Net::MsgHandlerCode			s_handle_goals_data;
	static Net::MsgHandlerCode			s_handle_client_proceed;
	static Net::MsgHandlerCode			s_handle_join_refused;
	static Net::MsgHandlerCode			s_handle_join_proceed;
	static Net::MsgHandlerCode			s_handle_observe;
	static Net::MsgHandlerCode			s_handle_server_response;
	static Net::MsgHandlerCode			s_handle_observe_proceed;
	static Net::MsgHandlerCode			s_handle_observe_refused;
	static Net::MsgHandlerCode			s_handle_ready_query;
	static Net::MsgHandlerCode			s_handle_ready_response;
	static Net::MsgHandlerCode			s_handle_player_info_ack_req;
	static Net::MsgHandlerCode			s_handle_player_info_ack;
	static Net::MsgHandlerCode			s_handle_new_king;
	static Net::MsgHandlerCode			s_handle_stole_flag;
	static Net::MsgHandlerCode			s_handle_took_flag;
	static Net::MsgHandlerCode			s_handle_captured_flag;
	static Net::MsgHandlerCode			s_handle_retrieved_flag;
	static Net::MsgHandlerCode			s_handle_player_restarted;
	static Net::MsgHandlerCode			s_handle_dropped_crown;
	static Net::MsgHandlerCode			s_handle_dropped_flag;
	static Net::MsgHandlerCode			s_handle_run_ended;
	static Net::MsgHandlerCode			s_handle_game_over;
	static Net::MsgHandlerCode			s_handle_end_game;
	static Net::MsgHandlerCode			s_handle_object_update;
	static Net::MsgHandlerCode			s_handle_panel_message;
	static Net::MsgHandlerCode			s_handle_auto_server_notification;
	static Net::MsgHandlerCode			s_handle_fcfs_assignment;
	static Net::MsgHandlerCode			s_handle_fcfs_request;
	static Net::MsgHandlerCode			s_handle_team_change_request;
	static Net::MsgHandlerCode			s_handle_team_change;
	static Net::MsgHandlerCode			s_handle_set_num_teams;
	static Net::MsgHandlerCode			s_handle_chat;
	static Net::MsgHandlerCode			s_handle_beat_goal;
	static Net::MsgHandlerCode			s_handle_beat_goal_relay;
	static Net::MsgHandlerCode			s_handle_started_goal;
	static Net::MsgHandlerCode			s_handle_started_goal_relay;
	static Net::MsgHandlerCode			s_handle_toggle_proset;
	static Net::MsgHandlerCode			s_handle_challenge;
	
	static Net::MsgHandlerCode			s_handle_challenge_response;
	static Net::MsgHandlerCode			s_handle_cheat_checksum_request;
	static Net::MsgHandlerCode			s_handle_cheat_checksum_response;
	static Net::MsgHandlerCode			s_handle_level_data_request;

	Tsk::Task< Manager >	*m_timeout_connections_task;
	Tsk::Task< Manager >	*m_render_metrics_task;
	Tsk::Task< Manager >	*m_render_scores_task;
	Tsk::Task< Manager >	*m_server_add_new_players_task;
	Tsk::Task< Manager >	*m_client_add_new_players_task;
	Tsk::Task< Manager >	*m_enter_chat_task;
	Tsk::Task< Manager >	*m_modem_state_task;
	Tsk::Task< Manager >	*m_join_timeout_task;
	Tsk::Task< Manager > 	*m_server_list_state_task;
	Tsk::Task< Manager > 	*m_join_state_task;
	Tsk::Task< Manager > 	*m_start_network_game_task;
	Tsk::Task< Manager > 	*m_change_level_task;
    Tsk::Task< Manager >	*m_auto_refresh_task;
	Tsk::Task< Manager >	*m_auto_server_task;
	Tsk::Task< Manager >	*m_ctf_logic_task;
	
	static	Inp::Handler< Manager >::Code 	s_observer_input_logic_code;      
	Inp::Handler< Manager >*	m_observer_input_handler;
	ObserverCommand			m_observer_command;


	Prefs::Preferences		m_network_preferences;
	// Ken: Taunt preferences used to be part of the network preferences, but then they were 
	// required to be saved out with the career file when saving to mem card, rather than with
	// the net settings (See TT6068) so I separated them out into a separate set of preferences.
	Prefs::Preferences		m_taunt_preferences;
    
	Lst::Head< PlayerInfo >		m_players;
	Lst::Head< NewPlayerInfo >	m_new_players;
	Lst::Head< TriggerEvent >	m_trigger_events;
	Lst::Head< ServerInfo > 	m_servers;
    Lst::Head< ServerInfo > 	m_temp_servers;
	int							m_sort_key;
	uint32						m_conn_refused_reason;
	
	//						These are currently only for clients since servers are currently
	//						the only ones sending out ready queries. Clients keep track of the latest
	//						received queries in these members
	int						m_latest_ready_query;
	bool					m_ready_to_play;
	int						m_last_score_update;
	Flags< int >			m_flags;
	Flags< char >			m_proset_flags;
	JoinState				m_join_state;
	int						m_join_ip;
	int						m_join_private_ip;
	int						m_join_port;
	int						m_last_modem_state;
	Tmr::Time				m_join_start_time;
	Tmr::Time				m_lobby_start_time;
	bool					m_game_pending;
	bool					m_waiting_for_game_to_start;

	ServerListState 		m_server_list_state;
	ServerListState 		m_next_server_list_state;
	Tmr::Time				m_server_list_wait_time;

	uint32					m_connection_success_script;
	uint32					m_connection_failure_script;

	PlayerInfo*				m_cam_player;	// the player that I'm watching with my camera
	PlayerInfo*				m_leader;
	int						m_leading_team;

	char					m_goals_data[vMAX_GOAL_SIZE];
	int						m_goals_data_size;
	uint32					m_goals_level;

#	ifndef __PLAT_XBOX__
	char					m_net_thread_stack[ vNET_THREAD_STACK_SIZE ]	__attribute__ ((aligned(16)));
#	else
#	pragma pack( 16 )
	char					m_net_thread_stack[ vNET_THREAD_STACK_SIZE ];
#	pragma pack()
#	endif // __PLAT_XBOX__
	int						m_net_thread_id;

	char					m_master_servers[vMAX_MASTER_SERVERS][16];
	
#ifdef __PLAT_NGPS__
	// Gamespy members
public:
	void					RemoveMessageOfTheDay( void );
	static bool				ScriptLoadNetConfigs(Script::CStruct *pParams, Script::CScript *pScript);
	static bool				ScriptNoNetConfigFiles(Script::CStruct *pParams, Script::CScript *pScript);
	static bool				ScriptFillNetConfigList(Script::CStruct *pParams, Script::CScript *pScript);
	static bool				ScriptChooseNetConfig(Script::CStruct *pParams, Script::CScript *pScript);
	static bool 			ScriptFillPlayerListMenu( Script::CStruct* pParams, Script::CScript* pScript );
	static bool 			ScriptPostGame( Script::CStruct* pParams, Script::CScript* pScript );
	static bool 			ScriptAuthenticateClient( Script::CStruct* pParams, Script::CScript* pScript );
	static bool 			ScriptWriteDNASBinary( Script::CStruct* pParams, Script::CScript* pScript );
	

private:
	char					m_motd[1024];	// Message of the day text
	GHTTPRequest 			m_ghttp_request;
	Tmr::Time				m_ghttp_start_time;
	bool					m_got_motd;
	//ezNetCnfCombination_t*	mp_combinations[6];
	ezNetCnfCombinationList_t m_combination_list;
	sceNetcnfifData_t		m_env_data[6];
    
	
	static void	s_create_game_callback( PEER peer, PEERBool success, PEERJoinResult result, RoomType roomType, 
											void * param );
	static void s_server_list_callback( PEER peer, PEERBool success, const char * name, SBServer server, PEERBool staging, int msg, int progress, void * param );
	static GHTTPBool MotdComplete( GHTTPRequest request, GHTTPResult result, char* buffer, int buffer_len, 
						  void* param );
	static GHTTPBool TrackUsageComplete( GHTTPRequest request, GHTTPResult result, char* buffer, int buffer_len, 
						  void* param );

	static	void			s_threaded_get_message_of_the_day( Manager* gamenet_man );
	static	void			s_threaded_track_usage( Manager* gamenet_man );
#endif

	DeclareSingletonClass( Manager );
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


class ScoreRank : public Lst::Node< ScoreRank >
{
public:
	ScoreRank( void )
		: Lst::Node< ScoreRank > ( this ) {}

	char	m_Name[32];
	bool	m_IsKing;
	bool	m_HasFlag;
	int		m_WhichFlags;
	int		m_ColorIndex;
	int		m_TotalScore;
	int		m_TeamId;
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

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace GameNet

#endif	// __GAMENET_GAMENET_H


