//****************************************************************************
//* MODULE:         Sk/Components/
//* FILENAME:       EditorCameraComponent.h
//* OWNER:          KSH
//* CREATION DATE:  26 Mar 2003
//****************************************************************************

#ifndef __COMPONENTS_EDITORCAMERACOMPONENT_H__
#define __COMPONENTS_EDITORCAMERACOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

// Replace this with the CRCD of the component you are adding
#define		CRC_EDITORCAMERA CRCD(0xac8946e2,"EditorCamera")

//  Standard accessor macros for getting the component either from within an object, or 
//  given an object				 
#define		GetEditorCameraComponent() ((Obj::CEditorCameraComponent*)GetComponent(CRC_EDITORCAMERA))
#define		GetEditorCameraComponentFromObject(pObj) ((Obj::CEditorCameraComponent*)(pObj)->GetComponent(CRC_EDITORCAMERA))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Nx
{
	class CModel;
}

namespace Gfx
{
	class CSimpleShadow;
}
	
namespace Obj
{
class CInputComponent;

class CEditorCameraComponent : public CBaseComponent
{
	bool m_active;
	
	// The point at which the camera looks. This basically equals m_cursor_pos
	// except it smoothly catches up with m_cursor_pos so as to smooth out glitches.
	Mth::Vector m_camera_focus_pos;
	Mth::Vector m_cam_pos;
	
	// The position of the Goal model that get moves around by the controls.
	Mth::Vector m_cursor_pos;
	
	// The orthogonal horizontal vectors that define the cursor's orientation					 
	Mth::Vector m_cursor_u;
	Mth::Vector m_cursor_v;
	
	float m_radius;
	float m_radius_min;
	float m_radius_max;
	
	float m_angle;
	float m_tilt_angle;
	float m_tilt_angle_min;
	float m_tilt_angle_max;

	float m_cursor_height;
	float m_min_height;

	bool m_simple_collision;
	bool m_allow_movement_through_walls;
		
	CInputComponent *mp_input_component;
	
	Gfx::CSimpleShadow *mp_shadow;
	void create_shadow();
	void delete_shadow();
	void update_shadow();
	#define SHADOW_COLLISION_CHECK_DISTANCE -10000.0f
	#define SHADOW_SCALE 1.2f
	
	bool cursor_occluded(Mth::Vector *p_ray_starts, int numRays, Mth::Vector end);

	bool find_y(Mth::Vector start, Mth::Vector end, float *p_y);
	bool find_collision_at_offset(Mth::Vector oldCursorPos, float upOffset);
	

public:
    CEditorCameraComponent();
    virtual ~CEditorCameraComponent();

public:
	Mth::Vector&					GetCursorPos() {return m_cursor_pos;}
	void							SetCursorPos(const Mth::Vector& pos) {m_cursor_pos=pos;}
	void							GetCursorOrientation(Mth::Vector *p_u, Mth::Vector *p_v);
	float							GetCursorOrientation() {return m_angle;}
	void							SetCursorOrientation(float angle) {m_angle=angle;}
	
	void							SetCursorHeight(float height) {m_cursor_height=height;}
	float							GetCursorHeight() {return m_cursor_height;}
	void							SetCursorMinHeight(float minHeight) {m_min_height=minHeight;}
	
	void							SetSimpleCollision(bool whatever) {m_simple_collision=whatever;}
	
	virtual	void 					Finalize();
	virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
	virtual void					Hide( bool shouldHide );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
};

}

#endif // __COMPONENTS_EDITORCAMERACOMPONENT_H__
