//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       AnimationComponent.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/24/2002
//****************************************************************************

#ifndef __COMPONENTS_ANIMATIONCOMPONENT_H__
#define __COMPONENTS_ANIMATIONCOMPONENT_H__
								   
#include <core/defines.h>
#include <core/support.h>

#include <core/list.h>

#include <gel/object/basecomponent.h>

#include <gfx/blendchannel.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_ANIMATION CRCD(0x72ad7b23,"Animation")
#define		GetAnimationComponent() ((Obj::CAnimationComponent*)GetComponent(CRC_ANIMATION))
#define		GetAnimationComponentFromObject(pObj) ((Obj::CAnimationComponent*)(pObj)->GetComponent(CRC_ANIMATION))

namespace Gfx
{
	class CBonedAnimFrameData;
	class CPose;
	class CProceduralBone;
}

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CSuspendComponent;
	class CSkeletonComponent;
	class CModelComponent;

class CAnimationComponent : public CBaseComponent
{
public:
    CAnimationComponent();
    virtual ~CAnimationComponent();

public:
	// Basic component functions
    virtual void            		Update();
    virtual void					Finalize( );
    virtual void            		InitFromStructure( Script::CStruct* pParams ); 
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void					GetDebugInfo( Script::CStruct* p_info );
	virtual void					ProcessWait( Script::CScript * pScript );
	
    static CBaseComponent*			s_create();

public:
	// Viewer functions
    void       		            	PrintStatus();
	void							AddTime( float incVal );
	float                       	GetCurrentAnimTime( void );

public:
    void							SetAnims( uint32 anim_checksum );
	void                        	Reset();
	void				        	PlaySequence(uint32 checksum, float BlendPeriod=0.3f );    
    bool				        	AnimEquals(Script::CStruct *pParams, Script::CScript *pScript );
    uint32                      	PlayAnim(Script::CStruct *pParams, Script::CScript *pScript, float defaultBlendPeriod = 0.3f );
	uint32							GetAnimScriptName() const { return m_animScriptName; }
	uint32							GetAnimEventTableName() const { return m_animEventTableName; }

public:
    // TODO:  Eventually, make this class completely independent of Gfx...
    // (You shouldn't need to reference CBonedAnimFrameData from this class...)
    float                       	AnimDuration( uint32 checksum );
    bool                       		AnimExists( uint32 checksum );
    		
protected:
	bool							ShouldBlend();

public:
	void							EnableBlending( bool enabled );
	void 							ToggleFlipState();
	void							UpdateSkeleton();

public:
	// CLIENT FUNCTIONS
	void				        	PlayPrimarySequence( uint32 index, bool propagate, float start_time = 0.0f, float end_time = 1000.0f, Gfx::EAnimLoopingType loop_type = Gfx::LOOPING_CYCLE, float blend_period = 0.3f, float speed = 1.0f );

	void				        	SetLoopingType( Gfx::EAnimLoopingType loopingType, bool propagate );
	void				        	SetAnimSpeed( float speed, bool propagate, bool all_channels = false );
    float				        	GetAnimSpeed( Script::CStruct* pParams, Script::CScript* pScript );
	void							ReverseDirection( bool propagate );	

	// called once per frame, during manuals!
	void				        	SetWobbleTarget(float alpha, bool propagate);

	// should not be called once per frame...  only when the trick changes
	void				        	SetWobbleDetails( const Gfx::SWobbleDetails& wobble_details, bool propagate );
	
	bool				    		FlipAnimation( uint32 objId, bool flip, uint32 time, bool propagate );
	void							SetFlipState( bool shouldFlip );
	void							SetBoneRotation( uint32 boneId, bool shouldRotate );
	bool							IsFlipped();
	void							AddAnimController( Script::CStruct* pParams );
	bool							ShouldAnimate();
	
public:
	// SERVER-ONLY FUNCTIONS
	bool				        	IsAnimComplete( void );
	bool				        	IsLoopingAnim( void );
	uint32				        	GetCurrentSequence( void );
	void				        	GetPrimaryAnimTimes(float *pStart, float *pCurrent, float *pEnd);

protected:
	Gfx::CBlendChannel*				get_primary_channel();
	void							pack_degenerate_channels();
    Gfx::CBonedAnimFrameData*		find_actual_anim( uint32 checksum );
	int								get_num_degenerate_anims();

protected:
	bool							has_anims() { return m_animScriptName != 0; }
	void							update_skeleton();

	void							get_blend_channel( int blendChannel, Gfx::CPose* pResultPose, float* pBlendVal );
	void							destroy_blend_channels();
	void							create_new_blend_channel( float blend_period );
	void							add_anim_tags( uint32 animName );
	void							delete_anim_tags();

protected:
	Lst::Head<Gfx::CAnimChannel>	m_blendChannelList;

    // Get set by the BlendPeriodOut command.
	bool                        	mGotBlendPeriodOut;
	float                       	mBlendPeriodOut;

	bool							m_shouldBlend;
	
	// If set, then PlayAnim will do nothing until the current anim has finished.
	bool							m_dont_interrupt;
	
    uint32                      	m_animScriptName;
	uint32							m_flags;
	
	// GJ:  This might get split into its own component later on
	// (among other things, it can then have its own suspend logic)
	uint32							m_animEventTableName;
	
	float							m_last_animation_time;
	bool							m_animation_script_block_active;
	float							m_animation_script_unblock_point;

	float							m_parent_object_dist_to_camera;

	// GJ:  The following used to be in the CProceduralAnimController;
	// however, this data needs to be shared among different blend
	// channels so I had to move it here.  Maybe there should be a 
	// way to copy a controller from one blend channel to another?  
	// For now, I'm just going to go with the easy implementation
	// until there's a pressing need for it.  Maybe this should be
	// separated off into a separate CProceduralAnimComponent that 
	// the CProceduralAnimController acts upon?)
protected:
	int								m_numProceduralBones;
	Gfx::CProceduralBone*			mp_proceduralBones;
	bool							InitializeProceduralBone( uint32 boneName );
	void							initialize_procedural_bones( Script::CStruct* pParams );
	void							update_procedural_bones();
	
protected:
	CSuspendComponent*				mp_suspend_component;
	CSkeletonComponent*				mp_skeleton_component;
	CModelComponent*				mp_model_component;

public:
	Gfx::CProceduralBone*			GetProceduralBoneByName( uint32 id );
	int								GetNumProceduralBones();
	Gfx::CProceduralBone*			GetProceduralBones();
};

}

#endif
