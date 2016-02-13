/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SKATE3													**
**																			**
**	Module:			SSEC					 								**
**																			**
**	File name:		SuperSector.cpp											**
**																			**
**	Created by:		01/12/01	-	spg										**
**																			**
**	Description:	Functions concerning SuperSectors						**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>


#include <engine/SuperSector.h>					
#include <gel/collision/collision.h>
#include <gel/collision/colltridata.h>

#include <sys/timer.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/




namespace SSec
{





/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define	COLL_LINE_EXTENSION	0.5f

#define	vMAX_QUAL_SECTORS		1024	   	// never saw this go above 100

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

class WorldSectorNode : public Lst::Node< Nx::CSector >
{
public:
	WorldSectorNode( Nx::CSector* sector ) : Lst::Node< Nx::CSector > ( sector ) {}
};

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static	uint8	sOperationId = 0;	// operation id for sectors
//#ifdef	__PLAT_NGPS__
//const	Nx::CCollStatic**	QualCollSectors = (const Nx::CCollStatic**)0x70000000;	// use ScratchPad Ram (SPR) for speed
//#else
static	Nx::CCollStatic*	QualCollSectors[vMAX_QUAL_SECTORS];
//#endif

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

Sector::Sector( void )
{
	

	m_NumSectors = 0;
	m_NumCollSectors = 0;
	m_SectorList = NULL;
	m_CollSectorList = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Sector::~Sector( void )
{
	

	if( m_SectorList )
	{   
		delete [] m_SectorList;
	}
	if( m_CollSectorList )
	{   
		Mem::Free(m_CollSectorList);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Sector::remove_collision(Nx::CCollStatic *p_coll)
{
	for (int idx = 0; idx < m_NumCollSectors; idx++)
	{
		if (m_CollSectorList[idx] == p_coll)
		{
			// Shift the rest of the pointers over
			for (int copy_idx = idx + 1; copy_idx < m_NumCollSectors; copy_idx++)
			{
				m_CollSectorList[copy_idx - 1] = m_CollSectorList[copy_idx];
			}

			// And put a NULL on the last one (in case it isn't there, yet)
			m_CollSectorList[m_NumCollSectors - 1] = NULL;

			return true;
		}
	}

	// Never found it
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Sector::replace_collision(Nx::CCollStatic *p_coll, Nx::CCollStatic *p_replace_coll)
{
	for (int idx = 0; idx < m_NumCollSectors; idx++)
	{
		if (m_CollSectorList[idx] == p_coll)
		{
			m_CollSectorList[idx] = p_replace_coll;
			return true;
		}
	}
	
	// Never found it
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Sector::has_collision(Nx::CCollStatic *p_coll)
{
	for (int idx = 0; idx < m_NumCollSectors; idx++)
	{
		if (m_CollSectorList[idx] == p_coll)
		{
			return true;
		}
	}

	// Never found it
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8		GetCurrentSectorOperationId( void )
{
	

	return sOperationId;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8		GetNewSectorOperationId( void )
{   
	

	return ++sOperationId;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


void	Manager::GenerateSuperSectors(const Mth::CBBox& world_bbox )
{
	int i, j;
	float world_width, world_depth;
	float start_x, start_z;

	Dbg_Printf( "Generating Blank SuperSectors\n" );

	m_world_bbox = world_bbox;

	world_width = world_bbox.GetMax()[X] - world_bbox.GetMin()[X];
	world_depth = world_bbox.GetMax()[Z] - world_bbox.GetMin()[Z];

	m_sector_width = world_width / NUM_PARTITIONS_X;
	m_sector_depth = world_depth / NUM_PARTITIONS_Z;

	// For each super sector, find any world sectors which intersect the ss bounding box
	// and add that ws to ss's list of ws's
	start_x = world_bbox.GetMin()[X];
		
	Mth::Vector sec_min, sec_max;
	sec_min[W] = 0.0f;		 			// to avoid assertions
	sec_max[W] = 0.0f;
	for( i = 0; i < NUM_PARTITIONS_X; i++ )
	{
		start_z = world_bbox.GetMin()[Z];

		for( j = 0; j < NUM_PARTITIONS_Z; j++ )
		{
			sec_min[X] = start_x;
			sec_min[Y] = (float) -1000000000000.0f; //world_bbox.GetMin()[Y];
			sec_min[Z] = start_z;
            
			sec_max[X] = start_x + m_sector_width;
			sec_max[Y] = (float) 1000000000000.0f; //world_bbox.GetMax()[Y];
			sec_max[Z] = start_z + m_sector_depth;

			m_super_sector_list[i][j].m_Bbox.Set(sec_min, sec_max);
			
			start_z += m_sector_depth;
		}

		start_x += m_sector_width;
	}

	m_num_sectors_x = NUM_PARTITIONS_X;
	m_num_sectors_z = NUM_PARTITIONS_Z;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::AddCollisionToSuperSectors( Nx::CCollStatic *coll, int num_coll_sectors )
{
	Lst::Head< Nx::CCollStatic > sector_list;
	Lst::Node< Nx::CCollStatic > *sector, *next;

//	for (int i=0;i<100;i++)
//	{
//		printf ("%f,%f -> %f,%f\n",
//		coll[i].GetGeometry()->GetBBox().GetMin().GetX(),
//		coll[i].GetGeometry()->GetBBox().GetMin().GetZ(),
//		coll[i].GetGeometry()->GetBBox().GetMax().GetX(),
//		coll[i].GetGeometry()->GetBBox().GetMax().GetZ());
//	}


					
	for( int i = 0; i < NUM_PARTITIONS_X; i++ )
	{
		for( int j = 0; j < NUM_PARTITIONS_Z; j++ )
		{
			// Allocate temp list on high heap
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

			for ( int idx = 0; idx < num_coll_sectors; idx++ )
			{
				// Check to see if this collision sector belongs in the SuperSector
				Dbg_Assert(coll[idx].GetGeometry());
				if (coll[idx].GetGeometry()->GetNumFaces()>0 && coll[idx].GetGeometry()->GetBBox().Intersect(m_super_sector_list[i][j].m_Bbox))
				{
					Lst::Node< Nx::CCollStatic > *node = new Lst::Node< Nx::CCollStatic > ( &(coll[idx]) );
					sector_list.AddToTail( node );
				}
			}

			Mem::Manager::sHandle().PopContext();

			if( sector_list.CountItems() > 0 )
			{
				m_super_sector_list[i][j].m_CollSectorList = (Nx::CCollStatic **) Mem::Malloc(sizeof(Nx::CCollStatic *) * sector_list.CountItems());
				m_super_sector_list[i][j].m_NumCollSectors = sector_list.CountItems();
				int k = 0;
				for( sector = sector_list.GetNext(); sector; sector = next )
				{
					next = sector->GetNext();

					m_super_sector_list[i][j].m_CollSectorList[k] = sector->GetData();
					delete sector;
					k++;
				}
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::UpdateCollisionSuperSectors(Lst::Head<Nx::CCollStatic> &add_list,
											 Lst::Head<Nx::CCollStatic> &remove_list,
											 Lst::Head<Nx::CCollStatic> &update_list)
{
	Lst::Head< Nx::CCollStatic > sector_add_list;
	Lst::Node< Nx::CCollStatic > *sector, *next;
	Nx::CCollStatic *p_coll_sector;

	uint num_add_items = add_list.CountItems();
	uint num_remove_items = remove_list.CountItems();
	uint num_update_items = update_list.CountItems();

	//Dbg_Message("In UpdateCollisionSuperSectors with add size %d, remove size %d", num_add_items, num_remove_items);

	uint idx;
	int num_change;
	for( int i = 0; i < NUM_PARTITIONS_X; i++ )
	{
		for( int j = 0; j < NUM_PARTITIONS_Z; j++ )
		{
			SSec::Sector *p_super_sector = &(m_super_sector_list[i][j]);
			num_change = 0;

			// Allocate temp list on high heap
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

			// Check removed sectors first
			sector = remove_list.GetNext();
			for (idx = 0; idx < num_remove_items; idx++)
			{
				p_coll_sector = sector->GetData();

				// Check to see if this collision sector should be in the SuperSector
				if (p_coll_sector->GetGeometry()->GetBBox().Intersect(p_super_sector->m_Bbox))
				{
					//Dbg_Message("Removing collision %x to SuperSector (%d, %d)", p_coll_sector->GetChecksum(), i, j);
					num_change = remove_from_super_sector(p_coll_sector, p_super_sector, sector_add_list, num_change);
#if 0
				} else {
					if (p_super_sector->remove_collision(p_coll_sector))
					{
						num_change--;
						Dbg_MsgAssert(0, ("UpdateCollisionSuperSectors: Removed collision %x that shouldn't be in SuperSector (%d, %d)", 
										  p_coll_sector->GetChecksum(), i, j));
					}
#endif
				}

				sector = sector->GetNext();
			}

			// Now check added sectors
			sector = add_list.GetNext();
			for (idx = 0; idx < num_add_items; idx++)
			{
				p_coll_sector = sector->GetData();

				// Check to see if this collision sector belongs in the SuperSector
				if (p_coll_sector->GetGeometry()->GetBBox().Intersect(p_super_sector->m_Bbox))
				{
					//Dbg_Message("Adding collision %x to SuperSector (%d, %d)", p_coll_sector->GetChecksum(), i, j);
					num_change = add_to_super_sector(p_coll_sector, p_super_sector, sector_add_list, num_change);
				}

				sector = sector->GetNext();
			}
			// And check updated sectors
			sector = update_list.GetNext();
			for (idx = 0; idx < num_update_items; idx++)
			{
				p_coll_sector = sector->GetData();
				bool on_list = p_super_sector->has_collision(p_coll_sector);
				if (p_coll_sector->GetGeometry()->GetBBox().Intersect(p_super_sector->m_Bbox) != on_list)
				{
					if (on_list)
					{
						//Dbg_Message("Removing updated collision %x to SuperSector (%d, %d)", p_coll_sector->GetChecksum(), i, j);
						// Remove sector
						num_change = remove_from_super_sector(p_coll_sector, p_super_sector, sector_add_list, num_change);
					}
					else
					{
						//Dbg_Message("Adding updated collision %x to SuperSector (%d, %d)", p_coll_sector->GetChecksum(), i, j);
						// Add sector
						num_change = add_to_super_sector(p_coll_sector, p_super_sector, sector_add_list, num_change);
					}
				}

				sector = sector->GetNext();
			}

			Mem::Manager::sHandle().PopContext();

			int new_size = p_super_sector->m_NumCollSectors + num_change;

			if (num_change < 0)				// Re-alloc array down
			{
				if (!p_super_sector->m_CollSectorList ||
					!Mem::Manager::sHandle().ReallocateShrink(sizeof(Nx::CCollStatic *) * new_size, p_super_sector->m_CollSectorList))
				{
					// If we can't realloc down, then we have to use the old realloc
					p_super_sector->m_CollSectorList = (Nx::CCollStatic **) Mem::Realloc(p_super_sector->m_CollSectorList,
																						 sizeof(Nx::CCollStatic *) * new_size);
				}
			}
			else if (num_change > 0)		// Re-alloc array up and copy in sector_add_list
			{
				if (!p_super_sector->m_CollSectorList ||
				    !Mem::Manager::sHandle().ReallocateUp(sizeof(Nx::CCollStatic *) * new_size, p_super_sector->m_CollSectorList))
				{
					// If we can't realloc up, then we have to use the old realloc
					p_super_sector->m_CollSectorList = (Nx::CCollStatic **) Mem::Realloc(p_super_sector->m_CollSectorList,
																						 sizeof(Nx::CCollStatic *) * new_size);
				}

				// Copy add list in
				idx = p_super_sector->m_NumCollSectors;
				for(sector = sector_add_list.GetNext(); sector; sector = next)
				{
					next = sector->GetNext();

					p_super_sector->m_CollSectorList[idx] = sector->GetData();
					delete sector;
					idx++;
				}
				//Dbg_Message("Adding %d sectors to [%d, %d]. New size %d", num_change, i, j, new_size);
			}

			Dbg_Assert(sector_add_list.IsEmpty());

			p_super_sector->m_NumCollSectors = new_size;
		}
	}
	
	
	
	for( int i = 0; i < NUM_PARTITIONS_X; i++ )
	{
		for( int j = 0; j < NUM_PARTITIONS_Z; j++ )
		{
			SSec::Sector *p_super_sector = &(m_super_sector_list[i][j]);
			Nx::CCollStatic** pp_old = p_super_sector->m_CollSectorList;
			if (pp_old)
			{
				uint32 count = p_super_sector->m_NumCollSectors;
				if (count)
				{
					p_super_sector->m_CollSectorList = (Nx::CCollStatic**)Mem::Malloc(4*(count));
					memcpy(p_super_sector->m_CollSectorList, pp_old, 4*(count));
					Mem::Free(pp_old);
				}
				else
				{
					p_super_sector->m_CollSectorList = NULL;
					Mem::Free(pp_old);
				}
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::add_to_super_sector(Nx::CCollStatic *p_coll, SSec::Sector *p_super_sector, Lst::Head< Nx::CCollStatic > & add_list, int num_changes)
{
	if (num_changes < 0)
	{
		// Just append to end of existing array
		int avail_idx = p_super_sector->m_NumCollSectors + num_changes;
		p_super_sector->m_CollSectorList[avail_idx] = p_coll;
	} else {
		// Append to add list
		Lst::Node< Nx::CCollStatic > *node = new Lst::Node< Nx::CCollStatic > (p_coll);
		add_list.AddToTail(node);
	}

	return ++num_changes;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::remove_from_super_sector(Nx::CCollStatic *p_coll, SSec::Sector *p_super_sector, Lst::Head< Nx::CCollStatic > & add_list, int num_changes)
{
	if (num_changes > 0)
	{
		// Move a sector off the add list, if there is one
		Lst::Node< Nx::CCollStatic > *p_sector_node;

		p_sector_node = add_list.GetNext();
		if (!p_super_sector->replace_collision(p_coll, p_sector_node->GetData()))
		{
			Dbg_MsgAssert(0, ("UpdateCollisionSuperSectors: Can't replace collision %x that should be in SuperSector", 
							  p_coll->GetChecksum()));
		}
		delete p_sector_node;
	}
	else 
	{
		// Just remove the sector
		if (!p_super_sector->remove_collision(p_coll))
		{
			Dbg_MsgAssert(0, ("UpdateCollisionSuperSectors: Can't remove collision %x that should be in SuperSector", 
							  p_coll->GetChecksum()));
		}
	}

	return --num_changes;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::ClearCollisionSuperSectors()
{
	// Just delete all the arrays, since whoever called this knows they are all going away
	for( int i = 0; i < NUM_PARTITIONS_X; i++ )
	{
		for( int j = 0; j < NUM_PARTITIONS_Z; j++ )
		{
			SSec::Sector *p_super_sector = &(m_super_sector_list[i][j]);

			if( p_super_sector->m_CollSectorList )
			{   
				Mem::Free(p_super_sector->m_CollSectorList);
				p_super_sector->m_CollSectorList = NULL;
			}

			p_super_sector->m_NumCollSectors = 0;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::CCollStatic** Manager::GetIntersectingCollSectors( Mth::CBBox& bbox )
{
	int op_id = GetNewSectorOperationId();
	
	float x_min = bbox.GetMin()[X] - m_world_bbox.GetMin()[X];
	float x_max = bbox.GetMax()[X] - m_world_bbox.GetMin()[X];
	float z_min = bbox.GetMin()[Z] - m_world_bbox.GetMin()[Z];
	float z_max = bbox.GetMax()[Z] - m_world_bbox.GetMin()[Z];
	
	// extent the min and max in each direction to catch boundary conditions
	x_min -= COLL_LINE_EXTENSION;
	x_max += COLL_LINE_EXTENSION;
	z_min -= COLL_LINE_EXTENSION;
	z_max += COLL_LINE_EXTENSION;
	
	// determine the corresponding rectangle in terms of super sectors indicies
	int start_x_box = static_cast< int >(x_min / m_sector_width);
	int start_z_box = static_cast< int >(z_min / m_sector_depth);
	int end_x_box = static_cast< int >(x_max / m_sector_width);
	int end_z_box = static_cast< int >(z_max / m_sector_depth);
	
	// cap the super sector indicies
	if (start_x_box < 0)
	{
		start_x_box = 0;
	}
	else if (start_x_box >= m_num_sectors_x)
	{
		start_x_box = m_num_sectors_x - 1;
	}
	if (start_z_box < 0)
	{
		start_z_box = 0;
	}
	else if (start_z_box >= m_num_sectors_z)
	{
		start_z_box = m_num_sectors_z - 1;
	}
	if (end_x_box < 0)
	{
		end_x_box = 0;
	}
	else if (end_x_box >= m_num_sectors_x)
	{
		end_x_box = m_num_sectors_x - 1;
	}
	if (end_z_box < 0)
	{
		end_z_box = 0;
	}
	else if (end_z_box >= m_num_sectors_z)
	{
		end_z_box = m_num_sectors_z - 1;
	}
	
	// loop over the corresponding super sectors and add the sectors to the qualifying list
	int qual_sector_idx = 0;
	for (int i = start_x_box; i <= end_x_box; i++)
	{
		for (int j = start_z_box; j <= end_z_box; j++)
		{
			for (int k = 0; k < m_super_sector_list[i][j].m_NumCollSectors; k++)
			{
				Nx::CCollStatic* cs = m_super_sector_list[i][j].m_CollSectorList[k];
				
				if (cs->GetObjectFlags() & (mSD_NON_COLLIDABLE | mSD_KILLED)) continue;
				
				if (cs->GetSuperSectorID() == op_id) continue;
				
				QualCollSectors[qual_sector_idx++] = cs;
				
				cs->SetSuperSectorID(op_id);
				
				Dbg_MsgAssert(qual_sector_idx < (vMAX_QUAL_SECTORS*8/10),("Too many %d qualifying collision sectors",qual_sector_idx));						
			}
		}
	}
	
	QualCollSectors[qual_sector_idx] = NULL;
	
	return static_cast< Nx::CCollStatic** >(QualCollSectors);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::CCollStatic** Manager::GetIntersectingCollSectors( Mth::Line &line )
{
	
	
	int qual_sector_idx;
	Mth::Line test_line;
	Mth::Vector dir;
	float x_offset, z_offset;
	int start_x_box, start_z_box;
	int end_x_box, end_z_box;
	int temp,k;
	int op_id;
		
	qual_sector_idx = 0;


	// Extend the line a foot in each direction just to catch boundary conditions
	dir = Mth::Vector( line.m_end[X] - line.m_start[X],
					   line.m_end[Y] - line.m_start[Y],
					   line.m_end[Z] - line.m_start[Z] );
	dir.Normalize();
	dir.Scale( COLL_LINE_EXTENSION );
	test_line.m_start[X]	= line.m_start[X] - dir.GetX();
	test_line.m_start[Y]	= line.m_start[Y] - dir.GetY();
	test_line.m_start[Z]	= line.m_start[Z] - dir.GetZ();
	test_line.m_end[X]		= line.m_end[X] + dir.GetX();
	test_line.m_end[Y]		= line.m_end[Y] + dir.GetY();
	test_line.m_end[Z]		= line.m_end[Z] + dir.GetZ();

	// Figure out which super sector the start point is in
	x_offset = test_line.m_start[X] - m_world_bbox.GetMin()[X];
	z_offset = test_line.m_start[Z] - m_world_bbox.GetMin()[Z];

	start_x_box = (int) ( x_offset / m_sector_width );
	start_z_box = (int) ( z_offset / m_sector_depth );

//#define DISABLE_SUPER_SECTORS
#ifdef DISABLE_SUPER_SECTORS
	start_x_box = start_z_box = 0;
#endif // DISABLE_SUPER_SECTORS

	// Some sanity checks
	if( start_x_box < 0 )
	{
		start_x_box = 0;
	}
	else if( start_x_box >= m_num_sectors_x )
	{
		start_x_box = m_num_sectors_x - 1;
	}

	if( start_z_box < 0 )
	{
		start_z_box = 0;
	}
	else if( start_z_box >= m_num_sectors_z )
	{
		start_z_box = m_num_sectors_z - 1;
	}

	// Figure out which super sector the end point is in
	x_offset = test_line.m_end[X] - m_world_bbox.GetMin()[X];
	z_offset = test_line.m_end[Z] - m_world_bbox.GetMin()[Z];
		
	end_x_box = (int) ( x_offset / m_sector_width );
	end_z_box = (int) ( z_offset / m_sector_depth );

#ifdef DISABLE_SUPER_SECTORS
	end_x_box = m_num_sectors_x - 1;
	end_z_box = m_num_sectors_z - 1;
#endif // DISABLE_SUPER_SECTORS

	// Some sanity checks
	if( end_x_box < 0 )
	{
		end_x_box = 0;
	}
	else if( end_x_box >= m_num_sectors_x )
	{
		end_x_box = m_num_sectors_x - 1;
	}

	if( end_z_box < 0 )
	{
		end_z_box = 0;
	}
	else if( end_z_box >= m_num_sectors_z )
	{
		end_z_box = m_num_sectors_z - 1;
	}

	// Organize vars for two-dimensional "for" loop
	if( end_x_box < start_x_box )
	{
		temp = end_x_box;
		end_x_box = start_x_box;
		start_x_box = temp;
	}

	if( end_z_box < start_z_box )
	{
		temp = end_z_box;
		end_z_box = start_z_box;
		start_z_box = temp;
	}

	//Dbg_Message("SuperSector collision start(%d, %d) end(%d, %d)", start_x_box, start_z_box, end_x_box, end_z_box);
	//Dbg_Message("SuperSector testline start(%f, %f) end(%f, %f)", test_line.m_start[X], test_line.m_start[Z], test_line.m_end[X], test_line.m_end[Z]);
	//Dbg_Message("SuperSector world start(%f, %f) sector size(%f, %f)", m_world_bbox.inf.x, m_world_bbox.inf.z, m_sector_width, m_sector_depth);


// Optimize for the case (95%) where start and end are the same
// Just copy over the qualifying sectors pointers from one supersector
// This makes the routine about 30% fater on average.  
	if (start_x_box == end_x_box && start_z_box == end_z_box)
	{
		Sector * p_sec = &m_super_sector_list[start_x_box][start_z_box]; 
		int num = p_sec->m_NumCollSectors;
		Nx::CCollStatic** pp_cs = p_sec->m_CollSectorList; 
		Nx::CCollStatic**  pp_qual = QualCollSectors;		
//		printf ("%8d: %3d (%d,%d)\n",(int)Tmr::GetRenderFrame(),num,start_x_box, start_z_box);
		for( k = 0; k < num; k++ )
		{
			Nx::CCollStatic* cs= *pp_cs++;
			if ( !( cs->GetObjectFlags() & ( mSD_NON_COLLIDABLE | mSD_KILLED ) ) )
			{
				*pp_qual++ = cs;
			}

		}
		*pp_qual = NULL;
		return (Nx::CCollStatic**)QualCollSectors;
	}
	

//	int checked = 0;
//	int passed = 0;


	op_id = GetNewSectorOperationId();
		
	// Add world sectors from these super sectors to the qualifying list
	for( int i = start_x_box; i <= end_x_box; i++ )
	{
		for( int j = start_z_box; j <= end_z_box; j++ )
		{
			for( k = 0; k < m_super_sector_list[i][j].m_NumCollSectors; k++ )
			{
//				checked++;
				Nx::CCollStatic* cs;
				
				cs = m_super_sector_list[i][j].m_CollSectorList[k];
				
				if ( !( cs->GetObjectFlags() & ( mSD_NON_COLLIDABLE | mSD_KILLED ) ) )
				{
					// If we haven't already added it, add it
					if( cs->GetSuperSectorID() != op_id )
					{
						if (qual_sector_idx < vMAX_QUAL_SECTORS-1)
						{
//							passed++;
							QualCollSectors[qual_sector_idx++] = cs;
							// Mark it as having been added in this operation
							cs->SetSuperSectorID(op_id);
							
						}
						// normally we just return
						// but in case someone does something that involves the whole
						// world, we add this assertion...
						Dbg_MsgAssert(qual_sector_idx < (vMAX_QUAL_SECTORS*8/10),("Too many %d qualifying collision sectors (%d,%d)-(%d,%d).\n  Is a non-playable portion of the level flagged as collidable? Maybe the clouds, or the sea?",
						qual_sector_idx, start_x_box,start_z_box,end_x_box,end_z_box ));						
					}
				}
			}
		}
	}        
	
	//printf ("(%d,%d) to (%d, %d), checked %d, returning %d\n",start_x_box,start_z_box,end_x_box,end_z_box,checked, passed);                              

	QualCollSectors[qual_sector_idx] = NULL;
	return (Nx::CCollStatic**)QualCollSectors;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::CSector**	Manager::GetIntersectingWorldSectors( Mth::Line &line )
{


	Dbg_MsgAssert(0,("GetIntersectingWorldSectors is an old function"));	
	return NULL;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace SSec





