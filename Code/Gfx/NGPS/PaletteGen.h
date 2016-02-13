#ifndef __NX_NGPS_PALETTEGEN_H__
#define __NX_NGPS_PALETTEGEN_H__

#include <gfx/image/imagebasic.h>

/* caching weights at every node improves performance at expense of memory */
#define CACHEWEIGHTSx

namespace Nx
{

/****************************************************************************
 Global Types
 */

typedef struct
{
	float	red;
	float	green;
	float	blue;
	float	alpha;
} ColorReal;

typedef struct
{
	Image::RGBA col0;	/* min value, inclusive */
	Image::RGBA col1;	/* max value, exclusive */
} box;



#ifdef CACHEWEIGHTS
typedef struct OctNode OctNode;
#else
typedef union OctNode OctNode;
#endif

typedef struct LeafNode LeafNode;
struct LeafNode
{
	float weight;
	ColorReal ac;
	float m2;
	unsigned char palIndex;
};

typedef struct BranchNode BranchNode;
struct BranchNode
{
	OctNode *dir[16];
};

#ifdef CACHEWEIGHTS
struct OctNode
#else
union OctNode
#endif
{
	LeafNode   Leaf;
	BranchNode Branch;
};

/* working data */
typedef struct
{
	box *Mcube;
	float *Mvv;
	OctNode *root;	
}  PalQuant;

/****************************************************************************
 Function prototypes
 */

bool PalQuantInit(PalQuant *pq);
void PalQuantTerm(PalQuant *pq);

void PalQuantAddImage(PalQuant *pq, uint8 *img, int width, int height, int image_bpp, float weight);
int PalQuantResolvePalette(unsigned char *palette, int maxcols, PalQuant *pq, int pbpp);
void GeneratePalette( Image::RGBA *p_palette, uint8* src_image, int width, int height, int image_bpp, int max_colors );


} // Namespace Nx  			

#endif	// __NX_NGPS_PALETTEGEN_H__
