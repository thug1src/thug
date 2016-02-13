#include <stdio.h>
#include <stdlib.h>
#include <libgraph.h>
#include <libdma.h>
#include <libsn.h>
#include <devvu1.h>
#include <math.h>
#include <sifrpc.h>
#include <sifdev.h>
#include <libpad.h>
#include <core\math.h>
#include <core\defines.h>
#include <core\debug.h>
#include <gfx\nxviewman.h>
#include <sys\profiler.h>
#include <sys\timer.h>
#include "mikemath.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gif.h"
#include "gs.h"
#include "vu1code.h"
#include "dmacalls.h"
#include "line.h"
#include "chars.h"
#include "texture.h"
#include "material.h"
#include "group.h"
#include "scene.h"
#include "mesh.h"
#include "nx_init.h"
#include "render.h"
#include "interrupts.h"
#include "switches.h"
#include "sprite.h"
#include "instance.h"
#include "geomnode.h"
#include "occlude.h"
#include "resource.h"
#include "vu1context.h"
#include "light.h"
#include "fx.h"
#include "asmdma.h"
#include "vu1\newvu1code.h"

#include "gfx\nx.h"
#include "gfx\ngps\p_nxscene.h"
#include "gfx\ngps\p_nxtexture.h"


//#undef	__USE_PROFILER__


namespace Inp
{
	extern uint32	gDebugButtons[];
	extern uint32	gDebugMakes[];

}

namespace NxPs2
{

extern uint32	*p_patch_PRMODE;
extern uint32 gPassMask1;


extern int geom_stats_total;
extern int geom_stats_inactive ;
extern int geom_stats_sky;
extern int geom_stats_transformed;
extern int geom_stats_skeletal;
extern int geom_stats_camera_sphere;
extern int geom_stats_clipcull;
extern int geom_stats_culled;
extern int geom_stats_leaf_culled;
extern int geom_stats_boxcheck;
extern int geom_stats_occludecheck;
extern int geom_stats_occluded;
extern int geom_stats_occludecheck;
extern int geom_stats_occluded;
extern int geom_stats_occludecheck;
extern int geom_stats_occluded;
extern int geom_stats_colored;
extern int geom_stats_leaf;
extern int geom_stats_wibbleUV;
extern int geom_stats_wibbleVC;
extern int geom_stats_sendcontext;
extern int geom_stats_sorted;
extern int geom_stats_shadow;

// Mick:  Letterboxing on the ps2 is a post-processing
// step one frame ahead in the rendering pipeline (as
// compared to things like setting the FOV), so 
// it must be delayed by 1 frame to synchronize it
// with such events.
bool ActualDoLetterbox = false;
bool DoLetterbox = false;

int	DMAOverflowOK  = false;

bool DoFlipCopy = true;
sGroup PrologueGroup;
float SkaterY;

int	SingleRender = 0;

// skinning globals
Mat SkaterTransform;
Mat BoneTransforms[64];
int NumBones=0;


#if STENCIL_SHADOW
void CastStencilShadow(CInstance *pInstance, sGroup *p_group, Mth::Vector &ShadowVec, Mth::Vector &TweakVec);
#else
void CastShadow(CInstance *pInstance, sGroup *p_group);
#endif

void EnableFlipCopy(bool flip)
{
	DoFlipCopy = flip;
}

bool FlipCopyEnabled()
{
	return DoFlipCopy;
}

//////////////////////////////////////////////////////////////////
// void RenderPrologue(void)
//
// called at the start of a frame
// sets up the initial DMA list
// by setting up a prologue "group"
// consisting of an empty upload portion, and 
// a render portion that flips the visible buffers,
// and sets up various vif settings
// finally we initilize 	vu1::Loc = vu1::Buffer=0;
//
// Note, Field will be 0 or 1, depending on which buffer we are drawing into

void RenderPrologue(void)
{
	#ifdef	__NOPT_ASSERT__
	// a nasty patch by Mick to allow us to see the extra passes in flat shaded mode
	if (gPassMask1)
	{
		if (Inp::gDebugMakes[0] & (1<<5))			  // check for Circle pressed
		{
			*p_patch_PRMODE ^= 1;
		}
	}
	else
	{
		*p_patch_PRMODE = 1;
	}
	#endif
	
	
	
	// record the time
	render::sTime = (int)Tmr::GetTime();
					 
	// set dma pointer to the start of the dma buffer
	dma::pLoc = dma::pList[render::Field];
	dma::pBase = dma::pLoc;

	PrologueGroup.profile_color = 0x008080;		// yellow = prologue group

	// prologue render begins here
	PrologueGroup.pRender[render::Field] = dma::pLoc;
	PrologueGroup.Used[render::Field] = true; 			// prologue always used

	// call the DMA routine to flip frame buffers and clear the draw buffer
	// currently the flip is accomplished by rendering a full screen sprite from the
	// draw buffer to the display buffer.  This converts it from 32-bit to 16-bit (dithered)
	// and means there is no actual buffer switching going on
	// the draw buffer is always at the same address.
	// When the game is running at 60fps, this copying takes place during the vblank.
	
	//static bool last_flip_copy = true;
	if (DoFlipCopy /*&& last_flip_copy*/)	  // DoFlipCopy is set/cleared by the loading screen code (p_NxLoadScreen.cpp)
	{
		if (ActualDoLetterbox)
			dma::Gosub(FLIP_N_CLEAR_LETTERBOX,2);
		else
			dma::Gosub(FLIP_N_CLEAR,2);
		ActualDoLetterbox=DoLetterbox;
	}
	else
	{
		dma::Gosub(CLEAR_ZBUFFER,2);
	}
	//last_flip_copy = DoFlipCopy;

	// set the GS to the default rendering state
	dma::Gosub(SET_RENDERSTATE,2);
	dma::Gosub(SET_FOGCOL,2);

	// upload VU1 microcode	(the code in microcode.dsm)
	// This is done every frame, at the start of the frame
	// it actually only needs doing once, but it does not really take
	// any time, and doing it every frame gives us the flexibility to uploaded new
	// microcode at a later stage in rendering.
	// for now, all the microcode fits in the 16K code area of VU1
	dma::Tag(dma::ref, ((uint8 *)&MPGEnd-(uint8 *)&MPGStart+15)/16, (uint)&MPGStart);
	vif::NOP();
	vif::NOP();
	
	// VIF1 and VU1 setup
	dma::BeginTag(dma::end, 0);
	vif::FLUSH();
	vif::STMASK(0);
	vif::STMOD(0);
	vif::STCYCL(1,1);
	vif::BASE(0);
	vif::OFFSET(0);
	vif::MSCAL(VU1_ADDR(Setup));
	dma::EndTag();
	
	// prepare all groups for rendering
	for (sGroup *p_group=sGroup::pHead; p_group; p_group=p_group->pNext)
	{
		// initialise the dma list
		dma::BeginList(p_group);

		// assume textures aren't used until something actually gets rendered
		p_group->Used[render::Field] = false;
	}
	dma::sp_group = NULL;


	// while the dma context is null, set up an empty upload for the particles group and fog group
	sGroup::pParticles->pUpload[render::Field] = dma::pLoc;
	sGroup::pFog->pUpload[render::Field] = dma::pLoc;
	dma::Tag(dma::end, 0, 0);
	vif::NOP();
	vif::NOP();
	
	// signal that the particle group is used
	sGroup::pParticles->Used[render::Field] = true;

	// reset current marker number
	render::sMarkerIndex = 0;

	// count particles
	render::sTotalNewParticles = 0;
}



void RenderWorld(Mth::Matrix* camera_orient, Mth::Vector* camera_pos, float view_angle, float screen_aspect)
{
	Mth::Matrix transform(*camera_orient);
	transform[3] = *camera_pos;

	// need the transform to be well-formed as a 4x4 matrix
	transform[0][3] = 0.0f;
	transform[1][3] = 0.0f;
	transform[2][3] = 0.0f;
	transform[3][3] = 1.0f;

	Mth::Rect viewport1(0.0f, 0.0f, 1.0f, 1.0f);
	RenderViewport(0, transform, viewport1, view_angle, screen_aspect, -1.0f, -100000.0f);
}

void RenderInitImmediateMode()
{
	dma::SetList(sGroup::pEpilogue);

	// epilogue render (a terrible hack to put this here!)
	//sGroup::pEpilogue->pRender[render::Field] = dma::pLoc;
	sGroup::pEpilogue->Used[render::Field] = true;			// epilogue always rendered

	// VIF1 and VU1 setup
	dma::BeginTag(dma::cnt, 0);
	vif::FLUSH();
	vif::STROW((int)render::RowRegI[0], (int)render::RowRegI[1], (int)render::RowRegI[2], (int)render::RowRegI[3]);
	vif::STMASK(0);
	vif::STMOD(0);
	vif::STCYCL(1,1);
	vif::BASE(0);
	vif::OFFSET(0);
	vif::MSCAL(VU1_ADDR(Setup));
	vu1::Loc = vu1::Buffer=0;
	//vu1::BeginPrim(ABS, VU1_ADDR(L_VF10));
	//vu1::StoreVec(*(Vec *)&render::ViewportScale);				// VF10
	//vu1::StoreVec(*(Vec *)&render::ViewportOffset);				// VF11
	//vu1::StoreMat(*(Mat *)&render::AdjustedWorldToFrustum);		// VF12-15
	//vu1::StoreMat(*(Mat *)&render::AdjustedWorldToViewport);	// VF16-19
	//vu1::EndPrim(0);
	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::XYOFFSET_1, PackXYOFFSET(XOFFSET, YOFFSET));
	gs::Reg1(gs::SCISSOR_1,	 PackSCISSOR(0,HRES - 1,0,VRES - 1));
	gs::Reg1(gs::TEST_1, PackTEST(0,0,0,0,0,0,1,ZGEQUAL));
	gs::Reg1(gs::CLAMP_1,PackCLAMP(CLAMP,CLAMP,0,0,0,0));
	gs::Reg1(gs::PRMODECONT,	PackPRMODECONT(1));
	gs::EndPrim(1);
	vif::MSCAL(VU1_ADDR(Parser));
	dma::EndTag();
}

void RenderSwitchImmediateMode()
{
	dma::SetList(sGroup::pEpilogue);
}


// camera_transform must be a well-formed 4x4 matrix
void RenderViewport(uint viewport_num, const Mth::Matrix& camera_transform, const Mth::Rect& viewport_rec, float view_angle,
					float viewport_aspect, float near, float far, bool just_setup)
{
	// set up a load of persistent data for current viewport
	render::SetupVars(viewport_num, camera_transform, viewport_rec, view_angle, viewport_aspect, near, far);

	// Perform the DMA seperately, as the actual VARS might be re-set by update_render_vars called from plat_transform_to_screen
	render::SetupVarsDMA(viewport_num, near, far);

		// old and new style renders
		#	ifdef __USE_PROFILER__
			Sys::CPUProfiler->PushContext( 255, 0, 0 );	 	// Red (Under Yellow) = OldStyle Rendering	(Skins and stuff)
		#	endif // __USE_PROFILER__
			render::OldStyle();
		#	ifdef __USE_PROFILER__
			Sys::CPUProfiler->PopContext( );
		#	endif // __USE_PROFILER__
		
		
		#	ifdef __USE_PROFILER__
			Sys::CPUProfiler->PushContext( 0, 255, 0 );		// Green (Under Yellow) = NewStyle rendering (Environment)
		#	endif // __USE_PROFILER__
			
			render::NewStyle(just_setup);
		#	ifdef __USE_PROFILER__
			Sys::CPUProfiler->PopContext(  );
		#	endif // __USE_PROFILER__
}

// Uncomment the next line to use the color-per-mesh feature
#define USE_COLOR_PER_MESH

void render::OldStyle()
{
	// Don't render anything if it's not going to be displayed	
	if (!FlipCopyEnabled())
	{
		return;
	}
	
	float x,y,z,R;
	sMesh *pMesh;
	sGroup *p_group;
	int j;
	uint32 vu1_flags;
	uint32 colour;
	float colourComponent;

	Mth::Matrix LocalToWorld;
	Mth::Matrix LocalToCamera;
	Mth::Matrix LocalToFrustum;
	Mth::Matrix LocalToViewport;
	Mth::Matrix RootToWorld;
	Mth::Matrix RootToCamera;
	Mth::Matrix RootToFrustum;
	Mth::Matrix RootToViewport;
	Mth::Matrix LightMatrix;
	Mth::Matrix ColourMatrix;

	Mth::Vector ambientColour;
	Mth::Vector diffuseColour0;
	Mth::Vector diffuseColour1;
	Mth::Vector diffuseColour2;
	Mth::Vector colouredAmbientColour;
	Mth::Vector colouredDiffuseColour0;
	Mth::Vector colouredDiffuseColour1;
	Mth::Vector colouredDiffuseColour2;
	Mth::Vector v;

// Mick: Initialize these local matricies to known values....
	LocalToWorld.Zero();
	LocalToCamera.Zero();
	LocalToFrustum.Zero();
	LocalToViewport.Zero();
	RootToWorld.Zero();
	RootToCamera.Zero();
	RootToFrustum.Zero();
	RootToViewport.Zero();
	LightMatrix.Zero();
// And the vectors...	
	ambientColour.Set(0.0f,0.0f,0.0f,1.0f);
	diffuseColour0.Set(0.0f,0.0f,0.0f,1.0f);
	diffuseColour1.Set(0.0f,0.0f,0.0f,1.0f);
	diffuseColour2.Set(0.0f,0.0f,0.0f,1.0f);
	colouredAmbientColour.Set(0.0f,0.0f,0.0f,1.0f);
	colouredDiffuseColour0.Set(0.0f,0.0f,0.0f,1.0f);
	colouredDiffuseColour1.Set(0.0f,0.0f,0.0f,1.0f);
	colouredDiffuseColour2.Set(0.0f,0.0f,0.0f,1.0f);
	v.Set(0.0f,0.0f,0.0f,1.0f);
	

	
	#if STENCIL_SHADOW
	// prepare stencil shadow dma list for generating the stencil buffer
	dma::SetList(sGroup::pShadow);

	// send the GS setup
	dma::BeginTag(dma::cnt, 0);
	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::XYOFFSET_2,	PackXYOFFSET(XOFFSET, YOFFSET));
	gs::Reg1(gs::ZBUF_2,		PackZBUF(ZBUFFER_START,PSMZ24,1));
	gs::Reg1(gs::SCISSOR_2,		PackSCISSOR(0,HRES - 1,0,VRES - 1));
	gs::Reg1(gs::FRAME_2,		PackFRAME(0x1BA,HRES/64,PSMCT16S,0x00000000));
	//gs::Reg1(gs::FRAME_2,		PackFRAME(FRAME_START,HRES/64,PSMCT32,0x00000000));
	gs::Reg1(gs::ALPHA_2,		PackALPHA(0,2,2,1,128));
	gs::Reg1(gs::FBA_2,			PackFBA(0));
	gs::Reg1(gs::PABE,			PackPABE(0));
	gs::Reg1(gs::COLCLAMP,		PackCOLCLAMP(0));
	gs::Reg1(gs::PRMODECONT,	PackPRMODECONT(1));
	gs::Reg1(gs::DTHE,			PackDTHE(0));
	gs::Reg1(gs::TEST_2,		PackTEST(0,0,0,0,0,0,1,ZALWAYS));
	gs::Reg1(gs::PRIM,			PackPRIM(SPRITE,0,0,0,0,0,0,1,0));			// clear the stencil buffer
	gs::Reg1(gs::RGBAQ,			PackRGBAQ(0,0,0,0,0));
	gs::Reg1(gs::XYZ2,			PackXYZ(XOFFSET,YOFFSET,0));
	gs::Reg1(gs::XYZ2,			PackXYZ(0x10000-XOFFSET,0x10000-YOFFSET,0));
	gs::Reg1(gs::TEST_2,		PackTEST(0,0,0,0,0,0,1,ZGREATER));
	gs::Reg1(gs::FRAME_2,		PackFRAME(0x1BA,HRES/64,PSMCT16S,0xFF000000));
	gs::EndPrim(1);
	vif::MSCAL(VU1_ADDR(Parser));
	dma::EndTag();
	#else
	// prepare shadow dma list for generating the shadow texture
	dma::SetList(sGroup::pShadow);

	// send the GS setup
	dma::BeginTag(dma::cnt, 0);
	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::XYOFFSET_2,	PackXYOFFSET(0x7800,0x7800));
	gs::Reg1(gs::ZBUF_2,		PackZBUF(0,PSMZ16,1));
	gs::Reg1(gs::TEST_2,		PackTEST(0,0,0,0,0,0,1,ZALWAYS));
	gs::Reg1(gs::SCISSOR_2,		PackSCISSOR(0,255,0,255));
	gs::Reg1(gs::FRAME_2,		PackFRAME(0x1F0,4,PSMCT16,0x00000000));
	//gs::Reg1(gs::FRAME_2,		PackFRAME(0x000,HRES/64,PSMCT32,0x00000000));
	gs::Reg1(gs::FBA_2,			PackFBA(0));
	gs::Reg1(gs::PRIM,			PackPRIM(SPRITE,0,0,0,0,0,0,1,0));			// clear the texture
	gs::Reg1(gs::RGBAQ,			PackRGBAQ(0,0,0,0,0));
	gs::Reg1(gs::XYZ2,			PackXYZ(0x7800,0x7800,0));
	gs::Reg1(gs::XYZ2,			PackXYZ(0x8800,0x8800,0));
	gs::Reg1(gs::FBA_2,			PackFBA(1));								// set A on all pixels we touch
	gs::Reg1(gs::SCISSOR_2,		PackSCISSOR(1,254,1,254));					// don't touch edge of texture to avoid streaks
	gs::Reg1(gs::PRMODECONT,	PackPRMODECONT(0));
	gs::Reg1(gs::PRMODE,		PackPRMODE(0,0,0,0,0,0,1,0));
	gs::EndPrim(1);
	vif::MSCAL(VU1_ADDR(Parser));
	dma::EndTag();
	#endif


	// assume no dma context
	dma::SetList(NULL);

	// render old-style groups
	for (p_group=sGroup::pHead; p_group!=sGroup::pEpilogue; p_group=p_group->pNext)
	{
		// skip the old render mechanism if the group belongs to a pip-style scene
		if (p_group==sGroup::pShadow || !p_group->pScene || p_group->pScene->Flags & SCENEFLAG_USESPIP)
		{
			continue;
		}

		dma::SetList(p_group);

		dma::BeginTag(dma::cnt, 0);

			// VIF1 and VU1 setup
			vif::FLUSH();
			vif::STMASK(0);
			vif::STMOD(0);
			vif::STCYCL(1,1);
			vif::STCOL(0x3F800000,0,0,0);
			vif::BASE(0);
			vif::OFFSET(0);
			vif::MSCAL(VU1_ADDR(Setup));
			vu1::Loc = vu1::Buffer=0;

			// constant part of vu1 context data
			vu1::BeginPrim(ABS, VU1_ADDR(L_VF09));
//			vu1::StoreVec(*(Vec *)&render::AltFrustum);				// VF09
//			vu1::StoreVec(*(Vec *)&render::InverseViewportScale);	// VF10
//			vu1::StoreVec(*(Vec *)&render::InverseViewportOffset);	// VF11
			vu1::CopyQuads((uint32*)&render::AltFrustum, 
							(uint32*)&render::InverseViewportScale,
							(uint32*)&render::InverseViewportOffset
							);
			vu1::EndPrim(0);

			// send the GS viewport context
			gs::BeginPrim(ABS, 0, 0);
			gs::Reg1(gs::XYOFFSET_1, render::reg_XYOFFSET);
			gs::Reg1(gs::SCISSOR_1,  render::reg_SCISSOR);
			gs::EndPrim(1);
			vif::MSCAL(VU1_ADDR(Parser));

		dma::EndTag();


		Mth::Matrix refl_temp;
		Mth::Matrix  	ReflVecs;
		refl_temp[0] = Mth::Vector(1.0f, 0.0f, 0.0f, 0.0f);
		refl_temp[1] = Mth::Vector(0.0f, 1.0f, 0.0f, 0.0f);
		refl_temp[2] = Mth::Vector(0.0f, 0.0f, 1.4f, 0.0f);
		refl_temp[3] = Mth::Vector(0.0f, 0.0f, 0.0f, 0.0f);

		// this part handles the instanced non-skinned models
		if (p_group->pScene && (p_group->pScene->Flags & SCENEFLAG_INSTANCEABLE) &&
			p_group->pMeshes && !(p_group->pMeshes->Flags & MESHFLAG_SKINNED))
		{
			for (CInstance *pInstance=p_group->pScene->pInstances; pInstance; pInstance=pInstance->GetNext())
			{
				// make sure the instance is active
				if (!pInstance->IsActive())
				{
					continue;
				}

				LocalToWorld = *(Mth::Matrix *)pInstance->GetTransform();
				LocalToCamera = LocalToWorld * render::WorldToCamera;
				LocalToFrustum = LocalToWorld * render::WorldToFrustum;
				LocalToViewport = LocalToFrustum * render::FrustumToViewport;

				// set up reflection map transform
				sceVu0MulMatrix((sceVu0FVECTOR*)&ReflVecs,
								(sceVu0FVECTOR*)&refl_temp,
								(sceVu0FVECTOR*)&LocalToCamera);

				// send VIF/VU context
				dma::BeginTag(dma::cnt, 0);
				vif::STROW(0,0,0,0);
				vu1::BeginPrim(ABS, VU1_ADDR(L_VF12));
				vu1::StoreMat(*(Mat *)&LocalToViewport);	// VF12-15
				vu1::StoreMat(*(Mat *)&ReflVecs);			// VF16-19
				vu1::EndPrim(0);

				//gs::BeginPrim(ABS,0,0);
				//gs::Reg1(gs::FOGCOL, PackFOGCOL(0x60,0x40,0xC0));
				//gs::EndPrim(0);

				dma::EndTag();

				// traverse array of meshes in this group
				for (j=0,pMesh=p_group->pMeshes; j<p_group->NumMeshes; j++,pMesh++)
				{
					// skip if invisible
					if (!pMesh->IsActive())
					{
						continue;
					}

					// do nothing if mesh doesn't have a dma subroutine
					if (!pMesh->pSubroutine)
					{
						continue;
					}

					v = pMesh->Sphere;
					v[3] = 1.0f;
					v *= LocalToCamera;
					x = v[0];
					y = v[1];
					z = v[2];
					R = pMesh->Sphere[3];

					// if all outside view volume then cull
					if (R<z-render::Near || R<render::Far-z							||
						R<render::sy*z+render::cy*y || R<render::sy*z-render::cy*y	||
						R<render::sx*z-render::cx*x || R<render::sx*z+render::cx*x)
						continue;

					// else if all inside outer volume then render normally
					else if (R<render::Near-z && R<z-render::Far							&&
							 R<-render::Sy*z-render::Cy*y && R<-render::Sy*z+render::Cy*y	&&
							 R<-render::Sx*z+render::Cx*x && R<-render::Sx*z-render::Cx*x)
					{
						dma::Gosub3D(pMesh->pSubroutine, PROJ);
					}

					// otherwise cull at the vertex level
					else
					{
						dma::Gosub3D(pMesh->pSubroutine, CLIP);
					}
					
					p_group->Used[render::Field] = true;	  	// Note, should have culling before here
					

				}

			}

		}


		// this part handles the instanced skinned models
		if (p_group->pScene && (p_group->pScene->Flags & SCENEFLAG_INSTANCEABLE) &&
			p_group->pMeshes && (p_group->pMeshes->Flags & MESHFLAG_SKINNED))
		{
			for (CInstance *pInstance=p_group->pScene->pInstances; pInstance; pInstance=pInstance->GetNext())
			{
				// make sure the instance is active
				if (!pInstance->IsActive())
				{
					continue;
				}

				// shadow
				if (pInstance->CastsShadow())
				{
					#if STENCIL_SHADOW
					Mth::Vector shadow_vec(20.0f, -100.0f, 0.0f, 0.0f);
					Mth::Vector tweak_vec(0.4f, -2.0f, 0.0f, 0.0f);
					CastStencilShadow(pInstance, p_group, shadow_vec, tweak_vec);
					#else
					CastShadow(pInstance, p_group);
					#endif
				}

				// mat for bounding sphere test
				RootToWorld = *(Mth::Matrix *)pInstance->GetTransform();
				RootToCamera = RootToWorld * render::WorldToCamera;

				// get bounding sphere
				v = pInstance->GetBoundingSphere();
				R = v[3];
				v[3] = 1.0f;
				v *= RootToCamera;
				x = v[0];
				y = v[1];
				z = v[2];

				// if all outside view volume then cull
				if (R<z-render::Near || R<render::Far-z							||
					R<render::sy*z+render::cy*y || R<render::sy*z-render::cy*y	||
					R<render::sx*z-render::cx*x || R<render::sx*z+render::cx*x)
					continue;

				// else if all inside outer volume then render normally
				else if (R<render::Near-z && R<z-render::Far							&&
						 R<-render::Sy*z-render::Cy*y && R<-render::Sy*z+render::Cy*y	&&
						 R<-render::Sx*z+render::Cx*x && R<-render::Sx*z-render::Cx*x)
				{
					vu1_flags = PROJ;
				}

				// otherwise cull at the vertex level
				else
				{
					vu1_flags = CULL;
				}

				// wireframe mode
				if (pInstance->IsWireframe())
				{
					vu1_flags |= WIRE;
				}

				p_group->Used[render::Field] = true;	  	// Note, should have culling before here					

				dma::BeginTag(dma::cnt, 0);


				// adjust matrix for 0:8 fixed point weights format (used to be 1:15)
				RootToCamera[0] *= 128.0f;
				RootToCamera[1] *= 128.0f;
				RootToCamera[2] *= 128.0f;

				// send vu1 context
				RootToFrustum = RootToCamera * render::CameraToFrustum;
				RootToViewport = RootToFrustum * render::FrustumToIntViewport;

				CLightGroup *pLightGroup = pInstance->GetLightGroup();

				if (pLightGroup)
				{
					const Mth::Vector &light_dir0 = pLightGroup->GetDirection(0);
					const Mth::Vector &light_dir1 = pLightGroup->GetDirection(1);
					const Mth::Vector &light_dir2 = pLightGroup->GetDirection(2);

					LightMatrix[0][0] = light_dir0[0];
					LightMatrix[1][0] = light_dir0[1];
					LightMatrix[2][0] = light_dir0[2];
					LightMatrix[0][1] = light_dir1[0];
					LightMatrix[1][1] = light_dir1[1];
					LightMatrix[2][1] = light_dir1[2];
					LightMatrix[0][2] = light_dir2[0];
					LightMatrix[1][2] = light_dir2[1];
					LightMatrix[2][2] = light_dir2[2];
				} else {
					const Mth::Vector &light_dir0 = CLightGroup::sGetDefaultDirection(0);
					const Mth::Vector &light_dir1 = CLightGroup::sGetDefaultDirection(1);
					const Mth::Vector &light_dir2 = CLightGroup::sGetDefaultDirection(2);

					LightMatrix[0][0] = light_dir0[0];
					LightMatrix[1][0] = light_dir0[1];
					LightMatrix[2][0] = light_dir0[2];
					LightMatrix[0][1] = light_dir1[0];
					LightMatrix[1][1] = light_dir1[1];
					LightMatrix[2][1] = light_dir1[2];
					LightMatrix[0][2] = light_dir2[0];
					LightMatrix[1][2] = light_dir2[1];
					LightMatrix[2][2] = light_dir2[2];
				}

				Mth::Matrix temp = *(Mth::Matrix *)pInstance->GetTransform();
				temp[0].Normalize();							// do something sensible with a scaled matrix
				temp[1].Normalize();
				temp[2].Normalize();
				LightMatrix = temp * LightMatrix;

				if (pLightGroup)
				{
					ambientColour  = pLightGroup->GetAmbientColor();
					diffuseColour0 = pLightGroup->GetDiffuseColor(0);
					diffuseColour1 = pLightGroup->GetDiffuseColor(1);
					diffuseColour2 = pLightGroup->GetDiffuseColor(2);
				}
				else
				{
					ambientColour  = CLightGroup::sGetDefaultAmbientColor();
					diffuseColour0 = CLightGroup::sGetDefaultDiffuseColor(0);
					diffuseColour1 = CLightGroup::sGetDefaultDiffuseColor(1);
					diffuseColour2 = CLightGroup::sGetDefaultDiffuseColor(2);
				}

				// apply instance colour

#ifdef USE_COLOR_PER_MESH
				if (pInstance->HasColorPerMaterial())
				{
					// Get first color for now
					colour = pInstance->GetMaterialColorByIndex(0);
				}
				else
				{
					colour = pInstance->GetColor();
				}
#else
				colour = pInstance->GetColor();
#endif
				colourComponent = (float)(colour & 0xFF) * 0.0078125f;
				colouredAmbientColour[0]  = ambientColour[0]  * colourComponent;
				colouredDiffuseColour0[0] = diffuseColour0[0] * colourComponent;
				colouredDiffuseColour1[0] = diffuseColour1[0] * colourComponent;
				colouredDiffuseColour2[0] = diffuseColour2[0] * colourComponent;
				colourComponent = (float)(colour>>8 & 0xFF) * 0.0078125f;
				colouredAmbientColour[1]  = ambientColour[1]  * colourComponent;
				colouredDiffuseColour0[1] = diffuseColour0[1] * colourComponent;
				colouredDiffuseColour1[1] = diffuseColour1[1] * colourComponent;
				colouredDiffuseColour2[1] = diffuseColour2[1] * colourComponent;
				colourComponent = (float)(colour>>16 & 0xFF) * 0.0078125f;
				colouredAmbientColour[2]  = ambientColour[2]  * colourComponent;
				colouredDiffuseColour0[2] = diffuseColour0[2] * colourComponent;
				colouredDiffuseColour1[2] = diffuseColour1[2] * colourComponent;
				colouredDiffuseColour2[2] = diffuseColour2[2] * colourComponent;

				LightMatrix[3]  = colouredAmbientColour  * 1.0f/(128.0f*8388608.0f);
				ColourMatrix[0] = colouredDiffuseColour0 * 1.0f/(128.0f*8388608.0f);
				ColourMatrix[1] = colouredDiffuseColour1 * 1.0f/(128.0f*8388608.0f);
				ColourMatrix[2] = colouredDiffuseColour2 * 1.0f/(128.0f*8388608.0f);

				// vu context data
				vu1::BeginPrim(ABS, VU1_ADDR(L_VF10));
				
//				vu1::StoreVec(*(Vec *)&render::InverseIntViewportScale);	// VF10
//				vu1::StoreVec(*(Vec *)&render::InverseIntViewportOffset);	// VF11
				vu1::CopyQuads((uint32*)&render::InverseIntViewportScale,
							   (uint32*)&render::InverseIntViewportOffset);
					
				vu1::StoreMat(*(Mat *)&RootToFrustum);			// VF12-15
				vu1::StoreMat(*(Mat *)&RootToViewport);			// VF16-19
				vu1::StoreMat(*(Mat *)&LightMatrix);			// VF20-23
				vu1::StoreMat(*(Mat *)&ColourMatrix);			// VF24-27
				vu1::EndPrim(0);

				// gs context data, specifically the fog coefficient
				float w,f;
				w = -RootToCamera[3][2];
				if ((w > FogNear) && EnableFog)	// Garrett: We have to check for EnableFog here because the VU1 code isn't
				{
					f = 1.0 + EffectiveFogAlpha * (FogNear/w - 1.0f);
				}
				else
				{
					f = 1.0f;
				}
				gs::BeginPrim(ABS,0,0);
				gs::Reg1(gs::FOG, PackFOG((int)(f*255.99f)));
				//gs::Reg1(gs::FOGCOL, PackFOGCOL(0x60,0x40,0xC0));
				gs::EndPrim(1);

				vif::MSCAL(VU1_ADDR(Parser));

				// get free run of the VUMem
				vif::FLUSHA();

				dma::EndTag();

				// send transforms
				vu1::Loc = 0;
				dma::Tag(dma::ref, pInstance->GetNumBones()<<2, (uint)pInstance->GetBoneTransforms());
				vif::NOP();
				vif::UNPACK(0,V4_32,pInstance->GetNumBones()<<2,ABS,SIGNED,0);

				dma::BeginTag(dma::cnt, 0);

				vif::ITOP(pInstance->GetNumBones()<<2);
				vif::MSCAL(VU1_ADDR(ReformatXforms));

				// not using pre-translate
				// don't forget to switch off the offset mode on xyz unpack if using the row reg for something else
				vif::STROW(0,0,0,0);

				dma::EndTag();

				// traverse colour-unlocked meshes
				for (j=0,pMesh=p_group->pMeshes; j<p_group->NumMeshes; j++,pMesh++)
				{
					// skip if invisible, no subroutine, or not affected by dynamic colouring
					if (!pMesh->IsActive() || !pMesh->pSubroutine || (pMesh->Flags & MESHFLAG_COLOR_LOCKED))
					{
						continue;
					}

#ifdef USE_COLOR_PER_MESH
					uint32 mesh_color;
					// Garrett: Thought I had the material color array in the right order, but the render order
					// is different.  Right now I'm just searching out each color, but I'll need to go back
					// and see if it is possible to put the color array in render order (i.e. is the render order
					// static).
					if (pInstance->HasColorPerMaterial() && (colour != (mesh_color = pInstance->GetMaterialColor(pMesh->MaterialName, pMesh->Pass))))
					{
						colour = mesh_color;
						colourComponent = (float)(colour & 0xFF) * 0.0078125f;
						colouredAmbientColour[0]  = ambientColour[0]  * colourComponent;
						colouredDiffuseColour0[0] = diffuseColour0[0] * colourComponent;
						colouredDiffuseColour1[0] = diffuseColour1[0] * colourComponent;
						colouredDiffuseColour2[0] = diffuseColour2[0] * colourComponent;
						colourComponent = (float)(colour>>8 & 0xFF) * 0.0078125f;
						colouredAmbientColour[1]  = ambientColour[1]  * colourComponent;
						colouredDiffuseColour0[1] = diffuseColour0[1] * colourComponent;
						colouredDiffuseColour1[1] = diffuseColour1[1] * colourComponent;
						colouredDiffuseColour2[1] = diffuseColour2[1] * colourComponent;
						colourComponent = (float)(colour>>16 & 0xFF) * 0.0078125f;
						colouredAmbientColour[2]  = ambientColour[2]  * colourComponent;
						colouredDiffuseColour0[2] = diffuseColour0[2] * colourComponent;
						colouredDiffuseColour1[2] = diffuseColour1[2] * colourComponent;
						colouredDiffuseColour2[2] = diffuseColour2[2] * colourComponent;

						LightMatrix[3]  = colouredAmbientColour  * 1.0f/(128.0f*8388608.0f);
						ColourMatrix[0] = colouredDiffuseColour0 * 1.0f/(128.0f*8388608.0f);
						ColourMatrix[1] = colouredDiffuseColour1 * 1.0f/(128.0f*8388608.0f);
						ColourMatrix[2] = colouredDiffuseColour2 * 1.0f/(128.0f*8388608.0f);

						// change lighting context
						dma::BeginTag(dma::cnt, 0);
						vif::BASE(256);
						vif::OFFSET(0);
						vu1::Loc = 256;
						vu1::BeginPrim(ABS, VU1_ADDR(L_VF20));
						vu1::StoreMat(*(Mat *)&LightMatrix);			// VF20-23
						vu1::StoreMat(*(Mat *)&ColourMatrix);			// VF24-27
						vu1::EndPrim(1);
						vif::MSCAL(VU1_ADDR(ParseInit));
						vif::FLUSH();
						dma::EndTag();
					}
#endif // USE_COLOR_PER_MESH

					// render the mesh
					vu1::Loc = vu1::Buffer = 256;
					dma::BeginTag(dma::call,(uint)pMesh->pSubroutine);
					vif::BASE(256);
					vif::OFFSET(0);
					vif::MSCAL(VU1_ADDR(Setup));
					vif::ITOP(vu1_flags);
					vif::FLUSH();
					dma::EndTag();
					vu1::Loc += ((uint16 *)pMesh->pSubroutine)[1];
				}


				// change lighting context
				dma::BeginTag(dma::cnt, 0);
				vif::BASE(256);
				vif::OFFSET(0);
				vu1::Loc = 256;
				LightMatrix[3]  = ambientColour  * 1.0f/(128.0f*8388608.0f);
				ColourMatrix[0] = diffuseColour0 * 1.0f/(128.0f*8388608.0f);
				ColourMatrix[1] = diffuseColour1 * 1.0f/(128.0f*8388608.0f);
				ColourMatrix[2] = diffuseColour2 * 1.0f/(128.0f*8388608.0f);
				vu1::BeginPrim(ABS, VU1_ADDR(L_VF20));
				vu1::StoreMat(*(Mat *)&LightMatrix);			// VF20-23
				vu1::StoreMat(*(Mat *)&ColourMatrix);			// VF24-27
				vu1::EndPrim(1);
				vif::MSCAL(VU1_ADDR(ParseInit));
				vif::FLUSH();
				dma::EndTag();

				// traverse colour-locked meshes
				for (j=0,pMesh=p_group->pMeshes; j<p_group->NumMeshes; j++,pMesh++)
				{
					// skip if invisible, no subroutine, or affected by dynamic colouring
					if (!pMesh->IsActive() || !pMesh->pSubroutine || !(pMesh->Flags & MESHFLAG_COLOR_LOCKED))
					{
						continue;
					}

					// render the mesh
					vu1::Loc = vu1::Buffer = 256;
					dma::BeginTag(dma::call,(uint)pMesh->pSubroutine);
					vif::BASE(256);
					vif::OFFSET(0);
					vif::MSCAL(VU1_ADDR(Setup));
					vif::ITOP(vu1_flags);
					vif::FLUSH();
					dma::EndTag();
					vu1::Loc += ((uint16 *)pMesh->pSubroutine)[1];
				}


			}

		}

	}


	#if STENCIL_SHADOW
	// postprocess stencil buffer
	dma::SetList(sGroup::pShadow);

	// send the GS setup
	dma::BeginTag(dma::cnt, 0);
	gs::BeginPrim(ABS,0,0);

	gs::Reg1(gs::TEXFLUSH,	0);

	gs::Reg1(gs::COLCLAMP,	PackCOLCLAMP(1));
	gs::Reg1(gs::TEST_1,	PackTEST(0,0,0,0,0,0,1,ZALWAYS));
	gs::Reg1(gs::TEX0_1,	PackTEX0(0x3740,HRES/64,PSMCT16S,10,10,1,DECAL,0,0,0,0,0));
	gs::Reg1(gs::TEX1_1,	PackTEX1(1,0,0,0,0,0,0));
	gs::Reg1(gs::ALPHA_1,	PackALPHA(2,1,0,1,0));
	gs::Reg1(gs::TEXA,		PackTEXA(ShadowDensity, 1, ShadowDensity));
	gs::Reg1(gs::ZBUF_1,	PackZBUF(ZBUFFER_START,PSMZ24,1));

	gs::Reg1(gs::PRIM,		PackPRIM(SPRITE,0,1,0,1,0,1,0,0));
	gs::Reg1(gs::UV,		PackUV(0x0008,0x0008));
	gs::Reg1(gs::XYZ2,		PackXYZ(XOFFSET,YOFFSET,0));
	gs::Reg1(gs::UV,		PackUV((HRES<<4)+8, (VRES<<4)+8));
	gs::Reg1(gs::XYZ2,		PackXYZ(XOFFSET+(HRES<<4), YOFFSET+(VRES<<4), 0));//0x10000-XOFFSET,0x10000-YOFFSET,0));

	gs::Reg1(gs::TEXA,		PackTEXA(0x00,0,0x80));
	gs::Reg1(gs::ZBUF_1,	PackZBUF(ZBUFFER_START,PSMZ24,0));

	gs::EndPrim(1);
	vif::MSCAL(VU1_ADDR(Parser));
	dma::EndTag();
	#endif



	// assume no dma context
	dma::SetList(NULL);

}

void patch_texture_dma()
{
	Nx::CScene *p_nxscene =  Nx::CEngine::sGetMainScene();
	if (p_nxscene)
	{
		Nx::CPs2TexDict *p_tex_dict = (Nx::CPs2TexDict *)p_nxscene->GetTexDict();
		if (p_tex_dict)
		{
			sScene *p_scene = p_tex_dict->GetEngineTextureDictionary();
		
		
			sTexture *p_textures = p_scene->pTextures;
			int num_textures = p_scene->NumTextures;
		
			
		//	int total_referenced = 0;
		//	int num_unique = 0;
			int i;
			for (i=0;i<num_textures;i++)
			{
				
				uint32* p_dma = (uint32*)p_textures[i].mp_dma;
				
		//			gif::Tag2(0, 0, IMAGE, 0, 0, 0, NumTexQWords[j]);  			// 0..3 words, NumTexQWords is in the lower 15 bits of the first word [0]
		//			dma::EndTag();									   			// Just Aligns upo to QW boundry
		//			dma::Tag(dma::ref, NumTexQWords[j], (uint)pTextureSource);  // 0..1 (after alignment) NumTexQWords goes in the lower 28 bits of word 0
		
				if (p_textures[i].m_render_count)
				{
					// Reset it for next time around	
					p_textures[i].m_render_count = 0;
					// fix it, so it gets uploaded
					// This test does speed up the routine by about 20%
					// Generally the visibility does not change
					uint32	current_value = (p_dma[0]);
					if (current_value == 0)
					{
						p_dma[0] = p_textures[i].m_quad_words;; 
						uint32* p_dma2 = (uint32*)((((int)(p_dma+4)) + 3)&0xfffffff0);
						p_dma2[0] = (dma::ref << 28) | p_textures[i].m_quad_words; 
					}
				}
				else
				{
					// patch it, so nothing gets uploaded
					uint32	current_value = (p_dma[0]);
					if (current_value != 0)
					{
						p_dma[0] =  0;
						uint32* p_dma2 = (uint32*)((((int)(p_dma+4)) + 3)&0xfffffff0);
						p_dma2[0] = (dma::ref << 28);
					}
				}
				
			}
		}
	}
}


void render::NewStyle(bool just_setup)
{

	sGroup *p_group;

	// add viewport-specific GS registers to each new-style group
	for (p_group=sGroup::pHead; p_group!=sGroup::pEpilogue; p_group=p_group->pNext)
	{
		if ((p_group!=sGroup::pShadow) && p_group->pScene && (p_group->pScene->Flags & SCENEFLAG_USESPIP))
		{
			// send the GS viewport context
			dma::SetList(p_group);

			// mark our place if it's a (the?) sorted group
			// remember to assert there's no overflow
			if (p_group->flags & GROUPFLAG_SORT)
			{
				//printf("Marking location %08X, p_group==%08X, flags=%08X\n", (int)dma::pLoc, (int)p_group, p_group->flags);
				Dbg_MsgAssert(sMarkerIndex < MAX_SORTED_LIST_MARKERS, ("too many sorted list markers - tell Mike"));
				render::sSortedListMarker[render::sMarkerIndex++] = (int)dma::pLoc;
			}

			dma::BeginTag(dma::cnt, 0xFE000000);
			vif::BASE(vu1::Loc);
			vif::OFFSET(0);
			uint vu1_loc = vu1::Loc;
			vu1::Loc = 0;   							// must do this as a relative prim for a sortable list...

			// constant part of vu1 context data
			vu1::BeginPrim(REL, VU1_ADDR(L_VF09));
			vu1::StoreVec(*(Vec *)&render::AltFrustum);				// VF09
			if (p_group->flags & GROUPFLAG_SKY)
			{
				vu1::StoreVec(*(Vec *)&render::SkyInverseViewportScale);	// VF10
				vu1::StoreVec(*(Vec *)&render::SkyInverseViewportOffset);	// VF11
			}
			else
			{
				vu1::StoreVec(*(Vec *)&render::InverseViewportScale);	// VF10
				vu1::StoreVec(*(Vec *)&render::InverseViewportOffset);	// VF11
			}
			vu1::EndPrim(0);

			gs::BeginPrim(REL, 0, 0);
			gs::Reg1(gs::XYOFFSET_1, render::reg_XYOFFSET);
			gs::Reg1(gs::SCISSOR_1,  render::reg_SCISSOR);
			gs::EndPrim(1);
			vif::MSCAL(VU1_ADDR(Parser));
			dma::EndTag();
			((uint16 *)dma::pTag)[1] |= vu1::Loc & 0x3FF;	// must write some code for doing this automatically
			vu1::Loc += vu1_loc;

			if (p_group->flags & GROUPFLAG_SORT)
			{
				dma::Tag(dma::cnt,0,0);
				vif::NOP();
				vif::NOP();
			}

			// assume no vu1 context for each group
			p_group->pVu1Context = NULL;
		}

	}

	// reserve space from dma list for new trans context
	dma::SetList(NULL);
	//CVu1Context *p_ctxt = new(dma::pLoc) CVu1Context;
	CVu1Context *p_ctxt = (CVu1Context *)dma::pLoc;
	//dma::pLoc += sizeof(CVu1Context);
	dma::pLoc += (STD_CTXT_SIZE+7)*16;
	p_ctxt->Init();
	Mth::Matrix I;
	I.Ident();
	p_ctxt->StandardSetup(I);

	// shadow stuff
	sGroup::pShadow->Used[render::Field] = true;

	#if !STENCIL_SHADOW

	// while the dma context is null, set up an empty upload for the shadow group
	sGroup::pShadow->pUpload[render::Field] = dma::pLoc;
	dma::Tag(dma::end, 0, 0);
	vif::NOP();
	vif::NOP();
	
	// ...and also a vu1 context for the shadow group
	CVu1Context *p_shdw_ctxt = new(dma::pLoc) CVu1Context(*p_ctxt);
	dma::pLoc += sizeof(CVu1Context);
	p_shdw_ctxt->InitExtended();
	p_shdw_ctxt->SetShadowVecs(SkaterY);

	// add a z-push
	p_shdw_ctxt->AddZPush(16.0f);


	dma::SetList(sGroup::pShadow);

	// initial cnt tag, for sending row reg and z-sort key
	Mth::Vector *p_trans = p_shdw_ctxt->GetTranslation();
	dma::Tag(dma::cnt, 1, 0);
	vif::NOP();
	vif::STROW((int)(*p_trans)[0], (int)(*p_trans)[1], (int)(*p_trans)[2], 0);

	// send transform context
	dma::Tag(dma::ref, (EXT_CTXT_SIZE+2)|(EXT_CTXT_SIZE+1)<<16, p_shdw_ctxt->GetDma());
	vif::BASE(vu1::Loc);
	vif::OFFSET(0);
	vu1::Loc += EXT_CTXT_SIZE+1;
	sGroup::pShadow->pVu1Context = p_shdw_ctxt;

	// send the GS context for the shadow list
	dma::BeginTag(dma::cnt, 0);

	vu1::BeginPrim(ABS, VU1_ADDR(L_VF10));
	vu1::StoreVec(*(Vec *)&render::InverseViewportScale);	// VF10
	vu1::StoreVec(*(Vec *)&render::InverseViewportOffset);	// VF11
	vu1::EndPrim(0);

	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::TEXFLUSH,		PackTEXFLUSH(0));
	gs::Reg1(gs::FBA_2,			PackFBA(0));
	gs::Reg1(gs::TEX0_2,		PackTEX0(0x3E00,4,PSMCT16,8,8,1,DECAL,0,0,0,0,0));
	gs::Reg1(gs::TEX1_2,		PackTEX1(1,0,NEAREST,NEAREST,0,0,0));
	gs::Reg1(gs::CLAMP_2,		PackCLAMP(CLAMP,CLAMP,0,0,0,0));
	gs::Reg1(gs::FRAME_2,		PackFRAME(FRAME_START,HRES/64,PSMCT32,0xFF000000));
	gs::Reg1(gs::ZBUF_2,		PackZBUF(ZBUFFER_START,PSMZ24,1));
	gs::Reg1(gs::XYOFFSET_2,	reg_XYOFFSET);
	gs::Reg1(gs::SCISSOR_2,		reg_SCISSOR);
	gs::Reg1(gs::ALPHA_2,		PackALPHA(2,1,2,1,ShadowDensity));
	gs::Reg1(gs::TEST_2,		PackTEST(1,AGEQUAL,0x80,KEEP,0,0,1,ZGEQUAL));
	gs::Reg1(gs::PRMODECONT,	PackPRMODECONT(0));
	gs::Reg1(gs::PRMODE,		PackPRMODE(0,1,0,1,0,0,1,0));
	gs::EndPrim(0);
	dma::EndTag();
	#endif


	geom_stats_total=0;
	geom_stats_inactive =0;
	geom_stats_sky=0;
	geom_stats_transformed=0;
	geom_stats_skeletal=0;
	geom_stats_camera_sphere=0;
	geom_stats_clipcull=0;
	geom_stats_culled=0;
	geom_stats_leaf_culled=0;
	geom_stats_clipcull=0;
	geom_stats_boxcheck=0;
	geom_stats_occludecheck=0;
	geom_stats_occluded=0;
	geom_stats_colored=0;
	geom_stats_leaf=0;
	geom_stats_wibbleUV=0;
	geom_stats_wibbleVC=0;
	geom_stats_sendcontext=0;
	geom_stats_sorted=0;
	geom_stats_shadow=0;

	// render the world
	if (!just_setup)
	{
		
		#ifdef __NOPT_ASSERT__
		const int span = 60;
		static int avg[span];
		static int slot = 0;
		Tmr::CPUCycles	start = Tmr::GetTimeInCPUCycles();
		#endif

		#if STENCIL_SHADOW
		CGeomNode::sWorld.RenderWorld(*p_ctxt, RENDERFLAG_CLIP);
		#else
		CGeomNode::sWorld.RenderWorld(*p_ctxt, RENDERFLAG_CLIP | RENDERFLAG_SHADOW);
		#endif
		
		
		#ifdef __NOPT_ASSERT__		
		Tmr::CPUCycles len = Tmr::GetTimeInCPUCycles() - start;
		int ticks = len;
		avg[slot++] = ticks;
		if (slot == span) slot = 0;
		int tot = 0;
		for (int a=0;a<span;a++)
		{
			tot += avg[a];
		}
		tot/=span;
//		printf ("%9d %9d\n",tot,ticks);	// printf average, and the last frame
		#endif
	}

#if 0
		printf ("\ngeom_stats_total = %d\n",          geom_stats_total);         
		printf ("geom_stats_inactive  = %d\n",      geom_stats_inactive );     
		printf ("geom_stats_sky = %d\n",            geom_stats_sky);           
		printf ("geom_stats_transformed = %d\n",    geom_stats_transformed);   
		printf ("geom_stats_skeletal = %d\n",       geom_stats_skeletal);      
		printf ("geom_stats_camera_sphere = %d\n",  geom_stats_camera_sphere); 
		printf ("geom_stats_clipcull = %d\n",       geom_stats_clipcull);      
		printf ("geom_stats_boxcheck = %d\n",       geom_stats_boxcheck);      
		printf ("geom_stats_occludecheck = %d\n",   geom_stats_occludecheck);  
		printf ("geom_stats_occluded = %d\n",       geom_stats_occluded);      
		printf ("geom_stats_colored = %d\n",        geom_stats_colored);       
		printf ("geom_stats_leaf = %d\n",           geom_stats_leaf);          
		printf ("geom_stats_wibbleUV = %d\n",       geom_stats_wibbleUV);      
		printf ("geom_stats_wibbleVC = %d\n",       geom_stats_wibbleVC);      
		printf ("geom_stats_sendcontext = %d\n",    geom_stats_sendcontext);   
		printf ("geom_stats_sorted = %d\n",         geom_stats_sorted);        
		printf ("geom_stats_shadow = %d\n",         geom_stats_shadow);        
#endif	


	#if !STENCIL_SHADOW
	// undo the havoc caused by the shadow group
	dma::SetList(sGroup::pShadow);
	dma::BeginTag(dma::cnt, 0);
	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::PRMODECONT,	PackPRMODECONT(1));
	gs::EndPrim(1);
	vif::MSCAL(VU1_ADDR(Parser));
	dma::EndTag();
	#endif
}





#if !STENCIL_SHADOW
void SetTextureProjectionCamera( Mth::Vector *p_pos, Mth::Vector *p_at )
{
//	sTextureProjectionDetails *p_details = pTextureProjectionDetailsTable->GetItem((uint32)p_texture );
//	if( p_details )
	{
		render::ShadowCameraPosition[0] = p_pos->GetX();
		render::ShadowCameraPosition[1] = p_pos->GetY();
		render::ShadowCameraPosition[2] = p_pos->GetZ();
	}
}
#endif







// Added by Mick
// quick determination of if something is visible or not
// uses the previously calculated s and c vectors
// and the WorldToCamera transform
// (note, no attempt is made to ensure this is the same camera that the object
// will eventually be rendered with)
bool	IsVisible(Mth::Vector &center, float radius)
{


	float x,y,z;
	Mth::Vector v(center);

	v[3] = 1.0f;		// needed?
	v *= render::WorldToCamera;
	x = v[0];
	y = v[1];
	z = v[2];

	// if all outside view volume then cull
	if (radius<z-render::Near || radius<render::Far-z							||
		radius<render::sy*z+render::cy*y || radius<render::sy*z-render::cy*y	||
		radius<render::sx*z-render::cx*x || radius<render::sx*z+render::cx*x)
	{
		return false;
	}
	else
	{
//		return true;
	
		if( TestSphereAgainstOccluders( &center, radius ))
		{
			// Occluded.
			return false;
		}
		else
		{
			return true;
		}
	}


}


// The same function, but without occlusion
bool	IsInFrame(Mth::Vector &center, float radius)
{

	float x,y,z;
	Mth::Vector v(center);

	v[3] = 1.0f;			// needed?
	v *= render::WorldToCamera;
	x = v[0];
	y = v[1];
	z = v[2];

	// if all outside view volume then cull
	if (radius<z-render::Near || radius<render::Far-z							||
		radius<render::sy*z+render::cy*y || radius<render::sy*z-render::cy*y	||
		radius<render::sx*z-render::cx*x || radius<render::sx*z+render::cx*x)
	{
		return false;
	}
	else
	{
		return true;
	}
}




void RenderMesh(sMesh *pMesh, uint RenderFlags)
{
	dma::BeginTag(dma::call,(uint)pMesh->pSubroutine);
	vif::BASE(vu1::Loc);
	vif::OFFSET(0);
	vif::ITOP(RenderFlags);
	dma::EndTag();
	vu1::Loc += ((uint16 *)(pMesh->pSubroutine))[1];
}


void RenderEpilogue(void)
{
	sGroup *p_group;

	// ---------------------------------------------
	// tag onto end of particle group
	dma::SetList(sGroup::pParticles);

	// restore vu1 code (and buffering scheme) at end of particles group
	dma::Tag(dma::ref, ((uint8 *)&MPGEnd-(uint8 *)&MPGStart+15)/16, (uint)&MPGStart);
	vif::NOP();
	vif::NOP();
	dma::BeginTag(dma::cnt, 0);
	vif::FLUSH();
	vif::STMASK(0);
	vif::STMOD(0);
	vif::STCYCL(1,1);
	vif::BASE(0);
	vif::OFFSET(0);
	vif::MSCAL(VU1_ADDR(Setup));
	vu1::Loc = vu1::Buffer = 0;
	dma::EndTag();

	dma::SetList(NULL);
	// ---------------------------------------------
	
	// add a GS SIGNAL primitive to each group
	for (p_group=sGroup::pHead; p_group; p_group=p_group->pNext)
	{
		// end the dma list
		dma::EndList(p_group);
	}

	dma::SetList(NULL);

	// epilogue VRAM upload (fonts and sprites)

	// Upload the fonts, so we can draw 2D text on screen	
	sGroup::pEpilogue->pUpload[render::Field] = dma::pLoc;
	uint32 *p = (uint32*)(dma::pLoc);
	for (SFont *pFont=pFontList; pFont; pFont=pFont->pNext)
	{
	   	// Mick: we build this upload list directly, to cut down on the number of writes
		*p++ = (dma::ref << 28) + ((pFont->VifSize+15)>>4); //		dma::Tag(dma::ref, (pFont->VifSize+15)>>4, (uint)pFont->pVifData);
		*p++ = (uint32) pFont->pVifData;
		*p++ = 0;    //		vif::NOP();
		*p++ = 0;    //		vif::NOP();
	}

	// Upload the "Single Textures", the 2D screen elements (sprites)
	for (SSingleTexture *pTexture=SSingleTexture::sp_stexture_list; pTexture; pTexture=pTexture->mp_next)
	{
		if (pTexture->IsInVRAM())
		{
			*p++ = (dma::call << 28) + (0); //	dma::Tag(dma::call, 0, (uint)pTexture->mp_VifData);
			*p++ = (uint32) pTexture->mp_VifData;
			*p++ = 0;    //		vif::NOP();
			*p++ = 0;    //		vif::NOP();
		}
	}
	dma::pLoc = (uint8*)p;

	
	// (?) writing to TEXFLUSH will stall the gs until all the fonts and sprites
	// have been uploaded.
	// since we are not double buffering anything now, we have to wait until
	// everything is in VRAM before we can start rendering any 2D
	// this is a potential GS hiccup, but removing it would be tricky
	// and possibly not worth the effort
	// Garrett: Now the 2D uses both buffers at once.
		
	dma::BeginTag(dma::end, 0);
	vif::NOP();
	vif::NOP();
	gif::Tag2(gs::A_D,1,PACKED,0,0,1,1);
	gs::Reg2(gs::TEXFLUSH,	0);
	dma::EndTag();


	// We need the scratchpad in SortGroup()
	bool got_scratch = CSystemResources::sRequestResource(CSystemResources::vSCRATCHPAD_MEMORY);
	Dbg_Assert(got_scratch);

	// sort the dma lists of transparent groups
	for (p_group=sGroup::pHead; p_group!=sGroup::pEpilogue; p_group=p_group->pNext)
	{
		if ((p_group->flags & GROUPFLAG_SORT) 	   // Mick, most groups are not SORT, so this test first is the fastest
			&& p_group!=sGroup::pShadow
			&& p_group->pScene 
			&& (p_group->pScene->Flags & SCENEFLAG_USESPIP)
			)
		{
			p_group->pRender[render::Field] = dma::SortGroup(p_group->pRender[render::Field]);
		}
	}

	if (got_scratch)
	{
		CSystemResources::sFreeResource(CSystemResources::vSCRATCHPAD_MEMORY);
	}

	// swap epilogue and particle texture upload pointers (a cheat!)
	uint8 *temp_ptr = sGroup::pParticles->pUpload[render::Field];
	sGroup::pParticles->pUpload[render::Field] = sGroup::pEpilogue->pUpload[render::Field];
	sGroup::pEpilogue->pUpload[render::Field] = temp_ptr;
	
	int size = (int)dma::pLoc - (int)dma::pBase;
	static int max = 0;
	static int max2 = 0;
	if (size > max) max = size;
	if (size > max2) max2 = size;
	static int tick = 0;
	tick++; 
	tick &= 0x1f;
	if (!tick)
	{
	  //  printf ("%6d, %6d, %6d (of %6d)\n",size,max2,max, dma::size);
		max2 -= 50000;
	}

	Dbg_MsgAssert(DMAOverflowOK || size < (dma::size*98/100), ("DMA Buffer Overflow used %d\nShow screenshot to Mick (see .bmp saved, above)",size)); 
	
	if (DMAOverflowOK)
	{
		DMAOverflowOK--;
	}
	
}


void SendDisplayList(void)
{

// Mick:  Mike added this just before he left, but did not check it in
//	sceGsSyncPath(0, 0);

	// patch up the DMA list
// Moved to StuffAfterGSFinished
//	patch_texture_dma();	
	
	// Writeback data from D-Cache to Memory
	FlushCache(WRITEBACK_DCACHE);

#	ifdef __USE_PROFILER__
// Initial render group does not cause an interrupt
// so don't push context
//			Sys::VUProfiler->PushContext( 80,80, 228 );	   									
#	endif // __USE_PROFILER__
	// kick prologue render
	*D1_QWC  = 0;						// must zero QWC because the first action will be to use current MADR & QWC
	*D1_TADR = (uint)PrologueGroup.pRender[!render::Field];	// address of 1st tag
	*D1_CHCR = 0x145;					// start transfer, tte=1, chain mode, from memory
	sceGsSyncPath(0, 0);

	#if USE_INTERRUPTS
	
	// use interrupts
	sGroup::pUploadGroup = sGroup::pHead;
	sGroup::pRenderGroup = sGroup::pUploadGroup;
	

//	SingleRender = 0;

	#if 0  // debugging code to force all gorups to be executed, even if empty
	sGroup * p_group = (sGroup*)sGroup::pUploadGroup; // Skip prologue and sky
	while (p_group->pNext)		// all except the last one
	{
		p_group->Used[!render::Field] = 1;
		p_group = p_group->pNext;
	}
	#endif


	// Some debugging code to only render/upload a single selectable group
	// so we can see what is taking all the time
	if (SingleRender)
	{
		sGroup * p_group = (sGroup*)sGroup::pUploadGroup; // Skip prologue and sky
		int g = 0;
		while (p_group->pNext)		// all except the last one
		{
			g++;
			if (g > 2 && g != SingleRender)
			{
					p_group->Used[!render::Field] = 0;			
			}
			p_group = p_group->pNext;
		}
	}
	
	UploadStalled  = 0;
	RenderStalled  = 1;
	FlushCache(WRITEBACK_DCACHE);

#	ifdef __USE_PROFILER__
	Sys::DMAProfiler->PushContext(  sGroup::pUploadGroup->profile_color );	   									
#	endif // __USE_PROFILER__


	// kick off first upload, should trigger all the rest by interrupt
	*D2_QWC = 0;						// must zero QWC because the first action will be to use current MADR & QWC
	*D2_TADR = (uint)sGroup::pUploadGroup->pUpload[!render::Field];	// address of 1st tag
	*D2_CHCR = 0x105;					// start transfer, tte=0, chain mode, from memory



	#else
	
	// iterate over groups, uploading textures then rendering meshes in each
	// last group will be epilogue group
	for (sGroup *p_group=sGroup::pHead; p_group!=sGroup::pEpilogue; p_group=p_group->pNext)
	{
		if (p_group->Used[!render::Field])
		{
			// kick dma to upload textures from this group
			*D2_QWC = 0;						// must zero QWC because the first action will be to use current MADR & QWC
			*D2_TADR = (uint)p_group->pUpload[!render::Field];	// address of 1st tag
			*D2_CHCR = 0x105;					// start transfer, tte=0, chain mode, from memory
			sceGsSyncPath(0, 0);

			// kick dma to render scene
			*D1_QWC = 0;						// must zero QWC because the first action will be to use current MADR & QWC
			*D1_TADR = (uint)p_group->pRender[!render::Field];	// address of 1st tag
			*D1_CHCR = 0x145;					// start transfer, tte=1, chain mode, from memory
			sceGsSyncPath(0, 0);
		}
	}

	// kick dma to upload textures from this group
	*D2_QWC = 0;						// must zero QWC because the first action will be to use current MADR & QWC
	*D2_TADR = (uint)sGroup::pEpilogue->pUpload[!render::Field];	// address of 1st tag
	*D2_CHCR = 0x105;					// start transfer, tte=0, chain mode, from memory
	sceGsSyncPath(0, 0);

	// kick dma to render scene
	*D1_QWC = 0;						// must zero QWC because the first action will be to use current MADR & QWC
	*D1_TADR = (uint)sGroup::pEpilogue->pRender[!render::Field];	// address of 1st tag
	*D1_CHCR = 0x145;					// start transfer, tte=1, chain mode, from memory
	sceGsSyncPath(0, 0);
	#endif
}


/////////////////////////////////////////////////////////////////////////////
//
static bool sUpdate2DDma = false;
static bool	sVRAMNeedsUpdating = false;


void	Clear2DVRAM()
{
	// Reset VRAM pointer
	FontVramBase = FontVramStart;

	// And set flag
	sUpdate2DDma = true;
}

void	VRAMNeedsUpdating()
{
	sVRAMNeedsUpdating = true;
}

void	Reallocate2DVRAM()
{
	if (!sVRAMNeedsUpdating)
	{
		return;
	}

	// Reset VRAM pointer
	Clear2DVRAM();

	for (SFont *pFont=pFontList; pFont; pFont=pFont->pNext)
	{
		pFont->ReallocateVRAM();
	}

	for (SSingleTexture *pTexture=SSingleTexture::sp_stexture_list; pTexture; pTexture=pTexture->mp_next)
	{
		pTexture->ReallocateVRAM();
	}

	// And set flags
	sUpdate2DDma = true;
	sVRAMNeedsUpdating = false;
}

/////////////////////////////////////////////////////////////////////////////
//

void	Update2DDMA()
{
	// Only go through the lists if it was requested
	if (!sUpdate2DDma)
		return;

	// Go through each item
	for (SFont *pFont=pFontList; pFont; pFont=pFont->pNext)
	{
		pFont->UpdateDMA();
	}

	for (SSingleTexture *pTexture=SSingleTexture::sp_stexture_list; pTexture; pTexture=pTexture->mp_next)
	{
		pTexture->UpdateDMA();
	}

	// Clear flag
	sUpdate2DDma = false;
}






#if STENCIL_SHADOW

void CastStencilShadow(CInstance *pInstance, sGroup *p_group, Mth::Vector &ShadowVec, Mth::Vector &TweakVec)
{
	sShadowVolumeHeader * p_shadow = pInstance->GetScene()->mp_shadow_volume_header;
	if (!p_shadow)
		return;

	Mth::Matrix RootToFrustum,RootToViewport, WorldToRoot;

	RootToFrustum = *(Mth::Matrix *)pInstance->GetTransform() * render::WorldToFrustum;
	RootToViewport = RootToFrustum * render::FrustumToIntViewport;

	WorldToRoot = *(Mth::Matrix *)pInstance->GetTransform();
	WorldToRoot.Transpose();
	ShadowVec *= WorldToRoot;
	TweakVec *= WorldToRoot;

	dma::SetList(sGroup::pShadow);

	dma::BeginTag(dma::cnt, 0);
	vu1::BeginPrim(ABS, VU1_ADDR(L_VF14));
	vu1::StoreVec(*(Vec *)&ShadowVec);			// VF14, shadow vec
	vu1::StoreVec(*(Vec *)&TweakVec);			// VF15, tweak vec
	vu1::StoreMat(*(Mat *)&RootToViewport);		// VF16-19
	vu1::EndPrim(1);
	vif::MSCAL(VU1_ADDR(Parser));
	vif::FLUSH();
	dma::EndTag();

	// send transforms
	vu1::Loc = 0;
	dma::Tag(dma::ref, pInstance->GetNumBones()<<2, (uint)pInstance->GetBoneTransforms());
	vif::STCYCL(1,1);
	vif::UNPACK(0,V4_32,pInstance->GetNumBones()<<2,ABS,SIGNED,0);

	// send data
	dma::BeginTag(dma::cnt, 0);
	vif::STCYCL(7,21);

	sShadowVertex * p_vertex = p_shadow->p_vertex;
	sShadowConnect * p_connect = p_shadow->p_connect;
	sShadowNeighbor * p_neighbor = p_shadow->p_neighbor;

	int vert, num_faces;
	for (uint32 i=0; i<p_shadow->num_faces; i++)
	{
		if (i%36==0)
		{
			vu1::Loc = 256;
			vif::BASE(256);
			vif::OFFSET(0);
			vif::MSCAL(VU1_ADDR(Setup));
		}

		if (i%12==0)
		{
			num_faces = p_shadow->num_faces-i;
			if (num_faces>12)
				num_faces=12;


			vif::UNPACK(0, V4_32, 7*num_faces, ABS, UNSIGNED, 0);
		}

		if ((i%12==11) || (i==p_shadow->num_faces-1))
		{
			vif::StoreV4_32(0x3480800A, 0x21224000, 0x00000051, 0x8000);
		}
		else
		{
			vif::StoreV4_32(0x3480800A, 0x21224000, 0x00000051, 0x0000);
		}

		uint32 * p32 = (uint32 *)dma::pLoc;

		vert = p_connect[i].corner[0];
		*p32++ = *((uint32 *)(&p_vertex[vert].x));
		*p32++ = *((uint32 *)(&p_vertex[vert].y));
		*p32++ = *((uint32 *)(&p_vertex[vert].z));
		*p32++ = *((uint32 *)(&p_vertex[vert].idx));

		vert = p_connect[i].corner[1];
		*p32++ = *((uint32 *)(&p_vertex[vert].x));
		*p32++ = *((uint32 *)(&p_vertex[vert].y));
		*p32++ = *((uint32 *)(&p_vertex[vert].z));
		*p32++ = *((uint32 *)(&p_vertex[vert].idx));

		vert = p_connect[i].corner[2];
		*p32++ = *((uint32 *)(&p_vertex[vert].x));
		*p32++ = *((uint32 *)(&p_vertex[vert].y));
		*p32++ = *((uint32 *)(&p_vertex[vert].z));
		*p32++ = *((uint32 *)(&p_vertex[vert].idx));

		vert = p_neighbor[i].edge[0];
		*p32++ = *((uint32 *)(&p_vertex[vert].x));
		*p32++ = *((uint32 *)(&p_vertex[vert].y));
		*p32++ = *((uint32 *)(&p_vertex[vert].z));
		*p32++ = *((uint32 *)(&p_vertex[vert].idx));

		vert = p_neighbor[i].edge[1];
		*p32++ = *((uint32 *)(&p_vertex[vert].x));
		*p32++ = *((uint32 *)(&p_vertex[vert].y));
		*p32++ = *((uint32 *)(&p_vertex[vert].z));
		*p32++ = *((uint32 *)(&p_vertex[vert].idx));

		vert = p_neighbor[i].edge[2];
		*p32++ = *((uint32 *)(&p_vertex[vert].x));
		*p32++ = *((uint32 *)(&p_vertex[vert].y));
		*p32++ = *((uint32 *)(&p_vertex[vert].z));
		*p32++ = *((uint32 *)(&p_vertex[vert].idx));

		dma::pLoc = (uint8 *)p32;

		vu1::Loc += 21;

		if ((i%12 == 11) || (i==p_shadow->num_faces-1))
		{
			vif::MSCAL(VU1_ADDR(ShadowVolumeSkin));
		}
	}

	dma::EndTag();

	dma::SetList(p_group);
}

#else

void CastShadow(CInstance *pInstance, sGroup *p_group)
{
	int j;
	sMesh *pMesh;
	Mth::Matrix ProjMat, temp;

	// set up projection matrix
	ProjMat = *pInstance->GetTransform();
	SkaterY = ProjMat[3][1];
	ProjMat[3].Set(-36.0f/**SUB_INCH_PRECISION*/*ProjMat[1][0],
				   -36.0f/**SUB_INCH_PRECISION*/*ProjMat[1][1],
				   -36.0f/**SUB_INCH_PRECISION*/*ProjMat[1][2], 1.0f);
	temp[0].Set(32.0f/**RECIPROCAL_SUB_INCH_PRECISION*/, 0.0f, 0.0f, 0.0f);
	temp[1].Set(0.0f, 0.0f, 0.0f, 0.0f);
	temp[2].Set(0.0f, 32.0f/**RECIPROCAL_SUB_INCH_PRECISION*/, 0.0f, 0.0f);
	temp[3].Set(8421376.0f, 8421376.0f, 0.0f, 1.0f);
	ProjMat *= temp;

	dma::SetList(sGroup::pShadow);
	dma::BeginTag(dma::cnt, 0);
	vu1::BeginPrim(ABS, VU1_ADDR(L_VF12));
	vu1::StoreMat(*(Mat *)&ProjMat);		// VF16-19
	vu1::EndPrim(1);

	vif::MSCAL(VU1_ADDR(Parser));

	// not using pre-translate
	// don't forget to switch off the offset mode on xyz unpack if using the row reg for somthing else
	vif::STROW(0,0,0,0);

	// get free run of the VUMem
	vif::FLUSH();

	dma::EndTag();

	// send transforms
	vu1::Loc = 0;
	dma::Tag(dma::ref, pInstance->GetNumBones()<<2, (uint)pInstance->GetBoneTransforms());
	vif::NOP();
	vif::UNPACK(0,V4_32,pInstance->GetNumBones()<<2,ABS,SIGNED,0);

	dma::Tag(dma::cnt, 0, 0);
	vif::ITOP(pInstance->GetNumBones()<<2);
	vif::MSCAL(VU1_ADDR(ReformatXforms));

	// traverse array of meshes in the model
	for (j=0,pMesh=p_group->pMeshes; j<p_group->NumMeshes; j++,pMesh++)
	{
		// skip if invisible
		if (!pMesh->IsActive())
		{
			continue;
		}

		// do nothing if mesh doesn't have a dma subroutine
		if (!pMesh->pSubroutine)
		{
			continue;
		}

		// render the mesh
		vu1::Loc = vu1::Buffer = 256;
		dma::BeginTag(dma::call,(uint)pMesh->pSubroutine);
		vif::BASE(256);
		vif::OFFSET(0);
		vif::MSCAL(VU1_ADDR(Setup));
		vif::ITOP(SHDW);
		vif::FLUSH();
		dma::EndTag();
		vu1::Loc += ((uint16 *)pMesh->pSubroutine)[1];
	}

	dma::Tag(dma::cnt, 0, 0);
	vif::FLUSH();
	vif::NOP();

	dma::SetList(p_group);
}

#endif




namespace render
{
	Mth::Matrix WorldToCamera;
	Mth::Matrix WorldToCameraRotation;		// shouldn't need this
	Mth::Matrix WorldToFrustum;
	Mth::Matrix WorldToViewport;
	Mth::Matrix WorldToIntViewport;
	Mth::Matrix CameraToWorld;
	Mth::Matrix CameraToWorldRotation;
	Mth::Matrix CameraToFrustum;
	Mth::Matrix CameraToViewport;
	Mth::Matrix FrustumToViewport;
	Mth::Matrix FrustumToIntViewport;
	Mth::Matrix AdjustedWorldToCamera;
	Mth::Matrix AdjustedWorldToFrustum;
	Mth::Matrix AdjustedWorldToViewport;
	Mth::Matrix AdjustedWorldToIntViewport;
	Mth::Matrix SkyWorldToCamera;
	Mth::Matrix SkyWorldToFrustum;
	Mth::Matrix SkyWorldToViewport;
	Mth::Matrix SkyFrustumToViewport;

	Mth::Vector ViewportScale;
	Mth::Vector ViewportOffset;
	Mth::Vector IntViewportScale;
	Mth::Vector IntViewportOffset;
	Mth::Vector InverseViewportScale;
	Mth::Vector InverseViewportOffset;
	Mth::Vector InverseIntViewportScale;
	Mth::Vector InverseIntViewportOffset;
	Mth::Vector SkyViewportScale;
	Mth::Vector SkyViewportOffset;
	Mth::Vector SkyInverseViewportScale;
	Mth::Vector SkyInverseViewportOffset;
	Mth::Vector RowReg;
	Mth::Vector RowRegI;
	Mth::Vector RowRegF;
	#if !STENCIL_SHADOW
	Mth::Vector ShadowCameraPosition;
	#endif
	Mth::Vector AltFrustum;
	Mth::Vector CameraPosition;

	uint64 reg_XYOFFSET;
	uint64 reg_SCISSOR;

	float sx,sy,cx,cy,tx,ty,Sx,Sy,Cx,Cy,Tx,Ty,Near,Far;
	//float sMultipassMaxDist = 1000000000.0f;
	float sMultipassMaxDist = 4000.0f;

	uint Frame, Field;
	uint ViewportMask;

	uint RenderVarFrame = 0xFFFFFFFF; 
	uint RenderVarViewport = 0xFFFFFFFF;

	bool FrustumFlagsPlus[3], FrustumFlagsMinus[3];

	int sSortedListMarker[MAX_SORTED_LIST_MARKERS];
	int sMarkerIndex;
	int sTime;
	int sTotalNewParticles;

	float FogNear=3000.0f, FogAlpha=0.80f;
	uint32 FogCol=0xFF4070;
	bool EnableFog=false;
	float EffectiveFogAlpha;

	uint32 *p_patch_FOGCOL;

	uint8 ShadowDensity = 48;
}



void render::SetupVars(uint viewport_num, const Mth::Matrix& camera_transform, const Mth::Rect& viewport_rec, float view_angle,
					   float viewport_aspect, float near, float far)
{

	// fog variables
	EffectiveFogAlpha = FogAlpha;
	if (FogAlpha < 0.003f)
	{
		EffectiveFogAlpha = 0.003f;
	}
	if (FogAlpha > 0.997f)
	{
		EffectiveFogAlpha = 0.997f;
	}
	if (!EnableFog)		// TT5158: ZPush is affected by FogAlpha, so having a very low number causes z-fighting.  Garrett
	{
		EffectiveFogAlpha = 0.75f;
	}
	*p_patch_FOGCOL = FogCol;


	// Check if we already set up the current variables
	if (VarsUpToDate(viewport_num))
	{
		return;
	}

	// camera transform
	CameraToWorld = camera_transform;

	Dbg_MsgAssert(CameraToWorld[0][3] == 0.0f, ("CameraToWorld[0][3] is %f, not 0.0", CameraToWorld[0][3]));
	Dbg_MsgAssert(CameraToWorld[1][3] == 0.0f, ("CameraToWorld[1][3] is %f, not 0.0", CameraToWorld[1][3]));
	Dbg_MsgAssert(CameraToWorld[2][3] == 0.0f, ("CameraToWorld[2][3] is %f, not 0.0", CameraToWorld[2][3]));
	Dbg_MsgAssert(CameraToWorld[3][3] == 1.0f, ("CameraToWorld[3][3] is %f, not 1.0", CameraToWorld[3][3]));

	// need the transform to be well-formed as a 4x4 matrix
	//CameraToWorld[0][3] = 0.0f;
	//CameraToWorld[1][3] = 0.0f;
	//CameraToWorld[2][3] = 0.0f;
	//CameraToWorld[3][3] = 1.0f;

	CameraToWorldRotation = CameraToWorld;
	CameraToWorldRotation[3].Set(0.0f, 0.0f, 0.0f, 1.0f);

	// world view transform
	WorldToCameraRotation.Transpose(CameraToWorldRotation);
	WorldToCamera = CameraToWorld;
	WorldToCamera.InvertUniform();

	// massage to get ROW register value
	RowReg = SUB_INCH_PRECISION * WorldToCamera[3] * CameraToWorldRotation;
	RowRegI[0] = (float)(int)RowReg[0];
	RowRegI[1] = (float)(int)RowReg[1];
	RowRegI[2] = (float)(int)RowReg[2];
	RowRegI[3] = 0.0f;
	RowRegF = RowReg - RowRegI;
	RowRegF[3] = 0.0f;

	// massage to get modified matrix
	AdjustedWorldToCamera = WorldToCamera;
	AdjustedWorldToCamera[3] = RowRegF * WorldToCameraRotation;
	AdjustedWorldToCamera[3] *= RECIPROCAL_SUB_INCH_PRECISION;
	AdjustedWorldToCamera[3][3] = 1.0f;

	// view frustum
	tx = tanf(view_angle*8.72664626e-03f);		// tan of half (angle in radians)
	ty = -(tx / viewport_aspect);
	sx	= 1.0f/sqrtf(1.0f+1.0f/(tx*tx));
	sy	= 1.0f/sqrtf(1.0f+1.0f/(ty*ty));
	cx	= 1.0f/sqrtf(1.0f+tx*tx);
	cy	= 1.0f/sqrtf(1.0f+ty*ty);
	Near = near;
	Far  = far;

	// outer frustum
	Tx = tx * (620.0f/(((float) HRES) * 0.5f * viewport_rec.GetWidth()));
	Ty = ty * (524.0f/(((float) VRES) * 0.5f * viewport_rec.GetHeight()));
	Sx	= 1.0f/sqrtf(1.0f+1.0f/(Tx*Tx));
	Sy	= 1.0f/sqrtf(1.0f+1.0f/(Ty*Ty));
	Cx	= 1.0f/sqrtf(1.0f+Tx*Tx);
	Cy	= 1.0f/sqrtf(1.0f+Ty*Ty);

	// frustum transform
	CameraToFrustum.Zero();
	CameraToFrustum[0][0] = 1.0f/Tx;
	CameraToFrustum[1][1] = 1.0f/Ty;
	CameraToFrustum[2][2] = (near+far) / (far-near);
	CameraToFrustum[3][2] = 2.0f * near * far / (near-far);
	CameraToFrustum[2][3] = -1.0f;
	AdjustedWorldToFrustum = AdjustedWorldToCamera * CameraToFrustum;
	WorldToFrustum = WorldToCamera * CameraToFrustum;

	// viewport (scale and offset from outer frustum)
	ViewportScale.Set( 620.0f, 524.0f, 0.999f*powf(2,22), EffectiveFogAlpha);
	ViewportOffset.Set(2048.0f, 2048.0f, 1.5f*powf(2,23), 0);
	IntViewportScale.Set(620.0f, 524.0f, 0.999f*powf(2,-95), powf(2,32));
	IntViewportOffset.Set(2048.0f+powf(2,19), 2048.0f+powf(2,19), 1.5f*powf(2,-94), powf(2,-32));

	// viewport transform
	FrustumToViewport.Zero();
	FrustumToViewport[0][0] = ViewportScale[0];
	FrustumToViewport[1][1] = ViewportScale[1];
	FrustumToViewport[2][2] = ViewportScale[2];
	FrustumToViewport[3][3] = ViewportScale[3];
	FrustumToViewport[3][0] = ViewportOffset[0];
	FrustumToViewport[3][1] = ViewportOffset[1];
	FrustumToViewport[3][2] = ViewportOffset[2];
	AdjustedWorldToViewport = AdjustedWorldToFrustum * FrustumToViewport;
	WorldToViewport = WorldToFrustum * FrustumToViewport;
	CameraToViewport = CameraToFrustum * FrustumToViewport;

	FrustumToIntViewport.Zero();
	FrustumToIntViewport[0][0] = IntViewportScale[0];
	FrustumToIntViewport[1][1] = IntViewportScale[1];
	FrustumToIntViewport[2][2] = IntViewportScale[2];
	FrustumToIntViewport[3][3] = IntViewportScale[3];
	FrustumToIntViewport[3][0] = IntViewportOffset[0];
	FrustumToIntViewport[3][1] = IntViewportOffset[1];
	FrustumToIntViewport[3][2] = IntViewportOffset[2];
	AdjustedWorldToIntViewport = AdjustedWorldToFrustum * FrustumToIntViewport;
	WorldToIntViewport = WorldToFrustum * FrustumToIntViewport;


	// sky transforms
	//SkyViewportScale.Set(1900.0f, 1900.0f, powf(2,-102), powf(2,32));		// convert these to constants
	//SkyViewportOffset.Set(2048.0f+powf(2,19), 2048.0f+powf(2,19), 1.5f*powf(2,-101), powf(2,-32));	// convert these to constants
	SkyViewportScale.Set( 620.0f, 524.0f, 0.01f, EffectiveFogAlpha);
	SkyViewportOffset.Set(2048.0f, 2048.0f, 1.0f, 0);

	SkyWorldToCamera = AdjustedWorldToCamera;
	SkyWorldToCamera[3][0] = 0;
	SkyWorldToCamera[3][1] = 0;
	SkyWorldToCamera[3][2] = 0;
	SkyWorldToFrustum = SkyWorldToCamera * CameraToFrustum;
	SkyFrustumToViewport = FrustumToViewport;
	SkyFrustumToViewport[2][2] = SkyViewportScale[2];
	SkyFrustumToViewport[3][2] = SkyViewportOffset[2];
	SkyWorldToViewport = SkyWorldToFrustum * SkyFrustumToViewport;

	// used in 3D clipping
	AltFrustum[0] = Near;
	AltFrustum[1] = Far;
	AltFrustum[2] = 620.0f/(((float) HRES) * 0.5f * viewport_rec.GetWidth());
	AltFrustum[3] = 524.0f/(((float) VRES) * 0.5f * viewport_rec.GetHeight());

	// invert the viewport scale and offset
	InverseViewportScale[0]  = 1.0f / ViewportScale[0];
	InverseViewportScale[1]  = 1.0f / ViewportScale[1];
	InverseViewportScale[2]  = 1.0f / ViewportScale[2];
	InverseViewportScale[3]  = 1.0f / ViewportScale[3];

	InverseViewportOffset[0] = -InverseViewportScale[0] * ViewportOffset[0] * InverseViewportScale[3];
	InverseViewportOffset[1] = -InverseViewportScale[1] * ViewportOffset[1] * InverseViewportScale[3];
	InverseViewportOffset[2] = -InverseViewportScale[2] * ViewportOffset[2] * InverseViewportScale[3];
	InverseViewportOffset[3] = -InverseViewportScale[3] * ViewportOffset[3] * InverseViewportScale[3];
	InverseViewportOffset[3] = FogNear;
	
	InverseIntViewportScale[0]  = 1.0f / IntViewportScale[0];
	InverseIntViewportScale[1]  = 1.0f / IntViewportScale[1];
	InverseIntViewportScale[2]  = 1.0f / IntViewportScale[2];
	InverseIntViewportScale[3]  = 1.0f / IntViewportScale[3];

	InverseIntViewportOffset[0] = -InverseIntViewportScale[0] * IntViewportOffset[0] * InverseIntViewportScale[3];
	InverseIntViewportOffset[1] = -InverseIntViewportScale[1] * IntViewportOffset[1] * InverseIntViewportScale[3];
	InverseIntViewportOffset[2] = -InverseIntViewportScale[2] * IntViewportOffset[2] * InverseIntViewportScale[3];
	InverseIntViewportOffset[3] = 0.0f;
	
	SkyInverseViewportScale[0]  = 1.0f / SkyViewportScale[0];
	SkyInverseViewportScale[1]  = 1.0f / SkyViewportScale[1];
	SkyInverseViewportScale[2]  = 1.0f / SkyViewportScale[2];
	SkyInverseViewportScale[3]  = 1.0f / SkyViewportScale[3];

	SkyInverseViewportOffset[0] = -SkyInverseViewportScale[0] * SkyViewportOffset[0] * SkyInverseViewportScale[3];
	SkyInverseViewportOffset[1] = -SkyInverseViewportScale[1] * SkyViewportOffset[1] * SkyInverseViewportScale[3];
	SkyInverseViewportOffset[2] = -SkyInverseViewportScale[2] * SkyViewportOffset[2] * SkyInverseViewportScale[3];
	SkyInverseViewportOffset[3] = 0.0f;
	


	// generate the GS register values we'll need for the viewport
	reg_XYOFFSET = PackXYOFFSET((int)(16.0f * (2048.0f - ((float) HRES) * (viewport_rec.GetOriginX() + 0.5f * viewport_rec.GetWidth()))),
								(int)(16.0f * (2048.0f - ((float) VRES) * (viewport_rec.GetOriginY() + 0.5f * viewport_rec.GetHeight()))));
	reg_SCISSOR  = PackSCISSOR( (int)(((float) HRES) * viewport_rec.GetOriginX()),
								(int)(((float) HRES) * (viewport_rec.GetOriginX() + viewport_rec.GetWidth())) - 1,
								(int)(((float) VRES) * viewport_rec.GetOriginY()),
								(int)(((float) VRES) * (viewport_rec.GetOriginY() + viewport_rec.GetHeight())) - 1);


	// inverse frustum transform
	//ViewFrustumToCamera.Zero();
	//ViewFrustumToCamera[0][0] = tx;
	//ViewFrustumToCamera[1][1] = ty;
	//ViewFrustumToCamera[2][3] = (near - far) / (2.0f * near * far);
	//ViewFrustumToCamera[3][2] = -1.0f;
	//ViewFrustumToCamera[3][3] = -(near + far) / (2.0f * near * far);

	// get world coords of the far frustum vertices
	Mth::Vector vFTL, vFTR, vFBL, vFBR;
	vFTL.Set( tx*far, ty*far, far, 1.0f);
	vFTL *= CameraToWorld;
	vFTR.Set(-tx*far, ty*far, far, 1.0f);
	vFTR *= CameraToWorld;
	vFBL.Set( tx*far,-ty*far, far, 1.0f);
	vFBL *= CameraToWorld;
	vFBR.Set(-tx*far,-ty*far, far, 1.0f);
	vFBR *= CameraToWorld;

	Mth::Vector pos(WorldToCamera[3]);
	for (int i=0; i<3; i++)
	{
		FrustumFlagsPlus[i]  = (vFTL[i]>=pos[i] && vFTR[i]>=pos[i] && vFBL[i]>=pos[i] && vFBR[i]>=pos[i]);
		FrustumFlagsMinus[i] = (vFTL[i]<=pos[i] && vFTR[i]<=pos[i] && vFBL[i]<=pos[i] && vFBR[i]<=pos[i]);
	}


    // Set ViewportMask.  Will only draw CGeomNodes that have a non-zero result with (ViewportMask & Visibility)
	ViewportMask = (1 << (VISIBILITY_FLAG_BIT + viewport_num));

	// Mark current viewport and frame
	RenderVarFrame = Frame;
	RenderVarViewport = viewport_num;

}

void	render::SetupVarsDMA(uint viewport_num, float near, float far)
{


	// initialisation for new bounding volume tests
	Mth::Matrix Normals;

	asm ("vmaxw vf31,vf00,vf00w": :);

	// view frustum	 R,  L,  B,  T
	Normals[0].Set(-cx, cx,  0,  0);
	Normals[1].Set(  0,  0, cy,-cy);
	Normals[2].Set(-sx,-sx,-sy,-sy);
	Normals[3].Set(  0,  0,  0,  0);

	Normals = WorldToCamera * Normals;

	asm __volatile__("

		lqc2		vf10,(%0)
		lqc2		vf11,(%1)
		lqc2		vf12,(%2)
		lqc2		vf16,(%3)
		vabs		vf13,vf10
		vabs		vf14,vf11
		vabs		vf15,vf12

	": : "r" (&Normals[0]), "r" (&Normals[1]), "r" (&Normals[2]), "r" (&Normals[3]));

	// both frusta    ?,   ?    N,   F,
	Normals[0].Set(   0,   0,   0,   0);
	Normals[1].Set(   0,   0,   0,   0);
	Normals[2].Set(   0,   0,  -1,   1);
	Normals[3].Set(   0,   0,near,-far);

	Normals = WorldToCamera * Normals;

	asm __volatile__("

		lqc2		vf17,(%0)
		lqc2		vf18,(%1)
		lqc2		vf19,(%2)
		lqc2		vf23,(%3)
		vabs		vf20,vf17
		vabs		vf21,vf18
		vabs		vf22,vf19

	": : "r" (&Normals[0]), "r" (&Normals[1]), "r" (&Normals[2]), "r" (&Normals[3]));


	// outer frustum R,  L,  B,  T
	Normals[0].Set(-Cx, Cx,  0,  0);
	Normals[1].Set(  0,  0, Cy,-Cy);
	Normals[2].Set(-Sx,-Sx,-Sy,-Sy);
	Normals[3].Set(  0,  0,  0,  0);

	Normals = WorldToCamera * Normals;

	asm __volatile__("

		lqc2		vf24,(%0)
		lqc2		vf25,(%1)
		lqc2		vf26,(%2)
		lqc2		vf30,(%3)
		vabs		vf27,vf24
		vabs		vf28,vf25
		vabs		vf29,vf26

	": : "r" (&Normals[0]), "r" (&Normals[1]), "r" (&Normals[2]), "r" (&Normals[3]));




	#if OLD_FOG
	// link in the dma list that does fogging postprocessing
	dma::SetList(sGroup::pFog);
	dma::Tag(dma::call, 0, (uint)&FogDma);
	vif::NOP();
	vif::NOP();
	sGroup::pFog->Used[render::Field] = true;

	dma::Gosub(SET_RENDERSTATE,2);
	dma::Gosub(SET_FOGCOL,2);
	#endif


	// upload new vu1 code and set up double-buffering
	dma::SetList(sGroup::pParticles);
	dma::Tag(dma::ref, ((uint8 *)&NewMPGEnd-(uint8 *)&NewMPGStart+15)/16, (uint)&NewMPGStart);
	vif::BASE(0);
	vif::OFFSET(32);
	
	// send vu1 data for particle setup
	dma::BeginTag(dma::cnt, 0);
	vif::FLUSH();
	vif::BeginDIRECT();
	gif::Tag2(gs::A_D,1,PACKED,0,0,1,3);
	gs::Reg2(gs::SCISSOR_1, render::reg_SCISSOR);
	gs::Reg2(gs::XYOFFSET_1, render::reg_XYOFFSET);
	gs::Reg2(gs::ZBUF_1, PackZBUF(ZBUFFER_START,PSMZ24,1));
	vif::EndDIRECT();
	vif::UNPACK(0, V4_32, 7, REL, SIGNED, 0);
	vu1::StoreMat(*(Mat *)&render::WorldToIntViewport);			// view transform
	vif::StoreV4_32F(320.0f, 320.0f, 0.0f, 0.0f);				// kvec
	vif::StoreV4_32(0x39005000, 0x39006000, 0, 0x3F800000);		// NTL cull test vec - w-component limits tan of apparent radius
	vif::StoreV4_32(0x3900B000, 0x3900A000, 0, 0);				// BR cull test vec
	vif::MSCAL(8);		// init sprites
	dma::EndTag();
	dma::SetList(NULL);
}



// Checks to see if the render variables are current
bool render::VarsUpToDate(uint viewport_num)
{
	return (RenderVarFrame == Frame) && (RenderVarViewport == viewport_num);
}

// Invalidates the render variables
void render::InvalidateVars()
{
	RenderVarFrame = 0xFFFFFFFF; 
	RenderVarViewport = 0xFFFFFFFF;
}

// a quick hack for Dave...
void DrawRectangle(int x0, int y0, int x1, int y1, uint32 rgba)
{
	dma::SetList(sGroup::pEpilogue);
	dma::BeginTag(dma::cnt, 0);
	vif::FLUSH();
	vif::BeginDIRECT();
	gif::BeginTag2(gs::A_D, 1, PACKED, SPRITE|ABE, 1);
	gs::Reg2(gs::ALPHA_1,	PackALPHA(0,1,0,1,0));
	gs::Reg2(gs::RGBAQ,		(uint64)rgba);
	gs::Reg2(gs::XYZ2,		PackXYZ(x0,y0,0xFFFFFF));
	gs::Reg2(gs::XYZ2,		PackXYZ(x1,y1,0xFFFFFF));
	gif::EndTag2(1);
	vif::EndDIRECT();
	dma::EndTag();
}



} // namespace NxPs2

