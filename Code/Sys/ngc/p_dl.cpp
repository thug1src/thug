///********************************************************************************
// *																				*
// *	Module:																		*
// *				DL																*
// *	Description:																*
// *				Object container for a pre-built display list.					*
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
//#include "p_dl.h"
//#include "p_prim.h"
//#include "p_render.h"
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
// *				upload															*
// *	Inputs:																		*
// *				pTex	Pointer to the texture to upload.						*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Uploads the specified texture.									*
// *																				*
// ********************************************************************************/
//
////int	NsCollision::findCollision	( NsSphere * pSphere, NsCollision_SphereCallback pCb, void * pData );
//int	NsDL::findCollision( NsLine * pLine, NsDL::Collision_LineCallback pCb, void * pData )
//
//{
//	unsigned int			lp;
//	int						result;
//	int						rv = 0;
//	NsTriangle				ct;
//	float					distance;
//	NsCollisionTriangle		tri;
//	NsBBox					line;
//	unsigned short		  * pConnect;
//
//	line.m_min.x = pLine->start.x < pLine->end.x ? pLine->start.x : pLine->end.x;
//	line.m_min.y = pLine->start.y < pLine->end.y ? pLine->start.y : pLine->end.y;
//	line.m_min.z = pLine->start.z < pLine->end.z ? pLine->start.z : pLine->end.z;
//	line.m_max.x = pLine->start.x > pLine->end.x ? pLine->start.x : pLine->end.x;
//	line.m_max.y = pLine->start.y > pLine->end.y ? pLine->start.y : pLine->end.y;
//	line.m_max.z = pLine->start.z > pLine->end.z ? pLine->start.z : pLine->end.z;
//
//	pConnect = getConnectList();
//	// Calculate the intersection on each triangle.
//	for ( lp = 0; lp < m_numPoly; lp++ ) {
//		// Build the collision triangle.
//		tri.vertices[0] = getVertex( *pConnect++ );
//		tri.vertices[1] = getVertex( *pConnect++ );
//		tri.vertices[2] = getVertex( *pConnect++ );
//		*ct.corner( 0 ) = *tri.vertices[0];
//		*ct.corner( 1 ) = *tri.vertices[1];
//		*ct.corner( 2 ) = *tri.vertices[2];
//
//		// First, see if bounding box passes.
//		if (	( ct.corner(0)->x > line.m_max.x ) &&
//				( ct.corner(1)->x > line.m_max.x ) &&
//				( ct.corner(2)->x > line.m_max.x ) ) continue;
//		if (	( ct.corner(0)->x < line.m_min.x ) &&
//				( ct.corner(1)->x < line.m_min.x ) &&
//				( ct.corner(2)->x < line.m_min.x ) ) continue;
//		if (	( ct.corner(0)->y > line.m_max.y ) &&
//				( ct.corner(1)->y > line.m_max.y ) &&
//				( ct.corner(2)->y > line.m_max.y ) ) continue;
//		if (	( ct.corner(0)->y < line.m_min.y ) &&
//				( ct.corner(1)->y < line.m_min.y ) &&
//				( ct.corner(2)->y < line.m_min.y ) ) continue;
//		if (	( ct.corner(0)->z > line.m_max.z ) &&
//				( ct.corner(1)->z > line.m_max.z ) &&
//				( ct.corner(2)->z > line.m_max.z ) ) continue;
//		if (	( ct.corner(0)->z < line.m_min.z ) &&
//				( ct.corner(1)->z < line.m_min.z ) &&
//				( ct.corner(2)->z < line.m_min.z ) ) continue;
//
//		// Now, do triangle test.
//		result = pLine->intersectTriangle( &distance, &ct );
//		// If triangle intersection hapenned, deal with it.
//		if ( result ) {
//			rv = 1;
//			// Create triangle data.
//			NsVector l10;
//			NsVector l20;
//			l10.sub( *ct.corner(1), *ct.corner(0) );
//			l20.sub( *ct.corner(2), *ct.corner(0) );
//			tri.normal.cross( l10, l20 );
//			tri.normal.normalize();
//
////			tri.normal.set( 0.0f, 1.0f, 0.0f );
//			tri.point.set( tri.vertices[0]->x, tri.vertices[0]->y, tri.vertices[0]->z );
//			tri.index		= lp;
//			// Execute callback.
//			if ( pCb ) pCb( pLine, *this, &tri, distance, pData );
//
//
////RpCollisionTriangle *world_collision_cb( RpIntersection *is, 
////                 NsWorldSector *sector,
////                 RpCollisionTriangle *collPlane,
////                 RwReal &distance,
////                 void *data )
//
//
//
//
//
//
//
////			if ( NsPrim::begun() ) {
////				NsVector	r;
////				// Calculate intersection point.
////				r.sub( *&pLine->end, *&pLine->start );
////				r.x = ( r.x * distance ) + pLine->start.x;
////				r.y = ( r.y * distance ) + pLine->start.y;
////				r.z = ( r.z * distance ) + pLine->start.z;
////				// Draw a collision cross.
////				NsRender::setBlendMode( NsBlendMode_None, NULL, (unsigned char)0 );
////				NsPrim::line( r.x, r.y, r.z, r.x+25.0f, r.y, r.z,       (GXColor){255,0,0,255} );
////				NsPrim::line( r.x, r.y, r.z, r.x-25.0f, r.y, r.z,       (GXColor){255,0,0,255} );
////				NsPrim::line( r.x, r.y, r.z, r.x,       r.y, r.z+25.0f, (GXColor){255,0,0,255} );
////				NsPrim::line( r.x, r.y, r.z, r.x,       r.y, r.z-25.0f, (GXColor){255,0,0,255} );
////
////				NsPrim::line(
////					ct.corner(0)->x, ct.corner(0)->y, ct.corner(0)->z,
////					ct.corner(1)->x, ct.corner(1)->y, ct.corner(1)->z,
////					(GXColor){0,255,0,255} );
////				NsPrim::line(
////					ct.corner(1)->x, ct.corner(1)->y, ct.corner(1)->z,
////					ct.corner(2)->x, ct.corner(2)->y, ct.corner(2)->z,
////					(GXColor){0,255,0,255} );
////				NsPrim::line(
////					ct.corner(2)->x, ct.corner(2)->y, ct.corner(2)->z,
////					ct.corner(0)->x, ct.corner(0)->y, ct.corner(0)->z,
////					(GXColor){0,255,0,255} );
////				NsPrim::line(
////					ct.corner(0)->x, ct.corner(0)->y, ct.corner(0)->z,
////					ct.corner(0)->x+tri.normal.x, ct.corner(0)->y+tri.normal.y, ct.corner(0)->z+tri.normal.z,
////					(GXColor){0,255,0,255} );
////			}
//		}
//	}
//	return rv;
//}
//
//int NsDL::cull ( NsMatrix * m )
//{
//	int				lp;
//	unsigned int	code;
//	unsigned int	codeAND;
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
//	minx = m_cull.box.m_min.x;
//	miny = m_cull.box.m_min.y;
//	minz = m_cull.box.m_min.z;
//	maxx = m_cull.box.m_max.x;
//	maxy = m_cull.box.m_max.y;
//	maxz = m_cull.box.m_max.z;
//	GXProject ( minx, miny, minz, view, p, vp, &rx[0], &ry[0], &rz[0] );
//	GXProject ( minx, maxy, minz, view, p, vp, &rx[1], &ry[1], &rz[1] );
//	GXProject ( maxx, miny, minz, view, p, vp, &rx[2], &ry[2], &rz[2] );
//	GXProject ( maxx, maxy, minz, view, p, vp, &rx[3], &ry[3], &rz[3] );
//	GXProject ( minx, miny, maxz, view, p, vp, &rx[4], &ry[4], &rz[4] );
//	GXProject ( minx, maxy, maxz, view, p, vp, &rx[5], &ry[5], &rz[5] );
//	GXProject ( maxx, miny, maxz, view, p, vp, &rx[6], &ry[6], &rz[6] );
//	GXProject ( maxx, maxy, maxz, view, p, vp, &rx[7], &ry[7], &rz[7] );
//	m_cull.visible = 1;
//
//	// Generate clip code. {page 178, Procedural Elements for Computer Graphics}
//	// 1001|1000|1010
//	//     |    |
//	// ----+----+----
//	// 0001|0000|0010
//	//     |    |
//	// ----+----+----
//	// 0101|0100|0110
//	//     |    |
//	//
//	// Addition: Bit 4 is used for z behind.
//
//	codeAND	= 0x001f;
//	for ( lp = 0; lp < 8; lp++ ) {
//		// Only check x/y if z is valid (if z is invalid, the x/y values will be garbage).
//		if ( rz[lp] > 1.0f   ) {
//			code = (1<<4);
//		} else {
//			code = 0;
//			if ( rx[lp] < clip_l ) code |= (1<<0);
//			if ( rx[lp] > clip_r ) code |= (1<<1);
//			if ( ry[lp] > clip_b ) code |= (1<<2);
//			if ( ry[lp] < clip_t ) code |= (1<<3);
//		}
//		codeAND	&= code;
//	}
//	m_cull.clipCodeAND = codeAND;
//	// If any bits are set in the AND code, the object is invisible.
//	if ( codeAND ) {
//		m_cull.visible = 0;
//	}
//	
//	return !m_cull.visible;		// 0 = not culled, 1 = culled.
//}








