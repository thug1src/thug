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
**	Description:	XBox Buddy List Code 									**
**																			**
*****************************************************************************/

#ifndef __GAMENET_XBOX_P_BUDDY_H
#define __GAMENET_XBOX_P_BUDDY_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/dynamictable.h>
#include <Xbox.h>
#include <XOnline.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace GameNet
{

class BuddyMan
{
public:
	typedef enum
	{
		vSTATE_FILL_BUDDY_LIST,
		vSTATE_UPDATE,
		vSTATE_SUCCESS,
		vSTATE_CANCEL,
		vSTATE_ERROR,
		vSTATE_WAIT,
	} BuddyState;

	BuddyMan( void );

	void	SpawnBuddyList( int user_index );

private:

	void	update_buddy_list( void );
	void	fill_buddy_list( void );

	int		m_current_user;
	int		m_num_friends;
	int		m_state;
	int		m_next_state;
	XONLINETASK_HANDLE	mh_friends_task;
	XONLINETASK_HANDLE	mh_friends_enum_task;
	Lst::DynamicTable< XONLINE_USER > m_potential_friends_list;
	Lst::DynamicTable< XONLINE_FRIEND > m_friends_list;

	static	Tsk::Task< BuddyMan >::Code  s_buddy_state_code; 
	Tsk::Task< BuddyMan >*		mp_buddy_state_task;
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

#endif	// __GAMENET_XBOX_P_BUDDY_H


