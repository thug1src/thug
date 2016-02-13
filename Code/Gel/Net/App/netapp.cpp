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
**	File name:		netapp.cpp												**
**																			**
**	Created:		01/29/01	-	spg										**
**																			**
**	Description:	Network app code										**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <string.h>

#include <core/defines.h>
#include <core/crc.h>
#include <core/thread.h>

#include <sys/timer.h>

#ifndef __PLAT_WN32__
#include <sys/profiler.h>
#endif// __PLAT_WN32__

#include <gel/net/net.h>

#include <gel/mainloop.h>


/*****************************************************************************
**								  Externals									**
*****************************************************************************/

//extern "C"
//{
	void NS_SetSemaphores( int send_semaphore, int recv_semaphore );
//}

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

//#define DEBUG_MESSAGES
 
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

const unsigned int App::MAX_LATENCY = 2000;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/* Initialize the socket and set socket options					  */
/* Also, add default message handlers                             */
/******************************************************************/

bool	App::init( void )
{	
#ifndef __PLAT_NGPS__
	if( !IsLocal())
	{
		// create socket
		m_socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
#ifdef __PLAT_NGC__
		if( m_socket < 0 )
#else
		if( m_socket == INVALID_SOCKET )
#endif
		{
			ReportError();
			return false;
		}
	
#ifdef __PLAT_NGC__
		int val;
		 
		val = SOFcntl( m_socket, SO_F_GETFL, 0);
		if( val < 0 )
		{
			ReportError();
			return false;
		}
		val = SOFcntl( m_socket, SO_F_SETFL, val | SO_O_NONBLOCK);
		if( val < 0 )
		{
			ReportError();
			return false;
		}
#else
		unsigned long argp = 1L;	// non-zero enables non-blocking mode

		if( ioctlsocket( m_socket, FIONBIO, &argp ) == SOCKET_ERROR )
		{
			ReportError();
			return false;
		}
	
		if( m_flags.TestMask( mBROADCAST ))
		{
			if(( setsockopt( m_socket, SOL_SOCKET, SO_BROADCAST, 
					(char*) &argp, sizeof(argp))) == SOCKET_ERROR )
			{
				ReportError();
				return false;
			}
		}
#endif 	// __PLAT_NGC
	}
#else // __PLAT_NGPS__
	if( !IsLocal())
	{
		sn_int32 true_val, false_val;
	
		true_val = 1;
		false_val = 0;
		
		if(( m_socket = socket( AF_INET, SOCK_DGRAM, PF_INET )) == INVALID_SOCKET )
		{
			ReportError();
			return false;
		}
		
		// make socket non-blocking
		if( setsockopt( m_socket, SOL_SOCKET, SO_NBIO, &true_val, sizeof( sn_int32 )) == SOCKET_ERROR )
		{
			ReportError();
			return false;
		}
	
		if( m_flags.TestMask( mBROADCAST ))
		{
			if(( setsockopt( m_socket, SOL_SOCKET, SO_BROADCAST, 
					(char*) &true_val, sizeof( sn_int32 ))) == SOCKET_ERROR )
			{
				ReportError();
				return false;
			}
		}
		else
		{
			if(( setsockopt( m_socket, SOL_SOCKET, SO_BROADCAST, 
					(char*) &false_val, sizeof( sn_int32 ))) == SOCKET_ERROR )
			{
				ReportError();
				return false;
			}
		}
	}

	m_shutting_down = false;

	m_socket_thread_data.m_pEntry = threaded_transfer_data;
	m_socket_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
	m_socket_thread_data.m_pStackBase = m_socket_thread_stack;
	m_socket_thread_data.m_iStackSize = vSOCKET_THREAD_STACK_SIZE;
	m_socket_thread_data.m_utid = vBASE_SOCKET_THREAD_ID + m_net_man->NumApps();
	Thread::CreateThread( &m_socket_thread_data );
	m_socket_thread_id = m_socket_thread_data.m_osId;
	
	struct SemaParam params;

	params.initCount = 1;
	params.maxCount = 10;

	m_send_semaphore_id = CreateSema( &params );
	m_receive_semaphore_id = CreateSema( &params );
	m_transfer_semaphore_id = CreateSema( &params );
	m_active_semaphore_id = CreateSema( &params );

    if( !IsLocal())
	{
        NS_SetSemaphores( m_send_semaphore_id, m_receive_semaphore_id );
	}

	#ifdef __USER_STEVE__
	Dbg_Printf( "Send Semaphore %d\n", m_send_semaphore_id );
	Dbg_Printf( "Receive Semaphore %d\n", m_receive_semaphore_id );
	Dbg_Printf( "Transfer Semaphore %d\n", m_transfer_semaphore_id );
	#endif

	StartThread( m_socket_thread_id, this );
#endif	// __PLAT_NGPS__

	m_num_connections = 0;	
	m_TotalBytesIn = 0;
	m_TotalBytesOut = 0;
	m_LostPackets = 0;
	m_LatePackets = 0;
	m_DupePackets = 0;
	m_FrameCounter = 0;

	m_Dispatcher.Init();
    
	m_Dispatcher.AddHandler( MSG_ID_ACK, handle_ack, mHANDLE_LATE | mHANDLE_FOREIGN );
	m_Dispatcher.AddHandler( MSG_ID_SEQUENCED, handle_sequenced_messages, mHANDLE_LATE );
	m_Dispatcher.AddHandler( MSG_ID_STREAM_START, handle_stream_messages, mHANDLE_LATE );
	m_Dispatcher.AddHandler( MSG_ID_STREAM_DATA, handle_stream_messages, mHANDLE_LATE );
	m_Dispatcher.AddHandler( MSG_ID_PING_TEST, handle_latency_test, mHANDLE_LATE );
	m_Dispatcher.AddHandler( MSG_ID_PING_RESPONSE, handle_latency_response, mHANDLE_LATE );

	return true;
}

/******************************************************************/
/* Bind a socket                                      			  */
/*                                                                */
/******************************************************************/

bool App::bind_app_socket( int address, unsigned short port )
{
	struct sockaddr_in host_address;
	int address_len, result;

	// bind the socket to an address.

	memset( &host_address, 0, sizeof(host_address));
	host_address.sin_family = AF_INET;
	host_address.sin_port = htons( port );
	// This should basically check if we're using DHCP, but right now modem is the only
	// device using it
	if( m_net_man->GetConnectionType() != vCONN_TYPE_MODEM )
	{
#ifdef __PLAT_XBOX__
		host_address.sin_addr.s_addr = INADDR_ANY;
#else
#ifdef __PLAT_NGC__
		host_address.s_addr.s_addr = address;
#else
		host_address.sin_addr.s_addr = address;
#endif
#endif				
	}
#ifdef __PLAT_NGC__
	host_address.len = sizeof( SOSockAddrIn );
#else
	memset ( &host_address.sin_zero, 0, 8 );  
#endif

	// bind(name) socket
	result = bind( m_socket, (struct sockaddr *)&host_address, sizeof(host_address));
	if( result < 0 )// == SOCKET_ERROR )
	{
#ifdef WIN32
		int err = WSAGetLastError();
		if( err == WSAEADDRINUSE )  
#else
#ifdef __PLAT_NGPS__
		int err = sn_errno( m_socket );
		if( err == EADDRINUSE )
#else
#ifdef __PLAT_NGC__
		if( result == SO_EADDRINUSE )
#endif
#endif
#endif

		{
			ReportError();
			host_address.sin_port = htons( 0 );
			result = bind( m_socket, (struct sockaddr *)&host_address, sizeof(host_address));
			if( result < 0 )// == SOCKET_ERROR )
			{
				ReportError();
				return false;
			}
		}
		else
		{
			ReportError();
			return false;
		}
	}		
    
#ifdef __PLAT_NGC__
	m_local_address.len = sizeof( SOSockAddrIn );
#endif
	address_len = sizeof( m_local_address );
	result = getsockname( m_socket, (sockaddr*) &m_local_address, &address_len );
	if( result < 0 )// == SOCKET_ERROR )
	{
		ReportError();
		return false;
	}

	return true;
}

/******************************************************************/
/* Free up memory used by the network app. Remove msg handlers    */
/*                                                                */
/******************************************************************/

void	App::deinit( void )
{
	

	Lst::Search< Conn > sh;
	Conn *conn, *next;

	if( !IsLocal())
	{
		closesocket( m_socket );
	}
	
	m_Dispatcher.Deinit();

	for( conn = sh.FirstItem( m_connections ); conn; conn = next )
	{
		next = sh.NextItem();
		delete conn;
	}
}

/******************************************************************/
/* Get the next free available handle                             */
/*                                                                */
/******************************************************************/

int		App::get_free_handle( void )
{
	int i;

	

	for( i = Conn::vHANDLE_FIRST; i < 256; i++ )
	{
		if( !GetConnectionByHandle( i ))
		{
			return i;
		}
	}

	Dbg_MsgAssert( 0,( "No free handles\n" ));
	return Conn::vHANDLE_INVALID;
}

/******************************************************************/
/* Process the list of sequenced messages that we have queued up  */
/*                                                                */
/******************************************************************/

void	App::process_sequenced_messages( Conn *conn )
{
	int i;
	Lst::Search< MsgSeqLink > sh;
	MsgSeqLink *msg_link, *next;
	MsgHandlerContext msg_context;
	int result;

	

	Dbg_Assert( conn );

	result = HANDLER_CONTINUE;	
	for( i = 0; i < MAX_SEQ_GROUPS; i++ )
	{
		for( msg_link = sh.FirstItem( conn->m_SequencedBuffer[i] );
				msg_link; msg_link = next )
		{
			next = sh.NextItem();
			
			if( msg_link->m_SequenceId == conn->m_WaitingForSequenceId[msg_link->m_GroupId] )
			{				
				msg_context.m_Conn = conn;
				msg_context.m_Msg = NULL;
				
				if( msg_link->m_QMsg->m_Data )
				{
					msg_context.m_Msg = msg_link->m_QMsg->m_Data;
				}
				msg_context.m_App = this;
				msg_context.m_PacketFlags = 0;
				msg_context.m_MsgId = msg_link->m_QMsg->m_MsgId;
				msg_context.m_MsgLength = msg_link->m_QMsg->m_MsgLength;

				conn->m_WaitingForSequenceId[msg_link->m_GroupId]++;

				result = m_Dispatcher.DispatchMessage( &msg_context );
								
				if( result != HANDLER_MSG_DESTROYED )
				{
					delete msg_link->m_QMsg;
					delete msg_link;
				}

				// If we've gotten a false result, that means we should
				// stop processing this connection
				if( result != HANDLER_CONTINUE )
				{
					return;
				}
			}
			else
			{
#ifdef NET_DEBUG_MESSAGES
				static unsigned int stall = 0;

				if( stall != msg_link->m_SequenceId )
				{
					Dbg_Printf( "*** Stalling on seq: %d, waiting for %d : msg %d, group %d\n", 
								msg_link->m_SequenceId, conn->m_WaitingForSequenceId[msg_link->m_GroupId],
								msg_link->m_QMsg->m_MsgId, msg_link->m_GroupId );
					stall = msg_link->m_SequenceId;
				}
#endif
				break;
			}

		}
	}
}

/******************************************************************/
/* Kill off invalid connections. Should be done at a safe time.   */
/* i.e. when you're not processing connections anymore            */
/******************************************************************/

void	App::terminate_invalid_connections( void )
{
	Conn *conn, *next_conn;
	Lst::Search< Conn > sh;

#ifdef __PLAT_NGPS__
	WaitSema( m_transfer_semaphore_id );
#endif	// __PLAT_NGPS__    
	for( conn = FirstConnection( &sh ); conn;
			conn = next_conn )
	{
		next_conn = NextConnection( &sh );

		// If we're in the process of disconnecting them and we have no more messages
		// to send them, go ahaed and disconnect them
		if( conn->TestStatus( Conn::mSTATUS_DISCONNECTING ))
		{
			if( MessagesToSend( conn ) == false )
			{
				conn->Invalidate();
			}
		}

		// Destroy invalid connections
		if( conn->IsValid() == false )
		{
			if( ( MessagesToProcess( conn ) == false ) && 
				( MessagesToSend( conn ) == false ))
			{
				TerminateConnection( conn );
			}
		}
	}       
#ifdef __PLAT_NGPS__
	SignalSema( m_transfer_semaphore_id );
#endif	// __PLAT_NGPS__    
}

/******************************************************************/
/* Processes any pending stream messages. Enqueuing data if the	  */
/* sliding window permits it                                      */
/******************************************************************/

void    App::process_stream_messages( void )
{
	int size, packet_len, msg_len;
	MsgStreamData data_msg;
	MsgDesc msg_desc;
	StreamDesc* stream_desc;
	StreamLink* stream_link, *next_link;
	Conn *conn;
	Lst::Search< Conn > sh;
	Lst::Search< StreamLink > stream_sh;
	int num_pending, num_to_send;
	Net::Manager* net_man = Net::Manager::Instance();

	for( conn = FirstConnection( &sh ); conn; conn = NextConnection( &sh ))
	{
		int pending_threshold, max_to_send;

		pending_threshold = 5;
		max_to_send = 3;
		if(( net_man->GetConnectionType() == Net::vCONN_TYPE_MODEM ) ||
		   ( conn->GetBandwidthType() == Conn::vNARROWBAND ))
		{
			pending_threshold = 2;
			max_to_send = 1;
		}
		// If we have any pending streams, see if there is any room to queue up chunks
		if( conn->m_StreamOutList.CountItems() > 0 )
		{
			//Dbg_Printf( "******** %d items remain to stream\n", conn->m_StreamOutList.CountItems());
			num_pending = conn->GetNumPendingStreamMessages();
			if( num_pending < pending_threshold ) // Should be configurable and dynamic
			{
				num_to_send = pending_threshold - num_pending;
				if( num_to_send > max_to_send )
				{
					num_to_send = max_to_send;
				}
				for( stream_link = stream_sh.FirstItem( conn->m_StreamOutList ); stream_link; stream_link = next_link )
				{
					next_link = stream_sh.NextItem();

					stream_desc = stream_link->m_Desc;
					size = (unsigned int) stream_desc->m_DataPtr - (unsigned int) stream_desc->m_Data;
					msg_len = stream_desc->m_Size;
					while(( size < msg_len ) && ( num_to_send > 0 ))
					{
						packet_len = msg_len - size;
						if( packet_len > MAX_STREAM_CHUNK_LENGTH )
						{
							packet_len = MAX_STREAM_CHUNK_LENGTH;
						}
				
						data_msg.m_StreamId = stream_desc->m_StreamId;
						memcpy( data_msg.m_Data, stream_desc->m_DataPtr, packet_len );
						
						msg_desc.m_Id = MSG_ID_STREAM_DATA;
						msg_desc.m_Data = &data_msg;
						msg_desc.m_Length = packet_len + sizeof( int );
						msg_desc.m_Queue = QUEUE_SEQUENCED;
						msg_desc.m_GroupId = stream_desc->m_GroupId;
						msg_desc.m_StreamMessage = 1;
						msg_desc.m_ForcedSequenceId = stream_desc->m_SequenceId++;
				
						//Dbg_Printf( "%d ******** [%d] Streaming %d bytes. ForcedSequence: %d\n", m_FrameCounter, stream_desc->m_StreamId, packet_len, stream_desc->m_SequenceId );
						EnqueueMessage( conn->GetHandle(), &msg_desc );
				
						stream_desc->m_DataPtr += packet_len;
						size += packet_len;

						num_to_send--;
					}

					if( size >= msg_len )
					{
						//Dbg_Printf( "******** [%d] Finished streaming. Destroying Stream Link\n", stream_desc->m_StreamId );
						delete stream_desc;
						delete stream_link;
					}
				}
			}
		}
	}
}

/******************************************************************/
/* Read the data from the stack and store in appropriate conn     */
/*                                                                */
/******************************************************************/

void	App::read_network_data( const Tsk::Task< App > &task )
{
	App *app;
    
    app = &task.GetData();

#ifdef __PLAT_NGPS__
	app->m_double_timestamp += ( Tmr::UncappedFrameLength() * 60.0f ) * 
									( Tmr::vRESOLUTION / 60.0f );
	app->m_Timestamp = (unsigned int) app->m_double_timestamp;
#else
	app->m_Timestamp = Tmr::GetTime();
#endif // _PLAT_NGPS__

	app->terminate_invalid_connections();
	
#ifdef __PLAT_NGPS__
	WaitSema( app->m_transfer_semaphore_id );
	SignalSema( app->m_transfer_semaphore_id );
	WakeupThread( app->m_socket_thread_id );
#else
	app->ReceiveData();
#endif
}

/******************************************************************/
/* Service metrics data                 					  	  */
/*                                                                */
/******************************************************************/

void	App::service_network_metrics( const Tsk::Task< App > &task )
{
	App *app;
	Conn *conn;
	Lst::Search< Conn > sh;
    
    app = &task.GetData();
	Dbg_Assert( app );

	for( conn = app->FirstConnection( &sh ); conn; conn = app->NextConnection( &sh ))
	{
		conn->GetInboundMetrics()->CalculateBytesPerSec( app->m_Timestamp );
		conn->GetOutboundMetrics()->CalculateBytesPerSec( app->m_Timestamp );
	}
}

/******************************************************************/
/* Process network data task                 					  */
/*                                                                */
/******************************************************************/

void	App::process_network_data( const Tsk::Task< App > &task )
{
	App *app;
    
    app = &task.GetData();
	Dbg_Assert( app );
	
		app->m_FrameCounter++;

#	ifdef __USE_PROFILER__
	Sys::CPUProfiler->PushContext( 255, 255, 255 );
#	endif // __USE_PROFILER__

#ifdef __PLAT_NGPS__
	WaitSema( app->m_receive_semaphore_id );
#endif	// __PLAT_NGPS__    
	app->ProcessData(); 
#ifdef __PLAT_NGPS__
	SignalSema( app->m_receive_semaphore_id );
#endif	// __PLAT_NGPS__    

#	ifdef __USE_PROFILER__
	Sys::CPUProfiler->PopContext();
#	endif // __USE_PROFILER__
}

/******************************************************************/
/* Threaded send/receive data for the PS2						  */
/*                                                                */
/******************************************************************/

#ifdef __PLAT_NGPS__

void	App::threaded_transfer_data( void *data )
{
	
    Net::App* app = (Net::App *) data;
    
	WaitSema( app->m_active_semaphore_id );

	if( !app->IsLocal())
	{
#ifdef DEBUG_MESSAGES
		Dbg_Printf( "Registering transfer thread %d with stack\n", GetThreadId());
#endif
	
#ifdef __NOPT_ASSERT__
		int result = 
#endif		
		sockAPIregthr();
		Dbg_Assert( result == 0 );
	}

	app->m_socket_thread_active = true;
	
	app->TransferData();

	if( !app->IsLocal())
	{   
#ifdef DEBUG_MESSAGES
		Dbg_Printf( "DeRegistering transfer thread %d with stack\n", GetThreadId());
#endif
		sockAPIderegthr();
	}

	app->m_socket_thread_active = false;
	
	SignalSema( app->m_active_semaphore_id );
}

#endif

/******************************************************************/
/* Send queued messages across the wire                           */
/*                                                                */
/******************************************************************/

void	App::send_network_data( const Tsk::Task< App > &task )
{
	App *app;

	app = &task.GetData();
	Dbg_Assert( app );

	app->process_stream_messages();

#ifdef __PLAT_NGPS__
	WaitSema( app->m_transfer_semaphore_id );
	SignalSema( app->m_transfer_semaphore_id );
	WakeupThread( app->m_socket_thread_id );
#else
	app->SendData( true );    
#endif

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	App::crc_and_copy_stream( char* in_stream, char* out_stream, int in_len, int* out_len )
{
	uint32 actual_crc;	
	int i;
	unsigned char* in, *out;
	unsigned char key;
	unsigned short hi_word, lo_word, sum_crc;

	
	
	Dbg_Assert( out_stream );
	Dbg_Assert( in_stream );
	Dbg_Assert( in_len > 0 );
	
	actual_crc = Crc::GenerateCRCCaseSensitive( in_stream, in_len );
	hi_word = ( actual_crc & 0xFFFF0000 ) >> 16;
	lo_word = ( actual_crc & 0xFFFF );
	sum_crc = hi_word + lo_word;

	memcpy( out_stream, &sum_crc, sizeof( unsigned short ));

	// Encrypt by XOring each byte with the lsB of the checksum
	out = (unsigned char*) ( out_stream + sizeof( unsigned short ));
	in = (unsigned char*) in_stream;
	key = (unsigned char) sum_crc;
	for( i = 0; i < in_len; i++ )
	{
		*out++ = ( *in++ ) ^ key;
	}
	
	*out_len = in_len + sizeof( unsigned short );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	App::validate_and_copy_stream( char* in_stream, char* out_stream, int in_len )
{
    uint32 actual_crc;
	int out_len;
	unsigned char key;
	unsigned short hi_word, lo_word;
	unsigned short packet_crc, sum_crc;
	unsigned char* in;
	int i;
	char packet_data[ Manager::vMAX_PACKET_SIZE ];

	
	
	Dbg_Assert( out_stream );
	Dbg_Assert( in_stream );
		
	if( in_len < Manager::vMIN_PACKET_SIZE )
	{
		return false;
	}

	// If this isn't a secure app, just process the data as unencrypted and un-checksumed
	if(( m_flags & mSECURE ) == 0 )
	{
		memcpy( out_stream, in_stream, in_len );
		return true;
	}

	out_len = in_len - sizeof( unsigned short );
	memcpy( &packet_crc, in_stream, sizeof( unsigned short ));
	memcpy( packet_data, in_stream + sizeof( unsigned short ), out_len );	// skip CRC

	key = (unsigned char) packet_crc;	// use the lsB of the packet CRC to decrypt the packet
	in = (unsigned char*) packet_data;
	for( i = 0; i < out_len; i++ )
	{
		*in = (*in) ^ key;
		in++;
	}

	actual_crc = Crc::GenerateCRCCaseSensitive( packet_data, out_len );
	hi_word = ( actual_crc & 0xFFFF0000 ) >> 16;
	lo_word = ( actual_crc & 0xFFFF );
	sum_crc = hi_word + lo_word;
	if( sum_crc != packet_crc )
	{
#ifdef DEBUG_MESSAGES
		Dbg_Printf( "CRC Check Failed!!! Got %d instead of %d\n", packet_crc, sum_crc );
#endif
		return false;
	}
	
	memcpy( out_stream, packet_data, out_len );

#ifdef WRITE_OUT_NET_PACKETS
	static int f = 0;
	char path[32];

	sprintf( path, "jpacket%d.net", f );
	void* handle;

	handle = File::Open( path, "wb");
	if( handle )
	{
		File::Write( out_stream, 1, out_len, handle );
		File::Close( handle );	
		f++;
	}
#endif

	return true;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

LatencyTest::LatencyTest( void )
{
	memset( m_TimeTests, 0, sizeof( int ) * NET_NUM_LATENCY_TESTS );
	m_AveLatency = Manager::vSTANDARD_RESEND_THRESHOLD / 2;
	m_CurrentTest = 0;
	m_SendTime = 0;
	m_ReceiveTime = 0;
}

/******************************************************************/
/* Add data to our latency calculator                             */
/*                                                                */
/******************************************************************/

void	LatencyTest::InputLatencyValue( int latency )
{
	int i, num_tests, total;

	m_TimeTests[ m_CurrentTest ] = latency;
	m_CurrentTest = ( m_CurrentTest + 1 ) % NET_NUM_LATENCY_TESTS;

	total = 0;
	num_tests = 0;
	// compute new average
	for( i = 0; i < NET_NUM_LATENCY_TESTS; i++ )
	{
		if( m_TimeTests[i] != 0 )
		{
			total += m_TimeTests[i];
			num_tests++;
		}
	}

	if( num_tests > 0 )
	{
        m_AveLatency = total/num_tests;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

App::App( int flags )
: m_Dispatcher( this ), m_node( this )
{
	m_network_metrics_task = new Tsk::Task< App > ( service_network_metrics, *this, 
														 Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_PROCESS_NETWORK_METRICS );
	m_receive_network_data_task = new Tsk::Task< App > ( read_network_data, *this, 
														 Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_RECEIVE_NETWORK_DATA );
	m_process_network_data_task = new Tsk::Task< App > ( process_network_data, *this,
														 Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_PROCESS_NETWORK_DATA );
	m_double_timestamp = 0;
	m_flags = flags;
	m_Timestamp = 0;
	m_foreign_handler = NULL;
#ifdef	__PLAT_NGPS__											  
	Dbg_MsgAssert(Mem::SameContext(this,Mem::Manager::sHandle().NetworkHeap()),("Net::App not on network heap"));	
#endif		//	__PLAT_NGPS__											  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

App::~App( void )
{
	
	BannedConn* banned_conn, *next;
	Lst::Search< BannedConn > sh;
	
	for( banned_conn = sh.FirstItem( m_banned_connections ); banned_conn;
			banned_conn = next )
	{
		next = sh.NextItem();
		delete banned_conn;
	}
	
    
	delete m_send_network_data_task;
	delete m_receive_network_data_task;
    delete m_process_network_data_task;
	delete m_network_metrics_task;
}

/******************************************************************/
/* Processes the messages in the input queue and also sequenced   */
/* messages. Sends them off to the dispatcher                     */
/******************************************************************/

void	App::ProcessData( void )
{
	Conn *conn, *next_conn;
	Lst::Search< Conn > sh;
	int result;

	for( conn = FirstConnection( &sh ); conn;
			conn = next_conn )
	{   
		next_conn = NextConnection( &sh );

		if( conn->IsLocal() &&
			conn->m_alias_connection )
		{   
			int num_bytes;
			
			num_bytes = (int) conn->m_alias_connection->m_write_ptr - 
							(int) conn->m_alias_connection->m_write_buffer;
			memcpy( conn->m_read_ptr, conn->m_alias_connection->m_write_buffer, num_bytes );
			conn->m_alias_connection->m_write_ptr = conn->m_alias_connection->m_write_buffer;
			conn->m_read_ptr += num_bytes;
		}

		result = m_Dispatcher.DispatchMsgHandlers( conn, 0 );
		if( result == HANDLER_ERROR )
		{
			// output that we've received mal-formatted data
		}
		else if( result == HANDLER_HALT )	// something terminable happened
		{
			return;
		}
		process_sequenced_messages( conn );
	}		
}

/******************************************************************/
/* Given an ip/port combo, return the corresponding connection    */
/* if one exists                                                  */
/******************************************************************/

Conn *App::GetConnectionByAddress( int ip, unsigned short port )
{
	Lst::Search< Conn > sh;
	Conn *conn;

	for( conn = sh.FirstItem( m_connections ); 
			conn; conn = sh.NextItem())
	{
		if(( conn->GetIP() == ip ) && ( conn->GetPort() == port ))
		{
			return conn;
		}
	}

	return NULL;
}

/******************************************************************/
/* Given a handle, return the connection if there is a match      */
/*                                                                */
/******************************************************************/

Conn *App::GetConnectionByHandle( int handle )
{
	Lst::Search< Conn > sh;
	Conn *conn;

	for( conn = sh.FirstItem( m_connections ); 
			conn; conn = sh.NextItem())
	{
		if( conn->GetHandle() == handle )
		{
			return conn;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int			App::GetID( void )
{
	return m_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char		*App::GetName( void )
{
	return m_name;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Tsk::BaseTask&	App::GetSendDataTask( void )
{
	return *m_send_network_data_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Tsk::BaseTask&	App::GetReceiveDataTask( void )
{
	return *m_receive_network_data_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Tsk::BaseTask&	App::GetProcessDataTask( void )
{
	return *m_process_network_data_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Tsk::BaseTask&	App::GetNetworkMetricsTask( void )
{
	return *m_network_metrics_task;
}

/******************************************************************/
/*  Create a new connection for an ip/port combo                  */
/*                                                                */
/******************************************************************/

Conn *App::NewConnection( int ip, unsigned short port, int flags )
{
	Conn *conn;
	
	

	conn = NULL;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	if( m_num_connections < m_max_connections )
	{
		conn = new Conn( flags );
		conn->SetIP( ip );
		conn->SetPort( port );
		conn->m_handle = get_free_handle();
		conn->m_app = this;
		conn->m_last_comm_time = Tmr::GetTime();
		m_connections.AddToTail( &conn->m_node );
	}

	Mem::Manager::sHandle().PopContext();

	return conn;
}

/******************************************************************/
/* Allocate a new sequenced message with a header                 */
/*                                                                */
/******************************************************************/

QueuedMsgSeq*	App::AllocateNewSeqMessage( unsigned char msg_id, unsigned short msg_len, char* data, unsigned char group_id )
{
	QueuedMsgSeq* queued_msg;

	queued_msg = new QueuedMsgSeq( msg_id, msg_len, data, group_id );

	return queued_msg;
}

/******************************************************************/
/* Build the outgoing stream to send to a connection.  This       */
/* will also re-send important messages which haven't been acked  */
/******************************************************************/

bool App::BuildMsgStream( Conn *conn, QUEUE_TYPE queue, bool resends_only )
{
	bool all_fit;
	unsigned short total_msg_len;

	all_fit = true;

	Dbg_Assert( conn );
	// For each queue, copy all messages into what will be the final
	// buffer to send over the net
	switch( queue )
	{
		case QUEUE_DEFAULT:
		{
			Lst::Search< MsgLink > sh;
			MsgLink *msg_link, *next_link;
			QueuedMsg* queued_msg;
						
			for( msg_link = sh.FirstItem( conn->m_normal_msg_list ); msg_link; msg_link = next_link )
			{   
				int msg_header_size;

				next_link = sh.NextItem();
				if( m_Timestamp < msg_link->m_SendTime )
				{
					continue;
				}

				if( BandwidthExceeded( conn ))
				{
					break;
				}

				queued_msg = msg_link->m_QMsg;              
				if( m_net_man->GetMessageFlags( queued_msg->m_MsgId ) & mMSG_SIZE_UNKNOWN )
				{
					msg_header_size = Manager::vMSG_HEADER_LENGTH;
				}
				else
				{
					msg_header_size = Manager::vMSG_HEADER_LENGTH_WITH_SIZE;
				}
				total_msg_len = msg_header_size + queued_msg->m_MsgLength;
				if((( conn->m_write_ptr - conn->m_write_buffer ) + total_msg_len ) < Manager::vMAX_PAYLOAD )
				{
					memcpy( conn->m_write_ptr, &queued_msg->m_MsgId, sizeof( unsigned char ));
					conn->m_write_ptr += sizeof( unsigned char );
					// Optionally write out the message's size to the stream
					if(( m_net_man->GetMessageFlags( queued_msg->m_MsgId ) & mMSG_SIZE_UNKNOWN ) == 0 )
					{
						memcpy( conn->m_write_ptr, &queued_msg->m_MsgLength, sizeof( unsigned short ));
						conn->m_write_ptr += sizeof( unsigned short );
					}
					if( queued_msg->m_MsgLength > 0 )
					{
						memcpy( conn->m_write_ptr, queued_msg->m_Data, queued_msg->m_MsgLength );
						conn->m_write_ptr += queued_msg->m_MsgLength;
					}
					conn->GetOutboundMetrics()->AddMessage( queued_msg->m_MsgId, queued_msg->m_MsgLength );
					
					// K: Moved this here so that multiple EnqueueMessage's can be done in one frame
					// without earlier ones being dropped.
					// Otherwise CScriptDebugger::SplitAndEnqueueMessage will not work, because
					// only the last message enqueued will get through.
					queued_msg->m_Ref.Release();
					if( queued_msg->m_Ref.InUse() == false )
					{   
						delete queued_msg;
					}
					delete msg_link;
				}
				else
				{
					all_fit = false;
				}
			}

			break;
		}

		case QUEUE_IMPORTANT:
		{
			MsgImpLink *msg_link;
			Lst::Search< MsgImpLink > sh;
			QueuedMsg* queued_msg;
						
			for( msg_link = sh.FirstItem( conn->m_important_msg_list ); msg_link; 
					msg_link = sh.NextItem())
			{
				if( m_Timestamp < msg_link->m_SendTime )
				{
					continue;
				}

				if( BandwidthExceeded( conn ))
				{
					break;
				}

				if(( msg_link->m_Timestamp == 0 ) && resends_only )
				{
					continue;
				}

				// resend if it takes longer than roughly twice the latency
				if( ( msg_link->m_Timestamp == 0 ) ||	// i.e. hasn't been sent yet
					(( m_Timestamp - msg_link->m_Timestamp ) > (unsigned int) conn->GetResendThreshold()))
				{
					int msg_header_size;

					queued_msg = msg_link->m_QMsg;
#ifdef DEBUG_MESSAGES
					if( conn->IsRemote())
					{
						if( msg_link->m_Timestamp != 0 )
						{
							Dbg_Printf( "** RESEND #%d (%d) Conn %d: (%d) %s T %d %d, P %d %d, L %d %d\n", 
								msg_link->m_NumResends + 1,
								m_FrameCounter,
								conn->GetHandle(),
	
								queued_msg->m_MsgId,
								m_net_man->GetMessageName( queued_msg->m_MsgId ),
								
								m_Timestamp,
								msg_link->m_Timestamp,
								
								conn->m_latest_sent_packet_stamp,
								msg_link->m_Packetstamp,
								
								conn->GetResendThreshold(),
								conn->GetAveLatency());							
						}
					}
#endif
					if( m_net_man->GetMessageFlags( queued_msg->m_MsgId ) & mMSG_SIZE_UNKNOWN )
					{
						msg_header_size = Manager::vMSG_HEADER_LENGTH;
					}
					else
					{
						msg_header_size = Manager::vMSG_HEADER_LENGTH_WITH_SIZE;
					}

					total_msg_len = msg_header_size + queued_msg->m_MsgLength;
					if((( conn->m_write_ptr - conn->m_write_buffer ) + total_msg_len ) < Manager::vMAX_PAYLOAD )
					{
						memcpy( conn->m_write_ptr, &queued_msg->m_MsgId, sizeof( unsigned char ));
						conn->m_write_ptr += sizeof( unsigned char );
						// Optionally write out the message's size to the stream
						if(( m_net_man->GetMessageFlags( queued_msg->m_MsgId ) & mMSG_SIZE_UNKNOWN ) == 0 )
						{
							memcpy( conn->m_write_ptr, &queued_msg->m_MsgLength, sizeof( unsigned short ));
							conn->m_write_ptr += sizeof( unsigned short );
						}
						
						if( queued_msg->m_MsgLength > 0 )
						{
							memcpy( conn->m_write_ptr, queued_msg->m_Data, queued_msg->m_MsgLength );
							conn->m_write_ptr += queued_msg->m_MsgLength;
						}
						conn->GetOutboundMetrics()->AddMessage( queued_msg->m_MsgId, queued_msg->m_MsgLength );
						
						if( msg_link->m_Timestamp != 0 )
						{   
							msg_link->m_NumResends++;							
						}
						msg_link->m_Timestamp = m_Timestamp;
						msg_link->m_Packetstamp = conn->m_latest_sent_packet_stamp;
					}					
					else
					{
						all_fit = false;
					}
				}
			}

			break;
		}

		case QUEUE_SEQUENCED:
		{
			MsgSeqLink *msg_link;
			Lst::Search< MsgSeqLink > sh;
			QueuedMsgSeq* queued_msg;
			unsigned char seq_msg_id;
			unsigned short seq_msg_length;
						
			seq_msg_id = MSG_ID_SEQUENCED;
			for( msg_link = sh.FirstItem( conn->m_sequenced_msg_list ); msg_link; 
					msg_link = sh.NextItem())
			{   
				if( m_Timestamp < msg_link->m_SendTime )
				{
					continue;
				}

				if( BandwidthExceeded( conn ))
				{
					break;
				}

				if(( msg_link->m_Timestamp == 0 ) && resends_only )
				{
					continue;
				}

				// resend if it takes longer than roughly twice the latency
				if( ( msg_link->m_Timestamp == 0 ) ||	// i.e. hasn't been sent yet
					(( m_Timestamp - msg_link->m_Timestamp ) > (unsigned int) conn->GetResendThreshold()))
				{
					queued_msg = msg_link->m_QMsg;
#ifdef DEBUG_MESSAGES
					if( conn->IsRemote())
					{
						if( msg_link->m_Timestamp != 0 )
						{
							Dbg_Printf( "** RESEND #%d (%d) Conn %d: (%d) %s T %d %d, P %d %d, Seq: %d Group: %d\n", 
								msg_link->m_NumResends + 1,
								m_FrameCounter,
								conn->GetHandle(),
								
								queued_msg->m_MsgId,
								m_net_man->GetMessageName( queued_msg->m_MsgId ),
								
								m_Timestamp,
								msg_link->m_Timestamp,
								
								conn->m_latest_sent_packet_stamp,
								msg_link->m_Packetstamp,
								
								msg_link->m_SequenceId,
								msg_link->m_GroupId );							
						}						
					}
#endif
					total_msg_len = Manager::vMSG_HEADER_LENGTH_WITH_SIZE + Manager::vMSG_SEQ_HEADER_LENGTH + queued_msg->m_MsgLength;
					seq_msg_length = Manager::vMSG_SEQ_HEADER_LENGTH + queued_msg->m_MsgLength;
					if((( conn->m_write_ptr - conn->m_write_buffer ) + total_msg_len ) < ( Manager::vMAX_PAYLOAD ))
					{						
						memcpy( conn->m_write_ptr, &seq_msg_id, sizeof( unsigned char ));
						conn->m_write_ptr += sizeof( unsigned char );
                        memcpy( conn->m_write_ptr, &seq_msg_length, sizeof( unsigned short ));
						conn->m_write_ptr += sizeof( unsigned short );
                        memcpy( conn->m_write_ptr, &msg_link->m_GroupId, sizeof( char ));
						conn->m_write_ptr += sizeof( char );
						memcpy( conn->m_write_ptr, &msg_link->m_SequenceId, sizeof( unsigned int ));
						conn->m_write_ptr += sizeof( unsigned int );
						memcpy( conn->m_write_ptr, &queued_msg->m_MsgId, sizeof( unsigned char ));
						conn->m_write_ptr += sizeof( unsigned char );
						if( queued_msg->m_MsgLength > 0 )
						{
							memcpy( conn->m_write_ptr, queued_msg->m_Data, queued_msg->m_MsgLength );
							conn->m_write_ptr += queued_msg->m_MsgLength;
						}
						
						conn->GetOutboundMetrics()->AddMessage( queued_msg->m_MsgId, queued_msg->m_MsgLength );
						if( msg_link->m_Timestamp != 0 )
						{
							msg_link->m_NumResends++;
						}
						msg_link->m_Timestamp = m_Timestamp;
						msg_link->m_Packetstamp = conn->m_latest_sent_packet_stamp;
					}                   
					else
					{
						all_fit = false;
					}
				}
			}
			break;
		}
	}

	return all_fit;
}

/******************************************************************/
/* Any pending messages to process from this client??     		  */
/*                                                                */
/******************************************************************/

bool	App::MessagesToProcess( Conn* conn )
{
	if( conn->m_read_ptr > conn->m_read_buffer )
	{
		return true;
	}
		
	return false;
}

/******************************************************************/
/* Are there any important messages to send?					  */
/*                                                                */
/******************************************************************/

bool	App::ImportantMessagesToSend( Conn* conn )
{
	{
		MsgImpLink *msg_link;
		Lst::Search< MsgImpLink > imp_sh;
		for( msg_link = imp_sh.FirstItem( conn->m_important_msg_list ); 
				msg_link; msg_link = imp_sh.NextItem())
		{
			// resend if it takes longer than roughly twice the latency
			if(	( msg_link->m_Timestamp == 0 ) ||
				(( m_Timestamp - msg_link->m_Timestamp ) > (unsigned int) conn->GetResendThreshold()))
			{
				return true;
			}
		}
	}

	{
		MsgSeqLink *msg_link;
		Lst::Search< MsgSeqLink > seq_sh;
		for( msg_link = seq_sh.FirstItem( conn->m_sequenced_msg_list ); 
				msg_link; msg_link = seq_sh.NextItem())
		{
			// resend if it takes longer than roughly twice the latency
			if( ( msg_link->m_Timestamp == 0 ) || 
				(( m_Timestamp - msg_link->m_Timestamp ) > (unsigned int) conn->GetResendThreshold()))
			{
				return true;		
			}
		}
	}

	return false;
}

/******************************************************************/
/* Are there any messages to send to a particular connection?     */
/*                                                                */
/******************************************************************/

bool	App::MessagesToSend( Conn* conn )
{   
	{
		Lst::Search< MsgLink > sh;
		MsgLink* msg_link;
		for( msg_link = sh.FirstItem( conn->m_normal_msg_list ); 
				msg_link; msg_link = sh.NextItem())
		{
			if( m_Timestamp < msg_link->m_SendTime )
			{
				continue;
			}
			return true;
		}
	}

	{
		MsgImpLink *msg_link;
		Lst::Search< MsgImpLink > imp_sh;
		for( msg_link = imp_sh.FirstItem( conn->m_important_msg_list ); 
				msg_link; msg_link = imp_sh.NextItem())
		{
			// resend if it takes longer than roughly twice the latency
			if( ( msg_link->m_Timestamp == 0 ) ||
				( m_Timestamp < msg_link->m_SendTime ) ||
				(( m_Timestamp - msg_link->m_Timestamp ) > (unsigned int) conn->GetResendThreshold()))
			{
				return true;
			}
		}
	}

	{
		MsgSeqLink *msg_link;
		Lst::Search< MsgSeqLink > seq_sh;
		for( msg_link = seq_sh.FirstItem( conn->m_sequenced_msg_list ); 
				msg_link; msg_link = seq_sh.NextItem())
		{
			// resend if it takes longer than roughly twice the latency
			if(	( msg_link->m_Timestamp == 0 ) || 
				( m_Timestamp < msg_link->m_SendTime ) ||
				(( m_Timestamp - msg_link->m_Timestamp ) > (unsigned int) conn->GetResendThreshold()))
			{
				return true;		
			}
		}
	}

	return false;
}

/******************************************************************/
/* Dequeue Messages of type <msg_id>   							  */
/*                                                                */
/******************************************************************/

void App::DequeueMessagesByType( Net::Conn* conn, QUEUE_TYPE queue, unsigned char msg_id )
{
	

    switch( queue )
	{
		case QUEUE_DEFAULT:
		{
			Lst::Search< MsgLink > sh;
			MsgLink *msg_link, *next;
			QueuedMsg *msg;

			for( msg_link = sh.FirstItem( conn->m_normal_msg_list ); msg_link;
					msg_link = next )
			{
				next = sh.NextItem();
				msg = msg_link->m_QMsg;
				if( msg->m_MsgId == msg_id )
				{
					msg->m_Ref.Release();
					if( msg->m_Ref.InUse() == false )
					{
						delete msg;
					}
					delete msg_link;
				}
			}
			
			break;
		}

		case QUEUE_IMPORTANT:
		{
			Lst::Search< MsgImpLink > sh;
			MsgImpLink *msg_link, *next;
			QueuedMsg *msg;

			for( msg_link = sh.FirstItem( conn->m_important_msg_list ); msg_link;
					msg_link = next )
			{
				next = sh.NextItem();
				msg = msg_link->m_QMsg;
				if( msg->m_MsgId == msg_id )
				{
					msg->m_Ref.Release();
					if( msg->m_Ref.InUse() == false )
					{
						delete msg;
					}
					delete msg_link;
				}
			}
			break;
		}

		case QUEUE_SEQUENCED:
		{
			Lst::Search< MsgSeqLink > sh;
			MsgSeqLink *msg_link, *next;
			QueuedMsgSeq *msg;

			for( msg_link = sh.FirstItem( conn->m_sequenced_msg_list ); msg_link;
					msg_link = next )
			{
				next = sh.NextItem();
				msg = msg_link->m_QMsg;
				if( msg->m_MsgId == msg_id )
				{
					msg->m_Ref.Release();
					if( msg->m_Ref.InUse() == false )
					{
						delete msg;
					}
					delete msg_link;
				}
			}
			break;
		}
	}
}

/******************************************************************/
/* Enqueue a new message to be sent to a connection (by handle)   */
/*                                                                */
/******************************************************************/

void App::EnqueueMessage( int handle, MsgDesc* desc )
{
	Conn *conn;
	Lst::Search< Conn > sh;

#ifdef __PLAT_NGPS__
	WaitSema( m_send_semaphore_id );
#endif
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());
	// Make sure the message + headers isn't larger than a packet or else it will never be sent
	Dbg_MsgAssert( desc->m_Length < ( Manager::vMAX_PAYLOAD - 64 ), ("Network Message size (%d) is bigger than %d",desc->m_Length, Manager::vMAX_PAYLOAD - 64) );
	
	// Add new message to the appropriate queue	
	switch( desc->m_Queue )
	{
		case QUEUE_DEFAULT:
		{
			MsgLink *msg_link;
			QueuedMsg *queued_msg;
			
			queued_msg = new QueuedMsg( desc->m_Id, desc->m_Length, desc->m_Data );
			if( handle == HANDLE_ID_BROADCAST )
			{	
				for( conn = sh.FirstItem( m_connections ); conn; conn = sh.NextItem())
				{   // Only enqueue the message if the connection is ready and valid
					if( conn->GetStatus() == Conn::mSTATUS_READY )
					{
						if( desc->m_Singular )
						{
							DequeueMessagesByType( conn, desc->m_Queue, desc->m_Id );
						}

						queued_msg->m_Ref.Acquire();
						msg_link = new MsgLink( queued_msg );
						msg_link->SetPri( desc->m_Priority );
						msg_link->m_SendTime = m_Timestamp + desc->m_Delay;
						conn->m_normal_msg_list.AddNodeFromTail( msg_link );
					}
				}
			}
			else if( handle & HANDLE_ID_EXCLUDE_BROADCAST )
			{
				int excluded_handle;

				excluded_handle = handle & ~HANDLE_ID_EXCLUDE_BROADCAST;
				for( conn = sh.FirstItem( m_connections ); conn; conn = sh.NextItem())
				{
					// Broadcast to all but one handle
					if( conn->GetHandle() == excluded_handle )
					{
						continue;
					}
					// Only enqueue the message if the connection is ready and valid
					if( conn->GetStatus() == Conn::mSTATUS_READY )
					{
						if( desc->m_Singular )
						{
							DequeueMessagesByType( conn, desc->m_Queue, desc->m_Id );
						}
						queued_msg->m_Ref.Acquire();
						msg_link = new MsgLink( queued_msg );
						msg_link->SetPri( desc->m_Priority );
						msg_link->m_SendTime = m_Timestamp + desc->m_Delay;
						conn->m_normal_msg_list.AddNodeFromTail( msg_link );
					}
				}
			}
			else
			{
				conn = this->GetConnectionByHandle( handle );
				Dbg_Assert( conn );

				// Only enqueue the message if the connection is ready and valid
				if(	( conn->GetStatus() == Conn::mSTATUS_READY ) ||
					( desc->m_Id == MSG_ID_TIMESTAMP ))
				{
					if( desc->m_Singular )
					{
						DequeueMessagesByType( conn, desc->m_Queue, desc->m_Id );
					}
					queued_msg->m_Ref.Acquire();
					msg_link = new MsgLink( queued_msg );
					msg_link->SetPri( desc->m_Priority );
					msg_link->m_SendTime = m_Timestamp + desc->m_Delay;
					conn->m_normal_msg_list.AddNodeFromTail( msg_link );
				}
			}
			// If it didn't actually get enqueued, delete it
			if( queued_msg->m_Ref.InUse() == false )
			{
				delete queued_msg;
			}
			break;
		}

		case QUEUE_IMPORTANT:
		{
			MsgImpLink* msg_link;
			QueuedMsg* queued_msg;
			
			queued_msg = new QueuedMsg( desc->m_Id, desc->m_Length, desc->m_Data );
			if( handle == HANDLE_ID_BROADCAST )
			{	
				for( conn = sh.FirstItem( m_connections ); conn; conn = sh.NextItem())
				{
					if( !( conn->TestStatus( Conn::mSTATUS_DISCONNECTING )))
					{
						if( desc->m_Singular )
						{
							DequeueMessagesByType( conn, desc->m_Queue, desc->m_Id );
						}
						queued_msg->m_Ref.Acquire();
						msg_link = new MsgImpLink( queued_msg );
						msg_link->SetPri( desc->m_Priority );
						msg_link->m_SendTime = m_Timestamp + desc->m_Delay;
						msg_link->m_Timestamp = 0;
						conn->m_important_msg_list.AddNodeFromTail( msg_link ); 
					}
				}
			}
			else if( handle & HANDLE_ID_EXCLUDE_BROADCAST )
			{
				int excluded_handle;

				excluded_handle = handle & ~HANDLE_ID_EXCLUDE_BROADCAST;
				for( conn = sh.FirstItem( m_connections ); conn; conn = sh.NextItem())
				{
					// Broadcast to all but one handle
					if( conn->GetHandle() == excluded_handle )
					{
						continue;
					}
					if( !( conn->TestStatus( Conn::mSTATUS_DISCONNECTING )))
					{
						if( desc->m_Singular )
						{
							DequeueMessagesByType( conn, desc->m_Queue, desc->m_Id );
						}
						queued_msg->m_Ref.Acquire();
						msg_link = new MsgImpLink( queued_msg );
						msg_link->SetPri( desc->m_Priority );
						msg_link->m_SendTime = m_Timestamp + desc->m_Delay;
						msg_link->m_Timestamp = 0;
						conn->m_important_msg_list.AddNodeFromTail( msg_link ); 
					}
				}
			}
			else
			{
				conn = this->GetConnectionByHandle( handle );
				Dbg_Assert( conn );
			
				if( !( conn->TestStatus( Conn::mSTATUS_DISCONNECTING )))
				{
					if( desc->m_Singular )
					{
						DequeueMessagesByType( conn, desc->m_Queue, desc->m_Id );
					}
					queued_msg->m_Ref.Acquire();
					msg_link = new MsgImpLink( queued_msg );
					msg_link->SetPri( desc->m_Priority );
					msg_link->m_SendTime = m_Timestamp + desc->m_Delay;
					msg_link->m_Timestamp = 0;
					conn->m_important_msg_list.AddNodeFromTail( msg_link ); 
				}
			}
			// If it didn't actually get enqueued, delete it
			if( queued_msg->m_Ref.InUse() == false )
			{
				delete queued_msg;
			}
			break;
		}

		case QUEUE_SEQUENCED:
		{
			MsgSeqLink *msg_link;
			QueuedMsgSeq* queued_msg;
			
			queued_msg = AllocateNewSeqMessage( desc->m_Id, desc->m_Length, (char *) desc->m_Data, desc->m_GroupId );
						
			if( handle == HANDLE_ID_BROADCAST )
			{	
				for( conn = sh.FirstItem( m_connections ); conn; conn = sh.NextItem())
				{
					if( !( conn->TestStatus( Conn::mSTATUS_DISCONNECTING )))
					{
						Dbg_Assert( desc->m_Singular == false );
						
						queued_msg->m_Ref.Acquire();
						msg_link = new MsgSeqLink( queued_msg );
						msg_link->SetPri( desc->m_Queue );
						msg_link->m_SendTime = m_Timestamp + desc->m_Delay;
						msg_link->m_Timestamp = 0;
						msg_link->m_GroupId = desc->m_GroupId;
						if( desc->m_ForcedSequenceId > 0 )
						{
							msg_link->m_SequenceId = desc->m_ForcedSequenceId;
						}
						else
						{
							msg_link->m_SequenceId = conn->m_SequenceId[desc->m_GroupId]++;
						}
						
						msg_link->m_StreamMessage = desc->m_StreamMessage;
						conn->m_sequenced_msg_list.AddNodeFromTail( msg_link );
					}
				}
			}
			else if( handle & HANDLE_ID_EXCLUDE_BROADCAST )
			{
				int excluded_handle;

				excluded_handle = handle & ~HANDLE_ID_EXCLUDE_BROADCAST;
				for( conn = sh.FirstItem( m_connections ); conn; conn = sh.NextItem())
				{
					// Broadcast to all but one handle
					if( conn->GetHandle() == excluded_handle )
					{
						continue;
					}
					if( !( conn->TestStatus( Conn::mSTATUS_DISCONNECTING )))
					{
						Dbg_Assert( desc->m_Singular == false );

						queued_msg->m_Ref.Acquire();
						msg_link = new MsgSeqLink( queued_msg );
						msg_link->SetPri( desc->m_Priority );
						msg_link->m_SendTime = m_Timestamp + desc->m_Delay;
						msg_link->m_Timestamp = 0;
						msg_link->m_GroupId = desc->m_GroupId;
						if( desc->m_ForcedSequenceId > 0 )
						{
							msg_link->m_SequenceId = desc->m_ForcedSequenceId;
						}
						else
						{
							msg_link->m_SequenceId = conn->m_SequenceId[desc->m_GroupId]++;
						}
						msg_link->m_StreamMessage = desc->m_StreamMessage;
						conn->m_sequenced_msg_list.AddNodeFromTail( msg_link );
					}
				}
			}
			else
			{
				conn = GetConnectionByHandle( handle );
				Dbg_Assert( conn );

				if( !( conn->TestStatus( Conn::mSTATUS_DISCONNECTING )))
				{
					Dbg_Assert( desc->m_Singular == false );

					queued_msg->m_Ref.Acquire();
					msg_link = new MsgSeqLink( queued_msg );
					msg_link->SetPri( desc->m_Priority );
					msg_link->m_SendTime = m_Timestamp + desc->m_Delay;
					msg_link->m_Timestamp = 0;
					msg_link->m_GroupId = desc->m_GroupId;
					if( desc->m_ForcedSequenceId > 0 )
					{
						msg_link->m_SequenceId = desc->m_ForcedSequenceId;
						//Dbg_Printf( "Sending stream data sequence %d\n", msg_link->m_SequenceId );
					}
					else
					{
						msg_link->m_SequenceId = conn->m_SequenceId[desc->m_GroupId]++;
						//Dbg_Printf( "Sending stream start sequence %d\n", msg_link->m_SequenceId );
					}
					msg_link->m_StreamMessage = desc->m_StreamMessage;
					conn->m_sequenced_msg_list.AddNodeFromTail( msg_link );
				}
			}
			// If it didn't actually get enqueued, delete it
			if( queued_msg->m_Ref.InUse() == false )
			{
				delete queued_msg;
			}
			break;
		}

		default:
			Dbg_Assert( 0 );		// unsupported queue type
			break;

	}
	
	Mem::Manager::sHandle().PopContext();

#ifdef __PLAT_NGPS__
	SignalSema( m_send_semaphore_id );
#endif
}

/******************************************************************/
/* Stream a new message, given a connection instead of a handle	  */
/*                                              				  */
/******************************************************************/

void	App::StreamMessageToConn( Net::Conn* conn, unsigned char msg_id, unsigned short msg_len, void* data, char* desc, unsigned char group_id,
								  bool all_at_once, bool send_in_place )
{
	char* data_ptr;
	int size, packet_len;
	MsgStreamStart start_msg;
	MsgStreamData data_msg;
	MsgDesc msg_desc;
	StreamDesc* stream_desc;
	StreamLink* stream_link;

	Dbg_Assert( desc );
	Dbg_Assert( conn );

    // Also, for now, just split the data up into chunks no larger than MAX_STREAM_CHUNK_LENGTH
	// but eventually, maybe let the user decide what that chunk size should be.

    start_msg.m_Size = msg_len;
	start_msg.m_StreamId = conn->m_NextStreamId;
	start_msg.m_MsgId = msg_id;
	start_msg.m_Checksum = Crc::GenerateCRCCaseSensitive((char*) data, msg_len );
	strcpy( start_msg.m_StreamDesc, desc );
	size = 0;

	memcpy( start_msg.m_Data, data, ( msg_len <= MAX_STREAM_CHUNK_LENGTH ) ? msg_len : MAX_STREAM_CHUNK_LENGTH );
	size += ( msg_len <= MAX_STREAM_CHUNK_LENGTH ) ? msg_len : MAX_STREAM_CHUNK_LENGTH;
	packet_len = ( sizeof( start_msg ) - MAX_STREAM_CHUNK_LENGTH ) + size;

	msg_desc.m_Id = MSG_ID_STREAM_START;
	msg_desc.m_Length = packet_len;
	msg_desc.m_Data = &start_msg;
	msg_desc.m_Queue = QUEUE_SEQUENCED;
	msg_desc.m_GroupId = group_id;
	msg_desc.m_StreamMessage = 1;

	//Dbg_Printf( "******** [%d] Streaming %d bytes\n", conn->m_NextStreamId, packet_len );
    EnqueueMessage( conn->GetHandle(), &msg_desc );

	if( msg_len > MAX_STREAM_CHUNK_LENGTH )
	{
		if( all_at_once )
		{
			data_ptr = (char*) data + size;
			while( size < msg_len )
			{
				packet_len = msg_len - size;
				if( packet_len > MAX_STREAM_CHUNK_LENGTH )
				{
					packet_len = MAX_STREAM_CHUNK_LENGTH;
				}
		
				data_msg.m_StreamId = conn->m_NextStreamId;
				memcpy( data_msg.m_Data, data_ptr, packet_len );
				
				msg_desc.m_Id = MSG_ID_STREAM_DATA;
				msg_desc.m_Data = &data_msg;
				msg_desc.m_Length = packet_len + sizeof( int );
				msg_desc.m_Queue = QUEUE_SEQUENCED;
				msg_desc.m_GroupId = group_id;
				msg_desc.m_StreamMessage = 1;
		
				//Dbg_Printf( "%d ******** [%d] Streaming %d bytes.\n", m_FrameCounter, conn->m_NextStreamId, packet_len );
				EnqueueMessage( conn->GetHandle(), &msg_desc );
		
				data_ptr += packet_len;
				size += packet_len;
			}
		}
		else
		{
			// If this message is larger than one chunk, add it to our list of out-bound streams so that we can
			// send it to the connection with a sliding window
			int num_extra_msgs;
			char* send_data;
	
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());
	
			num_extra_msgs = ( msg_len - 1 ) / MAX_STREAM_CHUNK_LENGTH;
			
			if( send_in_place )
			{
				send_data = (char*) data;
			}
			else
			{
				send_data = new char[msg_len];
				memcpy( send_data, data, msg_len );
			}
				
			data_ptr = (char*) send_data + size;
	
			stream_desc = new StreamDesc;
			stream_link = new StreamLink( stream_desc );
		
			stream_desc->m_StreamId = conn->m_NextStreamId;
			stream_desc->m_Size = msg_len;
			stream_desc->m_MsgId = msg_id;
			strcpy( stream_desc->m_StreamDesc, desc );
			stream_desc->m_Data = (char*) send_data;
			stream_desc->m_DataPtr = data_ptr;
			stream_desc->m_GroupId = group_id;
			stream_desc->m_SequenceId = conn->m_SequenceId[group_id];
			stream_desc->m_SendInPlace = send_in_place;
	
			conn->m_StreamOutList.AddToTail( stream_link );
	
			//Dbg_Printf( "******** [%d] Stream rest (%d bytes, %d chunks) later....\n", conn->m_NextStreamId, msg_len - size, num_extra_msgs );
	
			conn->m_SequenceId[group_id] += num_extra_msgs;
			
			Mem::Manager::sHandle().PopContext();
		}
	}

	conn->m_NextStreamId++;
}

/******************************************************************/
/* Stream a new message to send to the connection.				  */
/*                                              				  */
/******************************************************************/

void	App::StreamMessage( int handle, unsigned char msg_id, unsigned short msg_len, void* data, char* desc, unsigned char group_id,
							bool all_at_once, bool send_in_place )
{
	Lst::Search< Conn > sh;
	Conn* conn;

	Dbg_Assert( desc );
	
	if( handle == HANDLE_ID_BROADCAST )
	{	
		for( conn = sh.FirstItem( m_connections ); conn; conn = sh.NextItem())
		{
			if( conn->IsValid())
			{
				StreamMessageToConn( conn, msg_id, msg_len, data, desc, group_id, all_at_once, send_in_place );
			}
		}
	}
	else
	{
		conn = GetConnectionByHandle( handle );
		Dbg_Assert( conn );

		if( conn->IsValid())
		{
			StreamMessageToConn( conn, msg_id, msg_len, data, desc, group_id, all_at_once, send_in_place );
		}
	}
}

/******************************************************************/
/* Stream a new message to send to the server. Just a shortcut	  */
/* to EnqueueMessage                                              */
/******************************************************************/

void	App::StreamMessageToServer( unsigned char msg_id, unsigned short msg_len, void* data, char* desc, unsigned char group_id,
									bool all_at_once, bool send_in_place )
{
	Conn *conn;
	Lst::Search< Conn > sh;

	if(( conn = FirstConnection( &sh )))
	{
		StreamMessageToConn( conn, msg_id, msg_len, data, desc, group_id, all_at_once, send_in_place );
	}
}

/******************************************************************/
/* Enqueue a new message to send to the server. Just a shortcut   */
/* to EnqueueMessage                                              */
/******************************************************************/

void	App::EnqueueMessageToServer( MsgDesc* desc )
{
	Conn *conn;
	Lst::Search< Conn > sh;

	

	if(( conn = FirstConnection( &sh )))
	{
		EnqueueMessage( conn->GetHandle(), desc );
	}
}

/******************************************************************/
/* Free the message queue for a given connection                  */
/*                                                                */
/******************************************************************/

void	App::FreeConnMessageQueue( Conn *conn, QUEUE_TYPE queue )
{
		
	

	Dbg_Assert( conn );

	switch( queue )
	{
		case QUEUE_DEFAULT:
		{
			Lst::Search< MsgLink > sh;
			MsgLink *msg_link, *next;
			QueuedMsg *msg;

			for( msg_link = sh.FirstItem( conn->m_normal_msg_list ); msg_link;
					msg_link = next )
			{
				next = sh.NextItem();
				msg = msg_link->m_QMsg;
				msg->m_Ref.Release();
				if( msg->m_Ref.InUse() == false )
				{
					delete msg;
				}
				delete msg_link;

			}
			
			break;
		}

		case QUEUE_IMPORTANT:
		{
			Lst::Search< MsgImpLink > sh;
			MsgImpLink *msg_link, *next;
			QueuedMsg *msg;

			for( msg_link = sh.FirstItem( conn->m_important_msg_list ); msg_link;
					msg_link = next )
			{
				next = sh.NextItem();
				msg = msg_link->m_QMsg;
				msg->m_Ref.Release();
				if( msg->m_Ref.InUse() == false )
				{
					delete msg;
				}
				delete msg_link;

			}
			break;
		}

		case QUEUE_SEQUENCED:
		{
			Lst::Search< MsgSeqLink > sh;
			MsgSeqLink *msg_link, *next;
			QueuedMsgSeq *msg;

			for( msg_link = sh.FirstItem( conn->m_sequenced_msg_list ); msg_link;
					msg_link = next )
			{
				next = sh.NextItem();
				msg = msg_link->m_QMsg;
				msg->m_Ref.Release();
				if( msg->m_Ref.InUse() == false )
				{
					delete msg;
				}
				delete msg_link;

			}
			break;
		}
	}
}

/******************************************************************/
/* Free message queues of a certain type from all connections     */
/*                                                                */
/******************************************************************/

void	App::FreeMessageQueue( QUEUE_TYPE queue )
{
	Conn *conn;
	Lst::Search< Conn > sh;

	for( conn = FirstConnection( &sh ); conn; conn = NextConnection( &sh ))
	{
		FreeConnMessageQueue( conn, queue );
	}
}

/******************************************************************/
/* Medium-level send function.  Works as the message level		  */
/*                                                                */
/******************************************************************/

bool	App::SendMessageTo( unsigned char msg_id, unsigned short msg_len, void* data,
							int ip, unsigned short port, int flags )
{
	char	msg_data[Manager::vMAX_PACKET_SIZE];
	bool result;

	msg_data[0] = msg_id;
	memcpy( &msg_data[1], &msg_len, sizeof( unsigned short ));
	if( msg_len > 0 )
	{
		memcpy( &msg_data[3], data, msg_len ); 
	}
    
#ifdef __PLAT_NGPS__	
	WaitSema( m_send_semaphore_id );
#endif // __PLAT_NGPS__
	
	result = SendTo( ip, port, msg_data, msg_len + Manager::vMSG_HEADER_LENGTH_WITH_SIZE, flags );

#ifdef __PLAT_NGPS__    
	SignalSema( m_send_semaphore_id );
#endif // __PLAT_NGPS__
	
	return result;
}

/******************************************************************/
/* Lower level, pre-connection network send function              */
/*                                                                */
/******************************************************************/

bool	App::SendTo( int ip, unsigned short port, char *data, int len, int flags )
{
	struct sockaddr_in	to_address;
#ifdef __PLAT_NGC__
    to_address.len = sizeof( sockaddr );
#else
	int addr_len = sizeof(to_address);
#endif
	int	result = 0;
	int send_len;
	
	

	Dbg_Assert( data );
	Dbg_Assert( len <= Manager::vMAX_PAYLOAD );

	// Send data immediately to a destination socket
	memset( &to_address, 0, sizeof(to_address));
	to_address.sin_family = AF_INET;
	to_address.sin_port = htons( port );
		
	// use ip = INADDR_BROADCAST to broadcast
	to_address.sin_addr.s_addr = ip;
	if( m_flags & mSECURE )
	{
		crc_and_copy_stream( data, m_out_packet_buffer, len, &send_len );
		result = sendto( m_socket, m_out_packet_buffer, send_len, flags, 
				(struct sockaddr *) &(to_address), addr_len );
	}
	else
	{
		result = sendto( m_socket, data, len, flags, (struct sockaddr *) &(to_address), addr_len );
	}
	
#ifndef __PLAT_NGC__
	if(	result == SOCKET_ERROR )
	{
		int err;
#if defined( __PLAT_WN32__ ) || defined( __PLAT_XBOX__ )
		err = WSAGetLastError();
		if(	( err != WSAEWOULDBLOCK ) &&
			( err != WSAEINPROGRESS ))
#else
#ifdef __PLAT_NGPS__
		err = sn_errno( m_socket );
		if( ( err != EWOULDBLOCK ) &&
			( err != EINPROGRESS ))
#endif
#endif
		{
#ifdef DEBUG_MESSAGES
		Dbg_Printf( "Sendto Error: packet length %d\n", len );
			ReportError();
#endif
			return false;
		}
	}
#endif

	return true;
}

/******************************************************************/
/* Lower level, pre-(app-level) connection network send function  */
/*                                                                */
/******************************************************************/

bool	App::Send( char *data, int len, int flags )
{
	int	result = 0;
	int send_len;
		
	

	Dbg_Assert( data );
	
	if( m_connected == false )
	{
		return false;
	}

	Dbg_Assert( len <= Manager::vMAX_PAYLOAD );

	// Send data immediately to a destination socket
	if( m_flags & mSECURE )
	{
		crc_and_copy_stream( data, m_out_packet_buffer, len, &send_len );
		result = send( m_socket, m_out_packet_buffer, send_len, flags );
	}
	else
	{
		result = send( m_socket, data, len, flags );
	}

	if( result == SOCKET_ERROR )
	{
#ifdef DEBUG_MESSAGES
		Dbg_Printf( "Send Error: packet length %d\n", len );
		ReportError();
#endif
		return false;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Conn	*App::FirstConnection( Lst::Search< Conn > *sh )
{
	

	Dbg_Assert( sh );

	return( sh->FirstItem( m_connections ));	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Conn	*App::NextConnection( Lst::Search< Conn > *sh )
{
	

	Dbg_Assert( sh );

	return( sh->NextItem());
}

/******************************************************************/
/* Immediately terminate connection.  Don't use this in msg		  */
/* handlers. Instead, invalidate the connection only              */
/******************************************************************/

void	App::TerminateConnection( Conn* conn )
{
	
	
	Dbg_Assert( conn );

	struct in_addr address;

	address.s_addr = conn->GetIP();
	conn->m_node.Remove();
	delete conn;
}

/******************************************************************/
/* Report the last error on the socket                            */
/*                                                                */
/******************************************************************/

void		App::ReportError( void )
{
	

#ifdef WIN32
	char msg[1024];

	//Dbg_Printf( "%s: Error %d\n", m_name, WSAGetLastError());
	sprintf( msg, "%s: Error %d\n", m_name, WSAGetLastError());
	OutputDebugString( msg );
#else
#ifdef __PLAT_NGPS__
#ifdef DEBUG_MESSAGES
	Dbg_Printf( "(%d) %s: Error %d\n", m_FrameCounter, m_name, sn_errno( m_socket ));
#endif
#endif
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager*	App::GetManager( void )
{
	return m_net_man;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		App::SendEnqueuedMessages( void )
{
	Conn *conn;
	Lst::Search< Conn > sh;
		
	
    
#ifdef __PLAT_NGPS__
    WaitSema( m_send_semaphore_id );
#endif	// __PLAT_NGPS__    
	for( conn = FirstConnection( &sh );
			conn; conn = NextConnection( &sh ))
	{
		SendEnqueuedMessages( conn );
	}
	
#ifdef __PLAT_NGPS__
	SignalSema( m_send_semaphore_id );
#endif	// __PLAT_NGPS__

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#ifdef __PLAT_NGPS__

void		App::TransferData( void )
{
	while( 1 )
	{
		// Receive data
		SleepThread();
		if( m_shutting_down )
		{
			break;
		}
		WaitSema( m_transfer_semaphore_id );
		ReceiveData();
		SignalSema( m_transfer_semaphore_id );
		
		// Now wait until that data has been processed and it is time to send new data
		// outbound
		SleepThread();

		// Send Data
		if( m_shutting_down )
		{
			break;
		}
		WaitSema( m_transfer_semaphore_id );
		SendEnqueuedMessages();
		SignalSema( m_transfer_semaphore_id );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		App::WaitForAsyncCallsToFinish( void )
{
	
    
	WaitForTransferSemaphore();
    SignalTransferSemaphore(); 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		App::WaitForTransferSemaphore( void )
{   
	WaitSema( m_transfer_semaphore_id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		App::SignalTransferSemaphore( void )
{   
	SignalSema( m_transfer_semaphore_id );
}

#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		App::SendData( bool scheduled_send )
{
	
	Lst::Search< Conn > sh;
	Conn* conn;

	// If someone is requesting a send of all curretly enqueued data before its time
	// we need to tell each connection to ignore the logic to send every N frames and rather
	// send immediately
	if( scheduled_send == false )
	{   
		m_Timestamp++;	// This will ensure that the timestamp is unique. Otherwise
						// the recipient will think it's a dupe packet
		for( conn = FirstConnection( &sh ); conn; conn = NextConnection( &sh ))
		{
			conn->SetForceSendThisFrame( true );
		}
	}

#ifdef __PLAT_NGPS__
	if( !IsLocal() && m_socket_thread_active )
	{   
		WaitSema( m_transfer_semaphore_id );
	}
#endif

	SendEnqueuedMessages();

#ifdef __PLAT_NGPS__
	if( !IsLocal() && m_socket_thread_active )
	{   
		SignalSema( m_transfer_semaphore_id );
	}
#endif	// __PLAT_NGPS_
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		App::ShouldSendThisFrame( Conn* conn )
{
	if( (( m_Timestamp - conn->m_last_send_time ) >= (unsigned int) conn->GetSendInterval()) ||
		( conn->GetForceSendThisFrame() ))
		
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	App::ShutDown( void )
{
	
#ifdef DEBUG_MESSAGES
	Dbg_Printf( "Shutting NetApp %s Down\n", GetName());
#endif

#ifdef __PLAT_NGPS__
	m_shutting_down = true;

	//if( m_socket_thread_active )
	{   
		WakeupThread( m_socket_thread_id );
	}
#ifdef DEBUG_MESSAGES	
	Dbg_Printf( "Waiting for active semaphore\n" );
#endif
	WaitSema( m_active_semaphore_id );

#ifdef DEBUG_MESSAGES
    Dbg_Printf( "Deleting semaphores\n" );
#endif
	DeleteSema( m_send_semaphore_id );
	DeleteSema( m_receive_semaphore_id );
	DeleteSema( m_transfer_semaphore_id );
	DeleteSema( m_active_semaphore_id );
	
#ifdef DEBUG_MESSAGES
	Dbg_Printf( "Deleting socket thread\n" );
#endif
	DeleteThread( m_socket_thread_id );
    
#endif
    
    deinit();
#ifdef DEBUG_MESSAGES
	Dbg_Printf( "NetApp %s successfully shut down\n", GetName());
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	App::AliasConnections( Conn* server_conn, Conn* client_conn )
{
	

	Dbg_Assert( server_conn->IsLocal() && client_conn->IsLocal());
	server_conn->m_alias_connection = client_conn;
	client_conn->m_alias_connection = server_conn;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	App::IsLocal( void )
{
	return m_flags.TestMask( mLOCAL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	App::AcceptForeignConnections( bool accept )
{
	if( accept )
	{
		m_flags.SetMask( mACCEPT_FOREIGN_CONN );
	}
	else
	{
		m_flags.ClearMask( mACCEPT_FOREIGN_CONN );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	App::AcceptsForeignConnections( void )
{
	return m_flags.TestMask( mACCEPT_FOREIGN_CONN );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

SOCKET	App::GetSocket( void )
{
	return m_socket;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	App::SetForeignPacketHandler( ForeignPacketHandlerCode* code )
{
	m_foreign_handler = code;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	App::BanConnection( Conn* conn )
{
	BannedConn* banned_conn;

	banned_conn = new BannedConn;

	banned_conn->m_Ip = conn->GetIP();
	m_banned_connections.AddToTail( banned_conn );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	App::IsConnectionBanned( Conn* conn )
{
	BannedConn* banned_conn;
	Lst::Search< BannedConn > sh;
	
	for( banned_conn = sh.FirstItem( m_banned_connections ); banned_conn;
			banned_conn = sh.NextItem())
	{
		if( banned_conn->m_Ip == conn->GetIP())
		{
			return true;
		}
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		App::BandwidthUsed( void )
{
	int total_bandwidth;
	Conn* conn;
	Lst::Search< Conn > sh;

	total_bandwidth = 0;
	for( conn = FirstConnection( &sh ); conn; conn = NextConnection( &sh ))
	{
		if( conn->IsRemote())
		{
			Metrics* metrics;
	
			metrics = conn->GetOutboundMetrics();
			total_bandwidth += metrics->GetBytesPerSec();
		}
	}

	return total_bandwidth;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	App::BandwidthExceeded( Conn* conn )
{
	Metrics* metrics;
	int total_bandwidth, per_user_bandwidth, num_remote_connections;
	Conn* tmp_conn;
	Lst::Search< Conn > sh;

    metrics = conn->GetOutboundMetrics();
	total_bandwidth = BandwidthUsed();
	// First, make sure we haven't exceeded the total bandwidth of our connection
	if( total_bandwidth >= m_net_man->GetBandwidth())
	{
		Dbg_Printf( "Total bandwidth exceeded : %d  %d\n", total_bandwidth, m_net_man->GetBandwidth() );
		return true;
	}

	// Then, make sure we haven't flooded the client
	if( metrics->GetBytesPerSec() >= conn->GetBandwidth())
	{
		Dbg_Printf( "(%d) Client flooded: %d %d\n", m_FrameCounter, metrics->GetBytesPerSec(), conn->GetBandwidth() );
		return true;
	}

	num_remote_connections = 0;
	for( tmp_conn = FirstConnection( &sh ); tmp_conn; tmp_conn = NextConnection( &sh ))
	{
		if( tmp_conn->IsRemote() && !tmp_conn->IsForeign())
		{
			num_remote_connections++;
		}
	}

	if( num_remote_connections == 0 )
	{
		num_remote_connections = 1;	// Avoid DBZ
	}
	// Also, check if we've used up this client's share of our bandwidth
	per_user_bandwidth = ( m_net_man->GetBandwidth() / num_remote_connections );
	if( metrics->GetBytesPerSec() > per_user_bandwidth )
	{
		Dbg_Printf( "Share flooded: %d %d\n",  metrics->GetBytesPerSec(), per_user_bandwidth );
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}	// namespace Net
