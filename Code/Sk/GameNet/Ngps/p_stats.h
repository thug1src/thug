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
**	File name:		p_stats.h												**
**																			**
**	Created by:		04/30/03	-	SPG										**
**																			**
**	Description:	PS2 Stat-tracking code 									**
**																			**
*****************************************************************************/

#ifndef __GAMENET_NGPS_P_STATS_H
#define __GAMENET_NGPS_P_STATS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <gstats/gstats.h>
#include <gstats/gpersist.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define vNUM_TRACKED_LEVELS	12

namespace GameNet
{
	
enum
{
	vFILE_HIGH_SCORE_ALL_TIME,
	vFILE_BEST_COMBO_ALL_TIME,
	vFILE_HIGH_SCORE_MONTHLY,
	vFILE_BEST_COMBO_MONTHLY,
	vFILE_RATINGS,
	vNUM_RATINGS_FILES
};

enum
{
	vSTATS_STATE_WAITING,
	vSTATS_STATE_CONNECTING,
	vSTATS_STATE_CONNECTED,
	vSTATS_STATE_FAILED_LOG_IN,
	vSTATS_STATE_LOGGED_IN,
	vSTATS_STATE_RETRIEVING,
	vSTATS_STATE_IN_GAME,
};


class Stats
{
	friend class StatsMan;

public:
	Stats( void );

	int		GetRating( void );
	int		GetHighScore( uint32 level );
	int		GetBestCombo( uint32 level );

	void	FillMenu( void );
private:
	int		m_rating;
	uint32	m_highscore[vNUM_TRACKED_LEVELS];
	uint32	m_bestcombo[vNUM_TRACKED_LEVELS];
};

class StatsPlayer : public Lst::Node< StatsPlayer >
{
public:
	enum
	{
		vMAX_STATS_PLAYER_NAME_LEN = 32
	};
	StatsPlayer( void );
	
	char	m_Name[vMAX_STATS_PLAYER_NAME_LEN];
	uint32	m_Rating;
	uint32	m_Score;
};

class StatsLevel : public Lst::Node< StatsLevel >
{
public:
	StatsLevel( void );
	~StatsLevel( void );

	uint32		m_Level;
	Lst::Head< StatsPlayer >	m_Players;
};

class StatsKeeper 
{
public:
	
	void	Cleanup( void );
	int		NumEntries( int max_entries_per_level );
	void	FillMenu( bool just_ratings = false );
	void	FillSectionedMenu( void );
	
	Lst::Head< StatsLevel > 	m_Levels;
	Lst::Head< StatsPlayer > 	m_Players;
	char	m_Date[32];
};

class StatsMan
{
public:
	StatsMan( void );
	~StatsMan( void );

	void	Connect( void );
	void	Disconnect( void );
	void	Connected( void );
	bool	IsLoggedIn( void );
	bool	NeedToRetrieveStats( void );
	char*	GetLevelName( uint32 level_crc );

	void	StartNewGame( void );
	void	ReportStats( bool final );
	char*	GenerateAuthResponse( char* challenge, char* password, char* response );
	void	AuthorizePlayer( int id, char* response );
	void	PlayerLeft( int id );
	void	EndGame( void );
	Stats*	GetStats( void );

	static	bool		ScriptStatsLoggedIn(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptStatsLogOff(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptStatsLogIn(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptReportStats(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptRetrievePersonalStats(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptRetrieveTopStats(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptNeedToRetrieveTopStats(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptCleanUpTopStats(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptFillStatsArrays(Script::CStruct *pParams, Script::CScript *pScript);
	static	bool		ScriptGetRank(Script::CStruct *pParams, Script::CScript *pScript);
    
private:
	static	void s_threaded_stats_connect( StatsMan* stats_man );
	static	void s_stats_retrieval_callback(int localid, int profileid, persisttype_t type, 
											int index, int success, char *data, int len, 
											void *instance );
	static	GHTTPBool s_stats_file_dl_complete(	GHTTPRequest request,       // The request.
											GHTTPResult result,         // The result (success or an error).
											char* buffer,              // The file's bytes (only valid if ghttpGetFile[Ex] was used).
											int buffer_len,              // The file's length.
											void * param                // User-data.
											);

	void	parse_score_list( int type, char* buffer, bool read_date );
	void	parse_ratings( char* buffer );

	Tsk::Task< StatsMan >*					m_stats_logic_task;
	static	Tsk::Task< StatsMan >::Code   	s_stats_logic_code;

	bool		m_logged_in;
	int			m_state;
	statsgame_t	m_cur_game;
	Tmr::Time	m_time_since_last_report;
	Stats		m_stats;

	int			m_cur_file_index;
	StatsKeeper	m_stats_keepers[vNUM_RATINGS_FILES];
	bool		m_need_to_retrieve_stats;
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

} // namespace GameNet

#endif	// __GAMENET_NGPS_P_STATS_H


