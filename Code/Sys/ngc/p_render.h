#ifndef _RENDER_H_
#define _RENDER_H_

#include <dolphin.h>

typedef enum {
	NsBlendMode_None = 0,		// 0:  Do nothing. No cut-out, no blend.
	NsBlendMode_Sub,			// 1:  Subtractive, using texture alpha
	NsBlendMode_Blend,			// 2:  Blend, using texture alpha
	NsBlendMode_Add,			// 3:  Additive, using texture alpha
	NsBlendMode_SubAlpha,		// 4:  Subtractive, using constant alpha
	NsBlendMode_BlendAlpha,		// 5:  Blend, using constant alpha
	NsBlendMode_AddAlpha,		// 6:  Additive, using constant alpha
	NsBlendMode_AlphaTex,		// 7:  Blend framebuffer color and texture alpha.
	NsBlendMode_InvAlphaTex,	// 8:  Blend framebuffer color and inverse texture alpha.
	NsBlendMode_AlphaFB,		// 9:  Blend ( fbRGB * texture alpha ) + fbRGB.
	NsBlendMode_Black,			// 10: 	Output RGB 0,0,0.

	NsBlendMode_Max
} NsBlendMode;

typedef enum {
	NsZMode_Never			= GX_NEVER,
	NsZMode_Less			= GX_LESS,
	NsZMode_LessEqual		= GX_LEQUAL,
	NsZMode_Equal			= GX_EQUAL,
	NsZMode_NotEqual		= GX_NEQUAL,
	NsZMode_Greater		= GX_GREATER,
	NsZMode_GreaterEqual	= GX_GEQUAL,
	NsZMode_Always		= GX_ALWAYS,

	NsZMode_Max
} NsZMode;

typedef enum {
	NsCullMode_Never	= GX_CULL_NONE,
	NsCullMode_Front	= GX_CULL_FRONT,
	NsCullMode_Back	= GX_CULL_BACK,
	NsCullMode_Always	= GX_CULL_ALL,

	NsCullMode_Max
} NsCullMode;

namespace NsRender
{
	void		begin			( void );

////	void		setBlendMode	( NsBlendMode blendMode, /*NsTexture*/void * pTex, float fixAlpha );
////	void		setBlendMode	( NsBlendMode blendMode, /*NsTexture*/void * pTex, unsigned char fixAlpha );
//	void		getZMode		( NsZMode* mode, int* zcompare, int* zupdate, int* rejectBeforeTexture );
//	void		setZMode		( NsZMode mode, int zcompare, int zupdate, int rejectBeforeTexture );
//	void		setCullMode		( NsCullMode cullMode );
//	bool		enableFog		( bool enable );
//	void		setFogColor		( int r, int g, int b );
//	void		setFogNear		( float near );
//	void		setFogFar		( float far );
//	NsBlendMode	getBlendMode	( unsigned int blend0 );

	void		end				( void );
};

#endif		// _RENDER_H_
