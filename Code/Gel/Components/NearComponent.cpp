#if 0

//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       NearComponent.cpp
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/19/03
//****************************************************************************

#include <gel/components/nearcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

namespace Obj
{
	
sSweepPruneArray CNearComponent::m_sweep_prune_array[3]	= { sSweepPruneArray( 0 ), sSweepPruneArray( 1 ), sSweepPruneArray( 2 ) };

	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CBaseComponent* CNearComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CNearComponent );	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNearComponent::CNearComponent() : CBaseComponent()
{
	SetType( CRC_NEAR );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNearComponent::~CNearComponent()
{
	// Remove from the axis arrays.
	for( int axis = 0; axis < 3; ++axis )
	{
		m_sweep_prune_array[axis].RemoveComponent( this );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNearComponent::InitFromStructure( Script::CStruct* pParams )
{
	// Not sure where bounding box dimensions will come from.
	m_untransformed_bbox.SetMin( Mth::Vector( -120.0f, -120.0f, -120.0f ));
	m_untransformed_bbox.SetMax( Mth::Vector(  120.0f,  120.0f,  120.0f ));

	// Obtain the position of the parent object, and build the transformed bounding box.
	if( GetObject())
	{
		Mth::Vector t;

		t = m_untransformed_bbox.GetMin() + GetObject()->GetPos();
		m_transformed_bbox.SetMin( t );

		t = m_untransformed_bbox.GetMax() + GetObject()->GetPos();
		m_transformed_bbox.SetMax( t );
	}

	// Now add this component to the axis arrays.
	for( int axis = 0; axis < 3; ++axis )
	{
		m_sweep_prune_array[axis].InsertComponent( this );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNearComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Remove from axis arrays, then reinitialise.
	for( int axis = 0; axis < 3; ++axis )
	{
		m_sweep_prune_array[axis].RemoveComponent( this );
	}

	InitFromStructure( pParams );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNearComponent::Update()
{
	// Re-calculate the bounding box position.
	if( GetObject())
	{
		Mth::Vector t;

		t = m_untransformed_bbox.GetMin() + GetObject()->GetPos();
		m_transformed_bbox.SetMin( t );

		t = m_untransformed_bbox.GetMax() + GetObject()->GetPos();
		m_transformed_bbox.SetMax( t );

		// Resort the component.
		for( int axis = 0; axis < 3; ++axis )
		{
			m_sweep_prune_array[axis].ResortComponent( this );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CBaseComponent::EMemberFunctionResult CNearComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
/*
        // @script | DoSomething | does some functionality
		case 0xbb4ad101:		// DoSomething
			DoSomething();
			break;

        // @script | ValueIsTrue | returns a boolean value
		case 0x769260f7:		// ValueIsTrue
		{
			return ValueIsTrue() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		break;
*/

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	// the "default" case of the switch statement handles
	// unrecognized functions;  if we make it down here,
	// that means that the component already handled it
	// somehow
    return CBaseComponent::MF_TRUE;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNearComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert( p_info, ( "NULL p_info sent to CNearComponent::GetDebugInfo" ));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums

	/*	Example:
	p_info->AddInteger("m_never_suspend",m_never_suspend);
	p_info->AddFloat("m_suspend_distance",m_suspend_distance);
	*/
	
	// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}



#define MAX_COMPONENTS_PER_WORKSPACE_ARRAY		64
static int				perAxisIntersections[3];
static CNearComponent*	perAxisWorkspace[3][MAX_COMPONENTS_PER_WORKSPACE_ARRAY];
static CNearComponent*	combinedAxisWorkspace[MAX_COMPONENTS_PER_WORKSPACE_ARRAY];




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int sSweepPruneArray::BinarySearchFirstBigger( float value )
{
	int first_bigger	= -1;
	int l				= 0;
	int u				= m_num_entries - 1;
	while( l <= u )
	{
		int m = ( l + u ) / 2;
		if( m_array[m].GetValue( m_axis ) <= value )
		{
			// x[m] <= t
			l = m + 1;
		}
		else
		{
			// x[m] > t
			first_bigger = m;
			u = m - 1;
		}
	}
	return first_bigger;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int sSweepPruneArray::BinarySearchLastSmaller( float value )
{
	int last_smaller	= -1;
	int l				= 0;
	int u				= m_num_entries - 1;
	while( l <= u )
	{
		int m = ( l + u ) / 2;
		if( m_array[m].GetValue( m_axis ) <= value )
		{
			// x[m] <= t
			last_smaller = m;
			l = m + 1;
		}
		else
		{
			// x[m] > t
			u = m - 1;
		}
	}
	return last_smaller;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int sSweepPruneArray::BuildCompositeIntersectingArray( void )
{
	// Now build the composite array of intersections that appear in all three axis lists.
	int final_intersections	= 0;
	int source, check1, check2;

	// Use the smallest list to check against.
	if(( perAxisIntersections[0] <= perAxisIntersections[1] ) && ( perAxisIntersections[0] <= perAxisIntersections[2] ))
	{
		source	= 0;
		check1	= 1;
		check2	= 2;
	}
	else if(( perAxisIntersections[1] <= perAxisIntersections[0] ) && ( perAxisIntersections[1] <= perAxisIntersections[2] ))
	{
		source	= 1;
		check1	= 0;
		check2	= 2;
	}
	else
	{
		source	= 2;
		check1	= 0;
		check2	= 1;
	}

	for( int i = 0; i < perAxisIntersections[source]; ++i )
	{
		bool found;
		
		// Is this intersection also in the first check list?
		found = false;
		for( int j = 0; j < perAxisIntersections[check1]; ++j )
		{
			if( perAxisWorkspace[source][i] == perAxisWorkspace[check1][j] )
			{
				found = true;
				break;
			}
		}

		// If it wasn't in the first check list, there's no intersection.
		if( !found )
		{
			break;
		}

		// Is this intersection also in the second check list?
		found = false;
		for( int j = 0; j < perAxisIntersections[check2]; ++j )
		{
			if( perAxisWorkspace[source][i] == perAxisWorkspace[check2][j] )
			{
				found = true;
				break;
			}
		}

		// If it wasn't in the second check list, there's no intersection.
		if( !found )
		{
			break;
		}

		// At this stage the intersection appeared in all three axis lists, so it is a valid intersection.
		// As a final check, make sure it isn't already in the final list.
		found = false;
		for( int j = 0; j < final_intersections; ++j )
		{
			if( combinedAxisWorkspace[j] == perAxisWorkspace[source][i] )
			{
				found = true;
				break;
			}
		}

		if( !found )
		{
			combinedAxisWorkspace[final_intersections] = perAxisWorkspace[source][i];
			++final_intersections;
		}
	}
	return final_intersections;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNearComponent** CNearComponent::GetIntersectingNearComponents( Mth::Vector &bbmin, Mth::Vector &bbmax, int *p_num_intersecting )
{
	// Zero axis intersection count array.
	perAxisIntersections[0] = perAxisIntersections[1] = perAxisIntersections[2] = 0;

	for( int axis = 0; axis < 3; ++axis )
	{
		// Find the start index.
		int start_index = m_sweep_prune_array[axis].BinarySearchFirstBigger( bbmin[axis] );
		if( start_index == -1 )
		{
			// Nothing was found, therefore there can be no intersections.
			*p_num_intersecting = 0;
			return NULL;
		}

		// Find the end index.
		int end_index = m_sweep_prune_array[axis].BinarySearchLastSmaller( bbmax[axis] );
		if( end_index == -1 )
		{
			// Nothing was found, therefore there can be no intersections.
			*p_num_intersecting = 0;
			return NULL;
		}

		Dbg_Assert( end_index > start_index );

		// Build up the workspace array for this axis.
		int intersections_for_this_axis = 0;
		for( int i = start_index; i <= end_index; ++i )
		{
			// Don't want to include the calling component in the list.
			if( m_sweep_prune_array[axis].m_array[i].mp_component != this )
			{
				perAxisWorkspace[axis][intersections_for_this_axis++] = m_sweep_prune_array[axis].m_array[i].mp_component;
			}
		}

		// If we found no intersections for this axis, there can be no components that intersect.
		if( intersections_for_this_axis == 0 )
		{
			*p_num_intersecting = 0;
			return NULL;
		}

		// Store off the number of intersections for this axis.
		perAxisIntersections[axis] = intersections_for_this_axis;

	}

	// Build the composite array.
	int final_intersections = sSweepPruneArray::BuildCompositeIntersectingArray();

	*p_num_intersecting = final_intersections;
	return ( final_intersections > 0 ) ? &combinedAxisWorkspace[0] : NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNearComponent** CNearComponent::GetIntersectingNearComponents( int *p_num_intersecting )
{
	// Zero axis intersection count array.
	perAxisIntersections[0] = perAxisIntersections[1] = perAxisIntersections[2] = 0;

	// Build up each workspace array.
	for( int axis = 0; axis < 3; ++axis )
	{
		int i;
		for( i = 0; i < m_sweep_prune_array[axis].m_num_entries; ++i )
		{
			if( m_sweep_prune_array[axis].m_array[i].mp_component == this )
			{
				// We have found the start of the span for this component.
				Dbg_Assert( m_sweep_prune_array[axis].m_array[i].m_min_max == 0 );
				++i;
				break;
			}
		}

		// Now add any other components we come across before we hit the end of the span.
		while(( i < m_sweep_prune_array[axis].m_num_entries ) && ( m_sweep_prune_array[axis].m_array[i].mp_component != this ))
		{
			perAxisWorkspace[axis][perAxisIntersections[axis]] = m_sweep_prune_array[axis].m_array[i].mp_component;
			++perAxisIntersections[axis];
			++i;
		}

		// If we found no intersections for this axis, there can be no components that intersect.
		if( perAxisIntersections[axis] == 0 )
		{
			*p_num_intersecting = 0;
			return NULL;
		}
	}

	// Build the composite array.
	int final_intersections = sSweepPruneArray::BuildCompositeIntersectingArray();

	*p_num_intersecting = final_intersections;
	return ( final_intersections > 0 ) ? &combinedAxisWorkspace[0] : NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sSweepPruneEntry::SetValues( void )
{
	Mth::Vector v	= ( m_min_max ) ? mp_component->GetMax() : mp_component->GetMin();
	m_value[0]		= v[0];
	m_value[1]		= v[1];
	m_value[2]		= v[2];
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sSweepPruneArray::sSweepPruneArray( int axis )
{
	m_axis			= axis;
	m_num_entries	= 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sSweepPruneArray::~sSweepPruneArray( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sSweepPruneArray::PushUp( int index )
{
	Dbg_MsgAssert( m_num_entries < MAX_SWEEP_PRUNE_ENTRIES_PER_AXIS, ( "CNearComponent::SweepPruneList overflow" ));

	// All array entries from index (inclusive) upwards are moved up one position.
	for( int i = m_num_entries - 1; i >= index; --i )
	{
		m_array[i + 1] = m_array[i];
	}
	++m_num_entries;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sSweepPruneArray::PullDown( int index )
{
	Dbg_MsgAssert( m_num_entries > 0, ( "CNearComponent::SweepPruneList underflow" ));

	// All array entries from index (inclusive) upwards are moved down one position.
	for( int i = index; i < ( m_num_entries - 1 ); ++i )
	{
		m_array[i] = m_array[i + 1];
	}
	--m_num_entries;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sSweepPruneArray::ResortComponent( CNearComponent *p_component )
{
	// Does a simple check to see whether the component is still in the correct order.
	// If it isn't, sorting is applied.

	// Check min first.
	int i;
	for( i = 0; i < m_num_entries; ++i )
	{
		if( m_array[i].mp_component == p_component )
		{
			// This should be the min entry.
			Dbg_Assert( m_array[i].m_min_max == 0 );

			// Set the new min value.
			m_array[i].SetValues();

			// Check this is bigger than or equal to the previous value, if there is one.
			while(( i > 0 ) && ( m_array[i].GetValue( m_axis ) < m_array[i - 1].GetValue( m_axis )))
			{
				// Move this entry down in the array.
				sSweepPruneEntry swap	= m_array[i];
				m_array[i]				= m_array[i - 1];
				m_array[i - 1]			= swap;
				--i;
			}

			// Check this is smaller than or equal to the next value, if there is one.
			while(( i < ( m_num_entries - 1)) && ( m_array[i].GetValue( m_axis ) > m_array[i + 1].GetValue( m_axis )))
			{
				// Move this entry up in the array.
				sSweepPruneEntry swap	= m_array[i];
				m_array[i]				= m_array[i + 1];
				m_array[i + 1]			= swap;
				++i;
			}
			break;
		}
	}
	
	// Now check max.
	++i;
	for( ; i < m_num_entries; ++i )
	{
		if( m_array[i].mp_component == p_component )
		{
			// This should be the max entry.
			Dbg_Assert( m_array[i].m_min_max == 1 );

			// Set the new max value.
			m_array[i].SetValues();

			// Check this is bigger than or equal to the previous value, if there is one.
			while(( i > 0 ) && ( m_array[i].GetValue( m_axis ) < m_array[i - 1].GetValue( m_axis )))
			{
				// Move this entry down in the array.
				sSweepPruneEntry swap	= m_array[i];
				m_array[i]				= m_array[i - 1];
				m_array[i - 1]			= swap;
				--i;
			}

			// Check this is smaller than or equal to the next value, if there is one.
			while(( i < ( m_num_entries - 1)) && ( m_array[i].GetValue( m_axis ) > m_array[i + 1].GetValue( m_axis )))
			{
				// Move this entry up in the array.
				sSweepPruneEntry swap	= m_array[i];
				m_array[i]				= m_array[i + 1];
				m_array[i + 1]			= swap;
				++i;
			}
			break;
		}
	}
}

	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sSweepPruneArray::InsertComponent( CNearComponent *p_component )
{
	float axis_min = p_component->GetMin()[m_axis];
	float axis_max = p_component->GetMax()[m_axis];

	// Scan through until we find an entry that is bigger than the minimum for this axis, or we come to the end of the list.
	for( int i = 0; i <= m_num_entries; ++i )
	{
		if(( i == m_num_entries ) || ( m_array[i].mp_component->GetMin()[m_axis] > axis_min ))
		{
			// Insert this entry here.
			PushUp( i );
			m_array[i].m_min_max	= 0;
			m_array[i].mp_component = p_component;

			// Set the new min value.
			m_array[i].SetValues();
			break;
		}
	}

	// Now scan through until we find an entry that is bigger than the maximum for this axis, or we come to the end of the list.
	for( int i = 0; i <= m_num_entries; ++i )
	{
		if(( i == m_num_entries ) || ( m_array[i].mp_component->GetMax()[m_axis] > axis_max ))
		{
			// Insert this entry here.
			PushUp( i );
			m_array[i].m_min_max	= 1;
			m_array[i].mp_component = p_component;

			// Set the new min value.
			m_array[i].SetValues();
			break;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sSweepPruneArray::RemoveComponent( CNearComponent *p_component )
{
	// The component should appear twice in the list.
	for( int p = 0; p < 2; ++p )
	{
		// Scan through until we find this entry.
		for( int i = 0; i < m_num_entries; ++i )
		{
			if( m_array[i].mp_component == p_component )
			{
				// Remove this entry.
				PullDown( i );
				break;
			}
		}
	}
}















}


#endif
