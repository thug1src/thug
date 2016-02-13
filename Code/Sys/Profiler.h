
/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SysLib (System Library)									**
**																			**
**	Module:			Sys  													**
**																			**
**	File name:		sys/profiler.h											**
**																			**
**	Created: 		11/15/00	-	mjb										**
**																			**
*****************************************************************************/

#ifndef __SYS_PROFILER_H
#define __SYS_PROFILER_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/list.h>
#include <sys/timer.h>

#ifndef	__SYS_MEM_MEMPTR_H
#include <sys/mem/memptr.h>
#endif
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#ifndef __PLAT_WN32__

#ifndef		__NOPT_FINAL__
#ifdef		__PLAT_NGPS__	
#define		__USE_PROFILER__			// use this one to turn profiling on/off
#endif		//	__PLAT_NGPS__	


#define	PROF_MAX_LEVELS		16		 			// depth of profiler stack
#define	PROF_MAX_CONTEXTS	512				// number of push/pops possible

namespace Sys
{


void		Render_Profiler();			// easy function to allow it to be rendered from simpler code 

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

// a profiler context represtens a single atomic measurement of time
// a simgle block of color on the profiler bar, at a particular level
struct  SProfileContext 
{
	uint32							m_color;				// color of this context
	int								m_depth;				// level in the stack
	Tmr::MicroSeconds				m_start_time;			// when it started
	Tmr::MicroSeconds				m_end_time;				// when it ended
};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  Profiler  : public Spt::Class
{
	

public:


#ifdef		__USE_PROFILER__
	void						PushContext( uint8 r, uint8 g, uint8 b, bool acc = false );
	void						PushContext( uint32 rgba, bool acc = false);
	void						PopContext( void );
		
	static void					sSetUp( uint vblanks );
	static void					sCloseDown( void );
	static void					sRender( void );

	static void					sEnable( void );
	static void					sDisable( void );
	static void					sStartFrame( void );
	static void					sEndFrame( void );
#else
	inline void					PushContext( uint8 r,
											 uint8 g,
											 uint8 b,
											 bool acc ){}
	inline void					PushContext( uint32 rgba,
											 bool acc ){}
	inline void					PopContext( void ){}
		
	inline  static void						sSetUp( uint vblanks ){}
	inline  static  void					sCloseDown( void ){}
	inline  static  void					sRender( void ){}

	inline  static  void					sEnable( void ){}
	inline  static  void					sDisable( void ){}
	inline  static  void					sStartFrame( void ){}
	inline  static  void					sEndFrame( void ){}
#endif	

								~Profiler( void );		 

private:
#ifdef		__USE_PROFILER__
								Profiler( uint line );
								
	void						render( void );
	void						start_new_frame( void );
	SProfileContext				m_context_list[ 2 ][PROF_MAX_CONTEXTS];		// a flat array of all contexts
	SProfileContext*			mp_current_context_list;					// pointer to one of the above, for speed
	
	SProfileContext	*			m_context_stack[ 2 ][PROF_MAX_LEVELS];		// stack of pushed contexts (usually max 3 deep)
	SProfileContext**			mpp_current_context_stack;					// pointer to one of the above, for speed
	
	int							m_context_count[2];
	int	*						mp_current_context_count;
	
	SProfileContext*			mp_current_context;							// current entry in context list (will need to pop back....
	int							m_context_stack_depth;						// depth in stack (index into m_context_stack[])
	int							m_next_context;								// index into array
	
	float						m_top_y;								// screen coord of top of profiler
	
	static Tmr::MicroSeconds	s_base_time[2];
	static int					s_flip;
	static bool					s_enabled;
	static bool					s_active;


	static uint					s_vblanks;
	static float				s_width;
	static float				s_height;
	static float 				s_scale;
	
	static float				s_left_x;

	static uint32				s_default_color;
	static Profiler*			s_cpu_profiler;

	bool						m_acc;					// Accumulation?
	bool						m_acc_first;
	Tmr::MicroSeconds			m_acc_time;				// Accumulation time.
	SProfileContext*			mp_acc_context;
#endif
};

/*****************************s************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

extern Profiler*	CPUProfiler;
extern Profiler*	VUProfiler;
extern Profiler*	DMAProfiler;

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/


} // namespace Sys

#endif
#endif	// __PLAT_WN32__
#endif // __SYS_PROFILER_H

