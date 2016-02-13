/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	Module:			Graphics (GFX)		 									**
**																			**
**	File name:		bbox.cpp												**
**																			**
**	Created:		02/01/00	-	mjb	(actualy Matt's now)				**
**																			**
**	Description:	some functions for line/box intersection
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/math.h>
#include <gfx/bbox.h>
#include <core/math/vector.h>
#include <core/math/vector.inl>


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/


namespace Gfx
{


/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

bool PointInsideBox( const Mth::Vector &point, const Mth::Vector &boxMax, const Mth::Vector &boxMin )
{
	
	if ( point[ X ] > boxMax[ X ] )
		return ( false );
	if ( point[ Y ] > boxMax[ Y ] )
		return ( false );
	if ( point[ Z ] > boxMax[ Z ] )
		return ( false );
	if ( point[ X ] < boxMin[ X ] )
		return ( false );
	if ( point[ Y ] < boxMin[ Y ] )
		return ( false );
	if ( point[ Z ] < boxMin[ Z ] )
		return ( false );
	
	return ( true );
}// end of PointInsideBox( )


// Checks whether pStart-pEnd collides with the axis-aligned bounding box defined by
// pMax and pMin.
bool LineCollidesWithBox( const Mth::Vector &pStart, const Mth::Vector &pEnd, const Mth::Vector &pMax, const Mth::Vector &pMin )
{
	

	// if either point is in the box... that counts!
	if ( PointInsideBox( pStart, pMax, pMin ) ||
		PointInsideBox( pEnd, pMax, pMin ) )
	{
		return ( true );
	}

	// Trivial rejection.
	if((pStart[ Y ]>pMax[ Y ])&&( pEnd[ Y ]>pMax[ Y ])) return false;
	if((pStart[ Y ]<pMin[ Y ])&&( pEnd[ Y ]<pMin[ Y ])) return false;
	if((pStart[ X ]>pMax[ X ])&&( pEnd[ X ]>pMax[ X ])) return false;
	if((pStart[ X ]<pMin[ X ])&&( pEnd[ X ]<pMin[ X ])) return false;
	if((pStart[ Z ]>pMax[ Z ])&&( pEnd[ Z ]>pMax[ Z ])) return false;
	if((pStart[ Z ]<pMin[ Z ])&&( pEnd[ Z ]<pMin[ Z ])) return false;

	float dx=pEnd[ X ]-pStart[ X ];
	float dy=pEnd[ Y ]-pStart[ Y ];
	float dz=pEnd[ Z ]-pStart[ Z ];
	
	// avoid divide by zeros.
	if ( !dx )
	{
		dx = ( 0.000001 );
	}
	if ( !dy )
	{
		dy = ( 0.000001 );
	}
	if ( !dz )
	{
		dz = ( 0.000001 );
	}

	// Check the max-x face.
	if (pStart[ X ]>pMax[ X ] && pEnd[ X ]<pMax[ X ])
	{
		// It crosses the plane of the face, so calculate the y & z coords
		// of the intersection and see if they are in the face,
		float d=pMax[ X ]-pStart[ X ];
		float y=d*dy/dx+pStart[ Y ];
		float z=d*dz/dx+pStart[ Z ];
		if (y<pMax[ Y ] && y>pMin[ Y ] && z<pMax[ Z ] && z>pMin[ Z ])
		{
			// It does collide!
			return true;
		}
	}

	// Check the min-x face.
	if (pStart[ X ]<pMin[ X ] && pEnd[ X ]>pMin[ X ])
	{
		// It crosses the plane of the face, so calculate the y & z coords
		// of the intersection and see if they are in the face,
		float d=pMin[ X ]-pStart[ X ];
		float y=d*dy/dx+pStart[ Y ];
		float z=d*dz/dx+pStart[ Z ];
		if (y<pMax[ Y ] && y>pMin[ Y ] && z<pMax[ Z ] && z>pMin[ Z ])
		{
			// It does collide!
			return true;
		}
	}

	// Check the max-y face.
	if (pStart[ Y ]>pMax[ Y ] && pEnd[ Y ]<pMax[ Y ])
	{
		// It crosses the plane of the face, so calculate the x & z coords
		// of the intersection and see if they are in the face,
		float d=pMax[ Y ]-pStart[ Y ];
		float x=d*dx/dy+pStart[ X ];
		float z=d*dz/dy+pStart[ Z ];
		if (x<pMax[ X ] && x>pMin[ X ] && z<pMax[ Z ] && z>pMin[ Z ])
		{
			// It does collide!
			return true;
		}
	}

	// Check the min-y face.
	if (pStart[ Y ]<pMin[ Y ] && pEnd[ Y ]>pMin[ Y ])
	{
		// It crosses the plane of the face, so calculate the x & z coords
		// of the intersection and see if they are in the face,
		float d=pMin[ Y ]-pStart[ Y ];
		float x=d*dx/dy+pStart[ X ];
		float z=d*dz/dy+pStart[ Z ];
		if (x<pMax[ X ] && x>pMin[ X ] && z<pMax[ Z ] && z>pMin[ Z ])
		{
			// It does collide!
			return true;
		}
	}

	// Check the max-z face.
	if (pStart[ Z ]>pMax[ Z ] && pEnd[ Z ]<pMax[ Z ])
	{
		// It crosses the plane of the face, so calculate the x & y coords
		// of the intersection and see if they are in the face,
		float d=pMax[ Z ]-pStart[ Z ];
		float x=d*dx/dz+pStart[ X ];
		float y=d*dy/dz+pStart[ Y ];
		if (x<pMax[ X ] && x>pMin[ X ] && y<pMax[ Y ] && y>pMin[ Y ])
		{
			// It does collide!
			return true;
		}
	}


	// Check the min-z face.
	if (pStart[ Z ]<pMin[ Z ] && pEnd[ Z ]>pMin[ Z ])
	{
		// It crosses the plane of the face, so calculate the x & y coords
		// of the intersection and see if they are in the face,
		float d=pMin[ Z ]-pStart[ Z ];
		float x=d*dx/dz+pStart[ X ];
		float y=d*dy/dz+pStart[ Y ];
		if (x<pMax[ X ] && x>pMin[ X ] && y<pMax[ Y ] && y>pMin[ Y ])
		{
			// It does collide!
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
} // namespace Gfx
