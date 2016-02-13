///********************************************************************************
// *																				*
// *	Module:																		*
// *				Render															*
// *	Description:																*
// *				Allows a rendering context to be opened and modified as desired	*
// *				by the user. A rendering context must be open before Prim or	*
// *				Model commands can be issued.									*
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
//#include "p_clump.h"
//#include "p_scene.h"
//#ifndef __BSP_UTILS_H__
//#include <sk/engine/bsputils.h>
//#endif
//#include <math.h>
//#include "p_model.h"
//#include "p_assert.h"
//
///********************************************************************************
// * Defines.																		*
// ********************************************************************************/
//#define rpWORLDUVS ( rpWORLDTEXTURED2 | rpWORLDTEXTURED )
//#define DL_BUILD_SIZE (32*1024)
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
//	mSD_INVISIBLE			= 0x0001,	// Invisible in primary viewport
//	mSD_NON_COLLIDABLE		= 0x0002,
//	mSD_KILLED				= 0x0004,
//	mSD_DONT_FOG			= 0x0008,
//	mSD_ALWAYS_FACE			= 0x0010,
//	mSD_NO_SKATER_SHADOW	= 0x0020,	// This is set at runtime for sectors with every face flagged mFD_SKATER_SHADOW.
//	mSD_INVISIBLE2			= 0x0040,	// Invisible in secondary viewport (Mick)
//};
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
///*---------------------------------------------------------------------------*
//    Name:           BuildMaterials
//    
//    Description:    Builds a material entry for each material.
// 
//    Arguments:      pModel	The Model data.
//    
//    Returns:        Head of sorted material list.
// 
// *---------------------------------------------------------------------------*/
//void NsModel::buildMaterials ( NsMaterialMan * pMaterialList, NsTextureMan * pTexMan )
//{
//	int				lp, it;
//	NsMaterialInfo* pMaterial;
////	char			buf[40];
//	NsTexture	  * pTex;
//	NsMaterial	  *	pBest;
//	NsMaterial	  * pML;
//
//	// Sanity.
//	assertf ( (int)m_numMaterials <= pMaterialList->m_materialListSize, ( "Material list too small." ) );
//
// 	// Point up stuff we're interested in.	
// 	pMaterial = (NsMaterialInfo *)&this[1];
//	pML = pMaterialList->m_pMaterialList;
//
//	// Add each material in turn.
//	for ( lp = 0; lp < (int)m_numMaterials; lp++ ) {
//		pTex = pTexMan->retrieve ( (const char *)&pMaterial->name );
//		pML->init ( lp );
//		pML->m_pTexture		= pTex;
//		pML->m_blendMode	= (NsBlendMode)pMaterial->blendmode;
//		pML->m_color		= pMaterial->color;
//		pML->m_alpha		= pMaterial->alpha;
//		pML->m_type			= pMaterial->type;
//		pML->m_flags		= pMaterial->flags;
//		pML->m_priority		= pMaterial->priority;
//		pML->m_pWibbleData	= NULL;
//		
//		NsMaterialInfo* pLastMaterial = pMaterial;
//		pMaterial++;
//
//		// Deal with wibble data if we have it.
//		pML->m_UVWibbleEnabled = pLastMaterial->uvwibble;
//		if ( pLastMaterial->uvwibble ) {
//			pML->setFlags( pML->getFlags() | mUV_WIBBLE_SUPPORT );
//
//			float * pf = (float *)pMaterial;
//			pML->m_uvel		= *pf++;
//			pML->m_vvel     = *pf++;
//			pML->m_uamp     = *pf++;
//			pML->m_vamp     = *pf++;
//			pML->m_uphase   = *pf++;
//			pML->m_vphase   = *pf++;
//			pML->m_ufreq    = *pf++;
//			pML->m_vfreq    = *pf++;
//			pMaterial = (NsMaterialInfo *)pf;
//		}
//		if ( pLastMaterial->vcwibble ) {
//			pML->setFlags( pML->getFlags() | mVC_WIBBLE_SUPPORT );
//
//			// Set up all sequences.
//			unsigned int * p32 = (unsigned int *)pMaterial;
//			pML->m_pWibbleData = new NsWibbleSequence[8];
//			for ( int kk = 0; kk < 8; kk++ ) {
//				pML->m_pWibbleData[kk].numKeys = *p32++;
//				pML->m_pWibbleData[kk].pKeys = NULL;
//				if ( pML->m_pWibbleData[kk].numKeys ) {
//					pML->m_pWibbleData[kk].pKeys = new NsWibbleKey[pML->m_pWibbleData[kk].numKeys];
//					memcpy( pML->m_pWibbleData[kk].pKeys, p32, sizeof( NsWibbleKey ) * pML->m_pWibbleData[kk].numKeys );
//					p32 += pML->m_pWibbleData[kk].numKeys * ( sizeof( NsWibbleKey ) / 4 );
//				}
//			}
//			pMaterial = (NsMaterialInfo *)p32;
//		}
//		pML++;
//	}
//
//	// Sort the materials. This is slow, but it works, and it only took me 15 minutes to write...
//	for ( lp = 0; lp < (int)m_numMaterials; lp++ ) {
//		pBest = &pMaterialList->m_pMaterialList[0];
//		for ( it = 0; it < (int)m_numMaterials; it++ ) {
//			if ( pMaterialList->m_pMaterialList[it].m_priority < pBest->m_priority ) pBest = &pMaterialList->m_pMaterialList[it];
//		}
//		pBest->m_priority |= 0x40000000;		// Force this not to be used again.
//		pMaterialList->m_pSortedMaterialList[lp] = pBest;
//	}
//	// Restore priority.
//	for ( lp = 0; lp < (int)m_numMaterials; lp++ ) {
//		pMaterialList->m_pMaterialList[lp].m_priority &= ~0x40000000;
//	}
////	// Set up the linked list.
////	pFirstMaterial = SortedMaterialList[0];
////	for ( lp = 0; lp < ( numMaterials - 1 ); lp++ ) {
////		SortedMaterialList[lp]->pNextSorted = SortedMaterialList[lp+1];
////	}																   
////		SortedMaterialList[numMaterials-1]->pNextSorted = NULL;
///*
//	// Set up the linked list.
//	pHead = &pMaterialList[0];
//	for ( lp = 0; lp < ( numMaterials - 1 ); lp++ ) {
//		pMaterialList[lp].pNextSorted = &pMaterialList[lp+1];
//	}
//	pMaterialList[numMaterials-1].pNextSorted = NULL;
//*/
////	OSFree ( SortedMaterialList );
//}
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
//void * NsModel::draw ( NsMaterialMan * pMaterialList, NsCollision * pCollision, int stripNormals, int addToHashTable )
//{
//    int				it;   // index of triangle
//	NsTexture	  * pTex;
////	unsigned int	numPoly;
//	unsigned int	material;
//	unsigned short	c0;	//, c1, c2;
//
//	NsGeometry	  * pGeom;
//	NsMaterialInfo* pMaterial;
//	float		  * pVertex;
//	float		  * pNormal;
//	unsigned int  * pColor;
//	unsigned int  * pPoly;
//	unsigned short* pConnect;
//	unsigned short* pFlags;
//	unsigned short* pFaceFlags;
//	unsigned short* pFaceMaterial;
//	float		  * pUV;
//	int				lp;
//	unsigned int	lp2;
//	unsigned int	geomflags;
//	int				mesh;
//
//	int				flags;
//	int				numMesh;
//	int				totalIdx;
//	int				numIdx;
//	int				numFlags;
//
//	NsMaterial	  * pMat;
//
//	NsDL		  * pDL = NULL;		// To fix warning
//	NsDL		  * pParentDL = NULL;
//	char		  * p8;
//	char		  * pPoolStart;
//	int				stride;
//
//	int				polybase;
//
//	// pModel[0] = int numMat
//	// pModel[1] = int numWorld
//	// pModel[2] = float invOrigin.x
//	// pModel[3] = float invOrigin.y
//	// pModel[4] = float invOrigin.z
//
// 	// Point up stuff we're interested in.	
// 	pMaterial = (NsMaterialInfo *)&this[1];
//
//	// Skip material data.
//	for ( lp = 0; lp < (int)m_numMaterials; lp++ ) {
//		NsMaterialInfo* pLastMaterial = pMaterial;
//		pMaterial++;
//		// Deal with wibble data if we have it.
//		if ( pLastMaterial->uvwibble ) {
//			unsigned int * p32 = (unsigned int *)pMaterial;
//			p32 += 8;
//			pMaterial = (NsMaterialInfo *)p32;
//		}
//		if ( pLastMaterial->vcwibble ) {
//			unsigned int * p32 = (unsigned int *)pMaterial;
//			for ( int kk = 0; kk < 8; kk++ ) {
//				unsigned int numWibbleKeys = *p32++;
//				p32 += numWibbleKeys * ( sizeof( NsWibbleKey ) / 4 );
//			}
//			pMaterial = (NsMaterialInfo *)p32;
//		}
//	}
//	pGeom = (NsGeometry *)pMaterial;
//
//	//	pTex = TexMan_Retrieve ( "SUB_AS_satellite_01.png" );
//	for ( lp = 0; lp < (int)m_numGeom; lp++ ) {
//		// Point to various pools we need.
//		pVertex = (float *)&pGeom[1];
//		m_frameNumber = pGeom->frameNumber;
//										
//		geomflags = pGeom->flags;
//
//		switch ( geomflags & ( rpWORLDPRELIT | rpWORLDTEXTURED | rpWORLDTEXTURED2 | rpWORLDNORMALS ) ) {
//			case rpWORLDPRELIT:
//				pColor	= (unsigned int *)&pVertex[pGeom->numVertices*3];
//				pUV		= NULL;
//				pNormal	= NULL;
//				pConnect= (unsigned short *)&pColor[pGeom->numVertices];
//				break;
//			case rpWORLDPRELIT |						rpWORLDNORMALS:
//				pColor	= (unsigned int *)&pVertex[pGeom->numVertices*3];
//				pUV		= NULL;
//				pNormal = (float *)&pColor[pGeom->numVertices];
//				pConnect= (unsigned short *)&pNormal[pGeom->numVertices*3];
//				break;
//			case rpWORLDPRELIT |	rpWORLDTEXTURED:
//			case rpWORLDPRELIT |	rpWORLDTEXTURED2:
//				pColor	= (unsigned int *)&pVertex[pGeom->numVertices*3];
//				pUV		= (float *)&pColor[pGeom->numVertices];
//				pNormal	= NULL;
//				pConnect= (unsigned short *)&pUV[pGeom->numVertices*2];
//				break;
//			case rpWORLDPRELIT |	rpWORLDTEXTURED |	rpWORLDNORMALS:
//			case rpWORLDPRELIT |	rpWORLDTEXTURED2 |	rpWORLDNORMALS:
//				pColor	= (unsigned int *)&pVertex[pGeom->numVertices*3];
//				pUV		= (float *)&pColor[pGeom->numVertices];
//				pNormal	= (float *)&pUV[pGeom->numVertices*2];
//				pConnect= (unsigned short *)&pNormal[pGeom->numVertices*3];
//				break;
//			case 0:
//				pColor	= NULL;
//				pUV		= NULL;
//				pNormal	= NULL;
//				pConnect= (unsigned short *)&pVertex[pGeom->numVertices*3];
//				break;
//			case										rpWORLDNORMALS:
//				pColor	= NULL;
//				pUV		= NULL;
//				pNormal	= (float *)&pVertex[pGeom->numVertices*3];
//				pConnect= (unsigned short *)&pNormal[pGeom->numVertices*3];
//				break;
//			case 					rpWORLDTEXTURED:
//			case 					rpWORLDTEXTURED2:
//				pColor	= NULL;
//				pUV		= (float *)&pVertex[pGeom->numVertices*3];
//				pNormal	= NULL;
//				pConnect= (unsigned short *)&pUV[pGeom->numVertices*2];
//				break;
//			case 					rpWORLDTEXTURED |	rpWORLDNORMALS:
//			case 					rpWORLDTEXTURED2 |	rpWORLDNORMALS:
//				pColor	= NULL;
//				pUV		= (float *)&pVertex[pGeom->numVertices*3];
//				pNormal	= (float *)&pUV[pGeom->numVertices*2];
//				pConnect= (unsigned short *)&pNormal[pGeom->numVertices*3];
//				break;
//			default:
//				pColor	= NULL;
//				pUV		= NULL;
//				pNormal	= NULL;
//				pConnect= (unsigned short *)&pVertex[pGeom->numVertices*3];
//				break;
//		}
//		// We always have these.
//		numFlags = pGeom->numPolygons + ( pGeom->numPolygons & 1 );
//		pFlags  = (unsigned short *)&pConnect[pGeom->numPolygons*4];
//		pPoly	= (unsigned int *)&pFlags[numFlags];
//
//		// Strip normals if requested.
//		if ( stripNormals ) {
//			if ( geomflags & rpWORLDNORMALS ) {
//				geomflags &= ~rpWORLDNORMALS;
//				pNormal	= NULL;
//			}
//		}
//
//		// Set stride.
//		if ( pNormal ) {
//			// We have vertices and normals, we have to adjust the stride.
//			stride = 6 * sizeof ( float );
//		} else {
//			// We only have vertices, set standard stride.
//			stride = 3 * sizeof ( float );
//		}
//
//		// If the header says no polys, don't do anything.
//		if ( pGeom->numPolygons ) {
//
//			// Re-calculate bounding box.
//			pGeom->infx = pVertex[0];
//			pGeom->infy = pVertex[1];
//			pGeom->infz = pVertex[2];
//			pGeom->supx = pVertex[0];
//			pGeom->supy = pVertex[1];
//			pGeom->supz = pVertex[2];
//			for ( unsigned int i = 0; i < pGeom->numVertices; i++ )
//			{
//				if ( pVertex[(i*3)+0] < pGeom->infx ) pGeom->infx = pVertex[(i*3)+0];
//				if ( pVertex[(i*3)+1] < pGeom->infy ) pGeom->infy = pVertex[(i*3)+1];
//				if ( pVertex[(i*3)+2] < pGeom->infz ) pGeom->infz = pVertex[(i*3)+2];
//				if ( pVertex[(i*3)+0] > pGeom->supx ) pGeom->supx = pVertex[(i*3)+0];
//				if ( pVertex[(i*3)+1] > pGeom->supy ) pGeom->supy = pVertex[(i*3)+1];
//				if ( pVertex[(i*3)+2] > pGeom->supz ) pGeom->supz = pVertex[(i*3)+2];
//			}
//
//			// Copy face flags.
//			pFaceFlags = new unsigned short[pGeom->numPolygons];
//			memcpy( pFaceFlags, pFlags, pGeom->numPolygons * sizeof ( unsigned short ) );
//	
//			int size = 0;
//			if ( pVertex ) {
//				size += ( pGeom->numVertices + 1 ) * sizeof ( float ) * 3;
//			}
//			if ( pNormal ) {
//				size += ( pGeom->numVertices + 1 ) * sizeof ( float ) * 3;
//			}
//			if ( pColor ) {
//				size += sizeof ( unsigned int ) * pGeom->numVertices;
//			}
//			if ( pUV ) {
//				size += sizeof ( float ) * 2 * pGeom->numVertices;
//			}		
//			size += pGeom->numPolygons * sizeof ( unsigned short ) * 3;
//
//			p8 = new char[size];
//			pPoolStart = p8;
//
//			// Positions and normals should be doubled up (so that the
//			// animation system can directly set in this format).
//			// PJR - Note: +1 for blend error.
//			for ( lp2 = 0; lp2 < pGeom->numVertices; lp2++ ) {
//				if ( pVertex ) {
//					memcpy ( p8, &pVertex[lp2*3], sizeof ( float ) * 3 );
//					p8 += sizeof ( float ) * 3;
//				}
//
//				if ( pNormal ) {
//					memcpy ( p8, &pNormal[lp2*3], sizeof ( float ) * 3 );
//					p8 += sizeof ( float ) * 3;
//				}
//			}
//
//			if ( pVertex ) {
//				pVertex = (float *)pPoolStart;
//				p8 += sizeof ( float ) * 3;
//			}
//
//			if ( pNormal ) {
//				pNormal = (float *)pPoolStart;
//				p8 += sizeof ( float ) * 3;
//			}
//
//			// Copy the colors and UVs (not doubled up).
//			if ( pColor ) {
//				memcpy ( p8, pColor, sizeof ( unsigned int ) * pGeom->numVertices );
//				pColor = (unsigned int *)p8;
//				p8 += sizeof ( unsigned int ) * pGeom->numVertices;
//			}
//
//			if ( pUV ) {
//				memcpy ( p8, pUV, sizeof ( float ) * 2 * pGeom->numVertices );
//				pUV = (float *)p8;
//				p8 += sizeof ( float ) * 2 * pGeom->numVertices;
//			}
//
//			// Copy connect information.
//			for ( lp2 = 0; lp2 < pGeom->numPolygons; lp2++ ) {
//				memcpy ( p8, &pConnect[(lp2*4)+1], sizeof ( unsigned short ) * 3 );
//				p8 += sizeof ( unsigned short ) * 3;
//			}
//
//			m_numVertex = pGeom->numVertices;
//			m_pVertexPool = (NsVector *)pVertex;
//			m_flags  = pVertex ? (1<<0) : 0;
//			m_flags |= pNormal ? (1<<1) : 0;
//			m_flags |= pColor  ? (1<<2) : 0;
//			m_flags |= pUV     ? (1<<3) : 0;
//
//			flags = pPoly[0];
//			numMesh = pPoly[1];
//			totalIdx = pPoly[2];
//			pPoly += 3;
//			polybase = 0;
//
//			// Setup face material list.
//			pFaceMaterial = new unsigned short[pGeom->numPolygons];
//		    for ( it = 0 ; it < (int)pGeom->numPolygons; ++it ) {
//				pFaceMaterial[it] = pConnect[(it*4)+0];
//			}
//
//			// Draw each chunk of polygons.
//			for ( mesh = 0; mesh < numMesh; mesh++ ) {
//				// Get number of polys and advance to poly list.
//	/*			numPoly = pPoly[0];
//				material = pPoly[1];
//			    pPoly += 2;*/
//
//				numIdx = pPoly[0];
//				material = pPoly[1];
//				pPoly += 2;
//
//				pMat = pMaterialList->retrieve ( material );
//				assertf ( material < m_numMaterials, ( "Material too high: %d - max: %d\n", material, m_numMaterials ) );
//
//				// Clear the decal flag if not static environment.
//				if( !addToHashTable )
//				{
//					pMat->setFlags( pMat->getFlags() & ~mDECAL );
//				}
//
//			    // Only attempt to upload/draw if we have polygons.
//	//			if ( numPoly ) {
//				if ( numIdx ) {
//			
//					// Setup display list header & open display list context.
//					Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
//					pDL = (NsDL *)new char[DL_BUILD_SIZE];
//					DCFlushRange ( &pDL[1], DL_BUILD_SIZE );
//					Mem::Manager::sHandle().PopContext();
//	
//					GXBeginDisplayList ( &pDL[1], DL_BUILD_SIZE );
//					GXResetWriteGatherPipe();
//
//					// Set cull item.
//					pDL->m_cull.box.m_min.x = pGeom->infx;
//					pDL->m_cull.box.m_min.y = pGeom->infy;
//					pDL->m_cull.box.m_min.z = pGeom->infz;
//					pDL->m_cull.box.m_max.x = pGeom->supx;
//					pDL->m_cull.box.m_max.y = pGeom->supy;
//					pDL->m_cull.box.m_max.z = pGeom->supz;
//					pDL->m_cull.visible = 1;
//
//					pDL->m_id = pGeom->checksum;		// Checksum.
////					pDL->version = 0;
////					pDL->cullIdx = lp;
//					pDL->m_flags = geomflags;
//					pDL->m_rwflags = pGeom->rflags;
//					pDL->m_numIdx = numIdx;
//					pDL->m_polyBase = polybase;
//					pDL->m_pVertexPool = (NsVector *)pVertex;
//					pDL->m_numVertex = pGeom->numVertices;
//					pDL->m_numPoly = pGeom->numPolygons;
//					pDL->m_vpoolFlags = m_flags;
//					pDL->m_operationID = 0;
//
//				    // Set format of position, color and tex coordinates.
//				    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
//				    if ( geomflags & rpWORLDPRELIT ) GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
//					if ( geomflags & rpWORLDUVS ) GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
//					if ( geomflags & rpWORLDNORMALS ) GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
//
//					// Set collision.
////					if ( m_numGeom > 1 ) {
////						pDL->m_pCollision = new NsCollision( pGeom->numPolygons );
////					    for ( it = 0 ; it < (int)pGeom->numPolygons; ++it ) {
////							c0 = pConnect[(it*4)+1]*(stride/4);
////							c1 = pConnect[(it*4)+2]*(stride/4);
////							c2 = pConnect[(it*4)+3]*(stride/4);
////							pDL->m_pCollision->addTriangle(
////								pVertex[c0+0], pVertex[c0+1], pVertex[c0+2],
////								pVertex[c1+0], pVertex[c1+1], pVertex[c1+2],
////								pVertex[c2+0], pVertex[c2+1], pVertex[c2+2] );
////						}
////					}
//					// Face flags.
//					pDL->m_pFaceFlags = pFaceFlags;
//
//					// Material Indices.
//					pDL->m_pFaceMaterial = pFaceMaterial;
//
//				    // Sets up vertex descriptors
//				    GXClearVtxDesc();
//				    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
//					if ( geomflags & rpWORLDPRELIT ) GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
//					if ( geomflags & rpWORLDNORMALS ) GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
//					if ( geomflags & rpWORLDUVS ) GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
//					//GXSetVtxDesc(GX_VA_NRM, GX_INDEX8);
//
//					// Point the hardware at these pools.
//					if ( m_numGeom > 1 ) {
//					    GXSetArray(GX_VA_POS, pVertex, stride);
//					    if ( geomflags & rpWORLDNORMALS ) GXSetArray(GX_VA_NRM, pNormal, stride);
//					}
//				    if ( geomflags & rpWORLDPRELIT ) GXSetArray(GX_VA_CLR0, pColor, 4*sizeof(u8));
//					
//					// If this texture exists, and it isn't already uploaded, upload it.
//					pTex = pMat->m_pTexture;
////					pTex = TexMan_Retrieve ( (char *)&pMaterial[material*64] );
//					//if ( !pTex ) pTex = TexMan_Retrieve ( "" );
//
//					if ( geomflags & rpWORLDUVS ) GXSetArray(GX_VA_TEX0, pUV, 8);
//
//					switch ( flags ) {
//						case 1:		// Tri-strip
//							// Open drawing context.
//							//GXSetCullMode ( GX_CULL_NONE );
//						    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, (unsigned short)numIdx );
//							pDL->m_vpoolFlags |= (1<<5);
//							break;
//						default:
//						case 0:		// Regular
//							// Open drawing context.
//							//GXSetCullMode ( GX_CULL_BACK );
//						    GXBegin(GX_TRIANGLES, GX_VTXFMT0, (unsigned short)numIdx );
//							polybase += numIdx / 3;		// Note: can only calculate this for trilists. Will be 0 for tristrips.
//							break;
//					}
//
//					unsigned short vertMin	= 0xFFFFU;
//					unsigned short vertMax	= 0x0000U;
//				    for ( it = 0 ; it < numIdx; ++it ) {
//						c0 = (unsigned short )*pPoly;
//				    	GXPosition1x16(c0);
//						if ( geomflags & rpWORLDNORMALS ) GXNormal1x16(c0);
//					    if ( geomflags & rpWORLDPRELIT ) GXColor1x16(c0);
//						if ( geomflags & rpWORLDUVS ) GXTexCoord1x16(c0);
//						if ( c0 < vertMin ) {
//							vertMin = c0;
//						}
//						if ( c0 > vertMax ) {
//							vertMax = c0;
//						}
//						pPoly++;
//					}
//
//				    // Close drawing context.
//				    GXEnd();
//
//					// Set vertex ranges.
//					pDL->m_vertBase		= vertMin;
//					pDL->m_vertRange	= ( vertMax - vertMin ) + 1;
//
//					// Close display list context, assign it to the correct material & advance to next one.
//					pDL->m_size = GXEndDisplayList();
//					assert( pDL->m_size );
//
//					Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
////					pDL = (NsDL *)Mem::Realloc( pDL, pDL->m_size + sizeof( NsDL ) );
//					NsDL * pOldDL = pDL;
//					pDL = (NsDL *)new char[pOldDL->m_size + sizeof( NsDL )];
//					memcpy ( pDL, pOldDL, pOldDL->m_size + sizeof( NsDL ) );
//					delete pOldDL;
//					Mem::Manager::sHandle().PopContext();
//					DCFlushRange ( &pDL[1], pDL->m_size );
//
//					pMat->addDL ( pDL );
//					// If this is the first mesh in this sector, add it to the
//					// hash table. If not, link it to the head DL.
//					if ( mesh == 0 ) {
//						// Hook into skate3...
//						//if ( addToHashTable ) Bsp::AddToHashTable( pDL, NULL );
//						pDL->m_pParent = pDL;
////						pDL->m_pChild = NULL;
//						pParentDL = pDL;
//					} else {
//						// Hook this to the parent DL.
//						pDL->m_pParent = pParentDL;
//						// Set this DL's child to be the parent's child.
////						pDL->m_pChild = pParentDL->m_pChild;
//						// Set the parent's child to be this dl.
////						pParentDL->m_pChild = pDL;
//					}
//					// Point up the material manager.
//					pDL->m_pMaterialMan = pMaterialList;
//			    } else {
//					// Skip.
//					pPoly += numIdx;
//				}
//		    }
//		}
//
//		// Pull skin plugin data out.
//		NsBBox			  * pSkinBox;
//		unsigned short	  * pCAS16;
//		unsigned int	  * pFlipPairs;
//		unsigned int	  * pCAS32;
//		unsigned int	  * pPartChecksums;
//
//		m_pSkinData = (NsSkinData *)pPoly;
//		pSkinBox = (NsBBox *)&m_pSkinData[1];
//		pCAS16 = (unsigned short *)&pSkinBox[m_pSkinData->numSkinBoxes];
//		pFlipPairs = (unsigned int *)&pCAS16[m_pSkinData->numCAS16+(m_pSkinData->numCAS16&1)];
//		pCAS32 = (unsigned int *)&pFlipPairs[m_pSkinData->numFlipPairs * 2];
//		pPartChecksums = (unsigned int *)&pCAS32[m_pSkinData->numCAS32];
//
//		// Copy wibble data.
//		unsigned char * pWibble = (unsigned char *)&pPartChecksums[m_pSkinData->numPartChecksums];
//		pParentDL->m_pWibbleData = NULL;
//		if ( pGeom->wibbleDataFollows ) {
//			pParentDL->m_pWibbleData = new unsigned char[pParentDL->m_numVertex * 6];
//			memcpy( pParentDL->m_pWibbleData, pParentDL->getColorPool(), pParentDL->m_numVertex * 4 );
//			memcpy( &pParentDL->m_pWibbleData[pParentDL->m_numVertex * 4], pWibble, pParentDL->m_numVertex * 2 );
//			pWibble += ( pParentDL->m_numVertex * 2 ) + ( ( pParentDL->m_numVertex & 1 ) * 2 );
//		}
//		
//		// Skip bsp tree data.
//		NsBranch	  * pBranches;
//		NsLeaf		  * pLeaves;
//		unsigned int  * pTriangles;
//
//		pBranches = (NsBranch *)pWibble;
//		pLeaves = (NsLeaf *)&pBranches[m_pSkinData->numLeaf ? m_pSkinData->numLeaf-1 : 0];
//		pTriangles = (unsigned int *)&pLeaves[m_pSkinData->numLeaf];
//
//	    // Onto next geometry chunk.
//		pGeom = (NsGeometry *)&pTriangles[m_pSkinData->numTri];
////		pGeom = (NsGeometry *)pPoly;
//
//		// Add BSP tree.
//		if ( pCollision && ( m_pSkinData->numLeaf > 0 ) && ( m_pSkinData->numTri > 0 ) ) {
//			pCollision->addTree( pBranches, m_pSkinData->numLeaf, m_pSkinData->numTri, pParentDL );
//		}
//	}
//
//	// Set extra data pointer up.
//	m_pExtraData = (unsigned int *)pGeom;
//
//	return m_pExtraData;
//}






