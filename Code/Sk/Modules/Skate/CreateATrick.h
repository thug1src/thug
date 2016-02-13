
// CreateATrick.h
#ifndef _SK_MODULES_SKATE_CREATEATRICK_H_
#define _SK_MODULES_SKATE_CREATEATRICK_H_

#include <gel/scripting/struct.h>

namespace Script
{
    class CStruct;
}

namespace Game
{

enum
{
    vMAX_CREATED_TRICKS=11,
    vMAX_ROTATIONS=6,
    vMAX_ANIMATIONS=6
};

class CCreateATrick
{
public:
						CCreateATrick();
	virtual				~CCreateATrick();

    //char*   m_name;
	//int     m_score;
    //bool    m_rotateafter;

    Script::CStruct *mp_other_params;
    Script::CArray *mp_rotations;
    Script::CArray *mp_animations;

};

// Get
bool ScriptGetCreateATrickParams(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptGetCreateATrickOtherParams(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptGetCreateATrickRotations(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptGetCreateATrickAnimations(Script::CScriptStructure *pParams, Script::CScript *pScript);

// Set
bool ScriptSetCreateATrickParams(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptSetCreateATrickOtherParams(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptSetCreateATrickRotations(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptSetCreateATrickAnimations(Script::CScriptStructure *pParams, Script::CScript *pScript);

// other scripts
bool ScriptCAT_SetNumAnims(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptCAT_GetNumAnims(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptCAT_SetAnimsDone(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptCAT_GetAnimsDone(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptCAT_SetRotsDone(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptCAT_GetRotsDone(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptCAT_SetBailDone(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptCAT_GetBailDone(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptCAT_SetFlipSkater(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptCAT_GetFlipSkater(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptCAT_SetHoldTime(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptCAT_GetHoldTime(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptCAT_SetTotalY(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptCAT_GetTotalY(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptCAT_SetTotalX(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptCAT_GetTotalX(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptCAT_SetTotalZ(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptCAT_GetTotalZ(Script::CScriptStructure *pParams, Script::CScript *pScript);


}	// namespace Game

#endif
