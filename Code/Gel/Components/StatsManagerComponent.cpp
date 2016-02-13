//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       StatsManagerComponent.cpp
//* OWNER:          Zac Drake
//* CREATION DATE:  5/29/03
//****************************************************************************

//  This component watches the skater and checks if certain always active goals are met.
//  When these goals are met, the skaters stats are increased. These goals are pulled from
//  the stats_goals script array in stats.q

#include <gel/components/statsmanagercomponent.h>

#include <gel/object/compositeobjectmanager.h>
#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h> 
#include <gel/scripting/checksum.h> 
#include <gel/scripting/component.h>
#include <gel/scripting/utils.h>

#include <core/math.h>
#include <core/math/math.h>
#include <core/flags.h>
#include <sk/modules/skate/score.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/goalmanager.h>
#include <sk/components/skaterstatecomponent.h>
#include <sk/components/skaterphysicscontrolcomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skaterbalancetrickcomponent.h>

#include <sk/objects/playerprofilemanager.h>


namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// s_create is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
// s_create	returns a CBaseComponent*, as it is to be used
// by factor creation schemes that do not care what type of
// component is being created
// **  after you've finished creating this component, be sure to
// **  add it to the list of registered functions in the
// **  CCompositeObjectManager constructor  

CBaseComponent* CStatsManagerComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CStatsManagerComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// All components set their type, which is a unique 32-bit number
// (the CRC of their name), which is used to identify the component	
CStatsManagerComponent::CStatsManagerComponent() : CBaseComponent()
{
	SetType( CRC_STATSMANAGER );
    
    mpSkater= NULL;
    mpSkaterStateComponent = NULL;
    mpSkaterPhysicsControlComponent = NULL;
    mpSkaterCorePhysicsComponent = NULL;

    vert_start_pos.Set( 0.0f, 0.0f, 0.0f, 0.0f );
    vert_end_pos.Set( 0.0f, 0.0f, 0.0f, 0.0f );
    jump_start_pos.Set( 0.0f, 0.0f, 0.0f, 0.0f );
    jump_end_pos.Set( 0.0f, 0.0f, 0.0f, 0.0f );

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CStatsManagerComponent::~CStatsManagerComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CStatsManagerComponent::InitFromStructure( Script::CStruct* pParams )
{
	// ** Add code to parse the structure, and initialize the component
    
    showed_info = false;    // need to grab this from memcard eventually
    stat_goals_on = false;
    
    new_event = NULL;
    last_event = NULL;
    
    vert_set = 0;
    vert_height = 0;
    vert_score = 0;
    vert_start_base=0;
    vert_start_mult=0;
    
    jump_set = 0;
    
    num_fliptricks=0;
    num_grabtricks=0;

    air_combo_score=0;

    m_restarted_this_frame = true;

    // Set p_skater
    uint32 target_id = Obj::CBaseComponent::GetObject()->GetID();
    Dbg_MsgAssert(target_id >=0 && target_id <= 15,("Bad StatsManagerTarget 0x%x in SkaterStatsManagerComponent",target_id));
    
	CSkater* p_skater = static_cast<CSkater*>(CCompositeObjectManager::Instance()->GetObjectByID(target_id)); 
	Dbg_MsgAssert(p_skater,("Can't find skater %d in SkaterStatsManagerComponent",target_id));
    
	SetSkater(p_skater);

    // get difficulty level
    Game::CGoalManager* pGoalManager = Game::GetGoalManager();
    Dbg_Assert( pGoalManager );
    dif_level = (int)pGoalManager->GetDifficultyLevel(true);

    printf("STATSManager: difficulty ====================== %i\n", dif_level );

    // should bump stats?
    int bump = Script::GetInteger( "bump_stats", Script::ASSERT );
    if ( bump == 1 )
    {
        bump_stats=true;
    }

    // reset stats to defaults
    /*Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile( 0 );
    pProfile->SetPropertyValue( CRCD(0x439f4704,"air"), 3 );
    pProfile->SetPropertyValue( CRCD(0x9016b4e7,"switch"), 3 );
    pProfile->SetPropertyValue( CRCD(0xedf5db70,"spin"), 3 );
    pProfile->SetPropertyValue( CRCD(0xaf895b3f,"run"), 3 );
    pProfile->SetPropertyValue( CRCD(0xf0d90109,"speed"), 3 );
    pProfile->SetPropertyValue( CRCD(0xae798769,"lip_balance"), 3 );
    pProfile->SetPropertyValue( CRCD(0xf73a13e3,"rail_balance"), 3 );
    pProfile->SetPropertyValue( CRCD(0xb1fc0722,"manual_balance"), 3 );
    pProfile->SetPropertyValue( CRCD(0x6dcb497c,"flip_speed"), 3 );
    pProfile->SetPropertyValue( CRCD(0x9b65d7b8,"ollie"), 3 );
    */
    
    // get initial values from script
    Script::CArray* p_array = Script::GetArray( "stats_goals", Script::ASSERT );
    Dbg_MsgAssert( p_array->GetType() == ESYMBOLTYPE_STRUCTURE, ( "This function only works on arrays of structures." ) );

    int array_size = p_array->GetSize();
    Script::CStruct* p_struct = NULL;
    
    //for( int index = 0; index < NUM_STATS_GOALS; index++)
    for ( int index = 0; index < array_size; index++ )
    {
        p_struct = p_array->GetStructure( index );
        if ( p_struct->ContainsComponentNamed( "goaltype" ) )
        {
            uint32 goal_value;
            p_struct->GetChecksum( "goaltype", &goal_value, Script::ASSERT );
            stat_goals[index].goaltype = goal_value;
        }
        if ( p_struct->ContainsComponentNamed( "stattype" ) )
        {
            uint32 stat_value;
            p_struct->GetChecksum( "stattype", &stat_value, Script::ASSERT );
            stat_goals[index].stattype = stat_value;
        }
        if ( p_struct->ContainsComponentNamed( "value" ) )
        {
            Script::CArray* value_array = NULL;
            int value_value;
            p_struct->GetArray( "value", &value_array, Script::ASSERT );
            value_value = value_array->GetInteger(dif_level);
            stat_goals[index].value = value_value;
        }
        if ( p_struct->ContainsComponentNamed( "value_string" ) )
        {
            Script::CArray* value_string_array = NULL;
            const char*  value_vstring;
            p_struct->GetArray( "value_string", &value_string_array, Script::ASSERT );
            value_vstring = value_string_array->GetString(dif_level);
            stat_goals[index].v_string = value_vstring;
        }
        if ( p_struct->ContainsComponentNamed( "text" ) )
        {
            const char* text_value="";
            p_struct->GetString( "text", &text_value, Script::ASSERT );
            stat_goals[index].text = text_value;
        }
        if ( p_struct->ContainsComponentNamed( "complete" ) )
        {
            int complete_value;
            p_struct->GetInteger( "complete", &complete_value, Script::ASSERT );
            stat_goals[index].complete = complete_value;
        }
        if ( p_struct->ContainsComponentNamed( "value_trick" ) )
        {
            Script::CArray* value_trick_array = NULL;
            uint32  value_vtrick;
            p_struct->GetArray( "value_trick", &value_trick_array, Script::ASSERT );
            value_vtrick = value_trick_array->GetChecksum(dif_level);
            stat_goals[index].trick = value_vtrick;
        }
        if ( p_struct->ContainsComponentNamed( "value_taps" ) )
        {
            Script::CArray* value_taps_array = NULL;
            uint32  value_vtaps;
            p_struct->GetArray( "value_taps", &value_taps_array, Script::ASSERT );
            value_vtaps = value_taps_array->GetInteger(dif_level);
            stat_goals[index].taps = value_vtaps;
        }
        stat_goals[index].shown = false;
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*void CStatsManagerComponent::Finalize()
{
	// Virtual function, can be overridden to provided finialization to 
	// a component after all components have been added to an object
	// Usually this consists of getting pointers to other components
	// Note:  It is GUARENTEED (at time of writing) that
	// Finalize() will be called AFTER all components are added
	// and BEFORE the CCompositeObject::Update function is called
	
	// Example:
	// mp_suspend_component =  GetSuspendComponentFromObject( GetObject() );
    
}*/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// RefreshFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CStatsManagerComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Default to just calling InitFromStructure()
	// but if that does not handle it, then will need to write a specific 
	// function here. 
	// The user might only want to update a single field in the structure
	// and we don't want to be asserting because everything is missing 
	
	InitFromStructure(pParams);


}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// The component's Update() function is called from the CCompositeObject's 
// Update() function.  That is called every game frame by the CCompositeObjectManager
// from the s_logic_code function that the CCompositeObjectManager registers
// with the task manger.
void CStatsManagerComponent::Update()
{
	// **  You would put in here the stuff that you would want to get run every frame
	// **  for example, a physics type component might update the position and orientation
	// **  and update the internal physics state

    if( mpSkater == NULL )
	{
		return;
	}

    // Return if not in career mode
    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
    if ( !skate_mod->GetGameMode()->IsTrue( CRCD(0x1ded1ea4,"is_career") ) )
    {
        return;
    }
    
    // Return if stat goals turned off
    if ( !stat_goals_on )
    {
        return;
    }
    
    // Return if in frontend
    if (skate_mod->m_cur_level == CRCD(0x9f2bafb7,"load_skateshop"))
    {
        return;
    }

    if (m_restarted_this_frame)
    {
        vert_set = 0;
        jump_set = 0;

        m_restarted_this_frame = false;

        return;
    }
    
    // Get current trick score this frame???
    Mdl::Score *pScore = mpSkater->GetScoreObject();
    Dbg_Assert( pScore );
    current_score = pScore->GetScorePotValue();
    current_mult = pScore->GetCurrentMult();
    if ( current_mult != 0 )
    {
        current_base = (current_score / current_mult);
    }
    else
    {
        current_base = 0;
    }
    
    // Get skater state
    EStateType state = mpSkaterStateComponent->GetState();

    // reset new event
    new_event = CRCD(0x26a7cadf,"other");
    
    // check if skating
    if (mpSkaterPhysicsControlComponent->IsSkating())
    {
        // check for vert airs and landings
        if ( mpSkaterStateComponent->GetFlag( VERT_AIR ) )
        {
            new_event = CRCD(0xb39b4f1b,"vert_air");
            if ( last_event != CRCD(0xb39b4f1b,"vert_air") )
            {
                // vert take off
                vert_set = 1;
                vert_height = 0;
                vert_start_pos = GetObject()->GetPos();
                vert_start_base = current_base;
                vert_start_mult = current_mult;
                printf("vert_start_score = %i\n", current_score );
                vert_score = 0;
            }
            else
            {
                // vert in air
                Mth::Vector new_pos = GetObject()->GetPos();
                if ( ( new_pos[1] - vert_start_pos[1] ) > vert_height )
                {
                    vert_height = ( new_pos[1] - vert_start_pos[1] );
                }

                // the dif in base times the dif in multiplier
                air_combo_score = ((current_base - vert_start_base) * (current_mult - vert_start_mult));

                if ( air_combo_score >= vert_score )
                {
                    vert_score = air_combo_score;
                }
            }
        }
        else
        {
            // landed from vert ( not really landed... could still revert to manual )
            if ( last_event == CRCD(0xb39b4f1b,"vert_air") )
            {
                int air_combo_mult=0;

                if ( vert_set == 1 )
                {
                    vert_end_pos = GetObject()->GetPos();
                    air_combo_score = ((current_base - vert_start_base) * (current_mult - vert_start_mult));
                    air_combo_mult = (current_mult - vert_start_mult);
                    vert_set = 2;
                }

                // air combo messages
                if ( air_combo_mult > 1 )
                {
                    if ( air_combo_score != 0 )
                    {
                        Script::CStruct* p_params = new Script::CStruct();
                        p_params->AddInteger( "score", air_combo_score );
                        Script::RunScript( Script::GenerateCRC( "show_vert_combo_message" ), p_params );
                        delete p_params;
                    }
                }
                air_combo_score=0;
            }
            // not vert related
            else
            {
                vert_set = 0;
                vert_height = 0;
                vert_score = 0;
            }
        }

        // check for jump takeoffs and landings
        if ( (state == AIR) && (!mpSkaterStateComponent->GetFlag( VERT_AIR )) )
        {
            new_event = CRCD(0x439f4704,"air");
            if ( last_event != CRCD(0x439f4704,"air") )
            {
                if ( !mpSkaterStateComponent->GetFlag(IS_BAILING) )
                {
                    jump_set = 1;
                    jump_start_pos = GetObject()->GetPos();
                }
            }
        }
        else
        {
            if ( last_event == CRCD(0x439f4704,"air") )
            {
                if ( jump_set == 1 )
                {
                    jump_end_pos = GetObject()->GetPos();
                    jump_set = 2;
                }
            }
            else
            {
                jump_set = 0;
            }
        }
    }
    else
    {
        jump_set = 0;
    }
    
    // check for triggered goals
    bool beat_one = false;
    for (int i=0; i < NUM_STATS_GOALS; i++)
    {
        // Combo goals
        if ( stat_goals[i].goaltype == CRCD(0x4ec3cfb5,"combo") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {
                    int value = stat_goals[i].value;
                    if ( current_score >= value )
                    {
                        ShowStatsMessage(i);
                        beat_one=true;
                    }
                }
            }
        }

        // Multiplier goals
        if ( stat_goals[i].goaltype == CRCD(0x5b6556a4,"multiplier") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {
                    int value = stat_goals[i].value;
                    if ( current_mult >= value )
                    {
                        ShowStatsMessage(i);
                        beat_one=true;
                    }
                }
            }
        }

        // Vert Distance
        if ( stat_goals[i].goaltype == CRCD(0xc3ec3eed,"vertdist") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {
                    if ( vert_set == 2 )
                    {
                        float value = float(stat_goals[i].value);
                        float vert_dist = 1;
                        
                        // set Y values to zero
                        vert_end_pos[1] = 0;
                        vert_start_pos[1] = 0;
    
                        vert_dist = Mth::Distance(vert_end_pos, vert_start_pos);
                        //printf("vert_dist = %f value = %f \n", (vert_dist/12), value );
                        
                        if ( vert_dist >= (12*value) ) // value is in feet, vert_dist is in inches
                        {
                            ShowStatsMessage(i);
                            beat_one=true;
                        }
                    }
                }
            }
        }

        // Vert Height
        if ( stat_goals[i].goaltype == CRCD(0x430b80c4,"vertheight") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {
                    if ( vert_set != 0 )
                    {
                        float value = float(stat_goals[i].value);
                        
                        if ( vert_height >= (12*value) ) // value is in feet, vert_height is in inches
                        {
                            ShowStatsMessage(i);
                            beat_one=true;
                        }
                    }
                }
            }
        }

        // Vert Score
        if ( stat_goals[i].goaltype == CRCD(0x1dbbb148,"vertscore") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {
                    if ( vert_set == 2 )
                    {
                        int value = stat_goals[i].value;
                        
                        if ( vert_score >= value )
                        {
                            ShowStatsMessage(i);
                            beat_one=true;
                        }
                    }
                }
            }
        }

        // Vert Spin
        if ( stat_goals[i].goaltype == CRCD(0x509aca95,"vertspin") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {
                    if ( vert_set != 0 )
                    {
                        float value = float(stat_goals[i].value);
                        
                        if ( vert_spin >= value )
                        {
                            ShowStatsMessage(i);
                            beat_one=true;
                        }
                    }
                }
            }
        }

        // Jump Distance
        if ( stat_goals[i].goaltype == CRCD(0x8f6f9f05,"olliedist") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {
                    if ( jump_set == 2 )
                    {
                        float value = float(stat_goals[i].value);
                        float jump_dist = 1;
                        Mth::Vector end;
						Mth::Vector start;
						
                        // grab start and end points minus Y values
                        end[0] = jump_end_pos[0];
                        end[1] = 0;
                        end[2] = jump_end_pos[2];
			end[3] = 1.0f;

                        start[0] = jump_start_pos[0];
                        start[1] = 0;
                        start[2] = jump_start_pos[2];
                        start[3] = 1.0f;

                        jump_dist = Mth::Distance(end, start);
                        //printf("jump_dist = %f value = %f \n", (jump_dist/12), value );
                        
                        if ( jump_dist >= (12*value) ) // value is in feet, jump_dist is in inches
                        {
                            ShowStatsMessage(i);
                            beat_one=true;
                        }
                    }
                }
            }
        }

        // Jump Height
        if ( stat_goals[i].goaltype == CRCD(0x39e7537b,"highollie") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {
                    if ( jump_set == 2 )
                    {
                        float value = float(stat_goals[i].value);
                        float jump_high = 1;
                        
                        jump_high = ( jump_end_pos[1] - jump_start_pos[1] );
                        
                        if ( jump_high >= (12*value) ) // value is in feet, jump_high is in inches
                        {
                            ShowStatsMessage(i);
                            beat_one=true;
                        }
                    }
                }
            }
        }

        // Jump Drop
        if ( stat_goals[i].goaltype == CRCD(0x7e064ad0,"olliedrop") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {
                    if ( jump_set == 2 )
                    {
                        float value = float(stat_goals[i].value);
                        float jump_low = 1;
                        
                        jump_low = ( jump_start_pos[1] - jump_end_pos[1] );
                        
                        if ( jump_low >= (12*value) ) // value is in feet, jump_low is in inches
                        {
                            ShowStatsMessage(i);
                            beat_one=true;
                        }
                    }
                }
            }
        }

        // Num Fliptricks
        if ( stat_goals[i].goaltype == CRCD(0x364e8c16,"numfliptricks") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {
                    int value = (stat_goals[i].value);
                    
                    if ( num_fliptricks >= value )
                    {
                        ShowStatsMessage(i);
                        beat_one=true;
                    }
                }
            }
        }

        // Num grabtricks
        if ( stat_goals[i].goaltype == CRCD(0x68703ae9,"numgrabs") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {
                    int value = (stat_goals[i].value);
                    
                    if ( num_grabtricks >= value )
                    {
                        ShowStatsMessage(i);
                        beat_one=true;
                    }
                }
            }
        }

        // String Counter
        if ( stat_goals[i].goaltype == CRCD(0x9068b4e5,"stringcount") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {   
                    int value = (stat_goals[i].value);
                    const char* string = (stat_goals[i].v_string);
                    //printf("%s", string );

                    if ( pScore->CountStringMatches(string) >= value )
                    {
                        ShowStatsMessage(i);
                        beat_one=true;
                    }
                }
            }
        }

        // Trick Counter
        if ( stat_goals[i].goaltype == CRCD(0x69b0f836,"trickcount") )
        {
            if (!stat_goals[i].complete)
            {
                if (!stat_goals[i].shown)
                {   
                    int value = (stat_goals[i].value);
                    uint32 trick = (stat_goals[i].trick);
                    int num_taps = (stat_goals[i].taps);
                    
                    /*int tricktimes = pScore->GetCurrentNumberOfOccurrencesByName(trick, 0, false, num_taps);
                    if ( tricktimes > 0)
                    {
                        printf("CountTrickMatches = %i\n", tricktimes );
                    }*/
                    
                    if ( pScore->GetCurrentNumberOfOccurrencesByName(trick, 0, false, num_taps) >= value )
                    {
                        ShowStatsMessage(i);
                        beat_one=true;
                    }
                }
            }
        }
    }
    
    if (beat_one)
    {
        Script::RunScript( Script::GenerateCRC( "showed_stat_message_sound" ) );
    }

    vert_spin = 0;
    last_event = new_event;
    last_score = current_score;
    last_base = current_base;
    last_mult = current_mult;
    
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the "Checksum" of a script command, then possibly handle it
// if it's a command that this component will handle	
CBaseComponent::EMemberFunctionResult CStatsManagerComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
/*
        // @script | DoSomething | does some functionality
		case 0xbb4ad101:		// DoSomething
			DoSomething();
			break;

        // @script | ValueIsTrue | returns a boolean value
		case 0x769260f7:		// ValueIsTrue
		{
			return ValueIsTrue() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		break;
*/
        
        // @script | StatsManager_ReInit | reinitializes stats manger. used when starting new career/story
        case 0x82ce30ce: // StatsManager_ReInit
		{
			RefreshFromStructure( pParams );
		}
		break;

        // @script | StatsManager_Reset | resets stats manger. used when reinitializing skater
        case 0x57879c96: //StatsManager_Reset
		{
			Bail();
		}
		break;

        // @script | StatsManager_ActivateGoals | activates stats goals
        case 0xee59073c: // StatsManager_ActivateGoals
		{
			stat_goals_on = true;
            Script::RunScript( Script::GenerateCRC( "console_unhide" ) );
		}
		break;

        // @script | StatsManager_DeactivateGoals | deactivates stats goals
        case 0xf1745991: // StatsManager_DeactivateGoals
		{
			stat_goals_on = false;
            Script::RunScript( Script::GenerateCRC( "console_hide" ) );
		}
		break;
        
        // @script | StatsManager_GetGoalStatus | returns status of stats goal with provided index
        case 0x78e7d855: // StatsManager_GetGoalStatus
        {
            int index;
            int status;
            pParams->GetInteger( NONAME, &index, Script::ASSERT );
            status = stat_goals[index].complete;
            pScript->GetParams()->AddInteger( CRCD(0x84ff9ae3,"status"), status );
        }
        break;

        // @script | StatsManager_UnlockAmGoals | unlocks amateur stats goals
        case 0xb341b5a5: // StatsManager_UnlockAmGoals
        {
            for (int i=0; i < NUM_STATS_GOALS; i++)
            {
                if ( stat_goals[i].complete == 2 )
                {
                    stat_goals[i].complete = 0;
                }
            }
        }
        break;

        // @script | StatsManager_UnlockProGoals | unlocks pro stats goals
        case 0xb7fac894: // StatsManager_UnlockProGoals
        {
            for (int i=0; i < NUM_STATS_GOALS; i++)
            {
                if ( stat_goals[i].complete == 3 )
                {
                    stat_goals[i].complete = 0;
                }
            }
        }
        break;

        default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	// the "default" case of the switch statement handles
	// unrecognized functions;  if we make it down here,
	// that means that the component already handled it
	// somehow
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CStatsManagerComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CStatsManagerComponent::GetDebugInfo"));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums

	/*	Example:
	p_info->AddInteger(CRCD(0x7cf2a233,"m_never_suspend"),m_never_suspend);
	p_info->AddFloat(CRCD(0x519ab8e0,"m_suspend_distance"),m_suspend_distance);
	*/
	
// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CStatsManagerComponent::SetSkater( CSkater* p_skater )
{
	mpSkater = p_skater;
	
	if( p_skater == NULL )
	{
		mpSkaterStateComponent = NULL;
        mpSkaterPhysicsControlComponent = NULL;
		//mpSkaterPhysicsControlComponent = NULL;
	}
	else
	{
		mpSkaterStateComponent = GetSkaterStateComponentFromObject(mpSkater);
		Dbg_Assert(mpSkaterStateComponent);

        mpSkaterPhysicsControlComponent = GetSkaterPhysicsControlComponentFromObject(mpSkater);
		Dbg_Assert(mpSkaterPhysicsControlComponent);
		
		// non-local clients will not have a CSkaterPhysicsControlComponent
		//mpSkaterPhysicsControlComponent = GetSkaterPhysicsControlComponentFromObject(mpSkater);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CStatsManagerComponent::SetTrick( char *trick_name, int base_score, bool is_switch )
{
    if ( base_score != last_base_score )
    {
        last_trick_name = trick_name;
//        printf("trick_name = %s\n", trick_name );
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CStatsManagerComponent::CheckTimedRecord( uint32 checksum, float time, bool final )
{
    if ( !(Mdl::Skate::Instance()->GetGameMode()->IsTrue( CRCD(0x1ded1ea4,"is_career") )) )
    {
        return;
    }

    if ( !stat_goals_on )
    {
        return;
    }
    
    bool beat_one = false;
    for (int i=0; i < NUM_STATS_GOALS; i++)
    {
        if (!stat_goals[i].complete)
        {
            if (!stat_goals[i].shown)
            {
                int value = stat_goals[i].value;

                switch ( checksum )
                {
                    case 0x0ac90769: //"noseManual"
                    case 0xef24413b: //"manual"
                        if ( stat_goals[i].goaltype == CRCD(0xd9ffc141,"manualtime") )
                        {
                            if ( time >= value )
                            {
                                ShowStatsMessage(i);
                                beat_one=true;
                            }
                        }
                        break;
                    case 0x255ed86f: //"grind"
                    case 0x8d10119d: //"slide"
                        if ( stat_goals[i].goaltype == CRCD(0xc1336019,"grindtime") )
                        {
                            if ( time >= value )
                            {
                                ShowStatsMessage(i);
                                beat_one=true;
                            }
                        }
                        break;
                    case 0xa549b57b: //"lip"
                        if ( stat_goals[i].goaltype == CRCD(0xb67cff14,"liptime") )
                        {
                            if ( time >= value )
                            {
                                ShowStatsMessage(i);
                                beat_one=true;
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

    if (beat_one)
    {
        Script::RunScript( Script::GenerateCRC( "showed_stat_message_sound" ) );
    }
    
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
                                                                    
void CStatsManagerComponent::Bail()
{
    Script::RunScript( Script::GenerateCRC( "stats_message_bail" ) );

    for (int i=0; i < NUM_STATS_GOALS; i++)
    {
        if (!stat_goals[i].complete)
        {
            if (stat_goals[i].shown)
            {
                stat_goals[i].shown = false;
            }
        }
    }

    vert_set = 0;
    jump_set = 0;
    num_fliptricks=0;
    num_grabtricks=0;
    jump_start_pos = jump_end_pos;
    vert_start_pos = vert_end_pos;
    
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CStatsManagerComponent::Land()
{
    if ( !(Mdl::Skate::Instance()->GetGameMode()->IsTrue( CRCD(0x1ded1ea4,"is_career") )) )
    {
        return;
    }

    if ( !stat_goals_on )
    {
        return;
    }
    
    Script::CStruct* p_params = new Script::CStruct();

    Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile( 0 );

    uint32 stat;
    bool cleared=false;
    
    for (int i=0; i < NUM_STATS_GOALS; i++)
    {
        if (!stat_goals[i].complete)
        {
            if (stat_goals[i].shown)
            {
                if (!cleared)
                {
                    // clear current messages
                    Script::RunScript( Script::GenerateCRC( "stats_message_land" ) );
                    cleared = true;
                }

                // set goal as complete
                stat_goals[i].complete = 1;

                // increase proper stat
                stat = ( stat_goals[i].stattype );

                switch ( stat )
                {
                    case 0xb1fc0722: //"manual_balance"
                        p_params->AddString( "text", Script::GetString( CRCD(0x69f2596,"manual_increase_text") ) );
                        break;
                    case 0xf73a13e3: //"rail_balance"
                        p_params->AddString( "text", Script::GetString( CRCD(0x6e7a996d,"rail_increase_text") ) );
                        break;
                    case 0xae798769: //"lip_balance"
                        p_params->AddString( "text", Script::GetString( CRCD(0x1f1bcdef,"lip_increase_text") ) );
                        break;
                    case 0xf0d90109: //"speed"
                        p_params->AddString( "text", Script::GetString( CRCD(0x1686ebd3,"speed_increase_text") ) );
                        break;
                    case 0x9b65d7b8: //"ollie"
                        p_params->AddString( "text", Script::GetString( CRCD(0xa8386951,"ollie_increase_text") ) );
                        break;
                    case 0x439f4704: //"air"
                        p_params->AddString( "text", Script::GetString( CRCD(0xa523df6c,"air_increase_text") ) );
                        break;
                    case 0x6dcb497c: //"flip_speed"
                        p_params->AddString( "text", Script::GetString( CRCD(0x585887ca,"flip_increase_text") ) );
                        break;
                    case 0x9016b4e7: //"switch"
                        p_params->AddString( "text", Script::GetString( CRCD(0x1a6b74ea,"switch_increase_text") ) );
                        break;
                    case 0xedf5db70: //"spin"
                        p_params->AddString( "text", Script::GetString( CRCD(0xb959cc23,"spin_increase_text") ) );
                        break;
                    case 0xaf895b3f: //"run"
                        p_params->AddString( "text", Script::GetString( CRCD(0xa17ac668,"run_increase_text") ) );
                        break;
                    default:
                        break;
                }

                // display message
                p_params->AddInteger( "got_it", 1 );
                Script::RunScript( Script::GenerateCRC( "show_stats_message" ), p_params );
                
                if ( bump_stats )
                {
                    // update skater stats
                    int value = ( pProfile->GetStatValue( stat ) + 1 );
                	pProfile->SetPropertyValue( stat, value );
                }
            }
        }
    }

    if (cleared)
    {
        printf("updating skater stats\n");
        mpSkater->UpdateStats(pProfile);
        
        if (!showed_info)
        {
            Game::CGoalManager* pGoalManager = Game::GetGoalManager();
            Dbg_Assert( pGoalManager );
            if ( pGoalManager->CanStartGoal() )
            {
                Script::RunScript( Script::GenerateCRC( "beat_first_stat_goal" ) );
                showed_info = true;
            }
        }
        Script::RunScript( Script::GenerateCRC( "stat_play_win_sound" ) );
    }
    
    delete p_params;
    num_fliptricks=0;
    num_grabtricks=0;
    
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CStatsManagerComponent::ShowStatsMessage( int index )
{
    Script::CStruct* p_params = new Script::CStruct();
    p_params->AddString( "string", stat_goals[index].text );
    p_params->AddInteger( "value", stat_goals[index].value );
    if ( stat_goals[index].v_string )
    {
        p_params->AddString( "vstring", stat_goals[index].v_string );
    }
    p_params->AddInteger( "index", index );
    
    Script::RunScript( Script::GenerateCRC( "show_stats_message" ), p_params );
    stat_goals[index].shown = true;
    delete p_params;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CStatsManagerComponent::SetSpin( float angle )
{
    //printf("angle = %f\n", angle);
    vert_spin = angle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CStatsManagerComponent::SetTrickType( uint32 type )
{
    switch ( type )
    {
        case 0xba15fc7f: //"fliptrick"
            num_fliptricks++;
//            printf("num_fliptricks = %i\n", num_fliptricks );
            break;
        case 0xb9ae26d2: //"grabtrick"
            num_grabtricks++;
//            printf("num_grabtricks = %i\n", num_grabtricks );
            break;
        default:
            break;
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CStatsManagerComponent::StatGoalsOn()
{
    stat_goals_on = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CStatsManagerComponent::StatGoalsOff()
{
    stat_goals_on = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CStatsManagerComponent::StatGoalGetStatus(int index)
{
    return (stat_goals[index].complete);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CStatsManagerComponent::StatGoalSetStatus(int index, int value)
{
    stat_goals[index].complete = value;
    if ( value == 1 )
    {
        // This means we must be loading stats, so don't show the first stat message!
        showed_info = true;
    }
}

}
