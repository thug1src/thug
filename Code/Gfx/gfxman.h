/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	Module:			Graphics  (GFX)											**
**																			**
**	File name:		gfx/gfxman.h											**
**																			**
**	Created: 		07/26/99	-	mjb										**
**																			**
*****************************************************************************/

#ifndef __GFX_GFXMAN_H
#define __GFX_GFXMAN_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/


#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/support.h>
#include <core/singleton.h>
#include <core/task.h>
#include <core/macros.h>

#include <sys/timer.h>


			  
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define USE_SKIN

namespace Gfx
{

#define OBJECT_RENDER_FLAG_BASIC_ATOMIC		( 1 << 0 )
#define OBJECT_RENDER_FLAG_SKIN_ATOMIC		( 1 << 1 )
#define OBJECT_RENDER_FLAG_PARTICLE_ATOMIC	( 1 << 2 )
#define OBJECT_RENDER_FLAG_HUD_ATOMIC		( 1 << 3 )



/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/
     
class  Manager  : public Spt::Class
{
	
	
public :
		

//	static const int vDEFAULT_VIDEO_HEIGHT = SCREEN_CONV_Y( 448 );
// Mick, this only applies to the virtual viewport, so don't convert for PAL	
	static const int vDEFAULT_VIDEO_HEIGHT = ( 448 );

//	static 	Image::RGBA				sBackgroundColor; 


	void						ToggleMetrics( void );
	void						ToggleVRAMViewer( void );
	void						DumpVRAMUsage( void );
	void						SetTrivialFarZClip( bool on )	{ m_trivial_far_z_clip = on; }

//	Metrics*					GetMetrics( void );
#ifdef __NOPT_ASSERT__
	void						AssertText( int line, const char* text );
	void						AssertFlush( void );
#endif

#	ifdef __PLAT_XBOX__
	void						SetGammaNormalized( float fr, float fg, float fb );
	void						GetGammaNormalized( float *fr, float *fg, float *fb );
#	endif

	void						ScreenShot( const char *fileroot );
	void						DumpMemcardScreeenshots();
	

	void						SetMinVblankWait( int num_vblanks );

	// used by other graphics system to update displays
	// This is used instead of GetTime() because this will
	// stop incrementing when the game is paused
	Tmr::Time					GetGfxTime( void ) {return m_time; }
													
private :

								~Manager( void );
								Manager( void );
	
	static	Tsk::Hook< Manager >::Code	s_start_render_code;
	static	Tsk::Hook< Manager >::Code	s_end_render_code;
	static	Tsk::Task< Manager >::Code	s_timer_code;

	Tsk::Hook< Manager >*		m_render_start_hook;
	Tsk::Hook< Manager >*		m_render_end_hook;
	Tsk::Task< Manager >*		m_timer_task;

	void						start_engine( void );
	void						stop_engine( void );
	
//	Metrics*					m_metrics;
	bool						m_metrics_active;
	bool						m_vram_viewer_active;
	bool						m_trivial_far_z_clip;


	uint32						m_min_vblank_wait;
	Tmr::Time					m_time;	// used by other graphics system to update displays
										// This is used instead of GetTime() because this will
										// stop incrementing when the game is paused
	
	DeclareSingletonClass(Manager)

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
	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void	Manager::ToggleVRAMViewer( void )
{
	
	
	m_vram_viewer_active = !m_vram_viewer_active;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void	Manager::SetMinVblankWait( int num_vblanks )
{
	m_min_vblank_wait = num_vblanks;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx

#endif	// __GFX_GFXMAN_H
