//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       StreamComponent.h
//* OWNER:          ???
//* CREATION DATE:  ??/??/??
//****************************************************************************

#ifndef __COMPONENTS_STREAMCOMPONENT_H__
#define __COMPONENTS_STREAMCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#define		CRC_STREAM CRCD(0xf1641e3,"stream")
#define		GetStreamComponent() ((Obj::CStreamComponent*)GetComponent(CRC_STREAM))
#define		GetStreamComponentFromObject(pObj) ((Obj::CStreamComponent*)(pObj)->GetComponent(CRC_STREAM))

#define MAX_STREAMS_PER_OBJECT	3


namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Gfx
{
	class	Camera;
}

namespace Obj
{
	// Forward declarations
	class CEmitterObject;
	class CProximNode;

class CStreamComponent : public CBaseComponent
{
public:
    CStreamComponent();
    virtual ~CStreamComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	Mth::Vector						GetClosestEmitterPos( Gfx::Camera *pCamera );

	bool							HasProximNode() const { return mp_proxim_node != NULL; }
	void							ClearProximNode() { mp_proxim_node = NULL; }								// Needed for early proxim node cleanup
	bool							GetClosestDropoffPos( Gfx::Camera *pCamera, Mth::Vector & dropoff_pos );	// returns true if position defined

	static CBaseComponent*			s_create();


public:
	// PUBLIC FOR NOW!!!!
	uint32		mStreamingID[ MAX_STREAMS_PER_OBJECT ];

	
private:	
	bool		PlayStream( Script::CStruct *pParams, Script::CScript *pScript );
	// if this is non-zero, the object has streaming sound from the CD attached to it.
	// it is cleared and set in music.cpp.
	// Returns 0 if no stream slots empty, or the index + 1
	int			StreamSlotEmpty();

	CEmitterObject *mp_emitter;
	CProximNode *	mp_proxim_node;

// End of Stream stuff
////////////////////////////////////////////
	
	
	
};

}

#endif
