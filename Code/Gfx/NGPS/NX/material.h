#ifndef __MATERIAL_H
#define __MATERIAL_H


namespace NxPs2
{

// Material Flags
#define MATFLAG_UV_WIBBLE   	(1<<0)
#define MATFLAG_VC_WIBBLE   	(1<<1)
#define MATFLAG_TEXTURED    	(1<<2)
#define MATFLAG_ENVIRONMENT 	(1<<3)
#define MATFLAG_DECAL       	(1<<4)
#define MATFLAG_SMOOTH      	(1<<5)
#define MATFLAG_TRANSPARENT		(1<<6)
#define MATFLAG_ONE_SIDED 		(1<<7)
#define MATFLAG_INVISIBLE 		(1<<8)
#define MATFLAG_TWO_SIDED 		(1<<9)
#define MATFLAG_SPECULAR 		(1<<10)
#define MATFLAG_ANIMATED_TEX	(1<<11)
#define MATFLAG_FORCE_ALPHA		(1<<13)


struct sMaterial
{
	uint32	Flags;
	uint32	Checksum;
	struct	sTexture *pTex;
	uint64	RegALPHA, RegTEX1, RegCLAMP;
	uint8	Aref;
	float*	pUVWibbleInfo;
	uint32*	pVCWibbleInfo;
	sint16	RefMapScaleU, RefMapScaleV;

	static uint32 Version;
};


void * LoadMaterials(void *pFile, struct sScene *pScene, uint32 texDictOffset);


} // namespace NxPs2

#endif // __MATERIAL_H

