//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       StatsManagerComponent.h
//* OWNER:          Zac Drake
//* CREATION DATE:  5/29/03
//****************************************************************************

#ifndef __COMPONENTS_STATSMANAGERCOMPONENT_H__
#define __COMPONENTS_STATSMANAGERCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <sk/modules/skate/score.h>

#include <sk/objects/skater.h>

// Replace this with the CRCD of the component you are adding
#define		CRC_STATSMANAGER CRCD(0x4ff8dac6,"StatsManager")

//  Standard accessor macros for getting the component either from within an object, or 
//  given an object				 
#define		GetStatsManagerComponent() ((Obj::CStatsManagerComponent*)GetComponent(CRC_STATSMANAGER))
#define		GetStatsManagerComponentFromObject(pObj) ((Obj::CStatsManagerComponent*)(pObj)->GetComponent(CRC_STATSMANAGER))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
    class CSkaterStateComponent;
    class CSkaterPhysicsControlComponent;

const static int NUM_STATS_GOALS=70;

struct stat_goal {
    uint32 goaltype;
    uint32 stattype;
    int value;
    bool shown;
    int complete;
    const char* text;
    const char* v_string;
    uint32 trick;
    int taps;
};


class CStatsManagerComponent : public CBaseComponent
{
public:
    CStatsManagerComponent();
    virtual ~CStatsManagerComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
	//virtual	void 					Finalize();
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
    void							SetSkater( CSkater* p_skater );

    void							SetTrick( char *trick_name, int base_score, bool is_switch );

    void                            CheckTimedRecord( uint32 checksum, float time, bool final );
    void                            Bail();
    void                            Land();
    void                            SetSpin( float angle );
    void                            SetTrickType( uint32 type );
    
    int                             StatGoalGetStatus(int index);
    void                            StatGoalSetStatus(int index, int value);

    void                            StatGoalsOn();
    void                            StatGoalsOff();
    
    bool                            m_restarted_this_frame;
    
private:
    
    stat_goal                       stat_goals[NUM_STATS_GOALS];
    bool                            stat_goals_on;
    bool                            bump_stats;
    bool                            showed_info;

    int                             current_score;
    int                             last_score;
    
    int                             current_base;
    int                             last_base;

    int                             current_mult;
    int                             last_mult;

    char                            *last_trick_name;
    int                             last_base_score;

    int                             dif_level;

    uint32                          new_event;
    uint32                          last_event;

    Mth::Vector                     vert_start_pos;
    Mth::Vector                     vert_end_pos;
    int                             vert_start_base;
    int                             vert_start_mult;
    int                             vert_score;
    int                             vert_set;
    float                           vert_height;

    float                           vert_spin;

    Mth::Vector                     jump_start_pos;
    Mth::Vector                     jump_end_pos;
    int                             jump_set;

    int                             num_fliptricks;
    int                             num_grabtricks;

    int                             air_combo_score;

    CSkater*						mpSkater;
    CSkaterStateComponent*			mpSkaterStateComponent;
    CSkaterPhysicsControlComponent* mpSkaterPhysicsControlComponent;
    //CSkaterScoreComponent*			mpSkaterScoreComponent;
    CSkaterCorePhysicsComponent*	mpSkaterCorePhysicsComponent;

    void                            ShowStatsMessage( int index );
    
};

}

#endif
