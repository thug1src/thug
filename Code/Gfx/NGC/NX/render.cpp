#include <sys/profiler.h>
#include <core/defines.h>
#include <core/debug.h>
#include <core/HashTable.h>
#include <stdio.h>
#include <stdlib.h>
#include "nx_init.h"
#include "scene.h"
#include "render.h"
#include <gfx\ngc\p_nxgeom.h>
#include <gfx\nxflags.h>
#include "occlude.h"
#include 	"gfx/ngc/nx/mesh.h"
#include "charpipeline\skinning.h"
#include	"gfx\NxMiscFX.h"

#include	<sys/ngc\p_camera.h>
#include	<sys/ngc\p_frame.h>
#include	<sys/ngc\p_prim.h>
#include	<sys/ngc\p_render.h>

#include "dolphin/base/ppcwgpipe.h"
#include "dolphin/gx/gxvert.h"

extern int g_object;
extern int g_view_object;
extern int g_material;
extern NxNgc::sScene * g_view_scene;

//extern u16 colorMap[];

bool g_skip_correctable = 0;

#define SHADOW_TEXTURE_SIZE 256
extern uint8 * shadowTextureData;
extern uint16 shadowPalette[16];
extern uint8 * zTextureData;
extern uint8 * blurTextureData;
NxNgc::sMaterial * p_last_material = NULL;
uint32 last_color_override = 0;
uint16	last_mesh_flags = 0;
bool last_correct = true;
bool reload_camera = false;

Mtx g_matrix0;
Mtx g_matrix90;
Mtx g_matrix180;
Mtx g_matrix270;

Mth::Vector g_shadow_object_pos;

int g_kill_layer = -1;

extern "C"
{
extern void FakeEnvMap( ROMtx m, s16 * srcPos, s16 * srcNorm, float * dstBase, u32 count );
extern void FakeEnvMapSkin( ROMtx m, s16 * srcPosNorm, float * dstBase, u32 count );
extern void FakeEnvMapFloat( ROMtx m, float * srcPos, s16 * srcNorm, float * dstBase, u32 count );
}

static float * sLastUV = NULL;

static bool first_mat = false;
//static bool one_mat = false;

extern uint16 shadowPalette[16];

//bool gOverDraw = false;

bool gMeshUseCorrection = false;
bool gMeshUseAniso = false;

const float CORRECTION_CUTOFF = ( 25.0f * 12.0f );
const float ANISO_CUTOFF = ( 100.0f * 12.0f );

int meshes_considered = 0;

namespace NxNgc
{

Lst::HashTable< sTextureProjectionDetails >	*pTextureProjectionDetailsTable = NULL;

NsCamera	current_cam;

#ifdef SHORT_VERT
static void cam_offset( float x, float y, float z )
{
	Mtx m;

	MTXTrans( m, x, y, z );
	MTXConcat( EngineGlobals.current_uploaded, m, m );

    GX::LoadPosMtxImm( m, GX_PNMTX0 );
}
#endif		// SHORT_VERT

//static void reflect_uvs( sMesh * p_mesh, float * p_tex, s16 * p_posBuffer, s16 * p_normBuffer, int step )
//{
//	if ( sLastUV == p_tex ) return;
//
//	// Inverse transform the camera position by the local to world matrix.
//	NsMatrix mp;
////	NsMatrix mi;
//
//	mp.copy( *((NsMatrix*)EngineGlobals.current_uploaded) );
////	mp.copy( EngineGlobals.camera );
//#ifdef SHORT_VERT
//	NsVector v( p_mesh->m_offset_x, p_mesh->m_offset_y, p_mesh->m_offset_z );
//	mp.translate( &v, NsMatrix_Combine_Post );
//#endif		// SHORT_VERT
////	mi.invert( mp );
////	mp.setRight( mi.getRight() );
////	mp.setUp( mi.getUp() );
////	mp.setAt( mi.getAt() );
//
////	mp.invert();
//
////	mp.setPosX( mi.getPosX() );
////	mp.setPosY( mi.getPosY() );
////	mp.setPosZ( mi.getPosZ() );
//	
//	ROMtx sm;
//
//	sm[0][0] = mp.getRightX();
//	sm[1][0] = mp.getRightY();
//	sm[2][0] = mp.getRightZ();
//					   
//	sm[0][1] = mp.getUpX();
//	sm[1][1] = mp.getUpY();
//	sm[2][1] = mp.getUpZ();
//					   
//	sm[0][2] = mp.getAtX();
//	sm[1][2] = mp.getAtY();
//	sm[2][2] = mp.getAtZ();
//			     
//	sm[3][0] = mp.getPosX();
//	sm[3][1] = mp.getPosY();
//	sm[3][2] = mp.getPosZ();
//
//	if ( step == 6 )
//	{
//		// Skinned
//		FakeEnvMapSkin( sm, p_posBuffer, p_tex, p_mesh->m_num_vertex );
//	}
//	else
//	{
//		// Not Skinned
//		GQRSetup6( p_mesh->m_vertex_format - 1,		// Pos
//				   GQR_TYPE_S16,
//				   0,					// UV
//				   GQR_TYPE_F32 );
//
//		GQRSetup7( 14,		// Normal
//				   GQR_TYPE_S16,
//				   0,					// UV
//				   GQR_TYPE_F32 );
//
////		NsMatrix mn;
////		mn.copy( mp );
//////		mn.invert(;
////		mn.setPos( 0.0f, 0.0f, 0.0f );
////
////		float shift = (float)(1<<(p_mesh->m_vertex_format-1));
////		for ( int lp = 0; lp < p_mesh->m_num_vertex; lp++ )
////		{
////			NsVector in, tpos, tnorm;
////			in.x = (((float)p_posBuffer[(lp*3)+0])/shift);
////			in.y = (((float)p_posBuffer[(lp*3)+1])/shift);
////			in.z = (((float)p_posBuffer[(lp*3)+2])/shift);
////			mp.multiply( &in, &tpos );
////			in.x = (((float)p_normBuffer[(lp*3)+0])/(float)(1<<14));
////			in.y = (((float)p_normBuffer[(lp*3)+1])/(float)(1<<14));
////			in.z = (((float)p_normBuffer[(lp*3)+2])/(float)(1<<14));
////			mn.multiply( &in, &tnorm );
//////			float z1 = 1.0f / 2000.0f;
//////			p_tex[(lp*2)+0] = ( ( ( ( ( tpos.x / (tpos.z+(float)gAdd) ) * (float)gScale ) + tnorm.x ) + 1.0f ) * 0.5f ) * (float)gRepeat;
//////			p_tex[(lp*2)+1] = ( ( ( ( ( tpos.y / (tpos.z+(float)gAdd) ) * (float)gScale )  + tnorm.y ) + 1.0f ) * 0.5f ) * (float)gRepeat;
////			
////			p_tex[(lp*2)+0] = ( ( ( ( ( tpos.x / ( tpos.z > 1 ? 1 : tpos.z ) ) + tnorm.x ) * 1.0f ) * 0.5f ) + 0.5f );
////			p_tex[(lp*2)+1] = ( ( ( ( ( tpos.y / ( tpos.z > 1 ? 1 : tpos.z ) ) + tnorm.y ) * 1.0f ) * 0.5f ) + 0.5f );
////
//////			p_tex[(lp*2)+0] = ( ( ( ( tpos.x + ( tnorm.x * tpos.z ) ) * ( 0.5f * tpos.z ) ) * ( 3.0f * tpos.z ) ) + ( 0.5f * tpos.z ) );
//////			p_tex[(lp*2)+1] = ( ( ( ( tpos.y + ( tnorm.y * tpos.z ) ) * ( 0.5f * tpos.z ) ) * ( 3.0f * tpos.z ) ) + ( 0.5f * tpos.z ) );
////			
//////			p_tex[(lp*2)+0] = ( ( tnorm.x ) + 1.0f ) * 0.5f;
//////			p_tex[(lp*2)+1] = ( ( tnorm.y ) + 1.0f ) * 0.5f;
////		}
//
//		FakeEnvMap( sm, p_posBuffer, p_normBuffer, p_tex, p_mesh->m_num_vertex );
//
//		// GQR back to normal.
//		GQRSetup7( GQR_SCALE_128,		// Pos/Normal
//				   GQR_TYPE_S16,
//				   GQR_SCALE_128,
//				   GQR_TYPE_S16 );
//		GQRSetup6( GQR_SCALE_256,		// Weights
//				   GQR_TYPE_U8,
//				   GQR_SCALE_256,
//				   GQR_TYPE_U8 );
//	}
//	sLastUV = p_tex;
//}

#ifndef SHORT_VERT
//static void float_reflect_uvs( sMesh * p_mesh, float * p_tex, float * p_posBuffer, s16 * p_normBuffer, int step )
//{
//	if ( sLastUV == p_tex ) return;
//
//	// Inverse transform the camera position by the local to world matrix.
//	NsMatrix mp;
//
//	mp.copy( *((NsMatrix*)EngineGlobals.current_uploaded) );
//	
//	ROMtx sm;
//
//	sm[0][0] = mp.getRightX();
//	sm[1][0] = mp.getRightY();
//	sm[2][0] = mp.getRightZ();
//					   
//	sm[0][1] = mp.getUpX();
//	sm[1][1] = mp.getUpY();
//	sm[2][1] = mp.getUpZ();
//					   
//	sm[0][2] = mp.getAtX();
//	sm[1][2] = mp.getAtY();
//	sm[2][2] = mp.getAtZ();
//			     
//	sm[3][0] = mp.getPosX();
//	sm[3][1] = mp.getPosY();
//	sm[3][2] = mp.getPosZ();
//
////	GQRSetup6( 0,					// Pos
////			   GQR_TYPE_F32,
////			   0,					// UV
////			   GQR_TYPE_F32 );
////
////	GQRSetup7( 14,					// Normal
////			   GQR_TYPE_S16,
////			   0,					// UV
////			   GQR_TYPE_F32 );
//
//	FakeEnvMapFloat( sm, p_posBuffer, p_normBuffer, p_tex, p_mesh->m_num_vertex );
//
////	// GQR back to normal.
////	GQRSetup7( GQR_SCALE_128,		// Pos/Normal
////			   GQR_TYPE_S16,
////			   GQR_SCALE_128,
////			   GQR_TYPE_S16 );
////	GQRSetup6( GQR_SCALE_256,		// Weights
////			   GQR_TYPE_U8,
////			   GQR_SCALE_256,
////			   GQR_TYPE_U8 );
//	sLastUV = p_tex;
//}
#endif	// SHORT_VERT

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void init_render_system( void )
{
	pTextureProjectionDetailsTable = new Lst::HashTable< sTextureProjectionDetails >( 8 );

	MTXRotDeg( g_matrix0, 'y', 0.0f );
	MTXRotDeg( g_matrix90, 'y', 90.0f );
	MTXRotDeg( g_matrix180, 'y', 180.0f );
	MTXRotDeg( g_matrix270, 'y', 270.0f );
}

void set_blend_mode( uint64 mode )
{
	// Low 32 bits contain the mode, high 32 bits contain the fixed alpha value.
	uint32 actual_mode	= (uint32)mode;
//	uint32 fixed_alpha	= (uint32)( mode >> 32 );

	// Only do something if the blend mode is changing.
//	if( actual_mode != EngineGlobals.blend_mode_value )
	{
		switch( actual_mode )
		{
			case vBLEND_MODE_ADD:
			case vBLEND_MODE_ADD_FIXED:
				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case vBLEND_MODE_SUBTRACT:
			case vBLEND_MODE_SUB_FIXED:
				GX::SetBlendMode ( GX_BM_SUBTRACT, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case vBLEND_MODE_BLEND:
			case vBLEND_MODE_BLEND_FIXED:
				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case vBLEND_MODE_BRIGHTEN:
			case vBLEND_MODE_BRIGHTEN_FIXED:
				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case vBLEND_MODE_MODULATE_FIXED:
			case vBLEND_MODE_MODULATE:
				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
			case vBLEND_MODE_DIFFUSE:
			default:
				GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
				break;
		}
	}
}



void set_render_state( uint32 type, uint32 state )
{
	switch( type )
	{
		case RS_ZWRITEENABLE:
		{
			if( state > 0 )
			{
				if( EngineGlobals.z_write_enabled == FALSE )
				{
					GX::SetZMode ( EngineGlobals.z_test_enabled ? GX_TRUE : GX_FALSE, EngineGlobals.z_test_enabled ? GX_LEQUAL : GX_ALWAYS, GX_TRUE );
					EngineGlobals.z_write_enabled = TRUE;
				}
			}
			else
			{
				if( EngineGlobals.z_write_enabled == TRUE )
				{
					GX::SetZMode ( EngineGlobals.z_test_enabled ? GX_TRUE : GX_FALSE, EngineGlobals.z_test_enabled ? GX_LEQUAL : GX_ALWAYS, GX_FALSE );
					EngineGlobals.z_write_enabled = FALSE;
				}
			}
			break;
		}

		case RS_ZTESTENABLE:
		{
			if( state > 0 )
			{
				if( EngineGlobals.z_test_enabled == FALSE )
				{
					GX::SetZMode ( GX_TRUE, GX_LEQUAL, EngineGlobals.z_write_enabled ? GX_TRUE : GX_FALSE );
					EngineGlobals.z_test_enabled = TRUE;
				}
			}
			else
			{
				if( EngineGlobals.z_test_enabled == TRUE )
				{
					GX::SetZMode ( GX_FALSE, GX_ALWAYS, EngineGlobals.z_write_enabled ? GX_TRUE : GX_FALSE );
					EngineGlobals.z_test_enabled = FALSE;
				}
			}
			break;
		}

		case RS_ALPHACUTOFF:
		{
//			GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//			// Convert from state (where 1 means "render all pixels with alpha 1 or higher") to the D3D
//			if( state != EngineGlobals.alpha_ref )
//			{
//				EngineGlobals.alpha_ref = state;
//				if( state > 0 )
//				{
//					D3DDevice_SetRenderState( D3DRS_ALPHAFUNC,			D3DCMP_GREATEREQUAL );
//					D3DDevice_SetRenderState( D3DRS_ALPHAREF,			state );
//					D3DDevice_SetRenderState( D3DRS_ALPHATESTENABLE,	TRUE );
//				}
//				else
//				{
//					D3DDevice_SetRenderState( D3DRS_ALPHATESTENABLE,	FALSE );
//				}
//			}
			break;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float frustum_sort_mid( Mth::Vector& p_sphere )
{
	NsVector	test_in;
	NsVector	test_mid;
	
//	D3DXMATRIX *p_world_transform;
//	
//	// Build the composite transform if required.
//	if( p_bbox_transform )
//	{
//		p_world_transform = p_bbox_transform;
//	}
//	else
//	{
//		p_world_transform = NULL;
//	}
	
	test_in.x	= p_sphere[X];
	test_in.y	= p_sphere[Y];
	test_in.z	= p_sphere[Z];

	EngineGlobals.local_to_camera.multiply( &test_in, &test_mid );
	
//	if( p_world_transform )
//	{
//		D3DXVec3Transform( &test_mid, &test_in, p_world_transform );
//		test_in.x = test_mid.x;
//		test_in.y = test_mid.y;
//		test_in.z = test_mid.z;
//	}
//	D3DXVec3Transform( &test_mid, &test_in, &EngineGlobals.view_matrix );
	return test_mid.z;
}		



struct sSortedMeshEntry
{
	sMesh				*p_mesh;
	float				sort;
	sSortedMeshEntry	*pNext;
};


//static sSortedMeshEntry	sortedMeshArray[1000];

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int cmp( const void *p1, const void *p2 )
{
	return((sSortedMeshEntry*)p1)->sort < ((sSortedMeshEntry*)p2)->sort ? -1 : ((sSortedMeshEntry*)p1)->sort > ((sSortedMeshEntry*)p2)->sort ? 1 : 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void create_texture_projection_details( sTexture *p_texture, Nx::CNgcModel *p_model, sScene *p_scene )
{
	sTextureProjectionDetails *p_details = new sTextureProjectionDetails;

	p_details->p_model		= p_model;
	p_details->p_scene		= p_scene;
	p_details->p_texture	= p_texture;
	
	// Flag this scene as receiving shadows.
//	p_scene->m_flags		|= SCENE_FLAG_RECEIVE_SHADOWS;
	
	pTextureProjectionDetailsTable->PutItem((uint32)p_texture, p_details );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void destroy_texture_projection_details( sTexture *p_texture )
{
	sTextureProjectionDetails *p_details = pTextureProjectionDetailsTable->GetItem((uint32)p_texture );
	if( p_details )
	{
		pTextureProjectionDetailsTable->FlushItem((uint32)p_texture );
		delete p_details;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void set_texture_projection_camera( sTexture *p_texture, NsVector * p_pos, NsVector * p_at )
{
//	NsMatrix view_matrix;

	sTextureProjectionDetails *p_details = pTextureProjectionDetailsTable->GetItem((uint32)p_texture );
	if( p_details )
	{
		NsVector	up;
		// Check for 'straight down' vector.
		if(( p_pos->x == p_at->x ) && ( p_pos->z == p_at->z ))
		{
//			D3DXMatrixLookAtRH( &p_details->view_matrix, p_pos, p_at, &D3DXVECTOR3( 0.0f, 0.0f, 1.0f ));


			up.set( 0.0f, 0.0f, 1.0f );

		}
		else
		{
			up.set( 0.0f, 1.0f, 0.0f );
//			D3DXMatrixLookAtRH( &p_details->view_matrix, p_pos, p_at, &D3DXVECTOR3( 0.0f, 1.0f, 0.0f ));
		}

		// Get the 'right' vector as the cross product of camera 'at and world 'up'.
//		NsVector screen_right;
//		NsVector screen_up;
//		screen_right.cross( *p_at, up );
//		screen_up.cross( screen_right, *p_at );
//
//		screen_right.normalize();
//		screen_up.normalize();
//
//		p_details->view_matrix.setRight( &screen_right );
//		p_details->view_matrix.setUp( &screen_up );
//		p_details->view_matrix.setAt( p_at );
//		p_details->view_matrix.setPos( p_pos );
		p_details->view_matrix.lookAt( p_pos, &up, p_at );

	

//		D3DXMatrixOrthoRH( &p_details->projection_matrix, 96.0f, 96.0f, 1.0f, 512.0f );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

//static D3DXMATRIX	world;
//static D3DXMATRIX	view;
//static D3DXMATRIX	projection;

void set_camera( Mth::Matrix *p_matrix, Mth::Vector *p_position, float screen_angle, float aspect_ratio, float scale )
{
	NsMatrix	m;
	NsFrame		f;

	EngineGlobals.tx = 2.0f * tanf(screen_angle*8.72664626e-03f);		// tan of half (angle in radians)
	EngineGlobals.ty = -(EngineGlobals.tx / aspect_ratio);
	EngineGlobals.near	= -2.0;
	EngineGlobals.far	= -40000.0;

	current_cam.perspective( EngineGlobals.tx, -EngineGlobals.ty, -EngineGlobals.near, -EngineGlobals.far * 2 );

//	if ( scale > 1.0f )
//	{
//		current_cam.SetViewMatrix( 2, 2, -0.999999f );
//		current_cam.SetViewMatrix( 3, 2, 0.0f );
//	}

	EngineGlobals.tx = tanf( Mth::DegToRad( screen_angle * 0.5f )); ;		// tan of half (angle in radians)
	EngineGlobals.ty = -(EngineGlobals.tx / aspect_ratio);
	EngineGlobals.sx	= 1.0f/sqrtf(1.0f+1.0f/(EngineGlobals.tx*EngineGlobals.tx));
	EngineGlobals.sy	= 1.0f/sqrtf(1.0f+1.0f/(EngineGlobals.ty*EngineGlobals.ty));
	EngineGlobals.cx	= 1.0f/sqrtf(1.0f+EngineGlobals.tx*EngineGlobals.tx);
	EngineGlobals.cy	= 1.0f/sqrtf(1.0f+EngineGlobals.ty*EngineGlobals.ty);
//	EngineGlobals.near	= -2.0;
//	EngineGlobals.far	= -40000.0;

	m.setRight( -p_matrix->GetRight().GetX(), -p_matrix->GetRight().GetY(), -p_matrix->GetRight().GetZ() );
	m.setUp( p_matrix->GetUp().GetX(), p_matrix->GetUp().GetY(), p_matrix->GetUp().GetZ() );
	m.setAt( -p_matrix->GetAt().GetX(), -p_matrix->GetAt().GetY(), -p_matrix->GetAt().GetZ() );
	m.setPos( p_position->GetX(), p_position->GetY(), p_position->GetZ() );

	f.setModelMatrix( &m );
	current_cam.setFrame( &f );
	current_cam.begin();		// No need to end...

	EngineGlobals.world_to_camera = *current_cam.getCurrent();
	EngineGlobals.local_to_camera = *current_cam.getCurrent();
	EngineGlobals.camera = m;

	EngineGlobals.object_pos.x = 0.0f;
	EngineGlobals.object_pos.y = 0.0f;
	EngineGlobals.object_pos.z = 0.0f;

	MTXCopy ( (MtxPtr)current_cam.getCurrent(), EngineGlobals.current_uploaded );


//	NsVector wscale( 1.0f, 1.0f, 1.0f );	//scale, scale, scale );
	NsVector wscale( scale, scale, scale );
	NsMatrix ms;
	ms.copy( m );
	ms.scale( &wscale, NsMatrix_Combine_Pre );

	f.setModelMatrix( &ms );
	current_cam.setFrame( &f );
	current_cam.end();
	current_cam.begin();		// No need to end...

//	NsMatrix ms;
//	ms.setRight( m.getRightX() * scale, m.getRightY() * scale, m.getRightZ() * scale );
//	ms.setUp( m.getUpX() * scale, m.getUpY() * scale, m.getUpZ() * scale );
//	ms.setAt( m.getAtX() * scale, m.getAtY() * scale, m.getAtZ() * scale );
//	ms.setPos( m.getPosX(), m.getPosY(), m.getPosZ() );


}

/******************************************************************/
/*                                                                */
/* Checks a bounding box against the current view frustum		  */
/* (iBgnoring far clipping),										  */
/* returns true if any part is visible.							  */
/* Horribly inefficient at present.								  */
/*                                                                */
/******************************************************************/
bool frustum_check_box( Mtx m, Mth::CBBox& box )
{
	Vec	test_in, test_out;
	
	uint32	cumulative_projection_space_outcode	= 0xFF;
	float	min_x = box.GetMin().GetX();
	float	min_y = box.GetMin().GetY();
	float	min_z = box.GetMin().GetZ();
	float	max_x = box.GetMax().GetX();
	float	max_y = box.GetMax().GetY();
	float	max_z = box.GetMax().GetZ();

	for( uint32 v = 0; v < 8; ++v )
	{
		uint32 projection_space_outcode = 0;

		test_in.x = ( v & 0x04 ) ? max_x : min_x;
		test_in.y = ( v & 0x02 ) ? max_y : min_y;
		test_in.z = ( v & 0x01 ) ? max_z : min_z;

		// At this point it's important to check to see whether the point is in postive or negative z-space, since
		// after the projection transform, both very large camera space z values and camera space z values where z < 0
		// will give results with z > 1. (Camera space values in the range [0,near] give negative projection space z values).
		MTXMultVec( m, &test_in, &test_out );

//		if(( -test_mid.z < 0.0f ) && ( !EngineGlobals.is_orthographic ))
//		{
//			test_out.x = -test_out.x;
//			test_out.y = -test_out.y;
//		}

		if( test_out.x > 1.0f )
		{
			projection_space_outcode |= 0x01;
		}
		else if( test_out.x < -1.0f )
		{
			projection_space_outcode |= 0x02;
		}

		if( test_out.y > 1.0f )
		{
			projection_space_outcode |= 0x04;
		}
		else if( test_out.y < -1.0f )
		{
			projection_space_outcode |= 0x08;
		}

		cumulative_projection_space_outcode	&= projection_space_outcode;

		if( cumulative_projection_space_outcode == 0 )
		{
			// Early out.
			return true;
		}
	}
	return false;
}

bool frustum_check_sphere( Mtx m, Mth::Vector& p_sphere )
{
//	Vec in;
//	Vec out;
//
//	in.x = p_sphere[X];
//	in.y = p_sphere[Y];
//	in.z = p_sphere[Z];
//
////	NsMatrix mat;
////	mat.copy( EngineGlobals.local_to_camera );
////	NsVector s( 0.5f, 0.5f, 0.5f );
////	mat.scale( &s, NsMatrix_Combine_Post );
////
////
////	mat.multiply( &in, &out );
////	EngineGlobals.local_to_camera.multiply( &in, &out );
//
//	MTXMultVec( m, &in, &out );
//
//	float x = out.x;
//	float y = out.y;
//	float z = out.z;
//	float R = p_sphere[W];
//
//	if ( R < ( z - EngineGlobals.near ) ) return false;
//	if ( R < ( EngineGlobals.far - z ) ) return false;
//
//	float sx_z = EngineGlobals.sx * z;
//	float cx_x = EngineGlobals.cx * x;
//	if ( R < ( sx_z - cx_x ) ) return false;
//	if ( R < ( sx_z + cx_x ) ) return false; 
//
//	float sy_z = EngineGlobals.sy * z;
//	float cy_y = EngineGlobals.cy * y;
//	if ( R < ( sy_z - cy_y ) ) return false;
//	if ( R < ( sy_z + cy_y ) ) return false; 
//
//	return true;

	Mth::CBBox bbox;
	Mth::Vector p;
	
	p.Set( p_sphere[X] + p_sphere[W], p_sphere[Y], p_sphere[Z] ); bbox.AddPoint( p );
	p.Set( p_sphere[X] - p_sphere[W], p_sphere[Y], p_sphere[Z] ); bbox.AddPoint( p );
	p.Set( p_sphere[X] , p_sphere[Y]+ p_sphere[W], p_sphere[Z] ); bbox.AddPoint( p );
	p.Set( p_sphere[X] , p_sphere[Y]- p_sphere[W], p_sphere[Z] ); bbox.AddPoint( p );
	p.Set( p_sphere[X] , p_sphere[Y], p_sphere[Z]+ p_sphere[W] ); bbox.AddPoint( p );
	p.Set( p_sphere[X] , p_sphere[Y], p_sphere[Z]- p_sphere[W] ); bbox.AddPoint( p );

	return frustum_check_box( m, bbox );
}

/******************************************************************/
/*                                                                */
/* Checks a bounding box against the current view frustum		  */
/* (iBgnoring far clipping),										  */
/* returns true if any part is visible.							  */
/* Horribly inefficient at present.								  */
/*                                                                */
/******************************************************************/
bool frustum_check_box( Mth::CBBox& box )
{
	int				lp;
	unsigned int	code;
	unsigned int	codeAND;
	f32				rx[8], ry[8], rz[8];
	f32				p[GX_PROJECTION_SZ];
	f32				vp[GX_VIEWPORT_SZ];
	u32				clip_x;
	u32				clip_y;
	u32				clip_w;
	u32				clip_h;
	float			clip_l;
	float			clip_t;
	float			clip_r;
	float			clip_b;
	MtxPtr			view;
	float			minx, miny, minz;
	float			maxx, maxy, maxz;

	GX::GetProjectionv( p );
	GX::GetViewportv( vp );
	GX::GetScissor ( &clip_x, &clip_y, &clip_w, &clip_h );
	clip_l = (float)clip_x;
	clip_t = (float)clip_y;
	clip_r = (float)(clip_x + clip_w);
	clip_b = (float)(clip_y + clip_h);

	view = (MtxPtr)&EngineGlobals.local_to_camera;

	minx = box.GetMin().GetX();
	miny = box.GetMin().GetY();
	minz = box.GetMin().GetZ();
	maxx = box.GetMax().GetX();
	maxy = box.GetMax().GetY();
	maxz = box.GetMax().GetZ();
	GX::Project ( minx, miny, minz, view, p, vp, &rx[0], &ry[0], &rz[0] );
	GX::Project ( minx, maxy, minz, view, p, vp, &rx[1], &ry[1], &rz[1] );
	GX::Project ( maxx, miny, minz, view, p, vp, &rx[2], &ry[2], &rz[2] );
	GX::Project ( maxx, maxy, minz, view, p, vp, &rx[3], &ry[3], &rz[3] );
	GX::Project ( minx, miny, maxz, view, p, vp, &rx[4], &ry[4], &rz[4] );
	GX::Project ( minx, maxy, maxz, view, p, vp, &rx[5], &ry[5], &rz[5] );
	GX::Project ( maxx, miny, maxz, view, p, vp, &rx[6], &ry[6], &rz[6] );
	GX::Project ( maxx, maxy, maxz, view, p, vp, &rx[7], &ry[7], &rz[7] );

	// Generate clip code. {page 178, Procedural Elements for Computer Graphics}
	// 1001|1000|1010
	//     |    |
	// ----+----+----
	// 0001|0000|0010
	//     |    |
	// ----+----+----
	// 0101|0100|0110
	//     |    |
	//
	// Addition: Bit 4 is used for z behind.

	codeAND	= 0x001f;
	for ( lp = 0; lp < 8; lp++ ) {
		// Only check x/y if z is valid (if z is invalid, the x/y values will be garbage).
		if ( rz[lp] > 1.0f   ) {
			code = (1<<4);
		} else {
			code = 0;
			if ( rx[lp] < clip_l ) code |= (1<<0);
			if ( rx[lp] > clip_r ) code |= (1<<1);
			if ( ry[lp] > clip_b ) code |= (1<<2);
			if ( ry[lp] < clip_t ) code |= (1<<3);
		}
		codeAND	&= code;
		if ( !codeAND ) return true;
	}
	if ( !codeAND ) return true;
	return false;
//	m_cull.clipCodeAND = codeAND;
//	// If any bits are set in the AND code, the object is invisible.
//	if ( codeAND ) {
//		m_cull.visible = 0;
//	}
//	
//	return !m_cull.visible;		// 0 = not culled, 1 = culled.


//
//	NsVector test_in, test_out;
//	
//	uint32	cumulative_projection_space_outcode	= 0xFF;
//	float	min_x = box.GetMin().GetX();
//	float	min_y = box.GetMin().GetY();
//	float	min_z = box.GetMin().GetZ();
//	float	max_x = box.GetMax().GetX();
//	float	max_y = box.GetMax().GetY();
//	float	max_z = box.GetMax().GetZ();
//
//	for( uint32 v = 0; v < 8; ++v )
//	{
//		uint32 projection_space_outcode = 0;
//
//		test_in.x = ( v & 0x04 ) ? max_x : min_x;
//		test_in.y = ( v & 0x02 ) ? max_y : min_y;
//		test_in.z = ( v & 0x01 ) ? max_z : min_z;
//
//		// At this point it's important to check to see whether the point is in postive or negative z-space, since
//		// after the projection transform, both very large camera space z values and camera space z values where z < 0
//		// will give results with z > 1. (Camera space values in the range [0,near] give negative projection space z values).
//		EngineGlobals.local_to_camera.multiply( &test_in, &test_out );
////		MTXMultVec( m, &test_in, &test_out );
//
////		if(( -test_mid.z < 0.0f ) && ( !EngineGlobals.is_orthographic ))
////		{
////			test_out.x = -test_out.x;
////			test_out.y = -test_out.y;
////		}
//
//		if( test_out.x > 1.0f )
//		{
//			projection_space_outcode |= 0x01;
//		}
//		else if( test_out.x < -1.0f )
//		{
//			projection_space_outcode |= 0x02;
//		}
//
//		if( test_out.y > 1.0f )
//		{
//			projection_space_outcode |= 0x04;
//		}
//		else if( test_out.y < -1.0f )
//		{
//			projection_space_outcode |= 0x08;
//		}
//
//		cumulative_projection_space_outcode	&= projection_space_outcode;
//
//		if( cumulative_projection_space_outcode == 0 )
//		{
//			// Early out.
//			return true;
//		}
//	}
//	return false;
}

/******************************************************************/
/* Quick determination of if something is visible or not, uses	  */
/* the previously calculated s and c vectors and the			  */
/* WorldToCamera transform (note, no attempt is made to ensure	  */
/* this is the same camera that the object will eventually be	  */
/* rendered with.												  */
/******************************************************************/
bool IsVisible( Mth::Vector &sphere )
{
	NsVector in;
	NsVector out;

	in.x = sphere.GetX();
	in.y = sphere.GetY();
	in.z = sphere.GetZ();

	EngineGlobals.world_to_camera.multiply( &in, &out );

	float x = out.x;
	float y = out.y;
	float z = out.z;
	float R = sphere[W];
	
	if(	//( -z + R < EngineGlobals.near ) ||
//	   if ( R < ( -10000.0f/*EngineGlobals.far*/ - z ) ) return false;
	   ( R < ( z - EngineGlobals.near ) ) ||
	   ( R < ( EngineGlobals.far - z ) ) ||

	   ( R < EngineGlobals.sy * z + EngineGlobals.cy * y ) ||
	   ( R < EngineGlobals.sy * z - EngineGlobals.cy * y ) ||
	   ( R < EngineGlobals.sx * z - EngineGlobals.cx * x ) ||
	   ( R < EngineGlobals.sx * z + EngineGlobals.cx * x ))
	{
		return false;
	}
	else
	{
		if( TestSphereAgainstOccluders( &sphere ))
		{
			// Occluded.
			return false;
		}
		else
		{
			return true;
		}
	}
//	if (R<z-near || R<far-z || R<sy*z+cy*y || R<sy*z-cy*y || R<sx*z-cx*x || R<sx*z+cx*x) visible = false;

}

bool frustum_check_sphere( Mth::Vector& p_sphere )
{
	NsVector in;
	NsVector out;

	in.x = p_sphere[X];
	in.y = p_sphere[Y];
	in.z = p_sphere[Z];

//	NsMatrix mat;
//	mat.copy( EngineGlobals.local_to_camera );
//	NsVector s( 0.5f, 0.5f, 0.5f );
//	mat.scale( &s, NsMatrix_Combine_Post );
//
//
//	mat.multiply( &in, &out );
	EngineGlobals.local_to_camera.multiply( &in, &out );

	float x = out.x;
	float y = out.y;
	float z = out.z;
	float R = p_sphere[W];
	
//	// See if mesh is close enough to use color correction.
//	if ( ( -z - R ) < CORRECTION_CUTOFF )
//	{
//		gMeshUseCorrection = true;
//	}
//	else
//	{
//		gMeshUseCorrection = false;
//	}
//
//	// See if mesh is close enough to use anisotropic filtering.
//	if ( ( -z - R ) < ANISO_CUTOFF )
//	{
//		gMeshUseAniso = true;
//	}
//	else
//	{
//		gMeshUseAniso = false;
//	}

	if ( R < ( z - EngineGlobals.near ) ) return false;
	if ( R < ( EngineGlobals.far - z ) ) return false;

	float sx_z = EngineGlobals.sx * z;
	float cx_x = EngineGlobals.cx * x;
	if ( R < ( sx_z - cx_x ) ) return false;
	if ( R < ( sx_z + cx_x ) ) return false; 

	float sy_z = EngineGlobals.sy * z;
	float cy_y = EngineGlobals.cy * y;
	if ( R < ( sy_z - cy_y ) ) return false;
	if ( R < ( sy_z + cy_y ) ) return false; 

	return true;
}

bool frustum_check_sphere( Mth::Vector& p_sphere, float * p_z )
{
	bool visible = true;

	NsVector in;
	NsVector out;

	in.x = p_sphere[X];
	in.y = p_sphere[Y];
	in.z = p_sphere[Z];

	EngineGlobals.local_to_camera.multiply( &in, &out );

	float x = out.x;
	float y = out.y;
	float z = out.z;
	float R = p_sphere[W];
	
	if (R<z-EngineGlobals.near || R<EngineGlobals.far-z || R<EngineGlobals.sy*z+EngineGlobals.cy*y || R<EngineGlobals.sy*z-EngineGlobals.cy*y || R<EngineGlobals.sx*z-EngineGlobals.cx*x || R<EngineGlobals.sx*z+EngineGlobals.cx*x) visible = false;

	if ( p_z ) *p_z = out.z;

	return visible;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void calculate_tex_proj_matrix( NsMatrix * p_tex_view_matrix, NsMatrix * p_tex_proj_matrix, NsMatrix * p_tex_transform_matrix, NsMatrix * p_world_matrix )
{
	// Get the current view matrix.
	NsMatrix matView, matInvView;

	matView.copy( EngineGlobals.camera );
	matInvView.invert( matView );

	NsMatrix matBiasScale;
    matBiasScale.identity();

//#	ifdef TEST_SB
//	static float x0 = 256.0f;
//	static float y0 = 256.0f;
//	
//	matBiasScale._11 = x0 * 0.5f;
//	matBiasScale._22 = y0 * -0.5f;
////	matBiasScale._11 = 0.5f;
////	matBiasScale._22 = -0.5f;
////	matBiasScale._33 = D3DZ_MAX_D16;
//	matBiasScale._33 = D3DZ_MAX_D24S8;
//#	else
//	matBiasScale._11 = 0.5f;
//	matBiasScale._22 = -0.5f;
//#	endif	
	matBiasScale.setUpY( 0.5f );
	matBiasScale.setAtZ( -0.5f );

//#	ifdef TEST_SB
//	static float x1 = 256.0f;
//	static float y1 = 256.0f;
//	
//	matBiasScale._41 = x1 * 0.5f + 0.5f;
//	matBiasScale._42 = y1 * 0.5f + 0.5f;
////	matBiasScale._41 = 0.5f;
////	matBiasScale._42 = 0.5f;
//#	else
//	matBiasScale._41 = 0.5f;
//	matBiasScale._42 = 0.5f;
//#	endif	
	matBiasScale.setPosY( 0.5f );
	matBiasScale.setPosZ( -0.5f );

	NsMatrix m_matTexProj;
//#	if defined( TEST_SB )
//	// Don't bother with inverse view transform for Shadow Buffer, since we are picking up world-space coordinates directly.
//	if( p_world_matrix )
//	{
//		m_matTexProj = *p_world_matrix;												// Transform to world space.
//		D3DXMatrixMultiply( &m_matTexProj, &m_matTexProj, p_tex_view_matrix );		// Transform to projection camera space.
//	}
//	else
//	{
//		m_matTexProj = *p_tex_view_matrix;											// Transform to projection camera space.
//	}
//	D3DXMatrixMultiply( &m_matTexProj, &m_matTexProj, p_tex_proj_matrix );			// Situate verts relative to projector's view
//    D3DXMatrixMultiply( p_tex_transform_matrix, &m_matTexProj, &matBiasScale );		// Scale and bias to map the near clipping plane to texcoords
//#	else
//	m_matTexProj = matInvView;														// Transform cameraspace position to worldspace
//	D3DXMatrixMultiply( &m_matTexProj, &m_matTexProj, p_tex_view_matrix );			// Transform to projection camera space.
//    D3DXMatrixMultiply( &m_matTexProj, &m_matTexProj, p_tex_proj_matrix );			// Situate verts relative to projector's view
//    D3DXMatrixMultiply( p_tex_transform_matrix, &m_matTexProj, &matBiasScale );		// Scale and bias to map the near clipping plane to texcoords
//#	endif
	m_matTexProj = matInvView;														// Transform cameraspace position to worldspace
	m_matTexProj.cat( m_matTexProj, *p_tex_view_matrix );
	m_matTexProj.cat( m_matTexProj, *p_tex_proj_matrix );
	p_tex_transform_matrix->cat( m_matTexProj, matBiasScale );

	// This is just for displaying the frustum lines.
/*
    // Convert from homogeneous texmap coords to worldspace
    D3DXMATRIX matInvTexView, matInvTexProj;
    D3DXMatrixInverse( &matInvTexView, NULL, &matTexView );
    D3DXMatrixInverse( &matInvTexProj, NULL, &matTexProj );          

    for( int i = 0; i < 8; i++ )
    {
        D3DXVECTOR4 vT( 0.0f, 0.0f, 0.0f, 1.0f );
        vT.x = (i%2) * ( i&0x2 ? -1.0f : 1.0f );
        vT.y = (i%2) * ( i&0x4 ? -1.0f : 1.0f );
        vT.z = (i%2) * ( 1.0f );

        D3DXVec4Transform( &vT, &vT, &matInvTexProj );
        D3DXVec4Transform( &vT, &vT, &matInvTexView );

        g_FrustumLines[i].x = vT.x / vT.w;
        g_FrustumLines[i].y = vT.y / vT.w;
        g_FrustumLines[i].z = vT.z / vT.w;
    }
*/
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void render_shadow_targets( void )
{
	GX::SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
//	D3DXMATRIX stored_view_matrix		= EngineGlobals.view_matrix;
//	D3DXMATRIX stored_projection_matrix	= EngineGlobals.projection_matrix;
	
	// Goes through the list of render target textures, rendering to each one in turn.
	pTextureProjectionDetailsTable->IterateStart();
	sTextureProjectionDetails *p_details = pTextureProjectionDetailsTable->IterateNext();
//
//	// Set the basic pixel shader that just copies a single color (in C0) over.
//	EngineGlobals.pixel_shader_constants[0]		= 1.0f;
//	EngineGlobals.pixel_shader_constants[1]		= 1.0f;
//	EngineGlobals.pixel_shader_constants[2]		= 1.0f;
//	EngineGlobals.pixel_shader_constants[3]		= 1.0f;
//	EngineGlobals.upload_pixel_shader_constants	= true;
//	set_pixel_shader( PixelShader3 );
//	EngineGlobals.pixel_shader_override			= PixelShader3;
//	
	// Clear background of shadow.
//	NsCamera cam;
//	cam.orthographic( 0, 0, 640, 448 );
//	cam.begin();
//
//	GX::SetZMode ( GX_FALSE, GX_ALWAYS, GX_TRUE );
//	//NsPrim::box( 0, 0, 256, 256, (GXColor){128,128,128,255} );
//	float zf = -( EngineGlobals.far + 0.5f );
//	NsPrim::quad( 0, 0, zf, 640, 0, zf, 640, 448, zf, 0, 448, zf, (GXColor){128,128,128,255} );
//	cam.end();

	while( p_details )
	{
		if( p_details->p_model )
		{
			// Set the new render target.
///			D3DDevice_SetRenderTarget( p_details->p_texture->pD3DSurface, NULL );
//
//			D3DDevice_Clear( 0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0 );

			// Set the view and projection transforms.
//			EngineGlobals.view_matrix		= p_details->view_matrix;
//			EngineGlobals.projection_matrix	= p_details->projection_matrix;
//			EngineGlobals.is_orthographic	= true;

			// Render all instances for the CGeom's contained in this model.
			int num_geoms = p_details->p_model->GetNumGeoms();
			for( int i = 0; i < num_geoms; ++i )
			{
				Nx::CNgcGeom *p_ngc_geom	= static_cast<Nx::CNgcGeom*>( p_details->p_model->GetGeomByIndex( i ));
				CInstance *p_instance		= p_ngc_geom->GetInstance();
				
				if( p_instance->GetActive())
				{
					// Create the camera and attach a frame.
					NsCamera cam;
					//NsFrame frame;
					//cam.setFrame( &frame );
	
					// Assume parallel projection in this case.
					cam.orthographic();
					cam.orthographic( 0, 0, 256, 256 );
					cam.setViewWindow( 128, 128 );
					
					// Position the camera somewhere near the object, pointing directly at it.
					float	at[3];
					float	pos[3];
	
					at[0]	= p_instance->GetTransform()->GetPos()[X];
					at[1]	= p_instance->GetTransform()->GetPos()[Y];
					at[2]	= p_instance->GetTransform()->GetPos()[Z];
	
					pos[0]	= p_instance->GetTransform()->GetPos()[X];
					pos[1]	= p_instance->GetTransform()->GetPos()[Y] + 64.0f;
					pos[2]	= p_instance->GetTransform()->GetPos()[Z];
	
					g_shadow_object_pos = p_instance->GetTransform()->GetPos();
					g_shadow_object_pos[Y] += ( 12.0f * 3.0f );		// Amount of leeway...

					// Also need to take account of the view offset.
					Mth::Vector object_up	= p_instance->GetTransform()->GetUp();
					at[0]	+= object_up.GetX() * 36.0f;
					at[1]	+= object_up.GetY() * 36.0f;
					at[2]	+= object_up.GetZ() * 36.0f;
					pos[0]	+= object_up.GetX() * 36.0f;
					pos[1]	+= object_up.GetY() * 36.0f;
					pos[2]	+= object_up.GetZ() * 36.0f;
	
					cam.pos( pos[0], pos[1], pos[2] );
					cam.up( 1.0f, 0.0f, 0.0f );
					cam.lookAt( at[0], at[1], at[2] );
	
//					EngineGlobals.world_to_camera.identity();
//					EngineGlobals.world_to_camera.setPos( pos[0], pos[1], pos[2] );
//					EngineGlobals.world_to_camera.setRight( 1.0f, 0.0f, 0.0f );
//					EngineGlobals.world_to_camera.setUp( 0.0f, 0.0f, 1.0f );
//					EngineGlobals.world_to_camera.setAt( 0.0f, -1.0f, 0.0f );
	
					cam.begin();
					MTXCopy ( (MtxPtr)cam.getCurrent(), EngineGlobals.current_uploaded );
	
					EngineGlobals.world_to_camera.copy( *cam.getCurrent() );
					EngineGlobals.shadow_camera.copy( *cam.getCurrent() );
	
					// Flag the scene as having the shadow version rendered.
					p_instance->GetScene()->m_flags |= SCENE_FLAG_RENDERING_SHADOW;
	
					// Render the model.
//					ROMtx bone_mtx[60];
//					p_instance->Transform( vRENDER_SHADOW_1ST_PASS, bone_mtx, NULL );
//					p_instance->ClearFlag( NxNgc::CInstance::INSTANCE_FLAG_TRANSFORM_ME );
					p_instance->Render( vRENDER_SHADOW_1ST_PASS );
	
					// Clear the flag the scene as having the shadow version rendered.
					p_instance->GetScene()->m_flags &= ~SCENE_FLAG_RENDERING_SHADOW;
				
					// Flag the scene as self shadowing.
					p_instance->GetScene()->m_flags |= SCENE_FLAG_SELF_SHADOWS;
				}
			}
		}
		p_details = pTextureProjectionDetailsTable->IterateNext();
	}
//
//	// Pixel shader override no longer required.
//	EngineGlobals.pixel_shader_override = 0;
//	
//	// Restore the view and projection transforms.
//	EngineGlobals.view_matrix		= stored_view_matrix;
//	EngineGlobals.projection_matrix	= stored_projection_matrix;
//	EngineGlobals.is_orthographic	= false;
//
//	// Restore the world transform.
//	D3DDevice_SetTransform( D3DTS_WORLD, &EngineGlobals.world_matrix );
//	
//	// Restore the default render target.
//	D3DDevice_SetRenderTarget( EngineGlobals.p_RenderSurface, EngineGlobals.p_ZStencilSurface );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void render_shadow_meshes( sScene *p_scene, int *p_mesh_indices, int num_meshes )
{
}


void figure_wibble_vc( sScene *p_scene )
{
	if ( !p_scene->mp_scene_data ) return;

	// The vertex color wibble flag is placed in pass 0.
	NxNgc::sMaterialVCWibbleKeyHeader *	p_key_header	= p_scene->mp_vc_wibble; 
	NxNgc::sMaterialVCWibbleKey *		p_key			= (NxNgc::sMaterialVCWibbleKey *)&p_key_header[1]; 

	int current_time = (int)Tmr::GetTime();

	uint32 * p_col = p_scene->mp_col_pool;

	for ( int wibble = 0; wibble < p_scene->mp_scene_data->m_num_vc_wibbles; wibble++ )
	{
		int						num_keys		= p_key_header->m_num_frames;
		int						phase_shift		= p_key_header->m_phase;

		// Time parameters.
		int						start_time		= p_key[0].m_time;
		int						end_time		= p_key[(num_keys - 1)].m_time;
		int						period			= end_time - start_time;
		int						time			= start_time + (( current_time + phase_shift ) % period );

		// Locate the keyframe.
		int key;
		for( key = num_keys - 1; key >= 0; --key )
		{
			if( time >= p_key[key].m_time )
			{
				break;
			}
		}
		
		// Parameter expressing how far we are between between this keyframe and the next.
		float t = (float)( time - p_key[key].m_time ) / (float)( p_key[key+1].m_time - p_key[key].m_time );

		// Interpolate the color.
		GXColor	rgba;
		rgba.r = (uint8)((( 1.0f - t ) * p_key[key].m_color.r ) + ( t * p_key[key + 1].m_color.r ));
		rgba.g = (uint8)((( 1.0f - t ) * p_key[key].m_color.g ) + ( t * p_key[key + 1].m_color.g ));
		rgba.b = (uint8)((( 1.0f - t ) * p_key[key].m_color.b ) + ( t * p_key[key + 1].m_color.b ));
		rgba.a = (uint8)((( 1.0f - t ) * p_key[key].m_color.a ) + ( t * p_key[key + 1].m_color.a ));

		p_col[wibble] = *((uint32*)&rgba);

		// Next wibble sequence.
		p_key_header	= (NxNgc::sMaterialVCWibbleKeyHeader *)&p_key[num_keys];
		p_key			= (NxNgc::sMaterialVCWibbleKey *)&p_key_header[1]; 
	}
}

			//sMaterialHeader * p_mat = p_mesh->mp_dl->m_material.p_header;
			//sMaterialPassHeader * p_pass = (sMaterialPassHeader *)&p_mat[1];

// Cached GX commands.

float *	g_p_pos		= NULL;
char *	g_p_col0	= NULL;
//char *	g_p_col1	= NULL;
GXColor g_col0		= (GXColor){0,0,0,0};
//GXColor g_col1		= (GXColor){0,0,0,0};

static void _reset( void )
{
	g_p_pos		= NULL;
	g_p_col0	= NULL;
//	g_p_col1	= NULL;
	g_col0		= (GXColor){0,0,0,0};
//	g_col1		= (GXColor){0,0,0,0};
	GX::SetChanMatColor( GX_COLOR0A0, g_col0 );
//	GX::SetChanMatColor( GX_COLOR0A0, g_col1 );
}

static void _set_pos( float * p_pos )
{
	if ( g_p_pos != p_pos )
	{
		GX::SetArray( GX_VA_POS, p_pos, sizeof( float ) * 3 );
		g_p_pos = p_pos;
	}
}

static void _set_col0( char * p_col0 )
{
	if ( g_p_col0 != p_col0 )
	{
		GX::SetArray( GX_VA_CLR0, p_col0, sizeof( uint32 ) );
		g_p_col0 = p_col0;
	}
}

//static void _set_col1( char * p_col1 )
//{
//	if ( g_p_col1 != p_col1 )
//	{
//		GX::SetArray( GX_VA_CLR1, p_col1, sizeof( uint32 ) );
//		g_p_col1 = p_col1;
//	}
//}

static void _set_mat0( GXColor col )
{
	if ( ( col.r != g_col0.r ) || ( col.g != g_col0.g ) || ( col.b != g_col0.b ) || ( col.a != g_col0.a ) )
	{
		g_col0 = col;
		GX::SetChanMatColor( GX_COLOR0A0, col );
	}
}

//static void _set_mat1( GXColor col )
//{
//	if ( ( col.r != g_col1.r ) || ( col.g != g_col1.g ) || ( col.b != g_col1.b ) || ( col.a != g_col1.a ) )
//	{
//		g_col1 = col;
//		GX::SetChanMatColor( GX_COLOR1A1, col );
//	}
//}

static void submit_mesh( sScene *p_scene, sMesh * p_mesh, s16* p_posNormBuffer, Mth::Matrix * p_bone_xform, Mth::Matrix * p_instance_xform, int * p_array_base, int * p_col_base, int * p_layer, float * p_pm, GXColor * p_base_color, bool * set_base_color, bool * p_world_fog, uint32 flags )
{
//	if ( g_skip_correctable && p_mesh->mp_dl->m_material.p_header->m_flags & (1<<6) ) return;
	if ( (int)p_mesh->mp_dl->m_material.p_header->m_layer_id == g_kill_layer ) return;

	if ( !p_posNormBuffer )
	{
		// Deal with cloned pos pools.
		if ( p_mesh->mp_dl->mp_pos_pool )
		{
			_set_pos( p_mesh->mp_dl->mp_pos_pool );
		}
		else
		{
			// Deal with verts outside of 65535.
			_set_pos( &p_scene->mp_pos_pool[p_mesh->mp_dl->m_array_base*3] );
		}
	}

	// Deal with cloned color pools.
	if ( p_mesh->mp_dl->mp_col_pool )
	{
		_set_col0( &((char*)p_mesh->mp_dl->mp_col_pool)[0] );
//		if ( p_mesh->mp_dl->m_material.p_header->m_flags & (1<<6) )
//		{
//			_set_col1( &((char*)p_mesh->mp_dl->mp_col_pool)[2] );
//		}
	}
	else
	{
		// Deal with resetting cloned color pools.
		// Assumes that this isn't a skinned object.
		_set_col0( &((char*)p_scene->mp_col_pool)[0] );
//		if ( p_mesh->mp_dl->m_material.p_header->m_flags & (1<<6) )
//		{
//			_set_col1( &((char*)p_scene->mp_col_pool)[2] );
//		}
	}

	// Deal with zbias.
	if ( (int)p_mesh->mp_dl->m_material.p_header->m_layer_id != *p_layer )
	{
		*p_layer = p_mesh->mp_dl->m_material.p_header->m_layer_id;

		float layer = p_mesh->mp_dl->m_material.p_header->m_layer_id;

		float value = p_pm[6] + ( 150.0f * layer ) * p_pm[5];

		GXWGFifo.u8 = GX_LOAD_XF_REG;
		GXWGFifo.u16 = 0;
		GXWGFifo.u16 = 0x1025;
		GXWGFifo.f32 = value;
	}



	// Billboard tweaking.
//	enum BillboardType
//	{
//		BBT_NOT_BILLBOARD,
//		BBT_SCREEN_ALIGNED,
//		BBT_AXIAL_ALIGNED,
//	};
	
//	if ( p_mesh->mp_mod )
//	{
//		Mtx t;
//		Mtx m;
//
////		MTXTrans( m, p_mesh->mp_mod.offset->GetX(), p_mesh->mp_mod.offset->GetY(), p_mesh->mp_mod.offset->GetZ() );
////
////		MTXRotAxisDeg( 
//
//		float w = p_mesh->mp_mod->offset[W];
//		Mth::ERot90 rot = *((Mth::ERot90*)&w);
//
//		if ( p_mesh->mp_mod->scale[X] > 1.0f ) OSReport( "X Scale > 1.0f: %8.3f\n", p_mesh->mp_mod->scale[X] );
//		if ( p_mesh->mp_mod->scale[Y] > 1.0f ) OSReport( "Y Scale > 1.0f: %8.3f\n", p_mesh->mp_mod->scale[Y] );
//		if ( p_mesh->mp_mod->scale[Z] > 1.0f ) OSReport( "Z Scale > 1.0f: %8.3f\n", p_mesh->mp_mod->scale[Z] );
//
//		switch ( rot )
//		{
//			default:
//			case Mth::ROT_0:
//				MTXScaleApply( g_matrix0, t, p_mesh->mp_mod->scale[X], p_mesh->mp_mod->scale[Y], p_mesh->mp_mod->scale[Z] );
//				break;
//			case Mth::ROT_90:
//				MTXScaleApply( g_matrix90, t, p_mesh->mp_mod->scale[X], p_mesh->mp_mod->scale[Y], p_mesh->mp_mod->scale[Z] );
//				break;
//			case Mth::ROT_180:
//				MTXScaleApply( g_matrix180, t, p_mesh->mp_mod->scale[X], p_mesh->mp_mod->scale[Y], p_mesh->mp_mod->scale[Z] );
//				break;
//			case Mth::ROT_270:
//				MTXScaleApply( g_matrix270, t, p_mesh->mp_mod->scale[X], p_mesh->mp_mod->scale[Y], p_mesh->mp_mod->scale[Z] );
//				break;
//		}
//		t[0][3] = p_mesh->mp_mod->offset[X];
//		t[1][3] = p_mesh->mp_mod->offset[Y];
//		t[2][3] = p_mesh->mp_mod->offset[Z];
//		MTXConcat( EngineGlobals.current_uploaded, t, m );
//
//		GX::LoadPosMtxImm( m, GX_PNMTX0 );
//		reload_camera = true;
//	}

	// Set Fog color if necessary.
	bool mesh_world_fog = true;
	if ( p_mesh->mp_dl->m_material.p_header )
	{
		switch ( p_mesh->mp_dl->m_material.p_header->m_base_blend )
		{
			case vBLEND_MODE_ADD:
			case vBLEND_MODE_ADD_FIXED:
			case vBLEND_MODE_SUBTRACT:
			case vBLEND_MODE_SUB_FIXED:
				mesh_world_fog = false;
				break;
			default:
				break;
		}
	}
	if ( mesh_world_fog != *p_world_fog )
	{
		*p_world_fog = mesh_world_fog;
		if ( mesh_world_fog )
		{
			Nx::CFog::sSetFogColor();
		}
		else
		{
			GX::SetFogColor( (GXColor){0,0,0,0} );
		}
	}


	MaterialSubmit( p_mesh, p_scene );

	switch ( p_mesh->mp_dl->mp_object_header->m_billboard_type )
	{
		case 2:
			{
				NsMatrix*	p_matrix	= &EngineGlobals.world_to_camera;
				NsVector	at = *p_matrix->getAt();
//				at.x = -at.x;
//				at.y = -at.y;
//				at.z = -at.z;

				NsVector screen_right;
				NsVector screen_up;
				NsVector screen_at;

				NsMatrix	root;

				// Create coordinate system based on camera at & pivot axis.
				screen_up.set( p_mesh->mp_dl->mp_object_header->m_axis[X], p_mesh->mp_dl->mp_object_header->m_axis[Y], p_mesh->mp_dl->mp_object_header->m_axis[Z] );
//				screen_right.cross( *p_matrix->getAt(), screen_up );
				screen_right.cross( at, screen_up );
				screen_at.cross( screen_up, screen_right );

				screen_right.normalize();
				screen_at.normalize();

				root.setRightX( screen_right.x );
				root.setRightY( screen_up.x    );
				root.setRightZ( screen_at.x    );

				root.setUpX( screen_right.y );
				root.setUpY( screen_up.y    );
				root.setUpZ( screen_at.y    );

				root.setAtX( screen_right.z );
				root.setAtY( screen_up.z    );
				root.setAtZ( screen_at.z    );

//				root.setRightX( screen_right.x );
//				root.setRightY( screen_right.y );
//				root.setRightZ( screen_right.z );
//
//				root.setUpX( screen_up.x );
//				root.setUpY( screen_up.y );
//				root.setUpZ( screen_up.z );
//
//				root.setAtX( screen_at.x );
//				root.setAtY( screen_at.y );
//				root.setAtZ( screen_at.z );

				root.setPosX( p_mesh->mp_dl->mp_object_header->m_origin[X] );
				root.setPosY( p_mesh->mp_dl->mp_object_header->m_origin[Y] );
				root.setPosZ( p_mesh->mp_dl->mp_object_header->m_origin[Z] );

				root.cat( EngineGlobals.world_to_camera, root );

				GX::LoadPosMtxImm( (MtxPtr)&root, GX_PNMTX0 );
				//GXSetCurrentMtx( GX_PNMTX1 );
				reload_camera = true;
			}
			break;
		case 1:
			{
				NsMatrix*	p_matrix	= &EngineGlobals.world_to_camera;

				NsMatrix	root;

				// Just transpose the matrix for screen-facing.
				root.setRightX( p_matrix->getRightX() );
				root.setRightY( p_matrix->getUpX()    );
				root.setRightZ( p_matrix->getAtX()    );

				root.setUpX( p_matrix->getRightY() );
				root.setUpY( p_matrix->getUpY()    );
				root.setUpZ( p_matrix->getAtY()    );

				root.setAtX( p_matrix->getRightZ() );
				root.setAtY( p_matrix->getUpZ()    );
				root.setAtZ( p_matrix->getAtZ()    );

				root.setPosX( p_mesh->mp_dl->mp_object_header->m_origin[X] );
				root.setPosY( p_mesh->mp_dl->mp_object_header->m_origin[Y] );
				root.setPosZ( p_mesh->mp_dl->mp_object_header->m_origin[Z] );

				root.cat( EngineGlobals.world_to_camera, root );

				GX::LoadPosMtxImm( (MtxPtr)&root, GX_PNMTX0 );
				//GXSetCurrentMtx( GX_PNMTX1 );
				reload_camera = true;
			}
			break;
		default:
			// Don't do anything - not a billboard.
			//GXSetCurrentMtx( GX_PNMTX0 );
			break;
	}

//	// Create array of bone matrices if required.
//	NsMatrix	bone_xform[80];
//	if ( p_scene->m_numHierarchyObjects && p_bone_xform )
//	{
//		for ( int bone = 0; bone < num_bones; bone++ )
//		{
//			Mth::Matrix temp = p_bone_xform[bone] * *p_instance_xform;
//			NsMatrix	local;
//
//			local.setRightX( temp.GetRight()[X] );
//			local.setRightY( temp.GetUp()[X] );
//			local.setRightZ( temp.GetAt()[X] );
//
//			local.setUpX( temp.GetRight()[Y] ); 
//			local.setUpY( temp.GetUp()[Y] );    
//			local.setUpZ( temp.GetAt()[Y] );    
//
//			local.setAtX( temp.GetRight()[Z] ); 
//			local.setAtY( temp.GetUp()[Z] );    
//			local.setAtZ( temp.GetAt()[Z] );    
//
//			local.setPosX( temp.GetPos()[X] );
//			local.setPosY( temp.GetPos()[Y] );
//			local.setPosZ( temp.GetPos()[Z] );
//
//			bone_xform[bone].cat( EngineGlobals.world_to_camera, local );
//		}
//	}


	if ( p_scene->m_numHierarchyObjects && p_bone_xform )
	{
		NsMatrix	bone_xform;
		Mth::Matrix temp = p_bone_xform[p_mesh->m_bone_idx] * *p_instance_xform;
		NsMatrix	local;

		local.setRightX( temp.GetRight()[X] );
		local.setRightY( temp.GetUp()[X] );
		local.setRightZ( temp.GetAt()[X] );

		local.setUpX( temp.GetRight()[Y] ); 
		local.setUpY( temp.GetUp()[Y] );    
		local.setUpZ( temp.GetAt()[Y] );    

		local.setAtX( temp.GetRight()[Z] ); 
		local.setAtY( temp.GetUp()[Z] );    
		local.setAtZ( temp.GetAt()[Z] );    

		local.setPosX( temp.GetPos()[X] );
		local.setPosY( temp.GetPos()[Y] );
		local.setPosZ( temp.GetPos()[Z] );

		bone_xform.cat( EngineGlobals.world_to_camera, local );

		GX::LoadPosMtxImm( (MtxPtr)&bone_xform, GX_PNMTX0 );
		reload_camera = true;

		GX::LoadNrmMtxImm((MtxPtr)&bone_xform, GX_PNMTX0);

	}

	if ( /*p_scene->mp_scene_data->m_num_nrm ||*/ p_posNormBuffer || ( p_scene->m_numHierarchyObjects && p_bone_xform ) )
	{
//		GXLightObj light_obj[3];
//
//		// Always set ambient & diffuse colors.
//		GX::SetChanAmbColor( GX_COLOR0A0, NxNgc::EngineGlobals.ambient_light_color );
////		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){128,128,128,255} );
////				GX::SetChanAmbColor( GX_COLOR1A1, NxNgc::EngineGlobals.ambient_light_color );
//	
//		GX::InitLightColor( &light_obj[0], NxNgc::EngineGlobals.diffuse_light_color[0] );
//		GX::InitLightPos( &light_obj[0], NxNgc::EngineGlobals.light_x[0], NxNgc::EngineGlobals.light_y[0], NxNgc::EngineGlobals.light_z[0] );
//		GX::LoadLightObjImm( &light_obj[0], GX_LIGHT0 );
//	
//		GX::InitLightColor( &light_obj[1], NxNgc::EngineGlobals.diffuse_light_color[1] );
//		GX::InitLightPos( &light_obj[1], NxNgc::EngineGlobals.light_x[1], NxNgc::EngineGlobals.light_y[1], NxNgc::EngineGlobals.light_z[1] );
//		GX::LoadLightObjImm( &light_obj[1], GX_LIGHT1 );
//
//		// Set channel control for diffuse.
//		GX::SetChanCtrl( GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT0|GX_LIGHT1, GX_DF_CLAMP, GX_AF_NONE );
//		GX::SetChanCtrl( GX_ALPHA0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE ); 
//
////		if ( p_mesh->mp_dl->m_material.p_header->m_shininess > 0.0f )
////		{
////			// Set channel control for specular.
////			Vec  ldir;
////
////			ldir.x = 0.0f;	//NxNgc::EngineGlobals.light_x[0] * ( 1.0f / 200000.0f );
////			ldir.y = 0.0f;  //NxNgc::EngineGlobals.light_y[0] * ( 1.0f / 200000.0f );
////			ldir.z = -1.0f;  //NxNgc::EngineGlobals.light_z[0] * ( 1.0f / 200000.0f );
////
////			GX::InitSpecularDirv( &light_obj[2], &ldir );
////			GX::InitLightShininess( &light_obj[2], 8.0f );
////			GX::InitLightColor( &light_obj[2], (GXColor){255,255,255,255} );
////			GX::LoadLightObjImm( &light_obj[2], GX_LIGHT2 );
////
////			GX::SetChanCtrl( GX_COLOR1, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT2, GX_DF_NONE, GX_AF_SPEC );
////			GX::SetChanCtrl( GX_ALPHA1, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE ); 
////		}
//
	}
	else
	{
		if ( !*set_base_color )
		{
			GX::SetChanCtrl( GX_COLOR0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
			GX::SetChanCtrl( GX_ALPHA0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE ); 
////			if ( p_mesh->mp_dl->m_material.p_header->m_flags & (1<<6) )
//			{
//				GX::SetChanCtrl( GX_COLOR1, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//				GX::SetChanCtrl( GX_ALPHA1, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE ); 
//			}
			*set_base_color = true;
		}
//		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){128,128,128,255} );
//		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){32,32,32,255} );
		GXColor col;
		col.r = p_mesh->m_base_color.r;
		col.g = p_mesh->m_base_color.g;
		col.b = p_mesh->m_base_color.b;
		col.a = 255;

////		if ( !(p_mesh->mp_dl->m_material.p_header->m_flags & (1<<7)) )
//		if ( !(flags & vRENDER_LIT) )
//		{
//			// Not sure why we need to force this to be set...
////			_set_mat0( col );
////			GX::SetChanMatColor( GX_COLOR0A0, col );
////			if ( p_mesh->mp_dl->m_material.p_header->m_flags & (1<<6) )
//			{
////				_set_mat1( col );
////				GX::SetChanMatColor( GX_COLOR1A1, col );
//			}
////			*((uint32*)p_base_color) = *((uint32*)&col);
//
//			col.r = col.r >> 1;
//			col.g = col.g >> 1;
//			col.b = col.b >> 1;
//		}

//		if ( *((uint32*)p_base_color) !=  *((uint32*)&col) )
		{
//			p_mesh->m_base_color.a = 255;
//			GX::SetChanMatColor( GX_COLOR0A0, col );

			_set_mat0( col );
//			if ( p_mesh->mp_dl->m_material.p_header->m_flags & (1<<6) )
//			{
////				GX::SetChanMatColor( GX_COLOR1A1, col );
//				_set_mat1( (GXColor){col.b,col.a,0,0} );
//			}
//			*((uint32*)p_base_color) = *((uint32*)&col);
		}

//		GX::SetChanCtrl( GX_COLOR1A1, GX_DISABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//		GX::SetChanMatColor( GX_COLOR1A1, (GXColor){128,128,128,255} );
//		GX::SetChanAmbColor( GX_COLOR1A1, (GXColor){128,128,128,255} );
//		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){64,64,64,128} );
//		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){64,64,64,128} );
	}

	// Major hack!!!
//   if ( p_posNormBuffer )
//	{
//		unsigned char * p8 = (unsigned char *)&p_mesh->mp_dl[1];
//		for ( uint lp = 0; lp < p_mesh->mp_dl->m_size; lp++ ) GXWGFifo.u8 = *p8++;
//	}
//	else
//	{
	/*if ( !p_mesh->mp_dl->mp_pos_pool )*/ GX::CallDisplayList ( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size );
//	}

	if ( reload_camera )
	{
		GX::LoadPosMtxImm( EngineGlobals.current_uploaded, GX_PNMTX0 );
//		GX::LoadPosMtxImm( (Mtx)current_cam.getCurrent(), GX_PNMTX0 );
		reload_camera = false;
	}

//	// Reload color texture if necessary.
//	if ( p_mesh->mp_dl->m_material.p_header->m_flags & (1<<5) )
//	{
//		// Make sure color map is uploaded.
//		GX::UploadTexture(	colorMap,
//							64,	//COLOR_MAP_SIZE,
//							64, //COLOR_MAP_SIZE,
//							GX_TF_RGBA8,
//							GX_CLAMP,
//							GX_CLAMP,
//							GX_FALSE,
//							GX_LINEAR,
//							GX_LINEAR,
//							0.0f,
//							0.0f,
//							0.0f,
//							GX_FALSE,
//							GX_TRUE,
//							GX_ANISO_1,
//							GX_TEXMAP7 ); 
//	}
}

void render_scene( sScene *p_scene, s16* p_posNormBuffer, uint32 flags, Mth::Matrix * p_bone_xform, Mth::Matrix * p_instance_xform, int num_bones )
{
	// Don't render dictionary scenes.
	if( p_scene->m_is_dictionary )
	{
		return;
	}
	
//	// This forces the texture to go into the unusable region.
//	GX::UploadTexture(	colorMap,
//						64,
//						64,
//						GX_TF_RGBA8,
//						GX_CLAMP,
//						GX_CLAMP,
//						GX_FALSE,
//						GX_LINEAR,
//						GX_LINEAR,
//						0.0f,
//						0.0f,
//						0.0f,
//						GX_FALSE,
//						GX_TRUE,
//						GX_ANISO_1,
//						GX_TEXMAP7 ); 

	sLastUV = NULL;

	first_mat = true;

	p_last_material = NULL;
	last_correct = false;
//	if ( p_scene->m_numHierarchyObjects == 0 )
//	{
//		// Not hierarchical, set to matrix 0.
//		GXSetCurrentMtx( GX_PNMTX0 );
//	}

	ResetMaterialChange();
//	int meshes_rendered		= 0;

	figure_wibble_vc( p_scene );

#define MAX_SHADOW_MESHES 512

//	int shadow_meshes = 0;
//	sMesh *pShadow[MAX_SHADOW_MESHES];

//	sMesh	*visible_mesh_array[2000];
//	int		visible_mesh_array_index = 0;

	// Increment the scene framestamp.
//_scene->m_framestamp++;

	// Set up the texture generation matrix here.
//	Mtx light;
//	MTXLightOrtho( light, -128.0f, 128.0f, -128.0f, 128.0f, 0.5f, 0.5f, 0.5f, 0.5f );
//	MTXConcat( light, (Mtx)&EngineGlobals.shadow_camera, light );
//#ifndef SHORT_VERT
//	GX::LoadTexMtxImm( light, GX_TEXMTX7, GX_MTX3x4 );
//#endif		// SHORT_VERT

	if ( p_posNormBuffer )
	{
		GX::SetArray( GX_VA_POS,  p_posNormBuffer, sizeof( uint16 ) * 6 );
		GX::SetArray( GX_VA_NRM,  &p_posNormBuffer[3], sizeof( uint16 ) * 6 );
	}
	else
	{
		GX::SetArray( GX_VA_POS,  p_scene->mp_pos_pool, sizeof( float ) * 3 );
		GX::SetArray( GX_VA_NRM,  p_scene->mp_nrm_pool, sizeof( uint16 ) * 3 );
	}
	int array_base = 0;
	int col_base = 2;
	int layer = -1;
	bool set_chan = false;
	GXColor base_color = { 0, 0, 0, 0 };
	bool world_fog = true;
	Nx::CFog::sSetFogColor();
	GX::SetChanMatColor( GX_COLOR0A0, base_color );
	GX::SetChanMatColor( GX_COLOR1A1, base_color );
	f32 pm[GX_PROJECTION_SZ];
	GX::GetProjectionv( pm );


//	if ( flags & vRENDER_NEW_TEST )
	{
		// Pool setup.
		GX::SetArray( GX_VA_CLR0, &((char*)p_scene->mp_col_pool)[0], sizeof( uint32 ) );
		GX::SetArray( GX_VA_CLR1, &((char*)p_scene->mp_col_pool)[2], sizeof( uint32 ) );

//		GX::SetArray( GX_VA_CLR1, p_scene->mp_col_pool, sizeof( uint32 ) );
//		GX::SetArray( GX_VA_TEX0, p_scene->mp_tex_pool, sizeof( float ) * 2 );
//
//		GX::SetArray( GX_VA_TEX1, p_scene->mp_tex_pool, sizeof( float ) * 2 );
//		GX::SetArray( GX_VA_TEX2, p_scene->mp_tex_pool, sizeof( float ) * 2 );
//		GX::SetArray( GX_VA_TEX3, p_scene->mp_tex_pool, sizeof( float ) * 2 );

		GX::SetArray( GX_VA_TEX0, p_scene->mp_tex_pool, sizeof( uint16 ) * 2 );
		GX::SetArray( GX_VA_TEX1, p_scene->mp_tex_pool, sizeof( uint16 ) * 2 );
		GX::SetArray( GX_VA_TEX2, p_scene->mp_tex_pool, sizeof( uint16 ) * 2 );
		GX::SetArray( GX_VA_TEX3, p_scene->mp_tex_pool, sizeof( uint16 ) * 2 );

//		GXSetArray( GX_VA_TEX4, p_scene->mp_tex_pool, sizeof( float ) * 2 );
//		GXSetArray( GX_VA_TEX5, p_scene->mp_tex_pool, sizeof( float ) * 2 );
//		GXSetArray( GX_VA_TEX6, p_scene->mp_tex_pool, sizeof( float ) * 2 );
//		GXSetArray( GX_VA_TEX7, p_scene->mp_tex_pool, sizeof( float ) * 2 );
		//GXSetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
	}





	if ( g_object == g_view_object ) g_view_scene = p_scene;

	// Render all meshes.
#	ifdef __USE_PROFILER__
						Sys::VUProfiler->PushContext( 128,0,0 );
#	endif		// __USE_PROFILER__

//						{
//							GX::SetFog( GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, (GXColor){0,0,0,0} );
//							GX::SetTevOrder( GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0 );
//
//							GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_KONST,
//												 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//
//							GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST,
//													 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
//													 GX_TEV_SWAP0, GX_TEV_SWAP0 );
//
//							GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
//
//							GX::SetTevKSel( GX_TEVSTAGE0, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A );
//							GX::SetTevKColor( GX_KCOLOR0, (GXColor){8,8,8,255} );
//							GX::SetChanCtrl( GX_ALPHA0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE ); 
//							GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255});
//							GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255});
//
//							GX::SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
//							GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE ); 
//						}

	///////////////////////////////////////////////////////////////////////////////////////////
	// Shadow Target.
	///////////////////////////////////////////////////////////////////////////////////////////

	if ( flags & vRENDER_SHADOW_1ST_PASS )
	{
//		GX::SetFog( GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, (GXColor){0,0,0,0} );
		GX::SetTevOrder( GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0 );

		GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_KONST,
							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );

		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST,
								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
								 GX_TEV_SWAP0, GX_TEV_SWAP0 );

		GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );		// Replace

		GX::SetTevKSel( GX_TEVSTAGE0, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A );
		GX::SetTevKColor( GX_KCOLOR0, (GXColor){64,64,64,255} );
		GX::SetChanCtrl( GX_ALPHA0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE ); 
		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255});
		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255});
		GX::SetChanMatColor( GX_COLOR1A1, (GXColor){255,255,255,255});
		GX::SetChanAmbColor( GX_COLOR1A1, (GXColor){255,255,255,255});
		base_color = (GXColor){ 255, 255, 255, 255 };

//		GX::SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE ); 

		for ( int mesh = 0; mesh < p_scene->m_num_filled_meshes; mesh++ )
		{
			sMesh * p_mesh = p_scene->mpp_mesh_list[mesh];

			if ( ( p_mesh->m_flags & sMesh::MESH_FLAG_VISIBLE ) &&
				 ( ( g_object != g_view_object ) || ( &p_scene->mp_material_header[g_material] != p_mesh->mp_dl->m_material.p_header ) ) )
			{
				GX::CallDisplayList ( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size );


//				submit_mesh( p_scene, p_mesh, p_posNormBuffer, p_bone_xform, p_instance_xform, &array_base, &col_base, &layer, pm, &world_fog, flags );
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// Opaque meshes.
	///////////////////////////////////////////////////////////////////////////////////////////

	_reset();
	if ( flags & vRENDER_OPAQUE )
	{
//		GX::SetZMode( GX_TRUE, GX_LEQUAL, GX_TRUE );
		for ( int mesh = 0; mesh < p_scene->m_num_opaque_meshes; mesh++ )
		{
			sMesh * p_mesh = p_scene->mpp_mesh_list[mesh];

			if ( ( p_mesh->m_flags & sMesh::MESH_FLAG_VISIBLE ) &&
				 ( ( g_object != g_view_object ) || ( &p_scene->mp_material_header[g_material] != p_mesh->mp_dl->m_material.p_header ) ) )
			{
				submit_mesh( p_scene, p_mesh, p_posNormBuffer, p_bone_xform, p_instance_xform, &array_base, &col_base, &layer, pm, &base_color, &set_chan, &world_fog, flags );
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// Semitransparent meshes.
	///////////////////////////////////////////////////////////////////////////////////////////

	if ( flags & vRENDER_SEMITRANSPARENT )
	{
//		GX::SetZMode( GX_TRUE, GX_LEQUAL, GX_TRUE );
		int opaque		= p_scene->m_num_opaque_meshes;
		int presemi		= opaque + p_scene->m_num_pre_semitrans_meshes;
		int dynsemi		= presemi + p_scene->m_num_dynamic_semitrans_meshes;
		int postsemi	= dynsemi + p_scene->m_num_post_semitrans_meshes;

		// Pre meshes.
		_reset();
		for ( int mesh = opaque; mesh < presemi; mesh++ )
		{
			sMesh * p_mesh = p_scene->mpp_mesh_list[mesh];

			if ( ( p_mesh->m_flags & sMesh::MESH_FLAG_VISIBLE ) &&
				 ( ( g_object != g_view_object ) || ( &p_scene->mp_material_header[g_material] != p_mesh->mp_dl->m_material.p_header ) ) )
			{
				submit_mesh( p_scene, p_mesh, p_posNormBuffer, p_bone_xform, p_instance_xform, &array_base, &col_base, &layer, pm, &base_color, &set_chan, &world_fog, flags );
			}
		}

		// Dynamic meshes.
		sSortedMeshEntry sortedMeshArray[1000];
		int next_sorted_mesh_entry	= 0;

		for ( int mesh = presemi; mesh < dynsemi; mesh++ )
		{
			sMesh * p_mesh = p_scene->mpp_mesh_list[mesh];

			if ( ( p_mesh->m_flags & sMesh::MESH_FLAG_VISIBLE ) &&
				 ( ( g_object != g_view_object ) || ( &p_scene->mp_material_header[g_material] != p_mesh->mp_dl->m_material.p_header ) ) )
			{
				sortedMeshArray[next_sorted_mesh_entry].p_mesh	= p_mesh;
				sortedMeshArray[next_sorted_mesh_entry].sort	= frustum_sort_mid( p_mesh->mp_dl->m_sphere );
				++next_sorted_mesh_entry;
			}
		}
		qsort( sortedMeshArray, next_sorted_mesh_entry, sizeof( sSortedMeshEntry ), cmp );

		_reset();
		for( int m = 0; m < next_sorted_mesh_entry; ++m )
		{
			submit_mesh( p_scene, sortedMeshArray[m].p_mesh, p_posNormBuffer, p_bone_xform, p_instance_xform, &array_base, &col_base, &layer, pm, &base_color, &set_chan, &world_fog, flags );
		}

		// Post meshes.
		_reset();
		for ( int mesh = dynsemi; mesh < postsemi; mesh++ )
		{
			sMesh * p_mesh = p_scene->mpp_mesh_list[mesh];

			if ( ( p_mesh->m_flags & sMesh::MESH_FLAG_VISIBLE ) &&
				 ( ( g_object != g_view_object ) || ( &p_scene->mp_material_header[g_material] != p_mesh->mp_dl->m_material.p_header ) ) )
			{
				submit_mesh( p_scene, p_mesh, p_posNormBuffer, p_bone_xform, p_instance_xform, &array_base, &col_base, &layer, pm, &base_color, &set_chan, &world_fog, flags );
			}
		}
	}



	///////////////////////////////////////////////////////////////////////////////////////////
	// Shadow Mesh.
	///////////////////////////////////////////////////////////////////////////////////////////

	if ( flags & vRENDER_SHADOW_2ND_PASS )
	{
		Nx::CFog::sEnableFog( false );
		Mtx light;
		MTXLightOrtho( light, -128.0f, 128.0f, -128.0f, 128.0f, 0.5f, 0.5f, 0.5f, 0.5f );
		MTXConcat( light, (Mtx)&EngineGlobals.shadow_camera, light );
		GX::LoadTexMtxImm( light, GX_TEXMTX7, GX_MTX3x4 );
		GX::SetZMode( GX_TRUE, GX_LEQUAL, GX_FALSE );

		GX::SetZCompLoc( GX_TRUE );
		GX::UploadTexture(  shadowTextureData,
							SHADOW_TEXTURE_SIZE,
							SHADOW_TEXTURE_SIZE,
							GX_TF_I4,
							GX_CLAMP,
							GX_CLAMP,
							GX_FALSE,
							GX_LINEAR,
							GX_LINEAR,
							0.0f,
							0.0f,
							0.0f,
							GX_FALSE,
							GX_TRUE,
							GX_ANISO_1,
							GX_TEXMAP0 ); 
		GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, SHADOW_TEXTURE_SIZE, SHADOW_TEXTURE_SIZE );

		GX::UploadPalette( shadowPalette,
						   GX_TL_RGB5A3,
						   GX_TLUT_16,
						   GX_TEXMAP0 );

		GX::SetTexChanTevIndCull( 1, 0, 1, 0, GX_CULL_NONE );
		GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, GX_FALSE, GX_PTIDENTITY );

		GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

		GX::SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0 );
//		GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );		// Replace
		GX::SetBlendMode( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
		GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_TEXMTX7, GX_TEXMTX7, GX_TEXMTX7, GX_TEXMTX7 );

		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV,
								 GX_TEV_SWAP0, GX_TEV_SWAP0 );
		GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ONE, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ZERO,
							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );


//out_reg = (d (op) ((1.0 - c)*a + c*b) + bias) * scale;

		GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );

//		visible_mesh_array[visible_mesh_array_index++] = p_scene->m_semitransparent_meshes[e];
//		for ( int mesh = 0; mesh < p_scene->m_num_filled_meshes; mesh++ )

		layer = 5;		// Force subsequent materials to reset the layer.

//		float value = pm[6] + ( 80.0f * p_mesh->mp_dl->m_material.p_header->m_layer_id ) * pm[5];
		float value = pm[6] + ( 150.0f * 5 ) * pm[5];

		GXWGFifo.u8 = GX_LOAD_XF_REG;
		GXWGFifo.u16 = 0;
		GXWGFifo.u16 = 0x1025;
		GXWGFifo.f32 = value;

//		for ( int mesh = 0; mesh < ( p_scene->m_num_opaque_meshes + p_scene->m_num_pre_semitrans_meshes ); mesh++ )
		for ( int mesh = 0; mesh < ( p_scene->m_num_filled_meshes ); mesh++ )

		{
			sMesh * p_mesh = p_scene->mpp_mesh_list[mesh];

//			if( frustum_check_sphere( light, p_scene->m_opaque_meshes[e]->m_sphere ))
			float by;

//			if ( p_mesh->mp_mod )
//			{
////				by = ( p_mesh->m_bottom_y * p_mesh->mp_mod->scale[Y] ) + p_mesh->mp_mod->offset[Y];
//				by = p_mesh->mp_mod->offset[Y];
//			}
//			else
//			{
				by = p_mesh->m_bottom_y;
//			}

			if ( ( p_mesh->m_flags & sMesh::MESH_FLAG_VISIBLE ) &&
				 frustum_check_sphere( light, p_mesh->mp_dl->m_sphere ) &&
				 ( g_shadow_object_pos[Y] > by ) &&
				 !( p_mesh->m_flags & sMesh::MESH_FLAG_NO_SKATER_SHADOW ) )
			{

//				if ( (int)p_mesh->mp_dl->m_material.p_header->m_layer_id == g_kill_layer ) return;

				// Deal with cloned pos pools.
				if ( p_mesh->mp_dl->mp_pos_pool )
				{
					array_base = -1;
					GX::SetArray( GX_VA_POS, p_mesh->mp_dl->mp_pos_pool, sizeof( float ) * 3 );
				}
				else
				{
					// Deal with verts outside of 65535.
					if ( p_mesh->mp_dl->m_array_base != (uint32)array_base )
					{
						array_base = p_mesh->mp_dl->m_array_base;

						// Assumes that this isn't a skinned object.
						GX::SetArray( GX_VA_POS, &p_scene->mp_pos_pool[p_mesh->mp_dl->m_array_base*3], sizeof( float ) * 3 );
					}
				}

				GX::CallDisplayList ( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size );


//				submit_mesh( p_scene, p_mesh, p_posNormBuffer, p_bone_xform, p_instance_xform, &array_base, &col_base, &layer, pm, &world_fog, flags );
			}
		}
		Nx::CFog::sEnableFog( true );
		world_fog = true;
	}








//	// Render opaque meshes.
//	if( (flags & vRENDER_OPAQUE) && !(flags & vRENDER_SHADOW_1ST_PASS) )
//	{
//		for( int e = 0; e < p_scene->m_num_opaque_entries; ++e )
//		{
//			if ( p_scene->m_opaque_meshes[e]->m_flags & sMesh::MESH_FLAG_VISIBLE )
//			{
//				sMesh * p_mesh = p_scene->m_opaque_meshes[e];
//		
////				set_render_state( RS_ZWRITEENABLE, p_mesh->m_zwrite );
//
//				if ( flags & vRENDER_NEW_TEST )
//				{
//					// Textured.
//					// First pass material setup.
//					sMaterialHeader * p_mat = p_mesh->mp_dl->m_material.p_header;
//					sMaterialPassHeader * p_pass = (sMaterialPassHeader *)&p_mat[1];
//
//					if ( p_pass[0].m_texture.p_data )
//					{
//						if ( first_mat || !one_mat )
//						{
//							first_mat = false;
//
//							MaterialSubmit( p_mesh );
//						}
//						GX::CallDisplayList ( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size ); 
//					}
//					else
//					{
//						// Untextured.
//						if ( !(p_mesh->mp_dl->m_flags & 0x00200000) )
//						{
//							GXSetNumTevStages( 1 );
//							GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//							GXSetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//							GXSetNumTexGens( 0 );
//							GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//							GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
////							GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//							GXSetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
//							GXSetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
//							GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
//							GXSetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//							GXSetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
////							GXSetChanMatColor( GX_COLOR0A0, p_pass->m_color );
//
//							GX::CallDisplayList ( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size ); 
//						}
//					}
//				}
//				else
//				{
//					multi_mesh( p_mesh, p_posNormBuffer );
//				}
//
//				// Now draw the opaque meshes with shadow mapped on them.
//				if( p_scene->m_flags & SCENE_FLAG_RECEIVE_SHADOWS )
//				{
//					if( !( p_mesh->m_flags & sMesh::MESH_FLAG_NO_SKATER_SHADOW ) )
//					{
//						if( frustum_check_sphere( light, p_scene->m_opaque_meshes[e]->m_sphere ))
//						{
//							//if( frustum_check_box( light, p_scene->m_opaque_meshes[e]->m_bbox ))
//							{
//								if ( shadow_meshes <  MAX_SHADOW_MESHES )
//								{
//									pShadow[shadow_meshes] = p_mesh;
//									shadow_meshes++;
//								}
//							}
//						}
//					}
//				}
//			}
//		}			
//	}			
//
//// Render semi-transparent meshes.
//	if( (flags & vRENDER_SEMITRANSPARENT) && !(flags & vRENDER_SHADOW_1ST_PASS) )
//	{
//		int e = 0;
//		int next_sorted_mesh_entry = 0;
//		
//		// Semitransparent rendering is done in three stages.
//		// The first stage is meshes in the list up to the point where dynamic sorting starts.
//		// The second stage is meshes in the list which use dynamic sorting.
//		// The third stage is meshes in the list beyond the point where dynamic sorting ends.
//		for( ; e < p_scene->m_first_dynamic_sort_entry; ++e )
//		{
//			if ( p_scene->m_semitransparent_meshes[e]->m_flags & sMesh::MESH_FLAG_VISIBLE )
//			{
//				sMesh * p_mesh = p_scene->m_semitransparent_meshes[e];
//		
////				set_render_state( RS_ZWRITEENABLE, p_mesh->m_zwrite );
//
//				++meshes_rendered;
//
//				if ( flags & vRENDER_NEW_TEST )
//				{
//					// Textured.
//					// First pass material setup.
//					sMaterialHeader * p_mat = p_mesh->mp_dl->m_material.p_header;
//					sMaterialPassHeader * p_pass = (sMaterialPassHeader *)&p_mat[1];
//
//					if ( p_pass[0].m_texture.p_data )
//					{
//						if ( first_mat || !one_mat )
//						{
//							first_mat = false;
//
//							MaterialSubmit( p_mesh );
//						}
//
//						GX::CallDisplayList ( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size ); 
//					}
//					else
//					{
//						// Untextured.
//						if ( !(p_mesh->mp_dl->m_flags & 0x00200000) )
//						{
//							GXSetNumTevStages( 1 );
//							GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//							GXSetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//							GXSetNumTexGens( 0 );
//							GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//							GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//							GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//							GXSetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
//							GXSetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
//							GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
//							GXSetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//							GXSetChanMatColor( GX_COLOR0A0, p_pass->m_color );
//
//							GX::CallDisplayList ( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size ); 
//						}
//					}
//				}
//				else
//				{
//					multi_mesh( p_mesh, p_posNormBuffer );
//				}
//			}
//		}
//
//		if( p_scene->m_num_dynamic_sort_entries > 0 )
//		{
//			// Second stage - dynamically sorted meshes.
//			int last_dynamic_sort_entry = p_scene->m_first_dynamic_sort_entry + p_scene->m_num_dynamic_sort_entries;
//			for( ; e < last_dynamic_sort_entry; ++e )
//			{
//				if ( p_scene->m_semitransparent_meshes[e]->m_flags & sMesh::MESH_FLAG_VISIBLE )
//				{
//					++meshes_rendered;
//					// Figure the midpoint of this mesh, and sort it into the render list.
//					float midpoint = frustum_sort_mid( p_scene->m_semitransparent_meshes[e]->m_sphere );
//
////					// Add this mesh to the visible list.
////					if( !( p_scene->m_semitransparent_meshes[e]->m_flags & sMesh::MESH_FLAG_NO_SKATER_SHADOW ))
////					{
////						visible_mesh_array[visible_mesh_array_index++] = p_scene->m_semitransparent_meshes[e];
////					}
//
//					sortedMeshArray[next_sorted_mesh_entry].p_mesh	= p_scene->m_semitransparent_meshes[e];
//					sortedMeshArray[next_sorted_mesh_entry].sort	= midpoint;
//					++next_sorted_mesh_entry;
//				}
//			}
//			if( next_sorted_mesh_entry > 0 )
//			{
//				// Sort the array into ascending sort order.
//				qsort( sortedMeshArray, next_sorted_mesh_entry, sizeof( sSortedMeshEntry ), cmp );
//		
//				for( int m = 0; m < next_sorted_mesh_entry; ++m )
//				{
//					sMesh * p_mesh = sortedMeshArray[m].p_mesh;
//
////					set_render_state( RS_ZWRITEENABLE, p_mesh->m_zwrite );
//
//					if ( flags & vRENDER_NEW_TEST )
//					{
//						// Textured.
//						// First pass material setup.
//						sMaterialHeader * p_mat = p_mesh->mp_dl->m_material.p_header;
//						sMaterialPassHeader * p_pass = (sMaterialPassHeader *)&p_mat[1];
//
//						if ( p_pass[0].m_texture.p_data )
//						{
//							if ( first_mat || !one_mat )
//							{
//								first_mat = false;
//
//								MaterialSubmit( p_mesh );
//							}
//
//							GX::CallDisplayList ( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size ); 
//						}
//						else
//						{
//							// Untextured.
//							if ( !(p_mesh->mp_dl->m_flags & 0x00200000) )
//							{
//	    						GXSetNumTevStages( 1 );
//								GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//								GXSetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//								GXSetNumTexGens( 0 );
//								GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//								GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//								GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//								GXSetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
//								GXSetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
//								GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
//								GXSetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//								GXSetChanMatColor( GX_COLOR0A0, p_pass->m_color );
//
//								GX::CallDisplayList ( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size ); 
//							}
//						}
//					}
//					else
//					{
//						multi_mesh( p_mesh, p_posNormBuffer );
//					}
//				}
//			}
//		
//			// Third stage - meshes after the dynamically sorted set.
//			for( ; e < p_scene->m_num_semitransparent_entries; ++e )
//			{
//				if ( p_scene->m_semitransparent_meshes[e]->m_flags & sMesh::MESH_FLAG_VISIBLE )
//				{
//					sMesh * p_mesh = p_scene->m_semitransparent_meshes[e];
//
////					set_render_state( RS_ZWRITEENABLE, p_mesh->m_zwrite );
//
//					++meshes_rendered;
//
//					if ( flags & vRENDER_NEW_TEST )
//					{
//						// Textured.
//						// First pass material setup.
//						sMaterialHeader * p_mat = p_mesh->mp_dl->m_material.p_header;
//						sMaterialPassHeader * p_pass = (sMaterialPassHeader *)&p_mat[1];
//
//						if ( p_pass[0].m_texture.p_data )
//						{
//							if ( first_mat || !one_mat )
//							{
//								first_mat = false;
//
//								MaterialSubmit( p_mesh );
//							}
//
//							GX::CallDisplayList ( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size ); 
//						}
//						else
//						{
//							// Untextured.
//							if ( !(p_mesh->mp_dl->m_flags & 0x00200000) )
//							{
//								GXSetNumTevStages( 1 );
//								GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//								GXSetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//								GXSetNumTexGens( 0 );
//								GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//								GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//								GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//								GXSetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
//								GXSetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
//								GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
//								GXSetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//								GXSetChanMatColor( GX_COLOR0A0, p_pass->m_color );
//
//								GX::CallDisplayList ( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size ); 
//							}
//						}
//					}
//					else
//					{
//						multi_mesh( p_mesh, p_posNormBuffer );
//					}
//				}
//			}
//		}
//
////		// Now draw the semitransparent meshes with shadow mapped on them.
////		if( p_scene->m_flags & SCENE_FLAG_RECEIVE_SHADOWS )
////		{
////			render_shadow_meshes( p_scene, visible_mesh_array, visible_mesh_array_index );
////		}
//	}
//
////	if ( 0 )
//	{
//	// Render shadow meshes.
//	if( flags & vRENDER_SHADOW_1ST_PASS )
//	{
//		int e;
//		for( e = 0; e < p_scene->m_num_opaque_entries; ++e )
//		{
//			if ( p_scene->m_opaque_meshes[e]->m_flags & sMesh::MESH_FLAG_VISIBLE )
//			{
//				sMesh * p_mesh = p_scene->m_opaque_meshes[e];
//		
////				set_render_state( RS_ZWRITEENABLE, p_mesh->m_zwrite );
//
//				if ( p_posNormBuffer ) {
//					// Skinned.
//					multi_start_16 ( &p_posNormBuffer[0], &p_posNormBuffer[3], 6, 6 );
//				} else {
//					// Not skinned.
//#ifdef SHORT_VERT
//					cam_offset( p_mesh->m_offset_x, p_mesh->m_offset_y, p_mesh->m_offset_z );
//					multi_start_16 ( p_mesh->mp_posBuffer, p_mesh->mp_normBuffer, 3, 3 );
//#else
//					multi_start ( p_mesh->mp_posBuffer, p_mesh->mp_normBuffer, 3, 3 );
//#endif		// SHORT_VERT
//				}
//				
//				multi_end( p_mesh, vRENDER_SHADOW_1ST_PASS );
//			}
//		}			
//
//		for( e = 0; e < p_scene->m_num_semitransparent_entries; ++e )
//		{
//			if ( p_scene->m_semitransparent_meshes[e]->m_flags & sMesh::MESH_FLAG_VISIBLE )
//			{
//				sMesh * p_mesh = p_scene->m_semitransparent_meshes[e];
//
////				set_render_state( RS_ZWRITEENABLE, p_mesh->m_zwrite );
//
//				if ( p_posNormBuffer ) {
//					// Skinned.
//					multi_start_16 ( &p_posNormBuffer[0], &p_posNormBuffer[3], 6, 6 );
//				} else {
//					// Not skinned.
//#ifdef SHORT_VERT
//					cam_offset( p_mesh->m_offset_x, p_mesh->m_offset_y, p_mesh->m_offset_z );
//					multi_start_16 ( p_mesh->mp_posBuffer, p_mesh->mp_normBuffer, 3, 3 );
//#else
//					multi_start ( p_mesh->mp_posBuffer, p_mesh->mp_normBuffer, 3, 3 );
//#endif		// SHORT_VERT
//				}
//
//				multi_end( p_mesh, vRENDER_SHADOW_1ST_PASS );
//			}
//		}			
//	}			
//
//	// Render shadows from list.
//#if 1
//	for ( int lp = 0; lp < shadow_meshes; lp++ )
//	{
//		GXTlutObj	palObj;
//		GXInitTlutObj( &palObj, &shadowPalette, GX_TL_RGB5A3, 16 );
//		GXLoadTlut ( &palObj, GX_TLUT0 );
//
//		// Set up shadow map texture
//#define SHADOW_TEXTURE_SIZE 256
//#define BLUR_TEXTURE_SIZE 64
//		GXTexObj blurTexture;
//		GXTexObj shadowTexture;
//		GXInitTexObjCI(
//			&blurTexture,
//			blurTextureData,		//shadowTextureData,
//			BLUR_TEXTURE_SIZE,		//SHADOW_TEXTURE_SIZE,
//			BLUR_TEXTURE_SIZE,        //SHADOW_TEXTURE_SIZE,
//			GX_TF_C4,
//			GX_CLAMP,
//			GX_CLAMP,
//			GX_FALSE,
//			GX_TLUT0 );
////	GXInitTexObjLOD(&blurTexture, GX_NEAR, GX_NEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
//		GXLoadTexObj( &blurTexture, GX_TEXMAP5 );
//		GXInitTexObjCI(
//			&shadowTexture,
//			shadowTextureData,
//			SHADOW_TEXTURE_SIZE,
//			SHADOW_TEXTURE_SIZE,
//			GX_TF_C4,
//			GX_CLAMP,
//			GX_CLAMP,
//			GX_FALSE,
//			GX_TLUT0 );
////	GXInitTexObjLOD(&shadowTexture, GX_NEAR, GX_NEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
//		GXLoadTexObj( &shadowTexture, GX_TEXMAP6 );
//	
//		// Offset the texture matrix.
//#ifdef SHORT_VERT
//		Mtx m;
//		MTXTrans( m, pShadow[lp]->m_offset_x, pShadow[lp]->m_offset_y, pShadow[lp]->m_offset_z );
//		MTXConcat( light, m, m );
//		GXLoadTexMtxImm( m, GX_TEXMTX7, GX_MTX3x4 );
//#endif		// SHORT_VERT
//
//		if ( p_posNormBuffer ) {
//			// Skinned.
//			multi_start_16 ( &p_posNormBuffer[0], &p_posNormBuffer[3], 6, 6 );
//		} else {
//			// Not skinned.
//#ifdef SHORT_VERT
//			cam_offset( pShadow[lp]->m_offset_x, pShadow[lp]->m_offset_y, pShadow[lp]->m_offset_z );
//			multi_start_16 ( pShadow[lp]->mp_posBuffer, pShadow[lp]->mp_normBuffer, 3, 3 );
//#else
//			multi_start ( pShadow[lp]->mp_posBuffer, pShadow[lp]->mp_normBuffer, 3, 3 );
//#endif		// SHORT_VERT
//		}
//
//		multi_end( pShadow[lp], vRENDER_SHADOW_2ND_PASS );
//	}
//#endif
//	}
//
////	if( flags & vRENDER_SEMITRANSPARENT )
////	{
////		// Sort semi-transparent meshes.
////		int bucket_size = p_scene->m_num_semitransparent_entries * 2;
////		sMesh * sorted_meshes[bucket_size];
////		memset( sorted_meshes, 0, bucket_size * 4 );
////		for( int e = 0; e < p_scene->m_num_semitransparent_entries; ++e )
////		{
////			if( p_scene->m_semitransparent_meshes[e]->m_flags & sMesh::MESH_FLAG_ACTIVE )
////			{
////				// Frustum cull this set of meshes, using the associated bounding box.
////				float z;
////				if( frustum_check_sphere( p_scene->m_semitransparent_meshes[e]->m_sphere, &z ))
////				{
////					int index = (int)( z / ( 20000.0f / bucket_size ) );
////					if ( index < 0 ) index = 0;
////					if ( index > (bucket_size - 1) ) index = (bucket_size - 1);
////	
////					if ( index >= (bucket_size / 2) )
////					{
////						// Search backwards to find an empty slot.
////						while ( sorted_meshes[index] ) index--;
////						if ( index >= 0 )
////						{
////							sorted_meshes[index] = p_scene->m_semitransparent_meshes[e];
////						}
////					}
////					else
////					{
////						// Search forwards to find an empty slot.
////						while ( sorted_meshes[index] ) index++;
////						if ( index < bucket_size )
////						{
////							sorted_meshes[index] = p_scene->m_semitransparent_meshes[e];
////						}
////					}
////				}
////			}
////		}
////	
////		// Render semi-transparent meshes.
////		for( int e = 0; e < bucket_size; ++e )
////		{
////			if( sorted_meshes[e] )
////			{
////				++meshes_rendered;
////	
////				sMesh * p_mesh = sorted_meshes[e];
////				
////				set_render_state( RS_ZWRITEENABLE, p_mesh->m_zwrite );
////
//////				if ( p_mesh->mp_uvBuffer && p_mesh->mp_material->pTex[0] )
////				{
////					uint32 layer;
////
////					if ( p_posNormBuffer ) {
////						// Skinned.
////						multi_start ( &p_posNormBuffer[0], &p_posNormBuffer[3], 6, 6 );
////					} else {
////						// Not skinned.
////						multi_start ( p_mesh->mp_posBuffer, p_mesh->mp_normBuffer, 3, 3 );
////					}
////					multi_add_color ( p_mesh->mp_colBuffer );
////					int uv_set = 0;
////					float * p_tex;
////					for ( layer = 0; layer < p_mesh->mp_material->Passes ; layer++ )
////					{
////						GXTexWrapMode u_mode;
////						GXTexWrapMode v_mode;
////						if ( p_mesh->mp_material->Flags[layer] & MATFLAG_ENVIRONMENT )
////						{
////							p_tex = NULL;
////							u_mode = GX_REPEAT;
////							v_mode = GX_REPEAT;
////						}
////						else
////						{
////							p_tex = &p_mesh->mp_uvBuffer[2*uv_set];
////							u_mode = p_mesh->mp_material->UVAddressing[layer] & (1<<0)  ? GX_CLAMP : GX_REPEAT;
////							v_mode = p_mesh->mp_material->UVAddressing[layer] & (1<<1) ? GX_CLAMP : GX_REPEAT;
////						}
////						uv_set++;
////
////						multi_add_texture ( p_tex, p_mesh->m_num_uv_sets, p_mesh->mp_material->uv_slot[layer], p_mesh->mp_material->Flags[layer], layer );
////					}
////
////					multi_end( p_mesh, 0 );
////				}
////			}
////		}			
////	}			
//	
//	if ( p_scene->m_numHierarchyObjects != 0 )
//	{
//		// Hierarchical, set to matrix 0.
//		GXSetCurrentMtx( GX_PNMTX0 );
//	}
	GX::SetProjectionv( pm );
	g_object++;
}

/******************************************************************/
/*                                                                */
/* Sets MESH_FLAG_VISIBLE if visible for all meshes in this scene */
/*                                                                */
/******************************************************************/
int cull_scene( sScene *p_scene, uint32 flags )
{
	int meshes_visible = 0;

	// Cull all meshes.
	sMesh ** pp_mesh = p_scene->mpp_mesh_list;

	for( int e = 0; e < p_scene->m_num_filled_meshes; ++e )
	{
		sMesh * p_mesh = pp_mesh[e];
		if( ( p_mesh->m_flags & sMesh::MESH_FLAG_ACTIVE ) && ( p_mesh->GetVisibility() & EngineGlobals.viewport ) )
		{
			Mth::Vector sphere = p_mesh->mp_dl->m_sphere;

			// Frustum cull this set of meshes, using the associated bounding box.
			if( frustum_check_sphere( sphere ))
			{
				// Sphere is visible.
				if( ( flags & vRENDER_OCCLUDED ) )
				{
					if ( TestSphereAgainstOccluders( &sphere ) )
					{
						// Occluded.
						p_mesh->m_flags &= ~sMesh::MESH_FLAG_VISIBLE;
					}
					else
					{
						// Not occluded.
						p_mesh->m_flags |= sMesh::MESH_FLAG_VISIBLE;
						++meshes_visible;
					}

				}
				else
				{
					// Sphere being visible is enough.
					p_mesh->m_flags |= sMesh::MESH_FLAG_VISIBLE;
					++meshes_visible;
				}
			}
			else
			{
				// Sphere not visible.
				p_mesh->m_flags &= ~sMesh::MESH_FLAG_VISIBLE;
			}
		}
		else
		{
			// Force to be not visible.
			p_mesh->m_flags &= ~sMesh::MESH_FLAG_VISIBLE;
		}
	}
	return meshes_visible;

//	make_scene_visible( p_scene );
//	return p_scene->m_num_filled_meshes;
}

/******************************************************************/
/*                                                                */
/* Sets MESH_FLAG_VISIBLE for all meshes in this scene.			  */
/*                                                                */
/******************************************************************/
void make_scene_visible( sScene *p_scene )
{
	// Set all meshes to be visible.
	sMesh ** pp_mesh = p_scene->mpp_mesh_list;

	for( int e = 0; e < p_scene->m_num_filled_meshes; ++e )
	{
		if( ( pp_mesh[e]->m_flags & sMesh::MESH_FLAG_ACTIVE ) && ( pp_mesh[e]->GetVisibility() & EngineGlobals.viewport ) )
		{
			pp_mesh[e]->m_flags |= sMesh::MESH_FLAG_VISIBLE;
		}
	}
}

void render_begin( void )
{

}

void render_end( void )
{

}

} // namespace NxNgc



