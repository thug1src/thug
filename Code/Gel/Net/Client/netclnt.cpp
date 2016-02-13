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
**	File name:		netclnt.cpp												**
**																			**
**	Created:		01/29/01	-	spg										**
**																			**
**	Description:	Network client code										**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <core/defines.h>

#include <sys/timer.h>

#include <gel/net/net.h>
#include <gel/net/client/netclnt.h>

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

bool	Client::init( void )
{
	

#ifdef	__PLAT_NGPS__											  
	Dbg_MsgAssert(Mem::SameContext(this,Mem::Manager::sHandle().NetworkHeap()),("Client not on network heap"));	
#endif		//	__PLAT_NGPS__											  


	m_connected = false;
	m_Timestamp = 0;
	if( App::init() == false )
	{
		return false;
	}
	
	m_Dispatcher.AddHandler( MSG_ID_CONNECTION_ACCEPTED, handle_connection_accepted, 
							 mHANDLE_FOREIGN | mHANDLE_LATE );
	m_Dispatcher.AddHandler( MSG_ID_CONNECTION_REFUSED, handle_connection_refusal, mHANDLE_LATE );
	m_Dispatcher.AddHandler( MSG_ID_CONNECTION_TERMINATED, handle_connection_refusal, mHANDLE_LATE );
	m_Dispatcher.AddHandler( MSG_ID_TIMESTAMP, handle_timestamp, mHANDLE_LATE | mHANDLE_FOREIGN );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Client::deinit( void )
{
	

	App::deinit();
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/* Connect (socket-level) to a server at a given IP/Port          */
/*                                                                */
/******************************************************************/

bool Client::ConnectToServer( int ip, unsigned short port )
{
	int result;
	bool connected;
	
	memset( &m_server_address, 0, sizeof(m_server_address));
	m_server_address.sin_family = AF_INET;
	m_server_address.sin_port = htons( port );	
	m_server_address.sin_addr.s_addr = ip;
#ifdef __PLAT_NGC__
	m_server_address.len = sizeof( sockaddr_in );
#else
	memset ( &m_server_address.sin_zero, 0, 8 );  
#endif

	Dbg_Printf( "%s Connecting to server\n", GetName());
	connected = false;
	do
	{
		if(( result = connect( m_socket, (struct sockaddr *)&m_server_address, 
					sizeof( m_server_address ))) == SOCKET_ERROR )
		{
#if defined( __PLAT_WN32__ ) || defined( __PLAT_XBOX__ )
			if( WSAGetLastError() != WSAEWOULDBLOCK )
#else
#ifdef __PLAT_NGPS__
			if( sn_errno( m_socket ) != EWOULDBLOCK )
#endif
#endif
			{
				ReportError();
				return false;
			}
		}
		else
		{
			connected = true;
		}
	} while( !connected );

	// "Connecting" to INADDR_ANY disassociates our socket from all addresses
	if( ip == INADDR_ANY )
	{   
		m_connected = false;
	}
	else
	{   
		m_connected = true;
	}
	return true;
}

/******************************************************************/
/* Client's send function.                                        */
/*                                                                */
/******************************************************************/

void	Client::SendEnqueuedMessages( Conn* conn )
{
	int buf_len;
	MsgPacketStamp msg;
	bool buffer_full;
    
	// Should we send this frame?
	if( ShouldSendThisFrame( conn ) == false )
	{
		return;
	}

	buffer_full = false;
    while( !buffer_full && MessagesToSend( conn ) && !BandwidthExceeded( conn ))
	{
		// Tack on a timestamp (for acking) if we are sending important
		// messages in this packet
		if( ImportantMessagesToSend( conn ))
		{
			MsgDesc msg_desc;
#ifdef __PLAT_NGPS__
			SignalSema( m_send_semaphore_id );
#endif  
			msg.m_Packetstamp = (unsigned short) conn->m_latest_sent_packet_stamp;
            //msg.m_Handle = conn->GetHandle();
			
			msg_desc.m_Data = &msg;
			msg_desc.m_Length = sizeof( MsgPacketStamp );
			msg_desc.m_Id = MSG_ID_TIMESTAMP;
			msg_desc.m_Priority = HIGHEST_PRIORITY;
			EnqueueMessage( conn->GetHandle(), &msg_desc );
#ifdef __PLAT_NGPS__
			WaitSema( m_send_semaphore_id );
#endif
		}
	
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
	
				conn->m_latency_test.m_SendTime = cur_time;
	
#ifdef __PLAT_NGPS__
				SignalSema( m_send_semaphore_id );
#endif  
				msg_desc.m_Data = &latency_msg;
				msg_desc.m_Id = MSG_ID_PING_TEST;
				msg_desc.m_Length = sizeof( MsgTimestamp );
				EnqueueMessage( conn->GetHandle(), &msg_desc ); 
	
#ifdef __PLAT_NGPS__
				WaitSema( m_send_semaphore_id );
#endif
			}			
		}
		
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
		
		buf_len = conn->m_write_ptr - conn->m_write_buffer;
		// If there is data to send
		if(	( buf_len > 0 ) &&
			( conn->IsRemote()))
		{
			if( m_connected )
			{           
				if( Send( conn->m_write_buffer, buf_len, 0 ))
				{
					m_TotalBytesOut += buf_len;
					conn->GetOutboundMetrics()->AddPacket( buf_len + vUDP_PACKET_OVERHEAD, m_Timestamp );
					conn->m_last_send_time = m_Timestamp;
					conn->SetForceSendThisFrame( false );
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
			else
			{
				if( SendTo( conn->GetIP(), conn->GetPort(), conn->m_write_buffer, buf_len, 0 ))
				{
					m_TotalBytesOut += buf_len;
					conn->GetOutboundMetrics()->AddPacket( buf_len + vUDP_PACKET_OVERHEAD, m_Timestamp );
					conn->m_last_send_time = m_Timestamp;
					conn->SetForceSendThisFrame( false );
				}
				else
				{
					Dbg_Printf( "*** SendTo Error: Flagging messages for resending\n" );
					// If it didn't send the messages properly, flag them for resend next frame
					conn->FlagMessagesForResending( conn->m_latest_sent_packet_stamp );
					conn->m_write_ptr = conn->m_write_buffer;
					break;
				}
				conn->m_write_ptr = conn->m_write_buffer;								
			}
		}
		
		conn->m_latest_sent_packet_stamp++;
	}
}

/******************************************************************/
/* Client's receive function                                      */
/*                                                                */
/******************************************************************/

void	Client::ReceiveData( void )
{
	Conn *conn;
	int num_bytes, actual_data_len;
	struct sockaddr from_address;
#ifdef __PLAT_NGC__
	from_address.len = sizeof( sockaddr );
#else
	int addr_len = sizeof( from_address );
#endif
	struct sockaddr_in *foreign_address;
	
		
	
	
	// Local clients never really receive. Their data is automatically "received" when
	// the local server writes its data into the client's receive buffer automatically
	if( IsLocal())
	{
		return;
	}
	// read data into local buffers
	do
	{   
		if( !m_connected )
		{   
			num_bytes = recvfrom( m_socket, m_in_packet_buffer, 
				Manager::vMAX_PACKET_SIZE, 0, &from_address, &addr_len );
		}
		else
		{   
			num_bytes = recv( m_socket, m_in_packet_buffer, Manager::vMAX_PACKET_SIZE, 0 );
		}
		if ( num_bytes < 0 )//== SOCKET_ERROR ) 
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
				Dbg_Printf( "Client Receive Error :" );
#ifdef __PLAT_WN32__
				OutputDebugString( "Client Receive Error!\n" );
#endif
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
				actual_data_len = num_bytes - sizeof( unsigned short );	// total size - CRC size
			}
			else
			{
				actual_data_len = num_bytes;
			}
			
			conn = GetConnectionByAddress( m_server_address.sin_addr.s_addr, 
											ntohs( m_server_address.sin_port ));
            if( conn )
			{
				if(( conn->m_read_ptr + actual_data_len ) < ( conn->m_read_buffer + Conn::vREAD_BUFFER_LENGTH ))
				{
					Tmr::Time cur_time;

					cur_time = Tmr::GetTime();
					if( !validate_and_copy_stream( m_in_packet_buffer, conn->m_read_ptr, num_bytes ))
					{
#ifdef __PLAT_NGPS__
						SignalSema( m_receive_semaphore_id );
#endif	// __PLAT_NGPS__
						// If the game would like to handle this foreign data, allow it to do so now
						if( m_foreign_handler )
						{
							m_foreign_handler( m_in_packet_buffer, num_bytes, &from_address );
						}
						continue;
					}
					conn->m_read_ptr += actual_data_len;
					conn->GetInboundMetrics()->AddPacket( num_bytes + vUDP_PACKET_OVERHEAD, m_Timestamp );
					if( cur_time > conn->m_last_comm_time )
					{
						conn->m_last_comm_time = cur_time;
					}					
				}
				else
				{
#ifdef __PLAT_WN32__
					char message[1024];
					sprintf( message, "****** %d: Dropped Packet\n", m_Timestamp );
					OutputDebugString( message ); 
#endif	// __PLAT_WN32__					
				}
			}
			else if( !m_connected )
			{
				Conn* foreign_conn;

				foreign_address = (struct sockaddr_in*) &from_address;
				// Foreign connection. Create temporary connection to process this foreign message
				foreign_conn = NewConnection( foreign_address->sin_addr.s_addr, 
												ntohs( foreign_address->sin_port ), 
											  Conn::mREMOTE | 
											  Conn::mFOREIGN );
				if( foreign_conn )
				{
					foreign_conn->m_app = this;
					//memcpy( foreign_conn->m_read_buffer, m_read_buffer, actual_data_len );				
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

#ifdef USE_ALIASES
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Client::ClearAliasTable( void )
{
	int i;

	for( i = 0; i < vNUM_ALIASES; i++ )
	{
		m_alias_table[i].m_Id = 0;
		m_alias_table[i].m_Expiration = -1;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

unsigned char	Client::GetObjectAlias( int obj_id, int cur_time )
{
	int hash, i;
	bool quit;

	hash = obj_id & 0xff;	// mask off all but lowest byte and use as a hash into the table
	hash %= vNUM_ALIASES;
	quit = false;
	i = hash;
	do
	{   
		if( m_alias_table[i].m_Id == obj_id )
		{   
			if( cur_time > m_alias_table[i].m_Expiration )
			{
				return (unsigned char) vNO_ALIAS;
			}
			return i;
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

unsigned char	Client::NewObjectAlias( int obj_id, int cur_time, bool expires )
{
	int hash, i;
	bool quit;

	

	hash = obj_id & 0xff;	// mask off all but lowest byte and use as a hash into the table
	hash %= vNUM_ALIASES;
	quit = false;
	i = hash;
	do
	{
		if( m_alias_table[i].m_Id == obj_id )
		{   
			m_alias_table[i].m_Expiration = cur_time + vALIAS_DURATION;
			return (unsigned char) i;
		}
		else if(( cur_time - m_alias_table[i].m_Expiration ) > vALIAS_DURATION )
		{
			m_alias_table[i].m_Id = obj_id;
			m_alias_table[i].m_Expiration = cur_time + vALIAS_DURATION;
			return (unsigned char) i;
		}
		i = ( i + 1 ) % vNUM_ALIASES;
		if( i == hash )	// complete revolution
		{
			break;
		}
	} while( !quit );

	// no free aliases
	return (unsigned char) vNO_ALIAS;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Client::GetObjectId( unsigned char alias )
{
	
    
	//Dbg_MsgAssert(( m_LatestPacketStamp <= m_alias_table[ alias ].m_Expiration ), "Server using expired alias\n" );
	return m_alias_table[ alias ].m_Id;
}

#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Client::Client( int flags )
: App( flags )
{
	m_send_network_data_task = new Tsk::Task< App > ( send_network_data, *this, 
													  Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_CLIENT_SEND_NETWORK_DATA );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}	// namespace Net
