//#include <math.h>
//#include "p_anim.h"
//#include "p_file.h"
//#include "p_assert.h"
//#include "p_matrix.h"
//
//#define rpSKINPOPPARENTMATRIX	 0x01
//#define rpSKINPUSHPARENTMATRIX	 0x02
//#define rpSKINANIMATEVERTEXGROUP 0x08
//
//NsAnim::NsAnim ( unsigned int numBones )
//{
//	m_q0				= new NsQFrame[numBones];
//	m_t0				= new NsTFrame[numBones];
//	m_q1				= new NsQFrame[numBones];
//	m_t1				= new NsTFrame[numBones];
//	m_qnow				= new NsQFrame[numBones];
//	m_tnow				= new NsTFrame[numBones];
////	m_pBoneMat			= new ROMtx[numBones];
//
//	m_pQ0				= NULL;
//	m_pT0				= NULL;
//	m_pQ1				= NULL;
//	m_pT1				= NULL;
//
//	m_pCurrentSequence	= NULL;
//
//	m_numCachedBones	= numBones;
//	m_duration			= 0.0f;
//	m_cachedTime		= -1.0f;
//	m_time				= 0.0f;
//}
//
//NsAnim::NsAnim ( NsAnim_Sequence * pAnim, unsigned int numBones )
//{
//	m_q0				= new NsQFrame[numBones];
//	m_t0				= new NsTFrame[numBones];
//	m_q1				= new NsQFrame[numBones];
//	m_t1				= new NsTFrame[numBones];
//	m_qnow				= new NsQFrame[numBones];
//	m_tnow				= new NsTFrame[numBones];
////	m_pBoneMat			= new ROMtx[numBones];
//
//	m_numCachedBones	= numBones;
//	m_cachedTime		= -1.0f;
//	m_time				= 0.0f;
//
//	setCurrentSequence( pAnim );
//}
//
//void NsAnim::setCurrentSequence ( NsAnim_Sequence * pSequence )
//{
//	if ( m_pCurrentSequence != pSequence ) {
//		m_pCurrentSequence = pSequence;
//		reset();
//	}
//}
//
//void NsAnim::reset ( void )
//{
//	if ( m_pCurrentSequence ) {
//		NsQFrame  * pQFrame;
//		NsTFrame  * pTFrame;
//		int			lp;
//
//		// Set duration.
//		m_duration = m_pCurrentSequence->duration;
//		m_cachedTime = 0.0f;
//
//		// Point to Q & T Frames.
//		pQFrame = (NsQFrame *)&m_pCurrentSequence[1];
//		pTFrame = (NsTFrame *)&pQFrame[m_pCurrentSequence->numQFrames];
//		// Fill in key 0.
//		for ( lp = 0; lp < m_numCachedBones; lp++ ) {
//			m_q0[lp].q.setX( -pQFrame->q.getX() );
//			m_q0[lp].q.setY( -pQFrame->q.getY() );
//			m_q0[lp].q.setZ( -pQFrame->q.getZ() );
//			m_q0[lp].q.setW(  pQFrame->q.getW() );
//			m_q0[lp].time =  pQFrame->time;
//
//			m_t0[lp].v.copy( pTFrame->v );
//			m_t0[lp].time = pTFrame->time;
//			pQFrame++;
//			pTFrame++;
//		}
//		m_pQ0 = pQFrame;
//		m_pT0 = pTFrame;
//		// Fill in key 1.
//		for ( lp = 0; lp < m_numCachedBones; lp++ ) {
//			m_q1[lp].q.setX( -pQFrame->q.getX() );
//			m_q1[lp].q.setY( -pQFrame->q.getY() );
//			m_q1[lp].q.setZ( -pQFrame->q.getZ() );
//			m_q1[lp].q.setW(  pQFrame->q.getW() );
//			m_q1[lp].time =  pQFrame->time;
//
//			m_t1[lp].v.copy( pTFrame->v );
//			m_t1[lp].time = pTFrame->time;
//			pQFrame++;
//			pTFrame++;
//		}
//		m_pQ1 = pQFrame;
//		m_pT1 = pTFrame;
//	}
//}
//
//NsAnim::~NsAnim()
//{
//	if ( m_q0 )			delete m_q0;
//	if ( m_t0 )			delete m_t0;
//	if ( m_q1 )			delete m_q1;
//	if ( m_t1 )			delete m_t1;
//	if ( m_qnow )		delete m_qnow;
//	if ( m_tnow )		delete m_tnow;
////	if ( m_pBoneMat )	delete m_pBoneMat;
//}
//
//NsAnim& NsAnim::clone ( void )
//{
//	NsAnim	  * pClonedAnim;
//
//	// Create a cloned anim.
//	pClonedAnim = new NsAnim( m_pCurrentSequence, m_numCachedBones );
//
//	return *pClonedAnim;
//}
//
//void NsAnim::update ( void )
//{
//	NsQFrame	  * pLastQFrame;
//	NsTFrame	  * pLastTFrame;
//	int				lp;
//	unsigned int	repeat;
//	float			lowesttime;
//
//	if ( !m_pCurrentSequence ) return;
//
//	assertp ( m_pQ0 );
//	assertp ( m_pQ1 );
//	assertp ( m_pT0 );
//	assertp ( m_pT1 );
//
//	// If the new time is less than the old time, re-init.
//	if ( m_cachedTime > m_time ) {
//		reset();
//	}
//	// If time is less than 0, reset and quit.
//	if ( m_time < 0.0f ) {
//		reset();
//		m_time = 0.0f;
//		m_cachedTime = m_time;
//		return;
//	}
//	// Cache the new time.
//	m_cachedTime = m_time;
//
////	// If the new time is past the end of the sequence, re-init.
////	if ( m_time > m_duration ) reset();
////	if ( m_time > m_duration ) m_time = m_duration;
////	// If the new time is before beginning of the sequence, re-init.
////	if ( m_time < 0.0f ) reset();
////	while ( m_time < 0.0f ) m_time += m_duration;
////	// Cache the new time.
////	m_cachedTime = m_time;
//
//	// Go through each key and see if we need a new quat.
//	// Do this on key1, and copy key1's old quat to key0 if a new key1 is required.
//	repeat = 1;
//
//	// Point to last Q & T Frames.
//	pLastQFrame = (NsQFrame *)&m_pCurrentSequence[1];
//	pLastQFrame = &pLastQFrame[m_pCurrentSequence->numQFrames];
//	pLastTFrame = (NsTFrame *)pLastQFrame;
//	pLastTFrame = &pLastTFrame[m_pCurrentSequence->numTFrames];
//	// Update quats.
//	while ( repeat ) {
//		repeat = 0;
//		// First, find the lowest q time.
//		lowesttime = m_time;
//		for ( lp = 0; lp < m_numCachedBones; lp++ ) {
//			if ( m_q1[lp].time <= lowesttime ) lowesttime = m_q1[lp].time;
//		}
//		// Now, update all keys with lowesttime.
//		for ( lp = 0; lp < m_numCachedBones; lp++ ) {
//			// See if we need a new Q Frame.
//			if ( !( m_q1[lp].time > lowesttime ) && ( m_pQ1 < pLastQFrame ) ) {
//				m_q0[lp].q.setX( m_q1[lp].q.getX() );
//				m_q0[lp].q.setY( m_q1[lp].q.getY() );
//				m_q0[lp].q.setZ( m_q1[lp].q.getZ() );
//				m_q0[lp].q.setW( m_q1[lp].q.getW() );
//				m_q0[lp].time = m_q1[lp].time;
//
//				m_q1[lp].q.setX( -m_pQ1->q.getX() );
//				m_q1[lp].q.setY( -m_pQ1->q.getY() );
//				m_q1[lp].q.setZ( -m_pQ1->q.getZ() );
//				m_q1[lp].q.setW(  m_pQ1->q.getW() );
//				m_q1[lp].time = m_pQ1->time;
//
//				m_pQ1++;
//				repeat = 1;		// Make this loop happen again as we may have passed more than 1 key.
//			}
//		}
//	}
//	// Update translations.
//	repeat = 1;
//	while ( repeat ) {
//		repeat = 0;
//		// Second, find the lowest t time.
//		lowesttime = m_time;
//		for ( lp = 0; lp < m_numCachedBones; lp++ ) {
//			if ( m_t1[lp].time <= lowesttime ) lowesttime = m_t1[lp].time;
//		}
//		// Now, update all keys with lowesttime.
//		for ( lp = 0; lp < m_numCachedBones; lp++ ) {
//			// See if we need a new T Frame.
//			if ( !( m_t1[lp].time > lowesttime ) && ( m_pT1 < pLastTFrame ) ) {
//				m_t0[lp].v.copy( m_t1[lp].v );
//				m_t0[lp].time = m_t1[lp].time;
//
//				m_t1[lp].v.copy( m_pT1->v );
//				m_t1[lp].time = m_pT1->time;
//
//				m_pT1++;
//				repeat = 1;		// Make this loop happen again as we may have passed more than 1 key.
//			}
//		}
//	}
//}
//
//
//
//
//
//void NsAnim::buildKeys( void )
//{
//	for( int i = 0; i < m_numCachedBones; ++i )
//	{
//		getQuat ( i, &m_qnow[i].q );
//		getTrans( i, &m_tnow[i].v.x, &m_tnow[i].v.y, &m_tnow[i].v.z );
//	}
//}
//
//
//
//
//
//
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				load															*
// *	Inputs:																		*
// *				pFilename	The filename to load								*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Loads an animation sequence (allocates memory), and assigns it	*
// *				to the current animation object. Also caches time=0.0f.			*
// *																				*
// ********************************************************************************/
//
//void NsAnim::load ( char * pFilename )
////static void LoadAnim ( void * pDest, int number )
//{
//	NsFile	f;
//
//	assertp ( pFilename );
//
//	// Load the animation sequence.
//	m_pCurrentSequence = (NsAnim_Sequence *)f.load ( pFilename );
//
//	// Cache at time=0.0f.
//	reset();
//}
//
//
//
//#if 0 // This functionality moved to NsClump.
//void NsAnim::flipBoneMatrices( ROMtx* p_matrices, unsigned int num_flip_pairs, unsigned int* p_flip_pairs )
//{
//	// Remember the incoming matrices have already been converted to fast-trasnformation format...
//	// To flip the anim, simply negate the x axis components of the 'at' and 'up' vectors, Tten rebuild
//	// the right vector. (Negating the x component of the right vector will lead to matrix inversion, which will flip textures etc.)
//	for( unsigned int i = 0;; i += 2 )
//	{
//		assertp(( i + 1 ) < ( num_flip_pairs * 2 ));
//		int matrix0 = p_flip_pairs[i];
//		if( matrix0 == -1 )
//		{
//			// Not interested.
//			continue;
//		}
//
//		int matrix1 = p_flip_pairs[i + 1];
//
//		if(( matrix0 == 0 ) && ( matrix1 == 0 ))
//		{
//			// Terminator.
//			break;
//		}
//
//		NsVector right, up, at, pos;
//
//		if( matrix1 == -1 )
//		{
//			// Just a simple rotate.
//			ROMtx* p_matrix0	= p_matrices + matrix0;
//
//			// Generate the right vector from up and at.
//			up.set( -p_matrix0[0][0][1], p_matrix0[0][1][1], p_matrix0[0][2][1] );
//			at.set( -p_matrix0[0][0][2], p_matrix0[0][1][2], p_matrix0[0][2][2] );
//			right.cross( up, at );
//
//			pos.set( -p_matrix0[0][3][0], p_matrix0[0][3][1], p_matrix0[0][3][2] );
//
//			p_matrix0[0][0][0]	= right.x;
//			p_matrix0[0][1][0]	= right.y;
//			p_matrix0[0][2][0]	= right.z;
//			p_matrix0[0][0][1]	= up.x;
//			p_matrix0[0][0][2]	= at.x;
//			p_matrix0[0][3][0]	= pos.x;
//		}
//		else
//		{
//			ROMtx* p_matrix0	= p_matrices + matrix0;
//			ROMtx* p_matrix1	= p_matrices + matrix1;
//
//			// Need to save matrix1 as it will now be written to.
//			ROMtx m1;
//			memcpy( &m1, p_matrix1, sizeof( ROMtx ));
//
//			// Generate the right vector from up and at.
//			up.set( -p_matrix0[0][0][1], p_matrix0[0][1][1], p_matrix0[0][2][1] );
//			at.set( -p_matrix0[0][0][2], p_matrix0[0][1][2], p_matrix0[0][2][2] );
//			right.cross( up, at );
//			pos.set( -p_matrix0[0][3][0], p_matrix0[0][3][1], p_matrix0[0][3][2] );
//
//			p_matrix1[0][0][0]	= right.x;
//			p_matrix1[0][1][0]	= right.y;
//			p_matrix1[0][2][0]	= right.z;
//			p_matrix1[0][0][1]	= up.x;
//			p_matrix1[0][1][1]	= up.y;
//			p_matrix1[0][2][1]	= up.z;
//			p_matrix1[0][0][2]	= at.x;
//			p_matrix1[0][1][2]	= at.y;
//			p_matrix1[0][2][2]	= at.z;
//			p_matrix1[0][3][0]	= pos.x;
//			p_matrix1[0][3][1]	= pos.y;
//			p_matrix1[0][3][2]	= pos.z;
//
//			// Generate the right vector from up and at.
//			up.set( -m1[0][1], m1[1][1], m1[2][1] );
//			at.set( -m1[0][2], m1[1][2], m1[2][2] );
//			right.cross( up, at );
//			pos.set( -m1[3][0], m1[3][1], m1[3][2] );
//
//			p_matrix0[0][0][0]	= right.x;
//			p_matrix0[0][1][0]	= right.y;
//			p_matrix0[0][2][0]	= right.z;
//			p_matrix0[0][0][1]	= up.x;
//			p_matrix0[0][1][1]	= up.y;
//			p_matrix0[0][2][1]	= up.z;
//			p_matrix0[0][0][2]	= at.x;
//			p_matrix0[0][1][2]	= at.y;
//			p_matrix0[0][2][2]	= at.z;
//			p_matrix0[0][3][0]	= pos.x;
//			p_matrix0[0][3][1]	= pos.y;
//			p_matrix0[0][3][2]	= pos.z;
//		}
//	}
//}
//#endif
//
//
//
//#if 0 // This functionality moved to NsClump.
//ROMtx * NsAnim::buildBoneMatrices ( NsAnim_BoneData * pBoneData )
//{
//	NsQuat		quat;
//    NsMatrix	bts;
//	int			lp;
//	int			currentMatrix;
//    NsMatrix	parentMatrix;
//    NsMatrix	matrixStack[32];
//    NsMatrix  * pMatrixStackTop = matrixStack;
//	float		tx, ty, tz;
//	NsMatrix	boneMatrix;
//	NsMatrix	tempMatrix;
//
//	// Point up what we need.
//	currentMatrix = -1;
//
//	// This is the root.
//    parentMatrix.identity();
//
//	for ( lp = 0; lp < m_numCachedBones; lp++ ) {
//		// Build bone to skin (bts) matrix.
//		bts.setRight( pBoneData[lp].rightx, pBoneData[lp].upx, pBoneData[lp].atx );
//		bts.setUp( pBoneData[lp].righty, pBoneData[lp].upy, pBoneData[lp].aty );
//		bts.setAt( pBoneData[lp].rightz, pBoneData[lp].upz, pBoneData[lp].atz );
//		bts.setPos( pBoneData[lp].posx, pBoneData[lp].posy, pBoneData[lp].posz );
//
//		// Push parent if required.
//		if ( pBoneData[lp].flags & rpSKINPUSHPARENTMATRIX ) {
//			memcpy ( pMatrixStackTop, &parentMatrix, sizeof ( NsMatrix ) );
//			pMatrixStackTop++;
//		}
//
//		getQuat ( lp, &quat );
//		getTrans( lp, &tx, &ty, &tz );
//		boneMatrix.fromQuat( &quat );
//		boneMatrix.setPos( tx, ty, tz );
///*
//		// Convert quaternion keyframe to matrix.
//		// Note: imaginary part is negated to be Watt-style...
//		quat0.x = -pQ[lp].q.x;
//		quat0.y = -pQ[lp].q.y;
//		quat0.z = -pQ[lp].q.z;
//		quat0.w =  pQ[lp].q.w;
//		quat1.x = -pQ[lp+numBones].q.x;
//		quat1.y = -pQ[lp+numBones].q.y;
//		quat1.z = -pQ[lp+numBones].q.z;
//		quat1.w =  pQ[lp+numBones].q.w;
//
//		quatSlerp ( &quat0, &quat1, time, &quat );
//
//		MTXQuat ( boneMatrix, &quat );
//
//		MTXRowCol(boneMatrix,0,3) = pT[lp].x;
//		MTXRowCol(boneMatrix,1,3) = pT[lp].y;
//		MTXRowCol(boneMatrix,2,3) = pT[lp].z;
//*/
//		// Multiply this matrix by the parent.
//		tempMatrix.cat( parentMatrix, boneMatrix );
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
//		// view * model.
////	    MTXConcat(v, objMat, pBoneMat[lp]);	// Note: pMatrixList[0] really objMat.
////		// (view * model) * bts.
////	    MTXConcat(pBoneMat[lp], bts, pBoneMat[lp]);
//
//		// Calculate light matrix.
////		MTXInverse(pBoneMat[lp], mvi);
////		MTXTranspose(mvi, ipBoneMat[lp]);
//
//		// Update parent (pop if required).
//		if ( pBoneData[lp].flags & rpSKINPOPPARENTMATRIX) {
//			pMatrixStackTop--;
//			memcpy ( &parentMatrix, pMatrixStackTop, sizeof ( NsMatrix ) );
//        } else {
//			memcpy ( &parentMatrix, &tempMatrix, sizeof ( NsMatrix ) );
//        }
//	}
//
//	return m_pBoneMat;
//}
//#endif
//
//
//
//void NsAnim::getQuat ( unsigned int bone, NsQuat * pQuat )
//{
//	float terp;
//	float time;
//
//	if ( m_time < 0.0f ) {
//		time = 0.0f;
//	} else if ( m_time > m_duration ) {
//		time = m_duration;
//	} else {
//		time = m_time;
//	}
//
//	// Normalize Q time.
//	terp = ( time - m_q0[bone].time ) / ( m_q1[bone].time - m_q0[bone].time );
//	// Interpolate quaternion.
//	pQuat->slerp( m_q0[bone].q, m_q1[bone].q, terp );
//
////	memcpy( (void *)pQuat, (void *)&q0[bone].q, sizeof ( NsQuat ) );
//}
//
//void NsAnim::getTrans ( unsigned int bone, float * pX, float * pY, float * pZ )
//{
//	float terp;
//	float time;
//
//	if ( m_time < 0.0f ) {
//		time = 0.0f;
//	} else if ( m_time > m_duration ) {
//		time = m_duration;
//	} else {
//		time = m_time;
//	}
//
//	// Normalize T time.
//	terp = ( time - m_t0[bone].time ) / ( m_t1[bone].time - m_t0[bone].time );
//	// Interpolate translation.
//	*pX = ( ( m_t1[bone].v.x - m_t0[bone].v.x ) * terp ) + m_t0[bone].v.x;
//	*pY = ( ( m_t1[bone].v.y - m_t0[bone].v.y ) * terp ) + m_t0[bone].v.y;
//	*pZ = ( ( m_t1[bone].v.z - m_t0[bone].v.z ) * terp ) + m_t0[bone].v.z;
//}















