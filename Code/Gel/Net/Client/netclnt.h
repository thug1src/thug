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
**	File name:		gel/netclnt.h											**
**																			**
**	Created: 		01/29/01	-	spg										**
**																			**
*****************************************************************************/

#ifndef __NETCLNT_H__
#define __NETCLNT_H__

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

class Client : public App
{
	friend class Manager;

public:
			Client( int flags = 0 );

	bool	ConnectToServer( int ip, unsigned short port );	
	void	ReceiveData( void );	
	void	SendEnqueuedMessages( Conn* conn );
	
#ifdef USE_ALIASES
	// Alias functions
	void	ClearAliasTable( void );
	unsigned char	GetObjectAlias( int obj_id, int cur_time );
	unsigned char	NewObjectAlias( int obj_id, int cur_time, bool expires = true );
	int		GetObjectId( unsigned char alias );
private:
	
	AliasEntry	m_alias_table[ vNUM_ALIASES ];
#endif
    
protected:
	bool	init( void );
	void	deinit( void );

	static	MsgHandlerCode	handle_timestamp;
	static	MsgHandlerCode	handle_connection_refusal;

	struct	sockaddr_in	m_server_address;

};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}	// namespace Net

#endif // __NETCLNT_H__