//#ifndef _COLLISION_H_
//#define _COLLISION_H_
//
//#include "p_hw.h"
//#include "p_triangle.h"
//#include "p_matrix.h"
//#include "p_dl.h"
//
//typedef struct {
//	unsigned short	type;
//	unsigned char	leftType;
//	unsigned char	rightType;
//	unsigned short	leftNode;
//	unsigned short	rightNode;
//	float			leftValue;
//	float			rightValue;
//} NsBranch;
//
//typedef struct {
//	unsigned short	numPolygons;
//	unsigned short	firstPolygon;
//} NsLeaf;
//
//class NsTree
//{
//	int		testPolygons	( unsigned int numPolygons, unsigned int polyOffset, NsLine * pLine, NsDL::Collision_LineCallback pCb, void * pData );
//public:
//	unsigned int		m_numBranchNodes;
//	unsigned int		m_numLeafNodes;
//	unsigned int		m_numTriangles;
//	NsBranch		  * m_pBranchNodes;
//	NsLeaf			  * m_pLeafNodes;
//	unsigned int	  * m_pTriangleMap;
//
//	NsDL			  * m_pDL;
//
//	NsTree			  * m_pNext;
//
//			NsTree			( void * pTree, unsigned int numLeaf, unsigned int numTri, NsDL * pDL ); 
//			~NsTree			();
//
//	int		findCollision	( NsLine * pLine, NsDL::Collision_LineCallback pCb, void * pData );
//};
//
//class NsCollision
//{
//	NsTree	  * m_pHead;
//public:
//
//			NsCollision		();
//			~NsCollision	();
//
//	void	addTree			( void * pTree, unsigned int numLeaf, unsigned int numTri, NsDL * pDL );
//
//	int		findCollision	( NsLine * pLine, NsDL::Collision_LineCallback pCb, void * pData );
//};
//
//#endif		// _COLLISION_H_








