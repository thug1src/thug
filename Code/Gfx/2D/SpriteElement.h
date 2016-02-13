#ifndef __GFX_2D_SPRITEELEMENT_H__
#define __GFX_2D_SPRITEELEMENT_H__

#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif
#include <gfx/2D/ScreenElement2.h>

namespace Nx
{
	class CSprite;
}

namespace Front
{

class CSpriteElement : public CScreenElement
{
	friend class CScreenElementManager;

public:
							CSpriteElement();
	virtual					~CSpriteElement();

	void					SetProperties(Script::CStruct *pProps);
	void					SetMorph(Script::CStruct *pProps);
	
	void 					SetTexture(uint32 texture_checksum);
	void					SetRotate( float angle, EForceInstant forceInstant = FORCE_INSTANT );

	void					WritePropertiesToStruct( Script::CStruct *pStruct );
	
protected:

	void					update();

	uint32					m_texture;

	Nx::CSprite *			mp_sprite;
};


}

#endif
