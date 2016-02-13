///////////////////////////////////////////////////////////////////////////////
// p_NxScene.h

#ifndef	__GFX_P_NX_SPRITE_H__
#define	__GFX_P_NX_SPRITE_H__

#include 	"Gfx/NxSprite.h"
#include 	"Gfx/Ngc/NX/texture.h"
#include 	"Gfx/Ngc/NX/sprite.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Machine specific implementation of the CSprite
class	CNgcSprite : public CSprite
{
public:
								CNgcSprite();
	virtual						~CNgcSprite();

private:		// It's all private, as it is machine specific
	virtual void				plat_initialize();

	virtual void				plat_update_hidden();		// Tell engine of update
	virtual void				plat_update_engine();		// Update engine primitives
	virtual void				plat_update_priority();

	NxNgc::sSprite *			mp_plat_sprite;
};

} // Namespace Nx  			

#endif

