/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			NET  (Net)												**
**																			**
**	File name:		gel/net.h												**
**																			**
**	Created: 		01/29/01	-	spg										**
**																			**
*****************************************************************************/

#ifndef __NETSERV_H__
#define __NETSERV_H__

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gel/net/net.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Net
{

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class Server : public App
{
	friend class Manager;	
public:
			Server( int flags = 0 );

	void	ReceiveData( void );	
	void	SendEnqueuedMessages( Conn* conn );
		
#ifdef USE_ALIASES
	// Alias Functionality
	void	AllocateAliasTables( void );
	void	ClearAliasTables( void );
	void	ClearAliasTable( int handle );
	unsigned char	GetObjectAlias( int handle, int obj_id, int cur_time );
	void	SetObjectAlias( int handle, unsigned char alias, int obj_id, int expiration );

private:
	AliasEntry*	m_alias_table[vNUM_ALIASES];
#endif

private:
	bool	init( void );
	void	deinit( void );
    
	static	MsgHandlerCode	handle_timestamp;		
	static	MsgHandlerCode	handle_disconn_req;

	AliasEntry*	m_alias_table[vNUM_ALIASES];
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}	// namespace Net

#endif // __NETSERV_H__