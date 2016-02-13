/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:																**
**																			**
**	Module:			Nx Imposter												**
**																			**
**	File name:		gfx/NxImposter.cpp										**
**																			**
**	Created by:		03/13/03	-	dc										**
**																			**
**	Description:	Imposter and Imposter Management code					**
**																			**
*****************************************************************************/

// start autoduck documentation
// @DOC nximposter
// @module nximposter | None
// @subindex Scripting Database
// @index script | nximposter

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <gfx/NxImposter.h>

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CImposterManager::CImposterManager( void )
{
	mp_group_table						= new Lst::HashTable< CImposterGroup >( 8 );
	m_max_imposters_to_redraw_per_frame	= 1;
}

	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CImposterManager::~CImposterManager( void )
{
	delete mp_group_table;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterManager::Cleanup( void )
{
	// Goes through and removes all imposter groups.
	mp_group_table->IterateStart();
	CImposterGroup *p_imposter_group = mp_group_table->IterateNext();
	while( p_imposter_group )
	{
		CImposterGroup *p_next	= mp_group_table->IterateNext();
		delete p_imposter_group;
		p_imposter_group		= p_next;
	}
	mp_group_table->FlushAllItems();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterManager::AddGeomToImposter( uint32 group_checksum, Nx::CGeom *p_geom )
{
	// See if this imposter group exists already, create it if not.
	CImposterGroup *p_group = mp_group_table->GetItem( group_checksum );
	if( p_group == NULL )
	{
		p_group = plat_create_imposter_group();
		mp_group_table->PutItem( group_checksum, p_group );

		// Set the bounding box to be that of the 'founding' CGeom.
		p_group->SetCompositeBoundingBox( p_geom->GetBoundingBox());
	}
	else
	{
		// Add the bounding box details of this CGeom.
		p_group->AddBoundingBox( p_geom->GetBoundingBox());
	}

	// Add the specified CGeom to the CImposterGroup's list.
	p_group->AddGeom( p_geom );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterManager::RemoveGeomFromImposter( uint32 group_checksum, Nx::CGeom *p_geom )
{
	CImposterGroup *p_group = mp_group_table->GetItem( group_checksum );
	if( p_group )
	{
		p_group->RemoveGeom( p_geom );
	}
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterManager::ProcessImposters( void )
{
	int imposters_drawn = 0;

	mp_group_table->IterateStart();
	CImposterGroup *p_imposter_group = mp_group_table->IterateNext();
	while( p_imposter_group )
	{
		// Once per frame process function, for platform-specific tasks.
		p_imposter_group->Process();

		// Determine whether this imposter is no longer visible, or is outside the switch distance.
		float d = p_imposter_group->CheckDistance();

		if( d <= p_imposter_group->GetSwitchDistance())
		{
			// This imposter is no longer outside the switch distance, or is not visible. Remove the imposter polygon if it exists.
			p_imposter_group->RemoveImposterPolygon();
		}
		else
		{
			// This imposter is outside the switch distance. Create the imposter polygon if it doesn't exist...
			if( p_imposter_group->ImposterPolygonExists() == false )
			{
				if( imposters_drawn < m_max_imposters_to_redraw_per_frame )
				{
					p_imposter_group->CreateImposterPolygon();
					++imposters_drawn;
				}
			}
			else
			{
				// ...Otherwise determine whether the imposter polygon requires updating.
				if( imposters_drawn < m_max_imposters_to_redraw_per_frame )
				{
					bool drawn = p_imposter_group->UpdateImposterPolygon();
					if( drawn )
						++imposters_drawn;
				}
			}
		}
		p_imposter_group = mp_group_table->IterateNext();
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterManager::DrawImposters( void )
{
	plat_pre_render_imposters();

	mp_group_table->IterateStart();
	CImposterGroup *p_imposter_group = mp_group_table->IterateNext();
	while( p_imposter_group )
	{
		if( p_imposter_group->ImposterPolygonExists())
		{
			p_imposter_group->DrawImposterPolygon();
		}
		p_imposter_group = mp_group_table->IterateNext();
	}

	plat_post_render_imposters();
}


















/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CImposterGroup::CImposterGroup( void )
{
	m_switch_distance			= 960.0f;
	mp_geom_list				= new Lst::Head < Nx::CGeom >;
	m_imposter_polygon_exists	= false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CImposterGroup::~CImposterGroup( void )
{
	// Remove all nodes from the table.
	Lst::Node<Nx::CGeom> *p_node, *p_next;
	for( p_node = mp_geom_list->GetNext(); p_node; p_node = p_next )
	{
		p_next = p_node->GetNext();
		delete p_node;
	}

	// Remove the table itself.
	delete mp_geom_list;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::AddGeom( CGeom *p_geom )
{
	// Create a new node.
	Lst::Node<Nx::CGeom> *p_new_node = new Lst::Node<Nx::CGeom>( p_geom );

	// Link in the new node.
	mp_geom_list->AddToTail( p_new_node );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::RemoveGeom( CGeom *p_geom )
{
	Lst::Node<Nx::CGeom> *p_node, *p_next;
	for( p_node = mp_geom_list->GetNext(); p_node; p_node = p_next )
	{
		p_next = p_node->GetNext();

		if( p_node->GetData() == p_geom )
		{
			delete p_node;
			break;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::SetCompositeBoundingBox( const Mth::CBBox & bbox )
{
	m_composite_bbox			= bbox;
	m_composite_bbox_mid		= m_composite_bbox.GetMin() + ( 0.5f * ( m_composite_bbox.GetMax() - m_composite_bbox.GetMin()));
	m_composite_bsphere_radius	= ( m_composite_bbox.GetMax() - m_composite_bbox_mid ).Length();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::AddBoundingBox( const Mth::CBBox & bbox )
{
	m_composite_bbox.AddPoint( bbox.GetMin());
	m_composite_bbox.AddPoint( bbox.GetMax());

	m_composite_bbox_mid		= m_composite_bbox.GetMin() + ( 0.5f * ( m_composite_bbox.GetMax() - m_composite_bbox.GetMin()));
	m_composite_bsphere_radius	= ( m_composite_bbox.GetMax() - m_composite_bbox_mid ).Length();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::Process( void )
{
	return plat_process();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float CImposterGroup::CheckDistance( void )
{
	return plat_check_distance();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::CreateImposterPolygon( void )
{
	plat_create_imposter_polygon();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::RemoveImposterPolygon( void )
{
	plat_remove_imposter_polygon();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CImposterGroup::UpdateImposterPolygon( void )
{
	++m_update_count;
	return plat_update_imposter_polygon();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::DrawImposterPolygon( void )
{
	plat_draw_imposter_polygon();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::plat_create_imposter_polygon( void )
{
	printf( "STUB: plat_create_imposter_polygon\n");
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::plat_remove_imposter_polygon( void )
{
	printf( "STUB: plat_remove_imposter_polygon\n");
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CImposterGroup::plat_update_imposter_polygon( void )
{
	printf( "STUB: plat_update_imposter_polygon\n");
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::plat_draw_imposter_polygon( void )
{
	printf( "STUB: plat_draw_imposter_polygon\n");
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float CImposterGroup::plat_check_distance( void )
{
	printf( "STUB: plat_check_distance\n");
	return 0.0f;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterGroup::plat_process( void )
{
	printf( "STUB: plat_process\n");
}

}

