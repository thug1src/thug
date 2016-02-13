/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2002 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate5													**
**																			**
**	Module:			GameNet			 										**
**																			**
**	File name:		p_stats.cpp												**
**																			**
**	Created by:		04/30/03	-	SPG										**
**																			**
**	Description:	PS2 Stat-tracking Code 									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <gel/mainloop.h>
#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/net/server/netserv.h>

#include <objects/skater.h>

#include <GameNet/GameNet.h>
#include <Gamenet/ngps/p_buddy.h>
#include <Gamenet/ngps/p_content.h>
#include <GameNet/Ngps/p_stats.h>

#include <sk/components/skaterscorecomponent.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

extern bool g_CheatsEnabled;

namespace GameNet
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define vSTATS_CONNECT_TIMEOUT Tmr::Seconds( 10 )
#define vSTATS_REPORT_INTERVAL Tmr::Minutes( 1 )
#define vMAX_STATS_FILE_SIZE ( 150 * 1024 );
#define vMAX_ENTRIES_PER_LEVEL 3
#define vSTATS_PLAYER_COUNT 50

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static 	Tmr::Time	s_connect_start_time = 0;
uint32	s_levels[] = { 	0xf6c822d4, 0x7276630a, 0xd7720de9, 0xee1c63cf, 0x399bd4e8, 
						0xd0f0229, 0x9a44ec8, 0x9db7727c, 0x991b5359, 
						0x73be7e94, 0xa7ff414b, 0x8ca30405 };
char	s_stats_files[vNUM_RATINGS_FILES][32] = {
	"hs_at.txt",
	"bc_at.txt",
	"hs_mo.txt",
	"bc_mo.txt",
	"ratings.txt"
};

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

class SortableStats : public Lst::Node< SortableStats >
{
public:
	SortableStats( void );

	char	m_Name[32];
	uint32	m_Level;
	int		m_Score;
	int		m_Rating;
};
/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

void	StatsMan::parse_ratings( char* buffer )
{
	StatsKeeper* keeper;
	char* token;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	keeper = &m_stats_keepers[vFILE_RATINGS];
	token = strtok( buffer, ":" );
    
	while( token )
	{
		StatsPlayer* player;

		player = new StatsPlayer;
		keeper->m_Players.AddToTail( player );
		player->m_Score = atoi( token );
		
		// Now parse, looking for the rating
		token = strtok( NULL, ":" );
		// Guard against corrupt files
		if( token == NULL )
		{
			Mem::Manager::sHandle().PopContext();
			return;
		}

		player->m_Rating = atoi( token );
		token = strtok( NULL, "\n" );
		// Guard against corrupt files
		if( token == NULL )
		{
			Mem::Manager::sHandle().PopContext();
			return;
		}

		strcpy( player->m_Name, token );
        
		// Now parse, looking for the next rating
		token = strtok( NULL, ":" );
	}

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsMan::parse_score_list( int type, char* buffer, bool read_date )
{
	StatsLevel* level;
	char* token;
	uint32 level_crc;

	Dbg_Assert( type <= vFILE_BEST_COMBO_MONTHLY );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

    if( read_date )
	{
		token = strtok( buffer, "\n" );

		strcpy( m_stats_keepers[type].m_Date, token );
		Dbg_Printf( "Date: %s\n", m_stats_keepers[type].m_Date );
		token = strtok( NULL, ":" );
	}
	else
	{
		token = strtok( buffer, ":" );
	}
	
	while( token && ( stricmp( token, "Level" ) == 0 ))
	{
		token = strtok( NULL, "\n" );
		// Guard against corrupt files
		if( token == NULL )
		{
			Mem::Manager::sHandle().PopContext();
			return;
		}
		level_crc = atoi( token );
		level = new StatsLevel;
		level->m_Level = level_crc;

		m_stats_keepers[type].m_Levels.AddToTail( level );

		// Now parse, looking for a list of players and scores
		token = strtok( NULL, ":" );
		// Guard against corrupt files
		if( token == NULL )
		{
			Mem::Manager::sHandle().PopContext();
			return;
		}

		// Read until we reach another "level" or EOF
		while( token && stricmp( token, "Level" ))
		{
			StatsPlayer* player;

			player = new StatsPlayer;

			level->m_Players.AddToTail( player );
			player->m_Score = atoi( token );
			Dbg_Printf( "Score %d  ", player->m_Score );
			token = strtok( NULL, ":" );
			// Guard against corrupt files
			if( token == NULL )
			{
				Mem::Manager::sHandle().PopContext();
				return;
			}
			player->m_Rating = atoi( token );
			Dbg_Printf( "Rating %d  ", player->m_Rating );
			token = strtok( NULL, "\n" );
			// Guard against corrupt files
			if( token == NULL )
			{
				Mem::Manager::sHandle().PopContext();
				return;
			}
			strcpy( player->m_Name, token );
			Dbg_Printf( "Name %s\n", player->m_Name );

			// Now parse, looking for the next level or player
			token = strtok( NULL, ":" );
		}
	}

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

GHTTPBool StatsMan::s_stats_file_dl_complete( GHTTPRequest request, GHTTPResult result, char* buffer,
											  int buffer_len, void * param )
{
	ContentMan* content_man;

	content_man = (ContentMan*) param;

	Dbg_Printf( "Got result %d\n", result );

	content_man->SetPercentComplete( 100 );
	if( result == GHTTPSuccess )
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		StatsMan* stats_man;

		content_man->SetResult( vCONTENT_RESULT_SUCCESS );
		content_man->SetError( 0 );
		
		stats_man = gamenet_man->mpStatsMan;
		switch( stats_man->m_cur_file_index )
		{
			case vFILE_HIGH_SCORE_ALL_TIME:
			case vFILE_BEST_COMBO_ALL_TIME:
				stats_man->parse_score_list( stats_man->m_cur_file_index, buffer, false );
				break;
			case vFILE_HIGH_SCORE_MONTHLY:
			case vFILE_BEST_COMBO_MONTHLY:
			{
				stats_man->parse_score_list( stats_man->m_cur_file_index, buffer, true );
				break;
			}
			case vFILE_RATINGS:
			{
				stats_man->parse_ratings( buffer );
				break;
			}
		}
	}
	else
	{
		content_man->SetResult( vCONTENT_RESULT_FAILURE );
		content_man->SetError( result );
	}
	
	return GHTTPTrue;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsMan::s_stats_logic_code( const Tsk::Task< StatsMan >& task )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
	StatsMan& man = task.GetData();

	switch( man.m_state )
	{
		case vSTATS_STATE_WAITING:
			break;
		case vSTATS_STATE_CONNECTING:
			if(( Tmr::GetTime() - s_connect_start_time ) > vSTATS_CONNECT_TIMEOUT )
			{
				Script::RunScript( "StatsRetrievalFailedDialog" );
				man.Disconnect();
				break;
			}
			break;
		case vSTATS_STATE_CONNECTED:
			man.m_state = vSTATS_STATE_LOGGED_IN;
			man.Connected();
			break;
		case vSTATS_STATE_FAILED_LOG_IN:
			Script::RunScript( "StatsLoginFailedDialog" );
			man.Disconnect();
			break;
		case vSTATS_STATE_RETRIEVING:
			if(( Tmr::GetTime() - s_connect_start_time ) > vSTATS_CONNECT_TIMEOUT )
			{
				Script::RunScript( "StatsRetrievalFailedDialog" );
				man.Disconnect();
				break;
			}
			if( PersistThink() == 0 )
			{
				Script::RunScript( "StatsRetrievalFailedDialog" );
				man.Disconnect();
			}
			break;
		case vSTATS_STATE_LOGGED_IN:
			break;
		case vSTATS_STATE_IN_GAME:
			if(( Tmr::GetTime() - man.m_time_since_last_report ) > vSTATS_REPORT_INTERVAL )
			{
				man.ReportStats( false );
			}
			break;
	}


	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 	StatsMan::s_threaded_stats_connect( StatsMan* stats_man )
{
	int result;

	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	// Register this thread with the sockets API
	sockAPIregthr();

    sprintf( gcd_gamename, "thps5ps2" );
	sprintf( gcd_secret_key, "G2k8cF" );
	Dbg_Printf( "About to connect to stats server...\n" );
	result = InitStatsConnection( GameNet::vHOST_PORT );
	switch( result )
	{
		case GE_NODNS:	// Unable to resolve stats server DNS
			gamenet_man->mpStatsMan->m_state = vSTATS_STATE_FAILED_LOG_IN;
			Dbg_Printf( "Error connecting to stats server: No DNS\n" );
			break;
		case GE_NOSOCKET: // Unable to create data socket
			gamenet_man->mpStatsMan->m_state = vSTATS_STATE_FAILED_LOG_IN;
			Dbg_Printf( "Error connecting to stats server: No Socket\n" );
			break;
		case GE_NOCONNECT: // Unable to connect to stats server
			gamenet_man->mpStatsMan->m_state = vSTATS_STATE_FAILED_LOG_IN;
			Dbg_Printf( "Error connecting to stats server: No Connect\n" );
			break;
		case GE_DATAERROR: // Unable to receive challenge from stats server, or bad challenge
			gamenet_man->mpStatsMan->m_state = vSTATS_STATE_FAILED_LOG_IN;
			Dbg_Printf( "Error connecting to stats server: Data error\n" );
			break;
		case GE_NOERROR: // Connected to stats server and ready to send data
			Dbg_Printf( "Connected to stats server\n" );
			gamenet_man->mpStatsMan->m_state = vSTATS_STATE_CONNECTED;
			break;
		default:
			Dbg_Assert( 0 );
			break;
	}

	// Deregister this thread with the sockets API
	sockAPIderegthr();

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

StatsMan::StatsMan( void )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	m_stats_logic_task = new Tsk::Task< StatsMan > ( s_stats_logic_code, *this );
	m_logged_in = false;
	m_state = vSTATS_STATE_WAITING;
	m_cur_game = NULL;
	m_need_to_retrieve_stats = true;

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

StatsMan::~StatsMan( void )
{
	delete m_stats_logic_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsMan::Connected( void )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	m_logged_in = true;
	Script::RunScript( "stats_logged_in" );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::IsLoggedIn( void )
{
	m_logged_in = ( IsStatsConnected() == 1 );

	return m_logged_in;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::NeedToRetrieveStats( void )
{
	return m_need_to_retrieve_stats;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsMan::Connect( void )
{   
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	Thread::PerThreadStruct	net_thread_data;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
	
	m_state = vSTATS_STATE_CONNECTING;
	s_connect_start_time = Tmr::GetTime();

	Script::RunScript( "stats_log_in_profile" );

    net_thread_data.m_pEntry = s_threaded_stats_connect;
	net_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
	net_thread_data.m_pStackBase = gamenet_man->GetNetThreadStack();
	net_thread_data.m_iStackSize = vNET_THREAD_STACK_SIZE;
	net_thread_data.m_utid = 0x152;
	Thread::CreateThread( &net_thread_data );
	gamenet_man->SetNetThreadId( net_thread_data.m_osId );
	StartThread( gamenet_man->GetNetThreadId(), this );

	if( !m_stats_logic_task->InList())
	{
		mlp_man->AddLogicTask( *m_stats_logic_task );
	}
	
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsMan::Disconnect( void )
{   
    CloseStatsConnection();
	m_stats_logic_task->Remove();
	m_logged_in = false;
	m_state = vSTATS_STATE_WAITING;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::ScriptStatsLoggedIn(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	StatsMan* stats_man;

	stats_man = gamenet_man->mpStatsMan;
	return stats_man->IsLoggedIn();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::ScriptStatsLogIn(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		
	gamenet_man->mpStatsMan->Connect();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::ScriptStatsLogOff(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	StatsMan* stats_man;

	stats_man = gamenet_man->mpStatsMan;
	stats_man->Disconnect();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::ScriptReportStats(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	StatsMan* stats_man;
	bool final;

	final = pParams->ContainsFlag( CRCD(0x132aa339,"final"));
	stats_man = gamenet_man->mpStatsMan;
	stats_man->ReportStats( final );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsMan::s_stats_retrieval_callback( int localid, int profileid, persisttype_t type, 
											  int index, int success, char *data, int len, 
											  void *instance )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Stats* stats;

	Dbg_Printf( "********* Got stats for %d type %d index %d success %d data %s len %d\n",
				profileid, type, index, success, data, len );
	
	if( success < 1 )
	{
		return;
	}
	stats = gamenet_man->mpStatsMan->GetStats();

	// Rating
	if( index == 0 )
	{
		char* ptr;
        
		ptr = strtok( data, "\\" );
		Dbg_Printf( "Ptr: %s\n", ptr );
		if( ptr )
		{
			ptr = strtok( NULL, "\\" );
			Dbg_Printf( "(2)Ptr: %s\n", ptr );
			stats->m_rating = atoi( ptr );
			Dbg_Printf( "(3)Rating: %d\n", stats->m_rating );
		}
	}
	else
	{
		char* ptr;
        
		// Levels
		ptr = strtok( data, "\\" );
		if( ptr )
		{
			Dbg_Printf( "**** 1\n" );
			if( stricmp( ptr, "HighScore" ) == 0 )
			{
				Dbg_Printf( "**** 2\n" );
				ptr = strtok( NULL, "\\" );
				if( ptr )
				{
					stats->m_highscore[index - 1] = atoi( ptr );
					Dbg_Printf( "**** 3 %d\n", stats->m_highscore[index -1 ] );
				}

				ptr = strtok( NULL, "\\" );
			}
			
			if( stricmp( ptr, "HighCombo" ) == 0 )
			{
				Dbg_Printf( "**** 4\n" );
				ptr = strtok( NULL, "\\" );
				if( ptr )
				{
					stats->m_bestcombo[index - 1] = atoi( ptr );
					Dbg_Printf( "**** 5 %d\n", stats->m_bestcombo[index -1 ] );
				}
			}
		}

		if( index == ( vNUM_TRACKED_LEVELS - 1 ))
		{
			Script::RunScript( "stats_retrieved" );
			gamenet_man->mpStatsMan->m_state = vSTATS_STATE_LOGGED_IN;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::ScriptRetrievePersonalStats(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	int i;

	Dbg_Printf( "Requesting rating\n" );
	GetPersistDataValues( 0, gamenet_man->mpBuddyMan->GetProfile(), pd_public_ro, 0,
							  "\\Rating", s_stats_retrieval_callback, NULL );
	for( i = 0; i < vNUM_TRACKED_LEVELS; i++ )
	{
		GetPersistDataValues( 0, gamenet_man->mpBuddyMan->GetProfile(), pd_public_ro, i + 1,
								  "\\HighScore\\HighCombo", s_stats_retrieval_callback, NULL );
		
	}
	
	gamenet_man->mpStatsMan->m_state = vSTATS_STATE_RETRIEVING;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::ScriptCleanUpTopStats(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	StatsMan* stats_man;
	Script::CArray *p_high_scores;
	int i;

	stats_man = gamenet_man->mpStatsMan;
	if( stats_man->m_need_to_retrieve_stats == true )
	{
		return true;
	}

	for( i = 0; i < vNUM_RATINGS_FILES; i++ )
	{
		stats_man->m_stats_keepers[i].Cleanup();
	}
	stats_man->m_need_to_retrieve_stats = true;
	 
	p_high_scores = Script::GetArray("high_scores_array_all_time");
	Script::CleanUpArray( p_high_scores );
	p_high_scores = Script::GetArray("best_combos_array_all_time");
	Script::CleanUpArray( p_high_scores );
	p_high_scores = Script::GetArray("high_scores_array_monthly");
	Script::CleanUpArray( p_high_scores );
	p_high_scores = Script::GetArray("best_combos_array_monthly");
	Script::CleanUpArray( p_high_scores );
	p_high_scores = Script::GetArray("top_players_array");
	Script::CleanUpArray( p_high_scores );
	p_high_scores = Script::GetArray("personal_stats_array");
	Script::CleanUpArray( p_high_scores );

	ghttpCleanup();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::ScriptGetRank(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	StatsMan* stats_man;
	int rank, rating;
	 
	stats_man = gamenet_man->mpStatsMan;
	rating = stats_man->m_stats.GetRating();
	rank = (int) (((float) rating / (float) vMAX_RATING ) * (float) vMAX_RANK );
	pScript->GetParams()->AddInteger( CRCD(0x7786171a,"rank"), rank ); 

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::ScriptFillStatsArrays(Script::CStruct *pParams, Script::CScript *pScript)
{
	//Script::CArray *p_high_scores;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	StatsMan* stats_man;
	uint32 array;
	 
	stats_man = gamenet_man->mpStatsMan;

	pParams->GetChecksum( CRCD(0x5ef31148,"array"), &array, true );
	Dbg_Printf( "Array 0x%x\n", array );
	switch( array )
	{
		case 0xcb95928f:	//high_scores_array_monthly
			stats_man->m_stats_keepers[vFILE_HIGH_SCORE_MONTHLY].FillSectionedMenu();
			stats_man->m_stats_keepers[vFILE_HIGH_SCORE_MONTHLY].FillMenu();
			break;
		case 0x67f1ae8a:	//best_combos_array_monthly
			stats_man->m_stats_keepers[vFILE_BEST_COMBO_MONTHLY].FillSectionedMenu();
			stats_man->m_stats_keepers[vFILE_BEST_COMBO_MONTHLY].FillMenu();
			break;
		case 0x5299291b: 	//top_players_array
			Dbg_Printf( "Filling top players menu\n" );
			stats_man->m_stats_keepers[vFILE_RATINGS].FillMenu( true );
			break;
		case 0xef20d3a1:	//high_scores_array_all_time
			stats_man->m_stats_keepers[vFILE_HIGH_SCORE_ALL_TIME].FillSectionedMenu();
			stats_man->m_stats_keepers[vFILE_HIGH_SCORE_ALL_TIME].FillMenu();
			break;
		case 0x9fe64312:	//best_combos_array_all_time
			stats_man->m_stats_keepers[vFILE_BEST_COMBO_ALL_TIME].FillSectionedMenu();
			stats_man->m_stats_keepers[vFILE_BEST_COMBO_ALL_TIME].FillMenu();
			break;
		case 0xfef84fff:	//personal_stats_array
			stats_man->m_stats.FillMenu();
			break;
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::ScriptRetrieveTopStats(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	StatsMan* stats_man;
	DownloadContext context;
	int file_index;

	stats_man = gamenet_man->mpStatsMan;

	file_index = 0;
	if( pParams->GetInteger( CRCD(0x7360c9ef,"file"), &file_index ))
	{
		stats_man->m_cur_file_index = file_index;
	}
	else
	{
		stats_man->m_cur_file_index++;
	}
    
	sprintf( context.m_path, "http://%s/thug/%s", SSHOST, s_stats_files[stats_man->m_cur_file_index] );
	context.m_file_not_found_script = CRCD(0x1c049404,"StatsRetrievalFailed");
	context.m_transfer_failed_script = CRCD(0x1c049404,"StatsRetrievalFailed");
	context.m_update_progress_script = CRCD(0x74e0cd35,"do_nothing");
	context.m_max_size = vMAX_STATS_FILE_SIZE;

	if( stats_man->m_cur_file_index == ( vNUM_RATINGS_FILES - 1 ))
	{
		context.m_transfer_complete_script = CRCD(0x7346e8e7,"stats_retrieval_complete");
	}
	else
	{
		context.m_transfer_complete_script = CRCD(0xd932b07b,"download_more_stats");
	}
	
	context.m_dl_complete = s_stats_file_dl_complete;

	gamenet_man->mpContentMan->DownloadFile( &context );

	stats_man->m_need_to_retrieve_stats = false;

    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	StatsMan::ScriptNeedToRetrieveTopStats(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	StatsMan* stats_man;
	
	stats_man = gamenet_man->mpStatsMan;
	return stats_man->NeedToRetrieveStats();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsMan::StartNewGame( void )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Script::CScriptStructure* pStructure;
	Prefs::Preferences* pPreferences;
	const char* server_name, *challenge;
	int score_limit, time_limit;
	Lst::Search< PlayerInfo > sh;
	PlayerInfo* player;
	Net::Server* server;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	m_cur_game = NewGame( 1 );
	challenge = GetChallenge( m_cur_game );
	server = gamenet_man->GetServer();
	Dbg_Assert( server != NULL );

	pPreferences = gamenet_man->GetNetworkPreferences();
	pStructure = pPreferences->GetPreference( Script::GenerateCRC("server_name") );
	pStructure->GetText( "ui_string", &server_name, true );

	time_limit = pPreferences->GetPreferenceValue( Script::GenerateCRC("time_limit"), Script::GenerateCRC("time") );
	score_limit = pPreferences->GetPreferenceValue( Script::GenerateCRC("target_score"), Script::GenerateCRC("score") );

    BucketStringOp( m_cur_game, "hostname", bo_set, (char*) server_name, bl_server, 0 );
	BucketStringOp( m_cur_game, "mapname", bo_set, gamenet_man->GetLevelName(), bl_server, 0 );
	BucketStringOp( m_cur_game, "gametype", bo_set, gamenet_man->GetGameModeName(), bl_server, 0 );
	BucketIntOp( m_cur_game, "mapcrc", bo_set, gamenet_man->GetNetworkLevelId(), bl_server, 0 );
	BucketIntOp( m_cur_game, "gametypecrc", bo_set, gamenet_man->GetGameTypeFromPreferences(), bl_server, 0 );
	BucketIntOp( m_cur_game, "maxplayers", bo_set, gamenet_man->GetMaxPlayers(), bl_server, 0 );
	BucketIntOp( m_cur_game, "timelimit", bo_set, time_limit, bl_server, 0 );
	BucketIntOp( m_cur_game, "fraglimit", bo_set, score_limit, bl_server, 0 );

	for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
	{
		char key_name[64];
		Net::MsgDesc msg;
		uint32 id;

		id = player->m_Skater->GetID();

		NewPlayer( m_cur_game, id, player->m_Name );
		
		sprintf( key_name, "player" );
		BucketStringOp( m_cur_game, key_name, bo_set, player->m_Name, bl_player, id );
		sprintf( key_name, "pid" );
		BucketIntOp( m_cur_game, key_name, bo_set, player->m_Profile, bl_player, id );

		if( player->m_Profile > 0 )
		{
			msg.m_Data = (char*) challenge;
			msg.m_Length = strlen( challenge ) + 1;
			msg.m_Id = GameNet::MSG_ID_CHALLENGE;
			msg.m_Queue = Net::QUEUE_SEQUENCED;
			msg.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	
			server->EnqueueMessage( player->GetConnHandle(), &msg );
		}
	}

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 	StatsMan::ReportStats( bool final )
{
	int result, cheating_occurred;
	Lst::Search< PlayerInfo > sh;
	PlayerInfo* player;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	if( m_cur_game == NULL )
	{
		return;
	}
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
	{
		uint32 id;
		Mdl::Score* score_obj;
				
		Obj::CSkaterScoreComponent* p_skater_score_component = GetSkaterScoreComponentFromObject( player->m_Skater );
		Dbg_Assert(p_skater_score_component);
	
		score_obj = p_skater_score_component->GetScore();

		id = player->m_Skater->GetID();
		
        BucketIntOp( m_cur_game, "score", bo_set, gamenet_man->GetPlayerScore( id ), bl_player, id );
		Dbg_Printf( "**** PLAYER %d, Best combo: %d\n", player->m_Skater->GetID(), player->m_BestCombo );
		BucketIntOp( m_cur_game, "combo", bo_set, player->m_BestCombo, bl_player, id );
	}

	// 'highscore' will really means 'cheats', but we don't want to give hackers any hints. 
	// It is meant to be misleading.
	cheating_occurred = g_CheatsEnabled | gamenet_man->HasCheatingOccurred();
	BucketIntOp( m_cur_game, "highscore", bo_set, cheating_occurred, bl_server, 0 );
    result = SendGameSnapShot( m_cur_game, NULL, (int) final );
	
	switch( result )
	{
		case GE_DATAERROR: 	// If game is NULL and the last game created by NewGame failed (because the connection was lost and disk logging is disabled)
			Dbg_Printf( "Error sending stats\n" );
			break;
		case GE_NOCONNECT: // If the connection is lost and disk logging is disabled
			Dbg_Printf( "Error sending stats : No connect\n" );
			break;
		case GE_NOERROR: //The update was sent, or disk logging is enabled and the game was logged
			Dbg_Printf( "Sent stats\n" );
			break;
	}

	if( final )
	{
		EndGame();
	}

	m_time_since_last_report = Tmr::GetTime();

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsMan::EndGame( void )
{
	m_state = vSTATS_STATE_LOGGED_IN;
	if( m_cur_game )
	{
		FreeGame( m_cur_game );
		m_cur_game = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsMan::PlayerLeft( int id )
{
	if( m_state == vSTATS_STATE_IN_GAME )
	{
		RemovePlayer( m_cur_game, id );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*	StatsMan::GenerateAuthResponse( char* challenge, char* password, char* response )
{
	return GenerateAuth( challenge, password, response );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsMan::AuthorizePlayer( int id, char* response )
{
	char key_name[64];

	sprintf( key_name, "auth" );
	BucketStringOp( m_cur_game, key_name, bo_set, response, bl_player, id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Stats*	StatsMan::GetStats( void )
{
	return &m_stats;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*	StatsMan::GetLevelName( uint32 level_crc )
{
	int i;
	Script::CArray* pArray = Script::GetArray("stats_level_names");
	Dbg_Assert( pArray );

	for( i = 0; i < (int)pArray->GetSize(); i++ )
	{   
		uint32 checksum;
		Script::CStruct* pStructure = pArray->GetStructure( i );
		Dbg_Assert( pStructure );

        pStructure->GetChecksum( "level", &checksum );
		if( level_crc == checksum )
		{
			const char* level_name;

			pStructure->GetText( "text", &level_name, true );
			return (char *) level_name;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Stats::Stats( void )
{
	m_rating = 0;
	memset( m_highscore, 0, sizeof( uint32 ) * vNUM_TRACKED_LEVELS );
	memset( m_bestcombo, 0, sizeof( uint32 ) * vNUM_TRACKED_LEVELS );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Stats::GetRating( void )
{
	return m_rating;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Stats::GetHighScore( uint32 level )
{
	int i;

	for( i = 0; i < vNUM_TRACKED_LEVELS; i++ )
	{
		if( level == s_levels[i] )
		{
			return m_highscore[i];
		}
	}

	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Stats::GetBestCombo( uint32 level )
{
	int i;

	for( i = 0; i < vNUM_TRACKED_LEVELS; i++ )
	{
		if( level == s_levels[i] )
		{
			return m_bestcombo[i];
		}
	}

	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Stats::FillMenu( void )
{
	Lst::Search< SortableStats > sh;
	SortableStats* p_stats, *next;
	Lst::Head< SortableStats > stat_list;
	int i, count, num_items;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	StatsMan* stats_man;

	stats_man = gamenet_man->mpStatsMan;

	num_items = 0;
	for( i = 0; i < vNUM_TRACKED_LEVELS; i++ )
	{
		if( m_highscore[i] > 0 )
		{
			p_stats = new SortableStats;
			p_stats->SetPri( m_highscore[i] );
			p_stats->m_Score = m_highscore[i];
			p_stats->m_Level = s_levels[i];

			stat_list.AddNode( p_stats );
			num_items++;
		}
	}

	count = 0;
	for( p_stats = sh.FirstItem( stat_list ); p_stats; p_stats = sh.NextItem())
	{
		Script::CStruct* p_entry;
			
		p_entry = new Script::CStruct;
		p_entry->AddInteger( "score", p_stats->m_Score );
		p_entry->AddString( "level", stats_man->GetLevelName( p_stats->m_Level ));

        Script::RunScript( CRCD(0xa6111f30,"add_stat_personal_menu_item"), p_entry );
		//Script::RunScript( CRCD(0xac09e3b,"add_stat_player_menu_item"), p_entry );
	}
	for( p_stats = sh.FirstItem( stat_list ); p_stats; p_stats = next )
	{
		next = sh.NextItem();
		delete p_stats;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

StatsPlayer::StatsPlayer( void )
: Lst::Node< StatsPlayer > ( this )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

StatsLevel::StatsLevel( void )
: Lst::Node< StatsLevel > ( this )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

StatsLevel::~StatsLevel( void )
{
	Lst::Search< StatsPlayer > sh;
	StatsPlayer* player, *next;

	for( player = sh.FirstItem( m_Players ); player; player = next )
	{
		next = sh.NextItem();
		delete player;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		StatsKeeper::NumEntries( int max_entries_per_level )
{
	int num_entries;
	Lst::Search< StatsLevel > sh;
	Lst::Search< StatsPlayer > player_sh;
	StatsLevel* level;
	StatsPlayer* player;
    
	num_entries = 0;
	for( level = sh.FirstItem( m_Levels ); level; level = sh.NextItem())
	{
		num_entries += ( (int) level->m_Players.CountItems() > max_entries_per_level ) ? 
							max_entries_per_level : level->m_Players.CountItems();
	}

	for( player = player_sh.FirstItem( m_Players ); player; player = player_sh.NextItem())
	{
		num_entries++;
	}

	return num_entries;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsKeeper::FillSectionedMenu( void )
{
	int rank, count, num_items;
	Lst::Search< StatsLevel > sh;
	Lst::Search< StatsPlayer > player_sh;
	StatsLevel* level;
	StatsPlayer* player;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	StatsMan* stats_man;
	Script::CStruct* p_header;

	stats_man = gamenet_man->mpStatsMan;
	num_items = NumEntries( vMAX_ENTRIES_PER_LEVEL );
	count = 0;
	
	Dbg_Printf( "*** Filling sectioned menu\n" );
	for( level = sh.FirstItem( m_Levels ); level; level = sh.NextItem())
	{
		int player_count;

		if( level->m_Players.CountItems() > 0)
		{
			Script::CStruct* p_level_entry;

			p_level_entry = new Script::CStruct;
			p_level_entry->AddString( "text", stats_man->GetLevelName( level->m_Level ));
			Script::RunScript( CRCD(0x1aadee21,"add_stat_header_menu_item"), p_level_entry );

			delete p_level_entry;
		}
		

		player_count = 0;
		for( player = player_sh.FirstItem( level->m_Players ); player; player = player_sh.NextItem())
		{
			Script::CStruct* p_entry;
			
			if( player_count >= vMAX_ENTRIES_PER_LEVEL )
			{
				break;
			}

			p_entry = new Script::CStruct;
			p_entry->AddInteger( "rating", player->m_Rating );
			p_entry->AddInteger( "score", player->m_Score );
			rank = (int) (((float) player->m_Rating / (float) vMAX_RATING ) * (float) vMAX_RANK );
			p_entry->AddInteger( "rank", rank );
			p_entry->AddString( "name", player->m_Name );
			p_entry->AddString( "level", stats_man->GetLevelName( level->m_Level ));

			Script::RunScript( CRCD(0x19f7bc28,"add_stat_score_menu_item"), p_entry );
			
			delete p_entry;

			player_count++;
		}
	}

    p_header = new Script::CStruct;
	p_header->AddString( "text", Script::GetLocalString( CRCD(0xc2283319,"category_all_levels")));
	Script::RunScript( CRCD(0x1aadee21,"add_stat_header_menu_item"), p_header );
	delete p_header;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsKeeper::FillMenu( bool just_ratings )
{
	int rank;
	Lst::Search< StatsLevel > sh;
	Lst::Search< StatsPlayer > player_sh;
	StatsLevel* level;
	StatsPlayer* player;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	StatsMan* stats_man;
	SortableStats* p_stats, *next;
	Lst::Head< SortableStats > stat_list;
	Lst::Search< SortableStats > stats_sh;
	int count;

	stats_man = gamenet_man->mpStatsMan;
	if( just_ratings )
	{
		for( player = player_sh.FirstItem( m_Players ); player; player = player_sh.NextItem())
		{
			p_stats = new SortableStats;
			p_stats->SetPri( player->m_Rating );
			p_stats->m_Score = player->m_Score;
			p_stats->m_Rating = player->m_Rating;
			strcpy( p_stats->m_Name, player->m_Name );
		
			stat_list.AddNode( p_stats );
		}
	}
	else
	{
		for( level = sh.FirstItem( m_Levels ); level; level = sh.NextItem())
		{
			for( player = player_sh.FirstItem( level->m_Players ); player; player = player_sh.NextItem())
			{
				p_stats = new SortableStats;
				p_stats->SetPri( player->m_Score );
				p_stats->m_Score = player->m_Score;
				p_stats->m_Level = level->m_Level;
				p_stats->m_Rating = player->m_Rating;
				strcpy( p_stats->m_Name, player->m_Name );
			
				stat_list.AddNode( p_stats );
			}
		}
	}
			
	count = 0;
	for( p_stats = stats_sh.FirstItem( stat_list ); p_stats; p_stats = stats_sh.NextItem())
	{
		Script::CStruct* p_entry;

		if(( p_stats->m_Score == 0 ) || ( count == vSTATS_PLAYER_COUNT ))
		{
			break;
		}
		p_entry = new Script::CStruct;
		p_entry->AddInteger( "rating", p_stats->m_Rating );
		p_entry->AddInteger( "score", p_stats->m_Score );
		rank = (int) (((float) p_stats->m_Rating / (float) vMAX_RATING ) * (float) vMAX_RANK );
		p_entry->AddInteger( "rank", rank );
		p_entry->AddString( "name", p_stats->m_Name );

		if( just_ratings )
		{
			Script::RunScript( CRCD(0xac09e3b,"add_stat_player_menu_item"), p_entry );
		}
		else
		{
			p_entry->AddString( "level", stats_man->GetLevelName( p_stats->m_Level ));
			Script::RunScript( CRCD(0x19f7bc28,"add_stat_score_menu_item"), p_entry );
		}
		delete p_entry;
		count++;
	}

	for( p_stats = stats_sh.FirstItem( stat_list ); p_stats; p_stats = next )
	{
		next = stats_sh.NextItem();
		delete p_stats;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StatsKeeper::Cleanup( void )
{
	Lst::Search< StatsLevel > sh;
	Lst::Search< StatsPlayer > player_sh;
	StatsLevel* level, *next;
	StatsPlayer* player, *next_player;

	for( level = sh.FirstItem( m_Levels ); level; level = next )
	{
		next = sh.NextItem();
		delete level;
	}

	for( player = player_sh.FirstItem( m_Players ); player; player = next_player )
	{
		next_player = player_sh.NextItem();
		delete player;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

SortableStats::SortableStats( void )
: Lst::Node< SortableStats > ( this )
{
	m_Score = 0;
	m_Level = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace GameNet




