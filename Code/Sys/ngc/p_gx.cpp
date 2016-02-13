/********************************************************************************
 *																				*
 *	Module:																		*
 *				NsGX															*
 *	Description:																*
 *				GXs matrices.													*
 *	Written by:																	*
 *				Paul Robinson													*
 *	Copyright:																	*
 *				2001 Neversoft Entertainment - All rights reserved.				*
 *																				*
 ********************************************************************************/

/********************************************************************************
 * Includes.																	*
 ********************************************************************************/

#undef __GX_H__

#ifdef __PLAT_WN32__

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <Strip/Strip.h>
#include <GC/GameCubeConv.h>
//#include <List/Search.h>
//#include <Util.h>
#include <Misc/GenCRC.h>
#include <windows.h>

#include "Utility.h"

#endif		// __PLAT_WN32__

#ifndef __PLAT_WN32__
#define __NO_DEFS__
#endif		// __PLAT_WN32__

#include <dolphin\gd.h>
#include "p_GX.h"

#ifndef __PLAT_WN32__
#include <gfx\ngc\nx\render.h>
#endif		// __PLAT_WN32__

int g_mat_passes = 4;

namespace GX
{

/********************************************************************************
 * Defines.																		*
 ********************************************************************************/
 
#define TEV_KSEL_MASK_KCSEL \
    (( 0x00001F << TEV_KSEL_KCSEL0_SHIFT ) | \
     ( 0x00001F << TEV_KSEL_KASEL0_SHIFT ) | \
     ( 0x00001F << TEV_KSEL_KCSEL1_SHIFT ) | \
     ( 0x00001F << TEV_KSEL_KASEL1_SHIFT ))

/********************************************************************************
 * Structures.																	*
 ********************************************************************************/

/********************************************************************************
 * Types.																		*
 ********************************************************************************/

typedef void (*_write_u8  )( u8  data );
typedef void (*_write_s8  )( s8  data );
typedef void (*_write_u16 )( u16 data );
typedef void (*_write_s16 )( s16 data );
typedef void (*_write_u32 )( u32 data );
typedef void (*_write_s32 )( s32 data );
typedef void (*_write_f32 )( f32 data );
typedef void (*_write_u24 )( u32 data );

typedef void (*_write_BPCmd      )( u32 regval                  );
typedef void (*_write_CPCmd      )( u8  addr, u32 val           );
typedef void (*_write_XFCmd      )( u16 addr, u32 val           );
typedef void (*_write_XFCmdHdr   )( u16 addr, u8 len            );
typedef void (*_write_XFIndxACmd )( u16 addr, u8 len, u16 index );
typedef void (*_write_XFIndxBCmd )( u16 addr, u8 len, u16 index );
typedef void (*_write_XFIndxCCmd )( u16 addr, u8 len, u16 index );
typedef void (*_write_XFIndxDCmd )( u16 addr, u8 len, u16 index );

/********************************************************************************
 * Local variables.																*
 ********************************************************************************/

GDLObj			g_dl;
//float			g_projection[7];

u32 MyTmemRegions[8][2] = {
   // TMEM LO, TMEM HI
    { 0x00000, 0x80000 },  // (default assignment for texmap 0)
    { 0x08000, 0x88000 },  //                                1
    { 0x10000, 0x90000 },  //                                2
    { 0x18000, 0x98000 },  //                                3
    { 0x20000, 0xa0000 },  //                                4
    { 0x28000, 0xa8000 },  //                                5
    { 0x30000, 0xb0000 },  //                                6
    { 0x38000, 0xb8000 }   //                                7
};
int tmem_count = 0;
int tlut_count = 0;

typedef enum
{
	USE_DIRECT = 0,
	USE_DL,

	USE_MAX
} USE;

USE	g_use = USE_MAX;		// Currently set usage.

static _write_u8  _u8 = NULL;
static _write_s8  _s8 = NULL;
static _write_u16 _u16 = NULL;
static _write_s16 _s16 = NULL;
static _write_u32 _u32 = NULL;
static _write_s32 _s32 = NULL;
static _write_f32 _f32 = NULL;
static _write_u24 _u24 = NULL;

static _write_BPCmd      _BPCmd = NULL;
static _write_CPCmd      _CPCmd = NULL;
static _write_XFCmd      _XFCmd = NULL;
static _write_XFCmdHdr   _XFCmdHdr = NULL;
static _write_XFIndxACmd _XFIndxACmd = NULL;
static _write_XFIndxBCmd _XFIndxBCmd = NULL;
static _write_XFIndxCCmd _XFIndxCCmd = NULL;
static _write_XFIndxDCmd _XFIndxDCmd = NULL;

u8 GDTexMode0Ids[8] = {
    TX_SETMODE0_I0_ID,
    TX_SETMODE0_I1_ID,
    TX_SETMODE0_I2_ID,
    TX_SETMODE0_I3_ID,
    TX_SETMODE0_I4_ID,
    TX_SETMODE0_I5_ID,
    TX_SETMODE0_I6_ID,
    TX_SETMODE0_I7_ID,
};

u8 GDTexMode1Ids[8] = {
    TX_SETMODE1_I0_ID,
    TX_SETMODE1_I1_ID,
    TX_SETMODE1_I2_ID,
    TX_SETMODE1_I3_ID,
    TX_SETMODE1_I4_ID,
    TX_SETMODE1_I5_ID,
    TX_SETMODE1_I6_ID,
    TX_SETMODE1_I7_ID,
};

u8 GDTexImage0Ids[8] = {
    TX_SETIMAGE0_I0_ID,
    TX_SETIMAGE0_I1_ID,
    TX_SETIMAGE0_I2_ID,
    TX_SETIMAGE0_I3_ID,
    TX_SETIMAGE0_I4_ID,
    TX_SETIMAGE0_I5_ID,
    TX_SETIMAGE0_I6_ID,
    TX_SETIMAGE0_I7_ID,
};

u8 GDTexImage1Ids[8] = {
    TX_SETIMAGE1_I0_ID,
    TX_SETIMAGE1_I1_ID,
    TX_SETIMAGE1_I2_ID,
    TX_SETIMAGE1_I3_ID,
    TX_SETIMAGE1_I4_ID,
    TX_SETIMAGE1_I5_ID,
    TX_SETIMAGE1_I6_ID,
    TX_SETIMAGE1_I7_ID,
};

u8 GDTexImage2Ids[8] = {
    TX_SETIMAGE2_I0_ID,
    TX_SETIMAGE2_I1_ID,
    TX_SETIMAGE2_I2_ID,
    TX_SETIMAGE2_I3_ID,
    TX_SETIMAGE2_I4_ID,
    TX_SETIMAGE2_I5_ID,
    TX_SETIMAGE2_I6_ID,
    TX_SETIMAGE2_I7_ID,
};

u8 GDTexImage3Ids[8] = {
    TX_SETIMAGE3_I0_ID,
    TX_SETIMAGE3_I1_ID,
    TX_SETIMAGE3_I2_ID,
    TX_SETIMAGE3_I3_ID,
    TX_SETIMAGE3_I4_ID,
    TX_SETIMAGE3_I5_ID,
    TX_SETIMAGE3_I6_ID,
    TX_SETIMAGE3_I7_ID,
};

u8 GDTexTlutIds[8] = {
    TX_SETTLUT_I0_ID,
    TX_SETTLUT_I1_ID,
    TX_SETTLUT_I2_ID,
    TX_SETTLUT_I3_ID,
    TX_SETTLUT_I4_ID,
    TX_SETTLUT_I5_ID,
    TX_SETTLUT_I6_ID,
    TX_SETTLUT_I7_ID,
};

u8 GD2HWFiltConv[] = {
    0, // TX_MIN_NEAREST,                     // GX_NEAR,
    4, // TX_MIN_LINEAR,                      // GX_LINEAR,
    1, // TX_MIN_NEAREST_MIPMAP_NEAREST,      // GX_NEAR_MIP_NEAR,
    5, // TX_MIN_LINEAR_MIPMAP_NEAREST,       // GX_LIN_MIP_NEAR,
    2, // TX_MIN_NEAREST_MIPMAP_LINEAR,       // GX_NEAR_MIP_LIN,
    6, // TX_MIN_LINEAR_MIPMAP_LINEAR,        // GX_LIN_MIP_LIN
};

#define GX_TMEM_HI  0x80000
#define GX_32k      0x08000
#define GX_8k       0x02000
u32 GXTlutRegions[20] = {
    GX_TMEM_HI + GX_32k*8 + GX_8k*0,
    GX_TMEM_HI + GX_32k*8 + GX_8k*1,
    GX_TMEM_HI + GX_32k*8 + GX_8k*2,
    GX_TMEM_HI + GX_32k*8 + GX_8k*3,
    GX_TMEM_HI + GX_32k*8 + GX_8k*4,
    GX_TMEM_HI + GX_32k*8 + GX_8k*5,
    GX_TMEM_HI + GX_32k*8 + GX_8k*6,
    GX_TMEM_HI + GX_32k*8 + GX_8k*7,
    GX_TMEM_HI + GX_32k*8 + GX_8k*8,
    GX_TMEM_HI + GX_32k*8 + GX_8k*9,
    GX_TMEM_HI + GX_32k*8 + GX_8k*10,
    GX_TMEM_HI + GX_32k*8 + GX_8k*11,
    GX_TMEM_HI + GX_32k*8 + GX_8k*12,
    GX_TMEM_HI + GX_32k*8 + GX_8k*13,
    GX_TMEM_HI + GX_32k*8 + GX_8k*14,
    GX_TMEM_HI + GX_32k*8 + GX_8k*15,
    GX_TMEM_HI + GX_32k*8 + GX_8k*16 + GX_32k*0,
    GX_TMEM_HI + GX_32k*8 + GX_8k*16 + GX_32k*1,
    GX_TMEM_HI + GX_32k*8 + GX_8k*16 + GX_32k*2,
    GX_TMEM_HI + GX_32k*8 + GX_8k*16 + GX_32k*3,
};

/********************************************************************************
 * Forward references.															*
 ********************************************************************************/

/********************************************************************************
 * Externs.																		*
 ********************************************************************************/

#ifndef __PLAT_WN32__
static void DirectWrite_u8(u8 data)
{
	GXWGFifo.u8 = data;
}

static void DirectWrite_s8(s8 data)
{
	GXWGFifo.s8 = data;
}

static void DirectWrite_u16(u16 data)
{
	GXWGFifo.u16 = data;
}

static void DirectWrite_s16(s16 data)
{
	GXWGFifo.s16 = data;
}

static void DirectWrite_u32(u32 data)
{
	GXWGFifo.u32 = data;
}

static void DirectWrite_s32(s32 data)
{
	GXWGFifo.s32 = data;
}

static void DirectWrite_f32(f32 data)
{
	GXWGFifo.f32 = data;
}

static void DirectWrite_u24(u32 data)
{
	GXWGFifo.u8 = (u8)((data >> 16) & 0xff);
	GXWGFifo.u8 = (u8)((data >>  8) & 0xff);
	GXWGFifo.u8 = (u8)((data >>  0) & 0xff);
}

static void DirectWriteBPCmd(u32 regval)
{
	GXWGFifo.u8 = GX_LOAD_BP_REG;
	GXWGFifo.u32 = regval;
}

static void DirectWriteCPCmd(u8 addr, u32 val)
{
	GXWGFifo.u8 = GX_LOAD_CP_REG;
	GXWGFifo.u8 = addr;
	GXWGFifo.u32 = val;
}
static void DirectWriteXFCmd(u16 addr, u32 val)
{
	GXWGFifo.u8 = GX_LOAD_XF_REG;
	GXWGFifo.u16 = 0;
	GXWGFifo.u16 = addr;
	GXWGFifo.u32 = val;
}

static void DirectWriteXFCmdHdr(u16 addr, u8 len)
{
	GXWGFifo.u8 = GX_LOAD_XF_REG;
	GXWGFifo.u16 = (u16)((len) - 1);
	GXWGFifo.u16 = addr;
}

static void DirectWriteXFIndxACmd(u16 addr, u8 len, u16 index)
{
	GXWGFifo.u8 = GX_LOAD_INDX_A;
	GXWGFifo.u16 = index;
	GXWGFifo.u16 = __XFAddrLen((addr), ((len)-1));
}

static void DirectWriteXFIndxBCmd(u16 addr, u8 len, u16 index)
{
	GXWGFifo.u8 = GX_LOAD_INDX_B;
	GXWGFifo.u16 = index;
	GXWGFifo.u16 = __XFAddrLen((addr), ((len)-1));
}

static void DirectWriteXFIndxCCmd(u16 addr, u8 len, u16 index)
{
	GXWGFifo.u8 = GX_LOAD_INDX_C;
	GXWGFifo.u16 = index;
	GXWGFifo.u16 = __XFAddrLen((addr), ((len)-1));
}

static void DirectWriteXFIndxDCmd(u16 addr, u8 len, u16 index)
{
	GXWGFifo.u8 = GX_LOAD_INDX_D;
	GXWGFifo.u16 = index;
	GXWGFifo.u16 = __XFAddrLen((addr), ((len)-1));
}
#endif		// __PLAT_WN32__

/********************************************************************************
 *																				*
 ********************************************************************************/

void begin ( void * p_buffer, int max_size )
{
#ifndef __PLAT_WN32__
	if ( p_buffer && max_size )
	{
		// We're writing to a display list.
		if ( g_use != USE_DL )
#endif		// __PLAT_WN32__
		{
			_u8 = GDWrite_u8;
			_s8 = GDWrite_s8;
			_u16 = GDWrite_u16;
			_s16 = GDWrite_s16;
			_u32 = GDWrite_u32;
			_s32 = GDWrite_s32;
			_f32 = GDWrite_f32;
			_u24 = GDWrite_u24;

			_BPCmd = GDWriteBPCmd;
			_CPCmd = GDWriteCPCmd;
			_XFCmd = GDWriteXFCmd;
			_XFCmdHdr = GDWriteXFCmdHdr;
			_XFIndxACmd = GDWriteXFIndxACmd;
			_XFIndxBCmd = GDWriteXFIndxBCmd;
			_XFIndxCCmd = GDWriteXFIndxCCmd;
			_XFIndxDCmd = GDWriteXFIndxDCmd;
			g_use = USE_DL;
		}

		GDInitGDLObj( &g_dl, p_buffer, max_size );
		GDSetCurrent( &g_dl );
#ifndef __PLAT_WN32__
	}
	else
	{
		// We're issuing direct commands.
		if ( g_use != USE_DIRECT )
		{
			_u8 = DirectWrite_u8;
			_s8 = DirectWrite_s8;
			_u16 = DirectWrite_u16;
			_s16 = DirectWrite_s16;
			_u32 = DirectWrite_u32;
			_s32 = DirectWrite_s32;
			_f32 = DirectWrite_f32;
			_u24 = DirectWrite_u24;

			_BPCmd = DirectWriteBPCmd;
			_CPCmd = DirectWriteCPCmd;
			_XFCmd = DirectWriteXFCmd;
			_XFCmdHdr = DirectWriteXFCmdHdr;
			_XFIndxACmd = DirectWriteXFIndxACmd;
			_XFIndxBCmd = DirectWriteXFIndxBCmd;
			_XFIndxCCmd = DirectWriteXFIndxCCmd;
			_XFIndxDCmd = DirectWriteXFIndxDCmd;
			g_use = USE_DIRECT;
		}

#ifndef __PLAT_WN32__
//		_BPCmd(TX_SETIMAGE1( MyTmemRegions[7][0] >> 5, 0, 0, 1,  GDTexImage1Ids[GX_TEXMAP7]));
//		if (MyTmemRegions[7][1] < GX_TMEM_MAX) // need to define this!
//		{
//			_BPCmd(TX_SETIMAGE2( MyTmemRegions[7][1] >> 5, 0, 0, GDTexImage2Ids[GX_TEXMAP7]));
//		}
#endif		// __PLAT_WN32__

	}
#endif		// __PLAT_WN32__
	tmem_count = 0;
	tlut_count = 0;
}
	
/********************************************************************************
 *																				*
 ********************************************************************************/
int end( void )
{
	int rv;

	if ( g_use == USE_DL )
	{
		GDPadCurr32();
		GDFlushCurrToMem();
		rv = GDGetCurrOffset(); 
	}
	else
	{
		rv = 0;
	}

#ifndef __PLAT_WN32__
	// Usage always goes back to direct after a display list is constructed.
	if ( g_use != USE_DIRECT )
	{
		_u8 = DirectWrite_u8;
		_s8 = DirectWrite_s8;
		_u16 = DirectWrite_u16;
		_s16 = DirectWrite_s16;
		_u32 = DirectWrite_u32;
		_s32 = DirectWrite_s32;
		_f32 = DirectWrite_f32;
		_u24 = DirectWrite_u24;

		_BPCmd = DirectWriteBPCmd;
		_CPCmd = DirectWriteCPCmd;
		_XFCmd = DirectWriteXFCmd;
		_XFCmdHdr = DirectWriteXFCmdHdr;
		_XFIndxACmd = DirectWriteXFIndxACmd;
		_XFIndxBCmd = DirectWriteXFIndxBCmd;
		_XFIndxCCmd = DirectWriteXFIndxCCmd;
		_XFIndxDCmd = DirectWriteXFIndxDCmd;
		g_use = USE_DIRECT;
	}
#endif		// __PLAT_WN32__

	return rv;
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetChanCtrl( GXChannelID	chan,
				  GXBool		enable,
				  GXColorSrc	amb_src,
				  GXColorSrc	mat_src,
				  u32			light_mask,
				  GXDiffuseFn	diff_fn,
				  GXAttnFn		attn_fn )
{
    u32 reg;

    reg =  XF_COLOR0CNTRL_F_PS( mat_src, enable, (light_mask & 0x0f), amb_src,
                                ((attn_fn==GX_AF_SPEC) ? GX_DF_NONE : diff_fn),
                                (attn_fn != GX_AF_NONE),
                                (attn_fn != GX_AF_SPEC),
                                ((light_mask >> 4) & 0x0f) );
    _XFCmd( (u16) (XF_COLOR0CNTRL_ID + (chan & 3)), reg );

    if (chan == GX_COLOR0A0 || chan == GX_COLOR1A1)
    {
        _XFCmd( (u16) (XF_COLOR0CNTRL_ID + ( chan - 2)), reg );
    }

//	GDSetChanCtrl( chan, enable, amb_src, mat_src, light_mask, diff_fn, attn_fn );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetTevOrder( GXTevStageID	evenStage,
				  GXTexCoordID	coord0,
				  GXTexMapID	map0,
				  GXChannelID	color0,
				  GXTexCoordID	coord1,
				  GXTexMapID	map1,
				  GXChannelID	color1 )
{
    static u8 c2r[] = {         // Convert enums to HW values
        RAS1_CC_0,              // GX_COLOR0
        RAS1_CC_1,              // GX_COLOR1
        RAS1_CC_0,              // GX_ALPHA0
        RAS1_CC_1,              // GX_ALPHA1
        RAS1_CC_0,              // GX_COLOR0A0
        RAS1_CC_1,              // GX_COLOR1A1
        RAS1_CC_Z,              // GX_COLOR_ZERO
        RAS1_CC_B,              // GX_COLOR_BUMP
        RAS1_CC_BN,             // GX_COLOR_BUMPN
        0,                      // 9
        0,                      // 10
        0,                      // 11
        0,                      // 12
        0,                      // 13
        0,                      // 14
        RAS1_CC_Z               // 15: GX_COLOR_NULL gets mapped here
    };

    _BPCmd( RAS1_TREF(
        (map0 & 7),                                             // map 0
        (coord0 & 7),                                           // tc 0
        ((map0 != GX_TEXMAP_NULL) && !(map0 & GX_TEX_DISABLE)), // enable 0
        c2r[ color0 & 0xf ],                                    // color 0
        (map1 & 7),                                             // map 1
        (coord1 & 7),                                           // tc 1
        ((map1 != GX_TEXMAP_NULL) && !(map1 & GX_TEX_DISABLE)), // enable 1
        c2r[ color1 & 0xf ],                                    // color 1
        (RAS1_TREF0_ID + (evenStage/2)) ));
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetTevAlphaInOpSwap( GXTevStageID	stage,
						  GXTevAlphaArg	a,
						  GXTevAlphaArg	b,
						  GXTevAlphaArg	c,
						  GXTevAlphaArg	d,
						  GXTevOp		op,
						  GXTevBias		bias,
						  GXTevScale	scale,
						  GXBool		clamp,
						  GXTevRegID	out_reg,
						  GXTevSwapSel	ras_sel,
						  GXTevSwapSel	tex_sel )
{
    if (op <= GX_TEV_SUB) 
    {
        _BPCmd( TEV_ALPHA_ENV( ras_sel, tex_sel,
                               d, c, b, a, 
                               bias, (op & 1), clamp, scale,
                               out_reg,
                               TEV_ALPHA_ENV_0_ID + 2 * (u32) stage ));
    } else {
        _BPCmd( TEV_ALPHA_ENV( ras_sel, tex_sel,
                               d, c, b, a,
                               GX_MAX_TEVBIAS, (op&1), clamp, ((op>>1)&3),
                               out_reg,
                               TEV_ALPHA_ENV_0_ID + 2 * (u32) stage ));
    }

//	GDSetTevAlphaCalcAndSwap( stage, a, b, c, d, op, bias, scale, clamp, out_reg, ras_sel, tex_sel );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetTevColorInOp( GXTevStageID	stage,
					  GXTevColorArg a,
					  GXTevColorArg b,
					  GXTevColorArg c,
					  GXTevColorArg d,
					  GXTevOp       op,
					  GXTevBias     bias,
					  GXTevScale    scale,
					  GXBool        clamp,
					  GXTevRegID    out_reg )
{
    if (op <= GX_TEV_SUB) 
    {
        _BPCmd( TEV_COLOR_ENV( d, c, b, a, 
                               bias, (op & 1), clamp, scale,
                               out_reg,
                               TEV_COLOR_ENV_0_ID + 2 * (u32) stage ));
    } else {
        _BPCmd( TEV_COLOR_ENV( d, c, b, a,
                               GX_MAX_TEVBIAS, (op&1), clamp, ((op>>1)&3),
                               out_reg,
                               TEV_COLOR_ENV_0_ID + 2 * (u32) stage ));
    }
	
//	GDSetTevColorCalc( stage, a, b, c, d, op, bias, scale, clamp, out_reg ); 
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetTevKSel( GXTevStageID	even_stage,
				 GXTevKColorSel color_even,
				 GXTevKAlphaSel alpha_even,
				 GXTevKColorSel color_odd,
				 GXTevKAlphaSel alpha_odd )
{
	_BPCmd( SS_MASK( TEV_KSEL_MASK_KCSEL ) );
	_BPCmd( TEV_KSEL( 0, 0, color_even, alpha_even, color_odd, alpha_odd, TEV_KSEL_0_ID + (even_stage/2) ));
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetTevKColor( GXTevKColorID id,
				   GXColor color )
{
    u32 regRA, regBG;

    regRA = TEV_REGISTERL( color.r, color.a, TEV_KONSTANT_REG,
                           TEV_REGISTERL_0_ID + id * 2 );
    regBG = TEV_REGISTERH( color.b, color.g, TEV_KONSTANT_REG, 
                           TEV_REGISTERH_0_ID + id * 2 );
    _BPCmd( regRA );
    _BPCmd( regBG );
    // The KColor registers do not have the load delay bug.

//	GDSetTevKColor( id, color );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetTevColor( GXTevRegID	reg,
				  GXColor		color )
{
    u32 regRA, regBG;

    regRA = TEV_REGISTERL( color.r, color.a, TEV_COLOR_REG, 
                           TEV_REGISTERL_0_ID + reg * 2 );
    regBG = TEV_REGISTERH( color.b, color.g, TEV_COLOR_REG, 
                           TEV_REGISTERH_0_ID + reg * 2 );
    _BPCmd( regRA );
	_BPCmd( regBG );
    // Due to color load delay bug, must put two more BP commands here...
    _BPCmd( regBG );
    _BPCmd( regBG );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetTexCoordGen( GXTexCoordID	dst_coord,
					 GXTexGenType	func,
					 GXTexGenSrc	src_param,
					 GXBool			normalize,
					 u32			postmtx )
{
    u32     form;
    u32     tgType;
    u32     proj;
    u32     row;
    u32     embossRow;
    u32     embossLit;
    
    form      = XF_TEX_AB11;
    proj      = XF_TEX_ST;
    row       = XF_TEX0_INROW;
    embossRow = XF_TEX0_INROW;
    embossLit = 0;

#ifndef __PLAT_WN32__
    ASSERTMSG((dst_coord < GX_MAX_TEXCOORD),
              "GDSetTexCoordGen: invalid texcoord ID");
#endif		// __PLAT_WN32__

    switch (src_param) {
      case GX_TG_POS:     row = XF_GEOM_INROW;       form = XF_TEX_ABC1; break;
      case GX_TG_NRM:     row = XF_NORMAL_INROW;     form = XF_TEX_ABC1; break; 
      case GX_TG_BINRM:   row = XF_BINORMAL_T_INROW; form = XF_TEX_ABC1; break;
      case GX_TG_TANGENT: row = XF_BINORMAL_B_INROW; form = XF_TEX_ABC1; break;
      case GX_TG_COLOR0:  row = XF_COLORS_INROW; break;
      case GX_TG_COLOR1:  row = XF_COLORS_INROW; break;
      case GX_TG_TEX0:    row = XF_TEX0_INROW;   break;
      case GX_TG_TEX1:    row = XF_TEX1_INROW;   break;
      case GX_TG_TEX2:    row = XF_TEX2_INROW;   break;
      case GX_TG_TEX3:    row = XF_TEX3_INROW;   break;
      case GX_TG_TEX4:    row = XF_TEX4_INROW;   break;
      case GX_TG_TEX5:    row = XF_TEX5_INROW;   break;
      case GX_TG_TEX6:    row = XF_TEX6_INROW;   break;
      case GX_TG_TEX7:    row = XF_TEX7_INROW;   break;
      case GX_TG_TEXCOORD0: embossRow = 0; break;
      case GX_TG_TEXCOORD1: embossRow = 1; break;
      case GX_TG_TEXCOORD2: embossRow = 2; break;
      case GX_TG_TEXCOORD3: embossRow = 3; break;
      case GX_TG_TEXCOORD4: embossRow = 4; break;
      case GX_TG_TEXCOORD5: embossRow = 5; break;
      case GX_TG_TEXCOORD6: embossRow = 6; break;
      default:
#ifndef __PLAT_WN32__
        ASSERTMSG(0, "GDSetTexCoordGen: invalid texgen source");
#endif		// __PLAT_WN32__
        break;
    }

    switch (func) {
      case GX_TG_MTX2x4:
        tgType = XF_TEXGEN_REGULAR;
        break;
        
      case GX_TG_MTX3x4:
        tgType = XF_TEXGEN_REGULAR;
        proj   = XF_TEX_STQ;
        break;

      case GX_TG_BUMP0:
      case GX_TG_BUMP1:
      case GX_TG_BUMP2:
      case GX_TG_BUMP3:
      case GX_TG_BUMP4:
      case GX_TG_BUMP5:
      case GX_TG_BUMP6:
      case GX_TG_BUMP7:
#ifndef __PLAT_WN32__
        ASSERTMSG((src_param >= GX_TG_TEXCOORD0) && 
                  (src_param <= GX_TG_TEXCOORD6),
                  "GDSetTexCoordGen: invalid emboss source");
#endif		// __PLAT_WN32__
        tgType = XF_TEXGEN_EMBOSS_MAP;
        embossLit = (u32) (func - GX_TG_BUMP0);
        break;

      case GX_TG_SRTG:
        if (src_param == GX_TG_COLOR0) {
            tgType = XF_TEXGEN_COLOR_STRGBC0;
        } else {
            tgType = XF_TEXGEN_COLOR_STRGBC1;
        }
        break;

      default:
#ifndef __PLAT_WN32__
        ASSERTMSG(0, "GDSetTexCoordGen: invalid texgen function");
#endif		// __PLAT_WN32__
        tgType = XF_TEXGEN_REGULAR;
        break;
    }

    _XFCmd( (u16) (XF_TEX0_ID + dst_coord), XF_TEX( proj, form, tgType, row, embossRow, embossLit ));

    //---------------------------------------------------------------------
    // Update DUALTEX state
    //---------------------------------------------------------------------

    _XFCmd( (u16) (XF_DUALTEX0_ID + dst_coord), XF_DUALTEX((postmtx - GX_PTTEXMTX0), normalize ));

//	GDSetTexCoordGen ( dst_coord, func, src_param, normalize, postmtx ); 
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void UploadTexture( void *			image_ptr,
					u16				width,
					u16				height,
					GXTexFmt		format,
					GXTexWrapMode	wrap_s,
					GXTexWrapMode	wrap_t,
					GXBool			mipmap,
					GXTexFilter		min_filt,
					GXTexFilter		mag_filt,
					f32				min_lod,
					f32				max_lod,
					f32				lod_bias,
					GXBool			bias_clamp,
					GXBool			do_edge_lod,
					GXAnisotropy	max_aniso,
					GXTexMapID		id )
{
#if 0
	GXTexObj to;

    GXInitTexObj(
		&to,
		image_ptr,
		width,
		height,
		format,
		wrap_s,
		wrap_t,
		mipmap );
	
	GXInitTexObjLOD(
		&to,
		min_filt,
		mag_filt,
		min_lod,
		max_lod,
		lod_bias,
		bias_clamp,
		do_edge_lod,
		max_aniso );

    GXLoadTexObj( &to, id );
#else

//	GDSetTexLookupMode(	id, wrap_s, wrap_t, min_filt, mag_filt, min_lod, max_lod, lod_bias, bias_clamp, do_edge_lod, max_aniso );

    _BPCmd(TX_SETMODE0(wrap_s,
                       wrap_t,
                       (mag_filt == GX_LINEAR),
                       GD2HWFiltConv[min_filt],
                       !do_edge_lod,
                       (u8)((s8)(lod_bias*32.0f)),
                       max_aniso,
                       bias_clamp,
                       GDTexMode0Ids[id]));
    _BPCmd(TX_SETMODE1((u8)(min_lod*16.0f),
                       (u8)(max_lod*16.0f),
                       GDTexMode1Ids[id]));

//	GDSetTexImgAttr(	id, width, height, format );

    _BPCmd(TX_SETIMAGE0(width - 1,
                        height - 1,
                        format,
                        GDTexImage0Ids[id]));

//	GDSetTexCached(		id, MyTmemRegions[tmem_count][0], GX_TEXCACHE_32K,
//							MyTmemRegions[tmem_count][1], GX_TEXCACHE_32K );

    _BPCmd(TX_SETIMAGE1( MyTmemRegions[tmem_count][0] >> 5,
						 GX_TEXCACHE_32K+3, 
						 GX_TEXCACHE_32K+3, 
                         0, 
                         GDTexImage1Ids[id]));
    if (GX_TEXCACHE_32K != GX_TEXCACHE_NONE && MyTmemRegions[tmem_count][1] < GX_TMEM_MAX)
    {
        _BPCmd(TX_SETIMAGE2( MyTmemRegions[tmem_count][1] >> 5,
							 GX_TEXCACHE_32K+3,
							 GX_TEXCACHE_32K+3,
                             GDTexImage2Ids[id]));
    }

	tmem_count++;
	tmem_count &= 7;

#ifdef __PLAT_WN32__
//			GDSetTexImgPtrRaw(	id, (uint32)image_ptr );

			_BPCmd(TX_SETIMAGE3((uint32)image_ptr, GDTexImage3Ids[id]));

#else
//			GDSetTexImgPtr(		id, image_ptr );

			_BPCmd(TX_SETIMAGE3(OSCachedToPhysical(image_ptr)>>5,
									  GDTexImage3Ids[id]));
#endif		// __PLAT_WN32__
#endif
}

void UploadTexture( void *			image_ptr,
					u16				width,
					u16				height,
					GXCITexFmt		format,
					GXTexWrapMode	wrap_s,
					GXTexWrapMode	wrap_t,
					GXBool			mipmap,
					GXTexFilter		min_filt,
					GXTexFilter		mag_filt,
					f32				min_lod,
					f32				max_lod,
					f32				lod_bias,
					GXBool			bias_clamp,
					GXBool			do_edge_lod,
					GXAnisotropy	max_aniso,
					GXTexMapID		id )
{
	UploadTexture( image_ptr,
				   width,
				   height,
				   (GXTexFmt)format,
				   wrap_s,
				   wrap_t, 
				   mipmap,
				   min_filt,
				   mag_filt,
				   min_lod,
				   max_lod,
				   lod_bias,
				   bias_clamp,
				   do_edge_lod,
				   max_aniso,
				   id );
};
						
/********************************************************************************
 *																				*
 ********************************************************************************/
void UploadPalette( void *		tlut_ptr,
					GXTlutFmt	tlut_format,
					GXTlutSize	tlut_size,
					GXTexMapID	id )
{
//    GDSetTexTlut ( GX_TEXMAP0, TLUT0_ADDR, GX_TL_RGB565 );
    _BPCmd(TX_SETTLUT( (GXTlutRegions[tlut_count]-GX_TMEM_HALF) >> 9, tlut_format, GDTexTlutIds[id]));

//    GDLoadTlutRaw ( 0, TLUT0_ADDR, GX_TLUT_256 );
#ifndef __PLAT_WN32__
    ASSERTMSG((tlut_size <= 1024), "GDLoadTlut: invalid TLUT size");
#endif		// __PLAT_WN32__

    // Flush the texture state (without modifying the indirect mask)
    _BPCmd(SS_MASK(0xffff00));
	_BPCmd((BU_IMASK_ID << 24));

#ifdef __PLAT_WN32__
	_BPCmd(TX_LOADTLUT0( ((uint32)tlut_ptr>>5), TX_LOADTLUT0_ID));
#else
	_BPCmd(TX_LOADTLUT0( (OSCachedToPhysical(tlut_ptr)>>5), TX_LOADTLUT0_ID));
#endif		// __PLAT_WN32__

    // Writing to this register will initiate the actual loading:
    _BPCmd(TX_LOADTLUT1( (GXTlutRegions[tlut_count]-GX_TMEM_HALF) >> 9, tlut_size,
                             TX_LOADTLUT1_ID));

    // Flush the texture state (without modifying the indirect mask)
    _BPCmd(SS_MASK(0xffff00));
    _BPCmd((BU_IMASK_ID << 24));

	tlut_count++;
	tlut_count %= 20;
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetTexCoordScale( GXTexCoordID	coord,
					   GXBool		enable,
					   u16			ss,
					   u16			ts )
{
//	GDSetTexCoordScaleAndTOEs( coord, ss, GX_DISABLE, GX_DISABLE, ts, GX_DISABLE, GX_DISABLE, GX_FALSE, GX_FALSE );

    _BPCmd(SU_TS0( ss - 1, GX_DISABLE, GX_DISABLE, GX_TRUE, GX_TRUE,
                         SU_SSIZE0_ID + (u32) coord * 2));
    _BPCmd(SU_TS1( ts - 1, GX_DISABLE, GX_DISABLE,
                         SU_TSIZE0_ID + (u32) coord * 2));
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetBlendMode( GXBlendMode		type,
                   GXBlendFactor	src_factor,
                   GXBlendFactor	dst_factor,
                   GXLogicOp		op,
				   GXBool			color_update_enable,
				   GXBool			alpha_update_enable,
				   GXBool			dither_enable )
{
//	GDSetBlendModeEtc( type, src_factor, dst_factor, op, GX_TRUE, GX_TRUE, GX_TRUE );

    _BPCmd( PE_CMODE0( 
        ((type == GX_BM_BLEND) || (type == GX_BM_SUBTRACT)),
        (type == GX_BM_LOGIC),
		GX_FALSE,	//dither_enable,
		color_update_enable,
		GX_FALSE,	//alpha_update_enable,
        dst_factor,
        src_factor,
        (type == GX_BM_SUBTRACT),
        op,
        PE_CMODE0_ID ));
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetTexChanTevIndCull( u8			nTexGens,
						   u8			nChans,
						   u8			nTevs,
						   u8			nInds,
						   GXCullMode	cm )
{
//	GDSetGenMode2( nTexGens, nChans, nTevs, nInds, cm );

	static u8 cm2hw[] = { 0, 2, 1, 3 };
	_XFCmd( XF_NUMTEX_ID, XF_NUMTEX( nTexGens ));
	_XFCmd( XF_NUMCOLORS_ID, XF_NUMCOLORS( nChans ));
	_BPCmd( GEN_MODE( nTexGens, nChans, 0, (nTevs-1), cm2hw[cm], nInds, 0, GEN_MODE_ID ));
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetChanAmbColor( GXChannelID chan,
					  GXColor color )
{
    _XFCmd( (u16) (XF_AMBIENT0_ID + (chan & 1)),
                  (((u32)color.r << 24) | 
                   ((u32)color.g << 16) |
                   ((u32)color.b <<  8) | 
                   ((u32)color.a)) );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetChanMatColor( GXChannelID chan,
					  GXColor color )
{
    _XFCmd( (u16) (XF_MATERIAL0_ID + (chan & 1)),
                  (((u32)color.r << 24) | 
                   ((u32)color.g << 16) |
                   ((u32)color.b <<  8) | 
                   ((u32)color.a)) );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetPointSize( u8			pointSize,
				   GXTexOffset	texOffsets )
{
	//GDSetLPSize
    _BPCmd( SU_LPSIZE( pointSize, pointSize, texOffsets, texOffsets, GX_FALSE, SU_LPSIZE_ID ));
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetFog( GXFogType	type,
			 f32		startz,
			 f32		endz,
			 f32		nearz,
			 f32		farz,
			 GXColor	color )
{
#ifndef __PLAT_WN32__
    f32     A, B, B_mant, C, A_f;
    u32     b_expn, b_m, a_hex, c_hex;
    u32     fsel, proj;

    ASSERTMSG(farz >= 0, "GDSetFog: The farz should be positive value");
    ASSERTMSG(farz >= nearz, "GDSetFog: The farz should be larger than nearz");

    fsel = (u32)(type & 0x07);
    proj = (u32)((type >> 3) & 0x01);

    if ( proj ) // ORTHOGRAPHIC
    {
        // Calculate constants a and c (TEV HW requirements).
        if ((farz == nearz) || (endz == startz))
        {
            // take care of the odd-ball case.
            A_f = 0.0f;
            C   = 0.0f;
        }
        else
        {
            A   = 1.0F / (endz - startz);
            A_f = (farz - nearz) * A;
            C   = (startz - nearz) * A;
        }

        b_expn = 0;
        b_m    = 0;
    }
    else // PERSPECTIVE
    {
        // Calculate constants a, b, and c (TEV HW requirements).
        if ((farz == nearz) || (endz == startz))
        {
            // take care of the odd-ball case.
            A = 0.0f;
            B = 0.5f;
            C = 0.0f;
        }
        else
        {
            A = (farz * nearz) / ((farz-nearz) * (endz-startz));
            B = farz / (farz-nearz);
            C = startz / (endz-startz);
        }
        
        B_mant = B;
        b_expn = 1;
        while (B_mant > 1.0) 
        {
            B_mant /= 2;
            b_expn++;
        }
        while ((B_mant > 0) && (B_mant < 0.5))
        {
            B_mant *= 2;
            b_expn--;
        }

        A_f   = A / (1 << (b_expn));
        b_m   = (u32)(B_mant * 8388638);
    }

    a_hex = (* (u32 *) &A_f);
    c_hex = (* (u32 *) &C);

    // Write out register values.
    _BPCmd( TEV_FOG_PARAM_0_PS( (a_hex >> 12), TEV_FOG_PARAM_0_ID ));

	_BPCmd( TEV_FOG_PARAM_1( b_m, TEV_FOG_PARAM_1_ID ));
	_BPCmd( TEV_FOG_PARAM_2( b_expn, TEV_FOG_PARAM_2_ID ));

	_BPCmd( TEV_FOG_PARAM_3_PS( (c_hex >> 12), proj, fsel, 
                               TEV_FOG_PARAM_3_ID ));

	_BPCmd( TEV_FOG_COLOR( color.b, color.g, color.r, TEV_FOG_COLOR_ID ));
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetFogColor( GXColor color )
{
	_BPCmd( TEV_FOG_COLOR( color.b, color.g, color.r, TEV_FOG_COLOR_ID ));
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Begin( GXPrimitive type,
			GXVtxFmt vtxfmt,
			u16 nverts )
{
    _u8((u8) (vtxfmt | type));
    _u16(nverts);
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void End( void )
{
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position3f32( f32 x,
				   f32 y,
				   f32 z )
{
	_f32( x );
	_f32( y );
	_f32( z );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position3u8( u8 x,
				  u8 y,
				  u8 z )
{
	_u8( x );
	_u8( y );
	_u8( z );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position3s8( s8 x,
				  s8 y,
				  s8 z )
{
	_s8( x );
	_s8( y );
	_s8( z );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position3u16( u16 x,
				   u16 y,
				   u16 z )
{
	_u16( x );
	_u16( y );
	_u16( z );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position3s16( s16 x,
				   s16 y,
				   s16 z )
{
	_s16( x );
	_s16( y );
	_s16( z );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position2f32( f32 x,
				   f32 y )
{
	_f32( x );
	_f32( y );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position2u8( u8 x,
				  u8 y )
{
	_u8( x );
	_u8( y );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position2s8( s8 x,
				  s8 y )
{
	_s8( x );
	_s8( y );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position2u16( u16 x,
				   u16 y )
{
	_u16( x );
	_u16( y );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position2s16( s16 x,
				   s16 y )
{
	_s16( x );
	_s16( y );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position1x16( u16 i )
{
	_u16( i );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Position1x8( u8 i )
{
	_u8( i );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Normal3f32( f32 x,
				 f32 y,
				 f32 z )
{
	_f32( x );
	_f32( y );
	_f32( z );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Normal3s16( s16 x,
				 s16 y,
				 s16 z )
{
	_s16( x );
	_s16( y );
	_s16( z );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Normal3s8( s8 x,
				s8 y,
				s8 z )
{
	_s8( x );
	_s8( y );
	_s8( z );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Normal1x16( u16 i )
{
	_u16( i );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Normal1x8( u8 i )
{
	_u8( i );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Color4u8( u8 r,
			   u8 g,
			   u8 b,
			   u8 a )
{
	_u8( r );
	_u8( g );
	_u8( b );
	_u8( a );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Color1u32( u32 rgba )
{
	_u32( rgba );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Color1x16( u16 i )
{
	_u16( i );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Color1x8( u8 i )
{
	_u8( i );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord2f32( f32 s,
				   f32 t )
{
	_f32( s );
	_f32( t );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord2s16( s16 s,
				   s16 t )
{
	_s16( s );
	_s16( t );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord2u16( u16 s,
				   u16 t )
{
	_u16( s );
	_u16( t );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord2s8( s8 s,
				  s8 t )
{
	_s8( s );
	_s8( t );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord2u8( u8 s,
				  u8 t )
{
	_u8( s );
	_u8( t );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord1f32( f32 s )
{
	_f32( s );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord1s16( s16 s )
{
	_s16( s );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord1u16( u16 s )
{
	_u16( s );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord1s8( s8 s )
{
	_s8( s );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord1u8( u8 s )
{
	_u8( s );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord1x16( u16 i )
{
	_u16( i );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void TexCoord1x8( u8 i )
{
	_u8( i );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void MatrixIndex1u8( u8 i )
{
	_u8( i );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetZMode( GXBool     compare_enable,
			   GXCompare  func,
			   GXBool     update_enable )
{
    _BPCmd( PE_ZMODE( compare_enable, func, update_enable, PE_ZMODE_ID ));
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetVtxDesc( int items, ... )
{
    u32 nnorms = 0;
    u32 ncols  = 0;
    u32 ntexs  = 0;

    u32 pnMtxIdx = GX_NONE;
    u32 txMtxIdxMask = 0;
    u32 posn = GX_DIRECT;
    u32 norm = GX_NONE;
    u32 col0 = GX_NONE;
    u32 col1 = GX_NONE;
    u32 tex0 = GX_NONE;
    u32 tex1 = GX_NONE;
    u32 tex2 = GX_NONE;
    u32 tex3 = GX_NONE;
    u32 tex4 = GX_NONE;
    u32 tex5 = GX_NONE;
    u32 tex6 = GX_NONE;
    u32 tex7 = GX_NONE;

	// Start the arguments.
	va_list	vl;
	va_start( vl, items );

	for ( int item = 0; item < items; item++ )
	{
		// Get this item.
		GXAttr		attribute	= va_arg( vl, GXAttr );
		GXAttrType	type		= va_arg( vl, GXAttrType );

#ifndef __PLAT_WN32__
        ASSERTMSG(((attribute >= GX_VA_PNMTXIDX) && 
                   (attribute <= GX_VA_MAX_ATTR)),
                  "GDSetVtxDescv: invalid attribute");

        ASSERTMSG(((type >= GX_NONE) && 
                   (type <= GX_INDEX16)),
                  "GDSetVtxDescv: invalid type");

        ASSERTMSG(((attribute >= GX_VA_PNMTXIDX) &&
                   (attribute <= GX_VA_TEX7MTXIDX)) ?
                  ((type == GX_NONE) ||
                   (type == GX_DIRECT)) : 1,
                  "GDSetVtxDescv: invalid type for given attribute");
#endif		// __PLAT_WN32__

        switch (attribute) {

          case GX_VA_PNMTXIDX: pnMtxIdx = type; break;

          case GX_VA_TEX0MTXIDX:
            txMtxIdxMask = (txMtxIdxMask &   ~1) | (type << 0); break;
          case GX_VA_TEX1MTXIDX:
            txMtxIdxMask = (txMtxIdxMask &   ~2) | (type << 1); break;
          case GX_VA_TEX2MTXIDX:
            txMtxIdxMask = (txMtxIdxMask &   ~4) | (type << 2); break;
          case GX_VA_TEX3MTXIDX:
            txMtxIdxMask = (txMtxIdxMask &   ~8) | (type << 3); break;
          case GX_VA_TEX4MTXIDX:
            txMtxIdxMask = (txMtxIdxMask &  ~16) | (type << 4); break;
          case GX_VA_TEX5MTXIDX:
            txMtxIdxMask = (txMtxIdxMask &  ~32) | (type << 5); break;
          case GX_VA_TEX6MTXIDX:
            txMtxIdxMask = (txMtxIdxMask &  ~64) | (type << 6); break;
          case GX_VA_TEX7MTXIDX:
            txMtxIdxMask = (txMtxIdxMask & ~128) | (type << 7); break;

          case GX_VA_POS: posn = type; break;

          case GX_VA_NRM:
            if (type != GX_NONE)
                { norm = type; nnorms = 1; }
            break;
          case GX_VA_NBT:
            if (type != GX_NONE)
                { norm = type; nnorms = 2; }
            break;

          case GX_VA_CLR0: col0=type; ncols+=(col0 != GX_NONE); break;
          case GX_VA_CLR1: col1=type; ncols+=(col1 != GX_NONE); break;

          case GX_VA_TEX0: tex0=type; ntexs+=(tex0 != GX_NONE); break;
          case GX_VA_TEX1: tex1=type; ntexs+=(tex1 != GX_NONE); break;
          case GX_VA_TEX2: tex2=type; ntexs+=(tex2 != GX_NONE); break;
          case GX_VA_TEX3: tex3=type; ntexs+=(tex3 != GX_NONE); break;
          case GX_VA_TEX4: tex4=type; ntexs+=(tex4 != GX_NONE); break;
          case GX_VA_TEX5: tex5=type; ntexs+=(tex5 != GX_NONE); break;
          case GX_VA_TEX6: tex6=type; ntexs+=(tex6 != GX_NONE); break;
          case GX_VA_TEX7: tex7=type; ntexs+=(tex7 != GX_NONE); break;
          default: break;
        }
	}

	va_end( vl );

	// Write out the attributes.
    _CPCmd( CP_VCD_LO_ID, CP_VCD_REG_LO_PS( pnMtxIdx, txMtxIdxMask, posn, norm, col0, col1 ));

    _CPCmd( CP_VCD_HI_ID, CP_VCD_REG_HI( tex0, tex1, tex2, tex3, tex4, tex5, tex6, tex7 ));

    _XFCmd( XF_INVTXSPEC_ID, XF_INVTXSPEC( ncols, nnorms, ntexs ));
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void LoadPosMtxImm( const f32 mtx[3][4],
					u32 id )
{
    _XFCmdHdr(__PosMtxToXFMem(id), (u8)(3*4));
    
    _f32(mtx[0][0]);
    _f32(mtx[0][1]);
    _f32(mtx[0][2]);
    _f32(mtx[0][3]);
        
    _f32(mtx[1][0]);
    _f32(mtx[1][1]);
    _f32(mtx[1][2]);
    _f32(mtx[1][3]);
        
    _f32(mtx[2][0]);
    _f32(mtx[2][1]);
    _f32(mtx[2][2]);
    _f32(mtx[2][3]);
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void LoadNrmMtxImm ( const f32 mtx[3][4],
					 u32 id )
{
    _XFCmdHdr(__NrmMtxToXFMem(id), (u8)(3*3));

    _f32(mtx[0][0]);
    _f32(mtx[0][1]);
    _f32(mtx[0][2]);
        
    _f32(mtx[1][0]);
    _f32(mtx[1][1]);
    _f32(mtx[1][2]);
        
    _f32(mtx[2][0]);
    _f32(mtx[2][1]);
    _f32(mtx[2][2]);
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void LoadTexMtxImm( const f32 mtx[3][4],
					u32 id,
					GXTexMtxType type )
{
    u16     addr;
    u8      count;

    if (id >= GX_PTTEXMTX0) 
    {
#ifndef __PLAT_WN32__
        ASSERTMSG(type == GX_MTX3x4, "GDLoadTexMtxImm: invalid matrix type");
#endif		// __PLAT_WN32__
        addr  = __DTTMtxToXFMem(id);
        count = (u8)(3*4);
    } else {
        addr  = __TexMtxToXFMem(id);
        count = (u8)((type == GX_MTX2x4) ? (2*4) : (3*4));
    }

    _XFCmdHdr(addr, count);

    _f32(mtx[0][0]);
    _f32(mtx[0][1]);
    _f32(mtx[0][2]);
    _f32(mtx[0][3]);
        
    _f32(mtx[1][0]);
    _f32(mtx[1][1]);
    _f32(mtx[1][2]);
    _f32(mtx[1][3]);
               
    if (type == GX_MTX3x4) 
    {
        _f32(mtx[2][0]);
        _f32(mtx[2][1]);
        _f32(mtx[2][2]);
        _f32(mtx[2][3]);
    }
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetAlphaCompare( GXCompare	comp0,
					   u8			ref0,
					   GXAlphaOp	op,
					   GXCompare	comp1,
					   u8			ref1 )
{
    _BPCmd( TEV_ALPHAFUNC( ref0, ref1, comp0, comp1, op, TEV_ALPHAFUNC_ID ));
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetCurrMtxPosTex03( u32 pn,
						 u32 t0,
						 u32 t1,
						 u32 t2,
						 u32 t3 )
{
	u32 regA;

    regA = MATIDX_REG_A(pn, t0, t1, t2, t3);

	_CPCmd( CP_MATINDEX_A, regA ); // W1

	_XFCmdHdr( XF_MATINDEX_A, 1 );
	_u32( regA ); // W1
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetCurrMtxTex47( u32 t4,
					  u32 t5,
					  u32 t6,
					  u32 t7 )
{
    u32 regB;

    regB = MATIDX_REG_B(t4, t5, t6, t7);

    _CPCmd( CP_MATINDEX_B, regB ); // W2

    _XFCmdHdr( XF_MATINDEX_B, 1 );
    _u32( regB ); // W2
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetProjection( const				f32 mtx[4][4],
					GXProjectionType	type )
{
//    u32 c;
//
//    c = ( type == GX_ORTHOGRAPHIC ) ? 3u : 2u;
//
//    GDWriteXFCmdHdr( XF_PROJECTION_ID, 7 );
//    GDWrite_f32( mtx[0][0] );
//    GDWrite_f32( mtx[0][c] );
//    GDWrite_f32( mtx[1][1] );
//    GDWrite_f32( mtx[1][c] );
//    GDWrite_f32( mtx[2][2] );
//    GDWrite_f32( mtx[2][3] );
//    GDWrite_u32( type );

#ifndef __PLAT_WN32__
	GXSetProjection( mtx, type );
#endif		// __PLAT_WN32__
//    u32 c;
//
//    c = ( type == GX_ORTHOGRAPHIC ) ? 3u : 2u;
//
//    _XFCmdHdr( XF_PROJECTION_ID, 7 );
//    _f32( mtx[0][0] );
//    _f32( mtx[0][c] );
//    _f32( mtx[1][1] );
//    _f32( mtx[1][c] );
//    _f32( mtx[2][2] );
//    _f32( mtx[2][3] );
//    _u32( type );
//
//	g_projection[0] = mtx[0][0];
//	g_projection[1] = mtx[0][c];
//	g_projection[2] = mtx[1][1];
//	g_projection[3] = mtx[1][c];
//	g_projection[4] = mtx[2][2];
//	g_projection[5] = mtx[2][3];
//	*((u32 *)&g_projection[6]) = type;
}

/********************************************************************************
 *																				*
 ********************************************************************************/
#define TEV_KSEL_MASK_SWAPMODETABLE \
    (( 0x000003 << TEV_KSEL_XRB_SHIFT ) | \
     ( 0x000003 << TEV_KSEL_XGA_SHIFT ))

void SetTevSwapModeTable( GXTevSwapSel    table,
						  GXTevColorChan  red,
						  GXTevColorChan  green,
						  GXTevColorChan  blue,
						  GXTevColorChan  alpha )
{
    // Mask to avoid touching the konstant-color selects
    _BPCmd( SS_MASK( TEV_KSEL_MASK_SWAPMODETABLE ) );
    _BPCmd( TEV_KSEL( red, green, 0, 0, 0, 0, TEV_KSEL_0_ID + (table * 2) ));
    _BPCmd( SS_MASK( TEV_KSEL_MASK_SWAPMODETABLE ) );
    _BPCmd( TEV_KSEL( blue, alpha, 0, 0, 0, 0, TEV_KSEL_0_ID + (table * 2) + 1 ));
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetScissorBoxOffset( s32	x_off,
						  s32	y_off )
{
#ifndef __PLAT_WN32__
	GXSetScissorBoxOffset( x_off, y_off );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetScissor( u32	left,
				 u32	top,
				 u32	wd,
				 u32	ht )
{
#ifndef __PLAT_WN32__
	GXSetScissor( left, top, wd, ht );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetDispCopyGamma( GXGamma gamma )
{
#ifndef __PLAT_WN32__
	GXSetDispCopyGamma( gamma );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
//typedef void (*GXBreakPtCallback)(void);
GXBreakPtCallback SetBreakPtCallback( GXBreakPtCallback cb )
{
#ifndef __PLAT_WN32__
	return GXSetBreakPtCallback( cb );
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Flush( void )
{
#ifndef __PLAT_WN32__
	GXFlush();
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void GetFifoPtrs( GXFifoObj*  fifo,
				  void**      readPtr,
				  void**      writePtr )
{
#ifndef __PLAT_WN32__
	GXGetFifoPtrs( fifo, readPtr, writePtr );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
GXFifoObj* GetCPUFifo( void )
{
#ifndef __PLAT_WN32__
	return GXGetCPUFifo();
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
GXFifoObj* GetGPFifo( void )
{
#ifndef __PLAT_WN32__
	return GXGetGPFifo();
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void EnableBreakPt( void* breakPtr )
{
#ifndef __PLAT_WN32__
	GXEnableBreakPt( breakPtr );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void DisableBreakPt( void )
{
#ifndef __PLAT_WN32__
	GXDisableBreakPt();
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetGPMetric( GXPerf0	perf0,
				  GXPerf1	perf1 )
{
#ifndef __PLAT_WN32__
	GXSetGPMetric( perf0, perf1 );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void ClearGPMetric( void )
{
#ifndef __PLAT_WN32__
	GXClearGPMetric();
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void ReadGPMetric( u32 *	cnt0,
				   u32 *	cnt1 )
{
#ifndef __PLAT_WN32__
	GXReadGPMetric( cnt0, cnt1 );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetDrawSync( u16 token )
{
#ifndef __PLAT_WN32__
	GXSetDrawSync( token );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
u16 ReadDrawSync( void )
{
#ifndef __PLAT_WN32__
	return GXReadDrawSync();
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetCopyFilter( GXBool		aa,
					const u8	sample_pattern[12][2],
					GXBool		vf,
					const u8	vfilter[7] )
{
#ifndef __PLAT_WN32__
	GXSetCopyFilter( aa, sample_pattern, vf, vfilter );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void CopyDisp( void *	dest,
			   GXBool	clear )
{
#ifndef __PLAT_WN32__
	GXCopyDisp( dest, clear );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetCopyClear( GXColor	clear_clr,
				   u32		clear_z )
{
#ifndef __PLAT_WN32__
	GXSetCopyClear( clear_clr, clear_z );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void PokeZ( u16 x,
			u16 y,
			u32 z )
{
#ifndef __PLAT_WN32__
	GXPokeZ( x, y, z );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void PeekZ( u16 x,
			u16 y,
			u32* z )
{
#ifndef __PLAT_WN32__
	GXPeekZ( x, y, z );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void PokeARGB( u16 x,
			   u16 y,
			   u32 color )
{
#ifndef __PLAT_WN32__
	GXPokeARGB( x, y, color );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void PeekARGB( u16 x,
			   u16 y,
			   u32* color )
{
#ifndef __PLAT_WN32__
	GXPeekARGB( x, y, color );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetClipMode( GXClipMode mode )
{
#ifndef __PLAT_WN32__
	GXSetClipMode( mode );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetZCompLoc( GXBool before_tex )
{
//	stb x61

//#ifndef __PLAT_WN32__
//	GXSetZCompLoc( before_tex );
//#else
    _BPCmd( ( PE_CONTROL_ID << 24 ) | GX_PF_RGB8_Z24 | ( before_tex << 6 ) );
//#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetPixelFmt( GXPixelFmt	pix_fmt,
				  GXZFmt16		z_fmt )
{
#ifndef __PLAT_WN32__
	GXSetPixelFmt( pix_fmt, z_fmt );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
GXFifoObj * Init( void *	base,
				  u32		size )
{
	begin();
#ifndef __PLAT_WN32__
	return GXInit( base, size );
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void AdjustForOverscan( const GXRenderModeObj *	rmin,
						GXRenderModeObj *		rmout,
						u16						hor,
						u16						ver )
{
#ifndef __PLAT_WN32__
	GXAdjustForOverscan( rmin, rmout, hor, ver );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetViewport( f32 left,
				  f32 top,
				  f32 wd,
				  f32 ht,
				  f32 nearz,
				  f32 farz )
{
#ifndef __PLAT_WN32__
	GXSetViewport( left, top, wd, ht, nearz, farz );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetDispCopySrc( u16 left,
					 u16 top,
					 u16 wd,
					 u16 ht )
{
#ifndef __PLAT_WN32__
	GXSetDispCopySrc( left, top, wd, ht );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetDispCopyDst( u16 wd,
					 u16 ht )
{
#ifndef __PLAT_WN32__
	GXSetDispCopyDst( wd, ht );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
u32 SetDispCopyYScale( f32 vscale )
{
#ifndef __PLAT_WN32__
	return GXSetDispCopyYScale( vscale );
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetVtxAttrFmt( GXVtxFmt	vtxfmt,
					GXAttr		attr,
					GXCompCnt	cnt,
					GXCompType	type,
					u8			frac )
{
#ifndef __PLAT_WN32__
	GXSetVtxAttrFmt( vtxfmt, attr, cnt, type, frac );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetViewportJitter( f32 left,
						f32 top,
						f32 wd,
						f32 ht,
						f32 nearz,
						f32 farz,
						u32 field )
{
#ifndef __PLAT_WN32__
	GXSetViewportJitter( left,  top, wd, ht, nearz, farz, field );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InvalidateVtxCache( void )
{
#ifndef __PLAT_WN32__
	GXInvalidateVtxCache();
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InvalidateTexAll( void )
{
#ifndef __PLAT_WN32__
	GXInvalidateTexAll();
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
GXDrawSyncCallback SetDrawSyncCallback( GXDrawSyncCallback cb )
{
#ifndef __PLAT_WN32__
	return GXSetDrawSyncCallback( cb );
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void DrawDone( void )
{
#ifndef __PLAT_WN32__
	GXDrawDone();
#endif		// __PLAT_WN32__
}

void AbortFrame( void )
{
#ifndef __PLAT_WN32__
	GXAbortFrame();
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetDrawDone( void )
{
#ifndef __PLAT_WN32__
	GXSetDrawDone();
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void WaitDrawDone( void )
{
#ifndef __PLAT_WN32__
	GXWaitDrawDone();
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
GXDrawDoneCallback SetDrawDoneCallback( GXDrawDoneCallback cb )
{
#ifndef __PLAT_WN32__
	return GXSetDrawDoneCallback( cb );
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void ReadXfRasMetric( u32 *          xf_wait_in,
					  u32 *          xf_wait_out,
					  u32 *          ras_busy,
					  u32 *          clocks )
{
#ifndef __PLAT_WN32__
	GXReadXfRasMetric( xf_wait_in, xf_wait_out, ras_busy, clocks );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void GetGPStatus( GXBool * overhi,
				  GXBool * underlow,
				  GXBool * readIdle,
				  GXBool * cmdIdle,
				  GXBool * brkpt )
{
#ifndef __PLAT_WN32__
	GXGetGPStatus( overhi, underlow, readIdle, cmdIdle, brkpt);
#endif		// __PLAT_WN32__
}
    
/********************************************************************************
 *																				*
 ********************************************************************************/
void GetFifoStatus( GXFifoObj *  fifo,
					GXBool *     overhi,
					GXBool *     underlow,
					u32 *        fifoCount,
					GXBool *     cpu_write,
					GXBool *     gp_read,
					GXBool *     fifowrap )
{
#ifndef __PLAT_WN32__
	GXGetFifoStatus( fifo, overhi, underlow, fifoCount, cpu_write, gp_read, fifowrap );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void * GetFifoBase( const GXFifoObj * fifo )
{
#ifndef __PLAT_WN32__
	return GXGetFifoBase( fifo );
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
u32 GetFifoSize( const GXFifoObj * fifo )
{
#ifndef __PLAT_WN32__
	return GXGetFifoSize( fifo );
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void GetFifoLimits( const GXFifoObj *	fifo,
					u32 *				hi,
					u32 *				lo )
{
#ifndef __PLAT_WN32__
	GXGetFifoLimits( fifo, hi, lo);
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
u32 GetOverflowCount( void )
{
#ifndef __PLAT_WN32__
	return GXGetOverflowCount();
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
u32 ResetOverflowCount( void )
{
#ifndef __PLAT_WN32__
	return GXResetOverflowCount();
#else
	return 0;
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void EnableHang( void )
{
	// Set up RAS Ready counter
	_u8( GX_LOAD_BP_REG );
	_u32( 0x2402c004 ); // ... 101 10000 00000 00100

	// Set up SU Ready counter
	_u8( GX_LOAD_BP_REG );
	_u32( 0x23000020 ); // ... 100 000

	// Set up XF TOP and BOT busy counters
	_u8( GX_LOAD_XF_REG );
	_u16( 0x0000 );
	_u16( 0x1006 );
	_u32( 0x00084400 ); // 10000 10001 00000 00000
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void DisableHang( void )
{
	// Disable RAS counters
	_u8( GX_LOAD_BP_REG );
	_u32( 0x24000000 );

	// Disable SU counters
	_u8( GX_LOAD_BP_REG );
	_u32( 0x23000000 );

	// Disable XF counters
	_u8( GX_LOAD_XF_REG );
	_u16( 0x0000 );
	_u16( 0x1006 );
	_u32( 0x00000000 );
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void ResolveDLTexAddr( NxNgc::sTextureDL *			p_dl,
					   NxNgc::sMaterialPassHeader *	p_pass,
					   int							num_passes )
{
//		GX::begin( p_dl->mp_dl, p_dl->m_dl_size );
//		multi_mesh( p_mat, p_pass, true, true );
//		p_dl->m_dl_size = GX::end();

#ifndef __PLAT_WN32__
	for ( int layer = 0; layer < num_passes; layer++ )
	{
		// Offset  0 = mode0  :: wrap, aniso, clamp, lod, filt
		// Offset  5 = mode1  :: lod values
		// Offset 10 = image0 :: width, height, format
		// Offset 15 = image1 :: cache slot
		// Offset 20 = image2 :: cache slot 
		// Offset 25 = image3 :: RAM Pointer
		// Offset 30 = SU_TS0 :: Scale (texel only)
		// Offset 35 = SU_TS1 :: Scale (texel only)

		if ( p_pass[layer].m_texture.p_data )
		{
			unsigned char * p8;
			uint32 address;
			uint8 levels = p_pass[layer].m_texture.p_data->Levels;
			u8 min = 0;
			u8 max = (u8)( ( levels > 1 ? levels - 1.0f : 0.0f ) * 16.0f ); 
			if ( p_pass[layer].m_texture.p_data->pTexelData && ( p_dl->m_tex_offset[layer] != -1 ) )
			{
				// Resolve address
				p8 = &((unsigned char *)p_dl->mp_dl)[p_dl->m_tex_offset[layer]+25];
				address = OSCachedToPhysical( p_pass[layer].m_texture.p_data->pTexelData ) >> 5;
				p8[2] = (uint8)( ( address >> 16 ) & 0xff );
				p8[3] = (uint8)( ( address >> 8  ) & 0xff );
				p8[4] = (uint8)( ( address >> 0  ) & 0xff );
				DCFlushRange ( &p8[2], 3 );
				// Resolve MIP
				p8 = &((unsigned char *)p_dl->mp_dl)[p_dl->m_tex_offset[layer]+5];
				p8[3] = max;
				p8[4] = min;
				DCFlushRange ( &p8[2], 3 );
				// Resolve Width & Height
				p8 = &((unsigned char *)p_dl->mp_dl)[p_dl->m_tex_offset[layer]+10];
				int width = p_pass[layer].m_texture.p_data->ActualWidth - 1;
				int height = p_pass[layer].m_texture.p_data->ActualHeight - 1;
				uint32 whf = ( width << 0 ) | ( height << 10 ) | ( GX_TF_CMPR << 20 );
				p8[2] = (uint8)( ( whf >> 16 ) & 0xff );
				p8[3] = (uint8)( ( whf >> 8  ) & 0xff );
				p8[4] = (uint8)( ( whf >> 0  ) & 0xff );
				DCFlushRange ( &p8[2], 3 );
				p8 = &((unsigned char *)p_dl->mp_dl)[p_dl->m_tex_offset[layer]+30];
				p8[3] = (uint8)( ( width >> 8  ) & 0xff );
				p8[4] = (uint8)( ( width >> 0  ) & 0xff );
				DCFlushRange ( &p8[3], 2 );
				p8 = &((unsigned char *)p_dl->mp_dl)[p_dl->m_tex_offset[layer]+35];
				p8[3] = (uint8)( ( height >> 8  ) & 0xff );
				p8[4] = (uint8)( ( height >> 0  ) & 0xff );
				DCFlushRange ( &p8[3], 2 );
			}
			if ( p_pass[layer].m_texture.p_data->pAlphaData && ( p_dl->m_alpha_offset[layer] != -1 ) )
			{
				// Resolve address
				p8 = &((unsigned char *)p_dl->mp_dl)[p_dl->m_alpha_offset[layer]+25];
				address = OSCachedToPhysical( p_pass[layer].m_texture.p_data->pAlphaData ) >> 5;
				p8[2] = (uint8)( ( address >> 16 ) & 0xff );
				p8[3] = (uint8)( ( address >> 8  ) & 0xff );
				p8[4] = (uint8)( ( address >> 0  ) & 0xff );
				DCFlushRange ( &p8[2], 3 );
				// Resolve MIP
				p8 = &((unsigned char *)p_dl->mp_dl)[p_dl->m_alpha_offset[layer]+5];
				p8[3] = max;
				p8[4] = min;
				DCFlushRange ( &p8[3], 2 );
				// Resolve Width & Height
				p8 = &((unsigned char *)p_dl->mp_dl)[p_dl->m_tex_offset[layer]+10];
				int width = p_pass[layer].m_texture.p_data->ActualWidth - 1;
				int height = p_pass[layer].m_texture.p_data->ActualHeight - 1;
				uint32 whf = ( width << 0 ) | ( height << 10 ) | ( GX_TF_CMPR << 20 );
				p8[2] = (uint8)( ( whf >> 16 ) & 0xff );
				p8[3] = (uint8)( ( whf >> 8  ) & 0xff );
				p8[4] = (uint8)( ( whf >> 0  ) & 0xff );
				DCFlushRange ( &p8[2], 3 );
			}
		}
	}
	DCFlushRange ( p_dl->mp_dl, p_dl->m_dl_size );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void ChangeMaterialColor( unsigned char *				p_dl,
						  unsigned int					size,
						  GXColor						color,
						  int							pass )
{
//	GX::begin( p_dl->mp_dl, p_dl->m_dl_size );
//	multi_mesh( p_mat, p_pass, true, true );
//	p_dl->m_dl_size = GX::end();

#ifndef __PLAT_WN32__
	unsigned char * p8 = p_dl;
	unsigned char * p_end = &p8[size];

	bool quit = false;

	while ( !quit )
	{
		switch ( p8[0] )
		{
			case GX_LOAD_BP_REG:
				{
					uint8 reg0 = TEV_REGISTERL_0_ID + ( pass * 2 );
					uint8 reg1 = TEV_REGISTERH_0_ID + ( pass * 2 );

					if ( ( p8[(0*5)+1] == reg0 ) && ( p8[(1*5)+1] == reg1 ) )
					{
						u32 regRA, regBG;

						regRA = TEV_REGISTERL( color.r, color.a, TEV_KONSTANT_REG,
											   TEV_REGISTERL_0_ID + pass * 2 );
						regBG = TEV_REGISTERH( color.b, color.g, TEV_KONSTANT_REG, 
											   TEV_REGISTERH_0_ID + pass * 2 );
						p8[(0*5)+1] = ( regRA >> 24 );
						p8[(0*5)+2] = ( regRA >> 16 );
						p8[(0*5)+3] = ( regRA >>  8 );
						p8[(0*5)+4] = ( regRA >>  0 );

						p8[(1*5)+1] = ( regBG >> 24 );
						p8[(1*5)+2] = ( regBG >> 16 );
						p8[(1*5)+3] = ( regBG >>  8 );
						p8[(1*5)+4] = ( regBG >>  0 );
						DCFlushRange ( p8, 10 );
					}
					p8 += 5;
				}
				break;
			case GX_LOAD_CP_REG:
				p8 += 6;
				break;
			case GX_LOAD_XF_REG:
				p8 += 1 + 2 + 2 + 4 + ( 4 * ( ( p8[1] << 8 ) | p8[2] ) );
				break;
			default:
				quit = true;
				break;
		}
		if ( p8 >= p_end ) quit = true;
	}
	DCFlushRange ( p_dl, size );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetTexCopySrc( u16 left,
					u16 top,
					u16 wd,
					u16 ht )
{
#ifndef __PLAT_WN32__
	GXSetTexCopySrc( left, top, wd, ht );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetTexCopyDst( u16 wd,
					u16 ht,
					GXTexFmt fmt,
					GXBool mipmap )
{
#ifndef __PLAT_WN32__
	GXSetTexCopyDst( wd, ht, fmt, mipmap );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void CopyTex( void *	dest,
			  GXBool	clear )
{
#ifndef __PLAT_WN32__
	GXCopyTex( dest, clear );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void PixModeSync( void )
{
#ifndef __PLAT_WN32__
	GXPixModeSync();
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void PokeAlphaUpdate( GXBool update_enable )
{
#ifndef __PLAT_WN32__
	GXPokeAlphaUpdate( update_enable );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void PokeColorUpdate( GXBool update_enable )
{
#ifndef __PLAT_WN32__
	GXPokeColorUpdate( update_enable );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void GetProjectionv( f32 * p )
{
#ifndef __PLAT_WN32__
	GXGetProjectionv( p );
#endif		// __PLAT_WN32__
//	p[0] = g_projection[0];
//	p[1] = g_projection[1];
//	p[2] = g_projection[2];
//	p[3] = g_projection[3];
//	p[4] = g_projection[4];
//	p[5] = g_projection[5];
//	p[6] = g_projection[6];
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetProjectionv( const f32 * ptr )
{
#ifndef __PLAT_WN32__
	GXSetProjectionv( ptr );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void GetViewportv( f32 * viewport )
{
#ifndef __PLAT_WN32__
	GXGetViewportv( viewport );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void GetScissor( u32 *    left,
				 u32 *    top,
				 u32 *    width,
				 u32 *    height )
{
#ifndef __PLAT_WN32__
	GXGetScissor( left, top, width, height );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void Project( f32			x,          // model coordinates
			  f32			y,
			  f32			z,
			  const f32		mtx[3][4],  // model-view matrix
			  const f32 *	pm,         // projection matrix, as returned by GXGetProjectionv
			  const f32 *	vp,         // viewport, as returned by GXGetViewportv
			  f32 *			sx,         // screen coordinates
			  f32 *			sy,
			  f32 *			sz )
{
#ifndef __PLAT_WN32__
	GXProject( x, y, z, mtx, pm, vp, sx, sy, sz );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void SetArray( GXAttr 		attr,
			   const void *	base_ptr,
			   u8 			stride )
{
#ifndef __PLAT_WN32__
//	GDSetArray( attr, base_ptr, stride );

    s32 cpAttr;

    cpAttr = (attr == GX_VA_NBT) ? (GX_VA_NRM - GX_VA_POS) : (attr - GX_VA_POS);
    
    _CPCmd( (u8) (CP_ARRAY_BASE_ID + cpAttr),
                  CP_ARRAY_BASE_REG( OSCachedToPhysical( base_ptr ) ));

    _CPCmd( (u8) (CP_ARRAY_STRIDE_ID + cpAttr),
                  CP_ARRAY_STRIDE_REG( stride ));
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitLightAttn( GXLightObj*   lt_obj,
					f32           a0,
					f32           a1,
					f32           a2,
					f32           k0,
					f32           k1,
					f32           k2 )
{
#ifndef __PLAT_WN32__
	GXInitLightAttn( lt_obj, a0, a1, a2, k0, k1, k2 );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitLightAttnA( GXLightObj *	lt_obj,
					 f32			a0,
					 f32			a1,
					 f32			a2 )
{
#ifndef __PLAT_WN32__
	GXInitLightAttnA( lt_obj, a0, a1, a2);
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitLightAttnK( GXLightObj *	lt_obj,
					 f32			k0,
					 f32			k1,
					 f32			k2 )
{
#ifndef __PLAT_WN32__
	GXInitLightAttnK( lt_obj, k0, k1, k2 );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitLightSpot( GXLightObj *	lt_obj,
					f32				cutoff,
					GXSpotFn		spot_func )
{
#ifndef __PLAT_WN32__
	GXInitLightSpot( lt_obj, cutoff, spot_func );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitLightDistAttn( GXLightObj *	lt_obj,
						f32				ref_distance,
						f32				ref_brightness,
						GXDistAttnFn	dist_func )
{
#ifndef __PLAT_WN32__
	GXInitLightDistAttn( lt_obj, ref_distance, ref_brightness, dist_func );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitLightPos( GXLightObj *	lt_obj,
				   f32			x,
				   f32			y,
				   f32			z )
{
#ifndef __PLAT_WN32__
	GXInitLightPos( lt_obj, x, y, z );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitLightColor( GXLightObj *	lt_obj,
					 GXColor		color )
{
#ifndef __PLAT_WN32__
	GXInitLightColor( lt_obj, color );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void LoadLightObjImm( const GXLightObj *	lt_obj,
					  GXLightID				light )
{
#ifndef __PLAT_WN32__
	GXLoadLightObjImm( lt_obj, light );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void LoadLightObjIndx( u32			lt_obj_indx,
					   GXLightID	light )
{
#ifndef __PLAT_WN32__
	GXLoadLightObjIndx( lt_obj_indx, light );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitLightDir( GXLightObj *	lt_obj,
				   f32			nx,
				   f32			ny,
				   f32			nz )
{
#ifndef __PLAT_WN32__
	GXInitLightDir( lt_obj, nx, ny, nz );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitSpecularDir( GXLightObj *	lt_obj,
					  f32			nx,
					  f32			ny,
					  f32			nz )
{
#ifndef __PLAT_WN32__
	GXInitSpecularDir( lt_obj, nx, ny, nz );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitSpecularDirHA( GXLightObj *	lt_obj,
						f32				nx,
						f32				ny,
						f32				nz,
						f32				hx,
						f32				hy,
						f32				hz )
{
#ifndef __PLAT_WN32__
	GXInitSpecularDirHA( lt_obj, nx, ny, nz, hx, hy, hz );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitLightPosv( GXLightObj *	lt_obj,
					Vec *			p )
{
#ifndef __PLAT_WN32__
	GXInitLightPos( lt_obj, p->x, p->y, p->z );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitLightDirv( GXLightObj *	lt_obj,
					Vec *			n )
{
#ifndef __PLAT_WN32__
	GXInitLightDir( lt_obj, n->x, n->y, n->z );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitSpecularDirv( GXLightObj *	lt_obj,
					   Vec *		n )
{
#ifndef __PLAT_WN32__
	GXInitSpecularDir( lt_obj, n->x, n->y, n->z );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitSpecularDirHAv( GXLightObj *	lt_obj,
						 Vec *			n,
						 Vec *			h )
{
#ifndef __PLAT_WN32__
	GXInitSpecularDirHA( lt_obj, n->x, n->y, n->z, h->x, h->y, h->z );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void InitLightShininess( GXLightObj *	lt_obj,
						 float			shininess )
{
#ifndef __PLAT_WN32__
	GXInitLightAttn( lt_obj, 0.0F, 0.0F, 1.0F, shininess / 2.0F, 0.0F, 1.0F - shininess / 2.0F );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void CallDisplayList( const void *	list,
					  u32			nbytes )
{
#ifndef __PLAT_WN32__
	GXCallDisplayList( list, nbytes );
#endif		// __PLAT_WN32__
}

/********************************************************************************
 *																				*
 ********************************************************************************/
void ResetGX( void )
{
#ifndef __PLAT_WN32__
	// Color definitions
	
	#define GX_DEFAULT_BG (GXColor){64, 64, 64, 255}
	#define BLACK (GXColor){0, 0, 0, 0}
	#define WHITE (GXColor){255, 255, 255, 255}
	
	//
	// Render Mode
	//
	// (set 'rmode' based upon VIGetTvFormat(); code not shown)
	
	//
	// Geometry and Vertex
	//
	GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX1, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX2, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD3, GX_TG_MTX2x4, GX_TG_TEX3, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD4, GX_TG_MTX2x4, GX_TG_TEX4, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD5, GX_TG_MTX2x4, GX_TG_TEX5, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD6, GX_TG_MTX2x4, GX_TG_TEX6, GX_IDENTITY);
	GXSetTexCoordGen(GX_TEXCOORD7, GX_TG_MTX2x4, GX_TG_TEX7, GX_IDENTITY);
	GXSetNumTexGens(1);
	GXClearVtxDesc();
	GXInvalidateVtxCache();
	
	GXSetLineWidth(6, GX_TO_ZERO);
	GXSetPointSize(6, GX_TO_ZERO);
	GXEnableTexOffsets( GX_TEXCOORD0, GX_DISABLE, GX_DISABLE );
	GXEnableTexOffsets( GX_TEXCOORD1, GX_DISABLE, GX_DISABLE );
	GXEnableTexOffsets( GX_TEXCOORD2, GX_DISABLE, GX_DISABLE );
	GXEnableTexOffsets( GX_TEXCOORD3, GX_DISABLE, GX_DISABLE );
	GXEnableTexOffsets( GX_TEXCOORD4, GX_DISABLE, GX_DISABLE );
	GXEnableTexOffsets( GX_TEXCOORD5, GX_DISABLE, GX_DISABLE );
	GXEnableTexOffsets( GX_TEXCOORD6, GX_DISABLE, GX_DISABLE );
	GXEnableTexOffsets( GX_TEXCOORD7, GX_DISABLE, GX_DISABLE );
	
	//
	// Transformation and Matrix
	//
	// (initialize 'identity_mtx' to identity; code not shown)
	
	// Note: projection matrix is not initialized!
//	GXLoadPosMtxImm(identity_mtx, GX_PNMTX0);
//	GXLoadNrmMtxImm(identity_mtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
//	GXLoadTexMtxImm(identity_mtx, GX_IDENTITY, GX_MTX3x4);
//	GXLoadTexMtxImm(identity_mtx, GX_PTIDENTITY, GX_MTX3x4);
//	GXSetViewport(0.0F, // left
//	0.0F, // top
//	(float)rmode->fbWidth, // width
//	(float)rmode->xfbHeight, // height
//	0.0F, // nearz
//	1.0F); // farz
	
	//
	// Clipping and Culling
	//
//	GXSetCoPlanar(GX_DISABLE);
	GXSetCullMode(GX_CULL_BACK);
	GXSetClipMode(GX_CLIP_ENABLE);
//	GXSetScissor(0, 0, (u32)rmode->fbWidth, (u32)rmode->efbHeight);
//	GXSetScissorBoxOffset(0, 0);
	
	//
	// Lighting - pass vertex color through
	//
	GXSetNumChans(0); // no colors by default
	
	GXSetChanCtrl(
	GX_COLOR0A0,
	GX_DISABLE,
	GX_SRC_REG,
	GX_SRC_VTX,
	GX_LIGHT_NULL,
	GX_DF_NONE,
	GX_AF_NONE );
	
	GXSetChanAmbColor(GX_COLOR0A0, BLACK);
	GXSetChanMatColor(GX_COLOR0A0, WHITE);
	
	GXSetChanCtrl(
	GX_COLOR1A1,
	GX_DISABLE,
	GX_SRC_REG,
	GX_SRC_VTX,
	GX_LIGHT_NULL,
	GX_DF_NONE,
	GX_AF_NONE );
	
	GXSetChanAmbColor(GX_COLOR1A1, BLACK);
	GXSetChanMatColor(GX_COLOR1A1, WHITE);
	
	//
	// Texture
	//
	GXInvalidateTexAll();
	
	// Allocate 8 32k caches for RGBA texture mipmaps.
	// Equal size caches to support 32b RGBA textures.
	//
	// (code not shown)
	
	// Allocate color index caches in low bank of TMEM.
	// Each cache is 32kB.
	// Even and odd regions should be allocated on different address.
	//
	// (code not shown)
	
	// Allocate TLUTs, 16 256-entry TLUTs and 4 1K-entry TLUTs.
	// 256-entry TLUTs are 8kB, 1k-entry TLUTs are 32kB.
	//
	// (code not shown)
	
	//
	// Set texture region and tlut region Callbacks
	//
//	GXSetTexRegionCallback(__GXDefaultTexRegionCallback);
//	GXSetTlutRegionCallback(__GXDefaultTlutRegionCallback);
//	
	//
	// Texture Environment
	//
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD2, GX_TEXMAP2, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD3, GX_TEXMAP3, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE4, GX_TEXCOORD4, GX_TEXMAP4, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE5, GX_TEXCOORD5, GX_TEXMAP5, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE6, GX_TEXCOORD6, GX_TEXMAP6, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE7, GX_TEXCOORD7, GX_TEXMAP7, GX_COLOR0A0);
	GXSetTevOrder(GX_TEVSTAGE8, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE9, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE10,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE11,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE12,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE13,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE14,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE15,GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GXSetNumTevStages(1);
	GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
//	GXSetZTexture(GX_ZT_DISABLE, GX_TF_Z8, 0);
//	
	for ( int i = GX_TEVSTAGE0; i < GX_MAX_TEVSTAGE; i++) {
	GXSetTevKColorSel((GXTevStageID) i, GX_TEV_KCSEL_1_4 );
	GXSetTevKAlphaSel((GXTevStageID) i, GX_TEV_KASEL_1 );
	GXSetTevSwapMode ((GXTevStageID) i, GX_TEV_SWAP0, GX_TEV_SWAP0 );
	}
//	GXSetTevSwapModeTable(GX_TEV_SWAP0,
//	GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
//	GXSetTevSwapModeTable(GX_TEV_SWAP1,
//	GX_CH_RED, GX_CH_RED, GX_CH_RED, GX_CH_ALPHA);
//	GXSetTevSwapModeTable(GX_TEV_SWAP2,
//	GX_CH_GREEN, GX_CH_GREEN, GX_CH_GREEN, GX_CH_ALPHA);
//	GXSetTevSwapModeTable(GX_TEV_SWAP3,
//	GX_CH_BLUE, GX_CH_BLUE, GX_CH_BLUE, GX_CH_ALPHA);
//	
//	// Indirect Textures.
//	for (i = GX_TEVSTAGE0; i < GX_MAX_TEVSTAGE; i++) {
//	GXSetTevDirect((GXTevStageID) i);
//	}
//	GXSetNumIndStages(0);
//	GXSetIndTexCoordScale( GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1 );
//	GXSetIndTexCoordScale( GX_INDTEXSTAGE1, GX_ITS_1, GX_ITS_1 );
//	GXSetIndTexCoordScale( GX_INDTEXSTAGE2, GX_ITS_1, GX_ITS_1 );
//	GXSetIndTexCoordScale( GX_INDTEXSTAGE3, GX_ITS_1, GX_ITS_1 );
//	
	//
	// Pixel Processing
	//
	GXSetFog(GX_FOG_NONE, 0.0F, 1.0F, 0.1F, 1.0F, BLACK);
	GXSetFogRangeAdj( GX_DISABLE, 0, 0 );
	GXSetBlendMode(GX_BM_NONE,
	GX_BL_SRCALPHA, // src factor
	GX_BL_INVSRCALPHA, // dst factor
	GX_LO_CLEAR);
	GXSetColorUpdate(GX_ENABLE);
	GXSetAlphaUpdate(GX_ENABLE);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GXSetZCompLoc(GX_TRUE); // before texture
	GXSetDither(GX_ENABLE);
	GXSetDstAlpha(GX_DISABLE, 0);
//	GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
//	GXSetFieldMask( GX_ENABLE, GX_ENABLE );
//	GXSetFieldMode((GXBool)(rmode->field_rendering),
//	((rmode->viHeight == 2*rmode->xfbHeight) ?
//	GX_ENABLE : GX_DISABLE));
//	
//	//
//	// Frame buffer
//	//
//	GXSetCopyClear(GX_DEFAULT_BG, GX_MAX_Z24);
//	GXSetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
//	GXSetDispCopyDst(rmode->fbWidth, rmode->efbHeight);
//	GXSetDispCopyYScale((f32)(rmode->xfbHeight) / (f32)(rmode->efbHeight));
//	GXSetCopyClamp((GXFBClamp)(GX_CLAMP_TOP | GX_CLAMP_BOTTOM));
//	GXSetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
//	GXSetDispCopyGamma( GX_GM_1_0 );
//	GXSetDispCopyFrame2Field(GX_COPY_PROGRESSIVE);
//	GXClearBoundingBox();
//	
//	//
//	// CPU direct EFB access
//	//
//	GXPokeColorUpdate(GX_TRUE);
//	GXPokeAlphaUpdate(GX_TRUE);
//	GXPokeDither(GX_FALSE);
//	GXPokeBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ONE, GX_LO_SET);
//	GXPokeAlphaMode(GX_ALWAYS, 0);
//	GXPokeAlphaRead(GX_READ_FF);
//	GXPokeDstAlpha(GX_DISABLE, 0);
//	GXPokeZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
//	
//	//
//	// Performance Counters
//	//
//	GXSetGPMetric(GX_PERF0_NONE, GX_PERF1_NONE);
//	GXClearGPMetric();
#endif		// __PLAT_WN32__
}
	
				
/********************************************************************************
 *																				*
 ********************************************************************************/
u32 GetTexBufferSize( u16		width,
					  u16		height,
					  u32		format,
					  GXBool	mipmap,
					  u8		max_lod )
{
#ifndef __PLAT_WN32__
	return GXGetTexBufferSize( width, height, format, mipmap, max_lod );
#else
	return 0;
#endif		// __PLAT_WN32__
}

}		// namespace GX

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __PLAT_WN32__
short			g_tex_off[4];
short			g_alpha_off[4];
int				g_num_tex;
#endif		// __PLAT_WN32__

namespace NxNgc
{

static int	stage_id = 0;	// 0-15
static int	tev_id = 0;					// 0-3
static int	uv_id = 0;					// 0-7
static int	uv_tex_id = 0;					// 0-7
static int	map_id = 0;					// 0-3
static int	layer_id = 0;					// 1-3
static bool correct_color = false;

GXTexCoordID	ordt[16];
GXTexMapID		ordm[16];
GXChannelID		ordc[16];
bool			ord[16];

GXTevKAlphaSel	asel[16]; 
GXTevKColorSel	csel[16];
bool			sel[16];

GXTexMtx		texmtx[8];
bool			settex = false;

bool			_su;
bool			_tb;
bool			_tc;
bool			_ta;

//typedef struct
//{
//	GXTexCoordID	id;
//	GXTexGenType	type;
//	GXTexGenSrc		src;
//} DEFER_GEN;
//
//DEFER_GEN defer_gen[16];
//int num_defer_gen = 0;

static void multi_start ( bool bl, bool tx )
{
	// Set everything to 0.
	// Set everything to 0.
	stage_id	= 0;
	tev_id		= 0;
	uv_id		= 0;
	uv_tex_id		= 0;
	map_id		= 0;
	layer_id	= 0;


//	num_defer_gen = 0;

	// Don't forget these!
//	GXSetNumChans(2);
//	GXSetNumTexGens(2);
//	GXSetNumTevStages(2);

	// See if we need to add 2 stages for color correction.
	if ( correct_color )
	{
		if(bl && _su)
		{
			GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG1,
													GX_TEV_SWAP0, GX_TEV_SWAP0 );

			GX::SetTevColorInOp( GX_TEVSTAGE0,		GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG1 );
	
			GX::SetTevAlphaInOpSwap( GX_TEVSTAGE1,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2,
													GX_TEV_SWAP0, GX_TEV_SWAP1 );
	
			GX::SetTevColorInOp( GX_TEVSTAGE1,		GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C1,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2 );
		}
		stage_id += 2;
	}

//	// Create 2 dummy stages.
//	GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
//
//	GXSetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL );
//	GXSetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GXSetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//	GXSetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//	GXSetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//	GXSetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//	stage_id++;
//
//	s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
//	
//	GXSetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR0A0 );
//	GXSetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GXSetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2 );
//	GXSetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2 );
//	GXSetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
//	GXSetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
//	stage_id++;

//	if ( !gCorrectColor || !gMeshUseCorrection )
//	{
////		// Create 2 dummy stages.
////		GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
////
////		GXSetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR0A0 );
////		GXSetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
////		GXSetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2 );
////		GXSetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2 );
////		GXSetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
////		GXSetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
////		stage_id++;
//		correct_color = false;
//	}

	// Clear out state settings.
	int lp;
	for ( lp = 0; lp < 16; lp++ )
	{
		asel[lp] = GX_TEV_KASEL_1;
		csel[lp] = GX_TEV_KCSEL_1;
		sel[lp] = false;

		ordt[lp] = (GXTexCoordID)(lp&7);
		ordm[lp] = (GXTexMapID)(lp&7);
		ordc[lp] = GX_COLOR0A0;
		ord[lp] = false;
	}

	for ( lp = 0; lp < 8; lp++ )
	{
		texmtx[lp] = GX_IDENTITY;
	}

	settex = false;
}

static void multi_add_layer ( BlendModes blendmode, int raw_fix, bool bl, bool tx )
{
	// Set inputs.
	GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
	
//	int reg_id = 1;	//layer_id;	//( layer_id > 1 ) ? 1 : layer_id;	//( tev_id == 3 ) ? 2 : tev_id;		// CPREV, C0, C1, C0.
  //  int reg_id = layer_id;	//( layer_id == 3 ) ? 2 : layer_id;		// CPREV, C0, C1, C0.
	int reg_id = tev_id;		// CPREV, C0, C1, C0.

//	GXTevAlphaArg newa= (GXTevAlphaArg)(((int)GX_CA_APREV)+reg_id);
	GXTevColorArg newc = (GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2));
	GXTevColorArg newca= (GXTevColorArg)(((int)GX_CC_APREV)+(reg_id*2));


    // out_reg = (d (op) ((1.0 - c)*a + c*b) + bias) * scale;

//	int fix = raw_fix;	//( raw_fix >= 128 ) ? 255 : raw_fix * 2;

	switch ( blendmode ) {
		case vBLEND_MODE_ADD:
		case vBLEND_MODE_ADD_FIXED:
			// Add using texture or fixed alpha.
			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR_NULL;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
													GX_TEV_SWAP0, GX_TEV_SWAP0 );

			if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, newc, newca, GX_CC_CPREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );

			stage_id++;
			break;
		case vBLEND_MODE_SUBTRACT:
		case vBLEND_MODE_SUB_FIXED:
			// Subtract using texture or fixed alpha.
			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR_NULL;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
													GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
													GX_TEV_SWAP0, GX_TEV_SWAP0 );
			if(bl && _su) GX::SetTevColorInOp ( s_id,		GX_CC_ZERO, newc, newca, GX_CC_CPREV,
													GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
			stage_id++;
			break;
		case vBLEND_MODE_BLEND:
		case vBLEND_MODE_BLEND_FIXED:
			// Blend using texture or fixed alpha.
			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR_NULL;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
													GX_TEV_SWAP0, GX_TEV_SWAP0 );
			if(bl && _su) GX::SetTevColorInOp ( s_id, 		GX_CC_CPREV, newc, newca, GX_CC_ZERO,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
			stage_id++;
			break;
		case vBLEND_MODE_MODULATE:
			// Modulate current layer with previous layer.
			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR_NULL;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
													GX_TEV_SWAP0, GX_TEV_SWAP0 );
			if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, newca, GX_CC_CPREV, GX_CC_ZERO,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
			stage_id++;
			break;
		case vBLEND_MODE_MODULATE_FIXED:
			// Modulate current layer with fixed alpha.
			if(bl && _su)
			{
				csel[(int)s_id] = (GXTevKColorSel)(((int)GX_TEV_KCSEL_K0_A)+layer_id);
				asel[(int)s_id] = GX_TEV_KASEL_1;
				sel[(int)s_id] = true;
			}

			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR_NULL;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
													GX_TEV_SWAP0, GX_TEV_SWAP0 );
			if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, GX_CC_CPREV, GX_CC_KONST, GX_CC_ZERO,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
			stage_id++;
			break;
		case vBLEND_MODE_BRIGHTEN:
			// Modulate current layer with previous layer, & add current layer.
			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR_NULL;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
													GX_TEV_SWAP0, GX_TEV_SWAP0 );
			if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, newca, GX_CC_CPREV, GX_CC_CPREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
			stage_id++;
			break;
		case vBLEND_MODE_BRIGHTEN_FIXED:
			// Modulate current layer with fixed alpha, & add current layer.
			if(bl && _su)
			{
				csel[(int)s_id] = (GXTevKColorSel)(((int)GX_TEV_KCSEL_K0_A)+layer_id);
				asel[(int)s_id] = GX_TEV_KASEL_1;
				sel[(int)s_id] = true;
			}

			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR_NULL;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
													GX_TEV_SWAP0, GX_TEV_SWAP0 );
			if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, GX_CC_CPREV, GX_CC_KONST, GX_CC_CPREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
			stage_id++;
			break;
		case vBLEND_MODE_BLEND_PREVIOUS_MASK:
			// Blend using previous alpha value.
			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR_NULL;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
													GX_TEV_SWAP0, GX_TEV_SWAP0 );
			if(bl && _su) GX::SetTevColorInOp ( s_id, 		GX_CC_CPREV, newc, newca, GX_CC_ZERO,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
			stage_id++;
			break;
		case vBLEND_MODE_BLEND_INVERSE_PREVIOUS_MASK:
			// Blend using previous alpha value.
			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR_NULL;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
													GX_TEV_SWAP0, GX_TEV_SWAP0 );
			if(bl && _su) GX::SetTevColorInOp ( s_id, 		newc, GX_CC_CPREV, newca, GX_CC_ZERO,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
			stage_id++;
			break;
		case vBLEND_MODE_DIFFUSE:
		default:
			// Replace with this texture. Shouldn't ever be used, but here for compatibility.
			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR_NULL;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
													GX_TEV_SWAP0, GX_TEV_SWAP0 );
			if(bl && _su) GX::SetTevColorInOp( s_id,		newc, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
			stage_id++;
			break;
	}
}

static void multi_add_texture ( sTexture * p_texture, GXColor matcol, GXTexWrapMode umode, GXTexWrapMode vmode, BlendModes blendmode, int fix, float k, float shininess, uint8 flags, int layer, bool ignore_alpha, bool bl, bool tx )
{
	// Mesh
	GXTexCoordID u_id = (GXTexCoordID)(((int)GX_TEXCOORD0)+uv_id);

	if ( flags & ( (1<<1) | (1<<2) ) )
	{
		// Environment mapped or UV wibbled.
		GXTexMtx mtx;
		if ( flags & (1<<1) )
		{
			switch ( layer )
			{
				case 0:
					mtx = GX_TEXMTX0;
					break;
				case 1:
					mtx = GX_TEXMTX1;
					break;
				case 2:
					mtx = GX_TEXMTX2;
					break;
				case 3:
					mtx = GX_TEXMTX3;
					break;
				default:
					mtx = GX_IDENTITY;
					break;
			}


			// Force repeat mode for environment mapping & NRM uv generation.
			umode = GX_REPEAT;
			vmode = GX_REPEAT;
			if(tx && _ta)
			{
//				defer_gen[num_defer_gen].id		= u_id;
//				defer_gen[num_defer_gen].type	= GX_TG_MTX2x4;
//				defer_gen[num_defer_gen].src	= GX_TG_NRM;
//				num_defer_gen++;
				GX::SetTexCoordGen( u_id, GX_TG_MTX2x4, GX_TG_NRM, GX_FALSE, GX_PTIDENTITY );
				texmtx[(int)u_id] = mtx;
				settex = true;
			}
		}
		else
		{
			switch ( layer )
			{
				case 0:
					mtx = GX_TEXMTX0;
					break;
				case 1:
					mtx = GX_TEXMTX1;
					break;
				case 2:
					mtx = GX_TEXMTX2;
					break;
				case 3:
					mtx = GX_TEXMTX3;
					break;
				default:
					mtx = GX_IDENTITY;
					break;
			}


			// Use texcoords for wibbling & offset matrix.
			if(tx && _ta)
			{
//				defer_gen[num_defer_gen].id		= u_id;
//				defer_gen[num_defer_gen].type	= GX_TG_MTX2x4;
//				defer_gen[num_defer_gen].src	= (GXTexGenSrc)(((int)GX_TG_TEX0)+uv_id);
//				num_defer_gen++;
				GX::SetTexCoordGen( u_id, GX_TG_MTX2x4, (GXTexGenSrc)(((int)GX_TG_TEX0)+uv_tex_id), GX_FALSE, GX_PTIDENTITY );
				texmtx[(int)u_id] = mtx;
				settex = true;
				uv_tex_id++;
			}
		}
	}
	else
	{
		// Regular mapping.
		if(tx && _ta)
		{
//			defer_gen[num_defer_gen].id		= u_id;
//			defer_gen[num_defer_gen].type	= GX_TG_MTX2x4;
//			defer_gen[num_defer_gen].src	= (GXTexGenSrc)(((int)GX_TG_TEX0)+uv_id);
//			num_defer_gen++;
			GX::SetTexCoordGen( u_id, GX_TG_MTX2x4, (GXTexGenSrc)(((int)GX_TG_TEX0)+uv_tex_id), GX_FALSE, GX_PTIDENTITY );
			texmtx[(int)u_id] = GX_IDENTITY;
			settex = true;
			uv_tex_id++;
		}
	}
	uv_id++;

	// Mat
	if ( map_id >= 8 ) return;
	
#ifdef __PLAT_WN32__
	unsigned char tb = 1;	//( 0 << 4 ) | layer;
	unsigned char ab = 1;   //( 1 << 4 ) | layer; 

	void * p_tex_data = (void *)((tb<<16)|(tb<<8)|tb);
	void * p_alpha_data = (p_texture->m_PaletteFormat & 1) ? (void *)((ab<<16)|(ab<<8)|ab) : NULL;
	GXTevSwapSel alpha_swap = GX_TEV_SWAP1;
	if ( (p_texture->m_PaletteFormat & 1) )
	{
		int channel = ( p_texture->m_PaletteFormat >> 8 ) & 255;
		switch ( channel )
		{
			case 0:
			default:
				alpha_swap = GX_TEV_SWAP1;		// Green
				break;
			case 1:
				alpha_swap = GX_TEV_SWAP2;		// Red
				break;
			case 2:
				alpha_swap = GX_TEV_SWAP3;		// Blue
				break;
		}
	}

	int width = p_texture->m_Width[0];
	int height = p_texture->m_Height[0];
	int levels = p_texture->m_MipLevels + 1;
#else
	void * p_tex_data = p_texture->pTexelData;
	bool has_alpha = ( p_texture->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA ) ? true : false;
	void * p_alpha_data = has_alpha ? p_texture->pAlphaData : NULL;
	GXTevSwapSel alpha_swap = GX_TEV_SWAP1;
	if ( has_alpha )
	{
		switch ( ( p_texture->flags & NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_MASK ) )
		{
			case NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_GREEN:
			default:
				alpha_swap = GX_TEV_SWAP1;		// Green
				break;
			case NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_RED:
				alpha_swap = GX_TEV_SWAP2;		// Red
				break;
			case NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_BLUE:
				alpha_swap = GX_TEV_SWAP3;		// Blue
				break;
		}
	}
	int width = p_texture->ActualWidth;
	int height = p_texture->ActualHeight;
	int levels = p_texture->Levels;
#endif		// __PLAT_WN32__
//	int reg_id = tev_id;	//( tev_id > 1 ) ? 1 : tev_id;	//( tev_id == 3 ) ? 2 : tev_id;		// CPREV, C0, C1, C0.
	int reg_id = tev_id;		// CPREV, C0, C1, C0.
	
	GXTevAlphaArg rasa;
	GXTevColorArg rasc;
	GXTevColorArg rasca;
	if ( correct_color )
	{
		rasa = GX_CA_A2;
		rasc = GX_CC_C2;
		rasca = GX_CC_A2;
	}
	else
	{
		rasa = GX_CA_RASA;
		rasc = GX_CC_RASC;
		rasca = GX_CC_RASA;
	}

	int cur_map = map_id + 1;
	int alpha_map = -1;

	if ( p_alpha_data )
	{
		switch ( blendmode )
		{
			case vBLEND_MODE_ADD:
			case vBLEND_MODE_SUBTRACT:
			case vBLEND_MODE_BLEND:
			case vBLEND_MODE_BRIGHTEN:
			case vBLEND_MODE_MODULATE:
			case vBLEND_MODE_DIFFUSE:
				if ( cur_map < 8 )
				{
					alpha_map = cur_map;
					cur_map++;
				}
				break;
			default:
				break;
		}
	}

//	GXTexObj	texObj;
//	GXInitTexObj( &texObj, p_tex_data, width, height, ((GXTexFmt)p_texture->format), umode, vmode, levels > 1 ? GX_TRUE : GX_FALSE );
//	if ( levels > 1 )
//	{
////		if ( gMeshUseAniso && ( tev_id == 0 ) )
////		{
////			// If we're correcting the color, we also want ANISO_4.
////			GXInitTexObjLOD( &texObj, GX_LIN_MIP_LIN, GX_LINEAR, 0.0f, levels - 1, k, GX_FALSE, GX_TRUE, GX_ANISO_4 );
////		}
////		else
////		{
//			GXInitTexObjLOD( &texObj, GX_LIN_MIP_LIN, GX_LINEAR, 0.0f, levels - 1, k, GX_FALSE, GX_TRUE, GX_ANISO_1 );
////		}
//	}
//	else
//	{
//		GXInitTexObjLOD( &texObj, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, k, GX_FALSE, GX_TRUE, GX_ANISO_1 );
//	}
//	GXLoadTexObj( &texObj, (GXTexMapID)(((int)GX_TEXMAP0)+map_id) );
//	GXSetTexCoordScaleManually( u_id, GX_TRUE, width, height );


	if(tx && _su)
	{
		void * p_td;
		if ( blendmode == vBLEND_MODE_BRIGHTEN )
		{
			p_td = p_alpha_data;
		}
		else
		{
			p_td = p_tex_data;
		}

		if ( p_td )
		{
#ifdef __PLAT_WN32__
			g_tex_off[layer] = GDGetCurrOffset();
//		printf( "\nAdded tex off: layer %d, off %d - %d %d %d %d", layer, g_tex_off[layer], _su, _tb, _tc, _ta );
#endif		// __PLAT_WN32__
			GX::UploadTexture(	p_td,
								width,
								height,
#ifdef __PLAT_WN32__
								/*( p_texture->m_PaletteFormat & 1 ) ? */GX_TF_CMPR/* : GX_TF_RGBA8*/,
#else
								((GXTexFmt)p_texture->format),
#endif		// __PLAT_WN32__
								umode,
								vmode,
								( levels > 1 ? GX_TRUE : GX_FALSE ),
								( levels > 1 ? GX_LIN_MIP_LIN : GX_LINEAR ),
								GX_LINEAR,
								0.0f,
								( levels > 1 ? levels - 1.0f : 0.0f ),
								k,
								GX_FALSE,
								GX_TRUE,
								/*( layer == 0 ) ? GX_ANISO_2 :*/ GX_ANISO_1,
								(GXTexMapID)(((int)GX_TEXMAP0)+map_id) ); 
		}
	}

	if(tx && _su) GX::SetTexCoordScale( u_id, GX_TRUE, width, height );

	if ( alpha_map >= 0 )
	{
		switch ( blendmode )
		{
			case vBLEND_MODE_ADD:
			case vBLEND_MODE_SUBTRACT:
			case vBLEND_MODE_BLEND:
			case vBLEND_MODE_MODULATE:
			case vBLEND_MODE_DIFFUSE:
				if(tx && _su)
				{
#ifdef __PLAT_WN32__
					g_alpha_off[layer] = GDGetCurrOffset();
//					printf( "\nAdded alp off %d - %d %d %d %d", layer, _su, _tb, _tc, _ta );
#endif		// __PLAT_WN32__
					GX::UploadTexture(	p_alpha_data,
											width,
											height,
#ifdef __PLAT_WN32__
											/*( p_texture->m_PaletteFormat & 1 ) ? */GX_TF_CMPR/* : GX_TF_RGBA8*/,
#else
											((GXTexFmt)p_texture->format),
#endif		// __PLAT_WN32__
											umode,
											vmode,
											( levels > 1 ? GX_TRUE : GX_FALSE ),
											( levels > 1 ? GX_LIN_MIP_LIN : GX_LINEAR ),
											GX_LINEAR,
											0.0f,
											( levels > 1 ? levels - 1.0f : 0.0f ),
											k,
											GX_FALSE,
											GX_TRUE,
											/*( layer == 0 ) ? GX_ANISO_2 :*/ GX_ANISO_1,
											(GXTexMapID)(((int)GX_TEXMAP0)+alpha_map) );
				}

				//if(tx && _su) GX::SetTexCoordScale( u_id, GX_TRUE, width, height );
				break;
			default:
				p_alpha_data = NULL;
				break;
		}
	}

	GXTexMapID t_id = (GXTexMapID)(((int)GX_TEXMAP0)+map_id);
	GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);

	GXColor col;
	if ( blendmode == vBLEND_MODE_BRIGHTEN_FIXED )
	{
		col.r = fix;
		col.g = fix;
		col.b = fix;
		col.a = fix;
	}
	else
	{
		if ( ( blendmode == vBLEND_MODE_BRIGHTEN ) & !p_alpha_data )
		{
			col.r = 255;
			col.g = 255;
			col.b = 255;
			col.a = 255;
		}
		else
		{
			col.r = matcol.r;
			col.g = matcol.g;
			col.b = matcol.b;
			col.a = fix;
		}
	}

	// Modulate material color.
	GXTevColorArg color_source = rasc;
	GXTevScale color_scale = GX_CS_SCALE_2;
	switch ( blendmode )
	{
		case vBLEND_MODE_BRIGHTEN_FIXED:
			if(tx && _su) GX::SetTevKColor( (GXTevKColorID)(((int)GX_KCOLOR0)+tev_id), col );
			if(bl && _su)
			{
				csel[(int)s_id] = (GXTevKColorSel)(((int)GX_TEV_KCSEL_K0)+tev_id);
				asel[(int)s_id] = GX_TEV_KASEL_1;
				sel[(int)s_id] = true;
			}

			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR0A0;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, rasa, GX_CA_KONST, GX_CA_ZERO,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
													GX_TEV_SWAP0, GX_TEV_SWAP0 );
			if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, rasca, GX_CC_KONST, GX_CC_ZERO,
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
			color_source = (GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2));
			color_scale = GX_CS_SCALE_2;
			stage_id++;
			s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
			break;
		case vBLEND_MODE_BRIGHTEN:
			{
				GXTevSwapSel aa;

#ifdef __PLAT_WN32__
				int channel = ( p_texture->m_PaletteFormat >> 8 ) & 255;
				switch ( channel )
				{
					case 0:
					default:
						aa = GX_TEV_SWAP1;		// Green
						break;
					case 1:
						aa = GX_TEV_SWAP2;		// Blue
						break;
					case 2:
						aa = GX_TEV_SWAP3;		// Red
						break;
				}
#else
				switch ( ( p_texture->flags & NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_MASK ) )
				{
					case NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_GREEN:
					default:
						aa = GX_TEV_SWAP1;		// Green
						break;
					case NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_RED:
						aa = GX_TEV_SWAP2;		// Blue
						break;
					case NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_BLUE:
						aa = GX_TEV_SWAP3;		// Red
						break;
				}
#endif		// __PLAT_WN32__

				if ( p_alpha_data )
				{
					if(bl && _su)
					{
						ordt[(int)s_id] = u_id;
						ordm[(int)s_id] = t_id;
						ordc[(int)s_id] = GX_COLOR0A0;
						ord[(int)s_id] = true;
					}

					if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, rasa, GX_CA_TEXA, GX_CA_ZERO,
															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
															GX_TEV_SWAP0, aa );
					if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, rasca, GX_CC_TEXC, GX_CC_ZERO,
															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
				}
				else
				{
					// No alpha, need to do same as BRIGHTEN_FIXED, but with a fix of 1.
					if(tx && _su) GX::SetTevKColor( (GXTevKColorID)(((int)GX_KCOLOR0)+tev_id), col );
					if(bl && _su)
					{
						csel[(int)s_id] = (GXTevKColorSel)(((int)GX_TEV_KCSEL_K0)+tev_id);
						asel[(int)s_id] = GX_TEV_KASEL_1;
						sel[(int)s_id] = true;
					}

					if(bl && _su)
					{
						ordt[(int)s_id] = GX_TEXCOORD0;
						ordm[(int)s_id] = GX_TEX_DISABLE;
						ordc[(int)s_id] = GX_COLOR0A0;
						ord[(int)s_id] = true;
					}

					if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, rasa, GX_CA_KONST, GX_CA_ZERO,
															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
															GX_TEV_SWAP0, GX_TEV_SWAP0 );
					if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, rasca, GX_CC_KONST, GX_CC_ZERO,
															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
				}
			}
			color_source = (GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2));
			color_scale = GX_CS_SCALE_2;
			stage_id++;
			s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
			break;
		default:
			if ( alpha_map >= 0 )
			{
				if(tx && _su) GX::SetTevKColor( (GXTevKColorID)(((int)GX_KCOLOR0)+tev_id), col );
				if(bl && _su)
				{
					csel[(int)s_id] = (GXTevKColorSel)(((int)GX_TEV_KCSEL_K0)+tev_id);
					asel[(int)s_id] = GX_TEV_KASEL_1;
					sel[(int)s_id] = true;
				}
				GXTexMapID a_id = (GXTexMapID)(((int)GX_TEXMAP0)+alpha_map);
				if(bl && _su)
				{
					ordt[(int)s_id] = u_id;
					ordm[(int)s_id] = a_id;
					ordc[(int)s_id] = GX_COLOR0A0;
					ord[(int)s_id] = true;
				}

				if(bl && _su) GX::SetTevAlphaInOpSwap( s_id, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
														GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
														GX_TEV_SWAP0, alpha_swap );
				if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, rasc, GX_CC_KONST, GX_CC_ZERO,
														GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
				color_source = (GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2));
				color_scale = GX_CS_SCALE_2;
				stage_id++;
				s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
			}
			else
			{
				if ( ( col.r != 128 ) || ( col.g != 128 ) || ( col.b != 128 ) )
				{
					// Unique material color.
					if(tx && _su) GX::SetTevKColor( (GXTevKColorID)(((int)GX_KCOLOR0)+tev_id), col );
					if(bl && _su)
					{
						csel[(int)s_id] = (GXTevKColorSel)(((int)GX_TEV_KCSEL_K0)+tev_id);
						asel[(int)s_id] = GX_TEV_KASEL_1;
						sel[(int)s_id] = true;
					}
					if(bl && _su)
					{
						ordt[(int)s_id] = GX_TEXCOORD0;
						ordm[(int)s_id] = GX_TEX_DISABLE;
						ordc[(int)s_id] = GX_COLOR0A0;
						ord[(int)s_id] = true;
					}

					if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	rasa, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
															GX_TEV_SWAP0, GX_TEV_SWAP0 );
					if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, rasc, GX_CC_KONST, GX_CC_ZERO,
															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
					color_source = (GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2));
					color_scale = GX_CS_SCALE_2;
					stage_id++;
					s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
				}
				else
				{
					// No need for material color stage.
					color_source = rasc;
					color_scale = GX_CS_SCALE_4;
				}
			}
			break;
	}
	
	// Set texture coordinates. Note: If p_tex is NULL, it means this layer is environment mapped.
//	if ( !(flags & MATFLAG_ENVIRONMENT) )
//	{
		//*p_uv_slot = uv_id;
//		uv_id++;
//	} else {
//		*p_uv_slot = -1;
//	}

	// Load this texture up into a temporary register for use later.
	// Note: conveniently, the 1st texture ends up in TEVPREV which means it will be fine
	// if no blends are performed.
	switch ( blendmode )
	{
		case vBLEND_MODE_BRIGHTEN:
		case vBLEND_MODE_BRIGHTEN_FIXED:
			// Just pass it on through...
			break;
		case vBLEND_MODE_ADD_FIXED:
		case vBLEND_MODE_SUB_FIXED:
		case vBLEND_MODE_BLEND_FIXED:
		case vBLEND_MODE_MODULATE_FIXED:
			// If we didn't upload with the material color, upload here.
			if ( color_source == rasc )
			{
				if(tx && _su) GX::SetTevKColor( (GXTevKColorID)(((int)GX_KCOLOR0)+tev_id), col );
			}
			if(bl && _su)
			{
				csel[(int)s_id] = GX_TEV_KCSEL_1;
				asel[(int)s_id] = (GXTevKAlphaSel)(((int)GX_TEV_KASEL_K0_A)+tev_id);
				sel[(int)s_id] = true;
			}

			if(bl && _su)
			{
				ordt[(int)s_id] = u_id;
				ordm[(int)s_id] = t_id;
				ordc[(int)s_id] = GX_COLOR0A0;
				ord[(int)s_id] = true;
			}

			if ( ignore_alpha )
			{
				if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
														GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
														GX_TEV_SWAP0, GX_TEV_SWAP0 );
			}
			else
			{
				if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_KONST, rasa, GX_CA_ZERO,
														GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
														GX_TEV_SWAP0, GX_TEV_SWAP0 );
			}
			if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, GX_CC_TEXC, color_source, GX_CC_ZERO,
													GX_TEV_ADD, GX_TB_ZERO, color_scale, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
			stage_id++;
			break;
		case vBLEND_MODE_BLEND_PREVIOUS_MASK:
		case vBLEND_MODE_BLEND_INVERSE_PREVIOUS_MASK:
			if(bl && _su)
			{
				ordt[(int)s_id] = u_id;
				ordm[(int)s_id] = t_id;
				ordc[(int)s_id] = GX_COLOR0A0;
				ord[(int)s_id] = true;
			}

			if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, (GXTevAlphaArg)(((int)GX_CA_APREV)+(reg_id-1)),
													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
													GX_TEV_SWAP0, GX_TEV_SWAP0 );
			if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, GX_CC_TEXC, color_source, GX_CC_ZERO,
													GX_TEV_ADD, GX_TB_ZERO, color_scale, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
			stage_id++;
			break;
		case vBLEND_MODE_ADD:
		case vBLEND_MODE_SUBTRACT:
		case vBLEND_MODE_BLEND:
		case vBLEND_MODE_MODULATE:
		case vBLEND_MODE_DIFFUSE:
		default:
			// We need to add a stage, if we have an alpha map.
			if ( ( alpha_map >= 0 )/* && ( blendmode != vBLEND_MODE_DIFFUSE )*/ )
			{
//				// Set inputs.
//				GXTexMapID a_id = (GXTexMapID)(((int)GX_TEXMAP0)+alpha_map);
//
//				if(bl && _su)
//				{
//					ordt[(int)s_id] = u_id;
//					ordm[(int)s_id] = a_id;
//					ordc[(int)s_id] = GX_COLOR0A0;
//					ord[(int)s_id] = true;
//				}
//
//				if ( ignore_alpha )
//				{
//					if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
//															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
//															GX_TEV_SWAP0, alpha_swap );       // Alpha map is in Green/Red/Blue channel, so use this to swap it to the alpha channel.
//				}
//				else
//				{
//					if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_TEXA, rasa, GX_CA_ZERO,
//															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
//															GX_TEV_SWAP0, alpha_swap );       // Alpha map is in Green/Red/Blue channel, so use this to swap it to the alpha channel.
//				}
//				if(bl && _su) GX::SetTevColorInOp( s_id,		(GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2)), GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
//														GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//				stage_id++;
//				s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);

				if(bl && _su)
				{
					ordt[(int)s_id] = u_id;
					ordm[(int)s_id] = t_id;
					ordc[(int)s_id] = GX_COLOR0A0;
					ord[(int)s_id] = true;
				}

				if ( ignore_alpha )
				{
					if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	(GXTevAlphaArg)(((int)GX_CA_APREV)+reg_id), GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
															GX_TEV_SWAP0, GX_TEV_SWAP0 );       // Alpha map is in Green/Red/Blue channel, so use this to swap it to the alpha channel.
				}
				else
				{
					if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, (GXTevAlphaArg)(((int)GX_CA_APREV)+reg_id), rasa, GX_CA_ZERO,
															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
															GX_TEV_SWAP0, GX_TEV_SWAP0 );       // Alpha map is in Green/Red/Blue channel, so use this to swap it to the alpha channel.
				}

////				if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	(GXTevAlphaArg)(((int)GX_CA_APREV)+reg_id), GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
////				if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, (GXTevAlphaArg)(((int)GX_CA_APREV)+reg_id), GX_CA_TEXA, GX_CA_ZERO,
//				if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	(GXTevAlphaArg)(((int)GX_CA_APREV)+reg_id), GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
//														GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
//														GX_TEV_SWAP0, GX_TEV_SWAP0 );
				if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, GX_CC_TEXC, color_source, GX_CC_ZERO,
														GX_TEV_ADD, GX_TB_ZERO, color_scale, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
				stage_id++;
			} else {
				if(bl && _su)
				{
					ordt[(int)s_id] = u_id;
					ordm[(int)s_id] = t_id;
					ordc[(int)s_id] = GX_COLOR0A0;
					ord[(int)s_id] = true;
				}

				if ( ignore_alpha )
				{
					if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
															GX_TEV_SWAP0, GX_TEV_SWAP0 );
				}
				else
				{
					if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_TEXA, rasa, GX_CA_ZERO,
															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
															GX_TEV_SWAP0, GX_TEV_SWAP0 );
				}
				if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, GX_CC_TEXC, color_source, GX_CC_ZERO,
														GX_TEV_ADD, GX_TB_ZERO, color_scale, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
				stage_id++;
			}

			break;
	}

//	if ( shininess > 0 )
//	{
//		GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
//
//		// Need to add in specular component.
//		if(bl && _su)
//		{
//			ordt[(int)s_id] = GX_TEXCOORD0;
//			ordm[(int)s_id] = GX_TEX_DISABLE;
//			ordc[(int)s_id] = GX_COLOR1A1;
//			ord[(int)s_id] = true;
//		}
//		if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	(GXTevAlphaArg)(((int)GX_CA_APREV)+reg_id), GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
//												GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id),
//												GX_TEV_SWAP0, GX_TEV_SWAP0 );
//		if(bl && _su) GX::SetTevColorInOp( s_id,		(GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2)), GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC, 
//												GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//		stage_id++;
//	}

	if ( tev_id > 0 ) {
		multi_add_layer ( blendmode, fix, bl, tx );
	} else {
		// Set blend mode for base layer.
		switch ( blendmode )
		{
			case vBLEND_MODE_ADD:
			case vBLEND_MODE_ADD_FIXED:
				if(bl && _su) GX::SetBlendMode( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case vBLEND_MODE_SUBTRACT:
			case vBLEND_MODE_SUB_FIXED:
				if(bl && _su) GX::SetBlendMode( GX_BM_SUBTRACT, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case vBLEND_MODE_BLEND:
			case vBLEND_MODE_BLEND_FIXED:
				if(bl && _su) GX::SetBlendMode( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case vBLEND_MODE_BRIGHTEN:
			case vBLEND_MODE_BRIGHTEN_FIXED:
				if(bl && _su) GX::SetBlendMode( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case vBLEND_MODE_MODULATE_FIXED:
			case vBLEND_MODE_MODULATE:
				if(bl && _su) GX::SetBlendMode( GX_BM_BLEND, GX_BL_ZERO, GX_BL_SRCALPHA, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case vBLEND_MODE_BLEND_PREVIOUS_MASK:
			case vBLEND_MODE_BLEND_INVERSE_PREVIOUS_MASK:
			case vBLEND_MODE_DIFFUSE:
			default:
				if(bl && _su) GX::SetBlendMode( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
		}
	}
	layer_id++;

	tev_id++;
	map_id = cur_map;
#ifdef __PLAT_WN32__
	g_num_tex = map_id;
#endif		// __PLAT_WN32__
}

static void multi_end ( BlendModes blendmode, uint8 pass_flags, uint8 has_holes, uint8 alphacutoff, uint8 flags, bool bl, bool tx, bool cull )
{
	int color_channels;
	if ( correct_color )
	{
		GXTexCoordID u_id;
		// Convert color0 to s,t for 2-D texture lookup (RG)
		u_id = (GXTexCoordID)(((int)GX_TEXCOORD0)+uv_id);
		if(tx && _ta) GX::SetTexCoordGen( u_id, GX_TG_SRTG, GX_TG_COLOR0, GX_FALSE, GX_PTIDENTITY );
		if(tx && _su) GX::SetTexCoordScale( u_id, GX_TRUE, 64, 64 );

		if(bl && _su)
		{
			ordt[(int)GX_TEVSTAGE0] = u_id;
			ordm[(int)GX_TEVSTAGE0] = GX_TEXMAP7;
			ordc[(int)GX_TEVSTAGE0] = GX_COLOR_NULL;
			ord[(int)GX_TEVSTAGE0] = true;
		}

		uv_id++;
	
		// Convert color1 to s,t for 2-D texture lookup (BA)
		u_id = (GXTexCoordID)(((int)GX_TEXCOORD0)+uv_id);
		if(tx && _ta) GX::SetTexCoordGen( u_id, GX_TG_SRTG, GX_TG_COLOR1, GX_FALSE, GX_PTIDENTITY );
		if(tx && _su) GX::SetTexCoordScale( u_id, GX_TRUE, 64, 64 );

		if(bl && _su)
		{
			ordt[(int)GX_TEVSTAGE1] = u_id;
			ordm[(int)GX_TEVSTAGE1] = GX_TEXMAP7;
			ordc[(int)GX_TEVSTAGE1] = GX_COLOR_NULL;
			ord[(int)GX_TEVSTAGE1] = true;
		}

		uv_id++;
		color_channels = 2;
	}
	else
	{
		color_channels = 1;
	}



	// Alpha cutoff.
	if ( tx && _su ) GX::SetAlphaCompare(GX_GEQUAL, alphacutoff, GX_AOP_AND, GX_GEQUAL, alphacutoff );
//	if ( tx ) GX::SetZCompLoc( has_holes ? GX_FALSE : GX_TRUE );

	// Special case. If the base layer uses subtractive mode, we need to add a stage to
	// modulate the color & alpha.
	switch ( blendmode )
	{
		case vBLEND_MODE_SUB_FIXED:
		case vBLEND_MODE_SUBTRACT:
			{
				GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);

				if(bl && _su)
				{
					ordt[(int)s_id] = GX_TEXCOORD0;
					ordm[(int)s_id] = GX_TEX_DISABLE;
					ordc[(int)s_id] = GX_COLOR_NULL;
					ord[(int)s_id] = true;
				}

				if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV,
														GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
														GX_TEV_SWAP0, GX_TEV_SWAP0 );
				if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, GX_CC_CPREV, GX_CC_APREV, GX_CC_ZERO,
														GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
				stage_id++;
			}
			break;
		default:
			break;
	}

//	if ( blendmode == vBLEND_MODE_DIFFUSE )
//	{
//		//Always pass pixels with multi-texturing as the base must be opaque.
//		GXSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_GEQUAL, 0 );
//		GXSetZCompLoc( GX_TRUE );
//	}
//	else
//	{
//	}

//	if ( correct_color )
//	{
//		GXTexCoordID u_id;
//		// Convert color0 to s,t for 2-D texture lookup (RG)
//		u_id = (GXTexCoordID)(((int)GX_TEXCOORD0)+uv_id);
//		GXSetTexCoordGen(u_id, GX_TG_SRTG, GX_TG_COLOR0, GX_IDENTITY);
//		GXSetTevOrder(GX_TEVSTAGE0, u_id, GX_TEXMAP7, GX_COLOR_NULL);
//		uv_id++;
//	
//		// Convert color1 to s,t for 2-D texture lookup (BA)
//		u_id = (GXTexCoordID)(((int)GX_TEXCOORD0)+uv_id);
//		GXSetTexCoordGen(u_id, GX_TG_SRTG, GX_TG_COLOR1, GX_IDENTITY);
//		GXSetTevOrder(GX_TEVSTAGE1, u_id, GX_TEXMAP7, GX_COLOR_NULL);
//		uv_id++;
//		GXSetNumChans( 2 );
//	}
//	else
//	{
//		GXSetNumChans( 1 );
//	}
//
//	// Set final number of textures/stages.
//	GXSetNumTexGens( uv_id );
//    GXSetNumTevStages( stage_id );

	GXCullMode cull_mode;
	if ( pass_flags & (1<<3) )
	{
		// Semitransparent.
		if ( flags & (1<<2) )
		{
			cull_mode = GX_CULL_FRONT;
		}
		else
		{
			cull_mode = GX_CULL_NONE;
		}
	}
	else
	{
		// Opaque.
		if ( flags & (1<<1) )
		{
			cull_mode = GX_CULL_NONE;
		}
		else
		{
			cull_mode = GX_CULL_FRONT;
		}
	}
	if ( !cull )
	{
		cull_mode = GX_CULL_NONE; 
	}

////	if ( EngineGlobals.poly_culling && ( flags & (1<<1) ) )
//	if ( flags & (1<<1) )
//	{
//		cull_mode = GX_CULL_FRONT;
//	}

//	if(bl && _ta) GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );





	// Issue buffered commands.
	int lp;
	for ( lp = 0; lp < 16; lp += 2 )
	{
		if ( sel[lp] || sel[lp+1] )
		{
			if ( bl && _su ) GX::SetTevKSel( (GXTevStageID)lp, csel[lp], asel[lp], csel[lp+1], asel[lp+1] );
//			GDSetTevKonstantSel( (GXTevStageID)lp, csel[lp], asel[lp], csel[lp+1], asel[lp+1] );
		}
		if ( ord[lp] || ord[lp+1] )
		{
			if ( bl && _su ) GX::SetTevOrder( (GXTevStageID)lp, ordt[lp], ordm[lp], ordc[lp], ordt[lp+1], ordm[lp+1], ordc[lp+1] );
//			GDSetTevOrder( (GXTevStageID)lp, ordt[lp], ordm[lp], ordc[lp], ordt[lp+1], ordm[lp+1], ordc[lp+1] );
		}
	}

	if ( settex )
	{
		GX::SetCurrMtxPosTex03( GX_PNMTX0, texmtx[0], texmtx[1], texmtx[2], texmtx[3] );
		GX::SetCurrMtxTex47( GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

//		GDSetCurrentMtx( GX_PNMTX0,			// Will be overridden.
//						 texmtx[0],
//						 texmtx[1],
//						 texmtx[2],
//						 texmtx[3],
//						 texmtx[4],
//						 texmtx[5],
//						 texmtx[6],
//						 texmtx[7] );
	}


	if(bl && _ta) GX::SetTexChanTevIndCull( uv_id, color_channels, stage_id, 0, cull_mode );
//	if(bl && _ta) GX::SetTexChanTevIndCull( uv_id, 1, stage_id, 0, GX_CULL_FRONT );

//	for ( int lp = 0; lp < num_defer_gen; lp++ )
//	{
//		GX::SetTexCoordGen( defer_gen[lp].id, defer_gen[lp].type, defer_gen[lp].src, GX_FALSE, GX_PTIDENTITY );
//	}
//	num_defer_gen = 0;

	// Clear so that subsequent flushes do nothing.
	for ( lp = 0; lp < 16; lp++ )
	{
		asel[lp] = GX_TEV_KASEL_1;
		csel[lp] = GX_TEV_KCSEL_1;
		sel[lp] = false;

		ordt[lp] = (GXTexCoordID)(lp&7);
		ordm[lp] = (GXTexMapID)(lp&7);
		ordc[lp] = GX_COLOR0A0;
		ord[lp] = false;
	}

	for ( lp = 0; lp < 8; lp++ )
	{
		texmtx[lp] = GX_IDENTITY;
	}

	settex = false;
}

static void _multi_mesh( sMaterialHeader * p_mat, sMaterialPassHeader * p_base_pass, bool bl, bool tx, bool cull )
{
	uint32 layer;

	multi_start( bl, tx );

	sMaterialPassHeader * p_pass = p_base_pass;

//	int uv_set = 0;

	int passes = p_mat->m_passes;
	if ( g_mat_passes < passes )
	{
		if ( g_mat_passes > 0 )
		{
			passes = g_mat_passes;
		}
	}

	for ( layer = 0; layer < (uint)passes; layer++, p_pass++ )
	{
		bool ignore_alpha = ( p_pass->m_flags & (1<<7) ) ? true : false;

		GXTexWrapMode u_mode;
		GXTexWrapMode v_mode;

		u_mode = p_pass->m_flags & (1<<5) ? GX_CLAMP : GX_REPEAT;
		v_mode = p_pass->m_flags & (1<<6) ? GX_CLAMP : GX_REPEAT;

		if ( p_pass->m_texture.p_data )
		{
			multi_add_texture ( p_pass->m_texture.p_data,
								p_pass->m_color,
								u_mode,
								v_mode,
								(BlendModes)p_pass->m_blend_mode,
								p_pass->m_alpha_fix,
								(float)(p_pass->m_k) * (1.0f / (float)(1<<8)),
								0.0f,	//p_mat->m_shininess,
								p_pass->m_flags,
								layer,
								ignore_alpha,
								bl,
								tx );
		}
		else
		{
			// Untextured.
			GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
			BlendModes blend = (BlendModes)p_pass->m_blend_mode;
			uint8 fix = p_pass->m_alpha_fix;

			
			GXColor col;
			if ( blend == vBLEND_MODE_BRIGHTEN_FIXED )
			{
				col.r = fix;
				col.g = fix;
				col.b = fix;
				col.a = fix;
			}
			else
			{
				col.r = p_pass->m_color.r;
				col.g = p_pass->m_color.g;
				col.b = p_pass->m_color.b;
				col.a = fix;
			}
			if(tx && _su) GX::SetTevKColor(				(GXTevKColorID)(((int)GX_KCOLOR0)+tev_id),
													col ); 

			if(bl && _su)
			{
				csel[(int)s_id] = (GXTevKColorSel)(((int)GX_TEV_KCSEL_K0)+layer_id); 
				asel[(int)s_id] = (GXTevKAlphaSel)(((int)GX_TEV_KASEL_K0_A)+layer_id);
				sel[(int)s_id] = true;
			}

			if(bl && _su)
			{
				ordt[(int)s_id] = GX_TEXCOORD0;
				ordm[(int)s_id] = GX_TEX_DISABLE;
				ordc[(int)s_id] = GX_COLOR0A0;
				ord[(int)s_id] = true;
			}

			switch ( blend )
			{
				case vBLEND_MODE_BRIGHTEN_FIXED:
					{
						if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_KONST, GX_CA_RASA, GX_CA_ZERO,
																GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id),
																GX_TEV_SWAP0, GX_TEV_SWAP0 );
						if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, GX_CC_KONST, GX_CC_RASA, GX_CC_ZERO,
																GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
						stage_id++;
					}
					break;
				case vBLEND_MODE_BRIGHTEN:
					if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
															GX_TEV_ADD, GX_TB_ADDHALF, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id),
															GX_TEV_SWAP0, GX_TEV_SWAP0 );
					if ( ignore_alpha )
					{
						if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
																GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
					}
					else
					{
						if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_RASA, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
																GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
					}

					stage_id++;
					break;
				case vBLEND_MODE_ADD_FIXED:
				case vBLEND_MODE_SUB_FIXED:
				case vBLEND_MODE_BLEND_FIXED:
				case vBLEND_MODE_MODULATE_FIXED:
					{
						if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
																GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id),
																GX_TEV_SWAP0, GX_TEV_SWAP0 );
						if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, GX_CC_RASC, GX_CC_KONST, GX_CC_ZERO,
																GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
						stage_id++;
					}
					break;
				case vBLEND_MODE_ADD:
				case vBLEND_MODE_SUBTRACT:
				case vBLEND_MODE_BLEND:
				case vBLEND_MODE_DIFFUSE:
				case vBLEND_MODE_MODULATE:
				case vBLEND_MODE_BLEND_PREVIOUS_MASK:
				case vBLEND_MODE_BLEND_INVERSE_PREVIOUS_MASK:
				default:
					if ( ignore_alpha )
					{
						if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
																GX_TEV_ADD, GX_TB_ADDHALF, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id),
																GX_TEV_SWAP0, GX_TEV_SWAP0 );
					}
					else
					{
						if(bl && _su) GX::SetTevAlphaInOpSwap( s_id,	GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
																GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id),
																GX_TEV_SWAP0, GX_TEV_SWAP0 );
					}

					if(bl && _su) GX::SetTevColorInOp( s_id,		GX_CC_ZERO, GX_CC_RASC, GX_CC_KONST, GX_CC_ZERO,
															GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
					stage_id++;
					break;
			}

			if ( tev_id > 0 ) {
				multi_add_layer ( blend, fix, bl, tx );
			} else {
				// Set blend mode for base layer.
				switch ( blend )
				{
					case vBLEND_MODE_ADD:
					case vBLEND_MODE_ADD_FIXED:
						if(bl && _su) GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
						break;
					case vBLEND_MODE_SUBTRACT:
					case vBLEND_MODE_SUB_FIXED:
						if(bl && _su) GX::SetBlendMode ( GX_BM_SUBTRACT, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
						break;
					case vBLEND_MODE_BLEND:
					case vBLEND_MODE_BLEND_FIXED:
						if(bl && _su) GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
						break;
					case vBLEND_MODE_BRIGHTEN:
					case vBLEND_MODE_BRIGHTEN_FIXED:
						if(bl && _su) GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
						break;
					case vBLEND_MODE_MODULATE_FIXED:
					case vBLEND_MODE_MODULATE:
						if(bl && _su) GX::SetBlendMode ( GX_BM_BLEND, GX_BL_ZERO, GX_BL_SRCALPHA, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
						break;
					case vBLEND_MODE_BLEND_PREVIOUS_MASK:
					case vBLEND_MODE_BLEND_INVERSE_PREVIOUS_MASK:
					case vBLEND_MODE_DIFFUSE:
					default:
						if(bl && _su) GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
						break;
				}
			}
			tev_id++;
			layer_id++;
		}
	}
	
#ifdef __PLAT_WN32__
	uint8 has_holes = p_base_pass->m_texture.p_data ? ( ( p_base_pass->m_texture.p_data->m_PaletteFormat >> 1 ) & 1 ) : 0;
#else
	uint8 has_holes = p_base_pass->m_texture.p_data ? ( ( p_base_pass->m_texture.p_data->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_HOLES ) ? 1 : 0 ) : 0;
#endif
	multi_end( (BlendModes)p_base_pass->m_blend_mode, p_base_pass->m_flags, has_holes, p_mat->m_alpha_cutoff, p_mat->m_flags, bl, tx, cull );
}

void multi_mesh( sMaterialHeader * p_mat, sMaterialPassHeader * p_base_pass, bool bl, bool tx, bool should_correct_color, bool cull )
{
	correct_color = should_correct_color;
#ifdef __PLAT_WN32__
	g_tex_off[0] = -1;
	g_tex_off[1] = -1;
	g_tex_off[2] = -1;
	g_tex_off[3] = -1;

	g_alpha_off[0] = -1;
	g_alpha_off[1] = -1;
	g_alpha_off[2] = -1;
	g_alpha_off[3] = -1;
#endif		// __PLAT_WN32__

#ifdef __PLAT_WN32__
	_su = true;
	_tb = false;
	_tc = false;
	_ta = false;
	_multi_mesh( p_mat, p_base_pass, bl, tx, cull );
	_su = false;
	_tb = true;
	_tc = false;
	_ta = false;
	_multi_mesh( p_mat, p_base_pass, bl, tx, cull );
	_su = false;
	_tb = false;
	_tc = true;
	_ta = false;
	_multi_mesh( p_mat, p_base_pass, bl, tx, cull );
	_su = false;
	_tb = false;
	_tc = false;
	_ta = true;
	_multi_mesh( p_mat, p_base_pass, bl, tx, cull );
#else
	_su = true;
	_tb = true;
	_tc = true;
	_ta = true;
	_multi_mesh( p_mat, p_base_pass, bl, tx, cull );
#endif		// __PLAT_WN32__

//	su = true;
//	tb = true;
//	tc = true;
//	ta = true;
//	_multi_mesh( p_mat, p_base_pass, true, false );
//	_multi_mesh( p_mat, p_base_pass, false, true );
////	_multi_mesh( p_mat, p_base_pass, true, true );
}

}		// namespace NxNgc



