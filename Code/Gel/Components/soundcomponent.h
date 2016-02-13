//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SoundComponent.h
//* OWNER:          Mick West
//* CREATION DATE:  10/17/2002
//****************************************************************************

#ifndef __COMPONENTS_SOUNDCOMPONENT_H__
#define __COMPONENTS_SOUNDCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/math.h>

#include <gel/object/basecomponent.h>
#include <gel/soundfx/soundfx.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_SOUND CRCD(0x7713c7b,"Sound")
#define		GetSoundComponent() ((Obj::CSoundComponent*)GetComponent(CRC_SOUND))
#define		GetSoundComponentFromObject(pObj) ((Obj::CSoundComponent*)(pObj)->GetComponent(CRC_SOUND))

//#define		GetSounds() ((CSoundComponent*)GetComponent(CRC_SOUND))

#define NUM_GENERAL_SOUNDS_PER_OBJECT	( 10 )


namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	// Forward declarations
	class CEmitterObject;
	class CProximNode;


class CSoundComponent : public CBaseComponent
{
public:
    CSoundComponent();
    virtual         ~CSoundComponent();
    virtual void    Update();
	virtual void    Teleport();
    virtual void    InitFromStructure( Script::CStruct* pParams );
	static CBaseComponent *	s_create();


	CBaseComponent::EMemberFunctionResult CallMemberFunction( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript );

	virtual void GetDebugInfo(Script::CStruct *p_info);
							
// Big old pile of crap....

public:	
	void 			SetSound( Script::CStruct *pParams );
	void			SoundOutOfRange( ObjectSoundInfo *pInfo, Tmr::Time gameTime );
	void			UpdateObjectSound( ObjectSoundInfo *pInfo );
	int 			PlaySound_VolumeAndPan( uint32 soundChecksum, float volume = 100.0f, float pitch = 100.0,
											float dropoffDist = 0.0f, EDropoffFunc dropoffFunction = DROPOFF_FUNC_STANDARD,
											bool noPosUpdate = false );
	virtual void 	BroadcastScriptedSound( uint32 soundChecksum, float volume, float pitch ) { };
	void			PlayScriptedSound( Script::CStruct *pParams );

	Mth::Vector		GetPosition() const { return m_pos; }
	Mth::Vector		GetClosestEmitterPos( Gfx::Camera *pCamera );
	Mth::Vector		GetClosestOldEmitterPos( Gfx::Camera *pCamera );

	bool			HasProximNode() const { return mp_proxim_node != NULL; }
	void			ClearProximNode() { mp_proxim_node = NULL; }								// Needed for early proxim node cleanup
	bool			GetClosestDropoffPos( Gfx::Camera *pCamera, Mth::Vector & dropoff_pos );	// returns true if position defined

	// If the sound is muted due to the object being far from the nearest camera,
	// update the pitch and volume so that when it comes back onto the screen it
	// is how the designer would expect it ( called from GameObj_Update )
	void			UpdateMutedSounds( void );

	#define		MAX_NUM_SOUNDFX_CHECKSUMS	8
	uint32		mSoundFXChecksums[ MAX_NUM_SOUNDFX_CHECKSUMS ];

	void AdjustObjectSound( Script::CStruct *pParams, Script::CScript *pScript );
	ObjectSoundInfo *GetLoopingSoundInfo( uint32 soundChecksum );
	
	#define		MAX_NUM_LOOPING_SOUNDS_PER_OBJECT	2
	ObjectSoundInfo	mSoundInfo[ MAX_NUM_LOOPING_SOUNDS_PER_OBJECT ];
							
protected:
							   
	Mth::Vector	m_pos,m_old_pos;
	CEmitterObject *mp_emitter;
	CProximNode *	mp_proxim_node;

};

}

#endif
