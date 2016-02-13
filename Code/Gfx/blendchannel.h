//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       BlendChannel.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/12/2002
//****************************************************************************

#ifndef __GFX_BLENDCHANNEL_H__
#define __GFX_BLENDCHANNEL_H__

#include <core/defines.h>
#include <core/support.h>

#include <gfx/animcontroller.h>
#include <gfx/baseanimcontroller.h>
#include <gfx/customanimkey.h>
	
namespace Object
{
	class CObject;
}
			 
namespace Script
{
	class CScript;
	class CStruct;
}

namespace Gfx
{
	class CBlendChannel;
	class CPose;
	class CBaseAnimController;

	// NOTE: if you change this enum, update the GetDebugInfo switch statement!	
	enum EAnimStatus
	{								 
		ANIM_STATUS_INACTIVE		= 0,	// No animation playing.
		ANIM_STATUS_DEGENERATING,			// Animation playing, but being faded out.
		ANIM_STATUS_ACTIVE					// Animation playing.
	};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class CBlendChannel : public CAnimChannel
{
protected:
	// TODO:  Replace with a more efficient list structure?
	enum
	{
		vMAX_CONTROLLERS = 4
	};

public:
	CBlendChannel( Obj::CCompositeObject* pCompositeObject );
	virtual	~CBlendChannel();

public:
	CBaseAnimController*		AddController( Script::CStruct* pParams );
	bool						GetPose( Gfx::CPose* pPose );
	float						GetBlendValue();
	bool						IsActive();
	bool						IsDegenerating();
	void						InvalidateCache();
    virtual EAnimFunctionResult	CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );

public:
	virtual void				Update();
	virtual void				Reset();
	virtual void				GetDebugInfo( Script::CStruct* p_info );
	virtual void        		PlaySequence( uint32 animName, float start_time, float end_time, EAnimLoopingType loop_type, float blend_period, float speed, bool flipped );

public:
	float						GetDegenerationTime() { return m_degenerationTime; }
	float						GetDegenerationTimeToBlendMultiplier() { return m_degenerationTimeToBlendMultiplier; }
	void						SetDegenerationTime( float time ) { m_degenerationTime = time; }
	void						SetDegenerationTimeToBlendMultiplier( float mult ) { m_degenerationTimeToBlendMultiplier = mult; }
	bool						Degenerate( float blendPeriod );

protected:		
	void						add_controller( CBaseAnimController* pAnimController, int priority = 0 );
	void						remove_controllers();
	void						remove_controller( CBaseAnimController* pAnimController );
	CBaseAnimController* 		get_controller_by_id( uint32 id );

protected:
	EAnimStatus					GetStatus() const;

public:
	void						SetStatus(EAnimStatus status);
	Obj::CCompositeObject*		GetObject();

protected:
	EAnimStatus					m_status;
                       
protected:
	CBaseAnimController*		mp_controllers[vMAX_CONTROLLERS];
	int							m_numControllers;
    
	float						m_blendSpeed;
	float						m_degenerationTime;
    float						m_degenerationTimeToBlendMultiplier;

	uint32						m_animScriptName;
	Obj::CCompositeObject*		mp_object;

public:
	// for doing anim events...
	void							ProcessCustomKeys(float startTime, float endTime);
	void							ResetCustomKeys();

protected:
	void 							add_custom_keys( uint32 animName );
	void							delete_custom_keys();
	Lst::Head<Gfx::CCustomAnimKey>	m_customAnimKeyList;	
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}

#endif

