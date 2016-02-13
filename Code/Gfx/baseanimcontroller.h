//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       BaseAnimController.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/6/2003
//****************************************************************************

#ifndef __GFX_BASEANIMCONTROLLER_H__
#define __GFX_BASEANIMCONTROLLER_H__

#include <core/defines.h>
#include <core/support.h>

// for procedural bones, which will be moved to another file...
#include <core/math.h>

namespace Obj
{
	class CCompositeObject;
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
	class CSkeleton;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// (If the order of these change, then please also
// change Obj::CBaseComponent::EMemberFunctionResult)
enum EAnimFunctionResult
{
	AF_FALSE 		= 0,
	AF_TRUE  		= 1,
	AF_NOT_EXECUTED = 2
};

// maybe anim controllers should subclass from Obj::CBaseComponent
// so that they can access to the same CallMemberFunction interface?

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// TODO:  Move this to some generic animcontrollertypes.h

struct SWobbleDetails
{
    float				wobbleAmpA;
    float				wobbleAmpB;
    float				wobbleK1;
    float				wobbleK2;
    float				spazFactor;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// TODO:  Move this to some generic procanim.h

class CProceduralBone
{
public:
	CProceduralBone()
	{
		m_name = 0;
	}

public:
	uint32					m_name;
	
	bool					transEnabled;
	Mth::Vector				trans0;
	Mth::Vector				trans1;
	Mth::Vector				deltaTrans;
	Mth::Vector				currentTrans;
	
	bool					rotEnabled;
	Mth::Vector				rot0;
	Mth::Vector				rot1;
	Mth::Vector				deltaRot;
	Mth::Vector				currentRot;
	
	bool					scaleEnabled;
	Mth::Vector				scale0;
	Mth::Vector				scale1;
	Mth::Vector				deltaScale;
	Mth::Vector				currentScale;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class CBaseAnimController : public Spt::Class
{
public:
								CBaseAnimController( CBlendChannel* pBlendChannel );
	virtual 					~CBaseAnimController();

	virtual bool				GetPose( Gfx::CPose* pResultPose );
	virtual void				GetDebugInfo( Script::CStruct* p_info );
    virtual EAnimFunctionResult CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );

public:
	virtual void 				InitFromStructure( Script::CStruct* pParams );
	virtual void 				Update();

	int							GetPriority() const
	{
		return m_priority;
	}

	void						SetPriority( int priority )
	{
		m_priority = priority;
	}

	uint32						GetID() const
	{
		return m_name;
	}

protected:
	Obj::CCompositeObject*		GetObject();
	Gfx::CSkeleton*				GetSkeleton();

protected:
	CBlendChannel*				mp_blendChannel;
	Obj::CCompositeObject*		mp_object;
	int							m_priority;
	uint32						m_name;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}

#endif

