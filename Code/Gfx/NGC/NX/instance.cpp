#include "nx_init.h"
#include "instance.h"
#include "anim.h"
#include "render.h"
#include "gfx\camera.h"
#include "gfx\nx.h"
#include "occlude.h"
#include "charpipeline\skinning.h"
#include "sys/ngc/p_gx.h"
#include "sys/ngc/p_aram.h"
#include "sys/ngc/p_dma.h"
#include <gfx\NxLight.h>
#include "sys/ngc/p_buffer.h"
#include <sys/profiler.h>
#include "sys/ngc/p_display.h"

#include "dolphin/base/ppcwgpipe.h"
#include "dolphin/gx/gxvert.h"

bool g_in_cutscene = false;

int g_twiddle = 900;

int g_hash_items = 0;

int g_count = 0;

float g_tweak = 0.0f;

float NEAR_Z = 50.0f;
float FAR_Z = 150.0f;

float MIN_LEN = 10.0f;
float MAX_LEN = 80.0f;

extern int gNewRender;
extern int g_object;

extern bool g_mc_hack;

//#define DEBUG_SPHERES

extern "C" {

extern void TransformSingle	( ROMtx m, s16 * srcBase, s16 * dstBase, u32 count );
extern void TransformDouble	( ROMtx m0, ROMtx m1, s16 * wtBase, s16 * srcBase, s16 * dstBase, u32 count );
extern void TransformAcc	( ROMtx m, u16 count, s16 * srcBase, s16 * dstBase, u16 * indices, s16 * weights );
extern void ConvertFixed196ToFloat	( s16 * p_source, float * p_dest, int count );
extern void CalculateDotProducts	( s16 * p_xyz, u16 * p_index_list, float * p_dot, int count, float px, float py, float pz );
extern void RenderShadows			( s16 * p_xyz, u16 * p_index_list, NxNgc::sShadowEdge * p_neighbor_list, float * p_dot, int count, float px, float py, float pz, float tx, float ty, float tz );

}

//static char doubleBuffer0[(100*1024)+32];
//static char doubleBuffer1[(100*1024)+32];
//static uint32 *p_double[2] = { (uint32 *)OSRoundUp32B(doubleBuffer0), (uint32 *)OSRoundUp32B(doubleBuffer1) };
//
//static volatile bool	dmaComplete;
//
//static void arqCallback( u32 pointerToARQRequest )
//{
//	dmaComplete = true;
//}

namespace NxNgc
{


//Lst::HashTable< CInstance > *pInstanceTable	= NULL;

static Mth::Matrix	*pLastBoneTransforms	= NULL;
CInstance			*pFirstInstance			= NULL;
#if 0
static CInstance	*pAnimInstance[256];
static int			instanceCount = 0;
#endif

#define INSTANCE_ARRAY_SIZE	2048
static CInstance *p_instances[INSTANCE_ARRAY_SIZE];
int current_index = 0;

void InitialiseInstanceTable( void )
{
//	if( pInstanceTable == NULL )
//	{
//		pInstanceTable = new Lst::HashTable<CInstance>( 8 );
//	}
}


//bool frustum_check_sphere( NsVector * p_center, float radius )
//{
//	bool visible = true;
//
//	NsVector out;
//
//	EngineGlobals.local_to_camera.multiply( p_center, &out );
//
//	float x = out.x;
//	float y = out.y;
//	float z = out.z;
//	float R = radius;
//	
//	if (R<z-EngineGlobals.near || R<EngineGlobals.far-z || R<EngineGlobals.sy*z+EngineGlobals.cy*y || R<EngineGlobals.sy*z-EngineGlobals.cy*y || R<EngineGlobals.sx*z-EngineGlobals.cx*x || R<EngineGlobals.sx*z+EngineGlobals.cx*x) visible = false;
//
//	return visible;
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#ifdef DEBUG_SPHERES
static void debug_render_sphere( Mth::Vector sphere, GXColor color )
{
	GX::SetPointSize( 6, GX_TO_ONE );

	GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
	GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
	GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );
	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);

	GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
										   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV,
										   GX_TEV_SWAP0, GX_TEV_SWAP0 );

	GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
									   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
	GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
	GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
	GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
	GX::SetChanMatColor( GX_COLOR0A0, color );

	GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );

	GX::Begin( GX_LINES, GX_VTXFMT0, 6 ); 
		GX::Position3f32( sphere[X] - sphere[W] , sphere[Y] , sphere[Z] );
		GX::Position3f32( sphere[X] + sphere[W] , sphere[Y] , sphere[Z] );
		GX::Position3f32( sphere[X] , sphere[Y] - sphere[W] , sphere[Z] );
		GX::Position3f32( sphere[X] , sphere[Y] + sphere[W] , sphere[Z] );
		GX::Position3f32( sphere[X] , sphere[Y] , sphere[Z] - sphere[W] );
		GX::Position3f32( sphere[X] , sphere[Y] , sphere[Z] + sphere[W] );
	GX::End();

#define NUM_SEGMENTS 32
#define SEGMENT_STEP ( 1.0f / (float)NUM_SEGMENTS )

	GX::Begin( GX_LINESTRIP, GX_VTXFMT0, NUM_SEGMENTS + 1 ); 
	for ( int lp = 0; lp < (NUM_SEGMENTS+1); lp++ )
	{
		GX::Position3f32( sphere[X] + ( sinf( Mth::PI * 2.0f * ( SEGMENT_STEP * (float)lp ) ) * sphere[W] ),
						  sphere[Y] + ( cosf( Mth::PI * 2.0f * ( SEGMENT_STEP * (float)lp ) ) * sphere[W] ),
						  sphere[Z] );
	}
	GX::End();

	GX::Begin( GX_LINESTRIP, GX_VTXFMT0, NUM_SEGMENTS + 1 ); 
	for ( int lp = 0; lp < (NUM_SEGMENTS+1); lp++ )
	{
		GX::Position3f32( sphere[X] + ( sinf( Mth::PI * 2.0f * ( SEGMENT_STEP * (float)lp ) ) * sphere[W] ),
						  sphere[Y],
						  sphere[Z] + ( cosf( Mth::PI * 2.0f * ( SEGMENT_STEP * (float)lp ) ) * sphere[W] ) );
	}
	GX::End();

	GX::Begin( GX_LINESTRIP, GX_VTXFMT0, NUM_SEGMENTS + 1 ); 
	for ( int lp = 0; lp < (NUM_SEGMENTS+1); lp++ )
	{
		GX::Position3f32( sphere[X],
						  sphere[Y] + ( sinf( Mth::PI * 2.0f * ( SEGMENT_STEP * (float)lp ) ) * sphere[W] ),
						  sphere[Z] + ( cosf( Mth::PI * 2.0f * ( SEGMENT_STEP * (float)lp ) ) * sphere[W] ) );
	}
	GX::End();
}
#endif DEBUG_SPHERES

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int sort_by_bone_transform( const void *p1, const void *p2 )
{
	CInstance	*p_mesh1		= *((CInstance**)p1 );
	CInstance	*p_mesh2		= *((CInstance**)p2 );

//	Mth::Matrix *p_mat1			= p_mesh1->GetBoneTransforms();
//	Mth::Matrix *p_mat2			= p_mesh2->GetBoneTransforms();
	
	// Setup some required pointers for both transforms.
	uint32 p_mat1 = (uint32)p_mesh1->GetScene()->mp_dl->mp_object_header->m_num_skin_verts;
	uint32 p_mat2 = (uint32)p_mesh2->GetScene()->mp_dl->mp_object_header->m_num_skin_verts;

	if(( p_mat1 == 0 ) || ( p_mesh1->GetScene()->m_numHierarchyObjects > 0 ))
	{
		if(( p_mat2 == NULL ) || ( p_mesh2->GetScene()->m_numHierarchyObjects > 0 ))
		{
			int num_st1 = p_mesh1->GetScene()->m_num_filled_meshes - p_mesh1->GetScene()->m_num_opaque_meshes;
			int num_st2 = p_mesh2->GetScene()->m_num_filled_meshes - p_mesh2->GetScene()->m_num_opaque_meshes;

			if( num_st1 == num_st2 )
			{
				// Try sorting on the material draw order of the first semitransparent mesh.
				if( num_st1 > 0 )
				{
					NxNgc::sMaterialHeader	*p_material1 = p_mesh1->GetScene()->mpp_mesh_list[p_mesh1->GetScene()->m_num_opaque_meshes]->mp_dl->m_material.p_header;
					NxNgc::sMaterialHeader	*p_material2 = p_mesh2->GetScene()->mpp_mesh_list[p_mesh2->GetScene()->m_num_opaque_meshes]->mp_dl->m_material.p_header; 

					if( p_material1 && p_material2 )
					{
						return ( p_material1->m_draw_order > p_material2->m_draw_order ) ? 1 : (( p_material1->m_draw_order < p_material2->m_draw_order ) ? -1 : 0 );
					}
				}
				return 0;
			}
	
			return ( num_st1 > num_st2 ) ? 1 : -1;
		}
		return 1;
	}
	else if(( p_mat2 == 0 ) || ( p_mesh2->GetScene()->m_numHierarchyObjects > 0 ))
	{
		return -1;
	}

	// At this stage we know both instances have bone transforms.
	if( p_mat1 == p_mat2 )
	{
		int num_st1 = p_mesh1->GetScene()->m_num_filled_meshes - p_mesh1->GetScene()->m_num_opaque_meshes;
		int num_st2 = p_mesh2->GetScene()->m_num_filled_meshes - p_mesh2->GetScene()->m_num_opaque_meshes;
	
		if( num_st1 == num_st2 )
			return 0;
	
		return ( num_st1 > num_st2 ) ? 1 : -1;
	}

	return((uint32)p_mat1 > (uint32)p_mat2 ) ? 1 : -1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void render_instance( CInstance *p_instance, uint32 flags )
{
//	// Seed the static pointer off to NULL, otherwise if there is only one object with bone transforms, it will never update.
//	pLastBoneTransforms = NULL;
//
//	if( p_instance->GetActive())
//	{
//		// Check whether this instance is visible.
//		set_frustum_bbox_transform( p_instance->GetTransform());
//		float sphere[4];
//		sphere[0] = p_instance->GetScene()->m_sphere_center.x;
//		sphere[1] = p_instance->GetScene()->m_sphere_center.y;
//		sphere[2] = p_instance->GetScene()->m_sphere_center.z;
//		sphere[3] = p_instance->GetScene()->m_sphere_radius;
//		if( frustum_check_sphere( sphere ))
//		{
//			if( !TestSphereAgainstOccluders( (Mth::Vector*)&p_instance->GetScene()->m_sphere_center, p_instance->GetScene()->m_sphere_radius ))
//			{
//				p_instance->Render( flags | vRENDER_OPAQUE | vRENDER_SEMITRANSPARENT );
//
////				// Restore world transform to identity.
////				D3DDevice_SetTransform( D3DTS_WORLD, &EngineGlobals.world_matrix );
////				set_frustum_bbox_transform( NULL );
//			}
//		}
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void render_instances( uint32 flags )
{
	int save_g_object = g_object;

	if( current_index > 0 )
	{
		int i = 0;

//		Mth::Matrix *pLast = NULL;
//		ROMtx bone_mtx[80];

		for( ; i < current_index; ++i )
		{
#ifndef __NOPT_FINAL__
			g_object = p_instances[i]->m_object_num;
#endif		// __NOPT_FINAL__
//			if ( ( flags & NxNgc::vRENDER_TRANSFORM ) && ( p_instances[i]->GetFlags() & NxNgc::CInstance::INSTANCE_FLAG_TRANSFORM_ME ) )
//			{
////				p_instances[i]->Transform( 0 /*flags*/ | vRENDER_OPAQUE | vRENDER_SEMITRANSPARENT, bone_mtx, pLast );
////				pLast = p_instances[i]->GetBoneTransforms();
//				p_instances[i]->ClearFlag( NxNgc::CInstance::INSTANCE_FLAG_TRANSFORM_ME );
//			}

			if(( p_instances[i]->GetBoneTransforms() == NULL ) || ( p_instances[i]->GetScene()->m_numHierarchyObjects > 0 ))
			{
				break;
			}

			if(( flags & vRENDER_OPAQUE ) || (( flags & vRENDER_SEMITRANSPARENT ) && ( flags & vRENDER_INSTANCE_PRE_WORLD_SEMITRANSPARENT )))
			{
				p_instances[i]->Render( flags );
//				p_instances[i]->SetFlag( NxNgc::CInstance::INSTANCE_FLAG_TRANSFORM_ME );
			}
		}

		for( ; i < current_index; ++i )
		{
#ifndef __NOPT_FINAL__
			g_object = p_instances[i]->m_object_num;
#endif		// __NOPT_FINAL__

//			if(( p_instances[i]->GetBoneTransforms()) && ( p_instances[i]->GetScene()->m_numHierarchyObjects == 0 ))
//				break;

			if(( flags & vRENDER_OPAQUE ) || (( flags & vRENDER_SEMITRANSPARENT ) && ( flags & vRENDER_INSTANCE_POST_WORLD_SEMITRANSPARENT )))
			{
				p_instances[i]->Render( flags );
			}
		}
	}
	g_object = save_g_object;
}

void CInstance::Transform( uint32 flags, ROMtx * p_mtx_buffer, Mth::Matrix *p_mtx_last )
{
//#	ifdef __USE_PROFILER__
////	Sys::VUProfiler->PushContext( 128,0,0 );
//#	endif		// __USE_PROFILER__

//	if ( gNewRender ) render_flags |= NxNgc::vRENDER_NEW_TEST;

	if( GetBoneTransforms() && ( GetScene()->m_numHierarchyObjects == 0 ) )
	{
		// Has bone transforms, is therefore an animated object such as a skater, pedestrian etc.
		mp_posNormRenderBuffer = GetPosNormalBuffer( 0 );

		if ( !p_mtx_last || ( GetBoneTransforms() != p_mtx_last ) )
		{
			float * ps;
			float * pd;
			for( int i = 0; i < GetNumBones(); ++i )
			{
				ps = (float*)&GetBoneTransforms()[i];
				pd = (float*)&p_mtx_buffer[i][0][0];

				*pd++ = *ps++;
				*pd++ = *ps++;
				*pd++ = *ps++;
				ps++;
				*pd++ = *ps++;
				*pd++ = *ps++;
				*pd++ = *ps++;
				ps++;
				*pd++ = *ps++;
				*pd++ = *ps++;
				*pd++ = *ps++;
				ps++;
				*pd++ = *ps++;
				*pd++ = *ps++;
				*pd++ = *ps++;
			}
		}

		// Setup some required pointers for both transforms.
//		sMesh*	p_mesh = GetScene()->mpp_mesh_list[0];
//		if ( GetScene()->m_num_opaque_entries )
//		{
//			p_mesh = GetScene()->m_opaque_meshes[0];
//		}
//		else if ( GetScene()->m_num_semitransparent_entries )
//		{
//			p_mesh = GetScene()->m_semitransparent_meshes[0];
//		}
//		else
//		{
//			Dbg_MsgAssert(0, ("No meshes in scene."));
//			p_mesh = NULL;
//		}

		// Transform the single lists.
		uint32*	p32 = (uint32 *)GetScene()->mp_dl->mp_object_header->m_skin.p_data;
		s16*	p_pos = mp_posNormRenderBuffer;
		for ( uint32 lp = 0; lp < GetScene()->mp_dl->mp_object_header->m_num_single_lists; lp++ ) {
			uint32		pairs = *p32++;
			uint32		mtx = *p32++;
			s16*		p_pn = (s16 *)p32;
			ROMtxPtr	p_bone = p_mtx_buffer[mtx];
	
			TransformSingle( p_bone, p_pn, p_pos, pairs == 1 ? 2 : pairs ); 
			p_pos += (6 * pairs);
			p32 = (uint32 *)&p_pn[pairs*6];
		}

		// Transform the double lists.
		for ( uint32 lp = 0; lp < GetScene()->mp_dl->mp_object_header->m_num_double_lists; lp++ ) {
			uint32		pairs = *p32++;
			uint32		mtx = *p32++;
			s16*		p_pn = (s16 *)p32;
			ROMtxPtr	p_bone0 = p_mtx_buffer[mtx&255];
			ROMtxPtr	p_bone1 = p_mtx_buffer[(mtx>>8)&255];
			s16*		p_weight = (s16 *)&p_pn[(6*pairs)];
	
			TransformDouble( p_bone0, p_bone1, p_weight, p_pn, p_pos, pairs == 1 ? 2 : pairs ); 
			p_pos += (6 * pairs);
			p32 = (uint32 *)&p_weight[pairs*2];
		}

		// Transform the acc lists.
		for ( uint32 lp = 0; lp < GetScene()->mp_dl->mp_object_header->m_num_add_lists; lp++ ) {
			uint32		singles = *p32++;
			uint32		mtx = *p32++;
			s16*		p_pn = (s16 *)p32;
			ROMtxPtr	p_bone = p_mtx_buffer[mtx];
			s16*		p_weight = (s16 *)&p_pn[(6*singles)];
			uint16*		p_index = (uint16 *)&p_weight[singles];
			
			TransformAcc( p_bone, singles, p_pn, mp_posNormRenderBuffer, p_index, p_weight ); 
//			p32 = (uint32 *)&p_index[singles+(singles&1)];
			p32 = (uint32 *)&p_index[singles];
		}



//		int num_vertex;
//		if ( mp_scene->m_num_opaque_entries )
//		{
//			num_vertex = mp_scene->m_opaque_meshes[0]->m_num_vertex;
//		}
//		else if ( mp_scene->m_num_semitransparent_entries )
//		{
//			num_vertex = mp_scene->m_semitransparent_meshes[0]->m_num_vertex;
//		}
//		else
//		{
//			Dbg_MsgAssert(0, ("No meshes in scene."));
//			num_vertex = 0;
//		}
//
		int num_vertex = GetScene()->mp_dl->mp_object_header->m_num_skin_verts;
		DCFlushRange ( mp_posNormRenderBuffer, sizeof ( s16 ) * 6 * ( num_vertex + 1 ) );
	}
//#	ifdef __USE_PROFILER__
////	Sys::VUProfiler->PopContext(  );
//#	endif		// __USE_PROFILER__
}



void CInstance::Render( uint32 flags )
{
	uint32 render_flags = flags;
//	if ( gNewRender ) render_flags |= NxNgc::vRENDER_NEW_TEST;

	NsMatrix	root;

	root.setRightX( GetTransform()->GetRight()[X] );
	root.setRightY( GetTransform()->GetUp()[X] );
	root.setRightZ( GetTransform()->GetAt()[X] );

	root.setUpX( GetTransform()->GetRight()[Y] ); 
	root.setUpY( GetTransform()->GetUp()[Y] );    
	root.setUpZ( GetTransform()->GetAt()[Y] );    

	root.setAtX( GetTransform()->GetRight()[Z] ); 
	root.setAtY( GetTransform()->GetUp()[Z] );    
	root.setAtZ( GetTransform()->GetAt()[Z] );    

	root.setPosX( GetTransform()->GetPos()[X] );
	root.setPosY( GetTransform()->GetPos()[Y] );
	root.setPosZ( GetTransform()->GetPos()[Z] );

//	Mth::Vector world_sphere(GetScene()->m_sphere_center.x,GetScene()->m_sphere_center.y,GetScene()->m_sphere_center.z);
//	world_sphere[W] = 1.0f;
//	world_sphere *= *GetTransform();							// => use mp_transform
//	if (TestSphereAgainstOccluders( &world_sphere, GetScene()->m_sphere_radius ))
//	{
//		return;
//	}

	// Create & upload current view matrix.
	NsMatrix	tr;
	tr.cat( EngineGlobals.world_to_camera, root );
	GX::LoadPosMtxImm( (MtxPtr)&tr, GX_PNMTX0 );

	// Need to force the current matrix to be just incase hierarchical models change it to 1.
	if ( render_flags & vRENDER_SHADOW_1ST_PASS )
	{
		GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );
	}

//	if ( !( render_flags & vRENDER_SHADOW_1ST_PASS ) )
	{
		//NsMatrix inv;
		//inv.invert( root );
		//GX::LoadNrmMtxImm((MtxPtr)&inv, GX_PNMTX0);
//		tr.cat( EngineGlobals.camera, root );
//		tr.invert( root/*EngineGlobals.world_to_camera*/ );
	
		Nx::CModelLights *p_lights = GetModel() ? GetModel()->GetModelLights() : NULL;
		if( p_lights )
		{
			p_lights->UpdateEngine( GetTransform()->GetPos(), true );
		}
		else
		{
			Nx::CLightManager::sUpdateEngine();
		}
	}
	MTXCopy ( (MtxPtr)&tr, EngineGlobals.current_uploaded );

	EngineGlobals.local_to_camera.copy( tr );

	EngineGlobals.object_pos.x = tr.getPosX();
	EngineGlobals.object_pos.y = tr.getPosY();
	EngineGlobals.object_pos.z = tr.getPosZ();




#ifdef DEBUG_SPHERES
	if( GetBoneTransforms() && ( GetScene()->m_numHierarchyObjects == 0 ) )
	{
		Mth::Vector sphere = GetScene()->mpp_mesh_list[0]->mp_dl->mp_object_header->m_sphere;
		debug_render_sphere( sphere, (GXColor){255,0,0,255} );
	}
	else
	{
		if ( GetScene()->m_numHierarchyObjects && GetBoneTransforms() )
		{
			// Car or something...
			Mth::Vector sphere = GetScene()->m_sphere;	//mpp_mesh_list[0]->mp_dl->mp_object_header->m_sphere;
			debug_render_sphere( sphere, (GXColor){0,255,0,255} );
		}
		else
		{
			// No bones, just render normally. Must be a trashcan or something.
			Mth::Vector sphere = GetScene()->mpp_mesh_list[0]->mp_dl->mp_object_header->m_sphere;
			debug_render_sphere( sphere, (GXColor){0,0,255,255} );
		}
	}
#endif		// DEBUG_SPHERES





	GXLightObj light_obj[3];

	// Always set ambient & diffuse colors.
	GX::SetChanAmbColor( GX_COLOR0A0, NxNgc::EngineGlobals.ambient_light_color );
//		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){128,128,128,255} );
//				GX::SetChanAmbColor( GX_COLOR1A1, NxNgc::EngineGlobals.ambient_light_color );

	GX::InitLightColor( &light_obj[0], NxNgc::EngineGlobals.diffuse_light_color[0] );
	GX::InitLightPos( &light_obj[0], NxNgc::EngineGlobals.light_x[0], NxNgc::EngineGlobals.light_y[0], NxNgc::EngineGlobals.light_z[0] );
	GX::LoadLightObjImm( &light_obj[0], GX_LIGHT0 );

	GX::InitLightColor( &light_obj[1], NxNgc::EngineGlobals.diffuse_light_color[1] );
	GX::InitLightPos( &light_obj[1], NxNgc::EngineGlobals.light_x[1], NxNgc::EngineGlobals.light_y[1], NxNgc::EngineGlobals.light_z[1] );
	GX::LoadLightObjImm( &light_obj[1], GX_LIGHT1 );

	GX::InitLightColor( &light_obj[2], NxNgc::EngineGlobals.diffuse_light_color[2] );
	GX::InitLightPos( &light_obj[2], NxNgc::EngineGlobals.light_x[2], NxNgc::EngineGlobals.light_y[2], NxNgc::EngineGlobals.light_z[2] );
	GX::LoadLightObjImm( &light_obj[2], GX_LIGHT2 );

	// Set channel control for diffuse.
	GX::SetChanCtrl( GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT0|GX_LIGHT1|GX_LIGHT2, GX_DF_CLAMP, GX_AF_NONE );
	GX::SetChanCtrl( GX_ALPHA0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE ); 







	if( GetBoneTransforms() && ( GetScene()->m_numHierarchyObjects == 0 ) )
	{
		// Has bone transforms, is therefore an animated object such as a skater, pedestrian etc.

//		if ( ( flags & vRENDER_SHADOW_1ST_PASS ) && GetScene()->mp_scene_data && GetScene()->mp_scene_data->m_num_shadow_faces && !g_in_cutscene )
//		{
//			f32 p_pm[GX_PROJECTION_SZ];
//			GX::GetProjectionv( p_pm );
//			float value = p_pm[6] + ( (float)(-g_twiddle) ) * p_pm[5];
//
//			GXWGFifo.u8 = GX_LOAD_XF_REG;
//			GXWGFifo.u16 = 0;
//			GXWGFifo.u16 = 0x1025;
//			GXWGFifo.f32 = value;
//
//
//			GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_FRONT ); 
//			GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );
//			GX::SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0 );
//
//			GX::SetTevOrder( GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0,
//										   GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0 );
//
//			GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0,	GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
//													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
//													GX_TEV_SWAP0, GX_TEV_SWAP0 );
//			GX::SetTevColorInOp( GX_TEVSTAGE0,		GX_CC_KONST, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
//													GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//
//			GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );
//
//			GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
////			GX::SetZMode( GX_TRUE, GX_GREATER, GX_FALSE );
////			GX::SetZMode( GX_TRUE, GX_GREATER, GX_FALSE );
//			GX::SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
//
//			GX::SetTevKColor( GX_KCOLOR0, (GXColor){4,4,4,4} );
//			GX::SetTevKSel( GX_TEVSTAGE0, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A );
//
////			if ( !g_in_cutscene ) GX::SetClipMode( GX_CLIP_DISABLE );
//			GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//
//
//			float px = 0.0f;	//NxNgc::EngineGlobals.skater_shadow_dir.x;
//			float py = -1.0f;    //NxNgc::EngineGlobals.skater_shadow_dir.y * 3.0f;
//			float pz = 0.0f;    //NxNgc::EngineGlobals.skater_shadow_dir.z;
//
//			float l = sqrtf( ( px * px ) + ( py * py ) + ( pz * pz ) );
//
//			px /= l;
//			py /= l;
//			pz /= l;
//
////#define NEAR_Z 50.0f
////#define FAR_Z 120.0f
////
////#define MIN_LEN 10.0f
////#define MAX_LEN 100.0f
//
//			// Shorten length if:
//			// 1. light vector & object vector are opposing.
//			// AND
//			// 2. camera is not above player.
//			// AND
//			// 3. camera is close to the object.
//			float base_length = MAX_LEN;
//
////			float cx = NxNgc::EngineGlobals.camera.getPosX() - GetTransform()->GetPos()[X];
////			float cy = NxNgc::EngineGlobals.camera.getPosY() - GetTransform()->GetPos()[Y];
////			float cz = NxNgc::EngineGlobals.camera.getPosZ() - GetTransform()->GetPos()[Z];
//
////			float obj_len = sqrtf( ( cx * cx ) + /*( cy * cy ) +*/ ( cz * cz ) );
//
////
////			// Fabricate a worst-case vector.
////			// 1st, pick a point at the head of the skater.
////			float wx = 0.0f;
////			float wy = 12.0f * 7.0f;		// Assuming 7ft skater.
////			float wz = 0.0f;
////
////			// 2nd, move towards camera by average value.
////			wx += ( cx / obj_len ) * ( 12.0f * 3.0f );
////			wy += ( cy / obj_len ) * ( 12.0f * 3.0f );
////			wz += ( cz / obj_len ) * ( 12.0f * 3.0f );
////
//			
////			float cam_dir_dot = ( px * cx ) + /*( py * cy ) +*/ ( pz * cz );
////
//////			OSReport( "Object len: %8.3f\n", obj_len );
////
////			if ( ( ( cam_dir_dot > -0.0f ) || ( obj_len <= NEAR_Z ) ) && ( cy < 100.0f ) )
////			{
////				// Opposing, get length.
////				if ( obj_len < FAR_Z )
////				{
////					if ( obj_len > NEAR_Z )
////					{
////						// Close enough, adjust base length.
////						base_length = MIN_LEN + ( ( ( obj_len - NEAR_Z ) * ( MAX_LEN - MIN_LEN ) ) / ( FAR_Z - NEAR_Z ) );
////					}
////					else
////					{
////						// Close enough, adjust base length.
////						base_length = MIN_LEN;
////					}
////				}
////			}
////
//			float length = NxNgc::EngineGlobals.skater_height + base_length;
//
//			root.setRightX( GetTransform()->GetRight()[X] );
//			root.setRightY( GetTransform()->GetRight()[Y] );
//			root.setRightZ( GetTransform()->GetRight()[Z] );
//
//			root.setUpX( GetTransform()->GetUp()[X] );
//			root.setUpY( GetTransform()->GetUp()[Y] );
//			root.setUpZ( GetTransform()->GetUp()[Z] );
//
//			root.setAtX( GetTransform()->GetAt()[X] );
//			root.setAtY( GetTransform()->GetAt()[Y] );
//			root.setAtZ( GetTransform()->GetAt()[Z] );
//
//			root.setPosX( 0.0f );
//			root.setPosY( 0.0f );
//			root.setPosZ( 0.0f );
//
//			NsVector in, out;
//
//			in.x = px;
//			in.y = py;
//			in.z = pz;
//			root.multiply( &in, &out );
//			px = out.x;
//			py = out.y;
//			pz = out.z;
//
//			float tx = px * g_tweak;
//			float ty = py * g_tweak;
//			float tz = pz * g_tweak;
//
//			px *= length;
//			py *= length;
//			pz *= length;
//
//			int num_shadow_faces = GetScene()->mp_scene_data->m_num_shadow_faces;
//
//			float p_dot[4096];
//			CalculateDotProducts( mp_posNormRenderBuffer, GetScene()->mp_shadow_volume_mesh, p_dot, num_shadow_faces, px, py, pz );
//#if 1
//			RenderShadows( mp_posNormRenderBuffer, GetScene()->mp_shadow_volume_mesh, GetScene()->mp_shadow_edge, p_dot, num_shadow_faces, px, py, pz, tx, ty, tz );
//#else
//			float * p_xyz = new float[mp_scene->mp_dl->mp_object_header->m_num_skin_verts*3];
//			ConvertFixed196ToFloat( mp_posNormRenderBuffer, p_xyz, mp_scene->mp_dl->mp_object_header->m_num_skin_verts );
//
//			for ( uint lp = 0; lp < GetScene()->mp_scene_data->m_num_shadow_faces; lp++ )
//			{
//				float dot = p_dot[lp];
//
//				if ( dot > 0 )
//				{
////					// See if we have to draw all sides.
////					// Check 01.
//					int shape = 0;
//					if ( ( GetScene()->mp_shadow_edge[lp].neighbor[0] == -1 ) || ( p_dot[ GetScene()->mp_shadow_edge[lp].neighbor[0] ] <= 0 ) )
//					{
//						shape |= (1<<0);
//					}
//					// Check 12.
//					if ( ( GetScene()->mp_shadow_edge[lp].neighbor[1] == -1 ) || ( p_dot[ GetScene()->mp_shadow_edge[lp].neighbor[1] ] <= 0 ) )
//					{
//						shape |= (1<<1);
//					}
//					// Check 23.
//					if ( ( GetScene()->mp_shadow_edge[lp].neighbor[2] == -1 ) || ( p_dot[ GetScene()->mp_shadow_edge[lp].neighbor[2] ] <= 0 ) )
//					{
//						shape |= (1<<2);
//					}
//
//					unsigned short idx0 = GetScene()->mp_shadow_volume_mesh[(lp*3)+0];
//					unsigned short idx1 = GetScene()->mp_shadow_volume_mesh[(lp*3)+1];
//					unsigned short idx2 = GetScene()->mp_shadow_volume_mesh[(lp*3)+2];
//
//					float x0 = p_xyz[(idx0*3)+0];
//					float y0 = p_xyz[(idx0*3)+1];
//					float z0 = p_xyz[(idx0*3)+2];
//
//					float x1 = p_xyz[(idx1*3)+0];
//					float y1 = p_xyz[(idx1*3)+1];
//					float z1 = p_xyz[(idx1*3)+2];
//
//					float x2 = p_xyz[(idx2*3)+0];
//					float y2 = p_xyz[(idx2*3)+1];
//					float z2 = p_xyz[(idx2*3)+2];
//
//					float x3 = x0 + px;
//					float y3 = y0 + py;
//					float z3 = z0 + pz;
//	
//					float x4 = x1 + px;
//					float y4 = y1 + py;
//					float z4 = z1 + pz;
//	
//					float x5 = x2 + px;
//					float y5 = y2 + py;
//					float z5 = z2 + pz;
//
//					GX::SetBlendMode ( GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_FALSE, GX_TRUE, GX_FALSE );
//
//					switch ( shape )
//					{
//						case 0:
//							GX::Begin( GX_TRIANGLES, GX_VTXFMT0, 3 ); 
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::End();
//							GX::Begin( GX_TRIANGLES, GX_VTXFMT0, 3 ); 
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::End();
//							break;
//						case 1:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 6 ); 
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::End();
//							break;
//						case 2:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 6 ); 
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::End();
//							break;
//						case 3:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 8 ); 
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::End();
//							break;
//						case 4:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 6 ); 
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::End();
//							break;
//						case 5:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 8 ); 
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::End();
//							break;
//						case 6:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 8 ); 
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::End();
//							break;
//						case 7:
//						default:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 10 ); 
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::End();
//							break;
//					}
//					GX::SetBlendMode ( GX_BM_SUBTRACT, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_FALSE, GX_TRUE, GX_FALSE );
//
//					switch ( shape )
//					{
//						case 0:
//							GX::Begin( GX_TRIANGLES, GX_VTXFMT0, 3 ); 
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::End();
//							GX::Begin( GX_TRIANGLES, GX_VTXFMT0, 3 ); 
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::End();
//							break;
//						case 1:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 6 ); 
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::End();
//							break;
//						case 2:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 6 ); 
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::End();
//							break;
//						case 3:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 8 ); 
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::End();
//							break;
//						case 4:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 6 ); 
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::End();
//							break;
//						case 5:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 8 ); 
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::End();
//							break;
//						case 6:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 8 ); 
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::End();
//							break;
//						case 7:
//						default:
//							GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 10 ); 
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::Position3f32( x1, y1, z1 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x4, y4, z4 );
//							GX::Position3f32( x3, y3, z3 );
//							GX::Position3f32( x5, y5, z5 );
//							GX::Position3f32( x0, y0, z0 );
//							GX::Position3f32( x2, y2, z2 );
//							GX::End();
//							break;
//					}
//				}
//			}
//			delete p_xyz;
//#endif
//			GX::SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
////			if ( !g_in_cutscene ) GX::SetClipMode( GX_CLIP_ENABLE );
//			GX::SetProjectionv( p_pm );
//		}
//		else
		{
			GX::LoadNrmMtxImm((MtxPtr)&root, GX_PNMTX0);
			// Render the scene & see if any meshes were drawn. If so, transform the
			// vertex buffer for use when the GPU comes to actually render it..
			NxNgc::EngineGlobals.poly_culling = false;

			make_scene_visible( GetScene() );
			GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_POS, GX_POS_XYZ, GX_S16, 6 );
			GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_POS, GX_POS_XYZ, GX_S16, 6 );
			GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_POS, GX_POS_XYZ, GX_S16, 6 );
			GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_POS, GX_POS_XYZ, GX_S16, 6 );
			GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_POS, GX_POS_XYZ, GX_S16, 6 );
			GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_POS, GX_POS_XYZ, GX_S16, 6 );
			render_scene( GetScene(), mp_posNormRenderBuffer, render_flags | vRENDER_NO_CULLING );
			GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
			GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
			GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
			GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
			GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
			GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
		}
	}
	else
	{
		if( ( m_flags & CInstance::INSTANCE_FLAG_LIGHTING_ALLOWED ) == 0 ) render_flags |= NxNgc::vRENDER_LIT;
		GX::LoadNrmMtxImm((MtxPtr)&tr, GX_PNMTX0);
		if ( GetScene()->m_numHierarchyObjects && GetBoneTransforms() )
		{
			// Car or something...
			NxNgc::EngineGlobals.poly_culling = true;
			make_scene_visible( GetScene() );
			render_scene( GetScene(), NULL, render_flags | vRENDER_NO_CULLING, GetBoneTransforms(), GetTransform(), GetNumBones() );
		}
		else
		{
			if( ( m_flags & CInstance::INSTANCE_FLAG_LIGHTING_ALLOWED ) == 0 ) render_flags |= NxNgc::vRENDER_LIT;
			// No bones, just render normally. Must be a trashcan or something.
			NxNgc::EngineGlobals.poly_culling = false;
			make_scene_visible( GetScene() );
//			int meshes_rendered = cull_scene( GetScene(), vRENDER_OCCLUDED, GetTransform() ); 
			render_scene( GetScene(), NULL, render_flags );
//			SetMeshesRendered( meshes_rendered );
		}
	}
}



void process_instances( void )
{
	current_index = 0;
	
	if ( g_mc_hack ) return;

	// Seed the static pointer off to NULL, otherwise if there is only one object with bone transforms, it will never update.
	pLastBoneTransforms = NULL;
	
	// First go through and build a list of the visible instances.
	CInstance *p_instance = pFirstInstance;
	while( p_instance )
	{
#ifndef __NOPT_FINAL__
		p_instance->m_object_num = g_object;
		g_object++;
#endif		// __NOPT_FINAL__
		if( p_instance->GetActive() && p_instance->GetScene()->mp_scene_data )
		{
			p_instances[current_index++] = p_instance;
		}
		p_instance = p_instance->GetNextInstance();
	}

	int save_g_object = g_object;

	// Now sort the list based on bone transform and number of semitransparent objects in scene.
	if( current_index > 0 )
	{
		if( current_index > 1 ) qsort( p_instances, current_index, sizeof( CInstance* ), sort_by_bone_transform );
	
		int i = 0;
		Mth::Matrix *pLast = NULL;
		ROMtx bone_mtx[60];

		for( ; i < current_index; ++i )
		{
#ifndef __NOPT_FINAL__
			g_object = p_instances[i]->m_object_num;
#endif		// __NOPT_FINAL__

			if(( p_instances[i]->GetBoneTransforms() == NULL ) || ( p_instances[i]->GetScene()->m_numHierarchyObjects > 0 ))
				break;

			p_instances[i]->Transform( 0 /*flags*/ | vRENDER_OPAQUE | vRENDER_SEMITRANSPARENT, bone_mtx, pLast );
			pLast = p_instances[i]->GetBoneTransforms();
		}

//		for( ; i < current_index; ++i )
//		{
//#ifndef __NOPT_FINAL__
//			g_object = p_instances[i]->m_object_num;
//#endif		// __NOPT_FINAL__
//
//			p_instances[i]->Transform( 0 /*flags*/ | vRENDER_OPAQUE | vRENDER_SEMITRANSPARENT, NULL, NULL );
//		}
	}
	g_object = save_g_object;
}



s16	*CInstance::GetPosNormalBuffer( int buffer )
{
	int num_vertex = mp_scene->mp_dl->mp_object_header->m_num_skin_verts;
	Dbg_MsgAssert(num_vertex, ("No verts in scene."));

	s16 * rv = (s16*)NsBuffer::alloc( sizeof(s16) * 6 * (num_vertex+1) ); 
	return rv;
}

CInstance::CInstance( sScene *pScene, Mth::Matrix &transform, int numBones, Mth::Matrix *pBoneTransforms )
{
	SetTransform( transform );
	mp_bone_transforms	= pBoneTransforms;
	m_num_bones			= numBones;
	mp_scene			= pScene;
	m_active			= true;
	m_flags				= 0;
	mp_model			= NULL;

	mp_next_instance	= pFirstInstance;
	pFirstInstance		= this;

	mp_posNormBuffer = NULL;
	m_visibility_mask = 0xff;

	// Check to see whether this instance is allowed to be lit or not (for non-skinned instances).
	// Currently, we assume that instances with a model containing valid normals requires lighting, except in the
	// situation where that model contains a mesh specifically flagged not to be lit.
	if(( pBoneTransforms == NULL ) || ( GetScene()->m_numHierarchyObjects > 0 ))
	{
		for( int m = 0; m < pScene->m_num_filled_meshes; ++m )
		{
			NxNgc::sMesh *p_mesh = pScene->mpp_mesh_list[m];
			if( !pScene->mp_scene_data->m_num_nrm )
				return;
			if( p_mesh->mp_dl->m_material.p_header->m_flags & (1<<7) )
				return;
		}
		m_flags |= CInstance::INSTANCE_FLAG_LIGHTING_ALLOWED;
	}
}




CInstance::~CInstance()
{
	if( m_flags & INSTANCE_FLAG_DELETE_ATTACHED_SCENE )
	{
		if( mp_scene )
		{
			delete mp_scene;
			mp_scene = NULL;
		}
	}
	
//	if ( mp_posNormBuffer )
//	{
//		delete  mp_posNormBuffer;
//		mp_posNormBuffer = NULL;
//	}

	// Remove this instance from the list.
	CInstance **pp_instance = &pFirstInstance;
	while( *pp_instance )
	{
		if( *pp_instance == this )
		{
			*pp_instance = mp_next_instance;
			break;
		}
		else
		{
			pp_instance = &(( *pp_instance )->mp_next_instance );
		}
	}
}

} // namespace NxNgc







