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
**	File name:		netdsptch.cpp											**
**																			**
**	Created:		01/29/01	-	spg										**
**																			**
**	Description:	Network Dispatching code								**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <string.h>
#include <gel/net/net.h>

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

//#define NET_PRINT_MESSAGES 1 
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

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

MsgHandler::MsgHandler( MsgHandlerCode *code, int flags, void *data, int pri )
: Lst::Node< MsgHandler >( this, pri ), m_code( code ), m_data( data ), m_flags( flags )
{   
}

/******************************************************************/
/* Register a handler for a net msg                               */
/*                                                                */
/******************************************************************/

MsgHandler*	Dispatcher::AddHandler( unsigned char net_msg_id, MsgHandlerCode *code, int flags, 
						void *data, int pri )
{	
	MsgHandler* handler;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	handler = new MsgHandler( code, flags, data, pri );

	m_handler_list[net_msg_id].AddNode( handler );

	Mem::Manager::sHandle().PopContext();

	return handler;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Dispatcher::Init( void )
{
	int i;

	for( i = 0; i < MAX_NET_MSG_ID; i++ )
	{
		m_handler_list[i].DestroyAllNodes();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Dispatcher::Deinit( void )
{
	int i;

	

	for( i = 0; i < MAX_NET_MSG_ID; i++ )
	{   
		Lst::Search< MsgHandler > sh;
		MsgHandler* handler, *next;
		
		for( handler = sh.FirstItem( m_handler_list[i] ); handler;
				handler = next )
		{
			next = sh.NextItem();

			delete handler;
		}
	}
}

/******************************************************************/
/* Dispatch messages to their appropriate handlers                */
/*                                                                */
/******************************************************************/

int	Dispatcher::DispatchMsgHandlers( Conn *conn, int flags )
{
	MsgHandlerContext msg_context;
	char msg_data[MAX_STREAM_LENGTH];
	int result;
	char *data;

	Dbg_Assert( conn != NULL );	

	msg_context.m_Conn = conn;
	msg_context.m_App = conn->m_app;
	msg_context.m_PacketFlags = flags;
	msg_context.m_Msg = msg_data;
	msg_context.m_MsgLength = 0;

	data = conn->m_read_buffer;		

	// loop through received data and handle messages
	while( data < conn->m_read_ptr )
	{		
		bool size_known;

        // *** IMPORTANT.  Because of alignment issues, I now copy the data from the byte stream
		// into an aligned message buffer.  Indexing into the buffer and casting to Msg's caused
		// load errors
		memcpy( &msg_context.m_MsgId, data, sizeof( unsigned char ));
		data += sizeof( unsigned char );

		// Check to see if the size of this message type is stored after the msg id
		size_known = (( conn->m_app->GetManager()->GetMessageFlags( msg_context.m_MsgId ) & mMSG_SIZE_UNKNOWN ) == 0 );
		//if( conn->IsRemote())
		//{
			//Dbg_Printf( "Got message %s with size_known of %d\n", conn->m_app->GetManager()->GetMessageName(msg_context.m_MsgId ), size_known);
		//}
		
		if( size_known )
		{
			unsigned short temp;
			memcpy( &temp, data, sizeof( unsigned short ));
			msg_context.m_MsgLength = (unsigned long)temp;

			data += sizeof( unsigned short );		
		}
        
		if( msg_context.m_MsgId == MSG_ID_SEQUENCED )
		{
			unsigned char embedded_msg_id;
			unsigned short embedded_msg_length;
            
			Dbg_Assert( size_known );

			memcpy( &embedded_msg_id, data + 5, sizeof( unsigned char ));
			embedded_msg_length = msg_context.m_MsgLength - Manager::vMSG_SEQ_HEADER_LENGTH;
						
			conn->GetInboundMetrics()->AddMessage( embedded_msg_id, msg_context.m_MsgLength );
#ifdef NET_PRINT_MESSAGES
			if( conn->IsRemote())
			{
				unsigned char group_id;
				unsigned int seq_id;

				memcpy( &group_id, data, sizeof( unsigned char ));
				memcpy( &seq_id, data + 1, sizeof( unsigned int ));
				Dbg_Printf( "(%d) Dispatching Sequenced Message (%d) Length (%d) Emb. Length (%d) : %s. Seq: %d Group: %d\n", 
							conn->m_app->m_FrameCounter,
							embedded_msg_id,
							msg_context.m_MsgLength,
							embedded_msg_length,
							conn->m_app->GetManager()->GetMessageName( embedded_msg_id ),
							seq_id,
							group_id );
			}
#endif NET_PRINT_MESSAGES
		}
		
		// If we know the size, just copy the message into the aligned buffer
		// otherwise, copy the maximum size of a message into the buffer and have the
		// handler tell us the size of the message by changint the context's m_MsgLength member
		if( size_known )
		{
			if( msg_context.m_MsgLength > 0 )
			{
				memcpy( msg_context.m_Msg, data, msg_context.m_MsgLength );
			}
		}
		else
		{
			memcpy( msg_context.m_Msg, data, Manager::vMAX_PACKET_SIZE );
		}

		if( conn->IsForeign())
		{
			msg_context.m_PacketFlags |= mHANDLE_FOREIGN;
		}
		result = DispatchMessage( &msg_context );
		if(	( result == HANDLER_ERROR ) ||
			( result == HANDLER_HALT ))
		{
			// output error indicating mal-formmated data
			Dbg_Printf( "Stream processing halted on message %d\n", msg_context.m_MsgId );
			conn->m_read_ptr = conn->m_read_buffer;
			return HANDLER_HALT;
		}

		// By this time we know the message size, whether it was explicit or implicit
		// so we can do the metrics calculation and the debug printout
		if( msg_context.m_MsgId != MSG_ID_SEQUENCED )
		{
			conn->GetInboundMetrics()->AddMessage( msg_context.m_MsgId, msg_context.m_MsgLength );
#ifdef NET_PRINT_MESSAGES
			/*if( conn->IsRemote())
			{
				Dbg_Printf( "(%d) Dispatching Message (%d) Length (%d) : %s\n", 
							conn->m_app->m_FrameCounter,
							msg_context.m_MsgId,
							msg_context.m_MsgLength,
							conn->m_app->GetManager()->GetMessageName( msg_context.m_MsgId ));
			}*/
#endif
		}

		data += msg_context.m_MsgLength;
	}

	conn->m_read_ptr = conn->m_read_buffer;
	return HANDLER_CONTINUE;
}

/******************************************************************/
/* Dispatch a message to its appropriate handler. Stop if higher  */
/* priority handlers say to                                       */
/******************************************************************/

int	Dispatcher::DispatchMessage( MsgHandlerContext *context )
{
	Lst::Search< MsgHandler > sh;
	MsgHandler *handler, *next;
	int result;

	

	Dbg_Assert( context );

    // If this is a message we want to handle, call the handler
	for( handler = sh.FirstItem( m_handler_list[context->m_MsgId] );
			handler; handler = next )
	{
		next = sh.NextItem();

		if(( handler->m_flags & context->m_PacketFlags ) != context->m_PacketFlags )
		{
			Dbg_Printf( "*** Conn %d packetflag mismatch on id %d : 0x%x 0x%x\n", context->m_Conn->GetHandle(), context->m_MsgId, handler->m_flags, context->m_PacketFlags );
			continue;
		}
		
		context->m_Data = handler->m_data;
		result = handler->m_code( context );
		// Check the result from the handler
		if(	( result == HANDLER_ERROR ) ||
			( result == HANDLER_HALT ) ||
			( result == HANDLER_MSG_DESTROYED ))
		{
			// Stop processing the buffer
			return result;
		}		
		// A higher int handler has chosen to mask this message from the rest
		if( ( result == HANDLER_MSG_DONE ))
		{
			return HANDLER_CONTINUE;
		}
	}

	return HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}	// namespace Net