///********************************************************************************
// *																				*
// *	Module:																		*
// *				MatMan															*
// *	Description:																*
// *				Material Manager.												*
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
//#include "core/defines.h"
//#include <engine/supersector.h>
//#include <engine/collide.h>
//#include <math.h>
//#include <charpipeline.h>
//#include <dolphin/perf.h>
//#include <charpipeline/GQRSetup.h>
//#include "p_material.h"
//#include "p_matman.h"
//#include "p_assert.h"
//#include "p_bbox.h"
//#include "p_scene.h"
//#include "p_profile.h"
//
///********************************************************************************
// * Defines.																		*
// ********************************************************************************/
//#define DEFAULT_MATMAN_SIZE 2048
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
//NsProfile CollisionTime( "Collision Time", 256 );
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
// *				MatMan															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Instances a material manager to the default table entry size of	*
// *				2048 entries.													*
// *																				*
// ********************************************************************************/
//NsMaterialMan::NsMaterialMan()
//{
//	NsMaterialMan ( DEFAULT_MATMAN_SIZE );
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				MatMan															*
// *	Inputs:																		*
// *				numEntries	How many entries the material manager should be.	*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Instances a material manager to a user-specified size.			*
// *																				*
// ********************************************************************************/
//NsMaterialMan::NsMaterialMan ( unsigned int numEntries )
//{
////	pMaterialList = (NsMaterial *)OSAlloc ( sizeof ( NsMaterial ) * numEntries );
////	pSortedMaterialList = (NsMaterial **)OSAlloc ( sizeof ( NsMaterial *) * numEntries );
//	m_pMaterialList = (NsMaterial *)new unsigned char[ sizeof ( NsMaterial ) * numEntries ];
//	m_pSortedMaterialList = (NsMaterial **)new unsigned char[ sizeof ( NsMaterial *) * numEntries ];
//
//	assert ( m_pMaterialList );
//	m_materialListSize = numEntries;
//
//	reset();
//
//	m_refCount = 0;
//
//	// Defaults.
//	for ( unsigned int lp = 0; lp < numEntries; lp++ ) {
//		m_pMaterialList[lp].m_wrap = NsTexture_Wrap_Repeat;
//	}
//}
//
//NsMaterialMan::~NsMaterialMan()
//{
//	int lp;
//	int count;
//
//	// Delete display lists from materials.
//	count = 0;
//	for ( lp = 0; lp < m_materialListSize; lp++ ) {
//		
//		// Remove pools from display lists.
//		NsDL * pDL = m_pSortedMaterialList[lp]->m_pDL;
//		NsDL * pKillDL;
//		while ( pDL ) {
//			pKillDL = pDL;
//			pDL = pDL->m_pNext;
//	
//	//		if ( pKillDL->m_pParent->m_pVertexPool ) {
//	//			delete pKillDL->m_pParent->m_pVertexPool;
//	//			pKillDL->m_pParent->m_pVertexPool = NULL;
//	//		}
//			if ( pKillDL->m_pParent->m_pFaceFlags ) {
//				delete pKillDL->m_pParent->m_pFaceFlags;
//				pKillDL->m_pParent->m_pFaceFlags = NULL;
//				count++;
//			}
//			if ( pKillDL->m_pParent->m_pFaceMaterial ) {
//				delete pKillDL->m_pParent->m_pFaceMaterial;
//				pKillDL->m_pParent->m_pFaceMaterial = NULL;
//				count++;
//			}
//			if ( pKillDL->m_pParent->m_pVertexPool && ( ( pKillDL->m_pParent->m_vpoolFlags & (1<<4) ) == 0 ) ) {
//				delete pKillDL->m_pParent->m_pVertexPool;
//				pKillDL->m_pParent->m_pVertexPool = NULL;
//				count++;
//			}
//			if ( pKillDL->m_pParent->m_pWibbleData ) {
//				delete pKillDL->m_pParent->m_pWibbleData;
//				pKillDL->m_pParent->m_pWibbleData = NULL;
//				count++;
//			}
//		}
//	}
//
//	// Delete display lists from materials.
//	count = 0;
//	for ( lp = 0; lp < m_materialListSize; lp++ ) {
//		// Remove all display lists.
//		NsDL * pDL = m_pSortedMaterialList[lp]->m_pDL;
//		NsDL * pKillDL;
//		while ( pDL ) {
//			pKillDL = pDL;
//			pDL = pDL->m_pNext;
//			delete pKillDL;
//			count++;
//		}
//		// Delete wibble data.
//		if ( m_pSortedMaterialList[lp] ) {
//			if ( m_pSortedMaterialList[lp]->m_pWibbleData ) {
//				for ( int kk = 0; kk < 8; kk++ ) {
//					if ( m_pSortedMaterialList[lp]->m_pWibbleData[kk].pKeys ) {
//						delete m_pSortedMaterialList[lp]->m_pWibbleData[kk].pKeys;
//					}
//				}
//				delete m_pSortedMaterialList[lp]->m_pWibbleData;
//			}
//		}
//	}
//	
//	// Delete display lists from materials.
//	for ( lp = 0; lp < m_materialListSize; lp++ ) {
//		if ( m_pSortedMaterialList[lp] ) m_pSortedMaterialList[lp]->deleteDLs();
//	}
//
//	// Delete materials and sorted material lists.
//	assertf( m_refCount == 0, ( "Tried to delete material manager when reference count was non-zero." ) );
//	if ( m_pMaterialList ) delete m_pMaterialList;
//	if ( m_pSortedMaterialList ) delete m_pSortedMaterialList;
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				retrieve														*
// *	Inputs:																		*
// *				number	The material number to retrieve.						*
// *	Output:																		*
// *				Pointer to the requested material. NULL if out of range.		*
// *	Description:																*
// *				Retrieve a material from the material manager.					*
// *																				*
// ********************************************************************************/
//NsMaterial * NsMaterialMan::retrieve ( unsigned int number )
//{
//	NsMaterial * rv;
//
//	if ( ( (int)number < m_materialListSize ) && ( number >= 0 ) ) {
//		rv = &m_pMaterialList[number];
//	} else {
//		rv = NULL;
//	}
//
//	return rv;
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				draw															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Draws all display lists attached to each material in this		*
// *				material manager object.										*
// *																				*
// ********************************************************************************/
//void NsMaterialMan::draw ( void )
//{
//	int lp;
//
//	for ( lp = 0; lp < m_materialListSize; lp++ ) {
//		if ( m_pSortedMaterialList[lp] ) m_pSortedMaterialList[lp]->draw();
////		pMaterialList[lp].draw();
//	}
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				cull															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Sets chunks of geometry that aren't visible to invisible.		*
// *																				*
// ********************************************************************************/
//unsigned int NsMaterialMan::cull ( NsMatrix * m )
//{
//	int				lp;
//	int				lp2;
//	NsMaterial	  * pMat;
//	NsDL		  * pDL;
//	unsigned int	code;
//	unsigned int	codeAND;
////	unsigned int	codeOR;
//	f32				rx[8], ry[8], rz[8];
//	f32				p[GX_PROJECTION_SZ];
//	f32				vp[GX_VIEWPORT_SZ];
//	u32				clip_x;
//	u32				clip_y;
//	u32				clip_w;
//	u32				clip_h;
//	float			clip_l;
//	float			clip_t;
//	float			clip_r;
//	float			clip_b;
//	MtxPtr			view;
//	float			minx, miny, minz;
//	float			maxx, maxy, maxz;
//	unsigned int	rv = 0;		// Invisible until we get a visible code.
//
//	GXGetProjectionv( p );
//	GXGetViewportv( vp );
//	GXGetScissor ( &clip_x, &clip_y, &clip_w, &clip_h );
//	clip_l = (float)clip_x;
//	clip_t = (float)clip_y;
//	clip_r = (float)(clip_x + clip_w);
//	clip_b = (float)(clip_y + clip_h);
//
//	view = m->m_matrix;
//
//	for ( lp = 0; lp < m_materialListSize; lp++ ) {
//		pMat = &m_pMaterialList[lp];
//		if ( pMat ) {
//			if ( !( pMat->m_flags & mINVISIBLE ) && pMat->m_nDL ) {
//				pDL = pMat->m_pDL;
//				do {
//					pDL->m_cull.visible = 1;
//					if ( pDL != pDL->m_pParent ) continue;
//					minx = pDL->m_cull.box.m_min.x;
//					miny = pDL->m_cull.box.m_min.y;
//					minz = pDL->m_cull.box.m_min.z;
//					maxx = pDL->m_cull.box.m_max.x;
//					maxy = pDL->m_cull.box.m_max.y;
//					maxz = pDL->m_cull.box.m_max.z;
//					GXProject ( minx, miny, minz, view, p, vp, &rx[0], &ry[0], &rz[0] );
//					GXProject ( minx, maxy, minz, view, p, vp, &rx[1], &ry[1], &rz[1] );
//					GXProject ( maxx, miny, minz, view, p, vp, &rx[2], &ry[2], &rz[2] );
//					GXProject ( maxx, maxy, minz, view, p, vp, &rx[3], &ry[3], &rz[3] );
//					GXProject ( minx, miny, maxz, view, p, vp, &rx[4], &ry[4], &rz[4] );
//					GXProject ( minx, maxy, maxz, view, p, vp, &rx[5], &ry[5], &rz[5] );
//					GXProject ( maxx, miny, maxz, view, p, vp, &rx[6], &ry[6], &rz[6] );
//					GXProject ( maxx, maxy, maxz, view, p, vp, &rx[7], &ry[7], &rz[7] );
//
//					// Generate clip code. {page 178, Procedural Elements for Computer Graphics}
//					// 1001|1000|1010
//					//     |    |
//					// ----+----+----
//					// 0001|0000|0010
//					//     |    |
//					// ----+----+----
//					// 0101|0100|0110
//					//     |    |
//					//
//					// Addition: Bit 4 is used for z behind.
//
//					codeAND	= 0x001f;
////					codeOR	= 0x0000;
//					for ( lp2 = 0; lp2 < 8; lp2++ ) {
//						// Only check x/y if z is valid (if z is invalid, the x/y values will be garbage).
//						if ( rz[lp2] > 1.0f   ) {
//							code = (1<<4);
//						} else {
//							code = 0;
//							if ( rx[lp2] < clip_l ) code |= (1<<0);
//							if ( rx[lp2] > clip_r ) code |= (1<<1);
//							if ( ry[lp2] > clip_b ) code |= (1<<2);
//							if ( ry[lp2] < clip_t ) code |= (1<<3);
//						}
//						codeAND	&= code;
////						codeOR	|= code;
//					}
//					pDL->m_cull.clipCodeAND = codeAND;
////					pDL->cull.clipCodeOR = codeOR;
//					// If any bits are set in the AND code, the object is invisible.
//					if ( codeAND ) {
//						pDL->m_cull.visible = 0;
//					} else {
//						rv = 1;
//					}
//				} while (( pDL = pDL->m_pNext) );
//			}
//		}
//	}
//	return rv;
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				calculateBoundingBox											*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Sets chunks of geometry that aren't visible to invisible.		*
// *																				*
// ********************************************************************************/
//void NsMaterialMan::calculateBoundingBox ( NsCull_Item * pCull )
//{
//	int				lp;
//	NsMaterial	  * pMat;
//	NsDL		  * pDL;
//	int				dirty = 1;
//
//	for ( lp = 0; lp < m_materialListSize; lp++ ) {
//		pMat = &m_pMaterialList[lp];
//		if ( pMat ) {
//			if ( pMat->m_nDL ) {
//				pDL = pMat->m_pDL;
//				do {
//					if ( dirty ) {
//						pCull->box.m_min.copy( pDL->m_cull.box.m_min );
//						pCull->box.m_max.copy( pDL->m_cull.box.m_max );
//						dirty = 0;
//					} else {
//						if ( pCull->box.m_min.x < pDL->m_cull.box.m_min.x ) pCull->box.m_min.x = pDL->m_cull.box.m_min.x;
//						if ( pCull->box.m_min.y < pDL->m_cull.box.m_min.y ) pCull->box.m_min.y = pDL->m_cull.box.m_min.y;
//						if ( pCull->box.m_min.z < pDL->m_cull.box.m_min.z ) pCull->box.m_min.z = pDL->m_cull.box.m_min.z;
//						if ( pCull->box.m_max.x > pDL->m_cull.box.m_max.x ) pCull->box.m_max.x = pDL->m_cull.box.m_min.x;
//						if ( pCull->box.m_max.y > pDL->m_cull.box.m_max.y ) pCull->box.m_max.y = pDL->m_cull.box.m_min.y;
//						if ( pCull->box.m_max.z > pDL->m_cull.box.m_max.z ) pCull->box.m_max.z = pDL->m_cull.box.m_min.z;
//					}
//				} while (( pDL = pDL->m_pNext) );
//			}
//		}
//	}
//}
//
//int NsMaterialMan::findCollision( NsLine * pLine, NsDL::Collision_LineCallback pCb, void * pData, void * pWorld )
//{
//	int				lp;
//	NsMaterial	  * pMat;
//	NsDL		  * pDL;
//	NsBBox			line;
//	int				rv = 0;
//
//	CollisionTime.start();
//	
//	line.m_min.x = pLine->start.x < pLine->end.x ? pLine->start.x : pLine->end.x;
//	line.m_min.y = pLine->start.y < pLine->end.y ? pLine->start.y : pLine->end.y;
//	line.m_min.z = pLine->start.z < pLine->end.z ? pLine->start.z : pLine->end.z;
//	line.m_max.x = pLine->start.x > pLine->end.x ? pLine->start.x : pLine->end.x;
//	line.m_max.y = pLine->start.y > pLine->end.y ? pLine->start.y : pLine->end.y;
//	line.m_max.z = pLine->start.z > pLine->end.z ? pLine->start.z : pLine->end.z;
//
//#	if 0
//	// Using the SuperSectors does seem to give a 15 - 20% speed increase.
//	SSec::Manager* p_ss_man	= (SSec::Manager*)((NsScene*)pWorld )->m_SSMan;
//	NsWorldSector**	pp_sectors = p_ss_man->GetIntersectingWorldSectors( (RwLine*)pLine );
//	for( ;; )
//	{
//		NsWorldSector* pDL = *pp_sectors++;
//		if( pDL == NULL )
//		{
//			// Finished.
//			break;
//		}
//
//		if( pDL->m_numPoly && !( pDL->m_pParent->m_rwflags & ( mSD_NON_COLLIDABLE | mSD_KILLED )))
//		{
//			if( Cld::BoxIntersectsBox( &pDL->m_cull.box, &line ))
//			{
//				rv |= pDL->findCollision( pLine, pCb, pData );
//			}
//		}
//	}
//#	else
//	for ( lp = 0; lp < m_materialListSize; lp++ ) {
//		pMat = &m_pMaterialList[lp];
//		if ( pMat ) {
//			if ( pMat->m_nDL ) {
//				pDL = pMat->m_pDL;
//				do {
//					if ( pDL->m_numPoly && !( pDL->m_pParent->m_rwflags & ( mSD_NON_COLLIDABLE | mSD_KILLED ) ) ) {
////		                if( Cld::BoxIntersectsBox( &pDL->m_cull.box, &line ) ) {
//							rv |= pDL->findCollision( pLine, pCb, pData );
////						}
//					}
//				} while (( pDL = pDL->m_pNext) );
//			}
//		}
//	}
//#	endif
//
//	CollisionTime.stop();
//
//	return rv;
//}
//
//
//
//void NsMaterialMan::reset ( void )
//{
//	int lp;
//
//	// Set material list up.
//	for ( lp = 0; lp < m_materialListSize; lp++ ) {
//		m_pMaterialList[lp].init ( lp );
//		m_pSortedMaterialList[lp] = NULL;
//	}
//}

