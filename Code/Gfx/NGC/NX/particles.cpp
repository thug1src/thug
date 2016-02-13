//
///*****************************************************************************
//**																			**
//**			              Neversoft Entertainment			                **
//**																		   	**
//**				   Copyright (C) 1999 - All Rights Reserved				   	**
//**																			**
//******************************************************************************
//**																			**
//**	Project:		GFX (Graphics Library)									**
//**																			**
//**	Module:			Graphics (GFX)		 									**
//**																			**
//**	File name:		particle.cpp											**
//**																			**
//**	Created:		03/27/02	-	dc										**
//**																			**
//**	Description:	Low level particle rendering code						**
//**																			**
//*****************************************************************************/
//
//int gParticleMem = 0;
//
///*****************************************************************************
//**							  	  Includes									**
//*****************************************************************************/
//		
//#include <core/math.h>
//#include <gfx/debuggfx.h>
//#include "render.h"
//#include "particles.h"
//#include "scene.h"
//#include <gfx/ngc/nx/instance.h>
//#include <gfx/ngc/p_nxmesh.h>
//#include <gfx/NxTexMan.h>
//#include <gfx/ngc/p_nxtexture.h>
//
///*****************************************************************************
//**								DBG Information								**
//*****************************************************************************/
//
//
///*****************************************************************************
//**								  Externals									**
//*****************************************************************************/
//
//namespace NxNgc
//{
//
//
///*****************************************************************************
//**								   Defines									**
//*****************************************************************************/
//
///*****************************************************************************
//**								Private Types								**
//*****************************************************************************/
//
///*****************************************************************************
//**								 Private Data								**
//*****************************************************************************/
//
///*****************************************************************************
//**								 Public Data								**
//*****************************************************************************/
//
///*****************************************************************************
//**							  Private Prototypes							**
//*****************************************************************************/
//
///*****************************************************************************
//**							   Private Functions							**
//*****************************************************************************/
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//
///*****************************************************************************
//**							   Public Functions								**
//*****************************************************************************/
//
//static BlendModes get_texture_blend( uint32 blend_checksum )
//{
//	BlendModes rv = NxNgc::vBLEND_MODE_DIFFUSE;
//	switch ( blend_checksum )
//	{
//		case 0x54628ed7:		// Blend
//			rv = NxNgc::vBLEND_MODE_BLEND;
//			break;
//		case 0x02e58c18:		// Add
//			rv = NxNgc::vBLEND_MODE_ADD;
//			break;
//		case 0xa7fd7d23:		// Sub
//		case 0xdea7e576:		// Subtract
//			rv = NxNgc::vBLEND_MODE_SUBTRACT;
//			break;
//		case 0x40f44b8a:		// Modulate
//			rv = NxNgc::vBLEND_MODE_MODULATE;
//			break;
//		case 0x68e77f40:		// Brighten
//			rv = NxNgc::vBLEND_MODE_BRIGHTEN;
//			break;
//		case 0x18b98905:		// FixBlend
//			rv = NxNgc::vBLEND_MODE_BLEND_FIXED;
//			break;
//		case 0xa86285a1:		// FixAdd
//			rv = NxNgc::vBLEND_MODE_ADD_FIXED;
//			break;
//		case 0x0d7a749a:		// FixSub
//		case 0x0eea99ff:		// FixSubtract
//			rv = NxNgc::vBLEND_MODE_SUB_FIXED;
//			break;
//		case 0x90b93703:		// FixModulate
//			rv = NxNgc::vBLEND_MODE_MODULATE_FIXED;
//			break;
//		case 0xb8aa03c9:		// FixBrighten
//			rv = NxNgc::vBLEND_MODE_BRIGHTEN_FIXED;
//			break;
//		case 0x515e298e:		// Diffuse
//		case 0x806fff30:		// None
//			rv = NxNgc::vBLEND_MODE_DIFFUSE;
//			break;
//		default:
//			Dbg_MsgAssert(0,("Illegal blend mode specified. Please use (fix)blend/add/sub/modulate/brighten or diffuse/none."));
//			break;
//	}
//	return rv;
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//sParticleSystem::sParticleSystem( uint32 max_particles, uint32 texture_checksum, uint32 blendmode_checksum, int fix )
//{
//	// Create a scene to store the mesh.
//	mp_scene = new sScene();
//
//	// Get the texture.
//
//	uint32 flags = MATFLAG_TRANSPARENT;
//	Nx::CTexture *p_texture;
//	Nx::CNgcTexture *p_ngc_texture;
//	NxNgc::sTexture * p_engine_texture = NULL;
//
//	p_texture = Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( texture_checksum );
//	p_ngc_texture = static_cast<Nx::CNgcTexture*>( p_texture );
//	if ( p_ngc_texture )
//	{
//		p_engine_texture = p_ngc_texture->GetEngineTexture(); 
//		flags |=MATFLAG_TEXTURED;
//	}
//
//	// Create a material used to render the mesh.
//	sMaterial *p_material				= new sMaterial();
//	p_material->m_sorted				= 0;
//	p_material->m_no_bfc				= 1;
//	p_material->Flags[0]				= flags;		// 0x40 makes it semitransparent.
//	p_material->Checksum				= 0x5c41d978;	// particle_system	//(uint32)rand() * (uint32)rand();
//	p_material->Passes					= 1;
//	p_material->pTex[0]					= p_engine_texture;
//	p_material->blendMode[0]			= (uint8)get_texture_blend( blendmode_checksum );
//	p_material->fixAlpha[0]				= fix & 255;
//	p_material->UVAddressing[0]			= 0;
//	p_material->matcol[0]				= (GXColor){128,128,128,255};
//	p_material->AlphaCutoff				= 1;
//	p_material->m_num_wibble_vc_anims	= 0;
//	p_material->mp_wibble_vc_params		= NULL;
//	p_material->mp_wibble_vc_colors		= NULL;
//	p_material->m_pUVControl			= NULL;
//
//	// Create a hash table to store the material...
//	mp_scene->mp_material_array = p_material;
//	mp_scene->m_num_materials = 1;
//
//	// Initialize the material (builds the display list).
//	InitializeMaterial( p_material );
//
//	// Create a mesh which will be used to render the system.
//	mp_mesh = new sMesh();
//
//	// Create a position buffer.
//#ifdef SHORT_VERT
//	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
//	float *p_pos		= new float[max_particles * 3 * 4];
//	Mem::Manager::sHandle().PopContext();
//	s16 *p_pos16		= new s16[max_particles * 3 * 4];
//	gParticleMem += max_particles * 3 * 4 * 2;
//#else
//	float *p_pos		= new float[max_particles * 3 * 4];
//#endif		// SHORT_VERT
//
//	for( uint32 lp = 0; lp < ( max_particles * 3 * 4 ); ++lp )
//	{
//		p_pos[lp] = 1024.0f;
//#ifdef SHORT_VERT
//		p_pos16[lp] = (s16)(1024.0f*(1 << 0));
//#endif		// SHORT_VERT
//	}
//	
//	// Save pointer to bounding sphere so that the particle system can update it.
//	mp_sphere = mp_mesh->m_sphere;
//
//	// Create an index buffer.
//	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
//	uint16 *p_indices	= new uint16[(max_particles * 4)+2];
//	gParticleMem += ( (max_particles * 4)+2 ) * 2;
//	Mem::Manager::sHandle().PopContext();
//
//	// Fill the index buffer.
//	uint16 *p16 = p_indices;
//	*p16++ = max_particles * 4;
//	for( uint32 i = 0; i < max_particles; ++i )
//	{
//		*p16++ = (uint16)i * 4 + 0;
//		*p16++ = (uint16)i * 4 + 1;
//		*p16++ = (uint16)i * 4 + 2;
//		*p16++ = (uint16)i * 4 + 3;
//	}
//	*p16++ = 0;
//	
//	// Create texture coordinates.
//	s16 *p_uv = NULL;
//	if ( p_engine_texture )
//	{
//		p_uv = new s16[max_particles * 2 * 4];
//		gParticleMem += max_particles * 2 * 4 * 2;
//
//		for( uint32 lp = 0; lp < max_particles; ++lp )
//		{
//			p_uv[(lp*2*4)+0] = (s16)(0.0f * ((float)(1<<10)));
//			p_uv[(lp*2*4)+1] = (s16)(0.0f * ((float)(1<<10)));
//			p_uv[(lp*2*4)+2] = (s16)(1.0f * ((float)(1<<10)));
//			p_uv[(lp*2*4)+3] = (s16)(0.0f * ((float)(1<<10)));
//			p_uv[(lp*2*4)+4] = (s16)(1.0f * ((float)(1<<10)));
//			p_uv[(lp*2*4)+5] = (s16)(1.0f * ((float)(1<<10)));
//			p_uv[(lp*2*4)+6] = (s16)(0.0f * ((float)(1<<10)));
//			p_uv[(lp*2*4)+7] = (s16)(1.0f * ((float)(1<<10)));
//		}
//	}
//
//	// Create Color list.
//	GXColor* p_color = new GXColor[max_particles * 4];
//	gParticleMem += max_particles * 4 * 2;
//
//	for( uint32 cc = 0; cc < max_particles; ++cc )
//	{
//		p_color[(cc*4)+0] = (GXColor){128,128,128,255};
//		p_color[(cc*4)+1] = (GXColor){128,128,128,255};
//		p_color[(cc*4)+2] = (GXColor){128,128,128,255};
//		p_color[(cc*4)+3] = (GXColor){128,128,128,255};
//	}
//	
//	// Set up the mesh.
//	mp_mesh->Initialize( max_particles * 4,		// Num vertices
//						p_pos,					// Positions
//#ifdef SHORT_VERT
//						p_pos16,
//						0,
//						0.0f,
//						0.0f,
//						0.0f,
//#endif		// SHORT_VERT
//						NULL,					// Normals
//						p_uv,					// Texture coordinates
//						1,						// Number of tc sets
//						(uint32*)p_color,		// Colors
//						max_particles * 4,		// Num indices (4 per particle)
//						p_indices,
//						p_material->Checksum,	// Material checksum
//						mp_scene,
//						0,						// Double lists.
//						NULL,
//						0,						// Single lists.
//						0,						// local index of 0 means data will be deleted.
//						GX_QUADS,				// Type of mesh. 
//						0,						// flags
//						NULL );					// Wibble.
//
//#ifdef SHORT_VERT
//	delete p_pos;
//#endif		// SHORT_VERT
//
////	// Also adjust the bounding sphere and bounding box of the mesh.
////	p_mesh->m_sphere_center = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
////	p_mesh->m_sphere_radius = 360.0f;
////	p_mesh->m_bbox.SetMin( Mth::Vector( -360.0f, -360.0f, -360.0f ));
////	p_mesh->m_bbox.SetMax( Mth::Vector(  360.0f,  360.0f,  360.0f ));
//	
//	// Now add the mesh to the scene.
//	mp_scene->CountMeshes( 1, &mp_mesh );
//	mp_scene->CreateMeshArrays();
//	mp_scene->AddMeshes( 1, &mp_mesh );
//	mp_scene->SortMeshes();
//
//	// Create an instance and attach the mesh to it.
//	Mth::Matrix temp;
//	temp.GetRight().Set( 1.0f, 0.0f, 0.0f );
//	temp.GetUp().Set( 0.0f, 1.0f, 0.0f );
//	temp.GetAt().Set( 0.0f, 0.0f, 1.0f );
//	temp.GetPos().Set( 0.0f, 0.0f, 0.0f );
//	mp_instance = new NxNgc::CInstance( mp_scene, temp, 1, NULL );
//
//	mp_material = p_material;
//
//	mp_engine_texture = p_engine_texture;
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//sParticleSystem::~sParticleSystem()
//{
// 	// Delete the single sMesh from the scene.
//	Dbg_Assert( mp_scene->m_num_opaque_entries == 0 );
//	Dbg_Assert( mp_scene->m_num_semitransparent_entries == 1 );
//	delete mp_scene->m_semitransparent_meshes[0];
//	
//	// Deleting the sScene will also remove the material table (which removes the material).
//	DeleteScene( mp_scene );
//
//	if ( mp_instance ) delete mp_instance;
////	mp_scene->RemoveMeshes( &mp_mesh );
////	delete mp_mesh;
////	delete mp_scene;
////	delete mp_instance;
//}
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void sParticleSystem::Render( void )
//{
////	render_scene( mp_scene, NULL, vRENDER_OPAQUE );
//
////	// Switch vertex buffers on the mesh.
////	mp_scene->m_opaque_meshes[0]->SwapVertexBuffers();
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//#ifdef SHORT_VERT
//s16 *sParticleSystem::GetVertexWriteBuffer( void )
//#else
//float *sParticleSystem::GetVertexWriteBuffer( void )
//#endif		// SHORT_VERT
//{
//	return mp_scene->m_semitransparent_meshes[0]->mp_posBuffer;
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//GXColor *sParticleSystem::GetColorWriteBuffer( void )
//{
//	return (GXColor*)mp_scene->m_semitransparent_meshes[0]->mp_colBuffer;
//}
//
//} // namespace NxNgc
//
//

