///////////////////////////////////////////////////////////////////////////////
// p_NxScene.h

#ifndef	__GFX_P_NX_SPRITE_H__
#define	__GFX_P_NX_SPRITE_H__

#include 	"Gfx/NxSprite.h"
#include 	"Gfx/xbox/NX/texture.h"
#include 	"Gfx/xbox/NX/sprite.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Machine specific implementation of the CSprite
class	CXboxSprite : public CSprite
{
public:
								CXboxSprite();
	virtual						~CXboxSprite();

private:		// It's all private, as it is machine specific
	virtual void				plat_initialize();

	virtual void				plat_update_hidden();		// Tell engine of update
	virtual void				plat_update_engine();		// Update engine primitives
	virtual void				plat_update_priority();

	NxXbox::sSprite *			mp_plat_sprite;
};

} // Namespace Nx  			

#endif
