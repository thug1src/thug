#include <core/defines.h>
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gs.h"
#include "group.h"
#include "render.h"
#include "vu1code.h"
#include "switches.h"
#include "geomnode.h"

/*

 Mick Notes:

 The calls the the DMA functions suggest that you are immediately executing something.
 You are not.  You are simply building a list of DMA commands that get executed the next frame
 
 The DMA list is the primary mechanism for initiating rendering on the PS2.
 
 To render something (like a line), you "simply" need to generate the appropiate DMA
 packets that contain GIF packets, containing GS primitives, and then link this into the 
 main DMA list.
 
 Things such as world sectors have pre-build DMA lists that contain commands
 to upload lists of vertices to VU1 micro-memory, and then upload lists of
 triangles, and then trigger the appropiate VU1 microcode to transform and render them.
 
 These pre-built lists are linked in each frame with the appropiate transformation 
 matricies that are needed based on the current camera position and the current
 object position (if it is a moving object)
 
 Thus, each object is mostly pre-build, and so rendering it requires very little CPU time.

 Each frame, we build a series of dynamic DMA lists.
 
 There are two lists for each group (see group.cpp).
 
 The first list is the texture upload.  This is pretty simple, just uploading a few textures.
 Texture uploading happens asyncrnously with rendering, so we can be executing a DMA list
 of rendering stuff at the same time as we are uploading the textures for the next group.   

 A lot of the DMA code is in DMA.H, as it is small inline functions.  

 The Runtime buffer is dynamically allocated in InitializeEngine().  Last time I looked it
 was 512K in size (note though, this is increased to 16MB for the debugging wireframe mode)
 
 The Runtime DMA lists are double buffered. You are executing one whilst building the next one.
 
 The pRunTimeBuffer is split in two, and pointers to the base of each half are stored in 
 pList[0] and pList[1]   

 The integer varaible "Field" is 0 or 1, and indicates which field (odd or even) we are rendering to.  
 
 The DMA lists are build using a global variable char * pLoc.  
 
 At the start of a frame, pLoc is initilized like:
 
 	dma::pLoc = dma::pList[Field];

 So it points to the start of one of our 256K DMA buffers (remember the other is being executed)
 
 DMA packets are now built using calls to the functions here and in DMA.H, for example: 
 
	dma::Tag(dma::ref, ((uint8 *)&MPGEnd-(uint8 *)&MPGStart+15)/16, (uint)&MPGStart);
	vif::NOP();
	vif::NOP();
	
 (the above simply transfers the microcode from RAM to VU1 micromem)	
																					   
 Now, it's important to note that DMA execution does NOT simply start at pList[Field] and
 then run all the way through a frame's worth of data.  There are actually several DMA lists,
 which are fired off individually.  Some of these (the groups) are logically connected so that 
 the end of one group's DMA (and the GS/VU activity it triggers) causes an interrupt,
 which the CPU handles by starting the DMA for another group (see interrupt.cpp)

 After the groups, there is the "immediate mode" DMA list, triggered by pEpilogue->pRender[Field]
 this is simply a raw DMA list that you can put anything you want into.  It starts at the value
 of pLoc after RenderWorld has been called (a terrible hack, as noted in the code :)
 
 All of the "immediate mode" rendering is considered to be part of pEpilogue.  It is terminated 
 by the RenderEpilogue() function, which just links in a final interrupt trigger, which will 
 get picked up by GsHandler(), and sGroup::pRenderGroup will be set to NULL, which is what
 WaitForRendering() uses to determine if rendering has finished.
 
 						  
						  

*/



namespace NxPs2
{



// begin a subroutine

void dma::BeginSub(eTag ID)
{
	Align(0,16);
	pSub = pLoc;
	dma::ID = ID;
	vu1::Loc = vu1::Buffer = 0;
}


// end a subroutine and return its address

uint64 dma::EndSub(void)
{
	uint64 GosubTag;
	Align(0,16);
	GosubTag = ((uint64)pSub<<32) | ((uint64)ID<<28) | (vu1::Loc & 0x3FF) << 16;
	Dbg_MsgAssert(ID!=refe && ID!=refs, ("refe and refs not supported in dma tag!"));
	if (ID==ref)
	{
		GosubTag |= (pLoc - pSub) >> 4;
	}
	return GosubTag;
}


// call a subroutine (using a dma::call or a dma::ref)

void dma::Gosub(uint Num, uint Path)
{
	register uint64 Tag = Gosubs[Num];

	Store64(Tag);

	switch (Path)
	{
	case 1:
		vif::BASE(vu1::Loc);
		vif::OFFSET(0);
		vu1::Loc += (Tag>>16) & 0x3FF;			// VUMem size
		break;

	case 2:
		vif::NOP();
		vif::NOP();
		break;

	case 3:
		pLoc += 8;								// no need to write anything since TTE=0
		break;

	#ifdef __PLAT_NGPS__
	default:
		printf("error: dma::Gosub() called with unrecognised path number\n");
		exit(1);
	#endif
	}

}



// begin a 3D subroutine

void dma::BeginSub3D(void)
{
	Align(0,16);
	pSub = pLoc;
	vu1::Loc = vu1::Buffer = 0;
}


// end a 3D subroutine and return its address

uint8 *dma::EndSub3D(void)
{
	((uint16 *)pSub)[1] |= vu1::Loc&0x3FF;
	return pSub;
}


// call a 3D subroutine (always using a dma::call)

void dma::Gosub3D(uint8 *pSub, uint RenderFlags)
{
	BeginTag(call,(uint)pSub);
	vif::BASE(vu1::Loc);
	vif::OFFSET(0);
	vif::ITOP(RenderFlags);
	EndTag();
	vu1::Loc += ((uint16 *)pSub)[1];
}



// dma list traversal function
uint8 *dma::NextTag(uint8 *pTag, bool stepInto)
{
	Dbg_MsgAssert((*(uint32 *)pTag&0x80000000)==0, ("IRQ bit set in dma tag"));
	Dbg_MsgAssert(*(uint32 *)pTag!=refe<<28 && *(uint32 *)pTag!=refs<<28, ("refe and refs not supported in dma tag!"));
	switch (*(uint32 *)pTag>>28)
	{
	case cnt:
		return pTag + 16 + ((uint)((uint16 *)pTag)[0] << 4);

	case next:
		return ((uint8 **)pTag)[1];

	case ref:
		return pTag + 16;

	case call:
		Dbg_MsgAssert(sp<2, ("dma call stack overflow"));
		if (stepInto)
		{
			Stack[sp++] = pTag + 16 + ((uint)((uint16 *)pTag)[0] << 4);
			return ((uint8 **)pTag)[1];
		}
		else
		{
			return pTag + 16 + ((uint)((uint16 *)pTag)[0] << 4);
		}

	case ret:
		Dbg_MsgAssert(sp>0, ("dma call stack underflow"));
		return Stack[--sp];

	case end:
	default:
		return NULL;
	}
}


#ifdef __PLAT_NGPS__

// auxilliary comparison function for dma sort
int dma::Cmp(const void *p1, const void *p2)
{
	return ((SSortElement *)p1)->z < ((SSortElement *)p2)->z ? -1 :
		   ((SSortElement *)p1)->z > ((SSortElement *)p2)->z ? +1 :
																0 ;
}


#if 0

// original version

// sort a dma list of mesh packets on z
// the z is stored in the unused ADDR word of the first cnt tag of the mesh packet

uint8 *dma::SortGroup(uint8 *pList)
{
	int num_elements;
	SSortElement *p_element;
	uint8 *p_tag, *p_end_tag, *p_prev_tag=NULL;
	eTag ID;

	// set array base at start of scratchpad
	SSortElement *p_array = (SSortElement *)0x70000000;

	// copy the addresses and z-values into array
	p_element = p_array;
	p_tag = pList;
	sp = 0;				// starting at top level of dma list
    while ((ID = (eTag)(*(uint32 *)p_tag>>28)) != end)
	{
		if (ID == cnt)
		{
			p_element->address = p_tag;
			p_element->z = ((float *)p_tag)[1];
			p_element++;
		}
		p_tag = NextTag(p_tag, false);
	}

	// check array fits within scratchpad
	num_elements = p_element-p_array;
	Dbg_MsgAssert(num_elements*sizeof(SSortElement)<=16384, ("Can't fit array in scratchpad"));

	// record address of end tag
	p_end_tag = p_tag;

	// sort the array
	qsort(p_array, num_elements, sizeof(SSortElement), Cmp);

	// reorder the dma list according to the sorted array
	for (p_element=p_array; p_element<p_array+num_elements; p_element++)
	{
		p_tag = p_element->address;

		do
		{
			p_prev_tag = p_tag;
			p_tag = NextTag(p_tag, false);
			ID = (eTag)(*(uint32 *)p_tag >> 28);
		} while (ID!=cnt && ID!=end);

		((uint8 **)p_prev_tag)[1] = (p_element+1)->address;
	}

	// patch up the final dma::next tag to point to the dma::end tag
	((uint8 **)p_prev_tag)[1] = p_end_tag;

	// chain through the whole list to adjust vu1 base pointers
	vu1::Loc = 0;
	for (p_tag = p_array->address; *(uint32 *)p_tag>>28 != end; p_tag = NextTag(p_tag, false))
	{
		//if (*(uint32 *)p_tag & 0x03FF0000)
		if (((uint8 *)p_tag)[11] == 0x03)
		{
			((uint16 *)p_tag)[4] &= ~0x3FF;
			((uint16 *)p_tag)[4] |= vu1::Loc;
			vu1::Loc += ((uint16 *)p_tag)[1];
		}
	}

	// return the address of the head tag
	return p_array->address;
}

#else

// new version:
// the list is sorted in segments corresponding to viewports

// sort a dma list of mesh packets on z
// the z is stored in the unused ADDR word of the first cnt tag of the mesh packet

uint8 *dma::SortGroup(uint8 *pList)
{
	int num_elements, i, num_segments;
	SSortElement *p_element, *p_segment[4];
	uint8 *p_tag, *p_end_tag, *p_prev_tag=NULL;
	eTag ID;

	// set array base at start of scratchpad
	SSortElement *p_array = (SSortElement *)0x70000000;

	// copy the addresses and z-values into array
	p_element = p_array;
	p_tag = pList;
	sp = 0;				// starting at top level of dma list
	num_segments = 0;
    while ((ID = (eTag)(*(uint32 *)p_tag>>28)) != end)
	{
		for (i=0; i<render::sMarkerIndex; i++)
		{
			if ((int)p_tag == render::sSortedListMarker[i])
			{
				//printf("matched marker %d\n", i);
				p_segment[num_segments++] = p_element;
			}
		}

		if (ID == cnt)
		{
			p_element->address = p_tag;
			p_element->z = ((float *)p_tag)[1];
			p_element++;
		}
		p_tag = NextTag(p_tag, false);
	}

	// check array fits within scratchpad
	num_elements = p_element-p_array;
	Dbg_MsgAssert(num_elements*sizeof(SSortElement)<=16384, ("Can't fit array in scratchpad"));

	// record address of end tag
	p_end_tag = p_tag;

	// sort the array in segments
	if (num_segments)
	{
		for (i=0; i<num_segments-1; i++)
		{
			//printf("sorting from %08X to %08X\n", p_segment[i], p_segment[i+1]);
			qsort(p_segment[i], p_segment[i+1]-p_segment[i], sizeof(SSortElement), Cmp);
		}
		//printf("sorting from %08X to %08X\n", p_segment[i], p_element);
		qsort(p_segment[i], p_element-p_segment[i], sizeof(SSortElement), Cmp);
	}

	// reorder the dma list according to the sorted array
	for (p_element=p_array; p_element<p_array+num_elements; p_element++)
	{
		p_tag = p_element->address;

		do
		{
			p_prev_tag = p_tag;
			p_tag = NextTag(p_tag, false);
			ID = (eTag)(*(uint32 *)p_tag >> 28);
		} while (ID!=cnt && ID!=end);

		((uint8 **)p_prev_tag)[1] = (p_element+1)->address;
	}

	// patch up the final dma::next tag to point to the dma::end tag
	((uint8 **)p_prev_tag)[1] = p_end_tag;

	// chain through the whole list to adjust vu1 base pointers
	vu1::Loc = 0;
	for (p_tag = p_array->address; *(uint32 *)p_tag>>28 != end; p_tag = NextTag(p_tag, false))
	{
		if (*(uint32 *)p_tag & 0x3FF0000)
		{
			((uint16 *)p_tag)[4] &= ~0x3FF;
			((uint16 *)p_tag)[4] |= vu1::Loc;
			vu1::Loc += ((uint16 *)p_tag)[1];
		}
	}

	// return the address of the head tag
	return p_array->address;
}

#endif


#endif


void dma::BeginList(void *pGroup)
{
#ifdef __PLAT_NGPS__
	// assume group isn't used
	((sGroup *)pGroup)->Used[render::Field] = false;

	// set the dma list pointer
	((sGroup *)pGroup)->pRender[render::Field] = pLoc;

	// VIF1 and VU1 setup
	BeginTag(cnt, 0xFF000000);		// bit of a cheat, so it will stay at the start of any sorted list
	vif::FLUSH();
	vif::STMASK(0);
	vif::STMOD(0);
	vif::STCYCL(1,1);
	vif::BASE(0);
	vif::OFFSET(0);
	vif::MSCAL(VU1_ADDR(Setup));
	EndTag();

	dma::Tag(dma::next, 0, 0);
	vif::NOP();
	vif::NOP();

	((sGroup *)pGroup)->vu1_loc = 0;
	((sGroup *)pGroup)->p_tag = pTag;
#endif
}


void dma::EndList(void *pGroup)
{

	SetList(pGroup);

	// end dma list for this group
	BeginTag(end, 0);
	#if USE_INTERRUPTS
	//vif::BASE(((sGroup *)pGroup)->vu1_loc);
	vif::BASE(vu1::Loc);
	vif::OFFSET(0);
	vu1::Loc = 0;   							// must do this as a relative prim for a sortable list...
	gs::BeginPrim(REL,0,0);
	gs::Reg1(gs::SIGNAL, PackSIGNAL(1,1));		// signal the end of rendering this group
	gs::EndPrim(1);
	vif::MSCAL(VU1_ADDR(Parser));
	#endif
	EndTag();
	((uint16 *)pTag)[1] |= vu1::Loc & 0x3FF;	// must write some code for doing this automatically
}


void dma::ReallySetList(void *pGroup)
{

	// finish with the previous dma context
	if (sp_group)
	{
		// ensure the last tag was a 'next'...

		// get the tag ID
		uint ID = *(uint32 *)pTag>>28;

		// take care of 'refe' and 'refs'
		Dbg_MsgAssert(ID!=refe && ID!=refs, ("refe and refs not supported in dma tag!"));

		// take care of 'call' and 'ref'
		if (ID==call || ID==ref)
		{
			Tag(next, 0, 0);
			vif::NOP();
			vif::NOP();
		}

		// take care of 'cnt'
		else if (ID==cnt)
		{
			pTag[3] = next<<4;
		}

		// 'end' and 'ret' won't have anything after them in the same context
		// and 'next' is fine as it is

		// save the vu1 location and dma tag location
		((sGroup *)sp_group)->vu1_loc = vu1::Loc;
		((sGroup *)sp_group)->p_tag = pTag;
	}

	// change bucket
	sp_group = pGroup;

	// set up the new bucket
	if (pGroup)
	{
		// restore the vu1 location and dma tag location
		vu1::Loc = ((sGroup *)pGroup)->vu1_loc;
		pTag = ((sGroup *)pGroup)->p_tag;

		// patch the pointer of the dangling 'next' tag
		((uint32 *)pTag)[1] = (uint32)pLoc;
	}
}




int dma::GetDmaSize(uint8 *pTag)
{
	return (*(uint16 *)pTag + 1) << 4;	// (QWC+1)*16 bytes
}



int dma::GetNumVertices(uint8 *pTag)
{
	// get start and end of dma packet
	uint8 *p_start = pTag + 8;
	uint8 *p_end   = pTag + 16 + (*(uint16 *)pTag << 4);

	// parse vifcodes
	uint8 *p_code = p_start;
	int   num_verts = 0;
	do
	{
		if (((p_code[3] & 0x7F) == 0x05) && (p_code[0] == 1))		// look for STMOD(1)
		{
			p_code = vif::NextCode(p_code);
			Dbg_MsgAssert((p_code[3] & 0x7E)==0x6C, ("0x%08X: expected UNPACK V4_16 or V4_32", *(uint32 *)p_code));
			num_verts += p_code[2];
		}

		p_code=vif::NextCode(p_code);
	}
	while (p_code < p_end);

	return num_verts;
}



int dma::GetNumTris(uint8 *pTag)
{
	// get start and end of dma packet
	uint8 *p_start = pTag + 8;
	uint8 *p_end   = pTag + 16 + (*(uint16 *)pTag << 4);

	// parse vifcodes
	uint8 *p_code = p_start;
	int   num_tris = 0;
	int   num_verts;
	do
	{
		if (((p_code[3] & 0x7F) == 0x05) && (p_code[0] == 1))		// look for STMOD(1)
		{
			p_code = vif::NextCode(p_code);
			Dbg_MsgAssert((p_code[3] & 0x7E)==0x6C, ("0x%08X: expected UNPACK V4_16 or V4_32", *(uint32 *)p_code));
			num_verts = p_code[2];

			// loop over verts, counting the adc bits which are zero
			if (p_code[3] & 0x01)	// V4_16
			{
				uint16 *p_adc = ((uint16 *)p_code)+5;
				for (int i=0; i<num_verts; i++,p_adc+=4)
				{
					if ((*p_adc & 0x8000) == 0)
					{
						num_tris++;
					}
				}
			}
			else					// V4_32
			{
				uint32 *p_adc = ((uint32 *)p_code)+4;
				for (int i=0; i<num_verts; i++,p_adc+=4)
				{
					if ((*p_adc & 0x00008000) == 0)
					{
						num_tris++;
					}
				}
			}
		}

		p_code=vif::NextCode(p_code);
	}
	while (p_code < p_end);

	return num_tris;
}



void dma::Copy(uint8 *pTag, uint8 *pDest)
{
	memcpy(pDest, pTag, (*(uint16 *)pTag + 1) << 4);
}



void dma::TransferValues(uint8 *pTag, uint8 *pArray, int size, int dir, uint32 vifcodeMask, uint32 vifcodePattern)
{
	// get start and end of dma packet
	uint8 *p_start = pTag + 8;
	uint8 *p_end   = pTag + 16 + (*(uint16 *)pTag << 4);

	// parse vifcodes
	uint8 *p_code = p_start;
	uint32 *pSource, *pDest;
	int i, num_words;

	*(dir ? &pSource : &pDest) = (uint32 *)pArray;

	do
	{
		if ((*(uint32 *)p_code & vifcodeMask) == vifcodePattern)
		{
			*(!dir ? &pSource : &pDest) = (uint32 *)(p_code+4);

			num_words = (p_code[2] * size) >> 2;
			for (i=0; i<num_words; i++)
			{
				*pDest++ = *pSource++;
			}
		}

		p_code=vif::NextCode(p_code);
	}
	while (p_code < p_end);
}


void dma::TransferColours(uint8 *pTag, uint8 *pArray, int dir)
{
	// get start and end of dma packet
	uint8 *p_start = pTag + 8;
	uint8 *p_end   = pTag + 16 + (*(uint16 *)pTag << 4);

	// parse vifcodes
	uint8 *p_code = p_start;
	uint8 *pSource, *pDest;
	int i, num_words;

	*(dir ? &pSource : &pDest) = (uint8 *)pArray;

	do
	{
		if ((*(uint32 *)p_code & 0x7B000000) == 0x6A000000)
		{
			if ((*(uint32 *)p_code & 0x7F000000) == 0x6E000000)
			{
				TransferValues(pTag, pArray, 4, dir, 0x7F000000, 0x6E000000);
				return;
			}

			*(!dir ? &pSource : &pDest) = p_code+4;

			num_words = p_code[2];
			for (i=0; i<num_words; i++)
			{
				*pDest++ = *pSource++;
				*pDest++ = *pSource++;
				*pDest++ = *pSource++;
				(*(dir ? &pSource : &pDest))++;
			}
		}

		p_code=vif::NextCode(p_code);
	}
	while (p_code < p_end);
}




int dma::GetBitLengthXYZ(uint8 *pTag)
{
	// get start and end of dma packet
	uint8 *p_start = pTag + 8;
	uint8 *p_end   = pTag + 16 + (*(uint16 *)pTag << 4);

	// parse vifcodes
	uint8 *p_code = p_start;
	int   bit_length = 0;
	do
	{
		if (((p_code[3] & 0x7F) == 0x05) && (p_code[0] == 1))		// look for STMOD(1)
		{
			p_code = vif::NextCode(p_code);
			Dbg_MsgAssert((p_code[3] & 0x7E)==0x6C, ("0x%08X: expected UNPACK V4_16 or V4_32", *(uint32 *)p_code));
			bit_length = 32 >> (p_code[3] & 0x03);
			break;
		}

		p_code=vif::NextCode(p_code);
	}
	while (p_code < p_end);

	return bit_length;
}


void dma::ExtractXYZs(uint8 *pTag, uint8 *pArray)
{
	// get start and end of dma packet
	uint8 *p_start = pTag + 8;
	uint8 *p_end   = pTag + 16 + (*(uint16 *)pTag << 4);

	// parse vifcodes
	uint8 *p_code = p_start;
	int i, num_words;
	sint32 *p_dest = (sint32 *)pArray;

	do
	{
		if ((*(uint32 *)p_code & 0x7F000001) == 0x05000001)
		{
			p_code = vif::NextCode(p_code);
			Dbg_MsgAssert((p_code[3] & 0x7E)==0x6C, ("0x%08X: expected UNPACK V4_16 or V4_32", *(uint32 *)p_code));

			num_words = (int)((((uint32)p_code[2]-1)&0xFF)+1) << 2;

			if ((p_code[3] & 0x7F) == 0x6C)
			{
				// 32 bit
				sint32 *p_source = (sint32 *)(p_code+4);
				for (i=0; i<num_words; i++)
				{
					*p_dest++ = *p_source++;
				}
			}
			else
			{
				// 16 bit
				sint16 *p_source = (sint16 *)(p_code+4);
				for (i=0; i<num_words; i++)
				{
					*p_dest++ = (sint32)*p_source++;
				}
			}

		}

		p_code=vif::NextCode(p_code);
	}
	while (p_code < p_end);
}


void dma::ReplaceXYZs(uint8 *pTag, uint8 *pArray, bool skipW)
{
	//printf("Replacing XYZs...\n");

	// get start and end of dma packet
	uint8 *p_start = pTag + 8;
	uint8 *p_end   = pTag + 16 + (*(uint16 *)pTag << 4);

	// parse vifcodes
	uint8 *p_code = p_start;
	int i, num_words;
	sint32 *p_source = (sint32 *)pArray;

	do
	{
		if ((*(uint32 *)p_code & 0x7F000001) == 0x05000001)
		{
			p_code = vif::NextCode(p_code);
			Dbg_MsgAssert((p_code[3] & 0x7E)==0x6C, ("0x%08X: expected UNPACK V4_16 or V4_32", *(uint32 *)p_code));

			num_words = (int)((((uint32)p_code[2]-1)&0xFF)+1) << 2;

			if ((p_code[3] & 0x7F) == 0x6C)
			{
				// 32 bit
				sint32 *p_dest = (sint32 *)(p_code+4);
				if (skipW)
				{
					for (i=0; i<num_words; i++, p_dest++, p_source++)
					{
						if ((i & 3) == W)
						{
							continue;
						}
						*p_dest = *p_source;
					}
				} else {
					for (i=0; i<num_words; i++)
					{
						*p_dest++ = *p_source++;
					}
				}
			}
			else
			{
				// 16 bit
				sint16 *p_dest = (sint16 *)(p_code+4);
				if (skipW)
				{
					for (i=0; i<num_words; i++, p_dest++, p_source++)
					{
						if ((i & 3) == W)
						{
							continue;
						}
						*p_dest = (sint32)*p_source;
					}
				} else {
					for (i=0; i<num_words; i++)
					{
						*p_dest++ = (sint32)*p_source++;
					}
				}
			}

		}

		p_code=vif::NextCode(p_code);
	}
	while (p_code < p_end);
}


void dma::ExtractRGBAs(uint8 *pTag, uint8 *pArray)
{
	//TransferValues(pTag, pArray, 4, 0, 0x7F000000, 0x6E000000);
	TransferColours(pTag, pArray, 0);
}


void dma::ReplaceRGBAs(uint8 *pTag, uint8 *pArray)
{
	//TransferValues(pTag, pArray, 4, 1, 0x7F000000, 0x6E000000);
	TransferColours(pTag, pArray, 1);
}


void dma::ExtractSTs(uint8 *pTag, uint8 *pArray)
{
	// get start and end of dma packet
	uint8 *p_start = pTag + 8;
	uint8 *p_end   = pTag + 16 + (*(uint16 *)pTag << 4);

	// parse vifcodes
	uint8 *p_code = p_start;
	int i, num_words;
	sint32 *p_dest = (sint32 *)pArray;

	do
	{
		if ((*(uint32 *)p_code & 0x7E000000) == 0x64000000)
		{
			num_words = (int)((((uint32)p_code[2]-1)&0xFF)+1) << 1;

			if ((p_code[3] & 0x7F) == 0x64)
			{
				// 32-bit
				sint32 *p_source = (sint32 *)(p_code+4);
				for (i=0; i<num_words; i++)
				{
					*p_dest++ = *p_source++;
				}
			}
			else
			{
				// 16-bit
				sint16 *p_source = (sint16 *)(p_code+4);
				for (i=0; i<num_words; i++)
				{
					*p_dest++ = *p_source++;
				}
			}
		}

		p_code=vif::NextCode(p_code);
	}
	while (p_code < p_end);
}


void dma::ReplaceSTs(uint8 *pTag, uint8 *pArray)
{
	// get start and end of dma packet
	uint8 *p_start = pTag + 8;
	uint8 *p_end   = pTag + 16 + (*(uint16 *)pTag << 4);

	// parse vifcodes
	uint8 *p_code = p_start;
	int i, num_words;
	sint32 *p_source = (sint32 *)pArray;

	do
	{
		if ((*(uint32 *)p_code & 0x7E000000) == 0x64000000)
		{
			num_words = (int)((((uint32)p_code[2]-1)&0xFF)+1) << 1;

			if ((p_code[3] & 0x7F) == 0x64)
			{
				// 32-bit
				sint32 *p_dest = (sint32 *)(p_code+4);
				for (i=0; i<num_words; i++)
				{
					*p_dest++ = *p_source++;
				}
			}
			else
			{
				// 16-bit
				sint16 *p_dest = (sint16 *)(p_code+4);
				for (i=0; i<num_words; i++)
				{
					*p_dest++ = *p_source++;
				}
			}
		}

		p_code=vif::NextCode(p_code);
	}
	while (p_code < p_end);
}


void dma::TransformSTs(uint8 *pTag, const Mth::Matrix &mat)
{
	// get start and end of dma packet
	uint8 *p_start = pTag + 8;
	uint8 *p_end   = pTag + 16 + (*(uint16 *)pTag << 4);

	// parse vifcodes
	uint8 *p_code = p_start;
	int i, num_verts;
	Mth::Vector texcoords;
	
	do
	{
		if ((p_code[3] & 0x6E) == 0x64)
		{
			num_verts = (int)((((uint32)p_code[2]-1)&0xFF)+1);

			if ((p_code[3] & 0x6F) == 0x64)
			{
				// 32-bit float st's
				float *p_coords = (float *)(p_code+4);
				for (i=0; i<num_verts; i++)
				{
					texcoords[0] = p_coords[0];
					texcoords[1] = p_coords[1];
					texcoords[2] = 0.0f;
					texcoords[3] = 1.0f;
					texcoords *= mat;
					*p_coords++ = texcoords[0];
					*p_coords++ = texcoords[1];
					//printf("(%g, %g)\n", texcoords[0], texcoords[1]);
				}
			}
			else
			{
				// 16-bit fixed uv's
				sint16 *p_coords = (sint16 *)(p_code+4);
				for (i=0; i<num_verts; i++)
				{
					texcoords[0] = (float)(p_coords[0]-0x2000);
					texcoords[1] = (float)(p_coords[1]-0x2000);
					texcoords[2] = 0.0f;
					texcoords[3] = 1024.0f;		// this should really be "texture width * 16",
												// but we don't have access to texture width here
					texcoords *= mat;
					*p_coords++ = (sint16)texcoords[0] + 0x2000;
					*p_coords++ = (sint16)texcoords[1] + 0x2000;
					//printf("(%g, %g)\n", texcoords[0], texcoords[1]);
				}
			}
		}

		p_code=vif::NextCode(p_code);
	}
	while (p_code < p_end);
}



void dma::SqueezeADC(uint8 *pTag)
{
	uint8  *p_code, *p_end, *p_unpack[5];
	uint16 addr, vumem_size;
	uint32 nloop=0, nreg=0, texcrds_size, *p_giftag=NULL, i, num_squeezed, addr_diff;
	uint32 *p_texcrds_source, *p_texcrds_dest;
	uint8  *p_weights_source, *p_weights_dest;
	uint16 *p_normal_source,  *p_normal_dest;
	uint32 *p_colour_source,  *p_colour_dest;
	uint32 *p_coords_source,  *p_coords_dest;
	int    unpack_num;
	bool   seenMSCAL;

	// get start and end of vifcode packet
	p_code = pTag + 8;
	p_end  = pTag + 16 + (*(uint16 *)pTag << 4);

	// initialise state
	addr_diff = 0;
	unpack_num = -1;
	seenMSCAL  = true;

	// parse vifcodes
	while (p_code < p_end)
	{
		if ((p_code[3] & 0x60) == 0x60)		// if it's an unpack
		{
			// adjust ADDR field
			addr = *(uint16 *)p_code;
			addr -= addr_diff;
			addr &= 0x3FF;
			*(uint16 *)p_code = *(uint16 *)p_code & 0xFC00 | addr;

			// if we have a VU1_ADR(Jump) in a giftag, reset offset
			if (((uint16 *)p_code)[1]==0x6C01 && ((uint32 *)p_code)[1]==0x00008000 && ((uint32 *)p_code)[2]==0x00000052)
				addr_diff = 0;

			// look for vertex packets
			if (vif::WL==1 && vif::CL>1 && seenMSCAL)
			{
				// look for giftag
				if (unpack_num==-1)
				{
					p_giftag = (uint32 *)p_code + 1;
					nloop = p_giftag[0] & 0x7FFF;
					nreg  = p_giftag[1] >> 28;
				}

				// look for vertex elements
				else
				{
					p_unpack[unpack_num] = p_code;
				}

				// next element
				unpack_num++;

			}

		}
		else
		{
			if ((p_code[3] & 0x7F) == 0x14)		// mscal
			{
				seenMSCAL = true;
			}
		}

		// step to next vifcode
		p_code = vif::NextCode(p_code);

		// have we found all 5 unpacks of a vertex packet?
		if (unpack_num==5)
		{
			// perform compression...

			// set element pointers
			p_texcrds_source = p_texcrds_dest = (uint32 *)(p_unpack[0]+4);
			p_weights_source = p_weights_dest = (uint8  *)(p_unpack[1]+4);
			p_normal_source	 = p_normal_dest  = (uint16 *)(p_unpack[2]+4);
			p_colour_source	 = p_colour_dest  = (uint32 *)(p_unpack[3]+4);
			p_coords_source	 = p_coords_dest  = (uint32 *)(p_unpack[4]+4);

			// set datasize for tex coords
			texcrds_size = ((p_unpack[0][3] & 0x07) == 0x04) ? 2 : 1;

			// loop over source vertices
			num_squeezed = 0;
			for (i=0; i<nloop; i++)
			{
				// skip if vertex is redundant
				if (   (i<=nloop-3)	&& (p_coords_source[1] & 0x80000000)
									&& (p_coords_source[3] & 0x80000000)
									&& (p_coords_source[5] & 0x80000000)
					|| (i==nloop-2)	&& (p_coords_source[1] & 0x80000000)
									&& (p_coords_source[3] & 0x80000000)
					|| (i==nloop-1)	&& (p_coords_source[1] & 0x80000000))
				{
					p_texcrds_source += texcrds_size;
					p_weights_source += 3;
					p_normal_source  += 3;
					p_colour_source  += 1;
					p_coords_source	 += 2;
					num_squeezed++;
				}
				else if (p_coords_source != p_coords_dest)
				// copy vertex
				{
					*p_texcrds_dest++ = *p_texcrds_source++;
					if (texcrds_size==2)
						*p_texcrds_dest++ = *p_texcrds_source++;
					*p_weights_dest++ = *p_weights_source++;
					*p_weights_dest++ = *p_weights_source++;
					*p_weights_dest++ = *p_weights_source++;
					*p_normal_dest++  = *p_normal_source++;
					*p_normal_dest++  = *p_normal_source++;
					*p_normal_dest++  = *p_normal_source++;
					*p_colour_dest++  = *p_colour_source++;
					*p_coords_dest++  = *p_coords_source++;
					*p_coords_dest++  = *p_coords_source++;
				}
				else
				// just inc pointers
				{
					p_texcrds_source += texcrds_size;
					p_weights_source += 3;
					p_normal_source  += 3;
					p_colour_source  += 1;
					p_coords_source	 += 2;
					p_texcrds_dest	 += texcrds_size;
					p_weights_dest	 += 3;
					p_normal_dest	 += 3;
					p_colour_dest	 += 1;
					p_coords_dest	 += 2;
				}
			}

			// reduce nloop
			nloop -= num_squeezed;

			// make sure there are at least 2 vertices left
			if (nloop < 2)
			{
				num_squeezed -= 2-nloop;
				p_texcrds_dest += (2-nloop) * texcrds_size;
				p_weights_dest += (2-nloop) * 3;
				p_normal_dest  += (2-nloop) * 3;
				p_colour_dest  += (2-nloop) * 1;
				p_coords_dest  += (2-nloop) * 2;
				nloop = 2;
			}

			// adjust SIZE fields in unpacks
			for (i=0; i<5; i++)
			{
				p_unpack[i][2] = nloop;
			}

			// pad the dead space after each unpack with vif NOPs
			for (i=0; i<num_squeezed; i++)
			{
				*p_texcrds_dest++ = 0;
				if (texcrds_size==2)
					*p_texcrds_dest++ = 0;
				*p_weights_dest++ = 0;
				*p_weights_dest++ = 0;
				*p_weights_dest++ = 0;
				*p_normal_dest++  = 0;
				*p_normal_dest++  = 0;
				*p_normal_dest++  = 0;
				*p_colour_dest++  = 0;
				*p_coords_dest++  = 0;
				*p_coords_dest++  = 0;
			}

			// adjust NLOOP and SIZE in giftag
			p_giftag[0] = p_giftag[0] & 0xFFFF8000 | nloop;
			p_giftag[3] = nloop*nreg;

			// accumulate savings
			addr_diff += num_squeezed * 5;

			// reset for next vertex packet
			unpack_num = -1;
			seenMSCAL  = false;
		}

	}

	// adjust vu-mem size in dma tag
	vumem_size = ((uint16 *)pTag)[1];
	vumem_size -= addr_diff;
	vumem_size &= 0x3FF;
	((uint16 *)pTag)[1] = ((uint16 *)pTag)[1] & 0xFC00 | vumem_size;

}




void dma::SqueezeNOP(uint8 *pTag)
{
	uint8  *p_code, *p_end;
	uint32 *p_source, *p_dest;

	// get start and end of vifcode packet
	p_code = pTag + 8;
	p_end  = pTag + 16 + (*(uint16 *)pTag << 4);

	// setup
	p_source = p_dest = (uint32 *)p_code;

	// parse vifcodes
	while (p_code < p_end)
	{
		if (p_code[3] == 0x00)	// NOP
		{
			p_code = vif::NextCode(p_code);
			p_source++;
		}
		else
		{
			p_code = vif::NextCode(p_code);

			// copy memory
			while (p_source < (uint32 *)p_code)
			{
				*p_dest++ = *p_source++;
			}
		}

	}

	// pad to qword boundary with nops
	while ((uint)p_dest & 0xF)
		*p_dest++ = 0;

	// adjust dma size in dma tag
	((uint16 *)pTag)[0] = ((uint8 *)p_dest - pTag - 16) >> 4;
}




//---------------------------------
//		S T A T I C   D A T A
//---------------------------------

uint8 *		dma::pBase;			// base of dynamic DMA buffer for this frame
uint8 *		dma::pLoc;			// current position in it that we are building DMA packets
uint8 *		dma::pTag;
uint8 *		dma::pPrebuiltBuffer;
uint8 *		dma::pDummyBuffer;		// (Mick) used to simulate memory usage
uint8 *		dma::pRuntimeBuffer;
uint8 *		dma::pList[2];
uint64 *	dma::Gosubs;
uint8 *		dma::pSub;
dma::eTag	dma::ID;
int    		dma::sp;
uint8 *		dma::Stack[2];
void *		dma::sp_group;
int			dma::size;

} // namespace NxPs2

