//****************************************************************************
//* MODULE:         Ass
//* FILENAME:       skeletonasset.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  ??/??/????
//****************************************************************************

// interface between asset manager, and the actual skeleton data
// provides a common interface to the skeleton data

#ifndef	__GEL_SKELETONASSET_H__
#define	__GEL_SKELETONASSET_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/assman/asset.h>

namespace Ass
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
class CSkeletonAsset : public CAsset
{

public:
        virtual int 				Load(const char *p_file, bool async_load, bool use_pip, void* pExtraData, Script::CStruct *pStruct);	// create or load the asset
		virtual int 				Load(uint32* p_data, int data_size);		// create or load the asset from data stream
        virtual int 				Unload();                     				// unload the asset
        virtual int 				Reload(const char *p_file);
		virtual bool				LoadFinished();    							// check to make sure asset is actually there
     	virtual const char *  		Name();            							// printable name, for debugging
		virtual EAssetType 			GetType();         							// type is hard wired into asset class 
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}

#endif