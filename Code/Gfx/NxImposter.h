///////////////////////////////////////////////////////////////////////////////////
// NxImposter.H - Neversoft Engine, Rendering portion, Platform independent interface

#ifndef	__GFX_NX_IMPOSTER_H__
#define	__GFX_NX_IMPOSTER_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/hashtable.h>
#include <core/math/geometry.h>
#include <gfx/NxGeom.h>

namespace Nx
{
/////////////////////////////////////////////////////////////////////////
// Forward declarations


	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
class CImposterGroup
{
	public:
								CImposterGroup( void );
	virtual						~CImposterGroup();

	void						AddGeom( CGeom *p_geom );
	void						RemoveGeom( CGeom *p_geom );
								
	Mth::CBBox &				GetCompositeBoundingBox( void )					{ return m_composite_bbox; }
	void						SetCompositeBoundingBox( const Mth::CBBox & bbox );
	void						AddBoundingBox( const Mth::CBBox & bbox );
	float						GetSwitchDistance( void )						{ return m_switch_distance; }
	float						CheckDistance( void );
	void						Process( void );			// Called once per frame for platform specific processing.

	void						CreateImposterPolygon( void );
	void						RemoveImposterPolygon( void );
	bool						UpdateImposterPolygon( void );
	void						DrawImposterPolygon( void );

	bool						ImposterPolygonExists( void )					{ return m_imposter_polygon_exists; }

	protected:

	float						m_switch_distance;			// Distance at which the imposter switches in/out.
	Mth::CBBox					m_composite_bbox;			// Composite bounding box from all geometey added to the group.
	Mth::Vector					m_composite_bbox_mid;		// Mid point of composite bounding box (for speed).
	float						m_composite_bsphere_radius;	// Composite bounding sphere radius (mid point is same as box).
	Lst::Head <Nx::CGeom> *		mp_geom_list;				// List of geometry for this imposter group.
	Mth::Vector					m_cam_pos;					// The camera position at the time of creation.
	int							m_tex_width, m_tex_height;	// Current imposter texture width and height.
	int							m_update_count;				// Incremented each update, used to allow intermittent checks.
	bool						m_imposter_polygon_exists;

	private:

	// The virtual functions have a stub implementation.
	virtual void				plat_create_imposter_polygon( void );
	virtual void				plat_remove_imposter_polygon( void );
	virtual bool				plat_update_imposter_polygon( void );
	virtual void				plat_draw_imposter_polygon( void );
	virtual float				plat_check_distance( void );
	virtual void				plat_process( void );
};
	
	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
class CImposterManager
{
public:
										CImposterManager( void );
										~CImposterManager();

	void								Cleanup( void );
	void								AddGeomToImposter( uint32 group_checksum, Nx::CGeom *p_geom );
	void								RemoveGeomFromImposter( uint32 group_checksum, Nx::CGeom *p_geom );
	void								ProcessImposters( void );
	void								DrawImposters( void );

	CImposterGroup*						plat_create_imposter_group( void );
	void								plat_pre_render_imposters( void );
	void								plat_post_render_imposters( void );

protected:

private:

	Lst::HashTable< CImposterGroup >	*mp_group_table;
	int									m_max_imposters_to_redraw_per_frame;


};


///////////////////////////////////////////////////////////////////////
//
// Inline functions
//
}
#endif