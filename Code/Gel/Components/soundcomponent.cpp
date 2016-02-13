//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SoundComponent.cpp
//* OWNER:          Mick West
//* CREATION DATE:  10/17/2002
//****************************************************************************

// start autoduck documentation
// @DOC soundcomponent.cpp
// @module soundcomponent | None
// @subindex Scripting Database
// @index script | soundcomponent

#include <gel/components/soundcomponent.h>

#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/component.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/utils.h>
#include <gel/objsearch.h>
#include <gel/objman.h>

#include <gel/object/compositeobject.h>
#include <gfx/nxviewman.h>

// TODO:  Refactor this - 
#include <sk/modules/frontend/frontend.h>
#include <sk/objects/emitter.h>
#include <sk/objects/proxim.h>


namespace Obj
{


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent *	CSoundComponent::s_create()
{
	return static_cast<CBaseComponent*>(new CSoundComponent);	
}
    
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSoundComponent::CSoundComponent() : CBaseComponent()
{
	SetType(CRC_SOUND);
	mp_emitter = NULL;
	mp_proxim_node = NULL;
	m_pos.Set();
	m_old_pos.Set();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSoundComponent::~CSoundComponent()
{
	// Tell the sound manager that the sond component is being removed to clean up
	// any sound effects that are attached to this
	Sfx::CSfxManager::Instance()->ObjectBeingRemoved( this );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSoundComponent::InitFromStructure( Script::CStruct* pParams )
{
	uint32 proxim_node_checksum;

	if (pParams->GetChecksum("ProximNode", &proxim_node_checksum))
	{
		//Dbg_Message("SoundComponent: Looking for ProximNode %s", Script::FindChecksumName(proxim_node_checksum));
		mp_proxim_node = CProximManager::sGetNode(proxim_node_checksum);
		//if (mp_proxim_node)
		//{
		//	Dbg_Message("Found ProximNode %s", Script::FindChecksumName(mp_proxim_node->m_name));
		//}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSoundComponent::Update()
{
	
	// For tracking the position of the sound, then 
	// we need to update the old position and the current position
	m_old_pos = m_pos;
	m_pos = GetObject()->m_pos;
	
	
	UpdateMutedSounds();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSoundComponent::Teleport()
{
	// Garrett: Note that this doesn't update currently playing sounds.  That won't happen
	// until the next sound update iteration.
	Update();

	//Dbg_Message("SoundComponent: Teleporting to (%f, %f, %f)", m_pos[X], m_pos[Y], m_pos[Z]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


CBaseComponent::EMemberFunctionResult CSoundComponent::CallMemberFunction( uint32 Checksum, Script::CScriptStructure *pParams, Script::CScript *pScript )
{

	switch (Checksum)
	{
		// @script | Obj_PlaySound | Add your own enums to sfx_*.q.  Up to 8 allowed (0 - 7), 
		// @flag Filename | name of the sound (filename without the quotes) or 
		// Type = [as defined in Obj_SetSound] 
		// @parm name | Type | the type (if this is being called from a car script, for example, 
		// use CARSFX_HONK, or another enum from sfx_car.q (similarly for ped: sfx_ped.q)
		// @parmopt float | vol | 100 | This is the volume of the wav AFTER being modified 
		// by the volume (if specified) in LoadSound 
		// @parmopt float | pitch | 100 | range: 1 to 300 (assuming normal pitch in LoadSound) 
		case 0xafa20e18:  // "Obj_PlaySound"
			PlayScriptedSound( pParams );
			return MF_TRUE;
	
		// @script | Obj_AdjustSound | This function adjusts a looping sound 
		// associated with an object. If no volumeStep is specified, volume is 
		// adjusted immediately, same with pitchStep and pitch.  If volumeStep 
		// or pitchStep are specified, the current volume/pitch is adjusted towards 
		// the target by the step, each frame (or 60 times a second, rather). So 
		// if the current pitch is 100, and you call: Obj_AdjustSound pitch = 40 
		// pitchStep = 1 It will take one second for the pitch to go from 100 to 40. 
		// The percent values (pitchPercent and volumePercent) modify the overall 
		// pitch or volume after taking into consideration the pitch specified globally 
		// during the LoadSound call, then specified per this object on the Obj_
		// PlaySound call (see example).
		// @flag Filename | name of the sound you want to adjust (no quotes)
		// @parm float | VolumePercent | Percentage to adjust the volume
		// @parm float | PitchPercent | Percentage to adjust the pitch
		// @parmopt float | VolumeStep | 0 |  Amount of time in seconds it takes to go from the current volume to new volume.
		// @parmopt float | PitchStep | 0 | Amount of time in seconds it takes to go from the current pith to new pitch.
		case 0x3e5e8023: // "Obj_AdjustSound"
			AdjustObjectSound( pParams, pScript );
			return MF_TRUE;
		
		// @script | Obj_StopSound | Obj_StopSound doesn't take a type = parameter... 
		// it is used only for stopping a looping sound by checksum. If you don't 
		// specify the name of the sound, all sounds that were loaded with PosUpdate 
		// or PosUpdateWithDoppler parameters and are associated with the object 
		// calling this will be stopped.
		// @flag Filename | name of the sound you want to stop (no quotes)
		case 0x4e4132d6: // "Obj_StopSound"
		{
			uint32 soundChecksum = 0;
			pParams->GetChecksum( NONAME, &soundChecksum );
			Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();
			sfx_manager->StopObjectSound( this, soundChecksum );
			// if it's temporarily out of range, we gotta stop it too!
			int i;
			for ( i = 0; i < MAX_NUM_LOOPING_SOUNDS_PER_OBJECT; i++ )
			{
				if ( mSoundInfo[ i ].checksum == soundChecksum )
				{
					// since a sound checksum of 0 is invalid, this turns off the update:
					mSoundInfo[ i ].checksum = 0;
				}
			}
			return MF_TRUE;
		}
		
		// @script | Obj_SetSound | Add your own enums to sfx_*.q.  Up to 8 allowed (0 - 7), 
		// can be increased easily.  Use Obj_PlaySound (with the optional Type = * parameter 
		// instead of the filename) to trigger one of these sounds to be played.  Don't 
		// trigger a looping sound!  The engine loop on the car needs to be set up per car 
		// using this function, but the update happens in the game engine (pun regretted because 
		// it's confusing).  Don't forget to load the sounds specified by 'filename'.
		// @flag Filename | Name of the sound, no quotes
		// @parm name | Type | the type (if this is being called from a car script, for example, 
		// use CARSFX_HONK, or another enum from sfx_car.q (similarly for ped: sfx_ped.q)
		case 0xe8347171:  // "Obj_SetSound"
			SetSound( pParams );
			return MF_TRUE;
	}
	return MF_NOT_EXECUTED;
}

void CSoundComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSoundComponent::GetDebugInfo"));
	// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
	
	Script::CArray *p_array=new Script::CArray;
	p_array->SetSizeAndType(MAX_NUM_SOUNDFX_CHECKSUMS,ESYMBOLTYPE_NAME);
	for (int i=0; i<MAX_NUM_SOUNDFX_CHECKSUMS; ++i)
	{
		p_array->SetChecksum(i,mSoundFXChecksums[i]);
	}
	p_info->AddArrayPointer("mSoundFXChecksums",p_array);

	p_array=new Script::CArray;
	p_array->SetSizeAndType(MAX_NUM_LOOPING_SOUNDS_PER_OBJECT,ESYMBOLTYPE_STRUCTURE);
	for (int i=0; i<MAX_NUM_LOOPING_SOUNDS_PER_OBJECT; ++i)
	{
		Script::CStruct *p_struct=new Script::CStruct;
		mSoundInfo[i].GetDebugInfo(p_struct);
		p_array->SetStructure(i,p_struct);
	}	
	p_info->AddArrayPointer("mSoundInfo",p_array);
#endif				 
}

#define CHECKSUM_TYPE	0x7321a8d6  // 'type'

// Set individual soundfx for objects...
// Designers can also trigger these sounds to be played by using the Obj_PlaySound
// script command, using type =  (instead of using the checksum) to define the sound.
void CSoundComponent::SetSound( Script::CScriptStructure *pParams )
{
	
	int type = -1;
	uint32 soundChecksum = 0;
	pParams->GetInteger( CHECKSUM_TYPE, &type );
	pParams->GetChecksum( NONAME, &soundChecksum );
	if ( type == -1 )
	{
		Dbg_MsgAssert( 0,( "Script command Obj_SetSound used without specifying sound type." ));
		return;
	}
	if ( !soundChecksum )
	{
		Dbg_MsgAssert( 0,( "Script command Obj_SetSound used without specifying name of sound." ));
		return;
	}
	if ( ( type > MAX_NUM_SOUNDFX_CHECKSUMS ) || ( type == -1 ) )
	{
		Dbg_MsgAssert( 0,( "Matt needs to increase MAX_NUM_SOUND_CHECKSUMS (or bad type %d sent in...)", type ));
		return;
	}
	mSoundFXChecksums[ type ] = soundChecksum;
} // end of SetSound( )


void CSoundComponent::PlayScriptedSound( Script::CScriptStructure *pParams )
{
	uint32 soundChecksum = 0;
	uint32 emitterChecksum = 0;
	uint32 dropoffFuncChecksum;
	float volume = 100.0f;
	float pitch = 100.0f;
	float dropoffDist = 0.0f;
	EDropoffFunc dropoffFunction = DROPOFF_FUNC_STANDARD;
	bool noPosUpdate = false;
    
	pParams->GetChecksum( NONAME, &soundChecksum );
	pParams->GetFloat( 0xf6a36814, &volume ); // "vol"
	pParams->GetFloat( 0xd8604126, &pitch ); // "pitch"
	
	if (pParams->ContainsComponentNamed(CRCD(0x9e497fc6, "Percent")))
	{
		float minVol = 0.0f;
		float maxVol = 100.0f;
		float minPitch = 100.0f;
		float maxPitch = 100.0f;
		
		pParams->GetFloat(CRCD(0x0693daaf, "MaxVol"), &maxVol);
		pParams->GetFloat(CRCD(0x4391992d, "MinVol"), &minVol);
		pParams->GetFloat(CRCD(0xfa3e14c5, "MaxPitch"), &maxPitch);
		pParams->GetFloat(CRCD(0x1c5ebb24, "MinPitch"), &minPitch);
		
		float percent;
		pParams->GetFloat(CRCD(0x9e497fc6, "Percent"), &percent, Script::ASSERT);
		percent *= (1.0f / 100.0f);
		
		volume = Mth::Lerp(minVol, maxVol, percent);
		pitch = Mth::Lerp(minPitch, maxPitch, percent);
	}
	
	if ( pParams->GetFloat( 0xff2020ec, &dropoffDist ) ) // "dropoff"
	{
		dropoffDist = FEET_TO_INCHES( dropoffDist );
	}
	if ( pParams->GetChecksum( 0xc6ac50a, &dropoffFuncChecksum ) ) // "dropoff_function"
	{
		dropoffFunction = Sfx::GetDropoffFunctionFromChecksum( dropoffFuncChecksum );
	}
	if ( pParams->ContainsFlag( 0xbb39837f ) ) // "NoPosUpdate"
	{
		noPosUpdate = true;
	}
	if (pParams->GetChecksum( 0x8a7132ce, &emitterChecksum ) ) // "emitter"
	{
		mp_emitter = CEmitterManager::sGetEmitter(emitterChecksum);
	}
	else
	{
		mp_emitter = NULL;
	}

	if ( !soundChecksum )
	{
		// If the checksum isn't specified, the designer may be requesting
		// to trigger a sound that can be individually specified per object
		// using the script command Obj_SetSound...
		// If that is the case, there will be a 'type = X' included, where
		// X is an enum such as CARSFX_HONK.
		int type = -1;
		pParams->GetInteger( "Type", &type );
		if ( type == -1 )
		{
			Dbg_MsgAssert( 0,( "Obj_PlaySound requires the name of the sound or a type specified (as in type = CARSFX_HONK)" ));
			return;
		}
		if ( type > MAX_NUM_SOUNDFX_CHECKSUMS )
		{
			Dbg_MsgAssert( 0,( "Bad type (%d) sent to Obj_PlaySound", type ));
			return;
		}
		soundChecksum = mSoundFXChecksums[ type ];
		if ( !soundChecksum )
		{
			Dbg_Message( "Object trying to play sound type %d, which hasn't been defined.", type );
			return;
		}
	}
	
	// network stuff for higher level objects (need to add the update flag to this...)
	BroadcastScriptedSound( soundChecksum, volume, pitch );
	
	PlaySound_VolumeAndPan( soundChecksum, volume, pitch, dropoffDist, dropoffFunction, noPosUpdate );
}// end of PlayScriptedSound( )

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector		CSoundComponent::GetClosestEmitterPos( Gfx::Camera *pCamera )
{
	if (mp_emitter && pCamera)
	{
		return mp_emitter->GetClosestPoint(pCamera->GetPos());
	}
	else
	{
		return m_pos;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector		CSoundComponent::GetClosestOldEmitterPos( Gfx::Camera *pCamera )
{
	if (mp_emitter && pCamera)
	{
		// This doesn't take into consideration the old position
		return mp_emitter->GetClosestPoint(pCamera->GetPos());
	}
	else
	{
		return m_old_pos;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CSoundComponent::GetClosestDropoffPos( Gfx::Camera *pCamera, Mth::Vector & dropoff_pos )
{
	if (pCamera && mp_proxim_node)
	{
		dropoff_pos = mp_proxim_node->GetClosestIntersectionPoint(pCamera->GetPos());
		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ObjectSoundInfo *CSoundComponent::GetLoopingSoundInfo( uint32 soundChecksum )
{
	
	int i;
	for ( i = 0; i < MAX_NUM_LOOPING_SOUNDS_PER_OBJECT; i++ )
	{
		if ( mSoundInfo[ i ].checksum == soundChecksum )
			return &mSoundInfo[ i ];
	}
	return ( NULL );
}

void CSoundComponent::UpdateMutedSounds( void )
{
	
	int i;
	Tmr::Time gameTime;
	gameTime = Tmr::GetTime();

	for ( i = 0; i < MAX_NUM_LOOPING_SOUNDS_PER_OBJECT; i++ )
	{
		if ( mSoundInfo[ i ].checksum )
		{
			ObjectSoundInfo *pInfo = &mSoundInfo[ i ];
			UpdateObjectSound( pInfo );
			if ( pInfo->timeForNextDistCheck <= gameTime )
			{
//				Dbg_Message( "checking if we need to turn %s on", Script::FindChecksumName( mSoundInfo[ i ].checksum ) );
				// see if we're dropoffDist*1.33 from the nearest camera...
				// if so, start the sound up again!
				float dist;
				Gfx::Camera *p_camera = Nx::CViewportManager::sGetClosestCamera( m_pos, &dist );
				if ( p_camera && ( dist < ( pInfo->dropoffDist + DIST_FROM_DROPOFF_AT_WHICH_TO_START_SOUND ) ) )
				{
					// start the sound playing again on this object:
					if ( PlaySound_VolumeAndPan( pInfo->checksum, pInfo->origVolume, pInfo->origPitch, pInfo->dropoffDist, pInfo->dropoffFunction ) )
					{
						//Dbg_Message( "sound %s auto on object %x time %d", Script::FindChecksumName( pInfo->checksum ), ( int ) this, gameTime );
						ObjectSoundInfo *tempInfo;
						Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();
						tempInfo = sfx_manager->GetObjectSoundProperties( this, pInfo->checksum );
						// and set up the targets to match the adjustments the scripts made
						// while the sound was turned off:
						tempInfo->targetVolume = pInfo->targetVolume;
						tempInfo->targetPitch = pInfo->targetPitch;
						tempInfo->deltaPitch = pInfo->deltaPitch;
                        // give it a ramp up in volume, so it doesn't frickin'jump in our faces:
						tempInfo->currentVolume = 0.0f;
						tempInfo->deltaVolume = tempInfo->targetVolume / 100.0f;
						// don't forget to clear the slot to avoid updating it:
						pInfo->checksum = 0;
					}
				}
				else
				{
					// vary the time between distance checks, depending on how far
					// away the object is right now...
					pInfo->timeForNextDistCheck = gameTime;
					pInfo->timeForNextDistCheck += ( int )( ( 1000.0f * ( dist - pInfo->dropoffDist ) ) / 600.0f );
					pInfo->timeForNextDistCheck += 333;  // plus a third of a second...
				}
			}
		}
	}
}

void CSoundComponent::UpdateObjectSound( ObjectSoundInfo *pInfo )
{
	
	// adjust pitch and volume, if necessary:
	if ( pInfo->targetPitch != pInfo->currentPitch )
	{
		pInfo->currentPitch = Mth::FRunFilter( pInfo->targetPitch, pInfo->currentPitch, pInfo->deltaPitch * Tmr::FrameRatio( ) );
	}
	if ( pInfo->targetVolume != pInfo->currentVolume )
	{
		pInfo->currentVolume = Mth::FRunFilter( pInfo->targetVolume, pInfo->currentVolume, pInfo->deltaVolume * Tmr::FrameRatio( )  );
	}
}

void CSoundComponent::AdjustObjectSound( Script::CScriptStructure *pParams, Script::CScript *pScript )
{
	
#ifndef __PLAT_NGC__
	ObjectSoundInfo *pInfo;
	float volumePercent = 100.0f;
	float pitchPercent = 100.0f;
	uint32 soundChecksum;

	if ( !pParams->GetChecksum( NONAME, &soundChecksum ) )
	{
		Dbg_MsgAssert( 0,( "\n%s\nSound checksum required on Obj_AdjustSound", pScript->GetScriptInfo( ) ));
	}
	Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();
	pInfo = sfx_manager->GetObjectSoundProperties( this, soundChecksum );
	if ( !pInfo )
	{
		//Dbg_MsgAssert( 0,( "\n%s\nAttempting to adjust object sound that isn't playing.", pScript->GetScriptInfo( ) ));
		
		// Object went out of range, and the sound was stopped.
		// Store the current properties, so that when the sound
		// is started again the pitch/volume are correct!
		
		// NOTE:  If keeping the pitch/volume updated accurately by
		// adjusting by deltaPitch and deltaVolume causes noticable
		// slowdown, just set the pitch and vol immediately in this
		// function if the pInfo comes from the next line:
		pInfo = GetLoopingSoundInfo( soundChecksum );

#		ifdef __PLAT_XBOX__
		if( !pInfo )
		{
			return;
		}
#		endif

		if (!pInfo)
		{
			//Dbg_MsgAssert( 0,( "\n%s\nCan't adjust a sound (%s) that was never played on an object.",  pScript->GetScriptInfo( ), Script::FindChecksumName(soundChecksum)  ));
			Dbg_Message("\n%s\nCan't adjust a sound (%s) that was never played on an object.",  pScript->GetScriptInfo( ), Script::FindChecksumName(soundChecksum));
			return;
		}
	}

	if ( pParams->GetFloat( 0x330f3415, &volumePercent ) ) // volumePercent
	{
		pInfo->targetVolume = PERCENT( pInfo->origVolume, volumePercent );
		if ( !pParams->GetFloat( 0xdaa9a3c2, &pInfo->deltaVolume ) )// volumeStep
		{
			// adjust immediately:
			pInfo->currentVolume = pInfo->targetVolume;
		}
		else
		{
			Dbg_MsgAssert( pInfo->deltaVolume,( "\n%s\nCan't have zero delta volume.", pScript->GetScriptInfo( ) ));
		}
	}
	if ( pParams->GetFloat( 0xee616c13, &pitchPercent ) )	// pitchPercent
	{
		pInfo->targetPitch = PERCENT( pInfo->origPitch, pitchPercent );
		if ( !pParams->GetFloat( 0x7b090774, &pInfo->deltaPitch ) ) // pitchStep
		{
			// adjust immediately:
			pInfo->currentPitch = pInfo->targetPitch;
		}
		else
		{
			Dbg_MsgAssert( pInfo->deltaPitch,( "\n%s\nCan't have zero delta pitch.", pScript->GetScriptInfo( ) ));
		}
	}
#endif		// __PLAT_NGC__
}

void CSoundComponent::SoundOutOfRange( ObjectSoundInfo *pInfo, Tmr::Time gameTime )
{
	
	int i;
	for ( i = 0; i < MAX_NUM_LOOPING_SOUNDS_PER_OBJECT; i++ )
	{
		if ( !mSoundInfo[ i ].checksum  )
		{
			mSoundInfo[ i ] = *pInfo;
			mSoundInfo[ i ].timeForNextDistCheck = gameTime + 2000;
			return;
		}
	}
	Dbg_MsgAssert( 0,( "No empty looping sound slots for (%s) on object... (Limit %d)",
					Script::FindChecksumName( pInfo->checksum ) ,MAX_NUM_LOOPING_SOUNDS_PER_OBJECT ));
}


// Will play a sound, setting the volume and pan according to world position
// from the closest camera(s).
int CSoundComponent::PlaySound_VolumeAndPan( uint32 soundChecksum, float volume, float pitch, float dropoffDist,
											 EDropoffFunc dropoffFunction, bool noPosUpdate )
{
	
	
	Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();
	Sfx::SoundUpdateInfo soundUpdateInfo;
	soundUpdateInfo.volume = volume;
	soundUpdateInfo.pitch = pitch;
	soundUpdateInfo.dropoffDist = dropoffDist;
	soundUpdateInfo.dropoffFunction = dropoffFunction;
	#if 0
	// TODO:  Replay code handling sound components
	Replay::WritePositionalSoundEffect(m_id,soundChecksum,volume,pitch,dropoffDist);
	#endif
	return ( sfx_manager->PlaySoundWithPos( soundChecksum, &soundUpdateInfo, this, noPosUpdate ) );
} // end of PlaySound_VolumeAndPan( )

	
}
