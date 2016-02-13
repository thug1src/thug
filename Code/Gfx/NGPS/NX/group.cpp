#include <core/defines.h>
#include "group.h"
	
/*

 (Notes by Mick)
 
 A Group is a set of textures, and a set of meshes or primitives that use those textures.
 
 Rendering is performed one group at a time. 
 
 The textures for a group are uploaded, and then it is rendered.
 
 Whilst a group is being rendered, the uploading of the next group's textures
 can start to be uploaded.
 
 This all happens asyncronously, whilst the CPU is doing the calculations for the next frame.
 
 The uploading/rendering cycle is syncronized by interrupts.
 
 The groups are rendered in this order:
 
 1) PrologueGroup - set up in render.cpp - flips the display buffers and initializes VU1 microcode 
 2) The groups linked by PrologueGroup->pNext, which represent groups of textures and polygons
 3) pEpilogue, (the last in this list) - uploads fonts and sprites, allowing them to be rendered next

 after the groups are rendered, we can render the 2D elements, namely the sprites and fonts (see render.cpp)

*/
				
				
namespace NxPs2
{

sGroup *sGroup::pHead;
sGroup *sGroup::pTail;
sGroup *sGroup::pShadow;
sGroup *sGroup::pFog;
sGroup *sGroup::pParticles;
sGroup *sGroup::pEpilogue;
volatile sGroup *sGroup::pUploadGroup;    // Group that is currently uploading
volatile sGroup *sGroup::pRenderGroup;	  // Group that is currently rendering
uint32 sGroup::VramBufferBase;

} // namespace NxPs2

