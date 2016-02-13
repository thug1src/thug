/*
	This is just a database for arrays of skater sounds on different terrains.
	
	For each type of sound in sounds.h (like grind, land, slide, wheel roll, etc...)
	there is an array of sounds to play, one for each possible terrain type.
	
	A terrain type of zero indicates that the sound is the default (for surfaces not
	defined in the list).
*/


#include <string.h>
#include <core/defines.h>
#include <core/singleton.h>

#include <gel/soundfx/soundfx.h>
#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>
#include <gel/scripting/checksum.h>
						   
#include <engine/sounds.h>
#include <sk/gamenet/gamenet.h>
#include <sys/config/config.h>
#include <sys/replay/replay.h>



namespace Sk3Sfx
{



DefineSingletonClass( CSk3SfxManager, "Sk3 Sound FX" );

CSk3SfxManager::CSk3SfxManager( void )
{
	
	Reset( );
}

CSk3SfxManager::~CSk3SfxManager( void )
{
	
}

void CSk3SfxManager::Reset( void )
{
	
	int i;
	for ( i = 0; i < vNUM_SOUND_TYPES; i++ )
	{
		mNumEntries[ i ] = 0;
	}
} // end of Reset( )

// Sound FX for the level...
/*	The surface flag indicates which surface the skater is currently on (grass, cement, wood, metal)
	whichSound is the checksum from the name of the looping sound (should be loaded using LoadSound
		in the script file for each level)
	whichArray indicates whether this sound belongs in the list of wheels rolling sounds, or
		grinding sounds, etc...
*/
void CSk3SfxManager::SetSkaterSoundInfo( int surfaceFlag, uint32 whichSound, int whichArray,
	float maxPitch, float minPitch, float maxVol, float minVol )
{
	
	if (Sfx::NoSoundPlease()) return;
	
	// must initialize PInfo!
	int i;
	Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();

	if ( NULL == sfx_manager->GetWaveTableIndex( whichSound ) )
	{
		Dbg_MsgAssert( 0,( "Terrain sound not loaded! surface %d sound %s checksum %d soundType %d",
			surfaceFlag, Script::FindChecksumName( whichSound ), whichSound, whichArray ));
		return;
	}
	int numEntries = mNumEntries[ whichArray ];
	SkaterSoundInfo	*pArray = mSoundArray[ whichArray ];
	SkaterSoundInfo *pInfo = NULL;
	
	for ( i = 0; i < numEntries; i++ )
	{
		if ( pArray[ i ].surfaceFlag == surfaceFlag )
		{
			Dbg_Message( "Re-defining soundtype %d for surfaceFlag %d", whichArray, surfaceFlag );
			pInfo = &pArray[ i ];
			break;
		}
	}
	if ( !pInfo )
	{
		pInfo = &pArray[ mNumEntries[ whichArray ] ];
		mNumEntries[ whichArray ] += 1;
		Dbg_MsgAssert( mNumEntries[ whichArray ] < vMAX_NUM_ENTRIES,( "Array too small type %d.  Increase MAX_NUM_ENTRIES.", whichArray ));
	}
	
	Dbg_MsgAssert( pInfo,( "Please fire Matt immediately after kicking him in the nuts." ));
	
	// surfaceFlag of zero will be used for the default
	pInfo->surfaceFlag = surfaceFlag;
	// if soundChecksum is zero, no sound will play on this surface.
	pInfo->soundChecksum = whichSound;
	pInfo->maxPitch = maxPitch;
	pInfo->minPitch = minPitch;
	pInfo->maxVol = maxVol;
	pInfo->minVol = minVol;

} // end of SetSkaterSoundInfo( )

SkaterSoundInfo	*CSk3SfxManager::GetSkaterSoundInfo( int surfaceFlag, int whichArray )
{
	
	if (Sfx::NoSoundPlease()) return NULL;
	
	
	int numEntries = 0;
	numEntries = mNumEntries[ whichArray ];
	SkaterSoundInfo	*pArray = mSoundArray[ whichArray ];
	Dbg_MsgAssert( pArray,( "Fire Matt please." ));
	int i;
	for ( i = 0; i < numEntries; i++ )
	{
		if ( pArray[ i ].surfaceFlag == surfaceFlag )
		{
			return ( &pArray[ i ] );
		}
	}
	// couldn't find the surface flag in the table... return the default:
	for ( i = 0; i < numEntries; i++ )
	{
		if ( !pArray[ i ].surfaceFlag )
		{
			return ( &pArray[ i ] );
		}
	}

	return ( NULL );
} // end of GetSkaterSoundInfo( )

void CSk3SfxManager::PlaySound( int whichArray, int surfaceFlag, const Mth::Vector &pos, float volPercent,
								bool propogate )
{
	if (Sfx::NoSoundPlease()) return;
	

	Replay::WriteSkaterSoundEffect(whichArray,surfaceFlag,pos,volPercent);
	
	Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();
	GameNet::Manager * gamenet_manager = GameNet::Manager::Instance();
    
	SkaterSoundInfo	*pInfo = GetSkaterSoundInfo( surfaceFlag, whichArray );
	if ( !pInfo )
	{
		// no sounds are supposed to be played for this surface on this transition:
		return;
	}

	if( propogate )
	{
		Net::Client* client;
		GameNet::MsgPlaySound msg;
		Net::MsgDesc msg_desc;

		client = gamenet_manager->GetClient( 0 );
		Dbg_Assert( client );

		msg.m_WhichArray = (char) whichArray;
		msg.m_SurfaceFlag = (char) surfaceFlag;
		msg.m_Pos[0] = (short) pos[X];
		msg.m_Pos[1] = (short) pos[Y];
		msg.m_Pos[2] = (short) pos[Z];
		msg.m_VolPercent = (char) volPercent;

		msg_desc.m_Data = &msg;
		msg_desc.m_Length = sizeof( GameNet::MsgPlaySound );
		msg_desc.m_Id = GameNet::MSG_ID_PLAY_SOUND;
		client->EnqueueMessageToServer( &msg_desc );
	}

//	float volL, volR;
	Sfx::sVolume vol;
//	sfx_manager->SetVolumeFromPos( &volL, &volR, pos, sfx_manager->GetDropoffDist( pInfo->soundChecksum ) );
	sfx_manager->SetVolumeFromPos( &vol, pos, sfx_manager->GetDropoffDist( pInfo->soundChecksum ));

	// Adjust volume according to speed.
	volPercent = GetVolPercent( pInfo, volPercent );
//	volL = PERCENT( volL, volPercent );
//	volR = PERCENT( volR, volPercent );
	vol.PercentageAdjustment( volPercent );

//	sfx_manager->PlaySound( pInfo->soundChecksum, volL, volR );
	sfx_manager->PlaySound( pInfo->soundChecksum, &vol );
}

// set the volume according to the range specified by the designers...
float CSk3SfxManager::GetVolPercent( SkaterSoundInfo *pInfo, float volPercent, bool clipToMaxVol )
{
	
	Dbg_MsgAssert( pInfo,(( "Fire whoever called this function with this nonsense." )));
	if ( !( ( pInfo->minVol == 0.0f ) && ( pInfo->maxVol == 100.0f ) ) )
	{
		volPercent = ( pInfo->minVol + PERCENT( ( pInfo->maxVol - pInfo->minVol ), volPercent ) );
	}
	
	if ( clipToMaxVol )
	{
		if ( volPercent > pInfo->maxVol )
			volPercent = pInfo->maxVol;
	}
	return ( volPercent );
}

} // namespace sfx
