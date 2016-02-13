//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       moviedetails.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  06/17/2002
//****************************************************************************

#ifndef __OBJECTS_MOVIEDETAILS_H
#define __OBJECTS_MOVIEDETAILS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <core/list.h>
#include <core/math.h>

#include <gfx/animcontroller.h>						 

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

namespace Gfx
{
	class Camera;
	class CBonedAnimFrameData;
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

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

// CMovieDetails
class CMovieDetails : public Lst::Node<CMovieDetails>
{
public:
	CMovieDetails();
	virtual 				~CMovieDetails();

public:
	virtual bool			InitFromStructure( Script::CStruct* pParams ) = 0;
	virtual void			Update() = 0;
	virtual	bool			ResetCustomKeys() = 0;
	virtual bool			CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript ); 
	virtual bool			IsComplete() = 0;
	virtual bool			IsHeld() = 0;
	virtual bool			OverridesCamera() = 0;
	virtual void			Cleanup();
	virtual bool			HasMovieStarted() = 0;
	virtual bool			NeedsCameraTransition() { return false; }		// Only set to true on classes that need the
																			// old camera to stay around an extra frame
												  
public:
	void			    	SetParams( Script::CStruct* pParams );
	Script::CStruct* 		GetParams();
	bool					SetSkippable( bool skippable );
	bool					IsSkippable() { return m_skippable; }
	bool					SetName( uint32 name );
	uint32					GetName();
	bool					Abort( bool only_if_skippable );

	bool					ShouldPauseMovie() { return m_shouldPause; }
	bool					SetPauseMode( bool pause_mode );

	Gfx::Camera*			GetCamera() { return mp_camera; }
	
	uint32					GetExitScriptName() const
	{
		return m_exitScript;
	}
	Script::CStruct*		GetExitParams()
	{
		return mp_exitParams;
	}

protected:
	Gfx::Camera*			mp_camera;
	bool					m_holdOnLastFrame;
	bool					m_skippable;
	bool					m_aborted;
	bool					m_shouldPause;
	bool					m_allowPause;
	uint32					m_name;

	// exit parameters
	uint32					m_exitScript;
	Script::CStruct*		mp_exitParams;
};

// this can probably be split up into
// a target version and a non-target version
class CSkaterCamDetails : public CMovieDetails
{
public:
	CSkaterCamDetails();
	~CSkaterCamDetails();

public:
	virtual bool 	InitFromStructure(Script::CStruct *pParams);
	virtual void 	Update();
	virtual bool 	ResetCustomKeys();
	virtual bool	CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript ); 
	virtual bool 	IsComplete();
	virtual bool	IsHeld();
	virtual bool	OverridesCamera();
	virtual bool	HasMovieStarted()
	{
		return true;
	}
	virtual bool	NeedsCameraTransition() { return true; }		// Uses hack to work around glitches where there isn't a camera

protected:
	void			set_target_params( Script::CStruct* pParams );
	void			clear_target_params();
	void			set_frame_data( Gfx::CBonedAnimFrameData* pFrameData, Gfx::EAnimLoopingType loopingType, bool reverse );
	void			set_movie_length( float duration );
	void			get_current_frame( Mth::Quat* pQuat, Mth::Vector* pTrans );
	bool			process_custom_keys( float startTime, float endTime, bool end_time_inclusive );
	void			update_camera();
	void			update_camera_time();

protected:
	// target parameters
	bool			m_hasTarget;
	uint32			m_targetID;
	Mth::Vector		m_targetOffset;
	Mth::Vector		m_positionOffset;
	bool			m_hasTargetOffset;
	bool			m_hasPositionOffset;
	
	Gfx::CBonedAnimFrameData*	mp_frameData;
	
	// time-keeper
	Gfx::CAnimChannel		m_animController;
};					

class CObjectAnimDetails : public CMovieDetails
{
public:
	CObjectAnimDetails();
	virtual 		~CObjectAnimDetails();

public:
	virtual bool	InitFromStructure( Script::CStruct* pParams );
	virtual void	Update();
	virtual bool	ResetCustomKeys();
	virtual bool	IsComplete();
	virtual bool	IsHeld();
	virtual bool	OverridesCamera();
	virtual bool	HasMovieStarted()
	{
		return true;
	}
	
protected:
	void			set_frame_data( Gfx::CBonedAnimFrameData* pFrameData, Gfx::EAnimLoopingType loopingType, bool reverse );
	void			set_movie_length( float duration );
	bool			process_custom_keys( float startTime, float endTime, bool end_time_inclusive );
	void			update_moving_objects();

protected:
	Gfx::CBonedAnimFrameData*	mp_frameData;
	
	// time-keeper
	Gfx::CAnimChannel		m_animController;
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

#endif	// __OBJECTS_MOVIEDETAILS_H

