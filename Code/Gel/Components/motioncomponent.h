//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       MotionComponent.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  11/5/2002
//****************************************************************************

#ifndef __COMPONENTS_MOTIONCOMPONENT_H__
#define __COMPONENTS_MOTIONCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/math.h>

#include <gel/object/basecomponent.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_MOTION CRCD(0xa015e17,"Motion")
#define		GetMotionComponent() ((Obj::CMotionComponent*)GetComponent(CRC_MOTION))
#define		GetMotionComponentFromObject(pObj) ((Obj::CMotionComponent*)(pObj)->GetComponent(CRC_MOTION))

namespace Mth
{
	class SlerpInterpolator;
}

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CPathOb;
	class CPathObjectTracker;
    class CFollowOb;


	enum
	{
		// not used
		// MOVINGOBJ_STATUS_IDLE 				= 0,

		MOVINGOBJ_STATUS_ROTX 				= ( 1 << 0  ),
		MOVINGOBJ_STATUS_ROTY 				= ( 1 << 1  ),
		MOVINGOBJ_STATUS_ROTZ 				= ( 1 << 2  ),
		MOVINGOBJ_STATUS_MOVETO 			= ( 1 << 3  ),
		MOVINGOBJ_STATUS_ON_PATH 			= ( 1 << 4  ),
		MOVINGOBJ_STATUS_QUAT_ROT 			= ( 1 << 5  ),
		MOVINGOBJ_STATUS_LOOKAT 			= ( 1 << 6  ), // will be on in addition to rotx, y or z flags.
		MOVINGOBJ_STATUS_HOVERING 			= ( 1 << 7  ),
		MOVINGOBJ_STATUS_JUMPING  			= ( 1 << 8  ),
		MOVINGOBJ_STATUS_FOLLOWING_LEADER	= ( 1 << 9  ),

		// moved into lock obj component
		// MOVINGOBJ_STATUS_LOCKED_TO_OBJECT	= ( 1 << 10 ),

		// don't do any other processing, until this flag is off:
		// wasn't being used...
		// MOVINGOBJ_STATUS_HIGH_LEVEL 		= ( 1 << 31 ), // running C-code on a high level.
	};

	#define MOVINGOBJ_STATUS_ROTXYZ ( MOVINGOBJ_STATUS_ROTX | MOVINGOBJ_STATUS_ROTY | MOVINGOBJ_STATUS_ROTZ )
	
	// movement flags (to be used with m_movingobj_flags)
	#define MOVINGOBJ_FLAG_STICK_TO_GROUND		( 1 << 2 )
	#define MOVINGOBJ_FLAG_STOP_ALONG_PATH		( 1 << 3 )
	#define MOVINGOBJ_FLAG_INDEPENDENT_HEADING	( 1 << 4 )
	#define MOVINGOBJ_FLAG_CONST_ROTX			( 1 << 5 )
	#define MOVINGOBJ_FLAG_CONST_ROTY			( 1 << 6 )
	#define MOVINGOBJ_FLAG_CONST_ROTZ			( 1 << 7 )
	#define MOVINGOBJ_FLAG_CONSTANT_HEIGHT		( 1 << 8 )
	#define MOVINGOBJ_FLAG_RANDOM_PATH_MODE		( 1 << 9 )
	#define MOVINGOBJ_FLAG_NO_PITCH_PLEASE		( 1 << 10 )

// Used to store the coords of the last triangle collided with,
// so that it can be checked first next time as a collision speed optimization.
struct STriangle
{
	Mth::Vector mpVertices[3];
	void	Init() {mpVertices[0].Set();mpVertices[1].Set();mpVertices[2].Set();}
};

class CMotionComponent : public CBaseComponent
{
public:
    CMotionComponent();
    virtual ~CMotionComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
	virtual void					ProcessWait( Script::CScript * pScript );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	
	static CBaseComponent*			s_create();

	virtual void					GetDebugInfo( Script::CStruct* p_info );
	
public:
	CPathOb*				GetPathOb();
	void					Reset();
	bool					IsMoving( void );
	bool 					IsRotating( void );

	bool					IsHovering();
	void					GetHoverOrgPos( Mth::Vector *p_orgPos );

	// Added by Ken. If the object is following a path, this will return the node
	// index of the node it is heading towards. Returns -1 otherwise.
public:
	void					DoPathPhysics();
	int						GetDestinationPathNode();
	int						GetPreviousPathNode();
	void					EnsurePathobExists( CCompositeObject* pCompositeObj );
	CCompositeObject*   	GetNextObjectOnPath(float range);
	void 					OrientToNode( Script::CStruct* pNodeData );
	
	void					InitPath( int nodeNumber );
	void					HitWaypoint( int nodeIndex );
	bool					PickRandomWaypoint();

	bool					LookAt_Init( Script::CStruct* pParams, const Mth::Vector& lookAtPos );
	void					LookAtNodeLinked_Init( Script::CStruct* pParams, Script::CScript* pScript );
	void					LookAtNode_Init( Script::CStruct* pParams, Script::CScript* pScript );
	void					LookAtPos_Init( Script::CStruct* pParams, bool absolute );
//	bool					LookAtObject_Init( Script::CStruct* pParams, Script::CScript* pScript );
	bool					Move_Init( Script::CStruct* pParams, Script::CScript* pScript, Mth::Vector pos, int nodeNum = -1 );
	void					MoveToPos_Init( Script::CStruct* pParams, Script::CScript* pScript, bool absolute );
	void					MoveToNode_Init( Script::CStruct* pParams, Script::CScript* pScript );
	void					MoveToLink_Init( Script::CStruct* pParams, Script::CScript* pScript );
	void					FollowPath_Init( Script::CStruct* pParams );
	void					FollowPathLinked_Init( Script::CStruct* pParams, Script::CScript* pScript );
	void					FollowStoredPath_Init( Script::CStruct* pParams );
	void					FollowLeader_Init( Script::CStruct* pParams );
	void					QuatRot_Init( Script::CStruct* pParams );
	void					RotAxis_Init( Script::CStruct* pParams, Script::CScript* pScript, int whichAxis );

	// used for looking at a node or position...
	bool					SetUpLookAtPos( const Mth::Vector& lookToPos, const Mth::Vector& currentPos, int headingAxis, int rotAxis, float threshold=0.0f );
	void					QuatRot_Setup( Script::CStruct* pParams, const Mth::Matrix& rot );
	void					SetCorrectRotationDirection( int rotAxis );
	void					RotateToFacePos( const Mth::Vector pos );
	
	void					Rot( int rotAxis, float deltaRot );
	bool					Rotate( int rotAxis );
	void					RotUpdate( void );
	bool					Move( void );
	void					FollowPath( void );
	void					QuatRot( void );
	bool					StickToGround();
	void					UndoHover( void );
	void					ApplyHover( void );
public:
	// GJ:  Had to make this public during the transition,
	// because the moving objects need to access it
	// eventually, everything will be manipulated through
	// accessor functions				
    int						m_movingobj_status;
	int						m_movingobj_flags;
	
	float					m_moveto_speed;
	float					m_acceleration;
	float					m_deceleration;
	float					m_max_vel;
	float					m_vel_z;
	float					m_stop_dist;

	// when sticking to the ground, collision line length:
	float					m_col_dist_above;
	float					m_col_dist_below;
	STriangle				m_last_triangle;
	bool					m_point_stick_to_ground;
	
	STriangle				m_car_left_rear_last_triangle;
	STriangle				m_car_right_rear_last_triangle;
	STriangle				m_car_mid_front_last_triangle;

	// A CPathObjectTracker keeps track of all the objects that are following a
	// particular path. Stored so that the object can un-register with the tracker
	// when it starts following a new path.
	CPathObjectTracker*		mp_path_object_tracker;
protected:
	Mth::Vector				m_moveto_pos;
	float					m_moveto_dist;
	float					m_moveto_dist_traveled;
//	float					m_moveto_speed;			// TEMPORARILY PUBLIC
	float					m_moveto_speed_target;
	float					m_moveto_acceleration;
	
	// so that we can call SpawnObjScript or SwitchObjScript when we arrive...
	int						m_moveto_node_num;

	Mth::Vector				m_turn_ang_target;
	Mth::Vector				m_cur_turn_ang;
	Mth::Vector				m_turn_speed_target;
	Mth::Vector				m_turn_speed;
	Mth::Vector				m_delta_turn_speed;
	Mth::Vector				m_angular_friction;
	Mth::Vector				m_orig_pos;

	// path traversal:
//	float					m_acceleration;			// TEMPORARILY PUBLIC
//	float					m_deceleration;			// TEMPORARILY PUBLIC
//	float					m_max_vel;				// TEMPORARILY PUBLIC
//	float					m_vel_z;				// TEMPORARILY PUBLIC
//	float					m_stop_dist;        	// TEMPORARILY PUBLIC
	float					m_stop_dist_traveled;
	float					m_vel_z_start;
	float					m_min_stop_vel;
	
	// gotta remember where to return to when going to a higher level:
	Mth::Vector				m_stored_pos;
	int						m_stored_node;

	float					m_rot_time_target;
	float					m_rot_time;
	Mth::SlerpInterpolator*	mp_slerp;

	float					m_y_before_applying_hover;
	float					m_hover_offset;
	float					m_hover_amp;
	int						m_hover_period;

	float					m_time;

protected:
	// moved this down from CMovingObject
	// (information about the node that created this object)
	int						m_start_node; 	// renamed, as we want to use it for path objs, but not for node identification
	int						m_current_node;  // this may change if the object is moving... (needs to be at CMovingObject level for querying)

protected:
	CPathOb*				mp_pathOb;
	
// Mo stuff from MovingObject.h		
		
		void					FollowLeader( void );
		
		// Pointer to the follow ob, which contains all the info required to follow
		// an object on a path. Done this way so as to not use up memory for every instance
		// of a CMovingObject. (similar to CPathOb)
		CFollowOb*				mp_follow_ob;

		// K: I think I should have put this in CFollowOb too?
		bool					m_leave_y_unaffected_when_following_leader;

private:		
		
		void					DoJump( void );
		float					m_jump_speed;
		float					m_jump_gravity;
		Mth::Vector				m_jump_heading;
		bool					m_jump_use_heading;
		Mth::Vector				m_jump_pos;
		float					m_jump_original_speed;
		float					m_jump_col_dist_above;
		float					m_jump_col_dist_below;
		float					m_jump_y;
		bool					m_jump_use_land_height;
		float					m_jump_land_height;
		// The whole start pos is stored rather than just the y so that it knows whether 
		// it needs to do another collision check once the jump has got back to the start y.
		// If the x or z has changed then it needs to do another collision check.
		Mth::Vector	 		  	m_jump_start_pos;

	
	
};

}

#endif
