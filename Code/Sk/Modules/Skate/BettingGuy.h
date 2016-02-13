// BettingGuy.h
#ifndef _SK_MODULES_SKATE_BETTINGGUY_H_
#define _SK_MODULES_SKATE_BETTINGGUY_H_

#include <sk/modules/skate/goalmanager.h>

#include <gel/scripting/struct.h>

namespace Game
{

class CBettingGuy : public CGoal
{
public:
						CBettingGuy( Script::CStruct* pParams );
	virtual				~CBettingGuy();

	bool				IsActive();
	bool				ShouldUseTimer() { return false; }
	bool				CountAsActive() { return false; }
	bool				Activate();
	void				SetActive();
	bool				Update();
	bool				MoveToNewNode( bool force = false );
	bool				IsExpired();
	bool				Deactivate( bool force = false, bool affect_tree = true );
	bool				Win();
	bool				Win( uint32 goalId );
	bool				Lose();
	void				Reset();

	bool				EndBetAttempt( uint32 goalId );
	bool				StartBetAttempt( uint32 goalId );
	bool				BetIsActive( uint32 goalId );

	void				OfferMade();
	void				OfferRefused();
	void				OfferAccepted();

	Script::CStruct*	GetBetParams();
	void				SetupParams();

protected:
	bool				m_shouldMove;
	int					m_numBetsWon;
	uint32				m_currentMinigame;
	uint32				m_currentNode;
	int					m_currentDifficulty;
	bool				m_hasOffered;
	bool				m_inAttempt;
	int					m_numTries;
	int					m_currentChallenge;
	int					m_betAmount;
	int					m_straightLosses;
	int 				m_straightWins;
};

}	// namespace Game

#endif
