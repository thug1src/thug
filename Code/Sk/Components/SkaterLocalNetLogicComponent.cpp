//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterLocalNetLogicComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/12/3
//****************************************************************************

#include <core/math/slerp.h>

#include <sk/components/skaterlocalnetlogiccomponent.h>
#include <sk/components/skaterstatecomponent.h>
#include <sk/components/skaterendruncomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/modules/skate/skate.h>
#include <sk/gamenet/gamenet.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/net/client/netclnt.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterLocalNetLogicComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterLocalNetLogicComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterLocalNetLogicComponent::CSkaterLocalNetLogicComponent() : CBaseComponent()
{
	SetType( CRC_SKATERLOCALNETLOGIC );
	
	mp_state_component = NULL;
	mp_physics_component = NULL;
	m_last_update_time = 0;
	m_last_sent_terrain = -1;
	m_last_sent_flags = 0;
	m_last_sent_state = 0;
	m_last_sent_doing_trick = -1;
	m_last_sent_rail = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterLocalNetLogicComponent::~CSkaterLocalNetLogicComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLocalNetLogicComponent::InitFromStructure( Script::CStruct* pParams )
{
	Dbg_MsgAssert(GetObject()->GetType() == SKATE_TYPE_SKATER, ("CSkaterLocalNetLogicComponent added to non-skater composite object"));
	Dbg_MsgAssert(GetSkater()->IsLocalClient(), ("CSkaterLocalNetLogicComponent added to non-local skater"));
	
	m_last_update_time = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLocalNetLogicComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLocalNetLogicComponent::Finalize (   )
{
	
	mp_state_component = GetSkaterStateComponentFromObject(GetObject());
	mp_physics_component = GetSkaterCorePhysicsComponentFromObject(GetObject());
	
	Dbg_Assert(mp_state_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLocalNetLogicComponent::Update()
{
	if (!Mdl::Skate::Instance()->IsMultiplayerGame())
	{
		//Suspend(true);
		return;
	}
	if( GameNet::Manager::Instance()->GetLocalPlayer() == NULL )
	{
		return;
	}
		
	network_update();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterLocalNetLogicComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLocalNetLogicComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterLocalNetLogicComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}


/******************************************************************/
/* Print a graffiti steal message  								  */
/* 						  										  */
/******************************************************************/

int	CSkaterLocalNetLogicComponent::sHandleStealMessage( Net::MsgHandlerContext* context )
{
	GameNet::MsgStealMessage* p_msg = (GameNet::MsgStealMessage *) context->m_Msg;
	
	if (p_msg->m_GameId != GameNet::Manager::Instance()->GetNetworkGameId()) return Net::HANDLER_MSG_DONE;

	Obj::CSkater* pCurrSkater = (CSkater*) context->m_Data;
	Dbg_Assert( pCurrSkater );

	// For now, just exit out in these cases to avoid a crash
	Obj::CSkater* pNewSkater = Mdl::Skate::Instance()->GetSkaterById( p_msg->m_NewOwner );
	if( pNewSkater == NULL )
	{
		return Net::HANDLER_CONTINUE;
	}

	Obj::CSkater* pOldSkater = Mdl::Skate::Instance()->GetSkaterById( p_msg->m_OldOwner );
	if( pOldSkater == NULL )
	{
		return Net::HANDLER_CONTINUE;
	}

	// TODO:  Maybe send the color of this skater's graffiti tags
	
	if ( pCurrSkater->GetID() == (uint32)p_msg->m_NewOwner )
	{
		Script::CStruct* pTempStructure = new Script::CStruct;
		pTempStructure->Clear();
		pTempStructure->AddComponent( CRCD(0xa4b08520, "String0"), ESYMBOLTYPE_STRING, pOldSkater->GetDisplayName() );
		Script::RunScript( CRCD(0xb3ff911, "GraffitiStealYou"), pTempStructure, pCurrSkater );
		delete pTempStructure;
	}
	else if ( pCurrSkater->GetID() == (uint32)p_msg->m_OldOwner )
	{
		Script::CStruct* pTempStructure = new Script::CStruct;
		pTempStructure->Clear();
		pTempStructure->AddComponent( CRCD(0xa4b08520, "String0"), ESYMBOLTYPE_STRING, pNewSkater->GetDisplayName() );
		Script::RunScript( CRCD(0x4d7f6ffa, "GraffitiStealOther"), pTempStructure, pCurrSkater );
		delete pTempStructure;
	}
	else
	{
		// useless steal message encountered
		Dbg_Assert( 0 );
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	CSkaterLocalNetLogicComponent::get_update_flags()
{
	Net::Client* client;
	Mth::Vector eulers;
	short pos[3];
	short rot[3];
	char doing_trick, state, terrain, walking, driving;
	Flags< int > skater_flags;
	Flags< int > end_run_flags;
	int i;
	bool on_server, force_send;   
	int update_flags;
	sint16 rail_node;
	Obj::CSkaterEndRunComponent* p_skater_endrun_component;

	update_flags = 0;

	client = GameNet::Manager::Instance()->GetClient( GetSkater()->m_skater_number );
	Dbg_Assert( client );

	p_skater_endrun_component = GetSkaterEndRunComponentFromObject(GetSkater());

	on_server = GameNet::Manager::Instance()->OnServer();
	force_send = on_server || ( !( client->m_FrameCounter % vFORCE_SEND_INTERVAL )); 

	GetObject()->GetDisplayMatrix().GetEulers( eulers );
	
	state = (char) mp_state_component->m_state;
	terrain = (char) mp_state_component->m_terrain;
	doing_trick = (char) mp_state_component->m_doing_trick;
	walking = (char) (mp_state_component->m_physics_state == WALKING ? true : false);
	driving = (char) mp_state_component->m_driving;
	rail_node = mp_physics_component->GetRailNode();

	for( i = 0; i < NUM_ESKATERFLAGS; i++ )
	{     		
		skater_flags.Set( i, mp_state_component->m_skater_flags[( ESkaterFlag ) i ].Get() );
	}
	end_run_flags = p_skater_endrun_component->GetFlags();

	for( i = 0; i < 3; i++ )
	{
		if( i == Y )
		{
			pos[i] = (short) ( GetObject()->m_pos[i] * 4.0f );
			//pos[i] = GetObject()->m_pos[i];
		}
		else
		{
			pos[i] = (short) ( GetObject()->m_pos[i] * 2.0f );
			//pos[i] = GetObject()->m_pos[i];
		}
		if(( pos[i] != m_last_sent_pos[i] ) || force_send )
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
    
	for( i = 0; i < 3; i++ )
	{
		rot[i] = (short) ( eulers[i] * 4096.0f );
		if(( rot[i] != m_last_sent_rot[i] ) || force_send )
		{   
			m_last_sent_rot[i] = rot[i];
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
    
	if( ( state != m_last_sent_state ) ||
		( doing_trick != m_last_sent_doing_trick ) ||
		( terrain != m_last_sent_terrain ) ||
		( walking != m_last_sent_walking ) ||
		( driving != m_last_sent_driving ) ||
		( force_send ))
	{
			update_flags |= GameNet::mUPDATE_FIELD_STATE;
	}

    if(( skater_flags != m_last_sent_flags ) || 
	   ( end_run_flags != m_last_sent_end_run_flags ) || 
	   ( force_send ))
	{
		update_flags |= GameNet::mUPDATE_FIELD_FLAGS;
	}

	if( rail_node != m_last_sent_rail )
	{
		update_flags |= GameNet::mUPDATE_FIELD_RAIL_NODE;
	}

	return update_flags;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLocalNetLogicComponent::network_update ( void )
{
	// update the server with our current state so that he can relay the data to other clients
	
	Net::Client* client;
	Net::MsgMax p_msg;
	Mth::Vector eulers;
	short pos[3];
	short rot[3];
	char doing_trick, state, terrain, walking, driving;
	Flags< int > skater_flags;
	Flags< int > end_run_flags;
	int i, msg_len;
	sint16 rail_node;
	int update_flags;
	bool on_server, force_send;   
	Net::MsgDesc msg_desc;
	Net::BitStream stream;
	Obj::CSkaterEndRunComponent* p_skater_endrun_component;

	client = GameNet::Manager::Instance()->GetClient( GetSkater()->m_skater_number );
	Dbg_MsgAssert( client, ( "Could not find client: %d\n", GetSkater()->m_skater_number ));

	p_skater_endrun_component = GetSkaterEndRunComponentFromObject(GetSkater());

	on_server = GameNet::Manager::Instance()->OnServer();
	force_send = on_server || ( !( client->m_FrameCounter % vFORCE_SEND_INTERVAL )); 

	// Send object updates to narrowband servers only every other frame (except for forced-send frames)
	if( !on_server && !force_send )
	{
		Lst::Search< Net::Conn > sh;
		Net::Conn* server_conn;
		
		server_conn = client->FirstConnection( &sh );
		Dbg_Assert( server_conn );

		// Only send object updates as often as we send packets
		if(( client->m_Timestamp - m_last_update_time ) < (unsigned int) server_conn->GetSendInterval())
		{
			return;
		}
	}

	m_last_update_time = client->m_Timestamp;

	stream.SetOutputData( p_msg.m_Data, 1024 );
	
	GetObject()->GetDisplayMatrix().GetEulers( eulers );
	
	state = (char) mp_state_component->m_state;
	terrain = (char) mp_state_component->m_terrain;
	doing_trick = (char) mp_state_component->m_doing_trick;
	walking = (char) (mp_state_component->m_physics_state == WALKING ? true : false);
	driving = (char) mp_state_component->m_driving;
	rail_node = mp_physics_component->GetRailNode();

	for( i = 0; i < NUM_ESKATERFLAGS; i++ )
	{     		
		skater_flags.Set( i, mp_state_component->m_skater_flags[( ESkaterFlag ) i ].Get() );
	}
	
	end_run_flags = p_skater_endrun_component->GetFlags();

	// Write out the time for which this state info is valid
	stream.WriteValue( client->m_Timestamp, sizeof( int ) * 8 );
	update_flags = get_update_flags();
	stream.WriteValue( update_flags, 9 );

	if( on_server )
	{
		static Mth::Vector last_pos;
		Mth::Vector diff;

		diff = GetObject()->m_pos - last_pos;
		//Dbg_Printf( "Vel: %f %f %f\n", diff[X], diff[Y], diff[Z] );
		last_pos = GetObject()->m_pos;
	}

// Write out the object's position as three shorts (fixed-point)
	for( i = 0; i < 3; i++ )
	{
		if( i == Y )
		{
			pos[i] = (short) ( GetObject()->m_pos[i] * 4.0f );
			//pos[i] = GetObject()->m_pos[i];
		}
		else
		{
			pos[i] = (short) ( GetObject()->m_pos[i] * 2.0f );
			//pos[i] = GetObject()->m_pos[i];
		}
		if( i == X )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_POS_X )
			{
				stream.WriteValue( pos[i], sizeof( short ) * 8 );
				//stream.WriteFloatValue( pos[i] );
			}
		}
		else if( i == Y )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_POS_Y )
			{
				stream.WriteValue( pos[i], sizeof( short ) * 8 );
				//stream.WriteFloatValue( pos[i] );
			}
		}
		else if( i == Z )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_POS_Z )
			{
				stream.WriteValue( pos[i], sizeof( short ) * 8 );
				//stream.WriteFloatValue( pos[i] );
			}
		}

		m_last_sent_pos[i] = pos[i];
	}
    
	// Write out the object's orientation as three short euler angles (fixed-point)
	for( i = 0; i < 3; i++ )
	{
		rot[i] = (short) ( eulers[i] * 4096.0f );
		if( i == X )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_ROT_X )
			{
				stream.WriteValue( rot[i], sizeof( short ) * 8 );
			}
		}
		else if( i == Y )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_ROT_Y )
			{
				stream.WriteValue( rot[i], sizeof( short ) * 8 );
			}
		}
		else if( i == Z )
		{
			if( update_flags & GameNet::mUPDATE_FIELD_ROT_Z )
			{
				stream.WriteValue( rot[i], sizeof( short ) * 8 );
			}
		}

		m_last_sent_rot[i] = rot[i];
	}
    
	// Write out the skater's 'state'
	// Write out the skater's 'doing trick' state
	// Write out the skater's terrain
	if( update_flags & GameNet::mUPDATE_FIELD_STATE )
	{
		char mask;

		m_last_sent_state = state;
		m_last_sent_doing_trick = doing_trick;
		m_last_sent_terrain = terrain;
		m_last_sent_walking = walking;
		m_last_sent_driving = driving;

		mask = state;
		if( doing_trick )
		{
			mask |= GameNet::mDOING_TRICK_MASK;
		}
		
		stream.WriteValue( mask, 4 );
		stream.WriteValue( terrain, 6 );
		stream.WriteValue( walking, 1 );
		stream.WriteValue( driving, 1 );
	}

    // Write out the skaters' flags
	if( update_flags & GameNet::mUPDATE_FIELD_FLAGS )
	{
		m_last_sent_flags = skater_flags;
		m_last_sent_end_run_flags = end_run_flags;
		stream.WriteValue( skater_flags, 5 );
		stream.WriteValue( end_run_flags, 3 );
	}

	if( update_flags & GameNet::mUPDATE_FIELD_RAIL_NODE )
	{
		m_last_sent_rail = rail_node;
		stream.WriteValue( rail_node, sizeof( sint16 ) * 8 );
	}

	stream.Flush();
	msg_len = stream.GetByteLength();

	msg_desc.m_Id = GameNet::MSG_ID_OBJ_UPDATE_STREAM;
	msg_desc.m_Length = msg_len;
	msg_desc.m_Data = &p_msg;
	msg_desc.m_Singular = true;
	msg_desc.m_Priority = Net::NORMAL_PRIORITY + 1;
	client->EnqueueMessageToServer( &msg_desc );
}
                                           
}