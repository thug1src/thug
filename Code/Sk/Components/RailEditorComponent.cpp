//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       RailEditorComponent.cpp
//* OWNER:          Kendall Harrison
//* CREATION DATE:  3/21/2003
//****************************************************************************

#include <sk/components/RailEditorComponent.h>
#include <sk/components/EditorCameraComponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/string.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/utils.h>
#include <gfx/nx.h>
#include <sk/parkeditor2/parked.h>
#include <sk/engine/feeler.h>

DefinePoolableClass(Obj::CEditedRailPoint);
DefinePoolableClass(Obj::CEditedRail);

// TODO: The m_mode member could in theory be removed, just use the tags to store it instead.
// Hence could remove the SetEditingMode and GetEditingMode functions.
// (Nice to have as much logic moved to script as possible)

namespace Obj
{
Mth::Vector ZeroVector;

#define RAIL_SECTOR_HEIGHT 2.625f

static uint32 s_rail_unique_id=1;
static int s_highlight_flash_counter=0;

// This will rotate pos-rotateCentre by degrees about Y, add the result to newCentre, and return the result.
// Needed by the clipboard when displaying the copied rails as they rotate with the cursor.
static Mth::Vector s_rotate_and_translate(Mth::Vector& pos, Mth::Vector& rotateCentre, Mth::Vector& newCentre, float degrees)
{
	Mth::Vector d=pos-rotateCentre;
	
	float rad=degrees*3.141592654f/180.0f;
	float s=sinf(rad);
	float c=cosf(rad);

	Mth::Vector rotated;
	rotated[X]=c*d[X]+s*d[Z];
	rotated[Y]=d[Y];
	rotated[Z]=c*d[Z]-s*d[X];
	
	return newCentre+rotated;
}

// SPEEDOPT: Pass false to updateSuperSectors when creating a batch of sectors, and call
// UpdateSuperSectors afterwards.
static Nx::CSector *s_clone_sector(uint32 sector_name)
{
	Nx::CScene *p_source_scene=Nx::CEngine::sGetScene("sk5ed");
	if (!p_source_scene)
	{
		return NULL;
	}
		
	Nx::CScene *p_cloned_scene=Ed::CParkManager::sInstance()->GetGenerator()->GetClonedScene();
	if (!p_cloned_scene)
	{
		return NULL;
	}
		
	uint32 new_sector_checksum = p_source_scene->CloneSector(sector_name, p_cloned_scene, false, true);
	
	// SPEEDOPT: If a batch of sectors are cloned, only do this once at the end.
	p_cloned_scene->UpdateSuperSectors();
	
	Nx::CSector *p_new_sector = Nx::CEngine::sGetSector(new_sector_checksum);

	return p_new_sector;
}


static void s_generate_end_vert_positions(Mth::Vector &pos, Mth::Vector &dir, 
										  Mth::Vector *p_geom_verts,
										  int *p_vert_indices,
										  int numIndices)
{
	// Need to copy the origin into a local var cos the source is about to be modified.
	Mth::Vector origin=p_geom_verts[p_vert_indices[0]];
	
	Mth::Vector up(0.0f,1.0f,0.0f);
	Mth::Vector u=dir;
	u.Normalize();
	
	Mth::Vector w;
	if (fabs(u[Y]) > 0.99999f)
	{
		w.Set(0.0f,0.0f,1.0f);
	}
	else
	{
		w=Mth::CrossProduct(up,u);
		w.Normalize();
	}	
	
	Mth::Vector v=Mth::CrossProduct(u,w);
	v.Normalize();
	Mth::Matrix rot;
	rot[Mth::RIGHT]=u;
	rot[Mth::UP]=v;
	rot[Mth::AT]=w;
	rot[3][3]=1.0f;
	
	for (int i=0; i<numIndices; ++i)
	{
		Mth::Vector *p_geom_vert=&p_geom_verts[p_vert_indices[i]];
		Mth::Vector off=*p_geom_vert-origin;
		off=off*rot;
		*p_geom_vert=off+pos;
	}
}

// Used for the rails, and the dotted line going from the cursor to the last rail point placed.

// Given a cloned rail sector, this will write in the vertex coords such that the rail goes from
// startPos to endPos.
// This assumes a certain hard-wired vertex ordering, so currently sourceSectorChecksum can only be
// Sk5Ed_RA_Dynamic or Sk5Ed_RADot_Dynamic
// If p_lastRailSector is not NULL then it will make the vertices of the start of the rail match up
// with those at the end of p_lastRailSector.

// TODO: Fix bug where end of rail shrinks when rail is very steep.
static void s_calculate_rail_sector_vertex_coords(Mth::Vector &lastPos, Mth::Vector &startPos, Mth::Vector &endPos, 
												  Nx::CSector *p_clonedSector, uint32 sourceSectorChecksum, 
												  Nx::CSector *p_lastRailSector, bool update_collision)
{
	// These arrays are figured out manually by printf'ing the vertex coords.
	// Each end of the rail has 5 vertices, a top one, 2 middle ones and 2 bottom ones, forming a house shape.
	// The top one has the highest y coord. All the verts for one end have the same x coord.
	// Some of the vertices are duplicated however, which is why there is more than one top coord, & more than 2 middle, etc.
	// For each vertex in p_end_verts_a, the corresponding vertex in p_end_verts_b must have the same y and z coords.
	// This is so that the start of one rail piece can join up with the end of the last by tieing the vertices together.
	
	// TODO: These indices will need to be different on Xbox and GameCube
#	ifdef __PLAT_NGC__
	int p_end_verts_a[]=
	{
		2, 13,				// Top 
		0, 4, 11, 15,		// Middle 
		1, 3, 17, 19,       // Bottom 
	};
	int p_end_verts_b[]=
	{
		7, 12,				// Top     
		5, 9, 10, 14,       // Middle  
		6, 8, 16, 18,       // Bottom  
	};
#else
#	ifdef __PLAT_XBOX__
	int p_end_verts_a[]=
	{
		1,9,             	// Top
		3,10,14,19,			// Middle
		0,13,2,16,			// Bottom
	};
	int p_end_verts_b[]=
	{
		6,11,				// Top
		7,8,15,18,			// Middle
		4,12,5,17,			// Bottom
	};
#	else
	int p_end_verts_a[]=
	{
		2,19,             	// Top
		0,4,11,17,21,		// Middle
		1,3,13,15,			// Bottom
	};
	int p_end_verts_b[]=
	{
		7,18,				// Top
		9,5,10,16,20,		// Middle
		8,6,12,14,			// Bottom
	};
#	endif
#	endif

	Dbg_MsgAssert(sizeof(p_end_verts_a)==sizeof(p_end_verts_b),("End vert array size mismatch!"));
	int num_indices=sizeof(p_end_verts_a)/sizeof(int);

	// Get the default coords of the sectors vertices. These will be relative to the sectors origin.
	Nx::CSector *p_source_sector=Nx::CEngine::sGetSector(sourceSectorChecksum);
	Nx::CGeom *p_source_geom=p_source_sector->GetGeom();
	Dbg_MsgAssert(p_source_geom,("NULL p_source_geom ?"));
	
	int num_render_verts=p_source_geom->GetNumRenderVerts();
	Dbg_MsgAssert(num_render_verts==num_indices*2,("Unexpected extra vertices in rail sector, expected %d, got %d",2*num_indices,num_render_verts));
	
	// SPEEDOPT: If necessary, could use a static buffer for the verts
	Mth::Vector *p_modified_render_verts=(Mth::Vector*)Mem::Malloc(num_render_verts * sizeof(Mth::Vector));
	p_source_geom->GetRenderVerts(p_modified_render_verts);

	#ifdef __NOPT_ASSERT__
	for (int i=0; i<num_indices; ++i)
	{
		Dbg_MsgAssert(p_modified_render_verts[p_end_verts_a[i]].GetY()==p_modified_render_verts[p_end_verts_b[i]].GetY(),("Rail sector vertex Y mismatch between vertices %d and %d",p_end_verts_a[i],p_end_verts_b[i]));
		Dbg_MsgAssert(p_modified_render_verts[p_end_verts_a[i]].GetZ()==p_modified_render_verts[p_end_verts_b[i]].GetZ(),("Rail sector vertex Z mismatch between vertices %d and %d",p_end_verts_a[i],p_end_verts_b[i]));
	}	
	
	float rail_height=p_modified_render_verts[p_end_verts_a[0]].GetY()-p_modified_render_verts[p_end_verts_a[num_indices-1]].GetY();
	Dbg_MsgAssert(rail_height==RAIL_SECTOR_HEIGHT,("Need to update RAIL_SECTOR_HEIGHT to be %f",rail_height));
	#endif
	
	Mth::Vector dir;
	if (p_lastRailSector)
	{
		dir=((startPos-lastPos)+(endPos-startPos)) / 2.0f;
		
		s_generate_end_vert_positions(startPos, dir, p_modified_render_verts, p_end_verts_a, num_indices);
		
		// Set the last sector's end verts equal to the just calculated set of coords too.
		Nx::CGeom *p_last_geom=p_lastRailSector->GetGeom();
		Dbg_MsgAssert(p_last_geom,("NULL p_last_geom ?"));
		
		int last_num_render_verts=p_last_geom->GetNumRenderVerts();
		// SPEEDOPT: If necessary, could use a static buffer for the verts
		Mth::Vector *p_last_verts=(Mth::Vector*)Mem::Malloc(last_num_render_verts * sizeof(Mth::Vector));
		p_last_geom->GetRenderVerts(p_last_verts);

		// Tie the end verts to the new start verts.
		for (int i=0; i<num_indices; ++i)
		{
			Dbg_MsgAssert(p_end_verts_b[i] < last_num_render_verts,("Bad index into p_last_verts"));
			p_last_verts[p_end_verts_b[i]]=p_modified_render_verts[p_end_verts_a[i]];
		}	

		p_last_geom->SetRenderVerts(p_last_verts);

		Mem::Free(p_last_verts);
	}
	else
	{
		dir=endPos-startPos;
		s_generate_end_vert_positions(startPos, dir, p_modified_render_verts, p_end_verts_a, num_indices);
	}	
	
	dir=endPos-startPos;
	s_generate_end_vert_positions(endPos, dir, p_modified_render_verts, p_end_verts_b, num_indices);
	

	// Write the vertex coords just calculated into the cloned sector.
	Dbg_MsgAssert(p_clonedSector,("NULL p_clonedSector"));
	Nx::CGeom *p_geom=p_clonedSector->GetGeom();
	Dbg_MsgAssert(p_geom,("NULL p_geom ?"));
	Dbg_MsgAssert(p_geom->GetNumRenderVerts()==p_source_geom->GetNumRenderVerts(),("Source geom num verts mismatch"));
	p_geom->SetRenderVerts(p_modified_render_verts);

	/////////////////////////////////////////////////////////////////
	// Now update the collision verts
	/////////////////////////////////////////////////////////////////
	
	if (update_collision) // Uncomment this once the rail pieces have been reexported with correct collision
	{
		// Get the source render verts again so that we can match up the source collision verts with them, and hence get
		// the index of the modified render vert to use for each collision vert.
		Mth::Vector *p_source_render_verts=(Mth::Vector*)Mem::Malloc(num_render_verts * sizeof(Mth::Vector));
		p_source_geom->GetRenderVerts(p_source_render_verts);
		// The source render verts are in relative coords.
	
		// Get the source collision verts, which will also be in relative coords.
		Nx::CCollObjTriData *p_source_col_data=p_source_geom->GetCollTriData();
		int num_source_col_verts=p_source_col_data->GetNumVerts();

		Mth::Vector *p_collision_verts=(Mth::Vector*)Mem::Malloc(num_source_col_verts * sizeof(Mth::Vector));
		p_source_col_data->GetRawVertices(p_collision_verts);
	
		// For each of the collision verts, look for a matching coord in the source render verts,
		// and if found, write in the world coords calculated for it earlier.
		for (int i=0; i<num_source_col_verts; ++i)
		{
			Mth::Vector pos=p_collision_verts[i];

			// All but two of the collision verts should be found by this loop.
			bool found_exact_match=false;		
			for (int j=0; j<num_render_verts; ++j)
			{
				if (p_source_render_verts[j][X] == pos[X] && 
					p_source_render_verts[j][Y] == pos[Y] &&
					p_source_render_verts[j][Z] == pos[Z])
				{
					p_collision_verts[i]=p_modified_render_verts[j];
					found_exact_match=true;
					break;
				}
			}		
		
			if (!found_exact_match)
			{
				bool found_match=false;
			
				// The two verts that have no corresponding render vert are the two bottom vertices
				// of the collision poly that hangs below the rail.
				// For these, find the render verts that match the x & z, and use their world position,
				// but just drop the y down the same distance as it was below it in the original relative coords.
				for (int j=0; j<num_render_verts; ++j)
				{
					if (p_source_render_verts[j][X] == pos[X] && 
						p_source_render_verts[j][Z] == pos[Z])
					{
						// Get the y difference in the original relative coords ...
						float y_off=pos[Y]-p_source_render_verts[j][Y];
						// Set the collision vertex equal to the previously calculated world coords of the vertex ...
						p_collision_verts[i]=p_modified_render_verts[j];
						// Then move it down by the y difference again.
						p_collision_verts[i][Y]+=y_off;
					
						found_match=true;
						break;
					}
				}		
			
				if (!found_match)
				{
					// If no match was found, then maybe the sector got re-exported in a strange manner.
					pos.PrintContents();
					Dbg_MsgAssert(0,("Could not find a render-vert match for collision vert %d",i));
				}	
			}
		}	

		// Write the new collision verts into the cloned sector.
		#ifdef	__NOPT_ASSERT__
		Nx::CCollObjTriData *p_dest_col_data=p_geom->GetCollTriData();
		Dbg_MsgAssert(p_dest_col_data->GetNumVerts()==num_source_col_verts,("Bad p_dest_col_data->GetNumVerts() ?"));
		#endif

		p_clonedSector->SetRawCollVertices(p_collision_verts);
		Mem::Free(p_collision_verts);
		Mem::Free(p_source_render_verts);
	}
	
	Mem::Free(p_modified_render_verts);	
} 

// Returns true if the skater will be able to grind from a to b to c without being forced off
// due to the angle being too big.
static bool s_angle_is_ok_to_grind(Mth::Vector &a, Mth::Vector &b, Mth::Vector &c)
{
	Mth::Vector ab=b-a;
	ab[Y]=0.0f;
	ab.Normalize();
	
	Mth::Vector bc=c-b;
	bc[Y]=0.0f;
	bc.Normalize();
	
	float cosine=Mth::DotProduct(ab,bc);
	if (cosine < cosf(Mth::DegToRad(Script::GetFloat(CRCD(0x76c1da15,"Rail_Corner_Leave_Angle")))))
	{
		return false;
	}
	
	return true;	
}

static bool s_distance_is_too_long(Mth::Vector &a, Mth::Vector &b)
{
	Mth::Vector d=a-b;
	// The distance at which the dotted line will flash red to indicate the distance is too long
	return d.Length() > 4000.0f;
}

static bool s_distance_is_way_too_long(Mth::Vector &a, Mth::Vector &b)
{
	Mth::Vector d=a-b;
	// The distance at which the dotted line will disappear completely to avoid rendering problems.
	return d.Length() > 4500.0f;
}

static bool s_positions_OK(Mth::Vector &a, Mth::Vector &b)
{
	if (s_distance_is_too_long(a, b))
	{
		return false;
	}
	
	Mth::Vector diff=a-b;
	
	// Don't allow consecutive points to be placed too close, otherwise the user may accidentally
	// place points on top of each other, & it could cause weirdness in the calculations due
	// to zero vectors trying to get normalized etc.
	if (diff.Length() < Script::GetFloat(CRCD(0x8bea1c03,"RailEditorMinimumPointSeparation")))
	{
		return false;
	}
	
	// Also don't allow the rail segment to get too steep.
	diff.Normalize();
	if (fabs(diff[Y]) > sinf(Mth::DegToRad(Script::GetFloat(CRCD(0x9fd761af,"RailEditorMaxSlope")))))
	{
		return false;
	}
	
	return true;
}
										  
CEditedRailPoint::CEditedRailPoint()
{
	mPos.Set();
	mHasPost=false;
	mHighlighted=false;
	mHeightAboveGround=0.0f;
	mpNext=NULL;
	mpPrevious=NULL;
	mpClonedRailSector=NULL;
	mpPostSector=NULL;
}

CEditedRailPoint::~CEditedRailPoint()
{
	DestroyRailGeometry();
	DestroyPostGeometry();
}

// Creates the rail sector if it does not exist already, then writes the world
// positions into the vertices.
// rotateCentre, newCentre, and degrees allow the rail to be displayed rotated and translated from its original
// position. This is used when displaying a rail section on the clipboard cursor in the park editor.
void CEditedRailPoint::UpdateRailGeometry(Mth::Vector& rotateCentre, Mth::Vector& newCentre, float degrees)
{
	// Note: The last point that was placed is actually the next in the list, since they get added to the front.
	if (!mpNext)
	{
		// There is no last point, so a rail can't be created because there is nothing to join to.
		
		// TODO: Could maybe still make a small section of rail, so that the first post of a rail has
		// a bit of rail overhanging it. Would look better than having the rail stop right above the post.
		
		// Make sure any existing rail is destroyed.
		DestroyRailGeometry();
		return;
	}

	// Clone the sector if necessary
	if (!mpClonedRailSector)
	{
		mpClonedRailSector=s_clone_sector(CRCD(0x66fe99bf,"Sk5Ed_RA_Dynamic"));
	}
	
	Mth::Vector a, b, c;
	
	// Don't tie the end vertices of the two rail sections together if they meet at too great
	// an angle, cos otherwise one of the sections gets too thin.
	bool join_to_last_sector=true;
	if (mpNext->mpNext)
	{
		a=mpNext->mPos-mpNext->mpNext->mPos;
		b=mPos-mpNext->mPos;
		a.Normalize();
		b.Normalize();
		if (Mth::DotProduct(a,b) < cosf(Mth::DegToRad(Script::GetFloat(CRCD(0xb1459a78,"RailEditorMaxJoinAngle")))))
		{
			join_to_last_sector=false;
		}	

		a=s_rotate_and_translate(mpNext->mpNext->mPos, rotateCentre, newCentre, degrees);
		b=s_rotate_and_translate(mpNext->mPos, rotateCentre, newCentre, degrees);
		c=s_rotate_and_translate(mPos, rotateCentre, newCentre, degrees);
		
		s_calculate_rail_sector_vertex_coords(a, b, c,
											  mpClonedRailSector, CRCD(0x66fe99bf,"Sk5Ed_RA_Dynamic"),
											  join_to_last_sector ? mpNext->mpClonedRailSector:NULL, true);
	}
	else
	{
		Dbg_MsgAssert(mpNext->mpClonedRailSector==NULL,("Expected mpNext->mpClonedRailSector to be NULL ?"));
		
		Mth::Vector dummy;
		a=s_rotate_and_translate(mpNext->mPos, rotateCentre, newCentre, degrees);
		b=s_rotate_and_translate(mPos, rotateCentre, newCentre, degrees);
		s_calculate_rail_sector_vertex_coords(dummy, a, b, 
											  mpClonedRailSector, CRCD(0x66fe99bf,"Sk5Ed_RA_Dynamic"),
											  NULL, true);
	}											  
}

// Creates the post sector if it does not exist already, then writes the correct world
// positions into the vertices.
// This is called when the post is first created, and also gets called for all posts whenever a cell in
// the park is raised or lowered, so that the posts maintain contact with the ground.

// rotateCentre, newCentre, and degrees allow the post to be displayed rotated and translated from its original
// position. This is used when displaying a rail section on the clipboard cursor in the park editor.
void CEditedRailPoint::UpdatePostGeometry(Mth::Vector& rotateCentre, Mth::Vector& newCentre, float degrees)
{
	if (!mHasPost)
	{
		DestroyPostGeometry();
		return;
	}
	
	// Don't place a post if the rail is so close to the ground that there is not enough room for a post.
	// This often happens when snapping a rail point to the edge of some level geometry for example.	
	if (mHeightAboveGround < RAIL_SECTOR_HEIGHT)
	{
		DestroyPostGeometry();
		return;
	}
	
	// Get the coords of the source sectors vertices. These will be relative to the sectors origin,
	// which is in the middle of the post.
	Nx::CSector *p_source_sector=Nx::CEngine::sGetSector(CRCD(0x2756c52d,"Sk5Ed_RAp_Dynamic"));
	Nx::CGeom *p_source_geom=p_source_sector->GetGeom();
	Dbg_MsgAssert(p_source_geom,("NULL p_source_geom ?"));
	
	int num_verts=p_source_geom->GetNumRenderVerts();
	// SPEEDOPT: If necessary, could use a static buffer for the verts
	Mth::Vector *p_verts=(Mth::Vector*)Mem::Malloc(num_verts * sizeof(Mth::Vector));
	p_source_geom->GetRenderVerts(p_verts);

	// Find the y coords of the top and bottom
	float min_y=1000000.0f;
	float max_y=-1000000.0f;
	for (int i=0; i < num_verts; ++i)
	{
		float y=p_verts[i].GetY();
		if (y < min_y)
		{
			min_y=y;
		}
		if (y > max_y)
		{
			max_y=y;
		}	
	}
	
	// Check that max_y and min_y are the same, just opposite in sign.
	Dbg_MsgAssert(fabs(max_y-(-min_y)) < 0.01f,("Expected post geometry to have origin in centre, but min_y=%f, max_y=%f",min_y,max_y));
	Dbg_MsgAssert(max_y > 0.0f,("max_y < 0 ??  (max_y=%f)",max_y));
	
	float half_post_height=max_y;
	
	float rail_sector_height=RAIL_SECTOR_HEIGHT;
	// If the next post is at a different height, the height of the rail be a bit smaller than normal
	// due to the end face being tilted to point towards the next post.
	if (mpNext && mpPrevious)
	{
		Mth::Vector a=mpPrevious->mPos-mPos;
		Mth::Vector b=mPos-mpNext->mPos;
		// The end-face's normal will be the average of the two rails
		Mth::Vector av=(a+b)/2.0f;
		av.Normalize();
		// av[Y] is the sine of the angle, but we want the cosine
		rail_sector_height *= sqrtf(1.0f-av[Y]*av[Y]);
	}	

	Mth::Vector translated_rail_point=s_rotate_and_translate(mPos, rotateCentre, newCentre, degrees);
	
	// Translate the post so that it is centred on the rail point, then shift the y's down so that the
	// top of the post is at the bottom of the rail piece.
	// The height of the rail section needs to be added too because the rail point (mPos) is at the apex of the
	// rail section.
	float shift_down_amount=half_post_height+rail_sector_height;
	for (int i=0; i < num_verts; ++i)
	{
		p_verts[i]+=translated_rail_point;
		p_verts[i][Y]-=shift_down_amount;
	}

	// The top of the post is now in the correct position.
	// Now we need to move the bottom vertices down to ground level.
	
	// This array figured out manually, by printing out the vertex coords.
	// TODO: These indices will need to be different on Xbox and GameCube
#	ifdef __PLAT_NGC__
	int p_bottom_vert_indices[]=
	{
		0,2,4,6,		// Post bottom
		8,9,10,11		// Base plate
	};
#else
#	ifdef __PLAT_XBOX__
	int p_bottom_vert_indices[]=
	{
		8,9,10,11,		// Post bottom
		1,2,5,6			// Base plate
	};
#	else
	int p_bottom_vert_indices[]=
	{
		1,3,5,7,9,		// Post bottom
		10,11,12,13		// Base plate
	};
#	endif
#	endif
	int num_bottom_vertices=sizeof(p_bottom_vert_indices)/sizeof(int);
	Dbg_MsgAssert(num_bottom_vertices <= num_verts,("Too many bottom vertices!"));

	// The amount that the bottom vertices need to be moved down is the height, minus the
	// previous shift_down_amount cos we've already moved them by that much, minus
	// half the height of the post so that the bottom is on the ground.
	shift_down_amount=mHeightAboveGround-shift_down_amount-half_post_height;
	
	for (int i=0; i<num_bottom_vertices; ++i)
	{
		p_verts[p_bottom_vert_indices[i]][Y]-=shift_down_amount;
	}	

	// Now rotate the post about the y so that it is aligned with
	// the previous rail section.
	if (mpNext)
	{
		Mth::Vector u=mPos-mpNext->mPos;
		u[Y]=0.0f;
		u.Normalize();
		float ux=u[X];
		float uz=u[Z];
		float vx=-uz;
		float vz=ux;

		for (int i=0; i < num_verts; ++i)
		{
			float dx=p_verts[i][X]-translated_rail_point[X];
			float dz=p_verts[i][Z]-translated_rail_point[Z];
			p_verts[i][X]=ux*dx+vx*dz+translated_rail_point[X];
			p_verts[i][Z]=uz*dx+vz*dz+translated_rail_point[Z];
		}			
	}

	// Clone the sector if necessary and write in the vertex coords just calculated.
	if (!mpPostSector)
	{
		mpPostSector=s_clone_sector(CRCD(0x2756c52d,"Sk5Ed_RAp_Dynamic"));
	}
	
	Nx::CGeom *p_geom=mpPostSector->GetGeom();
	Dbg_MsgAssert(p_geom,("NULL p_geom ?"));
	Dbg_MsgAssert(p_geom->GetNumRenderVerts()==p_source_geom->GetNumRenderVerts(),("Source geom num verts mismatch"));
	p_geom->SetRenderVerts(p_verts);
	Mem::Free(p_verts);	
}

void CEditedRailPoint::DestroyRailGeometry()
{
	if (mpClonedRailSector)
	{
		Nx::CScene *p_cloned_scene=Ed::CParkManager::sInstance()->GetGenerator()->GetClonedScene();
		Dbg_MsgAssert(p_cloned_scene,("Missing cloned scene!"));
		p_cloned_scene->DeleteSector(mpClonedRailSector);
		mpClonedRailSector=NULL;

		#ifdef __PLAT_NGC__
		Nx::CEngine::sFinishRendering();
		#endif

		// SPEEDOPT: If a batch of things are deleted, only do this once at the end.
		if (CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors)
		{
			p_cloned_scene->UpdateSuperSectors();
		}	
	}
}

void CEditedRailPoint::DestroyPostGeometry()
{
	if (mpPostSector)
	{
		Nx::CScene *p_cloned_scene=Ed::CParkManager::sInstance()->GetGenerator()->GetClonedScene();
		Dbg_MsgAssert(p_cloned_scene,("Missing cloned scene!"));
		p_cloned_scene->DeleteSector(mpPostSector);
		mpPostSector=NULL;
		
		#ifdef __PLAT_NGC__
		Nx::CEngine::sFinishRendering();
		#endif
		
		// SPEEDOPT: If a batch of things are deleted, only do this once at the end.
		if (CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors)
		{
			p_cloned_scene->UpdateSuperSectors();
		}	
	}
}

// Does a drop down collision check to find the height of mPos above the ground.
float CEditedRailPoint::FindGroundY()
{
	Mth::Vector off(0.0f,10000.0f,0.0f,0.0f);
	
	CFeeler feeler;
	feeler.SetStart(mPos + off );
	feeler.SetEnd(mPos - off );
	if (feeler.GetCollision())
	{
		return feeler.GetPoint().GetY();
	}
	else
	{
		return 0.0f;
	}	
}

// This will adjust mPos[Y] to be mHeightAboveGround above the ground.
// This is used to update the rails whenever the ground is raised or lowered.
// The rails will maintain the post heights if a post was on the ground
// that moved.
void CEditedRailPoint::AdjustY()
{
	if (mHasPost)
	{
		mPos[Y]=FindGroundY()+mHeightAboveGround;
	}	
}

void CEditedRailPoint::InitialiseHeight()
{
	mHeightAboveGround=mPos[Y]-FindGroundY();
}

void CEditedRailPoint::Highlight(EFlash flash, EEndPosts includeEndPosts)
{
	if (mHighlighted)
	{
		return;
	}
		
	uint32 vis=0xffffffff;
	if (flash)
	{
		int flash_rate=Script::GetInteger(CRCD(0x4551e901,"RailEditorHighlightFlashRate"));
		if (s_highlight_flash_counter < flash_rate/2)
		{
			vis=0xffffffff;
		}
		else
		{
			vis=0;
		}
			
		++s_highlight_flash_counter;
		if (s_highlight_flash_counter >= flash_rate)
		{
			s_highlight_flash_counter=0;
		}	
	}

	uint8 good_r=Script::GetInteger(CRCD(0xad28b18b,"RailEditorHighlightColourR"));
	uint8 good_g=Script::GetInteger(CRCD(0xc0f55560,"RailEditorHighlightColourG"));
	uint8 good_b=Script::GetInteger(CRCD(0xb09fa1ef,"RailEditorHighlightColourB"));

	uint8 bad_r=Script::GetInteger(CRCD(0x89b66fb9,"RailEditorBadAngleHighlightColourR"));
	uint8 bad_g=Script::GetInteger(CRCD(0xe46b8b52,"RailEditorBadAngleHighlightColourG"));
	uint8 bad_b=Script::GetInteger(CRCD(0x94017fdd,"RailEditorBadAngleHighlightColourB"));
	
	uint8 r=good_r;
	uint8 g=good_g;
	uint8 b=good_b;
		
	bool section_stretched_too_long=false;
	if (mpNext)
	{
		if (s_distance_is_too_long(mPos, mpNext->mPos))
		{
			section_stretched_too_long=true;
		}
	}		
	if (mpPrevious)
	{
		if (s_distance_is_too_long(mPos, mpPrevious->mPos))
		{
			section_stretched_too_long=true;
		}
	}		
	
	if (mpPostSector)
	{
		if (AngleIsOKToGrind() && !section_stretched_too_long)
		{
			r=good_r;
			g=good_g;
			b=good_b;
		}
		else
		{
			r=bad_r;
			g=bad_g;
			b=bad_b;
		}	
		mpPostSector->SetColor(Image::RGBA(r,g,b,0));
		mpPostSector->SetVisibility(vis);
	}	
	
	if (mpClonedRailSector)
	{
		if (mpNext)
		{
			if (mpNext->AngleIsOKToGrind() && AngleIsOKToGrind() && !section_stretched_too_long)
			{
				// The angle at the point at the other end of mpClonedRailSector is OK,
				// and so is the angle at this point, so all OK.
				r=good_r;
				g=good_g;
				b=good_b;
			}
			else
			{
				r=bad_r;
				g=bad_g;
				b=bad_b;
			}	
		}
		
		mpClonedRailSector->SetColor(Image::RGBA(r,g,b,0));
		mpClonedRailSector->SetVisibility(vis);
	}
	if (mpPrevious && mpPrevious->mpClonedRailSector)
	{
		if (mpPrevious->AngleIsOKToGrind() && AngleIsOKToGrind() && !section_stretched_too_long)
		{
			// The angle at the point at the other end of the next segment is OK,
			// and so is the angle at this point, so all OK.
			r=good_r;
			g=good_g;
			b=good_b;
		}
		else
		{
			r=bad_r;
			g=bad_g;
			b=bad_b;
		}	
		
		mpPrevious->mpClonedRailSector->SetColor(Image::RGBA(r,g,b,0));
		mpPrevious->mpClonedRailSector->SetVisibility(vis);
	}	

	if (includeEndPosts)
	{
		if (mpNext && !mpNext->mpNext && mpNext->mpPostSector)
		{
			mpNext->mpPostSector->SetColor(Image::RGBA(good_r,good_g,good_b,0));
			mpNext->mpPostSector->SetVisibility(vis);
		}
		if (mpPrevious && !mpPrevious->mpPrevious && mpPrevious->mpPostSector)
		{
			mpPrevious->mpPostSector->SetColor(Image::RGBA(good_r,good_g,good_b,0));
			mpPrevious->mpPostSector->SetVisibility(vis);
		}
	}
		
	mHighlighted=true;
}

void CEditedRailPoint::UnHighlight()
{
	if (mHighlighted)
	{
		if (mpPostSector)
		{
			mpPostSector->ClearColor();
			mpPostSector->SetVisibility(0xffffffff);
		}	
		if (mpClonedRailSector)
		{
			mpClonedRailSector->ClearColor();
			mpClonedRailSector->SetVisibility(0xffffffff);
		}
		if (mpPrevious && mpPrevious->mpClonedRailSector)
		{
			mpPrevious->mpClonedRailSector->ClearColor();
			mpPrevious->mpClonedRailSector->SetVisibility(0xffffffff);
		}	

		if (mpNext && !mpNext->mpNext && mpNext->mpPostSector)
		{
			mpNext->mpPostSector->ClearColor();
			mpNext->mpPostSector->SetVisibility(0xffffffff);
		}	
		if (mpPrevious && !mpPrevious->mpPrevious && mpPrevious->mpPostSector)
		{
			mpPrevious->mpPostSector->ClearColor();
			mpPrevious->mpPostSector->SetVisibility(0xffffffff);
		}	
		
		mHighlighted=false;
	}	
}

// Used by graffiti games
void CEditedRailPoint::SetColor(Image::RGBA rgba)
{
	if (mpPostSector)
	{
		mpPostSector->SetColor(rgba);
	}	
	if (mpClonedRailSector)
	{
		mpClonedRailSector->SetColor(rgba);
	}	
}

// Used by graffiti games
void CEditedRailPoint::ClearColor()
{
	if (mpPostSector)
	{
		mpPostSector->ClearColor();
	}	
	if (mpClonedRailSector)
	{
		mpClonedRailSector->ClearColor();
	}	
}

void CEditedRailPoint::SetSectorActiveStatus(bool active)
{
	if (mpPostSector)
	{
		mpPostSector->SetActive(active);
	}	
	if (mpClonedRailSector)
	{
		mpClonedRailSector->SetActive(active);
	}	
}

bool CEditedRailPoint::AngleIsOKToGrind()
{
	if (mpNext && mpPrevious)
	{
		return s_angle_is_ok_to_grind(mpNext->mPos, mPos, mpPrevious->mPos);
	}
	return true;	
}

void CEditedRailPoint::WriteCompressedRailPoint(SCompressedRailPoint *p_dest)
{
	Dbg_MsgAssert(p_dest,("NULL p_dest"));
	
	p_dest->mHasPost=mHasPost;
	
	// Sometimes the point may have a height that is fractionally below zero, so instead of asserting
	// clamp it to zero.
	//Dbg_MsgAssert(mHeightAboveGround >= 0.0f,("Expected mHeightAboveGround to be >= 0 ? (is %f)",mHeightAboveGround));
	if (mHeightAboveGround < 0.0f)
	{
		p_dest->mHeight=0;
	}
	else
	{
		p_dest->mHeight=(uint16)(mHeightAboveGround*16.0f);
	}	
	
	Dbg_MsgAssert(fabs(mPos[X]*8.0f)<=32767.0f,("Rail point position x=%f out of range",mPos[X]));
	Dbg_MsgAssert(fabs(mPos[Y]*8.0f)<=32767.0f,("Rail point position y=%f out of range",mPos[Y]));
	Dbg_MsgAssert(fabs(mPos[Z]*8.0f)<=32767.0f,("Rail point position z=%f out of range",mPos[Z]));
	
	p_dest->mX=(sint16)(mPos[X]*8.0f);
	p_dest->mY=(sint16)(mPos[Y]*8.0f);
	p_dest->mZ=(sint16)(mPos[Z]*8.0f);
}

CEditedRail::CEditedRail()
{
	mId=s_rail_unique_id++;
	
	mpRailPoints=NULL;
	mpNext=NULL;
	mpPrevious=NULL;
	Clear();
}

CEditedRail::~CEditedRail()
{
	Clear();
}

void CEditedRail::Clear()
{
	CEditedRailPoint *p_rail_point=mpRailPoints;
	while (p_rail_point)
	{
		CEditedRailPoint *p_next=p_rail_point->mpNext;
		delete p_rail_point;
		p_rail_point=p_next;
	}	
	mpRailPoints=NULL;
}

// Runs through all the points and adjusts the y coords so that the stored heights match the actual
// heights above the ground. This is used to keep the post heights constant when raising or lowering the
// ground in the park editor.
void CEditedRail::AdjustYs()
{
	CEditedRailPoint *p_rail_point=mpRailPoints;
	while (p_rail_point)
	{
		p_rail_point->AdjustY();
		p_rail_point=p_rail_point->mpNext;
	}	
}

void CEditedRail::InitialiseHeights()
{
	CEditedRailPoint *p_rail_point=mpRailPoints;
	while (p_rail_point)
	{
		p_rail_point->InitialiseHeight();
		p_rail_point=p_rail_point->mpNext;
	}	
}

void CEditedRail::UpdateRailGeometry(Mth::Vector& rotateCentre, Mth::Vector& newCentre, float degrees)
{
	// Find the last rail point.
	CEditedRailPoint *p_rail_point=mpRailPoints;
	CEditedRailPoint *p_last=NULL;
	while (p_rail_point)
	{
		p_last=p_rail_point;
		p_rail_point=p_rail_point->mpNext;
	}	

	// It is necessary to traverse the list backwards because UpdateRailGeometry() uses some of the vertices of the next
	// rail in the list for the end points of its rail, so the next rail needs to have been updated first.
	p_rail_point=p_last;
	while (p_rail_point)
	{
		p_rail_point->UpdateRailGeometry(rotateCentre, newCentre, degrees);
		p_rail_point=p_rail_point->mpPrevious;
	}	
}

void CEditedRail::UpdatePostGeometry(Mth::Vector& rotateCentre, Mth::Vector& newCentre, float degrees)
{
	CEditedRailPoint *p_rail_point=mpRailPoints;
	while (p_rail_point)
	{
		p_rail_point->UpdatePostGeometry(rotateCentre, newCentre, degrees);
		p_rail_point=p_rail_point->mpNext;
	}	
}

void CEditedRail::DestroyRailGeometry()
{
	CEditedRailPoint *p_rail_point=mpRailPoints;
	while (p_rail_point)
	{
		p_rail_point->DestroyRailGeometry();
		p_rail_point=p_rail_point->mpNext;
	}	
}

void CEditedRail::DestroyPostGeometry()
{
	CEditedRailPoint *p_rail_point=mpRailPoints;
	while (p_rail_point)
	{
		p_rail_point->DestroyPostGeometry();
		p_rail_point=p_rail_point->mpNext;
	}	
}

CEditedRailPoint *CEditedRail::AddPoint()
{
	if (CEditedRailPoint::SGetNumUsedItems()==MAX_EDITED_RAIL_POINTS)
	{
		// No space left on the pool to store a new point.
		return NULL;
	}
	
	CEditedRailPoint *p_new=new CEditedRailPoint;
	p_new->mpNext=mpRailPoints;
	if (mpRailPoints)
	{
		mpRailPoints->mpPrevious=p_new;
	}
	p_new->mpPrevious=NULL;
	mpRailPoints=p_new;
	
	return p_new;	
}

int CEditedRail::CountPoints()
{
	int num_points=0;
	CEditedRailPoint *p_point=mpRailPoints;
	while (p_point)
	{
		++num_points;
		p_point=p_point->mpNext;
	}
	return num_points;
}

bool CEditedRail::FindNearestRailPoint(Mth::Vector &pos,
									   Mth::Vector *p_nearest_pos, float *p_dist_squared, int *p_rail_point_index, 
									   int ignore_index)
{
	if (!mpRailPoints)
	{
		return false;
	}
		
	CEditedRailPoint *p_point=mpRailPoints;
	int rail_point_index=0;
	float min_dist_squared=100000000.0f;
	while (p_point)
	{
		if (rail_point_index != ignore_index)
		{
			Mth::Vector diff=p_point->mPos-pos;
			// Zero the y so that one does not have to raise the cursor to make high rails highlight.
			// (Make's a big difference!)
			diff[Y]=0.0f;
			float dd=diff.LengthSqr();
			
			if (dd < min_dist_squared)
			{
				min_dist_squared=dd;
				
				*p_nearest_pos=p_point->mPos;
				*p_dist_squared=min_dist_squared;
				*p_rail_point_index=rail_point_index;
			}
		}
			
		p_point=p_point->mpNext;
		++rail_point_index;
	}
	
	return true;
}

int CEditedRail::CountPointsInArea(float x0, float z0, float x1, float z1)
{
	int num_points=0;

   	CEditedRailPoint *p_point=mpRailPoints;
	while (p_point)
	{
		if (p_point->mPos[X] > x0 && p_point->mPos[X] < x1 &&
			p_point->mPos[Z] > z0 && p_point->mPos[Z] < z1)
		{
			// The point is inside the area
			++num_points;
		}
			
		p_point=p_point->mpNext;
	}	
	
	return num_points;
}

void CEditedRail::DuplicateAndAddPoint(CEditedRailPoint *p_point)
{
	Dbg_MsgAssert(p_point,("NULL p_point"));
	
	CEditedRailPoint *p_new_point=AddPoint();
	Dbg_MsgAssert(p_new_point,("NULL p_new_point"));
	
	p_new_point->mPos=p_point->mPos;
	p_new_point->mHasPost=p_point->mHasPost;
	
	p_new_point->mHeightAboveGround=p_point->mHeightAboveGround;
}

CEditedRail *CEditedRail::GenerateDuplicateRails(float x0, float z0, float x1, float z1, CEditedRail *p_head)
{
	bool inside=false;
	CEditedRailPoint *p_first_point_of_new_rail=NULL;
	
   	CEditedRailPoint *p_point=mpRailPoints;
	while (p_point)
	{
		if (p_point->mPos[X] > x0 && p_point->mPos[X] < x1 &&
			p_point->mPos[Z] > z0 && p_point->mPos[Z] < z1)
		{
			// The point is inside the area
			
			if (inside)
			{
				if (p_first_point_of_new_rail)
				{
					CEditedRail *p_new_rail=new CEditedRail;
					p_new_rail->mpNext=p_head;
					p_head=p_new_rail;
					
					p_head->DuplicateAndAddPoint(p_first_point_of_new_rail);
					p_first_point_of_new_rail=NULL;
				}	
				p_head->DuplicateAndAddPoint(p_point);
			}
			else	
			{
				// Gone from outside to inside, so it's time to make a new rail.
				// Don't actually create the rail yet though, since it may only end up containing
				// one point, & don't want to create those.
				p_first_point_of_new_rail=p_point;
			}
			
			inside=true;
		}
		else
		{
			p_first_point_of_new_rail=NULL;
			inside=false;	
		}
			
		p_point=p_point->mpNext;
	}	

	return p_head;
}

void CEditedRail::UnHighlight()
{
   	CEditedRailPoint *p_point=mpRailPoints;
	while (p_point)
	{
		p_point->UnHighlight();
		p_point=p_point->mpNext;
	}	
}

CEditedRailPoint *CEditedRail::GetRailPointFromIndex(int index)
{
   	CEditedRailPoint *p_point=mpRailPoints;
	while (index)
	{
		if (!p_point)
		{
			break;
		}	
		p_point=p_point->mpNext;
		--index;
	}	
	Dbg_MsgAssert(p_point,("Could not find rail point with index %d",index));
	
	return p_point;
}

// Deletes p_point from the rail, and returns a pointer to any fragment that gets created.
// If the max rails has been reached and deleting the point will result in a fragment,
// then it won't delete the point after all, because a new rail will not be able to be created for
// the fragment.
CEditedRailPoint *CEditedRail::DeleteRailPoint(CEditedRailPoint *p_point)
{
	if (!p_point)
	{
		return NULL;
	}
	
	// Check that p_point is in the rail ...
	#ifdef __NOPT_ASSERT__
	bool found=false;
	CEditedRailPoint *p_check=mpRailPoints;
	while (p_check)
	{
		if (p_check == p_point)
		{
			found=true;
			break;
		}
		p_check=p_check->mpNext;
	}
	Dbg_MsgAssert(found,("p_point is not in the rail !"));		
	#endif
	
	if (p_point->mpPrevious && p_point->mpNext)
	{
		// A new rail will need to be created so bail out if there is not enough
		// space to create one.
		if (CEditedRail::SGetNumUsedItems()==MAX_EDITED_RAILS)
		{
			return NULL;
		}
	}
	
	// Update this rail if removing its first point.
	if (p_point == mpRailPoints)
	{
		Dbg_MsgAssert(p_point->mpPrevious==NULL,("Expected p_point->mpPrevious==NULL"));
		mpRailPoints=p_point->mpNext;
		if (p_point->mpNext)
		{
			p_point->mpNext->mpPrevious=NULL;
		}
		delete p_point;
		return NULL;
	}

	Dbg_MsgAssert(p_point->mpPrevious,("Expected p_point->mpPrevious not to be NULL"));
	// Remove any rail geometry connecting the adjacent point to this one.
	p_point->mpPrevious->DestroyRailGeometry();
	// Terminate this rail.
	p_point->mpPrevious->mpNext=NULL;
	
	// Return the new rail fragment.
	CEditedRailPoint *p_fragment_start=p_point->mpNext;
	delete p_point;
	
	if (p_fragment_start)
	{
		p_fragment_start->mpPrevious=NULL;
	}
	return p_fragment_start;
}

bool CEditedRail::UpdateRailPointPosition(int rail_point_index, Mth::Vector &pos, EUpdateSuperSectors updateSuperSectors)
{
   	CEditedRailPoint *p_point=GetRailPointFromIndex(rail_point_index);
	
	// UpdateRailPointPosition returns false if the position is bad in that it is
	// too close to an adjacent point or causes the rail to be too steep.
	// It still writes in the position so that the rail always stays attached to
	// the cursor as the user moves it around when in grab mode.
	// However, it won't write in the position if one of the rails will be stretched too long,
	// because that causes rendering problems.
	
	bool position_is_ok=true;
	if (p_point->mpNext)
	{
		if (!s_positions_OK(pos,p_point->mpNext->mPos))
		{
			position_is_ok=false;
		}
	}
	if (p_point->mpPrevious)
	{
		if (!s_positions_OK(pos,p_point->mpPrevious->mPos))
		{
			position_is_ok=false;
		}
	}

	// Don't allow points to be placed too close to the boundary of the park (TT5464)
	if (!Ed::IsWithinParkBoundaries(pos,PARK_BOUNDARY_MARGIN))
	{
		position_is_ok=false;
	}

	bool ok_to_update=true;
	if (p_point->mpNext)
	{
		if (s_distance_is_way_too_long(pos,p_point->mpNext->mPos))
		{
			ok_to_update=false;
		}	
	}
	if (p_point->mpPrevious)
	{
		if (s_distance_is_way_too_long(pos,p_point->mpPrevious->mPos))
		{
			ok_to_update=false;
		}	
	}


	if (ok_to_update)
	{
		p_point->mPos=pos;
		p_point->InitialiseHeight();
		p_point->UpdatePostGeometry();
		p_point->UpdateRailGeometry();
		if (p_point->mpPrevious)
		{
			p_point->mpPrevious->UpdateRailGeometry();
		}	
	
		// Updating the super sectors is technically required every time the collision verts on a sector
		// are modified, as they will be by this function.
		// If UpdateSuperSectors is not done, then when attempting to delete the sector later the low-level code
		// (in nxscene) will assert.
		// However, it can be slow to call UpdateSuperSectors every frame, so when a rail point is being dragged
		// around in grab mode it is not called. It is only called when placing the new point or when backing
		// out and restoring the old position.
		if (updateSuperSectors)
		{
			Nx::CScene *p_cloned_scene=Ed::CParkManager::sInstance()->GetGenerator()->GetClonedScene();
			Dbg_MsgAssert(p_cloned_scene,("NULL p_cloned_scene ?"));
			p_cloned_scene->UpdateSuperSectors();
		}
	}
		
	return position_is_ok;
}

// Used when pasting a rail
void CEditedRail::CopyRail(CEditedRail *p_source_rail)
{
	Clear();
	
	Dbg_MsgAssert(p_source_rail,("NULL p_source_rail"));
	
	CEditedRailPoint *p_source_point=p_source_rail->mpRailPoints;
	while (p_source_point)
	{
		CEditedRailPoint *p_new_point=AddPoint();
		
		p_new_point->mPos=p_source_point->mPos;
		p_new_point->mHasPost=p_source_point->mHasPost;
		p_new_point->mHeightAboveGround=p_source_point->mHeightAboveGround;
		
		p_source_point=p_source_point->mpNext;
	}	
}

// Used when pasting a rail
void CEditedRail::RotateAndTranslate(Mth::Vector& rotateCentre, Mth::Vector& newCentre, float degrees)
{
	CEditedRailPoint *p_point=mpRailPoints;
	while (p_point)
	{
		p_point->mPos=s_rotate_and_translate(p_point->mPos, rotateCentre, newCentre, degrees);
		p_point=p_point->mpNext;
	}
}

SCompressedRailPoint *CEditedRail::WriteCompressedRailPoints(SCompressedRailPoint *p_dest)
{
	Dbg_MsgAssert(p_dest,("NULL p_dest"));

	CEditedRailPoint *p_point=mpRailPoints;
	// Skip to the end of the list of points so that it can be traversed backwards.
	// This is so that when the rails are regenerated using InitUsingCompressedRailsBuffer()
	// they get recreated with the points in the original order, and hence the rail node indices 
	// will match on the server and client. 
	while (p_point && p_point->mpNext)
	{
		p_point=p_point->mpNext;
	}
	
	while (p_point)
	{
		p_point->WriteCompressedRailPoint(p_dest++);
		p_point=p_point->mpPrevious;
	}
	return p_dest;
}

// Used by graffiti games
void CEditedRail::ModulateRailColor(int seqIndex)
{
	Script::CArray* p_graffiti_col_tab = Script::GetArray( CRCD(0x7f1ba1aa,"graffitiColors") );
	Dbg_MsgAssert( (uint) seqIndex < p_graffiti_col_tab->GetSize(), ( "graffitiColors array too small" ) );

	Script::CArray *p_entry = p_graffiti_col_tab->GetArray(seqIndex);
	
	#ifdef	__NOPT_ASSERT__
	int size = p_entry->GetSize();
	Dbg_MsgAssert(size >= 3 && size <= 4, ("wrong size %d for color array", size));
	#endif
	
	Image::RGBA color;

	color.r = p_entry->GetInteger( 0 );
	color.g = p_entry->GetInteger( 1 );
	color.b = p_entry->GetInteger( 2 );
	color.a = 128;
	
	CEditedRailPoint *p_point=mpRailPoints;
	while (p_point)
	{
		p_point->SetColor(color);
		p_point=p_point->mpNext;
	}
}

// Used by graffiti games
void CEditedRail::ClearRailColor()
{
	CEditedRailPoint *p_point=mpRailPoints;
	while (p_point)
	{
		p_point->ClearColor();
		p_point=p_point->mpNext;
	}
}

void CEditedRail::SetSectorActiveStatus(bool active)
{
	CEditedRailPoint *p_point=mpRailPoints;
	while (p_point)
	{
		p_point->SetSectorActiveStatus(active);
		p_point=p_point->mpNext;
	}
}

void CEditedRail::GetDebugInfo( Script::CStruct* p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info"));
	
	p_info->AddChecksum(CRCD(0x40c698af,"id"),mId);
	
	int num_points=CountPoints();
		
	if (num_points)
	{
		Script::CArray *p_array=new Script::CArray;
		p_array->SetSizeAndType(num_points,ESYMBOLTYPE_STRUCTURE);

		int i=0;
		CEditedRailPoint *p_point=mpRailPoints;
		while (p_point)
		{
			Script::CStruct *p_struct=new Script::CStruct;
			p_struct->AddVector(CRCD(0x7f261953,"pos"),p_point->mPos[X],p_point->mPos[Y],p_point->mPos[Z]);
			p_struct->AddFloat(CRCD(0xab21af0,"Height"),p_point->mHeightAboveGround);
			p_struct->AddInteger(CRCD(0x41a51a93,"mpClonedRailSector"),(int)p_point->mpClonedRailSector);
			p_struct->AddInteger(CRCD(0x79ffd768,"mpPostSector"),(int)p_point->mpPostSector);
			
			p_array->SetStructure(i,p_struct);
			++i;
			p_point=p_point->mpNext;
		}
		
		p_info->AddArrayPointer(CRCD(0xd84571d6,"Points"),p_array);
	}
#endif				 
}

bool CEditedRail::ThereAreRailPointsOutsideArea(float x0, float z0, float x1, float z1)
{
	Dbg_MsgAssert(x0 <= x1 && z0 <= z1,("Bad area"));
	
   	CEditedRailPoint *p_point=mpRailPoints;
	while (p_point)
	{
		if (p_point->mPos[X] < x0 || p_point->mPos[X] > x1 ||
			p_point->mPos[Z] < z0 || p_point->mPos[Z] > z1)
		{
			return true;
		}	
		p_point=p_point->mpNext;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors=true;

// s_create is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
// s_create	returns a CBaseComponent*, as it is to be used
// by factor creation schemes that do not care what type of
// component is being created
// **  after you've finished creating this component, be sure to
// **  add it to the list of registered functions in the
// **  CCompositeObjectManager constructor  

CBaseComponent* CRailEditorComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CRailEditorComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// All components set their type, which is a unique 32-bit number
// (the CRC of their name), which is used to identify the component	
CRailEditorComponent::CRailEditorComponent() : CBaseComponent()
{
	SetType( CRC_RAILEDITOR );

	Dbg_MsgAssert(MAX_EDITED_RAILS == MAX_EDITED_RAIL_POINTS/2,("Bad MAX_EDITED_RAILS"));
	
	CEditedRailPoint::SCreatePool(MAX_EDITED_RAIL_POINTS, "CEditedRailPoint");
	CEditedRail::SCreatePool(MAX_EDITED_RAILS, "CEditedRail");

	mp_input_component=NULL;
	mp_editor_camera_component=NULL;
	mp_dotted_line_sector=NULL;
	m_dotted_line_sector_name=0;
	mp_edited_rails=NULL;

	clear_compressed_rails_buffer();
	Clear();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CRailEditorComponent::~CRailEditorComponent()
{
	Clear();
	CEditedRailPoint::SRemovePool();
	CEditedRail::SRemovePool();
}

void CRailEditorComponent::clear_compressed_rails_buffer()
{
	for (uint i=0; i<sizeof(mp_compressed_rails_buffer); ++i)
	{
		mp_compressed_rails_buffer[i]=0;
	}
}

void CRailEditorComponent::generate_compressed_rails_buffer()
{
	clear_compressed_rails_buffer();
	
	*(uint16*)mp_compressed_rails_buffer=count_rails();
	uint16 *p_num_points_in_rail=(uint16*)(mp_compressed_rails_buffer+2);
	SCompressedRailPoint *p_rail_point=(SCompressedRailPoint*)(p_num_points_in_rail+MAX_EDITED_RAILS);
	
	CEditedRail *p_rail=mp_edited_rails;
	// Skip to the end of the list of rails so that it can be traversed backwards.
	// This is so that when the rails are regenerated using InitUsingCompressedRailsBuffer()
	// they get recreated in the original order, and hence the rail node indices will match on the
	// server and client. 
	while (p_rail && p_rail->mpNext)
	{
		p_rail=p_rail->mpNext;
	}
		
	while (p_rail)
	{
		*p_num_points_in_rail++=p_rail->CountPoints();
		p_rail_point=p_rail->WriteCompressedRailPoints(p_rail_point);	
		
		p_rail=p_rail->mpPrevious;
	}
	
	Dbg_MsgAssert((uint8*)p_rail_point <= mp_compressed_rails_buffer+sizeof(mp_compressed_rails_buffer),("p_rail_point overwrote end of buffer"));
	Dbg_MsgAssert(p_num_points_in_rail-(uint16*)(mp_compressed_rails_buffer+2)==*(uint16*)mp_compressed_rails_buffer,("Num rails mismatch"));
}

// Used when sending rail info over the net.
uint8 *CRailEditorComponent::GetCompressedRailsBuffer()
{
	generate_compressed_rails_buffer();
	return mp_compressed_rails_buffer;
}

void CRailEditorComponent::SetCompressedRailsBuffer(uint8 *p_buffer)
{
	Dbg_MsgAssert(p_buffer,("NULL p_buffer"));
	memcpy(mp_compressed_rails_buffer, p_buffer, sizeof(mp_compressed_rails_buffer));
}

void CRailEditorComponent::InitUsingCompressedRailsBuffer()
{
	Clear();
	
	uint16 num_rails=*(uint16*)mp_compressed_rails_buffer;
	uint16 *p_num_points_in_rail=(uint16*)(mp_compressed_rails_buffer+2);
	SCompressedRailPoint *p_rail_point=(SCompressedRailPoint*)(p_num_points_in_rail+MAX_EDITED_RAILS);
	
	for (uint i=0; i<num_rails; ++i)
	{
		uint16 num_points=*p_num_points_in_rail++;
		
		CEditedRail *p_new_rail=NewRail();
		for (uint p=0; p<num_points; ++p)
		{
			CEditedRailPoint *p_point=p_new_rail->AddPoint();
			if (p_point)
			{
				p_point->mHasPost=p_rail_point->mHasPost;
				p_point->mHeightAboveGround=p_rail_point->mHeight / 16.0f;
				p_point->mPos[X]=p_rail_point->mX / 8.0f;
				p_point->mPos[Y]=p_rail_point->mY / 8.0f;
				p_point->mPos[Z]=p_rail_point->mZ / 8.0f;
			}
			++p_rail_point;
		}	
	}
}

void CRailEditorComponent::AdjustYs()
{
	SetSectorActiveStatus(false);

	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		p_rail->AdjustYs();
		p_rail=p_rail->mpNext;
	}

	SetSectorActiveStatus(true);
}

void CRailEditorComponent::InitialiseHeights()
{
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		p_rail->InitialiseHeights();
		p_rail=p_rail->mpNext;
	}
}

void CRailEditorComponent::UpdateRailGeometry()
{
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		p_rail->UpdateRailGeometry();
		p_rail=p_rail->mpNext;
	}
	
	Nx::CScene *p_cloned_scene=Ed::CParkManager::sInstance()->GetGenerator()->GetClonedScene();
	Dbg_MsgAssert(p_cloned_scene,("NULL p_cloned_scene ?"));
	p_cloned_scene->UpdateSuperSectors();
}

void CRailEditorComponent::UpdatePostGeometry()
{
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		p_rail->UpdatePostGeometry();
		p_rail=p_rail->mpNext;
	}
	
	Nx::CScene *p_cloned_scene=Ed::CParkManager::sInstance()->GetGenerator()->GetClonedScene();
	Dbg_MsgAssert(p_cloned_scene,("NULL p_cloned_scene ?"));
	p_cloned_scene->UpdateSuperSectors();
}

void CRailEditorComponent::DestroyRailGeometry()
{
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		p_rail->DestroyRailGeometry();
		p_rail=p_rail->mpNext;
	}
}

void CRailEditorComponent::DestroyPostGeometry()
{
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		p_rail->DestroyPostGeometry();
		p_rail=p_rail->mpNext;
	}
}

void CRailEditorComponent::RefreshGeometry()
{
	InitialiseHeights();
	UpdateRailGeometry();
	UpdatePostGeometry();
}

void CRailEditorComponent::UnHighlightAllRails()
{
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		p_rail->UnHighlight();
		p_rail=p_rail->mpNext;
	}
}

// Removes the point p_point from the rail p_rail, so p_rail may contain no points afterwards.
// May result in the creation of a new rail, if the point was not one of the end points of p_rail.
// Returns true if it did create a new rail.
bool CRailEditorComponent::DeleteRailPoint(CEditedRail *p_rail, CEditedRailPoint *p_point)
{
	Dbg_MsgAssert(p_rail,("NULL p_rail"));
	Dbg_MsgAssert(p_point,("NULL p_point"));

	CEditedRailPoint *p_fragment=p_rail->DeleteRailPoint(p_point);
	if (p_fragment)
	{
		CEditedRail *p_new_rail=NewRail();
		p_new_rail->mpRailPoints=p_fragment;
		return true;
	}	
	
	return false;
}

// Used when resizing the park, or when deleting rail points within an area using the area-select tool.
// This will remove all rail points outside of, or inside of (x0,z0) (x1,z1)
// It needs to be a member function of CRailEditorComponent rather than CEditedRail, because
// it could result in the creation of new rails.
// Returns true if it did create new rails.
bool CRailEditorComponent::ClipRail(CEditedRail *p_rail, float x0, float z0, float x1, float z1, EClipType clipType)
{
	Dbg_MsgAssert(x0 <= x1,("Need x0 <= x1"));
	Dbg_MsgAssert(z0 <= z1,("Need z0 <= z1"));

	CEditedRailPoint *p_point=p_rail->mpRailPoints;
	while (p_point)
	{
		CEditedRailPoint *p_next=p_point->mpNext;
		
		bool delete_point=false;
		
		if (p_point->mPos[X] > x0 && p_point->mPos[X] < x1 &&
			p_point->mPos[Z] > z0 && p_point->mPos[Z] < z1)
		{
			// The point is inside the area
			if (clipType==DELETE_POINTS_INSIDE)
			{
				delete_point=true;
			}	
		}
		else	
		{
			// The point is outside the area
			if (clipType==DELETE_POINTS_OUTSIDE)
			{
				delete_point=true;
			}	
		}
		
		if (delete_point)
		{
			if (DeleteRailPoint(p_rail, p_point))
			{
				// Deleting the point resulted in a new rail being created, which means that
				// p_next no longer belongs to this rail, so bail out.
				return true;
			}
		}
		
		p_point=p_next;
	}		
	
	return false;
}

void CRailEditorComponent::ClipRails(float x0, float z0, float x1, float z1, EClipType clipType)
{
	if (x0 > x1)
	{
		float t=x0;
		x0=x1;
		x1=t;
	}
	if (z0 > z1)
	{
		float t=z0;
		z0=z1;
		z1=t;
	}
		
	while (true)
	{
		bool created_new_rails=false;
		
		CEditedRail *p_rail=mp_edited_rails;
		while (p_rail)
		{
			if (ClipRail(p_rail, x0, z0, x1, z1, clipType))
			{
				created_new_rails=true;
			}	
			p_rail=p_rail->mpNext;
		}		
		
		// Repeat until no new rails were created.
		// Any new rails created by the above ClipRail calls will have been stuck on the front of
		// the list, so mp_edited_rails will be different next time around.
		if (!created_new_rails)
		{
			break;
		}
	}
	
	RemoveEmptyAndSinglePointRails();
}

bool CRailEditorComponent::ThereAreRailPointsOutsideArea(float x0, float z0, float x1, float z1)
{
	if (x0 > x1)
	{
		float t=x0;
		x0=x1;
		x1=t;
	}
	if (z0 > z1)
	{
		float t=z0;
		z0=z1;
		z1=t;
	}
		
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		if (p_rail->ThereAreRailPointsOutsideArea(x0, z0, x1, z1))
		{
			return true;
		}	
		p_rail=p_rail->mpNext;
	}		
		
	return false;	
}

// Used when copying a set of rails to the clipboard.
// Counts how many rail points will need to be created when copying the given area to the clipboard.
// This allows it to bail out before attempting to copy if there are not enough points left in the pool.
int CRailEditorComponent::CountPointsInArea(float x0, float z0, float x1, float z1)
{
	if (x0 > x1)
	{
		float t=x0;
		x0=x1;
		x1=t;
	}
	if (z0 > z1)
	{
		float t=z0;
		z0=z1;
		z1=t;
	}
		
	int num_points=0;
		
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		num_points += p_rail->CountPointsInArea(x0, z0, x1, z1);
		p_rail=p_rail->mpNext;
	}		
	
	return num_points;
}

bool CRailEditorComponent::AbleToCopyRails(float x0, float z0, float x1, float z1)
{
	return CountPointsInArea(x0,z0,x1,z1) <= GetNumFreePoints();
}

CEditedRail *CRailEditorComponent::GenerateDuplicateRails(float x0, float z0, float x1, float z1)
{
	CEditedRail *p_duplicated_rails=NULL;
	
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		p_duplicated_rails=p_rail->GenerateDuplicateRails(x0, z0, x1, z1, p_duplicated_rails);
		p_rail=p_rail->mpNext;
	}
		
	return p_duplicated_rails;
}

bool CRailEditorComponent::FindNearestRailPoint(Mth::Vector &pos, Mth::Vector *p_nearest_pos, 
												float *p_dist, 
												uint32 *p_rail_id, int *p_rail_point_index,
												uint32 ignore_rail_id, int ignore_rail_point_index)
{
	float min_dist_squared=100000000.0f;
	bool found_nearest_point=false;
	
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		Mth::Vector nearest;
		float dist;
		int rail_point_index=0;
		
		int ignore_index=-1;
		if (p_rail->mId==ignore_rail_id)
		{
			ignore_index=ignore_rail_point_index;
		}	
		
		if (p_rail->FindNearestRailPoint(pos, &nearest, &dist, &rail_point_index, ignore_index))
		{
			if (dist < min_dist_squared)
			{
				min_dist_squared=dist;
				*p_nearest_pos=nearest;
				*p_dist=dist;
				*p_rail_id=p_rail->mId;
				*p_rail_point_index=rail_point_index;
				found_nearest_point=true;
			}	
		}
		
		p_rail=p_rail->mpNext;
	}
	
	return found_nearest_point;
}

// For writing to memcard
void CRailEditorComponent::WriteIntoStructure(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info"));
	
	int num_rails=count_rails();
	
	Script::CArray *p_rails=new Script::CArray;
	p_rails->SetSizeAndType(num_rails,ESYMBOLTYPE_STRUCTURE);
	
	int index=0;
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		Script::CStruct *p_rail_info=new Script::CStruct;
		
		// Note: Don't need to write out the mId member because ReadFromBuffer will create new rails,
		// which will each get a new id on creation.
		
		int num_points=p_rail->CountPoints();
		Script::CArray *p_points=new Script::CArray;
		p_points->SetSizeAndType(num_points,ESYMBOLTYPE_STRUCTURE);
		
		int point_index=0;
		CEditedRailPoint *p_point=p_rail->mpRailPoints;
		while (p_point)
		{
			Script::CStruct *p_point_info=new Script::CStruct;
			p_point_info->AddVector(CRCD(0x7f261953,"Pos"),p_point->mPos[X],p_point->mPos[Y],p_point->mPos[Z]);
			if (p_point->mpPostSector)
			{
				p_point_info->AddChecksum(NONAME,CRCD(0xdcded772,"HasPost"));
			}	
			p_points->SetStructure(point_index,p_point_info);
			
			p_point=p_point->mpNext;
			++point_index;
		}	

		p_rail_info->AddArrayPointer(CRCD(0xd84571d6,"Points"),p_points);
		p_rails->SetStructure(index,p_rail_info);
		++index;
		
		p_rail=p_rail->mpNext;
	}
	
	p_info->AddArrayPointer(CRCD(0x244550a6,"CreatedRails"),p_rails);
}

// For reading from memcard
void CRailEditorComponent::ReadFromStructure(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info"));
	
	Clear();
	
	Script::CArray *p_rails=NULL;
	p_info->GetArray(CRCD(0x244550a6,"CreatedRails"),&p_rails);
	if (!p_rails)
	{
		return;
	}

	Dbg_MsgAssert((int)p_rails->GetSize() <= GetNumFreeRails(),("Too many rails, got %d, max is %d",p_rails->GetSize(),GetNumFreeRails()));
	
	for (uint32 i=0; i<p_rails->GetSize(); ++i)
	{
		Script::CStruct *p_rail_info=p_rails->GetStructure(i);
		Dbg_MsgAssert(p_rail_info,("Eh? NULL p_rail_info ?"));
		
		NewRail();
		
		Script::CArray *p_points=NULL;
		p_rail_info->GetArray(CRCD(0xd84571d6,"Points"),&p_points,Script::ASSERT);
		for (uint32 p=0; p<p_points->GetSize(); ++p)
		{
			Dbg_MsgAssert(mp_current_rail,("NULL mp_current_rail ?"));
			CEditedRailPoint *p_new_point=mp_current_rail->AddPoint();
			
			Script::CStruct *p_point_info=p_points->GetStructure(p);
			p_point_info->GetVector(CRCD(0x7f261953,"pos"),&p_new_point->mPos);
			if (p_point_info->ContainsFlag(CRCD(0xdcded772,"HasPost")))
			{
				p_new_point->mHasPost=true;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CRailEditorComponent::InitFromStructure( Script::CStruct* pParams )
{
	// ** Add code to parse the structure, and initialize the component

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// RefreshFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CRailEditorComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Default to just calline InitFromStructure()
	// but if that does not handle it, then will need to write a specific 
	// function here. 
	// The user might only want to update a single field in the structure
	// and we don't want to be asserting becasue everything is missing 
	
	InitFromStructure(pParams);
}

void CRailEditorComponent::Hide( bool shouldHide )
{
	Ed::CParkManager *p_park_manager=Ed::CParkManager::sInstance();

	if (shouldHide)
	{
		Script::RunScript(CRCD(0xec0a515,"rail_editor_destroy_cursor"));
		p_park_manager->GetGenerator()->CleanUpOutRailSet();
		UnHighlightAllRails();
		DeleteDottedLine();
	}	
	else
	{
		Script::RunScript(CRCD(0x52d1ce6f,"rail_editor_create_cursor"));

		// Create the 'out rail' set within the park generator, required for snapping to the
		// nearest rail point.
		p_park_manager->GetGenerator()->GenerateOutRailSet(p_park_manager->GetConcreteMetaList());
		
		// Make sure we're using the skater's pad
		Script::CStruct *p_params=new Script::CStruct;
		p_params->AddInteger(CRCD(0x67e6859a,"player"),0);
		mp_input_component->InitFromStructure(p_params);
		delete p_params;

		
		// This is a quick fix to a bug where button triggers are stored up whilst the
		// rail editor component is suspended. I think it's a bug in the input component, cos
		// surely it should ignore the pad whist suspended?
		CControlPad& control_pad = mp_input_component->GetControlPad();
		control_pad.Reset();
	}
}

void CRailEditorComponent::get_pos_from_camera_component(Mth::Vector *p_pos, float *p_height, float *p_angle)
{
	Dbg_MsgAssert(mp_editor_camera_component,("NULL mp_editor_camera_component ?"));
	*p_pos=mp_editor_camera_component->GetCursorPos();
	*p_height=mp_editor_camera_component->GetCursorHeight();
	*p_angle=mp_editor_camera_component->GetCursorOrientation();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CRailEditorComponent::Finalize()
{
	// Get the pointers to the other required components.
	
	Dbg_MsgAssert(mp_input_component==NULL,("mp_input_component not NULL ?"));
	mp_input_component = GetInputComponentFromObject(GetObject());
	Dbg_MsgAssert(mp_input_component,("CRailEditorComponent requires parent object to have an input component!"));

	Dbg_MsgAssert(mp_editor_camera_component==NULL,("mp_editor_camera_component not NULL ?"));
	mp_editor_camera_component = GetEditorCameraComponentFromObject(GetObject());
	Dbg_MsgAssert(mp_editor_camera_component,("CRailEditorComponent requires parent object to have an EditorCamera component!"));
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// The component's Update() function is called from the CCompositeObject's 
// Update() function.  That is called every game frame by the CCompositeObjectManager
// from the s_logic_code function that the CCompositeObjectManager registers
// with the task manger.
void CRailEditorComponent::Update()
{
	CControlPad& control_pad = mp_input_component->GetControlPad();
	
	if (control_pad.m_x.GetTriggered())
	{
		control_pad.m_x.ClearTrigger();
		Script::RunScript(CRCD(0x415f2afa,"RailEditorX"),NULL,GetObject());
	}	

	#ifdef __PLAT_NGC__
	if (control_pad.m_square.GetTriggered())
	{
		control_pad.m_square.ClearTrigger();
		Script::RunScript(CRCD(0x3ad21c4e,"RailEditorTriangle"),NULL,GetObject());
	}	

	if (control_pad.m_triangle.GetTriggered())
	{
		control_pad.m_triangle.ClearTrigger();
		Script::RunScript(CRCD(0xd8d956c9,"RailEditorSquare"),NULL,GetObject());
	}	
	#else
	if (control_pad.m_triangle.GetTriggered())
	{
		control_pad.m_triangle.ClearTrigger();
		Script::RunScript(CRCD(0x3ad21c4e,"RailEditorTriangle"),NULL,GetObject());
	}	

	if (control_pad.m_square.GetTriggered())
	{
		control_pad.m_square.ClearTrigger();
		Script::RunScript(CRCD(0xd8d956c9,"RailEditorSquare"),NULL,GetObject());
	}	
	#endif
	
	if (control_pad.m_circle.GetTriggered())
	{
		control_pad.m_circle.ClearTrigger();
		Script::RunScript(CRCD(0xc18d5b19,"RailEditorCircle"),NULL,GetObject());
	}	

	Script::RunScript(CRCD(0x97153f0e,"RailEditorEveryFrame"),NULL,GetObject());
}

void CRailEditorComponent::Clear()
{
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		CEditedRail *p_next=p_rail->mpNext;
		delete p_rail;
		p_rail=p_next;
	}
	mp_edited_rails=NULL;
	mp_current_rail=NULL;
	m_mode=0;
	DeleteDottedLine();
}	

void CRailEditorComponent::DeleteRail(CEditedRail *p_rail)
{
	if (mp_edited_rails==p_rail)
	{
		mp_edited_rails=p_rail->mpNext;
	}	
	if (mp_current_rail==p_rail)
	{
		mp_current_rail=p_rail->mpNext;
	}	

	if (p_rail->mpNext)
	{
		p_rail->mpNext->mpPrevious=p_rail->mpPrevious;
	}
	if (p_rail->mpPrevious)
	{
		p_rail->mpPrevious->mpNext=p_rail->mpNext;
	}
	delete p_rail;
}

CEditedRail *CRailEditorComponent::NewRail()
{
	CEditedRail *p_new=new CEditedRail;
	
	p_new->mpNext=mp_edited_rails;
	p_new->mpPrevious=NULL;
	if (p_new->mpNext)
	{
		p_new->mpNext->mpPrevious=p_new;
	}
	mp_edited_rails=p_new;
	mp_current_rail=p_new;	
	
	return p_new;
}

CEditedRail *CRailEditorComponent::GetRail(uint32 id)
{
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		if (p_rail->mId==id)
		{
			return p_rail;
		}
		p_rail=p_rail->mpNext;
	}		
	return NULL;
}

void CRailEditorComponent::RemoveEmptyAndSinglePointRails()
{
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		CEditedRail *p_next=p_rail->mpNext;
		
		if (p_rail->mpRailPoints == NULL || (p_rail->mpRailPoints && p_rail->mpRailPoints->mpNext==NULL))
		{
			DeleteRail(p_rail);
		}	
		p_rail=p_next;
	}		
}

int CRailEditorComponent::count_rails()
{
	int num_rails=0;
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		++num_rails;
		p_rail=p_rail->mpNext;
	}
	return num_rails;
}

int CRailEditorComponent::GetTotalRailNodesRequired()
{
	int total_nodes=0;
	
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		CEditedRailPoint *p_point=p_rail->mpRailPoints;
		while (p_point)
		{
			++total_nodes;
			p_point=p_point->mpNext;
		}	
		p_rail=p_rail->mpNext;
	}
	
	return total_nodes;
}

void CRailEditorComponent::SetUpRailNodes(Script::CArray *p_nodeArray, int *p_nodeNum, uint32 firstID)
{
	Dbg_MsgAssert(p_nodeArray,("NULL p_nodeArray"));
	Dbg_MsgAssert(p_nodeNum,("NULL p_nodeNum"));
	
	int node_index=*p_nodeNum;
	uint32 rail_point_id=firstID;

	int rail_index=0;	
	char p_rail_name[50];
	
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		CEditedRailPoint *p_point=p_rail->mpRailPoints;
		while (p_point)
		{
			Script::CStruct *p_node=p_nodeArray->GetStructure(node_index++);
	
			p_node->AddVector(CRCD(0x7f261953,"Pos"),p_point->mPos[X],p_point->mPos[Y],p_point->mPos[Z]);
			p_node->AddVector(CRCD(0x9d2d0915,"Angles"),0.0f,0.0f,0.0f);
			
			p_node->AddChecksum(CRCD(0xa1dc81f9,"Name"),rail_point_id++);
			
			p_node->AddChecksum(CRCD(0x12b4e660,"Class"),CRCD(0x8e6b02ad,"RailNode"));
			p_node->AddChecksum(NONAME,CRCD(0x7c2552b9,"CreatedAtStart"));
			
			p_node->AddChecksum(NONAME,CRCD(0x1645b830,"TrickObject"));
			sprintf(p_rail_name,"CreatedRailCluster%d",rail_index);
			p_node->AddChecksum(CRCD(0x1a3a966b,"Cluster"),Script::GenerateCRC(p_rail_name));
			
			// hardwire to metal for the moment
			p_node->AddChecksum(CRCD(0x54cf8532,"TerrainType"),CRCD(0xa9ecf4e9,"TERRAIN_METALSMOOTH"));
			
			if (p_point->mpNext)
			{
				Script::CArray *p_links = new Script::CArray();
				p_links->SetSizeAndType(1,ESYMBOLTYPE_INTEGER);
				p_links->SetInteger(0, node_index);
				p_node->AddArrayPointer(CRCD(0x2e7d5ee7,"Links"), p_links);
			}			

			// An attempt to make graffiti mode able to tag created rails, didn't work though
			// (TT11801)
			//p_node->AddChecksum(NONAME,CRCD(0x1645b830,"TrickObject"));
			//char p_cluster_name[20];
			//sprintf(p_cluster_name,"RailCluster%d",node_index);
			//p_node->AddChecksum(CRCD(0x1a3a966b,"Cluster"),Script::GenerateCRC(p_cluster_name));
			
			p_point=p_point->mpNext;
		}	
		p_rail=p_rail->mpNext;
		++rail_index;
	}

	*p_nodeNum=node_index;
}

void CRailEditorComponent::DeleteDottedLine()
{
	if (mp_dotted_line_sector)
	{
		Nx::CScene *p_cloned_scene=Ed::CParkManager::sInstance()->GetGenerator()->GetClonedScene();
		Dbg_MsgAssert(p_cloned_scene,("Missing cloned scene!"));
		p_cloned_scene->DeleteSector(mp_dotted_line_sector);
		
		mp_dotted_line_sector=NULL;
		m_dotted_line_sector_name=0;

		#ifdef __PLAT_NGC__
		Nx::CEngine::sFinishRendering();
		#endif
		
		p_cloned_scene->UpdateSuperSectors();
	}	
}

void CRailEditorComponent::DrawDottedLine(Mth::Vector& pos)
{
	// Do nothing if there is no last position to draw from.
	if (!mp_current_rail || !mp_current_rail->mpRailPoints)
	{
		return;
	}
	CEditedRailPoint *p_last_point=mp_current_rail->mpRailPoints;
	
	
	uint32 dotted_line_sector_name=CRCD(0x5dc3c690,"Sk5Ed_RAdot_Dynamic_Green");
	if (s_distance_is_too_long(p_last_point->mPos, pos))
	{
		if (s_distance_is_way_too_long(p_last_point->mPos, pos))
		{
			// If the rail sector is stretched too far it renders all weird, so don't draw it at all.
			DeleteDottedLine();
			return;
		}	

		dotted_line_sector_name=CRCD(0x8dda17d6,"Sk5Ed_RADot_Dynamic");
	}
	else if (p_last_point->mpNext)
	{
		if (!s_angle_is_ok_to_grind(p_last_point->mpNext->mPos, p_last_point->mPos, pos))
		{
			dotted_line_sector_name=CRCD(0x8dda17d6,"Sk5Ed_RADot_Dynamic");
		}
	}		
	
	
	// Clone the sector if the dotted line does not exist or needs changing colour
	if (!mp_dotted_line_sector || dotted_line_sector_name != m_dotted_line_sector_name)
	{
		DeleteDottedLine();
		mp_dotted_line_sector=s_clone_sector(dotted_line_sector_name);
		m_dotted_line_sector_name=dotted_line_sector_name;
	}
	
	Mth::Vector dummy;
	s_calculate_rail_sector_vertex_coords(dummy, p_last_point->mPos, pos, 
										  mp_dotted_line_sector, m_dotted_line_sector_name,
										  NULL, false);

	if (dotted_line_sector_name==CRCD(0x8dda17d6,"Sk5Ed_RADot_Dynamic"))
	{
		int flash_rate=Script::GetInteger(CRCD(0xeebe4394,"RailEditorRedLineFlashRate"));
		uint32 vis=0xffffffff;
		if (s_highlight_flash_counter >= flash_rate/2)
		{
			vis=0;
		}
			
		++s_highlight_flash_counter;
		if (s_highlight_flash_counter >= flash_rate)
		{
			s_highlight_flash_counter=0;
		}	
		mp_dotted_line_sector->SetVisibility(vis);
	}
										  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the "Checksum" of a script command, then possibly handle it
// if it's a command that this component will handle	
CBaseComponent::EMemberFunctionResult CRailEditorComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		case 0x8f3e0b97: // GetCursorPosition
		{
			Mth::Vector cursor_pos(0.0f,0.0f,0.0f);		
			float cursor_height=0.0f;
			float cursor_angle=0.0f;
			get_pos_from_camera_component(&cursor_pos,&cursor_height,&cursor_angle);
			cursor_pos[Y]+=cursor_height;
			
			pScript->GetParams()->AddVector(CRCD(0x7f261953,"Pos"),cursor_pos[X],cursor_pos[Y],cursor_pos[Z]);
			pScript->GetParams()->AddFloat(CRCD(0xff7ebaf6,"Angle"),mp_editor_camera_component->GetCursorOrientation()*180.0f/3.141592654f);
			break;
		}
		
		// Returns false if a new rail could not be created, due to the limit on the number of rails 
		// being reached.
		case 0xd2c98192: // NewRail
		{
			if (GetNumFreePoints()==0)
			{
				return CBaseComponent::MF_FALSE;
			}
				
			NewRail();
			break;
		}

		case 0x31f66e6b: // MaxRailsReached
		{
			if (GetNumFreePoints()==0)
			{
				return CBaseComponent::MF_TRUE;
			}
			else
			{
				return CBaseComponent::MF_FALSE;
			}	
			break;
		}
			
		case 0x5828e852: // AddNewPosition
		{
			Mth::Vector pos;
			pos.Set();
			pParams->GetVector(CRCD(0x7f261953,"pos"),&pos);
			if (mp_current_rail)
			{
				CEditedRailPoint *p_last_point=mp_current_rail->mpRailPoints;
				if (p_last_point)
				{
					// Don't allow placement of points that are too close, or segments that are too steep
					if (!s_positions_OK(p_last_point->mPos, pos))
					{
						// But return true anyway, otherwise the script will make the cursor switch out
						// of rail-layout mode.
						return CBaseComponent::MF_TRUE;
					}
				}	

				// Don't allow points to be placed too close to the boundary of the park (TT5464)
				if (!Ed::IsWithinParkBoundaries(pos,PARK_BOUNDARY_MARGIN))
				{
					if (p_last_point)
					{
						// We must be in rail-layout mode, so do not add the point but return true
						// so that the cursor stays in rail layout mode.
						return CBaseComponent::MF_TRUE;
					}
					else	
					{
						return CBaseComponent::MF_FALSE;
					}	
				}
						
				Spt::SingletonPtr<Ed::CParkEditor> p_editor;
				if (p_editor->IsParkFull())
				{
					return CBaseComponent::MF_FALSE;
				}
		
				CEditedRailPoint *p_point=mp_current_rail->AddPoint();
				if (p_point)
				{
					p_point->mPos=pos;
					p_point->mHeightAboveGround=pos[Y]-p_point->FindGroundY();
					p_point->mHasPost=pParams->ContainsFlag(CRCD(0xcb4b4880,"AddPost"));
					p_point->UpdateRailGeometry();
					p_point->UpdatePostGeometry();
					return CBaseComponent::MF_TRUE;
				}
			}
			
			return CBaseComponent::MF_FALSE;
			break;
		}
			
		case 0x9c0762c4: // ClearRailEditor
		{
			Clear();
			break;
		}
		
		case 0x1cc3a173: // SetEditingMode
		{
			pParams->GetChecksum(CRCD(0x6835b854,"mode"),&m_mode);
			
			Script::CStruct *p_params=new Script::CStruct;
			p_params->AddChecksum(CRCD(0x6835b854,"mode"),CRCD(0xffd81c08,"rail_placement"));
			Script::RunScript(CRCD(0x9db065ad,"parked_set_helper_text_mode"),p_params);
			delete p_params;
			break;
		}
		
		case 0x63dd1204: // GetEditingMode
		{
			pScript->GetParams()->AddChecksum(CRCD(0x6835b854,"mode"),m_mode);
			break;
		}	
		
		case 0xaa438d71: // DrawDottedLine
		{
			Mth::Vector pos;
			pParams->GetVector(CRCD(0x7f261953,"pos"),&pos);
			DrawDottedLine(pos);
			break;
		}

		case 0x9386dcd9: // DeleteDottedLine
		{
			DeleteDottedLine();
			break;
		}
		
		case 0x6d2eee1b: // UpdateRailPointPosition
		{
			uint32 rail_id=0;
			pParams->GetChecksum(CRCD(0xa61e7cd9,"rail_id"),&rail_id);
			
			int rail_point_index=0;
			pParams->GetInteger(CRCD(0xab3c14,"rail_point_index"),&rail_point_index);
			
			Mth::Vector pos;
			pParams->GetVector(CRCD(0x7f261953,"Pos"),&pos);

			CEditedRail *p_rail=GetRail(rail_id);
			if (p_rail)
			{
				// UpdateRailPointPosition will always write in the new position, but returns
				// false if the position is bad in that it is too close to an adjacent point
				// or causes the rail to be too steep.
				// It still writes in the position so that the rail always stays attached to
				// the cursor as the user moves it around.
				if (!p_rail->UpdateRailPointPosition(rail_point_index, pos, 
													 (CEditedRail::EUpdateSuperSectors)pParams->ContainsFlag(CRCD(0xddf9a2bb,"UpdateSuperSectors"))))
				{
					return CBaseComponent::MF_FALSE;
				}	
			}	
			break;
		}

		case 0x8798e959: // HighlightRailPoint
		{
			uint32 rail_id=0;
			pParams->GetChecksum(CRCD(0xa61e7cd9,"rail_id"),&rail_id);
			
			int rail_point_index=0;
			pParams->GetInteger(CRCD(0xab3c14,"rail_point_index"),&rail_point_index);

			CEditedRail *p_rail=GetRail(rail_id);
			if (p_rail)
			{
				CEditedRailPoint *p_point=p_rail->GetRailPointFromIndex(rail_point_index);
				p_point->Highlight((CEditedRailPoint::EFlash)pParams->ContainsFlag(CRCD(0x5031a0fc,"Flash")), 
								   (CEditedRailPoint::EEndPosts)pParams->ContainsFlag(CRCD(0x1a52e4b8,"IncludeEndPosts")));
			}					   
			break;
		}

		case 0x6716227c: // DeleteRailPoint
		{
			uint32 rail_id=0;
			pParams->GetChecksum(CRCD(0xa61e7cd9,"rail_id"),&rail_id);
			CEditedRail *p_rail=GetRail(rail_id);
			if (!p_rail)
			{
				return CBaseComponent::MF_FALSE;
			}
			
			int rail_point_index=0;
			pParams->GetInteger(CRCD(0xab3c14,"rail_point_index"),&rail_point_index);
			CEditedRailPoint *p_point=p_rail->GetRailPointFromIndex(rail_point_index);
			if (!p_point)
			{
				return CBaseComponent::MF_FALSE;
			}
			
			DeleteRailPoint(p_rail, p_point);
			RemoveEmptyAndSinglePointRails();
			break;
		}
			
		case 0xd5a87eee: // UnHighlightAllRails
		{
			UnHighlightAllRails();
			break;
		}	

		case 0xfb6a0888: // GetEditedRailInfo
		{
			uint32 rail_id=0;
			pParams->GetChecksum(CRCD(0xa61e7cd9,"rail_id"),&rail_id);
			CEditedRail *p_rail=GetRail(rail_id);
			
			if (pParams->ContainsFlag(CRCD(0xe234f2b2,"CurrentRail")))
			{
				p_rail=mp_current_rail;
			}
			
			if (p_rail)
			{
				pScript->GetParams()->AddChecksum(CRCD(0xa61e7cd9,"rail_id"),p_rail->mId);
				pScript->GetParams()->AddInteger(CRCD(0x6164266e,"num_points"),p_rail->CountPoints());
			}	
			else
			{
				return CBaseComponent::MF_FALSE;
			}	
			break;
		}
			
		case 0x353ea2a9: // DeleteRail
		{
			uint32 rail_id=0;
			pParams->GetChecksum(CRCD(0xa61e7cd9,"rail_id"),&rail_id);
			CEditedRail *p_rail=GetRail(rail_id);
			if (p_rail)
			{
				DeleteRail(p_rail);
			}
			break;
		}

		case 0xd70781c3: // DestroyEditedRailSectors
		{
			CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors=true;
			if (pParams->ContainsFlag(CRCD(0x23a11cbc,"DoNotUpdateSuperSectors")))
			{
				CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors=false;
			}	
			DestroyRailGeometry();
			DestroyPostGeometry();
			CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors=true;
			break;
		}
		
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	// the "default" case of the switch statement handles
	// unrecognized functions;  if we make it down here,
	// that means that the component already handled it
	// somehow
    return CBaseComponent::MF_TRUE;
}

int CRailEditorComponent::GetNumFreePoints()
{
	return MAX_EDITED_RAIL_POINTS-CEditedRailPoint::SGetNumUsedItems();
}

int CRailEditorComponent::GetNumFreeRails()
{
	return MAX_EDITED_RAILS-CEditedRail::SGetNumUsedItems();
}

CEditedRail *CRailEditorComponent::get_rail_from_cluster_name(uint32 clusterChecksum)
{
	char p_rail_cluster_name[50];
	int rail_index=0;
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		sprintf(p_rail_cluster_name,"CreatedRailCluster%d",rail_index);
		if (clusterChecksum == Script::GenerateCRC(p_rail_cluster_name))
		{
			return p_rail;
		}	
			
		p_rail=p_rail->mpNext;
		++rail_index;
	}	
	return NULL;
}

// Used by graffiti games
void CRailEditorComponent::ModulateRailColor(uint32 clusterChecksum, int seqIndex)
{
	CEditedRail *p_rail=get_rail_from_cluster_name(clusterChecksum);
	if (p_rail)
	{
		p_rail->ModulateRailColor(seqIndex);
	}	
}

// Used by graffiti games
void CRailEditorComponent::ClearRailColor(uint32 clusterChecksum)
{
	CEditedRail *p_rail=get_rail_from_cluster_name(clusterChecksum);
	if (p_rail)
	{
		p_rail->ClearRailColor();
	}	
}

// Used for switching off collision on any created-rail sectors whilst the rail editor
// cursor is doing collision checks.
// This is because the rail sectors have a vertical collidable poly, which occasionally does cause
// collisions, making the cursor ping up into the air. Must be some sort of floating point innaccuracy
// (or too much accuracy), probably because the collision check vector is straight down the vertical poly.
void CRailEditorComponent::SetSectorActiveStatus(bool active)
{
	CEditedRail *p_rail=mp_edited_rails;
	while (p_rail)
	{
		p_rail->SetSectorActiveStatus(active);
		p_rail=p_rail->mpNext;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRailEditorComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to CRailEditorComponent::GetDebugInfo"));

	p_info->AddInteger(CRCD(0x8c75a6c1,"NumFreeRailPoints"),GetNumFreePoints());
	p_info->AddInteger(CRCD(0xe954b3d4,"NumFreeRails"),GetNumFreeRails());
	
	if (mp_current_rail)
	{
		Script::CStruct *p_struct=new Script::CStruct;
		mp_current_rail->GetDebugInfo(p_struct);
		p_info->AddStructurePointer(CRCD(0xe234f2b2,"CurrentRail"),p_struct);
	}
	else
	{
		p_info->AddChecksum(CRCD(0xe234f2b2,"CurrentRail"),CRCD(0xda3403b0,"NULL"));
	}
		
	int num_rails=count_rails();
	if (num_rails)
	{
		Script::CArray *p_array=new Script::CArray;
		p_array->SetSizeAndType(num_rails,ESYMBOLTYPE_STRUCTURE);
		
		int i=0;
		CEditedRail *p_rail=mp_edited_rails;
		while (p_rail)
		{
			Script::CStruct *p_struct=new Script::CStruct;
			p_rail->GetDebugInfo(p_struct);
			p_array->SetStructure(i,p_struct);
			++i;
			p_rail=p_rail->mpNext;
		}
		Dbg_MsgAssert(i==num_rails,("Eh ?"));
		
		p_info->AddArrayPointer(CRCD(0x7e17dfa9,"Rails"),p_array);
	}
		
	
	// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CRailEditorComponent *GetRailEditor()
{
	Obj::CCompositeObject *p_obj=(Obj::CCompositeObject*)Obj::CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0x5b509ad3,"RailEditor"));
	Dbg_MsgAssert(p_obj,("No RailEditor object"));
	Obj::CRailEditorComponent *p_rail_editor=GetRailEditorComponentFromObject(p_obj);
	Dbg_MsgAssert(p_rail_editor,("No rail editor component ???"));
	
	return p_rail_editor;
}

}


