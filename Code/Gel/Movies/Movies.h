/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			movies													**
**																			**
**	File name:		gel/movies/movies.h  									**
**																			**
**	Created: 		5/14/01	-	mjd											**
**																			**
*****************************************************************************/

#ifndef __GEL_MOVIES_H
#define __GEL_MOVIES_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/list.h>
#include <core/macros.h>
//#include <gel/scripting/script.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Flx
{



void PlayMovie( const char *pMovieName );

} // namespace Flx

#endif	// __GEL_MOVIES_H



