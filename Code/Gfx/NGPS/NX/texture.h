#ifndef __TEXTURE_H
#define __TEXTURE_H


namespace NxPs2
{


#define TEXTURE_BUFFER_SIZE 4194304

////////////////////////
// Flags
#define TEXFLAG_ORIGINAL			(1<<0)
#define TEXFLAG_ANIMATED			(1<<1)
#define TEXFLAG_MIP0_SWIZZLE_16BIT	(1<<18)			// The swizzle bit flags go all the way to the end of the Flag variable
#define TEXFLAG_MIP1_SWIZZLE_16BIT	(1<<19)
#define TEXFLAG_MIP2_SWIZZLE_16BIT	(1<<20)
#define TEXFLAG_MIP3_SWIZZLE_16BIT	(1<<21)
#define TEXFLAG_MIP4_SWIZZLE_16BIT	(1<<22)
#define TEXFLAG_MIP5_SWIZZLE_16BIT	(1<<23)
#define TEXFLAG_MIP6_SWIZZLE_16BIT	(1<<24)
#define TEXFLAG_MIP0_SWIZZLE_32BIT	(1<<25)
#define TEXFLAG_MIP1_SWIZZLE_32BIT	(1<<26)
#define TEXFLAG_MIP2_SWIZZLE_32BIT	(1<<27)
#define TEXFLAG_MIP3_SWIZZLE_32BIT	(1<<28)
#define TEXFLAG_MIP4_SWIZZLE_32BIT	(1<<29)
#define TEXFLAG_MIP5_SWIZZLE_32BIT	(1<<30)
#define TEXFLAG_MIP6_SWIZZLE_32BIT	(1<<31)

#define TEXFLAG_MIP0_SWIZZLE		(TEXFLAG_MIP0_SWIZZLE_16BIT | TEXFLAG_MIP0_SWIZZLE_32BIT)

#define TEXFLAG_MIP_SWIZZLE_16BIT(m)	(TEXFLAG_MIP0_SWIZZLE_16BIT << m)
#define TEXFLAG_MIP_SWIZZLE_32BIT(m)	(TEXFLAG_MIP0_SWIZZLE_32BIT << m)
#define TEXFLAG_MIP_SWIZZLE(m)			(TEXFLAG_MIP0_SWIZZLE << m)

struct sGroup;

struct sTexture
{
	uint16			GetWidth() const;
	uint16			GetHeight() const;
	uint8			GetBitdepth() const;
	uint8			GetClutBitdepth() const;
	uint8			GetNumMipmaps() const;

	uint8 *			GetTextureBuffer() const { return mp_texture_buffer; }
	uint32			GetTextureBufferSize() const { return m_texture_buffer_size; }
	uint8 *			GetClutBuffer() const { return mp_clut_buffer; }
	uint32			GetClutBufferSize() const { return m_clut_buffer_size; }

	bool			HasSwizzleMip() const;
	void			ReplaceTextureData(uint8 *p_texture_data);

	uint32 Checksum, GroupChecksum, MXL;
	uint64 RegTEX0, RegMIPTBP1, RegMIPTBP2;

	uint8  *mp_texture_buffer;
	uint32 m_texture_buffer_size;
	uint8  *mp_clut_buffer;
	uint32 m_clut_buffer_size;

	//bool   Original;
	uint32 Flags;

	static uint32 sVersion;
	
	
	uint8		*	mp_dma;	 			// pointer to DMA list if we want to patch up this texture
	uint32		m_render_count;			// count of how many times this has been rendered this frame
	uint32 		m_quad_words;			// number of quad words to upload in first pass
};



struct sScene *LoadTextures(uint32* pData, int dataSize, bool IsSkin=false, bool IsInstanceable=false, bool UsesPip=false, uint32 texDictOffset=0);
struct sScene *LoadTextures(const char *Filename, bool IsSkin=false, bool IsInstanceable=false, bool UsesPip=false, uint32 texDictOffset=0, uint32 *p_size = NULL);
void LoadTextureGroup(void *pFile, sScene *pScene, sGroup *pGroup, bool IsSkin, bool IsInstanceable, bool UsesPip, uint32 texDictOffset);
void DeleteTextures(sScene *pScene);
sTexture *ResolveTextureChecksum(uint32 TextureChecksum);
sTexture *ResolveTextureChecksum(uint32 TextureChecksum, uint32 GroupChecksum);


} // namespace NxPs2

#endif // __TEXTURE_H

