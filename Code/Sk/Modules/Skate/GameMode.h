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
**	File name:		Skate/GameMode.h										**
**																			**
**	Created by:		02/07/01	-	gj										**
**																			**
**	Description:	Defines various game modes								**
**																			**
*****************************************************************************/

#ifndef __MODULES_SKATE_GAMEMODE_H
#define __MODULES_SKATE_GAMEMODE_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/list.h>
#include <gel/scripting/struct.h>
#include <sk/modules/skate/victorycond.h>
#include <gfx/nxviewman.h>
#include <sys/timer.h>

#ifndef	__SCRIPTING_UTILS_H
#include <gel/scripting/utils.h>
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Mdl
{
	// forward declaration
	class Score;
}

namespace Game
{
 


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

struct SRanking
{
	int				obj_id;
	int				ranking;
	Mdl::Score*		pScore;
};

class  CGameMode  : public Spt::Class
{
	

public:
	enum
	{
		vGAMEMODE_NETLOBBY = 0x1c471c60,		// "netlobby"
	};
			
public:
	CGameMode();
	virtual						~CGameMode();

	// clears the game mode to default, which is effectively "freeskate"
	bool						Reset();

	// loads a game mode, as defined in a script
	bool						LoadGameType( uint32 game_type );
	bool						OverrideOptions( Script::CScriptStructure *pParams );
	
public:
	void						CalculateRankings( SRanking* p_player_rankings );
	void						SetScoreFrozen( bool is_frozen );
	bool						ScoreFrozen();
	void						SetScoreAccumulation( bool should_accumulate );
	void						SetScoreDegradation( bool should_degrade );
	void						SetNumTeams( int num_teams );
	bool						ShouldAccumulateScore();
	bool						ShouldTrackBestCombo();
	bool						ShouldTrackTrickScore();
	bool						ShouldDegradeScore();
	bool						ShouldUseClock();
	bool						ShouldStopAtZero();
	bool						ShouldRunIntroCamera();
	bool						ShouldShowRankingScreen();
	bool						IsTeamGame();
	int							NumTeams();
	bool						IsTimeLimitConfigurable();
	bool						ShouldTimerBeep();
	uint32						GetTimeLimit();
	uint32						GetTargetScore();
	uint32						GetInitialNumberOfPlayers();
	uint32						GetMaximumNumberOfPlayers();
	void						SetMaximumNumberOfPlayers( uint32 num_players );
	bool						EndConditionsMet();
	Tmr::Time					GetTimeLeft();
	void						SetTimeLeft( Tmr::Time time_left );
	bool						IsWinner( uint32 skater_num );
	Nx::ScreenMode				GetScreenMode();
	bool						IsFrontEnd();
	bool						IsParkEditor();

	uint32 						GetNameChecksum();
	uint32 						ConvertNetNameChecksum(uint32 nameChecksum);
	
	int							GetFireballLevel();

	// for generic tests, like "is career mode"
	bool						IsTrue( const char* pFieldName );
	bool						IsTrue( uint32 checksum );
	
	void						PrintContents()
	{
#ifdef __NOPT_ASSERT__
		Script::PrintContents(&m_ModeDefinition);
#endif	
	}

protected:
	void						clear_victorycondition_list();
	void						build_victorycondition_list();
	bool						remove_component_if_exists( const char* p_component_name, Script::CScriptStructure* p_params );
	
protected:
	Script::CScriptStructure	m_ModeDefinition;
	
	// for determining who is the winner, as well
	// as ending the game before time has run out
	Lst::Head<CVictoryCondition>	m_VictoryConditionList;
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

bool ScriptOverrideGameModeOptions( Script::CScriptStructure *pParams, Script::CScript *pScript );

/*****************************************************************************
**						Inline Functions									**
*****************************************************************************/


} // namespace Game

#endif // __MODULES_SKATE_GAMEMODE_H