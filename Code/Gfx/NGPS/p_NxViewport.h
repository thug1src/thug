///////////////////////////////////////////////////////////////////////////////
// p_NxViewport.h

#ifndef	__GFX_P_NX_VIEWPORT_H__
#define	__GFX_P_NX_VIEWPORT_H__

#include 	"Gfx/NxViewport.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Machine specific implementation of the CViewport
class	CPs2Viewport : public CViewport
{
public:
								CPs2Viewport();
								CPs2Viewport(const Mth::Rect* rect, Gfx::Camera* cam = NULL);
	virtual						~CPs2Viewport();

protected:
	void						update_render_vars();

private:		// It's all private, as it is machine specific
	virtual float				plat_transform_to_screen_coord(const Mth::Vector & world_pos, float & screen_pos_x, float & screen_pos_y, ZBufferValue & screen_pos_z);

};

} // Namespace Nx  			

#endif
