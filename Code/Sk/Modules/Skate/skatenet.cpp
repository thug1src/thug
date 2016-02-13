
/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Skate Module (SKATE) 									**
**																			**
**	File name:		modules/skatenet.cpp									**
**																			**
**	Created by:		02/08/2001	-	spg										**
**																			**
**	Description:	Skate Module's Network Handlers							**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

//#include <string.h>

#include <core/defines.h>
#include <core/singleton.h>

#include <core/task.h>
#include <core/math.h>

#include <sys/sys.h>
#include <sys/mem/memman.h>
#include <sys/timer.h>

#include <gfx/gfxman.h>
#include <gfx/animcontroller.h>
#include <gfx/facetexture.h>
#include <gfx/skeleton.h>
#include <gfx/nxmodel.h>

//#include <gel/inpman.h>
#include <gel/objman.h>
#include <gel/object.h>
#include <gel/net/net.h>
#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>
#include <gel/mainloop.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/environment/terrain.h>

#include <modules/skate/skate.h>
#include <modules/skate/goalmanager.h>
#include <modules/skate/gamemode.h>
#include <modules/frontend/frontend.h>

#include <objects/skater.h>
#include <objects/skaterprofile.h>
#include <sk/objects/skatercareer.h>

#include <sk/gamenet/gamenet.h>
#include <sk/components/skaterstatehistorycomponent.h>
#include <sk/components/skaterlocalnetlogiccomponent.h>

#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <scripting/cfuncs.h>


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Mdl
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define	vFORCE_UPDATE_INTERVAL	60
#define vOBJ_UPDATE_THRESHOLD	FEET( 150 )

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

class ObjUpdateInfo
{
public:
	Net::BitStream			m_BitStream;
	int 					m_Handle;
	GameNet::PlayerInfo* 	m_Player;
};

/*****************************************************************************
**							   Private Classes								**
*****************************************************************************/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


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
/* Create all network-related tasks and handlers                  */
/*                                                                */
/******************************************************************/

void	Skate::net_setup( void )
{
	// Server net tasks
	m_object_update_task = new Tsk::Task< Skate > ( Skate::s_object_update_code, *this,
										Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_OBJECT_UPDATE );
	m_score_update_task = new Tsk::Task< Skate > ( Skate::s_score_update_code, *this,
 										Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_SCORE_UPDATE );
}

/******************************************************************/
/* delete all network-related tasks and handlers                  */
/*                                                                */
/******************************************************************/

void	Skate::net_shutdown( void )
{
	// Server net tasks
	delete m_object_update_task;
	delete m_score_update_task;
}

/******************************************************************/
/* Determines whether we should send "send_player" an update 	  */
/* concerning "object"                                            */
/******************************************************************/

bool		Skate::should_send_update( GameNet::PlayerInfo* send_player, Obj::CObject* object )
{
	Obj::SPosEvent* latest_state;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	GameNet::PlayerInfo* skater_player;
	Obj::CSkater* skater_obj;
	int id, index;
	
	// Dont send a client updates on his own skater. He is up to date.
	skater_player = gamenet_man->GetPlayerByObjectID( object->GetID() );
	if(	( skater_player == NULL ) ||
		( skater_player == send_player ))
	{
		return false;
	}
	
	skater_obj = skater_player->m_Skater;
	// Check if we have something to report
	if( skater_obj->GetStateHistory()->GetNumPosUpdates() == 0 )
	{
		//Dbg_Printf( "Not enough pos updates\n" );
		return false;
	}

	id = skater_obj->GetID();
	index = ( skater_obj->GetStateHistory()->GetNumPosUpdates() - 1 ) % Obj::CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
	latest_state = &skater_obj->GetStateHistory()->GetPosHistory()[index];

	// Only send an update if there's something new to tell
	if( latest_state->LoTime == send_player->m_LastSentProps.m_LastSkaterUpdateTime[id] )
	{
		//Dbg_Printf( "Nothing new to tell\n" );
		return false;
	}

	return true;
}

/******************************************************************/
/* Decide which fields need to be updated for this object		  */
/*                                                                */
/******************************************************************/

static	int		s_get_update_flags( GameNet::PlayerInfo* player, 
									Obj::SPosEvent* latest_state,
									int skater_id ) 
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	int i, update_flags;
	short pos[3];
	short rot[3];
	Flags< int > skater_flags;
	Flags< int > end_run_flags;
	Mth::Vector eulers;
	char state, doing_trick, terrain, walking, driving;
	bool forced_send;
	unsigned char id;
	sint16 rail_node;

	update_flags = 0;

	// Force an object update every N frames
	forced_send = !( gamenet_man->GetServer()->m_FrameCounter % vFORCE_UPDATE_INTERVAL ); 
	id = skater_id;

	for( i = 0; i < 3; i++ )
	{
		eulers[i] = latest_state->Eulers[i];
	}

	// Write out the object's position as three shorts (fixed-point)
	for( i = 0; i < 3; i++ )
	{
		if( i == Y )
		{
			pos[i] = (short) ( latest_state->Position[i] * 4.0f );
			//pos[i] = latest_state->Position[i];
		}
		else
		{
			pos[i] = (short) ( latest_state->Position[i] * 2.0f );
			//pos[i] = latest_state->Position[i];
		}
		if(( pos[i] != player->m_LastSentProps.m_LastSkaterPosUpdate[id][i] ) || forced_send )
		{
			if( i == X )
			{
				update_flags |= GameNet::mUPDATE_FIELD_POS_X;
			}
			else if( i == Y )
			{
				update_flags |= GameNet::mUPDATE_FIELD_POS_Y;
			}
			else if( i == Z )
			{
				update_flags |= GameNet::mUPDATE_FIELD_POS_Z;
			}
		}
	}
	
	// Write out the object's orientation as three short euler angles (fixed-point)
	for( i = 0; i < 3; i++ )
	{
		rot[i] = (short) ( eulers[i] * 4096.0f );
		if(( rot[i] != player->m_LastSentProps.m_LastSkaterRotUpdate[id][i] ) || forced_send )
		{   
			if( i == X )
			{
				update_flags |= GameNet::mUPDATE_FIELD_ROT_X;
			}
			else if( i == Y )
			{
				update_flags |= GameNet::mUPDATE_FIELD_ROT_Y;
			}
			else if( i == Z )
			{
				update_flags |= GameNet::mUPDATE_FIELD_ROT_Z;
			}
		}
	}                                          

	state = (char) latest_state->State;
	doing_trick = (char) latest_state->DoingTrick;
	terrain = (char) latest_state->Terrain;
	walking = (char) latest_state->Walking;
	driving = (char) latest_state->Driving;

	if(( state != player->m_LastSentProps.m_LastSkaterStateUpdate[id] ) ||
	   ( doing_trick != player->m_LastSentProps.m_LastDoingTrickUpdate[id] ) ||
	   ( terrain != player->m_LastSentProps.m_LastSkaterTerrain[id] ) || 
	   ( walking != player->m_LastSentProps.m_LastSkaterWalking[id] ) || 
	   ( driving != player->m_LastSentProps.m_LastSkaterDriving[id] ) || 
	   ( forced_send == true ))
	{
		update_flags |= GameNet::mUPDATE_FIELD_STATE;
	}

	skater_flags = latest_state->SkaterFlags;
	end_run_flags = latest_state->EndRunFlags;

	if(( skater_flags != player->m_LastSentProps.m_LastSkaterFlagsUpdate[id] ) || 
	   ( end_run_flags != player->m_LastSentProps.m_LastEndRunFlagsUpdate[id] ) || 
	   ( forced_send == true ))
	{
		update_flags |= GameNet::mUPDATE_FIELD_FLAGS;
	}

	rail_node = latest_state->RailNode;
	if(( rail_node != player->m_LastSentProps.m_LastRailNodeUpdate[id] ) ||
	   ( forced_send == true ))
	{
		update_flags |= GameNet::mUPDATE_FIELD_RAIL_NODE;
	}

	return update_flags;
}

/******************************************************************/
/* Place positional/state update data in the outbound stream	  */
/*                                                                */
/******************************************************************/

void		Skate::s_object_update( Obj::CObject* object, void* data )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	GameNet::PlayerInfo* player, *skater_player;
	ObjUpdateInfo* update_info;
	Obj::CSkater* skater_obj;
	Mth::Vector eulers;
	short pos[3];
	short rot[3];
	unsigned char id;
	char state, doing_trick, terrain, walking, driving;
	Flags< int > skater_flags;
	Flags< int > end_run_flags;
	int update_flags;
	Obj::SPosEvent* latest_state;
	int i, index;
	unsigned short time;
	
	Dbg_Assert( data );

	update_info = (ObjUpdateInfo *) data;

	skater_player = gamenet_man->GetPlayerByObjectID( object->GetID() );
	skater_obj = skater_player->m_Skater;
	id = (char) skater_obj->GetID();
	player = update_info->m_Player;
	index = ( skater_obj->GetStateHistory()->GetNumPosUpdates() - 1 ) % Obj::CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
	latest_state = &skater_obj->GetStateHistory()->GetPosHistory()[index];
	player->m_LastSentProps.m_LastSkaterUpdateTime[id] = latest_state->GetTime();

	for( i = 0; i < 3; i++ )
	{
		eulers[i] = latest_state->Eulers[i];
	}

	time = latest_state->LoTime;
	update_info->m_BitStream.WriteValue( time, sizeof( uint16 ) * 8 );

	update_flags = s_get_update_flags( player, latest_state, id );
	update_info->m_BitStream.WriteValue( update_flags, 9 );

	//Dbg_Printf( "*** Sending Pos[%.2f %.2f %.2f]\n", latest_state->Position[X], latest_state->Position[Y], latest_state->Position[Z] );
	// Write out the object's position as three shorts (fixed-point)
	for( i = 0; i < 3; i++ )
	{
		if( i == Y )
		{
			pos[i] = (short) ( latest_state->Position[i] * 4.0f );
			//pos[i] = latest_state->Position[i];
		}
		else
		{
			pos[i] = (short) ( latest_state->Position[i] * 2.0f );
			//pos[i] = latest_state->Position[i];
		}
		
		if( i == X )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_POS_X )
			{
				update_info->m_BitStream.WriteValue( pos[i], sizeof( short ) * 8 );
				//update_info->m_BitStream.WriteFloatValue( pos[i] );
			}
		}
		else if( i == Y )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_POS_Y )
			{
				update_info->m_BitStream.WriteValue( pos[i], sizeof( short ) * 8 );
				//update_info->m_BitStream.WriteFloatValue( pos[i] );
			}
		}
		else if( i == Z )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_POS_Z )
			{
				update_info->m_BitStream.WriteValue( pos[i], sizeof( short ) * 8 );
				//update_info->m_BitStream.WriteFloatValue( pos[i] );
			}
		}
		
		player->m_LastSentProps.m_LastSkaterPosUpdate[id][i] = pos[i];
	}
	
	// Write out the object's orientation as three short euler angles (fixed-point)
	for( i = 0; i < 3; i++ )
	{
		rot[i] = (short) ( eulers[i] * 4096.0f );
		if( i == X )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_ROT_X )
			{
				update_info->m_BitStream.WriteValue( rot[i], sizeof( short ) * 8 );
			}
		}
		else if( i == Y )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_ROT_Y )
			{
				update_info->m_BitStream.WriteValue( rot[i], sizeof( short ) * 8 );
			}
		}
		else if( i == Z )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_ROT_Z )
			{
				update_info->m_BitStream.WriteValue( rot[i], sizeof( short ) * 8 );
			}
		}
		
		player->m_LastSentProps.m_LastSkaterRotUpdate[id][i] = rot[i];
	}                                          

	state = (char) latest_state->State;
	doing_trick = (char) latest_state->DoingTrick;
	terrain = (char) latest_state->Terrain;
	walking = (char) latest_state->Walking;
	driving = (char) latest_state->Driving;

	// Write out the skaters' state
	// Write out the skater's "doing trick" flag
	// Write out the skater's terrain type
	if( update_flags & GameNet::mUPDATE_FIELD_STATE )
	{
		char mask;

		mask = state;
		if( doing_trick )
		{
			mask |= GameNet::mDOING_TRICK_MASK;
		}

		update_info->m_BitStream.WriteValue( mask, 4 );
		update_info->m_BitStream.WriteValue( terrain, 6 );
		update_info->m_BitStream.WriteValue( walking, 1 );
		update_info->m_BitStream.WriteValue( driving, 1 );

		player->m_LastSentProps.m_LastSkaterStateUpdate[id] = state;
		player->m_LastSentProps.m_LastDoingTrickUpdate[id] = doing_trick;
		player->m_LastSentProps.m_LastSkaterTerrain[id] = terrain;
		player->m_LastSentProps.m_LastSkaterWalking[id] = walking;
		player->m_LastSentProps.m_LastSkaterDriving[id] = driving;
	}

	// Write out the skaters' flags
	skater_flags = latest_state->SkaterFlags;
	end_run_flags = latest_state->EndRunFlags;

	if( update_flags & GameNet::mUPDATE_FIELD_FLAGS )
	{
		player->m_LastSentProps.m_LastSkaterFlagsUpdate[id] = skater_flags;
		player->m_LastSentProps.m_LastEndRunFlagsUpdate[id] = end_run_flags;
		update_info->m_BitStream.WriteValue( skater_flags, 5 );
		update_info->m_BitStream.WriteValue( end_run_flags, 3 );
	}

	// Write out the skaters' rail node
	if( update_flags & GameNet::mUPDATE_FIELD_RAIL_NODE )
	{
		player->m_LastSentProps.m_LastRailNodeUpdate[id] = latest_state->RailNode;
		update_info->m_BitStream.WriteValue( latest_state->RailNode, sizeof( sint16 ) * 8 );
	}
	
}

/******************************************************************/
/* Update Clients with objects' positional/rotational/anim data	  */
/*                                                                */
/******************************************************************/

void		Skate::s_object_update_code ( const Tsk::Task< Skate >& task )
{   
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	GameNet::PlayerInfo* player;
	Lst::Search< GameNet::PlayerInfo > sh;
	Net::Server* server;
    
    Dbg_AssertType ( &task, Tsk::Task< Skate > );
	Skate& mdl = task.GetData();
	
    server = gamenet_man->GetServer();

	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = 
				gamenet_man->NextPlayerInfo( sh, true ))
	{
		Net::MsgMax update_msg;
		unsigned int length;
		ObjUpdateInfo update_info;
		GameNet::PlayerInfo* update_player;
		int i, start_id, obj_id, num_updates;
        
		// Don't send updates to the local client. His stuff is already up-to-date
		if( player->m_Conn->IsRemote())
		{
			int obj_id_mask;

			update_info.m_BitStream.SetOutputData( update_msg.m_Data, 4096 );	// skip Obj ID Mask
			update_info.m_Handle = player->m_Conn->GetHandle();
			update_info.m_Player = player;
					
			obj_id_mask = 0;

			// First decide which objects' updates we will send, then append them to the stream
			// in increasing id order 
			num_updates = 0;
			start_id = ( player->GetLastObjectUpdateID() + 1 ) % vMAX_SKATERS;
			for( i = 0; i < vMAX_SKATERS; i++ )
			{
				obj_id = ( start_id + i ) % vMAX_SKATERS;
				update_player = gamenet_man->GetPlayerByObjectID( obj_id );
				if( update_player )
				{
					if( mdl.should_send_update( player, update_player->m_Skater ))
					{
						obj_id_mask |= ( 1 << obj_id );
						if( ++num_updates >= player->GetMaxObjectUpdates())
						{
							break;
						}
					}
				}
			}

			if( num_updates == 0 )
			{
				continue;
			}

			update_info.m_BitStream.WriteValue( obj_id_mask, sizeof( char ) * 8 );
			for( i = 0; i < vMAX_SKATERS; i++ )
			{
				if( obj_id_mask & ( 1 << i ))
				{
					update_player = gamenet_man->GetPlayerByObjectID( i );

					s_object_update( update_player->m_Skater, &update_info );
					player->SetLastObjectUpdateID( i );
				}
			}

			update_info.m_BitStream.Flush();
			length = update_info.m_BitStream.GetByteLength();

			if( length > 0 )
			{   
				Net::MsgDesc msg_desc;

				msg_desc.m_Data = &update_msg;
				msg_desc.m_Length = length;
				msg_desc.m_Id = GameNet::MSG_ID_OBJ_UPDATE_STREAM;
				server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
			}
		}
	}
}

/******************************************************************/
/* Update Clients with other players' scores					  */
/*                                                                */
/******************************************************************/

void		Skate::SendScoreUpdates( bool final )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	GameNet::PlayerInfo* player, *score_player;
	Lst::Search< GameNet::PlayerInfo > sh, score_sh;

    for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = 
				gamenet_man->NextPlayerInfo( sh, true ))
	{
		GameNet::MsgScoreUpdate score_msg;
		char num_scores;
		int length;
		char* data;
        
		score_msg.m_Final = (char) final;
		if( final )
		{
			score_msg.m_TimeLeft = 0;
		}
		else
		{
			score_msg.m_TimeLeft = Tmr::InSeconds( GetGameMode()->GetTimeLeft());
		}
		num_scores = 0;
		data = score_msg.m_ScoreData;
		for( score_player = gamenet_man->FirstPlayerInfo( score_sh ); score_player; score_player = 
			gamenet_man->NextPlayerInfo( score_sh ))
		{
			bool send_all_scores;

			send_all_scores = 	final ||
								( GetGameMode()->IsTrue( "should_modulate_color" )) ||
								( GetGameMode()->ShouldTrackTrickScore() == false );
			if( ( score_player != player ) || ( send_all_scores ) )
			{
				char id;
				int score;
				bool in_goal_attack;
				
				id = (char) score_player->m_Skater->GetID();
				in_goal_attack = GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netgoalattack" );

				if( in_goal_attack )
				{
					Game::CGoalManager* pGoalManager;
			 
					pGoalManager = Game::GetGoalManager();
					score = pGoalManager->NumGoalsBeatenBy( score_player->m_Skater->GetID());
				}
				else
				{
					score = gamenet_man->GetPlayerScore( score_player->m_Skater->GetID());
				}

				*data = id;
				data++;
				memcpy( data, &score, sizeof( int ));
				data += sizeof( int );

				num_scores++;
			}
		}

		if( num_scores > 0 )
		{
			Net::MsgDesc msg_desc;
            
			score_msg.m_Cheats = GetCheatChecksum();
			score_msg.m_NumScores = num_scores;
			length = 	sizeof( int ) +		// m_TimeLeft
						sizeof( int ) +		// m_Cheats
						sizeof( char ) + 	// m_Final
						sizeof( char ) +	// m_NumScores
						(( sizeof( char ) + sizeof( int )) * num_scores );

			msg_desc.m_Data = &score_msg;
			msg_desc.m_Length = length;
			msg_desc.m_Id = GameNet::MSG_ID_SCORE_UPDATE;
			// If it's the final tally or if it's reporting cheats, it HAS to get there
			if( final || ( score_msg.m_Cheats != 0 ))
			{
				msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
				msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
			}

			gamenet_man->GetServer()->EnqueueMessage( player->GetConnHandle(), &msg_desc );

			if( !player->IsLocalPlayer() && player->IsFullyIn())
			{
				GameNet::MsgCheatChecksum cheat_msg;

				cheat_msg.m_ServerChecksum = GetCheatChecksum();
				cheat_msg.m_ServerChecksum ^= 0xDEADFACE;
				msg_desc.m_Id = GameNet::MSG_ID_CHEAT_CHECKSUM_REQUEST;
				msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
				msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
				msg_desc.m_Data = &cheat_msg;
				msg_desc.m_Length = sizeof( GameNet::MsgCheatChecksum );
	
				gamenet_man->GetServer()->EnqueueMessage( player->GetConnHandle(), &msg_desc );
			}
		}
	}

	gamenet_man->SetLastScoreUpdateTime( gamenet_man->GetServer()->m_Timestamp );
}

/******************************************************************/
/* Periodic task to update Clients with other players' scores	  */
/*                                                                */
/******************************************************************/

void		Skate::s_score_update_code ( const Tsk::Task< Skate >& task )
{   
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Skate&	mdl = task.GetData();

	

	Dbg_AssertType ( &task, Tsk::Task< Skate > );

	if( gamenet_man->ShouldSendScoreUpdates() == false )
	{
		return;
	}

	if(( gamenet_man->GetServer()->m_Timestamp - gamenet_man->GetLastScoreUpdateTime()) <
			GameNet::vSCORE_UPDATE_FREQUENCY )
	{
		return;
	}

	mdl.SendScoreUpdates();
}

/*****************************************************************************
**						Server Handlers										**
*****************************************************************************/

/******************************************************************/
/* The client has requested a score event.  Decide whether or not */
/* to allow it (don't allow it if time is up, for example), and   */
/* relay the results back whenever we want to (which right now is */
/* immediately, but it doesn't have to be)						  */
/******************************************************************/

int	Skate::handle_score_request( Net::MsgHandlerContext* context )
{
	GameNet::MsgEmbedded* msg;
	GameNet::PlayerInfo* player;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
    
	msg = (GameNet::MsgEmbedded* ) context->m_Msg;
	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	if( player == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}
    
	Mdl::Score *pScore = player->m_Skater->GetScoreObject();

	switch( msg->m_SubMsgId )
	{
		case GameNet::SCORE_MSG_ID_LOG_TRICK_OBJECT:
		{
			GameNet::MsgScoreLogTrickObject* log_msg;
			
			log_msg = (GameNet::MsgScoreLogTrickObject* ) context->m_Msg;
			if( log_msg->m_GameId == gamenet_man->GetNetworkGameId())
			{
				pScore->LogTrickObject( log_msg->m_OwnerId, log_msg->m_Score, log_msg->m_NumPendingTricks, log_msg->m_PendingTrickBuffer, true );
			}
			break;
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Skater is reporting his high combo							  */
/* 						  										  */
/******************************************************************/

int Skate::handle_combo_report( Net::MsgHandlerContext* context )
{
	GameNet::PlayerInfo* player;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Skate* skate_mod;
	GameNet::MsgScoreLanded* msg;
    
	skate_mod = (Skate*) context->m_Data;

	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	if( player == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}
	msg = (GameNet::MsgScoreLanded*) context->m_Msg;

	// Ignore combo messages in freeskate
	if( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0x1c471c60,"netlobby"))
	{
		return Net::HANDLER_MSG_DONE;
	}

	// Make sure this reported score is for this game (could be late -- could be a cheat replay packet)
    if( gamenet_man->GetNetworkGameId() != msg->m_GameId )
	{
		Dbg_Printf( "Wrong game id! %d %d\n", gamenet_man->GetNetworkGameId(), msg->m_GameId );
		return Net::HANDLER_MSG_DONE;
	}

	// We should never get a score that's less than or equal to our recorded best combo for this
	// player since they are only supposed to report combo scores if they eclipse their current
	// mark.  This is most-likely cheating
	/*if( msg->m_Score <= player->m_BestCombo )
	{
		Dbg_Printf( "**** COMBO CHEAT DETECTED: Combo reported: %d , previous best: %d\n", msg->m_Score, player->m_BestCombo );
		gamenet_man->CheatingOccured();
		return Net::HANDLER_MSG_DONE;
	}*/

	//Dbg_Printf( "Player: %d  Combo: %d\n", player->m_Skater->GetID(), msg->m_Score );
	player->m_BestCombo = msg->m_Score;
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Skater landed a trick and is reporting his score		  		  */
/* 						  										  */
/******************************************************************/

int Skate::handle_landed_trick( Net::MsgHandlerContext* context )
{
	GameNet::PlayerInfo* player;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	int trick_score;
	Skate* skate_mod;
	GameNet::MsgScoreLanded* msg;
    
	skate_mod = (Skate*) context->m_Data;

	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	if( player == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}
	msg = (GameNet::MsgScoreLanded*) context->m_Msg;

	// Make sure this reported score is for this game (could be late -- could be a cheat replay packet)
    if( gamenet_man->GetNetworkGameId() != msg->m_GameId )
	{
		Dbg_Printf( "Wrong game id! %d %d\n", gamenet_man->GetNetworkGameId(), msg->m_GameId );
		return Net::HANDLER_MSG_DONE;
	}

	trick_score = msg->m_Score;
	Mdl::Score *pScore = player->m_Skater->GetScoreObject();

	if( trick_score > pScore->GetBestCombo())
	{
		pScore->SetBestCombo( trick_score );
	}
	if ( !skate_mod->GetGameMode()->IsTrue("should_modulate_color") )
	{
		if( skate_mod->GetGameMode()->ShouldTrackTrickScore())
		{
			if (skate_mod->GetGameMode()->ShouldAccumulateScore())
			{
				pScore->SetTotalScore( pScore->GetTotalScore() + trick_score );
			}
			else if( skate_mod->GetGameMode()->ShouldTrackBestCombo())
			{
				if( pScore->GetTotalScore() < trick_score )
				{
					pScore->SetTotalScore( trick_score );
				}
			}
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Skater is entering a car								  		  */
/* 						  										  */
/******************************************************************/

int Skate::handle_enter_vehicle_server( Net::MsgHandlerContext* context )
{
	GameNet::PlayerInfo* player;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Skate* skate_mod;
	GameNet::MsgEnterVehicle* msg;
    
	skate_mod = (Skate*) context->m_Data;

	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	if( player == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}
	msg = (GameNet::MsgEnterVehicle*) context->m_Msg;
	
	player->m_VehicleControlType = msg->m_ControlType;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Relay non-important msgs to all other clients		  		  */
/* 						  										  */
/******************************************************************/

int	Skate::handle_msg_relay( Net::MsgHandlerContext* context )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Lst::Search< GameNet::PlayerInfo > sh;
	GameNet::PlayerInfo* player, *orig_player;

	orig_player = gamenet_man->GetPlayerByConnection( context->m_Conn );

	// Make sure this is from a current player
	if( orig_player == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}

	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player;
			player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		Net::MsgDesc msg_desc;

		// Don't relay it back to the original client
		if( player == orig_player )
		{
			continue;
		}

		msg_desc.m_Data = context->m_Msg;
		msg_desc.m_Length = context->m_MsgLength;
		msg_desc.m_Id = context->m_MsgId;
		if( ( context->m_MsgId == GameNet::MSG_ID_BLOOD_OFF ) ||
			( context->m_MsgId == GameNet::MSG_ID_CHAT ) ||
			( context->m_MsgId == GameNet::MSG_ID_ENTER_VEHICLE ))
		{
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
		}
		else if( context->m_MsgId == GameNet::MSG_ID_SPAWN_PROJECTILE )
		{
			Skate* mod = (Skate*) context->m_Data;

			if(	( mod->GetGameMode()->GetNameChecksum() == CRCD(0x3d6d444f,"firefight") ) ||
				( mod->GetGameMode()->GetNameChecksum() == CRCD(0xbff33600,"netfirefight")))
			{
				GameNet::MsgProjectile* msg;
	
				msg = (GameNet::MsgProjectile*) context->m_Msg;
				if( ( msg->m_Scale <= 10.0f ) &&
					( msg->m_Radius <= 240 ))
				{
					msg->m_Id = orig_player->m_Skater->GetID();
					msg->m_Latency = context->m_Conn->GetAveLatency();
				}
			}
		}
			
		context->m_App->EnqueueMessage( player->GetConnHandle(), &msg_desc );
	}
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Relay script msgs to nearby clients only		  		  	  	  */
/* 						  										  */
/******************************************************************/

int	Skate::handle_selective_msg_relay( Net::MsgHandlerContext* context )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Lst::Search< GameNet::PlayerInfo > sh;
	GameNet::PlayerInfo* player, *orig_player;
	Obj::CSkater* orig_skater, *skater;

	orig_player = gamenet_man->GetPlayerByConnection( context->m_Conn );

	// Make sure this is from a current player
	if( orig_player == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}

	orig_skater = orig_player->m_Skater;
	if( orig_skater == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}

	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player;
			player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		bool send;

		send = false;
		// Don't relay it back to the original client
		if( player != orig_player )
		{
			if( player->IsObserving())
			{
				send = true;
			}
			else
			{
				skater = player->m_Skater;
				if( skater )
				{
					Mth::Vector diff;

					diff = skater->GetPos() - orig_skater->GetPos();
					if( diff.Length() < vOBJ_UPDATE_THRESHOLD )
					{
						send = true;
					}
				}
			}
		}

		if( send )
		{
			Net::MsgDesc msg_desc;

			msg_desc.m_Data = context->m_Msg;
			msg_desc.m_Length = context->m_MsgLength;
			msg_desc.m_Id = context->m_MsgId;
			context->m_App->EnqueueMessage( player->GetConnHandle(), &msg_desc );
		}
	}
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Relay script msgs to all other clients		  		  	  	  */
/* 						  										  */
/******************************************************************/

int	Skate::handle_script_relay( Net::MsgHandlerContext* context )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Lst::Search< GameNet::PlayerInfo > sh;
	GameNet::PlayerInfo* player, *orig_player;

	orig_player = gamenet_man->GetPlayerByConnection( context->m_Conn );

	// Make sure this is from a current player
	if( orig_player == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}

	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player;
			player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		Net::MsgDesc msg_desc;
		
		// Don't relay it back to the original client
		if( player == orig_player )
		{
			continue;
		}

		msg_desc.m_Data = context->m_Msg;
		msg_desc.m_Length = context->m_MsgLength;
		msg_desc.m_Id = context->m_MsgId;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
		context->m_App->EnqueueMessage( player->GetConnHandle(), &msg_desc );
	}

	if( context->m_MsgId == GameNet::MSG_ID_SPAWN_AND_RUN_SCRIPT )
	{
		GameNet::MsgSpawnAndRunScript* msg;

		msg = (GameNet::MsgSpawnAndRunScript *) context->m_Msg;

		// Only save the event if it changes the level in a permanent way
		if( msg->m_Permanent )
		{
			gamenet_man->AddSpawnedTriggerEvent( msg->m_Node, msg->m_ObjID, msg->m_ScriptName );
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Relay non-important msgs to all other clients		  		  */
/* 						  										  */
/******************************************************************/

int	Skate::handle_bail_done( Net::MsgHandlerContext* context )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	GameNet::PlayerInfo* player;
    
	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	if( player )
	{
		player->ClearNonCollidable();
	}
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Change levels	  											  */
/* 						  										  */
/******************************************************************/

int	Skate::handle_change_level( Net::MsgHandlerContext* context )
{
	Skate* skate;
	skate = (Skate*)context->m_Data;

	GameNet::MsgChangeLevel* pMsg;
	pMsg = (GameNet::MsgChangeLevel*) context->m_Msg;
	
	skate->ChangeLevel( pMsg->m_Level );

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Receive and store client skater states for relay later on	  */
/* 						  										  */
/******************************************************************/

int Skate::handle_object_update( Net::MsgHandlerContext* context )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Obj::CSkater* skater;
	GameNet::PlayerInfo* player;
	int i, index, last_index;
	unsigned int timestamp;
	char state, doing_trick, terrain, walking, driving;
	Flags< int > skater_flags;
	Flags< int > end_run_flags;
	short pos[3];
	Mth::Vector eulers;
	sint16 rail_node;
    short fixed;
	int update_flags;
	Obj::SPosEvent* latest_state, *previous_state;
	Net::BitStream stream;

    player = gamenet_man->GetPlayerByConnection( context->m_Conn );

	// Ignore late/foreign updates and update from non-ready/non-existent clients
	if( ( player == NULL ) || 
		( player->m_Conn->TestStatus( Net::Conn::mSTATUS_READY ) == false ) ||
        ( context->m_PacketFlags & ( Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN )))
	{
		int size, value, bit_data;

		size = 0;
		bit_data = 0;
		
		// Even though we don't process the data, we need to calculate the msg length
		// so that dispatcher knows how many bytes to skip
		stream.SetInputData( context->m_Msg, 1024 );

		bit_data += sizeof( int ) * 8;	// Timestamp size
		value = stream.ReadSignedValue( sizeof( int ) * 8 );
		update_flags = stream.ReadUnsignedValue( 9 );
		bit_data += 9;

		if( update_flags & GameNet::mUPDATE_FIELD_POS_X )
		{
			//bit_data += sizeof( float ) * 8;
			bit_data += sizeof( short ) * 8;
		}
		if( update_flags & GameNet::mUPDATE_FIELD_POS_Y )
		{
			//bit_data += sizeof( float ) * 8;
			bit_data += sizeof( short ) * 8;
		}
		if( update_flags & GameNet::mUPDATE_FIELD_POS_Z )
		{
			//bit_data += sizeof( float ) * 8;
			bit_data += sizeof( short ) * 8;
		}   
		if( update_flags & GameNet::mUPDATE_FIELD_ROT_X )
		{
			bit_data += sizeof( short ) * 8;
		}
		if( update_flags & GameNet::mUPDATE_FIELD_ROT_Y )
		{
			bit_data += sizeof( short ) * 8;
		}
		if( update_flags & GameNet::mUPDATE_FIELD_ROT_Z )
		{
			bit_data += sizeof( short ) * 8;
		}
		if( update_flags & GameNet::mUPDATE_FIELD_STATE )
		{   
			bit_data += 12;
		}
		if( update_flags & GameNet::mUPDATE_FIELD_FLAGS )
		{
			bit_data += 5;
			bit_data += 3;
		}              
		if( update_flags & GameNet::mUPDATE_FIELD_RAIL_NODE )
		{
			bit_data += 16;
		}

		size += (( bit_data + 7 ) >> 3 );

		context->m_MsgLength = size;
		return Net::HANDLER_CONTINUE;
	}

	skater = player->m_Skater;
	if( skater == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}
	
	index = skater->GetStateHistory()->GetNumPosUpdates() % Obj::CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
	latest_state = &skater->GetStateHistory()->GetPosHistory()[index];
	previous_state = NULL;
    
	if( skater->GetStateHistory()->GetNumPosUpdates() > 0 )
	{
		last_index = ( skater->GetStateHistory()->GetNumPosUpdates() + ( Obj::CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS - 1 )) % 
					Obj::CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
		previous_state = &skater->GetStateHistory()->GetPosHistory()[last_index];

        doing_trick = previous_state->DoingTrick;
		state = previous_state->State;
		skater_flags = previous_state->SkaterFlags;
		end_run_flags = previous_state->EndRunFlags;
		terrain = previous_state->Terrain;
		walking = previous_state->Walking;
		driving = previous_state->Driving;
		rail_node = previous_state->RailNode;
		for( i = 0; i < 3; i++ )
		{
			pos[i] = previous_state->ShortPos[i];
		}
		for( i = 0; i < 3; i++ )
		{
			eulers[i] = previous_state->Eulers[i];
		}
	}
	else
	{
		doing_trick = 0;
		state = 0;
		skater_flags = 0;
		end_run_flags = 0;
		terrain = 0;
		walking = 0;
		driving = 0;
		rail_node = -1;
		for( i = 0; i < 3; i++ )
		{
			pos[i] = 0;
		}
		for( i = 0; i < 3; i++ )
		{
			eulers[i] = 0;
		}
	}
	

	stream.SetInputData( context->m_Msg, 1024 );

	// Read in timestamp
	timestamp = stream.ReadUnsignedValue( sizeof( int ) * 8 );
	update_flags = stream.ReadUnsignedValue( 9 );
	
	// Read in fixed point position
	if( update_flags & GameNet::mUPDATE_FIELD_POS_X )
	{
		pos[X] = stream.ReadSignedValue( sizeof( short ) * 8 );
		//pos[X] = stream.ReadFloatValue();
	}
	if( update_flags & GameNet::mUPDATE_FIELD_POS_Y )
	{
		pos[Y] = stream.ReadSignedValue( sizeof( short ) * 8 );
		//pos[Y] = stream.ReadFloatValue();
	}
	if( update_flags & GameNet::mUPDATE_FIELD_POS_Z )
	{
		pos[Z] = stream.ReadSignedValue( sizeof( short ) * 8 );
		//pos[Z] = stream.ReadFloatValue();
	}   
			
	// Read in fixed point eulers
	if( update_flags & GameNet::mUPDATE_FIELD_ROT_X )
	{
		fixed = stream.ReadSignedValue( sizeof( short ) * 8 );
		eulers[X] = ((float) fixed ) / 4096.0f;
	}
	if( update_flags & GameNet::mUPDATE_FIELD_ROT_Y )
	{
		fixed = stream.ReadSignedValue( sizeof( short ) * 8 );
		eulers[Y] = ((float) fixed ) / 4096.0f;
	}
	if( update_flags & GameNet::mUPDATE_FIELD_ROT_Z )
	{
		fixed = stream.ReadSignedValue( sizeof( short ) * 8 );
		eulers[Z] = ((float) fixed ) / 4096.0f;
	}

	// Read in skater's 'state'
	if( update_flags & GameNet::mUPDATE_FIELD_STATE )
	{   
		char mask;

		mask = stream.ReadUnsignedValue( 4 );

		state = mask & GameNet::mSTATE_MASK;
		doing_trick = (( mask & GameNet::mDOING_TRICK_MASK ) != 0 );
		
		terrain = stream.ReadUnsignedValue( 6 );
		
		walking = stream.ReadUnsignedValue( 1 );
		driving = stream.ReadUnsignedValue( 1 );
	}

	if( update_flags & GameNet::mUPDATE_FIELD_FLAGS )
	{
		skater_flags = stream.ReadUnsignedValue( 5 );
		end_run_flags = stream.ReadUnsignedValue( 3 );
	}                           

	if( update_flags & GameNet::mUPDATE_FIELD_RAIL_NODE )
	{
		rail_node = stream.ReadSignedValue( sizeof( sint16 ) * 8 );
	}                           
	

	// Tell the dispatcher the size of this message because it does not know (optimization)
	context->m_MsgLength = stream.GetByteLength();

	if( ( skater->GetStateHistory()->GetNumPosUpdates() > 0 ) &&
		( timestamp < previous_state->GetTime()))
	{
		return Net::HANDLER_MSG_DONE;
	}

	// Use those euler angles to compose a matrix
	Mth::Matrix obj_matrix( eulers[X], eulers[Y], eulers[Z] );
    
	latest_state->Matrix = obj_matrix;
	latest_state->Position.Set(	((float) pos[X] ) / 2.0f,//pos[X],
				  				((float) pos[Y] ) / 4.0f,//pos[Y],
				  				((float) pos[Z] ) / 2.0f,//pos[Z],
				  				0.0f );

	latest_state->SetTime( timestamp );

	// Update short versions in the player history
	for( i = 0; i < 3; i++ )
	{
		latest_state->ShortPos[i] = pos[i];
		latest_state->Eulers[i] = eulers[i];
	}

	latest_state->DoingTrick = doing_trick;
	latest_state->SkaterFlags = skater_flags;
	latest_state->EndRunFlags = end_run_flags;
	latest_state->State = state;
	latest_state->Terrain = (ETerrainType) terrain;
	latest_state->RailNode = rail_node;
	latest_state->Walking = walking;
	latest_state->Driving = driving;

	skater->GetStateHistory()->IncrementNumPosUpdates();
	
	return Net::HANDLER_CONTINUE;
}

/*****************************************************************************
**						Client Handlers										**
*****************************************************************************/

/******************************************************************/
/* Execute a script	  									  		  */
/* 						  										  */
/******************************************************************/

int Skate::handle_run_script( Net::MsgHandlerContext* context )
{
	Skate* mod;
	GameNet::MsgRunScript* msg;
    Script::CScriptStructure *pParams;
	Obj::CMovingObject* obj;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());

	mod = (Skate *) context->m_Data;
	msg = (GameNet::MsgRunScript *) context->m_Msg;

	obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjID );

    pParams = new Script::CScriptStructure;
	Script::ReadFromBuffer(pParams,(uint8 *) msg->m_Data );

	Script::RunScript( msg->m_ScriptName, pParams, obj );

	delete pParams;

	Mem::Manager::sHandle().PopContext();

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Spawn and run a script 										  */
/* 						  										  */
/******************************************************************/

int Skate::handle_spawn_run_script( Net::MsgHandlerContext* context )
{
	Skate* mod;
	Script::CScript* pScript;
	Obj::CMovingObject* obj;
	GameNet::MsgSpawnAndRunScript* msg;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	mod = (Skate *) context->m_Data;
	msg = (GameNet::MsgSpawnAndRunScript *) context->m_Msg;

    if( gamenet_man->ReadyToPlay())
	{
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjID );
	
		pScript = Script::SpawnScript( msg->m_ScriptName, NULL, 0, NULL, msg->m_Node ); // K: The 0,NULL bit means no callback script specified
		#ifdef __NOPT_ASSERT__
		pScript->SetCommentString("Spawned from Skate::handle_spawn_run_script");
		#endif
		pScript->mpObject = obj; 

		pScript->Update(); 
	}
	else
	{
		gamenet_man->AddSpawnedTriggerEvent( msg->m_Node, msg->m_ObjID, msg->m_ScriptName );
	}
	

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Unpacks wobble details	  								  	  */
/* 						  										  */
/******************************************************************/

static float s_get_wobble_details_from_mask( char mask, int field )
{
	float value;

	value = 0;
	switch( field )
	{
		case GameNet::MsgSetWobbleDetails::vWOBBLE_AMP_A:
		{
			int field_mask;

			field_mask = mask & GameNet::MsgSetWobbleDetails::mWOBBLE_AMPA_MASK;
            if( field_mask == 0 )
			{
				value = 0.05f;
			}
			else if( field_mask == 1 )
			{
				value = 0.1f;
			}
			else if( field_mask == 2 )
			{
				value = 10.1f;
			}
			else
			{
				Dbg_Assert( 0 );
			}
			break;
		}
		case GameNet::MsgSetWobbleDetails::vWOBBLE_AMP_B:
		{
			int field_mask;

			field_mask = mask & GameNet::MsgSetWobbleDetails::mWOBBLE_AMPB_MASK;
			field_mask >>= 2;
            if( field_mask == 0 )
			{
				value = 0.04f;
			}
			else if( field_mask == 1 )
			{
				value = 10.1f;
			}
			else
			{
				Dbg_Assert( 0 );
			}
			break;
		}
		case GameNet::MsgSetWobbleDetails::vWOBBLE_K1:
		{
			int field_mask;

			field_mask = mask & GameNet::MsgSetWobbleDetails::mWOBBLE_K1_MASK;
			field_mask >>= 3;
            if( field_mask == 0 )
			{
				value = 0.0022f;
			}
			else if( field_mask == 1 )
			{
				value = 20.0f;
			}
			else
			{
				Dbg_Assert( 0 );
			}
			break;
		}
		case GameNet::MsgSetWobbleDetails::vWOBBLE_K2:
		{
			int field_mask;

			field_mask = mask & GameNet::MsgSetWobbleDetails::mWOBBLE_K2_MASK;
			field_mask >>= 4;
            if( field_mask == 0 )
			{
				value = 0.0017f;
			}
			else if( field_mask == 1 )
			{
				value = 10.0f;
			}
			else
			{
				Dbg_Assert( 0 );
			}
			break;
		}
		case GameNet::MsgSetWobbleDetails::vSPAZFACTOR:
		{
			int field_mask;

			field_mask = mask & GameNet::MsgSetWobbleDetails::mSPAZFACTOR_MASK;
			field_mask >>= 5;
            if( field_mask == 0 )
			{
				value = 1.5f;
			}
			else if( field_mask == 1 )
			{
				value = 1.0f;
			}
			else
			{
				Dbg_Assert( 0 );
			}
			break;
		}
		default:
			Dbg_Assert( 0 );
			break;
	}

	return value;
}

/******************************************************************/
/* A client has launched a projectile							  */
/* 						  										  */
/******************************************************************/

int	Skate::handle_projectile( Net::MsgHandlerContext* context )
{
	GameNet::MsgProjectile* msg;
	Script::CStruct* pParams;
	Obj::CSkater* skater;
	Skate* mod;
	Mth::Vector vel, dir, pos;
	Tmr::Time lag;
	float time;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
    
	mod = (Skate *) context->m_Data;

	msg = (GameNet::MsgProjectile*) context->m_Msg;
	skater = mod->GetSkaterById( msg->m_Id );
	if( skater == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}
	
	vel = msg->m_Vel;
	pos = msg->m_Pos;
	dir = vel;
	dir.Normalize();
	lag = msg->m_Latency;

	// If not on the server, also add the average latency from server to us
	if( gamenet_man->OnServer() == false )
	{
		Lst::Search< Net::Conn > sh;
		Net::Conn* server_conn;

		server_conn = context->m_App->FirstConnection( &sh );
		lag += server_conn->GetAveLatency();
	}

    time = (float) lag / 1000.0f;
    pos += dir * time;	// Time is in units of seconds. Velocity is in terms of feet/sec

	pParams = new Script::CStruct;
	pParams->AddChecksum( CRCD(0x81c39e06,"owner_id"), msg->m_Id );
	pParams->AddInteger( CRCD(0x7323e97c,"x"), pos[X] );
	pParams->AddInteger( CRCD(0x424d9ea,"y"), pos[Y] );
	pParams->AddInteger( CRCD(0x9d2d8850,"z"), pos[Z] );
	pParams->AddInteger( CRCD(0xbded7a76,"scaled_x"), msg->m_Vel[X] );
	pParams->AddInteger( CRCD(0xcaea4ae0,"scaled_y"), msg->m_Vel[Y] );
	pParams->AddInteger( CRCD(0x53e31b5a,"scaled_z"), msg->m_Vel[Z] );
	pParams->AddInteger( CRCD(0xc48391a5,"radius"), msg->m_Radius );
	pParams->AddFloat( CRCD(0x13b9da7b,"scale"), msg->m_Scale );
	
	Script::RunScript( CRCD(0xaa2b5552,"ClientLaunchFireball"), pParams, skater );
	delete pParams;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* A client has entered a vehicle								  */
/* 						  										  */
/******************************************************************/

int	Skate::handle_enter_vehicle_client( Net::MsgHandlerContext* context )
{
	GameNet::MsgEnterVehicle* msg;
	Obj::CSkater* skater;
	Skate* mod;
    
	mod = (Skate *) context->m_Data;

	msg = (GameNet::MsgEnterVehicle*) context->m_Msg;
	skater = mod->GetSkaterById( msg->m_Id );
	if( skater == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}
	
	Dbg_Assert(GetSkaterStateHistoryComponentFromObject(skater));
	GetSkaterStateHistoryComponentFromObject(skater)->SetCurrentVehicleControlType(msg->m_ControlType);
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* The server has sent a list of cheats that are enabled		  */
/* 						  										  */
/******************************************************************/

int	Skate::handle_cheat_list( Net::MsgHandlerContext* context )
{
	GameNet::MsgEnabledCheats* msg;
	int i;

	Dbg_Printf( "**** HANDLING CHEAT LIST\n" );
	CFuncs::ScriptClearCheats( NULL, NULL );

	msg = (GameNet::MsgEnabledCheats*) context->m_Msg;
	for( i = 0; i < msg->m_NumCheats; i++ )
	{
		Script::CStruct* pParams;

		pParams = new Script::CStruct;
		pParams->AddChecksum( CRCD(0xf649d637,"on"), 0 );
		pParams->AddChecksum( CRCD(0xae94c183,"cheat_flag"), msg->m_Cheats[i] );
		Script::RunScript( CRCD(0x3cf9e86d,"client_toggle_cheat"), pParams );

		delete pParams;
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* The server has toggled a cheat. Mimic behavior on this end	  */
/* 						  										  */
/******************************************************************/

int Skate::handle_cheats( Net::MsgHandlerContext* context )
{
	GameNet::MsgToggleCheat* msg;
	Script::CStruct* pParams;

	pParams = new Script::CStruct;
	msg = (GameNet::MsgToggleCheat*) context->m_Msg;
	
	if( msg->m_On )
	{
		pParams->AddChecksum( CRCD(0xf649d637,"on"), 0 );
	}

	pParams->AddChecksum( CRCD(0xae94c183,"cheat_flag"), msg->m_Cheat );

	Script::RunScript( CRCD(0x3cf9e86d,"client_toggle_cheat"), pParams );

	delete pParams;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Get the number of times the 16-bit value was flipped since the */
/* last time			  										  */
/******************************************************************/

int	s_get_num_flips( uint32 new_time, uint32 old_time )
{
	uint32 hi_new, hi_old;

	hi_new = new_time >> 16;
	hi_old = old_time >> 16;

	return ( hi_new - hi_old );
}

/******************************************************************/
/* Animate non-local clients	  								  */
/* 						  										  */
/******************************************************************/

int Skate::handle_anims( Net::MsgHandlerContext* context )
{
	Obj::SAnimEvent* latest_event;
	Obj::CMovingObject* obj;
	Obj::CSkater* skater;
	Skate* mod;
    
	mod = (Skate *) context->m_Data;
    if( context->m_MsgId == GameNet::MSG_ID_PRIM_ANIM_START )
	{
		GameNet::MsgPlayPrimaryAnim anim_msg;
		char* stream;
        
        stream = context->m_Msg;
        
		//memcpy( &anim_msg.m_Time, stream, sizeof( unsigned short ));
		//Dbg_Printf( "(%d) Got Primary %d\n", context->m_App->m_FrameCounter, anim_msg.m_Time );
		//stream += sizeof( int );
		memcpy( &anim_msg.m_LoopingType, stream, sizeof( char ));
		stream++;
		memcpy( &anim_msg.m_ObjId, stream, sizeof( char ));
		stream++;
		memcpy( &anim_msg.m_Index, stream, sizeof( uint32 ));
		stream += sizeof( uint32 );
		memcpy( &anim_msg.m_StartTime, stream, sizeof( unsigned short ));
		stream += sizeof( unsigned short );
		memcpy( &anim_msg.m_EndTime, stream, sizeof( unsigned short ));
		stream += sizeof( unsigned short );
		memcpy( &anim_msg.m_BlendPeriod, stream, sizeof( unsigned short ));
		stream += sizeof( unsigned short );
		memcpy( &anim_msg.m_Speed, stream, sizeof( unsigned short ));
		stream += sizeof( unsigned short );

		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( anim_msg.m_ObjId );
		if( obj )
		{
			GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
			float blend_period;

			skater = static_cast< Obj::CSkater *> ( obj );
			Dbg_MsgAssert( skater != NULL,( "Got anim for non-CSkater" ));
			
			blend_period = (float) anim_msg.m_BlendPeriod / 4096.0f;

			// Only blend skaters that we are observing
			if ( gamenet_man->InNetGame() )
			{
				GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
				GameNet::PlayerInfo* observed_player, *anim_player;

				anim_player = gamenet_man->GetPlayerByObjectID( obj->GetID() );
				observed_player = gamenet_man->GetCurrentlyObservedPlayer();
				if(( observed_player == NULL ) || ( anim_player != observed_player ))
				{
					blend_period = 0.0f;
				}
			}
			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
				latest_event->m_MsgId = GameNet::MSG_ID_PRIM_ANIM_START;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
				//Dbg_Printf( "(%d) Setting anm time: %d\n", context->m_App->m_FrameCounter, latest_event->GetTime());

				latest_event->m_ObjId = anim_msg.m_ObjId;
				latest_event->m_Asset = anim_msg.m_Index;
				latest_event->m_StartTime = (float) anim_msg.m_StartTime / 4096.0f;
				latest_event->m_EndTime = (float) anim_msg.m_EndTime / 4096.0f;
				latest_event->m_BlendPeriod = blend_period;
				latest_event->m_Speed = (float) anim_msg.m_Speed / 4096.0f;
				latest_event->m_LoopingType = anim_msg.m_LoopingType;

				skater->GetStateHistory()->IncrementNumAnimUpdates();
			}
		}
	}
	else if( context->m_MsgId == GameNet::MSG_ID_SET_WOBBLE_TARGET )
	{
		GameNet::MsgSetWobbleTarget* msg;

		msg = (GameNet::MsgSetWobbleTarget *) context->m_Msg;
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
		if( obj )
		{   
			skater = static_cast< Obj::CSkater *> ( obj );
			Dbg_MsgAssert( skater != NULL,( "Got anim for non-CSkater" ));

			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
				latest_event->m_MsgId = GameNet::MSG_ID_SET_WOBBLE_TARGET;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
				//Dbg_Printf( "(%d) Got Wobble %d\n", context->m_App->m_FrameCounter, msg->m_Time );
				latest_event->m_ObjId = msg->m_ObjId;
				latest_event->m_Alpha = (float) msg->m_Alpha / 255.0f;
				skater->GetStateHistory()->IncrementNumAnimUpdates();
			}
		}
	}
	else if( context->m_MsgId == GameNet::MSG_ID_ROTATE_SKATEBOARD )
	{
		GameNet::MsgRotateSkateboard* msg;

		msg = (GameNet::MsgRotateSkateboard *) context->m_Msg;
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
		if( obj )
		{
			skater = static_cast< Obj::CSkater *> ( obj );
			Dbg_MsgAssert( skater != NULL,( "Got anim for non-CSkater" ));

			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
				latest_event->m_MsgId = GameNet::MSG_ID_ROTATE_SKATEBOARD;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
                //Dbg_Printf( "(%d) Got Rotate %d\n", context->m_App->m_FrameCounter, msg->m_Time );
				latest_event->m_ObjId = msg->m_ObjId;
				latest_event->m_Rotate = msg->m_Rotate;
				skater->GetStateHistory()->IncrementNumAnimUpdates();
			}
		}
	}
	else if( context->m_MsgId == GameNet::MSG_ID_SET_WOBBLE_DETAILS )
	{
		GameNet::MsgSetWobbleDetails* msg;

		Obj::CMovingObject* obj;

		msg = (GameNet::MsgSetWobbleDetails *) context->m_Msg;
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
		if( obj )
		{
			skater = static_cast< Obj::CSkater *> ( obj );
			Dbg_MsgAssert( skater != NULL,( "Got anim for non-CSkater" ));

			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
				latest_event->m_MsgId = GameNet::MSG_ID_SET_WOBBLE_DETAILS;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
				//Dbg_Printf( "(%d) Got details %d\n", context->m_App->m_FrameCounter, msg->m_Time );
				latest_event->m_ObjId = msg->m_ObjId;
				latest_event->m_WobbleAmpA = s_get_wobble_details_from_mask( msg->m_WobbleDetails, GameNet::MsgSetWobbleDetails::vWOBBLE_AMP_A );
				latest_event->m_WobbleAmpB = s_get_wobble_details_from_mask( msg->m_WobbleDetails, GameNet::MsgSetWobbleDetails::vWOBBLE_AMP_B );
				latest_event->m_WobbleK1 = s_get_wobble_details_from_mask( msg->m_WobbleDetails, GameNet::MsgSetWobbleDetails::vWOBBLE_K1 );
				latest_event->m_WobbleK2 = s_get_wobble_details_from_mask( msg->m_WobbleDetails, GameNet::MsgSetWobbleDetails::vWOBBLE_K2 );
				latest_event->m_SpazFactor = s_get_wobble_details_from_mask( msg->m_WobbleDetails, GameNet::MsgSetWobbleDetails::vSPAZFACTOR );

				skater->GetStateHistory()->IncrementNumAnimUpdates();
			}
		}
	}
	else if( context->m_MsgId == GameNet::MSG_ID_SET_LOOPING_TYPE )
	{
		GameNet::MsgSetLoopingType* msg;

		Obj::CMovingObject* obj;

		msg = (GameNet::MsgSetLoopingType *) context->m_Msg;
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
		if( obj )
		{
			skater = static_cast< Obj::CSkater *> ( obj );
			Dbg_MsgAssert( skater != NULL,( "Got anim for non-CSkater" ));

			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
                latest_event->m_MsgId = GameNet::MSG_ID_SET_LOOPING_TYPE;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
				//Dbg_Printf( "(%d) Got looping %d\n", context->m_App->m_FrameCounter, msg->m_Time );
				latest_event->m_ObjId = msg->m_ObjId;
				latest_event->m_LoopingType = msg->m_LoopingType;

				skater->GetStateHistory()->IncrementNumAnimUpdates();
			}
		}
	}
	else if( context->m_MsgId == GameNet::MSG_ID_SET_HIDE_ATOMIC )
	{
		GameNet::MsgHideAtomic* msg;

		Obj::CMovingObject* obj;

		msg = (GameNet::MsgHideAtomic *) context->m_Msg;
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
		if( obj )
		{
			skater = static_cast< Obj::CSkater *> ( obj );
			Dbg_MsgAssert( skater != NULL,( "Got anim for non-CSkater" ));

			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
                latest_event->m_MsgId = GameNet::MSG_ID_SET_HIDE_ATOMIC;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
				latest_event->m_ObjId = msg->m_ObjId;
				latest_event->m_Asset = msg->m_AtomicName;
				latest_event->m_Hide = msg->m_Hide;

				skater->GetStateHistory()->IncrementNumAnimUpdates();
			}
		}
	}
	else if( context->m_MsgId == GameNet::MSG_ID_SET_ANIM_SPEED )
	{
		GameNet::MsgSetAnimSpeed* msg;

		Obj::CMovingObject* obj;

		msg = (GameNet::MsgSetAnimSpeed *) context->m_Msg;
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
		if( obj )
		{
			skater = static_cast< Obj::CSkater *> ( obj );
			Dbg_MsgAssert( skater != NULL,( "Got anim for non-CSkater" ));

			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
                latest_event->m_MsgId = GameNet::MSG_ID_SET_ANIM_SPEED;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
				//Dbg_Printf( "(%d) Got speed %d\n", context->m_App->m_FrameCounter, msg->m_Time );
				latest_event->m_ObjId = msg->m_ObjId;
				latest_event->m_Speed = msg->m_AnimSpeed;

				skater->GetStateHistory()->IncrementNumAnimUpdates();
			}
		}
	}
	else if( context->m_MsgId == GameNet::MSG_ID_FLIP_ANIM )
	{
		GameNet::MsgFlipAnim* msg;

		Obj::CMovingObject* obj;
		
		msg = (GameNet::MsgFlipAnim *) context->m_Msg;
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
		if( obj )
		{
			skater = mod->GetSkaterById( msg->m_ObjId );
			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
                latest_event->m_MsgId = GameNet::MSG_ID_FLIP_ANIM;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
				//Dbg_Printf( "(%d) Got flip %d\n", context->m_App->m_FrameCounter, msg->m_Time );
				latest_event->m_Flipped = msg->m_Flipped;
				latest_event->m_ObjId = msg->m_ObjId;

				skater->GetStateHistory()->IncrementNumAnimUpdates();

				// If we got a flip message for a skater, we've now handled it. Otherwise,
				// let it fall through as it is for some other object in the game
				return Net::HANDLER_MSG_DONE;
			}
		}
	}
	else if( context->m_MsgId == GameNet::MSG_ID_ROTATE_DISPLAY )
	{
		GameNet::MsgRotateDisplay* msg;

		Obj::CMovingObject* obj;
		
		msg = (GameNet::MsgRotateDisplay *) context->m_Msg;
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
		if( obj )
		{
			skater = mod->GetSkaterById( msg->m_ObjId );
			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
                latest_event->m_MsgId = GameNet::MSG_ID_ROTATE_DISPLAY;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
				//Dbg_Printf( "(%d) Got rotate %d\n", context->m_App->m_FrameCounter, msg->m_Time );
				latest_event->m_ObjId = msg->m_ObjId;
				latest_event->m_Duration = msg->m_Duration;
				latest_event->m_SinePower = msg->m_SinePower;
				latest_event->m_StartAngle = msg->m_StartAngle;
				latest_event->m_DeltaAngle = msg->m_DeltaAngle;
				latest_event->m_HoldOnLastAngle = msg->m_HoldOnLastAngle;
				latest_event->m_Flags = msg->m_Flags;

				skater->GetStateHistory()->IncrementNumAnimUpdates();
			}
		}
	}
	else if( context->m_MsgId == GameNet::MSG_ID_CLEAR_ROTATE_DISPLAY )
	{
		GameNet::MsgObjMessage* msg;

		Obj::CMovingObject* obj;
		
		msg = (GameNet::MsgObjMessage *) context->m_Msg;
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
		if( obj )
		{
			skater = mod->GetSkaterById( msg->m_ObjId );
			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
                latest_event->m_MsgId = GameNet::MSG_ID_CLEAR_ROTATE_DISPLAY;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
				latest_event->m_ObjId = msg->m_ObjId;

				skater->GetStateHistory()->IncrementNumAnimUpdates();
			}
		}
	}
	else if( context->m_MsgId == GameNet::MSG_ID_CREATE_SPECIAL_ITEM )
	{    
		Obj::CMovingObject* obj;
		GameNet::MsgSpecialItem* msg;

		msg = (GameNet::MsgSpecialItem *) context->m_Msg;
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
		if( obj )
		{
			skater = mod->GetSkaterById( msg->m_ObjId );
			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
                latest_event->m_MsgId = GameNet::MSG_ID_CREATE_SPECIAL_ITEM;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
				latest_event->m_ObjId = msg->m_ObjId;
				latest_event->m_Bone = msg->m_Bone;
				latest_event->m_Index = msg->m_Index;
				latest_event->m_Asset = msg->m_Params;

				skater->GetStateHistory()->IncrementNumAnimUpdates();
			}
		}
	}
	else if( context->m_MsgId == GameNet::MSG_ID_DESTROY_SPECIAL_ITEM )
	{
		Obj::CMovingObject* obj;
		GameNet::MsgSpecialItem* msg;

		msg = (GameNet::MsgSpecialItem *) context->m_Msg;
		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
		if( obj )
		{
			skater = mod->GetSkaterById( msg->m_ObjId );
			if( skater )
			{
				latest_event = skater->GetStateHistory()->GetLatestAnimEvent();
                latest_event->m_MsgId = GameNet::MSG_ID_DESTROY_SPECIAL_ITEM;
				latest_event->SetTime( skater->GetStateHistory()->GetLastPosEvent()->GetTime());
				latest_event->m_ObjId = msg->m_ObjId;
				latest_event->m_Index = msg->m_Index;

				skater->GetStateHistory()->IncrementNumAnimUpdates();
			}
		}
	}
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Skate::handle_flip_anim( Net::MsgHandlerContext* context ) 
{
	Skate* mod;

	mod = (Skate *) context->m_Data;
		
	GameNet::MsgFlipAnim* msg;

	Obj::CMovingObject* obj;
	msg = (GameNet::MsgFlipAnim *) context->m_Msg;
	obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
	if( obj )
	{
		Obj::CMovingObject* moving_obj;
	 
		moving_obj = static_cast< Obj::CMovingObject *> ( obj );
		Dbg_MsgAssert( moving_obj != NULL,( "Got object update for non-CMovingObj\n" ));
		
		if( moving_obj )
		{
			Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( moving_obj );
			if ( pAnimationComponent )
			{
				pAnimationComponent->FlipAnimation( msg->m_ObjId, msg->m_Flipped, context->m_App->m_Timestamp, false );
			}
		}
	}
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Skate::handle_player_quit( Net::MsgHandlerContext* context )
{
	GameNet::PlayerInfo* player;
	Skate* mod;
	GameNet::MsgPlayerQuit* msg;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	Dbg_Printf( "Got player quit message" );

	mod = (Skate *) context->m_Data;
	msg = (GameNet::MsgPlayerQuit *) context->m_Msg; 
	player = gamenet_man->GetPlayerByObjectID( msg->m_ObjId );
	if( player )
	{   
		Obj::CSkater* quitting_skater;

		quitting_skater = player->m_Skater;
		
		Dbg_Printf( "Got player quit message 2" );
		gamenet_man->DropPlayer( player, (GameNet::DropReason) msg->m_Reason );
		mod->remove_skater( quitting_skater );
	}
	else
	{
		GameNet::NewPlayerInfo* pending_player;

		pending_player = gamenet_man->GetNewPlayerInfoByObjectID( msg->m_ObjId );
		if( pending_player )
		{
			gamenet_man->DestroyNewPlayer( pending_player );
		}
	}

	// The server may have been waiting for us to load this player.  If so, tell the server we're ready now.
	gamenet_man->RespondToReadyQuery();
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#if 0
static bool HostQuitDialogHandler( Front::EDialogBoxResult result )
{
	

	if( result == Front::DB_OK )
	{   
		Mdl::FrontEnd* front = Mdl::FrontEnd::Instance();
            
		Script::RunScript( "chosen_leave_server" );
		front->PauseGame( false );
		return false;
	}

	return true;
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Skate::handle_disconn_accepted( Net::MsgHandlerContext* context )
{
	GameNet::Manager* gamenet_man = GameNet::Manager::Instance();

	Dbg_Message( "Logging off of server\n" );

	Script::RunScript( "CreateServerQuitDialog" );
	gamenet_man->CleanupPlayers();
	gamenet_man->ClientShutdown();
 
	return Net::HANDLER_HALT;
}

/******************************************************************/
/* You've been kicked by the server								  */
/* 						  										  */
/******************************************************************/

int Skate::handle_kicked( Net::MsgHandlerContext* context )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Skate * skate_mod = Skate::Instance();

	

	Dbg_Message( "Logging off of server\n" );

	Script::DeleteSpawnedScripts();

	Script::CScriptStructure* pStructure = new Script::CScriptStructure;
	
	pStructure->AddComponent( Script::GenerateCRC( "width" ), ESYMBOLTYPE_INTEGER, 300 );
	pStructure->AddComponent( Script::GenerateCRC( "title" ), ESYMBOLTYPE_NAME, (int) Script::GenerateCRC("net_notice_msg"));
	if( context->m_MsgId == GameNet::MSG_ID_KICKED )
	{
		Script::RunScript( "CreateServerRemovedYouDialog" );
	}
	else if( context->m_MsgId == GameNet::MSG_ID_LEFT_OUT )
	{
		Script::RunScript( "CreateServerMovedOnDialog" );
	}
	
	if( skate_mod->m_requested_level == Script::GenerateCRC( "load_skateshop" ))
	{
		// If we got partially in, cancel the join and remove partially-loaded players
		// Shut the client down so that it doesn't bombard the server ip with messages
		gamenet_man->CancelJoinServer();
		gamenet_man->SetJoinState( GameNet::vJOIN_STATE_FINISHED );
	}
	else
	{
		gamenet_man->CleanupPlayers();
		gamenet_man->ClientShutdown();
	}

	delete pStructure;	
 
	return Net::HANDLER_HALT;
}

/******************************************************************/
/* Observer-specific trick object handling						  */
/* 						  										  */
/******************************************************************/

int Skate::handle_obs_log_trick_objects( Net::MsgHandlerContext* context )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	GameNet::MsgObsScoreLogTrickObject* msg;
	Skate* skate_mod;

	skate_mod = (Skate*) context->m_Data;
	msg = (GameNet::MsgObsScoreLogTrickObject*) context->m_Msg;

	if( msg->m_GameId != gamenet_man->GetNetworkGameId())
	{
		return Net::HANDLER_MSG_DONE;
	}

	int seq;

	if( skate_mod->GetGameMode()->IsTeamGame())
	{
		GameNet::PlayerInfo* player;

		player = gamenet_man->GetPlayerByObjectID( msg->m_OwnerId );
		seq = player->m_Team;
	}
	else
	{
		seq = msg->m_OwnerId;
	}

	// modulate the color here
	for( uint32 i = 0; i < msg->m_NumPendingTricks; i++ )
	{
		skate_mod->GetTrickObjectManager()->ModulateTrickObjectColor( msg->m_PendingTrickBuffer[i], seq );
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* handle custom face data						  				  */
/* 						  										  */
/******************************************************************/

int Skate::handle_face_data( Net::MsgHandlerContext* context )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	GameNet::PlayerInfo* sender;
	Obj::CSkater* skater;
	uint8* face_data;
	int face_data_length;
	uint8 skater_id;

	Dbg_Printf( "**** HANDLING FACE DATA****" );

    // Create a new temporary CFaceTexture.  
	Gfx::CFaceTexture* pFaceTexture = new Gfx::CFaceTexture;

	face_data = (uint8*) context->m_Msg;
	face_data_length = context->m_MsgLength;
	skater_id = 0;
	if( gamenet_man->OnServer())
	{
		sender = gamenet_man->GetPlayerByConnection( context->m_Conn );
		if( sender == NULL )
		{
			return Net::HANDLER_MSG_DONE;
		}
		
		skater = sender->m_Skater;
		if( skater == NULL ) 
		{
			return Net::HANDLER_MSG_DONE;
		}
	}
	else
	{
		skater_id = *face_data++;
		face_data_length -= sizeof( char );

		sender = gamenet_man->GetPlayerByObjectID( skater_id );
		if( sender == NULL )
		{
			return Net::HANDLER_MSG_DONE;
		}
		
		skater = sender->m_Skater;
		if( skater == NULL ) 
		{
			return Net::HANDLER_MSG_DONE;
		}
	}
	
	// Read in data from network message
	pFaceTexture->ReadFromBuffer((uint8*) face_data, face_data_length );
	pFaceTexture->SetValid( true );

	// Get the skater's model
	Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( skater ); 
	Dbg_Assert( pModelComponent );
	Nx::CModel* pModel = pModelComponent->GetModel();
	Dbg_Assert( pModel );
	
	// Apply face texture to new model (eventually these parameters 
	// might need to come through the same network message, 
	// depending on the sex of the skater) 
	const char* pSrcTexture = "CS_NSN_facemap.png";
	pModel->ApplyFaceTexture( pFaceTexture, pSrcTexture, Nx::CModel::vREPLACE_GLOBALLY );
	
	if( gamenet_man->OnServer())
	{
		GameNet::PlayerInfo* face_player, *player;
		Lst::Search< GameNet::PlayerInfo > sh;
		Net::Server* server;

		server = (Net::Server*) context->m_App;

		// need to copy face texture data somewhere
		// so that future joining players can get 
		// everyone else's face textures (unimplemented)
		face_player = gamenet_man->GetPlayerByConnection( context->m_Conn );
		face_player->SetFaceData((uint8*) context->m_Msg, context->m_MsgLength );

		for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
		{
			if( player->IsLocalPlayer() || ( player == face_player ))
			{
				continue;
			}

			server->StreamMessageToConn( player->m_Conn, GameNet::MSG_ID_FACE_DATA, Gfx::CFaceTexture::vTOTAL_CFACETEXTURE_SIZE + 1, face_player->GetFaceData(), "Face Data",
									   GameNet::vSEQ_GROUP_FACE_MSGS, false, true );
		}
	}
	
	delete pFaceTexture;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Observer-specific trick object handling						  */
/* for when the level starts up (observer needs to "catch up"     */
/* to everyone else's trick object state)   					  */
/******************************************************************/

int Skate::handle_obs_init_graffiti_state( Net::MsgHandlerContext* context )
{
	
	
	GameNet::MsgInitGraffitiState* msg;
	Skate* skate_mod;
	int i;

	skate_mod = (Skate*) context->m_Data;
	msg = (GameNet::MsgInitGraffitiState*) context->m_Msg;

	if ( !skate_mod->GetGameMode()->IsTrue( "should_modulate_color" ) )
	{
		printf( "No need to continue cause we're not in graffiti mode" );
		
		// no need to process the msg here unless it's graffiti mode
		return Net::HANDLER_MSG_DONE;
	}

	uint8* pStartOfStream = &msg->m_TrickObjectStream[0];

	Obj::CTrickObjectManager* pTrickObjectManager = skate_mod->GetTrickObjectManager();
	Dbg_Assert( pTrickObjectManager );

	// push heap
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());

	Script::CScriptStructure* pStructure = new Script::CScriptStructure;

	// modulate the color here
	for( i = 0; i < vMAX_SKATERS; i++ )
	{
		if ( msg->m_NumTrickObjects[i] )
		{
			Script::CArray* pArray = new Script::CArray;
			
			pArray->SetArrayType( msg->m_NumTrickObjects[i], ESYMBOLTYPE_NAME );
			
			for ( int j = 0; j < msg->m_NumTrickObjects[i]; j++ )
			{
				uint32 trick_object_checksum = pTrickObjectManager->GetUncompressedTrickObjectChecksum( *pStartOfStream );
				pArray->SetChecksum( j, trick_object_checksum );
//				pTrickObjectManager->ModulateTrickObjectColor( trick_object_checksum, i );
				pStartOfStream++;
			}

			char arrayName[32];
			sprintf( arrayName, "Skater%d", i );			
			pStructure->AddComponent( Script::GenerateCRC( arrayName ), pArray );
		}
	}

	pTrickObjectManager->SetObserverGraffitiState( pStructure );
	pTrickObjectManager->ApplyObserverGraffitiState();

	delete pStructure;

	// apply color modulation a few frames later...
	//
	//
	// This was never defined, so I commented it out. I just apply it immediately and this
	// seems to work for now
	//Script::SpawnScript( "deferred_observer_trick_modulation", pStructure, 0, NULL );

	// pop heap
	Mem::Manager::sHandle().PopContext();

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Set terrain type										  		  */
/*                           									  */
/******************************************************************/

int	Skate::handle_score_update( Net::MsgHandlerContext* context )
{
    GameNet::Manager* gamenet_man = GameNet::Manager::Instance();
	Mdl::FrontEnd* front_end = Mdl::FrontEnd::Instance();
	GameNet::MsgScoreUpdate* msg;
	Obj::CMovingObject *obj;
	Skate* mod;
	int i, score;
	char* data;
	char id;
    
	msg = (GameNet::MsgScoreUpdate*) context->m_Msg;
	data = msg->m_ScoreData;
    mod = (Skate*) context->m_Data;

	if( !gamenet_man->OnServer())
	{
		mod->GetGameMode()->SetTimeLeft( msg->m_TimeLeft );
	}

	for( i = 0; i < msg->m_NumScores; i++ )
	{           
		id = *data;
		data++;
		memcpy( &score, data, sizeof( int ));
		data += sizeof( int );

		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( id );
		if( obj )
		{
			Obj::CSkater* skater_obj;
			
			skater_obj = static_cast< Obj::CSkater *> ( obj );
			Dbg_MsgAssert( skater_obj != NULL,( "Got score update for non-CSkater\n" ));

			if(	( mod->GetGameMode()->IsTrue( "should_modulate_color" )) ||
				( mod->GetGameMode()->ShouldTrackTrickScore() == false ))
			{
				Mdl::Score *pScore = skater_obj->GetScoreObject();

				// Check if a skater has just been eliminated in a firefight game
				if(	( mod->GetGameMode()->GetNameChecksum() == CRCD(0xbff33600,"netfirefight" )) &&
					( skater_obj->IsLocalClient() == false ))
				{
					// If this is the positive edge then hide the skater
					if(( pScore->GetTotalScore() > 0 ) && ( score == 0 ))
					{
						Script::CStruct* params;
						params = new Script::CStruct;
						params->AddString( CRCD(0xa1dc81f9,"name"), skater_obj->GetDisplayName());
						Script::RunScript( CRCD(0x9b043179,"announce_elimination"), params );
						delete params;
						mod->HideSkater( skater_obj, true );
					}
				}

				pScore->SetTotalScore( score );
			}
			else if( skater_obj && !skater_obj->IsLocalClient())
			{
				Mdl::Score *pScore = skater_obj->GetScoreObject();
				pScore->SetTotalScore( score );
			}
		}
	}

	if( msg->m_Final )
	{          
		gamenet_man->MarkReceivedFinalScores( true );
	}

	if( msg->m_Cheats != mod->GetCheatChecksum())
	{
		if( gamenet_man->HasCheatingOccurred() == false )
		{
			Script::RunScript( CRCD(0x1cedb62c,"notify_cheating"));
			gamenet_man->CheatingOccured();
		}
	}
    
	if( mod->GetGameMode()->IsTeamGame())
	{
		int num_teams, score, highest_score, leading_team;
		GameNet::PlayerInfo* my_player;

		num_teams = mod->GetGameMode()->NumTeams();
		leading_team = GameNet::vNO_TEAM;
		highest_score = 0;
		for( i = 0; i < num_teams; i++ )
		{
			score = gamenet_man->GetTeamScore( i );
			if( score > highest_score )
			{
				highest_score = score;
				leading_team = i;
			}
			else if( score == highest_score )
			{
				// A tie doesn't result in the panel message
				leading_team = GameNet::vNO_TEAM;
			}
		}

		if( leading_team != GameNet::vNO_TEAM )
		{
			my_player = gamenet_man->GetLocalPlayer();
			if( msg->m_Final )
			{
				if(( my_player->m_Skater ) && ( my_player->m_Team == leading_team ))
				{
					my_player->m_Skater->SelfEvent(CRCD(0x96935417,"WonGame"));
				}
				else
				{
					if( my_player->m_Skater )
					{
						my_player->m_Skater->SelfEvent(CRCD(0xe3ec7e9e,"LostGame"));
					}
				}
			}
			else if( ( gamenet_man->GetCurrentLeadingTeam() != leading_team ) &&
					 ( mod->GetGameMode()->IsTrue( "show_leader_messages" ) ) &&
                     ( !gamenet_man->GameIsOver()))
	
			{
				// only print the message if you're competing against someone
				if(( my_player->m_Skater ) && ( my_player->m_Team == leading_team ))
				{   
					Script::RunScript( "NewScoreLeaderYourTeam", NULL, my_player->m_Skater );
				}
				else
				{   
					char team_name[128];

					sprintf( team_name, "team_%d_name", leading_team + 1 );
					Script::CScriptStructure* pTempStructure = new Script::CScriptStructure;
					pTempStructure->Clear();
					pTempStructure->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, Script::GetString( team_name ));
					Script::RunScript( "NewScoreLeaderOtherTeam", pTempStructure, my_player->m_Skater );
					delete pTempStructure;
				}
				gamenet_man->SetCurrentLeadingTeam( leading_team );
			}
		}
	}
	else
	{
		GameNet::PlayerInfo* player, *leader, *my_player;
		Lst::Search< GameNet::PlayerInfo > sh;
		int score, highest_score, num_players;

		// Do the logic that figures out if there's a new winner and provides feedback
		leader = NULL;
		highest_score = 0;
		num_players = 0;
		for( player = gamenet_man->FirstPlayerInfo( sh ); player;
				player = gamenet_man->NextPlayerInfo( sh ))
		{
			//if( player->IsLocalPlayer() && mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netgoalattack" ))
			{
				score = gamenet_man->GetPlayerScore( player->m_Skater->GetID());
			}
			//else
			//{
				//Mdl::Score *pScore = player->m_Skater->GetScoreObject();
				
				//score = pScore->GetTotalScore();
			//}
			if( score > highest_score )
			{
				highest_score = score;
				leader = player;
			}
			else if( score == highest_score )
			{
				// A tie doesn't result in the panel message
				leader = NULL;
			}
			num_players++;
		}
        
		if( leader )
		{   
			my_player = gamenet_man->GetLocalPlayer();
			if( msg->m_Final )
			{
				if( leader->IsLocalPlayer())
				{
					leader->m_Skater->SelfEvent(CRCD(0x96935417,"WonGame"));
				}
				else
				{
					if( my_player->m_Skater )
					{
						leader->m_Skater->SelfEvent( CRCD(0xe3ec7e9e,"LostGame") );
					}
				}
			}
			else if( ( gamenet_man->GetCurrentLeader() != leader ) &&
					 ( num_players > 1 ) &&
					 ( mod->GetGameMode()->IsTrue( CRCD(0xd63dfd0f,"show_leader_messages") ) ) &&
                     ( !front_end->GamePaused()))
	
			{
				// only print the message if you're competing against someone
				if( leader->IsLocalPlayer())
				{   
					Script::RunScript( CRCD(0xa1d334d0,"NewScoreLeaderYou"), NULL, leader->m_Skater );
				}
				else
				{   
					Script::CScriptStructure* pTempStructure = new Script::CScriptStructure;
					pTempStructure->Clear();
					pTempStructure->AddComponent( CRCD(0xa4b08520,"String0"), ESYMBOLTYPE_STRING, leader->m_Name );
					Script::RunScript( CRCD(0x7a4b9d14,"NewScoreLeaderOther"), pTempStructure, my_player->m_Skater );
					delete pTempStructure;
				}
				gamenet_man->SetCurrentLeader( leader );
			}
		}
	}
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Start/Stop spark effect										  */
/*                           									  */
/******************************************************************/

int	Skate::handle_sparks( Net::MsgHandlerContext* context )
{
	Skate* mod;
	GameNet::MsgObjMessage *msg;
	Obj::CMovingObject *obj;

	

	msg = (GameNet::MsgObjMessage *) context->m_Msg;
	
	mod = (Skate *) context->m_Data;
	obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
	if( obj )
	{
		Obj::CSkater* skater_obj;
		
		skater_obj = static_cast< Obj::CSkater *> ( obj );
		Dbg_MsgAssert( skater_obj != NULL,( "Got sparks message for non-CSkater\n" ));
		
		if( skater_obj )
		{   
			switch( context->m_MsgId )
			{
				case GameNet::MSG_ID_SPARKS_ON:
					skater_obj->SparksOn();
					break;
				case GameNet::MSG_ID_SPARKS_OFF:
					skater_obj->SparksOff();
					break;
				default:
					break;
			}
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Start/Stop blood effect										  */
/*                           									  */
/******************************************************************/

int	Skate::handle_blood( Net::MsgHandlerContext* context )
{
	Skate* mod;
	Obj::CMovingObject *obj;
	GameNet::MsgObjMessage* msg;

	

	mod = (Skate *) context->m_Data;
	msg = (GameNet::MsgObjMessage* ) context->m_Msg;
	obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( msg->m_ObjId );
	if( obj )
	{
		Obj::CSkater* skater_obj;

		skater_obj = static_cast< Obj::CSkater *> ( obj );
		Dbg_MsgAssert( skater_obj != NULL,( "Got blood message for non-CSkater\n" ));
		if( skater_obj )
		{   
			switch( context->m_MsgId )
			{
				case GameNet::MSG_ID_BLOOD_ON:
				{
					GameNet::MsgBlood *msg;

					msg = (GameNet::MsgBlood *) context->m_Msg;
//					skater_obj->BloodOn( msg->m_BodyPart, msg->m_Size, msg->m_Frequency, msg->m_RandomRadius );
					break;
				}
				case GameNet::MSG_ID_BLOOD_OFF:
				{
					GameNet::MsgBloodOff *msg;

					msg = (GameNet::MsgBloodOff *) context->m_Msg;
//					skater_obj->BloodOff( msg->m_BodyPart );
					break;
				}
				default:
					break;
			}
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Play a one-off sound given a position  						  */
/*                           									  */
/******************************************************************/

int	Skate::handle_play_sound( Net::MsgHandlerContext* context )
{
	GameNet::MsgPlaySound *msg;
	Mth::Vector pos;
	float vol, pitch, choice;

	msg = (GameNet::MsgPlaySound *) context->m_Msg;
	
	pos[X] = msg->m_Pos[0];
	pos[Y] = msg->m_Pos[1];
	pos[Z] = msg->m_Pos[2];

	vol = (float) msg->m_VolPercent;
	pitch = (float) msg->m_PitchPercent;
	choice = (float) msg->m_SoundChoice;

	Env::CTerrainManager::sPlaySound( (Env::ETerrainActionType) msg->m_WhichArray, (ETerrainType) msg->m_SurfaceFlag, pos, vol, pitch, choice, false );

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Get all the game options  						  			  */
/*                           									  */
/******************************************************************/

int	Skate::handle_start_info( Net::MsgHandlerContext* context )
{   
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Skate* skate_mod;
	GameNet::MsgStartInfo* msg;
	Script::CArray* mode_array;
	int i;

	Dbg_Printf( "********************************* Got lobby info\n" );

	gamenet_man->SetJoinState( GameNet::vJOIN_STATE_CONNECTED );
	gamenet_man->FreeServerList();

	skate_mod = (Skate *) context->m_Data;
	msg = (GameNet::MsgStartInfo*) context->m_Msg;

	// Note whether or not the server is narrowband/broadband
	if( msg->m_Broadband )
	{
		context->m_Conn->SetBandwidthType( Net::Conn::vBROADBAND );
		context->m_Conn->SetSendInterval( Tmr::VBlanks( 2 ));
	}
	else
	{
		context->m_Conn->SetBandwidthType( Net::Conn::vNARROWBAND );
		context->m_Conn->SetSendInterval( Tmr::VBlanks( 2 ));
	}

	gamenet_man->SetNetworkGameId( msg->m_GameId );
	gamenet_man->SetLevel( msg->m_LevelId );
	gamenet_man->SetCrownSpawnPoint( msg->m_CrownSpawnPoint );
	gamenet_man->SetSkaterStartingPoints( msg->m_StartPoints );
	
	// set up the game mode for the client cause we're now in the lobby
	// anything that's lobby-dependent should go after this line
	Script::CScriptStructure* pTempStructure = new Script::CScriptStructure;
	pTempStructure->Clear();
	pTempStructure->AddComponent( Script::GenerateCRC("level"), ESYMBOLTYPE_NAME, (int)msg->m_LevelId );					
	Script::RunScript( "request_level", pTempStructure, NULL );
	delete pTempStructure;

	Script::RunScript( "leave_front_end" );
	Script::RunScript( "cleanup_before_loading_level" );
	
	context->m_Conn->UpdateCommTime();	// update the current comm time so it doesn't time out after
										// laoding the level

	skate_mod->GetGameMode()->SetMaximumNumberOfPlayers( msg->m_MaxPlayers );
	skate_mod->GetGameMode()->LoadGameType( msg->m_GameMode );
	//skate_mod->GetGameMode()->SetMaximumNumberOfPlayers( msg->m_MaxPlayers );
	skate_mod->LaunchGame();
	skate_mod->GetGameMode()->LoadGameType( msg->m_GameMode );
	skate_mod->GetGameMode()->SetNumTeams( msg->m_TeamMode );
	
	gamenet_man->SetProSetFlags( msg->m_ProSetFlags );
	for( i = 0; i < GameNet::vMAX_NUM_PROSETS; i++ )
	{
		if(( 1 << i ) & msg->m_ProSetFlags )
		{
			Script::CStruct* pSetParams;
			pSetParams = new Script::CStruct;
			pSetParams->AddInteger( "flag", Script::GetInt( "FLAG_PROSET1_GEO_ON" ) + i );
			Script::RunScript( "SetFlag", pSetParams );
			delete pSetParams;
		}
	}
	
    {
		Script::CScriptStructure* pTempStructure = new Script::CScriptStructure;
		
		if( msg->m_GameMode != Script::GenerateCRC("netlobby" ))
		{
			// In king of the hill, interpret time limit as a target time
			if(	( msg->m_TimeLimit == 0 ) &&
				( msg->m_GameMode != Script::GenerateCRC( "netctf" )) &&
				( msg->m_GameMode != Script::GenerateCRC( "netgoalattack" )))
			{
				if( ( msg->m_GameMode != CRCD(0x3d6d444f,"firefight")) &&
					( msg->m_GameMode != CRCD(0xbff33600,"netfirefight")))
				{
					Script::CArray* pArray = new Script::CArray;
					Script::CopyArray(pArray,Script::GetArray("targetScoreArray"));
					
					Script::CScriptStructure* pSubStruct = pArray->GetStructure(0);
					Dbg_Assert(pSubStruct);
					pSubStruct->AddComponent(Script::GenerateCRC("score"),ESYMBOLTYPE_INTEGER, msg->m_TargetScore );
					pTempStructure->AddComponent( Script::GenerateCRC("victory_conditions"), pArray );
				}
				pTempStructure->AddComponent( Script::GenerateCRC("default_time_limit"), ESYMBOLTYPE_INTEGER, (int) 0 );
				skate_mod->SetTimeLimit( 0 );
				skate_mod->GetGameMode()->OverrideOptions( pTempStructure );
			}
			else
			{
				pTempStructure->AddComponent( Script::GenerateCRC("default_time_limit"), ESYMBOLTYPE_INTEGER, (int) msg->m_TimeLimit );
				if( msg->m_GameMode == Script::GenerateCRC( "netctf" ))
				{
					Script::CArray* pArray = new Script::CArray;
					Script::CopyArray(pArray,Script::GetArray("highestScoreArray") );
					pTempStructure->AddComponent( Script::GenerateCRC("victory_conditions"), pArray );
				}
				skate_mod->SetTimeLimit( msg->m_TimeLimit );
				skate_mod->GetGameMode()->SetTimeLeft( msg->m_TimeLeft );
				skate_mod->GetGameMode()->OverrideOptions( pTempStructure );
			}
		}

		delete pTempStructure;
	}

	if( gamenet_man->InNetGame())
	{
		mode_array = Script::GetArray( "net_game_type_info" );
	}
	else
	{
		mode_array = Script::GetArray( "mp_game_type_info" );
	}
	Dbg_Assert( mode_array );

	for( i = 0; i < (int)mode_array->GetSize(); i++ )
	{   
		uint32 value, script;
		Script::CStruct* mode_struct;
		 
		mode_struct = mode_array->GetStructure( i );
		Dbg_Assert( mode_struct );
		
		mode_struct->GetChecksum( "checksum", &value, true );
		if( value == msg->m_GameMode )
		{
			Script::CStruct* params;
						
			params = new Script::CStruct;
			params->AddInteger( "time", msg->m_TimeLimit );
			params->AddInteger( "score", msg->m_TargetScore );
			if( msg->m_TimeLimit == 0 )
			{
				params->AddInteger( CRCD(0xf0e712d2,"unlimited_time"), 1 );
			}
			else
			{
				params->AddInteger( CRCD(0xf0e712d2,"unlimited_time"), 0 );
			}
			//Dbg_Printf( "Target Score : %d\n", msg->m_TargetScore );
			mode_struct->GetChecksum( "goal_script", &script, true );
			Script::RunScript( script, params );
			delete params;
			break;
		}
	}

	if( ( gamenet_man->InNetGame()) &&
		( gamenet_man->GetJoinMode() == GameNet::vJOIN_MODE_PLAY ))
	{
		if( !CFuncs::ScriptIsObserving( NULL, NULL ))
		{
			gamenet_man->SendFaceDataToServer();
		}
	}

    return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Parse the object update stream from the server and update the  */
/* appropriate objects                          				  */
/******************************************************************/

int Skate::handle_object_update_relay( Net::MsgHandlerContext* context )
{
	Skate* mod;
	int update_flags;
	unsigned char obj_id, obj_id_mask;
	Net::BitStream stream;
    
    Dbg_Assert( context );
     
	mod = (Skate *) context->m_Data;
	stream.SetInputData( context->m_Msg, 1024 );
	obj_id_mask = stream.ReadUnsignedValue( sizeof( char ) * 8 );
	
	for( obj_id = 0; obj_id < vMAX_SKATERS; obj_id++ )
	{
		GameNet::ObjectUpdate obj_update;
		Obj::CMovingObject* obj;
		Mth::Vector eulers;
		short pos[3];
		short fixed;
		char doing_trick, state, terrain, walking, driving;
		Flags< int > skater_flags;
		Flags< int > end_run_flags;
		sint16 rail_node;
		int i;
		unsigned short timestamp;
		 
		// If the bit for this player number is not set, that means there is no 
		// object update for them. Continue
		if(( obj_id_mask & ( 1 << obj_id )) == 0 )
		{
			continue;
		}

		//Dbg_Printf( "================ Got update for player %d\n", obj_id );

    	timestamp = stream.ReadUnsignedValue( sizeof( uint16 ) * 8 );
		update_flags = stream.ReadUnsignedValue( 9 );
		
		//Dbg_Printf( "Got Object Update For %d, Time: %d Mask %d\n", obj_id, timestamp, obj_id_mask );

		doing_trick = 0;
		state = 0;
		skater_flags = 0;
		end_run_flags = 0;
		terrain = 0;
		walking = 0;
		driving = 0;
		rail_node = -1;
        
		if( update_flags & GameNet::mUPDATE_FIELD_POS_X )
		{   
			pos[X] = stream.ReadSignedValue( sizeof( short ) * 8 );
			//pos[X] = stream.ReadFloatValue();
		}
		if( update_flags & GameNet::mUPDATE_FIELD_POS_Y )
		{
			pos[Y] = stream.ReadSignedValue( sizeof( short ) * 8 );
			//pos[Y] = stream.ReadFloatValue();
		}
		if( update_flags & GameNet::mUPDATE_FIELD_POS_Z )
		{
			pos[Z] = stream.ReadSignedValue( sizeof( short ) * 8 );
			//pos[Z] = stream.ReadFloatValue();
		}   
				
		if( update_flags & GameNet::mUPDATE_FIELD_ROT_X )
		{
			fixed = stream.ReadSignedValue( sizeof( short ) * 8 );
			eulers[X] = ((float) fixed ) / 4096.0f;
		}
		if( update_flags & GameNet::mUPDATE_FIELD_ROT_Y )
		{
			fixed = stream.ReadSignedValue( sizeof( short ) * 8 );
			eulers[Y] = ((float) fixed ) / 4096.0f;
		}
		if( update_flags & GameNet::mUPDATE_FIELD_ROT_Z )
		{
			fixed = stream.ReadSignedValue( sizeof( short ) * 8 );
			eulers[Z] = ((float) fixed ) / 4096.0f;
		}
        
		if( update_flags & GameNet::mUPDATE_FIELD_STATE )
		{   
			char mask;

			mask = stream.ReadUnsignedValue( 4 );
		
			state = mask & GameNet::mSTATE_MASK;
			doing_trick = (( mask & GameNet::mDOING_TRICK_MASK ) != 0 );
			terrain = stream.ReadUnsignedValue( 6 );
			walking = stream.ReadUnsignedValue( 1 );
			driving = stream.ReadUnsignedValue( 1 );
		}

		if( update_flags & GameNet::mUPDATE_FIELD_FLAGS )
		{
			skater_flags = stream.ReadUnsignedValue( 5 );
			end_run_flags = stream.ReadUnsignedValue( 3 );
		}

		if( update_flags & GameNet::mUPDATE_FIELD_RAIL_NODE )
		{
			rail_node = stream.ReadSignedValue( sizeof( sint16 ) * 8 );
			//Dbg_Printf( "Got rail node %d\n", rail_node );
		}

		obj = (Obj::CMovingObject *) mod->GetObjectManager()->GetObjectByID( obj_id );
		
		// Make sure that we actually have this object in our list of objects
		// Also, reject late/foreign object update messages. We still needed to do all the stream
		// parsing though because we need to return the length of the message to the dispatcher
		if( ( obj != NULL ) &&
			(( context->m_PacketFlags & ( Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN )) == 0 ))
		{
			Obj::CSkater* skater_obj;
			 
			skater_obj = static_cast< Obj::CSkater *> ( obj );
			Dbg_MsgAssert( skater_obj != NULL,( "Got object update for non-CSkater\n" ));
												   
			if( skater_obj )
			{
				Obj::SPosEvent* p_pos_history = skater_obj->GetStateHistory()->GetPosHistory();
				
				int index, last_index;
	
				if( skater_obj->GetStateHistory()->GetNumPosUpdates() > 0 )
				{
					last_index = ( skater_obj->GetStateHistory()->GetNumPosUpdates() - 1 ) % Obj::CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
				}
				else
				{
					Mth::Matrix obj_matrix;
					Mth::Vector eulers;
					last_index = 0;

					obj_matrix = skater_obj->GetMatrix();
					obj_matrix.GetEulers( eulers );
					for( i = 0; i < 3; i++ )
					{
						if( i == Y )
						{
							p_pos_history[0].ShortPos[i] = (short) ( skater_obj->m_pos[i] * 4.0f );
							//p_pos_history[0].ShortPos[i] = skater_obj->m_pos[i];
						}
						else
						{
							p_pos_history[0].ShortPos[i] = (short) ( skater_obj->m_pos[i] * 2.0f );
							//p_pos_history[0].ShortPos[i] = skater_obj->m_pos[i];
						}
						p_pos_history[0].Eulers[i] = eulers[i];
					}
				}
				
				index = skater_obj->GetStateHistory()->GetNumPosUpdates() % Obj::CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
				
                if( !(  update_flags & GameNet::mUPDATE_FIELD_POS_X ))
				{
					pos[X] = p_pos_history[last_index].ShortPos[X];
				}
				if( !(  update_flags & GameNet::mUPDATE_FIELD_POS_Y ))
				{
					pos[Y] = p_pos_history[last_index].ShortPos[Y];
				}
				if( !(  update_flags & GameNet::mUPDATE_FIELD_POS_Z ))
				{
					pos[Z] = p_pos_history[last_index].ShortPos[Z];
				}
				
				for( i = 0; i < 3; i++ )
				{
					p_pos_history[index].ShortPos[i] = pos[i];
				}

				obj_update.m_Pos.Set(	((float) pos[X] ) / 2.0f,//pos[X],
										((float) pos[Y] ) / 4.0f,//pos[Y],
										((float) pos[Z] ) / 2.0f,//pos[Z],
										0.0f );
				//Dbg_Printf( "*** Got Pos[%.2f %.2f %.2f]\n", obj_update.m_Pos[X], obj_update.m_Pos[Y], obj_update.m_Pos[Z] );

				if( !(  update_flags & GameNet::mUPDATE_FIELD_ROT_X ))
				{
					eulers[X] = p_pos_history[last_index].Eulers[X];
				}
				if( !(  update_flags & GameNet::mUPDATE_FIELD_ROT_Y ))
				{
					eulers[Y] = p_pos_history[last_index].Eulers[Y];
				}
				if( !(  update_flags & GameNet::mUPDATE_FIELD_ROT_Z ))
				{
					eulers[Z] = p_pos_history[last_index].Eulers[Z];
				}

				// Update history
				for( i = 0; i < 3; i++ )
				{
					p_pos_history[index].Eulers[i] = eulers[i];
				}

				// Use those euler angles to compose a matrix
				Mth::Matrix obj_matrix( eulers[X], eulers[Y], eulers[Z] );
				obj_update.m_Matrix = obj_matrix;

                p_pos_history[index].Matrix = obj_update.m_Matrix;
				p_pos_history[index].Position = obj_update.m_Pos;
                
				Mth::Vector vel;
				vel = p_pos_history[index].Position - p_pos_history[last_index].Position;
				//Dbg_Printf( "(%d) Vel: %f %f %f\n", timestamp, vel[X], vel[Y], vel[Z] );

				if( !( update_flags & GameNet::mUPDATE_FIELD_STATE ))
				{
					state = p_pos_history[last_index].State;
					doing_trick = p_pos_history[last_index].DoingTrick;
					terrain = p_pos_history[last_index].Terrain;
					walking = p_pos_history[last_index].Walking;
					driving = p_pos_history[last_index].Driving;
				}
				if( !( update_flags & GameNet::mUPDATE_FIELD_FLAGS ))
				{
					skater_flags = p_pos_history[last_index].SkaterFlags;
					end_run_flags = p_pos_history[last_index].EndRunFlags;
				}
				if( !( update_flags & GameNet::mUPDATE_FIELD_RAIL_NODE ))
				{
					rail_node = p_pos_history[last_index].RailNode;
				}
                
				p_pos_history[index].SkaterFlags = skater_flags;
				p_pos_history[index].EndRunFlags = end_run_flags;
				p_pos_history[index].RailNode = rail_node;
				p_pos_history[index].State = state;
				p_pos_history[index].DoingTrick = doing_trick;
				p_pos_history[index].Terrain = (ETerrainType) terrain;
				p_pos_history[index].Walking = walking;
				p_pos_history[index].Driving = driving;

				// Update the lo and hi words of the timestamp
				p_pos_history[index].HiTime = p_pos_history[last_index].HiTime;

				// If the lo-word has flipped, increment the hi-word
				if( p_pos_history[last_index].LoTime > timestamp )
				{
					p_pos_history[index].HiTime++;
				}
				p_pos_history[index].LoTime = timestamp;
				//Dbg_Printf( "(%d) Got time: %d\n", context->m_App->m_FrameCounter, p_pos_history[index].GetTime());
				
				
				skater_obj->GetStateHistory()->IncrementNumPosUpdates();				
			}
		}
	}
			
	return Net::HANDLER_CONTINUE;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::StartServer( void )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance(); 
	Net::Server* server;
	bool net_game;

	net_game = false;
	// prevents fragmentation
	//CFuncs::ScriptCleanup( NULL, NULL );
	if( gamenet_man->InNetMode())
	{	
		net_game = true;
		gamenet_man->ClientShutdown();
	}
	
	server = gamenet_man->SpawnServer( !net_game, true );
	if( server == NULL )
	{
		return;
	}

	mlp_manager->AddLogicTask ( *m_object_update_task );
	mlp_manager->AddLogicTask ( *m_score_update_task );
	
	// These handlers remaio active throughout the server's life
	server->m_Dispatcher.AddHandler( GameNet::MSG_ID_SCORE, handle_score_request );
	server->m_Dispatcher.AddHandler( GameNet::MSG_ID_LANDED_TRICK, handle_landed_trick, 0, this );
	server->m_Dispatcher.AddHandler( GameNet::MSG_ID_COMBO_REPORT, handle_combo_report, 0, this );
    server->m_Dispatcher.AddHandler( GameNet::MSG_ID_CHAT, handle_msg_relay, Net::mHANDLE_LATE );
	server->m_Dispatcher.AddHandler( GameNet::MSG_ID_BAIL_DONE, handle_bail_done, Net::mHANDLE_LATE );
	server->m_Dispatcher.AddHandler( GameNet::MSG_ID_OBJ_UPDATE_STREAM, handle_object_update, 
									 Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN );

	// In Split Screen Games, Don't relay any messages
	if( !server->IsLocal())
	{
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_FACE_DATA, handle_face_data );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_SPAWN_PROJECTILE, handle_msg_relay, 0, this );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_ENTER_VEHICLE, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_ENTER_VEHICLE, handle_enter_vehicle_server );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_PRIM_ANIM_START, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_SET_WOBBLE_TARGET, handle_selective_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_ROTATE_SKATEBOARD, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_ROTATE_DISPLAY, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_CLEAR_ROTATE_DISPLAY, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_CREATE_SPECIAL_ITEM, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_DESTROY_SPECIAL_ITEM, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_FLIP_ANIM, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_SET_WOBBLE_DETAILS, handle_selective_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_SET_LOOPING_TYPE, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_SET_HIDE_ATOMIC, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_SET_ANIM_SPEED, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_PLAY_SOUND, handle_selective_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_PLAY_LOOPING_SOUND, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_BLOOD_ON, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_BLOOD_OFF, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_SPARKS_ON, handle_msg_relay );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_SPARKS_OFF, handle_msg_relay, Net::mHANDLE_LATE );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_RUN_SCRIPT, handle_script_relay, Net::mHANDLE_LATE );
		server->m_Dispatcher.AddHandler( GameNet::MSG_ID_SPAWN_AND_RUN_SCRIPT, handle_script_relay, Net::mHANDLE_LATE );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::LaunchNetworkGame( void )
{
	RestartLevel();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Skate::AddNetworkMsgHandlers( Net::Client* client, int index )
{
	Obj::CSkater* skater;
	
	Dbg_Assert( client );
    
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_START_INFO, handle_start_info, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_OBJ_UPDATE_STREAM, handle_object_update_relay, 
									 Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_PRIM_ANIM_START, handle_anims, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_TOGGLE_CHEAT, handle_cheats, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SET_WOBBLE_TARGET, handle_anims, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_ROTATE_DISPLAY, handle_anims, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_CLEAR_ROTATE_DISPLAY, handle_anims, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_CREATE_SPECIAL_ITEM, handle_anims, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_DESTROY_SPECIAL_ITEM, handle_anims, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_ROTATE_SKATEBOARD, handle_anims, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_FLIP_ANIM, handle_anims, 0, this, Net::HIGHEST_PRIORITY );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_FLIP_ANIM, handle_flip_anim, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SET_WOBBLE_DETAILS, handle_anims, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SET_LOOPING_TYPE, handle_anims, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SET_HIDE_ATOMIC, handle_anims, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SET_ANIM_SPEED, handle_anims, 0, this );     
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_PLAYER_QUIT, handle_player_quit, 
									 Net::mHANDLE_LATE, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_HOST_QUIT, handle_disconn_accepted, 
									 Net::mHANDLE_LATE, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_PLAY_SOUND, handle_play_sound, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_PLAY_LOOPING_SOUND, handle_play_sound, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SPARKS_ON, handle_sparks, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SPARKS_OFF, handle_sparks, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_BLOOD_ON, handle_blood, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_BLOOD_OFF, handle_blood, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SCORE_UPDATE, handle_score_update, 
									 Net::mHANDLE_LATE, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_CHANGE_LEVEL, handle_change_level, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_RUN_SCRIPT, handle_run_script, Net::mHANDLE_LATE, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SPAWN_AND_RUN_SCRIPT, handle_spawn_run_script, Net::mHANDLE_LATE, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_KICKED, handle_kicked, Net::mHANDLE_LATE, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_LEFT_OUT, handle_kicked, Net::mHANDLE_LATE, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_OBSERVER_LOG_TRICK_OBJ, handle_obs_log_trick_objects, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_OBSERVER_INIT_GRAFFITI_STATE, handle_obs_init_graffiti_state, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_FACE_DATA, handle_face_data, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_CHEAT_LIST, handle_cheat_list, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SPAWN_PROJECTILE, handle_projectile, 0, this );
	client->m_Dispatcher.AddHandler( GameNet::MSG_ID_ENTER_VEHICLE, handle_enter_vehicle_client, 0, this );

	for( uint i = 0; i < vMAX_SKATERS; i++ )
	{
		skater = GetSkater( i );
		if( skater && skater->IsLocalClient() && ( skater->GetHeapIndex() == index ))
		{
			client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SCORE, Mdl::Score::s_handle_score_message, 0, skater );
			client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SKATER_COLLIDE_WON, Obj::CSkaterStateHistoryComponent::sHandleCollision, 0, skater );
			client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SKATER_COLLIDE_LOST, Obj::CSkaterStateHistoryComponent::sHandleCollision, 0, skater );
			client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SKATER_PROJECTILE_HIT_TARGET, Obj::CSkaterStateHistoryComponent::sHandleProjectileHit, 0, skater );
			client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SKATER_HIT_BY_PROJECTILE, Obj::CSkaterStateHistoryComponent::sHandleProjectileHit, 0, skater );
			client->m_Dispatcher.AddHandler( GameNet::MSG_ID_STEAL_MESSAGE, Obj::CSkaterLocalNetLogicComponent::sHandleStealMessage, 0, skater );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Skate::LeaveServer( void )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Net::Client* client;
    
	
    
	if( gamenet_man->OnServer())
	{   
		Net::App* server;
		Net::MsgDesc msg_desc;

		server = gamenet_man->GetServer();
		Dbg_Assert( server );

		// If we were auto serving, shut that service down
		if( gamenet_man->GetHostMode() == GameNet::vHOST_MODE_AUTO_SERVE )
		{
			gamenet_man->AutoServerShutdown();
		}

		// First, wait for any pending network tasks to finish before sending out final disconn
		// message
#ifdef __PLAT_NGPS__
		//server->WaitForAsyncCallsToFinish();
#endif  		

		// Send off last message, notifying clients that I'm about to quit
		Dbg_Printf( "Starting server shutdown\n" );
		msg_desc.m_Id = GameNet::MSG_ID_HOST_QUIT;
		msg_desc.m_Priority = Net::HIGHEST_PRIORITY;
		server->EnqueueMessage( Net::HANDLE_ID_BROADCAST, &msg_desc );
		// Wake the network thread up to send the data
		server->SendData();
		
		// Wait for that final send to complete
#ifdef __PLAT_NGPS__
		//server->WaitForAsyncCallsToFinish();
#endif  
		gamenet_man->ServerShutdown();

		m_object_update_task->Remove();
		m_score_update_task->Remove();
	}
	else
	{
		client = gamenet_man->GetClient( 0 );
		if( client )
		{
			Net::MsgDesc msg_desc;
			// First, wait for any pending network tasks to finish before sending out final disconn
			// message
#ifdef __PLAT_NGPS__
			//client->WaitForAsyncCallsToFinish();
#endif  		
			Dbg_Printf( "Leaving server\n" );
			
			msg_desc.m_Id = Net::MSG_ID_DISCONN_REQ;
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
			// Send off last message, notifying server that I'm about to quit
			client->EnqueueMessageToServer( &msg_desc );
			// Wake the network thread up to send the data
			client->SendData();
		
			// Wait for that final send to complete
#ifdef __PLAT_NGPS__
			//client->WaitForAsyncCallsToFinish();
#endif  

		}
	}
    
	gamenet_man->DisconnectFromServer();
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Skate::SetLaunchingQueuedScripts( void )
{
	m_launching_queued_scripts = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Skate::ClearLaunchingQueuedScripts( void )
{
	m_launching_queued_scripts = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Skate::LaunchingQueuedScripts( void )
{
	return m_launching_queued_scripts;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Skate::SendCheatList( GameNet::PlayerInfo* player )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	GameNet::MsgEnabledCheats cheat_msg;
	Net::MsgDesc msg_desc;

	cheat_msg.m_NumCheats = 0;

	if( mp_career->GetCheat( CRCD(0xeefecf1b, "CHEAT_ALWAYS_SPECIAL")))
	{
		cheat_msg.m_Cheats[cheat_msg.m_NumCheats++] = CRCD(0xeefecf1b, "CHEAT_ALWAYS_SPECIAL");
	}
	if( mp_career->GetCheat( CRCD(0xb38341c9, "CHEAT_PERFECT_MANUAL")))
	{
		cheat_msg.m_Cheats[cheat_msg.m_NumCheats++] = CRCD(0xb38341c9, "CHEAT_PERFECT_MANUAL");
	}
	if( mp_career->GetCheat( CRCD(0xcd09e062, "CHEAT_PERFECT_RAIL")))
	{
		cheat_msg.m_Cheats[cheat_msg.m_NumCheats++] = CRCD(0xcd09e062, "CHEAT_PERFECT_RAIL");
	}
	if( mp_career->GetCheat( CRCD(0x69a1ce96, "CHEAT_PERFECT_SKITCH")))
	{
		cheat_msg.m_Cheats[cheat_msg.m_NumCheats++] = CRCD(0x69a1ce96, "CHEAT_PERFECT_SKITCH");
	}
	if( mp_career->GetCheat( CRCD(0xeefecf1b, "CHEAT_ALWAYS_SPECIAL")))
	{
		cheat_msg.m_Cheats[cheat_msg.m_NumCheats++] = CRCD(0xeefecf1b, "CHEAT_ALWAYS_SPECIAL");
	}
	if( mp_career->GetCheat( CRCD(0x6b0f24bc, "CHEAT_STATS_13")))
	{
		cheat_msg.m_Cheats[cheat_msg.m_NumCheats++] = CRCD(0x6b0f24bc, "CHEAT_STATS_13");
	}
	if( mp_career->GetCheat( CRCD(0x520a66ef,"FLAG_G_EXPERT_MODE_NO_REVERTS")))
	{
		cheat_msg.m_Cheats[cheat_msg.m_NumCheats++] = CRCD(0x520a66ef,"FLAG_G_EXPERT_MODE_NO_REVERTS");
	}
	if( mp_career->GetCheat( CRCD(0xeba89179,"FLAG_G_EXPERT_MODE_NO_WALKING")))
	{
		cheat_msg.m_Cheats[cheat_msg.m_NumCheats++] = CRCD(0xeba89179,"FLAG_G_EXPERT_MODE_NO_WALKING");
	}
	if( mp_career->GetCheat( CRCD(0xc4ffc672,"FLAG_G_EXPERT_MODE_NO_MANUALS")))
	{
		cheat_msg.m_Cheats[cheat_msg.m_NumCheats++] = CRCD(0xc4ffc672,"FLAG_G_EXPERT_MODE_NO_MANUALS");
	}
	if( mp_career->GetCheat( CRCD(0xd5982173,"NO_G_DISPLAY_BALANCE")))
	{
		cheat_msg.m_Cheats[cheat_msg.m_NumCheats++] = CRCD(0xd5982173,"NO_G_DISPLAY_BALANCE");
	}

	// Send it, even if there are zero cheats. This triggers the clearing of all cheats on
	// the client side
	msg_desc.m_Id = GameNet::MSG_ID_CHEAT_LIST;
	msg_desc.m_Data = &cheat_msg;
	msg_desc.m_Length = sizeof( int ) +( sizeof( uint32 ) * cheat_msg.m_NumCheats );

	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
	
	gamenet_man->GetServer()->EnqueueMessage( player->GetConnHandle(), &msg_desc );

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	Skate::GetCheatChecksum( void )
{
	uint32 checksum;

	checksum = 0;
	
	if( mp_career->GetCheat( CRCD(0xeefecf1b, "CHEAT_ALWAYS_SPECIAL")))
	{
		checksum |= CRCD(0xeefecf1b, "CHEAT_ALWAYS_SPECIAL");
	}
	if( mp_career->GetCheat( CRCD(0xb38341c9, "CHEAT_PERFECT_MANUAL")))
	{
		checksum |= CRCD(0xb38341c9, "CHEAT_PERFECT_MANUAL");
	}
	if( mp_career->GetCheat( CRCD(0xcd09e062, "CHEAT_PERFECT_RAIL")))
	{
		checksum |= CRCD(0xcd09e062, "CHEAT_PERFECT_RAIL");
	}
	if( mp_career->GetCheat( CRCD(0x69a1ce96, "CHEAT_PERFECT_SKITCH")))
	{
		checksum |= CRCD(0x69a1ce96, "CHEAT_PERFECT_SKITCH");
	}
	if( mp_career->GetCheat( CRCD(0xeefecf1b, "CHEAT_ALWAYS_SPECIAL")))
	{
		checksum |= CRCD(0xeefecf1b, "CHEAT_ALWAYS_SPECIAL");
	}
	if( mp_career->GetCheat( CRCD(0x6b0f24bc, "CHEAT_STATS_13")))
	{
		checksum |= CRCD(0x6b0f24bc, "CHEAT_STATS_13");
	}
	if( mp_career->GetCheat( CRCD(0x520a66ef,"FLAG_G_EXPERT_MODE_NO_REVERTS")))
	{
		checksum |= CRCD(0x520a66ef,"FLAG_G_EXPERT_MODE_NO_REVERTS");
	}
	if( mp_career->GetCheat( CRCD(0xeba89179,"FLAG_G_EXPERT_MODE_NO_WALKING")))
	{
		checksum |= CRCD(0xeba89179,"FLAG_G_EXPERT_MODE_NO_WALKING");
	}
	if( mp_career->GetCheat( CRCD(0xc4ffc672,"FLAG_G_EXPERT_MODE_NO_MANUALS")))
	{
		checksum |= CRCD(0xc4ffc672,"FLAG_G_EXPERT_MODE_NO_MANUALS");
	}
	if( mp_career->GetCheat( CRCD(0xd5982173,"NO_G_DISPLAY_BALANCE")))
	{
		checksum |= CRCD(0xd5982173,"NO_G_DISPLAY_BALANCE");
	}

	return checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mdl
