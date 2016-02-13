//#ifndef _DL_H_
//#define _DL_H_
//
//#include "p_hw.h"
//#include "p_frame.h"
//#include "p_matrix.h"
//#include "p_bbox.h"
//#include "p_triangle.h"
//
//typedef struct NsCull_Item NsCull_Item;
//struct NsCull_Item {
//	NsBBox			box;				// 24
//	unsigned int	visible;			// 4
//	unsigned short	clipCodeAND;		// 2
//	unsigned short	clipCodeOR;			// 2
//};										// Total: 32*/
//
//typedef struct {
//    NsVector	normal; /**< Triangle normal */
//    NsVector	point;  /**< First triangle vertex */
//    int			index;  /**< Index of triangle in object (if applicable) */
//    NsVector  * vertices[3]; /**< Pointers to three triangle vertices */
//} NsCollisionTriangle;
//
//class NsDL
//{
//public:
//	typedef NsCollisionTriangle *(*Collision_LineCallback)( NsLine * line, NsDL& pDL, NsCollisionTriangle * tri, float distance, void * pData );
//	typedef NsCollisionTriangle *(*Collision_SphereCallback)( NsSphere * line, NsDL& pDL, NsCollisionTriangle * tri, float distance, void * pData );
//
//	unsigned int	m_id;			// 4
//	unsigned int	m_size;			// 4
//	NsDL		  * m_pNext;		// 4
//	unsigned short	m_flags;		// 2 - Geometry Flags.
//	unsigned short	m_rwflags;		// 2 - Renderware Flags.
//
//	NsVector	  * m_pVertexPool;	// 4
//	unsigned int	m_numVertex;	// 4
//	unsigned short	m_numPoly;		// 2
//	unsigned short	m_numIdx;		// 2 - number of indices in this DL
//	unsigned char * m_pWibbleData;	// 4
//
//	NsDL		  * m_pParent;		// 4 - Next geometry chunk.
//	NsFrame		  * m_pFrame;		// 4 - Instance information
//	unsigned short* m_pFaceFlags;	// 4 - Per polygon flag data.
//	unsigned short* m_pFaceMaterial;// 4 - Per polygon material number.
//
//	unsigned short	m_polyBase;		// 2 - The first poly index in this DL.
//	unsigned short	m_vpoolFlags;	// 2 - describes the vertex pool and following pools
//	unsigned int	m_operationID;	// 4 - Mirrors the NxPlugin data field for world sectors.	
//	void		  * m_pMaterialMan;	// 4 - Pointer back to the material manager this DL uses.
////	NsDL		  * m_pChild;		// 4 - Next geometry chunk.
//	unsigned short	m_vertBase;		// 2 - The first vert index in this DL.
//	unsigned short	m_vertRange;	// 2 - The range of verts in this DL.
//
//	NsCull_Item		m_cull;			// 32
//
//	int				findCollision	( NsLine * pLine, Collision_LineCallback pCb, void * pData );
//	int				findCollision	( NsSphere * pSphere, Collision_SphereCallback pCb, void * pData );
//
//	NsVector	  * getVertexPool	( void ) { return m_pVertexPool; }
//	NsVector	  * getNormalPool	( void ) { return m_vpoolFlags & (1<<1) ? &m_pVertexPool[1] : NULL; }
//	unsigned int  * getColorPool	( void ) { return m_vpoolFlags & (1<<2) ? (unsigned int *)&m_pVertexPool[getStride() * m_numVertex + 1] : NULL; }
//	float		  * getUVPool		( void ) { return m_vpoolFlags & (1<<3) ? getColorPool() ? (float *)&getColorPool()[m_numVertex] : (float *)&m_pVertexPool[getStride() * m_numVertex + 1] : NULL; }
//	unsigned short* getConnectList	( void ) { return m_vpoolFlags & (1<<3) ? getUVPool() ? (unsigned short *)&getUVPool()[m_numVertex * 2] : getColorPool() ? (unsigned short *)&getColorPool()[m_numVertex] : (unsigned short *)&m_pVertexPool[getStride() * m_numVertex + 1] : NULL; }
//	int				getStride		( void ) { return ( m_vpoolFlags & (1<<1) ? 2 : 1 ); }
//
//	NsVector	  * getVertex		( int index ) { return &getVertexPool()[index*getStride()]; }
//
//	unsigned int  * getWibbleColor	( void ) { return (unsigned int *)m_pWibbleData; }
//	unsigned char * getWibbleIdx	( void ) { return &m_pWibbleData[m_numVertex*4]; }
//	unsigned char * getWibbleOff	( void ) { return &m_pWibbleData[m_numVertex*5]; }
//
//	int				cull			( NsMatrix * m );
//};
//
//#endif		// _DL_H_


