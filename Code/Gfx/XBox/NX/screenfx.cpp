#include <core/defines.h>
#include <core/debug.h>
#include <xgraphics.h>
#include "nx_init.h"
#include "render.h"
#include "screenfx.h"

//-----------------------------------------------------------------------------
// A filter sample holds a subpixel offset and a filter value
// to be multiplied by a source texture to compute an arbitrary
// filter.  See filter_copy for more details.
//-----------------------------------------------------------------------------
struct FilterSample 
{
    FLOAT fValue;               // Coefficient
    FLOAT fOffsetX, fOffsetY;   // Subpixel offsets of supersamples in destination coordinates
};

//-----------------------------------------------------------------------------
// The depth-mapping pixel shader attempts to do higher precision
// math with eight-bit color registers.  The _x4 instruction modifier
// is used twice to get a 16x range of values.
//-----------------------------------------------------------------------------
FLOAT g_fPixelShaderScale = 16.0f;   // to get into the right range, we scale up the value in the pixel shader
FLOAT m_fDepth0	= 0.900f;
FLOAT m_fDepth1	= 0.997f;
D3DTexture* m_pTextureFocusRange = NULL;   // Lookup table for range of z values to use

// Enumeration of blur filters available in this sample to compare the speed and quality of different types of blur filters for
// the out-of-focus parts of the scene.
enum FILTERMODE 
{
        FM_BOX,
        FM_VERT,
        FM_HORIZ, 
        FM_BOX2,
        FM_VERT2,
        FM_HORIZ2,
        FM_BOX2_BOX2,
		FM_VERT2_HORIZ2,
        FM_HORIZ2_VERT2,
        FM_VERT2_HORIZ,
        FM_HORIZ2_VERT,
        FM_VERT2_HORIZ2_BOX2,
        FM_BOX2_BOX2_BOX2,
        FM_VERT2_HORIZ2_VERT2,
        FM_HORIZ2_VERT2_HORIZ2,
        FM_VERT2_HORIZ2_VERT2_HORIZ2,
        FM_HORIZ2_VERT2_HORIZ2_VERT2,
        FM_BOX2_BOX2_BOX2_BOX2,
        _FM_MAX
    };





namespace NxXbox
{


void draw_rain( void )
{
	static float table_x[512];
	static float table_y[512];
	static bool table_set = false;
	static int table_index = 0;
	if( !table_set )
	{
		table_set = true;
		for( int i = 0; i < 512; ++i )
		{
			table_x[i] = ((float)( rand() - ( RAND_MAX / 2 )) * 32.0f ) / (float)RAND_MAX;
			table_y[i] = ((float)rand() * 12.0f ) / (float)RAND_MAX;
		}
	}
	
	static float radius = 24.0f;

	// Distance of the drip from the camera.
	static float distance = 10.0f * 12.0f;

	// Drip vector.
	static Mth::Vector offset( 0.0f, -1.0f * radius, 0.0f );
	static Mth::Vector offset_target( 0.0f, -1.0f * radius, 0.0f );

	static int num_drops = 1000;

	if(( rand() & 63 ) == 1 )
	{
		offset_target[X] = ( radius * 0.25f * ( rand() - ( RAND_MAX / 2 ))) / RAND_MAX;
		offset_target[Y] = ( radius * -1.0f * rand() ) / RAND_MAX;
		offset_target[Z] = ( radius * 0.25f * ( rand() - ( RAND_MAX / 2 ))) / RAND_MAX;
		
		offset_target.Normalize( radius );
	}
	
	offset += ( offset_target - offset ) * 0.2f;
	offset.Normalize( radius );	
	
	D3DXVECTOR4 or( EngineGlobals.cam_position.x + ( distance * EngineGlobals.cam_at.x ),
					EngineGlobals.cam_position.y + ( distance * EngineGlobals.cam_at.y ),
					EngineGlobals.cam_position.z + ( distance * EngineGlobals.cam_at.z ),
					1.0f );
	D3DXVECTOR4 of( or.x + offset[X], or.y + offset[Y], or.z + offset[Z], 1.0f );

	D3DXVec4Transform( &or, &or, (D3DXMATRIX*)&EngineGlobals.view_matrix );
	D3DXVec4Transform( &of, &of, (D3DXMATRIX*)&EngineGlobals.view_matrix );

	D3DXVec4Transform( &or, &or, (D3DXMATRIX*)&EngineGlobals.projection_matrix );
	D3DXVec4Transform( &of, &of, (D3DXMATRIX*)&EngineGlobals.projection_matrix );

	or.x /= or.w;
	or.y /= or.w;

	of.x /= of.w;
	of.y /= of.w;

	of.y = -of.y;
	
//	printf( "(%.2f %.2f) (%.2f %.2f)\n", or.x, or.y, of.x, of.y );

	// Obtain push buffer lock.
	DWORD *p_push; 
	DWORD dwords_per_particle	= 10;
	DWORD dword_count			= dwords_per_particle * num_drops;

	// Submit particle material.
//	mp_engine_particle->mp_material->Submit();
	NxXbox::set_blend_mode( vBLEND_MODE_BLEND );
		
	// Set up correct vertex and pixel shader.
	NxXbox::set_vertex_shader( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );
	NxXbox::set_pixel_shader( PixelShader5 );
		
	// The additional number (+5 is minimum) is to reserve enough overhead for the encoding parameters. It can safely be more, but no less.
	p_push = D3DDevice_BeginPush( dword_count + 32 );

	// Note that p_push is returned as a pointer to write-combined memory. Writes to write-combined memory should be
	// consecutive and in increasing order. Reads should be avoided. Additionally, any CPU reads from memory or the
	// L2 cache can force expensive partial flushes of the 32-byte write-combine cache.
	p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
	p_push[1]	= D3DPT_LINELIST;
	p_push		+= 2; 

	// Set up loop variables here, since we be potentially entering the loop more than once.
	int lp = 0;

	while( dword_count > 0 )
	{
		int dwords_written = 0;

		// NOTE: A maximum of 2047 DWORDs can be specified to D3DPUSH_ENCODE. If there is more than 2047 DWORDs of vertex
		// data, simply split the data into multiple D3DPUSH_ENCODE( D3DPUSH_INLINE_ARRAY ) sections.
		p_push[0] = D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, ( dword_count > 2047 ) ? ((int)( 2047 / dwords_per_particle )) * dwords_per_particle: dword_count );
		++p_push;
		
		for( ; lp < num_drops; lp++ )
		{
			// Check to see if writing another particle will take us over the edge.
			if(( dwords_written + dwords_per_particle ) > 2047 )
			{
				break;
			}

			float	screen_x0		= (float)(( NxXbox::EngineGlobals.backbuffer_width * rand() ) / RAND_MAX );
			float	screen_y0		= (float)(( NxXbox::EngineGlobals.backbuffer_height * rand() ) / RAND_MAX );

			int		random_length	= rand();
			
			float	screen_x1		= (float)( screen_x0 + (( of.x * ( EngineGlobals.backbuffer_width / 2 ) * random_length ) / RAND_MAX ));
			float	screen_y1		= (float)( screen_y0 + (( of.y * ( EngineGlobals.backbuffer_height / 2 ) * random_length ) / RAND_MAX ));

			screen_x1		+= table_x[table_index];
			screen_y1		+= table_y[table_index];
			table_index		= ( table_index >= 512 ) ? 0 : ( table_index + 1 );

			p_push[0]	= *((DWORD*)&screen_x0 );
			p_push[1]	= *((DWORD*)&screen_y0 );
			p_push[2]	= 0x00000000UL;
			p_push[3]	= 0x00000000UL;
			p_push[4]	= 0xA0808080UL;
			p_push		+= 5;

			p_push[0]	= *((DWORD*)&screen_x1 );
			p_push[1]	= *((DWORD*)&screen_y1 );
			p_push[2]	= 0x00000000UL;
			p_push[3]	= 0x00000000UL;
			p_push[4]	= 0x20808080UL;
			p_push		+= 5;

			dwords_written	+= dwords_per_particle;
			dword_count		-= dwords_per_particle;
		}
	}
	p_push[0] = D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
	p_push[1] = 0;
	p_push += 2;
	D3DDevice_EndPush( p_push );
}

	
	
	
//////////////////////////////////////////////////////////////////////
// For mapping from the depth buffer to blend values using
// a texture map lookup. See media\shaders\depthlookup.psh
//
// This is more general than computing the range as in
// media\shaders\depth.psh, since the ramp can be filled in
// arbitrarily, but may be more expensive due to the extra texture
// lookup.
//
float FUnitMap(float fAlpha, float fBlue, float fAlphaOffset, float fAlphaSlope, float fBlueOffset, float fBlueSlope)
{
    //return g_fPixelShaderScale * fAlphaSlope * (fAlpha - fAlphaOffset) + fBlueSlope * fBlue + fBlueOffset - 0.5f;
    return g_fPixelShaderScale * fAlphaSlope * (fAlpha - fAlphaOffset) + fBlueSlope * (fBlue - fBlueOffset);
}

float FQuantizedDepth(float fDepth, float *pfAlpha, float *pfBlue)
{
    float fDepth16 = fDepth * (float)(1 << 16);
    DWORD dwDepth16 = (DWORD)(fDepth16 /*+ 0.5f*/);
    *pfAlpha = (dwDepth16 >> 8) * (1.0f / 255.0f);
    *pfBlue = (dwDepth16 & 0xff) * (1.0f / 255.0f);
    return (float)dwDepth16 / (float)(1 << 16);
}

	
	
	
//-----------------------------------------------------------------------------
// Name: CalculateDepthMapping()
// Desc: Calculate offsets and slope to map given z range to 0,1 in
//       the depth and focus pixel shaders.
//-----------------------------------------------------------------------------
static HRESULT calculate_depth_mapping( float fDepth0, float fDepth1, float* pfAlphaOffset, float* pfAlphaSlope, float* pfBlueOffset, float* pfBlueSlope )
{
    // Check range of args
    if( fDepth0 < 0.0f ) fDepth0 = 0.0f;
    if( fDepth0 > 1.0f ) fDepth0 = 1.0f;
    if( fDepth1 < 0.0f ) fDepth1 = 0.0f;
    if( fDepth1 > 1.0f ) fDepth1 = 1.0f;

    if( fDepth1 < fDepth0 )
    {
        // Swap depth to make fDepth0 <= fDepth1
        float t = fDepth1;
        fDepth1 = fDepth0;
        fDepth0 = t;
    }
    
    // Calculate quantized values
    float fAlpha0, fBlue0;
    float fQuantizedDepth0 = FQuantizedDepth(fDepth0, &fAlpha0, &fBlue0);
    float fAlpha1, fBlue1;
    float fQuantizedDepth1 = FQuantizedDepth(fDepth1, &fAlpha1, &fBlue1);

    // Calculate offset and slopes
    float fScale = 1.0f / (fQuantizedDepth1 - fQuantizedDepth0);
    if( fScale > g_fPixelShaderScale )
    {
        fScale = g_fPixelShaderScale; // This is the steepest slope we can handle
        fDepth0 = 0.5f * (fDepth0 + fDepth1) - 0.5f / fScale; // Move start so that peak is in middle of fDepth0 and fDepth1
        fDepth1 = fDepth0 + 1.0f / fScale;
        fQuantizedDepth0 = FQuantizedDepth(fDepth0, &fAlpha0, &fBlue0);
        fQuantizedDepth1 = FQuantizedDepth(fDepth1, &fAlpha1, &fBlue1);
    }
    
    (*pfAlphaOffset) = fAlpha0;
    (*pfAlphaSlope)  = fScale / g_fPixelShaderScale;
    (*pfBlueSlope)   = fScale * (1.0f/255.0f); // blue ramp adds more levels to the ramp

    // Align peak of map to center by calculating the quantized alpha value
//    *pfBlueOffset = 0.5f;   // zero biased up by 0.5f
//    float fZeroDesired = (fQuantizedDepth0 - fDepth0) / (fDepth1 - fDepth0);
//    float fZero = FUnitMap(fAlpha0, fBlue0, *pfAlphaOffset, *pfAlphaSlope, *pfBlueOffset, *pfBlueSlope);
//    float fOneDesired = (fQuantizedDepth1 - fDepth0) / (fDepth1 - fDepth0);
//    float fOne = FUnitMap(fAlpha1, fBlue1, *pfAlphaOffset, *pfAlphaSlope, *pfBlueOffset, *pfBlueSlope);
//    *pfBlueOffset = 0.5f * (fZeroDesired-fZero + fOneDesired-fOne) + 0.5f;  // biased up by 0.5f
    (*pfBlueOffset) = fBlue0;
    
    return S_OK;
}

	

	
	
static HRESULT fill_focus_range_texture( bool bRamp )
{
    HRESULT hr;

    static const DWORD Width	= 256;
    static const DWORD Height	= 256;
    
    // Create the focus range texture
    if( m_pTextureFocusRange )
        m_pTextureFocusRange->Release();
	EngineGlobals.p_Device->CreateTexture( Width, Height, 1, 0, D3DFMT_A8, 0, &m_pTextureFocusRange );
    
    // Fill the focus range texture
    D3DLOCKED_RECT lockedRect;
    hr = m_pTextureFocusRange->LockRect( 0, &lockedRect, NULL, 0L );
    if( FAILED(hr) )
        return hr;
    
    DWORD dwPixelStride = 1;
    Swizzler s(Width, Height, 0);
    s.SetV(s.SwizzleV(0));
    s.SetU(s.SwizzleU(0));
    if( bRamp )
    {
        for( DWORD j = 0; j < Height; j++ )
        {
            for( DWORD i = 0; i < Width; i++ )
            {
                BYTE *p = (BYTE *)lockedRect.pBits + dwPixelStride * s.Get2D();
                *p = (BYTE)i;
                s.IncU();
            }
            s.IncV();
        }
    }
    else
    {
//        float fAlphaOffset, fAlphaSlope, fBlueOffset, fBlueSlope;
//        calculate_depth_mapping( m_fDepth0, m_fDepth1, &fAlphaOffset, &fAlphaSlope, &fBlueOffset, &fBlueSlope );
        for( DWORD i = 0; i < Width; i++ )
        {
			float z_high = (float)i / ( Width - 1 );

            for( DWORD j = 0; j < Height; j++ )
            {
                BYTE *p = (BYTE *)lockedRect.pBits + dwPixelStride * s.Get2D();
//                float fAlpha = (float)i / (Width - 1);
//                float fBlue  = (float)j / (Height - 1);
//                float fUnit  = 2.0f * (FUnitMap(fAlpha, fBlue, fAlphaOffset, fAlphaSlope, fBlueOffset, fBlueSlope) - 0.5f);
//                float fMap   = 1.0f - fUnit * fUnit;
//                if( fMap < 0.0f ) fMap = 0.0f;
//                if( fMap > 1.0f ) fMap = 1.0f;
//                *p = (BYTE)(255 * fMap + 0.5f);
                
				float z_low			= (float)j / ( Height - 1 );
				float quantized_z	= ((( z_high * 256.0f ) + z_low ) * 256.0f ) / 65536.0f;

				if( quantized_z < m_fDepth0 )
				{
					*p = 0;
				}
				else if( quantized_z > m_fDepth1 )
				{
					*p = 0;
				}
				else
				{
					*p = 255;
				}
				s.IncV();
            }
            s.IncU();
        }
    }
    
	m_pTextureFocusRange->UnlockRect( 0 );

    return S_OK;
}
	
	
	
	
	
//-----------------------------------------------------------------------------
// Name: filter_copy()
// Desc: Filter the source texture by rendering into the destination texture
//       with subpixel offsets. Does 4 filter coefficients at a time, using all
//       the stages of the pixel shader.
//-----------------------------------------------------------------------------

D3DTexture* m_pBlur;
	
static HRESULT filter_copy( LPDIRECT3DTEXTURE8 pTextureDst,
                                 LPDIRECT3DTEXTURE8 pTextureSrc,
                                 DWORD dwNumSamples,
                                 FilterSample rSample[],
                                 DWORD dwSuperSampleX,
                                 DWORD dwSuperSampleY )
{
    // Set destination as render target, with no-depth buffer
    LPDIRECT3DSURFACE8 pSurface;
    pTextureDst->GetSurfaceLevel( 0, &pSurface );
    EngineGlobals.p_Device->SetRenderTarget( pSurface, NULL );
    pSurface->Release();

    // Get descriptions of source and destination
    D3DSURFACE_DESC descSrc;
    pTextureSrc->GetLevelDesc( 0, &descSrc );

	if( descSrc.MultiSampleType == D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_LINEAR )
	{
		descSrc.Width *= 2;
	}
	
	// Set render state for filtering
    EngineGlobals.p_Device->SetRenderState( D3DRS_LIGHTING,         FALSE );
	set_render_state( RS_ZWRITEENABLE,		0 );
	set_render_state( RS_ZTESTENABLE,		0 );
	set_render_state( RS_ALPHATESTENABLE,	0 );
	set_render_state( RS_ALPHABLENDENABLE,	0 );
    EngineGlobals.p_Device->SetRenderState( D3DRS_BLENDOP,          D3DBLENDOP_ADD ); // Setup subsequent renderings to add to previous value
    EngineGlobals.p_Device->SetRenderState( D3DRS_SRCBLEND,         D3DBLEND_ONE );
    EngineGlobals.p_Device->SetRenderState( D3DRS_DESTBLEND,        D3DBLEND_ONE );

    // Set texture state
    DWORD xx;
    for( xx = 0; xx < 4; xx++ )
    {
        set_texture( xx, pTextureSrc );  // use our source texture for all four stages

        EngineGlobals.p_Device->SetTextureStageState( xx, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );  // pass texture coords without transformation
        EngineGlobals.p_Device->SetTextureStageState( xx, D3DTSS_TEXCOORDINDEX, xx ); // each texture has different tex coords
		set_render_state( RS_UVADDRESSMODE0 + xx, 0x00010001UL );
        EngineGlobals.p_Device->SetTextureStageState( xx, D3DTSS_ALPHAKILL, D3DTALPHAKILL_DISABLE );
    }
    
	// Use blur pixel shader.
	set_pixel_shader( PixelShaderFocusBlur );
	set_vertex_shader( D3DFVF_XYZRHW | D3DFVF_TEX4 );

    // Prepare quadrilateral vertices
    float x0 = -0.5f;
    float y0 = -0.5f;
    float x1 = (float)( descSrc.Width  / dwSuperSampleX ) - 0.5f;
    float y1 = (float)( descSrc.Height / dwSuperSampleY ) - 0.5f;
    struct QUAD
    {
        float x, y, z, w1;
        struct uv 
        {
            float u, v;
        } tex[4];   // each texture has different offset
    };
    
    QUAD aQuad[4] = 
    {
        { x0, y0, 1.0f, 1.0f, }, // texture coords are set below
        { x1, y0, 1.0f, 1.0f, },
        { x0, y1, 1.0f, 1.0f, },
        { x1, y1, 1.0f, 1.0f, }
    };

    // Draw a quad for each block of 4 filter coefficients
    xx = 0; // current texture stage
    FLOAT fOffsetScaleX, fOffsetScaleY; // convert destination coords to source texture coords
    FLOAT u0, v0, u1, v1;   // base source rectangle.
	if( XGIsSwizzledFormat( descSrc.Format ))
    {
        FLOAT fWidthScale  = 1.0f / (FLOAT)descSrc.Width;
        FLOAT fHeightScale = 1.0f / (FLOAT)descSrc.Height;
        fOffsetScaleX = (FLOAT)dwSuperSampleX * fWidthScale;
        fOffsetScaleY = (FLOAT)dwSuperSampleY * fHeightScale;
        u0 = 0.0f;
        v0 = 0.0f;
        u1 = (FLOAT)descSrc.Width * fWidthScale;
        v1 = (FLOAT)descSrc.Height * fHeightScale;
    }
    else
    {
        fOffsetScaleX = (FLOAT)dwSuperSampleX;
        fOffsetScaleY = (FLOAT)dwSuperSampleY;
        u0 = 0.0f;
        v0 = 0.0f;
        u1 = (FLOAT)descSrc.Width;
        v1 = (FLOAT)descSrc.Height;
	}
    D3DCOLOR rColor[4];
    DWORD rPSInput[4];
    for( DWORD dwSample = 0; dwSample < dwNumSamples; dwSample++ )
    {
        // Set filter coefficients
        FLOAT fValue = rSample[dwSample].fValue;
//      float rf[4] = {fValue, fValue, fValue, fValue};
//      EngineGlobals.p_Device->SetPixelShaderConstant(xx, rf, 1);            // positive coeff
  
        if( fValue < 0.0f )
        {
            rColor[xx] = D3DXCOLOR(-fValue, -fValue, -fValue, -fValue);
            rPSInput[xx] = PS_INPUTMAPPING_SIGNED_NEGATE | ((xx % 2) ? PS_REGISTER_C1 : PS_REGISTER_C0);
        }
        else
        {
            rColor[xx] = D3DXCOLOR(fValue, fValue, fValue, fValue);
            rPSInput[xx] = PS_INPUTMAPPING_SIGNED_IDENTITY | ((xx % 2) ? PS_REGISTER_C1 : PS_REGISTER_C0);
        }

        // Align supersamples with center of destination pixels
        FLOAT fOffsetX = rSample[dwSample].fOffsetX * fOffsetScaleX;
        FLOAT fOffsetY = rSample[dwSample].fOffsetY * fOffsetScaleY;
        aQuad[0].tex[xx].u = u0 + fOffsetX;
        aQuad[0].tex[xx].v = v0 + fOffsetY;
        aQuad[1].tex[xx].u = u1 + fOffsetX;
        aQuad[1].tex[xx].v = v0 + fOffsetY;
        aQuad[2].tex[xx].u = u0 + fOffsetX;
        aQuad[2].tex[xx].v = v1 + fOffsetY;
        aQuad[3].tex[xx].u = u1 + fOffsetX;
        aQuad[3].tex[xx].v = v1 + fOffsetY;
        
        xx++; // Go to next stage
        if( xx == 4 || dwSample == dwNumSamples - 1 )  // max texture stages or last sample
        {
            // zero out unused texture stage coefficients 
            // (only for last filter sample, when number of samples is not divisible by 4)
            for( ; xx < 4; xx++ )
            {
				set_texture( xx, NULL );
                rColor[xx] = 0;
                rPSInput[xx] = PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_REGISTER_ZERO;
            }
        
            // Set coefficients
            EngineGlobals.p_Device->SetRenderState( D3DRS_PSCONSTANT0_0, rColor[0] );
            EngineGlobals.p_Device->SetRenderState( D3DRS_PSCONSTANT1_0, rColor[1] );
            EngineGlobals.p_Device->SetRenderState( D3DRS_PSCONSTANT0_1, rColor[2] );
            EngineGlobals.p_Device->SetRenderState( D3DRS_PSCONSTANT1_1, rColor[3] );

            // Remap coefficients to proper sign
            EngineGlobals.p_Device->SetRenderState( D3DRS_PSRGBINPUTS0,
                                          PS_COMBINERINPUTS( rPSInput[0] | PS_CHANNEL_RGB,   PS_REGISTER_T0 | PS_CHANNEL_RGB   | PS_INPUTMAPPING_SIGNED_IDENTITY,
                                                             rPSInput[1] | PS_CHANNEL_RGB,   PS_REGISTER_T1 | PS_CHANNEL_RGB   | PS_INPUTMAPPING_SIGNED_IDENTITY ) );
            EngineGlobals.p_Device->SetRenderState( D3DRS_PSALPHAINPUTS0,
                                          PS_COMBINERINPUTS( rPSInput[0] | PS_CHANNEL_ALPHA, PS_REGISTER_T0 | PS_CHANNEL_ALPHA | PS_INPUTMAPPING_SIGNED_IDENTITY,
                                                             rPSInput[1] | PS_CHANNEL_ALPHA, PS_REGISTER_T1 | PS_CHANNEL_ALPHA | PS_INPUTMAPPING_SIGNED_IDENTITY ) );
            EngineGlobals.p_Device->SetRenderState( D3DRS_PSRGBINPUTS1,
                                          PS_COMBINERINPUTS( rPSInput[2] | PS_CHANNEL_RGB,   PS_REGISTER_T2 | PS_CHANNEL_RGB   | PS_INPUTMAPPING_SIGNED_IDENTITY,
                                                             rPSInput[3] | PS_CHANNEL_RGB,   PS_REGISTER_T3 | PS_CHANNEL_RGB   | PS_INPUTMAPPING_SIGNED_IDENTITY ) );
            EngineGlobals.p_Device->SetRenderState( D3DRS_PSALPHAINPUTS1,
                                          PS_COMBINERINPUTS( rPSInput[2] | PS_CHANNEL_ALPHA, PS_REGISTER_T2 | PS_CHANNEL_ALPHA | PS_INPUTMAPPING_SIGNED_IDENTITY,
                                                             rPSInput[3] | PS_CHANNEL_ALPHA, PS_REGISTER_T3 | PS_CHANNEL_ALPHA | PS_INPUTMAPPING_SIGNED_IDENTITY ) );
            
            // Draw the quad to filter the coefficients so far
			EngineGlobals.p_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, aQuad, sizeof( QUAD )); // one quad blends 4 textures

			// On subsequent renderings, add to what's in the render target.
			set_render_state( RS_ALPHABLENDENABLE,	1 );
			xx = 0;
        }
    }

    // Clear texture stages
    for( xx=0; xx<4; xx++ )
    {
		set_texture( xx, NULL );
    }

	// Restore render target, zbuffer, and state.
    set_pixel_shader( NULL );
	EngineGlobals.p_Device->SetRenderTarget( EngineGlobals.p_RenderSurface, EngineGlobals.p_ZStencilSurface );

	return S_OK;
}
	
	
	
//-----------------------------------------------------------------------------
// Name: Blur()
// Desc: Blur backbuffer and set m_pBlur to the current blur texture.  Calls
//       filter_copy() with different filter coefficients and offsets, based on
//       the current FILTERMODE setting.
//-----------------------------------------------------------------------------
static HRESULT focus_blur( void )
{
	D3DTexture	m_BackBufferTexture;
	D3DTexture	m_BlurTexture[5];
	
	FILTERMODE filter_mode = FM_BOX2_BOX2;
	
	int	multisample_adjusted_width = EngineGlobals.backbuffer_width * 2;
	
	XGSetTextureHeader( multisample_adjusted_width, EngineGlobals.backbuffer_height, 1, 0,
                        EngineGlobals.backbuffer_format, 0, &m_BackBufferTexture, 
						EngineGlobals.p_RenderSurface->Data,
                        multisample_adjusted_width * XGBytesPerPixelFromFormat( EngineGlobals.backbuffer_format ));

	XGSetTextureHeader( EngineGlobals.backbuffer_width, EngineGlobals.backbuffer_height, 1, 0,
	                    EngineGlobals.blurbuffer_format, 0, &m_BlurTexture[0], 
						EngineGlobals.p_BlurSurface[0]->Data,
				        EngineGlobals.backbuffer_width * XGBytesPerPixelFromFormat( EngineGlobals.blurbuffer_format ));

	XGSetTextureHeader( EngineGlobals.backbuffer_width / 2, EngineGlobals.backbuffer_height / 2, 1, 0,
	                    EngineGlobals.blurbuffer_format, 0, &m_BlurTexture[1], 
						EngineGlobals.p_BlurSurface[1]->Data,
				        EngineGlobals.backbuffer_width / 2 * XGBytesPerPixelFromFormat( EngineGlobals.blurbuffer_format ));

	XGSetTextureHeader( EngineGlobals.backbuffer_width / 4, EngineGlobals.backbuffer_height / 4, 1, 0,
	                    EngineGlobals.blurbuffer_format, 0, &m_BlurTexture[2], 
						EngineGlobals.p_BlurSurface[2]->Data,
				        EngineGlobals.backbuffer_width / 4 * XGBytesPerPixelFromFormat( EngineGlobals.blurbuffer_format ));

	XGSetTextureHeader( EngineGlobals.backbuffer_width / 8, EngineGlobals.backbuffer_height / 8, 1, 0,
	                    EngineGlobals.blurbuffer_format, 0, &m_BlurTexture[3], 
						EngineGlobals.p_BlurSurface[3]->Data,
				        EngineGlobals.backbuffer_width / 8 * XGBytesPerPixelFromFormat( EngineGlobals.blurbuffer_format ));
	
	// Filters align to blurriest point in supersamples, on the 0.5 boundaries.
    // This takes advantage of the bilinear filtering in the texture map lookup.
    static FilterSample BoxFilter[] =     // for 2x2 downsampling
    {
        { 0.25f, -0.5f, -0.5f },
        { 0.25f,  0.5f, -0.5f },
        { 0.25f, -0.5f,  0.5f },
        { 0.25f,  0.5f,  0.5f },
    };
    static FilterSample YFilter[] =       // 1221 4-tap filter in Y
    {
        { 1.0f/6.0f, 0.0f, -1.5f },
        { 2.0f/6.0f, 0.0f, -0.5f },
        { 2.0f/6.0f, 0.0f,  0.5f },
        { 1.0f/6.0f, 0.0f,  1.5f },
    };
    static FilterSample XFilter[] =       // 1221 4-tap filter in X
    {
        { 1.0f/6.0f, -1.5f, 0.0f },
        { 2.0f/6.0f, -0.5f, 0.0f },
        { 2.0f/6.0f,  0.5f, 0.0f },
        { 1.0f/6.0f,  1.5f, 0.0f },
    };
    static FilterSample Y141Filter[] =    // 141 3-tap filter in Y
    {
        { 1.0f/6.0f, 0.0f, -1.0f },
        { 4.0f/6.0f, 0.0f,  0.0f },
        { 1.0f/6.0f, 0.0f,  1.0f },
    };
    static FilterSample X141Filter[] =        // 141 3-tap filter in X
    {
        { 1.0f/6.0f, -1.0f, 0.0f },
        { 4.0f/6.0f,  0.0f, 0.0f },
        { 1.0f/6.0f,  1.0f, 0.0f },
    };
    static FilterSample IdentityFilter[] = // No filtering
    {
        { 1.0f, 0.0f, 0.0f },
    };

    switch( filter_mode )
    {
        case FM_BOX:
        {
            // Blur from the backbuffer to the blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[0];
			filter_copy( pTextureDst, pTextureSrc, 4, BoxFilter, 1, 1 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[0];
            break;
        }

        case FM_VERT:
        {
            // Blur from the backbuffer to the blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[0];
            filter_copy( pTextureDst, pTextureSrc, 4, YFilter, 1, 1 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[0];
            break;
        }

        case FM_HORIZ:
        {
            // Blur from the backbuffer to the blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[0];
            filter_copy( pTextureDst, pTextureSrc, 4, XFilter, 1, 1 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[0];
            break;
        }

        case FM_BOX2:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture *pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, BoxFilter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[1];
            break;
        }

        case FM_VERT2:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, YFilter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[1];
            break;
        }

        case FM_HORIZ2:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture *pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, XFilter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[1];
            break;
        }
        
        case FM_VERT2_HORIZ2:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, YFilter, 2, 2 );
            
            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[1];
            pTextureDst = &m_BlurTexture[2];
            filter_copy( pTextureDst, pTextureSrc, 4, XFilter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[2];
            break;
        }

        case FM_HORIZ2_VERT2:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, XFilter, 2, 2 );
            
            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[1];
            pTextureDst = &m_BlurTexture[2];
            filter_copy( pTextureDst, pTextureSrc, 4, YFilter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[2];
            break;
        }

        case FM_VERT2_HORIZ:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, YFilter, 2, 2 );
            
            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[1];
            pTextureDst = &m_BlurTexture[2];
            filter_copy( pTextureDst, pTextureSrc, 3, X141Filter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[2];
            break;
        }

        case FM_HORIZ2_VERT:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, XFilter, 2, 2 );

            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[1];
            pTextureDst = &m_BlurTexture[2];  // destination is next blur texture
            filter_copy( pTextureDst, pTextureSrc, 3, Y141Filter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[2];
            break;
        }

        case FM_BOX2_BOX2:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[0];
            filter_copy( pTextureDst, pTextureSrc, 4, BoxFilter, 2, 1 );

            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[0];
            pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, BoxFilter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[1];
            break;
        }


        case FM_VERT2_HORIZ2_BOX2:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, YFilter, 2, 2 );

            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[1];
            pTextureDst = &m_BlurTexture[2];
            filter_copy( pTextureDst, pTextureSrc, 4, XFilter, 2, 2 );

            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[2];
            pTextureDst = &m_BlurTexture[3];
			filter_copy( pTextureDst, pTextureSrc, 4, BoxFilter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[3];
            break;
        }

        case FM_BOX2_BOX2_BOX2:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, BoxFilter, 2, 2 );

            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[1];
            pTextureDst = &m_BlurTexture[2];
            filter_copy( pTextureDst, pTextureSrc, 4, BoxFilter, 2, 2 );

            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[2];
            pTextureDst = &m_BlurTexture[3];
            filter_copy( pTextureDst, pTextureSrc, 4, BoxFilter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[3];
            break;
        }

        case FM_VERT2_HORIZ2_VERT2:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, YFilter, 2, 2 );

            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[1];
            pTextureDst = &m_BlurTexture[2];
            filter_copy( pTextureDst, pTextureSrc, 4, XFilter, 2, 2 );

            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[2];
            pTextureDst = &m_BlurTexture[3];
            filter_copy( pTextureDst, pTextureSrc, 4, YFilter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[3];
            break;
        }

        case FM_HORIZ2_VERT2_HORIZ2:
        {
            // Blur from the backbuffer to the 1/2 sized blur texture
            D3DTexture* pTextureSrc = &m_BackBufferTexture;
            D3DTexture* pTextureDst = &m_BlurTexture[1];
            filter_copy( pTextureDst, pTextureSrc, 4, XFilter, 2, 2 );

            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[1];
            pTextureDst = &m_BlurTexture[2];
            filter_copy( pTextureDst, pTextureSrc, 4, YFilter, 2, 2 );

            // Blur from the previous blur texture to the next blur texture
            pTextureSrc = &m_BlurTexture[2];
            pTextureDst = &m_BlurTexture[3];
            filter_copy( pTextureDst, pTextureSrc, 4, XFilter, 2, 2 );
            
            m_pBlur = (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[3];
            break;
        }

        default:
		{
            m_pBlur = NULL;
            break;
		}
    }
    return S_OK;
}




static HRESULT draw_focus_effect_using_planes( void )
{
    // Make a D3DTexture wrapper around the depth buffer surface
//    D3DTexture ZBufferTexture;
//    XGSetTextureHeader( EngineGlobals.backbuffer_width, EngineGlobals.backbuffer_height, 1, 0, 
//                        D3DFMT_LIN_A8B8G8R8, 0, &ZBufferTexture,
//                        EngineGlobals.p_ZStencilSurface->Data, EngineGlobals.backbuffer_width * 4 );

    // Get size of blur texture for setting texture coords of final blur
    D3DSURFACE_DESC descBlur;
    m_pBlur->GetLevelDesc( 0, &descBlur );
    float fOffsetX = 0.0f;
    float fOffsetY = 0.5f / (float)descBlur.Height; // vertical blur
    struct VERTEX 
    {
        D3DXVECTOR4 p;
        FLOAT tu0, tv0;

    } v[4];
    v[0].p = D3DXVECTOR4( -0.5f,									-0.5f,									1.0f, 1.0f );
    v[1].p = D3DXVECTOR4( EngineGlobals.backbuffer_width - 0.5f,	-0.5f,									1.0f, 1.0f );
    v[2].p = D3DXVECTOR4( -0.5f,									EngineGlobals.backbuffer_height - 0.5f,	1.0f, 1.0f );
    v[3].p = D3DXVECTOR4( EngineGlobals.backbuffer_width - 0.5f,	EngineGlobals.backbuffer_height - 0.5f, 1.0f, 1.0f );
    v[0].tu0 = fOffsetX;							v[0].tv0 = fOffsetY;
    v[1].tu0 = fOffsetX + (float)descBlur.Width;	v[1].tv0 = fOffsetY;
    v[2].tu0 = fOffsetX;							v[2].tv0 = fOffsetY + (float)descBlur.Height;
    v[3].tu0 = fOffsetX + (float)descBlur.Width;	v[3].tv0 = fOffsetY + (float)descBlur.Height;
	
	// Set pixel shader state
//    float fAlphaOffset, fAlphaSlope, fBlueOffset, fBlueSlope;
//   calculate_depth_mapping( m_fDepth0, m_fDepth1, &fAlphaOffset, &fAlphaSlope, &fBlueOffset, &fBlueSlope );
//    float Constants[] = 
//    {
//        0.0f, 0.0f, fBlueOffset, fAlphaOffset,      // offset
//        0.0f, 0.0f, fBlueSlope, 0.0f,               // 1x
//        0.0f, 0.0f, 0.0f, 0.0f,                     // 4x
//        0.0f, 0.0f, 0.0f, fAlphaSlope,              // 16x
//    };

	set_pixel_shader( 0 );
//    EngineGlobals.p_Device->SetPixelShaderConstant( 0, Constants, 4 );
    
    // Set render state
	set_render_state( RS_ZWRITEENABLE,		0 );
	set_render_state( RS_ZTESTENABLE,		1 );
    set_render_state( RS_ALPHATESTENABLE,	0 );
	set_render_state( RS_ALPHABLENDENABLE,	0 );
    set_render_state( RS_ALPHACUTOFF,		0 );
//    EngineGlobals.p_Device->SetRenderState( D3DRS_SRCBLEND,         D3DBLEND_ONE );
//    EngineGlobals.p_Device->SetRenderState( D3DRS_DESTBLEND,        D3DBLEND_INVSRCALPHA );
    EngineGlobals.p_Device->SetRenderState( D3DRS_SRCBLEND,         D3DBLEND_ONE );
    EngineGlobals.p_Device->SetRenderState( D3DRS_DESTBLEND,        D3DBLEND_ZERO );

	// Set texture state.
	set_texture( 0, m_pBlur );
	set_texture( 1, NULL );
	set_texture( 2, NULL );
    set_texture( 3, NULL );

	set_render_state( RS_UVADDRESSMODE0, 0x00010001UL );
    
	set_vertex_shader( D3DFVF_XYZRHW | D3DFVF_TEX1 );

	// Render the screen-aligned quadrilateral
	for( int vx = 0; vx < 4; ++vx )
	{
		v[vx].p.z = m_fDepth1;
	}
	EngineGlobals.p_Device->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
    EngineGlobals.p_Device->DrawPrimitiveUP( D3DPT_QUADSTRIP, 1, v, sizeof( VERTEX ));

	// Render the screen-aligned quadrilateral
	for( int vx = 0; vx < 4; ++vx )
	{
		v[vx].p.z = m_fDepth0;
	}
	EngineGlobals.p_Device->SetRenderState( D3DRS_ZFUNC, D3DCMP_GREATEREQUAL );
    EngineGlobals.p_Device->DrawPrimitiveUP( D3DPT_QUADSTRIP, 1, v, sizeof( VERTEX ));

    // Reset render states
	EngineGlobals.p_Device->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

    set_render_state( RS_ALPHATESTENABLE,	1 );
	set_render_state( RS_ALPHABLENDENABLE,	1 );
	set_render_state( RS_ZWRITEENABLE,		1 );
	set_render_state( RS_ZTESTENABLE,		1 );

	set_pixel_shader( 0 );

	set_texture( 0, NULL );
	set_texture( 1, NULL );
	set_texture( 2, NULL );
	set_texture( 3, NULL );

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: DrawFocusRange()
// Desc: Choose the focus range by mapping z to a focus value using pixel
//       shader arithmetic.  See media/shaders/focus.psh for more details.
//
//       High focus values leave the back-buffer unchanged.
//       Low focus values blend in the blurred texture computed by Blur().
//-----------------------------------------------------------------------------
static HRESULT draw_focus_effect_using_range( void )
{
    // Make a D3DTexture wrapper around the depth buffer surface
    D3DTexture ZBufferTexture;
    XGSetTextureHeader( EngineGlobals.backbuffer_width, EngineGlobals.backbuffer_height, 1, 0, 
                        D3DFMT_LIN_A8B8G8R8, 0, &ZBufferTexture,
                        EngineGlobals.p_ZStencilSurface->Data, EngineGlobals.backbuffer_width * 4 );

    // Get size of blur texture for setting texture coords of final blur
    D3DSURFACE_DESC descBlur;
    m_pBlur->GetLevelDesc( 0, &descBlur );
    float fOffsetX = 0.0f;
    float fOffsetY = 0.5f / (float)descBlur.Height; // vertical blur
    struct VERTEX 
    {
        D3DXVECTOR4 p;
        FLOAT tu0, tv0;
        FLOAT tu1, tv1;
    } v[4];
    v[0].p = D3DXVECTOR4( -0.5f,                      -0.5f,                       1.0f, 1.0f );
    v[1].p = D3DXVECTOR4( EngineGlobals.backbuffer_width - 0.5f, -0.5f,                       1.0f, 1.0f );
    v[2].p = D3DXVECTOR4( -0.5f,                      EngineGlobals.backbuffer_height - 0.5f, 1.0f, 1.0f );
    v[3].p = D3DXVECTOR4( EngineGlobals.backbuffer_width - 0.5f, EngineGlobals.backbuffer_height - 0.5f, 1.0f, 1.0f );
    v[0].tu0 = 0.0f;                       v[0].tv0 = 0.0f;
    v[1].tu0 = (float)EngineGlobals.backbuffer_width; v[1].tv0 = 0.0f;
    v[2].tu0 = 0.0f;                       v[2].tv0 = (float)EngineGlobals.backbuffer_height;
    v[3].tu0 = (float)EngineGlobals.backbuffer_width; v[3].tv0 = (float)EngineGlobals.backbuffer_height;
    v[0].tu1 = fOffsetX;                         v[0].tv1 = fOffsetY;
    v[1].tu1 = fOffsetX + (float)descBlur.Width; v[1].tv1 = fOffsetY;
    v[2].tu1 = fOffsetX;                         v[2].tv1 = fOffsetY + (float)descBlur.Height;
    v[3].tu1 = fOffsetX + (float)descBlur.Width; v[3].tv1 = fOffsetY + (float)descBlur.Height;
    
    // Set pixel shader state
    float fAlphaOffset, fAlphaSlope, fBlueOffset, fBlueSlope;
    calculate_depth_mapping( m_fDepth0, m_fDepth1, &fAlphaOffset, &fAlphaSlope, &fBlueOffset, &fBlueSlope );
    float Constants[] = 
    {
        0.0f, 0.0f, fBlueOffset, fAlphaOffset,      // offset
        0.0f, 0.0f, fBlueSlope, 0.0f,               // 1x
        0.0f, 0.0f, 0.0f, 0.0f,                     // 4x
        0.0f, 0.0f, 0.0f, fAlphaSlope,              // 16x
    };

	set_pixel_shader( PixelShaderFocusIntegrate );
    EngineGlobals.p_Device->SetPixelShaderConstant( 0, Constants, 4 );
    
    // Set render state
	set_render_state( RS_ZWRITEENABLE,		0 );
	set_render_state( RS_ZTESTENABLE,		0 );
    set_render_state( RS_ALPHATESTENABLE,	1 );
	set_render_state( RS_ALPHABLENDENABLE,	1 );
    set_render_state( RS_ALPHACUTOFF,		1 );
    EngineGlobals.p_Device->SetRenderState( D3DRS_SRCBLEND,         D3DBLEND_ONE );
    EngineGlobals.p_Device->SetRenderState( D3DRS_DESTBLEND,        D3DBLEND_INVSRCALPHA );

	// Set texture state.
    set_texture( 0, &ZBufferTexture );
	set_texture( 1, m_pBlur );
	set_texture( 2, NULL );
    set_texture( 3, NULL );

	set_render_state( RS_UVADDRESSMODE0, 0x00010001UL );
	set_render_state( RS_UVADDRESSMODE1, 0x00010001UL );
	set_render_state( RS_UVADDRESSMODE2, 0x00010001UL );
	set_render_state( RS_UVADDRESSMODE3, 0x00010001UL );
    
    // Render the screen-aligned quadrilateral
	set_vertex_shader( D3DFVF_XYZRHW | D3DFVF_TEX4 );
    EngineGlobals.p_Device->DrawPrimitiveUP( D3DPT_QUADSTRIP, 1, v, sizeof( VERTEX ));

    // Reset render states
    set_render_state( RS_ALPHATESTENABLE,	1 );
	set_render_state( RS_ALPHABLENDENABLE,	1 );
	set_render_state( RS_ZWRITEENABLE,		1 );
	set_render_state( RS_ZTESTENABLE,		1 );

	set_pixel_shader( 0 );

	set_texture( 0, NULL );
	set_texture( 1, NULL );
	set_texture( 2, NULL );
	set_texture( 3, NULL );

    return S_OK;
}
	

	




//-----------------------------------------------------------------------------
// Name: DrawFocusLookup()
// Desc: Choose the focus range by mapping z through a lookup texture.
//
//       See media/shaders/focuslookup.psh for more detail.
//
//       This technique has lower performance than using DrawFocus(),
//       but the focus values can be arbitrary, rather than the
//       limited types of z-to-focus value mappings available with
//       pixel shader arithmetic.
//
//       High focus values leave the back-buffer unchanged.
//       Low focus values blend in the blurred texture computed by Blur().
//-----------------------------------------------------------------------------
static HRESULT draw_focus_effect_using_lookup( void )
{
    // Make a D3DTexture wrapper around the depth buffer surface
    D3DTexture ZBufferTexture;
    XGSetTextureHeader( EngineGlobals.backbuffer_width, EngineGlobals.backbuffer_height, 1, 0, 
                        D3DFMT_LIN_A8B8G8R8, 0, &ZBufferTexture,
                        EngineGlobals.p_ZStencilSurface->Data, EngineGlobals.backbuffer_width * 4 );

    // Get size of blur texture for setting texture coords of final blur
    D3DSURFACE_DESC descBlur;
    m_pBlur->GetLevelDesc( 0, &descBlur );
    FLOAT fOffsetX = 0.0f;
    FLOAT fOffsetY = 0.5f / (FLOAT)descBlur.Height; // vertical blur

    // Define a set of vertices to draw a quad in screenspace
    struct VERTEX 
    {
        D3DXVECTOR4 p;
        FLOAT tu0, tv0;
        FLOAT tu1, tv1;
        FLOAT tu2, tv2;
        FLOAT tu3, tv3;
    } v[4];
    v[0].p = D3DXVECTOR4( -0.5f,                      -0.5f,                       1.0f, 1.0f );
    v[1].p = D3DXVECTOR4( EngineGlobals.backbuffer_width - 0.5f, -0.5f,                       1.0f, 1.0f );
    v[2].p = D3DXVECTOR4( -0.5f,                      EngineGlobals.backbuffer_height - 0.5f, 1.0f, 1.0f );
    v[3].p = D3DXVECTOR4( EngineGlobals.backbuffer_width - 0.5f, EngineGlobals.backbuffer_height - 0.5f, 1.0f, 1.0f );
    v[0].tu0 = 0.0f;                       v[0].tv0 = 0.0f;
    v[1].tu0 = (float)EngineGlobals.backbuffer_width; v[1].tv0 = 0.0f;
    v[2].tu0 = 0.0f;                       v[2].tv0 = (float)EngineGlobals.backbuffer_height;
    v[3].tu0 = (float)EngineGlobals.backbuffer_width; v[3].tv0 = (float)EngineGlobals.backbuffer_height;

    // tu1 and tv1 are ignored
    // offset final set of texture coords to apply an additional blur
    v[0].tu2 = -fOffsetX;                         v[0].tv2 = -fOffsetY;
    v[1].tu2 = -fOffsetX + (FLOAT)descBlur.Width; v[1].tv2 = -fOffsetY;
    v[2].tu2 = -fOffsetX;                         v[2].tv2 = -fOffsetY + (FLOAT)descBlur.Height;
    v[3].tu2 = -fOffsetX + (FLOAT)descBlur.Width; v[3].tv2 = -fOffsetY + (FLOAT)descBlur.Height;
    v[0].tu3 =  fOffsetX;                         v[0].tv3 =  fOffsetY;
    v[1].tu3 =  fOffsetX + (FLOAT)descBlur.Width; v[1].tv3 =  fOffsetY;
    v[2].tu3 =  fOffsetX;                         v[2].tv3 =  fOffsetY + (FLOAT)descBlur.Height;
    v[3].tu3 =  fOffsetX + (FLOAT)descBlur.Width; v[3].tv3 =  fOffsetY + (FLOAT)descBlur.Height;

    // Set pixel shader
	set_pixel_shader( PixelShaderFocusLookupIntegrate );

    // Set texture state
	set_texture( 0, &ZBufferTexture );
	set_texture( 1, m_pTextureFocusRange );
	set_texture( 2, m_pBlur );
	set_texture( 3, m_pBlur );

	set_render_state( RS_UVADDRESSMODE0, 0x00010001UL );
	set_render_state( RS_UVADDRESSMODE1, 0x00010001UL );
	set_render_state( RS_UVADDRESSMODE2, 0x00010001UL );
	set_render_state( RS_UVADDRESSMODE3, 0x00010001UL );
    
    // Set render state
	set_render_state( RS_ZWRITEENABLE,		0 );
	set_render_state( RS_ZTESTENABLE,		0 );
    set_render_state( RS_ALPHATESTENABLE,	1 );
	set_render_state( RS_ALPHABLENDENABLE,	1 );
    set_render_state( RS_ALPHACUTOFF,		1 );
    EngineGlobals.p_Device->SetRenderState( D3DRS_SRCBLEND,         D3DBLEND_ONE );
    EngineGlobals.p_Device->SetRenderState( D3DRS_DESTBLEND,        D3DBLEND_INVSRCALPHA );

    // Render the screen-aligned quadrilateral
	set_vertex_shader( D3DFVF_XYZRHW | D3DFVF_TEX4 );
	EngineGlobals.p_Device->DrawPrimitiveUP( D3DPT_QUADSTRIP, 1, v, sizeof( VERTEX ));

    // Reset render states
    set_render_state( RS_ALPHATESTENABLE,	1 );
	set_render_state( RS_ALPHABLENDENABLE,	1 );
	set_render_state( RS_ZWRITEENABLE,		1 );
	set_render_state( RS_ZTESTENABLE,		1 );

	set_pixel_shader( 0 );

	set_texture( 0, NULL );
	set_texture( 1, NULL );
	set_texture( 2, NULL );
	set_texture( 3, NULL );

    return S_OK;
}





























/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void set_focus_blur_focus( Mth::Vector & focal_point, float offset, float near_depth, float far_depth )
{
	Mth::Vector diff		= focal_point - Mth::Vector( EngineGlobals.cam_position[0], EngineGlobals.cam_position[1], EngineGlobals.cam_position[2] );
	Mth::Vector unit_diff	= diff.Normalize();

	Mth::Vector p0			= focal_point + (( offset - near_depth ) * unit_diff );
	Mth::Vector p1			= focal_point + (( offset + far_depth ) * unit_diff );

	D3DXVECTOR4 v0( p0[X], p0[Y], p0[Z], 1.0f );
	D3DXVECTOR4 v1( p1[X], p1[Y], p1[Z], 1.0f );

    D3DXVec4Transform( &v0, &v0, (D3DXMATRIX*)&EngineGlobals.view_matrix );
    D3DXVec4Transform( &v0, &v0, (D3DXMATRIX*)&EngineGlobals.projection_matrix );

    D3DXVec4Transform( &v1, &v1, (D3DXMATRIX*)&EngineGlobals.view_matrix );
    D3DXVec4Transform( &v1, &v1, (D3DXMATRIX*)&EngineGlobals.projection_matrix );
    
	m_fDepth0				= v0.z / v0.w;
	m_fDepth1				= v1.z / v1.w;

	// If a z value ends up > 1.0, it is likely from intersecting the near plane, in which case just set it to 0.
	if( m_fDepth0 > 1.0f )
		m_fDepth0 = 0.0f;

//	printf( "%.4f %.4f\n", m_fDepth0, m_fDepth1 );

	// If the two values are sufficiently close, it will cause problems since they have
	// to get quantized down. We have to ensure that ( 1 / ( d1 - d0 )) < 16, that is ( d1 - d0 ) > ( 1 / 16 ).
//	m_fDepth0 = m_fDepth1  - 0.0625f;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void start_focus_blur( void )
{
	D3DDevice_SetRenderTarget( NxXbox::EngineGlobals.p_RenderSurface, NxXbox::EngineGlobals.p_ZStencilSurface );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void finish_focus_blur( void )
{
	// If the focus blur is active, we want to render into a the blur buffer, otherwise into the regular frame buffer.
	if(( EngineGlobals.focus_blur > 0 ) && ( EngineGlobals.focus_blur_duration > 1 ))
	{
		EngineGlobals.p_Device->BlockUntilIdle();

		// Store and reset the min filter for each stage.
		DWORD min_filter[4];
		for( int s = 0; s < 4; ++s )
		{
			D3DDevice_GetTextureStageState( s, D3DTSS_MINFILTER, &min_filter[s] );
			D3DDevice_SetTextureStageState( s, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
		}
		
		// First step is to render the back buffer into the blur texture.
//		fill_focus_range_texture( false );

		focus_blur();

//		draw_focus_effect_using_range();
//		draw_focus_effect_using_lookup();
		draw_focus_effect_using_planes();

		// Restore the min filter for each stage.
		for( int s = 0; s < 4; ++s )
		{
			D3DDevice_SetTextureStageState( s, D3DTSS_MINFILTER, min_filter[s] );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void start_screen_blur( void )
{
	// If the screen blur is active, we want to render into a the blur buffer, otherwise into the regular frame buffer.
	if(( EngineGlobals.screen_blur > 0 ) && ( EngineGlobals.screen_blur_duration > 1 ))
	{
		D3DDevice_SetRenderTarget( NxXbox::EngineGlobals.p_BlurSurface[0], NxXbox::EngineGlobals.p_ZStencilSurface );
	}
	else
	{
		D3DDevice_SetRenderTarget( NxXbox::EngineGlobals.p_RenderSurface, NxXbox::EngineGlobals.p_ZStencilSurface );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void finish_screen_blur( void )
{
	// If the screen blur is active, we want to render into a the blur buffer, otherwise into the regular frame buffer.
	if(( EngineGlobals.screen_blur > 0 ) && ( EngineGlobals.screen_blur_duration > 1 ))
	{
		EngineGlobals.p_Device->BlockUntilIdle();

		// Now that everything has been drawn, set the backbuffer as the rendertarget, and draw the poly on top of it.
		D3DDevice_SetRenderTarget( NxXbox::EngineGlobals.p_RenderSurface, NxXbox::EngineGlobals.p_ZStencilSurface );

		NxXbox::set_blend_mode( NxXbox::vBLEND_MODE_BLEND );
	
		// Turn on clamping so that the linear textures work
		NxXbox::set_render_state( RS_UVADDRESSMODE0, 0x00010001UL );

		// Use a default vertex and pixel shader
		NxXbox::set_vertex_shader( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 );
		NxXbox::set_pixel_shader( PixelShader4 );

		// Select the texture (flush first, since the blur texture is linear).
		NxXbox::set_texture( 0, NULL );
		NxXbox::set_texture( 0, (IDirect3DTexture8*)NxXbox::EngineGlobals.p_BlurSurface[0] );

		// Setup up the vertices.
		struct sBlurVert
		{
			float	sx,sy,sz;
			float	rhw;
			uint32	color;
			float	tu,tv;
		};
	
		sBlurVert vertices[4];

		uint32 alpha		= ( 0xFF - EngineGlobals.screen_blur ) / 2;
		alpha				= ( alpha < 0x20 ) ? 0x20 : alpha;
		
		vertices[0].sx		= 0;
		vertices[0].sy		= 0;
		vertices[0].sz		= 0.0f;
		vertices[0].rhw		= 1.0f;
		vertices[0].color	= ( alpha << 24 ) | 0x808080;
		vertices[0].tu		= 0.0f;
		vertices[0].tv		= 0.0f;

		vertices[1]			= vertices[0];
		vertices[1].sx		= 640;
		vertices[1].tu		= 640.0f;

		vertices[2]			= vertices[0];
		vertices[2].sy		= 480;
		vertices[2].tv		= 480.0f;

		vertices[3]			= vertices[1];
		vertices[3].sy		= vertices[2].sy;
		vertices[3].tv		= vertices[2].tv;

		// Adjust if we are in letterbox mode.
		if( NxXbox::EngineGlobals.letterbox_active )
		{
			vertices[0].sy	+= NxXbox::EngineGlobals.backbuffer_height / 8;
			vertices[1].sy	+= NxXbox::EngineGlobals.backbuffer_height / 8;
			vertices[0].tv	+= (float)( NxXbox::EngineGlobals.backbuffer_height / 8 );
			vertices[1].tv	+= (float)( NxXbox::EngineGlobals.backbuffer_height / 8 );

			vertices[2].sy	-= NxXbox::EngineGlobals.backbuffer_height / 8;
			vertices[3].sy	-= NxXbox::EngineGlobals.backbuffer_height / 8;
			vertices[2].tv	-= (float)( NxXbox::EngineGlobals.backbuffer_height / 8 );
			vertices[3].tv	-= (float)( NxXbox::EngineGlobals.backbuffer_height / 8 );
		}

		// Draw the vertices.
		set_render_state( RS_CULLMODE,		D3DCULL_NONE );
		set_render_state( RS_ZWRITEENABLE,	0 );
		set_render_state( RS_ZTESTENABLE,	0 );

		D3DDevice_DrawVerticesUP( D3DPT_TRIANGLESTRIP, 4, vertices, sizeof( sBlurVert ));

		// Reflush linear texture.
		NxXbox::set_texture( 0, NULL );
	}
}


} // namespace NxXbox
