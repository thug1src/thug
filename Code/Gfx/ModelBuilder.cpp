//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       ModelBuilder.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/16/2001
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/modelbuilder.h>

#include <gfx/facetexture.h>
#include <gfx/GfxUtils.h>
#include <gfx/modelappearance.h>
#include <gfx/Nx.h>
#include <gfx/NxModel.h>
#include <gfx/nxtexman.h>
#include <gfx/nxtexture.h>
#include <gfx/skeleton.h>

#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/checksum.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Gfx
{

// NOTES:

// Given a CModelAppearance, the CModelBuilder generates the 
// appropriate Nx::CModel (creating its Nx::CGeoms, modulating
// its color, replacing its textures, etc.)

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

// some handy checksums
const uint32 vCHECKSUM_PART = 0xb6f08f39;							// part
const uint32 vCHECKSUM_USE_DEFAULT_HSV = 0x97dbdde6;				// use_default_hsv
const uint32 vCHECKSUM_HUE = 0x6e94f918;							// h
const uint32 vCHECKSUM_SATURATION = 0xe4f130f4;						// s
const uint32 vCHECKSUM_VALUE = 0x949bc47b;							// v
const uint32 vCHECKSUM_USE_DEFAULT_SCALE = 0x5a96985d;				// use_default_scale
const uint32 vCHECKSUM_X = 0x7323e97c;								// x
const uint32 vCHECKSUM_Y = 0x0424d9ea;								// y
const uint32 vCHECKSUM_Z = 0x9d2d8850;								// z
const uint32 vCHECKSUM_REPLACE = 0xf362dbba;						// replace
const uint32 vCHECKSUM_WITH = 0x676f1df1;							// with
const uint32 vCHECKSUM_IN = 0xa01371b1;								// in
const uint32 vCHECKSUM_REPLACE1 = 0x7a993873;						// replace1
const uint32 vCHECKSUM_WITH1 = 0x9b03adad;							// with1
const uint32 vCHECKSUM_IN1 = 0xed189051;							// in1
const uint32 vCHECKSUM_REPLACE2 = 0xe39069c9;						// replace2
const uint32 vCHECKSUM_WITH2 = 0x020afc17;							// with2
const uint32 vCHECKSUM_IN2 = 0x7411c1eb;							// in2
const uint32 vCHECKSUM_REPLACE3 = 0x9497595f;						// replace3
const uint32 vCHECKSUM_WITH3 = 0x750dcc81;							// with3
const uint32 vCHECKSUM_IN3 = 0x0316f17d;							// in3
const uint32 vCHECKSUM_REPLACE4 = 0x0af3ccfc;						// replace4
const uint32 vCHECKSUM_WITH4 = 0xeb695922;							// with4
const uint32 vCHECKSUM_IN4 = 0x9d7264de;							// in4
const uint32 vCHECKSUM_REPLACE5 = 0x7df4fc6a;						// replace5
const uint32 vCHECKSUM_WITH5 = 0x9c6e69b4;							// with5
const uint32 vCHECKSUM_IN5 = 0xea755448;							// in5
const uint32 vCHECKSUM_REPLACE6 = 0xe4fdadd0;						// replace6
const uint32 vCHECKSUM_WITH6 = 0x0567380e;							// with6
const uint32 vCHECKSUM_IN6 = 0x737c05f2;							// in6
const uint32 vCHECKSUM_MESH = 0x1e90c5a9;							// mesh
const uint32 vCHECKSUM_MESH1 = 0xfeca8bb3;							// mesh1
const uint32 vCHECKSUM_MESH2 = 0x67c3da09;							// mesh2
const uint32 vCHECKSUM_MESH3 = 0x10c4ea9f;							// mesh3
const uint32 vCHECKSUM_MESH4 = 0x8ea07f3c;							// mesh4
const uint32 vCHECKSUM_REMOVE = 0x977fe2cf;							// remove
const uint32 vCHECKSUM_TARGET = 0xb990d003;							// target
const uint32 vCHECKSUM_BODY_SHAPE = 0x812684ef;						// body_shape
const uint32 vCHECKSUM_APPLY_NORMAL_MODE = 0x2700a288;				// apply_normal_mode
const uint32 vCHECKSUM_BODY = 0x2457f44d;							// body
const uint32 vCHECKSUM_NO_SCALING_ALLOWED = 0xb25e39fd;				// no_scaling_allowed

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

Nx::CModel* CModelBuilder::mp_model = NULL;
Gfx::CSkeleton* CModelBuilder::mp_skeleton = NULL;
CModelAppearance* CModelBuilder::mp_appearance = NULL;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModelBuilder::CModelBuilder( bool useAssetManager, uint32 texDictOffset )
{
	// parameters that will be used for the duration of
	// the model-building:

	// whether the asset manager should be used
	// (it should NOT, for skaters and other
	// things that do texture replacement/poly
	// removal)
	m_useAssetManager = useAssetManager;

	// used for unique-ifying the texture dictionaries
	// when you've got multiple skaters
	m_texDictOffset = texDictOffset;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModelBuilder::BuildModel( CModelAppearance* pAppearance, Nx::CModel* pModel, Gfx::CSkeleton* pSkeleton, uint32 buildScriptName )
{
//	Tmr::Time baseTime = Tmr::ElapsedTime(0);

	Dbg_Assert( pAppearance );
	Dbg_Assert( pModel );

	// don't destroy the existing model...
	// leave it up to the particular buildscript
//	pModel->ClearGeoms();

	Dbg_MsgAssert( !mp_model, ( "Static temporary model already exists" ) );
	mp_model = pModel;
	
	Dbg_MsgAssert( !mp_skeleton, ( "Static skeleton already exists" ) );
	mp_skeleton = pSkeleton;
	
	Dbg_MsgAssert( !mp_appearance, ( "Static temporary appearance already exists" ) );
	mp_appearance = new CModelAppearance;
	
	// clear it out from the previous use
	mp_appearance->Init();

	// GJ 9/8/03:  since it's a temporary variable, we don't want 
	// it on the skater geom heap (which would fragment it on the
	// PS2 with 17K worth of face texture data)
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

  	// copy over the desired appearance
	// (must make a copy, so that we can
	// make changes to it)
	*mp_appearance = *pAppearance;
	
   	Mem::Manager::sHandle().PopContext();

	// this script is responsible for looping
	// through all the supported partChecksums
	// and running all the supported operations
	// on them.  Which partChecksums and operations
	// are "supported" should be completely in
	// script.
//    Dbg_Message( "BuildModel - %s", Script::FindChecksumName(buildScriptName) );
    Script::RunScript( buildScriptName, NULL, this );
    
	mp_model = NULL;
	mp_skeleton = NULL;

	delete mp_appearance;
	mp_appearance = NULL;

//	Dbg_Message( "BuildModel took %d ms", Tmr::ElapsedTime( baseTime ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( checksum )
	{
		case 0x30e2951d:		// DebugPrintAppearance
		{
			// print out the appearance to make sure
			// it's coming through correctly in net games
			mp_appearance->PrintContents();
		}
		return true;

		case 0x53da94f1:		// ModelAddGeom
		{
			uint32 partChecksum;
			pParams->GetChecksum( vCHECKSUM_PART, &partChecksum, Script::ASSERT );
			model_add_geom(partChecksum);
		}
		return true;

		case 0x681a03af:		// ModelHideGeom
		{
			uint32 partChecksum;
			pParams->GetChecksum( vCHECKSUM_PART, &partChecksum, Script::ASSERT );
		
			int hidden;
			pParams->GetInteger( NONAME, &hidden, Script::ASSERT );

			model_hide_geom(partChecksum, hidden);
		}
		return true;

		case 0xaee6e915:		// GeomModulateColor
		{
			geom_modulate_color( pParams );
		}
		return true;

		case 0x18924da1:		// GeomSetUVOffset
		{
			uint32 partChecksum;
			pParams->GetChecksum( vCHECKSUM_PART, &partChecksum, Script::ASSERT );
			
			uint32 material;
			pParams->GetChecksum( CRCD(0x83418a6a,"material"), &material, Script::ASSERT );

			int pass;
			pParams->GetInteger( CRCD(0x318f2bdb,"pass"), &pass, Script::ASSERT );

			geom_set_uv_offset(partChecksum, material, pass);
		}
		return true;

		case 0x3bf41299:		// GeomAllocateUVMatrixParams
		{
			uint32 material;
			pParams->GetChecksum( CRCD(0x83418a6a,"material"), &material, Script::ASSERT );

			int pass;
			pParams->GetInteger( CRCD(0x318f2bdb,"pass"), &pass, Script::ASSERT );

			geom_allocate_uv_matrix_params(material, pass);
		}
		return true;

		case 0xaa42a104:		// GeomReplaceTexture
		{
			uint32 partChecksum;
			pParams->GetChecksum( vCHECKSUM_PART, &partChecksum, Script::ASSERT );
			geom_replace_texture(partChecksum);
		}
		return true;
		
		case 0x9041e901:		// ModelRemovePolys
		{
			model_remove_polys();
		}
		return true;

		case 0x50f1285b:		// ModelApplyObjectScale
		{
			Script::CStruct* pBodyShapeStructure = NULL;
			if ( pParams->GetStructure( vCHECKSUM_BODY_SHAPE, &pBodyShapeStructure, false ) )
			{
				Mth::Vector vec = mp_model->GetScale();
				
				Mth::Vector theScale( 1.0f, 1.0f, 1.0f, 1.0f );
				if ( Gfx::GetScaleFromParams( &theScale, pBodyShapeStructure ) )
				{
					vec[X] *= theScale[X];
					vec[Y] *= theScale[Y];
					vec[Z] *= theScale[Z];
					vec[W] *= theScale[W];

					// if the body shape has a scale
					mp_model->SetScale( vec );
				}
			}
			else
			{
				model_apply_object_scale();
			}
		}
		return true;
		
		case 0x21aec583:		// ModelApplyBoneScale
		{
			if ( pParams->ContainsComponentNamed( CRCD(0x3f5f5bc2,"bone_scaling") ) )
			{
				uint32 partChecksum;
				pParams->GetChecksum( vCHECKSUM_PART, &partChecksum, Script::ASSERT );
				model_apply_bone_scale( partChecksum );
			}
		}
		return true;
		
		case 0xc0bc8271:		// ModelApplyBodyShape
		{
			Script::CStruct* pBodyShapeStructure = NULL;
			if ( pParams->GetStructure( vCHECKSUM_BODY_SHAPE, &pBodyShapeStructure, false ) )
			{
				// GJ:  PATCH TO FIX STEVE CABALLERO'S INCORRECT WEIGHTING IN BIGHEAD MODE
				Script::CStruct* pActualStruct = mp_appearance->GetActualDescStructure( CRCD(0x650fab6d,"skater_m_head") );
				if ( pActualStruct && pParams->ContainsFlag( CRCD(0x3c4d849d,"is_bighead_cheat") ) ) 
				{
				   pActualStruct->GetStructure( CRCD(0x1ade0dd8,"bighead_scale_info"), &pBodyShapeStructure );
				}

				// if a body shape was explicitly specified
				// (for kid and gorilla modes)
				if ( mp_skeleton )
				{
					return mp_skeleton->ApplyBoneScale( pBodyShapeStructure );
				}
	        }
			else
			{
				model_apply_body_shape();
			}
		}
		return true;

		case 0x73ab02bc:		// ModelApplyFaceTexture
		{
#ifdef __PLAT_NGPS__
			bool success = true;

			Gfx::CFaceTexture* pFaceTexture = mp_appearance->GetFaceTexture() ;
												   
			if ( pFaceTexture && pFaceTexture->IsValid() )
			{
				if ( mp_model )
				{
					const char* pSrcTexture;
					pParams->GetText( CRCD(0x9fbbdb72,"src"), &pSrcTexture, Script::ASSERT );

					// by default, it searches globally in the model for the correct texture
					uint32 partChecksumToReplace = Nx::CModel::vREPLACE_GLOBALLY;
					pParams->GetChecksum( CRCD(0xa01371b1,"in"), &partChecksumToReplace, Script::NO_ASSERT );
                    
					success = mp_model->ApplyFaceTexture( pFaceTexture, pSrcTexture, partChecksumToReplace );
				}
			}
			
			return success;
#else
			// X-box and NGC don't have to implement these
			// because they don't do face textures...
			return true;
#endif
		}

		case 0x6516b484:		// AppearanceAllowScalingCheat
		{
			Script::CStruct* pActualStruct = mp_appearance->GetActualDescStructure( vCHECKSUM_BODY );
			if ( pActualStruct ) 
			{
				return !pActualStruct->ContainsFlag( vCHECKSUM_NO_SCALING_ALLOWED );
			}

			return true;
		}
		break;

		case 0xcdeea636:		// ModelResetScale
		{
			model_reset_scale();
		}
		return true;

		case 0xbff957a9:		// ModelClearGeom
		{
			uint32 partChecksum;
			pParams->GetChecksum( vCHECKSUM_PART, &partChecksum, Script::ASSERT );
			model_clear_geom( partChecksum );
		}
		return true;

		case 0x0ecf0248:		// ModelClearAllGeoms
		{
			model_clear_all_geoms();
		}
		return true;

		case 0xb1c39a54:		// ModelRunScript
		{
			uint32 partChecksum;
			pParams->GetChecksum( vCHECKSUM_PART, &partChecksum, Script::ASSERT );
			
			uint32 targetChecksum;
			pParams->GetChecksum( vCHECKSUM_TARGET, &targetChecksum, Script::ASSERT );

			// runs an embedded script
			model_run_script(partChecksum, targetChecksum);
		}
		return true;

		case 0xcefe8478:		// ModelFinalize
		{
			model_finalize();
		}
		return true;
	}

	return Obj::CObject::CallMemberFunction( checksum, pParams, pScript );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::model_hide_geom( uint32 partChecksum, bool hidden )
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_appearance );

/*
	if ( !Script::GetArray( partChecksum, Script::NO_ASSERT ) )
	{
		// no array involved
		return false;
	}
*/

	mp_model->HideGeom( partChecksum, hidden );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::model_add_geom( uint32 partChecksum )
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_appearance );

	Script::CStruct* pActualStruct = mp_appearance->GetActualDescStructure( partChecksum );
	if ( pActualStruct ) 
	{
		int supports_multiple_colors = 0;
		pActualStruct->GetInteger( CRCD(0x92b43e79,"multicolor"), &supports_multiple_colors, Script::NO_ASSERT );

#ifdef __PLAT_NGPS__
		Gfx::CFaceTexture* pFaceTexture = mp_appearance->GetFaceTexture() ;

		// GJ:  THPS5 KLUDGE
		// WillEventuallyHaveFaceTexture is a flag used in net games...
		// this is needed because there is no face texture pointer
		// in net games (because it's handled in a separate net packet)...

		if ( ( pFaceTexture && pFaceTexture->IsValid() ) || ( mp_appearance->WillEventuallyHaveFaceTexture() ) )
		{
			// if the special parameter doesn't exist, then
			// fall through to the normal mesh-loading code...
			
			const char* pMeshName;
			if ( pActualStruct->GetString( CRCD(0xc9aed80f,"mesh_if_facemapped"), &pMeshName, Script::NO_ASSERT ) )
			{
				mp_model->AddGeom( pMeshName, partChecksum, m_useAssetManager, m_texDictOffset, false, supports_multiple_colors );
				return true;
			}
		}
#endif
		{
			const char* pMeshName;
			if ( pActualStruct->GetString( vCHECKSUM_MESH, &pMeshName, Script::NO_ASSERT ) )
			{
				mp_model->AddGeom( pMeshName, partChecksum, m_useAssetManager, m_texDictOffset, false, supports_multiple_colors );
			}
			if ( pActualStruct->GetString( vCHECKSUM_MESH1, &pMeshName, Script::NO_ASSERT ) )
			{
				mp_model->AddGeom( pMeshName, partChecksum, m_useAssetManager, m_texDictOffset, false, supports_multiple_colors );
			}
			if ( pActualStruct->GetString( vCHECKSUM_MESH2, &pMeshName, Script::NO_ASSERT ) )
			{
				mp_model->AddGeom( pMeshName, partChecksum, m_useAssetManager, m_texDictOffset, false, supports_multiple_colors );
			}
			if ( pActualStruct->GetString( vCHECKSUM_MESH3, &pMeshName, Script::NO_ASSERT ) )
			{
				mp_model->AddGeom( pMeshName, partChecksum, m_useAssetManager, m_texDictOffset, false, supports_multiple_colors );
			}
			if ( pActualStruct->GetString( vCHECKSUM_MESH4, &pMeshName, Script::NO_ASSERT ) )
			{
				mp_model->AddGeom( pMeshName, partChecksum, m_useAssetManager, m_texDictOffset, false, supports_multiple_colors );
			}
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::model_clear_geom( uint32 partChecksum )
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_appearance );

	// it's destructive, so make sure you're not 
	// doing it to the "real" appearance...
	mp_appearance->GetStructure()->RemoveComponent( partChecksum );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::model_clear_all_geoms( void )
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_appearance );

	mp_model->ClearGeoms();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::model_reset_scale()
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_skeleton );
	Dbg_Assert( mp_appearance );

	Mth::Vector theBoneScaleVector( 1.0f, 1.0f, 1.0f, 1.0f );
	mp_model->SetScale( theBoneScaleVector );
	
	if ( mp_skeleton )
	{
		mp_skeleton->ResetScale();
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::model_apply_body_shape()
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_skeleton );
	Dbg_Assert( mp_appearance );

	Script::CStruct* pStructure = mp_appearance->GetStructure();
	Dbg_Assert( pStructure );
	
	Script::CStruct* pBodyShapeStructure = NULL;
	if ( pStructure->GetStructure( vCHECKSUM_BODY_SHAPE, &pBodyShapeStructure, false ) )
	{
		return mp_skeleton->ApplyBoneScale( pBodyShapeStructure );
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::model_apply_bone_scale( uint32 partChecksum )
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_skeleton );
	Dbg_Assert( mp_appearance );
	
	Script::CStruct* pVirtualStruct = mp_appearance->GetVirtualDescStructure( partChecksum );
	if ( pVirtualStruct ) 
	{
		if ( !pVirtualStruct->ContainsComponentNamed( vCHECKSUM_USE_DEFAULT_SCALE ) )
		{
			return false;
		}

		int use_default_scale;
		pVirtualStruct->GetInteger( vCHECKSUM_USE_DEFAULT_SCALE, &use_default_scale );
	
		if ( use_default_scale )
		{
			return false;
		}

		int x, y, z;
		if ( pVirtualStruct->GetInteger( vCHECKSUM_X, &x, false )
			 && pVirtualStruct->GetInteger( vCHECKSUM_Y, &y, false )
			 && pVirtualStruct->GetInteger( vCHECKSUM_Z, &z, false ) )
		{
			Script::CArray* pBoneGroupArray = Script::GetArray( partChecksum, Script::NO_ASSERT );
			if ( pBoneGroupArray )
			{
				Mth::Vector theBoneScaleVector;
//				theBoneScaleVector[X] = (float)x / 100.0f;
//				theBoneScaleVector[Y] = (float)y / 100.0f;
//				theBoneScaleVector[Z] = (float)z / 100.0f;
				theBoneScaleVector[W] = 1.0f;

				for ( uint32 i = 0; i < pBoneGroupArray->GetSize(); i++ )
				{
					uint32 boneChecksum = pBoneGroupArray->GetChecksum(i);

					// get the existing scale...
					mp_skeleton->GetBoneScale( boneChecksum, &theBoneScaleVector );

					// then combine it with the new desired bone scale...
					theBoneScaleVector[X] *= ( (float)x / 100.0f );
					theBoneScaleVector[Y] *= ( (float)y / 100.0f );
					theBoneScaleVector[Z] *= ( (float)z / 100.0f );

					bool isLocalScale = true;

					// GJ:  The shoulder bones are handled slightly differently
					// so that the female skaters aren't so broad-shouldered.
					Script::CArray* pShoulderScalingArray = Script::GetArray( CRCD(0x20d9ac2f,"nonlocal_bones"), Script::NO_ASSERT );
					if ( pShoulderScalingArray )
					{
						for ( uint32 i = 0; i < pShoulderScalingArray->GetSize(); i++ )
						{
							if ( pShoulderScalingArray->GetChecksum(i) == boneChecksum )
							{
								isLocalScale = false;
							}
						}
					}

					mp_skeleton->SetBoneScale( boneChecksum, theBoneScaleVector, isLocalScale );
				}
			}
		}
	}
		
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::model_apply_object_scale()
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_appearance );

	Mth::Vector theScale( 1.0f, 1.0f, 1.0f );
	if ( Gfx::GetScaleFromParams( &theScale, mp_appearance->GetStructure() ) )
	{
		mp_model->SetScale( theScale );
		return true;
	}

	Script::CStruct* pStructure = mp_appearance->GetStructure();
	Dbg_Assert( pStructure );
	
	// sometimes the object scale can be found in the bone params
	Script::CStruct* pBodyShapeStructure = NULL;
	if ( pStructure->GetStructure( vCHECKSUM_BODY_SHAPE, &pBodyShapeStructure, false ) )
	{
		if ( Gfx::GetScaleFromParams( &theScale, pBodyShapeStructure ) )
		{
			mp_model->SetScale( theScale );
			return true;
		}
	}

	Script::CStruct* pVirtualStruct = mp_appearance->GetVirtualDescStructure( CRCD(0x8b314358,"object_scaling") );
	if ( pVirtualStruct ) 
	{
		if ( !pVirtualStruct->ContainsComponentNamed( vCHECKSUM_USE_DEFAULT_SCALE ) )
		{
			return false;
		}  	

		int use_default_scale;
		pVirtualStruct->GetInteger( vCHECKSUM_USE_DEFAULT_SCALE, &use_default_scale );
		
		if ( use_default_scale )
		{
			return false;
		}

		int x, y, z;
		if ( pVirtualStruct->GetInteger( vCHECKSUM_X, &x, false )
			 && pVirtualStruct->GetInteger( vCHECKSUM_Y, &y, false )
			 && pVirtualStruct->GetInteger( vCHECKSUM_Z, &z, false ) )
		{
			Mth::Vector theBoneScaleVector;
			theBoneScaleVector[X] = (float)x / 100.0f;
			theBoneScaleVector[Y] = (float)y / 100.0f;
			theBoneScaleVector[Z] = (float)z / 100.0f;
			theBoneScaleVector[W] = 1.0f;

			mp_model->SetScale( theBoneScaleVector );
		}

		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::model_remove_polys()
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_appearance );

	mp_model->RemovePolys();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::geom_modulate_color( Script::CStruct* pParams )
{
	uint32 partChecksum;
	pParams->GetChecksum( vCHECKSUM_PART, &partChecksum, Script::ASSERT );
			
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_appearance );

	Script::CStruct* pVirtualStruct = mp_appearance->GetVirtualDescStructure( partChecksum );
	if ( pVirtualStruct ) 
	{
		int use_default_hsv = 0;

		if ( !pVirtualStruct->ContainsComponentNamed( vCHECKSUM_USE_DEFAULT_HSV ) )
		{
			use_default_hsv = 1;
		}
		else
		{
			pVirtualStruct->GetInteger( vCHECKSUM_USE_DEFAULT_HSV, &use_default_hsv );
		}
	
		if ( use_default_hsv )
		{
			Script::CStruct* pActualStruct = mp_appearance->GetActualDescStructure( partChecksum );
			int h, s, v;
			if ( pActualStruct
				 && pActualStruct->GetInteger( CRCD(0xc6cc7f7f,"default_h"), &h, Script::NO_ASSERT )
				 && pActualStruct->GetInteger( CRCD(0x4ca9b693,"default_s"), &s, Script::NO_ASSERT )
				 && pActualStruct->GetInteger( CRCD(0x3cc3421c,"default_v"), &v, Script::NO_ASSERT ) )
			{
				// set the default color that's specified in the actual structure
				mp_model->SetColor( pParams, h, (float)s / 100.0f, (float)v / 100.0f );
			}
			else
			{
				// otherwise, clear out the color   
				mp_model->ClearColor( pParams );
			}
			
			return false;
		}
		else
		{
			int h, s, v;
			if ( pVirtualStruct->GetInteger( vCHECKSUM_HUE, &h, false )
				 && pVirtualStruct->GetInteger( vCHECKSUM_SATURATION, &s, false )
				 && pVirtualStruct->GetInteger( vCHECKSUM_VALUE, &v, false ) )
			{
				// set the non-default color that's specified in the virtual structure
				mp_model->SetColor( pParams, h, (float)s / 100.0f, (float)v / 100.0f );
			}
		}
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::geom_allocate_uv_matrix_params( uint32 matChecksum, int pass )
{
	Dbg_Assert(mp_model);
	mp_model->AllocateUVMatrixParams(matChecksum, pass);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::geom_set_uv_offset( uint32 partChecksum, uint32 matChecksum, int pass )
{
#if 0
// debugging
	if ( mp_appearance->GetActualDescStructure( Script::GenerateCRC("skater_m_head") ) )
	{
		mp_model->SetMaterialColor(matChecksum, pass, Image::RGBA(0,0,128,128));
	}
#endif	
	Script::CStruct* pVirtualStruct = mp_appearance->GetVirtualDescStructure( partChecksum );
	if ( pVirtualStruct ) 
	{
		if ( !pVirtualStruct->ContainsComponentNamed( CRCD(0x8602f6ee,"use_default_uv") ) )
		{
			return false;
		}
		
		int use_default_uv;
		pVirtualStruct->GetInteger( CRCD(0x8602f6ee,"use_default_uv"), &use_default_uv );
	
		if ( use_default_uv )
		{
// 			(do nothing;  keep the default UVs)
//			(assumes that we're starting from an unmodified model)
			return false;
		}

		float u_offset;
        pVirtualStruct->GetFloat(CRCD(0xcf6aa087,"uv_u"), &u_offset, Script::ASSERT);
		
		float v_offset;
		pVirtualStruct->GetFloat(CRCD(0x5663f13d,"uv_v"), &v_offset, Script::ASSERT);
		
		float uv_scale;
		pVirtualStruct->GetFloat(CRCD(0x266932c8,"uv_scale"), &uv_scale, Script::ASSERT);
		
		float uv_rot;
		pVirtualStruct->GetFloat(CRCD(0x1256b6c6,"uv_rot"), &uv_rot, Script::ASSERT);
		
		// GJ:  seems if you want a bigger image, you need smaller UV values...
		// is that right?

		Mth::Matrix theMat;
		theMat.Ident();
		theMat.Translate( Mth::Vector( -0.5f, -0.5f, 0.0f, 0.0f ) );
		theMat.Scale( Mth::Vector(uv_scale / 100.0f, uv_scale / 100.0f, uv_scale / 100.0f, 1.0f ) );
		theMat.Rotate( Mth::Vector( 0.0f, 0.0f, 1.0f, 1.0f ), Mth::DegToRad(uv_rot) );  // in degrees
		theMat.Translate( Mth::Vector( 0.5f, 0.5f, 0.0f, 0.0f ) );
		
		theMat.TranslateLocal( Mth::Vector( u_offset / 100.0f, v_offset / 100.0f, 0.0f, 0.0f ) );
		
		Dbg_Assert(mp_model);
		mp_model->SetUVMatrix(matChecksum, pass, theMat);
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::geom_replace_texture( uint32 partChecksum )
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_appearance );

	bool success = false;

/*
	if ( !Script::GetArray( partChecksum, Script::NO_ASSERT ) )
	{
		// no array involved
		return false;
	}
*/

	Script::CStruct* pActualStruct = mp_appearance->GetActualDescStructure( partChecksum );
	if ( pActualStruct ) 
	{
		const char* pSrcTexture;
		const char* pDestTexture;
		
		if ( pActualStruct->GetText( vCHECKSUM_REPLACE, &pSrcTexture, Script::NO_ASSERT ) )
		{
			pActualStruct->GetText( vCHECKSUM_WITH, &pDestTexture, Script::ASSERT );

			uint32 partChecksumToReplace = partChecksum;
			pActualStruct->GetChecksum( vCHECKSUM_IN, &partChecksumToReplace, Script::NO_ASSERT );
			success = mp_model->ReplaceTexture( partChecksumToReplace, pSrcTexture, pDestTexture );
		}
		if ( pActualStruct->GetText( vCHECKSUM_REPLACE1, &pSrcTexture, Script::NO_ASSERT ) )
		{
			pActualStruct->GetText( vCHECKSUM_WITH1, &pDestTexture, Script::ASSERT );

			uint32 partChecksumToReplace = partChecksum;
			pActualStruct->GetChecksum( vCHECKSUM_IN1, &partChecksumToReplace, Script::NO_ASSERT );
			success = mp_model->ReplaceTexture( partChecksumToReplace, pSrcTexture, pDestTexture );
		}
		if ( pActualStruct->GetText( vCHECKSUM_REPLACE2, &pSrcTexture, Script::NO_ASSERT ) )
		{
			pActualStruct->GetText( vCHECKSUM_WITH2, &pDestTexture, Script::ASSERT );

			uint32 partChecksumToReplace = partChecksum;
			pActualStruct->GetChecksum( vCHECKSUM_IN2, &partChecksumToReplace, Script::NO_ASSERT );
			success = mp_model->ReplaceTexture( partChecksumToReplace, pSrcTexture, pDestTexture );
		}
		if ( pActualStruct->GetText( vCHECKSUM_REPLACE3, &pSrcTexture, Script::NO_ASSERT ) )
		{
			pActualStruct->GetText( vCHECKSUM_WITH3, &pDestTexture, Script::ASSERT );

			uint32 partChecksumToReplace = partChecksum;
			pActualStruct->GetChecksum( vCHECKSUM_IN3, &partChecksumToReplace, Script::NO_ASSERT );
			success = mp_model->ReplaceTexture( partChecksumToReplace, pSrcTexture, pDestTexture );
		}
		if ( pActualStruct->GetText( vCHECKSUM_REPLACE4, &pSrcTexture, Script::NO_ASSERT ) )
		{
			pActualStruct->GetText( vCHECKSUM_WITH4, &pDestTexture, Script::ASSERT );

			uint32 partChecksumToReplace = partChecksum;
			pActualStruct->GetChecksum( vCHECKSUM_IN4, &partChecksumToReplace, Script::NO_ASSERT );
			success = mp_model->ReplaceTexture( partChecksumToReplace, pSrcTexture, pDestTexture );
		}
		if ( pActualStruct->GetText( vCHECKSUM_REPLACE5, &pSrcTexture, Script::NO_ASSERT ) )
		{
			pActualStruct->GetText( vCHECKSUM_WITH5, &pDestTexture, Script::ASSERT );

			uint32 partChecksumToReplace = partChecksum;
			pActualStruct->GetChecksum( vCHECKSUM_IN5, &partChecksumToReplace, Script::NO_ASSERT );
			success = mp_model->ReplaceTexture( partChecksumToReplace, pSrcTexture, pDestTexture );
		}
		if ( pActualStruct->GetText( vCHECKSUM_REPLACE6, &pSrcTexture, Script::NO_ASSERT ) )
		{
			pActualStruct->GetText( vCHECKSUM_WITH6, &pDestTexture, Script::ASSERT );

			uint32 partChecksumToReplace = partChecksum;
			pActualStruct->GetChecksum( vCHECKSUM_IN6, &partChecksumToReplace, Script::NO_ASSERT );
			success = mp_model->ReplaceTexture( partChecksumToReplace, pSrcTexture, pDestTexture );
		}
	}
	
	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::model_finalize()
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_appearance );

	mp_model->Finalize();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelBuilder::model_run_script( uint32 partChecksum, uint32 targetChecksum )
{
	Dbg_Assert( mp_model );
	Dbg_Assert( mp_appearance );

/*
	if ( !Script::GetArray( partChecksum, Script::NO_ASSERT ) )
	{
		// no array involved
		return false;
	}
*/

	Script::CStruct* pActualStruct = mp_appearance->GetActualDescStructure( partChecksum );
	if ( pActualStruct ) 
	{
		Script::SStructScript* pStructScript = new Script::SStructScript;
		if ( pActualStruct->GetScript( targetChecksum, pStructScript, Script::NO_ASSERT ) )
		{
			// construct a script and run it
			Script::CScript* pScript = new Script::CScript;
			pScript->SetScript( pStructScript, NULL, mp_appearance );	
			#ifdef __NOPT_ASSERT__
			pScript->SetCommentString("Created in CModelBuilder::model_run_script(...)");
			#endif
			int ret_val;
			while ((ret_val = pScript->Update()) != Script::ESCRIPTRETURNVAL_FINISHED)
			{
				// Script must not get blocked, otherwise it'll hang in this loop forever.
				Dbg_MsgAssert(ret_val != Script::ESCRIPTRETURNVAL_BLOCKED,("\n%s\nScript got blocked when being run by RunScript.",pScript->GetScriptInfo()));
			}
			delete pScript;
		}
		delete pStructScript;
	}
	
	return true;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx

#if 0

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelScalingModifier::ApplyModifier( Model* pModel, CModelAppearance* pAppearance )
{
	Dbg_Assert( pModel );
	Dbg_Assert( pAppearance );

	// Enforce that it's a skin model, for now...
	// TODO:  Move some skin model functionality into model class...
	Model* pModel = static_cast<Model*> ( pModel );
	if ( !pModel )
	{
		return false;
	}

	Mth::Vector vec = pModel->GetScale();

	// always reset the guy when scaling
	vec[X] = 1.0f;
	vec[Y] = 1.0f;
	vec[Z] = 1.0f;
	
	float heightScale = pAppearance->GetFloat( vCHECKSUM_HEIGHT_SCALE );
	
	vec.Scale( heightScale );

	pModel->SetScale( vec );

	// reset scaling mode...  for the most part,
	// it will be apply_normal_mode
	// unless someone overrides it in script
	uint32 scalingMode = vCHECKSUM_APPLY_NORMAL_MODE;

	pAppearance->GetChecksum( vCHECKSUM_SCALING_MODE, &scalingMode );

	// apply the desired scaling script here...
//	Dbg_Message( "Applying %s scaling mode\n", Script::FindChecksumName( scalingMode ) );
	Script::RunScript( scalingMode, NULL, pModel );

	pModel->ApplyScaling();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelWeightModifier::ApplyModifier( Model* pModel, CModelAppearance* pAppearance )
{
	Dbg_Assert( pModel );
	Dbg_Assert( pAppearance );

	// Enforce that it's a skin model, for now...
	// TODO:  Move some skin model functionality into model class...
	Model* pModel = static_cast<Model*> ( pModel );
	if ( !pModel )
	{
		return false;
	}

	// should automatically be reset to identity...
	CScalingTable scaling_table;

	Script::CScriptStructure* pTempStructure = new Script::CScriptStructure;
	float weight = pAppearance->GetFloat( "weight_scale" );
	scaling_table.SetWeightScale( weight );
	delete pTempStructure;

	pModel->ApplyScaling( &scaling_table );
	
	return true;
}
#endif
