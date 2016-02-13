//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       immediate.h
//* OWNER:          Garrett Jost
//* CREATION DATE:  7/19/2002
//****************************************************************************

#ifndef	__IMMEDIATE_H__
#define	__IMMEDIATE_H__
    
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
namespace NxPs2
{
// Forward declarations
struct SSingleTexture;

class CImmediateMode
{
public:
	// Init functions
	static void sViewportInit();
	static void sTextureGroupInit(uint unpackFLG);

	static void sSetZPush(float zpush = 1.0e-33f);
	static void sClearZPush();

	// Draw start functions
	static void sStartPolyDraw( SSingleTexture * p_engine_texture, uint64 blend, uint unpackFLG, bool clip = false );
	static void sStartPolyDraw( uint32 * p_packed_texture_regs, int num_texture_regs, uint unpackFLG, bool clip = false );

	// Draw functions
	static void sDrawQuadTexture( SSingleTexture * p_engine_texture, const Mth::Vector& vert0, const Mth::Vector& vert1,
								  const Mth::Vector& vert2, const Mth::Vector& vert3,
								  uint32 col0, uint32 col1, uint32 col2, uint32 col3 );
	static void sDrawTri( const Mth::Vector& vert0, const Mth::Vector& vert1, const Mth::Vector& vert2,
						  uint32 col0, uint32 col1, uint32 col2, uint unpackFLG );
	static void sDrawTriUV( const Mth::Vector& vert0, const Mth::Vector& vert1, const Mth::Vector& vert2,
							float u0, float v0, float u1, float v1, float u2, float v2,
							uint32 col0, uint32 col1, uint32 col2, uint unpackFLG );
	static void sDraw5QuadTexture( SSingleTexture * p_engine_texture, const Mth::Vector& vert0, const Mth::Vector& vert1,
								   const Mth::Vector& vert2, const Mth::Vector& vert3, const Mth::Vector& vert4,
								   uint32 col0, uint32 col1 );
	static void sDrawGlowSegment( const Mth::Vector& vert0, const Mth::Vector& vert1,
								  const Mth::Vector& vert2, const Mth::Vector& vert3, const Mth::Vector& vert4,
								  uint32 col0, uint32 col1, uint32 col2 );
	static void sDrawStarSegment( const Mth::Vector& vert0, const Mth::Vector& vert1,
								  const Mth::Vector& vert2, const Mth::Vector& vert3,
								  uint32 col0, uint32 col1, uint32 col2 );
	static void sDrawSmoothStarSegment( const Mth::Vector& vert0, const Mth::Vector& vert1,
										const Mth::Vector& vert2, const Mth::Vector& vert3, const Mth::Vector& vert4,
										uint32 col0, uint32 col1, uint32 col2 );
	static void sDrawLine( const Mth::Vector& vert0, const Mth::Vector& vert1, uint32 col0, uint32 col1 );

	static uint64 sGetTextureBlend( uint32 blend_checksum, int fix );

protected:
};

}

#endif 




