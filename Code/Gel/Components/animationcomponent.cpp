//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       AnimationComponent.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/24/2002
//****************************************************************************

#include <gel/components/animationcomponent.h>

#include <gel/components/modelcomponent.h>
#include <gel/components/skeletoncomponent.h>
#include <gel/components/suspendcomponent.h>

// TODO:  All the network code should
// be at the skater- or the player- level
#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>

#include <gel/object/compositeobject.h>

#include <gel/scripting/array.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/component.h>

#include <gfx/baseanimcontroller.h>
#include <gfx/bonedanim.h>
#include <gfx/gfxutils.h>
#include <gfx/nx.h>
#include <gfx/nxanimcache.h>
#include <gfx/nxmodel.h>
#include <gfx/pose.h>
#include <gfx/skeleton.h>

#include <sk/gamenet/gamenet.h>
		  
namespace Obj
{
	extern bool DebugSkaterScripts;
	
	// if the animation time increment is too small,
	// then ignore it...
	const float vMIN_SPEED_THRESHOLD = 0.05f;

	#define nxANIMCOMPONENTFLAGS_FLIPPED				(1<<29)
	
	// maximum number of degenerate animations
	const int vNUM_DEGENERATE_ANIMS = 3;

	// maximum number of procedural bones
	const int vMAXPROCEDURALBONES = 12;
	
	static bool s_updating_channels = false;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This static function is what is registered with the component
// factory object, (currently the CCompositeObjectManager) 
CBaseComponent* CAnimationComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CAnimationComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CAnimationComponent::CAnimationComponent() : CBaseComponent()
{
	SetType( CRC_ANIMATION );

	m_animScriptName = 0;
	m_animEventTableName = 0;

	m_shouldBlend = false;

	Reset();

	m_numProceduralBones = 0;

	mp_proceduralBones = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CAnimationComponent::~CAnimationComponent()
{
	destroy_blend_channels();
	
	if ( mp_proceduralBones )
	{
		delete[] mp_proceduralBones;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::Reset()
{
	// stops all the animations...
	destroy_blend_channels();

	// resets "global" blend parameters
	mGotBlendPeriodOut = false;
	mBlendPeriodOut = 0.0f;
	
	// reset script animation waits
	if (m_animation_script_block_active)
	{
		m_animation_script_block_active = false;
		if (GetObject()->GetScript())
		{
			GetObject()->GetScript()->UnBlock();
		}
	}
	m_animation_script_unblock_point = 0;
	
	// reset animation frame count
	m_last_animation_time = 0.0f;
	
	m_dont_interrupt=false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::SetAnims( uint32 anim_checksum )
{
    m_animScriptName = anim_checksum;

	PlayPrimarySequence( 0, false, 0.0f, 1000.0f, Gfx::LOOPING_HOLD, 0.3f, 1.0f );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CAnimationComponent::InitFromStructure( Script::CStruct* pParams )
{
	Dbg_Assert( pParams );

	pParams->GetChecksum( CRCD(0x5b8c6dc2,"animEventTableName"), &m_animEventTableName, false );

	uint32 animScriptName;

	if ( pParams->GetChecksum( CRCD(0x6c2bfb7f,"animName"), &animScriptName, false ) )
	{
		SetAnims( animScriptName );
		Reset();

		// safety-check to make sure he doesn't start out in the blair witch position
		// (because of our animation LOD-ing system)
		// this must be done before the models get initialized?
		uint32 defaultAnimName;
		if ( pParams->GetChecksum( CRCD(0xeae64b47,"defaultAnimName"), &defaultAnimName, false ) )
		{
			PlaySequence(defaultAnimName);
			SetLoopingType( Gfx::LOOPING_CYCLE, true );
		}
		else
		{
			// generally, the blair witch position
			PlaySequence( CRCD(0x1ca1ff20,"default") );
		}
	}

	// if it's the local skater, then give it some procedural animation as well
	// (the decision to do this should really be coming from a higher-level...)
 	if ( GetObject()->GetID() == 0 )
	{
		Dbg_MsgAssert( mp_proceduralBones == NULL, ( "Already has procedural bones!" ) );
		mp_proceduralBones = new Gfx::CProceduralBone[vMAXPROCEDURALBONES];

		Script::CStruct* pTempParams = new Script::CStruct;
		pTempParams->AppendStructure( pParams );

		pTempParams->AddChecksum( CRCD(0x7321a8d6,"type"), CRCD(0xfdf0436c,"ProceduralAnim") );
		pTempParams->AddChecksum( CRCD(0x3ed7262b,"bone_list"), CRCD(0x5d5c0e72,"procedural_skater_bones") );
		
		initialize_procedural_bones( pTempParams );

		delete pTempParams;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::ToggleFlipState( void )								   
{
	// Flip the animation to the correct orientation
	GameNet::Manager* gamenet_man = GameNet::Manager::Instance();
	Net::Client* client = gamenet_man->GetClient( 0 );
	this->FlipAnimation( GetObject()->GetID(), !IsFlipped(), client->m_Timestamp, true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CAnimationComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
    switch ( Checksum )
    {        
        // @script | Obj_EnableAnimBlending | 
        // @parm name | enabled | whether the anim blending should be enabled
		// peds should disable blending, as a speed optimization
		case 0x98a32669:	// Obj_EnableAnimBlending
		{
			int shouldBlend;
			pParams->GetInteger( CRCD(0xaf06447b,"enabled"), &shouldBlend, Script::ASSERT );
			EnableBlending( shouldBlend );
		}
		break;
        
		// @script | Obj_SetAnimCycleMode | 
        // @flag off | turn cycle off (otherwise it will turn it on)
		case 0x58c52f64:	// Obj_SetAnimCycleMode
        {
           if ( pParams->ContainsFlag( CRCD(0xd443a2bc,"off") ) )
           {
               SetLoopingType( Gfx::LOOPING_HOLD, true );
           }
           else
           {
               // maybe we should clear the cycle time or num loops?
               // or should we leave it at whatever was set before?  hmmm...
               SetLoopingType( Gfx::LOOPING_CYCLE, true );
           }
        }
        break;

        // @script | Obj_PlayAnim | 
        // @parm name | Anim | animation to play
        // @flag wobble | enable wobble
        // @parmopt float | BlendPeriod | 0.3f | blend period (default may change)
        // @parmopt int | BlendPeriodPercent | | blend period as a percentage of the anim length (20 means "20 percent")
        // @parmopt float | speed | 1.0 | 
        // @flag NoRestart | 
        // @flag Cycle | 
        // @flag PingPong | 
        // @flag Wobble | 
        // @flag DontInterrupt | Makes further PlayAnims have no effect until the current
		// animation has finished.
        // @parmopt name | From | | start, end, or current
        // @parmopt int | From | | from can be specified as an int (60ths)
        // @parmopt name | To | | start, end, or current
        // @parmopt int | To | | to can be specified as an int (60ths)
        // @flag Backwards | play animation backwards
		case 0x2cb9ad09:	// Obj_PlayAnim
		case 0x0b1e7291:	// PlayAnim
        {
			PlayAnim( pParams, pScript );
        
			Gfx::CBlendChannel* pPrimaryChannel = get_primary_channel();
			if ( pPrimaryChannel && pPrimaryChannel->GetLoopingType() == Gfx::LOOPING_WOBBLE )
			{
				// the skater balance trick needs to do some extra stuff to
				// handle wobbles, but needs to do it after the anim has been
				// launched.  the following gives any component a chance to
				// handle the wobble first...  if it is not handled by the
				// time it gets to the animation component, then it's ignored
				// (this works for the case of the skater balance trick, because
				// it comes early in the component list)
				GetObject()->CallMemberFunction( CRCD(0xea6d0efd,"SetWobbleDetails"), pParams, pScript );
			}
		}
		break;

		case 0xea6d0efd:	// SetWobbleDetails
			// this is a special case to make sure that the skater balance
			// trick component gets a chance to handle wobbles, if such
			// a component exists.
			return MF_TRUE;
        
        // @script | Obj_AnimComplete | returns true when the object
        // is done playing an anim.  This is only valid with "hold on
		// last frame" animations.	
		case 0x2889c6c9:	// Obj_AnimComplete
		case 0x76cc99d5: 	// AnimFinished	
			return ( IsAnimComplete() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE );
			break;

        // @script | Obj_AnimEquals | True if specified animation is
        // the one currently playing on the object, False otherwise
        // @uparmopt name | single animation name to check
        // @uparmopt [] | array of animation names to check
		case 0xb5fb7eb6:  	// Obj_AnimEquals
		case 0x1a0f9646:	// AnimEquals
			return ( AnimEquals( pParams, pScript ) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE );
			break;

        // @script | Obj_GetAnimSpeed | 
        // the one currently playing on the object
        case 0x5195bcbb: // Obj_GetAnimSpeed
        {
            pScript->GetParams()->AddFloat(CRCD(0xf0d90109,"speed"), GetAnimSpeed( pParams, pScript ));
        }
        break;
            
        // @script | BlendPeriodOut | next call to playanim will use this blend period
        // no matter what is specified in the actual call to playanim
        // @uparm 1.0 | blend period out value
		case 0x68c86aec:	// BlendPeriodOut	
		{
			if (!pParams->GetFloat(NONAME,&mBlendPeriodOut))
            {
				Dbg_MsgAssert(0,("\n%s\nBlendPeriodOut requires a floating point value",pScript->GetScriptInfo()));
			}
			// Set the flag so that PlayAnim will use mBlendPeriodOut instead next time.
			mGotBlendPeriodOut=true;	
		}
		break;
        
        // @script | LoopingAnim | true if the current animation will loop forever
		case 0x80fdbdf2:	// LoopingAnim	
			{
				return IsLoopingAnim() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			}
			break;
			
		case 0x42ffc3ce: // GetAnimLength
		{
			#ifdef __USER_DAN__
			printf("GetAnimLength\n");
			Script::PrintContents(pParams);
			#endif
			
			uint32 anim_checksum=0;
			pParams->GetChecksum(CRCD(0x98549ba4, "anim"), &anim_checksum);
			
			if (AnimExists(anim_checksum))
			{
				pScript->GetParams()->AddFloat(CRCD(0xfe82614d, "Length"), fabs(AnimDuration(anim_checksum)));
			}
			break;
		}
		
        // @script | FrameIs | checks if the current frame is equal
        // to the specified frame
        // @uparmopt 1 | frame number
		case 0x922e7d14: // FrameIs
		{
			#ifdef __USER_DAN__
			printf("FrameIs\n");
			Script::PrintContents(pParams);
			#endif
			
			float Start,Current,End;
			GetPrimaryAnimTimes(&Start,&Current,&End);
			
			float t=0;
			pParams->GetFloat(NO_NAME,&t);

			t/=60.0f;
			
			float new_frame=Current-Start;
			float old_frame=m_last_animation_time-Start;

			if (t==new_frame)
			{
				return CBaseComponent::MF_TRUE;
			}
			
			if (old_frame<t && t<new_frame)
			{
				return CBaseComponent::MF_TRUE;
			}
			
			if (new_frame<old_frame && old_frame<t)
			{
				return CBaseComponent::MF_TRUE;
			}

			if (t<new_frame && new_frame<old_frame)
			{
				return CBaseComponent::MF_TRUE;
			}
				
			return CBaseComponent::MF_FALSE;	
		}
			
        // @script | WaitAnim | 
        // @uparm 0.0 | wait time (default is milliseconds)
        // @flag frame | time in frames
        // @flag percent | time in percent
        // @flag second | time in seconds
        // @flag relative | 
        // @flag fromend | 
		case 0x74cf5c94: // WaitAnim
		{
			#ifdef __USER_DAN__
			// printf("WaitAnim\n");
			// Script::PrintContents(pParams);
			#endif
			
			float Start,Current,End;
			GetPrimaryAnimTimes(&Start,&Current,&End);
			float t=0;
			pParams->GetFloat(NO_NAME,&t);

			if (pParams->ContainsFlag(0x4a07c332/*"frame"*/) || pParams->ContainsFlag(0x19176c5/*"frames"*/))
			{
				t/=60.0f;
			}
			else if (pParams->ContainsFlag(0x9e497fc6/*"percent"*/))
			{
				t=Start+(End-Start)*t/100.0f; // Dodgy?
			}	
			else if (pParams->ContainsFlag(0x49e0ee96/*"second"*/) || pParams->ContainsFlag(0xd029f619/*"seconds"*/))
			{
				// t is in seconds, so nothing to do.
			}
			else
			{
				// If they did not specify any of the above, then t is in milliseconds, so convert to seconds.
				t/=1000.0f;
			}
			

			if (pParams->ContainsFlag(0x91a4c826/*"relative"*/))	
			{
				if (Start<=End)
				{
					m_animation_script_unblock_point=Current+t;
				}
				else
				{
					m_animation_script_unblock_point=Current-t;
				}	
			}
			else if (pParams->ContainsFlag(0xe18cd075/*"fromend"*/))	
			{
				if (Start<=End)
				{
					m_animation_script_unblock_point=End-t;
				}
				else
				{
					m_animation_script_unblock_point=Start+t;
				}	
			}
			else
			{
				m_animation_script_unblock_point=t;
			}
			
//			Dbg_MsgAssert( m_animation_script_unblock_point>=Start && m_animation_script_unblock_point<=End, ( "WaitAnim time %f out of range of anim (%f %f) in %s", m_animation_script_unblock_point, Start, End, pScript->GetScriptInfo() ) );

			if ( !GetObject()->GetScript() )
			{
				GetObject()->SetScript(new Script::CScript);
			}

			if ((Start<=End && Current>=m_animation_script_unblock_point) || (Start>=End && Current<=m_animation_script_unblock_point))
			{
				GetObject()->GetScript()->UnBlock();
				m_animation_script_block_active=false;
			}	
			else
			{
				GetObject()->GetScript()->Block();
				m_animation_script_block_active=true;
			}	
			
			return CBaseComponent::MF_TRUE;
		}	

		case 0x5f495ae0:	// InvalidateCache
		{
			int num_channels = m_blendChannelList.CountItems();
			Gfx::CBlendChannel* pChannel = (Gfx::CBlendChannel*)m_blendChannelList.GetNext();
			for ( int i = 0; i < num_channels; i++ )
			{
				pChannel->InvalidateCache();
				pChannel = (Gfx::CBlendChannel*)pChannel->GetNext();
			}
		}
		break;

        // @script | Obj_WaitAnimFinished | wait for animation to complete
		case 0xb628a959:  // Obj_WaitAnimFinished
			pScript->SetWait(Script::WAIT_TYPE_OBJECT_ANIM_FINISHED,this);
			break;
			

		// @script | AnimExists | returns true if the given anim exists
		// @param name | animation to look for
		case CRCC(0x9069f357, "AnimExists"):
		{
			uint32 anim_name;
			pParams->GetChecksum(NO_NAME, &anim_name);
			return AnimExists(anim_name) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE; 
		}

		case 0x83654874: // AddAnimController
		case 0x02565bd1: //	SetBoneTransMin
		case 0x3e5b6488: //	SetBoneTransMax
		case 0xc85b9ac0: //	SetBoneTransSpeed
		case 0x5ee9d5bd: //	SetBoneTransCurrent
		case 0x5661fb72: //	SetBoneTransActive
		case 0xa47767f2: //	SetBoneRotMin
		case 0x987a58ab: //	SetBoneRotMax
		case 0x599d3707: //	SetBoneRotSpeed
		case 0x7235daa7: //	SetBoneRotCurrent
		case 0x53f06acc: //	SetBoneRotActive
		case 0xc6889e91: //	SetBoneScaleMin
		case 0xfa85a1c8: //	SetBoneScaleMax
		case 0xd32c2724: //	SetBoneScaleSpeed
		case 0xd12b3713: //	SetBoneScaleCurrent
		case 0xf11daaae: //	SetBoneScaleActive
		case 0xd63a1b81: // GetPartialAnimParams
		case 0xbd4edd44: // SetPartialAnimSpeed
		case 0x6aaeb76f: // IncrementPartialAnimTime
		case 0xf5e2b871: // ReversePartialAnimDirection
			{
				Gfx::CBlendChannel* pPrimaryChannel = get_primary_channel();
				if ( pPrimaryChannel )
				{
					return pPrimaryChannel->CallMemberFunction( Checksum, pParams, pScript ) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
				}
			}
			break;
		
		case 0x986d274e: // RemoveAnimController
			{
				Gfx::CBlendChannel* pChannel = (Gfx::CBlendChannel*)m_blendChannelList.GetNext();
				while ( pChannel )
				{
					pChannel->CallMemberFunction( Checksum, pParams, pScript );
					pChannel = (Gfx::CBlendChannel*)pChannel->GetNext();
				}
				return CBaseComponent::MF_TRUE;
			}
			break;

		// @script | Obj_Flip | 
		case 0x27a98022:  // Obj_Flip
			ToggleFlipState( );
			break;
		
		// @script | Obj_AnimationFlipped | 
		case 0x6eceb234:  // Obj_AnimationFlipped
			return ( m_flags & nxANIMCOMPONENTFLAGS_FLIPPED ) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;
		
		default:
            return CBaseComponent::MF_NOT_EXECUTED;

    }
    
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::AddAnimController( Script::CStruct* pParams )
{
	Gfx::CBlendChannel* pPrimaryChannel = get_primary_channel();
	if ( pPrimaryChannel )
	{
		pPrimaryChannel->AddController( pParams );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::ProcessWait( Script::CScript * pScript )
{
	switch (pScript->GetWaitType())
	{
		case Script::WAIT_TYPE_OBJECT_ANIM_FINISHED:
			if (IsAnimComplete())
			{
				pScript->ClearWait();
			}
			break;
		default:
			Dbg_MsgAssert(0,("\n%s\nWait type of %d not supported by CAnimationComponent",pScript->GetScriptInfo(),pScript->GetWaitType()));
			break;
	}		
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::GetDebugInfo( Script::CStruct* p_info )
{

#ifdef	__DEBUG_CODE__

	Dbg_MsgAssert( p_info, ( "NULL p_info sent to CAnimationComponent::GetDebugInfo" ) );

	// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);
	
	p_info->AddInteger( CRCD(0x87ed8a3f,"m_shouldblend"), m_shouldBlend );
	p_info->AddInteger( CRCD(0xa2f4a5ea,"mGotBlendPeriodOut"), mGotBlendPeriodOut );
	p_info->AddFloat( CRCD(0xd0902723,"mBlendPeriodOut"), mBlendPeriodOut );
	p_info->AddChecksum( CRCD(0xc2fe9f39,"m_animScriptName"), m_animScriptName );
	p_info->AddInteger(CRCD(0x9fdd4257,"m_dont_interrupt"),m_dont_interrupt);
	
	Script::CStruct* p_tempParams = new Script::CStruct;
	p_tempParams->Clear();
	Gfx::CBlendChannel* pPrimaryChannel = get_primary_channel();
	if ( pPrimaryChannel )
	{
		pPrimaryChannel->GetDebugInfo( p_tempParams );
		p_info->AddStructure( CRCD(0x969f358c,"primary_channel"), p_tempParams );
	}

	delete p_tempParams;
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Gfx::CBlendChannel* CAnimationComponent::get_primary_channel()
{
	// just grab the first item from the list...
	Gfx::CBlendChannel* pBlendChannel = NULL;
	pBlendChannel = (Gfx::CBlendChannel*)m_blendChannelList.GetNext();
	return pBlendChannel;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::pack_degenerate_channels()
{
	// this removes all the channels that have expired...

	Gfx::CBlendChannel* pChannel = (Gfx::CBlendChannel*)m_blendChannelList.GetNext();
	while ( pChannel )
	{
		Gfx::CBlendChannel* pNext = (Gfx::CBlendChannel*)pChannel->GetNext();
		
		if ( !pChannel->IsActive() )
		{
			Dbg_MsgAssert(pChannel != get_primary_channel(), ("Removing primary channel"));
			Dbg_MsgAssert( !s_updating_channels, ( "Someone is trying to remove an animation channel while in channel update loop" ) );
			pChannel->Remove();
			delete pChannel;
		}
		
		pChannel = pNext;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::create_new_blend_channel( float blend_period )
{
	// If there	are too many channels, get rid of the tail channels
	Gfx::CBlendChannel* pChannel = (Gfx::CBlendChannel*)m_blendChannelList.GetNext();//Item( 0 );
	int channelCount = 0;

	while ( pChannel )
	{
		channelCount++;

		Gfx::CBlendChannel* pNext = (Gfx::CBlendChannel*)pChannel->GetNext();
		
		if ( channelCount >= vNUM_DEGENERATE_ANIMS )
		{
			// remove the tail items
			pChannel->Remove();
			Dbg_MsgAssert( !s_updating_channels, ( "Someone is trying to remove an animation channel while in channel update loop" ) );
			delete pChannel;
		}

		pChannel = pNext;
	}
		
	// degenerate the existing blend channels
	// make the first channel degenerate
	Gfx::CBlendChannel* pBlendChannel = m_blendChannelList.CountItems() ? get_primary_channel() : NULL;//	(Gfx::CBlendChannel*)m_blendChannelList.GetItem( 0 );
	if ( pBlendChannel )
	{
		if ( pBlendChannel->Degenerate( blend_period ) )
		{
			// degeneration worked, so we want to do this cumulative
			// blend value thing (not really sure what it is though)
			float cumulative_blend_value = 0.0f;
			Gfx::CBlendChannel* pOtherChannels = (Gfx::CBlendChannel*)pBlendChannel->GetNext();
			while ( pOtherChannels )
			{
				// if( !pOtherChannels->IsActive() )
				if( pOtherChannels->IsDegenerating() )
				{
					cumulative_blend_value += 
						pOtherChannels->GetDegenerationTime() 
						* pOtherChannels->GetDegenerationTimeToBlendMultiplier();
				}
				pOtherChannels = (Gfx::CBlendChannel*)pOtherChannels->GetNext();
			}
		
			float blend_value1 = 1.0f - ( cumulative_blend_value );
			pBlendChannel->SetDegenerationTimeToBlendMultiplier( blend_value1 / pBlendChannel->GetDegenerationTime() );
		}
	}
	
	// now add a new channel to the front of the list
	Gfx::CBlendChannel* pPrimaryChannel = new Gfx::CBlendChannel( GetObject() );
	m_blendChannelList.AddToHead( pPrimaryChannel );
	Dbg_MsgAssert( !s_updating_channels, ( "Someone is trying to add an animation channel while in channel update loop" ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAnimationComponent::AnimEquals( Script::CStruct *pParams, Script::CScript *pScript )
{	
	uint32 current_animation=GetCurrentSequence();
	
	Script::CComponent *p_comp=NULL;
	while (true)
	{
		p_comp=pParams->GetNextComponent(p_comp);
		if (!p_comp)
		{
			break;
		}

		if (p_comp->mNameChecksum==0)
		{
			// It's an unnamed component
			
			Script::CArray *p_array=NULL;
			if (p_comp->mType==ESYMBOLTYPE_NAME)
			{
				// It's an unnamed name. Maybe it's the name of a global array ...
				Script::CSymbolTableEntry *p_entry=Script::Resolve(p_comp->mChecksum);
				if (p_entry && p_entry->mType==ESYMBOLTYPE_ARRAY)
				{
					// It is a global array
					p_array=p_entry->mpArray;
				}
				else
				{
					// Nope, it's just a name, so see if it is the name of the current animation.
					if (p_comp->mChecksum==current_animation)
					{
						return true;
					}	
				}
			}
			else if (p_comp->mType==ESYMBOLTYPE_ARRAY)
			{
				p_array=p_comp->mpArray;
			}
			
			if (p_array)
			{
				Dbg_MsgAssert(p_array->GetType()==ESYMBOLTYPE_NAME,("\n%s\nAnimEquals: Array must be of names",pScript->GetScriptInfo()));
				for (uint32 i=0; i<p_array->GetSize(); ++i)
				{
					if (p_array->GetChecksum(i)==current_animation)
					{
						return true;
					}
				}
			}
		}		
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CAnimationComponent::AnimDuration( uint32 checksum )
{
	Gfx::CBonedAnimFrameData *p_anim = find_actual_anim( checksum );

    Dbg_MsgAssert( p_anim, ( "Trying to get duration on an animation that doesn't exist %s %s %s", 
							 Script::FindChecksumName(m_animScriptName), 
							 Script::FindChecksumName(checksum),
							 Script::FindChecksumName(GetObject()->GetID()) ) );

	return ( p_anim->GetDuration() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAnimationComponent::AnimExists( uint32 checksum )
{
	Gfx::CBonedAnimFrameData *p_anim = find_actual_anim( checksum );

	return p_anim;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::AddTime( float incVal )
{
	Gfx::CBlendChannel* pPrimaryChannel = get_primary_channel();
	if ( pPrimaryChannel )
	{
		pPrimaryChannel->AddTime( incVal );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Gfx::CBonedAnimFrameData* CAnimationComponent::find_actual_anim( uint32 checksum )
{
	// need to combine the animation name (Idle) 
	// with the owner animscript (animload_thps5_human)
	// to get the asset name
	return Nx::GetCachedAnim( m_animScriptName + checksum, false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::PrintStatus()
{
	// for debugging the viewer object
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::Update()
{
	if ( Gfx::CBlendChannel* primary_channel = get_primary_channel() )
	{
		m_last_animation_time = primary_channel->GetCurrentAnimTime();
		
		/*
		float s, c, e;
		primary_channel->GetAnimTimes(&s, &c, &e);
		printf("Animation Factor Complete = %f\n", c / (e - s));
		*/
	}
	
	if (m_animation_script_block_active)
	{
		if ( !GetObject()->GetScript() )
		{
			GetObject()->SetScript( new Script::CScript );
		}
		
		// The script should be blocked at this point, if it isn't that must
		// be because the script just got reloaded. So clear the m_animation_script_block_active
		// flag so that it doesn't get stuck on forever.
		if ( !GetObject()->GetScript()->getBlocked() )
		{
			m_animation_script_block_active = false;
		}	
		else
		{
			float start, current, end;
			GetPrimaryAnimTimes( &start, &current, &end);
			
			if ( ( start <= end && current >= m_animation_script_unblock_point ) || ( start >= end && current <= m_animation_script_unblock_point ) )
			{
				GetObject()->GetScript()->UnBlock();
				m_animation_script_block_active = false;
			}	
		}	
	}

	// Alwyas update the channels

	s_updating_channels = true;

	int num_channels = m_blendChannelList.CountItems();
	Gfx::CBlendChannel* pChannel = (Gfx::CBlendChannel*)m_blendChannelList.GetNext();
	for ( int i = 0; i < num_channels; i++ )
	{
		pChannel->Update();
		pChannel = (Gfx::CBlendChannel*)pChannel->GetNext();
	}

	s_updating_channels = false;

	// remove channels which have decided they are complete
	pack_degenerate_channels();

	// (Mick) If there is a suspend component
	// then ask it if we should animate
	
	// This call determines whether the object is sufficiently far away that no animation is disabled,
	// or possibly at an intermediate distance, interleaved. The distance from parent object to camera is cached
	// for subsequent animation LOD calculations.
	Dbg_Assert(mp_suspend_component);
	bool animate = mp_suspend_component->should_animate( &m_parent_object_dist_to_camera );
	if ( animate )
	{	
		update_procedural_bones();
		
		// This call determines whether the object is actually on screen; animation is not requried
		// for offscreen objects.
		if ( ShouldAnimate() )
		{
			update_skeleton();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::UpdateSkeleton()
{
	// don't use the animation cache, because
	// the viewer object doesn't call Update(),
	// which normally is responsible for
	// invalidating the cache at the
	// appropriate times
	Gfx::CBlendChannel* pPrimaryChannel = get_primary_channel();
	if ( pPrimaryChannel )
	{
		pPrimaryChannel->InvalidateCache();
	}

	// allows the viewer object to update the skeleton,
	// outside of the normal Update() function...
	// (the viewer object sometimes suspends the
	// component and updates the time manually,
	// in which case we'd still need to apply
	// the animation to the skeleton...)
	update_skeleton();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAnimationComponent::IsAnimComplete( void )
{
	if ( !get_primary_channel() )
	{
		return false;
	}

    return get_primary_channel()->IsAnimComplete();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAnimationComponent::IsLoopingAnim( void )
{
	if ( !get_primary_channel() )
	{
		return false;
	}

    return get_primary_channel()->IsLoopingAnim();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CAnimationComponent::GetCurrentSequence( void )
{
	if ( !get_primary_channel() )
	{
		return 0;
	}

    return get_primary_channel()->GetCurrentAnim();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
					  
void CAnimationComponent::GetPrimaryAnimTimes(float *pStart, float *pCurrent, float *pEnd)
{
	*pStart = 0.0f;
	*pCurrent = 0.0f;
	*pEnd = 0.0f;

	if ( get_primary_channel() )
	{
		get_primary_channel()->GetAnimTimes( pStart, pCurrent, pEnd );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CAnimationComponent::GetCurrentAnimTime( void )
{
	if ( !get_primary_channel() )
	{
		return 0.0f;
	}

    return get_primary_channel()->GetCurrentAnimTime();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::PlaySequence( uint32 checksum, float BlendPeriod )
{
#ifdef __NOPT_ASSERT__
	if ( !AnimExists( checksum ) )
	{
		Dbg_Message( "Couldn't find anim %s\n", Script::FindChecksumName(checksum) );
	}
#endif

	PlayPrimarySequence( checksum, false, 0.0f, AnimDuration(checksum), Gfx::LOOPING_HOLD, BlendPeriod, 1.0f );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::PlayPrimarySequence( uint32 animName, bool propagate, float start_time, float end_time, Gfx::EAnimLoopingType loop_type, float blend_period, float speed )
{
	if ( animName == 0 )
	{
		animName = CRCD(0x1ca1ff20,"default");
	}

	if ( propagate )   
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		GameNet::PlayerInfo* player;
		
		player = gamenet_man->GetPlayerByObjectID( GetObject()->GetID() );
		if ( player && player->IsLocalPlayer())
		{
			GameNet::MsgPlayPrimaryAnim	anim_msg;
			Net::MsgDesc msg_desc;
			char msg[ Net::Manager::vMAX_PACKET_SIZE ];
			char* stream;
			int size;
			Net::Client* client;
                    
			client = gamenet_man->GetClient( player->GetSkaterNumber());
			Dbg_Assert( client );

			//anim_msg.m_Time = client->m_Timestamp;
            anim_msg.m_Index = animName;
			anim_msg.m_ObjId = GetObject()->GetID();
			anim_msg.m_LoopingType = loop_type;
			anim_msg.m_StartTime = (unsigned short )( start_time * 4096.0f );
			anim_msg.m_EndTime = ((unsigned short )( end_time * 4096.0f ));
			anim_msg.m_BlendPeriod = (unsigned short )( blend_period * 4096.0f );
			anim_msg.m_Speed = (unsigned short )( speed * 4096.0f );

			stream = msg;
			//memcpy( stream, &anim_msg.m_Time, sizeof( unsigned int ));
			//stream += sizeof( int );
			*stream++ = anim_msg.m_LoopingType;
			*stream++ = anim_msg.m_ObjId;
			memcpy( stream, &anim_msg.m_Index, sizeof( uint32 ));
			stream += sizeof( uint32 );
			memcpy( stream, &anim_msg.m_StartTime, sizeof( unsigned short ));
			stream += sizeof( unsigned short );
			memcpy( stream, &anim_msg.m_EndTime, sizeof( unsigned short ));
			stream += sizeof( unsigned short );
			memcpy( stream, &anim_msg.m_BlendPeriod, sizeof( unsigned short ));
			stream += sizeof( unsigned short );
			memcpy( stream, &anim_msg.m_Speed, sizeof( unsigned short ));
			stream += sizeof( unsigned short );

			size = (unsigned int) stream - (unsigned int) msg;
            
			msg_desc.m_Data = msg;
			msg_desc.m_Length = size;
			msg_desc.m_Id = GameNet::MSG_ID_PRIM_ANIM_START;
			client->EnqueueMessageToServer( &msg_desc );
		}
	}
    
	// GJ:  Should this be broken up into two commands/
	// to degenerate the old, and start playing the new?

//	printf( "Playing anim %s\n", Script::FindChecksumName( animName ), blend_period );
    
#ifdef __NOPT_ASSERT__
	// Define this to disable blending
	if ( Script::GetInteger( CRCD(0xf098f123,"disable_blending"), Script::NO_ASSERT ) )
	{
		blend_period = 0.0f;
	}
#endif

	if ( blend_period == 0.0f || !ShouldBlend() )
	{
		destroy_blend_channels();
	}

	create_new_blend_channel( blend_period );

	Gfx::CBlendChannel* pPrimaryChannel = get_primary_channel();
	Dbg_Assert( pPrimaryChannel );
    pPrimaryChannel->PlaySequence( animName, start_time, end_time, loop_type, blend_period, speed, IsFlipped() );

	delete_anim_tags();
	add_anim_tags( animName );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::delete_anim_tags()
{
	Script::CStruct* pTags = GetObject()->GetTags();
	if ( pTags )
	{
		pTags->RemoveComponent( CRCD(0x5db4115f,"AnimTags") );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::add_anim_tags( uint32 animName )
{
	Script::CStruct* pAnimTagTable = Script::GetStructure(CRCD(0x95807202,"AnimTagTable"), Script::ASSERT);
	if ( pAnimTagTable )
	{
		Script::CStruct* pSubStruct;
		if ( pAnimTagTable->GetStructure( animName, &pSubStruct, Script::NO_ASSERT ) )
		{
			Script::CStruct* pTempStruct = new Script::CStruct;

			// add a new anim tags structure to the tags
			Script::CStruct* pTags = new Script::CStruct;
			pTags->AppendStructure( pSubStruct );
			pTempStruct->AddStructurePointer(CRCD(0x5db4115f,"AnimTags"), pTags);

			GetObject()->SetTagsFromScript( pTempStruct );

			delete pTempStruct;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::SetWobbleTarget( float alpha, bool propagate )
{
	if ( propagate )
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		GameNet::PlayerInfo* player;
		static unsigned char s_last_alpha = 255;
		
		player = gamenet_man->GetPlayerByObjectID( GetObject()->GetID() );
		if ( player && player->IsLocalPlayer())
		{
			Net::Client* client;
			GameNet::MsgSetWobbleTarget msg;

			client = gamenet_man->GetClient( player->GetSkaterNumber() );
			Dbg_Assert( client );

			if( alpha < 0 )
			{ 
				alpha = 0;
			}
			else if( alpha > 1 )
			{
				alpha = 1;
			}
			
			//msg.m_Time = client->m_Timestamp;
			msg.m_Alpha = (unsigned char ) ( alpha * 255.0f );
			msg.m_ObjId = GetObject()->GetID();
			
			if( s_last_alpha != msg.m_Alpha )
			{
				Net::MsgDesc msg_desc;

				msg_desc.m_Data = &msg;
				msg_desc.m_Length = sizeof( GameNet::MsgSetWobbleTarget );
				msg_desc.m_Id = GameNet::MSG_ID_SET_WOBBLE_TARGET;
				msg_desc.m_Singular = true;
				client->EnqueueMessageToServer( &msg_desc );
				s_last_alpha = msg.m_Alpha;
			}
		}
	}
	
	if ( get_primary_channel() )
	{
		Script::CStruct* pTempParams = new Script::CStruct;

		pTempParams->AddFloat( CRCD(0x4d747fa0,"wobbletargetalpha"), alpha );

		get_primary_channel()->CallMemberFunction( CRCD(0xd0209498,"setwobbletarget"), pTempParams, NULL );

		delete pTempParams;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static int s_get_wobble_mask( int variable, float value )
{   
	int mask;

	mask = 0;
	switch( variable )
	{
		case GameNet::MsgSetWobbleDetails::vWOBBLE_AMP_A:
			if( value == 0.05f )
			{
				mask = 0;
			}
			else if( value == 0.1f )
			{
				mask = 1;
			}
			else if( value == 10.1f )
			{
				mask = 2;
			}
			else
			{
				Dbg_Printf( "value is %f\n", value );
				Dbg_Assert( 0 );
			}
			break;
		case GameNet::MsgSetWobbleDetails::vWOBBLE_AMP_B:
			if( value == 0.04f )
			{
				mask = 0;
			}
			else if( value == 10.1f )
			{
				mask = 1;
			}
			else
			{
				Dbg_Printf( "VARIABLE: %d Value %f\n", variable, value );
				Dbg_Assert( 0 );
			}
			break;
		case GameNet::MsgSetWobbleDetails::vWOBBLE_K1:
			if( value == 0.0022f )
			{
				mask = 0;
			}
			else if( value == 20.0f )
			{
				mask = 1;
			}
			else
			{
				Dbg_Printf( "value is %f\n", value );
				Dbg_Assert( 0 );
			}
			break;
		case GameNet::MsgSetWobbleDetails::vWOBBLE_K2:
			if( value == 0.0017f )
			{
				mask = 0;
			}
			else if( value == 10.0f )
			{
				mask = 1;
			}
			else
			{
				Dbg_Assert( 0 );
			}
			break;
		case GameNet::MsgSetWobbleDetails::vSPAZFACTOR:
			if( value == 1.5f )
			{
				mask = 0;
			}
			else if( value == 1.0f )
			{
				mask = 1;
			}
			else
			{
				Dbg_Printf( "value is %f\n", value );
				Dbg_Assert( 0 );
			}
			break;
		default:
			Dbg_Assert( 0 );
			break;
	}
	
	return mask;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::SetWobbleDetails( const Gfx::SWobbleDetails& wobble_details, bool propagate )
{
	if ( propagate )
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		GameNet::PlayerInfo* player;

		player = gamenet_man->GetPlayerByObjectID( GetObject()->GetID() );
		if ( player && player->IsLocalPlayer())
		{
			Net::Client* client;
			GameNet::MsgSetWobbleDetails msg;
			int mask;
			static char s_last_mask = -1;
			
			client = gamenet_man->GetClient( player->GetSkaterNumber() );
			Dbg_Assert( client );

			//msg.m_Time = client->m_Timestamp;
			
			msg.m_WobbleDetails = 0;
			mask = s_get_wobble_mask( GameNet::MsgSetWobbleDetails::vWOBBLE_AMP_A, wobble_details.wobbleAmpA );
			msg.m_WobbleDetails |= mask;
			mask = s_get_wobble_mask( GameNet::MsgSetWobbleDetails::vWOBBLE_AMP_B, wobble_details.wobbleAmpB );
			msg.m_WobbleDetails |= ( mask << 2 );
			mask = s_get_wobble_mask( GameNet::MsgSetWobbleDetails::vWOBBLE_K1, wobble_details.wobbleK1 );
			msg.m_WobbleDetails |= ( mask << 3 );
			mask = s_get_wobble_mask( GameNet::MsgSetWobbleDetails::vWOBBLE_K2, wobble_details.wobbleK2 );
			msg.m_WobbleDetails |= ( mask << 4 );
			mask = s_get_wobble_mask( GameNet::MsgSetWobbleDetails::vSPAZFACTOR, wobble_details.spazFactor );
			msg.m_WobbleDetails |= ( mask << 5 );
			msg.m_ObjId = GetObject()->GetID();

			// Only propagate if it's actually different from our last wobble details message
			if( s_last_mask != msg.m_WobbleDetails )
			{
				Net::MsgDesc msg_desc;

				msg_desc.m_Data = &msg;
				msg_desc.m_Length = sizeof( GameNet::MsgSetWobbleDetails );
				msg_desc.m_Id = GameNet::MSG_ID_SET_WOBBLE_DETAILS;
				msg_desc.m_Singular = true;

				client->EnqueueMessageToServer( &msg_desc );

				s_last_mask = msg.m_WobbleDetails;
			}
		}
	}
	
	if ( get_primary_channel() )
	{
		Script::CStruct* pTempParams = new Script::CStruct;

		pTempParams->AddFloat( CRCD(0xfd266a26,"wobbleAmpA"), wobble_details.wobbleAmpA );
		pTempParams->AddFloat( CRCD(0x642f3b9c,"wobbleAmpB"), wobble_details.wobbleAmpB );
		pTempParams->AddFloat( CRCD(0x0f43fd49,"wobbleK1"), wobble_details.wobbleK1 );
		pTempParams->AddFloat( CRCD(0x964aacf3,"wobbleK2"), wobble_details.wobbleK2 );
		pTempParams->AddFloat( CRCD(0xf90b0824,"spazFactor"), wobble_details.spazFactor );

 		get_primary_channel()->CallMemberFunction( CRCD(0xea6d0efd,"setwobbledetails"), pTempParams, NULL );

		delete pTempParams;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::SetLoopingType( Gfx::EAnimLoopingType looping_type, bool propagate )
{
	if ( propagate )
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		GameNet::PlayerInfo* player;
		static char s_last_type = -1;
		
		player = gamenet_man->GetPlayerByObjectID( GetObject()->GetID() );
		if ( player && player->IsLocalPlayer())
		{
			Net::Client* client;
			GameNet::MsgSetLoopingType msg;

			client = gamenet_man->GetClient( player->GetSkaterNumber() );
			Dbg_Assert( client );

			//msg.m_Time = client->m_Timestamp;
			msg.m_LoopingType = looping_type;
			msg.m_ObjId = GetObject()->GetID();
			if( s_last_type != looping_type )
			{
				Net::MsgDesc msg_desc;

				msg_desc.m_Data = &msg;
				msg_desc.m_Length = sizeof( GameNet::MsgSetLoopingType );
				msg_desc.m_Id = GameNet::MSG_ID_SET_LOOPING_TYPE;
				client->EnqueueMessageToServer( &msg_desc );
				s_last_type = looping_type;
			}
		}
	}

	if ( get_primary_channel() )
	{
		get_primary_channel()->SetLoopingType( looping_type );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::ReverseDirection( bool propagate )
{
    Dbg_Assert( !propagate );

	if ( get_primary_channel() )
	{
		get_primary_channel()->ReverseDirection();
	}

	// TODO:  eventually turn this into a net message.
	// For now, only the viewer objects should be reversing the direction
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::SetAnimSpeed( float speed, bool propagate, bool all_channels )
{	
	if ( !get_primary_channel() )
	{
		// no primary channel, so do nothing
		return;
	}

	float cur_speed = get_primary_channel()->GetAnimSpeed();

	if( !all_channels && Mth::Abs( cur_speed - speed ) < vMIN_SPEED_THRESHOLD )
	{
		return;
	}

	if ( propagate )
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		Net::Client* client;
		GameNet::MsgSetAnimSpeed msg;
		GameNet::PlayerInfo* player;

		player = gamenet_man->GetPlayerByObjectID( GetObject()->GetID() );
		if ( player && player->IsLocalPlayer())
		{
			client = gamenet_man->GetClient( player->GetSkaterNumber() );
			if( client )
			{
				Net::MsgDesc msg_desc;

				//msg.m_Time = client->m_Timestamp;
				msg.m_AnimSpeed = speed;
				msg.m_ObjId = GetObject()->GetID();

				msg_desc.m_Data = &msg;
				msg_desc.m_Length = sizeof( GameNet::MsgSetAnimSpeed );
				msg_desc.m_Id = GameNet::MSG_ID_SET_ANIM_SPEED;
				client->EnqueueMessageToServer( &msg_desc );
			}
		}
	}

	if (all_channels)
	{
		Gfx::CBlendChannel* pChannel = (Gfx::CBlendChannel*)m_blendChannelList.GetNext();
		while ( pChannel )
		{
			pChannel->SetAnimSpeed( speed );
			pChannel = (Gfx::CBlendChannel*)pChannel->GetNext();
		}
	}
	else
	{
		get_primary_channel()->SetAnimSpeed( speed );
	}
	
	if (DebugSkaterScripts && GetObject()->GetID() == 0)
	{
		printf("%d: Setting Anim Speed: %f\n",(int)Tmr::GetRenderFrame(),speed);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CAnimationComponent::GetAnimSpeed( Script::CStruct* pParams, Script::CScript* pScript )
{	
	if ( !get_primary_channel() )
	{
		// no primary channel, so do nothing
		return 0;
	}

	float cur_speed = get_primary_channel()->GetAnimSpeed();

    return cur_speed;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::EnableBlending( bool enabled )
{
	m_shouldBlend = enabled;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAnimationComponent::ShouldBlend()
{
	return m_shouldBlend;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Plays an animation given a set of parameters contained in pParams.
// This allows both the skater and other moving objects such as pedestrians to share a common-ish interface.

// Returns the checksum of the animation if it did get successfully played, 0 otherwise.
// (This feature is used by some debug code in the CSkater's PlayAnim script command)
uint32 CAnimationComponent::PlayAnim(Script::CStruct *pParams, Script::CScript *pScript, float defaultBlendPeriod )
{
	if (Gfx::CBlendChannel* primary_channel = get_primary_channel())
	{
		m_last_animation_time = primary_channel->GetCurrentAnimTime();
	}

	if (m_dont_interrupt && !IsAnimComplete())
	{
		return 0;
	}
		
    if ( !has_anims() )
    {
        // some items don't have animation data associated with it yet...
        return 0;
    }

	uint32 AnimChecksum=0;
	pParams->GetChecksum( CRCD(0x98549ba4,"Anim"), &AnimChecksum );
	if ( !AnimChecksum )
	{
		Script::PrintContents(pParams);
		Dbg_MsgAssert(0,("\n%s\nobj: %s\nPlayAnim requires an anim name",Script::FindChecksumName(GetObject()->GetID()),pScript->GetScriptInfo()));
	}
	
	float Speed=1.0f;
	pParams->GetFloat( CRCD(0xf0d90109,"Speed"), &Speed );   
	
	uint32 CurrentAnimChecksum = GetCurrentSequence();
	if ( AnimChecksum == CRCD(0x230ccbf4,"Current") )
	{
		AnimChecksum = CurrentAnimChecksum;
	}
	else
	{
		if (CurrentAnimChecksum==AnimChecksum)
		{
			if ( pParams->ContainsFlag( CRCD(0xfe09fe09,"NoRestart") ) )
			{
				// Return without doing anything, since the user specified NoRestart, and the
				// current anim is the same as the requested one.
				return 0;
			}
		}	
	}

	if ( !AnimExists( AnimChecksum ) )
	{
#ifdef __NOPT_ASSERT__
		if ( Script::GetInt( CRCD(0x56d4128e,"AssertOnMissingAnims"), Script::NO_ASSERT ) )
		{
			Dbg_MsgAssert( 0, ( "*** Anim %s (%s) was not defined for object %s!",
							Script::FindChecksumName(AnimChecksum),
							Script::FindChecksumName(m_animScriptName),
							Script::FindChecksumName(GetObject()->GetID() ) ) );
		}
		else if ( Script::GetInt( CRCD(0xf42cb81a,"WarnOnMissingAnims"), Script::NO_ASSERT ) )
		{
			Dbg_Message( "*** Anim %s (%s) was not defined for object %s!",
						 Script::FindChecksumName(AnimChecksum),
						 Script::FindChecksumName(m_animScriptName),
						 Script::FindChecksumName(GetObject()->GetID() ) );
		}
#endif

		// by default, a missing anim will play the christ-pose anim
		AnimChecksum = CRCD(0x1ca1ff20,"default");
	}
	
	float BlendPeriod=defaultBlendPeriod;
	float BlendPeriodPercent=0.0f;
	if (mGotBlendPeriodOut)
	{
		// If a BlendPeriodOut command was issued, use that value to override any
		// value specified by PlayAnim.
		BlendPeriod=mBlendPeriodOut;
		// but clear the flag for next time.
		mGotBlendPeriodOut=false;
	}
	else if (pParams->GetFloat(CRCD( 0x4f3abd6c,"BlendPeriodPercent"), &BlendPeriodPercent) )
	{
		BlendPeriod = AnimDuration(AnimChecksum) * (BlendPeriodPercent/100.0f);
	}
	else
	{
		pParams->GetFloat( CRCD(0x8f0d24ed,"BlendPeriod"), &BlendPeriod );
	}	
	
	float Duration=AnimDuration(AnimChecksum);
	float From=0;
	float Current=0;
	float To=Duration;
	
	if ( pParams->ContainsFlag( CRCD(0x099371ae,"SyncToPreviousAnim") ) )
	{
		float Dummy, PreviousAnimCurrent, PreviousAnimDuration;
		
		GetPrimaryAnimTimes(&Dummy, &PreviousAnimCurrent, &Dummy);
		PreviousAnimDuration = AnimDuration(CurrentAnimChecksum);
		pParams->GetFloat(CRCD(0x4b4a153a, "EffectivePreviousAnimDuration"), &PreviousAnimDuration);
		
		Current = Mth::Clamp(PreviousAnimCurrent / AnimDuration(CurrentAnimChecksum), 0.0f, 1.0f) * Duration;
		if ( pParams->ContainsFlag( CRCD(0xf8cfd515,"Backwards") ) )
		{
			Current = -(Duration - Current);
		}
	}
	else if ( pParams->ContainsFlag( CRCD(0xff5ac79e, "SyncToReversePreviousAnim") ) )
	{
		float Dummy, PreviousAnimCurrent, PreviousAnimDuration;
		
		GetPrimaryAnimTimes(&Dummy, &PreviousAnimCurrent, &Dummy);
		PreviousAnimDuration = AnimDuration(CurrentAnimChecksum);
		pParams->GetFloat(CRCD(0x4b4a153a, "EffectivePreviousAnimDuration"), &PreviousAnimDuration);
		
		Current = Mth::Clamp((PreviousAnimDuration - PreviousAnimCurrent) / PreviousAnimDuration, 0.0f, 1.0f) * Duration;
		if ( pParams->ContainsFlag( CRCD(0xf8cfd515,"Backwards") ) )
		{
			Current = -(Duration - Current);
		}
	}

	// Need TempCurrent, so that it doesn't conflict with regular Current
	// which is possibly modified in the above "SyncToPreviousAnim" code block
	float TempCurrent;
	float Dummy;
	GetPrimaryAnimTimes(&Dummy,&TempCurrent,&Dummy);
	Gfx::GetTimeFromParams( &From, &To, TempCurrent, Duration, pParams, pScript );

	Gfx::EAnimLoopingType LoopingType;
	Gfx::GetLoopingTypeFromParams( &LoopingType, pParams );

	m_dont_interrupt=pParams->ContainsFlag( CRCD(0x84f13067,"DontInterrupt") );
	
#ifdef __NOPT_ASSERT__
	if ( Script::GetInt( CRCD(0xca108bce,"DebugAnims"), false ) )
	{
		if ( GetObject()->GetID() == 0 )
		{
			printf( "DebugAnims:  Playing skater anim %s\n", Script::FindChecksumName(AnimChecksum) );
		}
	}
	
	if (DebugSkaterScripts && AnimChecksum && GetObject()->GetID() == 0)
	{
		printf("%d: Playing anim '%s'\n",(int)Tmr::GetRenderFrame(),Script::FindChecksumName(AnimChecksum));
	}
#endif

	PlayPrimarySequence( AnimChecksum, true, From, To, LoopingType, BlendPeriod, Speed );
	
	if (Current != 0.0f)
	{
		get_primary_channel()->AddTime(Current);
	}
	
	/*
	if ( GetObject()->GetID() == 0 )
	{
		printf("-standard-anim-\n");
		printf("Anim = %s\n", Script::FindChecksumName(AnimChecksum));
		printf("From = %f\n", From);
		printf("To = %f\n", To);
		printf("Duration = %f\n", Duration);
		printf("Current = %f\n", Current);
		printf("Speed = %f\n", Speed);
		printf("BlendPeriod = %f\n", BlendPeriod);
	}
	*/
	
	uint32 PartialAnim;
	if (pParams->GetChecksum(CRCD(0x6c565f57, "PartialAnimOverlay"), &PartialAnim))
	{
		Script::CStruct* pPartialAnimParams = new Script::CStruct;
		
		uint32 partial_anim_id;
		if (pParams->GetChecksum(CRCD(0xf0ce9e0f, "PartialAnimOverlayId"), &partial_anim_id))
		{
			pPartialAnimParams->AddChecksum(CRCD(0x40c698af, "Id"), partial_anim_id);
		}
		pPartialAnimParams->AddChecksum(CRCD(0x7321a8d6, "Type"), CRCD(0x659bf355, "PartialAnim"));
		pPartialAnimParams->AddChecksum(CRCD(0x6c2bfb7f, "AnimName"), PartialAnim);
		pPartialAnimParams->AddFloat(CRCD(0x46e55e8f, "From"), From);
		pPartialAnimParams->AddFloat(CRCD(0x28782d3b, "To"), To);
		pPartialAnimParams->AddChecksum(NO_NAME, CRCD(0xd029f619, "Seconds"));
		pPartialAnimParams->AddFloat(CRCD(0x230ccbf4, "Current"), Current);
		pPartialAnimParams->AddFloat(CRCD(0xf0d90109, "Speed"), Speed);
		switch (LoopingType)
		{
			case Gfx::LOOPING_HOLD:
				break;
			case Gfx::LOOPING_CYCLE:
				pPartialAnimParams->AddChecksum(NO_NAME, CRCD(0x4f792e6c, "Cycle"));
				break;
			case Gfx::LOOPING_PINGPONG:
				pPartialAnimParams->AddChecksum(NO_NAME, CRCD(0x3153e314, "PingPong"));
				break;
			case Gfx::LOOPING_WOBBLE:
				pPartialAnimParams->AddChecksum(NO_NAME, CRCD(0x6d941203, "Wobble"));
				break;
		}
		
		get_primary_channel()->AddController(pPartialAnimParams);
		
		delete pPartialAnimParams;
		
		#ifdef __NOPT_ASSERT__
		if ( GetObject()->GetID() == 0 && Script::GetInt( CRCD(0xca108bce,"DebugAnims"), false ) )
		{
			printf( "DebugAnims:  Playing skater partial anim overlay %s\n", Script::FindChecksumName(PartialAnim) );
		}
		
		if (DebugSkaterScripts && PartialAnim && GetObject()->GetID() == 0)
		{
			printf("%d: Playing partial anim overlay '%s'\n",(int)Tmr::GetRenderFrame(),Script::FindChecksumName(PartialAnim));
		}
		#endif
	}

	return AnimChecksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// static workspace data
static Gfx::CPose	sBlendPoses[vNUM_DEGENERATE_ANIMS];
static float		sBlendValues[vNUM_DEGENERATE_ANIMS];
static Gfx::CPose	sResultPose;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void blend( Gfx::CPose* pPoseList, float* blendVal, int numBones, Gfx::CPose* pResultPose, int numBlendChannels )
{
	// the blend value of the primary animation 
	// should be 1.0, since it's active.
	Dbg_Assert( blendVal[0] == 1.0f );

	float totalBlendVal = 0.0f;
	for ( int i = 1; i < numBlendChannels; i++ )
	{
		if ( blendVal[i] )
		{
			totalBlendVal += 1.0f;
		}
	}

	float degenerateBlendTotal = 0.0f;
	for ( int i = 1; i < numBlendChannels; i++ )
	{
		degenerateBlendTotal += blendVal[i];
	}
	
	// normalize them so that they all add up to total blend value
	blendVal[0] = totalBlendVal - degenerateBlendTotal;

	// Scan through the sets of frames, interpolating between the 2 based on the blend values.
	float sum = 0.0f;

	int i0, i1;
	for ( i1 = (numBlendChannels - 1); i1 > 0; --i1 )
	{
		float blend1 = blendVal[i1];
		if ( blend1 )
		{
			blend1 += sum;

			i0 = i1 - 1;
			while(( blendVal[i0] <= 0.0f ) && ( i0 > 0 ))
			{
				--i0;
			}

			float blend0 = blendVal[i0];

			sum += blendVal[i1];

			// Need to normalise blend0 and blend1 so they sum to 1.0.
			blend0 /= ( blend0 + blend1 );
			blend1 = 1.0f - blend0;

			Mth::Quat* pRotations0 = pPoseList[i0].m_rotations;
			Mth::Quat* pRotations1 = pPoseList[i1].m_rotations;
			Mth::Vector* pTranslations0 = pPoseList[i0].m_translations;
			Mth::Vector* pTranslations1 = pPoseList[i1].m_translations;

			for( int b = 0; b < numBones; b++ )
			{
				*pRotations0 = Mth::FastSlerp( *pRotations0, *pRotations1, 1.0f - blend0 );
				
				*pTranslations0 = Mth::Lerp( *pTranslations0, *pTranslations1, 1.0f - blend0 );

				pRotations0++;
				pRotations1++;
				pTranslations0++;
				pTranslations1++;
			}
		}
	}

	// copy the values into the result pose:

	Mth::Quat* pSrcQuat = &pPoseList[0].m_rotations[0];
	Mth::Vector* pSrcVector = &pPoseList[0].m_translations[0];
	
	Mth::Quat* pDstQuat = &pResultPose->m_rotations[0];
	Mth::Vector* pDstVector = &pResultPose->m_translations[0];

	memcpy( pDstQuat, pSrcQuat, numBones * sizeof(Mth::Quat) );
	memcpy( pDstVector, pSrcVector, numBones * sizeof(Mth::Vector) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CAnimationComponent::get_blend_channel( int blendChannel, Gfx::CPose* pResultPose, float* pBlendVal )
{
	float blendVal = 0.0f;
	Gfx::CBlendChannel* pBlendChannel = NULL;
	if ( 1 || ShouldBlend() )
	{
		Dbg_MsgAssert( blendChannel >= 0 && blendChannel < (int)m_blendChannelList.CountItems(), ( "out of range blend channel %d (0-%d)", blendChannel, m_blendChannelList.CountItems() ) ); 
		pBlendChannel = (Gfx::CBlendChannel*)m_blendChannelList.GetItem(blendChannel);
		blendVal = pBlendChannel->GetBlendValue();
		Dbg_MsgAssert( blendChannel != 0 || blendVal == 1.0f, ( "Expected primary channel to have a blend value of 1.0f" ) );
	}
	else
	{
		switch ( blendChannel )
		{
			case 0:
				pBlendChannel = get_primary_channel();
				blendVal = 1.0f;
				break;
			default:
				Dbg_MsgAssert( 0, ( "Invalid blend channel %d", blendChannel ) );
		}
	}
	
	if ( blendVal <= 0.0f )
	{
		// GJ TODO:  I'm not sure why this case happens,
		// but it happened on THPS4 as well...  i should
		// look into this at some point...
		*pBlendVal = 0.0f;
		return;
	}

	Gfx::CSkeleton* pSkeleton = NULL;

	Dbg_Assert(mp_skeleton_component);
	pSkeleton = mp_skeleton_component->GetSkeleton();

	if ( !pBlendChannel->GetPose( pResultPose ) )//, 
								  // m_flags & nxANIMCOMPONENTFLAGS_FLIPPED,
								  // m_flags & nxANIMCOMPONENTFLAGS_ROTATESKATEBOARD,
								  // pSkeleton ) )
	{
		// couldn't find pose...
		// if it's the primary animation, assert
		if ( blendChannel == 0 )
		{
			Dbg_MsgAssert( 0, ( "No primary animation found!  Is %08x/%s playing a default animation?", GetObject()->GetID(), Script::FindChecksumName(m_animScriptName) ) );
		}
	}
	
	*pBlendVal = blendVal;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAnimationComponent::ShouldAnimate()
{
#if 1

// The model will be inactive if off the screen
	Nx::CModel* p_model = mp_model_component->GetModel();
	return ( p_model && p_model->GetActive() );

#else

// Old version, kind of mixed up	
	
	
	// this function will eventually be used to LOD
	// our animations (i.e. updating every Nth frame
	// based on distance)
	
	// if we did this, we would want to make sure that
	// the model's update function to be called at least
	// once to sync up the skeleton initially (we don't 
	// want to do it in the constructor, because then
	// there's an order dependency on the components)
	
	// GJ:  Checking whether the model is visible
	// is a speed optimization, but it's ugly 
	// because it makes this component dependent on the 
	// model component...  Eventually, I'd like to 
	// find a more elegant solution
		
	// update the skeleton, if it exists
	Dbg_Assert(mp_model_component);
	Nx::CModel* p_model = mp_model_component->GetModel();
	if ( p_model && p_model->GetActive() )
	{
		Mth::Vector sphere = p_model->GetBoundingSphere();
		
		printf ("sphere = (%.2f,%.2f,%.2f,%.2f)\n",sphere[X],sphere[Y],sphere[Z],sphere[W]);

		// GJ:  The bounding sphere may have a position
		// offset built into it, so need to factor that in
		Mth::Vector pos = GetObject()->GetPos() + sphere;
		pos[W] = 1.0f;
		
		// Mick:  This visibility test always returns true
		// when in split screen mode 	
		if ( !Nx::CEngine::sIsVisible( pos,sphere[3] ) )
		{
			return false;
		}
	}

	// for now, just always animate, even if no model, or the model is inactive !?
	return true;
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::update_skeleton()
{
	if ( has_anims() )
	{
		Gfx::CSkeleton* pSkeleton = NULL;
		
		Dbg_Assert(mp_skeleton_component);
		pSkeleton = mp_skeleton_component->GetSkeleton();

		if ( !pSkeleton )
		{
			return;
		}

		// Set the current view distance for the skeleton so that it may decide on an appropriate set of
		// bones to use in the animation.
		pSkeleton->SetBoneSkipDistance( m_parent_object_dist_to_camera );

		int numBlendChannels = m_blendChannelList.CountItems();

		for ( int blendChannel = 0; blendChannel < numBlendChannels; blendChannel++ )
		{
			// this wil loop through each blend channel's animation channels
			get_blend_channel( blendChannel, &sBlendPoses[blendChannel], &sBlendValues[blendChannel] );
		}

		// TODO: Here, we could run any controllers that need to act on the final, post-blend pose...
		if( numBlendChannels > 1 )
		{
			blend( &sBlendPoses[0], &sBlendValues[0], pSkeleton->GetNumBones(), &sResultPose, numBlendChannels );
			pSkeleton->Update( &sResultPose );
		}
		else
		{
			// In the case that there is just one blend channel, there is no requirement to call the blend
			// function, which will needlessly copy the quaternion and translation values for each bone from
			// the sBlendPoses buffer to the sResultPose buffer. Just use the sBlendPoses buffer directly. 
			pSkeleton->Update( &sBlendPoses[0] );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAnimationComponent::FlipAnimation( uint32 objId, bool flip, uint32 time, bool propagate )
{
	if( propagate )
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		GameNet::PlayerInfo* player;
        		
		player = gamenet_man->GetPlayerByObjectID( objId );
		if( player && player->IsLocalPlayer())
		{
			Net::Client* client;
			GameNet::MsgFlipAnim msg;
			Net::MsgDesc msg_desc;
			
			client = gamenet_man->GetClient( player->GetSkaterNumber());
			Dbg_Assert( client );

			msg.m_ObjId = objId;
			msg.m_Flipped = flip;
			//msg.m_Time = time;

			msg_desc.m_Data = &msg;
			msg_desc.m_Length = sizeof( GameNet::MsgFlipAnim );
			msg_desc.m_Id = GameNet::MSG_ID_FLIP_ANIM;
			client->EnqueueMessageToServer( &msg_desc );
		}
	}

	bool oldFlipped = m_flags & nxANIMCOMPONENTFLAGS_FLIPPED;

	SetFlipState( flip );

    return oldFlipped;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CAnimationComponent::SetFlipState( bool shouldFlip )
{
	// TODO:  should actually be in blend channel code
	// since we want certain blend channels to be flipped
	// (the animation component will still need to know
	// which flip state to make new channels)

	m_flags &= ~nxANIMCOMPONENTFLAGS_FLIPPED;
	m_flags |= ( shouldFlip ? nxANIMCOMPONENTFLAGS_FLIPPED : 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CAnimationComponent::destroy_blend_channels()
{
	m_blendChannelList.DestroyAllNodes();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
		
void CAnimationComponent::initialize_procedural_bones( Script::CStruct* pParams )
{
	Script::CArray* pArray;

	pParams->GetArray( CRCD(0x3ed7262b,"bone_list"), &pArray, Script::ASSERT );
	
	for ( uint32 i = 0; i < pArray->GetSize(); i++ )
	{
		this->InitializeProceduralBone( pArray->GetChecksum(i) );
	}
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
		
void CAnimationComponent::update_procedural_bones()
{
	int numProceduralBones = this->GetNumProceduralBones(); 
	Gfx::CProceduralBone* p_current_procedural_bone = this->GetProceduralBones();

	for ( int i = 0; i < numProceduralBones; i++ )
	{
		// TODO:  Factor in the frame rate...
		
		if ( p_current_procedural_bone->transEnabled )
		{
			p_current_procedural_bone->currentTrans += p_current_procedural_bone->deltaTrans;
			p_current_procedural_bone->currentTrans[X] = (int)p_current_procedural_bone->currentTrans[X] & 0xfff;
			p_current_procedural_bone->currentTrans[Y] = (int)p_current_procedural_bone->currentTrans[Y] & 0xfff;
			p_current_procedural_bone->currentTrans[Z] = (int)p_current_procedural_bone->currentTrans[Z] & 0xfff;
		}

		if ( p_current_procedural_bone->rotEnabled )
		{
			p_current_procedural_bone->currentRot += p_current_procedural_bone->deltaRot;
			p_current_procedural_bone->currentRot[X] = (int)p_current_procedural_bone->currentRot[X] & 0xfff;
			p_current_procedural_bone->currentRot[Y] = (int)p_current_procedural_bone->currentRot[Y] & 0xfff;
			p_current_procedural_bone->currentRot[Z] = (int)p_current_procedural_bone->currentRot[Z] & 0xfff;
		}

		if ( p_current_procedural_bone->scaleEnabled )
		{
			p_current_procedural_bone->currentScale += p_current_procedural_bone->deltaScale;
			p_current_procedural_bone->currentScale[X] = (int)p_current_procedural_bone->currentScale[X] & 0xfff;
			p_current_procedural_bone->currentScale[Y] = (int)p_current_procedural_bone->currentScale[Y] & 0xfff;
			p_current_procedural_bone->currentScale[Z] = (int)p_current_procedural_bone->currentScale[Z] & 0xfff;
		}

		p_current_procedural_bone++;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CAnimationComponent::InitializeProceduralBone( uint32 boneName )
{
	if ( m_numProceduralBones >= vMAXPROCEDURALBONES )
	{
		Dbg_MsgAssert( 0, ( "Too many procedural bones" ) );
		return false;
	}

	Gfx::CProceduralBone* pBone = &mp_proceduralBones[m_numProceduralBones];

	pBone->m_name = boneName;

	pBone->transEnabled = false;
	pBone->rotEnabled = true;

	pBone->trans0[X] = Script::GetFloat( CRCD(0x802638f7,"trans_min_x"), Script::NO_ASSERT );
	pBone->trans0[Y] = Script::GetFloat( CRCD(0xf7210861,"trans_min_y"), Script::NO_ASSERT );
	pBone->trans0[Z] = Script::GetFloat( CRCD(0x6e2859db,"trans_min_z"), Script::NO_ASSERT);
	pBone->trans0[W] = 1.0f;

	pBone->trans1[X] = Script::GetFloat( CRCD(0x5d39cfda,"trans_max_x"), Script::NO_ASSERT );
	pBone->trans1[Y] = Script::GetFloat( CRCD(0x2a3eff4c,"trans_max_y"), Script::NO_ASSERT );
	pBone->trans1[Z] = Script::GetFloat( CRCD(0xb337aef6,"trans_max_z"), Script::NO_ASSERT );
	pBone->trans1[W] = 1.0f;

	pBone->currentTrans = Mth::Vector(0.0f,0.0f,0.0f,0.0f);
	pBone->deltaTrans = Mth::Vector(32.0f,32.0f,32.0f,0.0f);

	pBone->rot0[X] = Script::GetFloat( CRCD(0x2dafa9c5,"rot_min_x"), Script::NO_ASSERT ) * 2.0f * Mth::PI / 4096.0f;
	pBone->rot0[Y] = Script::GetFloat( CRCD(0x5aa89953,"rot_min_y"), Script::NO_ASSERT ) * 2.0f * Mth::PI / 4096.0f;
	pBone->rot0[Z] = Script::GetFloat( CRCD(0xc3a1c8e9,"rot_min_z"), Script::NO_ASSERT ) * 2.0f * Mth::PI / 4096.0f;
	pBone->rot0[W] = 1.0f;

	pBone->rot1[X] = Script::GetFloat( CRCD(0xf0b05ee8,"rot_max_x"), Script::NO_ASSERT ) * 2.0f * Mth::PI / 4096.0f;
	pBone->rot1[Y] = Script::GetFloat( CRCD(0x87b76e7e,"rot_max_y"), Script::NO_ASSERT ) * 2.0f * Mth::PI / 4096.0f;
	pBone->rot1[Z] = Script::GetFloat( CRCD(0x1ebe3fc4,"rot_max_z"), Script::NO_ASSERT ) * 2.0f * Mth::PI / 4096.0f;
	pBone->rot1[W] = 1.0f;

	pBone->currentRot = Mth::Vector(0.0f,0.0f,0.0f,0.0f);
	pBone->deltaRot = Mth::Vector(32.0f,32.0f,32.0f,0.0f);

	pBone->scale0[X] = 1.0f; //Script::GetFloat("scale_min_x",Script::NO_ASSERT) * 2.0f * Mth::PI / 4096.0f;
	pBone->scale0[Y] = 1.0f; //Script::GetFloat("scale_min_y",Script::NO_ASSERT) * 2.0f * Mth::PI / 4096.0f;
	pBone->scale0[Z] = 1.0f; //Script::GetFloat("scale_min_z",Script::NO_ASSERT) * 2.0f * Mth::PI / 4096.0f;
	pBone->scale0[W] = 1.0f;

	pBone->scale1[X] = 1.0f; //Script::GetFloat("scale_max_x",Script::NO_ASSERT) * 2.0f * Mth::PI / 4096.0f;
	pBone->scale1[Y] = 1.0f; //Script::GetFloat("scale_max_y",Script::NO_ASSERT) * 2.0f * Mth::PI / 4096.0f;
	pBone->scale1[Z] = 1.0f; //Script::GetFloat("scale_max_z",Script::NO_ASSERT) * 2.0f * Mth::PI / 4096.0f;
	pBone->scale1[W] = 1.0f;

	pBone->currentScale = Mth::Vector(0.0f,0.0f,0.0f,0.0f);
	pBone->deltaScale = Mth::Vector(32.0f,32.0f,32.0f,0.0f);

	m_numProceduralBones++;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Gfx::CProceduralBone* CAnimationComponent::GetProceduralBoneByName( uint32 boneName )
{
	for ( int i = 0; i < m_numProceduralBones; i++ )
	{
		if ( boneName == mp_proceduralBones[i].m_name )
		{
			return &mp_proceduralBones[i];
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CAnimationComponent::GetNumProceduralBones()
{
	return m_numProceduralBones;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Gfx::CProceduralBone* CAnimationComponent::GetProceduralBones()
{
	return mp_proceduralBones;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CAnimationComponent::IsFlipped()
{
	return m_flags & nxANIMCOMPONENTFLAGS_FLIPPED;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimationComponent::Finalize()
{
	mp_skeleton_component = GetSkeletonComponentFromObject( GetObject() );
	mp_suspend_component = GetSuspendComponentFromObject( GetObject() );
	mp_model_component = GetModelComponentFromObject( GetObject() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}
