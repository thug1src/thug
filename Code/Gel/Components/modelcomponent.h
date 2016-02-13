//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       ModelComponent.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/17/2002
//****************************************************************************

#ifndef __COMPONENTS_MODELCOMPONENT_H__
#define __COMPONENTS_MODELCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <core/math.h>

#include <gel/object/basecomponent.h>
#include <gfx/image/imagebasic.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_MODEL CRCD(0x286a8d26,"Model")
#define		GetModelComponent() ((Obj::CModelComponent*)GetComponent(CRC_MODEL))
#define		GetModelComponentFromObject(pObj) ((Obj::CModelComponent*)(pObj)->GetComponent(CRC_MODEL))

class CFeeler;
		
namespace Gfx
{
	class CModelAppearance;
}

namespace Nx
{
    class CModel;
}

namespace Script
{
	class CScript;
    class CStruct;
}
              
namespace Obj
{

	class CSkeletonComponent;
	class CAnimationComponent;
	class CSuspendComponent;


struct SDisplayRotation
{
	bool mDispRotating;
	bool mHoldOnLastAngle;
	float mDispDuration;
	Tmr::Time mDispStartTime;
	float mDispStartAngle;
	float mDispChangeInAngle;
	int	mDispSinePower;
	
	void SetUp(float duration, Tmr::Time start_time, float start_angle, float change_in_angle, int sine_power, bool holdOnLastAngle);
	float CalculateNewAngle();
	void Clear();
};

struct SDisplayRotationInfo
{
	// Factored out the data members into the SDisplayRotation structure so that
	// there can be 6 operating at once. Needed by Zac for create-a-trick.
	enum
	{
		MAX_ROTATIONS=6
	};
	SDisplayRotation mpRotations[MAX_ROTATIONS];
	bool	m_active;
	
	void SetUp(float duration, Tmr::Time start_time, float start_angle, float change_in_angle, int sine_power, bool holdOnLastAngle);
	float CalculateNewAngle();
	void Clear();
};

class CModelComponent : public CBaseComponent
{
public:
    CModelComponent();
    virtual	~CModelComponent();

public:
    virtual void    		Update();
    virtual void    		InitFromStructure( Script::CStruct* pParams );
    virtual void			Hide( bool should_hide );
    virtual void			Finalize();
	virtual void			Teleport();

    EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	
	// Used by the script debugger code to fill in a structure
	// for transmitting to the monitor.exe utility running on the PC.
	virtual void			GetDebugInfo(Script::CStruct *p_info);
	
	static CBaseComponent*	s_create();

public:
	void					InitModel( Script::CStruct* pParams );
	virtual void			InitModelFromProfile( Gfx::CModelAppearance* pAppearance, bool useAssetManager, uint32 texDictOffset, uint32 buildScript = 0 );
	void 					FinalizeModelInitialization();
	void					RefreshModel( Gfx::CModelAppearance* pAppearance, uint32 buildScript );
    Nx::CModel*     		GetModel();
	bool					HideGeom( uint32 geomName, bool hidden, bool propagate );
    bool					GeomHidden( uint32 geomName );
	void					SetModelLODDistance( int lodIndex, float distance );
	void					SetBoundingSphere( const Mth::Vector& newSphere );
	void					ApplyLightingFromCollision ( CFeeler& feeler );
	void					ApplySceneLighting(Image::RGBA color);
	void					UpdateBrightness();
	// K: This used to be protected, but I made it public for use by CLockObjComponent::LockToObject
	void					GetDisplayMatrixWithExtraRotation( Mth::Matrix& displayMatrix );
	
	void					SetDisplayOffset ( const Mth::Vector& display_offset ) { mDisplayOffset = display_offset; }
	
protected:
	void 					init_model_from_level_object( uint32 checksumName );
	bool					enable_lod(uint32 componentName, float distance);


protected:
	// Sometimes we need to temporarily change the bounding sphere
	// of an object (like when an animation takes a model far
	// from its original bouding sphere)...  the following
	// is used to correctly restore the original bounding sphere
	Mth::Vector				m_original_bounding_sphere;


// the following are used for tweaking the display matrix
// right before the rendered model actually uses it
// public during transition
public:
	
	// The offset relative to the skater's origin about which the display rotation rotates.
	Mth::Vector 			m_display_rotation_offset;
	
	
	// Stuff for rotating just the display matrix, so that
	// the physics and camera don't notice it.
	SDisplayRotationInfo 	mpDisplayRotationInfo[3];

	// Controlled by the script commands EnableDisplayFlip and DisableDisplayFlip.
	// Added to allow the lip script to finish its out-anim whilst on the ground without a glitch.
	// When set, this will cause the display matrix to be the regular matrix flipped, each frame.
	bool					mFlipDisplayMatrix;
	
	// Offset from object's position at which the model is drawn.  Defaults to zero.
	Mth::Vector				mDisplayOffset;

public:
	enum
	{
		vNUM_LODS = 4
	};

protected:
    Nx::CModel*    		 	mp_model;
	CSkeletonComponent*		mp_skeleton_component;
	CAnimationComponent*	mp_animation_component;
	CSuspendComponent*		mp_suspend_component;

protected:
	float				m_LODdist[vNUM_LODS];
	int					m_numLODs;

	// GJ:  ref objects are used by the cutscene code
	// for stealing another object's model
	// (maybe cutscene objects should just have
	// CRefModelComponent?)
public:
	uint32				GetRefObjectName();
	bool				HasRefObject()
	{
		return m_hasRefObject;
	}

protected:
	uint32				m_refObjectName;
	bool				m_hasRefObject;
	
private:
	bool				m_isLevelObject;	// True if it's a level object	
	
};

inline Nx::CModel* CModelComponent::GetModel()
{
    return mp_model;
}

}

#endif
