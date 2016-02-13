///********************************************************************************
// *																				*
// *	Module:																		*
// *				NsClump															*
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
//#include "p_clump.h"
//#include "p_file.h"
//#include "p_prim.h"
//#include <sk/engine/bsputils.h>
//#include "p_display.h"
//#include "p_assert.h"
//
///********************************************************************************
// * Defines.																		*
// ********************************************************************************/
//#define rpWORLDUVS ( rpWORLDTEXTURED2 | rpWORLDTEXTURED )
//
//#define rpSKINPOPPARENTMATRIX	 0x01
//#define rpSKINPUSHPARENTMATRIX	 0x02
//
//enum
//{
//	mTWO_SIDED			= 0x01,
//	mINVISIBLE			= 0x02,
//	mFRONT_FACING		= 0x04,
//	mTRANSPARENT		= 0x08,
//	mDECAL				= 0x10,
//	mUV_WIBBLE_SUPPORT	= 0x20,	// If these two values change, notify Steve as Rw references 
//	mVC_WIBBLE_SUPPORT	= 0x40, // these two flags by value.
//};
//
//enum RpWorldFlag
//{
//    rpWORLDTRISTRIP = 0x01,     /**<This world's meshes can be rendered
//                                   as tri strips */
//    rpWORLDTEXTURED = 0x04,     /**<This world has one set of texture coordinates */
//    rpWORLDPRELIT = 0x08,       /**<This world has luminance values */
//    rpWORLDNORMALS = 0x10,      /**<This world has normals */
//    rpWORLDLIGHT = 0x20,        /**<This world will be lit */
//    rpWORLDMODULATEMATERIALCOLOR = 0x40, /**<Modulate material color with
//                                            vertex colors (pre-lit + lit) */
//    rpWORLDTEXTURED2 = 0x80, /**<This world has 2 set of texture coordinates */
//
//    /*
//     * These flags are stored in the flags field in an RwObject, the flag field is 8bit.
//     */
//
//    rpWORLDSECTORSOVERLAP = 0x40000000,
//};
//typedef enum RpWorldFlag RpWorldFlag;
//
//enum
//{
//	MAX_NUM_SEQUENCES	= 8
//};
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
// *				NsClump															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Declares a clump object.										*
// *																				*
// ********************************************************************************/
//
//NsClump::NsClump()
//{
//	m_pAtomicHead = NULL;
//	m_numAtomic	= 0;
//
//	for( int i = 0; i < NUM_BLEND_ANIMS; ++i )
//	{
//		m_pAnim[i]			= NULL;
//		m_AnimWeights[i]	= ( i == 0 ) ? 1.0f : 0.0f;
//	}
//
//	m_AnimFlipped = false;
//	m_pFrameList = NULL;
//	m_pBoneMat = NULL;
//	m_pBoneData = NULL;
//
//	m_pWorld = NULL;
//	m_pUserData = NULL;
//	m_pTexMan	= NULL;
//}
//
//NsClump::NsClump( NsModel * pModelData )
//{
//	m_pAtomicHead = NULL;
//	m_numAtomic	= 0;
//
//	for( int i = 0; i < NUM_BLEND_ANIMS; ++i )
//	{
//		m_pAnim[i]		= NULL;
//		m_AnimWeights[i]	= ( i == 0 ) ? 1.0f : 0.0f;
//	}
//
//	m_AnimFlipped = false;
//	m_pFrameList = NULL;
//	m_pBoneMat = NULL;
//	m_pBoneData = NULL;
//
//	m_pWorld = NULL;
//	m_pUserData = NULL;
//	m_pTexMan	= NULL;
//
//	setModel( pModelData );
//}
//
//NsClump::NsClump( unsigned int * pDFF )
//{
//	m_pAtomicHead = NULL;
//	m_numAtomic	= 0;
//
//	for( int i = 0; i < NUM_BLEND_ANIMS; ++i )
//	{
//		m_pAnim[i]		= NULL;
//		m_AnimWeights[i]	= ( i == 0 ) ? 1.0f : 0.0f;
//	}
//
//	m_AnimFlipped = false;
//	m_pFrameList = NULL;
//	m_pBoneMat = NULL;
//	m_pBoneData = NULL;
//
//	m_pWorld = NULL;
//	m_pUserData = NULL;
//	m_pTexMan	= NULL;
//
//	setAtomics( pDFF );
//}
//
//NsClump::~NsClump()
//{
////	OSReport( "Deleting clump:\n" );
//
//	NsDisplay::flush();
//	
//	NsAtomic * pAtomic = m_pAtomicHead;
//	while ( pAtomic ) {
//		NsAtomic * pDelete = pAtomic;
//		pAtomic = pAtomic->m_pNext;
////		OSReport( "...pAtomic 0x%08x\n", pDelete );
//		delete pDelete;
//	}
////	OSReport( "m_pFrame 0x%08x\n", m_pFrame );
////	OSReport( "m_pBoneMat 0x%08x\n", m_pBoneMat );
////	OSReport( "m_pBoneData 0x%08x\n", m_pBoneData );
//	if ( m_pFrameList ) delete m_pFrameList;
//	if ( m_pBoneMat ) delete m_pBoneMat;
//
//	for ( int lp = 0; lp < NUM_BLEND_ANIMS; lp++ ) {
////		OSReport( "m_pAnim[%d] 0x%08x\n", lp, m_pAnim[lp] );
//		if ( m_pAnim[lp] ) delete m_pAnim[lp];
//	}
//
//	// Multi-reference stuff.
//	if ( m_pBoneData ) {
//		if ( m_pBoneData->totalReferences() == 0 ) {
//			delete m_pBoneData;
//		} else {
//			m_pBoneData->removeReference();
//		}
//	}
//}
//
//NsClump& NsClump::clone ( void )
//{
//	int					lp;
//	NsClump			  * pClonedClump;
//	NsAnim			  * pClonedAnim;
//	NsAtomic		  * pClonedAtomic;
//	NsAtomic		  * pAtomic;
//
//	// Create a new clump.
//	pClonedClump = new NsClump;
//	memcpy( pClonedClump, this, sizeof( NsClump ) );
//	pClonedClump->m_pAtomicHead = NULL;
//	pClonedClump->m_numAtomic = 0;
//	// Create new anim structure.
//	for( int i = 0; i < NUM_BLEND_ANIMS; ++i )
//	{
//		if ( m_pAnim[i] ) {
//			pClonedAnim = &m_pAnim[i]->clone();
//
////			memcpy( pClonedAnim, m_pAnim, sizeof( NsAnim ) );
//			pClonedClump->m_pAnim[i] = pClonedAnim;
//		}
//	}
//	// Copy the bone data.
////	if ( m_pBoneData ) {
////		pClonedClump->m_pBoneData = new NsAnim_BoneData[m_numBones];
////		memcpy ( pClonedClump->m_pBoneData, m_pBoneData, sizeof ( NsAnim_BoneData ) * m_numBones );
////	}
//
////	if ( m_pFrame ) {
////		pClonedClump->m_pFrame = new NsFrame;
////		memcpy ( pClonedClump->m_pFrame, m_pFrame, sizeof ( NsFrame ) );
////	}
//
//	pAtomic = m_pAtomicHead;
//	// Copy the atomics over.
//	for ( lp = 0; lp < m_numAtomic; lp++ ) {
//		// Create a cloned atomic.
//		pClonedAtomic = &pAtomic->clone();
//
//		// Add it to the cloned clump.
//		pClonedClump->addAtomic( pClonedAtomic );
//
////		// If this is the head node, set up the frame for the clump.
////		if ( pAtomic->getFrame() == m_pFrame ) {
////			delete pClonedAtomic->getFrame();
////			pClonedAtomic->setFrame( pClonedClump->m_pFrame );
//////			pClonedClump->m_pFrame = pClonedAtomic->getFrame();
////		}
//
//		// Onto the next atomic.
//		pAtomic = pAtomic->m_pNext;
//	}
////	if ( m_numAtomic ) pClonedClump->m_pFrame = pClonedClump->m_pAtomicHead->getFrame();
//
//
//	// Copy the framelist.
//	pClonedClump->m_pFrameList = new NsFrame[m_numFrames];
//	memcpy ( pClonedClump->m_pFrameList, m_pFrameList, sizeof ( NsFrame ) * m_numFrames );
//	pClonedClump->m_pFrame = &pClonedClump->m_pFrameList[0];
//
//	// Copy the bone matrices if present.
//	if( m_pBoneMat )
//	{
//		pClonedClump->m_pBoneMat = new ROMtx[m_numBones];
//		memcpy( pClonedClump->m_pBoneMat, m_pBoneMat, sizeof( ROMtx ) * m_numBones );
//	}
//
//	// Search all atomics, and attach it the ones with this framenumber.
//	pAtomic = pClonedClump->m_pAtomicHead;
//	while ( pAtomic ) {
////		delete pAtomic->getFrame();
//		pAtomic->setFrame( &pClonedClump->m_pFrameList[pAtomic->m_frameNumber] );
//		pAtomic = pAtomic->m_pNext;
//	}
//
//	// Multi-reference counts.
//	if ( m_pBoneData ) m_pBoneData->addReference();
//
//	return *pClonedClump;
//}
//
//NsModel * NsClump::setModel ( NsModel * pModelData )
//{
//	NsModel * m_pNextModel;
//
//	// Create the atomic.
//	NsAtomic * pAtomic = new NsAtomic;
//	m_pNextModel = pAtomic->setModel( pModelData, m_pTexMan );
//	addAtomic( pAtomic );
//
//	// Set the frame.
//	m_pFrame = new NsFrame;
//	m_pAtomicHead->setFrame( m_pFrame );
//
//	// Dummy up the framelist.
//	m_numFrames = 1;
//	m_pFrameList = m_pFrame;
//
//	return m_pNextModel;
//}
//
//void NsClump::setAtomics ( unsigned int * pDFF )
//{
//	NsFrame		  * pFrameList = (NsFrame *)&pDFF[8];
//	NsModel		  * pModelData = (NsModel *)&pFrameList[pDFF[2]];
//	NsModel		  * m_pNextModel;
//	unsigned int	lp;
//	NsAtomic	  * pAtomic;
//
//	// Note, these are added in reverse ( I believe Renderware does it like
//	// that, as adding them forwards is bogus).
//	m_pFrame = NULL;
//	for ( lp = 0; lp < pDFF[0]; lp++ ) {
//		pAtomic = new NsAtomic;
//		m_pNextModel = pAtomic->setModel( pModelData, m_pTexMan );
//		OSReport ( "Atomic %d: %d\n", lp, pModelData->m_frameNumber );
//		addAtomic( pAtomic );
////		pAtomic->m_pNext = m_pAtomicHead;
////		m_pAtomicHead = pAtomic;
////		m_numAtomic++;
//		pModelData = m_pNextModel;
//	}
//
//	// Turn the framelist into actual frames and attach to the atomics.
//	m_numFrames = pDFF[2];
//	m_pFrameList = new NsFrame[m_numFrames];
//	memcpy ( m_pFrameList, pFrameList, sizeof ( NsFrame ) * m_numFrames );
//	m_pFrame = &m_pFrameList[0];
//
//	// Search all atomics, and attach it the ones with this framenumber.
//	pAtomic = m_pAtomicHead;
//	while ( pAtomic ) {
//		pAtomic->setFrame( &m_pFrameList[pAtomic->m_frameNumber] );
//		pAtomic = pAtomic->m_pNext;
//	}
//
//	// If this clump has an animation hierarchy, set it up.
//	if ( pDFF[1] ) {
//
//		unsigned char	  * p8;
//		NsAnim_BoneHeader * pBoneHeader;
//		NsAnim_SkinHeader * pSkinHeader;
//		NsAnim_BoneData	  * pTempBone;
//
////		NsWeightPair	  * pWeight;
////		float			  *	pSingleWeight;
////		unsigned short	  * pIndices;
////
////		NsPosNormPair	  * pPosNormPair;
////		NsPosNormPair	  *	pTransformedVertices;
////		NsPosNormPair	  * pCurrentVertexPool;
////		unsigned int	  * pCount;
////
////		unsigned int	  * pIndex;
////
////		unsigned char	  * p8;
////
////		unsigned int	  * vertexcount;
//
//		// Skip the actual DFF data & point to the animation data.
//		p8 = (unsigned char *)pDFF;
//		pBoneHeader = (NsAnim_BoneHeader *)&p8[pDFF[1]];
//		pTempBone = (NsAnim_BoneData *)&pBoneHeader[1];
//		pSkinHeader = (NsAnim_SkinHeader *)&pTempBone[pBoneHeader->numBones];
//		m_numBones = pBoneHeader->numBones;
//
//		for( int i = 0; i < NUM_BLEND_ANIMS; ++i )
//		{
//			m_pAnim[i] = new NsAnim( m_numBones );
//		}
//
//		// Copy the bone data.
//		m_pBoneData = new NsRefBoneData( new NsAnim_BoneData[pBoneHeader->numBones] );
//		memcpy ( m_pBoneData->m_pBoneData, pTempBone, sizeof ( NsAnim_BoneData ) * pBoneHeader->numBones );
//
//		// Create the bone display matrices.
//		m_pBoneMat = new ROMtx[pBoneHeader->numBones];
//
//		// Attach animation data to all atomics.
//		pAtomic = m_pAtomicHead;
//		while ( pAtomic ) {
//			// The atomic will attach the animation data, and return a pointer past it.
//			pSkinHeader = pAtomic->attachSkinData( pSkinHeader );
//
//			// Next atomic.
//			pAtomic = pAtomic->m_pNext;
//		}
////		for ( lp = 0; lp < pDFF[0]; lp++ ) {
////			
////
////
////			pCount = (unsigned int *)&pSkinHeader[1];
////
////			// Scan to end of blended weights.
////			vertexcount = 0;
////			while ( pCount[0] ) {
////				pPosNormPair = (PosNormPair *)&pCount[2];
////				pWeight = (WeightPair *)&pPosNormPair[pCount[0]];
////				vertexcount += pCount[0];
////				pCount = (unsigned int *)&pWeight[pCount[0]];
////			}
////			pCount++;
////
////			// Now, we know how many vertices for this atomic, and how many
////			// pairs - attach them.
////
////			// Onto animation data for next atomic.
////			pTransformedVertices = pPool;
////		}
//	}
//}
//
//void NsClump::merge( NsClump& source )
//{
//	NsAtomic * pAtomic = source.m_pAtomicHead;
//
//	// Merge in the materials.
//	m_pTexMan->merge( *source.m_pTexMan );
//
//	// Add all atomics from the source.
//	while ( pAtomic ) {
//		// Point to the clump's frame.
//		pAtomic->m_pFrame = m_pFrame;
//		// Delete the  flippairs
//		if ( pAtomic->m_pFlipPairs ) {
//			if ( pAtomic->m_pFlipPairs->totalReferences() == 0 ) {
//				delete pAtomic->m_pFlipPairs;
//				pAtomic->m_pFlipPairs = NULL;
//				pAtomic->m_numFlipPairs = 0;
//			} else {
//				pAtomic->m_pFlipPairs->removeReference();
//			}
//		}
//		// Add this atomic to the clump.
//		addAtomic( pAtomic );
//		// Onto the next one, deleting as we go.
//		pAtomic = pAtomic->m_pNext;
//	}
//	source.m_pAtomicHead = NULL;
//	source.m_numAtomic = 0;
//}
//
//
//
//void NsClump::buildBoneMatrices( NsQFrame* p_q_frames, NsTFrame* p_t_frames )
//{
//    NsMatrix		bts;
//	unsigned int	lp;
//	int				currentMatrix;
//    NsMatrix		parentMatrix;
//    NsMatrix		matrixStack[32];
//    NsMatrix  *		pMatrixStackTop = matrixStack;
//	NsMatrix		boneMatrix;
//	NsMatrix		tempMatrix;
//
//	// Point up what we need.
//	currentMatrix = -1;
//
//	// This is the root.
//    parentMatrix.identity();
//
//	for ( lp = 0; lp < m_numBones; lp++ ) {
//		// Build bone to skin (bts) matrix.
//		bts.setRight( m_pBoneData->m_pBoneData[lp].rightx, m_pBoneData->m_pBoneData[lp].upx, m_pBoneData->m_pBoneData[lp].atx );
//		bts.setUp( m_pBoneData->m_pBoneData[lp].righty, m_pBoneData->m_pBoneData[lp].upy, m_pBoneData->m_pBoneData[lp].aty );
//		bts.setAt( m_pBoneData->m_pBoneData[lp].rightz, m_pBoneData->m_pBoneData[lp].upz, m_pBoneData->m_pBoneData[lp].atz );
//		bts.setPos( m_pBoneData->m_pBoneData[lp].posx, m_pBoneData->m_pBoneData[lp].posy, m_pBoneData->m_pBoneData[lp].posz );
//
//		// Push parent if required.
//		if ( m_pBoneData->m_pBoneData[lp].flags & rpSKINPUSHPARENTMATRIX ) {
//			memcpy ( pMatrixStackTop, &parentMatrix, sizeof ( NsMatrix ) );
//			pMatrixStackTop++;
//		}
//
//		boneMatrix.fromQuat( &p_q_frames[lp].q );
//		boneMatrix.setPos( p_t_frames[lp].v.x, p_t_frames[lp].v.y, p_t_frames[lp].v.z );
//
//		// Multiply this matrix by the parent.
//		tempMatrix.cat( parentMatrix, boneMatrix );
//
//		// Multiply this matrix by the parent.
//		bts.cat( tempMatrix, bts );
//
//		// Convert this matrix to the format required for fast transformation.
////		PSMTXReorder ( bts, pBoneMat[lp] );
//
//		m_pBoneMat[lp][0][0] = bts.getRightX();
//		m_pBoneMat[lp][0][1] = bts.getUpX();
//		m_pBoneMat[lp][0][2] = bts.getAtX();
//		m_pBoneMat[lp][1][0] = bts.getRightY();
//		m_pBoneMat[lp][1][1] = bts.getUpY();
//		m_pBoneMat[lp][1][2] = bts.getAtY();
//		m_pBoneMat[lp][2][0] = bts.getRightZ();
//		m_pBoneMat[lp][2][1] = bts.getUpZ();
//		m_pBoneMat[lp][2][2] = bts.getAtZ();
//		m_pBoneMat[lp][3][0] = bts.getPosX();
//		m_pBoneMat[lp][3][1] = bts.getPosY();
//		m_pBoneMat[lp][3][2] = bts.getPosZ();
//
//		// Update parent (pop if required).
//		if ( m_pBoneData->m_pBoneData[lp].flags & rpSKINPOPPARENTMATRIX) {
//			pMatrixStackTop--;
//			memcpy ( &parentMatrix, pMatrixStackTop, sizeof ( NsMatrix ) );
//        } else {
//			memcpy ( &parentMatrix, &tempMatrix, sizeof ( NsMatrix ) );
//        }
//	}
//}
//
//
//
//
//
//void NsClump::flipBoneMatrices( void )
//{
//	NsAtomic * pAtomic = m_pAtomicHead;
//	while ( pAtomic ) {
//		if ( pAtomic->m_numFlipPairs > 0 ) {
//			// Flips the ROMtx matrices (pre built fast-trasnformation format matrices).
//			// To flip the hierarchy, simply negate the x axis components of the 'at' and 'up' vectors, Then rebuild
//			// the right vector. (Negating the x component of the right vector will lead to matrix inversion, which will flip textures etc.)
//			for( unsigned int i = 0;; i += 2 )
//			{
//				assertp(( i + 1 ) < ( pAtomic->m_numFlipPairs * 2 ));
//				int matrix0 = pAtomic->m_pFlipPairs->m_pFlipPairs[i];
//				if( matrix0 == -1 )
//				{
//					// Not interested.
//					continue;
//				}
//
//				int matrix1 = pAtomic->m_pFlipPairs->m_pFlipPairs[i + 1];
//
//				if(( matrix0 == 0 ) && ( matrix1 == 0 ))
//				{
//					// Terminator.
//					break;
//				}
//
//				NsVector right, up, at, pos;
//
//				if( matrix1 == -1 )
//				{
//					// Just a simple rotate.
//					ROMtx* p_matrix0	= m_pBoneMat + matrix0;
//
//					// Generate the right vector from up and at.
//					up.set( -p_matrix0[0][0][1], p_matrix0[0][1][1], p_matrix0[0][2][1] );
//					at.set( -p_matrix0[0][0][2], p_matrix0[0][1][2], p_matrix0[0][2][2] );
//					right.cross( up, at );
//
//					pos.set( -p_matrix0[0][3][0], p_matrix0[0][3][1], p_matrix0[0][3][2] );
//
//					p_matrix0[0][0][0]	= right.x;
//					p_matrix0[0][1][0]	= right.y;
//					p_matrix0[0][2][0]	= right.z;
//					p_matrix0[0][0][1]	= up.x;
//					p_matrix0[0][0][2]	= at.x;
//					p_matrix0[0][3][0]	= pos.x;
//				}
//				else
//				{
//					ROMtx* p_matrix0	= m_pBoneMat + matrix0;
//					ROMtx* p_matrix1	= m_pBoneMat + matrix1;
//
//					// Need to save matrix1 as it will now be written to.
//					ROMtx m1;
//					memcpy( &m1, p_matrix1, sizeof( ROMtx ));
//
//					// Generate the right vector from up and at.
//					up.set( -p_matrix0[0][0][1], p_matrix0[0][1][1], p_matrix0[0][2][1] );
//					at.set( -p_matrix0[0][0][2], p_matrix0[0][1][2], p_matrix0[0][2][2] );
//					right.cross( up, at );
//					pos.set( -p_matrix0[0][3][0], p_matrix0[0][3][1], p_matrix0[0][3][2] );
//
//					p_matrix1[0][0][0]	= right.x;
//					p_matrix1[0][1][0]	= right.y;
//					p_matrix1[0][2][0]	= right.z;
//					p_matrix1[0][0][1]	= up.x;
//					p_matrix1[0][1][1]	= up.y;
//					p_matrix1[0][2][1]	= up.z;
//					p_matrix1[0][0][2]	= at.x;
//					p_matrix1[0][1][2]	= at.y;
//					p_matrix1[0][2][2]	= at.z;
//					p_matrix1[0][3][0]	= pos.x;
//					p_matrix1[0][3][1]	= pos.y;
//					p_matrix1[0][3][2]	= pos.z;
//
//					// Generate the right vector from up and at.
//					up.set( -m1[0][1], m1[1][1], m1[2][1] );
//					at.set( -m1[0][2], m1[1][2], m1[2][2] );
//					right.cross( up, at );
//					pos.set( -m1[3][0], m1[3][1], m1[3][2] );
//
//					p_matrix0[0][0][0]	= right.x;
//					p_matrix0[0][1][0]	= right.y;
//					p_matrix0[0][2][0]	= right.z;
//					p_matrix0[0][0][1]	= up.x;
//					p_matrix0[0][1][1]	= up.y;
//					p_matrix0[0][2][1]	= up.z;
//					p_matrix0[0][0][2]	= at.x;
//					p_matrix0[0][1][2]	= at.y;
//					p_matrix0[0][2][2]	= at.z;
//					p_matrix0[0][3][0]	= pos.x;
//					p_matrix0[0][3][1]	= pos.y;
//					p_matrix0[0][3][2]	= pos.z;
//				}
//			}
//		}
//		pAtomic = pAtomic->m_pNext;
//	}
//}
//
//
//
//void NsClump::processAnims( void )
//{
//	// Check there are valid animations attached.
//	assertp( m_pAnim[0] );
//
//	// Update the keyframes & buid the bone matrices if we're animated.
//	for( int i = 0; i < NUM_BLEND_ANIMS; ++i )
//	{
//		if( m_pAnim[i] && ( m_AnimWeights[i] > 0.0f ))
//		{
//			// Select the correct start and end keys for each bone.
//			m_pAnim[i]->update();
//
//			// Interpolate the intermediate key for time <t> for each bone.			
//			m_pAnim[i]->buildKeys();
//		}
//	}
//
//	// We now have one or more animations with correct keys. If not currently blending, we can use the
//	// keys from the first animation, otherwise, we have to build a weighted set of keys considering
//	// all animations that are currently in the blend.
//	if( m_AnimWeights[0] == 1.0f )
//	{
//		// No blending required.
//	}
//	else
//	{
//		// Scan through the sets of frames, interpolating between the 2 based on the blend values.
//		float sum = 0.0f;
//		for( int i1 = ( NUM_BLEND_ANIMS - 1 ); i1 > 0; --i1 )
//		{
//			float blend1 = m_AnimWeights[i1];
//			if( blend1 > 0.0f )
//			{
//				blend1 += sum;
//
//				int i0 = i1 - 1;
//				while(( m_AnimWeights[i0] <= 0.0f ) && ( i0 > 0 ))
//				{
//					--i0;
//				}
//
//				float blend0 = m_AnimWeights[i0];
//
//				sum += m_AnimWeights[i1];
//
//				// Need to normalise blend0 and blend1 so they sum to 1.0.
//				blend0 /= ( blend0 + blend1 );
//				blend1 = 1.0f - blend0;
//
//				for( unsigned int bone = 0; bone < m_numBones; ++bone )
//				{
//					// Linearly interpolate positions, SLERP the quaternions.
//					m_pAnim[i0]->getTFrameNow()[bone].v.lerp( m_pAnim[i0]->getTFrameNow()[bone].v, m_pAnim[i1]->getTFrameNow()[bone].v, blend1 );
//					m_pAnim[i0]->getQFrameNow()[bone].q.slerp( m_pAnim[i0]->getQFrameNow()[bone].q, m_pAnim[i1]->getQFrameNow()[bone].q, blend1 );
//				}
//			}
//		}
//	}
//
//	// Build the final bone matrices.
//	buildBoneMatrices( m_pAnim[0]->getQFrameNow(), m_pAnim[0]->getTFrameNow());
//
//	// If the animation is flipped, post-process the ROMtx bone matrix array.
//	if( m_AnimFlipped && m_pAnim[0] )
//	{
//		flipBoneMatrices();
//	}
//}
//
//
//
//void NsClump::getHookPosition( NsVector& hook_in, unsigned int bone, NsVector& hook_out )
//{
//    NsMatrix bm;
//
//	bm.setRightX( m_pBoneMat[bone][0][0] );
//	bm.setUpX( m_pBoneMat[bone][0][1] );
//	bm.setAtX( m_pBoneMat[bone][0][2] );
//	bm.setRightY( m_pBoneMat[bone][1][0] );
//	bm.setUpY( m_pBoneMat[bone][1][1] );
//	bm.setAtY( m_pBoneMat[bone][1][2] );
//	bm.setRightZ( m_pBoneMat[bone][2][0] );
//	bm.setUpZ( m_pBoneMat[bone][2][1] );
//	bm.setAtZ( m_pBoneMat[bone][2][2] );
//	bm.setPosX( m_pBoneMat[bone][3][0] );
//	bm.setPosY( m_pBoneMat[bone][3][1] );
//	bm.setPosZ( m_pBoneMat[bone][3][2] );
//
//	NsVector temp;
//
//	bm.multiply( &hook_in, &temp );
//	m_pFrame->getModelMatrix()->multiply( &temp, &hook_out );
//}
//
//void NsClump::draw( NsCamera * camera, unsigned int transform )
//{
//	NsMatrix		root;
//	NsMatrix		model_transform = *m_pFrame->getModelMatrix();
//	NsMatrix		m;
////	NsMatrix		final;
//	NsAtomic	  * pAtomic;
//	unsigned int	visible;
//
//	// Cat the clump's frame with the camera view matrix.
//	root.cat( *camera->getCurrent(), *m_pFrame->getModelMatrix() );
//
//	// Do animation processing where appropriate.
//	if( m_pAnim[0] )
//	{
//		processAnims();
//	}
//
//	// If you want to perform lighting, you must also set normal transformation matrix. In general case, such
//	// matrix can be obtained as inverse-transpose of the position transform matrix.
//	NsMatrix inverse_model_transform;
//
//	pAtomic = m_pAtomicHead;
//	while ( pAtomic ) {
//		// Cat this matrix with the the parent matrix (unless it is the parent).
//		if ( m_pFrame == pAtomic->getFrame() ) {
//			GX::LoadPosMtxImm( root.m_matrix, GX_PNMTX0 );
//			visible = pAtomic->getMaterials()->cull( &root );
//
//			MTXInverse( model_transform.m_matrix, inverse_model_transform.m_matrix );
//		} else {
//			m.cat( root, *pAtomic->getFrame()->getModelMatrix() );
//			GX::LoadPosMtxImm( m.m_matrix, GX_PNMTX0 );
//			visible = pAtomic->getMaterials()->cull( &m );
//			
//			m.cat( model_transform, *pAtomic->getFrame()->getModelMatrix() );
//			MTXInverse( m.m_matrix, inverse_model_transform.m_matrix );
//		}
//		if ( visible ) {
//			NsMatrix normal_transform;
//			MTXTranspose( inverse_model_transform.m_matrix, normal_transform.m_matrix );
//			GXLoadNrmMtxImm( normal_transform.m_matrix, GX_PNMTX0 );
//	
//			// Draw it.
//			if ( m_pAnim[0] ) {
//				// Animated.
//				pAtomic->draw( camera, m_pBoneMat, transform );
//			} else {
//				// Static.
//				pAtomic->draw( camera );
//			}
//		}
//
//		// Onto the next one.
//		pAtomic = pAtomic->m_pNext;
//	}
//}
//
////void NsClump::applyAnim( NsAnim * m_pAnim )
////{
////	Mtx	root;
////	Mtx	m;
////
////	NsAtomic * pAtomic = m_pAtomicHead;
////
////	MTXConcat(camera->getCurrent(), m_pFrame->getModelMatrix(), root);
////
////	while ( pAtomic ) {
////		// Cat this matrix with the the parent matrix (unless it is the parent).
////		if ( m_pFrame == pAtomic->getFrame() ) {
////		    GX::LoadPosMtxImm( root, GX_PNMTX0 );
////		} else {
////			MTXConcat(root, pAtomic->getFrame()->getModelMatrix(), m);
////		    GX::LoadPosMtxImm( m, GX_PNMTX0 );
////		}
////		// Draw it.
////		pAtomic->draw( camera );
////		// Onto the next one.
////		pAtomic = pAtomic->m_pNext;
////	}
////}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				draw															*
// *	Inputs:																		*
// *				pMaterialList	The list of materials this model uses.			*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Draws the specified model.										*
// *																				*
// ********************************************************************************/
////void NsClump::draw( NsCamera * camera,  )
////{
////	// Animated draw.
////	buildBoneMatrices( m_pAnim, pDFF, v, objMat, boneMat );
////	_drawSkinned ( pMaterialList, v, pDFF, objMat, m_pAnim, pPool, boneMat );
//
//// 	pModel = &pDFF[8];
////
////	p8 = (unsigned char *)pDFF;
////	pBoneHeader = (BoneHeader *)&p8[pDFF[1]];
////	m_pBoneData = (BoneData *)&pBoneHeader[1];
////	pSkinHeader = (SkinHeader *)&m_pBoneData[pBoneHeader->m_numBones];
////
////	pTransformedVertices = pPool;
////	//	pTex = TexMan_Retrieve ( "SUB_AS_satellite_01.png" );
////	for ( chunk = 0; chunk < pDFF[0]; chunk++ ) {
////
////	 	// Point up stuff we're interested in.	
////	 	pMaterial = (unsigned char *)&pModel[8];
////	 	pGeom = (unsigned int *)&pMaterial[pModel[0]*64];
////		pCount = (unsigned int *)&pSkinHeader[1];
////
////		// Transform paired vertices.
////	    GQRSetup6 ( 0, GQR_TYPE_F32, 0, GQR_TYPE_F32 );		// Set read/write scale & type.		
////	    GQRSetup7 ( 0, GQR_TYPE_F32, 0, GQR_TYPE_F32 );		// Set read/write scale & type.		
////		pCurrentVertexPool = pTransformedVertices;
////		vertexcount = 0;
////		while ( pCount[0] ) {
////			pPosNormPair = (PosNormPair *)&pCount[2];
////			pWeight = (WeightPair *)&pPosNormPair[pCount[0]];
////
////			if ( blendVtx ) {
////				BlendTransformFloat( pBoneMat[pCount[1]&255], pBoneMat[(pCount[1]>>8)&255], pWeight->w, pPosNormPair->pos, pTransformedVertices->pos, pCount[0] == 1 ? 2 : pCount[0] );
//////				DCFlushRange(pTransformedVertices, sizeof ( PosNormPair ) * pCount[0]);
////				pTransformedVertices += pCount[0];
////			} else {
////				TransformFloat( pBoneMat[pCount[1]&255], pPosNormPair->pos, pTransformedVertices->pos, pCount[0] == 1 ? 2 : pCount[0] );
//////				DCFlushRange(pTransformedVertices, sizeof ( PosNormPair ) * pCount[0]);
////				pTransformedVertices += pCount[0];
////			}
////			vertexcount += pCount[0];
////			pCount = (unsigned int *)&pWeight[pCount[0]];
////		}
////		pCount++;
////
////		// Transform accumulation vertices.
////		while ( pCount[0] ) {
////			pPosNormPair = (PosNormPair *)&pCount[2];
////			pSingleWeight = (float *)&pPosNormPair[pCount[0]];
////			pIndices = (unsigned short *)&pSingleWeight[pCount[0]];
////
////			if ( blendVtx ) {
////				AccTransformFloat ( pBoneMat[pCount[1]], pCount[0], pPosNormPair->pos, pCurrentVertexPool->pos, pIndices, pSingleWeight );
////			}
////			pCount = (unsigned int *)&pIndices[pCount[0]+(pCount[0]&1)];
////		}
////		pCount++;
////		DCFlushRange(pCurrentVertexPool, sizeof ( PosNormPair ) * vertexcount);
////
////	    // Set format of position, color and tex coordinates.
////	    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
////		if ( pGeom[3] & rpWORLDNORMALS ) GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
////	    if ( pGeom[3] & rpWORLDPRELIT ) GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
////		if ( pGeom[3] & rpWORLDUVS ) GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
////
////		GXSetNumTevStages(1);
////}
//
//
//
//void NsClump::setAnimWeights( float* p_weights, int num_weights )
//{
//	assertp( num_weights <= NUM_BLEND_ANIMS );
//
//	for( int i = 0; i < NUM_BLEND_ANIMS; ++i )
//	{
//		if( i < num_weights )
//		{
//			m_AnimWeights[i] = p_weights[i];
//		}
//		else
//		{
//			// Any weights not specified are assumed to be zero.
//			m_AnimWeights[i] = 0.0f;
//		}
//	}
//}
//
//
//
//void NsClump::forAllAtomics ( NsClump_Callback pCB, void * pData )
//{
//	NsAtomic * pAtomic = m_pAtomicHead;
//	while ( pAtomic ) {
//		if ( pCB ) pAtomic = pCB( pAtomic, pData );
//		if ( !pAtomic ) break;
//		pAtomic = pAtomic->m_pNext;
//	}
//}
//
//void NsClump::addAtomic ( NsAtomic * pAtomic )
//{
//	NsAtomic  * pSearchAtomic;
//
//	if ( m_pAtomicHead == NULL ) {
//		// First one, just add it.
//		m_pAtomicHead = pAtomic;
//	} else {
//		// Find the last atomic in the list (must do this to keep the order the same).
//		pSearchAtomic = m_pAtomicHead;
//		while ( pSearchAtomic->m_pNext ) pSearchAtomic = pSearchAtomic->m_pNext;
//		pSearchAtomic->m_pNext = pAtomic;
//	}
//	pAtomic->m_pNext = NULL;
//	m_numAtomic++;
//	pAtomic->m_pClump = this;
//}
//
//void NsClump::removeAtomic ( NsAtomic * pAtomicToRemove )
//{
//	NsAtomic *  pSearchAtomic;
//	NsAtomic ** ppLastAtomic;
//
//	pSearchAtomic = m_pAtomicHead;
//	ppLastAtomic = &m_pAtomicHead;
//	while ( pSearchAtomic ) {
//		// See if this is the one.
//		if ( pSearchAtomic == pAtomicToRemove ) {
//			// Link over this Atomic.
//			*ppLastAtomic = pSearchAtomic->m_pNext;
//			break;
//		}
//		// No match, point up the last Atomic pointer to pointer.
//		ppLastAtomic = &pSearchAtomic->m_pNext;
//		// Onto the next one.
//		pSearchAtomic = pSearchAtomic->m_pNext;
//	}
//	m_numAtomic--;
//}
//
//void NsClump::removeCASPolys( unsigned int flags )
//{
//	// Parse each atomic in turn.
//	NsAtomic * pAtomic = m_pAtomicHead;
//	while ( pAtomic ) {
//		if ( pAtomic->m_numCAS32 > 0 ) {
//			// Parse each material in turn.
//			for ( int lp = 0; lp < pAtomic->m_pMaterial->m_materialListSize; lp++ ) {
//				// Parse each DL in turn.
//				NsDL * pDL = pAtomic->m_pMaterial->retrieve( lp )->headDL();
//				while ( pDL ) {
//					unsigned char * p8;
//					unsigned char search[3];
//
//					p8 = (unsigned char *)&pDL[1];
//
//					search[0] = search[1] = search[2] = 0xff;
//					for ( unsigned int ss = 0; ss < pDL->m_size; ss++ ) {
//						search[0] = search[1];
//						search[1] = search[2];
//						search[2] = *p8++;
//
//						if (	( ( search[0] & 0xf8 ) == GX_DRAW_TRIANGLES ) &&
//								( ( ( search[1] << 8 ) | search[2] ) == (int)pDL->m_numIdx ) ) {
//							// Found it!!! Now, kill all polys with matching flags.
//							unsigned short * p16 = (unsigned short *)p8;
//							int	idxpervtx;
//
//							// Calculate how many indices per vertex.
//							idxpervtx = 1;	// Always have pos.
//						    if ( pDL->m_flags & rpWORLDPRELIT ) idxpervtx++;
//							if ( pDL->m_flags & rpWORLDNORMALS ) idxpervtx++;
//							if ( pDL->m_flags & rpWORLDUVS ) idxpervtx++;
//
//							// Kill all polys that have the specified bits set.
//							for ( int ii = 0; ii < ( pDL->m_numIdx / 3 ); ii++ ) {
//								if ( pAtomic->m_pCAS32->m_pCAS32[pDL->m_polyBase + ii] & flags ) {
//									for ( int vv = 0; vv < (idxpervtx * 3); vv++ ) {
//										p16[vv] = 0;
//									}
//								}
//								p16 += idxpervtx * 3;
//							}
//
////							OSReport( "Contents of DL: %02x %02x %02x %02x\n", p8[0], p8[1], p8[2], p8[3] );
//							break;
//						}
//					}
//					pDL = pDL->m_pNext;
//				}
//			}
//		}
//
//		pAtomic = pAtomic->m_pNext;
//	}
//
//
////	int lp;
////	NsMaterialMan * pMat = atomic->getMaterials();
////	for ( lp = 0; lp < pMat->m_numTotal; lp++ ) {
////		pMat->retrieve( lp )->headDL()->removePoly(0);
////	}
//
//
//
////	unsigned char * p8;
////	unsigned char * pEnd;
//
//	// We basically need to parse all display lists, and remove the polys with
//	// matching flags.
//
////	// Parse the display list.
////	p8 = (unsigned char *)&this[1];
////	pEnd = &p8[m_size];
////	while ( p8 < pEnd ) {
////		switch ( *p8 & 0xf8 ) {
////			case GX_NOP:
////				p8++;
////				break;
////			case GX_LOAD_BP_REG:
////				p8+=4;
////				break;
////			case GX_DRAW_QUADS:
////				break;
////			case GX_DRAW_TRIANGLES:
////				break;
////			case GX_DRAW_TRIANGLE_STRIP:
////				break;
////			case GX_DRAW_TRIANGLE_FAN:
////				break;
////			case GX_DRAW_LINES:
////				break;
////			case GX_DRAW_LINE_STRIP:
////				break;
////			case GX_DRAW_POINTS:
////				break;
////			default:
////				p8++;
////				break;
////		}
////	}
//}



















