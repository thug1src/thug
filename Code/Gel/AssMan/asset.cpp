///////////////////////////////////////////////////////////////////////////
// Asset.cpp
//
// Mick
//
// Base class of all assets
// provides basic interface to them


#include	<gel/assman/asset.h>
#include 	<gel/assman/assettypes.h>


namespace Ass
{

// when destroying an asset, we also unload it
// this should only be done via the asset manager

CAsset::CAsset()
{
}

CAsset::~CAsset()
{
}

// Unlink for the doubly linked list
void CAsset::Unlink()
{
	mp_next->mp_prev = mp_prev;
	mp_prev->mp_next = mp_next;
	mp_prev = this;
	mp_next = this;
}

int CAsset::Load(const char *p_file, bool async_load, bool use_pip, void* pExtraData , Script::CStruct *pParams)
{
	Dbg_MsgAssert(0,("CAsset::Load() should not be called"));
	return 0;
}  	

int CAsset::Load(uint32* p_data, int data_size)
{
	Dbg_MsgAssert(0,("CAsset::Load() from data buffer should not be called"));
	return 0;
}

int CAsset::Reload(const char *p_file)   
{
	Dbg_MsgAssert(0,("CAsset::Reload() should not be called"));
	return 0;
}

int 	CAsset::Unload()                  
{
	Dbg_MsgAssert(0,("CAsset::Unload() should not be called"));
	return 0;
}

bool	CAsset::LoadFinished()
{
	Dbg_MsgAssert(0,("CAsset::LoadFinished() should not be called"));
	return false;
}



const char *  CAsset::Name()            // printable name, for debugging
{
	return	"Unnamed Asset";	
}

void * 	CAsset::GetData()             // return a pointer to the asset's data
{
	return mp_data;					
}

void  	CAsset::SetData(void *p_data)             
{
	mp_data = p_data;				   
}


//----------------------------------------------------

void   CAsset::SetID(uint32 id)         
{
	m_id = id;
}

uint32 CAsset::GetID()
{
	return m_id;
}

void   CAsset::SetGroup(uint32 group)     
{
	m_group = group;
}

uint32 CAsset::GetGroup()
{
	return m_group;
}


EAssetType CAsset::GetType()         // type is hard wired into asset class 
{
	return ASSET_UNKNOWN; 		// for now return 0, not sure if this should return the EAssetType
}

}
