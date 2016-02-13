///////////////////////////////////////////////////////////////////////////
// SkinAsset.cpp
//
// Asset depended code for loading, unloading and reloading a skin
//
// Mick
//


#include	<gel/assman/skinasset.h>

#include	<core/string/stringutils.h>
						   
#include	<gel/assman/assettypes.h>

#include	<gfx/nx.h>
#include	<gfx/nxmesh.h>

namespace Ass
{

int CSkinAsset::Load(const char *p_file, bool async_load, bool use_pip, void* pExtraData, Script::CStruct *pStruct)     // create or load the asset
{
	Dbg_MsgAssert(!async_load, ("Async load not supported on CSkinAsset"));

	Mem::PushMemProfile((char*)p_file);

	SSkinAssetLoadContext theContext;
	theContext.forceTexDictLookup = false;
	theContext.doShadowVolume = false;
	theContext.texDictOffset = 0;

	// GJ:  kludge to prevent tex dict offset clashes
	// in bail board asset...  i will remove this
	// after i figure out why the tex dict offsets
	// are clashing in the first place...
//	if ( strstr(p_file,"board_default") )
//	{
//		texDictOffset = -1;
//	}

	if ( pExtraData )
	{
		theContext = *(SSkinAssetLoadContext*)pExtraData;
	}
	
	Nx::CMesh* pMesh = Nx::CEngine::sLoadMesh( p_file, theContext.texDictOffset, theContext.forceTexDictLookup, theContext.doShadowVolume );
	if ( !pMesh )
	{
		Dbg_MsgAssert( 0,( "mesh %s doesn't exist.", p_file ));
		return -1;
	}

	SetData((void*)pMesh);

	// make sure the filename is in lower-case
	char msg[128];
	strcpy( msg, p_file );
	Str::LowerCase( msg );

	// only MDL files should load their collision data
	if ( strstr( msg, ".mdl" ) )
	{
		// Might be overrided by having a "nocollision=1" as a parameter
		int no_collision = 0;
		if (pStruct)
		{
			pStruct->GetInteger(CRCD(0xbf29bc0,"nocollision"),&no_collision);
		}
		if (!no_collision)
		{
			pMesh->LoadCollision(p_file);
		}
	}
	
	// Model node arrays are now loaded via the NodeArrayComponent.
//	pMesh->LoadModelNodeArray(p_file);

	Mem::PopMemProfile(/*"Skin"*/);

	return 0;
}

int CSkinAsset::Load(uint32* p_data, int data_size)     // create or load the asset
{
	char pDebugAssetString[256];
	sprintf( pDebugAssetString, "skin from data stream" );
	
	Mem::PushMemProfile((char*)pDebugAssetString);

	// GJ:  for now, we need to access 3 pointers for the MDL, TEX, and CAS
	// data...  eventually, i'd like to store each of the 3 files
	// separately in the asset manager, in which case we wouldn't
	// have to do this clumsy hack of sending 3 pointers wrapped up
	// in a struct...
	SCutsceneModelDataInfo* p_cutscene_data = (SCutsceneModelDataInfo*)p_data;

	Nx::CMesh* pMesh = Nx::CEngine::sLoadMesh( p_cutscene_data->modelChecksum,
											   p_cutscene_data->pModelData,
											   p_cutscene_data->modelDataSize,
											   p_cutscene_data->pCASData,
											   p_cutscene_data->texDictChecksum,
											   p_cutscene_data->pTextureData,
											   p_cutscene_data->textureDataSize,
											   p_cutscene_data->texDictOffset,
											   p_cutscene_data->isSkin,
											   p_cutscene_data->doShadowVolume );
	if ( !pMesh )
	{
		Dbg_MsgAssert( 0,( "Couldn't create mesh from data stream." ));
		return -1;
	}

	SetData((void*)pMesh);

	// for now, we're going to assume that data loaded in through
	// a data stream will not have collision or a node array,
	// since we're only using this for cutscene assets right 
	// now...  this can be changed very easily...

	Mem::PopMemProfile(/*"skin from data stream"*/);

	return 0;
}

int CSkinAsset::Unload()                     // Unload the asset
{
	if (GetData())
	{
		Nx::CEngine::sUnloadMesh( (Nx::CMesh*) GetData() );

        SetData(NULL);
	}
	
	return 0;
}

int CSkinAsset::Reload(const char *p_file)
{
	return 0;
}

bool CSkinAsset::LoadFinished()
{
	Dbg_MsgAssert(GetData(), ("LoadFinished(): Data pointer NULL (load probably was never started)"));

	// Since we don't support async, this is always true
	return true;
}

const char *  CSkinAsset::Name()            // printable name, for debugging
{
	return "Skin";	
}

EAssetType CSkinAsset::GetType()         // type is hard wired into asset class 
{
	return ASSET_SKIN; 					// for now return 0, not sure if this should return the EAssetType
}

}
