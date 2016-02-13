/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		skate3  												**
**																			**
**	Module:			sk						 								**
**																			**
**	File name:		competition.h											**
**																			**
**	Created by:		6/15/00 - Mick											**
**																			**
**	Description:	<description>		 									**
**																			**
*****************************************************************************/

#ifndef __SK_MODULES_COMPETITION_H
#define __SK_MODULES_COMPETITION_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/string/cstring.h>

#include <gel/scripting/struct.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace Mdl
{

						

	enum 
	{
		vMAX_PLAYERS = 8,
		vNUM_ROUNDS = 3,
		vNUM_JUDGES = 5,
	};


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/


class		CLeaderBoardEntry
{

public:
	int 						GetTotalScore(int round);	

	Str::String					m_name;
	int							m_player_number;
	int							m_score[vNUM_ROUNDS]; 	// score now stored in hundreths
	bool						visible;	
};


class  CCompetition  : public Spt::Class
{
	
	
public:	
	CCompetition( Script::CStruct* pParams = NULL );
	~CCompetition();
	
	void						StartCompetition(float bronze = 90.0f, float silver = 93.0f, float gold = 95.0f, float bonze_score=20000, float silver_score=40000,float gold_score = 60000,float bail = 1.0f);
	void						StartRun();
	void 						EndRun(int score);
	void 						Bail();
	void						Sort();
	
	int 						GetPosition(int player = 0);
	char *						GetJudgeName(int judge);
	float						GetJudgeScore(int judge);
	void						SetupJudgeText();
	bool 						IsTopJudge(int judge);
	bool 						PlaceIs(int place);
	bool 						RoundIs(int place);
	void 						EndCompetition();
	bool 						CompetitionEnded();

	void						EditParams( Script::CStruct* pParams );

private:
	void						ResetJudgement();
	
	// BB - i'm lazy, so I'm just adding a struct to hold various
	// params that i can use to change the behavior in the code
	Script::CStruct*			mp_params;
	
	int							m_current_round;

	CLeaderBoardEntry			m_leader_board[vMAX_PLAYERS];

	float 						m_bronze;		 	// score required to get bronze
	float						m_silver;
	float 						m_gold;
	float 						m_bronze_score;		// points required to get that score
	float 						m_silver_score;
	float 						m_gold_score;
	float 						m_bail;			    // points deducted for bailing	

	float						m_bail_points;			   	
	float						m_score[vNUM_JUDGES];  	// m_score is generated from m_total_score
	char *						m_judge_name[vNUM_JUDGES];  	//
	float						m_total_score;

	int							m_judge_score[vNUM_ROUNDS];

	bool						m_end_competition;
	
	int	m_place;

};

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace <module-name>

#endif		// __SK_MODULES_COMPETITION_H



