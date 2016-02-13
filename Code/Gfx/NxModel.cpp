//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       NxModel.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/21/2001
//****************************************************************************

#include <gfx/nxmodel.h>

#include <core/string/stringutils.h>

#include <gel/assman/assman.h>
#include <gel/assman/skinasset.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <gfx/debuggfx.h>
#include <gfx/facetexture.h>
#include <gfx/gfxutils.h>
#include <gfx/nx.h>
#include <gfx/nxgeom.h>
#include <gfx/nxmesh.h>
#include <gfx/nxlight.h>
#include <gfx/nxtexman.h>
#include <gfx/skeleton.h>

#include <sys/file/filesys.h>

namespace Nx
{
	
/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions are provided here:
// so engine implementors can leave certain functionality until later

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CModel::plat_init_skeleton( int numBones )
{
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CModel::plat_set_render_mode(ERenderMode mode)
{
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CModel::plat_set_color(uint8 r, uint8 g, uint8 b, uint8 a)
{
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CModel::plat_set_visibility(uint32 mask)
{
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CModel::plat_set_active(bool active)
{
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CModel::plat_set_scale(float scaleFactor)
{
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CModel::plat_render(Mth::Matrix* pRootMatrix, Mth::Matrix* ppBoneMatrices, int numBones)
{
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CModel::plat_replace_texture(char* p_srcFileName, char* p_dstFileName)
{
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CModel::plat_prepare_materials( void )
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CModel::plat_refresh_materials( void )
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CModel::plat_get_bounding_sphere()
{
	if ( m_numBones > 0 )
	{
		// the original hack for skinned models
		return Mth::Vector(0.0f, 36.0f, 0.0f, 48.0f);		
	}
	else
	{
		// the original hack for non-skinned models
		return Mth::Vector(0.0f, 36.0f, 0.0f, 1200.0f);		
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModel::plat_set_bounding_sphere( const Mth::Vector& boundingSphere )
{
	// Do nothing by default... 
	// (this is only needed by the PS2 version...)

	// TODO:  Set the appropriate bounding sphere that will be returned
	// by plat_get_bounding_sphere...
}

/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

// These functions are the platform independent part of the interface to 
// the platform specific code
// parameter checking can go here....
// although we might just want to have these functions inline, or not have them at all?

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModel::CModel()
{
    m_numBones = 0;
    mp_skeletonMatrices = NULL;

    SetRenderMode( vNONE );

	m_numGeoms = 0;

	m_primaryMeshName = CRCD(0x6607dd3e,"Uninitialized");

	m_active = true;
	m_hidden = false;
	m_scale = Mth::Vector( 1.0f, 1.0f, 1.0f );
	m_scalingEnabled = false;
	m_shadowEnabled = false;
	m_doShadowVolume = false;

	mp_model_lights = NULL;

	m_boundingSphereCached = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModel::~CModel()
{
	if (mp_model_lights)
	{
		CLightManager::sFreeModelLights(mp_model_lights);
	}

	ClearGeoms();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*

// GJ 10/23/02:  This function didn't seem to be used, 
// so I commented it out for now

void CModel::ResetScale()
{
	SetScale( Mth::Vector( 1.0f, 1.0f, 1.0f ) );

	if ( mp_skeleton )
	{
		mp_skeleton->ResetScale();
	}
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::SetSkeleton( Gfx::CSkeleton* pSkeleton )
{

	if (!mp_skeletonMatrices)
	{
		m_numBones = pSkeleton->GetNumBones();
	
		plat_init_skeleton( pSkeleton->GetNumBones() );
	
		mp_skeletonMatrices = pSkeleton->GetMatrices();
	}
	else
	{
		Dbg_MsgAssert(m_numBones == pSkeleton->GetNumBones(),("Mismatch in number of bones"));
		
	}

    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::Render( Mth::Matrix* pMatrix, bool no_anim, Gfx::CSkeleton* pSkeleton )
{
	// don't display the skin, unless we're
	// in the standard "textured" mode
	bool is_textured = m_renderMode == vTEXTURED; 

	int numGeoms = GetNumGeoms();
	for ( int i = 0; i < numGeoms; i++ )
	{
		Dbg_Assert( GetGeomByIndex(i) );
		GetGeomByIndex(i)->SetActive( is_textured && m_active && (m_geomActiveMask&(1<<i)) );
	}

	// scale up the matrix by the appropriate amount...
	if ( m_scalingEnabled )
	{
		pMatrix->ScaleLocal(m_scale);
	}
       
	// make sure the skeleton matches up...
	if ( pSkeleton )					    
	{
		Dbg_MsgAssert( m_numBones != 0, ( "Trying to update a model with incorrect number of bones (expected %d, found %d)", m_numBones, pSkeleton->GetNumBones() ) );
		Dbg_Assert( mp_skeletonMatrices );
	}
	else
	{
		Dbg_MsgAssert( m_numBones == 0, ( "Trying to update a model with incorrect number of bones (expected %d, found %d)", 0, m_numBones ) );
		Dbg_Assert( mp_skeletonMatrices == NULL );
	}

    switch ( m_renderMode )
    {
        case vTEXTURED:
        {
			if ( pSkeleton )
            {
				(*pMatrix)[Mth::RIGHT][W] = 0.0f;
				(*pMatrix)[Mth::UP][W] = 0.0f;
				(*pMatrix)[Mth::AT][W] = 0.0f;
				(*pMatrix)[Mth::POS][W] = 1.0f;

				int numGeoms = GetNumGeoms();
								
				if ( no_anim )
				{
					// update root position without updating bones
					for ( int i = 0; i < numGeoms; i++ )
					{
						Dbg_Assert( GetGeomByIndex(i) );
						GetGeomByIndex(i)->Render( pMatrix, NULL, 0 );
					}
				}
				else
				{
 					// update both root position AND bones
					for ( int i = 0; i < numGeoms; i++ )
					{
						Dbg_Assert( GetGeomByIndex(i) );
						GetGeomByIndex(i)->Render( pMatrix, pSkeleton->GetMatrices(), pSkeleton->GetNumBones() );
					}	
				}
			}
			else
			{
				// the following should NOT be necessary,
				// but it is.
				(*pMatrix)[Mth::RIGHT][W] = 0.0f;
				(*pMatrix)[Mth::UP][W] = 0.0f;
				(*pMatrix)[Mth::AT][W] = 0.0f;
				(*pMatrix)[Mth::POS][W] = 1.0f;
				
				int numGeoms = GetNumGeoms();
				for ( int i = 0; i < numGeoms; i++ )
				{
					Dbg_Assert( GetGeomByIndex(i) );
					GetGeomByIndex(i)->Render( pMatrix, NULL, 0 );
				}
			}
        }
        break;
        
		case vSKELETON:
		{
            if ( pSkeleton )
            {
				pSkeleton->Display( pMatrix, 0.5f, 0.5f, 1.0f );
			}
		}
		break;

        case vBBOX:
        default:
        {
            Mth::Matrix matrix = *pMatrix;

            // really, the AddDebugBox function should be
            // able to draw with only one matrix, rather
            // than matrix+pos.
            matrix[Mth::POS] = Mth::Vector( 0, 0, 0 );

            Mth::Vector pos = pMatrix->GetPos();

            // set up bounding box
            SBBox theBox;
            theBox.m_max.Set(10.0f, 10.0f, 10.0f);
            theBox.m_min.Set(-10.0f, -10.0f, -10.0f);    

            // For now, draw a bounding box
            Gfx::AddDebugBox( matrix, pos, &theBox, NULL, 1, NULL ); 
		}
        break;
        
		case vGOURAUD:
        case vFLAT:
        case vWIREFRAME:
            Dbg_Assert( 0 );
        break;

        case vNONE:
            // draw nothing...
            break;
        
    }

    return true;
}
                            
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::SetBoneMatrixData( Gfx::CSkeleton* pSkeleton )
{
	// GJ:  X-box doesn't copy over bone matrix data once per frame
	// like the PS2 does...  so the following allows me to hijack the
	// skater's model matrices and copy over the matrices
	// from a different model... (for skater substitution in cutscenes,
	// and perhaps for the bail board)

	Dbg_Assert( pSkeleton );
	
	int numGeoms = GetNumGeoms();
	for ( int i = 0; i < numGeoms; i++ )
	{
		Dbg_Assert( GetGeomByIndex(i) );
		GetGeomByIndex(i)->SetBoneMatrixData( pSkeleton->GetMatrices(), pSkeleton->GetNumBones() );
	}  

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::AddGeom(uint32 assetName, uint32 geomName, bool supportMultipleMaterialColors)
{
	// now that a new geom has been added to the model,
	// consider the bounding sphere "dirty"
	m_boundingSphereCached = false;

	// for now, assume that if we're using this method of creating assets,
	// then we'll be using the asset manager and a texDictOffset of 0
    
	bool success = false;

	Dbg_MsgAssert( m_numGeoms < MAX_GEOMS, ( "Too many geoms for this model (max=%d)", MAX_GEOMS ) );

	// the asset must already exist in the assman by this point!
	// note that we're not loading the asset, only getting it
	Ass::CAssMan * ass_man = Ass::CAssMan::Instance();

	Nx::CMesh* pMesh = NULL;
	pMesh = (Nx::CMesh*)ass_man->GetAsset( assetName, false );
	if( !pMesh )
	{
		Dbg_MsgAssert( false,( "Could not get asset %08x", assetName ));
	}	   		   

	m_preloadedMeshNames[m_numPreloadedMeshes] = assetName;
	m_numPreloadedMeshes++;

	mp_geom[m_numGeoms] = Nx::CEngine::sInitGeom();

	if ( mp_geom[m_numGeoms] )
	{
		mp_geom[m_numGeoms]->LoadGeomData(pMesh, this, supportMultipleMaterialColors);
		mp_geomTexDict[m_numGeoms] = pMesh->GetTextureDictionary();
		mp_geom[m_numGeoms]->EnableShadow(m_shadowEnabled);
		m_geomName[m_numGeoms] = geomName;
		m_geomActiveMask |= (1<<m_numGeoms);

		// Set the model lights, if any
		if (mp_model_lights)
		{
			mp_geom[m_numGeoms]->SetModelLights(mp_model_lights);
		}

		m_numGeoms++;
		
		SetRenderMode( vTEXTURED );
	}

	// TODO:  The mesh file name represents
	// mesh #0, but the mesh file name is
	// already kind of kludge that will be
	// cleaned up when CGeoms are properly
	// implemented.
	if ( m_numGeoms == 1 )
	{
		// doesn't have a filename because it came from a stream...
		m_primaryMeshName = Script::GenerateCRC("data_from_stream");
	}
    
	return success;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::AddGeom(const char* pMeshFileName, uint32 geomName, bool useAssetManager, uint32 texDictOffset, bool forceTexDictLookup, bool supportMultipleMaterialColors)
{
	// now that a new geom has been added to the model,
	// consider the bounding sphere "dirty"
	m_boundingSphereCached = false;

    bool success = false;

	Dbg_MsgAssert( m_numGeoms < MAX_GEOMS, ( "Too many geoms for this model (max=%d)", MAX_GEOMS ) );

	// grabs the data from the asset manager if it's not already loaded
	Ass::CAssMan * ass_man = Ass::CAssMan::Instance();

	Nx::CMesh* pMesh = NULL;
	if ( useAssetManager )
	{
		Ass::SSkinAssetLoadContext theContext;
		theContext.forceTexDictLookup = forceTexDictLookup;
		theContext.doShadowVolume = m_doShadowVolume;
		theContext.texDictOffset = texDictOffset;

		pMesh = (Nx::CMesh*)ass_man->LoadOrGetAsset( pMeshFileName, false, false, ass_man->GetDefaultPermanent(), 0, &theContext );
		if( !pMesh )
		{
			Dbg_MsgAssert( false,( "Could not load asset %s", pMeshFileName ));
		}	   		   

		m_preloadedMeshNames[m_numPreloadedMeshes] = Script::GenerateCRC(pMeshFileName);
		m_numPreloadedMeshes++;
	}
	else
	{
		Mem::PushMemProfile((char*)pMeshFileName);
		pMesh = Nx::CEngine::sLoadMesh( pMeshFileName, texDictOffset, forceTexDictLookup, m_doShadowVolume );
		Mem::PopMemProfile(/*(char*)pMeshFileName*/);
		Dbg_MsgAssert( m_numMeshes < MAX_MESHES, ( "Too many meshes for this model (max=%d)", MAX_MESHES ) );
		mp_mesh[m_numMeshes] = pMesh;
		m_numMeshes++;
	}

	mp_geom[m_numGeoms] = Nx::CEngine::sInitGeom();

	if ( mp_geom[m_numGeoms] )
	{
		mp_geom[m_numGeoms]->LoadGeomData(pMesh, this, supportMultipleMaterialColors);
		mp_geomTexDict[m_numGeoms] = pMesh->GetTextureDictionary();
		mp_geom[m_numGeoms]->EnableShadow(m_shadowEnabled);
		m_geomName[m_numGeoms] = geomName;
		m_geomActiveMask |= (1<<m_numGeoms);

		// Set the model lights, if any
		if (mp_model_lights)
		{
			mp_geom[m_numGeoms]->SetModelLights(mp_model_lights);
		}

		m_numGeoms++;
		
		SetRenderMode( vTEXTURED );
	}

	// TODO:  The mesh file name represents
	// mesh #0, but the mesh file name is
	// already kind of kludge that will be
	// cleaned up when CGeoms are properly
	// implemented.
	if ( m_numGeoms == 1 )
	{
		m_primaryMeshName = Script::GenerateCRC(pMeshFileName);
	}
    
	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::AddGeom(CGeom* pGeom, uint32 geomName)
{
	// now that a new geom has been added to the model,
	// consider the bounding sphere "dirty"
	m_boundingSphereCached = false;

    bool success = false;

	Dbg_MsgAssert( m_numGeoms < MAX_GEOMS, ( "Too many geoms for this model (max=%d)", MAX_GEOMS ) );

	mp_geom[m_numGeoms] = pGeom;
	mp_geom[m_numGeoms]->EnableShadow(m_shadowEnabled);
	m_geomName[m_numGeoms] = geomName;
	m_geomActiveMask |= (1<<m_numGeoms);

	// Set the model lights, if any
	if (mp_model_lights)
	{
		mp_geom[m_numGeoms]->SetModelLights(mp_model_lights);
	}

	m_numGeoms++;
		
	SetRenderMode( vTEXTURED );

	// TODO:  The mesh file name represents
	// mesh #0, but the mesh file name is
	// already kind of kludge that will be
	// cleaned up when CGeoms are properly
	// implemented.
	if ( m_numGeoms == 1 )
	{
		m_primaryMeshName = CRCD(0x9facc1a5,"TempMeshName");
	}
    
	// Maybe need to pass in the collision volume instead?

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::RemovePolys(void)
{
	uint32 polyRemovalMask = GetPolyRemovalMask();

	HidePolys( polyRemovalMask );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CModel::GetPolyRemovalMask()
{
	uint32 polyRemovalMask = 0;
	
	// loop through the meshes to collect the poly removal masks
	for ( short i = 0; i < m_numMeshes; i++ )
	{
		polyRemovalMask |= ( mp_mesh[i]->GetRemovalMask() );
	}

	// also account for cutscene objects, which are loaded
	// through the asset manager
	for ( short i = 0; i < m_numPreloadedMeshes; i++ )
	{
		Ass::CAssMan * ass_man = Ass::CAssMan::Instance();
		Nx::CMesh* pMesh = (Nx::CMesh*)ass_man->GetAsset( m_preloadedMeshNames[i], true );
		if( pMesh )
		{
			polyRemovalMask |= pMesh->GetRemovalMask();
		}
	}
	
	return polyRemovalMask;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModel::HidePolys( uint32 polyRemovalMask )
{
	// loop through the existing models, and remove existing polys
	for ( short i = 0; i < m_numGeoms; i++ )
	{
		mp_geom[i]->HidePolys( polyRemovalMask );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::CreateModelLights()
{
	Dbg_MsgAssert( !mp_model_lights, ("Model lights already exist") );

	mp_model_lights = Nx::CLightManager::sCreateModelLights();

	for ( short i = 0; i < m_numGeoms; i++ )
	{
		mp_geom[i]->SetModelLights( mp_model_lights );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::DestroyModelLights()
{
	Dbg_MsgAssert( mp_model_lights, ("No Model lights to destroy") );

	Nx::CLightManager::sFreeModelLights( mp_model_lights );

	mp_model_lights = NULL;
	for ( short i = 0; i < m_numGeoms; i++ )
	{
		mp_geom[i]->SetModelLights( NULL );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModelLights * CModel::GetModelLights() const
{
	return mp_model_lights;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::SetRenderMode(ERenderMode mode)
{
    bool success = false;

    if (mode != m_renderMode)
    {
        success = plat_set_render_mode(mode);
        
        if (success)
        {
            m_renderMode = mode;
        }
    }

    return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ERenderMode CModel::GetRenderMode()
{
	return m_renderMode;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModel::Hide( bool should_hide )
{   
	m_hidden = should_hide;
	
	SetActive( !should_hide );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::SetActive( bool active )
{
	if( m_hidden && active )
	{
		return false;									
	}

	// (Mick) if the active state is already set correctly, then just return
	// (this assumes that the geoms are correctly set already)	
	if (m_active == active)
	{
		return true;
	}

	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		Dbg_Assert( GetGeomByIndex(i) );
		GetGeomByIndex(i)->SetActive( active );
	}
	
	m_active = active;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom* CModel::GetGeom(uint32 geomName)
{
	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		// TODO:  there might be multiple geoms with
		// the same checksum...
		if ( geomName == m_geomName[i] )
		{
			Dbg_Assert( GetGeomByIndex( i ) );
			return GetGeomByIndex( i );
		}
	}

    return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::ReplaceTexture( uint32 geomName, const char* p_SrcTextureName, const char* p_DstTextureName )
{
	bool success = false;
	
	bool applyToAll = (geomName == vREPLACE_GLOBALLY);
	
	plat_prepare_materials();

	// The lower level code is expecting the .PNG extension
	// for the source texture name, so add it if's not there
	char src_texture_name[512];
	strcpy( src_texture_name, p_SrcTextureName );
	Str::LowerCase( src_texture_name );
	char* pSrcExt = strstr( src_texture_name, ".png" );
	if ( !pSrcExt )
	{
		strcat( src_texture_name, ".png" );
	}

	// The lower level code is NOT expecting the .PNG extension
	// for the destination texture name, so strip it out if it's 
	// there (the artists sometimes accidentally leave the .PNG in there)...
	char dest_texture_name[512];
	strcpy( dest_texture_name, p_DstTextureName );
	Str::LowerCase( dest_texture_name );
	char* pDstExt = strstr( dest_texture_name, ".png" );
	if ( pDstExt )
	{
		*pDstExt = NULL;
	}

	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		// m_geomName[i] = 0 will handle cutscene objects,
		// for texture replacement on the hi-res heads

		if ( geomName == m_geomName[i] || applyToAll || m_geomName[i] == 0 )
		{
			if ( mp_geomTexDict[i] && mp_geomTexDict[i]->ReplaceTexture( src_texture_name, dest_texture_name) )
			{
				success = true;
			}
		}
	}

	if ( !success && Script::GetInteger( CRCD(0x2a648514,"cas_artist") ) )
	{
		Dbg_Message( "Texture replacement of %s by %s failed!", p_SrcTextureName, dest_texture_name );
	}

	plat_refresh_materials();

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::ReplaceTexture( uint32 geomName, const char* p_SrcFileName, uint32 DstChecksum )
{
	bool success = false;
	
	bool applyToAll = (geomName == vREPLACE_GLOBALLY);
	
	plat_prepare_materials();

	// The lower level code is expecting the .PNG extension
	// for the source texture name, so add it if's not there
	char src_texture_name[512];
	strcpy( src_texture_name, p_SrcFileName );
	Str::LowerCase( src_texture_name );
	char* pSrcExt = strstr( src_texture_name, ".png" );
	if ( !pSrcExt )
	{
		strcat( src_texture_name, ".png" );
	}

	CTexture *p_dest_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(DstChecksum);
	if (!p_dest_texture)
	{
		Dbg_MsgAssert(0, ("Can't find destination texture %s", Script::FindChecksumName(DstChecksum)));
	}

	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		if ( geomName == m_geomName[i] || applyToAll )
		{
			if ( mp_geomTexDict[i] && mp_geomTexDict[i]->ReplaceTexture( src_texture_name, p_dest_texture) )
			{
				success = true;
			}
		}
	}

	if ( !success && Script::GetInteger( CRCD(0x2a648514,"cas_artist") ) )
	{
		Dbg_Message( "Texture replacement of %s by %s failed!", p_SrcFileName, Script::FindChecksumName(DstChecksum) );
	}

	plat_refresh_materials();

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::ClearColor( uint32 geomName )
{
	bool success = false;
	
	bool applyToAll = (geomName == vREPLACE_GLOBALLY);

	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		if ( geomName == m_geomName[i] || applyToAll )
		{
			Dbg_MsgAssert( !mp_geom[i]->MultipleColorsEnabled(), ( "Wasn't expecting multiple material colors" ) );

			mp_geom[i]->ClearColor();
			success = true;
		}
	}

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::ModulateColor( uint32 geomName, float h, float s, float v )
{
	// hue = [0.0, 360.0f)
	// satuation = [0.0, 1.0]
	// value = [0.0, 1.0]

	bool success = false;
	
	bool applyToAll = (geomName == vREPLACE_GLOBALLY);
	
	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		if ( geomName == m_geomName[i] || applyToAll )
		{
			if ( mp_geom[i]->MultipleColorsEnabled() )
			{
				// ignore geoms with the "multiple colors" flag
				// (you're supposed to use SetColorWithParams() for that)
			}
			else
			{
				float r, g, b;
				Gfx::HSVtoRGB( &r, &g, &b, h, s, v );

				Image::RGBA theColor;
				theColor.r = (unsigned char)( r * 255.0f + 0.5f );
				theColor.g = (unsigned char)( g * 255.0f + 0.5f );
				theColor.b = (unsigned char)( b * 255.0f + 0.5f );
				theColor.a = 255;

				Dbg_MsgAssert( !mp_geom[i]->MultipleColorsEnabled(), ( "Wasn't expecting multiple material colors" ) );

				mp_geom[i]->SetColor( theColor );
				success = true;
			}
		}
	}

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::SetUVOffset(uint32 mat_checksum, int pass, float u_offset, float v_offset)
{
	bool success = false;
	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		success |= mp_geom[i]->SetUVWibbleOffsets(mat_checksum, pass, u_offset, v_offset);
	}

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::SetUVMatrix(uint32 mat_checksum, int pass, const Mth::Matrix &mat)
{
	bool success = false;
	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		success |= mp_geom[i]->SetUVMatrix(mat_checksum, pass, mat);
	}

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::AllocateUVMatrixParams(uint32 mat_checksum, int pass)
{
	bool success = false;
	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		success |= mp_geom[i]->AllocateUVMatrixParams(mat_checksum, pass);
	}

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::SetColor( Script::CStruct* pParams, float h, float s, float v )
{
	// GJ:  This is a generic way of changing the colors in a model,
	// without having to know whether it has a separate color per material
	
	// GJ TODO:  Ideally, this function would not reference CStruct,
	// but would rather be passed the appropriate material/pass
	// parameters from a higher level...

	bool success = false;

	uint32 geomName;
	pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &geomName, Script::ASSERT );

	float r, g, b;
	Gfx::HSVtoRGB( &r, &g, &b, h, s, v );

	Image::RGBA theColor;
	theColor.r = (unsigned char)( r * 255.0f + 0.5f );
	theColor.g = (unsigned char)( g * 255.0f + 0.5f );
	theColor.b = (unsigned char)( b * 255.0f + 0.5f );
	theColor.a = 255;

	bool applyToAll = (geomName == vREPLACE_GLOBALLY);

	int numGeoms = GetNumGeoms();
	for ( int i = 0; i < numGeoms; i++ )
	{
		uint32 mat_checksum;
		int pass = 0;
		Script::CArray* pMaterialArray;
		
		if ( !mp_geom[i]->MultipleColorsEnabled() )
		{
			if ( geomName == m_geomName[i] || applyToAll )
			{
				mp_geom[i]->SetColor( theColor );
				success = true;
			}
		}
		else if ( pParams->GetArray( CRCD(0x64e8e94a,"materials"), &pMaterialArray, Script::NO_ASSERT ) )
		{
			for ( uint32 j = 0; j < pMaterialArray->GetSize(); j++ )
			{
				Script::CStruct* pMaterialStruct = pMaterialArray->GetStructure( j );
				pMaterialStruct->GetChecksum( NONAME, &mat_checksum, Script::ASSERT );
				pMaterialStruct->GetInteger( NONAME, &pass, Script::ASSERT ); 

				success |= mp_geom[i]->SetMaterialColor(mat_checksum, pass, theColor);
			}
		}
		else if ( pParams->GetChecksum( CRCD(0x83418a6a,"material"), &mat_checksum, Script::NO_ASSERT )	)
		{
			Script::CArray* pPassArray;
			if ( pParams->GetArray( CRCD(0x318f2bdb,"pass"), &pPassArray, Script::NO_ASSERT ) )
			{
				for ( uint32 j = 0; j < pPassArray->GetSize(); j++ )
				{
					success |= mp_geom[i]->SetMaterialColor(mat_checksum, pPassArray->GetInteger(j), theColor);
				}
			}
			else if ( pParams->GetInteger( CRCD(0x318f2bdb,"pass"), &pass, Script::ASSERT ) )
			{
				success |= mp_geom[i]->SetMaterialColor(mat_checksum, pass, theColor);
			}
		}
		
		// kludge for changing skin color
		// (ideally this would be in script, but we would have to uglify the
		// code to get around some of the model-building operations that
		// currently overwrite the body substructure, such as when
		// the head forces the body to a specific skin tone)
		if ( ( geomName == CRCD(0x650fab6d,"skater_m_head") 
			   || geomName == CRCD(0x0fc85bae,"skater_f_head") )
			 && m_geomName[i] == CRCD(0x2457f44d,"body") )
		{
			Dbg_MsgAssert( !mp_geom[i]->MultipleColorsEnabled(), ( "Wasn't expecting color per material on body geom" ) );
			this->ModulateColor( CRCD(0x2457f44d,"body"), h, s, v );
		}
	}	
	
	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::ClearColor( Script::CStruct* pParams )
{
	// GJ:  This is a generic way of changing the colors in a model,
	// without having to know whether it has a separate color per material
	
	bool success = false;

	uint32 geomName;
	pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &geomName, Script::ASSERT );

	bool applyToAll = (geomName == vREPLACE_GLOBALLY);

	int numGeoms = GetNumGeoms();
	for ( int i = 0; i < numGeoms; i++ )
	{
		uint32 mat_checksum;
		int pass = 0;
		Script::CArray* pMaterialArray;
		
		if ( !mp_geom[i]->MultipleColorsEnabled() )
		{
			if ( geomName == m_geomName[i] || applyToAll )
			{
				mp_geom[i]->ClearColor();
				success = true;
			}
		}
		else if ( pParams->GetArray( CRCD(0x64e8e94a,"materials"), &pMaterialArray, Script::NO_ASSERT ) )
		{
			Image::RGBA theColor;
			theColor.r = 128;
			theColor.g = 128;
			theColor.b = 128;
			theColor.a = 128;

			for ( uint32 j = 0; j < pMaterialArray->GetSize(); j++ )
			{
				Script::CStruct* pMaterialStruct = pMaterialArray->GetStructure( j );
				pMaterialStruct->GetChecksum( NONAME, &mat_checksum, Script::ASSERT );
				pMaterialStruct->GetInteger( NONAME, &pass, Script::ASSERT ); 

				success |= mp_geom[i]->SetMaterialColor(mat_checksum, pass, theColor);
			}
		}
		else if ( pParams->GetChecksum( CRCD(0x83418a6a,"material"), &mat_checksum, Script::NO_ASSERT )	)
		{
			Image::RGBA theColor;
			theColor.r = 128;
			theColor.g = 128;
			theColor.b = 128;
			theColor.a = 128;
			
			Script::CArray* pPassArray;
			if ( pParams->GetArray( CRCD(0x318f2bdb,"pass"), &pPassArray, Script::NO_ASSERT ) )
			{
				for ( uint32 j = 0; j < pPassArray->GetSize(); j++ )
				{
					success |= mp_geom[i]->SetMaterialColor(mat_checksum, pPassArray->GetInteger(j), theColor);
				}
			}
			else if ( pParams->GetInteger( CRCD(0x318f2bdb,"pass"), &pass, Script::ASSERT ) )
			{
				success |= mp_geom[i]->SetMaterialColor(mat_checksum, pass, theColor);
			}
		}

		// kludge for changing skin color
		if ( ( geomName == CRCD(0x650fab6d,"skater_m_head") 
			   || geomName == CRCD(0x0fc85bae,"skater_f_head") )
			 && m_geomName[i] == CRCD(0x2457f44d,"body") )
		{
			Dbg_MsgAssert( !mp_geom[i]->MultipleColorsEnabled(), ( "Wasn't expecting color per material on body geom" ) );
			mp_geom[i]->ClearColor();
			success = true;
		}
	}
	
	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom* CModel::GetGeomByIndex(int index)
{
	Dbg_Assert( index >= 0 && index < m_numGeoms );

    return mp_geom[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CModel::GetGeomNameByIndex(int index)
{
	Dbg_Assert( index >= 0 && index < m_numGeoms );

    return m_geomName[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexDict* CModel::GetTexDictByIndex(int index)
{
	Dbg_Assert( index >= 0 && index < m_numGeoms );

    return mp_geomTexDict[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CModel::GetNumGeoms() const
{
    return m_numGeoms;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CModel::GetGeomActiveMask() const
{
	// K: Used by replay code.
	return m_geomActiveMask;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModel::SetGeomActiveMask(uint32 mask)
{
	// K: Used by replay code.
	m_geomActiveMask=mask;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModel::Finalize()
{
	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		Dbg_Assert( GetGeomByIndex(i) );
		GetGeomByIndex(i)->Finalize();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModel::ClearGeoms()
{
	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		Dbg_Assert( GetGeomByIndex(i) );
		Nx::CEngine::sUninitGeom( GetGeomByIndex(i) );
	}

	m_numGeoms = 0;

	// eliminate all the meshes
	for ( short i = 0; i < m_numMeshes; i++ )
	{
		Nx::CEngine::sUnloadMesh( mp_mesh[i] );
	}

	m_numMeshes = 0;

	m_numPreloadedMeshes = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::HideGeom( uint32 geomName, bool hidden )
{
	bool success = false;

	bool applyToAll = (geomName == vREPLACE_GLOBALLY);
	
	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		if ( (geomName == m_geomName[i]) || applyToAll )
		{
			if ( hidden )
			{
				m_geomActiveMask &= ~(1<<i);
			}
			else
			{
				m_geomActiveMask |= (1<<i);
			}
			
			success = true;
		}
	}

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::GeomHidden( uint32 geomName )
{
    bool success = true;
    
    int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		if ( (geomName == m_geomName[i]) )
		{
			if ( m_geomActiveMask & (1<<i) )
            {
                success = false;
			}
		}
	}

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Matrix* CModel::GetBoneTransforms()
{
    // GJ:  eventually, i'd like to get rid of this ptr,
    // but for now, don't break the NGC and XBOX versions

    return mp_skeletonMatrices;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::SetColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	Image::RGBA rgba(r, g, b, a);

	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		Dbg_Assert( GetGeomByIndex(i) );
		GetGeomByIndex(i)->SetColor(rgba);
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::SetVisibility(uint32 mask)
{
	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		Dbg_Assert( GetGeomByIndex(i) );
		GetGeomByIndex(i)->SetVisibility(mask);
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::SetScale( const Mth::Vector& scale )
{
	m_scale = scale;
	
	m_scalingEnabled = ( scale[X] != 1.0f || scale[Y] != 1.0f || scale[Z] != 1.0f );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModel::EnableScaling( bool enabled )
{
	m_scalingEnabled = enabled;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModel::EnableShadow( bool enabled )
{
	m_shadowEnabled = enabled;

	int numGeoms = GetNumGeoms();
	for ( short i = 0; i < numGeoms; i++ )
	{
		Dbg_Assert( GetGeomByIndex(i) );
		GetGeomByIndex(i)->EnableShadow(enabled);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CModel::GetNumObjectsInHierarchy()
{
	// This function only works with single-CMesh items
	if ( m_numPreloadedMeshes == 1 )
	{
		Ass::CAssMan * ass_man = Ass::CAssMan::Instance();
		Nx::CMesh* pMesh = (Nx::CMesh*)ass_man->GetAsset( m_preloadedMeshNames[0] );
		Dbg_MsgAssert( pMesh, ( "Couldn't find preloaded mesh[0] to get hierarchy from" ) );
		return pMesh->GetNumObjectsInHierarchy();
	}
	else
	{
		return NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::CHierarchyObject* CModel::GetHierarchy()
{
	// This function only works with single-CMesh items
	if ( m_numPreloadedMeshes == 1 )
	{
		Ass::CAssMan * ass_man = Ass::CAssMan::Instance();
		Nx::CMesh* pMesh = (Nx::CMesh*)ass_man->GetAsset( m_preloadedMeshNames[0] );
		Dbg_MsgAssert( pMesh, ( "Couldn't find preloaded mesh[0] to get hierarchy from" ) );
		return pMesh->GetHierarchy();
	}
	else
	{
		return NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CModel::GetBoundingSphere()
{
	// unless the dirty flag has been set
	if ( m_boundingSphereCached )
	{
		return m_boundingSphere;
	}

	// recalculate the bounding sphere
	m_boundingSphere = plat_get_bounding_sphere();
	m_boundingSphereCached = true;

	return m_boundingSphere;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModel::SetBoundingSphere( const Mth::Vector& boundingSphere )
{
	m_boundingSphereCached = false;

	plat_set_bounding_sphere( boundingSphere );
}					  

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModel::ApplyFaceTexture( Gfx::CFaceTexture* pFaceTexture, const char* pSrcTexture, uint32 partChecksumToReplace )
{
	bool success = false;

	Dbg_Assert( pFaceTexture );
	Dbg_Assert( pFaceTexture->IsValid() );

	//-------------------------------------------------------------
	// step 1:  load up the temporary texture from the appearance's CFaceTexture
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

	bool alloc_vram = true;
	uint32 temp_checksum = CRCD(0xb00b0dc0,"dummy");
	Nx::CTexture* p_temp_texture = Nx::CTexDictManager::sp_sprite_tex_dict->LoadTextureFromBuffer(pFaceTexture->GetTextureData(), pFaceTexture->GetTextureSize(), temp_checksum, true, alloc_vram, false);
	Dbg_Assert( p_temp_texture );

	Nx::CTexture* p_temp_overlay = Nx::CTexDictManager::sp_sprite_tex_dict->LoadTexture(pFaceTexture->GetOverlayTextureName(), alloc_vram);
	Dbg_Assert( p_temp_overlay );
	
	Mem::Manager::sHandle().PopContext();
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	// step 2:  massage the temporary texture here, based on the appearance's CFaceTexture
	Nx::SFacePoints originalModelFacePoints;
	Script::CStruct* pModelFacePoints = Script::GetStructure( CRCD(0xa435752c,"original_model_face_points"), Script::ASSERT );
	GetFacePointsStruct( originalModelFacePoints, pModelFacePoints );
	Nx::CFaceTexMassager::sSetModelFacePoints( originalModelFacePoints );

	Nx::CFaceTexMassager::sSetFaceTextureOverlay( p_temp_overlay );

	// this speeds things up a little because the palette doesn't get rebuilt
	// (the image is not accurate, but it does end up matching the skintone more)
	//bool do_palette_gen = false;
	bool do_palette_gen = pFaceTexture->GetFacePoints().m_adjust_hsv;		// Rebuild the palette if HSV is modified

	// this is a test for Nolan to see if the colors match better
	// when the palette is generated
//	do_palette_gen = Script::GetInt( CRCD(0x8b32fff0,"do_palette_gen"), Script::NO_ASSERT );

	#ifdef	__NOPT_ASSERT__
	bool massage_success = 
	#endif
	Nx::CFaceTexMassager::sMassageTexture( p_temp_texture,
										   pFaceTexture->GetFacePoints(),
										   do_palette_gen );

	Dbg_Assert( massage_success );
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	// step 3:  do texture replacement on the actual model
	Dbg_Assert( pSrcTexture );
	success = this->ReplaceTexture( partChecksumToReplace, pSrcTexture, p_temp_texture->GetChecksum() );
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	// step 4:  get rid of the temporary texture
	Nx::CTexDictManager::sp_sprite_tex_dict->UnloadTexture( p_temp_texture );
	Nx::CTexDictManager::sp_sprite_tex_dict->UnloadTexture( p_temp_overlay );
	//-------------------------------------------------------------

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModel::EnableShadowVolume( bool enabled )
{
	// if true, any future geoms added to the 
	// model will use shadow volumes
	m_doShadowVolume = enabled;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // Nx

