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
**	File name:		occlude.cpp												**
**																			**
**	Created:		04/02/02	-	dc										**
**																			**
**	Description:	Occlusion testing code									**
**																			**
*****************************************************************************/


//#define	OCCLUDER_USES_SCRATCHPAD
//#define	OCCLUDER_USES_VU0_MACROMODE
//#define	OCCLUDER_USES_VU0_MICROMODE

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/
		
#include <core/math.h>
#include <gfx/debuggfx.h>
#include "occlude.h"

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/


/*****************************************************************************
**								  Externals									**
*****************************************************************************/

//extern D3DXMATRIX *p_bbox_transform;

namespace NxNgc
{


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

const uint32 MAX_NEW_OCCLUSION_POLYS_TO_CHECK_PER_FRAME = 4;

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

// Structure used to store details of a single poly. A list of these will be built at geometry load time.
struct sOcclusionPoly
{
	bool		in_use;		// Whether the poly is currently being used for occlusion.
	bool		available;	// Whether the poly is available for selection for occlusion.
	uint32		checksum;	// Name checksum of the occlusion poly.
	Mth::Vector	verts[4];
	Mth::Vector	normal;
};
	
const uint32	MAX_OCCLUDERS				= 8;
const uint32	MAX_VIEWS_PER_OCCLUDER		= 2;

struct sOccluder
{
	static uint32		NumOccluders;
	static sOccluder	Occluders[MAX_OCCLUDERS];

	static void			add_to_stack( sOcclusionPoly *p_poly );
	static void			sort_stack( void );
	static void			tidy_stack( void );

	sOcclusionPoly	*p_poly;
	Mth::Vector		planes[5];
	int				score[MAX_VIEWS_PER_OCCLUDER];	// Current rating on quality of occlusion - based on number of meshes occluded last frame.
};

const uint32	MAX_OCCLUSION_POLYS			= 512;
uint32			NumOcclusionPolys			= 0;
uint32			NextOcclusionPolyToCheck	= 0;
int				CurrentView					= 0;
sOcclusionPoly	OcclusionPolys[MAX_OCCLUSION_POLYS];

uint32		sOccluder::NumOccluders				= 0;
sOccluder	sOccluder::Occluders[MAX_OCCLUDERS];

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
void sOccluder::add_to_stack( sOcclusionPoly *p_poly )
{
	if( NumOccluders < MAX_OCCLUDERS )
	{
		Dbg_Assert( p_poly->available );

		Occluders[NumOccluders].p_poly	= p_poly;
		p_poly->in_use					= true;

		// Reset scores for all views.
		memset( Occluders[NumOccluders].score, 0, sizeof( int ) * MAX_VIEWS_PER_OCCLUDER );

		++NumOccluders;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int cmp( const void *p1, const void *p2 )
{
	// Sort based on the sum total of scores in all views.
	int score1 = 0;
	int score2 = 0;
	for( uint v = 0; v < MAX_VIEWS_PER_OCCLUDER; ++v )
	{
		// Zero the score for any occlusion poly that is no longer available. This will force it out of the stack.
		if(((sOccluder*)p1)->p_poly->available == false )
			((sOccluder*)p1)->score[v] = 0;

		if(((sOccluder*)p2)->p_poly->available == false )
			((sOccluder*)p2)->score[v] = 0;

		score1 += ((sOccluder*)p1)->score[v];
		score2 += ((sOccluder*)p2)->score[v];
	}

	return score1 < score2 ? 1 : score1 > score2 ? -1 : 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sOccluder::sort_stack( void )
{
	qsort( Occluders, NumOccluders, sizeof( sOccluder ), cmp );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sOccluder::tidy_stack( void )
{
	if( NumOccluders > 0 )
	{
		// Sort in descending score order.
		sort_stack();

		// Count backwards so we know we get all the bad occluders.
		for( int i = NumOccluders - 1; i >= 0; --i )
		{
			// If we have hit an occluder with zero meshes culled, cut off the stack at this point.
			int total_score = 0;
			for( uint v = 0; v < MAX_VIEWS_PER_OCCLUDER; ++v )
			{
				total_score += Occluders[i].score[v];
			}
			
			if( total_score == 0 )
			{
				// No longer using this poly.
				Occluders[i].p_poly->in_use = false;

				// One less occluder to worry about.
				--NumOccluders;
			}
			else
			{
				// Reset the good occluders.
				Occluders[i].score[CurrentView] = 0;
			}
		}
	}
	
}



/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/


/******************************************************************/
/*                                                                */
/* Used to add an occlusion poly to the list. Likely to be called */
/* as geometry is spooled in.                                     */
/*                                                                */
/******************************************************************/
void AddOcclusionPoly( Mth::Vector &v0, Mth::Vector &v1, Mth::Vector &v2, Mth::Vector &v3, uint32 checksum )
{
	Dbg_Assert( NumOcclusionPolys < MAX_OCCLUSION_POLYS );	
	
	OcclusionPolys[NumOcclusionPolys].in_use	= false;
	OcclusionPolys[NumOcclusionPolys].available	= true;
	OcclusionPolys[NumOcclusionPolys].checksum	= checksum;
	OcclusionPolys[NumOcclusionPolys].verts[0]	= v0;
	OcclusionPolys[NumOcclusionPolys].verts[1]	= v1;
	OcclusionPolys[NumOcclusionPolys].verts[2]	= v2;
	OcclusionPolys[NumOcclusionPolys].verts[3]	= v3;
	OcclusionPolys[NumOcclusionPolys].normal	= Mth::CrossProduct( v1 - v0, v3 - v0 );
	OcclusionPolys[NumOcclusionPolys].normal.Normalize();
	
	++NumOcclusionPolys;
}



/******************************************************************/
/*                                                                */
/* Used to toggle whether an occlusion poly can be used or not	  */
/*                                                                */
/******************************************************************/
void EnableOcclusionPoly( uint32 checksum, bool available )
{
	for( uint32 i = 0; i < NumOcclusionPolys; ++i )
	{
		if( OcclusionPolys[i].checksum == checksum )
		{
			OcclusionPolys[i].available	= available;
		}
	}
}



/******************************************************************/
/*                                                                */
/* Used to clear all occlusion polys (when a level is unloaded)   */
/*                                                                */
/******************************************************************/
void RemoveAllOcclusionPolys( void )
{
	Dbg_Assert( NumOcclusionPolys < MAX_OCCLUSION_POLYS );	
	
 	sOccluder::NumOccluders		= 0;
	NumOcclusionPolys			= 0;
	NextOcclusionPolyToCheck	= 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CheckForOptimalOccluders( Mth::Vector &cam_pos, Mth::Vector &view_direction )
{
	if( NumOcclusionPolys > 0 )
	{
		uint32 added	= 0;
		uint32 checked	= 0;

		while( added < MAX_NEW_OCCLUSION_POLYS_TO_CHECK_PER_FRAME )
		{
			// Given the current position of the camera, check through the unused, available occlusion polys to see if one scores higher
			// than the lowest scoring occlusion poly in use.
			sOcclusionPoly *poly_to_check = &OcclusionPolys[NextOcclusionPolyToCheck++];
			if(( !poly_to_check->in_use ) && ( poly_to_check->available ))
			{
				sOccluder::add_to_stack( poly_to_check );
				++added;
			}
			++checked;

			// Ensure we are always checking within bounds.
			if( NextOcclusionPolyToCheck >= NumOcclusionPolys )
			{
				NextOcclusionPolyToCheck = 0;
			}

			// Quit out if we have less available occluders than spaces to fill.
			if( checked >= NumOcclusionPolys )
			{
				break;
			}
		}
	}
}


//char scratch[16384];


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void BuildOccluders( Mth::Vector *p_cam_pos, int view )
{
//	for( uint32 i = 0; i < NumOcclusionPolys; ++i )
//	{
//		Gfx::AddDebugLine( OcclusionPolys[i].verts[0], OcclusionPolys[i].verts[1], 0, 0, 1 );
//		Gfx::AddDebugLine( OcclusionPolys[i].verts[1], OcclusionPolys[i].verts[2], 0, 0, 1 );
//		Gfx::AddDebugLine( OcclusionPolys[i].verts[2], OcclusionPolys[i].verts[3], 0, 0, 1 );
//		Gfx::AddDebugLine( OcclusionPolys[i].verts[3], OcclusionPolys[i].verts[0], 0, 0, 1 );
//	}
	
	// Set the current view - this will remain active until another call to this function.
	Dbg_Assert( (uint32)view < MAX_VIEWS_PER_OCCLUDER );
	CurrentView	= view;
	
	// Tidy up from last frame.
	sOccluder::tidy_stack();
	
	// Cyclically add more occluders for checking.
	CheckForOptimalOccluders( *p_cam_pos, *p_cam_pos );
	
	// Build all 5 planes for each occluder.
	Mth::Vector u0, u1, p;

	// The order in which the verts are used to build tha planes depends upon where the camera is in relation to the occlusion
	// poly. We use the default order when the viewpoint is on the side of the poly on which the default poly normal faces.
	for( uint32 i = 0; i < sOccluder::NumOccluders; ++i )
	{
		sOcclusionPoly *p_poly = sOccluder::Occluders[i].p_poly;

//		Gfx::AddDebugLine( p_poly->verts[0], p_poly->verts[1], 0, 0, 1 );
//		Gfx::AddDebugLine( p_poly->verts[1], p_poly->verts[2], 0, 0, 1 );
//		Gfx::AddDebugLine( p_poly->verts[2], p_poly->verts[3], 0, 0, 1 );
//		Gfx::AddDebugLine( p_poly->verts[3], p_poly->verts[0], 0, 0, 1 );
		
		if( Mth::DotProduct( *p_cam_pos - p_poly->verts[0], p_poly->normal ) >= 0.0f )
		{
			// Start with the front. We want to reverse the order of the front plane to ensure that objects *behind* the plane
			// are considered occluded. (1->3->4)...
			u0						= p_poly->verts[2] - p_poly->verts[0];
			u1						= p_poly->verts[3] - p_poly->verts[0];
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= Mth::DotProduct( p, p_poly->verts[0] );
			sOccluder::Occluders[i].planes[0]	= p;

			// ...then left (0->1->2)...
			u0						= p_poly->verts[0] - *p_cam_pos;
			u1						= p_poly->verts[1] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[1]	= p;

			// ...then right (0->3->4)...
			u0						= p_poly->verts[2] - *p_cam_pos;
			u1						= p_poly->verts[3] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[2]	= p;

			// ...then top (0->2->3)...
			u0						= p_poly->verts[1] - *p_cam_pos;
			u1						= p_poly->verts[2] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[3]	= p;

			// ...then bottom (0->4->1)...
			u0						= p_poly->verts[3] - *p_cam_pos;
			u1						= p_poly->verts[0] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[4]	= p;
		}
		else
		{
			// Start with the front. We want to reverse the order of the front plane to ensure that objects *behind* the plane
			// are considered occluded. (1->4->3)...
			u0						= p_poly->verts[3] - p_poly->verts[0];
			u1						= p_poly->verts[2] - p_poly->verts[0];
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= Mth::DotProduct( p, p_poly->verts[0] );
			sOccluder::Occluders[i].planes[0]	= p;

			// ...then left (0->2->1)...
			u0						= p_poly->verts[1] - *p_cam_pos;
			u1						= p_poly->verts[0] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[1]	= p;

			// ...then right (0->4->3)...
			u0						= p_poly->verts[3] - *p_cam_pos;
			u1						= p_poly->verts[2] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[2]	= p;

			// ...then top (0->3->2)...
			u0						= p_poly->verts[2] - *p_cam_pos;
			u1						= p_poly->verts[1] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[3]	= p;

			// ...then bottom (0->1->4)...
			u0						= p_poly->verts[0] - *p_cam_pos;
			u1						= p_poly->verts[3] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[4]	= p;
		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool TestSphereAgainstOccluders( Mth::Vector *p_sphere, uint32 meshes )
{
	// Test against each occluder.
	for( uint32 o = 0; o < sOccluder::NumOccluders; ++o )
	{
		bool occluded = true;

		// Test against each plane in the occluder.
		for( uint32 p = 0; p < 5; ++p )
		{
			float result =	( sOccluder::Occluders[o].planes[p][X] * p_sphere->GetX() ) +
							( sOccluder::Occluders[o].planes[p][Y] * p_sphere->GetY() ) +
							( sOccluder::Occluders[o].planes[p][Z] * p_sphere->GetZ() ) -
							( sOccluder::Occluders[o].planes[p][W] );
			if( result >= -p_sphere->GetW() )
			{
				// Outside of this plane, therefore not occluded by this occluder.
				occluded = false;
				break;
			}
		}

		if( occluded )
		{
			// Inside all planes, therefore occluded. Increase score for this occluder.
			sOccluder::Occluders[o].score[CurrentView] += meshes;
			return true;
		}
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


} // namespace NxXbox

