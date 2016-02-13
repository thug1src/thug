#ifndef __DMA_H
#define __DMA_H

#include "render.h"


namespace NxPs2
{


class CGeomNode;



#define PREBUILT_DMA_BUFFER_SIZE 32768		    		

// Random crashes in the engine can frequntly be traced to DMA buffer overflow
// We probably need to add some cheking for this.
//#define SPLIT_SCREEN_DMA_BUFFER_SIZE  	(675000 * 2)			// split screen needs more
//#define NON_DEBUG_DMA_BUFFER_SIZE  		(500000 * 2)			// non split screen needs less
#define NON_DEBUG_DMA_BUFFER_SIZE  		((int)((675000 * 2)&0xffffffe0))			// non split screen needs less
#define DEBUG_DMA_BUFFER_SIZE  			((int)(16256000 & 0xffffffe0))				// Mick, 12 MB for debugging....

class dma
{

public:

	enum eTag
	{	refe = 0,
		cnt  = 1,
		next = 2,
		ref  = 3,
		refs = 4,
		call = 5,
		ret  = 6,
		end  = 7
	};

	struct SSortElement
	{
		float z;
		uint8 *address;
	};

	//-------------------------------------------
	//		S T A T I C   F U N C T I O N S
	//-------------------------------------------

	static void   Align(uint Offset, uint Boundary);
	static void   Align();
	static void   Store32(uint32 Data);
	static void   Store32(uint32 Data1, uint32 Data2 );
	static void   Store32(uint32 Data1, uint32 Data2, uint32 Data3 );
	static void   Store32(uint32 Data1, uint32 Data2, uint32 Data3, uint32 Data4 );
	static void   Store64(uint64 Data);
	static void   Tag(eTag ID, uint QWC, uint ADDR);
	static void   BeginTag(eTag ID, uint ADDR);
	static void   EndTag(void);
	static void   BeginSub(eTag ID);
	static uint64 EndSub(void);
	static void   Gosub(uint Num, uint Path);
	static void   BeginSub3D(void);
	static uint8 *EndSub3D(void);
	static void   Gosub3D(uint8 *pSub, uint RenderFlags);
	static uint8 *NextTag(uint8 *pTag, bool stepInto);
	static int    Cmp(const void *p1, const void *p2);
	static uint8 *SortGroup(uint8 *pList);
	static void   BeginList(void *pGroup);
	static void   EndList(void *pGroup);
	static void   SetList(void *pGroup);
	static void   ReallySetList(void *pGroup);
	static int    GetDmaSize(uint8 *pTag);
	static int    GetNumVertices(uint8 *pTag);
	static int    GetNumTris(uint8 *pTag);
	static void   Copy(uint8 *pTag, uint8 *pDest);
	static int    GetBitLengthXYZ(uint8 *pTag);
	static void   TransferValues(uint8 *pTag, uint8 *pArray, int size, int dir, uint32 vifcodeMask, uint32 vifcodePattern);
	static void   TransferColours(uint8 *pTag, uint8 *pArray, int dir);
	static void   ExtractXYZs(uint8 *pTag, uint8 *pArray);
	static void   ReplaceXYZs(uint8 *pTag, uint8 *pArray, bool skipW = false);
	static void   ExtractRGBAs(uint8 *pTag, uint8 *pArray);
	static void   ReplaceRGBAs(uint8 *pTag, uint8 *pArray);
	static void   ExtractSTs(uint8 *pTag, uint8 *pArray);
	static void   ReplaceSTs(uint8 *pTag, uint8 *pArray);
	static void   TransformSTs(uint8 *pTag, const Mth::Matrix &mat);
	static void	  ConvertXYZToFloat(Mth::Vector &vec, sint32 *p_xyz);
	static void	  ConvertXYZToFloat(Mth::Vector &vec, sint32 *p_xyz, const Mth::Vector &center);
	static void	  ConvertXYZToFloat(Mth::Vector &vec, sint32 *p_xyz, const sint32 *p_center);
	static void	  ConvertSTToFloat(float & s, float & t, sint32 *p_st);
	static void	  ConvertFloatToXYZ(sint32 *p_xyz, Mth::Vector &vec);
	static void	  ConvertFloatToXYZ(sint32 *p_xyz, Mth::Vector &vec, const Mth::Vector &center);
	static void	  ConvertFloatToXYZ(sint32 *p_xyz, Mth::Vector &vec, const sint32 *p_center);
	static void	  ConvertFloatToST(sint32 *p_st, float & s, float & t);
	static void   SqueezeADC(uint8 *pTag);
	static void   SqueezeNOP(uint8 *pTag);


	//---------------------------------
	//		S T A T I C   D A T A
	//---------------------------------

	static uint8 *		pBase;
	static uint8 *		pLoc;
	static uint8 *		pTag;
	static uint8 *		pPrebuiltBuffer;
	static uint8 *		pDummyBuffer;
	static uint8 *		pRuntimeBuffer;
	static uint8 *		pList[2];
	static uint64 *		Gosubs;
	static uint8 *		pSub;
	static eTag			ID;
	static int			sp;
	static uint8 *		Stack[2];
	static void *		sp_group;
	static int			size;

}; // class dma


// -------------------------------------------------
//        INLINE FUNCTIONS
// -------------------------------------------------



// align to Boundary, then add Offset
// Boundary must be a power of 2

inline void dma::Align(uint Offset, uint Boundary)
{
	uint8 *NewDmaLoc = (uint8 *)((((uint)pLoc - Offset + Boundary - 1) & ((uint)(-(int)Boundary))) + Offset);
	while (pLoc < NewDmaLoc)
		*pLoc++ = 0;
}


// quick version for dma list building; assumes pLoc already word-aligned and you want it quadword-aligned

inline void dma::Align()
{
	while ((uint)pLoc & 0xF)
	{
		*(uint32 *)pLoc = 0;
		pLoc += 4;
	}
}





// store a word

inline void dma::Store32(uint32 Data)
{
	#if 0
	*(uint32 *)pLoc = Data;
	pLoc += 4;
	#else
	uint32 *p_loc = (uint32*)pLoc;
	p_loc[0] = Data;
	pLoc = (uint8*) (p_loc + 1);
	#endif	
}

// Store two words, quicker this way, as we only have to update pLoc once
inline void dma::Store32(uint32 Data1, uint32 Data2)
{
	uint32 *p_loc = (uint32*)pLoc;
	p_loc[0] = Data1;
	p_loc[1] = Data2;
	pLoc = (uint8*) (p_loc + 2);
}

// Store three words, quicker this way, as we only have to update pLoc once
inline void dma::Store32(uint32 Data1, uint32 Data2, uint32 Data3)
{
	uint32 *p_loc = (uint32*)pLoc;
	p_loc[0] = Data1;
	p_loc[1] = Data2;
	p_loc[2] = Data3;
	pLoc = (uint8*) (p_loc + 3);
}

// Store four words, quicker this way, as we only have to update pLoc once
inline void dma::Store32(uint32 Data1, uint32 Data2, uint32 Data3, uint32 Data4)
{
	uint32 *p_loc = (uint32*)pLoc;
	p_loc[0] = Data1;
	p_loc[1] = Data2;
	p_loc[2] = Data3;
	p_loc[3] = Data4;
	pLoc = (uint8*) (p_loc + 4);
}


// store a dword

inline void dma::Store64(uint64 Data)
{
	((uint32 *)pLoc)[0] = (uint32)Data;		// break into 2 words
	((uint32 *)pLoc)[1] = (uint32)(Data>>32);	// because pLoc is only word-aligned
	pLoc += 8;
}



//--------------------------
//		D M A   T A G S
//--------------------------



// generic source chain tag
//
//	 63  62                        32 31  30  28 27 26 25     16 15            0
//	ÚÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
//	³SPR³          ADDR          0000³IRQ³  ID  ³ PCE ³    -    ³      QWC      ³
//	ÀÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

inline void dma::Tag(eTag ID, uint QWC, uint ADDR)
{
	// assumes pLoc already qword aligned
	pTag = pLoc;
	Store32(ID<<28 | QWC, ADDR);
}


// Begin-End style

inline void dma::BeginTag(eTag ID, uint ADDR)
{
	// assumes pLoc already qword aligned
	pTag = pLoc;									// record tag location for patching later
	Store32(ID<<28, ADDR);
}


// Begin-End style

inline void dma::EndTag(void)
{
	uint ID = (*(uint32 *)pTag >> 28) & 7;				// get the tag ID field
	Align();											// align to qword boundary
	if (ID!=ref && ID!=refe && ID!=refs)				// check that the QWC is patchable
	{
		((uint16 *)pTag)[0] = (pLoc - pTag - 8) >> 4;
	}
}


inline void	 dma::ConvertXYZToFloat(Mth::Vector &vec, sint32 *p_xyz)
{
	vec[X] = ((float) *(p_xyz++)) * RECIPROCAL_SUB_INCH_PRECISION;
	vec[Y] = ((float) *(p_xyz++)) * RECIPROCAL_SUB_INCH_PRECISION;
	vec[Z] = ((float) *(p_xyz)  ) * RECIPROCAL_SUB_INCH_PRECISION;
	vec[W] = 1.0f;
}

inline void	 dma::ConvertXYZToFloat(Mth::Vector &vec, sint32 *p_xyz, const Mth::Vector &center)
{
	vec[X] = (((float) *(p_xyz++)) * RECIPROCAL_SUB_INCH_PRECISION) + center[X];
	vec[Y] = (((float) *(p_xyz++)) * RECIPROCAL_SUB_INCH_PRECISION) + center[Y];
	vec[Z] = (((float) *(p_xyz)  ) * RECIPROCAL_SUB_INCH_PRECISION) + center[Z];
	vec[W] = 1.0f;
}

inline void	 dma::ConvertXYZToFloat(Mth::Vector &vec, sint32 *p_xyz, const sint32 *p_center)
{
	vec[X] = ((float) (*(p_xyz++) + *(p_center++))) * RECIPROCAL_SUB_INCH_PRECISION;
	vec[Y] = ((float) (*(p_xyz++) + *(p_center++))) * RECIPROCAL_SUB_INCH_PRECISION;
	vec[Z] = ((float) (*(p_xyz)   + *(p_center)  )) * RECIPROCAL_SUB_INCH_PRECISION;
	vec[W] = 1.0f;
}

inline void	 dma::ConvertSTToFloat(float & s, float & t, sint32 *p_st)
{
	s = ((float) *(p_st++)) * (1.0f / 4096.0f);
	t = ((float) *(p_st)  ) * (1.0f / 4096.0f);
}

inline void	 dma::ConvertFloatToXYZ(sint32 *p_xyz, Mth::Vector &vec)
{
	*(p_xyz++) = (sint32) (vec[X] * SUB_INCH_PRECISION);
	*(p_xyz++) = (sint32) (vec[Y] * SUB_INCH_PRECISION);
	*(p_xyz)   = (sint32) (vec[Z] * SUB_INCH_PRECISION);
}

inline void	 dma::ConvertFloatToXYZ(sint32 *p_xyz, Mth::Vector &vec, const sint32 *p_center)
{
	*(p_xyz++) = ((sint32) (vec[X] * SUB_INCH_PRECISION)) - *(p_center++);
	*(p_xyz++) = ((sint32) (vec[Y] * SUB_INCH_PRECISION)) - *(p_center++);
	*(p_xyz)   = ((sint32) (vec[Z] * SUB_INCH_PRECISION)) - *(p_center);
}

inline void	 dma::ConvertFloatToST(sint32 *p_st, float & s, float & t)
{
	*(p_st++) = (sint32) (s * 4096.0f);
	*(p_st)   = (sint32) (t * 4096.0f);
}

inline void dma::SetList(void *pGroup)
{
	// do nothing if we're already in the right dma context
	// (Mick) This part has been moved to an inline function 
	// since the overhead of setting up the stack frame 
	// is large, this is a more efficient way of taking advantage of
	// group coherency.   This represents a 25% speed improvmeent for this function
	// just 0.25% of a frame.  But every little helps.
	if (sp_group != pGroup)
	{
		ReallySetList(pGroup);
	}
}

} // namespace NxPs2


#endif // __DMA_H

