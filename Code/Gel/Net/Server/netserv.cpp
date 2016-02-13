/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			Net (OBJ) 												**
**																			**
**	File name:		net.cpp													**
**																			**
**	Created:		01/29/01	-	spg										**
**																			**
**	Description:	Network Manager code									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <core/defines.h>
#include <core/thread.h>

#include <sys/timer.h>
#include <gel/net/net.h>
#include <gel/net/server/netserv.h>

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/

namespace Net
{



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
**							   Private Functions							**
*****************************************************************************/

bool	Server::init( void )
{
	m_Timestamp = 0;
	m_connected = false;
	m_max_connections = 0;

	if( App::init() == false )
	{
		return false;
	}
    
#ifdef __PLAT_XBOX__
	//DWORD dwStatus = XNetGetEthernetLinkStatus();
	//if( dwStatus == 0 )
	//{
		//return false;
	//}

	// Get host XNADDR (asynchronous).
	while( XNetGetTitleXnAddr/*XNetGetDebugXnAddr*/( &m_XboxAddr ) == XNET_GET_XNADDR_PENDING )
	{
		// Can do other work/rendering here ...
	}

	// Create session key and register the session with the SNL.
	XNetCreateKey( &m_XboxKeyId, &m_XboxKey );
	XNetRegisterKey( &m_XboxKeyId, &m_XboxKey );	// != NO_ERROR	   
#endif

	m_Dispatcher.AddHandler( MSG_ID_CONNECTION_REQ, handle_connection_request, 
							 mHANDLE_FOREIGN | mHANDLE_LATE, 
							 NULL, 
							 HIGHEST_PRIORITY );
	m_Dispatcher.AddHandler( MSG_ID_TIMESTAMP, handle_timestamp, mHANDLE_LATE | mHANDLE_FOREIGN );
	m_Dispatcher.AddHandler( MSG_ID_DISCONN_REQ, handle_disconn_req,mHANDLE_LATE,
								NULL, HIGHEST_PRIORITY );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Server::deinit( void )
{
#ifdef USE_ALIASES
	int i;

	for( i = 0; i < vNUM_ALIASES; i++ )
	{
		if( m_alias_table[i] )
		{
			delete [] m_alias_table[i];
		}
	}
#endif

	App::deinit();

#ifdef __PLAT_XBOX__
	XNetUnregisterKey( &m_XboxKeyId );
#endif
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/* Server's send function                                         */
/*                                                                */
/******************************************************************/

void	Server::SendEnqueuedMessages( Conn* conn )
{
	int buf_len;
	MsgPacketStamp msg;
	int num_resends;
	MsgDesc timestamp_msg_desc;
	bool buffer_full;
	
	// Should we send this frame?
	if(	ShouldSendThisFrame( conn ) == false )
	{
		return;
	}

	buffer_full = false;
	while( !buffer_full && MessagesToSend( conn ) && !BandwidthExceeded( conn ))
	{    
		// Send Latency Tests/Responses if applicable
		if( ( conn->IsRemote()) &&
			( conn->IsForeign() == false ) &&
			( m_flags.TestMask( App::mDYNAMIC_RESEND )))
		{
			MsgDesc msg_desc;
			MsgTimestamp latency_msg;
			unsigned int cur_time;
	
			cur_time = m_Timestamp;
			latency_msg.m_Timestamp = cur_time;
			// send out a new latency test, keeping track of the time at which
			// we sent it
			if(	( conn->m_latency_test.m_SendTime == 0 ) ||
				( ( cur_time - conn->m_latency_test.m_SendTime ) > App::MAX_LATENCY ))
			{   
				// If we never got a response, simulate an increased latency
				if( conn->m_latency_test.m_SendTime > conn->m_latency_test.m_ReceiveTime )
				{
					unsigned int latency_value;
	
					latency_value = conn->GetAveLatency();
					latency_value += 100;
	
					if( latency_value > App::MAX_LATENCY )
					{
						latency_value = App::MAX_LATENCY;
					}
					conn->m_latency_test.InputLatencyValue( latency_value );
				}
#ifdef __PLAT_NGPS__
				SignalSema( m_send_semaphore_id );
#endif
				conn->m_latency_test.m_SendTime = cur_time;
	
				msg_desc.m_Data = &latency_msg;
				msg_desc.m_Length = sizeof( MsgTimestamp );
				msg_desc.m_Id = MSG_ID_PING_TEST;
				EnqueueMessage( conn->GetHandle(), &msg_desc );
#ifdef __PLAT_NGPS__
				WaitSema( m_send_semaphore_id );
#endif
			}
		}	
    
#ifdef __PLAT_NGPS__
		SignalSema( m_send_semaphore_id );
#endif
	
		msg.m_Packetstamp = (unsigned short) conn->m_latest_sent_packet_stamp;
		//msg.m_Handle = conn->GetHandle();
	
		timestamp_msg_desc.m_Data = &msg;
		timestamp_msg_desc.m_Length = sizeof( MsgPacketStamp );
		timestamp_msg_desc.m_Id = MSG_ID_TIMESTAMP;
		timestamp_msg_desc.m_Priority = HIGHEST_PRIORITY;
		EnqueueMessage( conn->GetHandle(), &timestamp_msg_desc );

#ifdef __PLAT_NGPS__
		WaitSema( m_send_semaphore_id );
#endif

		num_resends = conn->GetNumResends();

		if( !BuildMsgStream( conn, QUEUE_DEFAULT ))
		{
			buffer_full = true;
		}
		// First, use up our bandwidth with re-sends or else they might remain un-sent
		// indefinitely, causing a bad backup on the client if he's waiting on 
		// one particular sequence
		if( !BuildMsgStream( conn, QUEUE_SEQUENCED, true ))
		{
			buffer_full = true;
		}
		if( !BuildMsgStream( conn, QUEUE_IMPORTANT, true ))
		{
			buffer_full = true;
		}
		if( !BuildMsgStream( conn, QUEUE_SEQUENCED ))
		{
			buffer_full = true;
		}
		if( !BuildMsgStream( conn, QUEUE_IMPORTANT ))
		{
			buffer_full = true;
		}

		// If we had to resend some messages this frame, affect the latency for this client
		// so that we don't expect so much from their connection.
		// Also, reduce their estimated bandwidth
		if( conn->GetNumResends() > num_resends )
		{
			conn->m_latency_test.InputLatencyValue( App::MAX_LATENCY );
			conn->SetBandwidth( conn->GetBandwidth() - 500 );
			// Make sure that we'll still be able to send at least one large (max-size) packet.
			// Otherwise, we might get deadlocked.
			if( conn->GetBandwidth() < ( MAX_UDP_PACKET_SIZE + vUDP_PACKET_OVERHEAD ))
			{
				conn->SetBandwidth( MAX_UDP_PACKET_SIZE + vUDP_PACKET_OVERHEAD );
			}
		}

		buf_len = conn->m_write_ptr - conn->m_write_buffer;
		// If there is data to send
		if( ( buf_len > 0 ) &&
			( conn->IsRemote()))
		{
#ifdef NET_DEBUG_MESSAGES
			/*Dbg_Printf( "(%d) Conn %d: Sending pstamp %d at time %d\n", 
							m_FrameCounter, 
							conn->GetHandle(),
							conn->m_latest_sent_packet_stamp,
							m_Timestamp );*/
#endif																	
			if ( SendTo( conn->GetIP(), conn->GetPort(), conn->m_write_buffer, buf_len, 0 ))
			{
				m_TotalBytesOut += buf_len;
				conn->GetOutboundMetrics()->AddPacket( buf_len + vUDP_PACKET_OVERHEAD, m_Timestamp );
				conn->SetForceSendThisFrame( false );
				conn->m_last_send_time = m_Timestamp;
			}
			else
			{
				// If it didn't send the messages properly, flag them for resend next frame
				conn->FlagMessagesForResending( conn->m_latest_sent_packet_stamp );
				conn->m_write_ptr = conn->m_write_buffer;		
				break;
			}
			
			conn->m_write_ptr = conn->m_write_buffer;		
		}

		conn->m_latest_sent_packet_stamp++;
	}

	// If we're successfully sending data at/near the connection's estimated max bandwidth and there
	// are no problems, try ramping up the estimated bandwidth for this connection
	if( BandwidthExceeded( conn ) && ( conn->GetNumResends() == 0 ))
	{
		conn->SetBandwidth( conn->GetBandwidth() + 1000 );
	}
}

/******************************************************************/
/* Server's Receive function                                      */
/*                                                                */
/******************************************************************/

void	Server::ReceiveData( void )
{
	Conn *conn;
	struct sockaddr from_address;
	struct sockaddr_in *client_address;
	int num_bytes, actual_data_len;
    
#ifdef __PLAT_NGC__
	from_address.len = sizeof( sockaddr );
#else
	int addr_len = sizeof( from_address );
#endif

	// Local servers (i.e. non-network play) never really receive. Their data is automatically 
	// "received" when the local client writes its data into the server's receive buffer transparently
	if( IsLocal())
	{
		return;
	}
	
	do
	{
		num_bytes = recvfrom( m_socket, m_in_packet_buffer, 
			Manager::vMAX_PACKET_SIZE, 0, &from_address, &addr_len );
		if ( num_bytes < 0 )// == SOCKET_ERROR ) 
		{
#if defined( __PLAT_WN32__ ) || defined( __PLAT_XBOX__ )
			if( WSAGetLastError() != WSAEWOULDBLOCK )
#else
#ifdef __PLAT_NGPS__
			if( sn_errno( m_socket ) != EWOULDBLOCK )
#else
#ifdef __PLAT_NGC__
			if( num_bytes != SO_EWOULDBLOCK )
#endif
#endif
#endif
			{
#ifdef NET_DEBUG_MESSAGES
				Dbg_Printf( "Server Receive Error: " );
				ReportError();
#endif
			}

			break;
		}
		else
		{   
#ifdef __PLAT_NGPS__
			WaitSema( m_receive_semaphore_id );
#endif	// __PLAT_NGPS__
			m_TotalBytesIn += num_bytes;
			if( m_flags & App::mSECURE )
			{
				actual_data_len = num_bytes - sizeof( unsigned short );	// subtract size of CRC
			}
			else
			{
				actual_data_len = num_bytes;
			}
			
			client_address = (struct sockaddr_in*) &from_address;
			//Dbg_Printf( "**** Received data from client 0x%x : %d, addr_len : %d\n", client_address->sin_addr.s_addr,
						//client_address->sin_port, addr_len );
			conn = GetConnectionByAddress( client_address->sin_addr.s_addr, 
											ntohs( client_address->sin_port ));
			if( conn )
			{
				if( conn->IsValid() || (( m_flags & App::mSECURE ) == 0 ))
				{
					if( ( conn->m_read_ptr + actual_data_len ) < ( conn->m_read_buffer + Conn::vREAD_BUFFER_LENGTH ))
					{
						if( !validate_and_copy_stream( m_in_packet_buffer, conn->m_read_ptr, num_bytes ))
						{
	#ifdef __PLAT_NGPS__
							SignalSema( m_receive_semaphore_id );
	#endif	// __PLAT_NGPS__
							
							// If the game would like to handle this foreign data, allow it to do so now
							if( m_foreign_handler )
							{
								m_foreign_handler( conn->m_read_ptr, num_bytes, &from_address );
							}
							continue;
						}
                        
						conn->m_read_ptr += actual_data_len;
						conn->GetInboundMetrics()->AddPacket( num_bytes + vUDP_PACKET_OVERHEAD, m_Timestamp );
						conn->m_last_comm_time = Tmr::GetTime();
					}
				}
			}
			else
			{
				Conn* foreign_conn;
								
				// Foreign connection. Create temporary connection to process this foreign message
				foreign_conn = NewConnection( client_address->sin_addr.s_addr, 
											  ntohs( client_address->sin_port ), 
											  Conn::mREMOTE |
											  Conn::mFOREIGN );

				if( foreign_conn )
				{
					foreign_conn->m_app = this;
					if( !validate_and_copy_stream( m_in_packet_buffer, foreign_conn->m_read_ptr, num_bytes ))
					{
#ifdef __PLAT_NGPS__
						SignalSema( m_receive_semaphore_id );
#endif	// __PLAT_NGPS__
						// If the game would like to handle this foreign data, allow it to do so now
						if( m_foreign_handler )
						{
							m_foreign_handler( m_in_packet_buffer, num_bytes, &from_address );
						}
						delete foreign_conn;
						continue;
					}
					foreign_conn->m_read_ptr = foreign_conn->m_read_buffer + actual_data_len;
					foreign_conn->Invalidate();
				}
			}
#ifdef __PLAT_NGPS__
			SignalSema( m_receive_semaphore_id );
#endif	// __PLAT_NGPS__
		}       
	} while( num_bytes > 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef USE_ALIASES

void	Server::AllocateAliasTables( void )
{
	int i;

	
    
	Dbg_Assert( m_max_connections > 0 );

	for( i = 0; i < vNUM_ALIASES; i++ )
	{
		m_alias_table[i] = new AliasEntry[ m_max_connections ];
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Server::ClearAliasTables( void )
{
	

	int i, j;

	for( i = 0; i < m_max_connections; i++ )
	{
		for( j = 0; j < vNUM_ALIASES; j++ )
		{
			m_alias_table[j][i].m_Id = ~0;
			m_alias_table[j][i].m_Expiration = -1;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Server::ClearAliasTable( int handle )
{
	

	int i;

	for( i = 0; i < vNUM_ALIASES; i++ )
	{
		m_alias_table[i][handle].m_Id = 0;
		m_alias_table[i][handle].m_Expiration = -1;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

unsigned char	Server::GetObjectAlias( int handle, int obj_id, int cur_time )
{
	int hash, i;
	bool quit;

	hash = obj_id & 0xff;	// mask off all but lowest byte and use as a hash into the table
	hash %= vNUM_ALIASES;
	quit = false;
	i = hash;
	do
	{
		if( m_alias_table[i][handle].m_Id == obj_id )
		{
			if(	( m_alias_table[i][handle].m_Expiration == -1 ) ||
				( cur_time > m_alias_table[i][handle].m_Expiration ))
			{   
				return (unsigned char) vNO_ALIAS;
			}
			return (unsigned char) i;
		}
		
		i = ( i + 1 ) % vNUM_ALIASES;
		if( i == hash )	// complete revolution
		{
			break;
		}
	} while( !quit );
    
	return (unsigned char) vNO_ALIAS;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Server::SetObjectAlias( int handle, unsigned char alias, int obj_id, int expiration )
{
	m_alias_table[alias][handle].m_Id = obj_id;
	m_alias_table[alias][handle].m_Expiration = expiration;
}

#endif // USE_ALIASES

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Server::Server( int flags )
: App( flags )
{
	m_send_network_data_task = new Tsk::Task< App > ( send_network_data, *this, 
													  Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_SERVER_SEND_NETWORK_DATA );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}	// namespace Net
