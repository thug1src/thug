//#ifndef _MODEL_H_
//#define _MODEL_H_
//
//#include "p_hw.h"
//#include "p_matman.h"
//#include "p_dlman.h"
//#include "p_texman.h"
//#include "p_camera.h"
//#include "p_frame.h"
//#include "p_anim.h"
//#include "p_collision.h"
//
////typedef enum
////{
////    rpWORLDTRISTRIP = 0x01,     /**<This world's meshes can be rendered
////                                   as tri strips */
////    rpWORLDTEXTURED = 0x04,     /**<This world has one set of texture coordinates */
////    rpWORLDPRELIT = 0x08,       /**<This world has luminance values */
////    rpWORLDNORMALS = 0x10,      /**<This world has normals */
////    rpWORLDLIGHT = 0x20,        /**<This world will be lit */
////    rpWORLDMODULATEMATERIALCOLOR = 0x40, /**<Modulate material color with
////                                            vertex colors (pre-lit + lit) */
////    rpWORLDTEXTURED2 = 0x80, /**<This world has 2 set of texture coordinates */
////
////    rpWORLDUVS = ( rpWORLDTEXTURED2 | rpWORLDTEXTURED ),
////    /*
////     * These flags are stored in the flags field in an RwObject, the flag field is 8bit.
////     */
////
////    rpWORLDSECTORSOVERLAP = 0x40000000,
////    //rpWORLDFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
////} RWFlags;
//
//typedef struct {
//	unsigned int	id;
//	unsigned short	numSkinBoxes;
//	unsigned short	numCAS16;
//	unsigned short	numFlipPairs;
//	unsigned short	numCAS32;
//	unsigned int	numPartChecksums;
//	unsigned int	removeCAS32;
//	unsigned short	removeCAS16;
//	unsigned short	pad;
//	unsigned int	numLeaf;
//	unsigned int	numTri;
//} NsSkinData;
//
//class NsModel
//{
//	typedef struct {
//		unsigned int	numPolygons;
//		unsigned int	numVertices;
//		unsigned int	numMorphTargets;
//		unsigned int	flags;
//		float			infx;
//		float			infy;
//		float			infz;
//		float			supx;
//		float			supy;
//		float			supz;
//		unsigned int	pad0;
//		unsigned int	pad1;
//		unsigned int	pad2;
//		unsigned int	wibbleDataFollows;
//		unsigned int	checksum;
//	 	unsigned short	rflags;
//		unsigned short	frameNumber;
//	} NsGeometry;
//	typedef struct {
//		char			name[52];
//		unsigned char	uvwibble;
//		unsigned char	vcwibble;
//		unsigned short	priority;
//		unsigned char	type;
//		unsigned char	flags;
//		unsigned char	alpha;
//		unsigned char	blendmode;
//		GXColor			color;
//	} NsMaterialInfo;
//
//
//public:
//	unsigned int	m_numMaterials;
//	unsigned int	m_numGeom;
//	unsigned int	m_numVertex;
//	NsVector	  * m_pVertexPool;		// Could be interleaved with normals (use getStride to step).
//	unsigned short	m_flags;
//	unsigned short	m_frameNumber;
//	unsigned int	m_checksum;
//	NsSkinData	  * m_pSkinData;
//	unsigned int  * m_pExtraData;
//
//	void		  * draw			( NsMaterialMan * pMaterialList, NsCollision * pCollision, int stripNormals, int addToHashTable );
//	void			buildMaterials	( NsMaterialMan * pMaterialList, NsTextureMan * pTexMan );
//
//	NsVector	  * getVertexPool	( void ) { return m_pVertexPool; }
//	NsVector	  * getNormalPool	( void ) { return m_flags & (1<<1) ? &m_pVertexPool[1] : NULL; }
//	unsigned int  * getColorPool	( void ) { return m_flags & (1<<2) ? (unsigned int *)&m_pVertexPool[getStride() * m_numVertex + 1] : NULL; }
//	float		  * getUVPool		( void ) { return m_flags & (1<<3) ? getColorPool() ? (float *)&getColorPool()[m_numVertex] : (float *)&m_pVertexPool[getStride() * m_numVertex + 1] : NULL; }
//	unsigned short* getConnectList	( void ) { return m_flags & (1<<3) ? getUVPool() ? (unsigned short *)&getUVPool()[m_numVertex * 2] : getColorPool() ? (unsigned short *)&getColorPool()[m_numVertex] : (unsigned short *)&m_pVertexPool[getStride() * m_numVertex + 1] : NULL; }
//	int				getStride		( void ) { return ( m_flags & (1<<1) ? 2 : 1 ); }
//};
//
//#endif		// _MODEL_H_

