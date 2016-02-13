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
**	File name:		Skate/VictoryCond.h										**
**																			**
**	Created by:		02/07/01	-	gj										**
**																			**
**	Description:	Defines various victory conditions for a game			**
**																			**
*****************************************************************************/

#ifndef __MODULES_SKATE_VICTORYCOND_H
#define __MODULES_SKATE_VICTORYCOND_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Script
{
    class CStruct;
}
        
namespace Game
{
 
const uint32 vVICTORYCOND_HIGHESTSCORE = 0x8f30fd26;
const uint32 vVICTORYCOND_TARGETSCORE = 0x71cd53a2;
const uint32 vVICTORYCOND_BESTCOMBO = 0x6fad9811;
const uint32 vVICTORYCOND_COMPLETEGOALS = 0x6fc19399;	// complete_goals
const uint32 vVICTORYCOND_LASTMANSTANDING = 0xcc06880f;	// last_man_standing
   
/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class CVictoryCondition : public Spt::Class
{
	

public:
	CVictoryCondition();
	virtual					~CVictoryCondition() {}
	virtual bool			IsWinner( uint32 skater_num ) { return false; }
	virtual bool			IsTerminal() = 0;
	virtual bool			ConditionComplete() = 0;
	virtual bool			PrintDebugInfo() = 0;
};

class CHighestScoreVictoryCondition : public  CVictoryCondition
{
public:
	CHighestScoreVictoryCondition();
	bool					ConditionComplete();
	bool					PrintDebugInfo();
	bool					IsTerminal();
	bool					IsWinner( uint32 skater_num );
};

class CScoreReachedVictoryCondition : public  CVictoryCondition
{
public:
	CScoreReachedVictoryCondition( uint32 target_score );
	bool					ConditionComplete();
	bool					PrintDebugInfo();
	bool					IsTerminal();
	bool					IsWinner( uint32 skater_num );
	int						GetTargetScore( void );
protected:
	uint32				m_TargetScore;
};

bool GoalAttackComplete();

class CGoalsCompletedVictoryCondition : public  CVictoryCondition
{
public:
	CGoalsCompletedVictoryCondition();
	bool					ConditionComplete();
	bool					PrintDebugInfo();
	bool					IsTerminal();
	bool					IsWinner( uint32 skater_num );
};

class CBestComboVictoryCondition : public  CVictoryCondition
{
public:
	CBestComboVictoryCondition( uint32 num_combos );
	bool					ConditionComplete();
	bool					PrintDebugInfo();
	bool					IsTerminal();
	bool					IsWinner( uint32 skater_num );
protected:
	uint32				m_NumCombos;
};

class CLastManStandingVictoryCondition : public CVictoryCondition
{
public:
	CLastManStandingVictoryCondition( void );
	bool					ConditionComplete();
	bool					PrintDebugInfo();
	bool					IsTerminal();
	bool					IsWinner( uint32 skater_num );
};

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/


/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

CVictoryCondition*	CreateVictoryConditionInstance( Script::CStruct* p_structure );

/*****************************************************************************
**						Inline Functions									**
*****************************************************************************/
					
} // namespace Game

#endif // __MODULES_SKATE_VICTORYCOND_H