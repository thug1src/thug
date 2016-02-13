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

// start autoduck documentation
// @DOC gamemode
// @module gamemode | None
// @subindex Scripting Database
// @index script | gamemode

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <sk/gamenet/gamenet.h>
#include <sk/modules/frontend/frontend.h>
#include <sk/modules/skate/GameMode.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/GoalManager.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>
#include <sk/scripting/cfuncs.h>
#include <sk/scripting/skfuncs.h>
#include <sk/objects/skater.h>		   // get player id
#include <sk/components/skaterscorecomponent.h>
#include <sys/config/config.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/



namespace Game
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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGameMode::clear_victorycondition_list()
{
	

	Lst::Node<CVictoryCondition> *pNode = m_VictoryConditionList.GetNext();
	while(pNode)
	{
		Lst::Node<CVictoryCondition> *pNext = pNode->GetNext();
		pNode->Remove();
		delete pNode->GetData();
		delete pNode;
		pNode = pNext;
	}

	Dbg_Assert( m_VictoryConditionList.IsEmpty() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGameMode::build_victorycondition_list()
{
	

	clear_victorycondition_list();

	Script::CArray* pArray;
	if ( !m_ModeDefinition.GetArray( CRCD(0x94a24c5e,"victory_conditions"), &pArray ) )
	{
		// no victory conditions found
		return;
	}

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());

	// loop through the victory conditions
	for ( uint32 i = 0; i < pArray->GetSize(); i++ )
	{
		Script::CScriptStructure* pStructure = pArray->GetStructure( i );
		Dbg_Assert( pStructure );

		CVictoryCondition* pVictoryCondition = CreateVictoryConditionInstance( pStructure );
		Dbg_Assert( pVictoryCondition );
		m_VictoryConditionList.AddToTail( new Lst::Node<CVictoryCondition>( pVictoryCondition ) );
	}

	Mem::Manager::sHandle().PopContext();
	// TODO: Scan for combinations that don't make sense
	// (i.e. highest score AND best combo)
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::remove_component_if_exists( const char* p_component_name, Script::CScriptStructure* p_params )
{
	if ( p_params->ContainsComponentNamed( p_component_name ) )
	{
		m_ModeDefinition.RemoveComponent( Script::GenerateCRC( p_component_name ) );
		return true;
	}
	
	return false;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGameMode::CGameMode()
{
	

	Reset();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGameMode::~CGameMode()
{
	

	clear_victorycondition_list();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Tmr::Time CGameMode::GetTimeLeft()
{

	CGoalManager* pGoalManager = Game::GetGoalManager();
	uint32	nameChecksum = ConvertNetNameChecksum(GetNameChecksum());
	CGoal* pGoal = pGoalManager->GetGoal(nameChecksum,false);

	if( !pGoal )
	{
		return 0;
	}

	return pGoal->GetTimeLeft();
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CGameMode::SetTimeLeft( Tmr::Time time_left )
{
	CGoalManager* pGoalManager = Game::GetGoalManager();
	uint32	nameChecksum = ConvertNetNameChecksum(GetNameChecksum());
	CGoal* pGoal = pGoalManager->GetGoal(nameChecksum,false);

	if( pGoal )
	{
		pGoal->SetTimer( time_left );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CGameMode::EndConditionsMet()
{
	// this checks whether time has run out
	if ( ShouldUseClock() )
	{
		CGoalManager* pGoalManager = Game::GetGoalManager();

		if( pGoalManager->GetNumActiveGoals( true ) > 0 && pGoalManager->GetGoalTime() <= 0 )
		{
			return true;
		}
	}
	
	uint32 i;
	int num_terminals = 0;
	int num_completed_terminals = 0;

	// TODO:  Allow boolean conditions (&'s and |'s)

	// Look through the victory conditions also
	for ( i = 0; i < m_VictoryConditionList.CountItems(); i++ )
	{
		Lst::Node<CVictoryCondition> *pNode = m_VictoryConditionList.GetItem( i );

		if (pNode->GetData()->IsTerminal())
		{
			num_terminals++;
			
			if ( pNode->GetData()->ConditionComplete() )
			{
				num_completed_terminals++;
			}
		}
	}

	// If all of the terminal ones have been met,
	// then we've also met our end conditions
	if ( num_terminals && ( num_completed_terminals == num_terminals ) )
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::ShouldUseClock()
{
	int time_limit;
	
	if(	( GetNameChecksum() == CRCD(0x5d32129c,"king")) ||
		( GetNameChecksum() == CRCD(0x6ef8fda0,"netking")) ||
		( GetNameChecksum() == CRCD(0xf135ecb6,"scorechallenge")) ||
		( GetNameChecksum() == CRCD(0x1498240a,"netscorechallenge")) ||
		// K: Added check for creategoals to fix bug where end of run triggered straight away on
		// entering a level in creategoals mode.
		( GetNameChecksum() == CRCD(0xe45f0ca2,"creategoals")) ||
		( GetNameChecksum() == CRCD(0x3d6d444f,"firefight")) ||
		( GetNameChecksum() == CRCD(0xbff33600,"netfirefight")))
	{
		return false;
	}

	m_ModeDefinition.GetInteger( CRCD(0x8d38a280,"default_time_limit"), &time_limit, true );
	return ( time_limit != 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::ShouldStopAtZero()
{
	Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	
	if( skate_mod->IsMultiplayerGame() == false )
	{
		return false;
	}

	if( ShouldUseClock())
	{
		int should_stop;

		m_ModeDefinition.GetInteger( CRCD(0x48e748b5,"stop_at_zero"), &should_stop, true );
		return ( should_stop == 1 );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CGameMode::ShouldAccumulateScore()
{
	int should_accumulate_score;
	m_ModeDefinition.GetInteger( CRCD(0xc1f5864b,"accumulate_score"), &should_accumulate_score, true );
	return should_accumulate_score;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CGameMode::ShouldTrackBestCombo()
{
	int should_track;
	if( m_ModeDefinition.GetInteger( CRCD(0xd57c16ba,"track_best_combo"), &should_track, false ) == false )
	{
		return false;
	}

	return should_track;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void
CGameMode::SetScoreAccumulation( bool should_accumulate )
{
	m_ModeDefinition.AddInteger( CRCD(0xc1f5864b,"accumulate_score"), should_accumulate );
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGameMode::SetScoreFrozen( bool is_frozen )
{
	m_ModeDefinition.AddInteger( CRCD(0x78fb9f04,"score_frozen"), is_frozen );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void
CGameMode::SetScoreDegradation( bool should_degrade )
{
	m_ModeDefinition.AddInteger( CRCD(0xca37e639,"degrade_score"), should_degrade );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void
CGameMode::SetNumTeams( int num_teams )
{
	GameNet::Manager* gamenet_man = GameNet::Manager::Instance();

	m_ModeDefinition.AddInteger( CRCD(0x2ebe6cd1,"num_teams"), num_teams );
	if( GetNameChecksum() == CRCD(0x1c471c60,"netlobby") )
	{
		gamenet_man->CreateTeamFlags( num_teams );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::ScoreFrozen()
{
	int score_is_frozen;
	m_ModeDefinition.GetInteger( CRCD(0x78fb9f04,"score_frozen"), &score_is_frozen, true );
	return ( score_is_frozen != 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CGameMode::ShouldTrackTrickScore()
{
	int should_track_trick_score;
	m_ModeDefinition.GetInteger( CRCD(0x47f763ff,"track_trick_score"), &should_track_trick_score, true );
	return should_track_trick_score;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CGameMode::ShouldDegradeScore()
{
	int should_degrade_score;
	m_ModeDefinition.GetInteger( CRCD(0xca37e639,"degrade_score"), &should_degrade_score, true );
	return should_degrade_score;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::ShouldRunIntroCamera()
{
	int should_run_intro_camera;
	if (Config::CD())
	{
		m_ModeDefinition.GetInteger( CRCD(0xe2355b5e,"should_run_intro_camera"), &should_run_intro_camera, true );
	}
	else
	{
		m_ModeDefinition.GetInteger( CRCD(0xea0d7454,"should_run_intro_camera_noncd"), &should_run_intro_camera, true );
	}	
	return should_run_intro_camera;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::ShouldShowRankingScreen()
{
	int should_show_ranking_screen;
	m_ModeDefinition.GetInteger( CRCD(0xf18020f1,"show_ranking_screen"), &should_show_ranking_screen, true );
	return should_show_ranking_screen;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CGameMode::IsTimeLimitConfigurable()
{
	
	
	uint32 time_limit_type;
	m_ModeDefinition.GetChecksum( CRCD(0x7d5e4bc1,"time_limit_type"), &time_limit_type, true );
	Dbg_MsgAssert( time_limit_type == CRCD(0x2b75d083,"config") || time_limit_type == CRCD(0x613631cd,"fixed"),( "Unrecognized time limit type" ));
	return ( time_limit_type == CRCD(0x2b75d083,"config") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CGameMode::IsTeamGame()
{
	GameNet::Manager* gamenet_man = GameNet::Manager::Instance();
	return ( gamenet_man->InNetGame() && ( NumTeams() > 0 ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int
CGameMode::NumTeams()
{
	int num_teams;
	m_ModeDefinition.GetInteger( CRCD( 0x2ebe6cd1, "num_teams" ), &num_teams, true );
	return num_teams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CGameMode::ShouldTimerBeep()
{
	int beeps;
	m_ModeDefinition.GetInteger( CRCD(0x35c8073,"timer_beeps"), &beeps, true );
	return beeps;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32
CGameMode::GetTimeLimit()
{
	
	int time_limit;
	m_ModeDefinition.GetInteger( CRCD(0x8d38a280,"default_time_limit"), &time_limit, true );
	return (uint32)time_limit;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32		CGameMode::GetTargetScore( void )
{
	uint32 i;
	// Look through the victory conditions also
	for ( i = 0; i < m_VictoryConditionList.CountItems(); i++ )
	{
		Lst::Node<CVictoryCondition> *pNode = m_VictoryConditionList.GetItem( i );

		CScoreReachedVictoryCondition* condition;

		condition = static_cast <CScoreReachedVictoryCondition*> (pNode->GetData());
		if( condition )
		{
			return condition->GetTargetScore();
		}
	}

	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CGameMode::GetInitialNumberOfPlayers()
{
	

	int num_players;
	m_ModeDefinition.GetInteger( CRCD(0x6dc3d6f1,"initial_players"), &num_players, true );
	return num_players;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CGameMode::SetMaximumNumberOfPlayers( uint32 num_players )
{
	m_ModeDefinition.AddInteger( CRCD(0x4d42e61d,"max_players"), num_players );
	Dbg_Printf( "****** SETTING MAX PLAYERS TO %d\n", num_players );
//	DumpUnwindStack( 20, 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CGameMode::GetMaximumNumberOfPlayers()
{
	int num_players;
	m_ModeDefinition.GetInteger( CRCD(0x4d42e61d,"max_players"), &num_players, true );
	return num_players;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::ScreenMode CGameMode::GetScreenMode()
{
	

	uint32 checksum;

	m_ModeDefinition.GetChecksum( CRCD(0x9bab2eae,"screenmode"), &checksum, true );
	switch (checksum)
	{
		case 0x3558d8e6: // single
			return Nx::vONE_CAM;
			break;
		case 0x9d65d0e7: // horse
			return Nx::vHORSE1;
			break;
		case 0x06ab02f2: // splitscreen
		{
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			Prefs::Preferences* pPreferences;

			pPreferences = skate_mod->GetSplitScreenPreferences();
			Dbg_Assert(pPreferences);

			Script::CScriptStructure* pStructure = pPreferences->GetPreference( CRCD(0x6f82c71e,"viewport_type") );
			Dbg_Assert(pStructure);

			uint32 checksum;
			pStructure->GetChecksum( CRCD(0x21902065,"checksum"), &checksum, true );
			if ( checksum == CRCD(0x92f5b4cb,"viewport_type_vertical") )
			{
				return Nx::vSPLIT_V;
			}
			else if ( checksum == CRCD(0x7c442511,"viewport_type_horizontal") )
			{
				return Nx::vSPLIT_H;
			}
			else
			{
				Dbg_Assert( 0 );
				return Nx::vSPLIT_V;
			}
		}
		break;
		default:
			Dbg_Assert( 0 );
			return (Nx::ScreenMode) 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::IsFrontEnd()
{
	

	int is_frontend;
	m_ModeDefinition.GetInteger( CRCD( 0xa6479767, "is_frontend" ), &is_frontend, true );
	return is_frontend;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::IsParkEditor()
{
	

	int is_parkeditor;
	m_ModeDefinition.GetInteger( CRCD(0x66d9ddb9,"is_parkeditor"), &is_parkeditor, true );
	return is_parkeditor;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	CGameMode::GetFireballLevel()
{
	int level;

	level = 3;
	m_ModeDefinition.GetInteger( CRCD(0xce87e4e3,"fireball_level"), &level );
	
	return level;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::IsTrue( const char* pFieldName )
{
	

	int field_value;
	m_ModeDefinition.GetInteger( pFieldName, &field_value, true );
	return field_value;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::IsTrue( uint32 checksum )
{
	

	int field_value;
	m_ModeDefinition.GetInteger( checksum, &field_value, true );
	return field_value;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Returns the checksum of the name of the game mode definition structure
// these structures are defined in gamemode.q, an example would be "graffiti"
// however, you should NOT use these names to check for things to do
// as there might be mutliple instance of similar modes (like, graffiti and netgraffiti)
// so you should use the fields within the structure, 
// use the IsTrue methods above, like IsTrue("is_graffiti");  

uint32 CGameMode::GetNameChecksum()
{
	

	uint32 checksum = 0;
	m_ModeDefinition.GetChecksum(CRCD(0xa1dc81f9,"name"), &checksum);
	return checksum;
}

					 
// if the checksum is the name of a net game, then return the name of the regular equivalent					 
// used by the time functions, wehre we need to know what type of game it is
// and not if it's net or not
uint32 CGameMode::ConvertNetNameChecksum(uint32 nameChecksum)
{

	uint32 checksum = nameChecksum;
	
	if( nameChecksum == CRCD(0x30c2ffa3,"nettrickattack"))
	{
		checksum =  CRCD( 0xf9283ee7,"trickattack" );
	}
	else if( nameChecksum == CRCD( 0x5e2ea50c, "netgraffiti" ))
	{
		checksum = CRCD(0xc8a82b5a, "graffiti");
	}
	else if( nameChecksum == CRCD( 0xc50affd0, "netcombomambo" ))
	{
		checksum = CRCD(0x23eb3da3, "combomambo");
	}
	else if ( nameChecksum == CRCD( 0xf9d5d933, "netslap" ))
	{
		checksum = CRCD( 0xca1f360f, "slap" );
	}
	else if ( nameChecksum == CRCD(0x6ef8fda0, "netking" ) )
	{
		checksum = CRCD( 0x5d32129c,"king" );
	}
	else if( nameChecksum == CRCD(0xbff33600,"netfirefight") )
	{
		checksum = CRCD(0x3d6d444f,"firefight");
	}
	else if( nameChecksum == CRCD(0x6c5ff266,"netctf") )
	{
		checksum = CRCD(0xa5ad2b0b,"ctf");
	}
	return checksum;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int compFunc( const void *arg1, const void *arg2 )
{
	SRanking* pRanking1 = (SRanking*)arg1;
	SRanking* pRanking2 = (SRanking*)arg2;

	int score1 = pRanking1->pScore->GetTotalScore();
	int score2 = pRanking2->pScore->GetTotalScore();

	if ( score1 == score2 )
	{
		return 0;
	}
	else if ( score1 < score2 )
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGameMode::CalculateRankings( SRanking* p_player_rankings )
{
	

	Dbg_AssertPtr( p_player_rankings );

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	for ( uint32 skater_num = 0; skater_num < skate_mod->GetNumSkaters(); skater_num++ )
	{
		Obj::CSkater* pSkater = skate_mod->GetSkater(skater_num);
		Dbg_AssertPtr( pSkater );
		
		Obj::CSkaterScoreComponent* pSkaterScoreComponent = GetSkaterScoreComponentFromObject(pSkater);
		Dbg_Assert( pSkaterScoreComponent );
		
		p_player_rankings[skater_num].obj_id = pSkater->GetID();
		p_player_rankings[skater_num].pScore = pSkaterScoreComponent->GetScore();
		p_player_rankings[skater_num].ranking = 0;
	}

	// sort them by score
	// TODO:  make the sorting function variable depending on the game mode
	qsort( &p_player_rankings[0], skate_mod->GetNumSkaters(), sizeof(SRanking), compFunc );

	for ( uint32 i = 0; i < skate_mod->GetNumSkaters(); i++ )
	{
		if ( (i != 0) && (p_player_rankings[i].pScore->GetTotalScore() == p_player_rankings[i-1].pScore->GetTotalScore()) )
		{
			p_player_rankings[i].ranking = p_player_rankings[i-1].ranking;
		}
		else
		{
			p_player_rankings[i].ranking = i + 1;
		}

		printf("ranking = skater=%d score=%d ranking=%d\n", p_player_rankings[i].obj_id, p_player_rankings[i].pScore->GetTotalScore(), p_player_rankings[i].ranking );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::IsWinner( uint32 skater_num )
{
	

	int completed_count[Mdl::Skate::vMAX_SKATERS];
	uint32 j;

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	for ( j = 0; j < skate_mod->GetNumSkaters(); j++ )
	{
		completed_count[j] = 0;
		
		for ( uint32 i = 0; i < m_VictoryConditionList.CountItems(); i++ )
		{
			Lst::Node<CVictoryCondition> *pNode = m_VictoryConditionList.GetItem( i );
			
			if ( pNode->GetData()->IsWinner( j ) )
			{
				completed_count[j]++;
			}
		}
	}

	for ( j = 0; j < skate_mod->GetNumSkaters(); j++ )
	{
		if ( skater_num != j && completed_count[skater_num] < completed_count[j] )
		{
			return false;
		}
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::Reset()
{
	clear_victorycondition_list();
	
	return LoadGameType( CRCD(0x34861a16,"freeskate") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::LoadGameType( uint32 gameTypeChecksum )
{
	GameNet::Manager* gamenet_man;
	int max_players;
	
	max_players = Mdl::Skate::vMAX_SKATERS;
	gamenet_man = GameNet::Manager::Instance();
	if( gamenet_man && gamenet_man->InNetGame())
	{
		max_players = GetMaximumNumberOfPlayers();
	}

	// Reset the current game mode first
	m_ModeDefinition.Clear();

	Script::CArray* pArray = Script::GetArray( CRCD(0xca5f7ffb,"mode_info"), Script::ASSERT );

	for ( uint32 i = 0; i < pArray->GetSize(); i++ )
	{
		Script::CScriptStructure* pStructure = pArray->GetStructure( i );
		Dbg_Assert( pStructure );

		uint32 checksum;
		pStructure->GetChecksum( CRCD(0x21902065,"checksum"), &checksum, true );
		
		if ( checksum == gameTypeChecksum )
		{

			const char* definition;
			pStructure->GetText( CRCD(0x97cfd027,"definition"), &definition, true );

			Script::CScriptStructure* pRefersToStructure = Script::GetStructure( definition, Script::ASSERT );
			m_ModeDefinition.AppendStructure( pRefersToStructure );

			// clears the existing lists, as we're about to rebuild them
			clear_victorycondition_list();
			
			// rebuilds the victory condition list based on the current game mode
			build_victorycondition_list();

			if( gamenet_man && gamenet_man->InNetGame())
			{
				SetMaximumNumberOfPlayers( max_players );
			}
			return true;
		}
	}

	Dbg_MsgAssert( 0,( "Unrecognized game type %08x %s", gameTypeChecksum, Script::FindChecksumName( gameTypeChecksum ) ));
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGameMode::OverrideOptions( Script::CScriptStructure *pParams )
{
	// first we should get rid of any named components that are duplicates
	// the trick may be not to use any named components?
	remove_component_if_exists( "time_limit_type", pParams );
	remove_component_if_exists( "victory_condition_type", pParams );
	remove_component_if_exists( "screenmode", pParams );
	
	m_ModeDefinition.AppendStructure( pParams );
	
	// clears the existing lists, as we're about to rebuild them
	clear_victorycondition_list();

	// rebuilds the victory condition list based on the current game mode
	build_victorycondition_list();

//#ifdef __USER_GARY__
#if 1
	Script::PrintContents( &m_ModeDefinition );
#endif
	

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | OverrideGameModeOptions | 
bool ScriptOverrideGameModeOptions( Script::CScriptStructure *pParams, Script::CScript *pScript )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	CGameMode* pGameMode = skate_mod->GetGameMode();
	return pGameMode->OverrideOptions( pParams );
}

} // namespace Game

