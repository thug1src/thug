///////////////////////////////////////////////////////////////////////////
// AnimAsset.cpp
//
// Asset depended code for loading, unloading and reloading an animation
//
// Mick
//


#include	<gel/assman/animasset.h>

#include	<gel/assman/assettypes.h>

#include	<gfx/bonedanim.h>
#include	<gfx/nx.h>

#include	<gel/scripting/symboltable.h>


namespace Ass
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CAnimAsset::Load(const char* p_file, bool async_load, bool use_pip, void* pExtraData, Script::CStruct *pStruct)     // create or load the asset
{
	int errorCode = -1;

	Mem::PushMemProfile((char*)p_file);
	
	Gfx::CBonedAnimFrameData* pSeq = new Gfx::CBonedAnimFrameData;

	char fullName[256];
	
	// add extension to create name of platform-specific SKA file
	sprintf( fullName, "%s.%s", p_file, Nx::CEngine::sGetPlatformExtension() );
	
	// Load the data
	if ( !pSeq->Load(fullName, true, async_load, use_pip) )
	{
		if ( Script::GetInteger( CRCD(0x25dc7904,"AssertOnMissingAssets") ) )
		{
			Dbg_MsgAssert( 0,( "Anim %s doesn't exist.", fullName ) );
		}
		delete pSeq;
		pSeq = NULL;
		goto failure;
	}

	// if we get this far, then it's successful
	errorCode = 0;

failure:
	// Add it to the list:
	SetData( (void*)pSeq );
	Mem::PopMemProfile(/*(char*)p_file*/);
	return errorCode;
}

int CAnimAsset::Load(uint32* p_data, int data_size)     // create or load the asset
{
	int errorCode = -1;

	char pDebugAssetString[256];
	sprintf( pDebugAssetString, "anim from data stream" );
	
	Mem::PushMemProfile((char*)pDebugAssetString);

	Gfx::CBonedAnimFrameData* pSeq = new Gfx::CBonedAnimFrameData;

	// Load the data
	if ( !pSeq->Load(p_data, data_size, true ) )
	{
		if ( Script::GetInteger( CRCD(0x25dc7904,"AssertOnMissingAssets") ) )
		{
			Dbg_Assert( 0 );
		}
		delete pSeq;
		pSeq = NULL;
		goto failure;
	}

	// if we get this far, then it's successful
	errorCode = 0;

failure:
	// Add it to the list:
	SetData( (void*)pSeq );
	Mem::PopMemProfile(/*(char*)p_file*/);
	return errorCode;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CAnimAsset::Unload()                     
{
	// Unload the asset
	
	if (GetData())
	{
        delete (Gfx::CBonedAnimFrameData*) GetData();
		
        SetData(NULL);
	}
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CAnimAsset::Reload(const char* p_file)
{
	Dbg_Message( "Reloading %s", p_file );
	
	Unload();

	return ( Load(p_file, false, 0, NULL, NULL ) == 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAnimAsset::LoadFinished()
{
	Gfx::CBonedAnimFrameData * p_anim = (Gfx::CBonedAnimFrameData*) GetData();
	Dbg_MsgAssert(p_anim, ("LoadFinished(): Data pointer NULL (load probably was never started)"));

	return p_anim->LoadFinished();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char* CAnimAsset::Name()            
{
	// printable name, for debugging
	return "Animation";	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

EAssetType CAnimAsset::GetType()         
{
	// type is hard wired into asset class 
	return ASSET_ANIM;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}
