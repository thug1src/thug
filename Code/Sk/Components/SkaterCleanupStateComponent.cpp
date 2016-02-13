//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterCleanupStateComponent.cpp
//* OWNER:        	Dan
//* CREATION DATE:  3/26/3
//****************************************************************************

#include <sk/components/skatercleanupstatecomponent.h>
#include <sk/components/skaterstatecomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/components/suspendcomponent.h>

#include <sys/replay/replay.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterCleanupStateComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterCleanupStateComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterCleanupStateComponent::CSkaterCleanupStateComponent() : CBaseComponent()
{
	SetType( CRC_SKATERCLEANUPSTATE );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterCleanupStateComponent::~CSkaterCleanupStateComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCleanupStateComponent::InitFromStructure( Script::CStruct* pParams )
{
	mp_state_component = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCleanupStateComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCleanupStateComponent::Finalize(   )
{
	mp_state_component = GetSkaterStateComponentFromObject(GetObject());
	
	Dbg_Assert(mp_state_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCleanupStateComponent::Update()
{
	// Make sure the sparks are off if not on a rail
	if (GetSkater()->mSparksRequireRail && mp_state_component->GetState() != RAIL)
	{
		GetSkater()->SparksOff();
	}
		
	// Dan: shouldn't need to do this; the suspend components update themselves
	// Perform LOD/culling/occluding
	// NOTE: Move to CModeldComponent::Update?  Ask Gary about this.
	// GetSuspendComponentFromObject(GetObject())->CheckModelActive();
	
	// Don't update the shadow if running a replay. The replay code will update it, using the replay dummy skater's position and matrix.
	// if (!Replay::RunningReplay())
	// {
	GetSkater()->UpdateShadow(GetObject()->m_pos, GetObject()->m_matrix);
	// }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterCleanupStateComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCleanupStateComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterCleanupStateComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}
	
}
