//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterEndRunComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/28/3
//****************************************************************************

#include <sk/components/skaterendruncomponent.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/gamenet/gamenet.h>
#include <sk/objects/skater.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/objtrack.h>

#define	FLAGEXCEPTION(X) GetObject()->SelfEvent(X)

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent* CSkaterEndRunComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterEndRunComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterEndRunComponent::CSkaterEndRunComponent() : CBaseComponent()
{
	SetType( CRC_SKATERENDRUN );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterEndRunComponent::~CSkaterEndRunComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterEndRunComponent::InitFromStructure( Script::CStruct* pParams )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterEndRunComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterEndRunComponent::Update()
{
	if (!Mdl::Skate::Instance()->IsMultiplayerGame())
	{
		Suspend(true);
		return;
	}
	
	if( static_cast< CSkater* >(GetObject())->IsLocalClient())
	{
		if (m_flags.Test(FINISHED_END_OF_RUN) && !GameNet::Manager::Instance()->HaveSentEndOfRunMessage())
		{
			GameNet::Manager::Instance()->SendEndOfRunMessage();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterEndRunComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | EndOfRunDone | 
		case CRCC(0x4c58771e, "EndOfRunDone"):
			m_flags.Set(FINISHED_END_OF_RUN);
			m_flags.Clear(IS_ENDING_RUN);
			break;
		
		case CRCC(0xb866bf9b, "RunStarted"):
			m_flags.Clear(STARTED_END_OF_RUN);
			m_flags.Clear( STARTED_GOAL_END_OF_RUN );
			m_flags.Clear(FINISHED_END_OF_RUN);
			m_flags.Clear( FINISHED_GOAL_END_OF_RUN );
			m_flags.Clear(IS_ENDING_RUN);
			m_flags.Clear(IS_ENDING_GOAL);
			break;

		case CRCC(0x962dea01, "EndOfRunStarted"):
			m_flags.Set(STARTED_END_OF_RUN);
			break;

		case CRCC(0xafc6a7,"Goal_EndOfRunStarted"):
			m_flags.Set(STARTED_GOAL_END_OF_RUN);
			break;
			
		// @script | Goal_EndOfRunDone | 
		case CRCC(0x69a9e37f, "Goal_EndOfRunDone"):
			m_flags.Set(FINISHED_GOAL_END_OF_RUN);
			m_flags.Clear(IS_ENDING_GOAL);
			break;
			
		case CRCC(0x95bdcfcd, "IsInEndOfRun"):
			return (m_flags.Test(STARTED_END_OF_RUN) && !m_flags.Test(FINISHED_END_OF_RUN)) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterEndRunComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterEndRunComponent::GetDebugInfo"));
	
	p_info->AddChecksum(CRCD(0x449233d0, "STARTED_END_OF_RUN"), m_flags.Test(STARTED_END_OF_RUN) ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	p_info->AddChecksum(CRCD(0x90259288, "FINISHED_END_OF_RUN"), m_flags.Test(FINISHED_END_OF_RUN) ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	p_info->AddChecksum(CRCD(0x0d2a8609, "IS_ENDING_RUN"), m_flags.Test(IS_ENDING_RUN) ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	p_info->AddChecksum(CRCD(0x80818017, "STARTED_GOAL_END_OF_RUN"), m_flags.Test(STARTED_GOAL_END_OF_RUN) ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	p_info->AddChecksum(CRCD(0x587092f3, "FINISHED_GOAL_END_OF_RUN"), m_flags.Test(FINISHED_GOAL_END_OF_RUN) ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	p_info->AddChecksum(CRCD(0xcc3b2295, "IS_ENDING_GOAL"), m_flags.Test(IS_ENDING_GOAL) ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterEndRunComponent::EndRun ( bool force_end )
{
	if (force_end || Mdl::Skate::Instance()->GetGameMode()->ShouldStopAtZero())
	{
		if (!m_flags.Test(IS_ENDING_RUN) && !m_flags.Test(FINISHED_END_OF_RUN))
		{
			//DumpUnwindStack( 20, 0 );
			m_flags.Set(IS_ENDING_RUN);
			if (static_cast< CSkater* >(GetObject())->IsLocalClient())
			{
				Script::RunScript(CRCD(0xf4ce3e97, "ForceEndOfRun"), NULL, GetObject());
			}
		}
	}
	else
	{
		//DumpUnwindStack( 20, 0 );
		FLAGEXCEPTION(CRCD(0x822e13a9, "RunHasEnded"));
		m_flags.Clear(FINISHED_END_OF_RUN);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterEndRunComponent::EndGoalRun ( bool force_end )
{
	m_flags.Clear(FINISHED_GOAL_END_OF_RUN);
	FLAGEXCEPTION( CRCD(0xab676175, "GoalHasEnded") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterEndRunComponent::RunHasEnded (   )
{
	return m_flags.Test(FINISHED_END_OF_RUN);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterEndRunComponent::GoalRunHasEnded (   )
{
	return m_flags.Test(FINISHED_GOAL_END_OF_RUN);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterEndRunComponent::StartedEndOfRun (   )
{
	return m_flags.Test(STARTED_END_OF_RUN);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterEndRunComponent::ClearStartedEndOfRunFlag (   )
{
	m_flags.Clear(STARTED_END_OF_RUN);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterEndRunComponent::StartedGoalEndOfRun (   )
{
	return m_flags.Test(STARTED_GOAL_END_OF_RUN);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterEndRunComponent::ClearStartedGoalEndOfRunFlag (   )
{
	m_flags.Clear(STARTED_GOAL_END_OF_RUN);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterEndRunComponent::IsEndingRun (   )
{
	return m_flags.Test(IS_ENDING_RUN);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterEndRunComponent::ClearIsEndingRun (   )
{
	m_flags.Clear(IS_ENDING_RUN);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Flags<int>	CSkaterEndRunComponent::GetFlags( void )
{
	return m_flags;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSkaterEndRunComponent::SetFlags( Flags<int> flags )
{
	m_flags = flags;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}
