/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Skate													**
**																			**
**	File name:		Skate/GameFlow.cpp										**
**																			**
**	Created by:		04/27/01	-	gj										**
**																			**
**	Description:	Defines various game flows								**
**																			**
*****************************************************************************/

//
/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <sk/modules/skate/GameFlow.h>
#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Mdl
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGameFlow::CGameFlow( void )
{
	mp_gameFlowScript = NULL;
	mp_tester_script = NULL;
	
	m_requestedScript = 0;
	m_requestNewScript = false;
	m_paused = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGameFlow::~CGameFlow( void )
{
	if ( mp_gameFlowScript )
		delete mp_gameFlowScript;
		
	if (mp_tester_script)
	{
		delete mp_tester_script;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGameFlow::Reset( uint32 scriptChecksum )
{
	m_requestedScript = scriptChecksum;
	m_requestNewScript = true;
	m_paused = false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameFlow::Pause( bool paused )
{
	bool old = m_paused;
	m_paused = paused;
	return old;
}

void CGameFlow::SetTesterScript(uint32 testerScript, Script::CStruct *p_params)
{
	if (!mp_tester_script)
	{
		mp_tester_script=new Script::CScript;
		#ifdef __NOPT_ASSERT__
		mp_tester_script->SetCommentString("Created in CGameFlow::SetTesterScript(...)");
		#endif
	}
		
	mp_tester_script->SetScript(testerScript, p_params);
}

// Returns true if the tester script did exist.
// This is so that the corresponding script command can return true/false, and hence
// messages can say stuff like 'test script killed' or 'no tester script present' if they want.
bool CGameFlow::KillTesterScript()
{
	if (mp_tester_script)
	{
		delete mp_tester_script;
		mp_tester_script=NULL;
		return true;
	}
		
	return false;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGameFlow::Update()
{
	if (mp_tester_script)
	{
		mp_tester_script->Update();
	}	
	

	// GJ:  This keeps us from loading a new script
	// while we're still in the middle of processing the old one
	if ( m_requestNewScript )
	{
		m_requestNewScript = false;

		if ( mp_gameFlowScript )
			delete mp_gameFlowScript;

		mp_gameFlowScript = new Script::CScript;
		mp_gameFlowScript->SetScript( m_requestedScript );
		#ifdef __NOPT_ASSERT__
		mp_gameFlowScript->SetCommentString("Created in CGameFlow::Update()");
		#endif
		
#ifdef __USER_GARY__
		printf("Resetting to script %s %08x!\n", Script::FindChecksumName( m_requestedScript ), (int)mp_gameFlowScript);
#endif
	}

	// if paused, then don't continue the script
	if ( m_paused )
		return;

	if ( mp_gameFlowScript )
	{
		if ( mp_gameFlowScript->Update() == Script::ESCRIPTRETURNVAL_FINISHED )
		{
			delete mp_gameFlowScript;
			mp_gameFlowScript = NULL;
		}

	}
}

} // namespace Game

