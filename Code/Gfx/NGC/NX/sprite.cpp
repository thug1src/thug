#include <string.h>
#include <core/defines.h>
#include <core/macros.h>
#include <core/debug.h>
#include <sys/file/filesys.h>
#include "nx_init.h"
#include "scene.h"
#include "render.h"
#include "sprite.h"

#include <sys/ngc/p_prim.h>
#include <sys/ngc/p_camera.h>
#include <sys/ngc/p_render.h>
#include <sys/ngc/p_display.h>
#include <sys/config/config.h>
#include <sys/ngc/p_gx.h>
#include <dolphin\gd.h>
#include <dolphin\gx\gxvert.h>


namespace NxNgc
{


/******************************************************************/
/*                                                                */
/* SDraw2D														  */
/*                                                                */
/******************************************************************/

SDraw2D *SDraw2D::sp_2D_draw_list = NULL;


SDraw2D::SDraw2D( float pri, bool hide )
{
	m_hidden = hide;
	m_pri = pri;

	mp_next = NULL;

	// add to draw list
	if( !m_hidden )
	{
		InsertDrawList();
	}
}



SDraw2D::~SDraw2D()
{
	// Try removing from draw list
	RemoveDrawList();
}



void SDraw2D::SetPriority( float pri )
{
	if (m_pri != pri)
	{
		m_pri = pri;

		// By removing and re-inserting, we re-sort the list
		if (!m_hidden)
		{
			RemoveDrawList();
			InsertDrawList();
		}
	}
}



void SDraw2D::SetHidden( bool hide )
{
	if (m_hidden != hide)
	{
		m_hidden = hide;
		if (hide)
		{
			RemoveDrawList();
		} else {
			InsertDrawList();
		}
	}
}



void SDraw2D::DrawAll()
{
	set_blend_mode( vBLEND_MODE_BLEND );

	SDraw2D *pDraw = sp_2D_draw_list;
	while (pDraw)
	{
		if (!pDraw->m_hidden)
		{
			pDraw->BeginDraw();
			pDraw->Draw();
			pDraw->EndDraw();
		}
		pDraw = pDraw->mp_next;
	}
}



void SDraw2D::InsertDrawList()
{
	if (!sp_2D_draw_list ||							// Empty List
		(m_pri <= sp_2D_draw_list->m_pri))	// Start List
	{
		mp_next = sp_2D_draw_list;
		sp_2D_draw_list = this;
	} else {				// Insert
		SDraw2D *p_cur = sp_2D_draw_list;
	
		// Find where to insert
		while(p_cur->mp_next)
		{
			if (m_pri <= p_cur->mp_next->m_pri)
				break;

			p_cur = p_cur->mp_next;
		}

		// Insert at this point
		mp_next = p_cur->mp_next;
		p_cur->mp_next = this;
	}
}



void SDraw2D::RemoveDrawList()
{
	// Take out from draw list
	if (sp_2D_draw_list == this)
	{
		sp_2D_draw_list = mp_next;
	} 
	else if (sp_2D_draw_list)
	{
		SDraw2D *p_cur = sp_2D_draw_list;

		while(p_cur->mp_next)
		{
			if (p_cur->mp_next == this)
			{
				p_cur->mp_next = mp_next;
				break;
			}

			p_cur = p_cur->mp_next;
		}
	}
}

	

//typedef struct
//{
//	float		x, y, z;
//	float		rhw;
//	D3DCOLOR	col;
//	float		u, v;
//}
//sSpriteVert;



sSprite::sSprite( float pri ) : SDraw2D( pri, true )
{
	mp_texture = NULL;
	
//	if( D3D_OK != D3DDevice_CreateVertexBuffer(	sizeof( sSpriteVert ) * 6,
//												0,										// Usage - ignored.
//												0,										// FVF - ignored.
//												0,										// Pool - ignored.
//												&p_vertex_buffer ))
//	{
//		exit( 0 );
//	}
//
//
}

sSprite::~sSprite()
{
//	p_vertex_buffer->Release();
}





void sSprite::BeginDraw( void )
{
	// Nothing required here right now.
}

//static inline void _GDWriteXFCmd(u16 addr, u32 val)
//{
//	GXWGFifo.u8 = GX_LOAD_XF_REG;
//	GXWGFifo.u16 = 0; // 0 means one value follows
//	GXWGFifo.u16 = addr;
//	GXWGFifo.u32 = val;
//}
//
//static inline void _GDWriteBPCmd(u32 regval)
//{
//	GXWGFifo.u8 = GX_LOAD_BP_REG;
//	GXWGFifo.u32 = regval;
//}
//
//void _SetGenMode( u8			nTexGens,
//				 u8			nChans,
//				 u8			nTevs,
//				 u8			nInds,
//				 GXCullMode	cm )
//{
//	static u8 cm2hw[] = { 0, 2, 1, 3 };
//	_GDWriteXFCmd( XF_NUMTEX_ID, XF_NUMTEX( nTexGens ));
//	_GDWriteXFCmd( XF_NUMCOLORS_ID, XF_NUMCOLORS( nChans ));
//	_GDWriteBPCmd( GEN_MODE( nTexGens, nChans, 0, (nTevs-1),
//							cm2hw[cm], nInds, 0, GEN_MODE_ID ));
//}


void sSprite::Draw( void )
{
	GXColor current_color;
	current_color.a = (m_rgba&0xff);
	if ( current_color.a == 0 ) return;
	current_color.b = ((m_rgba&0xff00)>>8);
	current_color.g = ((m_rgba&0xff0000)>>16);
	current_color.r = ((m_rgba&0xff000000)>>24);

	float start_screen_x = m_xpos;
	float start_screen_y = m_ypos;

	float x0,y0,x1,y1;

	float u0 = 0.0f;
	float v0 = 0.0f;
	float u1 = 0.0f;
	float v1 = 0.0f;

	if ( mp_texture )
	{
		u0 = 0.0f;
		v0 = 1.0f;
		u1 = (float)mp_texture->BaseWidth / (float)mp_texture->ActualWidth;       
		v1 = 1.0f - ( (float)mp_texture->BaseHeight / (float)mp_texture->ActualHeight ); 
	}

	// Check for flip
	float abs_scale_x = m_scale_x;
	float abs_scale_y = m_scale_y;
	if (abs_scale_x < 0.0f)
	{
		float temp = u0;
		u0 = u1;
		u1 = temp;
		abs_scale_x = -abs_scale_x;
	}
	if (abs_scale_y < 0.0f)
	{
		float temp = v0;
		v0 = v1;
		v1 = temp;
		abs_scale_y = -abs_scale_y;
	}

	x0 = (-m_xhot * abs_scale_x);
	y0 = (-m_yhot * abs_scale_y);

	if ( mp_texture )
	{
		x1 = x0 + ( /*m_width*/mp_texture->BaseWidth * abs_scale_x);
		y1 = y0 + ( /*m_height*/mp_texture->BaseHeight * abs_scale_y);
	//	x1 = x0 + ( m_width * abs_scale_x);
	//	y1 = y0 + ( m_height * abs_scale_y);
	}
	else
	{
		x1 = x0 + ( m_width * abs_scale_x);
		y1 = y0 + ( m_height * abs_scale_y);
	}
	
	Mth::Vector p0(x0, y0, 0.0f, 0.0f);
	Mth::Vector p1(x1, y0, 0.0f, 0.0f);
	Mth::Vector p2(x0, y1, 0.0f, 0.0f);
	Mth::Vector p3(x1, y1, 0.0f, 0.0f);

	if (m_rot != 0.0f)
	{
		p0.RotateZ(m_rot);
		p1.RotateZ(m_rot);
		p2.RotateZ(m_rot);
		p3.RotateZ(m_rot);
	}

	p0[X] += start_screen_x;
	p1[X] += start_screen_x;
	p2[X] += start_screen_x;
	p3[X] += start_screen_x;
	
	p0[Y] += start_screen_y;
	p1[Y] += start_screen_y;
	p2[Y] += start_screen_y;
	p3[Y] += start_screen_y;

	if( mp_texture == NULL )
	{
		// No alpha map.
		GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE ); 

		GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
		GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );
		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
								 GX_TEV_SWAP0, GX_TEV_SWAP0 );
		GX::SetTevColorInOp(	 GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC,
								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
		GX::SetTevOrder( 		 GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL );

//		GX::SetNumTexGens( 0 );
//		GX::SetNumTevStages( 1 );
		//GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//		GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY );
//		GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );

//		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
//		GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//		GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//		GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
//		GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
	
		// Set current vertex descriptor to enable position and color0.
		// Both use 8b index to access their data arrays.
		GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );
	
		// Set material color.
		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
		GX::SetChanAmbColor( GX_COLOR0A0, current_color );
	
		GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
	
		// Send coordinates.
		GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
			GX::Position3f32(p0[X], p0[Y], -1.0f);
			GX::Position3f32(p1[X], p1[Y], -1.0f);
			GX::Position3f32(p3[X], p3[Y], -1.0f);
			GX::Position3f32(p2[X], p2[Y], -1.0f);
		GX::End();
	
		return;
	}
	
//	// Upload the texture.
//	GXTexObj	texObj;
//	GXInitTexObj(	&texObj,
//					mp_texture->pTexelData,
//					mp_texture->ActualWidth,
//					mp_texture->ActualHeight,
//					(GXTexFmt)mp_texture->format,
//					GX_CLAMP,
//					GX_CLAMP,
//					GX_FALSE );
//	GXLoadTexObj(	&texObj,
//					GX_TEXMAP0 );

	GX::UploadTexture( mp_texture->pTexelData,
					   mp_texture->ActualWidth,
					   mp_texture->ActualHeight,
					   (GXTexFmt)mp_texture->format,
					   GX_CLAMP,
					   GX_CLAMP,
					   GX_FALSE,
					   GX_LINEAR,
					   GX_LINEAR,
					   0.0f,
					   0.0f,
					   0.0f,
					   GX_FALSE,
					   GX_FALSE,
					   GX_ANISO_1,
					   GX_TEXMAP0 );

	if ( ( mp_texture->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA ) && mp_texture->pAlphaData )
	{
//		// Upload alpha map.
//		GXTexObj	alphaObj;
//		GXInitTexObj(	&alphaObj,
//						mp_texture->pAlphaData,
//						mp_texture->ActualWidth,
//						mp_texture->ActualHeight,
//						(GXTexFmt)mp_texture->format,
//						GX_CLAMP,
//						GX_CLAMP,
//						GX_FALSE );
//		GXLoadTexObj(	&alphaObj,
//						GX_TEXMAP1 );

		GX::UploadTexture( mp_texture->pAlphaData,
						   mp_texture->ActualWidth,
						   mp_texture->ActualHeight,
						   (GXTexFmt)mp_texture->format,
						   GX_CLAMP,
						   GX_CLAMP,
						   GX_FALSE,
						   GX_LINEAR,
						   GX_LINEAR,
						   0.0f,
						   0.0f,
						   0.0f,
						   GX_FALSE,
						   GX_FALSE,
						   GX_ANISO_1,
						   GX_TEXMAP1 );

		// Has alpha map.
		GX::SetTexChanTevIndCull( 2, 1, 2, 0, GX_CULL_NONE ); 
		GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
		GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, mp_texture->ActualWidth, mp_texture->ActualHeight );
		GX::SetTexCoordScale( GX_TEXCOORD1, GX_TRUE, mp_texture->ActualWidth, mp_texture->ActualHeight );
		GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
		GX::SetTexCoordGen( GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
		GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO,
								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVREG0,
								 GX_TEV_SWAP0, GX_TEV_SWAP0 );
		GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_RASC, GX_CC_TEXC, GX_CC_ZERO,
							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVREG0 );

		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO,
								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV,
								 GX_TEV_SWAP0, GX_TEV_SWAP1 );
		GX::SetTevColorInOp( GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0,
							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
	}
	else
	{
		// No alpha map.
		GX::SetTexChanTevIndCull( 1, 1, 1, 0, GX_CULL_NONE ); 
		GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
		GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, mp_texture->ActualWidth, mp_texture->ActualHeight );
		GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
		GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO,
								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV,
								 GX_TEV_SWAP0, GX_TEV_SWAP0 );
		GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_RASC, GX_CC_TEXC, GX_CC_ZERO,
							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
	}

	// Set current vertex descriptor to enable position and color0.
	// Both use 8b index to access their data arrays.
	GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );

	// Set material color.
	GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
	GX::SetChanAmbColor( GX_COLOR0A0, current_color );

	GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//	GX::SetNumChans( 1 );

//	float uone = u1;		//(float)mp_texture->BaseWidth / (float)mp_texture->ActualWidth;
//	float vzro = v1;		//( (float)mp_texture->BaseHeight / (float)mp_texture->ActualHeight );

	// Send coordinates.
	GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
		GX::Position3f32(p0[X], p0[Y], -1.0f);
		GX::TexCoord2f32(u0, v0);
		GX::Position3f32(p1[X], p1[Y], -1.0f);
		GX::TexCoord2f32(u1, v0);
		GX::Position3f32(p3[X], p3[Y], -1.0f);
		GX::TexCoord2f32(u1, v1);
		GX::Position3f32(p2[X], p2[Y], -1.0f);
		GX::TexCoord2f32(u0, v1);
	GX::End();
}



void sSprite::EndDraw( void )
{
//	if( mp_texture == NULL )
//	{
//		return;
//	}
//
//	D3DDevice_SetPixelShader( 0 );
//	set_vertex_shader( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2( 0 ));
//	D3DDevice_SetRenderState( D3DRS_LIGHTING, FALSE );
//
//	D3DDevice_SetTexture( 0, (LPDIRECT3DBASETEXTURE8)mp_texture->pD3DTexture );
//	if( mp_texture->pD3DPalette )
//	{
//		D3DDevice_SetPalette( 0, mp_texture->pD3DPalette );
//	}
//	
//	D3DDevice_SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE2X );
//	D3DDevice_SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
//	D3DDevice_SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_TEXTURE );
//	D3DDevice_SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE2X );
//	D3DDevice_SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
//	D3DDevice_SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE );
//
//	D3DDevice_SetStreamSource( 0, p_vertex_buffer, sizeof( sSpriteVert ));
//	EngineGlobals.p_Device->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );
}






} // namespace NxNgc


