//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterStateHistoryComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/13/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERSTATEHISTORYCOMPONENT_H__
#define __COMPONENTS_SKATERSTATEHISTORYCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <sk/objects/skater.h>
#include <gfx/nxflags.h>

#define		CRC_SKATERSTATEHISTORY CRCD(0x18223fd6, "SkaterStateHistory")

#define		GetSkaterStateHistoryComponent() ((Obj::CSkaterStateHistoryComponent*)GetComponent(CRC_SKATERSTATEHISTORY))
#define		GetSkaterStateHistoryComponentFromObject(pObj) ((Obj::CSkaterStateHistoryComponent*)(pObj)->GetComponent(CRC_SKATERSTATEHISTORY))

namespace Script
{
    class CScript;
    class CStruct;
}

namespace Net
{
	class MsgHandlerContext;
}
              
namespace Obj
{

class SPosEvent
{   
public:
	SPosEvent( void );

	uint32			GetTime( void );
	void			SetTime( uint32 time );

	short			ShortPos[3];
	Mth::Matrix		Matrix;
	Mth::Vector		Position;
	Mth::Vector		Eulers;
	Flags< int >	SkaterFlags;
	Flags< int >	EndRunFlags;
	int				State;
	char			DoingTrick;
	char			Walking;
	char			Driving;
	uint16			LoTime;
	uint16			HiTime;
	ETerrainType	Terrain;
	sint16			RailNode;
};

class SAnimEvent
{   
public:
	SAnimEvent( void );

	uint32			GetTime( void );
	void			SetTime( uint32 time );

	char			m_MsgId;
	char			m_ObjId;
	char			m_LoopingType;
	char			m_Flags;
	uint16			m_LoTime;
	uint16			m_HiTime;
	bool			m_Flipped;
	bool			m_Rotate;
	bool			m_Hide;
	float			m_Alpha;
	float			m_StartTime;
	float			m_EndTime;
	float			m_BlendPeriod;
	float 			m_Speed;
	uint32			m_Asset;
	uint32			m_Bone;
	float			m_WobbleAmpA;
	float			m_WobbleAmpB;
	float			m_WobbleK1;
	float			m_WobbleK2;
	float			m_SpazFactor;
	int 			m_Duration;
	int 			m_SinePower;
	int				m_Index;
	float 			m_StartAngle;
	float 			m_DeltaAngle;
	bool			m_HoldOnLastAngle;
};

class CSkaterStateHistoryComponent : public CBaseComponent
{
public:
	enum
	{
		vNUM_POS_HISTORY_ELEMENTS = 20,
		vNUM_ANIM_HISTORY_ELEMENTS = 20
	};
	
public:
    CSkaterStateHistoryComponent();
    virtual ~CSkaterStateHistoryComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }
	
	static int						sHandleCollision ( Net::MsgHandlerContext* context );
	static int						sHandleProjectileHit ( Net::MsgHandlerContext* context );
	
	SPosEvent*						GetPosHistory (   ) { return mp_pos_history; }
	SPosEvent*						GetLatestPosEvent (   ) { return &mp_pos_history[m_num_pos_updates % vNUM_POS_HISTORY_ELEMENTS]; }
	SPosEvent*						GetLastPosEvent (   ) { return &mp_pos_history[(m_num_pos_updates + ( vNUM_POS_HISTORY_ELEMENTS - 1 )) % vNUM_POS_HISTORY_ELEMENTS]; }
	void							IncrementNumPosUpdates (   ) { m_num_pos_updates++; }
	void							ResetPosHistory (   ) { m_num_pos_updates = 0; }
	int								GetNumPosUpdates (   ) { return m_num_pos_updates; }
	
	uint32							GetLatestAnimTimestamp( void );
	void							SetLatestAnimTimestamp( uint32 timestamp );
	SAnimEvent*						GetAnimHistory (   ) { return mp_anim_history; }
	SAnimEvent*						GetLatestAnimEvent (   ) { return &mp_anim_history[m_num_anim_updates % vNUM_ANIM_HISTORY_ELEMENTS]; }
	SAnimEvent*						GetLastAnimEvent (   ) { return &mp_anim_history[(m_num_anim_updates + ( vNUM_ANIM_HISTORY_ELEMENTS - 1 )) % vNUM_ANIM_HISTORY_ELEMENTS]; }
	void							IncrementNumAnimUpdates (   ) { m_num_anim_updates++; }
	void							ResetAnimHistory (   ) { m_num_anim_updates = 0; }
	int								GetNumAnimUpdates (   ) { return m_num_anim_updates; }
	
	bool							CheckForCrownCollision (   );
	void							CollideWithOtherSkaters ( int start_index );
	bool							GetCollidingPlayerAndTeam ( Script::CStruct* pParams, Script::CScript* pScript );
	
	void							SetCurrentVehicleControlType ( uint32 control_type ) { m_current_vehicle_control_type = control_type; }
	uint32							GetCurrentVehicleControlType (   ) { return m_current_vehicle_control_type; }
	
private:
	Mth::Vector						get_latest_position (   );
	Mth::Vector						get_last_position (   );
	Mth::Vector						get_vel (   );
	int								get_time_between_last_update (   );
	
	float							get_collision_cylinder_coeff ( bool driving );
	float							get_collision_cylinder_radius ( bool first_driving, bool second_driving );
	
private:
	int								m_num_pos_updates;
	SPosEvent						mp_pos_history [ vNUM_POS_HISTORY_ELEMENTS ];
	
	int								m_num_anim_updates;
	SAnimEvent						mp_anim_history [ vNUM_ANIM_HISTORY_ELEMENTS ];

        uint32							m_last_anm_time;
	// if the non-local client is driving, this control type's model will be used; set via a network message
	uint32							m_current_vehicle_control_type;
};

}

#endif
