/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL Library												**
**																			**
**	Module:			Sound effects (Sfx)	 									**
**																			**
**	File name:		soundfx.cpp												**
**																			**
**	Created by:		01/10/01	-	dc										**
**																			**
**	Description:	SoundFX for the game. These are the sounds that are		**
**					loaded into memory. Streaming SoundFX are in the Music	**
**					modules (music.cpp and p_music.cpp) which should		**
**					actually be called the Streaming module.				**
**																			**
*****************************************************************************/



/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <gfx/nxviewman.h>
#include <gfx/nxloadscreen.h>
#include <gel/soundfx/soundfx.h>
#include <gel/music/music.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>

#include <modules/frontend/frontend.h>  // until gametimer is moved into lower level stuff..

#include <sys/config/config.h>
#include <sys/replay/replay.h>

#include <gel/components/soundcomponent.h>

#include <sk/modules/skate/skate.h>
#include <sk/objects/moviecam.h>
#include <sk/objects/movingobject.h>

// Used by the script debugger code to fill in a structure
// for transmitting to the monitor.exe utility running on the PC.
void ObjectSoundInfo::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to ObjectSoundInfo::GetDebugInfo"));
	
	p_info->AddChecksum("checksum",checksum);
	p_info->AddFloat("dropoffDist",dropoffDist);
	p_info->AddFloat("currentPitch",currentPitch);
	p_info->AddFloat("currentVolume",currentVolume);
	p_info->AddFloat("origPitch",origPitch);
	p_info->AddFloat("origVolume",origVolume);
	p_info->AddFloat("targetPitch",targetPitch);
	p_info->AddFloat("targetVolume",targetVolume);
	p_info->AddFloat("deltaPitch",deltaPitch);
	p_info->AddFloat("deltaVolume",deltaVolume);
	p_info->AddInteger("timeForNextDistCheck",timeForNextDistCheck);
#endif				 
}

namespace Sfx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool NoSoundPlease( void )
{
#	if NO_SOUND_PLEASE
	return true;
#	else
	// Cannot have sound if building for proview, because there is not enough IOP memory
	// (ProView uses up some IOP memory with it's monitor)
	if( Config::GetHardware() == Config::HARDWARE_PS2_PROVIEW )
	{
		return true;
	}
	return false;
#	endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

EDropoffFunc	GetDropoffFunctionFromChecksum(uint32 checksum)
{
	switch (checksum)
	{
	case 0xef082878: // standard
		return DROPOFF_FUNC_STANDARD;

	case 0xf221474c: // linear
		return DROPOFF_FUNC_LINEAR;

	case 0x9706104b: // exponential
		return DROPOFF_FUNC_EXPONENTIAL;

	case 0xca64c41: // inv_exponential
		return DROPOFF_FUNC_INV_EXPONENTIAL;

	default:
		Dbg_MsgAssert(0, ("Invalid dropoff function type %s (%x)", Script::FindChecksumName(checksum), checksum));
		return DROPOFF_FUNC_STANDARD;
	}
}



DefineSingletonClass( CSfxManager, "Sound FX" );

static float	DefaultDropoffDist = DEFAULT_DROPOFF_DIST;


VoiceInfo		VoiceInfoTable[NUM_VOICES];

// The entries will be permanent entries, followed by temporary entries...
WaveTableEntry	WaveTable[PERM_WAVE_TABLE_MAX_ENTRIES + WAVE_TABLE_MAX_ENTRIES];

// This structure is used on sounds that aren't looping, but are long enough that they need to be updated for
// volume and pan (like the things pedestrians are saying to each other for example).
struct PositionalSoundEntry
{
	int							uniqueID;
	uint32						checksum;
	uint32						flags;
	Obj::CSoundComponent				*pObj;
	struct PositionalSoundEntry	*pNext;
	struct PositionalSoundEntry	*pPrev;
};

#define POSITIONAL_SOUND_FLAG_OCCUPIED	( 1 << 0 )
#define POSITIONAL_SOUND_FLAG_DOPPLER	( 1 << 1 )

static PositionalSoundEntry				*GpPositionalSounds = NULL;
static PositionalSoundEntry				PositionalSounds[MAX_POSITIONAL_SOUNDS];

int										NumWavesInTable		= 0;
int										NumWavesInPermTable	= 0;
int										NumPositionalSounds = 0;
float									gVolume				= 100.0f;



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sVolume::sVolume( void )
{
	// Set the volume type to be the default volume type of the manager.
	Sfx::CSfxManager * p_manager = Sfx::CSfxManager::Instance();
	if( p_manager )
	{
		m_volume_type = p_manager->GetDefaultVolumeType();
	}
	else
	{
		m_volume_type = VOLUME_TYPE_BASIC_2_CHANNEL;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sVolume::sVolume( EVolumeType type )
{
	m_volume_type = type;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sVolume::sVolume( const sVolume& rhs )
{
	m_volume_type = rhs.m_volume_type;
	
	uint32 num_channels = ( m_volume_type == VOLUME_TYPE_5_CHANNEL_DOLBY5_1 ) ? 5 : 2;
	for( uint32 c = 1; c < num_channels; ++c )
	{
		m_channels[c] = rhs.m_channels[c];
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sVolume::~sVolume( void )
{
}


	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sVolume::PercentageAdjustment( float percentage )
{
	uint32 num_channels = ( m_volume_type == VOLUME_TYPE_5_CHANNEL_DOLBY5_1 ) ? 5 : 2;
	for( uint32 c = 0; c < num_channels; ++c )
	{
		m_channels[c] = PERCENT( m_channels[c], percentage );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float sVolume::GetLoudestChannel( void )
{
	float loudest = m_channels[0];
	
	uint32 num_channels = ( m_volume_type == VOLUME_TYPE_5_CHANNEL_DOLBY5_1 ) ? 5 : 2;
	for( uint32 c = 1; c < num_channels; ++c )
	{
		// Allow for phase shifting, which would make a channel negative.
		if( fabsf( m_channels[c] ) > fabsf( loudest ))
			loudest = m_channels[c];
	}
	return loudest;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool sVolume::IsSilent( void )
{
	// All volume types have at least two channels.
	if(( m_channels[0] != 0.0f ) || ( m_channels[1] != 0.0f ))
		return false;
	
	// Check remaining channels for 5.1 type volumes.
	if( m_volume_type == VOLUME_TYPE_5_CHANNEL_DOLBY5_1 )
	{
		if(( m_channels[2] != 0.0f ) || ( m_channels[3] != 0.0f ) || ( m_channels[4] != 0.0f ))
			return false;
	}

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sVolume::SetSilent( void )
{
	// Strictly speaking, we only need to set as many channels silent as the type
	// of the volume requires, but it's probably no less efficient to do it this way.
	memset( m_channels, 0, sizeof( float ) * MAX_CHANNELS );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool sVolume::operator== ( const sVolume& rhs )
{
	if (m_volume_type != rhs.m_volume_type)
	{
		return false;
	}
	
	uint32 num_channels = ( m_volume_type == VOLUME_TYPE_5_CHANNEL_DOLBY5_1 ) ? 5 : 2;
	for( uint32 c = 1; c < num_channels; ++c )
	{
		if (m_channels[c] != rhs.m_channels[c])
		{
			return false;
		}
	}
	
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSfxManager::CSfxManager( void )
{
	if( NoSoundPlease())
		return;

	// Set the default volume type to the most basic available - platform specific code
	// may override this.
	SetDefaultVolumeType( VOLUME_TYPE_BASIC_2_CHANNEL );

	InitSoundFX( this );

	// Clear out the uniqueID table. Only has to be done once (even if module is reset).
	for( int i = 0; i < NUM_VOICES; i++ )
	{
		VoiceInfoTable[i].uniqueID = 0;
		VoiceInfoTable[i].controlID = 0;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSfxManager::~CSfxManager( void )
{
	if( NoSoundPlease())
		return;

	CleanUp();	
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSfxManager::CleanUp( void )
{
	// Call this at the end of a level (or the beginning of the next level before loading sounds, or both).
	if( NoSoundPlease())
		return;

	GpPositionalSounds	= NULL;
	NumPositionalSounds	= 0;
	for( int i = 0; i < MAX_POSITIONAL_SOUNDS; i++ )
	{
		PositionalSounds[i].flags = 0;
	}
	
	// Should stop all sounds and do whatever cleanup necessary so soundfx can be used in the next phase
	// (next level, frontend, whatever).
	CleanUpSoundFX();

	NumWavesInTable		= 0;
	DefaultDropoffDist	= DEFAULT_DROPOFF_DIST;
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

bool CSfxManager::IDAvailable(uint32 id)
{
	return !SoundIsPlaying(id, NULL);
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

int CSfxManager::GetVoiceFromID(uint32 id)
{
	int i;

	// Check unique ID's first
	for( i = 0; i < NUM_VOICES; i++ )
	{
		if ((VoiceInfoTable[i].uniqueID == id) && VoiceIsOn(i))
		{
			return i;
		}
	}

	// Now the control ID's
	for( i = 0; i < NUM_VOICES; i++ )
	{
		if ((VoiceInfoTable[i].controlID == id) && VoiceIsOn(i))
		{
			if( Script::GetInteger( 0x6d2e270e /* DebugSoundFxUpdate */, Script::NO_ASSERT ) )
			{
				Dbg_MsgAssert(0, ("Trying to control duplicate sound %s by checksum", Script::FindChecksumName(id)));
			}
			else
			{
				Dbg_Message("ERROR: Trying to control duplicate sound %s by checksum", Script::FindChecksumName(id));
			}
			return i;
		}
	}

	return -1;
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

uint32 CSfxManager::GenerateUniqueID(uint32 id)
{
	// Keep incrementing ID until one works.
	while (!IDAvailable(id))
		id++;

	return id;
}


/******************************************************************/
/*																  */
/* Different levels might want to have different dropoff		  */
/* distances.													  */
/*                                                                */
/******************************************************************/
void CSfxManager::SetDefaultDropoffDist( float dist )
{
	if( NoSoundPlease())
		return;

	Dbg_MsgAssert( dist > 0.0f, ( "Can't set dropoff dist to 0 or less." ));

	DefaultDropoffDist = dist;
}



/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/
void CSfxManager::StopAllSounds( void )
{
	if( NoSoundPlease())
		return;
	
	StopAllSoundFX();
	GpPositionalSounds	= NULL;
	NumPositionalSounds	= 0;
	for( int i = 0; i < MAX_POSITIONAL_SOUNDS; i++ )
	{
		PositionalSounds[i].flags = 0;
	}
}



/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/
void CSfxManager::PauseSounds( void )
{
	if( NoSoundPlease())
		return;

	PauseSoundsPlease();
}



/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/
WaveTableEntry * CSfxManager::GetWaveTableIndex( uint32 checksum )
{
	if( NoSoundPlease())
		return NULL;

	// Check through list of perm sounds.
	for( int i = 0; i < NumWavesInPermTable; i++ )
	{
		if ( checksum == WaveTable[i].checksum )
			return &WaveTable[i];
	}

	// Check through list of temp sounds.
	for( int i = 0; i < NumWavesInTable; i++ )
	{
		if( checksum == WaveTable[i + PERM_WAVE_TABLE_MAX_ENTRIES].checksum )
			return &WaveTable[i + PERM_WAVE_TABLE_MAX_ENTRIES];
	}

	// Not found.
	return NULL;	
}



/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/
int CSfxManager::MemAvailable( void )
{
	return GetMemAvailable();
}


/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

bool CSfxManager::PositionalSoundSilenceMode()
{
	// Add any conditions here where you want to set the positional sound
	// to 0.

	// Don't update if loading screen is active
	if( Nx::CLoadScreen::sIsActive() )
	{
		return true;
	}

	// Don't update if the game is paused
	if( Mdl::FrontEnd::Instance()->GamePaused() )
	{
		return true;
	}

#if 0	// TT2808: Seemed to cause more problems than it helped.
	// Don't update if the game is playing a CamAnim
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	if ( skate_mod->GetMovieManager()->IsRolling() )
	{
		return true;
	}
#endif

	//Dbg_Message("Updating sounds frame %d", Tmr::GetRenderFrame());
	return false;
}

/******************************************************************/
/*																  */
/* Returns true if sound is still playing.                        */
/* Adjust the pitch for doppler, and the volume and pan for		  */
/* position.													  */
/*																  */
/******************************************************************/
bool CSfxManager::AdjustObjectSound( Obj::CSoundComponent *pObj, VoiceInfo *pVoiceInfo, Tmr::Time gameTime )
{
	Dbg_Assert( pVoiceInfo );

	ObjectSoundInfo *pInfo = &pVoiceInfo->info;
	
	float dist;

	// Get dropoff distance if sent to Obj_PlaySound. If not, get default (set up in LoadSound).
	float dropoffDist = ( pInfo->dropoffDist == 0.0f ) ? DefaultDropoffDist : pInfo->dropoffDist;

	Gfx::Camera *pCamera = Nx::CViewportManager::sGetClosestCamera( pObj->GetPosition(), &dist );
	if( !pCamera )
	{
		Dbg_Message( "Warning: Positional sound, but no camera... Sound won't be updated." );

		// If there is no camera, just don't update the sound but keep it playing the same as before.
		// (If we return false, the object will delete its sound).
		// We might not be able to find the camera at the start of the level or perhaps when changing levels.
		return true;
	}

	Mth::Vector dropoff_pos;
	Mth::Vector *p_dropoff_pos = NULL;
	if (pObj->GetClosestDropoffPos(pCamera, dropoff_pos))
	{
		p_dropoff_pos = &dropoff_pos;
	}
	// See if the object sound could be turned off (far enough away from the camera).
	else if( pInfo->timeForNextDistCheck <= gameTime )
	{
//		Dbg_Message( "checking if we need to turn %s off", Script::FindChecksumName( pInfo->checksum ) );
		if( dist >= ( dropoffDist + DIST_FROM_DROPOFF_AT_WHICH_TO_STOP_SOUND ))
		{
//			Dbg_Message( "sound %s auto off", Script::FindChecksumName( pInfo->checksum ) );

			// If the sound is looping, let the scripts continue to think the
			// sound is still around and keep pitch/volume adjusted for the action.
			if( pVoiceInfo->waveIndex->platformWaveInfo.looping )
			{
				// Turn the sound off, monitor the sound from the object update.
				pObj->SoundOutOfRange( pInfo, gameTime );
			}

			// Should cause this sound to be stopped and voice cleared!
			return false;
		}
		// Still okay, sound still in range. Continue playing for x more milliseconds.
		pInfo->timeForNextDistCheck = gameTime + 2000;
	}
	
	float pitch	= 100.0f;
	sVolume vol;
	vol.SetSilent();

	// If dist is beyond dropoff dist, leave volume at zero.
	if( /*!PositionalSoundSilenceMode() &&*/ ( p_dropoff_pos || (dist < dropoffDist) ) )
	{
		Mth::Vector obj_pos(pObj->GetClosestEmitterPos(pCamera));

		if( pVoiceInfo->waveIndex->flags & SFX_FLAG_POSITIONAL_UPDATE_WITH_DOPPLER )
		{
			// Even if the object didn't move, the camera could have!
			AdjustPitchForDoppler( &pitch, obj_pos, pObj->GetClosestOldEmitterPos(pCamera), Tmr::FrameLength(), pCamera );
		}
		SetVolumeFromPos( &vol, obj_pos, dropoffDist, pInfo->dropoffFunction, pCamera, p_dropoff_pos );
	}
	
	if( !Config::CD())
	{
		if( Script::GetInteger( 0x6d2e270e /* DebugSoundFxUpdate */, Script::NO_ASSERT ) && !vol.IsSilent())
		{
			Dbg_Message( "Updating sound %s with volL = %f volR = %f", Script::FindChecksumName( pInfo->checksum ), vol.GetChannelVolume(0), vol.GetChannelVolume(1));
		}
	}	

	return UpdateLoopingSound( pVoiceInfo->uniqueID, &vol, pitch );
}




/******************************************************************/
/*																  */
/* Only updates one sound for frame (since it's slow to send	  */
/* commands between the EE and the IOP)                           */
/*																  */
/******************************************************************/
void CSfxManager::UpdatePositionalSounds( void )
{
	PositionalSoundEntry	*pEntry		= GpPositionalSounds;
	PositionalSoundEntry	*pTemp;
	Tmr::Time				gameTime	= Tmr::GetTime();

//	static int				active		= 0;	   // Mick: Only update this one this frame.
//	int						count		= 0;	   // Counter for all the sounds.

//	uint32 start_time = Tmr::GetTimeInUSeconds();	
	while ( pEntry )
	{
		pTemp	= pEntry;
		pEntry	= pEntry->pNext;
		Dbg_MsgAssert( pTemp->flags & POSITIONAL_SOUND_FLAG_OCCUPIED, ( "Sounds in the list should have 'occupied' turned on." ));
		
		// This just filters the current volume/pitch towards the target, and should be done every frame per sound:
		Dbg_MsgAssert( pTemp->uniqueID,( "UniqueID 0 means no sound was played.  Shouldn't have gotten this far." ));
	
		int voiceIndex;
		if( !SoundIsPlaying( pTemp->uniqueID, &voiceIndex ))
		{
			// Wasn't a looping sound, and it's done playing.
			RemovePositionalSoundFromList( pTemp, false );
		}
		else
		{
			VoiceInfo *pVoiceInfo = &VoiceInfoTable[voiceIndex];
			pTemp->pObj->UpdateObjectSound( &pVoiceInfo->info );
			
			// Only do the low-down nitty gritty positional stuff on one sound per frame.
			//if(active == count)
			{
				Dbg_MsgAssert( pTemp->pObj,( "Null object pointer in positional sound." ));

#				ifdef __USE_PROFILER__
				//Sys::CPUProfiler->PushContext( 0, 0, 128 );		// blue = this bit of sound code.
#				endif // __USE_PROFILER__
				
				if( !AdjustObjectSound( pTemp->pObj, pVoiceInfo, gameTime ))
				{
					RemovePositionalSoundFromList( pTemp, true );
				}

#				ifdef __USE_PROFILER__
				//Sys::CPUProfiler->PopContext();
#				endif // __USE_PROFILER__
			}
		}
		//count++;
	}

//	uint32 end_time = Tmr::GetTimeInUSeconds();	
//	if ((end_time - start_time) > 250)
//		Dbg_Message("CSfxManager::UpdatePositionalSounds Frame %d Time %d (%x - %x)", Tmr::GetVblanks(), (end_time - start_time), end_time, start_time);

#if 0		// We can update all sounds now
	// Increment the active sound and wrap based on current count.
	active++;

	// Wrap based on current count.
	if( active >= count )
	{
		active = 0;
	}
#endif
}



/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/
void CSfxManager::RemovePositionalSoundFromList( PositionalSoundEntry *pEntry, bool stopIfPlaying )
{
	pEntry->flags = 0;
	
	// Stop the sound from playing if it is playing.
	if( stopIfPlaying && SoundIsPlaying( pEntry->uniqueID ))
	{
		StopSound( pEntry->uniqueID );
	}

	if( pEntry->pNext )
	{
		pEntry->pNext->pPrev = pEntry->pPrev;
	}

	if ( pEntry->pPrev )
	{
		pEntry->pPrev->pNext = pEntry->pNext;
	}
	else
	{
		GpPositionalSounds = pEntry->pNext;
	}
	NumPositionalSounds--;
}



/******************************************************************/
/*																  */
/* Called from the object's destructor.                           */
/* Stops all soundfx and streams associated with the object.	  */
/*																  */
/******************************************************************/
void CSfxManager::ObjectBeingRemoved( Obj::CSoundComponent *pObj )
{
	PositionalSoundEntry *pEntry = GpPositionalSounds;
	PositionalSoundEntry *pTemp;
	while( pEntry )
	{
		pTemp	= pEntry;
		pEntry	= pEntry->pNext;
		
		if( pTemp->pObj == pObj )
		{
			RemovePositionalSoundFromList( pTemp, true );

			// Keep scanning the list, in case there's more than one...
		}
	}

}



/******************************************************************/
/*																  */
/* Stop a sound on an object.									  */
/*																  */
/******************************************************************/
void CSfxManager::StopObjectSound( Obj::CSoundComponent *pObj, uint32 checksum )
{
	PositionalSoundEntry *pEntry = GpPositionalSounds;
	PositionalSoundEntry *pTemp;
	while( pEntry )
	{
		pTemp	= pEntry;
		pEntry	= pEntry->pNext;
		if( pTemp->pObj == pObj )
		{
			if(( !checksum ) || ( pTemp->checksum == checksum ))
			{
				// This will stop the sound as well as removing it from the list.
				RemovePositionalSoundFromList( pTemp, true );

				// Keep scanning the list, in case there's more than one.
			}
		}
	}
}



/******************************************************************/
/*																  */
/*																  */
/******************************************************************/
void CSfxManager::AddPositionalSoundToUpdateList( uint32 uniqueID, uint32 soundChecksum, Obj::CSoundComponent *pObj )
{
	if( !uniqueID )
	{
		// Sound play was unsuccessful.
		return;
	}
	if( NumPositionalSounds == MAX_POSITIONAL_SOUNDS )
	{
		Dbg_Message( "WARNING: Ran out of positional sound slots.\nIncrease the number of slots, or decrease the number of requests." );
		return;
	}
	NumPositionalSounds++;
	int i;
	for( i = 0; i < MAX_POSITIONAL_SOUNDS; i++ )
	{
		PositionalSoundEntry *pEntry = &PositionalSounds[i];

		if( !( pEntry->flags & POSITIONAL_SOUND_FLAG_OCCUPIED ))
		{
			if( GpPositionalSounds )
			{
				GpPositionalSounds->pPrev = pEntry;
			}
			pEntry->pNext = GpPositionalSounds;
			pEntry->pPrev = NULL;
			GpPositionalSounds = pEntry;
			pEntry->uniqueID = uniqueID;
			pEntry->checksum = soundChecksum;
			pEntry->flags = POSITIONAL_SOUND_FLAG_OCCUPIED;
			pEntry->pObj = pObj;
			return;
		}
	}
	Dbg_MsgAssert( 0, ( "This is impossible." ));
}



/******************************************************************/
/*																  */
/* SFX Name should be the name of the sound, no extension or	  */
/* path. It will be the responsibility of the loader for each	  */
/* platform to add the path and append the extension.			  */
/* This module assumes the sounds are all loaded at the beginning */
/* of a level or module, and all removed at the end.			  */
/*																  */
/******************************************************************/
bool CSfxManager::LoadSound( const char *sfxName,  int flags, float dropoff, float pitch, float volume )
{
	if( NoSoundPlease())
		return false;

	const char	*pNameMinusPath	= sfxName;
	int			stringLength	= strlen( sfxName );
	for( int i = 0; i < stringLength; i++ )
	{
		if(( sfxName[i] == '\\' ) || ( sfxName[i] == '/' ))
			pNameMinusPath = &sfxName[i + 1];
	}
	
	uint32			checksum	= Script::GenerateCRC( pNameMinusPath );
	WaveTableEntry	*pWaveEntry	= NULL;
	
	// See if the sound is already loaded.
	if( NULL != GetWaveTableIndex( checksum ))
	{
		if( !Config::CD())
		{
			if( Script::GetInt( 0xd7bb618d /* DebugSoundFx */, false )) 
			{
				Dbg_Message( "Warning: Sound '%s' already loaded (maybe from a different directory)?", sfxName );
			}
		}
		return false;
	}
	
	if( flags & SFX_FLAG_LOAD_PERM )
	{
		if( NumWavesInPermTable >= PERM_WAVE_TABLE_MAX_ENTRIES )
		{
			Dbg_Message( "Increase PERM_WAVE_TABLE_MAX_ENTRIES, no room for %s", sfxName );
			Dbg_MsgAssert( 0,( "Too many permanent sound waves being loaded." ));
			return false;
		}
		pWaveEntry = &WaveTable[ NumWavesInPermTable ];
		if( !LoadSoundPlease( sfxName, checksum, &pWaveEntry->platformWaveInfo, 1 ))
		{
			Dbg_Message( "failed to load sound %s", sfxName );
			return false;
		}
		NumWavesInPermTable++;
	}
	else
	{
		if( NumWavesInTable >= WAVE_TABLE_MAX_ENTRIES )
		{
			Dbg_MsgAssert( 0, ( "Too many sound waves being loaded." ));
			return false;
		}
		pWaveEntry = &WaveTable[ NumWavesInTable + PERM_WAVE_TABLE_MAX_ENTRIES ];
		if( !LoadSoundPlease( sfxName, checksum, &pWaveEntry->platformWaveInfo ))
		{
			return false;
		}
		NumWavesInTable++;
	}

	Dbg_MsgAssert( volume >= 0.0f,( "Tweak volume less than zero." ));
	Dbg_MsgAssert( volume <= MAX_VOL_ALLOWED,( "Tweak volume greater than max allowed." ));	
	Dbg_Assert( pWaveEntry );

	pWaveEntry->dropoff		= dropoff;
	pWaveEntry->pitch		= pitch;
	pWaveEntry->volume		= volume;
	pWaveEntry->checksum	= checksum;
	pWaveEntry->flags		= flags;
	return true;
}



/******************************************************************/
/*																  */
/*																  */
/******************************************************************/
void CSfxManager::TweakVolumeAndPitch( sVolume *p_vol, float *pitch, WaveTableEntry* waveTableIndex )
{
	if( NoSoundPlease())
		return;

	Dbg_Assert( p_vol && pitch );
	
	// 100% volume would result in 1/2 the real MAX. This gives headroom for louder sounds!
	p_vol->PercentageAdjustment( 50.0f );
	
	// Apply tweaked volumes, pitches to sound (variables from LoadSound).
	if( waveTableIndex->volume != 100.0f )
	{
		p_vol->PercentageAdjustment( waveTableIndex->volume );
	}
	if( waveTableIndex->pitch != 100.0f )
	{
		*pitch = PERCENT( *pitch, waveTableIndex->pitch );
	}
}




/******************************************************************/
/*																  */
/* Returns a unique ID each time a sound is played, 0 on failure. */
/* The 'unique ID' is just a number that is incremented each	  */
/* time a sound is played.										  */
/*																  */
/* For each platform, keep a table that stores these unique ID's  */
/* for each voice. PS2 has 48 voices.							  */
/*																  */
/* If somebody calls "StopSound" I check each of the entries in	  */
/* the table for a match, and if a match is found and that voice  */
/* is playing, I stop the voice.								  */
/*																  */
/* If a match is not found, that means the voice stopped playing  */
/* earlier and somebody else may be using the voice now, so that  */
/* voice shouldn't be stopped									  */
/*																  */
/******************************************************************/
uint32 CSfxManager::PlaySound( uint32 checksum, sVolume *p_vol, float pitch, uint32 controlID, SoundUpdateInfo *pUpdateInfo, const char *pSoundName )
{
	if( NoSoundPlease())
		return 0;

	Dbg_Assert( p_vol );
	
	WaveTableEntry *waveTableIndex = GetWaveTableIndex( checksum );
	if( waveTableIndex == NULL )
	{
		//Dbg_MsgAssert( 0,( "Couldn't find sound %s", Script::FindChecksumName( checksum )));
		Dbg_Message("******** NON-FATAL ERROR: Couldn't find sound %s", Script::FindChecksumName( checksum ));
		return 0;
	}

	// Just use checksum if no controlID is set
	if (controlID == 0)
	{
		controlID = checksum;
	}

	if( !Config::CD())
	{
		if( Script::GetInteger( 0xd7bb618d /* DebugSoundFx */, Script::NO_ASSERT ))
		{
			if (!pUpdateInfo && !IDAvailable(controlID))
			{
				Dbg_Message("Playing sound %s with same checksum as one already being played, won't be able to control from script directly.", Script::FindChecksumName(controlID));
			}
		}
	}

	TweakVolumeAndPitch( p_vol, &pitch, waveTableIndex );
	
	// Continue to play the sound, even if the volume is zero. We might be starting a looping sound,
	// so don't second guess this.
	// For the moment, don't record looping sounds, due to problems with running out of voices due to
	// voices still being used by game when it is paused. They just have their volume turned down to zero.
	if( !waveTableIndex->platformWaveInfo.looping )
	{
		// Only record the sound in the replay if the game is not paused, otherwise the menu sounds in the
		// paused menu will get recorded.
		Mdl::FrontEnd* p_frontend = Mdl::FrontEnd::Instance();
		if( !p_frontend->GamePaused())
		{
//			Replay::WritePlaySound( checksum, volL, volR, pitch );
		}	
	}
	
	// In PlaySoundPlease(), clip sounds to the max allowable. If no volume modifiers are listed in the script
	// commands (LoadSound or PlaySound) and the sound is played without attenuation, volL and volR will be 1/2
	// the max (so 50 at this point). Find the platform specific volumes like this:
	// leftVolume = PERCENT( volL, PLATFORM_MAX ). Clip to PLATFORM_MAX (or -PLATFORM_MAX if phasing is supported).
#ifdef __PLAT_NGPS__
	int whichVoice = PlaySoundPlease( &waveTableIndex->platformWaveInfo, p_vol, pitch, waveTableIndex->flags & SFX_FLAG_NO_REVERB);
#else
	int whichVoice = PlaySoundPlease( &waveTableIndex->platformWaveInfo, p_vol, pitch );
#endif

	if( whichVoice == -1 )
	{
		Dbg_Message( "Couldn't find free voice." );
		return 0;
	}
	Dbg_MsgAssert( whichVoice < NUM_VOICES,( "Voice larger than max allowed." ));
	VoiceInfo *pVoiceInfo = &VoiceInfoTable[whichVoice];
	pVoiceInfo->controlID = controlID;
	pVoiceInfo->uniqueID = GenerateUniqueID(controlID);

	// So we don't have to look this up every time we query this module for the properties of a sound that's
	// playing (which we do often for looping sounds).
	pVoiceInfo->waveIndex		= waveTableIndex;
	ObjectSoundInfo *pInfo		= &pVoiceInfo->info;
	if ( pUpdateInfo )
	{
		pInfo->currentPitch		= pUpdateInfo->pitch;
		pInfo->currentVolume	= pUpdateInfo->volume;
		pInfo->dropoffDist		= pUpdateInfo->dropoffDist ? pUpdateInfo->dropoffDist : waveTableIndex->dropoff;
		pInfo->origPitch		= pInfo->targetPitch = pInfo->currentPitch;
		pInfo->origVolume		= pInfo->targetVolume = pInfo->currentVolume;
		pInfo->deltaPitch		= 0.0f;
		pInfo->deltaVolume		= 0.0f;
		pInfo->dropoffFunction	= pUpdateInfo->dropoffFunction;
	}
	else
	{
		pInfo->currentPitch		= 100.0f;
		pInfo->currentVolume	= 100.0f;
		pInfo->dropoffDist		= 0.0f;
		pInfo->dropoffFunction	= DROPOFF_FUNC_STANDARD;
	}
	pInfo->checksum = checksum;
	if( !Config::CD())
	{
		if( Script::GetInteger( 0xd7bb618d /* DebugSoundFx */, Script::NO_ASSERT ))
		{
			//Dbg_Message( "Playing sound %s\n", (pSoundName) ? pSoundName : Script::FindChecksumName( checksum ));
			Dbg_Message( "Playing sound %s with volL = %f volR = %f on voice %d", (pSoundName) ? pSoundName : Script::FindChecksumName( checksum ),
						 p_vol->GetChannelVolume(0), p_vol->GetChannelVolume(1), whichVoice);
		}
	}	
	return pVoiceInfo->uniqueID;
}



/******************************************************************/
/*																  */
/* Send in the uniqueID returned from PlaySound()				  */
/*																  */
/******************************************************************/
bool CSfxManager::StopSound( uint32 uniqueID )
{
	if( NoSoundPlease())
		return true;

	int whichVoice = GetVoiceFromID(uniqueID);

	if (whichVoice >= 0)
	{
		// Stop the sound if it's playing (platform specific).
		StopSoundPlease( whichVoice );
		return true;
	}

	return false;
}


bool CSfxManager::SetSoundParams( uint32 uniqueID, sVolume *p_vol, float pitch )
{
	if( NoSoundPlease())
		return true;

	int whichVoice = GetVoiceFromID(uniqueID);

	if (whichVoice >= 0)
	{
		WaveTableEntry *waveTableIndex = VoiceInfoTable[ whichVoice ].waveIndex;
	
		if( waveTableIndex == NULL )
		{
			Dbg_MsgAssert(0, ( "Can't find wave table entry for sound %s", Script::FindChecksumName( uniqueID ) ) );
			return false;
		}
		if( !p_vol->IsSilent())
		{
			// Tweak the values to designer specs (from LoadSound and PlaySound).
			TweakVolumeAndPitch( p_vol, &pitch, waveTableIndex );
		}

#		if defined( __PLAT_NGPS__ ) || defined( __PLAT_NGC__ )
		// Adjust pitch to account for lower sampling rates.
		pitch = PERCENT( pitch, waveTableIndex->platformWaveInfo.pitchAdjustmentForSamplingRate );
#		endif

		// And finally, set the parameters.
		SetVoiceParameters( whichVoice, p_vol, pitch );

		return true;
	}

	if( !Config::CD())
	{
		if( Script::GetInteger( 0x6d2e270e /* DebugSoundFxUpdate */, Script::NO_ASSERT ))
		{
			Dbg_Message( "SetSoundParams: couldn't find sound %s", Script::FindChecksumName( uniqueID ));
		}
	}

	return false;
}


/******************************************************************/
/*																  */
/*																  */
/******************************************************************/
void CSfxManager::SetMainVolume( float volume )
{
	if( NoSoundPlease())
		return;
	
	Dbg_MsgAssert( volume >= 0.0f, ( "Volume below 0%" ));
	Dbg_MsgAssert( volume <= 100.0f, ( "Volume above 100%" ));
	
	if( Script::GetInteger( 0xd7bb618d /* DebugSoundFx */, Script::NO_ASSERT ))
	{
		Dbg_Message( "Setting gVolume to %f\n",volume );
	}
	  
	gVolume	= volume;
	volume	= volume * Config::GetMasterVolume() / 100.0f;
	if( volume < 0.1f )
	{
		volume = 0.0f;
	}	
	SetVolumePlease( volume );
	Pcm::SetAmbientVolume( volume );
}



/******************************************************************/
/*																  */
/*																  */
/******************************************************************/
float CSfxManager::GetMainVolume( void )
{
	return gVolume;
}



/******************************************************************/
/*																  */
/*																  */
/******************************************************************/
void CSfxManager::SetReverb( float reverbLevel, int reverbMode, bool instant )
{
	if( NoSoundPlease())
		return;

	Dbg_MsgAssert( reverbLevel >= 0.0f, ( "Reverb below 0%" ));
	Dbg_MsgAssert( reverbLevel <= 100.0f, ( "Reverb above 100%" ));
	SetReverbPlease( reverbLevel, reverbMode, instant );
}



/******************************************************************/
/*																  */
/*																  */
/******************************************************************/
bool CSfxManager::SoundIsPlaying( uint32 uniqueID, int *pWhichVoice )
{
	if( NoSoundPlease())
		return 0;

	int whichVoice;

	// Zero is not a valid ID... quickly return.
	if ( !uniqueID )
		return false;
		
	for( whichVoice = 0; whichVoice < NUM_VOICES; whichVoice++ )
	{
		if ( VoiceInfoTable[ whichVoice ].uniqueID == uniqueID )
		{
			if( !VoiceIsOn( whichVoice ))
			{
				return false;
			}
			if( pWhichVoice )
			{
				*pWhichVoice = whichVoice;
			}
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*																  */
/*																  */
/******************************************************************/

int CSfxManager::GetNumSoundsPlaying()
{
	// This is not the most efficient way to figure this out, but it doesn't
	// require any changes below the p-line.  We should add p-line code if
	// it get used a lot.
	int voicesUsed = 0;

	for( int whichVoice = 0; whichVoice < NUM_VOICES; whichVoice++ )
	{
		if( VoiceIsOn( whichVoice ) )
		{
			voicesUsed++;
		}
	}

	return voicesUsed;
}

/******************************************************************/
/*																  */
/*																  */
/******************************************************************/
float CSfxManager::GetDropoffDist( uint32 soundChecksum )
{
	if( NoSoundPlease())
		return DefaultDropoffDist;

	WaveTableEntry *waveIndex = GetWaveTableIndex( soundChecksum );
	if( waveIndex == NULL )
	{
		return 0;
	}
	float dropoffDist = waveIndex->dropoff;
	return ( dropoffDist ? dropoffDist : DefaultDropoffDist );
}



/******************************************************************/
/*																  */
/*																  */
/******************************************************************/
bool CSfxManager::UpdateLoopingSound( uint32 soundID, sVolume *p_vol, float pitch )
{
	if( NoSoundPlease())
		return true;

//    uint32 start_time = Tmr::GetTimeInUSeconds();

	int whichVoice;
	for( whichVoice = 0; whichVoice < NUM_VOICES; whichVoice++ )
	{
		if( VoiceInfoTable[whichVoice].uniqueID == soundID )
		{
			if( !VoiceIsOn( whichVoice ))
			{
				if( !Config::CD())
				{
					if( Script::GetInteger( 0x6d2e270e /* DebugSoundFxUpdate */, Script::NO_ASSERT ))
					{
						Dbg_Message( "Sound %s is done", Script::FindChecksumName( VoiceInfoTable[whichVoice].info.checksum ));
					}
				}	

				return false;
			}
			
			// Tweak sounds for this particular instance of PlaySound (this allows a designer to vary the pitch,
			// for example, of a carloop sound on each car, by adding a pitch = parameter to Obj_PlaySound. This
			// will be maintained, as will the volume, thanks to this code.
			if( !p_vol->IsSilent())
			{
				float tweakVolume = VoiceInfoTable[whichVoice].info.currentVolume;
				if( tweakVolume != 100.0f )
				{
					p_vol->PercentageAdjustment( tweakVolume );
				}
				float tweakPitch = VoiceInfoTable[ whichVoice ].info.currentPitch;
				if( tweakPitch != 100.0f )
				{
					pitch = PERCENT( pitch, tweakPitch );
				}
			}
			
			WaveTableEntry *waveTableIndex = VoiceInfoTable[ whichVoice ].waveIndex;
		
			if( waveTableIndex == NULL )
			{
				Dbg_Message( "WARNING:  Positional/looping sound error." );
				return false;
			}
			if( !p_vol->IsSilent())
			{
				// Tweak the values to designer specs (from LoadSound and PlaySound).
				TweakVolumeAndPitch( p_vol, &pitch, waveTableIndex );
			}

#			if defined( __PLAT_NGPS__ ) || defined( __PLAT_NGC__ )
			// Adjust pitch to account for lower sampling rates.
			pitch = PERCENT( pitch, waveTableIndex->platformWaveInfo.pitchAdjustmentForSamplingRate );
#			endif

			// And finally, set the parameters.
			SetVoiceParameters( whichVoice, p_vol, pitch );

//            uint32 end_time = Tmr::GetTimeInUSeconds();	
//			if ((end_time - start_time) > 250)
//				Dbg_Message("CSfxManager::UpdateLoopingSound Frame %d Time %d", Tmr::GetVblanks(), (end_time - start_time));
			return true;
		}
	}

	if( !Config::CD())
	{
		if( Script::GetInteger( 0x6d2e270e /* DebugSoundFxUpdate */, Script::NO_ASSERT ))
		{
			Dbg_Message( "UpdateSound: couldn't find sound %s", Script::FindChecksumName( VoiceInfoTable[whichVoice].info.checksum ));
		}
	}	
	return false;
}



/******************************************************************/
/*																  */
/* Calculates multipliers for front left, front right, rear left, */
/* rear right and center channel speakers.						  */
/*																  */
/******************************************************************/
void CSfxManager::Get5ChannelMultipliers( const Mth::Vector &sound_source, float *p_multipliers )
{
	float angle	= Mth::RadToDeg( atan2f( sound_source[X], -sound_source[Z] ));
	Get5ChannelMultipliers( angle, p_multipliers );
}



/******************************************************************/
/*																  */
/* Calculates multipliers for front left, front right, rear left, */
/* rear right and center channel speakers.						  */
/*																  */
/******************************************************************/
void CSfxManager::Get5ChannelMultipliers( float angle, float *p_multipliers )
{
	static float speakers[5][3]	= {	{ 330.00f, 240.01f, 359.99f },		// Front left max angle, min angle0, min angle1
									{  30.00f,   0.01f,	119.99f },		// Front right
									{ 240.00f, 120.01f, 329.99f },		// Rear left
									{ 120.0f,   30.01f, 239.99f },		// Rear right
									{   0.0f,  330.01f,  29.99f }};		// Center

	// Ensure angle is in a suitable range.
	angle = ( angle < 0.0f ) ? ( 360.0f + angle ) : angle;

	// Go through and calculate the relative volumes for each speaker.
	for( int spkr = 0; spkr < 5; ++spkr )
	{
		float amin0	= speakers[spkr][1];
		float amin1	= speakers[spkr][2];
		float amax	= speakers[spkr][0];
		float mul	= 0.0f;

		if( amin0 < amax )
		{
			// Regular test.
			if(( angle > amin0 ) && ( angle <= amax ))
			{
				// Angle lies between amin0 and amax.
				mul = ( angle - amin0 ) / ( amax - amin0 );
			}
			else if(( angle > amax ) && ( angle < amin1 ))
			{
				// Angle lies between amax and amin1.
				mul = 1.0f - (( angle - amax ) / ( amin1 - amax ));
			}
		}
		else
		{
			// Non regular test (center channel). Assumes center at 0.0.
			if( angle > amin0 )
			{
				mul = ( angle - amin0 ) / ( 360.0f - amin0 );
			}
			else if( angle < amin1 )
			{
				mul = ( amin1 - angle ) / ( amin1 );
			}
		}
						
		// Angle is within scope of this speaker. Figure multiplier.
		Dbg_Assert( mul <= 1.0f );
		p_multipliers[spkr] = sinf( mul * Mth::PI * 0.5f );
	}
}



/******************************************************************/
/*																  */
/* Sets volume and pan considering all cameras and current		  */
/* position.													  */
/*																  */
/******************************************************************/
void CSfxManager::SetVolumeFromPos( sVolume *p_vol, const Mth::Vector &soundSource, float dropoffDist, EDropoffFunc dropoff_func,
									Gfx::Camera *p_camera,  const Mth::Vector *p_dropoff_pos)
{
	if( NoSoundPlease())
		return;

	Dbg_Assert( p_vol );

	// Set the volume to 0
	p_vol->SetSilent();

	// Return if in Silence Mode
	if( PositionalSoundSilenceMode() )
		return;

	// Find the camera if one wasn't already supplied
	if (!p_camera)
	{
		p_camera = Nx::CViewportManager::sGetClosestCamera(soundSource);

		// If we still can't find a camera, return with the volume set to 0
		if (!p_camera)
		{
			return;
		}
	}

	// Find the dist to the sound
	float dist = Mth::Distance( p_camera->GetPos(), soundSource );

	// Calculate the dropoff dist
	if (p_dropoff_pos)
	{
		dropoffDist = dist + Mth::Distance( *p_dropoff_pos, p_camera->GetPos());
	}
	else if( !dropoffDist )
	{
		dropoffDist = DefaultDropoffDist;
	}
	
	// If we are outside the dropoff dist, we are out of here
	if( dist >= dropoffDist )
		return;

	// Sound is within range of this camera.
	float dropOff	= dist / dropoffDist;
	float volume = 0.0f;
	switch (dropoff_func)
	{
	case DROPOFF_FUNC_STANDARD:
		volume	= ( 100.0f * 
				  ((( 1.0f - ( dropOff * dropOff )) + 			// Exponential and...
				  ( 3.0f * ( 1.0f - ( dropOff )))) / 4.0f ));	// ...linear averaged together.
		break;

	case DROPOFF_FUNC_LINEAR:
		volume	= ( 100.0f * ( 1.0f - dropOff ) );
		break;

	case DROPOFF_FUNC_EXPONENTIAL:
		volume	= ( 100.0f * ( ( 1.0f - dropOff ) * ( 1.0f - dropOff ) ) );
		break;

	case DROPOFF_FUNC_INV_EXPONENTIAL:
		volume	= ( 100.0f * ( sqrtf ( 1.0f - dropOff ) ) );
		break;
	}

	switch( p_vol->GetVolumeType() )
	{
		case VOLUME_TYPE_BASIC_2_CHANNEL:
		case VOLUME_TYPE_2_CHANNEL_DOLBYII:
			{
				Dbg_Assert( p_camera );		// We should have returned by now

				if( fabsf( volume ) > fabsf( p_vol->GetLoudestChannel()))
				{
					Mth::Vector sound_pos_from_camera = soundSource - p_camera->GetPos();
					sound_pos_from_camera.Normalize();

					Mth::Vector camRightVector = -p_camera->GetMatrix()[X];
					
					// Project the obj_pos vector onto the right vector.
					// For some reason right and left were switched (must be left instead of right!)
					float panVal = Mth::DotProduct( sound_pos_from_camera, camRightVector );

					// +1.0f is all the way on the right, -1.0f is all the way on the left.
					// Doing an exponential curve, so that if the object is right in the middle
					// the right and left volume (at closest dist) is 75%. 100% volume if all
					// the way to the right/left.
					panVal		+= 1.0f;		// Will now be from 0 to 2 (Lmax to Rmax)..
					panVal		/= 2.0f;		// ...now from 0 to 1.
					float rVol	= ( 1.0f - ( panVal * panVal )) * volume;
					panVal		= 1.0f - panVal;
					float lVol	= ( 1.0f - ( panVal * panVal )) * volume;

					if( p_vol->GetVolumeType() == VOLUME_TYPE_2_CHANNEL_DOLBYII )
					{
						bool bSetPhase = false;

						if( lVol > p_vol->GetChannelVolume( 0 ))
						{
							p_vol->SetChannelVolume( 0, lVol );
							bSetPhase = true;
						}
						if( rVol > p_vol->GetChannelVolume( 1 ))
						{
							p_vol->SetChannelVolume( 1, rVol );
							bSetPhase = true;
						}
						if( bSetPhase )
						{
							// If sound is behind the camera, set volume negative and it will sound out of phase.
							Mth::Vector camAtVector = -p_camera->GetMatrix()[Z];

							float behindCamera = Mth::DotProduct( sound_pos_from_camera, camAtVector );
							if ( behindCamera < 0.0f )
							{
								// Just one channel needs to be reverse phased to get the effect.
								p_vol->SetChannelVolume( 0, p_vol->GetChannelVolume( 0 ) * -1.0f );
							}
						}
					}
					else
					{
						if( lVol > p_vol->GetChannelVolume( 0 ))
						{
							p_vol->SetChannelVolume( 0, lVol );
						}
						if( rVol > p_vol->GetChannelVolume( 1 ))
						{
							p_vol->SetChannelVolume( 1, rVol );
						}
					}
				}
			}
			break;
	
		case VOLUME_TYPE_5_CHANNEL_DOLBY5_1:
			{
				Dbg_Assert( p_camera );		// We should have returned by now

				if( fabsf( volume ) > p_vol->GetLoudestChannel())
				{
					// Transform the sound source into the camera's coordinate space.
					Mth::Matrix inv_view	= p_camera->GetMatrix();
					inv_view.Invert();

					Mth::Vector sound_src	= soundSource - p_camera->GetPos();
					Mth::Vector sound_pos	= inv_view.Transform( sound_src );
					sound_pos.Normalize();

					float channel_multipliers[5];
                    Get5ChannelMultipliers( sound_pos, &channel_multipliers[0] );

					// Go through and calculate the relative volumes for each speaker.
					for( int spkr = 0; spkr < 5; ++spkr )
					{
						float mul = channel_multipliers[spkr];
						if( mul > 0.0f )
						{
							// Angle is within scope of this speaker. Figure multiplier.
							Dbg_Assert( mul <= 1.0f );
							float vol = volume * mul;
							if( vol > p_vol->GetChannelVolume( spkr ))
							{
								p_vol->SetChannelVolume( spkr, vol );
							}
						}
					}

					// Final step is to normalize the channels out and reset to volume level.
					if( !p_vol->IsSilent())
					{
						float norm = 0.0;
						for( int spkr = 0; spkr < 5; ++spkr )
						{
							norm += p_vol->GetChannelVolume( spkr ) * p_vol->GetChannelVolume( spkr );
						}
						norm = sqrtf( norm );
						for( int spkr = 0; spkr < 5; ++spkr )
						{
							p_vol->SetChannelVolume( spkr, ( volume * p_vol->GetChannelVolume( spkr )) / norm );
						}
					}

				}
			}
			break;
		
		default:
			break;
	}
}



/* Real speed of sound ~= 760 mph.								  */
/* Lower that for dramatic effect.								  */
#define	SPEED_OF_OUR_SOUND	MPH_TO_INCHES_PER_SECOND( 700.0f )

/******************************************************************/
/*																  */
/* This should happen after all other pitch adjustments.		  */
/*																  */
/******************************************************************/
void CSfxManager::AdjustPitchForDoppler( float *pitch, const Mth::Vector &currentPos, const Mth::Vector &oldPos, float elapsedTime, Gfx::Camera *pCam )
{
#	ifndef __PLAT_NGC__
	const float cutoff_dist =  360.0f;	// in inches
#	endif		// __PLAT_NGC__

	if( NoSoundPlease())
		return;

#	ifndef __PLAT_NGC__
	if ( !pCam )
	{
		pCam = Nx::CViewportManager::sGetClosestCamera( currentPos );
		if( !pCam )
			return;
	}

	float prevDist	= Mth::Distance( pCam->m_old_pos, oldPos );
	float deltaDist	= Mth::Distance( pCam->GetPos(), currentPos ) - prevDist;

	if(( fabsf( deltaDist ) * Tmr::FrameRatio()) > Tmr::FrameRatio() * cutoff_dist )
	{
		// TT2794: Large movements in the camera are causing pitch glitches, so
		// it would be better to not change the pitch at all than to try to find
		// a "good" change.  Garrett
		return;

		// Clip so there aren't high pitched schreeches when camera is moved in net games and such:
		if ( deltaDist < 0.0f )
		{
			deltaDist = -( Tmr::FrameRatio( ) * cutoff_dist );
		}
		else
		{
			deltaDist = Tmr::FrameRatio( ) * cutoff_dist;
		}
	}
	float deltaPitch = SPEED_OF_OUR_SOUND * elapsedTime;
	Dbg_MsgAssert( deltaPitch,( "Divide by zero." ));
	deltaPitch = (( *pitch ) * deltaDist ) / deltaPitch;
	*pitch -= deltaPitch;
#	endif // __PLAT_NGC__
}



/******************************************************************/
/*																  */
/* This should happen after all other pitch adjustments.		  */
/*																  */
/******************************************************************/
ObjectSoundInfo *CSfxManager::GetObjectSoundProperties( Obj::CSoundComponent *pObj, uint32 checksum )
{
	PositionalSoundEntry *pEntry = GpPositionalSounds;
	while( pEntry )
	{
		if(( pEntry->pObj == pObj ) && ( pEntry->checksum == checksum ))
		{
			// Change the current volume, pitch.
			int voiceIndex;
			if( !SoundIsPlaying( pEntry->uniqueID, &voiceIndex ))
			{
				return NULL;
			}
			return &VoiceInfoTable[ voiceIndex ].info;
		}
		pEntry = pEntry->pNext;
	}
	return NULL;
}



/******************************************************************/
/*																  */
/* Plays sound, considering camera position(s) from sound source. */
/*																  */
/******************************************************************/
uint32 CSfxManager::PlaySoundWithPos( uint32 soundChecksum, SoundUpdateInfo *pUpdateInfo, Obj::CSoundComponent *pObj, bool noPosUpdate )
{
	if( NoSoundPlease())
		return 0;

	WaveTableEntry *waveTableIndex = GetWaveTableIndex( soundChecksum );
	if( waveTableIndex == NULL )
	{
		Dbg_MsgAssert( 0, ( "Asking to play sound that hasn't been loaded %s.", Script::FindChecksumName( soundChecksum )));
		return 0;
	}
	
	Dbg_MsgAssert( pObj,( "pObj should be non-NULL" ));

	sVolume vol;

	// Garrett: Shouldn't we be calling this here, too?  Or is it not necessary?
//	if( pVoiceInfo->waveIndex->flags & SFX_FLAG_POSITIONAL_UPDATE_WITH_DOPPLER )
//	{
//		// Even if the object didn't move, the camera could have!
//		AdjustPitchForDoppler( &pitch, pObj->m_pos, pObj->m_old_pos, Tmr::FrameLength(), pCamera );
//	}

	Gfx::Camera *pCamera = Nx::CViewportManager::sGetClosestCamera( pObj->GetPosition() );
	if( pCamera )
	{
		Mth::Vector dropoff_pos;
		Mth::Vector *p_dropoff_pos = NULL;
		if (pObj->GetClosestDropoffPos(pCamera, dropoff_pos))
		{
			p_dropoff_pos = &dropoff_pos;
		}

		SetVolumeFromPos( &vol, pObj->GetClosestEmitterPos(pCamera), pUpdateInfo->dropoffDist ? pUpdateInfo->dropoffDist : waveTableIndex->dropoff,
						  pUpdateInfo->dropoffFunction, pCamera, p_dropoff_pos );
	}
	else
	{
		vol.SetSilent();
	}
	
	Dbg_MsgAssert(!(noPosUpdate && ( waveTableIndex->flags & SFX_FLAG_POSITIONAL_UPDATE_WITH_DOPPLER )), ("Trying to play doppler sound with NoPosUpdate"));
	if( vol.IsSilent() && noPosUpdate )
	{
		return 0;
	}
	
	vol.PercentageAdjustment( pUpdateInfo->volume );
	
	uint32 uniqueID = PlaySound( soundChecksum, &vol, pUpdateInfo->pitch, 0, pUpdateInfo );
	
	if( !Config::CD())
	{
		if( Script::GetInteger( 0x6d2e270e /* DebugSoundFxUpdate */, Script::NO_ASSERT ))
		{
			Dbg_Message( "Starting sound %s with volL = %f volR = %f", Script::FindChecksumName( soundChecksum ), vol.GetChannelVolume(0), vol.GetChannelVolume(1));
		}
	}	

	// Should be able to have looping sounds, as long as the ones that are manually updated call PlaySound
	// instead of PlaySoundWithPos.
	if(!noPosUpdate)
	{
		AddPositionalSoundToUpdateList( uniqueID, soundChecksum, pObj );

		if( !Config::CD())
		{
			if( Script::GetInteger( 0x6d2e270e /* DebugSoundFxUpdate */, Script::NO_ASSERT ))
			{
				Dbg_Message( "Added sound %s to update list", Script::FindChecksumName( soundChecksum ));
			}
		}	
	}
	return uniqueID;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSfxManager::Update( void )
{
	PerFrameUpdate();
}

} // namespace Sfx

