////////////////////////////////////////////////////////////////////////////
// skinasset.h - interface between asset manager, and the actual skin
// provides a common interface to the asset manager
#ifndef	__GEL_SKINASSET_H__
#define	__GEL_SKINASSET_H__



#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include	<gel/assman/asset.h>

namespace Ass
{

struct SCutsceneModelDataInfo
{
	uint32		modelChecksum;
	uint32*		pModelData;
	int			modelDataSize;
	uint32*		pTextureData;
	int			textureDataSize;
	uint8*		pCASData;
	uint32		texDictChecksum;
	int			texDictOffset;
	bool		isSkin;
	bool		doShadowVolume;
};

struct SSkinAssetLoadContext
{
	int			forceTexDictLookup;
	bool		doShadowVolume;
	int			texDictOffset;
};

class 	CSkinAsset : public CAsset
{

public:
        virtual int 				Load(const char *p_file, bool async_load, bool use_pip, void* pExtraData, Script::CStruct *pStruct);	// create or load the asset
		virtual int 				Load(uint32* p_data, int data_size);	// create or load the asset
        virtual int 				Unload();                     // Unload the asset
        virtual int 				Reload(const char *p_file);
		virtual bool				LoadFinished();    // Check to make sure asset is actually there
     	virtual const char *  		Name();            // printable name, for debugging
		virtual EAssetType 			GetType();         // type is hard wired into asset class 
private:
		

};


} // end namespace Ass

#endif			// #ifndef	__GEL_SKINASSET_H__

