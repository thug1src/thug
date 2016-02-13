/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 2002 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SysLib (System Library)									**
**																			**
**	Module:			Sys					 									**
**																			**
**	File name:		profiler.cpp											**
**																			**
**	Description:	Profiler class											**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <stdio.h>
#include <core/defines.h>
#include <sys/sys.h>
#include <sys/timer.h>
#include <sys/profiler.h>

#ifndef __PLAT_WN32__
#include <gfx/gfxman.h>
#endif // __PLAT_WN32__

#ifdef	__PLAT_NGPS__	
#include <gfx/ngps/nx/line.h>
#endif
#ifdef	__PLAT_NGC__	
#include <sys\ngc\p_gx.h>
#include <sys\ngc\p_prim.h>
#include <sys\ngc\p_camera.h>
#endif



#ifdef		__USE_PROFILER__


/*****************************************************************************
**								  Externals									**
*****************************************************************************/


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Sys
{



/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

Tmr::MicroSeconds	Profiler::s_base_time[ 2 ];
uint32				Profiler::s_default_color = 0x00ff0000;
int					Profiler::s_flip = 0;
#ifdef	__PLAT_NGC__	
bool				Profiler::s_enabled = true;
bool				Profiler::s_active = true;
#else
bool				Profiler::s_enabled = false;
bool				Profiler::s_active = false;
#endif
uint				Profiler::s_vblanks;
float				Profiler::s_width;
float				Profiler::s_height;
float 				Profiler::s_scale;
float				Profiler::s_left_x;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

Profiler* CPUProfiler;		  
Profiler* VUProfiler;		  
Profiler* DMAProfiler;		  


/*****************************************************************************
**							Private Functions								**
*****************************************************************************/

/******************************************************************/

Profiler::Profiler( uint line )
{
	m_top_y = s_height * line;
	start_new_frame();			// just ensure everything points to something reasonable
}

/******************************************************************/

Profiler::~Profiler( void )
{
}

/******************************************************************/

void			box(int x0,int y0, int x1, int y1, uint32 color)
{
#ifdef	__PLAT_NGPS__	
	NxPs2::BeginLines2D(0x80000000 + color);
	NxPs2::DrawLine2D(x0,y0,0, x1,y0,0);
	NxPs2::DrawLine2D(x1,y0,0, x1,y1,0);
	NxPs2::DrawLine2D(x1,y1,0, x0,y1,0);
	NxPs2::DrawLine2D(x0,y1,0, x0,y0,0);
	NxPs2::EndLines2D();
#endif



#ifdef	__PLAT_NGC__	
//	NsPrim::line ( (float)x0, (float)y0, (float)x1, (float)y0, (GXColor){(color>>0)&0xff,(color>>8)&0xff,(color>>16)&0xff,255} );
//	NsPrim::line ( (float)x1, (float)y0, (float)x1, (float)y1, (GXColor){(color>>0)&0xff,(color>>8)&0xff,(color>>16)&0xff,255} );
//	NsPrim::line ( (float)x1, (float)y1, (float)x0, (float)y1, (GXColor){(color>>0)&0xff,(color>>8)&0xff,(color>>16)&0xff,255} );
//	NsPrim::line ( (float)x0, (float)y1, (float)x0, (float)y0, (GXColor){(color>>0)&0xff,(color>>8)&0xff,(color>>16)&0xff,255} );

	GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){(color>>0)&0xff,(color>>8)&0xff,(color>>16)&0xff,255} );

	// Send coordinates.
	GX::Begin( GX_LINESTRIP, GX_VTXFMT0, 5 ); 
		GX::Position3f32( (float)x0, (float)y0, -1.0f );
		GX::Position3f32( (float)x1, (float)y0, -1.0f );
		GX::Position3f32( (float)x1, (float)y1, -1.0f );
		GX::Position3f32( (float)x0, (float)y1, -1.0f );
		GX::Position3f32( (float)x0, (float)y0, -1.0f );
	GX::End();

#endif


}

void			line(int x0,int y0, int x1, int y1, uint32 color)
{
#ifdef	__PLAT_NGPS__	
	NxPs2::BeginLines2D(0x80000000 + color);
	NxPs2::DrawLine2D(x0,y0,0, x1,y1,0);
	NxPs2::EndLines2D();
#endif


#ifdef	__PLAT_NGC__	
//	NsPrim::line ( (float)x0, (float)y0, (float)x1, (float)y1, (GXColor){(color>>0)&0xff,(color>>8)&0xff,(color>>16)&0xff,255} );

	GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){(color>>0)&0xff,(color>>8)&0xff,(color>>16)&0xff,255} );

	// Send coordinates.
	GX::Begin( GX_LINES, GX_VTXFMT0, 2 ); 
		GX::Position3f32( (float)x0, (float)y0, -1.0f );
		GX::Position3f32( (float)x1, (float)y1, -1.0f );
	GX::End();

#endif

}

/******************************************************************/

void			box(float x0,float y0, float x1, float y1, uint32 color)
{
	box ((int)x0,(int)y0,(int)x1,(int)y1, color);
}


/******************************************************************/

void Profiler::start_new_frame( void )
{
	mp_current_context_list 	= m_context_list[ s_flip ];
	mpp_current_context_stack 	= m_context_stack[ s_flip ];
	mp_current_context 			= NULL;								// nothing has been pushed yet
	mp_current_context_count	= &m_context_count[ s_flip ];
	m_context_stack_depth		= 0;								// nothing on stack
	m_next_context				= 0;								// start of list (index into mp_current_context_list)
	m_acc						= false;
	mp_acc_context				= NULL;
}

/******************************************************************/

void Profiler::render()
{
	if ( s_active )
	{
		/* We draw from the buffer that's not being updated this frame */
		int drawFlip = s_flip ^ 1;
		
		SProfileContext *p_context = m_context_list[ drawFlip ];
		Tmr::MicroSeconds	base_time = s_base_time[ drawFlip ];
		int n = m_context_count[ drawFlip ];
		while ( n--)
		{
			float startx 	= ( (uint32)(p_context->m_start_time - base_time) ) * s_scale;
			float endx 		= ( (uint32)(p_context->m_end_time - base_time) ) * s_scale;
			float top_y 	= m_top_y +( p_context->m_depth)* (s_height+2);
			box((s_left_x+startx),(top_y),  (s_left_x+endx),( top_y + s_height) , p_context->m_color);   
			p_context++;
		}
	}
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

void Profiler::PushContext( uint32 color, bool acc )
{
	
	if( s_active )
	{
		if ( acc && mp_acc_context )
		{
			// Want to accumulate to last context.
			m_acc_time = Tmr::GetTimeInUSeconds(); 
			m_acc_first = false;
		}
		else
		{
			// if we have a current context, then push it
			if (mp_current_context)
			{
				Dbg_MsgAssert(m_context_stack_depth < PROF_MAX_LEVELS,("Context stack overflow"));
				mpp_current_context_stack[m_context_stack_depth++] = mp_current_context;
			}

			// get a new context from the list
			Dbg_MsgAssert(m_next_context < PROF_MAX_CONTEXTS,("Context heap overflow"));
			mp_current_context = &mp_current_context_list[m_next_context++];
			// and fill it in with the start time, depth and color
			// (the end time is filled in by the Pop) 
			mp_current_context->m_start_time = Tmr::GetTimeInUSeconds();
			mp_current_context->m_depth = m_context_stack_depth;
			mp_current_context->m_color = color;
			if ( acc )
			{
				mp_acc_context = mp_current_context;
				m_acc_first = true;
			}
		}
		m_acc = acc;
	}
}


void Profiler::PushContext( uint8 r, uint8 g, uint8 b, bool acc )
{
	uint32 color = (b<<16)+(g<<8)+(r);
	PushContext(color, acc);
}

/******************************************************************/

void Profiler::PopContext( void )
{
	if( s_active)
	{
		if ( m_acc && !m_acc_first )
		{
			mp_acc_context->m_end_time += Tmr::GetTimeInUSeconds() - m_acc_time;
		}
		else
		{
			// set time on the current context		
			Dbg_MsgAssert(mp_current_context,("Popped one more profiler context than we pushed"));

			mp_current_context->m_end_time = Tmr::GetTimeInUSeconds();

			if ( ! m_context_stack_depth)
			{
				// nothing left on stack, so set current context to NULL
				mp_current_context = NULL;
				// update the count of valid contexts
				*mp_current_context_count = m_next_context;
			}
			else
			{
				// get the last context pushed on the stack
				mp_current_context = mpp_current_context_stack[--m_context_stack_depth]; 
			}
		}
	}
}

/******************************************************************/

void Profiler::sSetUp( uint vblanks )
{
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ProfilerHeap());
	
	s_width = 640 * 0.8;
	s_height = 448 * 0.01;
	s_left_x = 640 * 0.1;
	s_vblanks = vblanks;
	s_scale = ( s_width * 60 ) / ( 1000000 * s_vblanks );

	VUProfiler  =  new Profiler( 84 );
	DMAProfiler =  new Profiler( 86 );
	CPUProfiler =  new Profiler( 88 );
	
#ifdef	__PLAT_NGC__	
	s_active = true;
	s_enabled = true;
#else
	s_active = false;
	s_enabled = false;
#endif
	
	Mem::Manager::sHandle().PopContext();
	
}

/******************************************************************/
    
void Profiler::sCloseDown( void )
{
	delete VUProfiler;
	delete DMAProfiler;
	delete CPUProfiler;
	VUProfiler = NULL;
	DMAProfiler = NULL;
	CPUProfiler = NULL;
}

/******************************************************************/
    
void Profiler::sRender( void )
{
	if (s_active && s_enabled)
	{
#ifdef	__PLAT_NGC__	
		NsCamera cam;
		cam.orthographic( 0, 0, 640, 448 );
		cam.begin();
		GX::SetZMode( GX_FALSE, GX_ALWAYS, GX_TRUE );

		GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);

		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
											   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
											   GX_TEV_SWAP0, GX_TEV_SWAP0 );

		GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC,
										   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );

		GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
		GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );

		GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );
#endif	// __PLAT_NGC__	

		for (uint x = 0; x<s_vblanks+1;x++)
		{
			line((int)(s_left_x + x * s_width / s_vblanks), 370,   
				 (int)(s_left_x + x * s_width / s_vblanks), 410, 0x800080);   
			
		}
	
		VUProfiler->render();
		DMAProfiler->render();
		CPUProfiler->render();
	
#ifdef	__PLAT_NGC__	
		cam.end();
#endif	// __PLAT_NGC__	
	}
}

/******************************************************************/

void Profiler::sStartFrame( void )
{
	

	s_active = s_enabled;
	s_flip ^= 1;
	s_base_time[ s_flip ] = Tmr::GetTimeInUSeconds();
	CPUProfiler->start_new_frame();
	VUProfiler->start_new_frame();
	DMAProfiler->start_new_frame();
}
/******************************************************************/

void Profiler::sEndFrame( void )
{

}

/******************************************************************/

void Profiler::sEnable( void )
{
	s_enabled = true;
}


/******************************************************************/

void Profiler::sDisable( void )
{
	s_enabled = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Render_Profiler()			// easy function to allow it to be rendered from simpler code 
{
	Profiler::sRender();
}
    
} // namespace Sys
				  

#endif	//		__USE_PROFILER__


