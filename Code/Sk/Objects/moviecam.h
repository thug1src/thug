//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       moviemanager.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  02/07/2002
//****************************************************************************

#ifndef __OBJECTS_MOVIEMANAGER_H
#define __OBJECTS_MOVIEMANAGER_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

#include <core/list.h>

#include <gel/object.h>

#include <gfx/camera.h>

#include <sk/objects/moviedetails.h>

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

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

// CMovieManager
class CMovieManager : public Obj::CObject
{
	public:
	CMovieManager();
	virtual			~CMovieManager();

public:
	virtual bool				Update();
	void						ClearMovieQueue();
	bool						AddMovieToQueue( uint32 name, Script::CStruct* pParams, uint32 movieType );
	bool						RemoveMovieFromQueue( uint32 name );
	void						ApplyLastCamera();
	bool						SetMovieSkippable( uint32 name, bool skippable );
	bool						SetMoviePauseMode( uint32 name, bool pause_mode );
	Script::CStruct*			GetMovieParams( uint32 name );
	bool						AbortCurrentMovie( bool only_if_skippable );
	bool						IsMovieComplete( uint32 name );
	bool						IsMovieHeld( uint32 name );
	bool						IsRolling() { return ( get_movie_count() != 0 ); }// || m_use_last_movie_camera;}
	bool						OverridesCamera();
	Gfx::Camera*				GetActiveCamera();
	uint32						GetCurrentMovieName();
	void						CleanupMovie( CMovieDetails* pMovieDetails );
	bool						HasMovieStarted();
	bool						IsMovieQueued();

public:
	bool						CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript ); 

protected:
	int							get_movie_count();
	CMovieDetails*				get_movie_details( int index );
	CMovieDetails*				get_movie_details_by_name( uint32 name );
	void						update_movie_frame();
	void						setup_last_movie_camera( CMovieDetails* pDetails );

protected:
	Lst::Head<CMovieDetails>	m_movieDetailsList;

	// Currently, when we switch movie cameras, we delete the old one before adding
	// a new one.  Because the new one isn't added until a frame later, we need the
	// ability to have a smooth transition.  So we keep the old camera around for
	// an extra frame.
	// In the future, this code should go away and the cameras should be switched
	// at the same time.
	Gfx::Camera					m_last_movie_camera;
	uint32						m_last_movie_frame;
	bool						m_use_last_movie_camera;
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

#endif	// __OBJECTS_MOVIEMANAGER_H

