//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       BouncyComponent.h
//* OWNER:          Mick West
//* CREATION DATE:  10/22/2002
//****************************************************************************

#ifndef __COMPONENTS_BOUNCYCOMPONENT_H__
#define __COMPONENTS_BOUNCYCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/math.h>
#include <gel/object/basecomponent.h>


#define		CRC_BOUNCY CRCD(0x69c257e9,"Bouncy")
#define		GetBouncyComponent() ((Obj::CBouncyComponent*)GetComponent(CRC_BOUNCY))

namespace Mth
{
    class SlerpInterpolator;
};


#define BOUNCYOBJ_FLAG_PLAYER_COLLISION_OFF			( 1 << 0 )


namespace Obj
{


// These are in the Obj:: namespace, whilse we are sperating the bouncy componet from the bouncy object
enum
{
	BOUNCYOBJ_STATE_INIT,
	BOUNCYOBJ_STATE_IDLE,
	BOUNCYOBJ_STATE_BOUNCING,
};

class CBouncyComponent : public CBaseComponent
{
	
public:
// Functions common to components - the interface to the outside world								
								CBouncyComponent();
    virtual         			~CBouncyComponent();
    virtual void    			Update();
    virtual void    			InitFromStructure( Script::CStruct* pParams );
	static CBaseComponent *		s_create();
	CBaseComponent::EMemberFunctionResult	CallMemberFunction( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript );
	virtual void 				GetDebugInfo( Script::CStruct* p_info );

// private functions - indirectly called from the above functions	
private:
	void						bounce( const Mth::Vector &vel );
	void 						bounce_from_object_vel( const Mth::Vector &vel, const Mth::Vector &pos );
	void						do_bounce( void );
	bool 						rotate_to_quat( void );
	void 						set_up_quats( void );

	void 						land_on_any_face( Mth::Vector &rot );
	void 						land_on_top_or_bottom( Mth::Vector &rot );
	void 						land_traffic_cone( Mth::Vector &rot );

	int							m_bouncyobj_flags;
	Mth::Vector 				m_rotation;
	int							m_rot_per_axis;
	int							m_bounce_rot;
	float						m_min_bounce_vel;
	// should the object come to rest flat (90 degree oriented?)
	int							m_rest_orientation_type;
	// percent of bounce vel maintained ( 1.0 would make it bounce the same height )
	float						m_bounce_mult;
	float						m_up_mag; // this will be the upward velocity (varies depending on initial vel)
	int							m_bounce_count;
	bool						m_quat_initialized;
	float						m_quatrot_time_elapsed;
	float						m_gravity;
	float						m_bounciness;
	float						m_min_initial_vel;
	int							m_random_dir; // on bounce, change direction by this much...
	Mth::SlerpInterpolator *	mp_bounce_slerp;
	uint32						m_bounce_sound;
	uint32						m_hit_sound;
					
	float						m_bounce_collision_radius;
	float						m_skater_collision_radius_squared;
						
	bool						m_destroy_when_done;

    // logic states...
    short 						m_state;
    short 						m_substate;

	// 
	uint32						m_collide_script;
	Script::CStruct*			mp_collide_script_params;	
	uint32						m_bounce_script;
	Script::CStruct*			mp_bounce_script_params;	
	
	// The following is stuff that might be better off in the composite object, or some general physics component?
	// or perhaps a "simplephysics" base compoennt
	
	Mth::Vector					m_pos;
	Mth::Vector					m_old_pos;
	Mth::Vector					m_vel;
	Mth::Matrix					m_matrix;
	float						m_time;


};

}

#endif
