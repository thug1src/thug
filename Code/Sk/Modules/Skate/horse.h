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
**	File name:		horse.h													**
**																			**
**	Created by:		7/17/01 - Gary											**
**																			**
**	Description:	<description>		 									**
**																			**
*****************************************************************************/

#ifndef __SK_MODULES_HORSE_H
#define __SK_MODULES_HORSE_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/string/cstring.h>
#include <sk/modules/skate/skate.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Script
{
    class CStruct;
};
                                
namespace Mdl
{

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

class  CHorse  : public Spt::Class
{
	

	enum
	{
		vMAX_RESTARTS = 32
	};
			
public:
	CHorse();

public:
	void						Init();
	void						SwitchPlayers();
	void						StartRun();
	void 						EndRun();
	bool						Ended();
	char*						GetWord();
	void						SetWord( Str::String word );
	Str::String					GetWordForPlayer( int skater_id );
	int							GetCurrentSkaterId();
	Str::String					GetString( uint32 checksum );
	uint32						GetCurrentRestartIndex();
	void						SetNextRestartPoint(void);
	bool						StatusEquals( Script::CStruct* pParams );

protected:
	int							get_next_valid_skater_id();
	int							get_score( int id );
	
protected:
	int							m_scoreToBeat;
	int							m_currentSkaterId;
	int							m_nextSkaterId;
	char						m_string[512];
	char						m_nextHorseMessage[512];
	uint32						m_numLetters[Skate::vMAX_SKATERS];
	uint32						m_numRestarts;
	uint32						m_currentRestartIndex;
	uint32						m_status;
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

} // namespace Mdl

#endif		// __SK_MODULES_HORSE_H



