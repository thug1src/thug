//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       cutscenedetails.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  01/13/2003
//****************************************************************************

#ifndef __OBJECTS_CUTSCENEDETAILS_H
#define __OBJECTS_CUTSCENEDETAILS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

#include <sk/objects/moviedetails.h>							   

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

namespace File
{
	class CFileLibrary;
}
		 
namespace Nx
{
	class CQuickAnim;
}

namespace Mth
{
	class Matrix;
	class Quat;
	class Vector;
}
				   
namespace Script
{
	class CScript;
	class CStruct;
}

namespace Obj
{

	class CCutsceneData;
	class CCompositeObject;

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

struct SCutsceneAnimInfo
{
	uint32						m_animName;
	Nx::CQuickAnim*				mp_bonedQuickAnim;
};

struct SCutsceneObjectInfo
{
	uint32						m_objectName;
	uint32						m_flags;
	uint32						m_skeletonName;
	bool						m_doPlayerSubstitution;
	SCutsceneAnimInfo*			mp_animInfo;
	Obj::CCompositeObject*		mp_object;
	Obj::CCompositeObject*		mp_parentObject;
	int							m_obaIndex;			// -1 if not valid
};

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

class CCutsceneData	: public Spt::Class
{
protected:
	enum
	{
		vMAX_CUTSCENE_ANIMS = 64,
		vMAX_CUTSCENE_OBJECTS = 64,
		vMAX_CUTSCENE_ASSETS = 64
	};

public:
	CCutsceneData();
	virtual ~CCutsceneData();

public:
    bool    			   		Load(const char* p_fileName, bool assertOnFail, bool async);
    bool    			   		PostLoad(bool assertOnFail);
	bool						LoadFinished();
	void						Update( Gfx::Camera* pCamera, Script::CStruct* pStruct );
	bool						IsAnimComplete();
	bool						OverridesCamera();
	bool						HasMovieStarted()
	{
		return m_videoStarted;
	}

protected:
	void						get_current_frame( Mth::Quat* pQuat, Mth::Vector* pTrans );
	void						update_camera( Gfx::Camera* pCamera );
	void						update_moving_objects();
	void						update_extra_cutscene_objects();
	void						update_boned_anims();	
	void 						add_boned_anim( uint32 objectName, uint32* pData, int fileSize );
	void						add_anim( uint32 animName, uint32* pData, int fileSize );
	void 						add_object( Script::CStruct* pParams );
	bool						create_objects( uint32* pData );
	SCutsceneObjectInfo*		get_object_info( uint32 objectName );
	SCutsceneAnimInfo*			get_anim_info( uint32 animName );
	void						update_video_playback( Gfx::Camera* pCamera, Script::CStruct* pStruct );

protected:
	void						init_objects();
	void						load_models();

protected:
	// need this pointer only during async loads
	File::CFileLibrary* 		mp_fileLibrary;
	bool						m_dataLoaded;
	
	Nx::CQuickAnim*				mp_cameraQuickAnim;
	Nx::CQuickAnim*				mp_objectQuickAnim;
	
	Mth::Vector					m_original_player_bounding_sphere;
	bool						m_original_hidden;
	bool						m_original_scale_enabled;
	Mth::Vector					m_original_pos;

	// this is used so that if there are multiple create-a-skaters,
	// they don't all try to overwrite the same m_original_*
	// fields...
	bool						m_original_params_set;

	SCutsceneObjectInfo			m_objects[vMAX_CUTSCENE_OBJECTS];
	int							m_numObjects;

	SCutsceneAnimInfo			m_bonedAnims[vMAX_CUTSCENE_ANIMS];
	int							m_numBonedAnims;

	uint32						m_assetName[vMAX_CUTSCENE_ASSETS];
	int							m_numAssets;
	
	bool						m_audioStarted;
	bool						m_videoStarted;
	bool						m_videoAborted;

	float						m_oldTime;
	float						m_currentTime;
	uint64						m_initialVBlanks;
};

class CCutsceneDetails : public CMovieDetails
{
public:
	CCutsceneDetails();
	virtual			~CCutsceneDetails();

public:
	virtual bool 				InitFromStructure(Script::CStruct *pParams);
	virtual void 				Update();
	virtual bool 				ResetCustomKeys();
	virtual bool 				IsComplete();
	virtual bool				IsHeld();
	virtual bool				OverridesCamera();
	virtual void				Cleanup();
	virtual bool				HasMovieStarted();
	
protected:
	void						hide_moving_objects();
	void						unhide_moving_objects();

protected:
	CCutsceneData*				mp_cutsceneAsset;
	uint32						m_cutsceneAssetName;
	uint32						m_startScriptName;
	bool						m_startScriptAlreadyRun;
	uint32						m_endScriptName;
	Script::CStruct*			mp_cutsceneStruct;

	float						m_oldAmbientLightModulationFactor;
	float						m_oldDiffuseLightModulationFactor[2];
	
	uint32*						mp_hiddenObjects;
	int							m_numHiddenObjects;
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

} // namespace Obj

#endif	// __OBJECTS_CUTSCENEDETAILS_H

