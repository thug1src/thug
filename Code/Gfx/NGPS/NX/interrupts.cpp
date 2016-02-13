#include <core/defines.h>
#include <eekernel.h>
#include <eeregs.h>
#include "texture.h"
#include "group.h"
#include "render.h"
#include "gs.h"
#include "switches.h"
#include <sys\profiler.h>

//#undef	__USE_PROFILER__


/*

 Mick Notes:

 All interrupts are set up in the "InitialiseEngine" function of nx_init.cpp
 
 There are three types of interrupt covered here
 
 1) GIF Interrupt - Used to signal the completion of texture upload	to VRAM
 2) GS  Interrupt - Used to signal the completion of GS rendering (drawing polygons)
 3) VIF Interrupt - NOT USED, could be used like the GS interrupt, but unreliable.

 There are two types of activity going on in parallel: Upload and Render
 
 Whenever one of these finishes, it causes an interrupt. 
 
 There is no way of knowing in advance what order the tasks will finish in
 so we must handle all possible cases. 

 At any time, either upload or render might be "Stalled".  In this case we are waiting
 for something before we can start rendering or uploading a new group.  Stalls are not
 good, as the relevent processor is sat idle.  We want to minimize stalls as much as
 possible.  
 
 Stalls happen when:
 
  - The initial render group is waiting for its textures to upload.  
    (?) The renderer is always stalled on the first group 
  - An upload group want to use some VRAM that is used by a group 
    that is currently rendering. 
  - A render group is waiting for it's textures to upload. This could
    happen if a group renders quicker than the next groups uploads.

*/
   
   
   
namespace NxPs2
{


#define START_UPLOAD()																\
{																					\
	*D2_QWC = 0;																	\
	*D2_TADR = (uint)sGroup::pUploadGroup->pUpload[!render::Field];							\
	*D2_CHCR = 0x105;																\
	UploadStalled = 0;																\
}

#if GS_INTERRUPT
#define START_RENDER()																\
{																					\
	*D1_QWC = 0;																	\
	*D1_TADR = (uint)sGroup::pRenderGroup->pRender[!render::Field];							\
	*D1_CHCR = 0x145;								   								\
	RenderStalled = 0;																\
}
#endif

#if VIF1_INTERRUPT
#define START_RENDER()																\
{																					\
if (sGroup::pRenderGroup != sGroup::pHead)											\
		*VIF1_FBRST = 1<<3;															\
	else																			\
	{																				\
		*D1_QWC = 0;																\
		*D1_TADR = (uint)sGroup::pRenderGroup->pRender[!render::Field];						\
		*D1_CHCR = 0x145;								   							\
	}																				\
}
#endif

#define NO_VRAM_CONFLICT()															\
(																					\
	sGroup::pRenderGroup->VramStart >= sGroup::pUploadGroup->VramEnd ||				\
	sGroup::pUploadGroup->VramStart >= sGroup::pRenderGroup->VramEnd				\
)


volatile uint32 UploadStalled, RenderStalled;



// handler called when an upload finishes

int GifHandler(int Cause)
{
	if (!sGroup::pUploadGroup)		// Mick:  Might not actually be anything to upload, if p_memview was uploading
		goto EXIT;
	
	// we've finished uploading a group
	sGroup::pUploadGroup = sGroup::pUploadGroup->pNext;
	while (sGroup::pUploadGroup && !sGroup::pUploadGroup->Used[!render::Field])		// if exists, but not used
	{
		sGroup::pUploadGroup = sGroup::pUploadGroup->pNext;	// then skip groups until reach end, or find one that is used
	}


#	ifdef __USE_PROFILER__
	Sys::DMAProfiler->PopContext();	 
#	endif // __USE_PROFILER__

	// is there another group to upload?
	if (sGroup::pUploadGroup)
	{

		
		// was there a stalled render?
		if (RenderStalled)
		{
			// kick off the stalled render
#	ifdef __USE_PROFILER__
				Sys::VUProfiler->PushContext( sGroup::pRenderGroup->profile_color );	   									
#	endif // __USE_PROFILER__
			START_RENDER();

			// is the next upload safe?
			if (NO_VRAM_CONFLICT())
			{
				// kick off the next upload
#	ifdef __USE_PROFILER__
				Sys::DMAProfiler->PushContext( sGroup::pUploadGroup->profile_color );	   									
#	endif // __USE_PROFILER__
				START_UPLOAD();
			}
			else
			{
				// stall the next upload
				UploadStalled = 1;
			}
		}

		else// there's a render in progress
		{
			// since we just finished an upload, then the render in progress MUST
			// be using the previously uploaded group
			// so there is no way that we can start a new upload until that render finishes.
			// so, stall the next upload
			UploadStalled = 1;
		}
	}

	else// no more groups to upload
	{
		// was the final render stalled?
		if (RenderStalled)
		{
			// kick it off
#	ifdef __USE_PROFILER__
				Sys::VUProfiler->PushContext(  sGroup::pRenderGroup->profile_color );	   									
#	endif // __USE_PROFILER__
			START_RENDER();
		}
	}

EXIT:
	ExitHandler();
	return 0;
}




// handler called when a render finishes (gs version)

int GsHandler(int Cause)
{
	// mask GS interrupts
	*GS_IMR = PackIMR(1,1,1,1);

	// reset SIGNAL event bit
	*GS_CSR = *GS_CSR & ~(uint64)0x30F | 1;

	// reset signal bit
	*GS_SIGLBLID &= ~(uint64)1;

	// maybe it was a false alarm...
	if (!sGroup::pRenderGroup)	// if nothing currently rendering, then we've finished!!
		goto EXIT;
	
	// we've finished rendering a group, so find the next group to render
	sGroup::pRenderGroup = sGroup::pRenderGroup->pNext;
	while (sGroup::pRenderGroup && !sGroup::pRenderGroup->Used[!render::Field])		// if exists, but not used
	{
		sGroup::pRenderGroup = sGroup::pRenderGroup->pNext;	// then skip groups until reach end, or find one that is used
	}

	#	ifdef __USE_PROFILER__
	Sys::VUProfiler->PopContext();	 
	#	endif // __USE_PROFILER__

	
	// is there another group to render?
	if (sGroup::pRenderGroup)
	{
		// was there a stalled upload?
		if (UploadStalled)
		{
			// is the next render waiting for the stalled upload?
			// (i.e., they both are the same group)
			if (sGroup::pRenderGroup == sGroup::pUploadGroup)
			{
				// kick off the stalled upload
#	ifdef __USE_PROFILER__
				Sys::DMAProfiler->PushContext(  sGroup::pUploadGroup->profile_color );	   									
#	endif // __USE_PROFILER__
				START_UPLOAD();

				// but stall the next render
				// as this upload group is needed for this render 
				RenderStalled = 2;
			}
			else
			{
				// kick off the next render, as it's from a previous group
#	ifdef __USE_PROFILER__
				Sys::VUProfiler->PushContext(  sGroup::pRenderGroup->profile_color );	   									
#	endif // __USE_PROFILER__
				START_RENDER();

				// is the next upload safe?
				if (NO_VRAM_CONFLICT())
				{
					// kick off the next upload
#	ifdef __USE_PROFILER__
					Sys::DMAProfiler->PushContext(  sGroup::pUploadGroup->profile_color );	   									
#	endif // __USE_PROFILER__
					START_UPLOAD();
				}
			}
		}

		// is there an upload in progress?
		// (Mick: Does it make sense to also check to see if the upload group is the same as the render group?
		// or is that always the case, and that's why Mike is not checking it?)
		else if (sGroup::pUploadGroup)
		{
			// stall the next render
			RenderStalled = 1;
		}

		// no uploads left
		else
		{
			// kick off the final render
#	ifdef __USE_PROFILER__
				Sys::VUProfiler->PushContext(  sGroup::pRenderGroup->profile_color );	   									
#	endif // __USE_PROFILER__
			START_RENDER();
		}
	}

EXIT:
	// re-enable GS interrupts
	*GS_IMR = PackIMR(0,1,1,1);

	ExitHandler();
	return 0;
}





// handler called when a render finishes (vif version)
// (Mick:  not currently used, ignore for now)
int Vif1Handler(int Cause)
{
	// make sure the mark register gives the go-ahead
	if (*VIF1_MARK != 1)
		goto EXIT;

	// maybe it was a false alarm...
	if (!sGroup::pRenderGroup)
		goto EXIT;
	
	// we've finished rendering a group
	sGroup::pRenderGroup = sGroup::pRenderGroup->pNext;		// get next group
	
	// is there another group to render?
	if (sGroup::pRenderGroup)
	{
		// was there a stalled upload?
		if (UploadStalled)
		{
			// is the next render waiting for the stalled upload?
			if (sGroup::pRenderGroup == sGroup::pUploadGroup)
			{
				// kick off the stalled upload
#	ifdef __USE_PROFILER__
				Sys::DMAProfiler->PushContext(  sGroup::pUploadGroup->profile_color );	   									
#	endif // __USE_PROFILER__
				START_UPLOAD();
				UploadStalled = 0;

				// but stall the next render
				RenderStalled = 2;
			}
			else
			{
				// kick off the next render
#	ifdef __USE_PROFILER__
				Sys::VUProfiler->PushContext(  sGroup::pRenderGroup->profile_color );	   									
#	endif // __USE_PROFILER__
				START_RENDER();

				// is the next upload safe?
				if (NO_VRAM_CONFLICT())
				{
					// kick off the next upload
#	ifdef __USE_PROFILER__
					Sys::DMAProfiler->PushContext(  sGroup::pUploadGroup->profile_color );	   									
#	endif // __USE_PROFILER__
					START_UPLOAD();
					UploadStalled = 0;
				}
			}
		}

		// is there an upload in progress?
		else if (sGroup::pUploadGroup)
		{
			// stall the next render
			RenderStalled = 1;
		}

		// no uploads left
		else
		{
			// kick off the final render
#	ifdef __USE_PROFILER__
				Sys::VUProfiler->PushContext(  sGroup::pRenderGroup->profile_color );	   									
#	endif // __USE_PROFILER__
			START_RENDER();
		}
	}

EXIT:
	ExitHandler();
	return 0;
}




} // namespace NxPs2

