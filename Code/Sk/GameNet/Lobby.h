/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2002 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate4													**
**																			**
**	Module:			GameNet													**
**																			**
**	File name:		Lobby.h													**
**																			**
**	Created:		04/04/02	- spg										**
**																			**
*****************************************************************************/

#ifndef	__LOBBY_H
#define	__LOBBY_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <peer/peer.h>
#include <natneg/natneg.h>

namespace GameNet
{

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

enum
{
	NUMOBSERVERS_KEY = NUM_RESERVED_KEYS + 1,
	MAXOBSERVERS_KEY,
	SKILLLEVEL_KEY,
	STARTED_KEY,
	HOSTED_MODE_KEY,
	RANKED_KEY,
	RATING__KEY,
};
	

/*****************************************************************************
**							     Class Definitions							**
*****************************************************************************/

class LobbyPlayerInfo : public Lst::Node< LobbyPlayerInfo >
{
	public:
		LobbyPlayerInfo( void );

		char	m_Name[64];
		int		m_Profile;
		int		m_Rating;
		bool	m_GotInfo;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class LobbyMan
{
public:
				LobbyMan( void );
				~LobbyMan( void );

	void		Initialize( void );
	void		Shutdown( void );

	void		StartReportingGame( void );
	void		StopReportingGame( void );
	bool		ReportedGame( void );

	void		StartLobbyList( void );
	void		StopLobbyList( void );
	void		FillLobbyList( void );

	LobbyInfo*	GetLobbyInfo( int index );

	void		SetLobbyId( int id );
	int			GetLobbyId( void );
	void		SetLobbyNumServers( int num_servers );
	int			GetLobbyNumServers( void );
	void		SetLobbyMaxServers( int max_servers );
	int			GetLobbyMaxServers( void );
	char*		GetServerName( void );
	void		SetServerName( char* name );
	void		SetLobbyName( char* name );
	char*		GetLobbyName( void );
	void		SetOfficialLobby( bool official );
	bool		IsLobbyOfficial( void );

	PEER		GetPeer( void );
	bool		InGroupRoom( void );
	char*		PlayerName( char* lobby_name );
    
	static	bool	ScriptGetNumPlayersInLobby(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool	ScriptChooseLobby(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool	ScriptRejoinLobby(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool	ScriptLeaveLobby(Script::CScriptStructure *pParams, Script::CScript* pScript );
	static	bool	ScriptLobbyConnect(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool	ScriptLobbyDisconnect(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool	ScriptSendMessage( Script::CScriptStructure* pParams, Script::CScript* pScript );
	static	bool	ScriptSetQuietMode( Script::CScriptStructure* pParams, Script::CScript* pScript );
	static	bool	ScriptFillPlayerList( Script::CScriptStructure* pParams, Script::CScript* pScript );
	static	bool	ScriptFillLobbyProspectiveBuddyList( Script::CScriptStructure* pParams, Script::CScript* pScript );
	static	bool	ScriptCanHostGame( Script::CScriptStructure* pParams, Script::CScript* pScript );
	static	bool	ScriptStartNatNegotiation(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool	ScriptCancelNatNegotiation(Script::CScriptStructure *pParams, Script::CScript *pScript);

private:
	Lst::Head< LobbyInfo > 	m_lobbies;
	Lst::Head< LobbyPlayerInfo > m_players;

	int				m_lobby_id;
	int				m_lobby_max_servers;
	int				m_lobby_num_servers;
	char			m_lobby_name[128];
	char			m_server_name[128];
	bool			m_in_group_room;
	bool			m_official;
	
	bool			m_expecting_disconnect;
	bool			m_connection_in_progress;
	bool			m_reported_game;

	Spt::Ref		m_nat_count;
		
	PEER			m_peer;
	int				m_cookie;
	
	void			add_lobby_to_menu( LobbyInfo* lobby, int index );
	void			add_player_to_menu( LobbyPlayerInfo* player );
	void			add_buddy_to_menu( LobbyPlayerInfo* player );
	void			refresh_player_list( void );
	void			clear_player_list( void );
	void			fill_player_list( void );
	void			fill_prospective_buddy_list( void );

	Tsk::Task< LobbyMan >*		m_lobby_logic_task;
	Tsk::Task< LobbyMan >*		m_nat_negotiate_task;
	Tsk::Task< LobbyMan >*		m_process_gamespy_queries_task;
	static	Tsk::Task< LobbyMan >::Code   	s_lobby_logic_code;
	static	Tsk::Task< LobbyMan >::Code   	s_nat_negotiate_think_code;
	static	Tsk::Task< LobbyMan >::Code		s_process_gamespy_queries_code;

	static	void	s_enum_players_callback( PEER peer, PEERBool success, RoomType roomType, int index, const char * nick,  // The nick of the player.
												int flags, void * param );
	static 	void	s_nick_error_callback( PEER peer, int type, const char* nick, void *param );
	static	void	s_connect_callback( PEER peer, PEERBool success, void* param );

	static	void 	s_disconnected_callback( PEER peer, const char* reason, void* param );
	static	void 	s_room_message_callback( PEER peer, RoomType roomType, const char * nick, const char * message,
										MessageType messageType, void * param );
	static	void 	s_player_message_callback( PEER peer, const char * nick, const char * message, MessageType messageType,
												void * param );
	static 	void 	s_player_joined_callback( PEER peer, RoomType roomType, const char * nick, void * param );
	static	void 	s_player_info_callback( PEER peer, RoomType roomType, const char * nick, unsigned int IP, int profileID,  
                                                void * param );
	static	void	s_player_proile_id_callback( PEER peer, PEERBool success,  const char * nick, int profileID, void * param );
	static	void	s_player_left_callback( PEER peer, RoomType roomType, const char * nick, const char * reason, 
												void * param );
	static	void	s_new_player_list_callback( PEER peer, RoomType roomType, void * param );
	static	void	s_room_key_callback( PEER peer, RoomType roomType, const char * nick, 
										 const char * key, const char * value, void * param );
	static	void	s_nat_negotiate_callback( PEER peer, int cookie, void * param );
	static	void	s_key_list_callback( PEER peer, qr2_key_type type, qr2_keybuffer_t keyBuffer, 
									void * param );
	static	void	s_server_key_callback( PEER peer, int key, qr2_buffer_t buffer, void * param );
	static	void	s_player_key_callback( PEER peer, int key, int index, qr2_buffer_t buffer, void * param );
	static	int		s_key_count_callback( PEER peer, qr2_key_type type, void * param );
	static	void    s_add_error_callback( PEER peer, qr2_error_t error, char * errorString, void * param );
	static	void    s_public_address_callback( PEER peer, unsigned int ip, unsigned short port, 
											   void * param );
	static	void	s_public_address_callback( PEER peer, qr2_error_t error, char * errorString, void * param );

	static	void	s_group_rooms_callback( PEER peer, PEERBool success, int groupID, SBServer server,  
									const char * name, int numWaiting, int maxWaiting, int numGames, int numPlaying,
									void* param );
	static	void	s_join_room_callback( PEER peer, PEERBool success, PEERJoinResult result, RoomType roomType, 
										void * param );
	static	void 	s_get_room_keys_callback( PEER peer, PEERBool success, RoomType roomType, 
											const char * nick, int num, char ** keys, char ** values, 
											void * param );

	//static 	void 	s_lobby_list_callback( GServerList serverlist, int msg, void *instance, void *param1, void *param2 );
	//static	void	s_threaded_get_lobby_list( LobbyMan* lobby_man );

	static	void	s_threaded_peer_connect( LobbyMan* lobby_man );
	static	void	s_nat_negotiate_progress( NegotiateState state, void *userdata );
	static	void	s_nat_negotiate_complete( NegotiateResult result, SOCKET gamesocket, 
												struct sockaddr_in *remoteaddr, void *userdata);

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

}	// namespace GameNet

#endif	// __LOBBY_H