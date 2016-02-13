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



/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/
		
#include <core/math.h>
#include <gfx/debuggfx.h>
//#include <libdma.h>
#include "occlude.h"
#include "resource.h"

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/


/*****************************************************************************
**								  Externals									**
*****************************************************************************/

//extern D3DXMATRIX *p_bbox_transform;

namespace NxPs2
{


/*****************************************************************************
**								   Defines									**
*****************************************************************************/


/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static bool sCopiedOccludersOffline = false;		// tells that occluders have been copied to a fast ram and the

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

uint32			NumOcclusionPolys			= 0;
uint32			NextOcclusionPolyToCheck	= 0;
sOcclusionPoly	OcclusionPolys[MAX_OCCLUSION_POLYS];

uint32		sOccluder::NumOccluders				= 0;
sOccluder	sOccluder::Occluders[MAX_OCCLUDERS];
bool		sOccluder::sUseVU0 = false;
bool		sOccluder::sUseScratchPad = false;


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
		Occluders[NumOccluders].p_poly	= p_poly;
		p_poly->in_use					= true;
		++NumOccluders;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int cmp( const void *p1, const void *p2 )
{
	// Zero the score for any occlusion poly that is no longer available. This will force it out of the stack.
	if(((sOccluder*)p1)->p_poly->available == false )
		((sOccluder*)p1)->score = 0;

	if(((sOccluder*)p2)->p_poly->available == false )
		((sOccluder*)p2)->score = 0;

	return((sOccluder*)p1)->score < ((sOccluder*)p2)->score ? 1 : ((sOccluder*)p1)->score > ((sOccluder*)p2)->score ? -1 : 0;
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
		

		int total = 0;
		int demoted = 0;
		// Count backwards so we know we get all the bad occluders.
		for( int i = NumOccluders - 1; i >= 0; --i )
		{
			// If we have hit an occluder with zero meshes culled, cut off the stack at this point.
			// also, if the stack is full, then demote the lowest scoring 4
			if( Occluders[i].score == 0  || i > (int)(MAX_OCCLUDERS-MIN_NEW_OCCLUSION_POLYS_TO_CHECK_PER_FRAME))
			{
				// No longer using this poly.
				Occluders[i].p_poly->in_use = false;

				// One less occluder to worry about.
				--NumOccluders;
				demoted++;
			}
			else
			{
				total += Occluders[i].score;
				// Reset the good occluders.
				Occluders[i].score = 0;
			}
		}
//		printf ("\nOccluded %d objects with %d Occluders (demoted %d)\n",total,NumOccluders,demoted);
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
//	printf (" Adding occlusion polygons, currently %d\n",NumOcclusionPolys);
	Dbg_MsgAssert( NumOcclusionPolys < MAX_OCCLUSION_POLYS,("Too many (%d) occlusion polygons",NumOcclusionPolys) );	
	
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

    sCopiedOccludersOffline = false;		// current scores now invalid
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

    sCopiedOccludersOffline = false;		// current scores now invalid
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
			// Given the current position of the camera, check through the unused occlusion polys to see if one scores higher
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
void BuildOccluders( Mth::Vector *p_cam_pos )
{
#ifdef	OCCLUDER_USES_VU0_MACROMODE
	if (sCopiedOccludersOffline)
	{
		sOccluder * p_occluder = &sOccluder::Occluders[0];
		uint128 * p_vu0_mem = (uint128*)0x11004060;     // Start of first score

		for( uint32 o = sOccluder::NumOccluders; o > 0 ; --o )
		{
			p_occluder->score = *((uint32*) (p_vu0_mem));
			p_occluder++;
			p_vu0_mem += 6;
		}

		sCopiedOccludersOffline = false;	// we are now up-to-date
	}
#endif
	for( uint32 i = 0; i < NumOcclusionPolys; ++i )
	{
//		Gfx::AddDebugLine( OcclusionPolys[i].verts[0], OcclusionPolys[i].verts[1], 0xFFFFFFFFUL, 0, 1 );
//		Gfx::AddDebugLine( OcclusionPolys[i].verts[1], OcclusionPolys[i].verts[2], 0xFFFFFFFFUL, 0, 1 );
//		Gfx::AddDebugLine( OcclusionPolys[i].verts[2], OcclusionPolys[i].verts[3], 0xFFFFFFFFUL, 0, 1 );
//		Gfx::AddDebugLine( OcclusionPolys[i].verts[3], OcclusionPolys[i].verts[0], 0xFFFFFFFFUL, 0, 1 );
	}
	
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
		if( Mth::DotProduct( *p_cam_pos - p_poly->verts[0], p_poly->normal ) >= 0.0f )
		{
			// Start with left (0->1->2)...
			u0						= p_poly->verts[0] - *p_cam_pos;
			u1						= p_poly->verts[1] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= -Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[0]	= p;

			// ...then right (0->3->4)...
			u0						= p_poly->verts[2] - *p_cam_pos;
			u1						= p_poly->verts[3] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= -Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[1]	= p;

			// ...then top (0->2->3)...
			u0						= p_poly->verts[1] - *p_cam_pos;
			u1						= p_poly->verts[2] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= -Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[2]	= p;

			// ...then bottom (0->4->1)...
			u0						= p_poly->verts[3] - *p_cam_pos;
			u1						= p_poly->verts[0] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= -Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[3]	= p;

			// ...then front. We want to reverse the order of the front plane to ensure that objects *behind* the plane
			// are considered occluded. (1->3->4)...
			u0						= p_poly->verts[2] - p_poly->verts[0];
			u1						= p_poly->verts[3] - p_poly->verts[0];
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= -Mth::DotProduct( p, p_poly->verts[0] );
			sOccluder::Occluders[i].planes[4]	= p;
		}
		else
		{
			// Start with left (0->2->1)...
			u0						= p_poly->verts[1] - *p_cam_pos;
			u1						= p_poly->verts[0] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= -Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[0]	= p;

			// ...then right (0->4->3)...
			u0						= p_poly->verts[3] - *p_cam_pos;
			u1						= p_poly->verts[2] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= -Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[1]	= p;

			// ...then top (0->3->2)...
			u0						= p_poly->verts[2] - *p_cam_pos;
			u1						= p_poly->verts[1] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= -Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[2]	= p;

			// ...then bottom (0->1->4)...
			u0						= p_poly->verts[0] - *p_cam_pos;
			u1						= p_poly->verts[3] - *p_cam_pos;
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= -Mth::DotProduct( p, *p_cam_pos );
			sOccluder::Occluders[i].planes[3]	= p;

			// ...then front. We want to reverse the order of the front plane to ensure that objects *behind* the plane
			// are considered occluded. (1->4->3)...
			u0						= p_poly->verts[3] - p_poly->verts[0];
			u1						= p_poly->verts[2] - p_poly->verts[0];
			p						= Mth::CrossProduct( u0, u1 );
			p.Normalize();
			p[W]					= -Mth::DotProduct( p, p_poly->verts[0] );
			sOccluder::Occluders[i].planes[4]	= p;
		}
	}


	if (sOccluder::sUseScratchPad)
	{
		// Copy the occlusion polygons to the scratchpad
		// as we will read them a zillion times during rendering
		sOccluder * p_occluder = &sOccluder::Occluders[0];
		sOccluder * p_scratch = (sOccluder*)0x70000000;
	//	sOccluder * p_scratch = (sOccluder*)scratch;
		for( uint32 o = sOccluder::NumOccluders; o > 0 ; --o )
		{
			*p_scratch = *p_occluder;
			p_occluder++;
			p_scratch++;
		}
		Dbg_MsgAssert((void*)p_scratch < (void*)0x70004000,("Scratchpad overflow"));  // 16k scratchpad.  Nice.
	}

#ifdef	OCCLUDER_USES_VU0_MACROMODE
	sOccluder * p_occluder = &sOccluder::Occluders[0];
	uint128 * p_vu0_mem = (uint128*)0x11004000;		// VU0 data memory

	// Copies the number of occulders into X
	*((uint32*) (p_vu0_mem++)) = sOccluder::NumOccluders;

	for( uint32 o = sOccluder::NumOccluders; o > 0 ; --o )
	{
		*(p_vu0_mem++) = *((uint128*) &(p_occluder->planes[0]));
		*(p_vu0_mem++) = *((uint128*) &(p_occluder->planes[1]));
		*(p_vu0_mem++) = *((uint128*) &(p_occluder->planes[2]));
		*(p_vu0_mem++) = *((uint128*) &(p_occluder->planes[3]));
		*(p_vu0_mem++) = *((uint128*) &(p_occluder->planes[4]));
		*((uint32*) (p_vu0_mem++)) = p_occluder->score;
		p_occluder++;
	}

	FlushCache(WRITEBACK_DCACHE);

	Dbg_MsgAssert((void*)p_vu0_mem < (void*)0x11005000,("VU0 memory overflow"));  // 4k vu0 mem.  Nice.

    sCopiedOccludersOffline = true;			// so we know to copy the scores back (technically, this should be in
											// TestSphereAgainstOccluders(), but we only want to write this var once
#endif




#ifdef	OCCLUDER_USES_VU0_MICROMODE

	// copy occlusion planes to VUMem0
	sOccluder * p_occluder = &sOccluder::Occluders[0];
// Mick: removed this to fix a warning in final build.
//	float * p_vumem0 = (float *)0x11004000;		// VUMem0
	Mth::Vector * p_plane;
	asm ("viaddi vi01,vi00,0": : : "$8");
	for( uint32 o = sOccluder::NumOccluders; o > 0 ; --o )
	{
		p_plane = &(p_occluder->planes[0]);
		for( uint32 p = 5; p > 0; --p )
		{
			#if 0
			*p_vumem0++ = p_plane->GetX();
			*p_vumem0++ = p_plane->GetY();
			*p_vumem0++ = p_plane->GetZ();
			*p_vumem0++ = p_plane->GetW();
			#else
			// this way we don't need the FlushCache()
			asm __volatile__("

				lq		$8,0(%0)
				qmtc2	$8,vf01
				vsqi	vf01,(vi01++)

			": : "r" (p_plane) : "$8");
			#endif
			p_plane++;
		}
		p_occluder++;
	}
// Mick - removed this assertions as it is no longer valid
//	Dbg_MsgAssert((void*)p_vumem0 < (void*)0x11004FA0,("VUMem0 overflow"));		// 50 occluders max

	// flush the cache so VU0 can work on the data now.... not needed anymore
	//FlushCache(WRITEBACK_DCACHE);

	// call vu0 microsubroutine to rearrange data into a special format
	asm __volatile__("

		#sync									# wait for write-back of the mem we just wrote to
		lw			$8,(%0)						# t0 = num occluders
		ctc2		$8,$vi01					# move it to vi01 in vu0
		vcallms		InitialiseOccluders			# call microsubroutine to rearrange vu0 memory
		vnop									# interlocking instruction, waits for vu0 completion
	
	": : "r" (&sOccluder::NumOccluders) : "$8");


#endif


}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool OccludeUseVU0()
{
	return sOccluder::sUseVU0 = CSystemResources::sRequestResource(CSystemResources::vVU0_MEMORY);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void OccludeDisableVU0()
{
	if (sOccluder::sUseVU0)
	{
		CSystemResources::sFreeResource(CSystemResources::vVU0_MEMORY);
		sOccluder::sUseVU0 = false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool OccludeUseScratchPad()
{
	return sOccluder::sUseScratchPad = CSystemResources::sRequestResource(CSystemResources::vSCRATCHPAD_MEMORY);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void OccludeDisableScratchPad()
{
	if (sOccluder::sUseScratchPad)
	{
		CSystemResources::sFreeResource(CSystemResources::vSCRATCHPAD_MEMORY);
		sOccluder::sUseScratchPad = false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


} // namespace NxXbox
