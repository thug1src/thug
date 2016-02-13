//****************************************************************************
//* MODULE:         Ass
//* FILENAME:       cutsceneasset.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  01/13/2003
//****************************************************************************

// interface between asset manager, and the actual cutscene

#ifndef	__GEL_CUTSCENEASSET_H__
#define	__GEL_CUTSCENEASSET_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/assman/asset.h>

namespace Ass
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
class CCutsceneAsset : public CAsset
{

public:
        virtual int 				Load(const char *p_file, bool async_load, bool use_pip, void* pExtraData, Script::CStruct *pStruct);	// create or load the asset
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

