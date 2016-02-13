//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       ProjectileCollisionComponent.cpp
//* OWNER:          SPG
//* CREATION DATE:  7/10/03
//****************************************************************************

// The CEmptyComponent class is an skeletal version of a component
// It is intended that you use this as the basis for creating new
// components.  
// To create a new component called "Watch", (CWatchComponent):
//  - copy emptycomponent.cpp/.h to watchcomponent.cpp/.h
//  - in both files, search and replace "Empty" with "Watch", preserving the case
//  - in WatchComponent.h, update the CRCD value of CRC_WATCH
//  - in CompositeObjectManager.cpp, in the CCompositeObjectManager constructor, add:
//		  	RegisterComponent(CRC_WATCH,			CWatchComponent::s_create); 
//  - and add the include of the header
//			#include <gel/components/watchcomponent.h> 
//  - Add it to build\gel.mkf, like:
//          $(NGEL)/components/WatchComponent.cpp\
//  - Fill in the OWNER (yourself) and the CREATION DATE (today's date) in the .cpp and the .h files
//	- Insert code as needed and remove generic comments
//  - remove these comments
//  - add comments specfic to the component, explaining its usage

#include <sk/components/ProjectileCollisionComponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/net/client/netclnt.h>
#include <gel/net/server/netserv.h>

#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/objects/skater.h>
#include <sk/components/skaterscorecomponent.h>
#include <sk/gamenet/gamenet.h>

#define vSKATER_RADIUS	FEET( 4 )
namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// s_create is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
// s_create	returns a CBaseComponent*, as it is to be used
// by factor creation schemes that do not care what type of
// component is being created
// **  after you've finished creating this component, be sure to
// **  add it to the list of registered functions in the
// **  CCompositeObjectManager constructor  

CBaseComponent* CProjectileCollisionComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CProjectileCollisionComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// All components set their type, which is a unique 32-bit number
// (the CRC of their name), which is used to identify the component	
CProjectileCollisionComponent::CProjectileCollisionComponent() : CBaseComponent()
{
    SetType( CRC_PROJECTILECOLLISION );
	m_radius = 0.0f;
	m_owner_id = 0;
	m_death_script = 0;
	m_scale = 0;
	m_dying = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CProjectileCollisionComponent::~CProjectileCollisionComponent()
{   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CProjectileCollisionComponent::SetCollisionRadius( float radius )
{
	m_radius = radius;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CProjectileCollisionComponent::InitFromStructure( Script::CStruct* pParams )
{
	pParams->GetFloat(CRCD(0xc48391a5,"radius"), &m_radius, Script::ASSERT);
	pParams->GetFloat(CRCD(0x13b9da7b,"scale"), &m_scale, Script::ASSERT);
	pParams->GetChecksum(CRCD(0x81c39e06,"owner_id"), &m_owner_id, Script::ASSERT);
	pParams->GetChecksum(CRCD(0x6647adc3,"death_script"), &m_death_script, Script::ASSERT);

	m_vel.Set( 0, 0, 1 );
	pParams->GetVector(CRCD(0xc4c809e, "vel"), &m_vel, Script::ASSERT);
	m_vel.Normalize();
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// RefreshFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CProjectileCollisionComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Default to just calline InitFromStructure()
	// but if that does not handle it, then will need to write a specific 
	// function here. 
	// The user might only want to update a single field in the structure
	// and we don't want to be asserting becasue everything is missing 
	
	//InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CProjectileCollisionComponent::Finalize()
{
	mp_suspend_component =  GetSuspendComponentFromObject( GetObject() );
}
	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CProjectileCollisionComponent::Hide( bool should_hide )
{
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// The component's Update() function is called from the CCompositeObject's 
// Update() function.  That is called every game frame by the CCompositeObjectManager
// from the s_logic_code function that the CCompositeObjectManager registers
// with the task manger.
void CProjectileCollisionComponent::Update()
{
	float radii;

	radii = m_radius + vSKATER_RADIUS;
	if (!mp_suspend_component->SkipLogic() && !m_dying )
	{
		float dist;
		bool team_game;
		Mdl::Skate* skate_mod = Mdl::Skate::Instance();
		GameNet::Manager* gamenet_man = GameNet::Manager::Instance();
		uint32 NumSkaters = skate_mod->GetNumSkaters();
		GameNet::PlayerInfo* target_player, *src_player;

		team_game = skate_mod->GetGameMode()->IsTeamGame();
		for( uint32 i = 0; i < NumSkaters; i++ )
		{
			Obj::CSkater *pSkater = skate_mod->GetSkater(i);
			Obj::CSkaterScoreComponent* p_score_comp;
			Mdl::Score* score;

			// Can't hit yourself with your own projectile
			if( pSkater->GetID() == m_owner_id )
			{
				continue;
			}

			src_player = gamenet_man->GetPlayerByObjectID( m_owner_id );
			target_player = gamenet_man->GetPlayerByObjectID( pSkater->GetID());
			
			// Allow players to get up after being knocked down
			if( target_player->IsVulnerable() == false )
			{
				continue;
			}

			p_score_comp = GetSkaterScoreComponentFromObject( pSkater );
			score = p_score_comp->GetScore();

			// Don't target dead players
			if( score->GetTotalScore() <= 0 )
			{
				continue;
			}

			if( team_game )
			{
				if( src_player->m_Team == target_player->m_Team )
				{
					uint32 enabled;

					enabled = gamenet_man->GetNetworkPreferences()->GetPreferenceChecksum( CRCD(0xe959c43a,"friendly_fire"), CRCD(0x21902065,"checksum"));
					if( enabled != CRCD(0xf81bc89b,"boolean_true"))
					{
						continue;
					}
				}
			}

			dist = (pSkater->GetPos() - GetObject()->GetPos()).Length();
			if( dist < radii )
			{
				GameNet::MsgProjectileHit hit_msg;
				Net::MsgDesc msg_desc;
				Net::Server* server;
				GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

				server = gamenet_man->GetServer();

				//Dbg_Printf( "COLLISION: Skater %d: owner: %d\n", pSkater->GetID(), m_owner_id);
				GetObject()->MarkAsDead();
				if( m_death_script != 0 )
				{
					Script::CStruct* params;
					Mth::Vector pos;

					pos = GetObject()->GetPos();
					params = new Script::CStruct;
					
					params->AddVector( CRCD(0x7f261953,"pos"), pos );
					params->AddVector( CRCD(0xc4c809e,"vel"), m_vel );
					params->AddFloat( CRCD(0x13b9da7b,"scale"), m_scale );

					Script::RunScript( m_death_script, params );
					delete params;
				}
	
				// a message explaining "You've been hit down by ID for N damage"
				hit_msg.m_Id = m_owner_id;
				hit_msg.m_Damage = get_damage_amount();
	
				/*if (CInputComponent* p_input_component = GetInputComponentFromObject(GetObject()))
				{
					p_input_component->DisableInput();
				}
				p_player->MarkAsNonCollidable();*/
				
				msg_desc.m_Id = GameNet::MSG_ID_SKATER_HIT_BY_PROJECTILE;
				msg_desc.m_Data = &hit_msg;
				msg_desc.m_Length = sizeof(GameNet::MsgProjectileHit);
				msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
				msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
				target_player->SetHitTime( Tmr::GetTime());
				server->EnqueueMessage( target_player->GetConnHandle(), &msg_desc );
	
				// basically a message explaining "You hit someone"
				hit_msg.m_Id = pSkater->GetID();
				msg_desc.m_Id = GameNet::MSG_ID_SKATER_PROJECTILE_HIT_TARGET;
				server->EnqueueMessage( src_player->GetConnHandle(), &msg_desc );	
				
				
				if(( score->GetTotalScore() - get_damage_amount() ) <= 0 )
				{
					// If this shot is killing them...
					if( score->GetTotalScore() > 0 )
					{
						if( target_player->IsLocalPlayer() == false )
						{
							Script::CStruct* params;
	
							Dbg_Printf( "*** Elimination\n" );
	
							params = new Script::CStruct;
							params->AddString( CRCD(0xa1dc81f9,"name"), pSkater->GetDisplayName());
							Script::RunScript( CRCD(0x9b043179,"announce_elimination"), params );
							delete params;
							skate_mod->HideSkater( pSkater, true );
						}
					}
					score->SetTotalScore( 0 );
				}
				else
				{
					score->SetTotalScore( score->GetTotalScore() - get_damage_amount());
				}
				
				break;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the "Checksum" of a script command, then possibly handle it
// if it's a command that this component will handle	
CBaseComponent::EMemberFunctionResult CProjectileCollisionComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	// the "default" case of the switch statement handles
	// unrecognized functions;  if we make it down here,
	// that means that the component already handled it
	// somehow
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CProjectileCollisionComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to CProjectileCollisionComponent::GetDebugInfo"));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums
	

// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int	CProjectileCollisionComponent::get_damage_amount( void )
{
	return (int) m_scale * 10;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CProjectileCollisionComponent::MarkAsDying( void )
{
	m_dying = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}
