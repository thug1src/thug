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

#include <core/defines.h>
#include <core/support.h>

#include <sys/timer.h>

#include <stdio.h>

#include <gel/net/net.h>
#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

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

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/* Ack important messages                               		  */
/*                                                                */
/******************************************************************/

int	handle_ack( MsgHandlerContext *context )
{
	MsgImpLink *msg_imp_link, *next_imp;
	MsgSeqLink *msg_seq_link, *next_seq;
	Lst::Search< MsgImpLink > imp_sh;
	Lst::Search< MsgSeqLink > seq_sh;
	MsgAck *ack_msg;
	QueuedMsg* msg;
	QueuedMsgSeq* msg_seq;
	unsigned int send_time;
		
	

	Dbg_Assert( context );

	ack_msg = (MsgAck*) context->m_Msg;
	send_time = 0;

#ifdef DEBUG_MESSAGES
	if( context->m_Conn->IsRemote())
	{
		Dbg_Printf( "(%d) Conn %d: (%d) Got Ack for Packetstamp %d\n", 
			context->m_App->m_FrameCounter, 
			context->m_Conn->GetHandle(),
			context->m_App->m_Timestamp, 
			ack_msg->m_Packetstamp );
	}
#endif

#ifdef __PLAT_NGPS__
	/*if( context->m_Conn->GetHandle() != ack_msg->m_Handle )
	{
		Lst::Search< Conn > sh;
		Conn* conn;

		for( conn = context->m_App->FirstConnection( &sh ); conn; conn = context->m_App->NextConnection( &sh ))
		{
			Dbg_Printf( "*** Conn %d : %p. Ip: 0x%x Port: %d\n", conn->GetHandle(), conn, conn->GetIP(), conn->GetPort());
		}
	}

	Dbg_MsgAssert( context->m_Conn->GetHandle() == ack_msg->m_Handle, ("*** Different handles! %d %d\n", context->m_Conn->GetHandle(), ack_msg->m_Handle ));*/
#endif

	// Tell the dispatcher the size of this message because it does not know (optimization)
	context->m_MsgLength = sizeof( MsgAck );

	// Don't ack foreign acks. We only even handle them because we need to tell the dispatcher
	// the length of the message
	if( context->m_PacketFlags & mHANDLE_FOREIGN )
	{
	    return HANDLER_MSG_DONE;
	}

	for( msg_imp_link = imp_sh.FirstItem( context->m_Conn->m_important_msg_list ); 
			msg_imp_link; msg_imp_link = next_imp )
	{
		next_imp = imp_sh.NextItem();
		if(	( ack_msg->m_Packetstamp == (unsigned short) msg_imp_link->m_Packetstamp ) &&
			( msg_imp_link->m_Timestamp != 0 ))	// guard against acking packet stamp of zero where zero is the initialized value
		{
			msg = msg_imp_link->m_QMsg;
			msg->m_Ref.Release();
			if( msg->m_Ref.InUse() == false )
			{
				delete msg;
			}
			send_time = msg_imp_link->m_Timestamp;
			delete msg_imp_link;
		}
	}	

	for( msg_seq_link = seq_sh.FirstItem( context->m_Conn->m_sequenced_msg_list ); 
			msg_seq_link; msg_seq_link = next_seq )
	{
		next_seq = seq_sh.NextItem();
		// ACK the message
		if( ( ack_msg->m_Packetstamp == (unsigned short) msg_seq_link->m_Packetstamp ) &&
			( msg_seq_link->m_Timestamp != 0 ))	// guard against acking packet stamp of zero where zero is the initialized value
		{
			msg_seq = msg_seq_link->m_QMsg;
			msg_seq->m_Ref.Release();
			// If this message is no longer being referenced, free it
			if( msg_seq->m_Ref.InUse() == false )
			{
				delete msg_seq;
			}
			send_time = msg_seq_link->m_Timestamp;
			delete msg_seq_link;
		}
	}

	// If we can detect that the connection is starting to take longer to ack
	// our messages then affect his calculated latency so that we don't expect
	// acks back so quickly (which prompts resends )
	if(	( context->m_Conn->IsRemote()) &&
		( context->m_Conn->TestStatus( Conn::mSTATUS_READY )) &&
		( send_time > 0 ))
	{
		unsigned int latency_value;

		latency_value = context->m_App->m_Timestamp - send_time;
		if( latency_value > (unsigned int) context->m_Conn->GetAveLatency())
		{
            if( latency_value > App::MAX_LATENCY )
			{
				latency_value = App::MAX_LATENCY;
			}
			
			context->m_Conn->m_latency_test.InputLatencyValue( latency_value );
		}
	}

	return HANDLER_CONTINUE;
}

/******************************************************************/
/* Aggregate stream messages									  */
/*                                                                */
/******************************************************************/

int	App::handle_stream_messages( MsgHandlerContext *context )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	//Dbg_Printf( "******** Got Stream Message\n" );

	switch( context->m_MsgId )
	{
		case MSG_ID_STREAM_START:
		{
			//Dbg_Printf( "******** Got Stream Start\n" );
			MsgStreamStart* msg;
			StreamLink* str_link;
			StreamDesc* new_stream;
			Lst::Search< StreamLink > str_sh;
			int packet_len;

			msg = (MsgStreamStart*) context->m_Msg;
			
			for( str_link = str_sh.FirstItem( context->m_Conn->m_StreamInList ); str_link; 
					str_link = str_sh.NextItem())
			{
				StreamDesc* desc;

				desc = str_link->m_Desc;
				if( desc->m_StreamId == msg->m_StreamId )
				{
					Mem::Manager::sHandle().PopContext();
					return HANDLER_MSG_DONE;
				}
			}
            
			new_stream = new StreamDesc;
			strcpy( new_stream->m_StreamDesc, msg->m_StreamDesc );
			Dbg_Printf( "StreamDesc: %s\n", new_stream->m_StreamDesc );
			new_stream->m_Size = msg->m_Size;
			new_stream->m_StreamId = msg->m_StreamId;
			new_stream->m_Checksum = msg->m_Checksum;
			new_stream->m_MsgId = msg->m_MsgId;
			new_stream->m_Data = new char[ msg->m_Size ];
			new_stream->m_DataPtr = new_stream->m_Data;
			packet_len = ( msg->m_Size < MAX_STREAM_CHUNK_LENGTH ) ? msg->m_Size : MAX_STREAM_CHUNK_LENGTH;
			memcpy( new_stream->m_Data, msg->m_Data, packet_len );
			new_stream->m_DataPtr += packet_len;
			str_link = new StreamLink( new_stream );
			context->m_Conn->m_StreamInList.AddToTail( str_link );

			if(((unsigned int) new_stream->m_DataPtr - (unsigned int) new_stream->m_Data ) >= (unsigned int) new_stream->m_Size )
			{
				MsgHandlerContext msg_context;
				int result;

				// Shouldn't go over
				Dbg_Assert( ((unsigned int) new_stream->m_DataPtr - (unsigned int) new_stream->m_Data ) == (unsigned int) new_stream->m_Size );

				msg_context.m_Conn = context->m_Conn;
				msg_context.m_App = context->m_App;
				msg_context.m_PacketFlags = 0;
				msg_context.m_MsgId = new_stream->m_MsgId;
				msg_context.m_MsgLength = new_stream->m_Size;
				msg_context.m_Msg = new char[new_stream->m_Size];
				
                memcpy( msg_context.m_Msg, new_stream->m_Data, new_stream->m_Size );

				result = context->m_App->m_Dispatcher.DispatchMessage( &msg_context );
				if( result != HANDLER_MSG_DESTROYED )
				{
					delete str_link->m_Desc;
					delete str_link;
				}

				delete [] msg_context.m_Msg;
				Mem::Manager::sHandle().PopContext();
				return result;
			}
			break;
		}
		
		case MSG_ID_STREAM_DATA:
		{
			MsgStreamData* msg;
			StreamLink* str_link;
			Lst::Search< StreamLink > str_sh;

			msg = (MsgStreamData*) context->m_Msg;

			//Dbg_Printf( "******** Got Stream Data\n" );
			for( str_link = str_sh.FirstItem( context->m_Conn->m_StreamInList ); str_link; 
					str_link = str_sh.NextItem())
			{
				StreamDesc* desc;
				int packet_len;

				desc = str_link->m_Desc;
				if( desc->m_StreamId == msg->m_StreamId )
				{   
					packet_len = context->m_MsgLength - sizeof( int );	// subtract the size of the stream id
					memcpy( desc->m_DataPtr, msg->m_Data, packet_len );
					desc->m_DataPtr += packet_len;

					if(((unsigned int) desc->m_DataPtr - (unsigned int) desc->m_Data ) >= (unsigned int) desc->m_Size )
					{
						MsgHandlerContext msg_context;
						int result;
						uint32 actual_crc;

						// Shouldn't go over
						Dbg_Assert( ((unsigned int) desc->m_DataPtr - (unsigned int) desc->m_Data ) == (unsigned int) desc->m_Size );

						msg_context.m_Conn = context->m_Conn;
						msg_context.m_App = context->m_App;
						msg_context.m_PacketFlags = 0;
						msg_context.m_MsgId = desc->m_MsgId;
						msg_context.m_MsgLength = desc->m_Size;
						msg_context.m_Msg = new char[desc->m_Size];
						
						memcpy( msg_context.m_Msg, desc->m_Data, desc->m_Size );
						actual_crc = Crc::GenerateCRCCaseSensitive( msg_context.m_Msg, desc->m_Size );
						if( actual_crc != desc->m_Checksum )
						{
							msg_context.m_PacketFlags |= mHANDLE_CRC_MISMATCH;
							Dbg_Printf( "CRC MISMATCH!!!\n" );
						}

						Dbg_Printf( "handling stream: %s\n", desc->m_StreamDesc );
						result = context->m_App->m_Dispatcher.DispatchMessage( &msg_context );
						if( result != HANDLER_MSG_DESTROYED )
						{
							delete str_link->m_Desc;
							delete str_link;
						}

						delete [] msg_context.m_Msg;

						Mem::Manager::sHandle().PopContext();
						return result;
					}
				}
			}
			break;
		}
	}

	Mem::Manager::sHandle().PopContext();
	return HANDLER_CONTINUE;
}

/******************************************************************/
/* Put sequenced messages in their queue in order in their group  */
/*                                                                */
/******************************************************************/

int	App::handle_sequenced_messages( MsgHandlerContext *context )
{
	MsgSeqLink *msg_link;	
	char* data;
	unsigned short	msg_len;
	QueuedMsgSeq* queued_msg;
	unsigned char group_id, msg_id;	
	unsigned int seq_id;
	Lst::Search< MsgHandler > sh;
	
	

	Dbg_Assert( context );

	data = context->m_Msg;
	memcpy( &group_id, data, sizeof( char ));
	data += sizeof( char );
	memcpy( &seq_id, data, sizeof( unsigned int ));
	data += sizeof (unsigned int );	
		
	// don't accept old sequenced messages
	if( seq_id < context->m_Conn->m_WaitingForSequenceId[ group_id ] )
	{
		return HANDLER_CONTINUE;
	}
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	memcpy( &msg_id, data, sizeof( unsigned char ));
	data += sizeof( unsigned char );

	msg_len = context->m_MsgLength - Manager::vMSG_SEQ_HEADER_LENGTH;
	
	queued_msg = new QueuedMsgSeq( msg_id, msg_len, data, group_id );

	msg_link = new MsgSeqLink( queued_msg );
	msg_link->SetPri( seq_id );
	msg_link->m_Timestamp = 0;
	msg_link->m_Packetstamp = 0;
	msg_link->m_GroupId = group_id;
	msg_link->m_SequenceId = seq_id;
    
	Mem::Manager::sHandle().PopContext();

	// This might be dangerous in some apps, but it makes sense right now.  Don't add
	// this message to the sequenced queue if we dont have a handler for it.
	/*if( sh.FirstItem( context->m_App->m_Dispatcher.m_handler_list[msg_id] ) == NULL )
	{
		Dbg_MsgAssert( 0, ( "No handler for msg %d\n", msg_id ));
		delete msg_link->m_QMsg;
		delete msg_link;	
		return HANDLER_MSG_DONE;
	}*/
		// Don't re-add identical sequenced messages
	if( context->m_Conn->m_SequencedBuffer[group_id].AddUniqueSequence( msg_link ) == false )
	{
	    delete msg_link->m_QMsg;
		delete msg_link;
		return HANDLER_MSG_DONE;
	}

	return HANDLER_CONTINUE;
}

/******************************************************************/
/* Ack the packet immediately when we get it                      */
/*                                                                */
/******************************************************************/

int	Server::handle_timestamp( MsgHandlerContext *context )
{
	MsgDesc msg_desc;
	MsgPacketStamp *time_msg;
	MsgAck ack_msg;
	
	Dbg_Assert( context );

	// Tell the dispatcher the size of this message because it does not know (optimization)
	context->m_MsgLength = sizeof( MsgPacketStamp );

	// Don't ack foreign timestamps. We only even handle them because we need to tell the dispatcher
	// the length of the message
	if( context->m_PacketFlags & mHANDLE_FOREIGN )
	{
		return HANDLER_MSG_DONE;
	}
	
	time_msg = (MsgPacketStamp *) context->m_Msg;	
	ack_msg.m_Packetstamp = time_msg->m_Packetstamp;
	//ack_msg.m_Handle = time_msg->m_Handle;

#ifdef DEBUG_MESSAGES
	if( context->m_Conn->IsRemote())
	{
		Dbg_Printf( "(%d) Conn %d: (%d) Got Packetstamp %d\n", 
			context->m_App->m_FrameCounter, 
			context->m_Conn->GetHandle(),
			context->m_App->m_Timestamp, 
			time_msg->m_Packetstamp );
	}
#endif

	msg_desc.m_Data = &ack_msg;
	msg_desc.m_Length = sizeof( MsgAck );
	msg_desc.m_Id = MSG_ID_ACK;
	context->m_App->EnqueueMessage( context->m_Conn->GetHandle(), &msg_desc );
	
	return HANDLER_CONTINUE;
}

/******************************************************************/
/*     															  */
/*                                                         		  */
/******************************************************************/

int Server::handle_disconn_req( MsgHandlerContext* context )
{
	MsgDesc msg_desc;

#ifdef DEBUG_MESSAGES
	Dbg_Message( "Got disconnection request. Setting connection status to mSTATUS_DISCONNECTING\n" );
#endif
	// Don't re-disconnect someone
	if( context->m_Conn->TestStatus( Conn::mSTATUS_DISCONNECTING ))
	{
		return HANDLER_MSG_DONE;
	}
	// Enqueue a disconn acceptance message before the cutoff point 
	// for enqueuing messages( Conn::mSTATUS_DISCONNECTING )
    msg_desc.m_Id = MSG_ID_DISCONN_ACCEPTED;
	context->m_App->EnqueueMessage( context->m_Conn->GetHandle(), &msg_desc );
	context->m_Conn->SetStatus( Conn::mSTATUS_DISCONNECTING );
	return HANDLER_CONTINUE;
}

/******************************************************************/
/* Handle the timestamp from the server. Check for lost & late    */
/* packets                                                        */
/******************************************************************/

int	Client::handle_timestamp( MsgHandlerContext *context )
{
	MsgPacketStamp *time_msg;
	MsgAck ack_msg;
	MsgDesc msg_desc;

	Dbg_Assert( context );

	// Tell the dispatcher the size of this message because it does not know (optimization)
	context->m_MsgLength = sizeof( MsgPacketStamp );

	// Don't ack foreign timestamps. We only even handle them because we need to tell the dispatcher
	// the length of the message
	if( context->m_PacketFlags & mHANDLE_FOREIGN )
	{
		return HANDLER_MSG_DONE;
	}
    
	time_msg = (MsgPacketStamp *) context->m_Msg;
	ack_msg.m_Packetstamp = time_msg->m_Packetstamp;
	//ack_msg.m_Handle = time_msg->m_Handle;

#ifdef DEBUG_MESSAGES
	if( context->m_Conn->IsRemote())
	{
		Dbg_Printf( "(%d) Conn %d: (%d) Got Packetstamp %d\n", 
			context->m_App->m_FrameCounter, 
			context->m_Conn->GetHandle(),
			context->m_App->m_Timestamp, 
			time_msg->m_Packetstamp );
	}
#endif
	
	msg_desc.m_Data = &ack_msg;
	msg_desc.m_Length = sizeof( MsgAck );
	msg_desc.m_Id = MSG_ID_ACK;
	context->m_App->EnqueueMessageToServer( &msg_desc );

	// Todo : Change this logic as packetstamps DO wraparound
	// if == 0, we just connected : accept first timestamp unconditionally
	if( context->m_Conn->m_latest_packet_stamp == 0 )
	{
		context->m_Conn->m_latest_packet_stamp = time_msg->m_Packetstamp;
	}
	else
	{
		// Check for late packets, but also make sure it's actually late, and not just wraparound
		if( time_msg->m_Packetstamp < context->m_Conn->m_latest_packet_stamp )
		{
			if(( context->m_Conn->m_latest_packet_stamp - time_msg->m_Packetstamp ) > 5000 )
			{
				context->m_Conn->m_latest_packet_stamp = time_msg->m_Packetstamp;
			}
			else
			{
				context->m_App->m_LatePackets++;
				context->m_App->m_LostPackets--;
				context->m_PacketFlags |= mHANDLE_LATE;
			}
		}
		else if( time_msg->m_Packetstamp == context->m_Conn->m_latest_packet_stamp )
		{
			context->m_App->m_DupePackets++;
			Dbg_Printf( "**** Got Duplicate Packet: %d\n", context->m_App->m_DupePackets ); 
			return HANDLER_ERROR;	// don't process dupes
		}
		else if(( time_msg->m_Packetstamp - context->m_Conn->m_latest_packet_stamp ) > 1 )
		{
			// Check for lost packets and make sure it's not just wraparound
			if(( time_msg->m_Packetstamp - context->m_Conn->m_latest_packet_stamp ) > 5000 )
			{
				context->m_App->m_LatePackets++;
				context->m_App->m_LostPackets--;
				context->m_PacketFlags |= mHANDLE_LATE;
			}
			else
			{
				context->m_App->m_LostPackets += 
						(( time_msg->m_Packetstamp - context->m_Conn->m_latest_packet_stamp ) - 1 );
				context->m_Conn->m_latest_packet_stamp = time_msg->m_Packetstamp;  
			}
		}
		else
		{
			context->m_Conn->m_latest_packet_stamp = time_msg->m_Packetstamp;  
		}
	}

	return HANDLER_CONTINUE;
}

/******************************************************************/
/* Respond to ping test                                           */
/*                                                                */
/******************************************************************/

int	handle_latency_test( MsgHandlerContext *context )
{
	MsgDesc msg_desc;
	MsgTimestamp latency_response_msg;
    MsgTimestamp *time_msg;
	Conn *conn;

	Dbg_Assert( context );
    
	time_msg = (MsgTimestamp *) context->m_Msg;
	conn = context->m_Conn;
    
	// "Pong" back to the ping
	latency_response_msg.m_Timestamp = time_msg->m_Timestamp;

	msg_desc.m_Data = &latency_response_msg;
	msg_desc.m_Length = sizeof( MsgTimestamp );
	msg_desc.m_Id = MSG_ID_PING_RESPONSE;
	context->m_App->EnqueueMessage( conn->GetHandle(), &msg_desc );   
    
	return HANDLER_CONTINUE;
}

/******************************************************************/
/* Take note of latency value                                     */
/*                                                                */
/******************************************************************/

int	handle_latency_response( MsgHandlerContext *context )
{
	MsgTimestamp *time_msg;
	Conn *conn;
	int cur_time;

	Dbg_Assert( context );

	time_msg = (MsgTimestamp *) context->m_Msg;
	conn = context->m_Conn;

	if( conn->IsLocal())
	{
		return HANDLER_CONTINUE;
	}
	
	cur_time = context->m_App->m_Timestamp;

	// if it's the one we're waiting for
	if( time_msg->m_Timestamp == conn->m_latency_test.m_SendTime )
	{   
		unsigned int latency_value;

		conn->m_latency_test.m_ReceiveTime = time_msg->m_Timestamp;
		latency_value = ( cur_time - time_msg->m_Timestamp );
		if( latency_value > App::MAX_LATENCY )
		{
			latency_value = App::MAX_LATENCY;
		}
		conn->m_latency_test.InputLatencyValue( latency_value );		
	}

	return HANDLER_CONTINUE;
}

/******************************************************************/
/* We've been accepted to the server                              */
/*                                                                */
/******************************************************************/

int	App::handle_connection_accepted( MsgHandlerContext *context )
{
	
#ifdef DEBUG_MESSAGES
	Dbg_Message( "Connection was accepted!\n" );
#endif
	return HANDLER_CONTINUE;
}

/******************************************************************/
/* The server rejected our connection request (app_level)         */
/*                                                                */
/******************************************************************/

int	Client::handle_connection_refusal( MsgHandlerContext *context )
{
	Client *client;

	

	Dbg_Assert( context );

	client = (Client *) context->m_App;
	// disconnect
	//client->ConnectToServer( INADDR_ANY, 0 );
	client->TerminateConnection( context->m_Conn );

	return HANDLER_HALT;
}

/******************************************************************/
/* Someone is requesting connection (app-level)                   */
/*                                                                */
/******************************************************************/

int	App::handle_connection_request( MsgHandlerContext *context )
{
	MsgDesc msg_desc;

	Dbg_Assert( context );

#ifdef DEBUG_MESSAGES
	Dbg_Message( "Got Connection Request!\n" );
#endif

	// Redundancy check
    if( context->m_PacketFlags & mHANDLE_FOREIGN )
	{
		int ip;

		if( context->m_App->AcceptsForeignConnections() == false )
		{
			return HANDLER_MSG_DONE;
		}

		ip = context->m_Conn->GetIP();
        
#ifndef __PLAT_NGC__
		/*Dbg_Message( "Accepting connection from IP:%s Port:%d\n", 
					 inet_ntoa( *(struct in_addr *) &ip), 
					 context->m_Conn->GetPort());*/
#endif		// __PLAT_NGC__
		context->m_App->SendMessageTo( MSG_ID_CONNECTION_ACCEPTED, 0, NULL,
									   context->m_Conn->GetIP(), context->m_Conn->GetPort(), 0 );

		return HANDLER_CONTINUE;
	}
	else if( context->m_Conn->IsLocal())
	{
		msg_desc.m_Id = MSG_ID_CONNECTION_ACCEPTED;
		msg_desc.m_Priority = HIGHEST_PRIORITY;
		msg_desc.m_Queue = QUEUE_IMPORTANT;
		context->m_App->EnqueueMessage( context->m_Conn->GetHandle(), &msg_desc );

		return HANDLER_CONTINUE;
	}
	
	return HANDLER_MSG_DONE;
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}	// namespace Net
