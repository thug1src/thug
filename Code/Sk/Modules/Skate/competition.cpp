/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		skate3													**
**																			**
**	Module:			skate			 										**
**																			**
**	File name:		competition.cpp											**
**																			**
**	Created by:		18/06/01	-	Mick									**
**																			**
**	Description:	handles playing single player competitions				**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/math/math.h>						 // for Mth::Rnd2()
#include <modules/skate/competition.h>
#include <gel/scripting/script.h>
#include <gel/scripting/string.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/array.h>

#include <sk/modules/skate/skate.h> 
#include <sk/modules/skate/goalmanager.h>

#include <sk/objects/skaterprofile.h>
#include <sk/objects/skatercareer.h>
#include <sk/objects/playerprofilemanager.h>

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
**							  Private Functions								**
*****************************************************************************/

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

CCompetition::CCompetition( Script::CStruct* pParams )
{
	mp_params = new Script::CStruct();
}

CCompetition::~CCompetition()
{
}
	
void CCompetition::EditParams( Script::CStruct* pParams )
{
	mp_params->AppendStructure( pParams );
}

void CCompetition::ResetJudgement()
{
	m_total_score = 0.0f;
	m_bail_points = 0.0f;
}

int CLeaderBoardEntry::GetTotalScore(int round)
{
	int score1 = 0;
	int score2 = 0;
	
	for (int i=0;i<round+1;i++)
	{
		if (m_score[i] > score1)
		{
			score2 = score1;
			score1 = m_score[i];
		}
		else
		if (m_score[i] > score2)
		{
			score2 = m_score[i];
		}
	}
	return score1 + score2;	
}


// start the competition, and pre-calculate all
// the scores
void	   CCompetition::StartCompetition(float bronze, float silver, float gold, float bronze_score, float silver_score, float gold_score, float bail)
{
	// store the defining parameters

	m_bronze = bronze;
	m_silver = silver;
	m_gold = gold;
	m_bronze_score = bronze_score;
	m_silver_score = silver_score;
	m_gold_score = gold_score;
	m_bail = bail;
	
	
	m_end_competition = false;

	// Get the current profile
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CPlayerProfileManager*	pPlayerProfileManager=pSkate->GetPlayerProfileManager();
	Obj::CSkaterProfile* pSkaterProfile=pPlayerProfileManager->GetCurrentProfile();

	// need to use GetDisplayName() wrapper function instead of GetUIString(), bec. ui string can now theoretically be empty
	m_leader_board[0].m_name = pSkaterProfile->GetDisplayName();

	// Let's count how many pros there are, so we can pcik from them at random

	int	possible_pros = pPlayerProfileManager->GetNumProfileTemplates();	

// get an array of pros, except the one that has the same name as me
	int	pro_order[100];
	int num_pros = 0;
	for (int i = 0;i<possible_pros;i++)
	{
	
		Obj::CSkaterProfile* pProfile = pPlayerProfileManager->GetProfileTemplateByIndex(i);
		
//		printf ("Pro=%d, Secret = %d, name = %s",pProfile->IsPro(), pProfile->IsSecret(), pProfile->GetDisplayName());
				
		if (!pProfile->IsPro() || pProfile->IsSecret() || Script::GenerateCRC(m_leader_board[0].m_name.getString()) == Script::GenerateCRC(pProfile->GetDisplayName()))
		{
			// skip over this one
		}
		else
		{
			pro_order[num_pros++] = i;
		}
	}
	
	// shuffle them
	for (int i = 0;i<num_pros;i++)
	{
		int	other = Mth::Rnd2(num_pros);
		int t = pro_order[i];
		pro_order[i] = pro_order[other];
		pro_order[other] = t;
	}

	// we initially populate the table with entries for each skater
	// other than skater 0, with random scores for 1st, 2nd and 3rd
	// that are close to the requied scores
	// and then populate the "round" scores with scores 
	// that will eventually add up to the final score
	
	// first determine a random order for the 7 other skaters
	
	int	order[vMAX_PLAYERS-1];
	
	// set up an initial linear order
	for (int i = 0;i<vMAX_PLAYERS-1;i++)
	{
		order[i] = i+1;
	}
	
	// shuffle the list of players
	for (int i = 0;i<vMAX_PLAYERS-1;i++)
	{
		int swap = Mth::Rnd2(vMAX_PLAYERS-1);
		int t = order[i];
		order[i] = order[swap];
		order[swap] = t;
	}

	// zero out the initial player
//	m_leader_board[0].mp_name = "PLAYER 1";
	m_leader_board[0].m_player_number = 0;
	for (int round = 0; round < vNUM_ROUNDS; round++)
	{
		m_leader_board[0].m_score[round] = 0;
	}

	// add in the other players in the shuffled order
	// for the first three, we need to calculate the score they are going to get
	
	bronze += Mth::PlusOrMinus(2.0f);	
	silver += Mth::PlusOrMinus(2.0f);	
	gold += Mth::PlusOrMinus(2.0f);	

	if (silver >= gold)
	{
		silver = gold - 0.1f;
	}
	if (bronze >= silver)
	{
		bronze = silver - 0.1f;
	}
	
	
	// go through all remaining players, and fill in the blanks	
	for (int i = 0;i<vMAX_PLAYERS-1;i++)
	{
		int player = i+1;
		m_leader_board[player].m_player_number = order[i];		// say which player is in this position		
		// ... note, need to get the name in here.
		// ...
		
		Script::CArray* pLeaderBoardNames;
		if ( mp_params->GetArray( CRCD(0x1cc6f48d,"leader_board_names"), &pLeaderBoardNames, Script::NO_ASSERT ) )
		{
			Dbg_MsgAssert( pLeaderBoardNames->GetSize() == vMAX_PLAYERS - 1, ( "leader_board_names has wrong size.  Needs to be %i", vMAX_PLAYERS - 1 ) );
			m_leader_board[player].m_name = pLeaderBoardNames->GetString( i );
		}
		else
			m_leader_board[player].m_name = pPlayerProfileManager->GetProfileTemplate(pPlayerProfileManager->GetProfileTemplateChecksum(pro_order[i]))->GetDisplayName();
		
		
		// now calculate the score
		float score = 0.0f;
		if (i == 0) score = gold;
		else if (i == 1) score = silver;
		else if (i == 2) score = bronze;
		else
		{
			// scores for the player after bronze are essentially any random number less than bronze
			score = bronze - (bronze/8) + Mth::PlusOrMinus(bronze/8);
			if (score >= bronze)
			{
				score = bronze - 0.1f;
			}			
		}
		// now we have the total score, we calculate the random components that
		// will add up to that score
		// the final sore is calculated as the average of the best of two scores
		// So... calculate two scores that add up to twice the total score
		// then add a third that is less than both of these
		// and put these in random order
		// 
		float score1 = score + Mth::PlusOrMinus(2.0f);
		float score2 = score*2.0f - score1;
		float lower = score1;
		if (score2 <lower)
		{
			lower = score2;
		}
		int pos1 = Mth::Rnd2(vNUM_ROUNDS);
		int pos2 = pos1;
		while (pos2 == pos1)
		{
			pos2 = Mth::Rnd2(vNUM_ROUNDS);
		}
		
		for (int round = 0;round <vNUM_ROUNDS;round++)
		{
			// all other scores default to numbers just below the lower of the two scores
			// m_leader_board[player].m_score[round] = (int)(10.0f*(lower - 2.0f + Mth::PlusOrMinus(1.9f)));
			m_leader_board[player].m_score[round] = (int)Mth::Round((lower - 2.0f + Mth::PlusOrMinus(1.9f)));
		}
		// finally fill in the two higher scores
		// into the randomly chosen poistions
		// m_leader_board[player].m_score[pos1] = (int)(score1*10.0f);
		// m_leader_board[player].m_score[pos2] = (int)(score2*10.0f);
		m_leader_board[player].m_score[pos1] = (int)Mth::Round(score1);
		m_leader_board[player].m_score[pos2] = (int)Mth::Round(score2);
	}

	printf ("\n\n");	
	// let up now print out the fruits of our efforts
	for (int i = 0;i<vMAX_PLAYERS;i++)
	{
		printf ("%d: pos %d, name %12s, %3d, %3d, %3d, %3d\n",
			i,
			m_leader_board[i].m_player_number,
			m_leader_board[i].m_name.getString(),
			m_leader_board[i].m_score[0],
			m_leader_board[i].m_score[1],
			m_leader_board[i].m_score[2],
			m_leader_board[i].GetTotalScore(3));
						
	}
	Sort();

	const char* p_first_place_name;
	if ( mp_params->GetString( CRCD(0xfaf19012,"first_place_name"), &p_first_place_name, Script::NO_ASSERT ) )
	{
		m_leader_board[0].m_name = p_first_place_name;
	}
	
	printf ("\nSorted\n");	
	// let up now print out the fruits of our efforts
	for (int i = 0;i<vMAX_PLAYERS;i++)
	{
		printf ("%d: pos %d, name %12s, %3d, %3d, %3d, %3d\n",
			i,
			m_leader_board[i].m_player_number,
			m_leader_board[i].m_name.getString(),
			m_leader_board[i].m_score[0],
			m_leader_board[i].m_score[1],
			m_leader_board[i].m_score[2],
			m_leader_board[i].GetTotalScore(3));
						
	}

// Default names for judges (dunno if we'll ever change it though...).	
#if (ENGLISH == 0)
	m_judge_name[0] = (char*)Script::GetLocalString( "comp_str_Judge1");
	m_judge_name[1] = (char*)Script::GetLocalString( "comp_str_Judge2");
	m_judge_name[2] = (char*)Script::GetLocalString( "comp_str_Judge3");
	m_judge_name[3] = (char*)Script::GetLocalString( "comp_str_Judge4");
	m_judge_name[4] = (char*)Script::GetLocalString( "comp_str_Judge5");
#else	
	m_judge_name[0] = "Judge 1";
	m_judge_name[1] = "Judge 2";
	m_judge_name[2] = "Judge 3";
	m_judge_name[3] = "Judge 4";
	m_judge_name[4] = "Judge 5";
#endif	
	m_current_round = -1;	
}

void	   CCompetition::StartRun()
{
	m_current_round++;
	ResetJudgement();
}


void CCompetition::EndRun(int score)
{
	// don't end a run if we're not in a comp
	if ( m_end_competition )
		return;

	int i;
	
	printf ("Ending Competition with score of %d\n",score);
	printf ("Bronze %f, silver %f, gold %f\n",m_bronze_score,m_silver_score,m_gold_score);
	
	float points = 0.0f;
	if (score < m_bronze_score)
	{
		points = m_bronze * score / m_bronze_score;		
	}
	else if (score < m_silver_score)
	{
		points = m_bronze + (m_silver - m_bronze) * (score - m_bronze_score) / (m_silver_score - m_bronze_score);
	}
	else if (score < m_gold_score)
	{
		points = m_silver + (m_gold - m_silver) * (score - m_silver_score) / (m_gold_score - m_silver_score);
	}
	else
	{
		// we have scored above the gold score
		// so interpolate up to perfect (100 for gold * 2)
		points = m_gold + (100.0f - m_gold) * (score - m_gold_score) / ( m_gold_score*2 - m_gold_score); 
	}

	printf ("points calculated at %f\n",points);

	// Now we have the score, so decrement it by the bail points	
	points -= m_bail_points;
	
	printf ("points decremented by %f bail points to %f\n",m_bail_points,points);
	
	
	// we've got the total score now (in points, for this round)
	// so store it	
	m_total_score = points;
	
	if (m_total_score > 99.9f)
		m_total_score = 99.9f;

	// we calculate a "varience", which is the maximum amount
	// a score can be randomly varied
	// this ranges from 10 to 0 as the actual score goes from 0 to 99.9
													   
	float variance = ( 99.9f - m_total_score ) / 10.0f;

	// perform some initial varying by half the variance
	// to make the score seen more random 
	m_total_score += Mth::PlusOrMinus(variance/2);

	// clamp it, just to be safe	
	if (m_total_score > 99.9f)
		m_total_score = 99.9f;
	if (m_total_score < 0.0f)
		m_total_score = 0.0f;
								
	// and find the n Judges scores
	// here the final score is the average of the top three scores.
	// so we need to generate three scores that add up to the 
	// total score
	
	
	m_score[0] = m_total_score + Mth::PlusOrMinus(variance);
	if (m_score[0] > 99.85f)
		m_score[0] = 99.9f;
	if (m_score[0] < 0.0f)
		m_score[0] = 0.0f;	
	m_score[1] = m_total_score + Mth::PlusOrMinus(variance);
	if (m_score[1] > 99.85f)
		m_score[1] = 99.9f;
	if (m_score[1] < 0.0f)
		m_score[1] = 0.0f;
	m_score[2] = (m_total_score * 3.0f) - (m_score[0] + m_score[1]);
	if (m_score[2] < 0.0f)
		m_score[2] = 0.0f;
	if (m_score[2] > 99.85f)
		m_score[2] = 99.9f;
		
		
	m_score[0] = (float)( (int) (m_score[0] * 10.0f)) /10.0f + 0.0001f;
	m_score[1] = (float)( (int) (m_score[1] * 10.0f)) /10.0f + 0.0001f;
	m_score[2] = (float)( (int) (m_score[2] * 10.0f)) /10.0f + 0.0001f;
	
	m_score[0] = (int)Mth::Round(m_score[0]);
	m_score[1] = (int)Mth::Round(m_score[1]);
	m_score[2] = (int)Mth::Round(m_score[2]);

	if ( m_score[0] > 99 )
		m_score[0] = 99;
	if ( m_score[1] > 99 )
		m_score[1] = 99;
	if ( m_score[2] > 99 )
		m_score[2] = 99;
	
	// recalculate the total score
	m_total_score = ((float)(m_score[0]) + (float)(m_score[1]) + (float)(m_score[2])) / 3;
	m_total_score = (int)Mth::Round( m_total_score );
	if ( m_total_score > 99 )
		m_total_score = 99;
	// m_judge_score[m_current_round] = (int)(10.0f * m_total_score + 0.5f);
	m_judge_score[m_current_round] = (int)m_total_score;

	printf ("3 scores are %f,%f,%f, total is %f\n",m_score[0],m_score[1],m_score[2],m_total_score);

	// Stick the total score in the correct place in the leaderboard	
	for (i = 0;i<vMAX_PLAYERS;i++)
	{
		if (m_leader_board[i].m_player_number == 0)
		{
			m_leader_board[i].m_score[m_current_round] = m_judge_score[m_current_round];
		}
	}
	
	// and re-sort it	
	Sort();	
	
	// re-calculate our	placing

	for (i = 0;i<vMAX_PLAYERS;i++)
	{
		if (m_leader_board[i].m_player_number == 0)
		{
			m_place = i+1;		
		}
	}	

	// then fill in the remaining judge scores with scores that are lower than the
	// lowest of these three scores

	// calculate lowest score	
	float lowest = m_score[0];
	if (m_score[1] < lowest)
		lowest = m_score[1];
	if (m_score[2] < lowest)
		lowest = m_score[2];
		
	printf("Lowest = %f\n",lowest);
		
	// add in remaining scores, in a range of (lowest - 2* variance) to (lowest)
	for (i = 3; i< vNUM_JUDGES; i++)
	{
		m_score[i] = lowest - variance + Mth::PlusOrMinus(variance);		
		printf ("Score %d is %f\n",i,m_score[i]);
	}

	// set any negative scores to 0.0f	   
	// (this will not effect the three highest scores
	for (i = 0; i< vNUM_JUDGES; i++)
	{
		if (m_score[i] < 0.0f)
		{
			m_score[i] = 0.0f;
		}
	}
	
	// Finally, shuffle the scores into random order
	// shuffle the list of players
	for (i = 0;i<vNUM_JUDGES-1;i++)
	{
		int swap = Mth::Rnd2(vNUM_JUDGES);
		float t = m_score[i];
		m_score[i] = m_score[swap];
		m_score[swap] = t;
	}
	
	Game::CGoalManager* pGoalManager = Game::GetGoalManager();
	Dbg_Assert( pGoalManager );
	if ( pGoalManager->IsInCompetition() )
	{
		// and let us have a look at what we come up with;
		Script::CStruct* p_run_scores = new Script::CStruct();
		// pack the scores in a carray to make them easy to deal with in script
		Script::CArray* p_run_scores_array = new Script::CArray();
		p_run_scores_array->SetSizeAndType( vNUM_JUDGES, ESYMBOLTYPE_STRUCTURE );
		printf( "\nJudgement Day\n\n" );
		for ( i = 0; i < vNUM_JUDGES; i++ )
		{
			// add an integer version of this score to the params, so it's 
			// easy to deal with in script
			Script::CStruct* p_temp = new Script::CStruct();
			p_temp->AddInteger( "score", (int)Mth::Round( m_score[i] ) );

			// add a flag to the top judges
			if ( IsTopJudge( i ) )
			{
				p_temp->AddChecksum( NONAME, CRCD( 0x7b1b959f, "top_judge" ) );
			}

			p_run_scores_array->SetStructure( i, p_temp );
			printf ("%d: %f Top=%d\n",i,m_score[i],IsTopJudge(i));
		}	
		
		p_run_scores->AddArray( "scores", p_run_scores_array );

		// add the total score
		// p_run_scores->AddInteger( "total_score", (int)Mth::Round( ( m_total_score * 10 ) ) );
		p_run_scores->AddInteger( "total_score", (int)Mth::Round( ( m_total_score ) ) );

		Script::CleanUpArray( p_run_scores_array );
		delete p_run_scores_array;
		#ifdef __NOPT_ASSERT__
		Script::CScript *p_script=Script::SpawnScript( "goal_comp_show_run_scores", p_run_scores );
		p_script->SetCommentString("Spawned from CCompetition::EndRun(...)");
		#else
		Script::SpawnScript( "goal_comp_show_run_scores", p_run_scores );
		#endif
		delete p_run_scores;
	}
}

// This function should be called every time the skater bails
void 	   CCompetition::Bail()
{
		m_bail_points += m_bail + Mth::PlusOrMinus(m_bail/2);
}

void	   CCompetition::Sort()
{
	// sort the list, based on total score 

	int round = m_current_round;									  
	// if the competition has ended (maybe early), then total up for all the rounds	
	if (CompetitionEnded())
	{
		round = 2;
	}
							 
	for (int i=0;i<vMAX_PLAYERS-1;i++)
	{
		int high = i;
		for (int j=i+1; j<vMAX_PLAYERS;j++)
		{
			if (m_leader_board[j].GetTotalScore(round) > m_leader_board[high].GetTotalScore(round))
			{
				high = j;
			}
			// give the player the higher position if there is a tie
			else if ( m_leader_board[j].m_player_number == 0 && m_leader_board[j].GetTotalScore(round) == m_leader_board[high].GetTotalScore(round) )
			{
				high = j;
			}
		}
		if (high != i)
		{
			CLeaderBoardEntry t = m_leader_board[high];
			m_leader_board[high] = m_leader_board[i];
			m_leader_board[i] = t;
		}
	}
}
	
int 	   CCompetition::GetPosition(int player)
{
	return 0;
}

	   
char *		CCompetition::GetJudgeName(int judge)
{
	return m_judge_name[judge];
}

float		CCompetition::GetJudgeScore(int judge)
{
	return m_score[judge];
}

void CCompetition::SetupJudgeText()
{
	Game::CGoalManager* pGoalManager = Game::GetGoalManager();
	Dbg_Assert( pGoalManager );

	if ( pGoalManager->IsInCompetition() )
	{
		Script::CStruct* p_score_params = new Script::CStruct();

		Script::CArray* p_score_array = new Script::CArray();
		p_score_array->SetSizeAndType( vNUM_JUDGES , ESYMBOLTYPE_STRUCTURE );

		printf ("\nSetupJudgeText\n");	
		// let up now print out the fruits of our efforts
		for (int i = 0;i<vMAX_PLAYERS;i++)
		{
			printf ("%d: pos %d, name %12s, %3d, %3d, %3d, %3d\n",
				i,
				m_leader_board[i].m_player_number,
				m_leader_board[i].m_name.getString(),
				m_leader_board[i].m_score[0],
				m_leader_board[i].m_score[1],
				m_leader_board[i].m_score[2],
				m_leader_board[i].GetTotalScore(3));
		
		}

		for ( int i = 0; i < vNUM_JUDGES; i++ )
		{
			Script::CStruct* p_leader_board_entry = new Script::CStruct();
			p_leader_board_entry->AddString( "name", m_leader_board[i].m_name.getString() );
			printf("m_current_round: %i\n", m_current_round);
			p_leader_board_entry->AddInteger( "score", m_leader_board[i].GetTotalScore(m_current_round) );

			if ( m_leader_board[i].m_player_number == 0 )
			{
				p_leader_board_entry->AddChecksum( NONAME, CRCD( 0x67e6859a, "player" ) );
			}

			p_score_array->SetStructure( i, p_leader_board_entry );
		}

		p_score_params->AddArray( "leader_board", p_score_array );
		Script::CleanUpArray( p_score_array );
		delete p_score_array;
		#ifdef __NOPT_ASSERT__
		Script::CScript *p_script=Script::SpawnScript( "goal_comp_add_leader_board", p_score_params );
		p_script->SetCommentString("Spawned from CCompetition::SetupJudgeText()");
		#else
		Script::SpawnScript( "goal_comp_add_leader_board", p_score_params );
		#endif
		delete p_score_params;
	}

/*//	int i_total = 0;
	for (int i = 0;i<vNUM_JUDGES;i++)
	{
		sprintf(Script::GetScriptString(i*2),GetJudgeName(i));
		sprintf(Script::GetScriptString(i*2+1),"%2.1f",GetJudgeScore(i));
//		if (IsTopJudge(i))
//		{
//			i_total += (int) (GetJudgeScore(i) * 10.0f); 
//		}
	}	
//	m_total_score = (float)((int)(10.0f * (m_score[0] + m_score[1] + m_score[2])/3.0f))/10.0f;	
//	total =         (float)((int)(total / 3.0f))/10.0f;
	
	
	sprintf(Script::GetScriptString(vNUM_JUDGES*2),"%2.1f",(float)(m_judge_score[m_current_round]/10.0f));

	// and let us set up the leaderboard text as well.....
	int line;
	for (line = 0;line<vMAX_PLAYERS;line++)
	{
		
		const char *pC;			  // format for name
		const char *pC21f;		  // format for number
		const char *xpC21f;		  // format for dark number
	
		if (m_leader_board[line].m_player_number == 0)
		{
			// use yellow colors for skater
			pC = "&C1%s";
			pC21f = "&C1%2.1f";
			xpC21f = "&C3%2.1f";
		}
		else
		{
			// and white/grey for other skaters
			pC = "%s";
			pC21f = "%2.1f";
			xpC21f = "&C2%2.1f";
		}
		
		sprintf(Script::GetScriptString(12 + line * 5), pC, m_leader_board[line].m_name.getString());

		int small = 0;
		if (m_leader_board[line].m_score[1] < m_leader_board[line].m_score[small]) small = 1;
		if (m_leader_board[line].m_score[2] < m_leader_board[line].m_score[small]) small = 2;

		int round = m_current_round;									  
		// if the competition has ended (maybe early), then total up for all the rounds	
		if (CompetitionEnded())
		{
			round = 2;
		}
				
		
		if (round == 2 && small == 0)
			sprintf(Script::GetScriptString(12 + line * 5 + 1),xpC21f, (float)(m_leader_board[line].m_score[0]/10.0f));
		else
			sprintf(Script::GetScriptString(12 + line * 5 + 1),pC21f, (float)(m_leader_board[line].m_score[0]/10.0f));
		
		if (round >=1)
		{
			if (round == 2 && small == 1)
				sprintf(Script::GetScriptString(12 + line * 5 + 2),xpC21f, (float)(m_leader_board[line].m_score[1]/10.0f));
			else
				sprintf(Script::GetScriptString(12 + line * 5 + 2),pC21f, (float)(m_leader_board[line].m_score[1]/10.0f));
		}
		else
			sprintf(Script::GetScriptString(12 + line * 5 + 2), pC,"-");

		if (round >=2)
		{
			if (round == 2 && small == 2)
				sprintf(Script::GetScriptString(12 + line * 5 + 3),xpC21f, (float)(m_leader_board[line].m_score[2]/10.0f));
			else
				sprintf(Script::GetScriptString(12 + line * 5 + 3),pC21f, (float)(m_leader_board[line].m_score[2]/10.0f));
		}
		else
		{
			sprintf(Script::GetScriptString(12 + line * 5 + 3),pC, "-");				
		}
		
		sprintf(Script::GetScriptString(12 + line * 5 + 4),pC21f, (float)(m_leader_board[line].GetTotalScore(round)/10.0f));
		
	}
	
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CPlayerProfileManager*	pPlayerProfileManager=pSkate->GetPlayerProfileManager();
	Obj::CSkaterProfile* pSkaterProfile=pPlayerProfileManager->GetCurrentProfile();
	sprintf (Script::GetScriptString(12 + line *5),	pSkaterProfile->GetDisplayName());
*/	
}

// Return true if this is a top 3 judge
// note, has to handle cases where score is the same
// works by sorting the judges, and then seeing if we are in the top three 
bool CCompetition::IsTopJudge(int judge)
{
	int	judge_order[vNUM_JUDGES];
	
	for (int i=0;i<vNUM_JUDGES;i++)
	{
		judge_order[i] = i;
	}
	for (int i=0;i<vNUM_JUDGES-1;i++)
	{
		for (int j = i;j<vNUM_JUDGES;j++)
		{
			if (GetJudgeScore(judge_order[i]) < GetJudgeScore(judge_order[j]))
			{
				int	t=judge_order[i];
				judge_order[i]=judge_order[j];
				judge_order[j]=t;  
			}
		}
	}
	
	// if it is in the top three, then return true
	if (judge_order[0] == judge || judge_order[1] == judge || judge_order[2] == judge)
	{
		return true;
	}
	
	return false;
}


bool CCompetition::PlaceIs(int place)
{
	return 	(place == m_place);
}

bool CCompetition::RoundIs(int round)
{
	return 	(round == m_current_round+1);	// ............... placign
}


void CCompetition::EndCompetition()
{
	m_end_competition = true;
}

bool CCompetition::CompetitionEnded()
{
	return m_end_competition;
}

	   

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mdl




