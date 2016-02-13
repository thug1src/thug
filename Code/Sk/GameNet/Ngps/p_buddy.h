/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2002 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate4													**
**																			**
**	Module:			GameNet			 										**
**																			**
**	File name:		p_buddy.h												**
**																			**
**	Created by:		02/28/02	-	SPG										**
**																			**
**	Description:	PS2 Buddy List Code 									**
**																			**
*****************************************************************************/

#ifndef __GAMENET_NGPS_P_BUDDY_H
#define __GAMENET_NGPS_P_BUDDY_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <gp/gp.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace GameNet
{

enum
{
	vCOMMAND_NONE,
	vCOMMAND_CONNECT,
	vCOMMAND_LOGIN
};

enum
{
	vSTATE_WAIT,
	vSTATE_CREATING_PROFILE,
	vSTATE_CONNECTING,
	vSTATE_BEGIN_LOGIN,
	vSTATE_LOGGED_IN
};

enum
{
	vMAX_BUDDIES = 30
};

class BuddyInfo : public Lst::Node< BuddyInfo >
{
public:
	BuddyInfo( void );

	char	m_Nick[GP_NICK_LEN];
	int		m_Profile;
	int		m_Status;
	bool	m_Pending;
	int		m_JoinIP;
	int		m_JoinPrivateIP;
	int		m_JoinPort;
	char	m_Location[GP_LOCATION_STRING_LEN];
	char	m_StatusString[GP_STATUS_STRING_LEN];
};

class BuddyMan
{
public:
	BuddyMan( void );
	~BuddyMan( void );

	void	Connect( void );
	void	CreateProfile( void );
	void	Disconnect( void );
	void	Connected( GPProfile profile );
	bool	IsLoggedIn( void );
	bool	IsAlreadyMyBuddy( int profile );
	int		NumBuddies( void );

	int		GetProfile( void );
	void	SetStatusAndLocation( int status, char* status_string, char* location );

	static	bool		ScriptSetUniqueId(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptProfileLoggedIn(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptCreateProfile(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptProfileLogOff(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptProfileLogIn(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptFillBuddyList(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptFillProspectiveBuddyList(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptAddBuddy(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptCancelAddBuddy(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptRemoveBuddy(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptJoinBuddy(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptHasBuddies(Script::CStruct *pParams, Script::CScript *pScript);
	static 	bool		ScriptBuddyListFull(Script::CStruct *pParams, Script::CScript *pScript);
	static 	bool		ScriptSetLobbyStatus(Script::CStruct *pParams, Script::CScript *pScript);
    
private:
	GPConnection	m_gp_conn;
	GPProfile		m_profile;
	int				m_state;

	static	void s_connect_callback( GPConnection* connection, void* arg, void* param );

	static	void s_error_callback( GPConnection* connection, void* arg, void* param );
	static 	void s_buddy_request( GPConnection* connection, void* arg, void* param );
	static	void s_buddy_status( GPConnection* connection, void* arg, void* param );
	static	void s_buddy_message( GPConnection* connection, void* arg, void* param );
	static	void s_game_invite( GPConnection* connection, void* arg, void* param );
	static	void s_transfer_callback( GPConnection* connection, void* arg, void* param );

	static	void s_threaded_buddy_connect( BuddyMan* buddy_man );
	static	void s_threaded_buddy_create( BuddyMan* buddy_man );

	void		add_player_to_menu( BuddyInfo* buddy, bool allow_join, bool allow_remove );
	void		fill_buddy_list( bool clear_list, bool allow_join, bool allow_remove );
	void		fill_prospective_buddy_list( void );
	void		add_buddy( char *name, int profile );
	void		cancel_add_buddy( void );
	void		remove_buddy( int profile );

	Tsk::Task< BuddyMan >*					m_buddy_logic_task;
	static	Tsk::Task< BuddyMan >::Code   	s_buddy_logic_code;

	bool	m_logged_in;
	Lst::Head< BuddyInfo > m_buddies;
};





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

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace GameNet

#endif	// __GAMENET_NGPS_P_BUDDY_H


