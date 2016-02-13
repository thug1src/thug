// CreateATrick.cpp

#include <sk/modules/skate/createatrick.h>
#include <sk/modules/skate/skate.h>

#include <sk/objects/skater.h>

#include <gel/scripting/script.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/symboltable.h>

#include <core/math/math.h>

namespace Game
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCreateATrick::CCreateATrick() 
	
{
	printf("creating a CCreateATrick\n");

    // Other Params
    Script::CStruct* p_other_params = Script::GetStructure( "Default_CAT_other_info", Script::ASSERT );
    mp_other_params = new Script::CStruct();
    mp_other_params->AppendStructure( p_other_params );
    
    // Rotations
    Script::CArray* p_rotations;
    p_rotations = Script::GetArray( "Default_CAT_rotation_info", Script::ASSERT );

    mp_rotations = new Script::CArray();
    mp_rotations->SetSizeAndType( vMAX_ROTATIONS, ESYMBOLTYPE_STRUCTURE );
    Script::CopyArray( mp_rotations, p_rotations );
    

    // Animations
    Script::CArray* p_animations;
    p_animations = Script::GetArray( "Default_CAT_animation_info", Script::ASSERT );

    mp_animations = new Script::CArray();
    mp_animations->SetSizeAndType( vMAX_ANIMATIONS, ESYMBOLTYPE_STRUCTURE );
    Script::CopyArray( mp_animations, p_animations );
    
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCreateATrick::~CCreateATrick()
{
	if ( mp_other_params )
    {
        delete mp_other_params;
    }
    if ( mp_rotations )
    {
        Script::CleanUpArray( mp_rotations );
        delete mp_rotations;
    }
    if ( mp_animations )
    {
        Script::CleanUpArray( mp_animations );
        delete mp_animations;
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetCreateATrickParams | Gets the values for rotations and animations of a particular skater
bool ScriptGetCreateATrickParams(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();

    int trick_index;

    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
	
        pParams->GetInteger( "trick_index", &trick_index, Script::ASSERT );
        Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[trick_index];
    	Dbg_Assert( pCreatedTrick );
    
        //Other params
        pScript->GetParams()->AddStructure( "other_params", pCreatedTrick->mp_other_params );
        
        //Rotation params
        pScript->GetParams()->AddArray( "rotation_info", pCreatedTrick->mp_rotations );
        
        //Animation params
        pScript->GetParams()->AddArray( "animation_info", pCreatedTrick->mp_animations );
        
        return true;
    }
    return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetCreateATrickOtherParams | Gets the values for trick name, score, etc.
bool ScriptGetCreateATrickOtherParams(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();

    int trick_index;

	Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger( "trick_index", &trick_index, Script::ASSERT );
        Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[trick_index];
    	Dbg_Assert( pCreatedTrick );
        
        //Other params
        pScript->GetParams()->AddStructure( "other_params", pCreatedTrick->mp_other_params );
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetCreateATrickRotations | Gets the values for rotations
bool ScriptGetCreateATrickRotations(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();

    int trick_index;

    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger( "trick_index", &trick_index, Script::ASSERT );
        Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[trick_index];
    	Dbg_Assert( pCreatedTrick );
    
        //Rotation params
        pScript->GetParams()->AddArray( "rotation_info", pCreatedTrick->mp_rotations );
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetCreateATrickAnimations | Gets the values for animations
bool ScriptGetCreateATrickAnimations(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();

    int trick_index;

    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger( "trick_index", &trick_index, Script::ASSERT );
        Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[trick_index];
    	Dbg_Assert( pCreatedTrick );
    
        //Animation params
        pScript->GetParams()->AddArray( "animation_info", pCreatedTrick->mp_animations );
    
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetCreateATrickParams | Sets the values for rotations and animations of a particular skater
bool ScriptSetCreateATrickParams(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();

    int trick_index;
    
    Script::CStruct *p_other_params;
    Script::CArray *p_rotation_info;
    Script::CArray *p_animation_info;

    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
    
        pParams->GetInteger( "trick_index", &trick_index, Script::ASSERT );
        //Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[trick_index];
    	//Dbg_Assert( pCreatedTrick );
        
        // make sure the skater actually has created tricks before we try to update them.
        if ( !(pSkater->m_created_trick[1]) )
        {
            printf("Skater has no created tricks!!\n");
            return false;
        }
        else
        {
            //Script::PrintContents( pSkater->m_created_trick[trick_index]->mp_other_params );
        }
        //Dbg_Assert( pSkater->m_created_trick[1] );
    
        //Params
        if(pParams->GetStructure( "other_params", &p_other_params, Script::ASSERT ))
        {
            printf("Storing param info %i\n", trick_index);
            pSkater->m_created_trick[trick_index]->mp_other_params->AppendStructure( p_other_params );
        }
        
        //Rotations
        if(pParams->GetArray( "rotation_info", &p_rotation_info, Script::ASSERT ))
        {
            printf("Storing rotation info %i\n", trick_index);
            Script::CopyArray( pSkater->m_created_trick[trick_index]->mp_rotations, p_rotation_info );
        }
    
        //Animations
        if(pParams->GetArray( "animation_info", &p_animation_info, Script::ASSERT ))
        {
            printf("Storing animation info %i\n", trick_index);
            Script::CopyArray( pSkater->m_created_trick[trick_index]->mp_animations, p_animation_info );
        }
        
            
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetCreateATrickOtherParams | Sets name, score, etc.
bool ScriptSetCreateATrickOtherParams(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();

    
    int trick_index;
    
    Script::CStruct *p_other_params;
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
	
        pParams->GetInteger( "trick_index", &trick_index, Script::ASSERT );
    
        Dbg_Assert( pSkater->m_created_trick[trick_index] );
        
        //Params
        if(pParams->GetStructure( "other_params", &p_other_params, Script::ASSERT ))
        {
            printf("Storing param info %i\n", trick_index);
            // pSkater->m_created_trick[trick_index]->mp_other_params->Clear();
            pSkater->m_created_trick[trick_index]->mp_other_params->AppendStructure( p_other_params );
            //Script::PrintContents(pSkater->m_created_trick[trick_index]->mp_other_params);
        }
    
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetCreateATrickRotations | Sets the values for rotations
bool ScriptSetCreateATrickRotations(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();

    int trick_index;
    
    Script::CArray *p_rotation_info;
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger( "trick_index", &trick_index, Script::ASSERT );
    
        Dbg_Assert( pSkater->m_created_trick[trick_index] );
        
        //Rotations
        if(pParams->GetArray( "rotation_info", &p_rotation_info, Script::ASSERT ))
        {
            printf("Storing rotation info %i\n", trick_index);
            Script::CopyArray( pSkater->m_created_trick[trick_index]->mp_rotations, p_rotation_info );
        }
    
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetCreateATrickAnimations | Sets the values for rotations
bool ScriptSetCreateATrickAnimations(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();

    int trick_index;
    
    Script::CArray *p_animation_info;
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger( "trick_index", &trick_index, Script::ASSERT );
        
        Dbg_Assert( pSkater->m_created_trick[trick_index] );
        
        //Animations
        if(pParams->GetArray( "animation_info", &p_animation_info, Script::ASSERT ))
        {
            printf("Storing animation info %i\n", trick_index);
            Script::CopyArray( pSkater->m_created_trick[trick_index]->mp_animations, p_animation_info );
        }
    
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_SetNumAnims | 
bool ScriptCAT_SetNumAnims(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    int value;

    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger(NONAME, &value, Script::ASSERT );
        pSkater->m_num_cat_animations_on = value;
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_GetNumAnims | 
bool ScriptCAT_GetNumAnims(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pScript->GetParams()->AddInteger( "num_animations_on", pSkater->m_num_cat_animations_on );
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_SetAnimsDone | 
bool ScriptCAT_SetAnimsDone(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    int value;
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger(NONAME, &value, Script::ASSERT );
        if ( value == 0 )
        {
            pSkater->m_cat_animations_done = false;
        }
        else
        {
            pSkater->m_cat_animations_done = true;
        }
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_GetAnimsDone | 
bool ScriptCAT_GetAnimsDone(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        if ( pSkater->m_cat_animations_done )
        {
            pScript->GetParams()->AddInteger( "cat_animations_done", 1 );
        }
        else
        {
            pScript->GetParams()->AddInteger( "cat_animations_done", 0 );
        }
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_SetRotsDone | 
bool ScriptCAT_SetRotsDone(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    int value;
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger(NONAME, &value, Script::ASSERT );
        if ( value == 0 )
        {
            pSkater->m_cat_rotations_done = false;
        }
        else
        {
            pSkater->m_cat_rotations_done = true;
        }
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_GetRotsDone | 
bool ScriptCAT_GetRotsDone(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        if ( pSkater->m_cat_rotations_done )
        {
            pScript->GetParams()->AddInteger( "cat_rotations_done", 1 );
        }
        else
        {
            pScript->GetParams()->AddInteger( "cat_rotations_done", 0 );
        }
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_SetBailDone | 
bool ScriptCAT_SetBailDone(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    int value;
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger(NONAME, &value, Script::ASSERT );
        if ( value == 0 )
        {
            pSkater->m_cat_bail_done = false;
        }
        else
        {
            pSkater->m_cat_bail_done = true;
        }
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_GetBailDone | 
bool ScriptCAT_GetBailDone(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        if ( pSkater->m_cat_bail_done )
        {
            pScript->GetParams()->AddInteger( "bailtimescriptdone", 1 );
        }
        else
        {
            pScript->GetParams()->AddInteger( "bailtimescriptdone", 0 );
        }
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_SetFlipSkater | 
bool ScriptCAT_SetFlipSkater(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    int value;
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger(NONAME, &value, Script::ASSERT );
        if ( value == 0 )
        {
            pSkater->m_cat_flip_skater_180 = false;
        }
        else
        {
            pSkater->m_cat_flip_skater_180 = true;
        }
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_GetFlipSkater | 
bool ScriptCAT_GetFlipSkater(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        if ( pSkater->m_cat_flip_skater_180 )
        {
            pScript->GetParams()->AddInteger( "cat_flip_skater_180", 1 );
        }
        else
        {
            pScript->GetParams()->AddInteger( "cat_flip_skater_180", 0 );
        }
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_SetHoldTime | 
bool ScriptCAT_SetHoldTime(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    float value;

    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetFloat(NONAME, &value, Script::ASSERT );
        pSkater->m_cat_hold_time = value;
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_GetHoldTime | 
bool ScriptCAT_GetHoldTime(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pScript->GetParams()->AddFloat( "cat_hold_time", pSkater->m_cat_hold_time );
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_SetTotalY | 
bool ScriptCAT_SetTotalY(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    int value;
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger(NONAME, &value, Script::ASSERT );
        pSkater->m_cat_total_y = value;
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_GetTotalY | 
bool ScriptCAT_GetTotalY(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pScript->GetParams()->AddInteger( "total_Y_angle", pSkater->m_cat_total_y );
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_SetTotalX | 
bool ScriptCAT_SetTotalX(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    int value;
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger(NONAME, &value, Script::ASSERT );
        pSkater->m_cat_total_x = value;
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_GetTotalX | 
bool ScriptCAT_GetTotalX(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pScript->GetParams()->AddInteger( "total_X_angle", pSkater->m_cat_total_x );
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_SetTotalZ | 
bool ScriptCAT_SetTotalZ(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    int value;
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pParams->GetInteger(NONAME, &value, Script::ASSERT );
        pSkater->m_cat_total_z = value;
        
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptCAT_GetTotalZ | 
bool ScriptCAT_GetTotalZ(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    
    Obj::CSkater* pSkater = pSkate->GetLocalSkater();
    if ( pSkater )
    {
        pScript->GetParams()->AddInteger( "total_Z_angle", pSkater->m_cat_total_z );
        return true;
    }
    return false;
}

 
}
