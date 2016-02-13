///********************************************************************************
// *																				*
// *	Module:																		*
// *				NsAtomic															*
// *	Description:																*
// *				Holds Clump data & draws it.									*
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
//#include <math.h>
//#include <string.h>
//#include "p_atomic.h"
//#include "p_frame.h"
//#include <charpipeline/GQRSetup.h>
//#include "p_profile.h"
//#include "p_display.h"
//#include "p_assert.h"
//
///********************************************************************************
// * Defines.																		*
// ********************************************************************************/
//// See 'RpAtomicFlag' in rpworld.h
//#define COLLIDABLE 1
//#define VISIBLE 4
//
///********************************************************************************
// * Structures.																	*
// ********************************************************************************/
//
///********************************************************************************
// * Local variables.																*
// ********************************************************************************/
//NsProfile AnimTime( "Anim Time", 256 );
//int gTripleMem = 0;
//
///********************************************************************************
// * Forward references.															*
// ********************************************************************************/
//
///********************************************************************************
// * Externs.																		*
// ********************************************************************************/
//extern "C" {
//
//extern void TransformSingle	( ROMtx m, float * srcBase, float * dstBase, u32 count );
//extern void TransformDouble	( ROMtx m0, ROMtx m1, float * wtBase, float * srcBase, float * dstBase, u32 count );
//extern void TransformAcc	( ROMtx m, u16 count, float * srcBase, float * dstBase, u16 * indices, float * weights );
//
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				NsAtomic															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Declares a clump object.										*
// *																				*
// ********************************************************************************/
//
//NsAtomic::NsAtomic()
//{
//	m_pMaterial	= NULL;
//	m_pFrame		= NULL;
//	m_pNext		= NULL;
//	m_pDoubleData	= NULL;
//	m_pAccData	= NULL;
//	m_flags = VISIBLE | COLLIDABLE;
//	m_pTransformedVertices = NULL;
//	m_pTransformedVertices2 = NULL;
//	m_pTransformedVertices3 = NULL;
//	m_pGeometryArrays = NULL;
//
//	m_pSkinBox = NULL;
//	m_pCAS16 = NULL;
//	m_pFlipPairs = NULL;
//	m_pCAS32 = NULL;
//	m_pPartChecksums = NULL;
//	m_pBranchNodes = NULL;
//	m_pLeafNodes = NULL;
//	m_pTriangleMap = NULL;
//
//	m_lastTransform = -1;		// Force 1st transform to always happen.
//}
//
//NsAtomic::NsAtomic( NsModel * pModelData, NsTextureMan * pTexMan )
//{
//	m_pMaterial	= NULL;
//	m_pFrame		= NULL;
//	m_pNext		= NULL;
//	m_pDoubleData	= NULL;
//	m_pAccData	= NULL;
//	m_flags = VISIBLE | COLLIDABLE;
//	m_pTransformedVertices = NULL;
//	m_pTransformedVertices2 = NULL;
//	m_pTransformedVertices3 = NULL;
//	m_pGeometryArrays = NULL;
//
//	m_pSkinBox = NULL;
//	m_pCAS16 = NULL;
//	m_pFlipPairs = NULL;
//	m_pCAS32 = NULL;
//	m_pPartChecksums = NULL;
//	m_pBranchNodes = NULL;
//	m_pLeafNodes = NULL;
//	m_pTriangleMap = NULL;
//
//	m_lastTransform = -1;		// Force 1st transform to always happen.
//	
//	setModel( pModelData, pTexMan );
//}
//
//NsAtomic::~NsAtomic()
//{
////	if ( pFrame ) delete pFrame;
//
//	NsDisplay::flush();
//	
//	// Multi-reference stuff.
//	if ( m_pMaterial ) {
//		if ( m_pMaterial->totalReferences() == 0 ) {
//			delete m_pMaterial;
//		} else {
//			m_pMaterial->removeReference();
//		}
//	}
//
//	if ( m_pDoubleData ) {
//		if ( m_pDoubleData->totalReferences() == 0 ) {
//			delete m_pDoubleData;
//		} else {
//			m_pDoubleData->removeReference();
//		}
//	}
//
//	if ( m_pAccData ) {
//		if ( m_pAccData->totalReferences() == 0 ) {
//			delete m_pAccData;
//		} else {
//			m_pAccData->removeReference();
//		}
//	}
//
//	if ( m_pTransformedVertices && ( m_pTransformedVertices != m_pGeometryArrays ) ) {
//		if ( m_pTransformedVertices->totalReferences() == 0 ) {
//			delete m_pTransformedVertices;
//		} else {
//			m_pTransformedVertices->removeReference();
//		}
//	}
//	if ( m_pTransformedVertices2 ) delete m_pTransformedVertices2;
//	if ( m_pTransformedVertices3 ) delete m_pTransformedVertices3;
//
//	if ( m_pGeometryArrays ) {
//		if ( m_pGeometryArrays->totalReferences() == 0 ) {
//			delete m_pGeometryArrays;
//		} else {
//			m_pGeometryArrays->removeReference();
//		}
//	}
//
//	if ( m_pSkinBox ) {
//		if ( m_pSkinBox->totalReferences() == 0 ) {
//			delete m_pSkinBox;
//		} else {
//			m_pSkinBox->removeReference();
//		}
//	}
//
//	if ( m_pCAS16 ) {
//		if ( m_pCAS16->totalReferences() == 0 ) {
//			delete m_pCAS16;
//		} else {
//			m_pCAS16->removeReference();
//		}
//	}
//
//	if ( m_pFlipPairs ) {
//		if ( m_pFlipPairs->totalReferences() == 0 ) {
//			delete m_pFlipPairs;
//		} else {
//			m_pFlipPairs->removeReference();
//		}
//	}
//
//	if ( m_pCAS32 ) {
//		if ( m_pCAS32->totalReferences() == 0 ) {
//			delete m_pCAS32;
//		} else {
//			m_pCAS32->removeReference();
//		}
//	}
//
//	if ( m_pPartChecksums ) {
//		if ( m_pPartChecksums->totalReferences() == 0 ) {
//			delete m_pPartChecksums;
//		} else {
//			m_pPartChecksums->removeReference();
//		}
//	}
//
//	if ( m_pBranchNodes ) {
//		if ( m_pBranchNodes->totalReferences() == 0 ) {
//			delete m_pBranchNodes;
//		} else {
//			m_pBranchNodes->removeReference();
//		}
//	}
//
//	if ( m_pLeafNodes ) {
//		if ( m_pLeafNodes->totalReferences() == 0 ) {
//			delete m_pLeafNodes;
//		} else {
//			m_pLeafNodes->removeReference();
//		}
//	}
//
//	if ( m_pTriangleMap ) {
//		if ( m_pTriangleMap->totalReferences() == 0 ) {
//			delete m_pTriangleMap;
//		} else {
//			m_pTriangleMap->removeReference();
//		}
//	}
//}
//
//NsModel * NsAtomic::setModel( NsModel * pModelData, NsTextureMan * pTexMan )
//{
//	NsModel * pNextModel;
//
//	// Generate a material manager.
//	m_pMaterial = new NsMaterialMan(pModelData->m_numMaterials);
//
//	// Build the materials.
//	pModelData->buildMaterials ( m_pMaterial, pTexMan );
//
//	// Attach a frame.
////	pFrame = new NsFrame;
//
//	// Build the display list.
//	pNextModel = (NsModel *)pModelData->draw( m_pMaterial, NULL, 0, 0 );
//	m_pTransformedVertices = m_pGeometryArrays = new NsRefTransformedVertices( (NsAnim_PosNormPair *)pModelData->getVertexPool() );
//
//	// The display lists no longer own the pools.
//	for ( int lp = 0; lp < m_pMaterial->m_materialListSize; lp++ ) {
//		NsDL * pDL = m_pMaterial->m_pSortedMaterialList[lp]->headDL();
//		while ( pDL ) {
//			if ( pDL->m_pParent->m_pVertexPool ) {
//				// Set flag to stop vpool being deallocated elsewhere.
//				pDL->m_pParent->m_vpoolFlags |= (1<<4);
//			}
//			pDL = pDL->m_pNext;
//		}
//	}
//
//	NsSkinData		  * pSkinData;
//	NsBBox			  * pSkinBox;
//	unsigned short	  * pCAS16;
//	unsigned int	  * pFlipPairs;
//	unsigned int	  * pCAS32;
//	unsigned int	  * pPartChecksums;
//	NsBranch		  * pBranchNodes;
//	NsLeaf			  * pLeafNodes;
//	unsigned int	  * pTriangleMap;
//
//	pSkinData = pModelData->m_pSkinData;
//	pSkinBox = (NsBBox *)&pSkinData[1];
//	pCAS16 = (unsigned short *)&pSkinBox[pSkinData->numSkinBoxes];
//	pFlipPairs = (unsigned int *)&pCAS16[pSkinData->numCAS16+(pSkinData->numCAS16&1)];
//	pCAS32 = (unsigned int *)&pFlipPairs[pSkinData->numFlipPairs * 2];
//	pPartChecksums = (unsigned int *)&pCAS32[pSkinData->numCAS32];
//	pBranchNodes = (NsBranch *)&pPartChecksums[pSkinData->numPartChecksums];
//	pLeafNodes = (NsLeaf *)&pBranchNodes[pSkinData->numLeaf ? pSkinData->numLeaf - 1 : 0];
//	pTriangleMap = (unsigned int *)&pLeafNodes[pSkinData->numLeaf];
//
//	m_numSkinBoxes = pSkinData->numSkinBoxes;
//	m_numCAS16 = pSkinData->numCAS16;
//	m_numFlipPairs = pSkinData->numFlipPairs;
//	m_numCAS32 = pSkinData->numCAS32;
//	m_numPartChecksums = pSkinData->numPartChecksums;
//	m_removeCAS32 = pSkinData->removeCAS32;
//	m_removeCAS16 = pSkinData->removeCAS16;
//	m_numBranchNodes = pSkinData->numLeaf ? pSkinData->numLeaf - 1 : 0;
//	m_numLeafNodes = pSkinData->numLeaf;
//	m_numTriangles = pSkinData->numTri;
//	
//	// Copy the skin boxes.
//	if ( m_numSkinBoxes ) {
//		m_pSkinBox = new NsRefSkinBox( new NsBBox[m_numSkinBoxes] );
//		memcpy ( m_pSkinBox->m_pSkinBox, pSkinBox, sizeof ( NsBBox ) * m_numSkinBoxes );
//	}
//
//	// Copy the 16 bit CAS flags.
//	if ( m_numCAS16 ) {
//		m_pCAS16 = new NsRefCAS16( new unsigned short[m_numCAS16] );
//		memcpy ( m_pCAS16->m_pCAS16, pCAS16, sizeof ( unsigned short ) * m_numCAS16 );
//	}
//
//	// Copy the flip pairs.
//	if ( m_numFlipPairs ) {
//		m_pFlipPairs = new NsRefFlipPairs( new unsigned int[m_numFlipPairs * 2] );
//		memcpy ( m_pFlipPairs->m_pFlipPairs, pFlipPairs, sizeof ( unsigned int ) * m_numFlipPairs * 2 );
//	}
//
//	// Copy the 32 bit CAS flags.
//	if ( m_numCAS32 ) {
//		m_pCAS32 = new NsRefCAS32( new unsigned int[m_numCAS32] );
//		memcpy ( m_pCAS32->m_pCAS32, pCAS32, sizeof ( unsigned int ) * m_numCAS32 );
//	}
//
//	// Copy the 32 bit CAS flags.
//	if ( m_numPartChecksums ) {
//		m_pPartChecksums = new NsRefPartChecksums( new unsigned int[m_numPartChecksums] );
//		memcpy ( m_pPartChecksums->m_pPartChecksums, pPartChecksums, sizeof ( unsigned int ) * m_numPartChecksums );
//	}
//
//	// Copy the branch nodes.
//	if ( m_numBranchNodes ) {
//		m_pBranchNodes = new NsRefBranchNodes( new NsBranch[m_numBranchNodes] );
//		memcpy ( m_pBranchNodes->m_pBranchNodes, pBranchNodes, sizeof ( NsBranch ) * m_numBranchNodes );
//	}
//
//	// Copy the leaf nodes.
//	if ( m_numLeafNodes ) {
//		m_pLeafNodes = new NsRefLeafNodes( new NsLeaf[m_numLeafNodes] );
//		memcpy ( m_pLeafNodes->m_pLeafNodes, pLeafNodes, sizeof ( NsLeaf ) * m_numLeafNodes );
//	}
//
//	// Copy the triangle map.
//	if ( m_numTriangles ) {
//		m_pTriangleMap = new NsRefTriangleMap( new unsigned int[m_numTriangles] );
//		memcpy ( m_pTriangleMap->m_pTriangleMap, pTriangleMap, sizeof ( unsigned int ) * m_numTriangles );
//	}
//
//	m_stride = pModelData->getStride() * sizeof ( NsVector );
//	m_numVertices = pModelData->m_numVertex;
//	m_frameNumber = (unsigned int)pModelData->m_frameNumber;
//
//	// Fix branch data.
//	if ( m_numBranchNodes ) {
//		unsigned int  * p32 = (unsigned int *)m_pBranchNodes->m_pBranchNodes;
//		unsigned int	value;
//		for ( unsigned int lp = 0; lp < m_numBranchNodes; lp++ ) {
//			value = *p32++;
//			m_pBranchNodes->m_pBranchNodes[lp].type = (unsigned short)( ( value >> 16 ) & 0x0000ffff );
//			m_pBranchNodes->m_pBranchNodes[lp].leftType = (unsigned char)( ( value >> 8 ) & 0x000000ff );
//			m_pBranchNodes->m_pBranchNodes[lp].rightType = (unsigned char)( value & 0x000000ff );
//			value = *p32++;
//			m_pBranchNodes->m_pBranchNodes[lp].leftNode = (unsigned short)( ( value >> 16 ) & 0x0000ffff );
//			m_pBranchNodes->m_pBranchNodes[lp].rightNode = (unsigned short)( value & 0x0000ffff );
//			p32 += 2;
//		}
//	}
//	
//	// Fix leaf data.
//	if ( m_numLeafNodes ) {
//		unsigned int  * p32 = (unsigned int *)m_pLeafNodes->m_pLeafNodes;
//		unsigned int	value;
//		for ( unsigned int lp = 0; lp < m_numLeafNodes; lp++ ) {
//			value = *p32++;
//			m_pLeafNodes->m_pLeafNodes[lp].numPolygons = (unsigned short)( ( value >> 16 ) & 0x0000ffff );
//			m_pLeafNodes->m_pLeafNodes[lp].firstPolygon = (unsigned short)( value & 0x0000ffff );
//		}
//	}
//	
//	return pNextModel;
//}
//
//void NsAtomic::draw( NsCamera * camera )
//{
//	// Check atomic visibility flags.
//	if ( !( m_flags & VISIBLE ) ) return;
//
////	Mtx			m;
////	Mtx			s;
//
//	// Setup draw parameters.
//	NsRender::setCullMode ( NsCullMode_Never );
////	sceneMaterial.cull( camera->getCurrent() );
////	sceneMaterial.draw();
//
//	NsAnim_PosNormPair	  * pPool;
//	if ( m_pDoubleData ) {
//		switch ( NsDisplay::getCurrentBufferIndex() ) {
//			case 0:
//				pPool = m_pTransformedVertices->m_pTransformedVertices;
//				break;
//			case 1:
//				pPool = m_pTransformedVertices2;
//				break;
//			case 2:
//				pPool = m_pTransformedVertices3;
//				break;
//			default:
//				assertf ( false, ( "Illegal triple buffer index.\n" ) );
//				pPool = NULL;
//				break;
//		}
//	} else {
//		pPool = m_pTransformedVertices->m_pTransformedVertices;
//	}
//
//	// Point to array for this atomic (could be cloned).
//    GXSetArray(GX_VA_POS, pPool->pos, m_stride);
//	if ( m_stride > (int)( 3 * sizeof ( float ) ) ) {
//		GXSetArray(GX_VA_NRM, pPool->norm, m_stride);
//	}
//
//	// Draw it.
//	m_pMaterial->draw();
//
//	// Draw a cube, too.
////	NsRender::setBlendMode ( NsBlendMode_None, NULL, (unsigned char)255 );
////    MTXScale(s,8,8,8);
////    MTXConcat(m,s,m);
////    GX::LoadPosMtxImm( m, GX_PNMTX0 );
////	GXDrawCube();
//}
//
//void NsAtomic::draw ( NsCamera * camera, ROMtx * pBoneMatrices, unsigned int transform )
//{
//	NsAnim_PosNormPair	  * pPool;
//	unsigned int		  * p32;
//	NsAnim_PosNormPair	  * pPosNormPair;
//	NsAnim_WeightPair	  * pWeight;
//	float				  * pSingleWeight;
//	unsigned short		  * pIndices;
//
//	// Check atomic visibility flags.
//	if ( !( m_flags & VISIBLE ) ) return;
//
//// 	// Point up stuff we're interested in.	
//// 	pMaterial = (unsigned char *)&pModel[8];
//// 	pGeom = (unsigned int *)&pMaterial[pModel[0]*64];
////	pCount = (unsigned int *)&pSkinHeader[1];
//
//	if ( m_pTransformedVertices && m_numVertices && ( m_lastTransform != NsDisplay::getCurrentBufferIndex() ) && transform ) {
//		AnimTime.start();
//
//		// Transform paired vertices.
//		switch ( NsDisplay::getCurrentBufferIndex() ) {
//			case 0:
//				pPool = m_pTransformedVertices->m_pTransformedVertices;
//				break;
//			case 1:
//				pPool = m_pTransformedVertices2;
//				break;
//			case 2:
//				pPool = m_pTransformedVertices3;
//				break;
//			default:
//				assertf ( false, ( "Illegal triple buffer index.\n" ) );
//				pPool = NULL;
//				break;
//		}
//
//		p32 = m_pDoubleData->m_pDoubleData;
//		while ( p32[0] ) {
//			pPosNormPair = (NsAnim_PosNormPair *)&p32[2];
//			pWeight = (NsAnim_WeightPair *)&pPosNormPair[p32[0]];
//
//			TransformDouble( pBoneMatrices[p32[1]&255], pBoneMatrices[(p32[1]>>8)&255], pWeight->w, pPosNormPair->pos, pPool->pos, p32[0] == 1 ? 2 : p32[0] );
//			//TransformSingle( pBoneMatrices[p32[1]&255], pPosNormPair->pos, pPool->pos, p32[0] == 1 ? 2 : p32[0] );
//			pPool += p32[0];
//			p32 = (unsigned int *)&pWeight[p32[0]];
//		}
//
//		// Transform accumulation vertices.
//		switch ( NsDisplay::getCurrentBufferIndex() ) {
//			case 0:
//				pPool = m_pTransformedVertices->m_pTransformedVertices;
//				break;
//			case 1:
//				pPool = m_pTransformedVertices2;
//				break;
//			case 2:
//				pPool = m_pTransformedVertices3;
//				break;
//			default:
//				assertf ( false, ( "Illegal triple buffer index.\n" ) );
//				pPool = NULL;
//				break;
//		}
//
//		p32 = m_pAccData->m_pAccData;
//		while ( p32[0] ) {
//			pPosNormPair = (NsAnim_PosNormPair *)&p32[2];
//			pSingleWeight = (float *)&pPosNormPair[p32[0]];
//			pIndices = (unsigned short *)&pSingleWeight[p32[0]];
//
//			TransformAcc ( pBoneMatrices[p32[1]], p32[0], pPosNormPair->pos, pPool->pos, pIndices, pSingleWeight );
//			p32 = (unsigned int *)&pIndices[p32[0]+(p32[0]&1)];
//		}
//		DCFlushRange(pPool, sizeof ( NsAnim_PosNormPair ) * m_numVertices);
//	
//		m_lastTransform = NsDisplay::getCurrentBufferIndex();
//		
//		AnimTime.stop();
//	
//		
////		NsAnim_PosNormPair	  * p;
////		p = m_pTransformedVertices->m_pTransformedVertices;
////		m_pTransformedVertices->m_pTransformedVertices = m_pTransformedVertices2;
////		m_pTransformedVertices2 = m_pTransformedVertices3;
////		m_pTransformedVertices3 = p;
//	}
//
//	draw( camera );
//}
//
//NsAtomic& NsAtomic::clone ( void )
//{
//	NsAtomic  * pClonedAtomic;
//
//	// Create a cloned atomic.
//	pClonedAtomic = new NsAtomic;
//	memcpy( pClonedAtomic, this, sizeof( NsAtomic ) );
//
////	// Clone the frame.
////	pClonedAtomic->setFrame( new NsFrame );
////	memcpy( pClonedAtomic->getFrame(), m_pFrame, sizeof( NsFrame ) );
//
//	// Allocate a new transformed vertex pool (only if we have a skin attached).
//	if ( m_pDoubleData ) {
//		pClonedAtomic->m_pTransformedVertices = new NsRefTransformedVertices( new NsAnim_PosNormPair[(m_numVertices+1)] );
//		pClonedAtomic->m_pTransformedVertices2 = new NsAnim_PosNormPair[(m_numVertices+1)];
//		pClonedAtomic->m_pTransformedVertices3 = new NsAnim_PosNormPair[(m_numVertices+1)];
//		gTripleMem += sizeof( NsAnim_PosNormPair ) * (m_numVertices+1) * 3;
//	}
//
//	// Multi-reference counts.
//	if ( m_pGeometryArrays ) m_pGeometryArrays->addReference();
//	if ( m_pMaterial ) m_pMaterial->addReference();
//	if ( m_pDoubleData ) m_pDoubleData->addReference();
//	if ( m_pAccData ) m_pAccData->addReference();
//	if ( m_pSkinBox ) m_pSkinBox->addReference();
//	if ( m_pCAS16 ) m_pCAS16->addReference();
//	if ( m_pFlipPairs ) m_pFlipPairs->addReference();
//	if ( m_pCAS32 ) m_pCAS32->addReference();
//	if ( m_pPartChecksums ) m_pPartChecksums->addReference();
//	if ( m_pBranchNodes ) m_pBranchNodes->addReference();
//	if ( m_pLeafNodes ) m_pLeafNodes->addReference();
//	if ( m_pTriangleMap ) m_pTriangleMap->addReference();
//
//	return *pClonedAtomic;
//}
//
//NsAnim_SkinHeader * NsAtomic::attachSkinData ( NsAnim_SkinHeader * pSkinHeader )
//{
//	unsigned int		  * pSkinData;
//	NsAnim_PosNormPair	  * pPosNormPair;
//	NsAnim_WeightPair	  * pWeight;
//	float				  * pSingleWeight;
//	unsigned short		  * pIndices;
//	unsigned int		  * pStart;
//	int						size;
//
//	pSkinData = (unsigned int *)&pSkinHeader[1];
//
//	// Skip the double blend data, and count the number of vertices in this atomic.
//	pStart = pSkinData;
//	while ( pSkinData[0] ) {
//		pPosNormPair = (NsAnim_PosNormPair *)&pSkinData[2];
//		pWeight = (NsAnim_WeightPair *)&pPosNormPair[pSkinData[0]];
//		pSkinData = (unsigned int *)&pWeight[pSkinData[0]];
//	}
//	pSkinData++;		// Skip the terminator.
//
//	// Make a new instance of this data.
//	size = ((int)pSkinData)-((int)pStart);
//	m_pDoubleData = new NsRefDoubleData( new unsigned int[size/4] );
//	memcpy ( m_pDoubleData->m_pDoubleData, pStart, size );
//
//	// Skip the accumulation blend data.
//	pStart = pSkinData;
//	while ( pSkinData[0] ) {
//		pPosNormPair = (NsAnim_PosNormPair *)&pSkinData[2];
//		pSingleWeight = (float *)&pPosNormPair[pSkinData[0]];
//		pIndices = (unsigned short *)&pSingleWeight[pSkinData[0]];
//		pSkinData = (unsigned int *)&pIndices[pSkinData[0]+(pSkinData[0]&1)];
//	}
//	pSkinData++;		// Skip the terminator.
//
//	// Make a new instance of this data.
//	size = ((int)pSkinData)-((int)pStart);
//	m_pAccData = new NsRefAccData( new unsigned int[size/4] );
//	memcpy ( m_pAccData->m_pAccData, pStart, size );
//
//	// Allocate 2nd & 3rd transform buffers.
//	m_pTransformedVertices2 = new NsAnim_PosNormPair[(m_numVertices+1)];
//	m_pTransformedVertices3 = new NsAnim_PosNormPair[(m_numVertices+1)];
//	gTripleMem += sizeof( NsAnim_PosNormPair ) * (m_numVertices+1) * 2;
//
//	return (NsAnim_SkinHeader *)pSkinData;
//}















