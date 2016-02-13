#include <stdio.h>
#include <eekernel.h>
#include <sifrpc.h>
#include <sifdev.h>
#include <libpad.h>
#include <libgraph.h>
#include <core/defines.h>
#include <sys/profiler.h>
#include "render.h"
#include "line.h"
#include "nx_init.h"
#include "chars.h"
#include "group.h"
#include "sprite.h"
#include "switches.h"
#include	"gfx/nx.h"
#include	"gfx/NxParticleMgr.h"
#include	"gfx/NxTextured3dPoly.h"
#include	"gfx/ngps/p_NxLightMan.h"


namespace Sys
{
	extern void Render_Profiler();		
}


namespace Gfx
{
	void DebugGfx_Draw( void );
}

namespace Tmr
{
	void VSync();	
}

namespace NxPs2
{

/*
void LineTest(void);
void CharTest(void);
*/

SFont *pMyFont1, *pMyFont2;



int maintest(int argc, char *argv[])
{
	return 0;
}




void test_init()
{
	InitialiseEngine();
}

// I'm forward declaring this, so the compiler does not try to inline it
volatile sGroup *get_render_group();
void	WaitForRendering();
void patch_texture_dma();


void	test_render_start_frame()
{

//------------------------------------------------------------

#if 0
// MICK: Note, this is done in the main loop now, for profiling reasons
	// wait for rendering completion
	NxPs2::WaitForRendering();
	// wait for vsync
	Tmr::VSync();
#endif
  
	// now the dma list has finished, we can swap dma buffers
	render::Field ^= 1;

	// kick off dma from previous frame (if there was one)
	if (render::Frame > 0)
		SendDisplayList();

}



// wait for pRenderGroup to be null
// note curius use of a seperate function to read pRenderGroup
// due to compler getting confused if we just read it directly
// even though it is volatile, it does not re-read it			   
void	WaitForRendering()
{
	while (get_render_group())
		;

	// Update the 2D DMA lists
	Update2DDMA();
	
}

void StuffAfterGSFinished()
{

	Nx::CPs2LightManager::sUpdateEngine();

	// particle systems effectively have their transform double-buffered for dma purposes
	// so now is the time to do the buffer flip by updating the transforms
	Nx::CEngine::sGetParticleManager()->UpdateParticles();

	// patch up texture DMA
	patch_texture_dma();	


}

					
volatile sGroup *get_render_group()
{
	return sGroup::pRenderGroup;
}



void	test_render(Mth::Matrix* camera_orient, Mth::Vector* camera_pos, float view_angle, float screen_aspect)
{
	//RenderPrologue();	 	// should this be in test_render_start_frame? (Garrett: Answer is no, it won't work)

	// render everything
	RenderWorld(camera_orient, camera_pos, view_angle, screen_aspect);
	//LineTest();
	//CharTest();
}

void	test_render_end_frame()
{
	SetTextWindow(0,HRES-1,0,VRES-1);						// a full-screen clipping window
	Gfx::DebugGfx_Draw( );

	// Make sure 2D VRAM is up-to-date
// Mick: Moved to CEngine::s_plat_render_world, which is earlier in the frame
// so it happens before we draw the texture splats.
//	Reallocate2DVRAM();

	SDraw2D::DrawAll();
	#ifdef		__USE_PROFILER__
	Sys::Render_Profiler();		
	#endif
	RenderEpilogue();
	
	// set frame for next render
	render::Frame++;
}



void LineTest(void)
{
/*	
	int xo,yo,zo;

	// do some 3D lines with r=0xFF, g=0x00, b=0x00, a=0x80
	BeginLines3D(0x800000FF);

	for (xo=-130; xo<=110; xo+=80)
		for (yo=-90; yo<=70; yo+=80)
			for (zo=-130; zo<=110; zo+=80)
			{
				DrawLine3D(-10+xo, 10+yo, 10+zo, 10+xo, 10+yo, 10+zo);
				DrawLine3D(-10+xo,-10+yo, 10+zo, 10+xo,-10+yo, 10+zo);
				DrawLine3D(-10+xo, 10+yo,-10+zo, 10+xo, 10+yo,-10+zo);
				DrawLine3D(-10+xo,-10+yo,-10+zo, 10+xo,-10+yo,-10+zo);
				
				DrawLine3D(-10+xo, 10+yo, 10+zo,-10+xo,-10+yo, 10+zo);
				DrawLine3D( 10+xo, 10+yo, 10+zo, 10+xo,-10+yo, 10+zo);
				DrawLine3D(-10+xo, 10+yo,-10+zo,-10+xo,-10+yo,-10+zo);
				DrawLine3D( 10+xo, 10+yo,-10+zo, 10+xo,-10+yo,-10+zo);
				
				DrawLine3D(-10+xo, 10+yo, 10+zo,-10+xo, 10+yo,-10+zo);
				DrawLine3D( 10+xo, 10+yo, 10+zo, 10+xo, 10+yo,-10+zo);
				DrawLine3D(-10+xo,-10+yo, 10+zo,-10+xo,-10+yo,-10+zo);
				DrawLine3D( 10+xo,-10+yo, 10+zo, 10+xo,-10+yo,-10+zo);
			}

	EndLines3D();

	// do some 2D lines with r=0xFF, g=0xFF, b=0xFF, a=0x80
	BeginLines2D(0x80FFFFFF);

	// "LINE"
	DrawLine2D(100,100,0, 100,150,0);
	DrawLine2D(100,150,0, 150,150,0);
	DrawLine2D(160,100,0, 160,150,0);
	DrawLine2D(170,100,0, 170,150,0);
	DrawLine2D(170,100,0, 220,150,0);
	DrawLine2D(220,100,0, 220,150,0);
	DrawLine2D(230,100,0, 230,150,0);
	DrawLine2D(230,100,0, 280,100,0);
	DrawLine2D(230,125,0, 260,125,0);
	DrawLine2D(230,150,0, 280,150,0);

	// "TEST"
	DrawLine2D(320,100,0, 370,100,0);
	DrawLine2D(345,100,0, 345,150,0);
	DrawLine2D(380,100,0, 380,150,0);
	DrawLine2D(380,100,0, 430,100,0);
	DrawLine2D(380,125,0, 410,125,0);
	DrawLine2D(380,150,0, 430,150,0);
	DrawLine2D(440,100,0, 490,100,0);
	DrawLine2D(440,125,0, 490,125,0);
	DrawLine2D(440,150,0, 490,150,0);
	DrawLine2D(440,100,0, 440,125,0);
	DrawLine2D(490,125,0, 490,150,0);
	DrawLine2D(500,100,0, 550,100,0);
	DrawLine2D(525,100,0, 525,150,0);

	EndLines2D();
*/
}




void CharTest(void)
{
#if 0
	float Width,Height;

	SetFont("small");
	QueryString("Hello World", &Width, &Height);	// width and height in pixels

	SetTextWindow(0,639,0,447);						// a full-screen clipping window

	BeginText(0x403070A0, 1.5);						// r=0xA0, g=0x70, b=0x30, a=0x40, scale=1.5
	DrawString("Score",  30, 30);
	DrawString("000000", 100, 30);
	EndText();

	BeginText(0x40A06080, 1.5);
	DrawString("Skate4", 520, 400);
	EndText();
#endif
}



} // namespace NxPs2

