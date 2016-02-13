///////////////////////////////////////////////////////////
// asset.h - base class for managed assets

#ifndef	__GEL_ASSET_H__
#define	__GEL_ASSET_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <gel/assman/assettypes.h>
#include <core/string/cstring.h>

namespace Script
{
	class CStruct;
}


namespace Ass
{
class 	CAsset : public Spt::Class
{
	friend class CAssMan;
	friend class CRefAsset;

protected:																	 
		CAsset();	  				// constructor is private, so only CAssMan can create them
		virtual ~CAsset(); 			//

		virtual    	int 		Load(const char *p_file, bool async_load, bool use_pip, void* pExtraData, Script::CStruct *pParams);	// create or load the asset
		virtual		int			Load(uint32* p_data, int data_size);
		virtual    	int 		Unload();                  	// Unload the asset
		virtual    	int 		Reload(const char *p_file);
		virtual		bool		LoadFinished();				// Check to make sure asset is actually there


					void 		Unlink();                  	// Unlink the asset

// Some acessor functions for memory usage metrics					
		virtual   	const char* Name();          	// printable name, for debugging

// acessors for seting the low level id stuff 
// maybe this should just be derived from CObject?    
		void      			   	SetID(uint32 id);           	// Unique asset ID
		uint32    			   	GetID();
		virtual void      		SetGroup(uint32 group);     	// Unique group ID
		virtual uint32    		GetGroup();
		
		virtual EAssetType	  	GetType();         				// type is hard wired into asset class 
		virtual void     		SetData(void *p_data);          // return a pointer to the asset….
		virtual void *    		GetData();             			// return a pointer to the asset….
		virtual	void			SetPermanent(bool perm);

		void 					SetText(const char *p_text);
		const char *			GetText();

private:
		uint32				   	m_id;
		uint32				   	m_group;
		
		char				   	m_permanent;  					// should it stay in memory after a cleanup
		char					m_dead;							// asset is dead, remove it 		
		
		void *					mp_data;   						// pointer to the asset data

//		Str::String				m_text;

protected:
		CRefAsset	*			mp_ref_asset;					// pointer to first CRefAsset that points to this 

// The assets are also stored in a simple doubly linked list with a head node
// this is to simplify the 
		CAsset  *				mp_prev;
		CAsset  *				mp_next;
	
};



//////////////////////////////////////////////////////////////////////////
// Inline member functions																	 


inline		void 					CAsset::SetText(const char *p_text)
{
//	m_text = p_text;
}

inline 		const char *			CAsset::GetText()
{
//	return m_text.getString();
	return "names removed";
}
																	 
inline	void	CAsset::SetPermanent(bool perm) {m_permanent = perm;}


} // end namespace Ass

#endif			// #ifndef	__GEL_ASSET_H__

