///********************************************************************************
// *																				*
// *	Module:																		*
// *				NsScene															*
// *	Description:																*
// *				Holds scene data & draws it.									*
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
//#include "p_scene.h"
//#include "p_file.h"
//#include "p_prim.h"
//#include "p_light.h"
//#include "gfx\nxflags.h"
//#ifndef __GFX_MODEL_H
//#include <gfx/model.h>
//#endif
//#ifndef	__SYS_MEM_MEMMAN_H
//#include <sys/mem/memman.h>
//#endif
//#include "p_profile.h"
//
//
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
//NsProfile ClumpTime( "Clump Time", 256 );
//NsProfile BSPTime( "BSP Time", 256 );
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
// *				NsScene															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Declares a scene object.										*
// *																				*
// ********************************************************************************/
//
//NsScene::NsScene()
//{
//	m_view.identity();
//
//	m_pMaterial		= NULL;
//	m_pCameraData	= NULL;
//	m_pClumpHead	= NULL;
//	m_SSMan			= NULL;
//	m_pTexMan		= NULL;
//	m_numParticles	= 0;
//	m_pParticleData	= NULL;
//	m_pCollision	= new NsCollision;
//}
//
//NsScene::~NsScene()
//{
//	if ( m_pMaterial ) delete m_pMaterial;
//	if ( m_pCameraData ) delete m_pCameraData;
////	if ( m_pTexMan ) delete m_pTexMan;
//	if ( m_pCollision ) delete m_pCollision;
//	if ( m_pParticleData ) delete m_pParticleData;
//
//	NsClump * pClump = m_pClumpHead;
//	while ( pClump ) {
//		NsClump * pDelete = pClump;
//		pClump = pClump->m_pNext;
//		delete pDelete;
//	}
//}
//
//void NsScene::loadBSP( const char * pFilename )
//{
//	NsFile			f;
//	NsModel		  * pModel;
//
//	// Load model data.
//	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
//	pModel	= (NsModel *)f.load ( pFilename );
//	Mem::Manager::sHandle().PopContext();
//
//	// Build materials and display lists.
//	m_pMaterial = new NsMaterialMan( pModel->m_numMaterials );
//	if ( pModel ) pModel->buildMaterials ( m_pMaterial, m_pTexMan );
//	if ( pModel ) pModel->draw ( m_pMaterial, m_pCollision, 1, 1 );
//
//	// Copy particle data.
//	unsigned int * p32 = pModel->m_pExtraData;
//	m_numParticles = *p32++;
//	if ( m_numParticles ) {
//		m_pParticleData = new NsParticle[m_numParticles];
//		memcpy ( m_pParticleData, p32, sizeof( NsParticle ) * m_numParticles );
//		p32 += (sizeof( NsParticle ) * m_numParticles) / 4;
//	} else {
//		m_pParticleData = NULL;
//	}
//
//	// Copy camera data.
//	unsigned int * pCameraData = p32;
//	int numCamera = p32[0];
//	p32 = &p32[(numCamera*3)+1];
//	int numCameraPackets = p32[0];
//	p32 = &p32[(numCameraPackets*16)+1];
//	int	size = ((int)p32) - ((int)pCameraData);
//	m_pCameraData = new unsigned int[size / 4];
//	memcpy ( m_pCameraData, pCameraData, size );
//
//	// Generate world bounding box information.
//	m_pMaterial->calculateBoundingBox ( &m_cull );
//
//	// Delete the original, we're done with it.
//	delete pModel;
//}
//
//void NsScene::draw( NsCamera * camera )
//{
//	NsClump * pClump = m_pClumpHead;
//
//	NsVector _v( 0.5, 0.5, 0.5 );
//	camera->getViewMatrix()->scale( &_v, NsMatrix_Combine_Post );
//
//
//	// Render the clumps attached to this world.
//	ClumpTime.start();
//	while ( pClump ) {
//
//		// If the clump user data field is set, this indicates an owner Gfx::Model, which
//		// expects it's render callback to be called at this point, for tasks such as setting
//		// up lighting etc.
//		void* p_user_data = pClump->getUserData();
//		if( p_user_data )
//		{
//			if((((Gfx::Model*)p_user_data )->m_model_flags & MODELFLAG_HIDE ) == 0 )
//			{
//				// Dirty dirty dirty.
////				((Gfx::Model*)p_user_data )->RenderCallback();
//
//				// Upload any lighting info that has changed.
//				NsLight::loadup();
//
//				pClump->draw( camera, 1 );
//
////				((Gfx::Model*)p_user_data )->PostRenderCallback();
//			}
//		}
//		else
//		{
//			pClump->draw( camera, 1 );
//		}
//		pClump = pClump->m_pNext;
//	}
//	ClumpTime.stop();
//
//	// Render the BSP.
//	BSPTime.start();
//	camera->begin();
//	m_pMaterial->cull( camera->getCurrent() );
//	m_pMaterial->draw();
//	camera->end();
//	BSPTime.stop();
//}
//
//void NsScene::addClump( NsClump * pClumpToAdd )
//{
//	pClumpToAdd->m_pNext = m_pClumpHead;
//	m_pClumpHead = pClumpToAdd;
//
//	pClumpToAdd->m_pWorld = this;
//}
//
//void NsScene::removeClump( NsClump * pClumpToRemove )
//{
//	NsClump *  pSearchClump;
//	NsClump ** ppLastClump;
//
//	pSearchClump = m_pClumpHead;
//	ppLastClump = &m_pClumpHead;
//	while ( pSearchClump ) {
//		// See if this is the one.
//		if ( pSearchClump == pClumpToRemove ) {
//			// Link over this clump.
//			*ppLastClump = pSearchClump->m_pNext;
//			// Set world to NULL.
//			pSearchClump->m_pWorld = NULL;
//			break;
//		}
//		// No match, point up the last clump pointer to pointer.
//		ppLastClump = &pSearchClump->m_pNext;
//		// Onto the next one.
//		pSearchClump = pSearchClump->m_pNext;
//	}
//}
//
//void NsScene::forAllClumps ( NsScene_Callback pCB, void * pData )
//{
//	NsClump * pClump = m_pClumpHead;
//	while ( pClump ) {
//		if ( pCB ) pCB( pClump, pData );
//		pClump = pClump->m_pNext;
//	}
//}
//
////void NsScene::forAllAtomics ( NsClump_Callback pCB, void * pData )
////{
////	NsClump * pClump = m_pClumpHead;
////	while ( pClump ) {
////		pClump->forAllAtomics( pCB, pData );
////		pClump = pClump->m_pNext;
////	}
////}
//
//int NsScene::findCollision( NsLine * pLine, NsDL::Collision_LineCallback pCb, void * pData )
//{
//	return m_pCollision->findCollision( pLine, pCb, pData );
//}






