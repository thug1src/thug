///////////////////////////////////////////////////////////////////////////////
// p_NxSector.h

#ifndef	__GFX_P_NX_SECTOR_H__
#define	__GFX_P_NX_SECTOR_H__

#include 	<core/math.h>

#include 	<gfx/nxsector.h>
#include 	<gfx/image/imagebasic.h>

namespace NxPs2
{
	class CGeomNode;
}

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CSector
// and we will also have a CXboxSector, CNgcSector, even a CPcSector
// maybe in the future we will have a CPS3Sector?
class	CPs2Sector : public CSector
{
public:
								CPs2Sector();
	virtual						~CPs2Sector();

	void						SetEngineObject(NxPs2::CGeomNode *p_object);
	NxPs2::CGeomNode *			GetEngineObject() const;

private:		// It's all private, as it is machine specific
	virtual void				plat_set_color(Image::RGBA rgba);
	virtual	Image::RGBA			plat_get_color() const;
	virtual void				plat_clear_color();

	virtual void				plat_set_visibility(uint32 mask);
	virtual	uint32				plat_get_visibility() const;

	virtual void				plat_set_active(bool on);	
	virtual	bool				plat_is_active() const;

	virtual const Mth::CBBox &	plat_get_bounding_box() const;

	virtual void				plat_set_world_position(const Mth::Vector& pos);
	virtual const Mth::Vector &	plat_get_world_position() const;

	virtual void				plat_set_y_rotation(Mth::ERot90 rot);

	virtual	void				plat_set_shatter(bool on);
	virtual	bool				plat_get_shatter() const;
	
	virtual CSector *			plat_clone(bool instance, CScene *p_dest_scene = NULL);

	// local functions
	void 						update_engine_matrix();

	NxPs2::CGeomNode *			mp_plat_object;

	Mth::Matrix					m_world_matrix;

	Image::RGBA					m_rgba;
	bool						m_active;
	bool						m_shatter;
};

/////////////////////////////////////////////////////////////////////////////////////
// Inlines
//

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CPs2Sector::SetEngineObject(NxPs2::CGeomNode *p_object)
{
	mp_plat_object = p_object;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline NxPs2::CGeomNode *	CPs2Sector::GetEngineObject() const
{
	return mp_plat_object;
}

} // Namespace Nx  			

#endif