///////////////////////////////////////////////////////////
// asset.h - base class for managed assets

#ifndef	__GEL_REFASSET_H__
#define	__GEL_REFASSET_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <gel/assman/assettypes.h>

namespace Ass
{



class 	CRefAsset : public CAsset
{
		friend class CAssMan;
protected:
						CRefAsset(CAsset *p_asset);
						~CRefAsset();
		
		int 			Load(const char *p_file, bool async_load, bool use_pip, void* pExtraData);	// create or load the asset
		int 			Unload();                  	// Unload the asset
		int 			Reload(const char *p_file);
		bool			LoadFinished();				// Check to make sure asset is actually there
		int  			RamUsage();
		int  			VramUsage();
		int  			SramUsage();
		const char* 	Name();          			// printable name, for debugging
		void      		SetGroup(uint32 group);     // Unique group ID
		uint32    		GetGroup();
		EAssetType	  	GetType();         			// type is hard wired into asset class 
		void     		SetData(void *p_data);      // return a pointer to the asset….
		void *    		GetData();             		// return a pointer to the asset….
		void			SetPermanent(bool perm);

private:
		CAsset	*				mp_asset;			// pointer to the actual asset 
		CRefAsset	*			mp_sibling;			// pointer to other CRefAsset that references mp_asset 	
};

//////////////////////////////////////////////////////////////////////////
// Inline member functions																	 
																	 


} // end namespace Ass

#endif			// #ifndef	__GEL_REFASSET_H__

