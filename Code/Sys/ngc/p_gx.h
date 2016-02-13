#ifndef _GX_H_
#define _GX_H_

#ifndef __PLAT_WN32__
#include <dolphin.h>
#include <gfx/ngc/nx/texture.h>
#include <core/math.h>
#endif		// __PLAT_WN32__

#ifdef __PLAT_WN32__
typedef struct
{

	f32 x, y, z;

} Vec, *VecPtr, Point3d, *Point3dPtr;
#endif		// __PLAT_WN32__

// Types we need, taken from gx headers.
#ifndef __NO_DEFS__

#define GX_FIFO_OBJ_SIZE            128

typedef struct{
    u8      pad[GX_FIFO_OBJ_SIZE];
} GXFifoObj;

typedef void (*GXBreakPtCallback)(void);
typedef void (*GXDrawSyncCallback)(u16 token);
typedef void (*GXDrawDoneCallback)(void);

extern GXRenderModeObj GXNtsc240Ds;
extern GXRenderModeObj GXNtsc240DsAa;
extern GXRenderModeObj GXNtsc240Int;
extern GXRenderModeObj GXNtsc240IntAa;
extern GXRenderModeObj GXNtsc480IntDf;
extern GXRenderModeObj GXNtsc480Int;
extern GXRenderModeObj GXNtsc480IntAa;
extern GXRenderModeObj GXNtsc480Prog;
extern GXRenderModeObj GXNtsc480ProgAa;
extern GXRenderModeObj GXMpal240Ds;
extern GXRenderModeObj GXMpal240DsAa;
extern GXRenderModeObj GXMpal240Int;
extern GXRenderModeObj GXMpal240IntAa;
extern GXRenderModeObj GXMpal480IntDf;
extern GXRenderModeObj GXMpal480Int;
extern GXRenderModeObj GXMpal480IntAa;
extern GXRenderModeObj GXPal264Ds;
extern GXRenderModeObj GXPal264DsAa;
extern GXRenderModeObj GXPal264Int;
extern GXRenderModeObj GXPal264IntAa;
extern GXRenderModeObj GXPal528IntDf;
extern GXRenderModeObj GXPal528Int;
extern GXRenderModeObj GXPal524IntAa; // Reduced due to overlap requirement!
extern GXRenderModeObj GXEurgb60Hz240Ds;
extern GXRenderModeObj GXEurgb60Hz240DsAa;
extern GXRenderModeObj GXEurgb60Hz240Int;
extern GXRenderModeObj GXEurgb60Hz240IntAa;
extern GXRenderModeObj GXEurgb60Hz480IntDf;
extern GXRenderModeObj GXEurgb60Hz480Int;
extern GXRenderModeObj GXEurgb60Hz480IntAa;

#define	GXPackedRGB565(r,g,b)   \
	((u16)((((r)&0xf8)<<8)|(((g)&0xfc)<<3)|(((b)&0xf8)>>3)))
#define	GXPackedRGBA4(r,g,b,a)  \
	((u16)((((r)&0xf0)<<8)|(((g)&0xf0)<<4)|(((b)&0xf0)   )|(((a)&0xf0)>>4)))
#define	GXPackedRGB5A3(r,g,b,a) \
	((u16)((a)>=224 ? \
	((((r)&0xf8)<<7)|(((g)&0xf8)<<2)|(((b)&0xf8)>>3)|(1<<15)): \
	((((r)&0xf0)<<4)|(((g)&0xf0)   )|(((b)&0xf0)>>4)|(((a)&0xe0)<<7))))
	
// Display list opcodes:

#define GX_NOP                      0x00
#define GX_DRAW_QUADS               0x80
#define GX_DRAW_TRIANGLES           0x90
#define GX_DRAW_TRIANGLE_STRIP      0x98
#define GX_DRAW_TRIANGLE_FAN        0xA0
#define GX_DRAW_LINES               0xA8
#define GX_DRAW_LINE_STRIP          0xB0
#define GX_DRAW_POINTS              0xB8

#define GX_LOAD_BP_REG              0x61
#define GX_LOAD_CP_REG              0x08
#define GX_LOAD_XF_REG              0x10
#define GX_LOAD_INDX_A              0x20
#define GX_LOAD_INDX_B              0x28
#define GX_LOAD_INDX_C              0x30
#define GX_LOAD_INDX_D              0x38

#define GX_CMD_CALL_DL              0x40
#define GX_CMD_INVL_VC              0x48

#define GX_OPCODE_MASK              0xF8
#define GX_VAT_MASK                 0x07

#define GX_PROJECTION_SZ  7
#define GX_VIEWPORT_SZ    6

#endif		// __NO_DEFS__

namespace NxNgc
{
struct sMaterialPassHeader;		// Forward reference.
struct sTextureDL;		// Forward reference.
}

// Allows switching between GX & GD.
namespace GX
{
	void				begin				( void *						p_buffer = NULL,
											  int							max_size = 0 );
								
	int					end					( void );           			
								
	void				SetChanCtrl			( GXChannelID					chan,
											  GXBool						enable,
											  GXColorSrc					amb_src,
											  GXColorSrc					mat_src,
											  u32							light_mask,
											  GXDiffuseFn					diff_fn,
											  GXAttnFn						attn_fn );
							
	void				SetTevOrder			( GXTevStageID					evenStage,
											  GXTexCoordID					coord0,
											  GXTexMapID					map0,
											  GXChannelID					color0,
											  GXTexCoordID					coord1,
											  GXTexMapID					map1,
											  GXChannelID					color1 );
						
	void				SetTevAlphaInOpSwap	( GXTevStageID					stage,
											  GXTevAlphaArg					a,
											  GXTevAlphaArg					b,
											  GXTevAlphaArg					c,
											  GXTevAlphaArg					d,
											  GXTevOp						op,
											  GXTevBias						bias,
											  GXTevScale					scale,
											  GXBool						clamp,
											  GXTevRegID					out_reg,
											  GXTevSwapSel					ras_sel,
											  GXTevSwapSel					tex_sel );
							
	void				SetTevColorInOp		( GXTevStageID					stage,
											  GXTevColorArg 				a,
											  GXTevColorArg 				b,
											  GXTevColorArg 				c,
											  GXTevColorArg 				d,
											  GXTevOp       				op,
											  GXTevBias     				bias,
											  GXTevScale    				scale,
											  GXBool        				clamp,
											  GXTevRegID    				out_reg );
							
	void				SetTevKSel			( GXTevStageID					even_stage,
											  GXTevKColorSel 				color_even,
											  GXTevKAlphaSel 				alpha_even,
											  GXTevKColorSel 				color_odd,
											  GXTevKAlphaSel 				alpha_odd );
						
	void				SetTevKColor		( GXTevKColorID					id,
											  GXColor						color );
						
	void				SetTevColor			( GXTevRegID					reg,
											  GXColor						color );

	void				SetTexCoordGen		( GXTexCoordID					dst_coord,
											  GXTexGenType					func,
											  GXTexGenSrc					src_param,
											  GXBool						normalize,
											  u32							postmtx );
						
	void				UploadTexture		( void *						image_ptr,
											  u16							width,
											  u16							height,
											  GXTexFmt						format,
											  GXTexWrapMode					wrap_s,
											  GXTexWrapMode					wrap_t,
											  GXBool						mipmap,
											  GXTexFilter					min_filt,
											  GXTexFilter					mag_filt,
											  f32							min_lod,
											  f32							max_lod,
											  f32							lod_bias,
											  GXBool						bias_clamp,
											  GXBool						do_edge_lod,
											  GXAnisotropy					max_aniso,
											  GXTexMapID					id );
						
	void				UploadTexture		( void *						image_ptr,
											  u16							width,
											  u16							height,
											  GXCITexFmt					format,
											  GXTexWrapMode					wrap_s,
											  GXTexWrapMode					wrap_t,
											  GXBool						mipmap,
											  GXTexFilter					min_filt,
											  GXTexFilter					mag_filt,
											  f32							min_lod,
											  f32							max_lod,
											  f32							lod_bias,
											  GXBool						bias_clamp,
											  GXBool						do_edge_lod,
											  GXAnisotropy					max_aniso,
											  GXTexMapID					id );
						
	void				UploadPalette		( void *						tlut_ptr,
											  GXTlutFmt						tlut_format,
											  GXTlutSize					tlut_size,
											  GXTexMapID                    id );

	void				SetTexCoordScale	( GXTexCoordID					coord,
											  GXBool						enable,
											  u16							ss,
											  u16							ts );
						
	void				SetBlendMode		( GXBlendMode					type,
											  GXBlendFactor					src_factor,
											  GXBlendFactor					dst_factor,
											  GXLogicOp						op,
											  GXBool						color_update_enable,
											  GXBool						alpha_update_enable,
											  GXBool						dither_enable );

	void				SetTexChanTevIndCull( u8 							nTexGens,
											  u8 							nChans,
											  u8 							nTevs,
											  u8 							nInds,
											  GXCullMode					cm );
						
	void				SetChanAmbColor		( GXChannelID					chan,
											  GXColor						color );
						
	void				SetChanMatColor		( GXChannelID chan, 			
											  GXColor color );  			
						
	void				SetPointSize		( u8							pointSize,
											  GXTexOffset					texOffsets );

	void				SetFog				( GXFogType						type,
											  f32							startz,
											  f32							endz,
											  f32							nearz,
											  f32							farz,
											  GXColor						color );

	void				SetFogColor			( GXColor						color );

	void				Begin				( GXPrimitive					type,
											  GXVtxFmt						vtxfmt,
											  u16							nverts );
						
	void				End					( void );           			
						
	void				Position3f32		( f32							x,
											  f32							y,
											  f32							z );
						
	void				Position3u8			( u8							x,
											  u8							y,
											  u8							z );
										
	void				Position3s8			( s8							x,
											  s8							y,
											  s8							z );
						
	void				Position3u16		( u16							x,
											  u16							y,
											  u16							z );
									
	void				Position3s16		( s16							x,
											  s16							y,
											  s16							z );
									
	void				Position2f32		( f32							x,
											  f32							y );
						
	void				Position2u8			( u8							x,
											  u8							y );
										
	void				Position2s8			( s8							x,
											  s8							y );
						
	void				Position2u16		( u16							x,
											  u16							y );
									
	void				Position2s16		( s16							x,
											  s16							y );
									
	void				Position1x16		( u16							i );
						
	void				Position1x8			( u8							i );
						
	void				Normal3f32			( f32							x,
											  f32							y,
											  f32							z );
						
	void				Normal3s16			( s16							x,
											  s16							y,
											  s16							z );
						
	void				Normal3s8			( s8							x,
											  s8							y,
											  s8							z );
						
	void				Normal1x16			( u16							i );
						
	void				Normal1x8			( u8							i );
						
	void				Color4u8			( u8							r,
											  u8							g,
											  u8							b,
											  u8							a );
						
	void				Color1u32			( u32							rgba );
						
	void				Color1x16			( u16							i );
						
	void				Color1x8			( u8							i );
						
	void				TexCoord2f32		( f32							s,
											  f32							t );
									
	void				TexCoord2s16		( s16							s,
											  s16							t );
									
	void				TexCoord2u16		( u16							s,
											  u16							t );
						
	void				TexCoord2s8			( s8							s,
											  s8							t );
						
	void				TexCoord2u8			( u8							s,
											  u8							t );
						
	void				TexCoord1f32		( f32							s );
						
	void				TexCoord1s16		( s16							s );
						
	void				TexCoord1u16		( u16							s );
						
	void				TexCoord1s8			( s8							s );
						
	void				TexCoord1u8			( u8							s );
						
	void				TexCoord1x16		( u16							i );
						
	void				TexCoord1x8			( u8							i );
										
	void				MatrixIndex1u8		( u8							i );
						
	void				SetZMode			( GXBool						compare_enable,
											  GXCompare						func,
											  GXBool						update_enable );
						
	void				SetVtxDesc			( int							items,
											  ... );            			
						
	void				LoadPosMtxImm		( const							f32 mtx[3][4],
											  u32							id );
						
	void				LoadNrmMtxImm		( const							f32 mtx[3][4],
											  u32							id );
						
	void				LoadTexMtxImm		( const							f32 mtx[3][4],
											  u32							id,
											  GXTexMtxType					type );
						
	void				SetAlphaCompare		( GXCompare						comp0,
											  u8							ref0,
											  GXAlphaOp						op,
											  GXCompare						comp1,
											  u8							ref1 );
						
	void				SetCurrMtxPosTex03	( u32							pn,
											  u32							t0,
											  u32							t1,
											  u32							t2,
											  u32							t3 );
						
	void				SetCurrMtxTex47		( u32							t4,
											  u32							t5,
											  u32							t6,
											  u32							t7 );
						
	void				SetProjection		( const							f32 mtx[4][4],
											  GXProjectionType				type );
						
	void				SetTevSwapModeTable	( GXTevSwapSel					table,
											  GXTevColorChan				red,
											  GXTevColorChan				green,
											  GXTevColorChan				blue,
											  GXTevColorChan				alpha );
						
	void				SetScissorBoxOffset	( s32							x_off,
											  s32							y_off );
						
	void				SetScissor			( u32							left,
											  u32							top,
											  u32							wd,
											  u32							ht );
						
	void				SetDispCopyGamma	( GXGamma						gamma );
			
	GXBreakPtCallback	SetBreakPtCallback	( GXBreakPtCallback				cb );
			
	void				Flush				( void );           			
			
	void				GetFifoPtrs			( GXFifoObj *					fifo,
											  void **						readPtr,
											  void **						writePtr );
			
	GXFifoObj *			GetCPUFifo			( void );           			
			
	GXFifoObj *			GetGPFifo			( void );           			
			
	void				EnableBreakPt		( void *						breakPtr );
			
	void				DisableBreakPt		( void );           			
			
	void				SetGPMetric			( GXPerf0						perf0,
											  GXPerf1						perf1 );
			
	void				ClearGPMetric		( void );           			
			
	void				ReadGPMetric		( u32 *							cnt0,
											  u32 *							cnt1 );
			
	void				SetDrawSync			( u16							token );
			
	u16					ReadDrawSync		( void );           			
			
	void				SetCopyFilter		( GXBool						aa,
											  const u8						sample_pattern[12][2],
											  GXBool						vf,
											  const u8						vfilter[7] );
			
	void				CopyDisp			( void *						dest,
											  GXBool						clear );
			
	void				SetCopyClear		( GXColor						clear_clr,
											  u32							clear_z );
			
	void				PokeZ				( u16							x,
											  u16							y,
											  u32							z );
			
	void				PeekZ				( u16							x,
											  u16							y,
											  u32 *							z );
			
	void				PokeARGB			( u16							x,
											  u16							y,
											  u32							color );
			
	void				PeekARGB			( u16							x,
											  u16							y,
											  u32 *							color );
			
	void				SetClipMode			( GXClipMode					mode );
			
	void				SetZCompLoc			( GXBool						before_tex );
			
	void				SetPixelFmt			( GXPixelFmt					pix_fmt,
											  GXZFmt16						z_fmt );
			
	GXFifoObj *			Init				( void *						base,
											  u32							size );
	
	void				AdjustForOverscan	( const GXRenderModeObj *		rmin,
											  GXRenderModeObj *				rmout,
											  u16							hor,
											  u16							ver );
	
	void				SetViewport			( f32							left,
											  f32							top,
											  f32							wd,
											  f32							ht,
											  f32							nearz,
											  f32							farz );
	
	void				SetDispCopySrc		( u16							left,
											  u16							top,
											  u16							wd,
											  u16							ht );
	
	void				SetDispCopyDst		( u16							wd,
											  u16							ht );
	
	u32					SetDispCopyYScale	( f32							vscale );
	
	void				SetVtxAttrFmt		( GXVtxFmt						vtxfmt,
											  GXAttr						attr,
											  GXCompCnt						cnt,
											  GXCompType					type,
											  u8							frac );
	
	void				SetViewportJitter	( f32							left,
											  f32							top,
											  f32							wd,
											  f32							ht,
											  f32							nearz,
											  f32							farz,
											  u32							field );
	
	void				InvalidateVtxCache	( void );                   	
	
	void				InvalidateTexAll	( void );                   	
	
	void				SetDrawSync			( u16							token );
	
	u16					ReadDrawSync		( void );                   	
	
	GXDrawSyncCallback	SetDrawSyncCallback	( GXDrawSyncCallback			cb );
	
	void				DrawDone			( void );                   	

	void				AbortFrame			( void );
	
	void				SetDrawDone			( void );                   	
	
	void				WaitDrawDone		( void );                   	
	
	GXDrawDoneCallback	SetDrawDoneCallback	( GXDrawDoneCallback			cb );
	
	void				ReadXfRasMetric		( u32 *							xf_wait_in,
											  u32 *							xf_wait_out,
											  u32 *							ras_busy,
											  u32 *							clocks );
	
	void				GetGPStatus			( GXBool *						overhi,
											  GXBool *						underlow,
											  GXBool *						readIdle,
											  GXBool *						cmdIdle,
											  GXBool *						brkpt );
	
	void				GetFifoStatus		( GXFifoObj *					fifo,
											  GXBool *						overhi,
											  GXBool *						underlow,
											  u32 *							fifoCount,
											  GXBool *						cpu_write,
											  GXBool *						gp_read,
											  GXBool *						fifowrap );
	
	void *				GetFifoBase			( const GXFifoObj *				fifo );
	
	u32					GetFifoSize			( const GXFifoObj *				fifo );
	
	void				GetFifoLimits		( const GXFifoObj *				fifo,
											  u32 *							hi,
											  u32 *							lo );

	u32					GetOverflowCount	( void );

	u32					ResetOverflowCount	( void );

	void				EnableHang			( void );

	void				DisableHang			( void );

	void				ResolveDLTexAddr	( NxNgc::sTextureDL *			p_dl,
											  NxNgc::sMaterialPassHeader *	p_pass,
											  int							num_passes );

	void				ChangeMaterialColor	( unsigned char *				p_dl,
											  unsigned int					size,
											  GXColor						color,
											  int							pass );

	void				SetTexCopySrc		( u16							left,
											  u16							top,
											  u16							wd,
											  u16							ht );

	void				SetTexCopyDst		( u16							wd,
											  u16							ht,
											  GXTexFmt						fmt,
											  GXBool						mipmap );

	void				CopyTex				( void *						dest,
											  GXBool						clear );

	void				PixModeSync			( void );

	void				PokeAlphaUpdate		( GXBool						update_enable );

	void				PokeColorUpdate		( GXBool						update_enable );

	void				GetProjectionv		( f32 *							p );

	void				SetProjectionv		( const f32 *					ptr );

	void				GetViewportv		( f32 *							viewport );

	void				GetScissor			( u32 *							left,
											  u32 *							top,
											  u32 *							width,
											  u32 *							height );

	void				Project				( f32							x,          // model coordinates
											  f32							y,
											  f32							z,
											  const f32						mtx[3][4],  // model-view matrix
											  const f32 *					pm,         // projection matrix, as returned by GXGetProjectionv
											  const f32 *					vp,         // viewport, as returned by GXGetViewportv
											  f32 *							sx,         // screen coordinates
											  f32 *							sy,
											  f32 *							sz );

	void				SetArray			( GXAttr 						attr,
											  const void *					base_ptr,
											  u8 							stride );

	void				InitLightAttn		( GXLightObj *					lt_obj,
											  f32							a0,
											  f32							a1,
											  f32							a2,
											  f32							k0,
											  f32							k1,
											  f32							k2 );

	void				InitLightAttnA		( GXLightObj *					lt_obj,
											  f32							a0,
											  f32							a1,
											  f32							a2 );

	void				InitLightAttnK		( GXLightObj *					lt_obj,
											  f32							k0,
											  f32							k1,
											  f32							k2 );

	void				InitLightSpot		( GXLightObj *					lt_obj,
											  f32							cutoff,
											  GXSpotFn						spot_func );

	void				InitLightDistAttn	( GXLightObj *					lt_obj,
											  f32							ref_distance,
											  f32							ref_brightness,
											  GXDistAttnFn					dist_func );

	void				InitLightPos		( GXLightObj *					lt_obj,
											  f32							x,
											  f32							y,
											  f32							z );

	void				InitLightColor		( GXLightObj *					lt_obj,
											  GXColor						color );

	void				LoadLightObjImm		( const GXLightObj *			lt_obj,
											  GXLightID						light );

	void				LoadLightObjIndx	( u32							lt_obj_indx,
											  GXLightID						light );

	void				InitLightDir		( GXLightObj *					lt_obj,
											  f32							nx,
											  f32							ny,
											  f32							nz );

	void				InitSpecularDir		( GXLightObj *					lt_obj,
											  f32							nx,
											  f32							ny,
											  f32							nz );

	void				InitSpecularDirHA	( GXLightObj *					lt_obj,
											  f32							nx,
											  f32							ny,
											  f32							nz,
											  f32							hx,
											  f32							hy,
											  f32							hz );

	void				InitLightPosv		( GXLightObj *					lt_obj,
											  Vec *							p );

	void				InitLightDirv		( GXLightObj *					lt_obj,
											  Vec *							n );

	void				InitSpecularDirv	( GXLightObj *					lt_obj,
											  Vec *							n );

	void				InitSpecularDirHAv	( GXLightObj *					lt_obj,
											  Vec *							n,
											  Vec *							h );

	void				InitLightShininess	( GXLightObj *					lt_obj,
											  float							shininess );

	void				CallDisplayList		( const void *					list,
											  u32							nbytes );

	void				ResetGX				( void );

	u32					GetTexBufferSize	( u16							width,
											  u16							height,
											  u32							format,
											  GXBool						mipmap,
											  u8							max_lod );

}

#endif		// _GX_H_

#ifndef _GX2_H_
#define _GX2_H_

namespace NxNgc
{

#ifdef __PLAT_WN32__
typedef NxTexture sTexture;
#endif		// __PLAT_WN32__

struct sMaterialDL
{
	uint32	m_dl_size;
	void *	mp_dl;
};

struct sMeshMod
{
	Mth::Vector	offset;
	Mth::Vector	scale;
};

struct sTextureDL
{
	uint32	m_dl_size;
	void *	mp_dl;
	int16	m_tex_offset[4];
	int16	m_alpha_offset[4];
};

struct sMaterialHeader
{
	uint32	m_checksum;
	uint8	m_passes;
	uint8	m_alpha_cutoff;
	uint8	m_flags;
	uint8	m_base_blend;		//m_material_dl_id;
	float	m_draw_order;
	uint16	m_pass_item;
	uint16	m_texture_dl_id;
	float	m_shininess;
	GXColor	m_specular_color;

	uint32	m_materialNameChecksum;
	uint32	m_layer_id;
};

struct sMaterialUVWibble
{
	float	m_u_vel;
	float	m_v_vel;
	float	m_u_freq;
	float	m_v_freq;
	float	m_u_amp;
	float	m_v_amp;
	float	m_u_phase;
	float	m_v_phase;
};

struct sMaterialPassHeader
{
	union {
		uint32		checksum;
		sTexture *	p_data;
	} m_texture;
	uint8	m_flags;
	uint8	m_filter;
	uint8	m_blend_mode;
	uint8	m_alpha_fix;
	uint32	m_uv_wibble_index;
	GXColor	m_color;

	s16		m_k;	//float	m_mip_k;
	s16		m_u_tile;
	s16		m_v_tile;
	s8		m_pad;
	s8		m_uv_enabled;

	s16		m_uv_mat[4];
};

struct sMaterialVCWibbleKeyHeader
{
	int		m_num_frames;
	int		m_phase;
};

struct sMaterialVCWibbleKey
{
	int		m_time;
	GXColor	m_color;
};

struct sSceneHeader
{
	uint32 m_num_pos;
	uint16 m_pad;
	uint16 m_num_nrm;
	uint16 m_num_col;
	uint16 m_num_tex;
	uint32 m_num_pool_bytes;
	uint16 m_num_objects;
	uint16 m_num_materials;
	uint32 m_num_shadow_faces;
	uint16 m_num_blend_dls;
	uint16 m_num_vc_wibbles;
	uint16 m_num_uv_wibbles;
	uint16 m_num_pass_items;
};

struct sObjectHeader
{
	uint16		m_num_meshes;                   // 2
	uint16		m_billboard_type;               // 2
	union {                         			        	
		int		num_bytes;						// 4
		void *	p_data;
	} m_skin;
	uint16		m_num_skin_verts;				// 2
	uint16		m_num_double_lists;				// 2
	uint8		m_num_single_lists;				// 1
	uint8		m_num_add_lists;				// 1
	uint16		m_bone_index;					// 2
	Mth::Vector	m_origin;                       // 16
	Mth::Vector	m_axis;                         // 16
	Mth::Vector	m_sphere;						// 16
};

struct sShadowEdge
{
	short neighbor[3];
};

struct sDLHeader
{
	// 32 bytes.
	uint32 					m_size;				// 4
	union {                                 	
		uint32				checksum;			// 4
		sMaterialHeader *	p_header;       	
	} m_material;                           	
	uint32					m_flags;            // 4
	uint32					m_checksum;         // 4
	
	// 32 bytes.                            	
	Mth::Vector				m_sphere;			// 16
	NxNgc::sObjectHeader *	mp_object_header;	// 4
	uint16					m_index_offset;		// 2 - Offset into DL where index data starts.
	uint16					m_index_stride;		// 2 - Stride of index data (pos always 1st).
	uint16					m_num_indices;		// 2 - Number of indices in this DL.
	uint8					m_color_offset;		// 1 - Offset of color index.
	uint8					m_correctable;		// 1 - Mesh is color correctable?
	uint32					m_mesh_index;		// 4 - Mesh index within object.
	uint32					m_original_dl_size;	// 4 - Same as m_size until hide_mesh has been run.
//	uint32					m_matrix_index;		// 4 - Typically 0 for env. Skinned/hierarchical will use this (skinned will use it if all verts are single weight, single matrix).
	float *					mp_pos_pool;		// 4 - Modified pos pool
	uint32					m_array_base;		// 4 - 0 unless pool is bigger than 65536.
	uint32 *				mp_col_pool;        // 4 - Modified col pool
//	uint32					m_pad;           	// 4
//	uint32					m_pad[2];           // 8



//	uint32					m_num_skin_verts;	// 4
//	union {                                 	
//		int					num_bytes;			// 4
//		void *				p_data;
//	} m_skin;
//	uint32					m_num_single_lists;	// 4
//	uint32					m_num_double_lists;	// 4
//	uint32					m_num_add_lists;	// 4
};

void multi_mesh( NxNgc::sMaterialHeader * p_mat, NxNgc::sMaterialPassHeader * p_base_pass, bool bl = true, bool tx = true, bool should_correct_color = false, bool cull = false );
}

#endif		// _GX2_H_



