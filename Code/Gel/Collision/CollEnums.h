/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2002 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Nx														**
**																			**
**	File name:		CollEnums.h												**
**																			**
**	Created: 		02/27/2002	-	grj										**
**																			**
*****************************************************************************/

#ifndef	__GEL_COLLENUMS_H
#define	__GEL_COLLENUMS_H

namespace Nx
{

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

////////////////////////////////////////////////////////////////
// Types of collision
enum CollType
{
	vCOLL_TYPE_NONE = 0,
	vCOLL_TYPE_BBOX,
	vCOLL_TYPE_SPHERE,
	vCOLL_TYPE_CYLINDRICAL,
	vCOLL_TYPE_GEOM,
};

// Typedefs
typedef uint16 FaceIndex;
typedef uint8  FaceByteIndex;

} // namespace Nx

#endif  //	__GEL_COLLENUMS_H
