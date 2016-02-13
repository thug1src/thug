/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		THPS4													**
**																			**
**	Module:			NxPs2					 								**
**																			**
**	File name:		geometry.h												**
**																			**
**	Created by:		02/08/02	-	mrd										**
**																			**
**	Description:	classes for PS2 geometry								**
**																			**
*****************************************************************************/

#ifndef __GFX_NGPS_NX_GEOMETRY_H
#define __GFX_NGPS_NX_GEOMETRY_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

//#include "vu1context.h"

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define MAX_LOD_DIST			(10000000.0f)

namespace Mth
{
	class CBBox;
}

namespace NxPs2
{
	// Forward declarations
	struct	sGroup;
	struct  sTexture;
	class	CLightGroup;
	class	CVu1Context;

// For the flags, note the visibility bits that are reserved below
#define VISIBILITY_FLAG_BIT		(24)
#define NUM_VISIBILITY_BITS		(8)
//#define ALL_VISIBILITY_BITS		(((1 << NUM_VISIBILITY_BITS) - 1) << VISIBILITY_FLAG_BIT)
#define ALL_VISIBILITY_BITS		((0xFFFFFFFF >> (32 - NUM_VISIBILITY_BITS)) << VISIBILITY_FLAG_BIT)

#define NODEFLAG_ACTIVE			(1<<0)
#define NODEFLAG_LEAF			(1<<1)
#define NODEFLAG_OBJECT			(1<<2)
#define NODEFLAG_TRANSFORMED	(1<<3)
#define NODEFLAG_COLOURED		(1<<4)
#define NODEFLAG_SKY			(1<<5)
#define NODEFLAG_ZPUSH0			(1<<6)
#define NODEFLAG_ZPUSH1			(1<<7)
#define NODEFLAG_NOSHADOW		(1<<8)
#define NODEFLAG_UVWIBBLE		(1<<9)
#define NODEFLAG_VCWIBBLE		(1<<10)
#define NODEFLAG_SKINNED		(1<<11)
#define NODEFLAG_SKELETAL		(1<<11)
#define NODEFLAG_INSTANCE		(1<<12)
#define NODEFLAG_OBJECT_LIGHTS	(1<<13)
#define NODEFLAG_ENVMAPPED		(1<<14)
#define NODEFLAG_BILLBOARD		(1<<15)
#define NODEFLAG_EXPLICIT_UVWIBBLE		(1<<16)
#define NODEFLAG_ZPUSH2			(1<<17)
#define NODEFLAG_ZPUSH3			(1<<18)
#define NODEFLAG_BLACKFOG		(1<<19)

#define NODEFLAG_LAST_OCCLUSION_VALID	(1 << 20)
#define NODEFLAG_LAST_OCCLUSION_TRUE	(1 << 21)

#ifdef	__NOPT_ASSERT__
#define NODEFLAG_WAS_RENDERED	(1 << 22) // True if rendered last frame.  Not always valid for leaf nodes
#endif

#if 0
#define NODEFLAG_VISIBILITY_TEST_PERFORMED		( 1 << 22 )
#define NODEFLAG_VISIBILITY_TEST_NOT_VISIBLE	( 1 << 23 )
#endif

#define NODEFLAG_VISIBILITY_0	(1<<(VISIBILITY_FLAG_BIT + 0))
#define NODEFLAG_VISIBILITY_1	(1<<(VISIBILITY_FLAG_BIT + 1))
#define NODEFLAG_VISIBILITY_2	(1<<(VISIBILITY_FLAG_BIT + 2))
#define NODEFLAG_VISIBILITY_3	(1<<(VISIBILITY_FLAG_BIT + 3))
#define NODEFLAG_VISIBILITY_4	(1<<(VISIBILITY_FLAG_BIT + 4))
#define NODEFLAG_VISIBILITY_5	(1<<(VISIBILITY_FLAG_BIT + 5))
#define NODEFLAG_VISIBILITY_6	(1<<(VISIBILITY_FLAG_BIT + 6))
#define NODEFLAG_VISIBILITY_7	(1<<(VISIBILITY_FLAG_BIT + 7))

#define NODEFLAG_ZPUSH			(NODEFLAG_ZPUSH0 | NODEFLAG_ZPUSH1 | NODEFLAG_ZPUSH2 | NODEFLAG_ZPUSH3)

#define LOADFLAG_RENDERNOW		(1<<0)
#define LOADFLAG_SKY			(1<<1)

// put these here temporarily
#define RENDERFLAG_CULL			(1<<0)
#define RENDERFLAG_CLIP			(1<<1)
#define RENDERFLAG_SHADOW		(1<<2)
#define RENDERFLAG_COLOURED		(1<<3)
#define RENDERFLAG_TRANSFORMED	(1<<4)
#define RENDERFLAG_SKELETAL		(1<<5)


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

// from CClass, so they get zeroed automatically 
class CGeomMetrics : public Spt::Class
{
public:
	int 	m_total;
	int		m_leaf;
	int		m_object;
	int		m_verts;
	int		m_polys;
};


class CGeomNode
{
public:

	#ifdef __PLAT_NGPS__
#	if 0
	void				RecursiveVisibilityTest( CVu1Context& ctxt, uint32 renderFlags );
#	endif
	void				RenderWorld(CVu1Context& ctxt, uint32 renderFlags);
	void				RenderScene(CVu1Context& ctxt, uint32 renderFlags);
	void				RenderObject(CVu1Context& ctxt, uint32 renderFlags);
	void				RenderTransformedLeaf(CVu1Context& ctxt, uint32 renderFlags);
	void				RenderNonTransformedLeaf(CVu1Context& ctxt, uint32 renderFlags);
	void				RenderMinimalLeaf(CVu1Context& ctxt);
	void				RenderAsSky(CVu1Context& ctxt, uint32 renderFlags);
	void 				SetBoneTransforms(Mth::Matrix *pMat);

	// These modify the actual DMA data
	void				Translate(const Mth::Vector & delta_trans);		// delta since we don't store the original pos
	void				RotateY(const Mth::Vector & world_origin, Mth::ERot90 rot_y, bool root = true);	// don't set root
	void				Scale(const Mth::Vector & world_origin, const Mth::Vector & delta_scale, bool root = true); // delta since we don't store the original scale

	void 				GetVerts(Mth::Vector *p_verts, bool root = true);		// don't set root on any of these
	void 				GetColors(uint32 *p_colors, bool root = true);
	void 				SetVerts(Mth::Vector *p_verts, bool root = true);
	void 				SetColors(uint32 *p_colors, bool root = true);
	#endif

	bool				IsLeaf();
	bool				IsActive();
	bool				IsObject();
	bool				IsTransformed();
	bool				IsSky();
	bool				IsColored();
	bool				IsSkeletal();
	bool				IsEnvMapped();
	bool				IsUVWibbled();
	bool				IsBillboard();

#ifdef	__NOPT_ASSERT__
	bool				WasRendered();
#else
	bool				WasRendered() {return false;};	
#endif
	

	void				SetLeaf(bool yes);
	void				SetActive(bool yes);
	void				SetObject(bool yes);
	void				SetTransformed(bool yes);
	void				SetSky(bool yes);
	void				SetZPush0(bool yes);
	void				SetZPush1(bool yes);
	void				SetZPush2(bool yes);
	void				SetZPush3(bool yes);
	void				SetNoShadow(bool yes);
	void				SetUVWibbled(bool yes);
	void				SetVCWibbled(bool yes);
	void				SetColored(bool yes);
	void				SetSkinned(bool yes);
	void				SetBillboard(bool yes);
	void				SetSkeletal(bool yes);
	void				SetEnvMapped(bool yes);
	void				SetBlackFog(bool yes);

	void				SetChecksum(uint32 checksum);
	void				SetBoundingSphere(float x, float y, float z, float R);	// change args to const Mth::Vector sphere
	void				SetBoundingBox(float x, float y, float z);				// change args to const Mth::Vector box
	void				SetDma(uint8 *pDma);
	void				SetDmaTag(uint8 *pDma);
	void				SetChild(CGeomNode *pChild);
	void				AddChild(CGeomNode *pChild);
	void				SetSibling(CGeomNode *pSibling);
	void				SetSlaveLOD(CGeomNode *pLOD);
	void				SetLODFarDist(float LOD_far_dist);
	void				SetGroup(sGroup *pGroup);
	void				SetMatrix(Mth::Matrix *p_mat);
	void				SetUVWibblePointer(float *pData);
	void				SetVCWibblePointer(uint32 *pData);
	void				SetColor(uint32 rgba);
	void				SetLightGroup(CLightGroup *p_light_group);
	void				SetVisibility(uint8 mask);
	void				SetBoneIndex(sint8 index);

	// Texture pointer starts off as a texture checksum in SceneConv
	void				SetTextureChecksum(uint32 checksum) {u4.mp_texture = (sTexture*)checksum;}
	uint32				GetTextureChecksum() {return (uint32)u4.mp_texture;}
	#ifdef __PLAT_NGPS__
	void				SetTexture(sTexture *p_texture) {u4.mp_texture = p_texture;}
	sTexture *			GetTexture() {return u4.mp_texture;}
	#endif

	void				SetUVWibbleParams(float u_vel, float u_amp, float u_freq, float u_phase,
										  float v_vel, float v_amp, float v_freq, float v_phase);
	void				UseExplicitUVWibble(bool yes);
	void				SetUVWibbleOffsets(float u_offset, float v_offset);

	void				SetUVOffset(uint32 material_name, int pass, float u_offset, float v_offset);
	void 				SetUVMatrix(uint32 material_name, int pass, Mth::Matrix &mat);
	
	uint32				GetChecksum();
	Mth::Matrix&		GetMatrix();
	CGeomNode*			GetChild();
	CGeomNode*			GetSibling();
	uint8*				GetDma();
	sGroup*				GetGroup();
	uint32				GetColor();
	float				GetLODFarDist();
	CLightGroup *		GetLightGroup();
	uint8				GetVisibility();
	Mth::Vector			GetBoundingSphere();
	Mth::CBBox			GetBoundingBox();
	sint8				GetBoneIndex();

	uint32				GetFlags(){return m_flags;}

	void				CountMetrics(CGeomMetrics *p_metrics);
	void				RenderWireframe(int mode);
	void				RenderWireframeRecurse(int mode, int &lines);
	
								   
	int					GetNumBones() { return m_num_bones; }
	int					GetNumVerts();
	int					GetNumPolys();
	int					GetNumBasePolys();

	// To get at the different objects in a heirarchy
	int					GetNumObjects();
	CGeomNode*			GetObject(int num, bool root = true);		// Don't set root


	#ifdef __PLAT_WN32__
	void 				Preprocess(uint8 *p_baseAddress);
	#endif

	#ifdef __PLAT_NGPS__
	static CGeomNode*	sProcessInPlace(uint8 *pPipData, uint32 loadFlags);
	static void *		sGetHierarchyArray(uint8 *pPipData, int& size);
	void				AddToTree(uint32 loadFlags);
	void				Cleanup();
	
	CGeomNode*			CreateInstance(Mth::Matrix *pMat, CGeomNode *p_parent = NULL);
	CGeomNode*			CreateInstance(Mth::Matrix *pMat, int numBones, Mth::Matrix *pBoneTransforms, CGeomNode *p_parent = NULL);
	void				DeleteInstance();
	CGeomNode*			CreateCopy(CGeomNode *p_parent = NULL, bool root = true);				// Don't set root
	void				DeleteCopy(bool root = true);											// Don't set root
	#endif
	
	bool				CullAgainstViewFrustum();
	bool				CullAgainstOuterFrustum();


	void				Init();

						CGeomNode();
						CGeomNode(const CGeomNode &node);
						~CGeomNode() {}


						static CGeomNode	sWorld;
						static CGeomNode	sDatabase;


private:

	#ifdef __PLAT_NGPS__
	bool				SphereIsOutsideViewVolume(Mth::Vector &s);
	bool				SphereIsInsideOuterVolume(Mth::Vector &s);
	bool				BoxIsOutsideViewVolume(Mth::Vector& b, Mth::Vector& s);
	bool				NeedsClipping(Mth::Vector &s);
	void				LinkDma(uint8 *p_newTail);
	int	 				ProcessNodeInPlace(uint8 *p_baseAddress);
	void 				BecomesChildOf(CGeomNode *p_parent);
	bool				RemoveFrom(CGeomNode *p_parent);
	bool				RemoveFromChildrenOf(CGeomNode *p_parent);
	void 				WibbleVCs();

	void 				recalculate_bounding_data(Mth::Vector *p_verts, int num_verts);
	#endif

	Mth::Vector			m_bounding_sphere;
	float				m_bounding_box[3];
	uint32				m_flags;
	union
	{
		CGeomNode *		mp_child;			// internal
		uint8 *			mp_dma;				// leaf
	} u1;
	union
	{
		Mth::Matrix *	mp_transform;		// internal
		uint32			m_dma_tag_lo32;		// leaf
	} u2;
	CGeomNode *			mp_sibling;
	union
	{
		sGroup *		mp_group;			// leaf
		CLightGroup *	mp_light_group;		// optional group of lights
	} u3;
	uint32				m_checksum;
	float *				mp_uv_wibble;		// leaf
	uint32 *			mp_vc_wibble;		// leaf
	uint32				m_colour;
	// used for skeletal models...
	sint8				m_num_bones;		// skeletal, internal, instance
	sint8				m_bone_index;		// skeletal, internal, source
	uint16				m_field;			// skeletal, internal, instance
	union 
	{
		Mth::Matrix *		mp_bone_transforms;	// skeletal, internal, instance
		sTexture	*		mp_texture;			// Level Geometry leaf- pointer to the texture we use
	} u4;
	float				m_LOD_far_dist;
	CGeomNode *			mp_next_LOD;
											// stay quadword-aligned with padding if necessary
};


// Hierarchy information available to game.  It is found at the beginning of a geom.ps2 file,
// although it is not used by the CGeomNodes.
class CHierarchyObject
{
public:
						CHierarchyObject();
						~CHierarchyObject();

	void				SetChecksum(uint32 checksum);
	uint32				GetChecksum() const;
	void				SetParentChecksum(uint32 checksum);
	uint32				GetParentChecksum() const;
	void				SetParentIndex(int16 index);
	int16				GetParentIndex() const;
	void				SetBoneIndex(sint8 index);
	sint8				GetBoneIndex() const;

	void				SetSetupMatrix(const Mth::Matrix& mat);
	const Mth::Matrix &	GetSetupMatrix() const;

	
protected:
	uint32				m_checksum;			// Object checksum
	uint32				m_parent_checksum;	// Checksum of parent, or 0 if root object
	int16				m_parent_index;		// Index of parent in the hierarchy array (or -1 if root object)
	sint8				m_bone_index;		// The index of the bone matrix used on this object
	uint8				m_pad_8;
	uint32				m_pad_32;
	Mth::Matrix			m_setup_matrix;		// Initial local to parent matrix
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


//extern CGeomNode	*pShadowList[SHADOW_LIST_SIZE];
extern uint			NumShadowListEntries;

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

} // namespace NxPs2

#endif	// __GFX_NGPS_NX_GEOMETRY_H

