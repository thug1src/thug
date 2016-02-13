//****************************************************************************
//* MODULE:         Gel/AssMan
//* FILENAME:       AssMan.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  11/17/2000
//****************************************************************************

#ifndef __GEL_ASSMAN_H
#define __GEL_ASSMAN_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <core/hashtable.h>

#include <gel/assman/assettypes.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Lst
{
	template<class _V> class HashTable;
}

namespace Script
{
	class CStruct;
}

namespace Ass
{
	class CAsset;

/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/
   
class CAssMan : public Spt::Class 
{	
	DeclareSingletonClass( CAssMan );

public:
    
	CAssMan( void );
    ~CAssMan( void );

	// New generic functions
	EAssetType	FindAssetType(const char *p_assetName);

	void*		LoadAssetFromStream(uint32 asset_name, uint32 asset_type, uint32* p_data, int data_size, bool permanent, uint32 group);
	void*		LoadAsset(const char *p_assetName, bool async_load, bool use_pip = false, bool permanent = false, uint32 group = 0, void* pExtraData = NULL, Script::CStruct * pParams = NULL);
	bool		ReloadAsset( uint32 assetID, const char* pFileName, bool assertOnFail );
	void*		GetAsset(const char *p_assetName, bool assertOnFail = true);	
	void*		GetAsset(uint32	assetID, bool assertOnFail = true);	
	void*		LoadOrGetAsset(const char *p_assetName, bool async_load, bool use_pip, bool permanent = false, uint32 group = 0, void* pExtraData = NULL, Script::CStruct *pParams = NULL);	
	
	bool		AssetAllocated(uint32	assetID);
	bool		AssetAllocated(const char *p_assetName);

	bool		AssetLoaded(uint32	assetID);
	bool		AssetLoaded(const char *p_assetName);
	
	void		AddRef(uint32 asset_id, uint32 ref_id, uint32 group = 0);
	void		AddRef(const char *p_assetName, uint32 ref_id, uint32 group = 0);
	
	void		SetReferenceChecksum(uint32 reference_checksum);	   
	uint32		GetReferenceChecksum() const;
	
	void		SetDefaultPermanent(bool permanent);
	bool		GetDefaultPermanent() const;

	void* 		GetFirstInGroup(uint32 group);
	void* 		GetNthInGroup(uint32 group, int n);

	int 		GetIndexInGroup(uint32 checksum);
	int 		GetGroup(uint32 checksum);
	int 		CountGroup(uint32 group);
	int 		CountSameGroup(uint32 checksum);

	void		UnloadAllTables( bool destroy_permanent = false );
	void		DestroyReferences( CAsset* pAsset );
	void		UnloadAsset( CAsset* pAsset );
	CAsset*		GetAssetNode( uint32 assetID, bool assertOnFail );
	
	// ideally, the LoadAnim function should be made more generic
	bool		LoadAnim(const char* pFileName, uint32 animName, uint32 referenceChecksum, bool async_load, bool use_pip);
	
private:

	bool							m_default_permanent;
	uint32 							m_reference_checksum;	// used for anims, bit of a patch
	Lst::HashTable<Ass::CAsset>*	mp_asset_table;
	CAsset*							mp_assetlist_head;		   
};

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Ass

#endif	// __GEL_ASSMAN_H

