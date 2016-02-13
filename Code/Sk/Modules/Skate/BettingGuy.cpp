// BettingGuy.cpp

#include <sk/modules/skate/bettingguy.h>
#include <sk/modules/skate/goalmanager.h>
#include <sk/modules/skate/skate.h>

#include <sk/objects/skaterprofile.h>
#include <sk/objects/skater.h>
#include <sk/objects/restart.h>
#include <sk/objects/movingobject.h>
#include <sk/objects/ped.h>

#include <gel/object.h>
#include <gel/objtrack.h>
#include <gel/object/compositeobjectmanager.h>

#include <gel/scripting/utils.h>
#include <gel/scripting/symboltable.h>

#include <sk/scripting/nodearray.h>

#include <core/math/math.h>

namespace Game
{

const int vMINSQUAREDIST		= 1000001;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBettingGuy::CBettingGuy( Script::CStruct* pParams ) 
	: CGoal( pParams )
{
	printf("creating a CBettingGuy\n");
	Reset();
	m_shouldMove = true;
	m_currentMinigame = 0;
	m_straightWins = 0;
	m_straightLosses = 0;
	m_currentDifficulty = 1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBettingGuy::~CBettingGuy()
{
	// nothing yet
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBettingGuy::Activate()
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	CGoal* pGoal = pGoalManager->GetGoal( m_currentMinigame );
	Dbg_Assert( pGoal );

	Script::CStruct* pGoalParams = pGoal->GetParams();

	// get the bet amount
	int bet_amount;
	if ( pGoalParams->GetInteger( "bet_amount", &bet_amount, Script::NO_ASSERT ) )
	{
		m_betAmount = bet_amount;
	}

	SetActive();
	printf("activated!\n");
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBettingGuy::SetActive()
{
	// necessary?
	m_active = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBettingGuy::BetIsActive( uint32 betId )
{
	// printf("betid: %x, current: %x\n", betId, m_currentMinigame );
	return ( betId == m_currentMinigame && IsActive() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBettingGuy::IsActive()
{
	return ( m_hasOffered && !m_shouldMove );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBettingGuy::Update()
{
	// do we need to pick a new minigame?
	if ( m_shouldMove )
	{
		// printf("I need to move\n");
		MoveToNewNode();
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBettingGuy::IsExpired()
{
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBettingGuy::Deactivate( bool force, bool affect_tree )
{
	// no more minigame associated with this
	Reset();
	m_straightWins = 0;
	m_straightLosses = 0;
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// win function that gets called from a trickscript
// checks that the calling minigame is the current minigame
bool CBettingGuy::Win( uint32 goalId )
{
	if ( IsActive() && m_currentMinigame == goalId )
	{
		// this is the right minigame!
		return Win();
	}
	return false;
}

// win func that gets called automatically by update endbetattempt func
bool CBettingGuy::Win()
{	
	printf("CBettingGuy::Win\n");
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->AddCash( m_betAmount );
	Reset();

	RunCallbackScript( vSUCCESS );
	m_numBetsWon++;
	m_straightWins++;
	if ( m_straightWins > 3 && m_currentDifficulty < 3 )
	{
		m_currentDifficulty++;
		m_straightWins = 0;
	}

	m_straightLosses = 0;
	m_shouldMove = true;

	SetInactive();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBettingGuy::Lose()
{
	printf("CbettingGuy::Lose\n");
	RunCallbackScript( vFAIL );
	Reset();
	m_straightLosses++;
	if ( m_straightLosses > 3 && m_currentDifficulty > 1 )
	{
		m_currentDifficulty--;
		m_straightLosses = 0;
	}

	m_straightWins = 0;
	m_shouldMove = true;
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBettingGuy::Reset()
{
	m_shouldMove = false;
	// m_currentMinigame = 0;
	m_numTries = 0;
	// m_currentDifficulty = 1;
	m_hasOffered = false;
	m_inAttempt = false;
	m_currentChallenge = 0;
	m_betAmount = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBettingGuy::MoveToNewNode( bool force )
{
	CGoalManager* pGoalManager = Game::GetGoalManager();
	Dbg_Assert( pGoalManager );
	uint32 minigameId = pGoalManager->GetRandomBettingMinigame();
	if ( minigameId == 0 || minigameId == m_currentMinigame )
		return false;	
			
	uint32 node_checksum;
	Script::CStruct* p_minigame_params = pGoalManager->GetGoal( minigameId )->GetParams();
	Dbg_Assert( p_minigame_params );
	p_minigame_params->GetChecksum( "betting_guy_node", &node_checksum, Script::ASSERT );

	if ( force )
	{
		// move him!
		// printf("moving betting guy\n");

		// record his suspend state and turn off suspension
		Reset();
		m_currentMinigame = minigameId;
		SetupParams();

		uint32 betting_guy_id;
		mp_params->GetChecksum( "betting_guy_id", &betting_guy_id, Script::ASSERT );
		
//		Obj::CMovingObject* p_bet_guy = (Obj::CMovingObject*)(Obj::CMovingObject::m_hash_table.GetItem( betting_guy_id ));
		Obj::CMovingObject* p_bet_guy = (Obj::CMovingObject*)(Obj::CCompositeObjectManager::Instance()->GetObjectByID( betting_guy_id ));
		
		Dbg_Assert( p_bet_guy );

// Mick:  removed this, as suspension handled differently under new components system
//		bool suspend_state = p_bet_guy->m_never_suspend;
//		p_bet_guy->m_never_suspend = true;

		Script::CStruct* p_temp_params = new Script::CStruct();
		p_temp_params->AddChecksum( "goal_id", GetGoalId() );
		p_temp_params->AddChecksum( "new_node", node_checksum );
		Script::RunScript( "betting_guy_move_to_node", p_temp_params );
		delete p_temp_params;

		// make sure he's updated and restore his suspension state
		p_bet_guy->Update();
		
		
//		p_bet_guy->m_never_suspend = suspend_state;
	
		mp_params->AddChecksum( "current_minigame", minigameId );

		m_currentNode = node_checksum;

		return true;
	}

	// printf("got a minigameId of %x\n", minigameId);
	
	// get the local skater's position
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* pSkater = skate_mod->GetLocalSkater();
	Dbg_Assert( pSkater );
	Mth::Vector skater_pos = pSkater->GetPos();
	
	// see if the skater is too close to the betting guy
	Mth::Vector current_pos;
	if ( m_currentNode )
	{
		int current_node = SkateScript::FindNamedNode( m_currentNode );
		SkateScript::GetPosition( current_node, &current_pos );
	
		float d = ( skater_pos - current_pos ).LengthSqr();
		// printf("square distance from the skater to the betting guy: %f\n", d);
		if ( d < vMINSQUAREDIST )
		{
			// printf("the skater is too close to the betting guy\n");
			return false;
		}
	}
	
	// try out the destination node	
	// get the node's position
	int node = SkateScript::FindNamedNode( node_checksum );
	Mth::Vector node_pos;
	SkateScript::GetPosition(node, &node_pos);
	
	// get the square of the distance between them
	float distSqr = ( skater_pos - node_pos ).LengthSqr();
	
	// make sure it isn't too close
	// printf("sqaure distance from skater to next node: %f\n", distSqr);
	if ( distSqr < vMINSQUAREDIST )
	{
		// printf("the skater is too close to the betting guy's destination node\n");
		return false;
	}
	else
	{			
		// move him!
		// printf("moving betting guy\n");
		Reset();
		m_currentMinigame = minigameId;
		SetupParams();

		Script::CStruct* p_temp_params = new Script::CStruct();
		p_temp_params->AddChecksum( "goal_id", GetGoalId() );
		p_temp_params->AddChecksum( "new_node", node_checksum );
		Script::RunScript( "betting_guy_move_to_node", p_temp_params );
		delete p_temp_params;
	
		mp_params->AddChecksum( "current_minigame", minigameId );

		m_currentNode = node_checksum;
		return true;
	}
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBettingGuy::StartBetAttempt( uint32 goalId )
{
	// this acts like a secondary activate
	if ( IsActive() && m_currentMinigame == goalId && !m_inAttempt && !m_shouldMove )
	{
		printf("starting bet attempt\n");
		m_inAttempt = true;
	}
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBettingGuy::EndBetAttempt( uint32 goalId )
{
	// this acts like a secondary deactivate
	if ( IsActive() && m_currentMinigame == goalId && m_inAttempt && !m_shouldMove )
	{
		//printf("CBettingGuy::EndBetAttempt\n");
		// printf("step 2\n");
		m_inAttempt = false;
		
		Script::CStruct* pBetParams = GetBetParams();
		
		// check that they haven't exceeded the number of tries
		int max_attempts;
		pBetParams->GetInteger( "tries", &max_attempts, Script::ASSERT );

		// we're challenging them to break a record
		CGoalManager* pGoalManager = GetGoalManager();
		Dbg_Assert( pGoalManager );
		Script::CStruct* pMinigameParams = pGoalManager->GetGoal( m_currentMinigame )->GetParams();
		Dbg_Assert( pMinigameParams );
		if ( m_currentChallenge )
		{
			// printf("step 3\n");
			int last_attempt;
			if ( pMinigameParams->GetInteger( "last_attempt", &last_attempt, Script::NO_ASSERT ) )
			{
				printf("got a last attempt of %i\n", last_attempt);
				printf("m_currentChallenge is %i\n", m_currentChallenge);
				if ( last_attempt >= m_currentChallenge )
				{						
					// printf("you won the bet!\n");
					// beat the bet and call the minigame's bet_success script
					this->Win();
					return true;
				}
			}
		}
		// check that this isn't their last try
		m_numTries++;
		if ( m_numTries >= max_attempts )
		{
			Lose();
			return true;
		}
		// if it's not a challenge minigame, it's probably a trickspot
		// minigame.  In that case, we'll be told when the game is won
		// the the gap script.
	}
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* CBettingGuy::GetBetParams()
{
	Script::CStruct* p_params = NULL;
	
	if ( m_currentMinigame )
	{		
		CGoalManager* pGoalManager = GetGoalManager();
		Dbg_Assert( pGoalManager );
		
		CGoal* pGoal = pGoalManager->GetGoal( m_currentMinigame );
		Dbg_Assert( pGoal );

		Script::CStruct* p_temp = pGoal->GetParams();
		
		switch ( m_currentDifficulty )
		{
		case 1:
			p_temp->GetStructure( "bet_easy", &p_params, Script::NO_ASSERT );
			break;
		case 2:
			p_temp->GetStructure( "bet_medium", &p_params, Script::NO_ASSERT );
			break;
		case 3:
			p_temp->GetStructure( "bet_hard", &p_params, Script::NO_ASSERT );
			break;
		default:
			Dbg_MsgAssert( 0, ( "Betting game had unknown difficulty level" ) );
		}
	}
	return p_params;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBettingGuy::OfferMade()
{
	m_hasOffered = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBettingGuy::OfferRefused()
{
	m_shouldMove = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBettingGuy::OfferAccepted()
{
}

void CBettingGuy::SetupParams()
{
	Script::CStruct* pBetParams = GetBetParams();
	Dbg_Assert( pBetParams );
	
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	Script::CStruct* p_minigameParams = pGoalManager->GetGoal( m_currentMinigame )->GetParams();
	Dbg_Assert( p_minigameParams );
	
	pBetParams->GetInteger( CRCD(0x1d7750fa,"bet_amount"), &m_betAmount, Script::ASSERT );
	
	mp_params->RemoveComponent( CRCD(0x1d7750fa,"bet_amount") );
	mp_params->AddInteger( CRCD(0x1d7750fa,"bet_amount"), m_betAmount );
	
	mp_params->RemoveComponent( CRCD(0xf3d0c702,"current_challenge") );
	if ( pBetParams->GetInteger( CRCD(0x1931ac3b,"bet_challenge"), &m_currentChallenge, Script::NO_ASSERT ) )
	{
		// m_currentChallenge = current_challenge;
		printf("got a challenge: %i\n", m_currentChallenge);
		mp_params->AddInteger( CRCD(0xf3d0c702,"current_challenge"), m_currentChallenge );
	}

	int max_attempts;
	pBetParams->GetInteger( CRCD(0x62396931,"tries"), &max_attempts, Script::ASSERT );
	mp_params->RemoveComponent( "num_tries" );
	mp_params->AddInteger( CRCD(0x25bad847,"num_tries"), max_attempts );
	
	mp_params->RemoveComponent( CRCD(0x6922090,"bet_unit") );
	mp_params->RemoveComponent( CRCD(0xd41aac2a,"bet_action") );
	mp_params->RemoveComponent( CRCD(0xa1617634,"location") );
	// get the trick string if necessary
	if ( p_minigameParams->ContainsFlag( CRCD(0xbc167894,"trickspot") ) )
	{
		Script::CArray* p_keyCombos;
		pBetParams->GetArray( CRCD(0x79704516,"key_combos"), &p_keyCombos, Script::ASSERT );
		Dbg_MsgAssert( p_keyCombos->GetType() == ESYMBOLTYPE_NAME, ( "Betting guy key_combos array has bad type" ) );
		int key_combo_index = GetRandomIndexFromKeyCombos( p_keyCombos );
		uint32 key_combo = p_keyCombos->GetChecksum( key_combo_index );
	
		uint32 trick_name_checksum;
		// get the global trick mapping
		Mdl::Skate* skate_mod = Mdl::Skate::Instance();
		Obj::CSkaterProfile* p_SkaterProfile = skate_mod->GetCurrentProfile();
		Script::CStruct* p_trickMappings = p_SkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );
		p_trickMappings->GetChecksum( key_combo, &trick_name_checksum, Script::ASSERT );
	
		const char* trick_text;
		if ( trick_name_checksum )
		{
			Script::CStruct* p_trick_structure = Script::GetStructure( trick_name_checksum, Script::ASSERT );
			Script::CStruct* p_trick_params;
			p_trick_structure->GetStructure( CRCD(0x7031f10c,"Params"), &p_trick_params, Script::ASSERT );
			p_trick_params->GetLocalText( CRCD(0xa1dc81f9,"Name"), &trick_text, Script::ASSERT );
		}
		mp_params->AddString( CRCD(0xd41aac2a,"bet_action"), trick_text );
	
		const char* location;
		p_minigameParams->GetString( CRCD(0xa1617634,"location"), &location, Script::ASSERT );
		mp_params->AddString( CRCD(0xa1617634,"location"), location );
	}
	else 
	{
		const char* bet_action;
		// the bet action is global
		p_minigameParams->GetString( CRCD(0xd41aac2a,"bet_action"), &bet_action, Script::ASSERT );
		mp_params->AddString( CRCD(0xd41aac2a,"bet_action"), bet_action );
		const char* bet_unit;
		p_minigameParams->GetString( CRCD(0x6922090,"bet_unit"), &bet_unit, Script::ASSERT );
		mp_params->AddString( CRCD(0x6922090,"bet_unit"), bet_unit );
	}
}

}
