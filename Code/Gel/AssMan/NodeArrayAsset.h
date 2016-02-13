////////////////////////////////////////////////////////////////////////////
// animasset.h - interface between asset manager, and the actual animation
// provides a common interface to the 
#ifndef	__GEL_NODEARRAYASSET_H__
#define	__GEL_NODEARRAYASSET_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include	<gel/assman/asset.h>

namespace Ass
{

class 	CNodeArrayAsset : public CAsset
{

public:
	virtual int 				Load( const char *p_file, bool async_load, bool use_pip, void* pExtraData, Script::CStruct *pStruct );	// create or load the asset
	virtual int 				Unload();										// Unload the asset
	virtual int 				Reload( const char *p_file);
	virtual bool				LoadFinished();									// Check to make sure asset is actually there
	virtual const char *  		Name();											// printable name, for debugging
	virtual EAssetType 			GetType();										// type is hard wired into asset class
private:


	uint32						m_qb_checksum;
};


} // end namespace Ass

#endif			// #ifndef	__GEL_ASSET_H__

