//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       SubAnimController.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/06/2003
//****************************************************************************

#ifndef __GFX_SUBANIMCONTROLLER_H__
#define __GFX_SUBANIMCONTROLLER_H__

#include <core/defines.h>
#include <core/support.h>

#include <core/math.h>

#include <gfx/baseanimcontroller.h>
#include <gfx/pose.h>

// This file contains all the custom animation controllers
// which can be chained together by an animation channel
// to produce its final array of RT data for each frame.
// For example, an animated skeleton might need to:
//   (1) grab interpolated keyframe data
//   (2) flip the skeleton
//   (3) rotate the skateboard by 180 degrees
//   (4) perform IK on the leg bones to lock them to the skateboard
//   (5) apply an additional keyframed anim to only the cloth bones
//   (6) orient the head towards the closest pedestrian.
// In this case, there would be one animation channel with
// six animation controllers.

// Eventually, we'll probably want to break up this
// code into a separate file for each animation
// controller, and perhaps create a factory class so that
// BlendChannel.cpp doesn't need to be updated every time
// a new animation controller is added.
						   
namespace Script
{
	class CScript;
	class CStruct;
}

namespace Nx
{
	class CQuickAnim;
}

namespace Gfx
{
	class CBlendChannel;
	class CProceduralBone;
	class CAnimChannel;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This controller is used for getting interpolated
// keyframe data from a CBonedAnimFrameData.  It
// accesses it through a Nx::CQuickAnim, which allows
// for future optimizations.

class CBonedAnimController : public CBaseAnimController
{
public:
	CBonedAnimController( CBlendChannel* pBlendChannel );
	virtual	~CBonedAnimController();

public:
	virtual void 				InitFromStructure( Script::CStruct* pParams );
	virtual void 				Update();
    virtual EAnimFunctionResult CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual bool				GetPose( Gfx::CPose* pResultPose );
	
protected:
	Nx::CQuickAnim*				mp_quickAnim;
	uint32						m_animScriptName;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This controller is used for wobble effects (such
// as grinding and manualling).  It's very similar to
// the boned anim controller, and should probably be
// refactored to share some code at some point.

class CWobbleController : public CBaseAnimController
{
public:
	CWobbleController( CBlendChannel* pBlendChannel );
	virtual	~CWobbleController();

public:
	virtual void 				InitFromStructure( Script::CStruct* pParams );
	virtual void 				Update();
	virtual void				GetDebugInfo( Script::CStruct* p_info );
    virtual EAnimFunctionResult CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual bool				GetPose( Gfx::CPose* pResultPose );

protected:
    SWobbleDetails  			m_wobbleDetails;
    float						m_wobbleTargetTime;
	Nx::CQuickAnim*				mp_quickAnim;
	uint32						m_animScriptName;

protected:
	float						get_new_wobble_time();

public:
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This controller is used for flipping the skeleton
// and rotating the skateboard.  Truthfully, the two
// actions are completely unrelated, but they are
// so simple that it seemed like a waste to separate
// them into two controllers.  Later on, I may decide
// to add separate controllers for each.

class CFlipRotateController : public CBaseAnimController
{
public:
	CFlipRotateController( CBlendChannel* pBlendChannel );
	virtual ~CFlipRotateController();

public:
	virtual void 				InitFromStructure( Script::CStruct* pParams );
	virtual void 				Update();
	virtual bool				GetPose( Gfx::CPose* pResultPose );
	
protected:
	bool						m_flipped;
	bool						m_rotated;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This controller is a placeholder for Left Field's
// IK code.  Although THPS4 did not use IK, it would
// be easy to add once LF and Neversoft both reorganized
// their code to use this system of animation channels
// and animation controllers.

class CIKController : public CBaseAnimController
{
public:
	CIKController( CBlendChannel* pBlendChannel );
	virtual ~CIKController();

public:
	virtual void 				InitFromStructure( Script::CStruct* pParams );
	virtual void 				Update();
	virtual void				GetDebugInfo( Script::CStruct* p_info );
    virtual EAnimFunctionResult CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual bool				GetPose( Gfx::CPose* pResultPose );
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This controller is also a placeholder.  On THPS4,
// we implemented something similar in order to
// programmatically animate our cloth bones.  This
// system was inefficient and unwieldy, and we've
// decided to scrap it in order to implement 
// "partial animations", which means the animators 
// can create SKA files containing animation
// data for only the cloth bones.  Left Field can
// potentially use the CProceduralAnimController
// to do programmatic animation on the bike's tires.

class CProceduralAnimController : public CBaseAnimController
{
public:
	CProceduralAnimController( CBlendChannel* pBlendChannel );
	virtual ~CProceduralAnimController();

public:
	virtual void 				InitFromStructure( Script::CStruct* pParams );
	virtual void 				Update();    
	virtual EAnimFunctionResult CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual bool				GetPose( Gfx::CPose* pResultPose );
	
protected:
	CProceduralBone*			get_procedural_bone_by_id( uint32 boneName );
	
protected:
	void		   				SetProceduralBoneTransMin( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneTransMax( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneTransSpeed( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneTransCurrent( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneTransActive( uint32 boneName, bool enabled );
	void						SetProceduralBoneRotMin( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneRotMax( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneRotSpeed( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneRotCurrent( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneRotActive( uint32 boneName, bool enabled );
	void						SetProceduralBoneScaleMin( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneScaleMax( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneScaleSpeed( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneScaleCurrent( uint32 boneName, const Mth::Vector& vec );
	void						SetProceduralBoneScaleActive( uint32 boneName, bool enabled );
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This controller just stores a single Gfx::CPose.
// Perhaps degenerating blend channels can
// collapse their list of animation controllers
// and replace them with the CPoseController
// as a slight optimization.

class CPoseController : public CBaseAnimController
{
public:
	CPoseController( CBlendChannel* pBlendChannel );
	virtual	~CPoseController();

public:
	virtual void 				InitFromStructure( Script::CStruct* pParams );
	virtual void 				Update();
	virtual bool				GetPose( Gfx::CPose* pResultPose );

public:
	void						SetPose( Gfx::CPose* pPose );

protected:
	Gfx::CPose					m_pose;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This controller hasn't been implemented yet.
// I was thinking that it could be used to
// reorient the head towards a particular lookup
// target.

class CLookAtController : public CBaseAnimController
{
public:
	CLookAtController( CBlendChannel* pBlendChannel );
	virtual	~CLookAtController();

public:
	virtual void 				InitFromStructure( Script::CStruct* pParams );
	virtual void 				Update();
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This controller hasn't been implemented yet.
// It will be used to read a partial SKA anim 
// from a file, for animating cloth bones and
// for doing secondary anims (for example, we
// may want to apply an additional wobble animation
// to the upper body when the skater lands from
// a particular high trick)
class CPartialAnimController : public CBaseAnimController
{

public:
	CPartialAnimController( CBlendChannel* pBlendChannel );
	virtual ~CPartialAnimController();

public:
	virtual void 				InitFromStructure( Script::CStruct* pParams );
	virtual void 				Update();
	virtual bool				GetPose( Gfx::CPose* pResultPose );
	EAnimFunctionResult			CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );

public:
	Gfx::CAnimChannel*			mp_animChannel;
	Nx::CQuickAnim*				mp_quickAnim;
	uint32						m_animScriptName;
};
																			   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}

#endif
