/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Object (OBJ)											**
**																			**
**	File name:		objects/skater.h										**
**																			**
**	Created: 		01/25/99	-	mjb										**
**																			**
*****************************************************************************/

#ifndef __OBJECTS_SKATER_H
#define __OBJECTS_SKATER_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

#include <core/math/vector.h>
#include <core/math/matrix.h>
#include <core/string/cstring.h>
// #include <core/timestampedflag.h>

// #include <sk/objects/skaterflags.h>

// #include <sk/objects/gap.h>
// #include <sk/objects/manual.h>
#include <sk/objects/movingobject.h>
#include <sk/objects/trickobject.h>
// #include <sk/objects/skaterpad.h>

// #include <sys/timer.h>

#include <gel/inpman.h>
// #include <gel/net/net.h>

#include <sk/modules/skate/score.h>
#include <sk/modules/skate/createatrick.h>

#include <gel/objptr.h>

#define		SNAP_OVER_THIN_WALLS  	// snap over thin walls (like rails)
#define		STICKY_WALLRIDES		 // attempt to snap sideways to wallrides

// temp debug macros
#define MESSAGE(a) { printf("M:%s:%i: %s\n", __FILE__ + 15, __LINE__, a); }
#define DUMPI(a) { printf("D:%s:%i: " #a " = %i\n", __FILE__ + 15, __LINE__, a); }
#define DUMPB(a) { printf("D:%s:%i: " #a " = %s\n", __FILE__ + 15, __LINE__, a ? "true" : "false"); }
#define DUMPF(a) { printf("D:%s:%i: " #a " = %g\n", __FILE__ + 15, __LINE__, a); }
#define DUMPE(a) { printf("D:%s:%i: " #a " = %e\n", __FILE__ + 15, __LINE__, a); }
#define DUMPS(a) { printf("D:%s:%i: " #a " = %s\n", __FILE__ + 15, __LINE__, a); }
#define DUMPP(a) { printf("D:%s:%i: " #a " = %p\n", __FILE__ + 15, __LINE__, a); }
#define DUMPC(a) { printf("D:%s:%i: " #a " = %s\n", __FILE__ + 15, __LINE__, Script::FindChecksumName(a)); }
#define DUMPV(a) { printf("D:%s:%i: " #a " = %g, %g, %g\n", __FILE__ + 15, __LINE__, (a)[X], (a)[Y], (a)[Z]); }
#define DUMP4(a) { printf("D:%s:%i: " #a " = %g, %g, %g, %g\n", __FILE__ + 15, __LINE__, (a)[X], (a)[Y], (a)[Z], (a)[W]); }
#define DUMPM(a) { DUMP4(a[X]); DUMP4(a[Y]); DUMP4(a[Z]); DUMP4(a[W]); }
#define MARK { printf("K:%s:%i: %s\n", __FILE__ + 15, __LINE__, __PRETTY_FUNCTION__); }
#define PERIODIC(n) for (static int p__ = 0; (p__ = ++p__ % (n)) == 0; )

#define RED MAKE_RGB(255, 0, 0)
#define GREEN MAKE_RGB(0, 255, 0)
#define BLUE MAKE_RGB(0, 0, 255)
#define YELLOW MAKE_RGB(200, 200, 0)
#define PURPLE MAKE_RGB(200, 0, 200)
#define CYAN MAKE_RGB(0, 200, 200)

// skater specific debug defines
#ifdef __NOPT_ASSERT__
#define HEADER { printf("%i:%s:%i: ", Tmr::GetRenderFrame(), __FILE__ + 15, __LINE__); }
#define DUMP_VELOCITY { if (Script::GetInteger(CRCD(0x71f0478a, "debug_skater_vel"))) { HEADER; printf("m_vel = %g, %g, %g (%g)\n", GetObject()->m_vel[X], GetObject()->m_vel[Y], GetObject()->m_vel[Z], GetObject()->m_vel.Length()); } }
#define DUMP_POSITION { if (Script::GetInteger(CRCD(0x029ade47, "debug_skater_pos"))) { HEADER; printf("m_pos = %g, %g, %g\n", GetObject()->m_pos[X], GetObject()->m_pos[Y], GetObject()->m_pos[Z]); } }
#define DUMP_SPOSITION { if (Script::GetInteger(CRCD(0x029ade47, "debug_skater_pos"))) { HEADER; printf("m_pos = %g, %g, %g\n", m_pos[X], m_pos[Y], m_pos[Z]); } }
#else
#define DUMP_VELOCITY {   }
#define DUMP_POSITION {   }
#define DUMP_SPOSITION {   }
#endif

// if uncommented here, uncomment in skaterpad.cpp too								   
//#define		PS2_SIMULATING_XBOX

#ifdef	__PLAT_NGPS__
	#ifdef	PS2_SIMULATING_XBOX
		#define	BREAK_SPINE_BUTTONS (control_pad.m_L2.GetPressed() && control_pad.m_R2.GetPressed())
		#define	WALK_SPINE_BUTTONS (control_pad.m_L2.GetPressed() && control_pad.m_R2.GetPressed())
	#else	
		#define	BREAK_SPINE_BUTTONS (control_pad.m_L2.GetPressed() || control_pad.m_R2.GetPressed())
		#define	WALK_SPINE_BUTTONS (control_pad.m_R2.GetPressed())
	#endif
#endif

#ifdef	__PLAT_XBOX__
#define	BREAK_SPINE_BUTTONS (control_pad.m_L2.GetPressed() && control_pad.m_R2.GetPressed())
#define	WALK_SPINE_BUTTONS (control_pad.m_L2.GetPressed() && control_pad.m_R2.GetPressed())
#endif

#ifdef	__PLAT_NGC__
#define	BREAK_SPINE_BUTTONS (control_pad.m_L1.GetPressed() && control_pad.m_R1.GetPressed())
#define	WALK_SPINE_BUTTONS (control_pad.m_L1.GetPressed() && control_pad.m_R1.GetPressed())
#endif


#define	VERT_RECOVERY_BUTTONS (BREAK_SPINE_BUTTONS)

// #define	USE_BIKE_PHYSICS (Script::GetInteger(CRCD(0x9e86d1c1,"use_bike_physics")))
// #define	USE_WALK_PHYSICS (Script::GetInteger(CRCD(0x9ae06a83,"use_walk_physics")))
// #define	CESS_SLIDE_BUTTONS (control_pad.m_R1.GetPressed() && USE_BIKE_PHYSICS)

#define	SKITCH_BUTTON (control_pad.m_up.GetPressed() && control_pad.m_up.GetPressedTime()>(uint32)GetPhysicsInt(CRCD(0x19b2c752,"Skitch_Hold_Time")))

// These are defined here, so they can easily be switched from	
// script flags to some more general cheat flags
//#define		CHEAT_SNOWBOARD		(control_pad.m_R1.GetPressed()) //   (GetPhysicsInt("CheatSnowBoard",false) || GetCheat(GetPhysicsInt(0x9bc667b5)))  //CHEAT_SNOWBOARD
#define		CHEAT_SNOWBOARD		0	//(GetPhysicsInt("CheatSnowBoard",false) || GetCheat(GetPhysicsInt(0x9bc667b5)))  //CHEAT_SNOWBOARD
#define		CHEAT_MATRIX		(Mdl::Skate::Instance()->GetCareer()->GetPhysicsInt(CRCD(0xc41d8a34,"CheatMatrix"),false)  || GetCheat(GetPhysicsInt(0xf9514c9e)))	  // CHEAT_MATRIX
#define		CHEAT_FIRST_PERSON	0//(GetPhysicsInt("CheatFirstPerson",false)  || GetCheat(GetPhysicsInt(0xca459091))) // CHEAT_FIRST_PERSON
#define		CHEAT_PERFECT_RAIL	 (Mdl::Skate::Instance()->GetCareer()->GetCheat(GetPhysicsInt(0xcd09e062)))	  // CHEAT_PERFECT_RAIL
#define		CHEAT_PERFECT_MANUAL (Mdl::Skate::Instance()->GetCareer()->GetCheat(GetPhysicsInt(0xb38341c9)))	  // CHEAT_PERFECT_MANUAL
#define		CHEAT_PERFECT_SKITCH (Mdl::Skate::Instance()->GetCareer()->GetCheat(GetPhysicsInt(0x69a1ce96)))	  // CHEAT_PERFECT_SKITCH

#define		CHEAT_STATS_13   (Mdl::Skate::Instance()->GetCareer()->GetCheat(GetPhysicsInt(0x6b0f24bc)))  
#define		CHEAT_KID        0//(GetCheat(GetPhysicsInt(0x406e5a16))) 
#define		CHEAT_SIM        (Mdl::Skate::Instance()->GetCareer()->GetCheat(GetPhysicsInt(0x2b87107a)))  
#define		CHEAT_MOON       (Mdl::Skate::Instance()->GetCareer()->GetCheat(GetPhysicsInt(0x9c8c6df1)))  

#define		CHEAT_SLOMO		(Mdl::Skate::Instance()->GetCareer()->GetCheat(GetPhysicsInt(0xf163e7e0)))		// CHEAT_SLOMO
#define		CHEAT_DISCO		(Mdl::Skate::Instance()->GetCareer()->GetCheat(GetPhysicsInt(0x9fc22bda)))		// CHEAT_DISCO

////////////////////////////////////////////////////////////////////////
// The following #defines generally enable a bunch of prints
// that track the changes in value or state of a particular 
// aspect of the physics.
// Some of them will also display debugging lines
//


//#define	DEBUG_WALKING
//#define	DEBUG_ACC_LINES
//#define	DEBUG_INSIDE_OBJECT
//#define	DEBUG_SKITCHING
//#define	DEBUG_BOUNCE_OFF_WALL
//#define	DEBUG_WALLRIDE
//#define	DEBUG_VELOCITY
//#define	DEBUG_POSITION
//#define	DEBUG_SPINE
//#define	SHOW_MOVEMENT_LINES
//#define	DEBUG_UBERFRIG
//#define	DEBUG_DISPLAY_MATRIX
//#define	DEBUG_MATRIX
//#define	DEBUG_COL_FLAGS 
//#define	DEBUG_RAILS
//#define	DEBUG_TERRAIN
//#define	DEBUG_AIR_SNAP_UP
//#define	DEBUG_SHOW_ALL_COLLISION_LINES
//#define	DEBUG_SHOW_SIDE_PUSH_LINES
//#define	DEBUG_TRIGGERS
//#define	DEBUG_GAPS
//#define	DEBUG_STATES
//#define	DEBUG_VERT
//#define	DEBUG_RECOVER
//#define	DEBUG_BRAKE
//#define	DEBUG_FRICTION
//#define	DEBUG_HIGH_SCORE_GOALS
					   
#define		STOPPED_TURN_RAMP_TIME		600		// ms ramp time for turning when stopped

/*****************************************************************************
**						   Forward Declarations								**
*****************************************************************************/
	
// These are for network games
#define vNUM_LAG_FRAMES	1.0f
#define vFORCE_SEND_INTERVAL	15	// every 15 frames, send a full network object update

class	CContact;

namespace Script
{
	class CComponent;
}
	
namespace Mdl
{
	class Skate;
}

namespace Nx
{
	class CCollCache;
}

namespace Gfx
{
	class	Camera;
}

namespace Obj
{
	class 	CRailNode;							   
	class 	CSkaterProfile;
	class	CMovieManager;

	class	CTrickBufferComponent;
	
	class	CAnimationComponent;
	class	CModelComponent;
	class	CShadowComponent;
	class	CTrickComponent;
	class	CTriggerComponent;
	class	CInputComponent;
	class	CSkaterCameraComponent;
	class	CVibrationComponent;
	class	CSkaterSoundComponent;
	class	CSkaterGapComponent;
	class	CSkaterRotateComponent;
	class	CSkaterScoreComponent;
	class	CSkaterStateHistoryComponent;
	class	CSkaterCorePhysicsComponent;
	class	CSkaterPhysicsControlComponent;
	class	CWalkComponent;
    class	CStatsManagerComponent;
	
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

enum EViewMode
{
	GAMEPLAY_SKATER_ACTIVE,
	VIEWER_SKATER_PAUSED,
	VIEWER_SKATER_ACTIVE,
	NUM_VIEW_MODES
};

#define MAX_TRICK_NAME_CHARS 50

// Used for storing any info needed to restore the skater after it is
// hidden during a replay.
// Doesn't contain much at the moment.
class CSkaterModelRestorationInfo
{
public:
	//bool mShadowOn;
	Mth::Vector mPreReplayPos;
	Mth::Matrix mPreReplayDisplayMatrix;
	uint32 mAtomicStates;
};

class  CSkater : public  CMovingObject 
{
	friend class Mdl::Skate;

public:

	void DodgyTest();

	enum	EPushStyle
	{
		vNEVER_MONGO = 0x8b966093,			// "never_mongo"
		vALWAYS_MONGO = 0x16c6df0b,			// "always_mongo"
		vMONGO_WHEN_SWITCH = 0x0cdba4f9		// "mongo_when_switch"
	};

	enum	EStat
	{
		STATS_AIR = 0,
		STATS_RUN,
		STATS_OLLIE,
		STATS_SPEED,
		STATS_SPIN,
		STATS_FLIPSPEED,
		STATS_SWITCH,
		STATS_RAILBALANCE,
		STATS_LIPBALANCE,
		STATS_MANUAL,	
			   
		NUM_ESTATS
	};

///////////////////////////////////////////////////////////////////////////////////////////////	
// Player specific	
public:
	void						UpdateStats( Obj::CSkaterProfile* pSkaterProfile );
	void 						UpdateSkaterInfo( Obj::CSkaterProfile* pSkaterProfile );
	float						GetStat(EStat stat);
	void						SetStat(EStat stat, float value);
	float 						GetStattedValue(const char *pName, EStat stat);
	float 				  	  	GetScriptedStat(Script::CStruct *pSS);
	float						GetScriptedStat(const uint32 checksum);
	float 						GetScriptedStat(uint32 Checksum, Script::CStruct *pSetOfStats);
	// float						GetHeight();
							   
	// Here's some data that needs to be stored
	// from the skater profile, in case various
	// scripts ask for it
	uint32	m_pushStyle;
	bool	m_isGoofy;
	bool	m_isPro;
	int		m_isMale;

	enum
	{
		MAX_LEN_FIRST_NAME = 64
	};

	char	m_firstName[MAX_LEN_FIRST_NAME];

	// Name of the skater, eg Hawk, Caballero, etc.
	// Grabbed from the skater profile.
	uint32	m_skaterNameChecksum;
	Str::String	m_displayName;

protected:
	
	CSkaterModelRestorationInfo m_model_restoration_info;
	
public :
	CSkater ( int player_num, CBaseManager* p_obj_man, bool local_client, int obj_id, int heap_num, bool should_tristrip);
	virtual						~CSkater ( void );

	// void HideForReplayPlayback();
	// void RestoreAfterReplayPlayback();

///////////////////////////////////////////////////////////////////////////
// Sub Games
	
public:
	bool						m_always_special;

///////////////////////////////////////////////////////////////////////////////////////////////
// Kind of redundant	
public:
	void						RemoveFromCurrentWorld( void );
	void						AddToCurrentWorld ( void );
	bool						IsInWorld( void );
	
///////////////////////////////////////////////////////////////////////////////////////////
// fundamental  to both skater and physics	
public:
	virtual void 				Pause();
	virtual void 				UnPause();
	virtual bool				CallMemberFunction( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript ); 
	virtual void 				Init(Obj::CSkaterProfile* pSkaterProfile);

protected :
	void 						Construct(Obj::CSkaterProfile* pSkaterProfile);
	bool _function0( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript );
	bool _function1( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript );

///////////////////////////////////////////////////////////////////////////////////
// Network

public:
	void	Resync( void );
protected:

	int							m_last_restart_node;

////////////////////////////////////////////////////////////////////////////////////////////
// Rendering related
public:
	Mth::Matrix&				GetDisplayMatrix( void );
	void						SetDisplayMatrix( Mth::Matrix& matrix );
	
protected:
	// True if sparks will be automatically switched off when
	// the skater comes off a rail.
	bool mSparksRequireRail;
	bool mSparksOn;
	void SparksOff();
	void SparksOn();

	// The skater's shadow will move beneath him (if directional) when he is in the air.
	int			m_air_timer;
	
//////////////////////////////////////////////////////////////////////////////////////////////
// high level accessors	
public:
	int							GetHeapIndex( void );

////////////////////////////////////////////////////////////////////////////////////////////////
// Level gameplay related
public:
	void						MoveToRandomRestart( void );
	virtual void				Reset();
	virtual void				SkipToRestart(int node, bool walk=false);
	virtual void				MoveToRestart(int node);

// should be private:	
	// Dan: I'd like to remove this, but it's still used by CEmiter, which is skater specific and does not
	// currently use ProximTriggerComponents
	bool						m_inside;		// flag set up by proxim node triggering


protected:


////////////////////////////////////////////////////////////////////////////////////////////////
// Misc
public:
	Lst::Node< CSkater >*		GetLinkNode( void );

//////////////////////////////////////////////////////////////////////////////
// Tricks 
public:	
    // CreateATrick 
    Game::CCreateATrick* m_created_trick[Game::vMAX_CREATED_TRICKS];
    void					AddCATInfo(Script::CStruct *pStuff);
    void					LoadCATInfo(Script::CStruct *pStuff);
    int                     m_num_cat_animations_on;
    bool                    m_cat_bail_done;
    bool                    m_cat_flip_skater_180;
    bool                    m_cat_animations_done;
    bool                    m_cat_rotations_done;
    float                   m_cat_hold_time;
    int                     m_cat_total_x;
    int                     m_cat_total_y;
    int                     m_cat_total_z;
    // Stat Goals
    void					AddStatGoalInfo(Script::CStruct *pStuff);
    void					LoadStatGoalInfo(Script::CStruct *pStuff);
    // Chapter Status
    void					AddChapterStatusInfo(Script::CStruct *pStuff);
    void					LoadChapterStatusInfo(Script::CStruct *pStuff);
	
public:
////////////////////////////////////////////////////////////////////////////////
// script

	void	JumpToScript(uint32 ScriptChecksum, Script::CStruct *pParams=NULL);
	void	JumpToScript(const char *pScriptName, Script::CStruct *pParams=NULL);

protected :
	
	void UpdateCamera(bool instantly = false);
	void UpdateCameras( bool instantly );
	
public:
	
	void DeleteResources(void);
	void ResetPhysics(bool walk=false,bool in_cleanup=false);
	virtual void ResetStates();
	virtual void ResetAnimation();		  
	virtual void ResetInitScript(bool walk, bool in_cleanup);

protected:
	// Moved this from the CMovingObject class
	// since we're trying to remove functionality
	// from the moving object class, and because
	// the skater was the only thing that was
	// calling it...
	void MovingObj_Reset( void );

public:	
	void UpdateShadow(const Mth::Vector& pos, const Mth::Matrix& matrix);
	// K: Added for use by replay code so that it can playback pad vibrations
	SIO::Device *GetDevice();

protected:

	Lst::Node< CSkater >		m_link_node;			// linked list node for list of skaters

	// pointers to oft used components of the skater
	CAnimationComponent*			mp_animation_component;
	CModelComponent*				mp_model_component;
	CShadowComponent*				mp_shadow_component;
	CTrickComponent*				mp_trick_component;
	CVibrationComponent*			mp_vibration_component;
	CTriggerComponent*				mp_trigger_component;
	CInputComponent*				mp_input_component;
	CSkaterSoundComponent*			mp_skater_sound_component;
	CSkaterGapComponent*			mp_skater_gap_component;
	CSkaterRotateComponent*			mp_skater_rotate_component;
	CSkaterScoreComponent*			mp_skater_score_component;
	CSkaterStateHistoryComponent*	mp_skater_state_history_component;
	CSkaterCorePhysicsComponent*	mp_skater_core_physics_component;
	CSkaterPhysicsControlComponent* mp_skater_physics_control_component;
	CWalkComponent*					mp_walk_component;
    CStatsManagerComponent*			mp_stats_manager_component;
	
public:
	// allow external code rapid access to these components
	CSkaterStateHistoryComponent*	GetStateHistory (   )
	{
		Dbg_Assert(mp_skater_state_history_component);
		return mp_skater_state_history_component;
	}
	
protected:
	
	float						m_stat[NUM_ESTATS];	 	// all the stats

	int							m_heap_index;			// Which skater heap is this skater using
											 
	bool						m_local_client;			// Is this cient being controlled by my PS2

	
	int							m_skater_number;		// 0 or 1 (maybe 2 or 3) 	  

	int							m_viewing_mode;					// 0 = normal, 1 = frozen, move cam, 2 = move, freeze cam
	
public:	
	int							GetViewingMode (   ) { return m_viewing_mode; }
	
protected:	
	// is the skater in the world?
	bool						m_in_world;
	
	// For custom restart point
	
	Mth::Vector					m_restart_pos;
	Mth::Matrix					m_restart_matrix;
	bool						m_restart_walking;
	bool						m_restart_valid;
	
public:	

	const char*	GetDisplayName();
	void		SetAlwaysSpecial(bool on) {m_always_special = on;}
	void		ToggleAlwaysSpecial() {m_always_special = !m_always_special;}

	// this used to be in the skater profile, but I changed it
	// because it was confusing when called from multiplayer games.
	bool		SkaterEquals( Script::CStruct* pParams );

public:
	// Returns a pointer to this skater's score object.
	Mdl::Score *GetScoreObject();
	bool	IsLocalClient() {return m_local_client;}

	int		GetSkaterNumber() {return m_skater_number;}
	
	Gfx::Camera *GetActiveCamera( void );
	
	CCompositeObject* CSkater::GetCamera (    );

	void    SetViewMode( EViewMode view_mode );
	int		GetViewMode() { return m_viewing_mode; }
	
	// hack to allow the collision radius to be adjusted while in a car
	float	m_rigid_body_collision_radius_boost;
	void	SetRigidBodyCollisionRadiusBoost ( float rigid_body_collision_radius_boost ) { m_rigid_body_collision_radius_boost = rigid_body_collision_radius_boost; }
	void	ResetRigidBodyCollisionRadiusBoost (   ) { m_rigid_body_collision_radius_boost = 0.0f; }
	float	GetRigidBodyCollisionRadiusBoost (   ) { return m_rigid_body_collision_radius_boost; }

	friend class CSkaterCam;
	
	friend class CSkaterCameraComponent;
	friend class CTrickComponent;
	friend class CSkaterLoopingSoundComponent;
	friend class CSkaterSoundComponent;
	friend class CSkaterGapComponent;
	friend class CSkaterRotateComponent;
	friend class CSkaterLocalNetLogicComponent;
	friend class CSkaterNonLocalNetLogicComponent;
	friend class CSkaterScoreComponent;
	friend class CSkaterMatrixQueriesComponent;
	friend class CSkaterStateHistoryComponent;
	friend class CSkaterPhysicsControlComponent;
	friend class CSkaterCorePhysicsComponent;
	friend class CSkaterAdjustPhysicsComponent;
	friend class CSkaterFinalizePhysicsComponent;
	friend class CSkaterCleanupStateComponent;
	friend class CSkaterFlipAndRotateComponent;
	
private:
	// skater physics variables beyond those inherited from CCompositeObject
};

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/
extern bool DebugSkaterScripts;

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

int GetPhysicsInt(const char *p_name, Script::EAssertType assert = Script::NO_ASSERT);
int GetPhysicsInt(uint32 id, Script::EAssertType assert = Script::NO_ASSERT);
float GetPhysicsFloat(const char *p_name, Script::EAssertType assert = Script::NO_ASSERT);
float GetPhysicsFloat(uint32 id, Script::EAssertType assert = Script::NO_ASSERT);

} // namespace Obj

#endif	// __OBJECTS_SKATER_H
