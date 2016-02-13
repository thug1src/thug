///////////////////////////////////////////////////////////////////////////
// RefAsset.cpp
//
// Mick
//
// Type of asset that just refers to another asset
// so you can have multiple indicies to reference an asset


#include	<gel/assman/asset.h>
#include	<gel/assman/refasset.h>


namespace Ass
{

CRefAsset::CRefAsset(CAsset *p_asset)
{

	mp_asset = p_asset;
	mp_sibling = NULL;
	if (p_asset->mp_ref_asset == NULL)
	{
		// this is the first reference, so just stick it in mp_ref_asset
		p_asset->mp_ref_asset = this;		
	}
	else
	{
		// it's a new asset, so insert it as the head
		// of siblings that refer to this
		CRefAsset *p_other_ref = p_asset->mp_ref_asset;
		p_asset->mp_ref_asset = this;
		mp_sibling = p_other_ref;
	}
	mp_data = NULL;							// Ref Assets explicitly have no data
	m_permanent = p_asset->m_permanent;		// inherit the permanence flag
}



CRefAsset::~CRefAsset()
{

// unhook the references
// from the assman table, the asset it refers to, and other assets
	
	if (mp_asset)
	{
		CRefAsset * p_prev = mp_asset->mp_ref_asset;
		if (p_prev == this)
		{
			mp_asset->mp_ref_asset = mp_sibling;
		}
		else
		{
			while (p_prev && p_prev->mp_sibling  != this)
			{
				p_prev = p_prev->mp_sibling;
			}
			Dbg_MsgAssert(p_prev,("Reference not listed in its parent asset")); 
			p_prev->mp_sibling = mp_sibling;
		}	
		mp_asset = NULL;	
		mp_sibling = NULL;
	}					
	

	Dbg_MsgAssert(mp_asset == NULL,("Destroying ref with non-null asset"));	
	Dbg_MsgAssert(mp_sibling == NULL,("Destroying ref with non-null sibling"));	
	Dbg_MsgAssert(mp_data == NULL,("Ref Asset has data!"));

}


////////////////////////////////////////////////////////////////////////////////////
// all other functions simply call the appropiate function on the referenced asset
// (might want to make these inline later)

int 			CRefAsset::Load(const char *p_file, bool async_load, bool use_pip, void* pExtraData)  	// create or load the asset
{
	Dbg_MsgAssert(!async_load, ("Async load not supported on CRefAsset"));

//	return mp_asset->Load(p_file);
	return 0;
}


int 			CRefAsset::Unload()                  	// Unload the asset
{
//	return mp_asset->Unload();
	return 0;
}


int 			CRefAsset::Reload(const char *p_file)
{
//	printf("Reloading dereferenced asset with %s\n", p_file);

	return mp_asset->Reload(p_file);
}

bool			CRefAsset::LoadFinished()
{
	Dbg_MsgAssert(GetData(), ("LoadFinished(): Data pointer NULL (load probably was never started)"));

	return mp_asset->LoadFinished();
}



const char* 	CRefAsset::Name()          			// printable name, for debugging
{
	return mp_asset->Name();
}


void      		CRefAsset::SetGroup(uint32 group)     // Unique group ID
{
//	mp_asset->SetGroup(group);
	m_group = group;
}


uint32    		CRefAsset::GetGroup()
{
//	return mp_asset->GetGroup();
	return m_group;
}


EAssetType	  	CRefAsset::GetType()         			// type is hard wired into asset class 
{
	return mp_asset->GetType();
}


void     		CRefAsset::SetData(void *p_data)      // return a pointer to the asset….
{
	mp_asset->SetData(p_data);
}


void *    		CRefAsset::GetData()             		// return a pointer to the asset….
{
	return mp_asset->GetData();
}


void			CRefAsset::SetPermanent(bool perm)
{
	m_permanent = perm;
	mp_asset->SetPermanent(perm);
}


}
