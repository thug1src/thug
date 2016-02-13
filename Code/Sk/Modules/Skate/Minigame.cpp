#include <core/defines.h>

#include <sk/modules/skate/Minigame.h>
#include <sk/modules/skate/goalmanager.h>
#include <sk/scripting/cfuncs.h>
#include <sk/scripting/skfuncs.h>
#include <sk/scripting/Nodearray.h>
#include <gel/scripting/struct.h>

#include <gfx/2D/ScreenElemMan.h>       // for minigame timer
#include <gfx/2D/ScreenElement2.h>
#include <gfx/2D/TextElement.h>

namespace Game
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CMinigame::CMinigame( Script::CStruct* pParams ) : CGoal( pParams )
{
	Script::CArray *pGeomArray;

	// Create the associated pieces of geometry
    if ( pParams->GetArray( "create_geometry", &pGeomArray ) )
    {
		uint32 i, geometry;

		for( i = 0; i < pGeomArray->GetSize(); i++ )
        {
            geometry = pGeomArray->GetChecksum( i );
			CFuncs::ScriptCreateFromNodeIndex( SkateScript::FindNamedNode( geometry ));
        }
	}
	
	m_record = 0;
	m_cashLimit = 0;
	pParams->GetInteger( "cash_limit", &m_cashLimit, Script::NO_ASSERT );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CMinigame::~CMinigame()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CMinigame::Activate()
{
	if ( !IsActive() && ShouldUseTimer() )
	{
		CGoalManager* pGoalManager = Game::GetGoalManager();
		Dbg_Assert( pGoalManager );
		pGoalManager->DeactivateMinigamesWithTimer();
	}
	return CGoal::Activate();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CMinigame::Update()
{
	if ( IsActive() && ShouldUseTimer() )
	{
		UpdateMinigameTimer();
	}
	return CGoal::Update();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CMinigame::ShouldUseTimer()
{
	return ( mp_params->ContainsComponentNamed( "time" ) );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// this is used for minigames that have their own timer.
void CMinigame::UpdateMinigameTimer()
{
	return;
/*	
	Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();

	Tmr::Time time_left = m_timeLeft / 1000;		
	if( time_left < 1 )
		time_left = 0;
	
	int seconds = time_left % 60;
	int minutes = time_left / 60;
		
	Front::CScreenElementPtr p_element = p_screen_elem_man->GetElement( CRCD( 0xcd9671b4, "minigame_timer") );
	if (p_element)
	{
		Dbg_MsgAssert(p_element->GetType() == CRCD(0x5200dfb6, "TextElement"), ("type is 0x%x", p_element->GetType()));
		Front::CTextElement *p_time_element = (Front::CTextElement *) p_element.Convert();
	
		char time_text[128];
		sprintf(time_text, "%2d:%.2d", minutes, seconds);
		
		// get the description if there is one
		const char* timer_description;
		if ( mp_params->GetString( CRCD( 0xe1fc4f74, "timer_description" ), &timer_description, Script::NO_ASSERT ) )
		{
			strcat( time_text, " " );
			strcat( time_text, timer_description );
		}
		p_time_element->SetText(time_text);
	}
*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CMinigame::IsExpired()
{
	return ( ShouldUseTimer() && ( (int)m_timeLeft <= 0 ) );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CMinigame::CheckRecord( int value )
{
	if ( value > m_record )
	{
		m_record = value;
		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CMinigame::AddTime( int amount )
{
	if ( IsActive() )
	{
		m_timeLeft += (Tmr::Time)( amount * 1000 );
		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CMinigame::GetRecord()
{
	mp_params->AddInteger( "minigame_record", m_record );
	return m_record;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CMinigame::CanRetry()
{
	// you cannot retry minigames - ie, they don't show up 
	// on the pause menu as "Retry last goal"
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMinigame::LoadSaveData( Script::CStruct*  pFlags )
{
	// minigame record
	int minigame_record;
	pFlags->GetInteger( "minigame_record", &minigame_record, Script::NO_ASSERT );
	if ( minigame_record != -1 )
	{
		// this will check and set the record
		CheckMinigameRecord( minigame_record );
	}

	int cash_limit;
	if ( pFlags->GetInteger( "cash_limit", &cash_limit, Script::NO_ASSERT ) )
		m_cashLimit = cash_limit;
	CGoal::LoadSaveData( pFlags );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMinigame::GetSaveData( Script::CStruct* pFlags )
{
	int minigame_record = GetMinigameRecord();
	pFlags->AddInteger( "minigame_record", minigame_record );
	pFlags->AddInteger( "cash_limit", m_cashLimit );
	CGoal::GetSaveData( pFlags );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMinigame::AwardCash( int amount )
{
	if ( m_cashLimit <= 0 )
	{
		Script::RunScript( "minigame_cash_depleted" );
		return false;
	}

	// clamp it
	if ( amount > m_cashLimit )
		amount = m_cashLimit;

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	if ( pGoalManager )
	{
		pGoalManager->AddCash( amount );
		m_cashLimit -= amount;
		Script::CStruct* pScriptParams = new Script::CStruct();
		pScriptParams->AddInteger( "amount", amount );
		Script::RunScript( "minigame_got_cash", pScriptParams );
		delete pScriptParams;
		return true;
	}
	return false;
}

}
