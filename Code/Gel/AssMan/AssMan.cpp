//****************************************************************************
//* MODULE:         Gel/AssMan
//* FILENAME:       AssMan.cpp
//* OWNER:          Matt Duncan
//* CREATION DATE:  11/17/2000
//****************************************************************************

// start autoduck documentation
// @DOC Assman
// @module Assman | None
// @subindex Scripting Database
// @index script | Assman

/*

	Assets are generally binary files that are processed in some way when loaded
	(although they might simply be loaded).
	
	They are always referenced by the 32-bit checksum of the name of the file

	Design and art guidelines already require that all filenames in the project 
	are unique names.

	The asset manager should handle generic assets...  (it shouldn't need
	to know anything about texture dictionaries or clumps or anims...)
*/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gel/assman/assman.h>

#include <core/singleton.h>
#include <core/string/stringutils.h>

#include <sys/file/AsyncFilesys.h>

#include <gel/assman/assettypes.h>
#include <gel/assman/animasset.h>
#include <gel/assman/cutsceneasset.h>
#include <gel/assman/skeletonasset.h>
#include <gel/assman/skinasset.h>
#include <gel/assman/refasset.h>
#include <gel/assman/nodearrayasset.h>

#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>							
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Ass
{

DefineSingletonClass( CAssMan, "Shared Asset Manager" );

struct SAssetLookup
{
   	char*		p_extension;
	EAssetType	type;
};

static SAssetLookup s_asset_lookup[] = 
{
	{"BIN",		ASSET_BINARY},
	{"CLD",		ASSET_COLLISION},
	{"SCN",		ASSET_SCENE},
	{"MDL",		ASSET_SKIN},
	{"SKA",		ASSET_ANIM},
	{"FAM",		ASSET_ANIM}, 	// facial anims
    {"SKE",		ASSET_SKELETON},
	{"SKIN",	ASSET_SKIN},
	{"TEX",		ASSET_TEXTURES},
	{"CUT",		ASSET_CUTSCENE},
	{"QB",		ASSET_NODEARRAY},
	
// Insert new types above this line	
	{NULL,		ASSET_UNKNOWN}		   				// terminator;
};
	
/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CAssMan::CAssMan()
{
	mp_asset_table = new Lst::HashTable<Ass::CAsset>( 10 );

	// for debugging
	mp_assetlist_head = new CAsset;
	mp_assetlist_head->SetText("Asset List Head (Possible error here?)");	
	
	mp_assetlist_head->mp_next = mp_assetlist_head;
	mp_assetlist_head->mp_prev = mp_assetlist_head;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CAssMan::~CAssMan()
{	
	UnloadAllTables( true );

	delete mp_asset_table;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

EAssetType CAssMan::FindAssetType( const char *p_assetName )
{
	// given a file name, then find the type of asset that this represents
	
	// the asset type is determined by the file extension
	// so first, find the last . in the assetName
	const char *p_ext = p_assetName + strlen(p_assetName);
	while ( p_ext > p_assetName && *p_ext != '.' )
	{
		p_ext--;
	}
	
	if ( *p_ext != '.' )
	{
		return ASSET_UNKNOWN;
	}
	
	p_ext++;
	// p_ext now points to the extension for the file name

	SAssetLookup *p_lookup = &s_asset_lookup[0];
	while ( p_lookup->p_extension != NULL )
	{
		// note, ignoring case
		if ( strcmpi( p_lookup->p_extension, p_ext ) == 0 )	
		{
			return p_lookup->type;
		}
		p_lookup++;
	}
	
	return ASSET_UNKNOWN;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void* CAssMan::LoadAssetFromStream(uint32 asset_name, uint32 asset_type, uint32* p_data, int data_size, bool permanent, uint32 group)
{
	// Note:  There's no reason to support async loads when
	// loading an asset from a data stream...

	// load an asset, and return a pointer to the asset data

	// given an asset name checksum, find the type of the asset, and then load it

	Dbg_MsgAssert(!AssetAllocated(asset_name),("Asset %08x already loaded",asset_name));

	CAsset* p_asset = NULL;	
		
	// based on the asset type, create an asset of the correct type	
	switch (asset_type)
	{
		case ASSET_ANIM:
			p_asset = new CAnimAsset;
			break;

		case ASSET_SKIN:
			p_asset = new CSkinAsset;
			break;

		default:
			// right now, the only file type that's supported
			// for data streams is the "cutscene model"
			Dbg_MsgAssert(0,("Asset %08x is of unsupported type %d",asset_name,asset_type));   		
	}

	p_asset->m_permanent = permanent;
	p_asset->m_group = group;
	p_asset->m_dead = false;	  			// it is not dead	

	// fake an asset name for debugging
	char pDebugAssetString[256];
	sprintf( pDebugAssetString, "%08x from data stream", asset_name );
	p_asset->SetText(pDebugAssetString);			
	
	// finally, call the asset's Load member function to load it
	int error = p_asset->Load(p_data, data_size);	
	if ( error != 0 )
	{
		if ( Script::GetInteger( CRCD(0x25dc7904,"AssertOnMissingAssets") ) )
		{
			Dbg_MsgAssert(0,("Loading asset %08x returned error %d",asset_name,error));
		}
		else
		{
			delete p_asset;
			p_asset = NULL;
		}
		return NULL;
	}
	else
	{
		// only add it to the table once we're sure there's no error
		uint32 checksum = asset_name;
		Dbg_MsgAssert(!mp_asset_table->GetItem( checksum ),("Asset %08x already in table",asset_name));	
		mp_asset_table->PutItem( checksum,  p_asset);
		p_asset->SetID(checksum);

		// link in at the end, so things are added in order
		p_asset->mp_next = mp_assetlist_head;
		p_asset->mp_prev = mp_assetlist_head->mp_prev;
		mp_assetlist_head->mp_prev->mp_next = p_asset;
		mp_assetlist_head->mp_prev = p_asset;

		Dbg_MsgAssert(mp_asset_table->GetItem( checksum ) == p_asset,("Asset does not match entry in table"));
		Dbg_Assert( p_asset );
		return p_asset->GetData();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void* CAssMan::LoadAsset(const char *p_assetName, bool async_load, bool use_pip, bool permanent, uint32 group, void* pExtraData, Script::CStruct *pParams)
{
	// load an asset, and return a pointer to the asset data

	// given an asset filename, find the type of the asset, and then load it

	Dbg_MsgAssert(!AssetAllocated(p_assetName),("Asset %s already loaded",p_assetName));

	CAsset	* p_asset = NULL;	
	
// Find the asset type	
	EAssetType	asset_type = FindAssetType(p_assetName); 
	Dbg_MsgAssert(asset_type != ASSET_UNKNOWN,("Asset %s is of unknown type",p_assetName));
	
// based on that, create an asset of the correct type	
	switch (asset_type)
	{
		case ASSET_ANIM:
			p_asset = new CAnimAsset;
			break;

        case ASSET_SKELETON:
            p_asset = new CSkeletonAsset;
            break;

		case ASSET_SKIN:
			p_asset = new CSkinAsset;
			break;

		case ASSET_CUTSCENE:
			p_asset = new CCutsceneAsset;
			break;
			
		case ASSET_NODEARRAY:
			p_asset = new CNodeArrayAsset;
			break;
			
		case ASSET_SCENE:
		case ASSET_TEXTURES:
		case ASSET_COLLISION:
		case ASSET_BINARY:
		default:
			Dbg_MsgAssert(0,("Asset %s is of unsupported type %d",p_assetName,asset_type));   		
	}

	p_asset->m_permanent = permanent;
	p_asset->m_group = group;
	p_asset->m_dead = false;	  			// it is not dead	

	p_asset->SetText(p_assetName);			// record asset name for debugging
	
	// Garrett: Make sure that we only set async on the things that can handle them
	if (async_load)
	{
		Dbg_MsgAssert(asset_type == ASSET_ANIM, ("Can't load this asset type asynchronously: %d", asset_type));
	}

	// finally, call the asset's Load member function to load it
	int	error = p_asset->Load(p_assetName, async_load, use_pip, pExtraData, pParams);	
	if ( error != 0 )
	{
		if ( Script::GetInteger( CRCD(0x25dc7904,"AssertOnMissingAssets") ) )
		{
			Dbg_MsgAssert(0,("Loading asset %s returned error %d",p_assetName,error));
		}
		else
		{
			delete p_asset;
			p_asset = NULL;
		}
		return NULL;
	}
	else
	{
		// only add it to the table once we're sure there's no error
		uint32 checksum = Script::GenerateCRC( p_assetName );
		Dbg_MsgAssert(!mp_asset_table->GetItem( checksum ),("Asset %s already in table",p_assetName));	
		mp_asset_table->PutItem( checksum,  p_asset);
		p_asset->SetID(checksum);

		// link in at the end, so things are added in order
		p_asset->mp_next = mp_assetlist_head;
		p_asset->mp_prev = mp_assetlist_head->mp_prev;
		mp_assetlist_head->mp_prev->mp_next = p_asset;
		mp_assetlist_head->mp_prev = p_asset;

		Dbg_MsgAssert(mp_asset_table->GetItem( Script::GenerateCRC( p_assetName )) == p_asset,("Asset does not match entry in table"));
		Dbg_Assert( p_asset );
		return p_asset->GetData();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void* CAssMan::GetFirstInGroup( uint32 group )
{
	// return the first asset in the asset manager that is in this group
	// will return NULL if there are none of teh specified group
	
	return GetNthInGroup(group,0);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void* CAssMan::GetNthInGroup( uint32 group, int n )
{
	// return the nth asset in the asset manager that is in this group
	// will return NULL if there are none of the specified group
	
	CAsset* p_asset = mp_assetlist_head->mp_next;
	while (p_asset != mp_assetlist_head)
	{
//		printf ("%d: Checkingfor %x, group = %x\n",n,group,p_asset->m_group); 
		CAsset *p_next = p_asset->mp_next;	
		if (p_asset->m_group == group)
		{
//			printf ("%d: GROUP MATCH %x, group = %x\n",n,group,p_asset->m_group); 
			if (n==0)
			{
//				printf ("%d: COUNT MATCH %x, group = %x\n",n,group,p_asset->m_group); 
				return	p_asset->GetData();				
			}
			n--;
		}			
		p_asset = p_next;		
	}
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CAssMan::GetGroup(uint32 checksum)
{
	CAsset* p_actual = mp_asset_table->GetItem(checksum);
	Dbg_MsgAssert(p_actual, ("GetIndexInGroup with asset not found\n")); 
	return p_actual->GetGroup();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CAssMan::GetIndexInGroup(uint32 checksum)
{
	// find this entry, and see which number it is in the group
	// i.e, returns the intex into the group
	
	CAsset* p_actual = mp_asset_table->GetItem(checksum);
	Dbg_MsgAssert(p_actual, ("GetIndexInGroup with asset not found\n")); 
	
	int n = 0;										  	// index starts at 0
	CAsset* p_asset = mp_assetlist_head->mp_next;
	while (p_asset != mp_assetlist_head)
	{
		CAsset *p_next = p_asset->mp_next;	
		if (p_asset == p_actual)
		{
			return n;						  			// if we've found it, then return the asset
		}
		if (p_asset->m_group == p_actual->m_group)	 	// if in same group
		{
			n++;										// increment index
		}
		p_asset = p_next;		
	}
	Dbg_MsgAssert(0, ("ERROR IN GetIndexInGroup\n")); 
	return 0;
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CAssMan::CountGroup(uint32 group)
{
	int n = 0;										  	// index starts at 0
	CAsset* p_asset = mp_assetlist_head->mp_next;
	while (p_asset != mp_assetlist_head)
	{
		CAsset *p_next = p_asset->mp_next;	
		if (p_asset->m_group == group)	 	// if in same group
		{
			n++;										// increment index
		}
		p_asset = p_next;		
	}
	return n;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CAssMan::CountSameGroup(uint32 checksum)
{
	// a utility function to count the assets in the same group as another asset
	
	return CountGroup(GetGroup(checksum));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
					   
void CAssMan::DestroyReferences( CAsset* pAsset )
{
	// Destroys an asset's references
	
	Dbg_Assert( pAsset );
		
	while (pAsset->mp_ref_asset)
	{
		// destroying a reference is similar to an asset, except we
		// do not need to call unload

//		printf ("Deleting Reference %p of type %d, <%s>, <%s>\n",p_asset->mp_ref_asset,p_asset->mp_ref_asset->GetType(),p_asset->mp_ref_asset->Name(), p_asset->mp_ref_asset->GetText());
		uint32 id = pAsset->mp_ref_asset->m_id;
		pAsset->mp_ref_asset->Unlink();
		delete pAsset->mp_ref_asset; 
		mp_asset_table->FlushItem(id);		
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAssMan::UnloadAsset( CAsset* pAsset )
{
	// Unload an asset and delete it
	// note the current pairing of create/load,  and unload/delete
	// in the future, we might need to seperate them out into two distinct stages 
	
	Dbg_Assert( pAsset );

	uint32 id = pAsset->m_id;

	Dbg_MsgAssert(pAsset->LoadFinished(), ("UnloadAsset(): Asset not finished loading"));

	pAsset->Unlink();
	pAsset->Unload();
	delete pAsset;				
	mp_asset_table->FlushItem(id);		
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAssMan::ReloadAsset( uint32 assetID, const char* pFileName, bool assertOnFail )
{
	Ass::CAsset* pAsset;

	pAsset = this->GetAssetNode( assetID, assertOnFail );

	if ( pAsset )
	{
		Dbg_MsgAssert(pAsset->LoadFinished(), ("ReloadAsset(): Asset not finished loading"));

		return pAsset->Reload( pFileName );
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CAsset*	CAssMan::GetAssetNode( uint32 assetID, bool assertOnFail )
{
	CAsset *p_asset = mp_asset_table->GetItem(assetID);
		
	if ( assertOnFail )
	{
		Dbg_MsgAssert(p_asset, ("Asset Not found <%s>", Script::FindChecksumName(assetID)));
		//Dbg_MsgAssert(p_asset->LoadFinished(), ("Asset not finished loading (%s)", Script::FindChecksumName(assetID)));
	}

	// Don't return pointer if it isn't done loading
	if ( p_asset && !p_asset->LoadFinished() )
	{
		//p_asset = NULL;

		// Wait if not done loading
		while (!p_asset->LoadFinished())
		{
			Dbg_Message("Waiting for async asset load to finish...");
			File::CAsyncFileLoader::sWaitForIOEvent(false);
		}
	}
	
	return p_asset;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void* CAssMan::GetAsset(uint32 assetID, bool assertOnFail )	
{
	// Return a pointer to the asset data, or NULL if not found
	
	CAsset *p_asset = mp_asset_table->GetItem(assetID);

	if (p_asset /*&& p_asset->LoadFinished()*/)
	{
		// Wait if not done loading
		while (!p_asset->LoadFinished())
		{
			Dbg_Message("Waiting for async asset load to finish...");
			File::CAsyncFileLoader::sWaitForIOEvent(false);
		}

		void *p_asset_data = p_asset->GetData();
		Dbg_MsgAssert(p_asset_data,("Asset has no data"));
		return	p_asset_data;
	}
	else
	{
		if ( assertOnFail )
		{
			Dbg_MsgAssert(p_asset, ("Asset 0x%x Not found (%s)",assetID,Script::FindChecksumName(assetID)));
			//Dbg_MsgAssert(p_asset->LoadFinished(), ("Asset 0x%x not finished loading (%s)",assetID,Script::FindChecksumName(assetID)));
		}
		return NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void* CAssMan::GetAsset(const char *p_assetName, bool assertOnFail)
{
	//returns a pointer to the asset data, does not load it if not found
	
	if ( assertOnFail )
	{
		Dbg_MsgAssert(AssetAllocated(p_assetName),("asset %s is not loaded in GetAsset",p_assetName));
	}
	void *p_asset_data = GetAsset(Script::GenerateCRC( p_assetName ), assertOnFail );
	
	if ( assertOnFail )
	{
		Dbg_MsgAssert(p_asset_data,("Asset has NULL data %s",p_assetName));
	}
	return p_asset_data;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void* CAssMan::LoadOrGetAsset(const char *p_assetName, bool async_load, bool use_pip, bool permanent, uint32 group, void* pExtraData, Script::CStruct * pParams)
{
	//returns a pointer to the asset data, and loads the asset if not found
	
	void *p_asset_data;
	CAsset *p_asset = mp_asset_table->GetItem(Script::GenerateCRC( p_assetName ));
	if (p_asset)
	{
		p_asset_data = p_asset->GetData();
		Dbg_MsgAssert(p_asset_data,("Asset has NULL data %s",p_assetName));
		//Dbg_MsgAssert(p_asset->LoadFinished(), ("Asset not finished loading (%s)", p_assetName));

		// Wait if not done loading
		while (!p_asset->LoadFinished())
		{
			Dbg_Message("Waiting for async asset load to finish...");
			File::CAsyncFileLoader::sWaitForIOEvent(false);
		}
	}
	else
	{
		p_asset_data = LoadAsset(p_assetName, async_load, use_pip, permanent, 0, pExtraData,pParams);	
	}
	
	if ( Script::GetInteger( CRCD(0x25dc7904,"AssertOnMissingAssets") ) )
	{
		// do some extra checks here if we're asserting on missing assets
		Dbg_MsgAssert(p_asset_data,("Could not load asset %s",p_assetName));
		Dbg_MsgAssert(AssetAllocated(p_assetName),("asset %s is not loaded after LoadOrGetAsset",p_assetName));
	}

	return p_asset_data;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAssMan::AssetAllocated(uint32 assetID)
{
	//  Return true if the asset has at least started loading
	
	return (NULL != mp_asset_table->GetItem(assetID));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAssMan::AssetAllocated(const char *p_assetName)
{
	//  Return true if the asset has at least started loading
	
	return (AssetAllocated(Script::GenerateCRC( p_assetName )));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAssMan::AssetLoaded(uint32 assetID)
{
	//  Return true if the asset is done loading
	CAsset *p_asset = mp_asset_table->GetItem(assetID);

	if (p_asset && p_asset->LoadFinished())
	{
		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAssMan::AssetLoaded(const char *p_assetName)
{
	//  Return true if the asset is done loading
	
	return (AssetLoaded(Script::GenerateCRC( p_assetName )));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAssMan::AddRef(uint32 asset_id, uint32 ref_id, uint32 group)
{
	// Add a reference to an asset, so we can access it via a new id

	Dbg_MsgAssert(AssetAllocated(asset_id),("Adding ref to unloaded asset\n"));
	CAsset *p_asset = new CRefAsset(mp_asset_table->GetItem(asset_id));
	Dbg_MsgAssert(!mp_asset_table->GetItem( ref_id ),("ref already in table/n"));	
	mp_asset_table->PutItem( ref_id,  p_asset);
	p_asset->SetID(ref_id);
	p_asset->SetGroup(group);

	// link it to the end of the list
	p_asset->mp_next = mp_assetlist_head;
	p_asset->mp_prev = mp_assetlist_head->mp_prev;
	mp_assetlist_head->mp_prev->mp_next = p_asset;
	mp_assetlist_head->mp_prev = p_asset;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAssMan::AddRef(const char *p_assetName, uint32 ref_id, uint32 group)
{
	Dbg_MsgAssert(!mp_asset_table->GetItem( ref_id ),("ref %lx already in table\nwhen adding to %s\n",ref_id,p_assetName));	
	Dbg_MsgAssert(AssetAllocated(p_assetName),("Adding ref to unloaded asset %s\n",p_assetName));
	uint32	checksum = Script::GenerateCRC( p_assetName );
	AddRef(checksum,ref_id, group);	
}
												 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAssMan::UnloadAllTables( bool destroy_permanent )
{
//	printf ( "Unloading all tables" );
	
	CAsset* p_asset;
	 
	p_asset = mp_assetlist_head->mp_next;
	while (p_asset != mp_assetlist_head)
	{
		CAsset *p_next = p_asset->mp_next;	
		if ( destroy_permanent || !p_asset->m_permanent )
		{
			// First destroy any references to this asset
			DestroyReferences( p_asset );
			
			// Need to update p_next, in case it was change by destroying above code
			p_next = p_asset->mp_next;	
	
			UnloadAsset( p_asset );
		}
		p_asset = p_next;		
	}

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAssMan::SetDefaultPermanent(bool permanent)
{
	// set the default permanent state of loaded assets
	// (generally this will be false)
	
	m_default_permanent = permanent;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAssMan::GetDefaultPermanent() const
{
	return m_default_permanent;
}

// The following functions are kind of kludge for referencing anims
// Ideally, I'd like to remove these kludges and simplify the
// asset manager

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAssMan::LoadAnim(const char* pFileName, uint32 animName, uint32 referenceChecksum, bool async_load, bool use_pip)
{
	// If the asset was previously loaded, then we are just creating a reference to it
	// so the m_permanent flag should be set based on m_default_permanent, and NOT INHERITED
			
	if (AssetAllocated(pFileName))
	{
		// if descChecksum was supplied, then adding a reference
		if (animName && !AssetAllocated(animName + referenceChecksum))
		{
			// adding a reference
			AddRef(pFileName, animName + referenceChecksum, referenceChecksum);	 
			// the m_permanent will have been inherited from the parent asset
			// Now get the reference asset, and set the permanent flag to m_defualt_permanet
			CAsset *p_asset = mp_asset_table->GetItem(animName + referenceChecksum);
			Dbg_MsgAssert(p_asset,("(Mick) Error re-getting asset ref for %s\n",pFileName));
			//p_asset->SetPermanent(m_default_permanent);  // <<<< this would also set the perm flag on the parent

			p_asset->m_permanent = m_default_permanent;		// just in the reference!!!
			return true;
		}
		else
		{
			//Dbg_MsgAssert( 0, ( "This file was already loaded %s", pFileName ) );
			return false;
		}
	}
	else
	{
		if (!LoadOrGetAsset(pFileName, async_load, use_pip, m_default_permanent, 0))
		{
			Dbg_MsgAssert(0,("Failed to load anim %s",pFileName));
			return false;
		}

		// Now add the reference to it, if one was requested
		// this gets combined with the "reference checksum"
		if (animName && !AssetAllocated(animName + referenceChecksum))
		{
			AddRef(pFileName, animName + referenceChecksum, referenceChecksum);	 
		}

		return true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAssMan::SetReferenceChecksum(uint32 reference_checksum)	   
{
	// kind of a kludge for anims

	m_reference_checksum = reference_checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CAssMan::GetReferenceChecksum() const
{
	return m_reference_checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Ass

#if 0
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAssMan::DumpAssets()
{
	CAsset * p_asset = mp_assetlist_head->mp_next;
	int i=0;
	while (p_asset != mp_assetlist_head)
	{
		CAsset *p_next = p_asset->mp_next;	
		printf ("Asset %3d: perm=%d %s\n",i,p_asset->m_permanent, p_asset->GetText());
		i++;
		p_asset = p_next;		
	}	
}
#endif
