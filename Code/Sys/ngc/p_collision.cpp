///********************************************************************************
// *																				*
// *	Module:																		*
// *				NsCollision														*
// *	Description:																*
// *	Written by:																	*
// *				Paul Robinson													*
// *	Copyright:																	*
// *				2001 Neversoft Entertainment - All rights reserved.				*
// *																				*
// ********************************************************************************/
//
///********************************************************************************
// * Includes.																	*
// ********************************************************************************/
//#include "p_hw.h"
//#include "p_collision.h"
//#include "p_assert.h"
//#include "p_render.h"
//#include "p_prim.h"
//
///********************************************************************************
// * Defines.																		*
// ********************************************************************************/
//
///********************************************************************************
// * Structures.																	*
// ********************************************************************************/
//
///********************************************************************************
// * Local variables.																*
// ********************************************************************************/
//
///********************************************************************************
// * Forward references.															*
// ********************************************************************************/
//
///********************************************************************************
// * Externs.																		*
// ********************************************************************************/
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				NsCollision														*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *																				*
// ********************************************************************************/
//NsCollision::NsCollision()
//{
//	m_pHead = NULL;
//}
//
//NsCollision::~NsCollision()
//{
//	// Delete all collision structures.
//	NsTree * pTree = m_pHead;
//	NsTree * pDelete;
//	while ( pTree ) {
//		pDelete = pTree;
//		pTree = pTree->m_pNext;
//		delete pDelete;
//	}
//}
//
//void NsCollision::addTree( void * pTree, unsigned int numLeaf, unsigned int numTri, NsDL * pDL )
//{
//	NsTree * p = new NsTree( pTree, numLeaf, numTri, pDL );
//	p->m_pNext = m_pHead;
//	m_pHead = p;
//}
//
//int NsCollision::findCollision( NsLine * pLine, NsDL::Collision_LineCallback pCb, void * pData )
//{
//	int rv = 0;
//	NsTree * pTree = m_pHead;
//	while ( pTree ) {
//		if ( pTree->m_numLeafNodes && pTree->m_numTriangles ) {
//			if ( pTree->findCollision( pLine, pCb, pData ) ) {
//				rv = 1;
//			}
//		}
//		pTree = pTree->m_pNext;
//	}
//	return rv;
//}
//
//NsTree::NsTree( void * pTree, unsigned int numLeaf, unsigned int numTri, NsDL * pDL )
//{
//	NsBranch		  * pBranchNodes;
//	NsLeaf			  * pLeafNodes;
//	unsigned int	  * pTriangleMap;
//
//	// Point up DL.
//	m_pDL = pDL;
//
//	// Set pointers & counts up.
//	m_numBranchNodes	= numLeaf ? numLeaf - 1 : 0;
//	m_numLeafNodes		= numLeaf;
//	m_numTriangles		= numTri;
//
//	pBranchNodes		= (NsBranch *)pTree;
//	pLeafNodes			= (NsLeaf *)&pBranchNodes[m_numBranchNodes];
//	pTriangleMap		= (unsigned int *)&pLeafNodes[m_numLeafNodes];
//
//	// Copy the branch nodes.
//	if ( m_numBranchNodes ) {
//		m_pBranchNodes = new NsBranch[m_numBranchNodes];
//		memcpy ( m_pBranchNodes, pBranchNodes, sizeof ( NsBranch ) * m_numBranchNodes );
//	} else {
//		m_pBranchNodes = NULL;
//	}
//
//	// Copy the leaf nodes.
//	if ( m_numLeafNodes ) {
//		m_pLeafNodes = new NsLeaf[m_numLeafNodes];
//		memcpy ( m_pLeafNodes, pLeafNodes, sizeof ( NsLeaf ) * m_numLeafNodes );
//	} else {
//		m_pLeafNodes = NULL;
//	}
//
//	// Copy the triangle map.
//	if ( m_numTriangles ) {
//		m_pTriangleMap = new unsigned int[m_numTriangles];
//		memcpy ( m_pTriangleMap, pTriangleMap, sizeof ( unsigned int ) * m_numTriangles );
//	} else {
//		m_pTriangleMap = NULL;
//	}
//
//	// Fix branch data.
//	{
//		unsigned int  * p32 = (unsigned int *)m_pBranchNodes;
//		unsigned int	value;
//		for ( unsigned int lp = 0; lp < m_numBranchNodes; lp++ ) {
//			value = *p32++;
//			m_pBranchNodes[lp].type = (unsigned short)( ( value >> 16 ) & 0x0000ffff );
//			m_pBranchNodes[lp].leftType = (unsigned char)( ( value >> 8 ) & 0x000000ff );
//			m_pBranchNodes[lp].rightType = (unsigned char)( value & 0x000000ff );
//			value = *p32++;
//			m_pBranchNodes[lp].leftNode = (unsigned short)( ( value >> 16 ) & 0x0000ffff );
//			m_pBranchNodes[lp].rightNode = (unsigned short)( value & 0x0000ffff );
//			p32 += 2;
//		}
//	}
//	
//	// Fix leaf data.
//	{
//		unsigned int  * p32 = (unsigned int *)m_pLeafNodes;
//		unsigned int	value;
//		for ( unsigned int lp = 0; lp < m_numLeafNodes; lp++ ) {
//			value = *p32++;
//			m_pLeafNodes[lp].numPolygons = (unsigned short)( ( value >> 16 ) & 0x0000ffff );
//			m_pLeafNodes[lp].firstPolygon = (unsigned short)( value & 0x0000ffff );
//		}
//	}
//}
//
//NsTree::~NsTree()
//{
//	if ( m_pBranchNodes )	delete m_pBranchNodes;
//	if ( m_pLeafNodes )		delete m_pLeafNodes;
//	if ( m_pTriangleMap )	delete m_pTriangleMap;
//}
//
///******************************************************************************
// *  
// *  Line test with polygons in BSP leaf node
// */
//int NsTree::testPolygons( unsigned int numPolygons, unsigned int polyOffset, NsLine * pLine, NsDL::Collision_LineCallback pCb, void * pData )
//{
//	unsigned short	  * polygons = m_pDL->getConnectList();
//	unsigned int	  * polyIndex = m_pTriangleMap;
//	NsVector		  * vertices = m_pDL->getVertexPool();
//
//    while (numPolygons--)
//    {
//		unsigned short	*poly;
//		NsTriangle		ct;
//		float			distance;
//		unsigned int	result;
//
//		// Build the collision triangle.
//        poly = polygons + ( *polyIndex * 3 );
//        *ct.corner( 0 ) = *(vertices + ( poly[0] * m_pDL->getStride() ));
//        *ct.corner( 1 ) = *(vertices + ( poly[1] * m_pDL->getStride() ));
//        *ct.corner( 2 ) = *(vertices + ( poly[2] * m_pDL->getStride() ));
//
//        /* Test for collision */
//		result = pLine->intersectTriangle( &distance, &ct );
//
//        if (result)
//        {
//			NsCollisionTriangle	tri; 
//
//			// Create triangle data.
//			NsVector l10;
//			NsVector l20;
//			l10.sub( *ct.corner(1), *ct.corner(0) );
//			l20.sub( *ct.corner(2), *ct.corner(0) );
//			tri.normal.cross( l10, l20 );
//			tri.normal.normalize();
//			tri.vertices[0] = ct.corner( 0 );
//			tri.vertices[1] = ct.corner( 1 );
//			tri.vertices[2] = ct.corner( 2 );
//			tri.point.set( tri.vertices[0]->x, tri.vertices[0]->y, tri.vertices[0]->z );
//			tri.index		= *polyIndex;
//			// Execute callback.
//			if ( pCb ) {
//				if ( !pCb( pLine, *m_pDL, &tri, distance, pData ) ) {
//					/* Early out */
//					return 0;
//				}
//			}
//        }
//
//        polyIndex++;
//    }
//
//	return 1;
//}
//
//
///******************************************************************************
// *
// *  Line intersections with collision BSP leaf nodes
// *
// *  Early out with NULL if callback returns NULL.
// */
//
//typedef struct nodeInfo nodeInfo;
//struct nodeInfo
//{
//    unsigned int	type;
//    unsigned int	index;
//};
//
//typedef union NsSplitBits NsSplitBits;
//union NsSplitBits
//{
//    float					nReal;
//    volatile int			nInt;
//    volatile unsigned int	nUInt;
//};
//
//#define NsCollision_LeafNode	(1)
//#define NsCollision_BranchNode	(2)
//#define NsCollision_MaxDepth	(32)
//
///* Local macro to aid readability */
//#define PUSH_NODE_INFO(_type, _index, lineStart, lineEnd)       \
//    {                                                           \
//        nStack++;                                               \
//        nodeStack[nStack].type = (_type);                       \
//        nodeStack[nStack].index = (_index);                     \
//        lineStack[nStack].start = (lineStart);                  \
//        lineStack[nStack].end = (lineEnd);                      \
//    }                                                           \
//
//#define GETCOORD(vect,y)										\
//    (*(float *)(((unsigned char *)(&((vect).x)))+(unsigned int)(y)))
//
//int NsTree::findCollision( NsLine * pLine, NsDL::Collision_LineCallback pCb, void * pData )
//{
//	// First, calculate the gradient.
//	NsVector	delta;
//	float		recip;
//	float		dydx;
//	float		dzdx;
//	float		dxdy;
//	float		dzdy;
//	float		dxdz;
//	float		dydz;
//
//	delta.sub( pLine->end, pLine->start );
//
//    recip = (delta.x != 0.0f) ? (1.0f / delta.x) : 0.0f;
//    dydx = delta.y * recip;
//    dzdx = delta.z * recip;
//
//    recip = (delta.y != 0.0f) ? (1.0f / delta.y) : 0.0f;
//    dxdy = delta.x * recip;
//    dzdy = delta.z * recip;
//
//    recip = (delta.z != 0.0f) ? (1.0f / delta.z) : 0.0f;
//    dxdz = delta.x * recip;
//    dydz = delta.y * recip;
//
//    /* Need data stack for recursion */
//    int			nStack;
//    nodeInfo	nodeStack[NsCollision_MaxDepth + 1], node;
//    NsLine		lineStack[NsCollision_MaxDepth + 1], currLine;
//
//    /* Go down tree recursively */
//    node.type	= m_pBranchNodes ? NsCollision_BranchNode : NsCollision_LeafNode;
//    node.index	= 0;
//    currLine	= *pLine;
//    nStack		= 0;
//
//    while (nStack >= 0)
//    {
//        if (node.type == NsCollision_LeafNode)
//        {
//            NsLeaf	  * leaf;
//
//            leaf = m_pLeafNodes + node.index;
//
//			if ( !testPolygons( leaf->numPolygons, leaf->firstPolygon, pLine, pCb, pData ) ) {
//				return 0;
//			}
//
//            /* Unstack */
//            node		= nodeStack[nStack];
//            currLine	= lineStack[nStack];
//            nStack--;
//        }
//        else
//        {
//            NsSplitBits		lStart, lEnd;
//            NsSplitBits		rStart, rEnd;
//            NsBranch	  * branch;
//
//            /* Its a plane, find out which way we need to go */
//            branch = m_pBranchNodes + node.index;
//
//            /* Find out where line end points are in relation to the plane
//             * Note: leftValue > rightValue as these mean value for left and right
//             * sector respectively. 
//             */
//            lStart.nReal = GETCOORD(currLine.start, branch->type)
//                - branch->leftValue;
//            lEnd.nReal = GETCOORD(currLine.end, branch->type)
//                - branch->leftValue;
//            rStart.nReal = GETCOORD(currLine.start, branch->type)
//                - branch->rightValue;
//            rEnd.nReal = GETCOORD(currLine.end, branch->type)
//                - branch->rightValue;
//
//            /* First test if it's entirely one side or the other 
//             * Note that the line can never lie in the plane, because we use 
//             * the sign bits to determine which sides the ends are on, so zero
//             * means on the right.
//             */
//            if (rStart.nInt < 0 && rEnd.nInt < 0)
//            {
//                /* totally left */
//                node.type = branch->leftType;
//                node.index = branch->leftNode;
//            }
//            else if (lStart.nInt >= 0 && lEnd.nInt >= 0)
//            {
//                /* totally right */
//                node.type = branch->rightType;
//                node.index = branch->rightNode;
//            }
//            else if (!((lStart.nInt ^ lEnd.nInt) & 0x80000000) &&
//                     !((rStart.nInt ^ rEnd.nInt) & 0x80000000))
//            {
//                /* Doesn't cross either, sat in overlap regions */
//                if (rStart.nInt < rEnd.nInt)
//                {
//                    /* go left, stack right */
//                    PUSH_NODE_INFO(branch->rightType, branch->rightNode,
//                                   currLine.start, currLine.end);
//                    node.type = branch->leftType;
//                    node.index = branch->leftNode;
//                }
//                else
//                {
//                    /* go right, stack left */
//                    PUSH_NODE_INFO(branch->leftType, branch->leftNode,
//                                   currLine.start, currLine.end);
//                    node.type = branch->rightType;
//                    node.index = branch->rightNode;
//                }
//            }
//            else
//            {
//                if (((lStart.nInt ^ lEnd.nInt) & 0x80000000) &&
//                    (rStart.nInt >= 0 && rEnd.nInt >= 0))
//                {
//                    /* crosses left and totally in right */
//                    NsVector	vTmp;
//
//                    /* Calculate an intersection point for left plane */
////                    _rpLinePlaneIntersectMacro(&vTmp, &currLine, grad,
////                                               branch->type,
////                                               branch->leftValue);
//					float	ld;
//					switch ( branch->type ) {
//						case 0:
//							ld = branch->leftValue - currLine.start.x;
//							vTmp.x = branch->leftValue;
//							vTmp.y = currLine.start.y + dydx * ld;
//							vTmp.z = currLine.start.z + dzdx * ld;
//							break;
//						case 4:
//							ld = branch->leftValue - currLine.start.y;
//							vTmp.x = currLine.start.x + dxdy * ld;
//							vTmp.y = branch->leftValue;
//							vTmp.z = currLine.start.z + dzdy * ld;
//							break;
//						case 8:
//							ld = branch->leftValue - currLine.start.z;
//							vTmp.x = currLine.start.x + dxdz * ld;
//							vTmp.y = currLine.start.y + dydz * ld;
//							vTmp.z = branch->leftValue;
//							break;
//					}
//
//                    if (lStart.nInt < 0)
//                    {
//                        /* stack right go left */
//                        PUSH_NODE_INFO(branch->rightType,
//                                       branch->rightNode,
//                                       currLine.start, currLine.end);
//                        node.type = branch->leftType;
//                        node.index = branch->leftNode;
//                        currLine.end = vTmp;
//                    }
//                    else
//                    {
//                        /* stack left go right */
//                        PUSH_NODE_INFO(branch->leftType,
//                                       branch->leftNode, vTmp,
//                                       currLine.end);
//                        node.type = branch->rightType;
//                        node.index = branch->rightNode;
//                    }
//                }
//                else if (((rStart.nInt ^ rEnd.nInt) & 0x80000000) &&
//                         (lStart.nInt < 0 && lEnd.nInt < 0))
//                {
//                    /* crosses right and totally in left */
//                    NsVector	vTmp;
//
//                    /* Calculate an intersection point for right plane */
////                    _rpLinePlaneIntersectMacro(&vTmp, &currLine, grad,
////                                               branch->type,
////                                               branch->rightValue);
//					float	ld;
//					switch ( branch->type ) {
//						case 0:
//							ld = branch->rightValue - currLine.start.x;
//							vTmp.x = branch->rightValue;
//							vTmp.y = currLine.start.y + dydx * ld;
//							vTmp.z = currLine.start.z + dzdx * ld;
//							break;
//						case 4:
//							ld = branch->rightValue - currLine.start.y;
//							vTmp.x = currLine.start.x + dxdy * ld;
//							vTmp.y = branch->rightValue;
//							vTmp.z = currLine.start.z + dzdy * ld;
//							break;
//						case 8:
//							ld = branch->rightValue - currLine.start.z;
//							vTmp.x = currLine.start.x + dxdz * ld;
//							vTmp.y = currLine.start.y + dydz * ld;
//							vTmp.z = branch->rightValue;
//							break;
//					}
//
//                    if (rStart.nInt < 0)
//                    {
//                        /* stack right go left */
//                        PUSH_NODE_INFO(branch->rightType,
//                                       branch->rightNode, vTmp,
//                                       currLine.end);
//                        node.type = branch->leftType;
//                        node.index = branch->leftNode;
//                    }
//                    else
//                    {
//                        /* stack left go right */
//                        PUSH_NODE_INFO(branch->leftType,
//                                       branch->leftNode, currLine.start,
//                                       currLine.end);
//                        node.type = branch->rightType;
//                        node.index = branch->rightNode;
//                        currLine.end = vTmp;
//                    }
//                }
//                else
//                {
//                    /* Must cross both planes */
//                    NsVector	vLeft, vRight;
//					float	ld;
//
//                    /* Calc intersections at planes */
////                    _rpLinePlaneIntersectMacro(&vLeft, &currLine, grad,
////                                               branch->type,
////                                               branch->leftValue);
//					switch ( branch->type ) {
//						case 0:
//							ld = branch->leftValue - currLine.start.x;
//							vLeft.x = branch->leftValue;
//							vLeft.y = currLine.start.y + dydx * ld;
//							vLeft.z = currLine.start.z + dzdx * ld;
//							break;
//						case 4:
//							ld = branch->leftValue - currLine.start.y;
//							vLeft.x = currLine.start.x + dxdy * ld;
//							vLeft.y = branch->leftValue;
//							vLeft.z = currLine.start.z + dzdy * ld;
//							break;
//						case 8:
//							ld = branch->leftValue - currLine.start.z;
//							vLeft.x = currLine.start.x + dxdz * ld;
//							vLeft.y = currLine.start.y + dydz * ld;
//							vLeft.z = branch->leftValue;
//							break;
//					}
//
////                    _rpLinePlaneIntersectMacro(&vRight, &currLine, grad,
////                                               branch->type,
////                                               branch->rightValue);
//					switch ( branch->type ) {
//						case 0:
//							ld = branch->rightValue - currLine.start.x;
//							vRight.x = branch->rightValue;
//							vRight.y = currLine.start.y + dydx * ld;
//							vRight.z = currLine.start.z + dzdx * ld;
//							break;
//						case 4:
//							ld = branch->rightValue - currLine.start.y;
//							vRight.x = currLine.start.x + dxdy * ld;
//							vRight.y = branch->rightValue;
//							vRight.z = currLine.start.z + dzdy * ld;
//							break;
//						case 8:
//							ld = branch->rightValue - currLine.start.z;
//							vRight.x = currLine.start.x + dxdz * ld;
//							vRight.y = currLine.start.y + dydz * ld;
//							vRight.z = branch->rightValue;
//							break;
//					}
//
//                    if (lStart.nInt < 0)
//                    {
//                        /* Stack right, go left */
//                        PUSH_NODE_INFO(branch->rightType,
//                                       branch->rightNode, vRight,
//                                       currLine.end);
//                        node.type = branch->leftType;
//                        node.index = branch->leftNode;
//                        currLine.end = vLeft;
//                    }
//                    else
//                    {
//                        /* Stack left, go right */
//                        /* Calc intersection for left plane and stack */
//                        PUSH_NODE_INFO(branch->leftType,
//                                       branch->leftNode, vLeft,
//                                       currLine.end);
//                        node.type = branch->rightType;
//                        node.index = branch->rightNode;
//                        currLine.end = vRight;
//                    }
//                }
//            }
//        }
//    }
//
//    /* All done */
//	return 1;
//}
	



