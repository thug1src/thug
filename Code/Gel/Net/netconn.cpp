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
**	File name:		netconn.cpp												**
**																			**
**	Created:		08/09/01	-	spg										**
**																			**
**	Description:	Network Connection code									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <stdlib.h>
#include <string.h>

#include <gel/net/net.h>
#include <gel/net/server/netserv.h>
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

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

Conn::Conn( int flags )
: m_node( this )
{
	m_flags = flags;
	m_status.ClearAll();
	m_status.SetMask( mSTATUS_READY );
	m_read_buffer = new char[ vREAD_BUFFER_LENGTH ];	
	m_read_ptr = m_read_buffer;
	m_write_buffer = new char[ Manager::vMAX_PACKET_SIZE ];
	m_write_ptr = m_write_buffer;
	m_send_interval = 0;
	m_last_send_time = 0;
	m_force_send_packet = false;
	m_alias_connection = NULL;
	m_latest_packet_stamp = 0;
	m_latest_sent_packet_stamp = 0;
#ifdef __PLAT_NGPS__
	m_bandwidth = 4200;	// Start with a 33.6kbps modem's payload threshold (i.e. including packet overhead)
#else
	m_bandwidth = 400000;	// On xbox, just assume broadband
#endif
	
	m_NextStreamId = 0;
	memset( m_SequenceId, 0, MAX_SEQ_GROUPS );	
	memset( m_WaitingForSequenceId, 0, MAX_SEQ_GROUPS );
#ifdef	__PLAT_NGPS__											  
	Dbg_MsgAssert(Mem::SameContext(this,Mem::Manager::sHandle().NetworkHeap()),("Conn not on network heap"));	
#endif		//	__PLAT_NGPS__		
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Conn::~Conn( void )
{
	DestroyAllMessageData();
    
	delete [] m_read_buffer;
	delete [] m_write_buffer;
}

/******************************************************************/
/*  Destroy pending sequenced messages							  */
/*                                                                */
/******************************************************************/

void 	Conn::destroy_pending_sequenced_messages( void )
{
	int i;
	Lst::Search< MsgSeqLink > sh;
    MsgSeqLink *msg_link, *next;
    
	

    for( i = 0; i < MAX_SEQ_GROUPS; i++ )
	{
        for( msg_link = sh.FirstItem( m_SequencedBuffer[i] );
				msg_link; msg_link = next )
		{
			next = sh.NextItem();
			
			delete msg_link->m_QMsg;
			delete msg_link;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Conn::DestroyAllMessageData( void )
{
	destroy_pending_sequenced_messages();

	AckAllMessages();
	DestroyMessageQueues();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Conn::DestroyMessageQueues( void )
{
	int i;
	Lst::Search< MsgLink >	msg_sh;
	Lst::Search< MsgImpLink >	imp_sh;
	Lst::Search< MsgSeqLink >	seq_sh;
	Lst::Search< StreamLink > stream_sh;

	MsgLink *msg_link, *next_msg_link;
	MsgImpLink *imp_link, *next_imp_link;
	MsgSeqLink *seq_link, *next_seq_link;
	StreamLink* str_link, *next_str_link;

	for( msg_link = msg_sh.FirstItem( m_normal_msg_list ); msg_link; msg_link = next_msg_link )
	{
		next_msg_link = msg_sh.NextItem();
        
		delete msg_link;
	}

	for( imp_link = imp_sh.FirstItem( m_important_msg_list ); imp_link; imp_link = next_imp_link )
	{
		next_imp_link = imp_sh.NextItem();
		
		delete imp_link;
	}

	for( seq_link = seq_sh.FirstItem( m_sequenced_msg_list ); seq_link; seq_link = next_seq_link )
	{
		next_seq_link = seq_sh.NextItem();
		
		delete seq_link;
	}

	for( i = 0; i < MAX_SEQ_GROUPS; i++ )
	{
		for( seq_link = seq_sh.FirstItem( m_SequencedBuffer[i] ); seq_link; seq_link = next_seq_link )
		{
			next_seq_link = seq_sh.NextItem();
			
			delete seq_link;
		}
	}

	//Dbg_Printf( "** Destroying StreamIn list\n" );
	for( str_link = stream_sh.FirstItem( m_StreamInList ); str_link; str_link = next_str_link )
	{
		next_str_link = stream_sh.NextItem();

		delete str_link->m_Desc;
		delete str_link;
	}

	//Dbg_Printf( "** Destroying StreamOut list\n" );
	for( str_link = stream_sh.FirstItem( m_StreamOutList ); str_link; str_link = next_str_link )
	{
		next_str_link = stream_sh.NextItem();

		delete str_link->m_Desc;
		delete str_link;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Conn::DestroyImportantMessageQueues( void )
{
	MsgImpLink *msg_imp_link, *next_imp;
	MsgSeqLink *msg_seq_link, *next_seq;
	Lst::Search< MsgLink > msg_sh;
	Lst::Search< MsgImpLink > imp_sh;
	Lst::Search< MsgSeqLink > seq_sh;
	QueuedMsg* msg;
	QueuedMsgSeq* msg_seq;
		
	for( msg_imp_link = imp_sh.FirstItem( m_important_msg_list ); 
			msg_imp_link; msg_imp_link = next_imp )
	{
		next_imp = imp_sh.NextItem();
		msg = msg_imp_link->m_QMsg;
		msg->m_Ref.Release();
		if( msg->m_Ref.InUse() == false )
		{
			delete msg;
		}
		
		delete msg_imp_link;
	}	

	for( msg_seq_link = seq_sh.FirstItem( m_sequenced_msg_list ); 
			msg_seq_link; msg_seq_link = next_seq )
	{
		next_seq = seq_sh.NextItem();
		// ACK the message
		msg_seq = msg_seq_link->m_QMsg;
		msg_seq->m_Ref.Release();

		Dbg_Printf( "*** Destroying seq message %d, still on queue\n", msg_seq->m_MsgId );
		// If this message is no longer being referenced, free it
		if( msg_seq->m_Ref.InUse() == false )
		{
			delete msg_seq;
		}

		delete msg_seq_link;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Conn::AckAllMessages( void )
{
	MsgLink* msg_link, *next_msg_link;
	MsgImpLink *msg_imp_link, *next_imp;
	MsgSeqLink *msg_seq_link, *next_seq;
	Lst::Search< MsgLink > msg_sh;
	Lst::Search< MsgImpLink > imp_sh;
	Lst::Search< MsgSeqLink > seq_sh;
	QueuedMsg* msg;
	QueuedMsgSeq* msg_seq;
		
	for( msg_link = msg_sh.FirstItem( m_normal_msg_list ); msg_link; msg_link = next_msg_link )
	{
		next_msg_link = msg_sh.NextItem();
		msg = msg_link->m_QMsg;
		msg->m_Ref.Release();
		if( msg->m_Ref.InUse() == false )
		{
			delete msg;
		}
	}

	for( msg_imp_link = imp_sh.FirstItem( m_important_msg_list ); 
			msg_imp_link; msg_imp_link = next_imp )
	{
		next_imp = imp_sh.NextItem();
		msg = msg_imp_link->m_QMsg;
		msg->m_Ref.Release();
		if( msg->m_Ref.InUse() == false )
		{
			delete msg;
		}
	}	

	for( msg_seq_link = seq_sh.FirstItem( m_sequenced_msg_list ); 
			msg_seq_link; msg_seq_link = next_seq )
	{
		next_seq = seq_sh.NextItem();
		// ACK the message
		msg_seq = msg_seq_link->m_QMsg;
		msg_seq->m_Ref.Release();
		// If this message is no longer being referenced, free it
		if( msg_seq->m_Ref.InUse() == false )
		{
			delete msg_seq;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Tmr::Time	Conn::GetLastCommTime( void )
{
	if( IsLocal())
	{
		return Tmr::GetTime();
	}
	return m_last_comm_time;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Conn::UpdateCommTime( Tmr::Time extra_time )
{
	Tmr::Time cur_time;

	cur_time = Tmr::GetTime();
	if(( cur_time + extra_time ) > m_last_comm_time )
	{
		m_last_comm_time = cur_time + extra_time;
	}
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Tmr::Time	Conn::GetTimeElapsedSinceCommunication( void )
{
	Tmr::Time cur_time;

	cur_time = Tmr::GetTime();

	if( cur_time < m_last_comm_time )
	{
		return 0;
	}

    return cur_time - m_last_comm_time;
}

/******************************************************************/
/* Used to invalidate a connection. This way, you can defer the   */
/* Destruction of the connection until later, but ignore it now   */
/******************************************************************/

void	Conn::Invalidate( void )
{
	//Dbg_Message( "Invalidating Connection\n" );
	ClearStatus( mSTATUS_READY );
	SetStatus( mSTATUS_INVALID );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Conn::IsValid( void )
{
	

	return( TestStatus( mSTATUS_INVALID ) == false );
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Conn::IsForeign( void )
{
	return m_flags.TestMask( mFOREIGN );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Conn::IsLocal( void )
{
	return m_flags.TestMask( mLOCAL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Conn::IsRemote( void )
{
	return m_flags.TestMask( mREMOTE );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Conn::GetResendThreshold( void )
{
	int threshold;
	 
	// Don't resend as often if the client is busy
	if( TestStatus( mSTATUS_BUSY ))
	{
		return Tmr::Seconds( 1 );
	}

	// by default, the resend threshold is two times your average latency
	threshold = 2 * GetAveLatency();

	// We have a threshold for resending.  So that clients don't spam the server with resends
	// while he's busy
	if( threshold < Manager::vMINIMUM_RESEND_THRESHOLD )
	{
		return Manager::vMINIMUM_RESEND_THRESHOLD;
	}

	return threshold;
}

/******************************************************************/
/* Send every N milliseconds                                  	  */
/*                                                                */
/******************************************************************/

void	Conn::SetSendInterval( int interval )
{
	m_send_interval = interval;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

int		Conn::GetSendInterval( void )
{
	return m_send_interval;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

void	Conn::SetForceSendThisFrame( bool force_send )
{
	m_force_send_packet = force_send;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

bool	Conn::GetForceSendThisFrame( void )
{
	return m_force_send_packet;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

int		Conn::GetNumResends( void )
{
	MsgImpLink *msg_imp_link;
	MsgSeqLink *msg_seq_link;
	Lst::Search< MsgImpLink > imp_sh;
	Lst::Search< MsgSeqLink > seq_sh;
	int highest_num_resends;
    
	

	highest_num_resends = 0;

	for( msg_imp_link = imp_sh.FirstItem( m_important_msg_list ); 
			msg_imp_link; msg_imp_link = imp_sh.NextItem())
	{
		if( msg_imp_link->m_NumResends > highest_num_resends )
		{
			highest_num_resends = msg_imp_link->m_NumResends;
		}
	}	

	for( msg_seq_link = seq_sh.FirstItem( m_sequenced_msg_list ); 
			msg_seq_link; msg_seq_link = seq_sh.NextItem())
	{
		if( msg_seq_link->m_NumResends > highest_num_resends )
		{
			highest_num_resends = msg_seq_link->m_NumResends;
		}
	}

	return highest_num_resends;
}

/******************************************************************/
/* Get the number of stream messages still pending for this conn. */
/* If num_pending > our sliding window's size, don't send any more*/
/******************************************************************/

int		Conn::GetNumPendingStreamMessages( void )
{
	MsgSeqLink *msg_seq_link;
	Lst::Search< MsgSeqLink > seq_sh;
	int num_pending;

	num_pending = 0;
	for( msg_seq_link = seq_sh.FirstItem( m_sequenced_msg_list ); msg_seq_link; msg_seq_link = seq_sh.NextItem())
	{
		if( msg_seq_link->m_StreamMessage )
		{
			num_pending++;
		}
	}

	return num_pending;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

void	Conn::ClearNumResends( void )
{
	MsgImpLink *msg_imp_link;
	MsgSeqLink *msg_seq_link;
	Lst::Search< MsgImpLink > imp_sh;
	Lst::Search< MsgSeqLink > seq_sh;
	
	

    for( msg_imp_link = imp_sh.FirstItem( m_important_msg_list ); 
			msg_imp_link; msg_imp_link = imp_sh.NextItem())
	{
		msg_imp_link->m_NumResends = 0;
	}	

	for( msg_seq_link = seq_sh.FirstItem( m_sequenced_msg_list ); 
			msg_seq_link; msg_seq_link = seq_sh.NextItem())
	{
		msg_seq_link->m_NumResends = 0;
	}
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

int		Conn::GetBandwidthType( void )
{
	return m_bandwidth_type;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

void	Conn::SetBandwidthType( int type )
{
	m_bandwidth_type = type;
	
	// Some default values for bandwidth limiting
	switch( m_bandwidth_type )
	{
		case vNARROWBAND:
			SetBandwidth( 4200 );	
			break;
		case vBROADBAND:
			SetBandwidth( 400000 );
			break;
		case vLAN:
			SetBandwidth( 800000 );
			break;
		default:
			break;
	}
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

void	Conn::SetBandwidth( int bytes_per_sec )
{
	m_bandwidth = bytes_per_sec;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

int		Conn::GetBandwidth( void )
{
	return m_bandwidth;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

void	Conn::FlagMessagesForResending( unsigned int packet_stamp )
{
	MsgImpLink *msg_imp_link;
	MsgSeqLink *msg_seq_link;
	Lst::Search< MsgImpLink > imp_sh;
	Lst::Search< MsgSeqLink > seq_sh;

	// Mark each message of a matching packet stamp with a timestamp of zero.
	// Doing so signifies that it should be sent next frame
	for( msg_imp_link = imp_sh.FirstItem( m_important_msg_list ); 
			msg_imp_link; msg_imp_link = imp_sh.NextItem() )
	{
		if( msg_imp_link->m_Packetstamp == packet_stamp )
		{
			msg_imp_link->m_Timestamp = 0;
		}
	}	

	for( msg_seq_link = seq_sh.FirstItem( m_sequenced_msg_list ); 
			msg_seq_link; msg_seq_link = seq_sh.NextItem() )
	{
		if( msg_seq_link->m_Packetstamp == packet_stamp )
		{
			msg_seq_link->m_Timestamp = 0;
		}
	}
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

}	// namespace Net