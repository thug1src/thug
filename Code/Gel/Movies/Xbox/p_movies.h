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
**	Module:			ps2 movies												**
**																			**
**	File name:		gel/movies/ngps/p_movies.h 								**
**																			**
**	Created: 		6/27/01	-	dc											**
**																			**
*****************************************************************************/

#ifndef __P_MOVIES_H
#define __P_MOVIES_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/singleton.h>
#include <core/list.h>
#include <core/macros.h>

#include <bink.h>

namespace Flx
{

	void PMovies_PlayMovie( const char *pName );

} // namespace Flx

#endif	// __P_MOVIES_H



