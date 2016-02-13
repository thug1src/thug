#ifndef __GROUP_H
#define __GROUP_H


namespace NxPs2
{


#define GROUPFLAG_TRANSPARENT	(1<<0)
#define GROUPFLAG_SKY			(1<<1)
#define GROUPFLAG_SORT			(1<<2)


class CVu1Context;
struct STransformContext;


struct sGroup
{
public:
	sGroup *pNext;

	float  Priority;
	uint32 Checksum;
	uint32 flags;
	uint8  *pUpload[2];
	uint8  *pRender[2];
	uint8  *pListEnd[2];
	bool	Used[2];			// flag saying if this group is actually used this frame (has something in the pRender data)
	struct sMesh  *pMeshes;
	int    NumMeshes;
	uint   vu1_loc;
	uint8  *p_tag;

	uint32 VramStart;
	uint32 VramEnd;

	struct sScene *pScene;

	sGroup *pPrev;

	static sGroup *pHead;
	static sGroup *pTail;
	static sGroup *pShadow;
	static sGroup *pFog;
	static sGroup *pParticles;
	static sGroup *pEpilogue;
	volatile static sGroup *pUploadGroup;
	volatile static sGroup *pRenderGroup;
	static uint32 VramBufferBase;
	//STransformContext *pTransformContext;
	CVu1Context *pVu1Context;
	uint32	profile_color;					// color we display it as in the profiler
};

} // namespace NxPs2

#endif // __GROUP_H
