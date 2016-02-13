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
**	File name:		p_auth.h												**
**																			**
**	Created by:		02/28/02	-	SPG										**
**																			**
**	Description:	XBox Online Authorization Code							**
**																			**
*****************************************************************************/

#ifndef __GAMENET_XBOX_P_AUTH_H
#define __GAMENET_XBOX_P_AUTH_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/dynamictable.h>
#include <sys/sioman.h>
#include <Xbox.h>
#include <XOnline.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace GameNet
{

// Number of services to authenticate
const DWORD vNUM_SERVICES = 2;						
//typedef std::vector< XONLINE_USER > XBUserList;
//typedef std::vector< XONLINE_SERVICE_INFO > ServiceInfoList;

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class PinEntry
{
public:
	void	BeginInput( int controller );    
	void	EndInput( void );
	void	ClearInput( void );
	bool	Complete( void );
	BYTE*	GetPin( void );

private:
//	XPININPUTHANDLE m_pin_input;                  // PIN input handle
	BYTE		m_frame_entry;				// PIN entry this frame
//    BYTE		m_pin[ XONLINE_PIN_LENGTH ];  // Current PIN
    int			m_pin_length;                // number of buttons entered
	bool		m_got_entry;
	bool		m_cancelled;
	bool		m_cleared;
	
	static	Inp::Handler< PinEntry >::Code 	s_pin_entry_input_logic_code;      
	Inp::Handler< PinEntry >*	mp_pin_entry_input_handler;

	static	Tsk::Task< PinEntry >::Code  s_pin_entry_logic_code; 
	Tsk::Task< PinEntry >*		mp_pin_entry_logic_task;
	
};

class AuthMan
{
public:
	typedef enum
	{
		vSTATE_FILL_ACCOUNT_LIST,
		vSTATE_CREATE_ACCOUNT,
		vSTATE_SELECT_ACCOUNT,
		vSTATE_GET_PIN,
		vSTATE_PIN_COMPLETE,
		vSTATE_PIN_CLEARED,
		vSTATE_PIN_CANCELLED,
		vSTATE_LOG_ON,
		vSTATE_LOGGING_ON,		
		vSTATE_LOGGED_IN,
		vSTATE_SUCCESS,
		vSTATE_CANCEL,
		vSTATE_ERROR,
		vSTATE_WAIT,
	} LogonState;

	enum
	{
		vERROR_WRONG_PIN,
		vERROR_GENERAL_LOGIN,
		vERROR_LOGIN_FAILED,
		vERROR_SERVICE_DOWN,
	};

	AuthMan( void );

	void	UserLogon( void );
	void	CancelLogon( void );
	void	SelectAccount( int index );
	void	PinAttempt( BYTE* pin );
	void	SetState( LogonState state );
	void	SignOut( void );
	bool	SignedIn( void );
	
private:
	int		gather_user_list( void );
	void	fill_user_list( void );
	void	create_account( void );
	void	logon( void );
	void	check_logon_progress( void );
	void	pump_logon_task( void );
	bool	xbox_has_voice_device( void ); 

	Lst::DynamicTable< XONLINE_USER > m_user_list;
	LogonState	m_state;
	LogonState	m_next_state;
	int			m_error;
	bool		m_signed_in;
	int			m_chosen_account;
	PinEntry	m_pin_entry;	
	XONLINETASK_HANDLE	mh_online_task;
	DWORD		mp_services[ vNUM_SERVICES ]; // List of desired services
	Lst::DynamicTable< XONLINE_SERVICE_INFO > m_service_info_list;

	static	Tsk::Task< AuthMan >::Code  s_logon_state_code; 
	Tsk::Task< AuthMan >*		mp_logon_state_task;
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

#endif	// __GAMENET_XBOX_P_AUTH_H


