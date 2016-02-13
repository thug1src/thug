//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterFlipAndRotateComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/31/3
//****************************************************************************

/*
 * Encapsulates the dirty, dirty skater flip, skater rotate, and board rotate animation logic.
 *
 * "Please!" says CSkaterFlipAndRotateComponent, "Deprecate me, and end my torturous existence!"
 */

#include <sk/components/skaterflipandrotatecomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skaterstatecomponent.h>
#include <sk/gamenet/gamenet.h>

#include <gel/object/compositeobject.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/net/client/netclnt.h>

namespace Obj
{
	extern bool DebugSkaterScripts;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterFlipAndRotateComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterFlipAndRotateComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterFlipAndRotateComponent::CSkaterFlipAndRotateComponent() : CBaseComponent()
{
	SetType( CRC_SKATERFLIPANDROTATE );
	
	mp_animation_component = NULL;
	mp_model_component = NULL;
	
	m_rotate_board = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterFlipAndRotateComponent::~CSkaterFlipAndRotateComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFlipAndRotateComponent::InitFromStructure( Script::CStruct* pParams )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFlipAndRotateComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFlipAndRotateComponent::Finalize (   )
{
	// Note: Non-local clients have a CSkaterFlipAndRotateComponent, but not a CSkaterCorePhysicsComponent.
	mp_core_physics_component = GetSkaterCorePhysicsComponentFromObject(GetObject());
	
	mp_model_component = GetModelComponentFromObject(GetObject());
   	mp_animation_component = GetAnimationComponentFromObject(GetObject());
	
	Dbg_Assert(mp_model_component);
	Dbg_Assert(mp_animation_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFlipAndRotateComponent::Update()
{
	Suspend(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterFlipAndRotateComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | Flip | flip animation
		case CRCC(0x65011baa, "Flip"):	
			ToggleFlipState();
			break;
			
        // @script | ResetSwitched | flip if we're switched
		case CRCC(0x72289a28, "ResetSwitched"):
			Dbg_Assert(mp_core_physics_component);
			if (mp_core_physics_component->IsSwitched())
			{
				ToggleFlipState();
			}
			break;
			
		// These three just set flags, which will cause the appropriate flip/rotate
		// when the next PlayAnim occurs. 
        // @script | FlipAfter | sets flag, which will cause the appropriate flip/rotate when the next PlayAnim occurs
		case CRCC(0xeba11b20, "FlipAfter"):
			mFlipAfter = true;
			break;
			
        // @script | RotateAfter | sets flag, which will cause the appropriate flip/rotate when the next PlayAnim occurs
		case CRCC(0xacac1bad, "RotateAfter"):
			mRotateAfter = true;
			break;
			
        // @script | BoardRotateAfter | sets flag, which will cause the appropriate flip/rotate when the next PlayAnim occurs
		case CRCC(0x63e93f69, "BoardRotateAfter"):
			mBoardRotateAfter = true;
			break;
			
		// @script | IsFlipAfterSet | returns mFlipAfter
		case CRCC(0xa6243daa, "IsFlipAfterSet"):
			return mFlipAfter ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | FlipAndRotate | flip Z and X, to rotate 180 degrees about y
		case CRCC(0xd9de0c65, "FlipAndRotate"):
		{
			Dbg_Assert(mp_core_physics_component);
			mp_core_physics_component->ReverseFacing();
			ToggleFlipState();
			break;
		}
			
		// @script | BoardRotate | 
        // @flag normal | put the board back to normal (otherwise just flip)
		case CRCC(0xe0f3a644, "BoardRotate"):
		{
			GameNet::PlayerInfo* player = GameNet::Manager::Instance()->GetPlayerByObjectID(GetObject()->GetID());
			Dbg_Assert(player);
			
			Net::Client* client = GameNet::Manager::Instance()->GetClient(player->GetSkaterNumber());
			Dbg_Assert(client);

			if (pParams->ContainsFlag(CRCD(0xde7a971b, "Normal")))
			{
				// Put the board back to normal
				RotateSkateboard(GetObject()->GetID(), false, client->m_Timestamp, true);
			}
			else
			{
				// Otherwise flip it, flip it good.
				RotateSkateboard(GetObject()->GetID(), !m_rotate_board, client->m_Timestamp, true);
			}	
			break;
		}

		// @script | BoardIsRotated | 
		case CRCC(0x79ee5ccf, "BoardIsRotated"):
			return m_rotate_board ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
		case CRCC(0xb1e7291, "PlayAnim"):
			// do any required flips and rotates, and then cascade the member function call
			DoAnyFlipRotateOrBoardRotateAfters();
			return CBaseComponent::MF_NOT_EXECUTED;
		
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFlipAndRotateComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterFlipAndRotateComponent::GetDebugInfo"));
	
	if (mp_core_physics_component)
	{
		p_info->AddChecksum(CRCD(0x8f66b80b, "switched"), mp_core_physics_component->IsSwitched() ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	}
	p_info->AddChecksum(CRCD(0xc7a712c, "flipped"), GetSkaterStateComponentFromObject(GetObject())->GetFlag(FLIPPED) ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	
	p_info->AddChecksum(CRCD(0xeba11b20, "FlipAfter"), mFlipAfter ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	p_info->AddChecksum(CRCD(0xacac1bad, "RotateAfter"), mRotateAfter ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	p_info->AddChecksum(CRCD(0x63e93f69, "BoardRotateAfter"), mBoardRotateAfter ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterFlipAndRotateComponent::RotateSkateboard ( uint32 objId, bool rotate, uint32 time, bool propagate )
{
	if ( propagate )
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		GameNet::PlayerInfo* player;

		player = gamenet_man->GetPlayerByObjectID( objId );
		if( player && player->IsLocalPlayer())
		{
			Net::Client* client;
			GameNet::MsgRotateSkateboard msg;
			Net::MsgDesc msg_desc;

			client = gamenet_man->GetClient( player->GetSkaterNumber());
			Dbg_Assert( client );

			msg.m_Rotate = rotate;
			msg.m_ObjId = objId;

			msg_desc.m_Data = &msg;
			msg_desc.m_Id = GameNet::MSG_ID_ROTATE_SKATEBOARD;
			msg_desc.m_Length = sizeof( GameNet::MsgRotateSkateboard );
			client->EnqueueMessageToServer(	&msg_desc );
		}
	}

	bool oldRotation = m_rotate_board;

	m_rotate_board = rotate;
	
	#ifdef __NOPT_ASSERT__
	if (DebugSkaterScripts && GetObject()->GetType() == SKATE_TYPE_SKATER)
	{
		printf("%d: Rotating board [rotated = %s]\n", (int) Tmr::GetRenderFrame(), m_rotate_board ? "true" : "false");
	}
	#endif

	return oldRotation;
}

/******************************************************************/

/*

If any of mFlipAfter, mRotateAfter or mBoardRotateAfter are set
this will flip or rotate the skater, or rotate the board, and then
reset those flags.

Quite often the skater needs to be flipped and/or rotated in order to
look correctly oriented, due to the way the animations are done.
For example, the nollie heelflip animation is actually animated backwards,
so to look correct the skater has to be flipped and rotated just for the
duration of that animation.
As soon as any sort of 'event' happens that could result in an animation
change, or requires that the orientation be correct for the physics, then
the skater needs to be flipped and rotated back.
So in the script, when running the nollie heelflip anim, Scott will flip and
rotate the skater first so that he looks correct, then set FlipAfter and RotateAfter.

It used to be that I'd check the flags only in the next call to the PlayAnim script 
member function.
However, this is often not early enough. For example, when the player lands whilst
doing a nollie heelflip, the land script will do some checks to see if the player 
is facing backwards before doing a playanim, so it will wrongly think the player
is backwards, when he isn't 'really'.

So instead I sprinkled calls to the following function whenever anything happens
that could cause a new script to be executed. In particular, before every SetScript,
and also as soon as a grind gets detected, so that the grind maths won't think the
skater is backwards when landing from a nollie heelflip.

*/

/******************************************************************/

void CSkaterFlipAndRotateComponent::DoAnyFlipRotateOrBoardRotateAfters (   )
{
	if (mFlipAfter)
	{
		ToggleFlipState();
		mFlipAfter = false;
	}
		
	if (mRotateAfter)
	{
		Dbg_Assert(mp_core_physics_component);
		mp_core_physics_component->ReverseFacing();
		mRotateAfter = false;
	}
		
	if (mBoardRotateAfter)
	{
		GameNet::PlayerInfo* player = GameNet::Manager::Instance()->GetPlayerByObjectID(GetObject()->GetID());
		Dbg_Assert(player);

		Net::Client* client = GameNet::Manager::Instance()->GetClient(player->GetSkaterNumber());
		Dbg_Assert(client);

		// Rotate the board.
		RotateSkateboard(GetObject()->GetID(), !m_rotate_board, client->m_Timestamp, true);
		mBoardRotateAfter = false;
	}	
	
	// This makes the skater display facing the correct way again.
	// The mFlipDisplayMatrix is only used when falling back in from a lip trick.
	// Since it would be bad if this flag got left on, make sure it is off whenever
	// a new anim runs. This function is a good place to put it, since it gets
	// run whenever the anim changes.
	mp_model_component->mFlipDisplayMatrix = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFlipAndRotateComponent::Reset (   )
{
	mFlipAfter = false;
	mRotateAfter = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFlipAndRotateComponent::ToggleFlipState (   )
{
	GetSkaterStateComponentFromObject(GetObject())->m_skater_flags[FLIPPED].Toggle();	
	
	#ifdef __NOPT_ASSERT__
	if (DebugSkaterScripts && GetObject()->GetType() == SKATE_TYPE_SKATER)
	{
		printf("%d: Flipping skater [flipped = %s]\n", (int) Tmr::GetRenderFrame(), mp_core_physics_component->GetFlag(FLIPPED) ? "true" : "false");
	}
	#endif
	
	// Flip the animation to the correct orientation Just setting a flag for Dave's code to handle when rendering
	ApplyFlipState();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFlipAndRotateComponent::ApplyFlipState (   )
{
	Net::Client* client = GameNet::Manager::Instance()->GetClient(GetSkater()->GetSkaterNumber());
	Dbg_Assert(client);
	
	mp_animation_component->FlipAnimation(GetObject()->GetID(), GetSkaterStateComponentFromObject(GetObject())->GetFlag(FLIPPED), client->m_Timestamp, true);
}

}
