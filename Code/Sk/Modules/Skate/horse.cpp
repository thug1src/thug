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
**	File name:		horse.cpp												**
**																			**
**	Created by:		07/17/01	-	Gary									**
**																			**
**	Description:	<description>		 									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/modules/skate/horse.h>

#include <core/defines.h>
#include <core/math/math.h>						 // for Mth::Rnd()
#include <core/string/stringutils.h>

#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <sk/objects/restart.h>
#include <sk/objects/skater.h>

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

static uint32 s_horse_status[] = {
   0x0fd80eba,	// GotLetter
   0x006be86f,	// TieScore
   0xbaa12466,	// BeatScore
   0xae7d29b0,	// Ended
   0x23db4aea,	// Idle
   0xa71dd8eb,	// NoScoreSet
   0			// Terminator
};

// should match the above
enum
{
	vGOTLETTER = 0,
	vTIESCORE,
	vBEATSCORE,
	vENDED,
	vIDLE,
	vNOSCORESET,
};

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static uint32 get_num_horse_restarts()
{
	uint32 dummy_nodes[128];
	return Obj::FindQualifyingNodes( Script::GenerateCRC("horse"), &dummy_nodes[0] );
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CHorse::CHorse()
{
	strcpy( m_string, "HORSE" );

	m_currentRestartIndex = 0;
}
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CHorse::Init()
{
	// reset the score to beat
	m_scoreToBeat = 0;

	// setting the "current horse player"
	m_currentSkaterId = 0;
	m_nextSkaterId = 0;
	sprintf( m_nextHorseMessage, "unknown" );

	// resetting each of their accumulated horse words
	for ( int i = 0; i < Skate::vMAX_SKATERS; i++ )
	{
		m_numLetters[i] = 0;
	}

	// select a random restart point
	uint32 num_restarts = get_num_horse_restarts();
	//Dbg_MsgAssert ( num_restarts > 0, ( "Couldn't find any horse restarts" ) );
	Dbg_Printf( "-- COULDN'T FIND ANY HORSE RESTARTS --\n" );
	m_currentRestartIndex = Mth::Rnd( num_restarts );

	// maybe do some kind of handicapping?
	// or separate words for each player?
	
	// their panels need to be suspended, but that's
	// done in the panel manager code.
	// TODO:  move it to some accessor in the panel stuff
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CHorse::GetCurrentSkaterId()
{
	return m_currentSkaterId;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CHorse::StartRun()
{
	
	
	//Front::MenuFactory* menu_factory = Front::MenuFactory::Instance();

	//m_currentSkaterId = m_nextSkaterId;

	m_status = s_horse_status[vIDLE];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CHorse::EndRun()
{
	// calculate the score post-run
	// update the horse data structure so that we know that
	// the score to beat is

	sprintf( m_nextHorseMessage, "unknown" );
	
	// by default, don't change the skater
	m_nextSkaterId = m_currentSkaterId;

	int score = get_score( m_currentSkaterId );

	printf( "End run = %d\n", score );

	if ( score > m_scoreToBeat )
	{
		// current player wins the run
		m_scoreToBeat = score;

		sprintf( m_nextHorseMessage, "%s %s", Script::GetLocalString( "horse_str_setscoreis" ), Str::PrintThousands(m_scoreToBeat) );
		m_status = s_horse_status[vBEATSCORE];
	}
	else if ( score < m_scoreToBeat )
	{
		// current player loses the run
		m_scoreToBeat = 0;
		
		m_numLetters[ m_currentSkaterId ]++;

		// skip to next restart point
		SetNextRestartPoint();
		
		if ( Ended() )
		{
			// GJ:  this string is never used...  it says "Player 1 is a horse" somewhere else
			sprintf( m_nextHorseMessage, "You are a %s!", m_string );

			m_status = s_horse_status[vENDED];
		}
		else
		{
			char letter[2];
			letter[0] = *(m_string + m_numLetters[m_currentSkaterId] - 1);
			// make it uppercase
			if ( letter[0] >= 'a' && letter[0] <= 'z' )
			{
				letter[0] -= 'a';
				letter[0] += 'A';
			}
			letter[1] = 0;
			sprintf( m_nextHorseMessage, "%s %s", Script::GetLocalString( "horse_str_getstheletter" ), letter );
			m_status = s_horse_status[vGOTLETTER];
		}
	}
	else
	{
		// current player draws
		if ( score == 0 )
		{
			strcpy( m_nextHorseMessage, Script::GetLocalString( "horse_str_noscoreset" ));
			m_status = s_horse_status[vNOSCORESET];
		}
		else
		{
			strcpy( m_nextHorseMessage, Script::GetLocalString( "horse_str_youtiedthetargetscore" ));
			m_status = s_horse_status[vTIESCORE];
		}
	}

#ifdef __USER_GARY__
	printf( "Current Score:  %s - %s\n",
			GetWordForPlayer( 0 ).getString(),
			GetWordForPlayer( 1 ).getString() );
#endif
	
	//SwitchPlayers();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CHorse::SwitchPlayers()
{
	m_nextSkaterId = get_next_valid_skater_id();
	m_currentSkaterId = m_nextSkaterId;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CHorse::StatusEquals( Script::CStruct* pParams )
{
	int i = 0;
	while ( s_horse_status[i] )
	{
		if ( pParams->ContainsFlag( s_horse_status[i] ) )
		{
			return ( m_status == s_horse_status[i] );
		}
		
		i++;
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CHorse::Ended()
{
	for ( int i = 0; i < Skate::vMAX_SKATERS; i++ )
	{
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		if ( !skate_mod->GetSkaterById( i ) )
			continue;

		// if the skater exists...
		if ( m_numLetters[i] >= strlen( m_string ) )
		{
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CHorse::SetWord( Str::String word )
{
	// validate string
	// make sure there's at least 1 letter

	Dbg_Assert( word.getString() && strlen( word.getString() ) > 0 );

	strcpy( m_string, word.getString());

	printf( "New word = %s\n", m_string );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Str::String CHorse::GetWordForPlayer( int skater_id )
{
	

	Dbg_Assert( skater_id >= 0 && skater_id < Skate::vMAX_SKATERS );
	
	char msg[256];
	
	strcpy( msg, m_string );
	for ( uint32 i = 0; i < strlen(msg); i++ )
	{
		if ( i >= m_numLetters[ skater_id ] )
			msg[i] = '-';
	}

	return msg;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char* CHorse::GetWord()
{
	return m_string;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CHorse::get_next_valid_skater_id()
{
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	Obj::CSkater *pSkater = NULL;
	int check_id = 0;

	for ( int i = 0; i < Skate::vMAX_SKATERS; i++ )
	{
		check_id = ( m_currentSkaterId + i + 1 ) % Skate::vMAX_SKATERS;
		pSkater = skate_mod->GetSkaterById( check_id );
		if ( pSkater )
			break;
	}
	
	Dbg_Assert( pSkater );

	printf( "new skater id = %d\n", check_id );

	if ( Script::GetInteger( "debug_horse" ) )
	{
		// DEBUGGING
		check_id = 0;
	}
	
	return check_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CHorse::get_score( int skater_id )
{
	

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater *pSkater = skate_mod->GetSkaterById( skater_id );
	Dbg_Assert( pSkater );

	Mdl::Score * pScore = ( pSkater->GetScoreObject() );
	Dbg_Assert( pScore );

	return pScore->GetTotalScore();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Str::String	CHorse::GetString( uint32 checksum )
{
	
	
	char msg[256];
	strcpy( msg, "Unknown" );

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater *pSkater = skate_mod->GetSkaterById( m_currentSkaterId );
	Dbg_Assert( pSkater );

	switch ( checksum )
	{
		case 0x9196d920: // playerName
		{
//			int currentPlayerNumber = pSkater->GetID() + 1;
//			sprintf( msg, "Player %d", currentPlayerNumber );
			sprintf( msg, "%s", skate_mod->GetSkaterById( pSkater->GetID() )->GetDisplayName() );
		}
		break;
		
		case 0x5078bc7c: // horsePreRun
		{
			if ( m_scoreToBeat > 0 )
			{
				sprintf( msg, "%s %s", Script::GetLocalString( "horse_str_scoretobeat" ), Str::PrintThousands(m_scoreToBeat) );
			}
			else
			{
				sprintf( msg, Script::GetLocalString( "horse_str_setscore" ) );
			}
		}
		break;
		
		case 0x2a13f850: // horsePostRun
		{
			strcpy( msg, m_nextHorseMessage );
		}
		break;

		case 0xde802177: // panelString1
		{
			sprintf( msg, "%s: %s", skate_mod->GetSkaterById( 0 )->GetDisplayName(), GetWordForPlayer( 0 ).getString() );
		}
		break;

		case 0x478970cd: // panelString2
		{
			sprintf( msg, "%s: %s", skate_mod->GetSkaterById( 1 )->GetDisplayName(), GetWordForPlayer( 1 ).getString() );
		}
		break;

		case 0x2523853d: // youAreA
		{
			bool starts_with_vowel = false;
			char first_letter = GetWord()[0];
			if ( first_letter >= 'a' && first_letter <= 'z' )
			{
				// make them upper case
				first_letter -= 'a';
				first_letter += 'A';
			}
			if ( first_letter == 'A' || first_letter == 'E' || first_letter == 'I'
				 || first_letter == 'O' || first_letter == 'U' )
			{
				starts_with_vowel = true;
			}

			sprintf( msg, "%s %s", GetString( Script::GenerateCRC("playerName") ).getString(), starts_with_vowel ?  Script::GetLocalString( "horse_str_isan" ) :  Script::GetLocalString( "horse_str_isa" ) );
		}
		break;

		case 0x5f7d73e9: // finalWord
		{
			sprintf( msg, "%s", GetWord() );
		}
		break;
	}
	
	return msg;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	CHorse::GetCurrentRestartIndex( void )
{
	

	uint32 num_restarts = get_num_horse_restarts();
	if ( m_currentRestartIndex >= num_restarts )
	{
		//Dbg_MsgAssert( 0, ( "Out of range horse restart index %d %d", m_currentRestartIndex, num_restarts ) );
	}

	return m_currentRestartIndex;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CHorse::SetNextRestartPoint( void )
{
	m_currentRestartIndex++;

	if ( m_currentRestartIndex >= get_num_horse_restarts() )
	{
		m_currentRestartIndex = 0;
	}

#ifdef __USER_GARY__
	printf( "\n*** Setting next restart point %d ***\n", m_currentRestartIndex );
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mdl




