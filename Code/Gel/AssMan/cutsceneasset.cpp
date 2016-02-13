//****************************************************************************
//* MODULE:         Ass
//* FILENAME:       cutsceneasset.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  01/13/2003
//****************************************************************************

#include <gel/assman/cutsceneasset.h>

#include <gel/assman/assettypes.h>
#include <gfx/nx.h>
#include <sk/objects/cutscenedetails.h>

namespace Ass
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CCutsceneAsset::Load( const char* p_file, bool async_load, bool use_pip, void* pExtraData, Script::CStruct *pStruct )     // create or load the asset
{
	Mem::PushMemProfile( (char*)p_file );
	
	Obj::CCutsceneData* p_cutsceneData = new Obj::CCutsceneData;

	// get the platform-specific CUT file name
	char platFileName[256];
	sprintf( platFileName, "%s.%s", p_file, Nx::CEngine::sGetPlatformExtension() );
	
	// load the data
	if ( !p_cutsceneData->Load( platFileName, true, async_load ) )
	{
		Dbg_MsgAssert( 0, ( "Cutscene %s doesn't exist.", platFileName ) );
		return -1;
	}
	
	// add it to the list:
	SetData( (void*)p_cutsceneData );

	Mem::PopMemProfile(/*(char*)p_file*/);

	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CCutsceneAsset::Unload()                     
{
	// Unload the asset
	Obj::CCutsceneData* pData = (Obj::CCutsceneData*)GetData();
	if ( pData )
	{
        delete pData;
        SetData(NULL);
	}
	
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CCutsceneAsset::Reload(const char* p_file)
{
	Dbg_Message( "Reloading %s...", p_file );
	
	Unload();

	return ( Load( p_file, false, 0, NULL, NULL ) == 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCutsceneAsset::LoadFinished()
{
	Obj::CCutsceneData* p_cutsceneData = (Obj::CCutsceneData*)GetData();
	Dbg_MsgAssert( p_cutsceneData, ( "LoadFinished(): Data pointer NULL (load probably was never started)" ) );
	return p_cutsceneData->LoadFinished();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char* CCutsceneAsset::Name()            
{
	// printable name, for debugging
	return "Cutscene";	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

EAssetType CCutsceneAsset::GetType()         
{
	// type is hard wired into asset class 
	return ASSET_CUTSCENE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}
