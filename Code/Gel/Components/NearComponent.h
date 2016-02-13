#if 0
//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       NearComponent.h
//* OWNER:          ???
//* CREATION DATE:  ??/??/??
//****************************************************************************

#ifndef __COMPONENTS_NEARCOMPONENT_H__
#define __COMPONENTS_NEARCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/math.h>
#include <core/math/geometry.h>

#include <gel/object/basecomponent.h>

// Replace this with the CRCD of the component you are adding
#define		CRC_NEAR							CRCD( 0x89b3e3ee, "near" )

//  Standard accessor macros for getting the component either from within an object, or given an object.
#define		GetNearComponent()					((Obj::CNearComponent*)GetComponent(CRC_NEAR))
#define		GetNearComponentFromObject( pObj )	((Obj::CNearComponent*)(pObj)->GetComponent(CRC_NEAR))

#define		MAX_SWEEP_PRUNE_ENTRIES_PER_AXIS	512

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	
class CNearComponent;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sSweepPruneEntry
{
	uint32				m_min_max;			// 0 for min, 1 for max
	float				m_value[3];			// This proves much quicker than dereferencing the component and reading the bounding box.
	CNearComponent		*mp_component;		// Pointer to the actual component.

	inline float		GetValue( int axis )	{ return m_value[axis]; }
	void				SetValues( void );
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sSweepPruneArray
{
public:
	int					m_axis;
	int					m_num_entries;
	sSweepPruneEntry	m_array[MAX_SWEEP_PRUNE_ENTRIES_PER_AXIS];

						sSweepPruneArray( int axis );
						~sSweepPruneArray( void );

	int					BinarySearchFirstBigger( float value );
	int					BinarySearchLastSmaller( float value );
	void				ResortComponent( CNearComponent *p_component );
	void				InsertComponent( CNearComponent *p_component );
	void				RemoveComponent( CNearComponent *p_component );
	static int			BuildCompositeIntersectingArray( void );

private:
	void				PushUp( int index );
	void				PullDown( int index );
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
class CNearComponent : public CBaseComponent
{
public:
    CNearComponent();
    virtual ~CNearComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	Mth::Vector						GetMin( void )	{ return m_transformed_bbox.GetMin(); }
	Mth::Vector						GetMax( void )	{ return m_transformed_bbox.GetMax(); }
	CNearComponent**				GetIntersectingNearComponents( int *p_num_intersecting );
	CNearComponent**				GetIntersectingNearComponents( Mth::Vector &bbmin, Mth::Vector &bbmax, int *p_num_intersecting );

	static CBaseComponent*			s_create();

private:

	Mth::CBBox						m_untransformed_bbox;
	Mth::CBBox						m_transformed_bbox;
	static sSweepPruneArray			m_sweep_prune_array[3];
};



}

#endif

#endif
