///////////////////////////////////////////////////////////////////////////////
// p_NxScene.h

#ifndef	__GFX_P_NX_SPRITE_H__
#define	__GFX_P_NX_SPRITE_H__

#include 	"Gfx/NxSprite.h"
#include 	"Gfx/NGPS/NX/texture.h"
#include 	"Gfx/NGPS/NX/sprite.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Machine specific implementation of the CSprite
class	CPs2Sprite : public CSprite
{
public:
								CPs2Sprite(CWindow2D *p_window = NULL);
	virtual						~CPs2Sprite();

private:		// It's all private, as it is machine specific
	virtual void				plat_initialize();

	virtual void				plat_update_hidden();		// Tell engine of update
	virtual void				plat_update_engine();		// Update engine primitives
	virtual void				plat_update_priority();
	virtual void				plat_update_window();

	NxPs2::SSprite *			mp_plat_sprite;
};

} // Namespace Nx  			

#endif
