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
**	File name:		GameMsg.h												**
**																			**
**	Created by:		02/20/01	-	spg										**
**																			**
**	Description:	Game-Side Network Messages								**
**																			**
*****************************************************************************/

#ifndef __GAMENET_GAMEMSG_H
#define __GAMENET_GAMEMSG_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/math.h>
#include <gel/net/net.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace GameNet
{

						

enum
{
	MSG_ID_SKATER_INPUT	= Net::MSG_ID_USER,	//	= 32 : C->S The Client's Pad Input
	MSG_ID_YOUR_PLAYER_CREATE,				//	= 33 : S->C Create local skater
	MSG_ID_PLAYER_CREATE,					//	= 34 : S->C Each player's info
	MSG_ID_OBJ_UPDATE_STREAM,				//	= 35 : S->C Stream of object updates
	MSG_ID_NEW_ALIAS,						//	= 36 : C->S Alias suggestion
	MSG_ID_READY_QUERY,						//	= 37 : S->C Are you ready to receive more data?
	MSG_ID_READY_RESPONSE,					//	= 38 : C->S or S->C Ready query response
	MSG_ID_PAUSE,							//	= 39 : S->C Pause instruction
	MSG_ID_UNPAUSE,							//	= 40 : S->C UnPause instruction
	MSG_ID_PRIM_ANIM_START,					//	= 41 : S->C Play primary animation
	MSG_ID_ROTATE_SKATEBOARD,				//	= 42 : S->C Rotate skateboard
	MSG_ID_SET_WOBBLE_TARGET,				//	= 43 : S->C Set wobble target
	MSG_ID_FLIP_ANIM,						//	= 44 : S->C Flip the animation
	MSG_ID_PLAYER_QUIT,						//	= 45 : S->C Notification of a player quitting
	MSG_ID_HOST_REQ,						//  = 46 : GameServer->MatchMaker Game Host Request
	MSG_ID_HOST_PROCEED,					//  = 47 : MatchMaker->GameServer Proceed hosting
	MSG_ID_GAMELIST_REQ,					//  = 48 : GameClient->MatchMaker Game list request
	MSG_ID_GAMELIST_RESPONSE,				//  = 49 : MatchMaker->GameClient Game list response
	MSG_ID_HOST_QUIT,						//	= 50 : S->C Server is shutting down
	MSG_ID_START_INFO,						//	= 51 : S->C Startup Game Option
	MSG_ID_GAME_INFO,						//	= 52 : S->C Game Options
	MSG_ID_PLAY_SOUND,						//	= 53 : S->C Play Sound
	MSG_ID_PLAY_LOOPING_SOUND,				// 	= 54 : S->C Play Looping Sound
	MSG_ID_SPARKS_ON,						//	= 55 : S->C Sparks on
	MSG_ID_SPARKS_OFF,						//	= 56 : S->C Sparks off
	MSG_ID_BLOOD_ON,						//	= 57 : S->C Blood on
	MSG_ID_BLOOD_OFF,						//	= 58 : S->C Blood off
	MSG_ID_SCORE,							//	= 59 : S->C General Score Msg: See SCORE_MSG_ID...
	MSG_ID_SET_WOBBLE_DETAILS,				//	= 60 : S->C Set the wobbling parameters
	MSG_ID_SET_LOOPING_TYPE,				//	= 61 : S->C Set the desired looping type
	MSG_ID_SET_HIDE_ATOMIC,					//	= 62 : S->C Hide an arbitrary atomic on the skin model
	MSG_ID_PANEL_MESSAGE,					//	= 63 : S->C Display this panel message
	MSG_ID_SCORE_UPDATE,					//	= 64 : S->C Score update for other clients
	MSG_ID_SET_ANIM_SPEED,					//	= 65 : S->C Set animation speed
	MSG_ID_PROCEED_TO_PLAY,					//	= 66 : S->C Init phase complete: Next state
	MSG_ID_SKATER_COLLIDE_LOST,				//	= 67 : S->C You were involved in a skater collision and lost
	MSG_ID_SKATER_COLLIDE_WON,				//	= 68 : S->C You were involved in a skater collision and won
	MSG_ID_BAIL_DONE,						//	= 69 : C->S Bail is done
	MSG_ID_CHANGE_LEVEL,					//	= 70 : S->C Change level
	MSG_ID_JOIN_REFUSED,					//	= 71 : S->C Server refused to allow player to join
	MSG_ID_RUN_SCRIPT,						//  = 72 : C->C Run this script
	MSG_ID_SPAWN_AND_RUN_SCRIPT,			//  = 73 : C->C Spawn and Run this script
	MSG_ID_OBSERVE,							//	= 74 : C->S A request to enter observer mode
	MSG_ID_STEAL_MESSAGE,					//	= 75 : S->C A request to display a "steal message" in graffiti
	MSG_ID_CHAT,							//	= 76 : C->C Chat message
	MSG_ID_OBSERVE_PROCEED,					//  = 77 : S->C Proceed to observe
	MSG_ID_OBSERVE_REFUSED,					//	= 78 : S->C Server says you can't observe
	MSG_ID_JOIN_PROCEED,					//	= 79 : S->C Go ahead and attempt to join
	MSG_ID_JOIN_REQ,						//  = 80 : C->S Request to join a connected server
	MSG_ID_KICKED,							//	= 81 : S->C You've been kicked
	MSG_ID_LANDED_TRICK,					//	= 82 : S->C Landed a trick
	MSG_ID_PLAYER_INFO_ACK_REQ,				//	= 83 : S->C Have you received all the player info data?
	MSG_ID_PLAYER_INFO_ACK,					//	= 84 : C->S Yes I have received all the player info data
	MSG_ID_NEW_KING,						//	= 85 : S->C Crown the new king
	MSG_ID_LEFT_OUT,						//  = 86 : S->C Sorry. We're playing without you
	MSG_ID_MOVED_TO_RESTART,				//	= 87 : C->S Client has moved to a restart
	MSG_ID_KING_DROPPED_CROWN,				//	= 88 : S->C Spawn a new crown in the world
	MSG_ID_RUN_HAS_ENDED,					//	= 89 : C->S My run has ended
	MSG_ID_GAME_OVER,						//	= 90 : S->C Game is over
	MSG_ID_OBSERVER_LOG_TRICK_OBJ,			//	= 91 : S->C Special-case observer trick object report
	MSG_ID_OBSERVER_INIT_GRAFFITI_STATE,	//	= 92 : S->C Special-case observer, to synchronize trick objects with the current graffiti state
	MSG_ID_PRINTF,							//  = 93 : S->C Printf
	MSG_ID_AUTO_SERVER_NOTIFICATION,		//	= 94 : S->C Notify the client that he's on an auto-serving server
	MSG_ID_FCFS_ASSIGNMENT,					//	= 95 : S->C Notify the client that he's the acting host
	MSG_ID_FCFS_START_GAME,					//	= 96 : C->S Request to start a game from a FCFS
	MSG_ID_FCFS_BAN_PLAYER,					//	= 97 : C->S Request to ban a player from a FCFS
	MSG_ID_FCFS_CHANGE_LEVEL,				//	= 98 : C->S Request to change levels from a FCFS
	MSG_ID_FCFS_TOGGLE_PROSET,				//	= 99 : C->S Request to toggle a pro set
	MSG_ID_FCFS_TOGGLE_GOAL_SELECTION,		//	= 100 : C->S Request to toggle a goal selection
	MSG_ID_FCFS_END_GAME,					//	= 101 : C->S Request to end the current game
	MSG_ID_FCFS_SET_NUM_TEAMS,				//	= 102 : C->S Request to set the number of teams
	MSG_ID_REQUEST_CHANGE_TEAM,				//	= 103 : C->S Request to change teams
	MSG_ID_TEAM_CHANGE,						//	= 104 : S->C Team change update
	MSG_ID_SET_NUM_TEAMS,					//	= 105 : S->C Change in the number of teams
	MSG_ID_END_GAME,						//	= 106 : S->C Server has chosen to end the game
	MSG_ID_REQUEST_LEVEL,					//	= 107 : C->S Client requests level data
	MSG_ID_LEVEL_DATA,						//	= 108 : S->C Server sending client level data
	MSG_ID_SELECT_GOALS,					//	= 109 : S->C A list of goals to complete
	MSG_ID_BEAT_GOAL,						//	= 110 : C->S I beat a goal
	MSG_ID_STARTED_GOAL,					//	= 111 : C->S I started a goal
	MSG_ID_TOGGLE_PROSET,					//	= 112 : S->C Toggle this proset
	MSG_ID_TOOK_FLAG,						//	= 113 : S->C Player took a ctf flag
	MSG_ID_CAPTURED_FLAG,					//	= 114 : S->C Player captured a ctf flag
	MSG_ID_RETRIEVED_FLAG,					//	= 115 : S->C Player retrieved a ctf flag
	MSG_ID_STOLE_FLAG,						//	= 116 : S->C Player stole a ctf flag
	MSG_ID_PLAYER_DROPPED_FLAG,				//	= 117 : S->C Player dropped a flag
	MSG_ID_ROTATE_DISPLAY,					//	= 118 : S->C Rotate the skater by this much over time
	MSG_ID_CREATE_SPECIAL_ITEM,				//	= 119 : S->C Create a special item
	MSG_ID_DESTROY_SPECIAL_ITEM,			//	= 120 : S->C Destroy a special item
	MSG_ID_JOIN_ACCEPTED,					//	= 121 : S->C You're in - completely
	MSG_ID_KILL_TEAM_FLAGS,					//	= 122 : S->C Kill team flag (i.e. no more switching teams)
	MSG_ID_WAIT_N_SECONDS,					//	= 123 : S->C Prepare for a period of N seconds of no communication
	MSG_ID_CHALLENGE,						//	= 124 : S->C GameSpy Stats Challenge
	MSG_ID_CHALLENGE_RESPONSE,				//	= 125 : C->S GameSpy Stats Challenge Response
	MSG_ID_FACE_DATA,						//	= 126 : C->C Custom face data
	MSG_ID_TOGGLE_CHEAT,					//	= 127 : S->C Toggle a cheat on or off
	MSG_ID_CHEAT_CHECKSUM_REQUEST,			//	= 128 : S->C Server requesting client's cheat checksum
	MSG_ID_CHEAT_CHECKSUM_RESPONSE,			//	= 129 : C->S Client responding to server's request
	MSG_ID_CHEAT_LIST,						//	= 130 : S->C A list of all active cheats
	MSG_ID_SPAWN_PROJECTILE,				//  = 131 : C->C Spawn a projectile
	MSG_ID_SKATER_HIT_BY_PROJECTILE,		//	= 132 : S->C You were hit by a projectile
	MSG_ID_SKATER_PROJECTILE_HIT_TARGET,	//	= 133 : S->C You hit someone with a projectile
	MSG_ID_COMBO_REPORT,					//	= 134 : C->S Reporting combo
	MSG_ID_GOALS_DATA,						//	= 135 : S->C Server sending client goals data
	MSG_ID_RAIL_DATA,						//	= 136 : S->C Server sending edited rail data
	MSG_ID_ENTER_VEHICLE,					//  = 137 : C->S Client entering a vehicle
	MSG_ID_REQUEST_GOALS_DATA,				//	= 138 : C->S Re-requesting goals data
	MSG_ID_REQUEST_RAILS_DATA,				//	= 139 : C->S Re-requesting rails data
	MSG_ID_CLEAR_ROTATE_DISPLAY,			//	= 140 : S->C Clear any model rotations
};

enum
{
	SCORE_MSG_ID_LAND,						// = 0 : S->C Tabulate final score
	SCORE_MSG_ID_LOG_TRICK_OBJECT,			// = 1 : S->C Color an object
};

enum
{
	JOIN_REFUSED_ID_FULL,					// = 0 : S->C Server Full
	JOIN_REFUSED_ID_PW,						// = 1 : S->C Wrong Password
	JOIN_REFUSED_ID_VERSION,				// = 2 : S->C Wrong version
	JOIN_REFUSED_ID_IN_PROGRESS,			// = 3 : S->C Game is in progress
	JOIN_REFUSED_ID_BANNED,					// = 4 : S->C You've been banned
	JOIN_REFUSED_ID_FULL_OBSERVERS,			// = 5 : S->C Server has no more room for observers
};

enum
{
	vSEQ_GROUP_PLAYER_MSGS = 8,				// Messages about new players & quitting players
	vSEQ_GROUP_FACE_MSGS,					// Face download messages
};

enum
{
	vMAX_CONNECTIONS			= 20,
	vMAX_PLAYERS 				= 8,
	vMAX_SERVER_NAME_LEN 		= 15,
	vMAX_PLAYER_NAME_LEN 		= 15,
	vMAX_APPEARANCE_DATA_SIZE 	= 4096,
	vMAX_PASSWORD_LEN			= 9,
	vMAX_CHAT_MSG_LEN			= 127,
	vMAX_TRICK_OBJECTS_IN_LEVEL = 256,	// actually custom parks can have more than 256 trick objects, so we'll need to address that in THPS4
};

enum
{
#ifdef __PLAT_XBOX__
	vCONNECTION_TIMEOUT = 5000,		//  5 seconds in ms
#else
	vCONNECTION_TIMEOUT = 20000,	//	30 seconds in ms
#endif
	vNOT_READY_TIMEOUT	= 60000,	//	Clients are marked "not ready" when we tell them to load
									//	levels/skaters/etc.  If that time exceeds this timeout
									//	we should disconnect them
	vJOIN_TIMEOUT		= 60000,	//  20 seconds in ms to join a server
};

enum
{
	vHOST_PORT 		= 5150,
	vJOIN_PORT		= 5151,
};

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/*****************************************************************************
**							Messages										**
*****************************************************************************/

class MsgReady
{
public:
	unsigned int	m_Time;
};

class MsgPlayPrimaryAnim
{
public:
	//uint32			m_Time;
	uint32			m_Index;
	char			m_LoopingType;
	char			m_ObjId;
	unsigned short	m_StartTime;
	unsigned short	m_EndTime;
	unsigned short	m_BlendPeriod;
	unsigned short	m_Speed;
};

class MsgFlagMsg
{
public:
	char m_ObjId;
	char m_Team;
};

class MsgFlipAnim
{
public:
	//uint32			m_Time;
	char			m_ObjId;
	char			m_Flipped;
};

class MsgRotateSkateboard
{
public:
	//uint32			m_Time;
	char			m_ObjId;
	char			m_Rotate;
};

class MsgRotateDisplay
{
public:
	//uint32			m_Time;
	int 			m_Duration;
	int 			m_SinePower;
	short 			m_StartAngle;
	short 			m_DeltaAngle;
	char 			m_ObjId;
	bool			m_HoldOnLastAngle;
	char			m_Flags;
};

class MsgSetWobbleTarget
{
public:
	//uint32			m_Time;
	char			m_ObjId;
	unsigned char	m_Alpha;
};

class MsgSetWobbleDetails
{
public:
	enum
	{
		vWOBBLE_AMP_A,
		vWOBBLE_AMP_B,
		vWOBBLE_K1,
		vWOBBLE_K2,
		vSPAZFACTOR,

		mWOBBLE_AMPA_MASK	= 0x03,
		mWOBBLE_AMPB_MASK	= 0x04,
		mWOBBLE_K1_MASK		= 0x08,
		mWOBBLE_K2_MASK		= 0x10,
		mSPAZFACTOR_MASK	= 0x20,
	};

	//uint32			m_Time;
	char			m_WobbleDetails;
	char			m_ObjId;
};

class MsgSetLoopingType
{
public:
	//uint32			m_Time;
	char			m_ObjId;
	char			m_LoopingType;
};

class MsgSpecialItem
{
public:
	//uint32			m_Time;
	unsigned int	m_Params;
	uint32			m_Bone;
	char			m_ObjId;
	char			m_Index;
};

class MsgHideAtomic
{
public:
	//uint32			m_Time;
	uint32			m_AtomicName;
	char			m_ObjId;
	char			m_Hide;
};

class MsgSetAnimSpeed
{
	public:
		//uint32			m_Time;
		float			m_AnimSpeed;
		char			m_ObjId;
};

// Game options sent to joining players
class MsgStartInfo
{
public:
	char			m_CrownSpawnPoint;
	char			m_Broadband;	// boolean
	unsigned char	m_GameId;
	char			m_ProSetFlags;
	char			m_MaxPlayers;
	int				m_TimeLeft;
	unsigned long	m_GameMode;
	uint32			m_LevelId;
	uint32			m_TeamMode;
	
	int				m_TimeLimit;
	int				m_TargetScore;
	int				m_StartPoints[vMAX_PLAYERS];
};

class MsgInitGraffitiState
{
public:
	uint8			m_NumTrickObjects[vMAX_PLAYERS];
	uint8			m_TrickObjectStream[vMAX_TRICK_OBJECTS_IN_LEVEL];
};

class MsgGameInfo
{
public:
	unsigned long	m_GameMode;
	unsigned long	m_TimeLimit;
	unsigned long	m_TargetScore;
	int				m_FireballLevel;
	char			m_NumPlayers;
	char			m_AccumulateScore;
	char			m_DegradeScore;
	char			m_UseClock;
	char			m_StopAtZero;
	unsigned char	m_GameId;
	uint32			m_TeamMode;
	char			m_CrownSpawnPoint;
	int				m_StartPoints[vMAX_PLAYERS];
};

class MsgConnectInfo
{
public:
	int				m_Version;
	char			m_Observer;	// Does this player just want to observe?
	char			m_WillingToWait;	// Willing to sit out until the next lobby
	char			m_Password[ vMAX_PASSWORD_LEN + 1 ];
};

class MsgJoinInfo : public MsgConnectInfo
{
public:
	int				m_Profile;
	int				m_Rating;
	char			m_Broadband;	// boolean
	uint32			m_VehicleControlType;
	char			m_Name[ vMAX_PLAYER_NAME_LEN + 1 ];
	uint8			m_AppearanceData[vMAX_APPEARANCE_DATA_SIZE];	// stream information for player appearance
};

class MsgNewPlayer
{
public:
	int			m_Profile;
	int			m_Rating;
	int			m_Flags;
	int			m_Score;
	char		m_ObjId;	// object id for this player
	char		m_Team;
	uint32		m_VehicleControlType;
	char		m_Name[ vMAX_PLAYER_NAME_LEN + 1 ];
	uint8		m_AppearanceData[vMAX_APPEARANCE_DATA_SIZE]; // stream information for player appearance
};

class MsgPlayerQuit
{
public:
	char	m_ObjId;
	char	m_Reason;
};

class MsgPanelMessage
{
public:
	enum
	{
		vMAX_STRING_PARAM_LENGTH = vMAX_PLAYER_NAME_LEN,
	};

	uint32	m_PropsStructureCRC;
	uint32	m_StringId;
	int		m_Time;
	char	m_Parm1[vMAX_STRING_PARAM_LENGTH + 1];
	char	m_Parm2[vMAX_STRING_PARAM_LENGTH + 1];
};

class MsgNewAlias
{
public:
	unsigned char	m_Alias;
	char		m_ObjId;
	int			m_Expiration;
};

#ifdef __PLAT_XBOX__
class MsgFindServer : public Net::MsgTimestamp
{
public:
	BYTE m_Nonce[8]; 
};
#else
typedef Net::MsgTimestamp	MsgFindServer;
#endif

class MsgServerDescription
{
public:
#ifdef __PLAT_XBOX__
	BYTE	m_Nonce[8]; 
	XNKID	m_XboxKeyId;
	XNKEY	m_XboxKey;
	XNADDR	m_XboxAddr;
#endif
	char	m_NumPlayers;
	char	m_MaxPlayers;
	char	m_NumObservers;
	char	m_MaxObservers;
	char	m_Password;	// boolean
	char	m_GameStarted;
	char	m_HostMode;
	char	m_Ranked;
	char	m_Name[GameNet::vMAX_SERVER_NAME_LEN + 1];
	char	m_Level[32];
	char	m_Mode[32];
	char	m_SkillLevel;
	int		m_Timestamp;
	char	m_PlayerNames[vMAX_PLAYERS][ vMAX_PLAYER_NAME_LEN + 1 ];
};

class ObjectUpdate
{
public:
	short			m_ObjId;
	Mth::Vector		m_Pos;
	Mth::Matrix		m_Matrix;
};

class MsgScriptedSound
{
public:
	char			m_ObjId;
	uint32 			m_SoundChecksum;
	int 			m_Volume;
	int 			m_Pitch;
};

class MsgPlaySound
{
public:
	short 			m_Pos[3];
	char 			m_WhichArray;
	char			m_SurfaceFlag;
	char 			m_VolPercent;
	char			m_PitchPercent;
	char			m_SoundChoice;
};

class MsgObjMessage
{
public:
	char			m_ObjId;
};

class MsgPlayLoopingSound
{
public:
	char			m_ObjId;
	int 			m_WhichArray;
	int				m_SurfaceFlag;
	float 			m_VolPercent;
};

class MsgBlood : public MsgObjMessage
{
public:
	char			m_BodyPart;
	float			m_Size;
	float			m_Frequency;
	float			m_RandomRadius;
};

class MsgBloodOff : public MsgObjMessage
{
public:
	char			m_BodyPart;
};

class MsgActuator
{
public:
	char			m_ActuatorId;	// left/right
	char			m_Percent;		// between 1 and 100
	unsigned short	m_Duration;		// ms duration
};

class MsgEmbedded
{
public:
	char	m_SubMsgId;		// Sub-message
};

class MsgScoreLogTrickObject : public MsgEmbedded
{
public:
	enum
	{
		vMAX_PENDING_TRICKS = 512,	// how much data we can send across
	};
	
	unsigned char	m_GameId;
	int		m_OwnerId;
	int		m_Score;
	uint32	m_NumPendingTricks;
	uint32	m_PendingTrickBuffer[vMAX_PENDING_TRICKS];
};

class MsgObsScoreLogTrickObject
{
public:
	enum
	{
		vMAX_PENDING_TRICKS = 512,	// how much data we can send across
	};
	
	unsigned char	m_GameId;
	int		m_OwnerId;
	uint32	m_NumPendingTricks;
	uint32	m_PendingTrickBuffer[vMAX_PENDING_TRICKS];
};

class MsgScoreLanded
{
public:
	unsigned char	m_GameId;
	int		m_Score;
};

class MsgBeatGoal
{
public:
	unsigned char	m_GameId;
	uint32			m_GoalId;
};

class MsgBeatGoalRelay
{
public:
	char			m_ObjId;
	uint32			m_GoalId;
};

typedef MsgBeatGoal MsgStartedGoal;
typedef MsgBeatGoalRelay MsgStartedGoalRelay;

class MsgScoreUpdate
{
public:
	int		m_TimeLeft;		// Time left in the game
	Flags< int > m_Cheats;
	char	m_Final;		// Final scores?
	char	m_NumScores;
	char	m_ScoreData[256];
};

class MsgSpawnAndRunScript
{
public:
	uint32	m_ScriptName;
	int		m_ObjID;
	int		m_Node;
	char	m_Permanent;	// is this an event that the server should queue up for new joiners?
};

class MsgToggleProSet
{
public:
	uint32	m_ParamId;
	char	m_Bit;
};

class MsgToggleCheat
{
public:
	uint32	m_Cheat;
	char	m_On;
};

class MsgEnabledCheats
{
public:
	enum
	{
		vMAX_CHEATS = 32,
	};
	int		m_NumCheats;
	uint32	m_Cheats[vMAX_CHEATS];
};

class MsgStealMessage
{
public:
	unsigned char	m_GameId;
	int		m_NewOwner;		// new owner of the trick object
	int		m_OldOwner;		// old owner of the trick object
};

class MsgRunScript
{
public:
	enum
	{
		vMAX_SCRIPT_PARAMS_LEN = 1024
	};
	uint32	m_ScriptName;
	int		m_ObjID;
	char	m_Data[vMAX_SCRIPT_PARAMS_LEN];
};

class MsgChangeLevel
{
public:
	int		m_Level;
	char	m_ShowWarning;
};

class MsgChangeTeam
{
public:
	char	m_ObjID;
	char	m_Team;
};

class MsgByteInfo		// Generic one-byte message
{
public:
	char	m_Data;
};

class MsgIntInfo		// Generic four-byte message
{
	public:
		int		m_Data;
};

class MsgChat
{
	public:
		char	m_ObjId;
		char	m_Name[vMAX_PLAYER_NAME_LEN + 1];
		char	m_ChatMsg[vMAX_CHAT_MSG_LEN + 1];
};

class MsgRequestLevel
{
public:
	uint32		m_LevelId;
	char		m_Source;	// Was this request the result of a "StartInfo" message or a "Change Level" message
};

class MsgRemovePlayerRequest
{
public:
	char	m_Ban;	// Should we ban? If not, just kick
	char	m_Index;
	char	m_Name[vMAX_PLAYER_NAME_LEN + 1];
};

class MsgStartGameRequest
{
public:
	uint32	m_GameMode;
	uint32	m_TimeLimit;
	uint32	m_SkillLevel;
	uint32	m_FireballLevel;
	uint32	m_PlayerCollision;
	uint32	m_FriendlyFire;
	uint32	m_TeamMode;
	uint32	m_TargetScore;
	uint32	m_StopAtZero;
	uint32	m_CTFType;
};

class MsgCheatChecksum
{
public:
	uint32	m_ServerChecksum;
	uint32	m_ClientChecksum;
};

class MsgProjectile
{
public:
	uint32		m_Latency;
	Mth::Vector	m_Pos;
	Mth::Vector	m_Vel;
	uint32		m_Type;
	int			m_Radius;
	float		m_Scale;
	char		m_Id;
};

class MsgProjectileHit
{
public:
	char	m_Id;
	int		m_Damage;

};

class MsgCollideLost
{
public:
	char		m_Id;
	Mth::Vector	m_Offset;
};

class MsgEnterVehicle
{
public:
	char		m_Id;
	uint32		m_ControlType;
};

class MsgProceed
{
public:
	int			m_PublicIP;
	int			m_PrivateIP;
	int			m_Port;
	char		m_ServerName[GameNet::vMAX_SERVER_NAME_LEN + 1];
	char		m_MaxPlayers;
	char		m_Broadband;
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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace GameNet

#endif	// __GAMENET_GAMEMSG_H


