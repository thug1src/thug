//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterStateHistoryComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/13/3
//****************************************************************************

#include <sk/components/skaterstatehistorycomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/walkcomponent.h>
#include <gel/net/server/netserv.h>

#include <sk/objects/crown.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/gamenet/gameNET.h>
#include <sk/gamenet/gamemsg.h>
#include <sk/scripting/cfuncs.h>
#include <sk/components/skaterscorecomponent.h>
#include <sk/components/skatercorephysicscomponent.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterStateHistoryComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterStateHistoryComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterStateHistoryComponent::CSkaterStateHistoryComponent() : CBaseComponent()
{
	SetType( CRC_SKATERSTATEHISTORY );

	m_last_anm_time = 0;
	m_num_pos_updates = 0;
	m_num_anim_updates = 0;
	memset( mp_pos_history, 0, sizeof( SPosEvent ) * vNUM_POS_HISTORY_ELEMENTS );
	memset( mp_anim_history, 0, sizeof( SAnimEvent ) * vNUM_ANIM_HISTORY_ELEMENTS );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterStateHistoryComponent::~CSkaterStateHistoryComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStateHistoryComponent::InitFromStructure( Script::CStruct* pParams )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStateHistoryComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStateHistoryComponent::Update()
{
	Suspend(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterStateHistoryComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
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

void CSkaterStateHistoryComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterStateHistoryComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterStateHistoryComponent::CheckForCrownCollision(   )
{
	Obj::CCrown* crown = GameNet::Manager::Instance()->GetCrown();
	Dbg_Assert(crown && !crown->OnKing());
	
	return (get_latest_position() - crown->GetPosition()).Length() < CCrown::vCROWN_RADIUS;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStateHistoryComponent::CollideWithOtherSkaters( int start_index )
{
    GameNet::PlayerInfo* p_player;

	p_player = GameNet::Manager::Instance()->GetPlayerByObjectID(GetObject()->GetID());
	if( p_player == NULL )
	{
		return;
	}

	// If we've marked you as non-collidable, you are exempt from collision
	// In koth, if collision is turned off, only the king is collidable
	if (p_player->IsNonCollidable()
		|| (!GameNet::Manager::Instance()->PlayerCollisionEnabled() && !p_player->IsKing() && !p_player->HasCTFFlag())) return;
	
	CSkaterStateComponent* p_my_state_component = GetSkaterStateComponentFromObject(GetSkater());
	Dbg_Assert(p_my_state_component);

	// If this p_player is bailing or noncollidable (and he's NOT the king) he is exempt from being smacked down
	// If you're in a slap game, you can always be hit; i.e. you've just been slapped but haven't teleported yet
    bool can_fall = !p_my_state_component->GetFlag(IS_BAILING) || CFuncs::ScriptInSlapGame(NULL, NULL);
	
	// However, fallen kings are vulnerable
	if (!can_fall && !p_player->IsKing() && !p_player->HasCTFFlag()) return;
	
	bool my_driving = p_my_state_component->GetDriving();
	Mth::Vector my_pos = get_latest_position();
	Mth::Vector my_vel = get_vel();
	Mth::Vector my_dir = my_vel;
	my_dir.Normalize();

	Mth::Line my_line;
	my_line.m_start = my_pos;

	// Loop through all other skaters and check for collisions
	for (int i = start_index; i < Mdl::Skate::vMAX_SKATERS; i++)
	{
		CSkater* p_other_skater = Mdl::Skate::Instance()->GetSkater(i);

		if (!p_other_skater
			|| p_other_skater == GetSkater()) continue;
		
		CSkaterStateComponent* p_other_state_component = GetSkaterStateComponentFromObject(p_other_skater);
		Dbg_Assert(p_other_state_component);
		if (p_other_state_component->GetFlag(IS_BAILING)) continue;
		
		GameNet::PlayerInfo* p_other_player = p_other_player = GameNet::Manager::Instance()->GetPlayerByObjectID(p_other_skater->GetID());
		Dbg_Assert(p_other_player);
		
		// Non-Collidable people and kings should never win
		if (p_other_player->IsNonCollidable() || p_other_player->IsKing()) continue;

		// If both players are carrying flags, and the subject doesn't have the other guy's flag, don't let them collide
		if (p_player->HasCTFFlag() && p_other_player->HasCTFFlag() && p_player->HasWhichFlag() != p_other_player->m_Team) continue;

		// No smacking teammates
		if (Mdl::Skate::Instance()->GetGameMode()->IsTeamGame() && p_other_player->m_Team == p_player->m_Team) continue;

		bool other_driving = p_other_state_component->GetDriving();
		Mth::Vector other_pos = p_other_skater->mp_skater_state_history_component->get_latest_position();
		Mth::Vector other_vel = p_other_skater->mp_skater_state_history_component->get_vel();
		Mth::Vector other_dir = other_vel;
		other_dir.Normalize();
		
		if( other_vel.Length() > 100.0f )
		{
			continue;
		}
		
		bool collided = false;
		
		// Collision extents are based upon the "other" skater -- whether or not he's a remote skater
		
		float collide_dist = get_collision_cylinder_radius(my_driving, other_driving);
		float driving_radius_multiplier = 1.0f;
		if (my_driving)
		{
			driving_radius_multiplier += Script::GetFloat("driving_radius_boost");
		}
		if (other_driving)
		{
			driving_radius_multiplier += Script::GetFloat("driving_radius_boost");
		}
		
		my_line.m_end = my_line.m_start + my_dir * get_collision_cylinder_coeff(my_driving);
		
		Mth::Line other_line;
		other_line.m_start = other_pos;
		other_line.m_end = other_line.m_start + other_dir * p_other_skater->mp_skater_state_history_component->get_collision_cylinder_coeff(other_driving);
		
		float temp;
		Mth::Vector my_pt, other_pt;
		if (Mth::LineLineIntersect(my_line, other_line, &my_pt, &other_pt, &temp, &temp, true))
		{
			if ((my_pt - other_pt).Length() < collide_dist)
			{
				collided = true;
			}   
		}
		else
		{
			// No solution exists -- lines must be parallel. Try testing endpoint lengths
			// Note: This only really works if velocities are relatively small
			if( (other_line.m_start - my_line.m_start).Length() < collide_dist)
			{
				collided = true;
				other_pt = other_line.m_start;
				my_pt = my_line.m_start;
			}
			else if ((other_line.m_start - my_line.m_end).Length() < collide_dist)
			{
				collided = true;
				other_pt = other_line.m_start;
				my_pt = my_line.m_end;
			}
			else if ((other_line.m_end - my_line.m_start).Length() < collide_dist)
			{
				collided = true;
				other_pt = other_line.m_end;
				my_pt = my_line.m_start;
			}
			else if ((other_line.m_end - my_line.m_end).Length() < collide_dist)
			{
				collided = true;
				other_pt = other_line.m_end;
				my_pt = my_line.m_end;
			}
		}
		
		if (!collided) continue;
		
		// If the "other" p_player is going faster or if
		// the subject p_player is king or if
		// the subject is skating and we're driving, the subject loses
		if (((my_driving == other_driving && my_vel.Length() > other_vel.Length())
			 || (my_driving && !other_driving)) && !p_player->IsKing()) continue;
		
		//Dbg_Printf( "**** My Vel: %.2f, Theirs: %.2f\n", my_vel.Length(), other_vel.Length() );
		Net::Server* p_server = GameNet::Manager::Instance()->GetServer();
		Dbg_Assert(p_server);
			
		if (can_fall)
		{
			GameNet::MsgCollideLost lost_msg;
			GameNet::MsgByteInfo won_msg;
			Net::MsgDesc msg_desc;

			// basically a one-byte message explaining "You've been knocked down by ID"
			lost_msg.m_Id = p_other_skater->GetID();
			lost_msg.m_Id |= (1 << 7) * other_driving;
			lost_msg.m_Offset = my_pt - other_pt;

			if (CInputComponent* p_input_component = GetInputComponentFromObject(GetObject()))
			{
				p_input_component->DisableInput();
			}
			p_player->MarkAsNonCollidable();
			
			msg_desc.m_Id = GameNet::MSG_ID_SKATER_COLLIDE_LOST;
			msg_desc.m_Data = &lost_msg;
			msg_desc.m_Length = sizeof(GameNet::MsgCollideLost);
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
			p_server->EnqueueMessage(p_player->GetConnHandle(), &msg_desc);

			// basically a one-byte message explaining "You knocked someone down"
			won_msg.m_Data = GetObject()->GetID();
			won_msg.m_Data |= (1 << 7) * my_driving;

			msg_desc.m_Id = GameNet::MSG_ID_SKATER_COLLIDE_WON;
			msg_desc.m_Data = &won_msg;
			msg_desc.m_Length = sizeof(GameNet::MsgByteInfo);
			p_server->EnqueueMessage(p_other_player->GetConnHandle(), &msg_desc);	
			
			if (CFuncs::ScriptInSlapGame(NULL, NULL))
			{
				Dbg_Assert(p_other_skater->mp_skater_score_component);
				Mdl::Score* score = p_other_skater->mp_skater_score_component->GetScore();
				score->SetTotalScore(score->GetTotalScore() + 1);
			}
		}
		
		if (Mdl::Skate::Instance()->GetGameMode()->GetNameChecksum() == CRCD(0x6ef8fda0, "netking")
			|| Mdl::Skate::Instance()->GetGameMode()->GetNameChecksum() == CRCD(0x5d32129c, "king"))
		{
			// Don't bother switching kings if the game is over
			if (Mdl::Skate::Instance()->GetGameMode()->EndConditionsMet()) return;
			
			// If the king was just slapped, the "slapper" (hehe) is the new king
			if (!p_player->IsKing()) continue;
			
			// It is important that we mark the king immediately (rather than through a
			// network message) since we do logic based on the "current" king
			p_other_player->MarkAsKing(true);
			
			Lst::Search< GameNet::PlayerInfo > search;
			for (GameNet::PlayerInfo* p_player = GameNet::Manager::Instance()->FirstPlayerInfo(search, true);
				p_player;
				p_player = GameNet::Manager::Instance()->NextPlayerInfo(search, true))
			{
				// Already marked the king for the local p_player (above)
				if (p_player->IsLocalPlayer()) continue;
				
				GameNet::MsgByteInfo msg;
				Net::MsgDesc msg_desc;
				
				msg.m_Data = p_other_skater->GetID();
				msg_desc.m_Data = &msg;
				msg_desc.m_Id = GameNet::MSG_ID_NEW_KING;
				msg_desc.m_Length = sizeof(GameNet::MsgByteInfo);
				msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
				msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
				p_server->EnqueueMessage(p_player->GetConnHandle(), &msg_desc);
			}
		}
		else if (Mdl::Skate::Instance()->GetGameMode()->GetNameChecksum() == CRCD(0x6c5ff266, "netctf"))
		{
			GameNet::Manager* gamenet_man = GameNet::Manager::Instance();
			// Don't bother handling this if the game is over
			//if (Mdl::Skate::Instance()->GetGameMode()->EndConditionsMet())
			if( gamenet_man->GameIsOver())
			{
				return;
			}
			
			if (!p_player->HasCTFFlag())
			{
				continue;
			}
			
			GameNet::MsgFlagMsg msg;
			Net::MsgDesc msg_desc;

			msg.m_ObjId = p_other_skater->GetID();
			msg.m_Team = p_player->HasWhichFlag();

			bool retrieve = p_player->HasWhichFlag() == p_other_player->m_Team;

			Lst::Search< GameNet::PlayerInfo > search;
			for (GameNet::PlayerInfo* send_player = GameNet::Manager::Instance()->FirstPlayerInfo(search, true);
				send_player; 
				send_player = GameNet::Manager::Instance()->NextPlayerInfo(search, true))
			{
				// If you knock down the p_player with your team's flag, you recover it. Otherwise, you steal it
				if (retrieve)
				{
					if (send_player->IsLocalPlayer())
					{
						p_other_player->RetrievedFlag();
						continue;
					}
					msg_desc.m_Data = &msg;
					msg_desc.m_Length = sizeof(GameNet::MsgFlagMsg);
					msg_desc.m_Id = GameNet::MSG_ID_RETRIEVED_FLAG;
					msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
					msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
					p_server->EnqueueMessage(send_player->GetConnHandle(), &msg_desc);
				}
				else
				{
					if (send_player->IsLocalPlayer())
					{
						p_other_player->StoleFlag(p_player->HasWhichFlag());
						continue;
					}
					msg_desc.m_Data = &msg;
					msg_desc.m_Length = sizeof(GameNet::MsgFlagMsg);
					msg_desc.m_Id = GameNet::MSG_ID_STOLE_FLAG;
					msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
					msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
					p_server->EnqueueMessage(send_player->GetConnHandle(), &msg_desc);
				}
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int			CSkaterStateHistoryComponent::sHandleProjectileHit ( Net::MsgHandlerContext* context )
{
	CSkater* p_skater = (CSkater*) (context->m_Data);
	GameNet::MsgProjectileHit* p_msg = (GameNet::MsgProjectileHit*) (context->m_Msg);
	
	switch (context->m_MsgId)
	{
		case GameNet::MSG_ID_SKATER_PROJECTILE_HIT_TARGET:
		{
			char name[32] = "Someone";
			
			if (GameNet::PlayerInfo* p_other_player = GameNet::Manager::Instance()->GetPlayerByObjectID(p_msg->m_Id))
			{
				strncpy(name, p_other_player->m_Skater->GetDisplayName(), 32);
			}

			Script::CStruct params;
			params.AddString(CRCD(0xa4b08520, "String0"), name);
			params.AddChecksum( NONAME, CRCD(0xd039432c,"fireball") );
			p_skater->SelfEvent(CRCD(0xa1021af0, "MadeOtherSkaterBail"),&params);
			
			break;
		}
		
		case GameNet::MSG_ID_SKATER_HIT_BY_PROJECTILE:
		{
			char name[32] = "Someone";

			if (GameNet::PlayerInfo* p_other_player = GameNet::Manager::Instance()->GetPlayerByObjectID(p_msg->m_Id))
			{
				GameNet::Manager* gamenet_man = GameNet::Manager::Instance();
				Mth::Vector offset = p_skater->GetPos() - p_other_player->m_Skater->GetPos();
				// Perform a "safe" normalization
				float offset_length = offset.Length();
				if (offset_length < 0.0001f)
				{
					// if skaters are super close, or coincident then pretend they are not, to avoid zero length vectors
					offset.Set(1.0f, 0.0f, 1.0f);
				}
				else
				{
					offset /= offset_length;
				}
				
				// fudge it for now
				// float speed = p_other_skater->GetVel().Length();
				float speed = 300.0f;
				p_skater->GetVel() = offset * speed * 1.5f; 
				p_skater->GetVel()[Y] = 200.0f;

				CSkaterPhysicsControlComponent* p_skater_physics_control_component = GetSkaterPhysicsControlComponentFromObject(p_skater);
				if (p_skater_physics_control_component->IsSkating())
				{
					CSkaterCorePhysicsComponent* p_skater_core_physics_component = GetSkaterCorePhysicsComponentFromObject(p_skater);
					p_skater_core_physics_component->CollideWithOtherSkaterLost(p_other_player->m_Skater);
				}
				else
				{
					CWalkComponent* p_walk_component = GetWalkComponentFromObject(p_skater);
					p_walk_component->CollideWithOtherSkaterLost(p_other_player->m_Skater);
				}

				strncpy(name, p_other_player->m_Skater->GetDisplayName(), 32);
				

				if( gamenet_man->OnServer() == false )
				{
					Mdl::Score* score = p_skater->mp_skater_score_component->GetScore();
					if(( score->GetTotalScore() - p_msg->m_Damage ) < 0 )
					{
						score->SetTotalScore( 0 );
					}
					else
					{
						score->SetTotalScore( score->GetTotalScore() - p_msg->m_Damage );
					}
				}
			}

			Script::CStruct params;
			params.AddString(CRCD(0xa4b08520, "String0"), name);
			params.AddChecksum( NONAME, CRCD(0xd039432c,"fireball") );
			p_skater->SelfEvent(CRCD(0x915e5e39, "SkaterCollideBail"),&params);
			break;
		}
		
		default:
			Dbg_Assert(false);
			break;
	}
		
	return Net::HANDLER_MSG_DONE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	CSkaterStateHistoryComponent::sHandleCollision( Net::MsgHandlerContext* context )
{
	CSkater* p_skater = (CSkater*) (context->m_Data);
	
	switch (context->m_MsgId)
	{
		case GameNet::MSG_ID_SKATER_COLLIDE_WON:
		{
			GameNet::MsgByteInfo* p_msg = (GameNet::MsgByteInfo*) (context->m_Msg);
			char name[32] = "Someone";
			
			bool loser_drivng = p_msg->m_Data & (1 << 7);
			p_msg->m_Data &= ~(1 << 7);
			
			if (GameNet::PlayerInfo* p_other_player = GameNet::Manager::Instance()->GetPlayerByObjectID(p_msg->m_Data))
			{
				strncpy(name, p_other_player->m_Skater->GetDisplayName(), 32);
			}

			Script::CStruct params;
			params.AddString(CRCD(0xa4b08520, "String0"), name);
			if (loser_drivng)
			{
				params.AddChecksum(NO_NAME, CRCD(0x7fd0663d, "LoserIsDriving"));
			}
			p_skater->SelfEvent(CRCD(0xa1021af0, "MadeOtherSkaterBail"),&params);
			
			break;
		}
		
		case GameNet::MSG_ID_SKATER_COLLIDE_LOST:
		{
			GameNet::MsgCollideLost* p_msg = (GameNet::MsgCollideLost*) (context->m_Msg);
			char name[32] = "Someone";
			
			bool winner_driving = p_msg->m_Id & (1 << 7);
			p_msg->m_Id &= ~(1 << 7);

			if (GameNet::PlayerInfo* p_other_player = GameNet::Manager::Instance()->GetPlayerByObjectID(p_msg->m_Id))
			{
				CSkaterPhysicsControlComponent* p_skater_physics_control_component = GetSkaterPhysicsControlComponentFromObject(p_skater);
				
				if (!p_skater_physics_control_component->IsDriving())
				{
					Mth::Vector offset = p_skater->GetPos() - p_other_player->m_Skater->GetPos();
					// Perform a "safe" normalization
					float offset_length = offset.Length();
					if (offset_length < 0.0001f)
					{
						// if skaters are super close, or coincident then pretend they are not, to avoid zero length vectors
						offset.Set(1.0f, 0.0f, 1.0f);
					}
					else
					{
						offset /= offset_length;
					}
					
					// fudge it for now
					// float speed = p_other_skater->GetVel().Length();
					float speed = 300.0f;
					p_skater->GetVel() = offset * speed * 1.5f; 
					p_skater->GetVel()[Y] = 200.0f;
	
					if (p_skater_physics_control_component->IsSkating())
					{
						CSkaterCorePhysicsComponent* p_skater_core_physics_component = GetSkaterCorePhysicsComponentFromObject(p_skater);
						p_skater_core_physics_component->CollideWithOtherSkaterLost(p_other_player->m_Skater);
					}
					else
					{
						CWalkComponent* p_walk_component = GetWalkComponentFromObject(p_skater);
						p_walk_component->CollideWithOtherSkaterLost(p_other_player->m_Skater);
					}
				}

				strncpy(name, p_other_player->m_Skater->GetDisplayName(), 32);
			}

			Script::CStruct params;
			params.AddString(CRCD(0xa4b08520, "String0"), name);
			params.AddVector(CRCD(0xa6f5352f, "Offset"), p_msg->m_Offset);
			if (winner_driving)
			{
				params.AddChecksum(NO_NAME, CRCD(0x2f679251, "WinnerIsDriving"));
			}
			p_skater->SelfEvent(CRCD(0x915e5e39, "SkaterCollideBail"),&params);
			break;
		}
		
		default:
			Dbg_Assert(false);
			break;
			
	}
		
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector	CSkaterStateHistoryComponent::get_vel (   )
{
	return GetObject()->m_vel * Tmr::UncappedFrameLength();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CSkaterStateHistoryComponent::get_latest_position(   )
{
	return GetObject()->m_pos;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CSkaterStateHistoryComponent::get_last_position(   )
{
    return GetObject()->m_old_pos;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	CSkaterStateHistoryComponent::get_time_between_last_update(   )
{
	int time_elapsed;
	
	time_elapsed = static_cast< int >(Tmr::FrameLength());
	
	if( time_elapsed == 0 )
	{
		time_elapsed = Tmr::VBlanks( 1 );
	}
	return time_elapsed;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CSkaterStateHistoryComponent::get_collision_cylinder_coeff( bool driving )
{
    // multiplied by the velocity of the skater to give the length of a line along which to check for collision
	
	float radius;
	
	if (GetSkater()->IsLocalClient())
	{
        radius = GetPhysicsFloat( CRCD(0xd24273b, "LanServerCollCoefficient"), Script::ASSERT);
	}
	else
	{
		radius = GetPhysicsFloat( CRCD(0x25f78481, "LanClientCollCoefficient"), Script::ASSERT);
	}
	
	if (!driving)
	{
		return radius;
	}
	else
	{
        return radius * (1.0f + Script::GetFloat(CRCD(0xa49ff23, "DrivingCoefficientBoostFactor")));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CSkaterStateHistoryComponent::get_collision_cylinder_radius( bool first_driving, bool second_driving )
{
	float radius;
	
	if (GetSkater()->IsLocalClient())
	{
		if (GameNet::Manager::Instance()->InInternetMode())
		{
			radius = GetPhysicsFloat(CRCD(0xf2e35fa3, "InternetServerCollRadius"), Script::ASSERT);
		}
		else
		{
			radius = GetPhysicsFloat(CRCD(0xecb41860, "LanServerCollRadius"), Script::ASSERT);
		}
	}
	else
	{
		if (GameNet::Manager::Instance()->InInternetMode())
		{
			radius = GetPhysicsFloat(CRCD(0xdbd6d58a, "InternetClientCollRadius"), Script::ASSERT);
		}
		else
		{
			radius = GetPhysicsFloat(CRCD(0xc5819249, "LanClientCollRadius"), Script::ASSERT);
		}
	}
	
	float driving_count = 0;
	if (first_driving)
	{
		driving_count++;
	}
	if (second_driving)
	{
		driving_count++;
	}
	
	return radius * (1.0f + driving_count * GetPhysicsFloat(CRCD(0xeef4afb3, "DrivingRadiusBoostFactor")));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

SPosEvent::SPosEvent( void )
{
	memset( this, 0, sizeof( SPosEvent ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	SPosEvent::GetTime( void )
{
	uint32 time;

	time = ( HiTime << 16 ) | ( LoTime );

	return time;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			SPosEvent::SetTime( uint32 time )
{
	LoTime = (unsigned short) ( time & 0xFFFF );
	HiTime = (unsigned short) ( time >> 16 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

SAnimEvent::SAnimEvent( void )
{
	memset( this, 0, sizeof( SAnimEvent ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	SAnimEvent::GetTime( void )
{
	uint32 time;

	time = ( m_HiTime << 16 ) | ( m_LoTime );

	return time;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			SAnimEvent::SetTime( uint32 time )
{
	m_LoTime = (unsigned short) ( time & 0xFFFF );
	m_HiTime = (unsigned short) ( time >> 16 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32			CSkaterStateHistoryComponent::GetLatestAnimTimestamp( void )
{
	return m_last_anm_time;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CSkaterStateHistoryComponent::SetLatestAnimTimestamp( uint32 timestamp )
{
	m_last_anm_time = timestamp;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}
