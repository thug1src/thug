///////////////////////////////////////////////////////////////////////////
// NodeArrayAsset.cpp
//
// Asset depended code for loading, unloading and reloading a node array
//
// Dave
//


#include	<gel/assman/nodearrayasset.h>
#include	<gel/assman/assettypes.h>
#include	<gfx/nx.h>
#include	<gel/scripting/symboltable.h>
#include	<gel/scripting/script.h>
#include	<sk/scripting/gs_file.h>

namespace Ass
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CNodeArrayAsset::Load( const char* p_file, bool async_load, bool use_pip, void* pExtraData , Script::CStruct *pStruct)
{
	SkateScript::LoadQB( p_file, Script::ASSERT_IF_DUPLICATE_SYMBOLS);
	m_qb_checksum = Script::GenerateCRC( p_file );

	char nodearrayname[128];

	// Skip back to the backslash.
	const char *p = &p_file[strlen( p_file ) - 1];
	while( *p != '\\' && *p != '/') p--;
	p++;

	strcpy( nodearrayname, p );												// nodearrayname is now "name.qb"
	sprintf( &nodearrayname[strlen( nodearrayname ) - 3], "_nodearray" );	// nodearrayname is now "name_nodearray"
	void *p_data = Script::GetArray( nodearrayname );

	// Set the array as the data returned when queried,
	SetData( p_data );

	Dbg_MsgAssert( p_data, ( "Could not find nodearray %s in %s", nodearrayname, p_file ));

	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int CNodeArrayAsset::Unload()                     
{
	// Unload the asset.
	SkateScript::UnloadQB( m_qb_checksum );
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CNodeArrayAsset::Reload( const char* p_file )
{
	Dbg_Message( "Reloading %s", p_file );
	
	Unload();

	return( Load( p_file, false, 0, NULL, NULL ) == 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CNodeArrayAsset::LoadFinished()
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char* CNodeArrayAsset::Name()            
{
	// Printable name, for debugging.
	return "Node Array";	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

EAssetType CNodeArrayAsset::GetType()         
{
	// type is hard wired into asset class 
	return ASSET_NODEARRAY;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}
