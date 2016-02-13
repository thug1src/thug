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
**	File name:		p_buddy.cpp												**
**																			**
**	Created by:		02/28/02	-	SPG										**
**																			**
**	Description:	XBox Buddy List Code 									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <gel/mainloop.h>
#include <GameNet/GameNet.h>
#include <GameNet/Xbox/p_buddy.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace GameNet
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

void	BuddyMan::s_buddy_state_code( const Tsk::Task< BuddyMan>& task )
{
	BuddyMan& man = task.GetData();

	switch( man.m_state )
	{
		case vSTATE_UPDATE:
			man.update_buddy_list();
			break;
		case vSTATE_FILL_BUDDY_LIST:
			man.fill_buddy_list();
			man.m_state = vSTATE_WAIT;
			break;
		case vSTATE_SUCCESS:
			break;
		case vSTATE_CANCEL:
			break;
		case vSTATE_ERROR:
			break;
		case vSTATE_WAIT:
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BuddyMan::fill_buddy_list( void )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BuddyMan::update_buddy_list( void )
{
#	if 0
	HRESULT hr = XOnlineTaskContinue( mh_friends_enum_task );
    // Enumeration failed
    if( FAILED( hr ) )
    {
        //m_UI.SetErrorStr( L"Friend enumeration failed. Error %x", hr );
        //Reset( TRUE );
        return;
    }
    
    // Update generic status
//    if( XOnlineNotificationIsPending( m_current_user, XONLINE_NOTIFICATION_TYPE_FRIENDREQUEST ))
	{
        //SetStatus( L"You have received a friend request" );
	}
//    else if( XOnlineNotificationIsPending( m_current_user, XONLINE_NOTIFICATION_TYPE_GAMEINVITE ))
	{
        //SetStatus( L"You have received a game invitation" );
	}
//    else
	{
        //SetStatus( L"Friend list refreshed" );
	}
    
	XONLINE_FRIEND	friend_list[MAX_FRIENDS];
	int	i, num_friends;

	num_friends = XOnlineFriendsGetLatest( m_current_user, MAX_FRIENDS, friend_list );
	if( num_friends != m_num_friends )
	{
		m_num_friends = num_friends;
		Lst::DynamicTableDestroyer<XONLINE_FRIEND> destroyer( &m_friends_list );
		destroyer.DeleteTableContents();

		for( i = 0; i < num_friends; i++ )
		{
			XONLINE_FRIEND* new_friend;

			new_friend = new XONLINE_FRIEND;
			memcpy( new_friend, &friend_list[i], sizeof( XONLINE_FRIEND ));
			m_friends_list.Add( new_friend );
		}
	}    
#	endif
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

void	BuddyMan::SpawnBuddyList( int user_index )
{
#	if 0
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();

	m_current_user = user_index;

	// Standard init
    HRESULT hr = XOnlineFriendsStartup( NULL, &mh_friends_task );
    if( FAILED(hr) )
    {
        //m_UI.SetErrorStr( L"Friends failed to initialize. Error %x", hr );
        //Reset( TRUE );
    }
    
    // Query server for latest list of friends
    hr = XOnlineFriendsEnumerate( m_current_user, NULL, &mh_friends_enum_task );
    if( FAILED(hr) )
    {
        //m_UI.SetErrorStr( L"Friend enum failed to initialize. Error %x", hr );
        //Reset( TRUE );
    }

	mp_buddy_state_task = new Tsk::Task< BuddyMan > ( s_buddy_state_code, *this,
										Tsk::BaseTask::Node::vNORMAL_PRIORITY );
	mlp_man->AddLogicTask( *mp_buddy_state_task );	
	m_state = vSTATE_UPDATE;	
#	endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

BuddyMan::BuddyMan( void )
: m_potential_friends_list( 1 ), m_friends_list( MAX_FRIENDS ), m_num_friends( 1 )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace GameNet




