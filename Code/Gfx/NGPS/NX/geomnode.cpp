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
**	Module:			geomnode				 								**
**																			**
**	File name:		geomnode.cpp											**
**																			**
**	Created by:		00/00/00	-	mrd										**
**																			**
**	Description:	process-in-place geometry nodes for PS2					**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/math.h>
#include <core/math/geometry.h>
#include <gel/scripting/checksum.h>
//#include <sys/timer.h>
#include "render.h"
#include "group.h"
#include "geomnode.h"
#include "vu1context.h"
#include "occlude.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gs.h"
#include "vu1code.h"
#include "light.h"
#include "line.h"
#include "dmacalls.h"
#include "scene.h"
#include "texture.h"
#include "switches.h"


// Mick - Defining this __GEOM_STATS__ will enable bunch of counters in CGeomNode::Render
// 
#define	__GEOM_STATS__

#define	__ASSERT_ON_ZERO_RADIUS__  0

#define	OCCLUSION_CACHE 0

// Just in case we forgot to take it out, this does it automatically on final burns   
#ifndef	__NOPT_ASSERT__
#undef	__GEOM_STATS__
#endif   
   
#ifdef	__GEOM_STATS__
#define	GEOMSTATINC(x) {x++;}
#else
#define	GEOMSTATINC(x) {}
#endif


//float	gClipDist = 0.1f;

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace NxPs2
{




int geom_stats_total=0;
int geom_stats_inactive =0;
int geom_stats_sky=0;
int geom_stats_transformed=0;
int geom_stats_skeletal=0;
int geom_stats_camera_sphere=0;
int geom_stats_clipcull=0;
int geom_stats_culled=0;
int geom_stats_leaf_culled=0;
int geom_stats_boxcheck=0;
int geom_stats_occludecheck=0;
int geom_stats_occluded=0;
int geom_stats_colored=0;
int geom_stats_leaf=0;
int geom_stats_wibbleUV=0;
int geom_stats_wibbleVC=0;
int geom_stats_sendcontext=0;
int geom_stats_sorted=0;
int geom_stats_shadow=0;



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

//#define	__LOD_MULTIPASS__

#define SHADOW_CAM_FRUSTUM_HALF_WIDTH	64.0f
#define SHADOW_CAM_FRUSTUM_HALF_HEIGHT	64.0f

#define GET_FLAG(FLAG) (m_flags & (FLAG) ? true : false)
#define SET_FLAG(FLAG) m_flags = yes ? m_flags|FLAG : m_flags&~FLAG

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

CGeomNode CGeomNode::sWorld;
CGeomNode CGeomNode::sDatabase;

CGeomNode *GpInstanceNode=NULL;

#ifdef	__NOPT_ASSERT__
// Mick: patch variables for debugging toggle of multi-passes
uint32		gPassMask1 = 0; 		// 1<<6 | 1<<7  (0x40, 0x80)
uint32		gPassMask0 = 0; 		// 1<<6 | 1<<7  (0x40, 0x80)
#endif 

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

#ifdef __PLAT_NGPS__

// This is only used in a couple of places, and it's 2% of CPU time in a release build
// so I'm making it inline for now to improve cache coherency 
// TODO: would be a good candidate for VU0 optimization
// assuming the ViewVolume could be stored in VU mem or regs?
inline bool CGeomNode::SphereIsOutsideViewVolume(Mth::Vector& s)
{
	return
		s[3] < s[2] - render::Near						||
		s[3] < render::sx * s[2] - render::cx * s[0]	||
		s[3] < render::sx * s[2] + render::cx * s[0]	||
		s[3] < render::sy * s[2] + render::cy * s[1] 	||
		s[3] < render::sy * s[2] - render::cy * s[1]	||
		s[3] < render::Far - s[2];
}


// Same with this
inline bool CGeomNode::SphereIsInsideOuterVolume(Mth::Vector& s)
{
	return
		s[3] < render::Near - s[2]						&&
		s[3] < -render::Sx * s[2] + render::Cx * s[0]	&&
		s[3] < -render::Sx * s[2] - render::Cx * s[0]	&&
		s[3] < -render::Sy * s[2] - render::Cy * s[1]	&&
		s[3] < -render::Sy * s[2] + render::Cy * s[1]	&&
		s[3] < s[2] - render::Far;
}


bool CGeomNode::BoxIsOutsideViewVolume(Mth::Vector& b, Mth::Vector& s)
{
	return ((render::FrustumFlagsPlus[0]  && (s[0]+b[0] <= render::CameraToWorld[3][0])) ||
			(render::FrustumFlagsPlus[1]  && (s[1]+b[1] <= render::CameraToWorld[3][1])) ||
			(render::FrustumFlagsPlus[2]  && (s[2]+b[2] <= render::CameraToWorld[3][2])) ||
			(render::FrustumFlagsMinus[0] && (s[0]-b[0] >= render::CameraToWorld[3][0])) ||
			(render::FrustumFlagsMinus[1] && (s[1]-b[1] >= render::CameraToWorld[3][1])) ||
			(render::FrustumFlagsMinus[2] && (s[2]-b[2] >= render::CameraToWorld[3][2])));
}


inline bool CGeomNode::NeedsClipping(Mth::Vector& s)
{
	return true;
}


void CGeomNode::LinkDma(uint8 *p_newTail)
{
	dma::Tag(dma::next, 0, 0);	// will be patched up by next call to LinkDma()
	vif::NOP();
	vif::NOP();
	((uint *)u3.mp_group->pListEnd[render::Field])[1] = (uint)p_newTail;
	u3.mp_group->pListEnd[render::Field] = dma::pLoc - 16;
}

// returns the number of leaf nodes
int CGeomNode::ProcessNodeInPlace(uint8 *p_baseAddress)
{
	int leaves = 0;
	
	if (!(m_flags & NODEFLAG_LEAF))
	{
		leaves++;
		// convert transform offset to pointer
		if ((int)u2.mp_transform != -1)
		{
			u2.mp_transform = (Mth::Matrix *)( (int)u2.mp_transform + p_baseAddress );
		}
		else
		{
			u2.mp_transform = (Mth::Matrix *)0;
		}
	}

	uint32	tex = GetTextureChecksum();
	SetTextureChecksum(0);  			// clear it in case we don't find it

	// convert group checksum to pointer
	if ((m_flags & NODEFLAG_LEAF) && u1.mp_dma)
	{
		sGroup *pGroup;
		for (pGroup=sGroup::pHead; pGroup; pGroup=pGroup->pNext)
			if (pGroup->Checksum == (uint)u3.mp_group)
				break;
		Dbg_MsgAssert(pGroup, ("Couldn't group checksum #%d\n", (uint)u3.mp_group));
		u3.mp_group = pGroup;
		
		// Resolve the group+texture checksum into a texture pointer		
//		printf ("%x\n",GetTextureChecksum());
		if (tex)
		{
			sGroup * p_group = GetGroup();
			sScene * p_scene = p_group->pScene;
			sTexture *p_textures = p_scene->pTextures;
			int num_textures = p_scene->NumTextures;
			for (int i=0;i<num_textures;i++)
			{
				// We are seraching through all the textures in the scene
				// so we will find multiple instances with the same texture checksum
				// so we need to check the group checksum as well
				if (p_textures[i].Checksum == tex && p_textures[i].GroupChecksum == GetGroup()->Checksum)
				{
					//printf ("Found %dx%dx%d %x mips at 0x%x (%d)\n",p_textures[i].GetWidth(),p_textures[i].GetHeight(),p_textures[i].GetClutBitdepth(),p_textures[i].GetNumMipmaps(),*(p_textures+i),i);
					SetTextureChecksum((uint32)(p_textures+i));
					break;  
				}
			}
//			printf ("-> 0x%x\n",GetTextureChecksum());
		}
		
	}

	if (m_flags & NODEFLAG_LEAF)
	{
		// convert dma offset to pointer
		if ((int)u1.mp_dma != -1)
		{
			u1.mp_dma = (uint)u1.mp_dma + p_baseAddress;
		}
		else
		{
			u1.mp_dma = (uint8 *)0;
		}
	}
	else
	{
		// convert child offset to pointer
		if ((int)u1.mp_child != -1)
		{
			u1.mp_child = (CGeomNode *)( (int)u1.mp_child + p_baseAddress );
		}
		else
		{
			u1.mp_child = (CGeomNode *)0;
		}
	}

	// convert sibling offset to pointer
	if ((int)mp_sibling != -1)
	{
		mp_sibling = (CGeomNode *)( (int)mp_sibling + p_baseAddress );
	}
	else
	{
		mp_sibling = (CGeomNode *)0;
	}

	// convert LOD offset to pointer
	if ((int)mp_next_LOD != -1)
	{
		mp_next_LOD = (CGeomNode *)( (int)mp_next_LOD + p_baseAddress );
	}
	else
	{
		mp_next_LOD = (CGeomNode *)0;
	}

	// convert uv wibble offset to pointer
	if (m_flags & NODEFLAG_UVWIBBLE)
	{
		if (!(m_flags & NODEFLAG_ENVMAPPED) || !(m_flags & NODEFLAG_LEAF))
		{
			Dbg_MsgAssert ((int)mp_uv_wibble!=-1, ("Null uv wibble pointer on a uv wibbled node"));
			mp_uv_wibble = (float *)( (int)mp_uv_wibble + p_baseAddress );
			Dbg_MsgAssert (!(((uint32)mp_uv_wibble) & 0xf0000003),("Corrupt UV wibble offset (0x%x) in node %s\n",((int)mp_uv_wibble - (int)p_baseAddress), Script::FindChecksumName(m_checksum)));
		}
		else
		{
			mp_uv_wibble = (float *)0;
			m_flags &= ~NODEFLAG_UVWIBBLE;
		}
	}
	
	// convert vc wibble offset to pointer
	if ((int)mp_vc_wibble != -1)
	{
		// first convert the wibble offset itself
		mp_vc_wibble = (uint32 *)( (int)mp_vc_wibble + p_baseAddress );

		// now the mesh info
		uint32 *p_meshdata = mp_vc_wibble;
		int seq, num_seqs, vert, num_verts;

		num_seqs = *p_meshdata++;
		for (seq=0; seq<num_seqs; seq++)
		{
			// convert sequence pointer
			*(uint32 **)p_meshdata = (uint32 *) ( (int)*p_meshdata + p_baseAddress );
			p_meshdata++;

			// convert rgba pointers
			num_verts = *p_meshdata++;
			for (vert=0; vert<num_verts; vert++)
			{
				*(uint32 **)p_meshdata = (uint32 *) ( (uint)*p_meshdata + p_baseAddress );
				p_meshdata++;
			}
		}
	}
	else
	{
		mp_vc_wibble = (uint32 *)0;
	}

	// recursively process any children
	if (!(m_flags & NODEFLAG_LEAF))
	{
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			leaves += p_child->ProcessNodeInPlace(p_baseAddress);
		}
	}

	// recursively process any LODs
	if (mp_next_LOD)
	{
		(void) mp_next_LOD->ProcessNodeInPlace(p_baseAddress);
	}
	
	return leaves;
}



void CGeomNode::BecomesChildOf(CGeomNode *p_parent)
{
	if (m_flags & NODEFLAG_LEAF)
	{
		mp_sibling = NULL;
		m_flags &= ~NODEFLAG_LEAF;
	}
	else
	{
		mp_sibling = p_parent->u1.mp_child;
	}
	p_parent->u1.mp_child = this;
}


bool CGeomNode::RemoveFrom(CGeomNode *p_parent)
{
	if ((m_flags & NODEFLAG_LEAF) || !p_parent->u1.mp_child)
	{
		return false;
	}
		
	if (p_parent->u1.mp_child == this)
	{
		p_parent->u1.mp_child = mp_sibling;
		return true;
	}
	else
	{
		for (CGeomNode *p_node=p_parent->u1.mp_child; p_node->mp_sibling; p_node=p_node->mp_sibling)
		{
			if (p_node->mp_sibling == this)
			{
				p_node->mp_sibling = mp_sibling;
				return true;
			}
		}
	}

	return false;
}


bool CGeomNode::RemoveFromChildrenOf(CGeomNode *p_parent)
{
	if ((m_flags & NODEFLAG_LEAF) || !p_parent->u1.mp_child)
	{
		return false;
	}

	for (CGeomNode *p_node=p_parent->u1.mp_child; p_node; p_node=p_node->mp_sibling)
	{
		if (RemoveFrom(p_node))
		{
			return true;
		}
	}

	return false;
}


#endif // #ifdef __PLAT_NGPS__


/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/


#if OCCLUSION_CACHE
static bool set_valid_flag = false;
#endif


CGeomNode::CGeomNode()
{
	m_flags = ALL_VISIBILITY_BITS;

	
#if OCCLUSION_CACHE
	if( set_valid_flag )
	{
		set_valid_flag = false;
		m_flags |= NODEFLAG_LAST_OCCLUSION_VALID;
	}
	else
	{
		set_valid_flag = true;
	}
#endif
	m_bounding_sphere[0] = 0.0f;
	m_bounding_sphere[1] = 0.0f;
	m_bounding_sphere[2] = 0.0f;
	m_bounding_sphere[3] = 1.0e+30f;

	m_bounding_box[0] = 1.0e+30f;
	m_bounding_box[1] = 1.0e+30f;
	m_bounding_box[2] = 1.0e+30f;

	m_LOD_far_dist	= MAX_LOD_DIST;

	u2.mp_transform	= NULL;
	u1.mp_child		= NULL;
	mp_sibling		= NULL;
	mp_next_LOD		= NULL;
	//u1.mp_dma			= NULL;
	u3.mp_group		= NULL;
	mp_uv_wibble	= NULL;
	mp_vc_wibble	= NULL;

	m_num_bones		   = 0;
	m_bone_index	   = -1;
	u4.mp_bone_transforms = NULL;

	m_colour = 0x80808080;
}


void CGeomNode::Init()
{
	m_flags = ALL_VISIBILITY_BITS;

	m_bounding_sphere[0] = 0.0f;
	m_bounding_sphere[1] = 0.0f;
	m_bounding_sphere[2] = 0.0f;
	m_bounding_sphere[3] = 1.0e+30f;

	m_bounding_box[0] = 1.0e+30f;
	m_bounding_box[1] = 1.0e+30f;
	m_bounding_box[2] = 1.0e+30f;

	m_LOD_far_dist	= MAX_LOD_DIST;

	u2.mp_transform	= NULL;
	u1.mp_child		= NULL;
	mp_sibling		= NULL;
	mp_next_LOD		= NULL;
	//u1.mp_dma			= NULL;
	u3.mp_group		= NULL;
	mp_uv_wibble	= NULL;
	mp_vc_wibble	= NULL;

	m_num_bones		   = 0;
	m_bone_index	   = -1;
	u4.mp_bone_transforms = NULL;

	m_colour = 0x80808080;
}


CGeomNode::CGeomNode(const CGeomNode &node)
{
	#if 1
	memcpy(this,&node,sizeof(CGeomNode));
	#else
	m_flags				= node.m_flags;
	m_bounding_sphere	= node.m_bounding_sphere;
	u2.mp_transform		= node.u2.mp_transform;
	u1.mp_child			= node.u1.mp_child;
	mp_sibling			= node.mp_sibling;
	mp_next_LOD			= node.mp_next_LOD;
	m_LOD_far_dist		= node.m_LOD_far_dist;
	//u1.mp_dma				= node.u1.mp_dma;
	u3.mp_group			= node.u3.mp_group;
	mp_uv_wibble		= node.mp_uv_wibble;
	mp_vc_wibble		= node.mp_vc_wibble;
	m_num_bones			= node.m_num_bones;
	m_bone_index		= node.m_bone_index;
	u4.mp_bone_transforms	= node.u4.mp_bone_transforms;
	m_colour			= node.m_colour;
	#endif
}



#ifdef __PLAT_NGPS__



//#define	DUMP_NODE_STRUCTURE


#ifdef	DUMP_NODE_STRUCTURE
static int node_level = 0;
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeomNode::RenderWorld(CVu1Context& ctxt, uint32 renderFlags)
{
	GEOMSTATINC(geom_stats_total);

	
// Don't render anything if it's not going to be displayed	
	if (!FlipCopyEnabled())
	{
		return;
	}
	
	// render children
	CGeomNode *p_child;
	for (p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
	{
		p_child->RenderScene(ctxt, renderFlags);
	}
	
#ifdef	__NOPT_ASSERT__
	m_flags |= NODEFLAG_WAS_RENDERED;		// Set flag saying it was rendered
#endif

#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeomNode::RenderScene(CVu1Context& ctxt, uint32 renderFlags)
{
	CVu1Context *p_ctxt;

#ifdef	__NOPT_ASSERT__
	m_flags &= ~NODEFLAG_WAS_RENDERED;		// Clear flag saying it was rendered
#endif

	GEOMSTATINC(geom_stats_total);


	// cull the node if not active
	if (!((m_flags & NODEFLAG_ACTIVE) && (m_flags & render::ViewportMask)))
	{
		GEOMSTATINC(geom_stats_inactive);
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
		return;
	}

	// is it the root node of a sky?
	if (m_flags & NODEFLAG_SKY)
	{
		GEOMSTATINC(geom_stats_sky);
		// reserve space from dma list for new trans context
		p_ctxt = ctxt.Localise();

		p_ctxt->SetupAsSky();
		RenderAsSky(*p_ctxt, renderFlags);
		dma::SetList(NULL);			// not quite sure why we need this, but we do
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
		return;
	}

	// is it really an object node in disguise?
	if (m_flags & NODEFLAG_OBJECT)
	{
		RenderObject(ctxt, renderFlags);
		return;
	}

	// get this node's transform context
	if (m_flags & NODEFLAG_TRANSFORMED)
	{
		// copy the parent vu1 context
		p_ctxt = ctxt.LocaliseExtend();

		// set up the parts of the context that will depend on the new matrix
		p_ctxt->StandardSetup(*u2.mp_transform);   

		// Propogate the 'transformed' flag when recursing.
		renderFlags |= RENDERFLAG_TRANSFORMED;
	}
	else
	{
		p_ctxt = &ctxt;
	}

	// make a note of the root node for a skeletal model
	if (m_flags & NODEFLAG_SKELETAL)
	{
		GpInstanceNode = this;
		renderFlags |= RENDERFLAG_SKELETAL;
	}


	// applying a colour to a node
	if (m_flags & NODEFLAG_COLOURED)
	{
		GEOMSTATINC(geom_stats_colored);
		// if there's no local context, copy the parent's
		if (p_ctxt == &ctxt)
		{
			p_ctxt = ctxt.Localise();
		}
	
		// copy node's colour to vu1 context
		p_ctxt->SetColour(this);
		renderFlags |= RENDERFLAG_COLOURED;
	}

	// Check for light group
	if (u3.mp_light_group && u3.mp_light_group != ctxt.GetLights())
	{
		// if there's no local context, copy the global one
		if (p_ctxt == &ctxt || !p_ctxt->IsExtended())
		{
			p_ctxt = ctxt.LocaliseExtend();
		}

		p_ctxt->SetLights(u3.mp_light_group);
	}


// If we got here, then it's been rendered, so set the flag
#ifdef	__NOPT_ASSERT__
	m_flags |= NODEFLAG_WAS_RENDERED;		// Set flag saying it was rendered
#endif

	// render any children
	for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
	{
		p_child->RenderObject(*p_ctxt, renderFlags);
	}

#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeomNode::RenderObject(CVu1Context& ctxt, uint32 renderFlags)
{
	CVu1Context *p_ctxt;

#ifdef	__NOPT_ASSERT__
	m_flags &= ~NODEFLAG_WAS_RENDERED;		// Clear flag saying it was rendered
#endif

	GEOMSTATINC(geom_stats_total);




	#if	__ASSERT_ON_ZERO_RADIUS__
	Dbg_MsgAssert( m_bounding_sphere[W] != 0.0f, ( "Wasn't expecting a radius of 0 for geomnode %s or a sub-mesh of it. \n perhaps it has isolated verts that need removing?",Script::FindChecksumName(GetChecksum()) ) );
	#endif

	// cull the node if not active
	if (!((m_flags & NODEFLAG_ACTIVE) && (m_flags & render::ViewportMask)))
	{
		GEOMSTATINC(geom_stats_inactive);
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
	    return;
	}

#if OCCLUSION_CACHE
	bool last_occlusion_check_valid = ( m_flags & NODEFLAG_LAST_OCCLUSION_VALID );
	if( last_occlusion_check_valid )
	{
		// Clear the valid flag.
		m_flags &= ~NODEFLAG_LAST_OCCLUSION_VALID;

		// Was this object occluded last frame? If so, occlude this frame also.
		if( m_flags & NODEFLAG_LAST_OCCLUSION_TRUE )
		{
			m_flags &= ~NODEFLAG_LAST_OCCLUSION_TRUE;
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
		return;
		}
	}
#endif

	// get this node's transform context
	if (m_flags & NODEFLAG_TRANSFORMED)
	{
		// copy the parent vu1 context
		p_ctxt = ctxt.LocaliseExtend();

		// set up the parts of the context that will depend on the new matrix
		p_ctxt->StandardSetup(*u2.mp_transform);   

		// Propogate the 'transformed' flag when recursing.
		renderFlags |= RENDERFLAG_TRANSFORMED;
	}
	else
	{
		p_ctxt = &ctxt;
	}

	// check for skeletal mesh
	if ((renderFlags & RENDERFLAG_SKELETAL) && (m_bone_index >= 0))
	{
		Dbg_MsgAssert(GpInstanceNode, ("GpInstanceNode is NULL"));

		GEOMSTATINC(geom_stats_skeletal);
		if (p_ctxt == &ctxt)
		{
			p_ctxt = ctxt.Localise();
		}
	
		p_ctxt->HierarchicalSetup(GpInstanceNode->u4.mp_bone_transforms[m_bone_index]);
	}

	Mth::Vector camera_sphere;
	GEOMSTATINC(geom_stats_camera_sphere);
	camera_sphere = m_bounding_sphere;
	camera_sphere[W] = 1.0f;
	camera_sphere *= *p_ctxt->GetMatrix();
	camera_sphere[W] = m_bounding_sphere[W];


	// view-volume bounding sphere tests, based on parent render flags
	// MickNote:  I experimented with only clipping the leaf nodes, but this was slightly
	// slower, as the higher level culling must cull enough to justify the overhead)
	if ( renderFlags & (RENDERFLAG_CLIP | RENDERFLAG_CULL))
	{
		GEOMSTATINC(geom_stats_clipcull);
		if (SphereIsOutsideViewVolume(camera_sphere))
		{
			GEOMSTATINC(geom_stats_culled);
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
			return;
		}

		if (SphereIsInsideOuterVolume(camera_sphere))
		{
			renderFlags &= ~(RENDERFLAG_CLIP|RENDERFLAG_CULL);
		}
	}

	// view-volume bounding box test
	if ((renderFlags & (RENDERFLAG_CLIP | RENDERFLAG_CULL)) &&
		!((renderFlags & RENDERFLAG_TRANSFORMED) || m_flags & NODEFLAG_TRANSFORMED))
	{
		GEOMSTATINC(geom_stats_boxcheck);
		if (BoxIsOutsideViewVolume(*(Mth::Vector *)&m_bounding_box, m_bounding_sphere))
		{
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
			return;
		}

	}

	// Occlusion test.
	if (m_bounding_sphere[W] > 1.0e20f)
	{
		// Don't bother if sphere is stupidly large.
	}
#if OCCLUSION_CACHE
	else if( !last_occlusion_check_valid )
#else
	else
#endif	
	{
		
#if OCCLUSION_CACHE
		m_flags |= NODEFLAG_LAST_OCCLUSION_VALID;
#endif
		if (m_flags & NODEFLAG_TRANSFORMED)					// root of a transformed branch
		{
			GEOMSTATINC(geom_stats_occludecheck);
			Mth::Vector world_sphere(m_bounding_sphere);
			world_sphere[W] = 1.0f;
			world_sphere *= *u2.mp_transform;							// => use u2.mp_transform
			if (TestSphereAgainstOccluders( &world_sphere, m_bounding_sphere[W] ))
			{
				GEOMSTATINC(geom_stats_occluded);
#if OCCLUSION_CACHE
				m_flags |= NODEFLAG_LAST_OCCLUSION_TRUE;
#endif
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
				return;
			}
		}
		else if (renderFlags & RENDERFLAG_TRANSFORMED)				// deeper within a transformed branch
		{
		}
		else														// not transformed => use world position as-is
		{
			GEOMSTATINC(geom_stats_occludecheck);
			if (TestSphereAgainstOccluders( &m_bounding_sphere, m_bounding_sphere[W] ))
			{
				GEOMSTATINC(geom_stats_occluded);
#if OCCLUSION_CACHE
				m_flags |= NODEFLAG_LAST_OCCLUSION_TRUE;
#endif				
				#ifdef	DUMP_NODE_STRUCTURE
					node_level--;
				#endif
				return;
			}
		}
	}


	#if !STENCIL_SHADOW
	// shadow test
	if (renderFlags & RENDERFLAG_SHADOW)
	{
		if (m_flags & (NODEFLAG_TRANSFORMED))
		{
			renderFlags &= ~RENDERFLAG_SHADOW;
		}
		else if (( m_bounding_sphere[X] + m_bounding_box[X] ) < ( render::ShadowCameraPosition[X] - SHADOW_CAM_FRUSTUM_HALF_WIDTH  )	||
				 ( m_bounding_sphere[X] - m_bounding_box[X] ) > ( render::ShadowCameraPosition[X] + SHADOW_CAM_FRUSTUM_HALF_WIDTH  )	||
				 ( m_bounding_sphere[Z] + m_bounding_box[Z] ) < ( render::ShadowCameraPosition[Z] - SHADOW_CAM_FRUSTUM_HALF_HEIGHT )	||
				 ( m_bounding_sphere[Z] - m_bounding_box[Z] ) > ( render::ShadowCameraPosition[Z] + SHADOW_CAM_FRUSTUM_HALF_HEIGHT ))
		{
			renderFlags &= ~RENDERFLAG_SHADOW;
		}
	}
	#endif

	// applying a colour to a node
	if (m_flags & NODEFLAG_COLOURED)
	{
		GEOMSTATINC(geom_stats_colored);
		// if there's no local context, copy the parent's
		if (p_ctxt == &ctxt)
		{
			p_ctxt = ctxt.Localise();
		}

		// copy node's colour to vu1 context
		p_ctxt->SetColour(this);
		renderFlags |= RENDERFLAG_COLOURED;
	}

	// Check for light group
	if (u3.mp_light_group && u3.mp_light_group != ctxt.GetLights())
	{
		// if there's no local context, copy the global one
		if (p_ctxt == &ctxt || !p_ctxt->IsExtended())
		{
			p_ctxt = ctxt.LocaliseExtend();
		}

		p_ctxt->SetLights(u3.mp_light_group);
	}


	// Check if we should use another LOD (Garrett: Logically, this should be right, but I still want to streamline it)
	float camera_dist = -camera_sphere[Z];
	CGeomNode *p_child = NULL;
	CGeomNode *p_lod;
	for (p_lod=this; p_lod; p_lod = p_lod->mp_next_LOD)
	{
		if (p_lod->m_LOD_far_dist > camera_dist)
		{
			p_child = p_lod->u1.mp_child;
			break;
		}
	}

// If we got here, then it's been rendered, so set the flag
#ifdef	__NOPT_ASSERT__
	m_flags |= NODEFLAG_WAS_RENDERED;		// Set flag saying it was rendered
#endif

	// render any children
	if (renderFlags & RENDERFLAG_TRANSFORMED)
	{
		for (; p_child; p_child=p_child->mp_sibling)
		{
			p_child->RenderTransformedLeaf(*p_ctxt, renderFlags);
		}
	}
	else
	{
		for (; p_child; p_child=p_child->mp_sibling)
		{
			p_child->RenderNonTransformedLeaf(*p_ctxt, renderFlags);
		}
	}

#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeomNode::RenderTransformedLeaf(CVu1Context& ctxt, uint32 renderFlags)
{
	CVu1Context *p_ctxt;

	#if	__ASSERT_ON_ZERO_RADIUS__
	Dbg_MsgAssert( m_bounding_sphere[W] != 0.0f, ( "Wasn't expecting a radius of 0 for geomnode %s or a sub-mesh of it. \n perhaps it has isolated verts that need removing?",Script::FindChecksumName(GetChecksum()) ) );
	#endif

#ifdef	__NOPT_ASSERT__
	m_flags &= ~NODEFLAG_WAS_RENDERED;		// Clear flag saying it was rendered
#endif

	GEOMSTATINC(geom_stats_total);


#if OCCLUSION_CACHE
	bool last_occlusion_check_valid = ( m_flags & NODEFLAG_LAST_OCCLUSION_VALID );
	if( last_occlusion_check_valid )
	{
		// Clear the valid flag.
		m_flags &= ~NODEFLAG_LAST_OCCLUSION_VALID;

		// Was this object occluded last frame? If so, occlude this frame also.
		if( m_flags & NODEFLAG_LAST_OCCLUSION_TRUE )
		{
			m_flags &= ~NODEFLAG_LAST_OCCLUSION_TRUE;
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
		return;
		}
	}
#endif

	Mth::Vector camera_sphere;
	GEOMSTATINC(geom_stats_camera_sphere);
	camera_sphere = m_bounding_sphere;
	camera_sphere[W] = 1.0f;
	camera_sphere *= *ctxt.GetMatrix();
	camera_sphere[W] = m_bounding_sphere[W];


	// view-volume bounding sphere tests, based on parent render flags
	// MickNote:  I experimented with only clipping the leaf nodes, but this was slightly
	// slower, as the higher level culling must cull enough to justify the overhead)
	if ( renderFlags & (RENDERFLAG_CLIP | RENDERFLAG_CULL))
	{
		GEOMSTATINC(geom_stats_clipcull);
		if (SphereIsOutsideViewVolume(camera_sphere))
		{
			GEOMSTATINC(geom_stats_culled);
			GEOMSTATINC(geom_stats_leaf_culled);
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
			return;
		}

		if (SphereIsInsideOuterVolume(camera_sphere))
		{
			renderFlags &= ~(RENDERFLAG_CLIP|RENDERFLAG_CULL);
		}
	}


	// Occlusion test.
#if OCCLUSION_CACHE
	if( !last_occlusion_check_valid )
#endif	
	{
#if OCCLUSION_CACHE
		m_flags |= NODEFLAG_LAST_OCCLUSION_VALID;
#endif
	}


	#if !STENCIL_SHADOW
	// shadow test
	if (renderFlags & RENDERFLAG_SHADOW)
	{
		if (m_flags & (NODEFLAG_BILLBOARD | NODEFLAG_ENVMAPPED))
		{
			renderFlags &= ~RENDERFLAG_SHADOW;
		}
		else if (( m_bounding_sphere[X] + m_bounding_box[X] ) < ( render::ShadowCameraPosition[X] - SHADOW_CAM_FRUSTUM_HALF_WIDTH  )	||
				 ( m_bounding_sphere[X] - m_bounding_box[X] ) > ( render::ShadowCameraPosition[X] + SHADOW_CAM_FRUSTUM_HALF_WIDTH  )	||
				 ( m_bounding_sphere[Z] + m_bounding_box[Z] ) < ( render::ShadowCameraPosition[Z] - SHADOW_CAM_FRUSTUM_HALF_HEIGHT )	||
				 ( m_bounding_sphere[Z] - m_bounding_box[Z] ) > ( render::ShadowCameraPosition[Z] + SHADOW_CAM_FRUSTUM_HALF_HEIGHT ))
		{
			renderFlags &= ~RENDERFLAG_SHADOW;
		}
	}
	#endif

	// get this node's transform context
	p_ctxt = &ctxt;

	// applying a colour to a node
	if (m_flags & NODEFLAG_COLOURED)
	{
		GEOMSTATINC(geom_stats_colored);
		p_ctxt = ctxt.Localise();

		// copy node's colour to vu1 context
		p_ctxt->SetColour(this);
		renderFlags |= RENDERFLAG_COLOURED;
	}

	GEOMSTATINC(geom_stats_leaf);
	#ifdef	__NOPT_ASSERT__
	// Mick: check patch variables for debugging toggle of multi-passes
	if ((m_flags & gPassMask1) != gPassMask0)
	{	
#ifdef	DUMP_NODE_STRUCTURE
node_level--;
#endif
		return;
	}
	#endif

	#ifdef	__LOD_MULTIPASS__
	// if 2nd+ pass, check to see if distance from camera is greater than allowed for multipass
	if ((m_flags & NODEFLAG_ZPUSH) && (-camera_sphere[Z] > render::sMultipassMaxDist))
	{
#ifdef	DUMP_NODE_STRUCTURE
node_level--;
#endif
		return;
	}
	#endif

	if (m_flags & (NODEFLAG_UVWIBBLE | NODEFLAG_ENVMAPPED | NODEFLAG_VCWIBBLE))
	{
		if (m_flags & (NODEFLAG_UVWIBBLE | NODEFLAG_ENVMAPPED))
		{
			// if there's no local context, copy the global one
			if (p_ctxt == &ctxt)
			{
				p_ctxt = ctxt.LocaliseExtend();
			}

			// uv wibble
			if (m_flags & NODEFLAG_UVWIBBLE)
			{
				GEOMSTATINC(geom_stats_wibbleUV);
				p_ctxt->WibbleUVs(mp_uv_wibble, m_flags&NODEFLAG_EXPLICIT_UVWIBBLE);
			}

			// environment mapping
			if (m_flags & NODEFLAG_ENVMAPPED)
			{
				// reflection map vectors
				p_ctxt->SetReflectionVecs((sint16)(uint)mp_uv_wibble, (sint16)(((uint)mp_uv_wibble)>>16));
			}
		}

		// vc wibble
		if (m_flags & NODEFLAG_VCWIBBLE)
		{
			GEOMSTATINC(geom_stats_wibbleVC);
			WibbleVCs();
		}
	}

	// set the dma context
	dma::SetList(u3.mp_group);
	u3.mp_group->Used[render::Field] = true;

	// flag the texture as used
	if (u4.mp_texture)
	{
		u4.mp_texture->m_render_count++;
	}


	Mth::Vector *p_trans = p_ctxt->GetTranslation();

	// initial cnt tag, for sending row reg and z-sort key
	dma::Tag(dma::cnt, 1, *(uint32 *)&camera_sphere[Z]);
	vif::NOP();
	vif::STROW((int)(*p_trans)[0] + (int)(m_bounding_sphere[0]*SUB_INCH_PRECISION),
			   (int)(*p_trans)[1] + (int)(m_bounding_sphere[1]*SUB_INCH_PRECISION),
			   (int)(*p_trans)[2] + (int)(m_bounding_sphere[2]*SUB_INCH_PRECISION), 0);


	// special case for billboards
	if (m_flags & NODEFLAG_BILLBOARD)
	{
		// compute billboard basis vecs
		Mth::Vector udir,vdir;
		vdir = (*p_ctxt->GetMatrix())[1];
		vdir[2] =  0.0f;
		vdir[3] =  0.0f;
		vdir.Normalize();
		udir[0] =  vdir[1];
		udir[1] = -vdir[0];
		udir[2] =  0.0f;
		udir[3] =  0.0f;

		udir *= render::CameraToFrustum;
		vdir *= render::CameraToFrustum;
		udir[2] =  0.0f;
		udir[3] =  0.0f;
		vdir[2] =  0.0f;
		vdir[3] =  0.0f;

		// send vu1 context
		dma::Tag(dma::next, 16, (uint)dma::pLoc+272);
		vif::BASE(vu1::Loc);
		vif::OFFSET(0);
		uint vu1_loc = vu1::Loc;
		vu1::Loc = 0;   							// must do this as a relative prim for a sortable list...
		vu1::BeginPrim(REL, VU1_ADDR(L_VF20));
		vu1::StoreVec(*(Vec *)&udir);
		vu1::StoreVec(*(Vec *)&vdir);
		vu1::StoreVec(*(Vec *)&render::IntViewportScale);
		vif::StoreV4_32(0,0,0,0);
		vif::StoreV4_32(0,0,0,0);
		vu1::StoreVec(*(Vec *)&render::CameraToWorld[3]);
		vu1::StoreVec(*(Vec *)ctxt.GetColour());
		vu1::StoreVec(*(Vec *)&render::IntViewportOffset);
		Mth::Matrix LocalToFrustum = (*p_ctxt->GetMatrix()) * render::CameraToFrustum;
		vu1::CopyMat((float*)&LocalToFrustum);
		vu1::EndPrim(0);

		// gs context data, specifically the fog coefficient
		float w,f;
		if (renderFlags & RENDERFLAG_TRANSFORMED)
		{
			w = -(*p_ctxt->GetMatrix())[3][2];
		}
		else
		{
			// Context only contains WorldToCamera matrix, so convert world position to camera relative position
			Mth::Vector camera_rel_pos = render::WorldToCamera.TransformAsPos(m_bounding_sphere);
			w = -camera_rel_pos[2];
		}
		if ((w > render::FogNear) && render::EnableFog)	// Garrett: We have to check for EnableFog here because the VU1 code isn't
		{
			f = 1.0 + render::EffectiveFogAlpha * (render::FogNear/w - 1.0f);
		}
		else
		{
			f = 1.0f;
		}
		gs::BeginPrim(REL,0,0);
		gs::Reg1(gs::FOG, PackFOG((int)(f*255.99f)));
		gs::EndPrim(0);

		dma::EndTag();
		((uint16 *)dma::pTag)[1] |= vu1::Loc & 0x3FF;	// must write some code for doing this automatically
		vu1::Loc += vu1_loc;

		u3.mp_group->pVu1Context = NULL;
	}

	// send transform context if necessary
	else if ((p_ctxt != u3.mp_group->pVu1Context) || (u3.mp_group->flags & GROUPFLAG_SORT))
	{
		GEOMSTATINC(geom_stats_sendcontext);
		int qwc = p_ctxt->IsExtended() ? EXT_CTXT_SIZE+2 : STD_CTXT_SIZE+2;
		dma::Tag(dma::ref, qwc|(qwc-1)<<16, p_ctxt->GetDma());
		vif::BASE(vu1::Loc);
		vif::OFFSET(0);
		vu1::Loc += qwc-1;
		u3.mp_group->pVu1Context = p_ctxt;
	}

	// render this node by adding to the chosen dma list
	uint vu1_flags = (renderFlags & RENDERFLAG_CLIP) ? CLIP :
					 (renderFlags & RENDERFLAG_CULL) ? CULL : PROJ;
	if (renderFlags & RENDERFLAG_COLOURED)
		vu1_flags |= COLR;
	if (render::EnableFog && (camera_sphere[3]-camera_sphere[2] > render::FogNear))
		vu1_flags |= FOGE;
	dma::pTag = dma::pLoc;		// must set this manually if we bypass the dma tag functions
	dma::Store32(u2.m_dma_tag_lo32,(uint32)u1.mp_dma);
	vif::BASE(vu1::Loc);
	vif::ITOP(vu1_flags);
	vu1::Loc += u2.m_dma_tag_lo32>>16;

	if (m_flags & NODEFLAG_BLACKFOG)
	{
		dma::Gosub(SET_FOGCOL,2);
		dma::pLoc -= 8;
		vif::FLUSHA();
		vif::NOP();
	}

	// must add a 'next' tag for sorted groups
	if (u3.mp_group->flags & GROUPFLAG_SORT)
	{
		GEOMSTATINC(geom_stats_sorted);
		dma::SetList(NULL);
	}

	#if !STENCIL_SHADOW
	// render shadow
	if ((renderFlags & RENDERFLAG_SHADOW) && !(m_flags & NODEFLAG_NOSHADOW))
	{
		GEOMSTATINC(geom_stats_shadow);
		// set the dma context
		dma::SetList(sGroup::pShadow);

		// set row reg
		dma::Tag(dma::cnt, 1, *(uint32 *)&camera_sphere[Z]);
		vif::NOP();
		vif::STROW((int)(*p_trans)[0] + (int)(m_bounding_sphere[0]*SUB_INCH_PRECISION),
				   (int)(*p_trans)[1] + (int)(m_bounding_sphere[1]*SUB_INCH_PRECISION),
				   (int)(*p_trans)[2] + (int)(m_bounding_sphere[2]*SUB_INCH_PRECISION), 0);

		// send transform context
		dma::Tag(dma::ref, 18|17<<16, sGroup::pShadow->pVu1Context->GetDma());
		vif::BASE(vu1::Loc);
		vif::OFFSET(0);
		vu1::Loc += 17;

		// render this node
		vu1_flags = (renderFlags & RENDERFLAG_CLIP) ? CLIP|SHDW :
					(renderFlags & RENDERFLAG_CULL) ? CULL|SHDW : PROJ|SHDW;
		if (render::EnableFog && (camera_sphere[3]-camera_sphere[2] > render::FogNear))
			vu1_flags |= FOGE;
		dma::pTag = dma::pLoc;		// must set this manually if we bypass the dma tag functions
		dma::Store32(u2.m_dma_tag_lo32,(uint32)u1.mp_dma);
		vif::BASE(vu1::Loc);
		vif::ITOP(vu1_flags);
		vu1::Loc += u2.m_dma_tag_lo32>>16;

		if (m_flags & NODEFLAG_BLACKFOG)
		{
			dma::Gosub(SET_FOGCOL,2);
			dma::pLoc -= 8;
			vif::FLUSHA();
			vif::NOP();
		}
	}
	#endif

#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeomNode::RenderNonTransformedLeaf(CVu1Context& ctxt, uint32 renderFlags)
{
	float camera_sphere_z;

	#if	__ASSERT_ON_ZERO_RADIUS__
	Dbg_MsgAssert( m_bounding_sphere[W] != 0.0f, ( "Wasn't expecting a radius of 0 for geomnode %s or a sub-mesh of it. \n perhaps it has isolated verts that need removing?",Script::FindChecksumName(GetChecksum()) ) );
	#endif	

	// kick off the frustum culling tests in VU0
	asm __volatile__("

		lqc2		vf08,(%0)					# sphere (x,y,z,R)
		lqc2		vf09,(%1)					# box (bx,by,bz,?)
		vcallms		BothCullTests				# call microsubroutine

	": : "r" (&m_bounding_sphere), "r" (&m_bounding_box) );


#ifdef	__NOPT_ASSERT__
	m_flags &= ~NODEFLAG_WAS_RENDERED;		// Clear flag saying it was rendered
#endif

	GEOMSTATINC(geom_stats_total);


//Mick:  Minimal leaf rendering makes no real difference 
// so I've removed this call while refactoring	

	// check for minimal leaf node for a quicker render
	if (!(m_flags & (NODEFLAG_COLOURED		|
					 NODEFLAG_BILLBOARD		|
					 NODEFLAG_UVWIBBLE		|
					 NODEFLAG_VCWIBBLE		|
					 NODEFLAG_ENVMAPPED))

		&& !(renderFlags & (RENDERFLAG_COLOURED		|
							#if !STENCIL_SHADOW
							RENDERFLAG_SHADOW		|
							#endif
							RENDERFLAG_CLIP			|
							RENDERFLAG_CULL)))
	{
		RenderMinimalLeaf(ctxt);
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
		return;
	}


#if OCCLUSION_CACHE
	bool last_occlusion_check_valid = ( m_flags & NODEFLAG_LAST_OCCLUSION_VALID );
	if( last_occlusion_check_valid )
	{
		// Clear the valid flag.
		m_flags &= ~NODEFLAG_LAST_OCCLUSION_VALID;

		// Was this object occluded last frame? If so, occlude this frame also.
		if( m_flags & NODEFLAG_LAST_OCCLUSION_TRUE )
		{
			m_flags &= ~NODEFLAG_LAST_OCCLUSION_TRUE;
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
		return;
		}
	}
#endif


	// view-volume bounding sphere tests, based on parent render flags
	// MickNote:  I experimented with only clipping the leaf nodes, but this was slightly
	// slower, as the higher level culling must cull enough to justify the overhead)
	if ( renderFlags & (RENDERFLAG_CLIP | RENDERFLAG_CULL))
	{
		GEOMSTATINC(geom_stats_clipcull);

		if (CullAgainstViewFrustum())
		{
			GEOMSTATINC(geom_stats_culled);
			GEOMSTATINC(geom_stats_leaf_culled);
#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
			return;
		}

		if (!CullAgainstOuterFrustum())
		{
			renderFlags &= ~(RENDERFLAG_CLIP|RENDERFLAG_CULL);
		}
	}


	// Occlusion test.
#if OCCLUSION_CACHE
	if( !last_occlusion_check_valid )
#endif	
	{
		
#if OCCLUSION_CACHE
		m_flags |= NODEFLAG_LAST_OCCLUSION_VALID;
#endif
		GEOMSTATINC(geom_stats_occludecheck);
		if (TestSphereAgainstOccluders( &m_bounding_sphere, m_bounding_sphere[W] ))
		{
			GEOMSTATINC(geom_stats_occluded);
#if OCCLUSION_CACHE
			m_flags |= NODEFLAG_LAST_OCCLUSION_TRUE;
#endif				
			#ifdef	DUMP_NODE_STRUCTURE
				node_level--;
			#endif
			return;
		}
	}


	#if !STENCIL_SHADOW
	// shadow test
	if (renderFlags & RENDERFLAG_SHADOW)
	{
		if (m_flags & (NODEFLAG_BILLBOARD | NODEFLAG_ENVMAPPED))
		{
			renderFlags &= ~RENDERFLAG_SHADOW;
		}
		else if (( m_bounding_sphere[X] + m_bounding_box[X] ) < ( render::ShadowCameraPosition[X] - SHADOW_CAM_FRUSTUM_HALF_WIDTH  )	||
				 ( m_bounding_sphere[X] - m_bounding_box[X] ) > ( render::ShadowCameraPosition[X] + SHADOW_CAM_FRUSTUM_HALF_WIDTH  )	||
				 ( m_bounding_sphere[Z] + m_bounding_box[Z] ) < ( render::ShadowCameraPosition[Z] - SHADOW_CAM_FRUSTUM_HALF_HEIGHT )	||
				 ( m_bounding_sphere[Z] - m_bounding_box[Z] ) > ( render::ShadowCameraPosition[Z] + SHADOW_CAM_FRUSTUM_HALF_HEIGHT ))
		{
			renderFlags &= ~RENDERFLAG_SHADOW;
		}
	}
	#endif

	// get this node's transform context
	CVu1Context *p_ctxt = &ctxt;

	// applying a colour to a node
	if (m_flags & NODEFLAG_COLOURED)
	{
		GEOMSTATINC(geom_stats_colored);
		p_ctxt = ctxt.Localise();

		// copy node's colour to vu1 context
		p_ctxt->SetColour(this);
		renderFlags |= RENDERFLAG_COLOURED;
	}

	GEOMSTATINC(geom_stats_leaf);
	#ifdef	__NOPT_ASSERT__
	// Mick: check patch variables for debugging toggle of multi-passes
	if ((m_flags & gPassMask1) != gPassMask0)
	{	
#ifdef	DUMP_NODE_STRUCTURE
node_level--;
#endif
		return;
	}
	#endif


	asm __volatile__("
		qmfc2		$8,$vf05
		sw			$8,(%0)
	": : "r" (&camera_sphere_z) : "$8" );

	#ifdef	__LOD_MULTIPASS__
	// if 2nd+ pass, check to see if distance from camera is greater than allowed for multipass
	if ((m_flags & NODEFLAG_ZPUSH) && (-camera_sphere_z > render::sMultipassMaxDist))
	{
#ifdef	DUMP_NODE_STRUCTURE
node_level--;
#endif
		return;
	}
	#endif

	if (m_flags & (NODEFLAG_UVWIBBLE | NODEFLAG_ENVMAPPED | NODEFLAG_VCWIBBLE))
	{
		if (m_flags & (NODEFLAG_UVWIBBLE | NODEFLAG_ENVMAPPED))
		{
			// if there's no local context, copy the global one
			if (p_ctxt == &ctxt)
			{
				p_ctxt = ctxt.LocaliseExtend();
			}

			// uv wibble
			if (m_flags & NODEFLAG_UVWIBBLE)
			{
				GEOMSTATINC(geom_stats_wibbleUV);
				p_ctxt->WibbleUVs(mp_uv_wibble, m_flags&NODEFLAG_EXPLICIT_UVWIBBLE);
			}

			// environment mapping
			if (m_flags & NODEFLAG_ENVMAPPED)
			{
				// reflection map vectors
				p_ctxt->SetReflectionVecs((sint16)(uint)mp_uv_wibble, (sint16)(((uint)mp_uv_wibble)>>16));
			}
		}

		// vc wibble
		if (m_flags & NODEFLAG_VCWIBBLE)
		{
			GEOMSTATINC(geom_stats_wibbleVC);
			WibbleVCs();
		}
	}

	// set the dma context
	dma::SetList(u3.mp_group);
	u3.mp_group->Used[render::Field] = true;

	// flag the texture as used
	if (u4.mp_texture)
	{
		u4.mp_texture->m_render_count++;
	}


	Mth::Vector *p_trans = p_ctxt->GetTranslation();

	// initial cnt tag, for sending row reg and z-sort key
	dma::Tag(dma::cnt, 1, *(uint32 *)&camera_sphere_z);
	vif::NOP();
	vif::STROW((int)(*p_trans)[0] + (int)(m_bounding_sphere[0]*SUB_INCH_PRECISION),
			   (int)(*p_trans)[1] + (int)(m_bounding_sphere[1]*SUB_INCH_PRECISION),
			   (int)(*p_trans)[2] + (int)(m_bounding_sphere[2]*SUB_INCH_PRECISION), 0);


	// special case for billboards
	if (m_flags & NODEFLAG_BILLBOARD)
	{
		// compute billboard basis vecs
		Mth::Vector udir,vdir;
		vdir = (*p_ctxt->GetMatrix())[1];
		vdir[2] =  0.0f;
		vdir[3] =  0.0f;
		vdir.Normalize();
		udir[0] =  vdir[1];
		udir[1] = -vdir[0];
		udir[2] =  0.0f;
		udir[3] =  0.0f;

		udir *= render::CameraToFrustum;
		vdir *= render::CameraToFrustum;
		udir[2] =  0.0f;
		udir[3] =  0.0f;
		vdir[2] =  0.0f;
		vdir[3] =  0.0f;

		// send vu1 context
		dma::Tag(dma::next, 16, (uint)dma::pLoc+272);
		vif::BASE(vu1::Loc);
		vif::OFFSET(0);
		uint vu1_loc = vu1::Loc;
		vu1::Loc = 0;   							// must do this as a relative prim for a sortable list...
		vu1::BeginPrim(REL, VU1_ADDR(L_VF20));
		vu1::StoreVec(*(Vec *)&udir);
		vu1::StoreVec(*(Vec *)&vdir);
		vu1::StoreVec(*(Vec *)&render::IntViewportScale);
		vif::StoreV4_32(0,0,0,0);
		vif::StoreV4_32(0,0,0,0);
		vu1::StoreVec(*(Vec *)&render::CameraToWorld[3]);
		vu1::StoreVec(*(Vec *)ctxt.GetColour());
		vu1::StoreVec(*(Vec *)&render::IntViewportOffset);
		Mth::Matrix LocalToFrustum = (*p_ctxt->GetMatrix()) * render::CameraToFrustum;
		vu1::CopyMat((float*)&LocalToFrustum);
		vu1::EndPrim(0);

		// gs context data, specifically the fog coefficient
		float w,f;
		if (renderFlags & RENDERFLAG_TRANSFORMED)
		{
			w = -(*p_ctxt->GetMatrix())[3][2];
		}
		else
		{
			// Context only contains WorldToCamera matrix, so convert world position to camera relative position
			Mth::Vector camera_rel_pos = render::WorldToCamera.TransformAsPos(m_bounding_sphere);
			w = -camera_rel_pos[2];
		}
		if ((w > render::FogNear) && render::EnableFog)	// Garrett: We have to check for EnableFog here because the VU1 code isn't
		{
			f = 1.0 + render::EffectiveFogAlpha * (render::FogNear/w - 1.0f);
		}
		else
		{
			f = 1.0f;
		}
		gs::BeginPrim(REL,0,0);
		gs::Reg1(gs::FOG, PackFOG((int)(f*255.99f)));
		gs::EndPrim(0);

		dma::EndTag();
		((uint16 *)dma::pTag)[1] |= vu1::Loc & 0x3FF;	// must write some code for doing this automatically
		vu1::Loc += vu1_loc;

		u3.mp_group->pVu1Context = NULL;
	}

	// send transform context if necessary
	else if ((p_ctxt != u3.mp_group->pVu1Context) || (u3.mp_group->flags & GROUPFLAG_SORT))
	{
		GEOMSTATINC(geom_stats_sendcontext);
		int qwc = p_ctxt->IsExtended() ? EXT_CTXT_SIZE+2 : STD_CTXT_SIZE+2;
		dma::Tag(dma::ref, qwc|(qwc-1)<<16, p_ctxt->GetDma());
		vif::BASE(vu1::Loc);
		vif::OFFSET(0);
		vu1::Loc += qwc-1;
		u3.mp_group->pVu1Context = p_ctxt;
	}

	// render this node by adding to the chosen dma list
	uint vu1_flags = (renderFlags & RENDERFLAG_CLIP) ? CLIP :
					 (renderFlags & RENDERFLAG_CULL) ? CULL : PROJ;
	if (renderFlags & RENDERFLAG_COLOURED)
		vu1_flags |= COLR;
	if (render::EnableFog && (m_bounding_sphere[W]-camera_sphere_z > render::FogNear))
		vu1_flags |= FOGE;
	dma::pTag = dma::pLoc;		// must set this manually if we bypass the dma tag functions
	dma::Store32(u2.m_dma_tag_lo32,(uint32)u1.mp_dma);
	vif::BASE(vu1::Loc);
	vif::ITOP(vu1_flags);
	vu1::Loc += u2.m_dma_tag_lo32>>16;

	if (m_flags & NODEFLAG_BLACKFOG)
	{
		dma::Gosub(SET_FOGCOL,2);
		dma::pLoc -= 8;
		vif::FLUSHA();
		vif::NOP();
	}

	// must add a 'next' tag for sorted groups
	if (u3.mp_group->flags & GROUPFLAG_SORT)
	{
		GEOMSTATINC(geom_stats_sorted);
		dma::SetList(NULL);
	}

	#if !STENCIL_SHADOW
	// render shadow
	if ((renderFlags & RENDERFLAG_SHADOW) && !(m_flags & NODEFLAG_NOSHADOW))
	{
		GEOMSTATINC(geom_stats_shadow);
		// set the dma context
		dma::SetList(sGroup::pShadow);

		// set row reg
		dma::Tag(dma::cnt, 1, *(uint32 *)&camera_sphere_z);
		vif::NOP();
		vif::STROW((int)(*p_trans)[0] + (int)(m_bounding_sphere[0]*SUB_INCH_PRECISION),
				   (int)(*p_trans)[1] + (int)(m_bounding_sphere[1]*SUB_INCH_PRECISION),
				   (int)(*p_trans)[2] + (int)(m_bounding_sphere[2]*SUB_INCH_PRECISION), 0);

		// send transform context
		dma::Tag(dma::ref, 18|17<<16, sGroup::pShadow->pVu1Context->GetDma());
		vif::BASE(vu1::Loc);
		vif::OFFSET(0);
		vu1::Loc += 17;

		// render this node
		vu1_flags = (renderFlags & RENDERFLAG_CLIP) ? CLIP|SHDW :
					(renderFlags & RENDERFLAG_CULL) ? CULL|SHDW : PROJ|SHDW;
		if (render::EnableFog && (m_bounding_sphere[W]-camera_sphere_z > render::FogNear))
			vu1_flags |= FOGE;
		dma::pTag = dma::pLoc;		// must set this manually if we bypass the dma tag functions
		dma::Store32(u2.m_dma_tag_lo32,(uint32)u1.mp_dma);
		vif::BASE(vu1::Loc);
		vif::ITOP(vu1_flags);
		vu1::Loc += u2.m_dma_tag_lo32>>16;

		if (m_flags & NODEFLAG_BLACKFOG)
		{
			dma::Gosub(SET_FOGCOL,2);
			dma::pLoc -= 8;
			vif::FLUSHA();
			vif::NOP();
		}
	}
	#endif

#ifdef	DUMP_NODE_STRUCTURE
	node_level--;
#endif
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// Mick:  I've moved this function here to improved I-Cache usage while rendering

bool TestSphereAgainstOccluders( Mth::Vector *p_center, float radius, uint32 meshes )
{
//	return false;
	// GJ:  This will catch bugs where a model's bounding sphere
	// is uninitialized, return inconsistent values depending on
	// the level you were in (this came up with the create-a-trick
	// skater, which uses "ref models", which don't have valid bounding
	// spheres)
	#if	__ASSERT_ON_ZERO_RADIUS__
	Dbg_MsgAssert( radius != 0.0f, ( "Wasn't expecting a radius of 0" ) );
	#endif

#if defined(OCCLUDER_USES_VU0_MACROMODE) || defined(OCCLUDER_USES_VU0_MICROMODE)

	if (sOccluder::sUseVU0)
	{
#ifdef	OCCLUDER_USES_VU0_MACROMODE
		bool result;

		asm __volatile__(
		"
		.set noreorder
		mfc1      $8, %3			 # radius
		qmtc2     $8, vf9

		vilwr.x   vi01, (vi00)		 # load number of occluders

		lqc2      vf10, 0x0(%1)		 # p_center
		ctc2      %2, $vi15			 # meshes

		vaddw.x   vf31, vf00, vf00w  # Load 1.0f to VF31x

		# Init loop
		cfc2      $10,  $vi01		 # move number of occluders to $10
		beq       $10,  $0, no_occlusions
		viaddi    vi02, vi00, 1		 # load address of first occluder

occ_loop:
		viaddi    vi03, vi02, 0		 # and put here, too
		vlqi.xyzw vf01, (vi03++)	 # load planes
		li        $9, 5				 # number of planes

plane_loop:
		# dot product
		vmul.xyz  vf11,vf01,vf10     # result = plane * center + plane_w [start]
		vmulax.x  ACC, vf01,vf10x
		vmaddaw.x ACC, vf31,vf01w	 # plane_w
		vmaddax.x ACC, vf31,vf09x    # also add radius in
		vmadday.x ACC, vf31,vf11y
		vmaddz.x  vf11,vf31,vf11z    # result = plane * center + plane_w + radius [ready]

		addi      $9, $9, -1         # decrement plane counter
		viaddi    vi04, vi02, 5		 # calc score address (since we are waiting anyway)
		vilwr.x   vi14, (vi04)		 # load score
		vlqi.xyzw vf01, (vi03++)	 # load next planes
		vnop

		cfc2      $8, $vi17			 # transfer if result (MAC register)
		andi	  $8, $8, 0x80		 # check result (v >= 0) not_occluded
		beq		  $8, $0, done_occluder
		nop

		bne       $9, $0, plane_loop
		nop

		# got through all planes - found occluder
found_occluder:
		li		  %0, 1				 # store true for return value
		viadd     vi14, vi14, vi15   # add meshes
		b         done
		viswr.x   vi14, (vi04)

done_occluder:
		addi      $10, $10, -1       # decrement occluder counter
		bne       $10, $0, occ_loop
		viaddi    vi02, vi02, 6		 # increment occluder address

no_occlusions:
		li		  %0, 0				 # store false for return value

done:
		.set reorder
		": "=r" (result) : "r" (p_center), "r" (meshes), "f" (radius) : "$8", "$9", "$10");

		return result;

#endif	// OCCLUDER_USES_VU0_MACROMODE
	
#ifdef OCCLUDER_USES_VU0_MICROMODE

		uint32 result, index;
		Mth::Vector sphere(*p_center);
		sphere[W] = radius;

		// call vu0 microsubroutine
		asm __volatile__("

			lqc2		vf07,(%0)					# load sphere (x,y,z,R) into vf07 of vu0
			vcallms		TestSphereAgainstOccluders	# call microsubroutine
			vnop									# interlocking instruction, waits for vu0 completion
			cfc2		$8,$vi01					# get boolean result from vu0
			cfc2		$9,$vi06					# get index of first successful occluder
			sw			$8,(%1)						# store boolean result
			sw			$9,(%2)						# store index
		
		": : "r" (&sphere), "r" (&result), "r" (&index) : "$8","$9");

		// keeping score
		if (result)
		{
			sOccluder::Occluders[index].score += meshes;
		}

		return (bool)result;

#endif	// OCCLUDER_USES_VU0_MICROMODE
	}
	else
#endif // defined(OCCLUDER_USES_VU0_MACROMODE) || defined(OCCLUDER_USES_VU0_MICROMODE)

	{
		float center_x = p_center->GetX();
		float center_y = p_center->GetY();
		float center_z = p_center->GetZ();

		sOccluder * p_occluder;
		if (sOccluder::sUseScratchPad)
		{
			p_occluder = (sOccluder*)0x70000000;
		} else {
			p_occluder = &sOccluder::Occluders[0];
		}

		// Test against each occluder.
		for( uint32 o = sOccluder::NumOccluders; o > 0 ; --o )
		{
			Mth::Vector * p_plane = &(p_occluder->planes[0]);
			// Test against each plane in the occluder.
			for( uint32 p = 5; p > 0; --p )
			{

				float result =	( p_plane->GetX() * center_x ) +
								( p_plane->GetY() * center_y ) +
								( p_plane->GetZ() * center_z ) +
								( p_plane->GetW() );

				if( result >= -radius )
				{
					// Outside of this plane, therefore not occluded by this occluder.
					goto	NOT_OCCLUDED;
				}
				p_plane++;
			}

			// Inside all five planes, therefore occluded. Increase score for this occluder.
			p_occluder->score += meshes;
			return true;

	NOT_OCCLUDED:		
			p_occluder++;
		}

		return false;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CGeomNode::CullAgainstViewFrustum()
{

	uint32 result;

	#if 0

	// test version... perform test and get results
	asm __volatile__("

		lqc2		vf08,(%0)					# sphere (x,y,z,R)
		lqc2		vf09,(%1)					# box (bx,by,bz,?)
		vcallms		ViewCullTest				# call microsubroutine
		vnop									# interlocking instruction, waits for vu0 completion
		cfc2		$8,$vi01					# get result
		sw			$8,(%2)						# store result

	": : "r" (&m_bounding_sphere), "r" (&m_bounding_box), "r" (&result) : "$8");


	#else
	
	// faster version... pick up results from test which kicked off earlier
	asm __volatile__("

		vnop									# interlocking instruction, waits for vu0 completion
		cfc2		$8,$vi01					# get result
		sw			$8,(%0)						# store result

	": : "r" (&result) : "$8");

	#endif

	return (bool)result;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CGeomNode::CullAgainstOuterFrustum()
{

	uint32 result;

	#if 0
	
	// test version... perform test and get results
	asm __volatile__("

		lqc2		vf08,(%0)					# sphere (x,y,z,R)
		lqc2		vf09,(%1)					# box (bx,by,bz,?)
		vcallms		OuterCullTest				# call microsubroutine
		vnop									# interlocking instruction, waits for vu0 completion
		cfc2		$8,$vi01					# get result
		sw			$8,(%2)						# store result

	": : "r" (&m_bounding_sphere), "r" (&m_bounding_box), "r" (&result) : "$8");

	#else
	
	// faster version... pick up results from test which kicked off earlier
	asm __volatile__("

		cfc2		$8,$vi11					# get result
		sw			$8,(%0)						# store result

	": : "r" (&result) : "$8");

	#endif

	return (bool)result;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeomNode::RenderMinimalLeaf(CVu1Context& ctxt)
{
	float camera_sphere_z;

	#if	__ASSERT_ON_ZERO_RADIUS__
	Dbg_MsgAssert( m_bounding_sphere[W] != 0.0f, ( "Wasn't expecting a radius of 0 for geomnode %s or a sub-mesh of it. \n perhaps it has isolated verts that need removing?",Script::FindChecksumName(GetChecksum()) ) );
	#endif
	
	GEOMSTATINC(geom_stats_total);

	GEOMSTATINC(geom_stats_occludecheck);
	if (TestSphereAgainstOccluders( &m_bounding_sphere, m_bounding_sphere[W] ))
	{
		GEOMSTATINC(geom_stats_occluded);
		return;
	}

	GEOMSTATINC(geom_stats_leaf);
	#ifdef	__NOPT_ASSERT__
	// Mick: check patch variables for debugging toggle of multi-passes
	if ((m_flags & gPassMask1) != gPassMask0)
	{	
		return;
	}
	#endif


	// set the dma context
	dma::SetList(u3.mp_group);
	u3.mp_group->Used[render::Field] = true;

	// flag the texture as used
	if (u4.mp_texture)
	{
		u4.mp_texture->m_render_count++;
	}


	Mth::Vector *p_trans = ctxt.GetTranslation();
	

	asm __volatile__("
		qmfc2		$8,$vf05
		sw			$8,(%0)
	": : "r" (&camera_sphere_z) : "$8" );


	#ifdef	__LOD_MULTIPASS__
	// if 2nd+ pass, check to see if distance from camera is greater than allowed for multipass
//		if ((m_flags & NODEFLAG_ZPUSH) && (-camera_sphere[Z] > render::sMultipassMaxDist))
	if ((m_flags & NODEFLAG_ZPUSH) && (-camera_sphere_z > render::sMultipassMaxDist))
		return;
	#endif

	// initial cnt tag, for sending row reg and z-sort key
	dma::Tag(dma::cnt, 1, *(uint32 *)&camera_sphere_z);
	vif::NOP();
	vif::STROW((int)(*p_trans)[0] + (int)(m_bounding_sphere[0]*SUB_INCH_PRECISION),
			   (int)(*p_trans)[1] + (int)(m_bounding_sphere[1]*SUB_INCH_PRECISION),
			   (int)(*p_trans)[2] + (int)(m_bounding_sphere[2]*SUB_INCH_PRECISION), 0);

	// send transform context if necessary
	if ((&ctxt != u3.mp_group->pVu1Context) || (u3.mp_group->flags & GROUPFLAG_SORT))
	{
		GEOMSTATINC(geom_stats_sendcontext);
		//Dbg_MsgAssert(!ctxt.IsExtended(), ("oh bugger"));
		int qwc = ctxt.IsExtended() ? EXT_CTXT_SIZE+2 : STD_CTXT_SIZE+2;
		dma::Tag(dma::ref, qwc|(qwc-1)<<16, ctxt.GetDma());
		vif::BASE(vu1::Loc);
		vif::OFFSET(0);
		vu1::Loc += qwc-1;
		u3.mp_group->pVu1Context = &ctxt;
	}

	// render this node by adding to the chosen dma list
	dma::pTag = dma::pLoc;		// must set this manually if we bypass the dma tag functions
	dma::Store32(u2.m_dma_tag_lo32,(uint32)u1.mp_dma);
	vif::BASE(vu1::Loc);
	if (render::EnableFog && (m_bounding_sphere[W]-camera_sphere_z > render::FogNear))
		vif::ITOP(PROJ|FOGE);
	else
		vif::ITOP(PROJ);
	vu1::Loc += u2.m_dma_tag_lo32>>16;

	if (m_flags & NODEFLAG_BLACKFOG)
	{
		dma::Gosub(SET_FOGCOL,2);
		dma::pLoc -= 8;
		vif::FLUSHA();
		vif::NOP();
	}

	// must add a 'next' tag for sorted groups
	if (u3.mp_group->flags & GROUPFLAG_SORT)
	{
		GEOMSTATINC(geom_stats_sorted);
		dma::SetList(NULL);
	}
	
	
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeomNode::RenderAsSky(CVu1Context& ctxt, uint32 renderFlags)
{
	CVu1Context *p_ctxt;

	// cull the node if not active
	if (!(m_flags & NODEFLAG_ACTIVE))
	{
		return;
	}

	// no support for transformed nodes yet...
	p_ctxt = &ctxt;

	Mth::Vector sphere;

	// bounding sphere tests, based on parent render flags
	if (renderFlags & (RENDERFLAG_CLIP | RENDERFLAG_CULL))
	{
		// get and transform centre of bounding sphere
		sphere = m_bounding_sphere;
		sphere[W] = 1.0f;
		sphere *= *p_ctxt->GetMatrix();
		sphere[W] = m_bounding_sphere[W];
					  
		if (SphereIsOutsideViewVolume(sphere))
		{
			return;
		}

		if (SphereIsInsideOuterVolume(sphere))
		{
			renderFlags &= ~(RENDERFLAG_CLIP|RENDERFLAG_CULL);
		}

		else if ((renderFlags & RENDERFLAG_CLIP) && !NeedsClipping(sphere))
		{
			renderFlags = renderFlags & ~RENDERFLAG_CLIP | RENDERFLAG_CULL;
		}
    }
	
	// applying a colour to a node (copied from Geomnode::Render)
	if (m_flags & NODEFLAG_COLOURED)
	{
		GEOMSTATINC(geom_stats_colored);
		// if there's no local context, copy the parent's
		if (p_ctxt == &ctxt)
		{
			p_ctxt = ctxt.Localise();
		}

		// copy node's colour to vu1 context
		p_ctxt->SetColour(this);
		renderFlags |= RENDERFLAG_COLOURED;
	}


	// is it a leaf node?
	if (m_flags & NODEFLAG_LEAF)
	{
		#ifdef	__NOPT_ASSERT__
		// Mick: check patch variables for debugging toggle of multi-passes
		if ((m_flags & gPassMask1) != gPassMask0)
		{	
			return;
		}
		#endif
		
		// uv wibble
		if (m_flags & NODEFLAG_UVWIBBLE)
		{
			// if there's no local context, copy the global one
			if (p_ctxt == &ctxt)
			{
				p_ctxt = ctxt.LocaliseExtend();
			}

			GEOMSTATINC(geom_stats_wibbleUV);
			p_ctxt->WibbleUVs(mp_uv_wibble, m_flags&NODEFLAG_EXPLICIT_UVWIBBLE);
		}

		// set the dma context
		dma::SetList(u3.mp_group);
		u3.mp_group->Used[render::Field] = true;

		// initial cnt tag, for sending row reg and z-sort key
		dma::Tag(dma::cnt, 1, 0);
		vif::NOP();
		vif::STROW((int)(m_bounding_sphere[0]*SUB_INCH_PRECISION),
				   (int)(m_bounding_sphere[1]*SUB_INCH_PRECISION),
				   (int)(m_bounding_sphere[2]*SUB_INCH_PRECISION), 0);

		// send transform context if necessary
		if (p_ctxt != u3.mp_group->pVu1Context)
		{
			int qwc = p_ctxt->IsExtended() ? EXT_CTXT_SIZE+2 : STD_CTXT_SIZE+2;
			dma::Tag(dma::ref, qwc|(qwc-1)<<16, p_ctxt->GetDma());
			vif::BASE(vu1::Loc);
			vif::OFFSET(0);
			vu1::Loc += qwc-1;
			u3.mp_group->pVu1Context = p_ctxt;
		}

		// render this node by adding to the chosen dma list
		uint vu1_flags = (renderFlags & RENDERFLAG_CLIP) ? CLIP :
						 (renderFlags & RENDERFLAG_CULL) ? CULL : PROJ;
		if (renderFlags & RENDERFLAG_COLOURED)
			vu1_flags |= COLR;
		if (render::EnableFog)
			vu1_flags |= FOGE;
		dma::pTag = dma::pLoc;		// must set this manually if we bypass the dma tag functions
		dma::Store32(u2.m_dma_tag_lo32, (uint32)u1.mp_dma);
		vif::BASE(vu1::Loc);
		vif::ITOP(vu1_flags);
		vu1::Loc += u2.m_dma_tag_lo32>>16;

		if (m_flags & NODEFLAG_BLACKFOG)
		{
			dma::Gosub(SET_FOGCOL,2);
			dma::pLoc -= 8;
			vif::FLUSHA();
			vif::NOP();
		}
	}
	else
	{
		// render any children
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			p_child->RenderAsSky(*p_ctxt, renderFlags);
		}
	}

}



void CGeomNode::SetBoneTransforms(Mth::Matrix *pMat)
{

	#if 0	// single-buffered
	
	// copy the given matrices into the matrix buffer
	for (int i = 0; i < m_num_bones; i++)
	{
		u4.mp_bone_transforms[i] = pMat[i];
	}

	#else	// double-buffered
	
	int startIndex = m_field ? m_num_bones : 0;

	// copy the given matrices into the correct half of the matrix buffer
	for (int i = 0; i < m_num_bones; i++)
	{
		u4.mp_bone_transforms[startIndex+i] = pMat[i];
	}

	// toggle field
	m_field = !m_field;

	#endif
	
}



#endif


bool CGeomNode::IsLeaf()
{
	return GET_FLAG(NODEFLAG_LEAF);
}


bool CGeomNode::IsObject()
{
	return GET_FLAG(NODEFLAG_OBJECT);
}


bool CGeomNode::IsActive()
{
	return GET_FLAG(NODEFLAG_ACTIVE);
}


bool CGeomNode::IsTransformed()
{
	return GET_FLAG(NODEFLAG_TRANSFORMED);
}


bool CGeomNode::IsSky()
{
	return GET_FLAG(NODEFLAG_SKY);
}


bool CGeomNode::IsColored()
{
	return GET_FLAG(NODEFLAG_COLOURED);
}


bool CGeomNode::IsSkeletal()
{
	return GET_FLAG(NODEFLAG_SKELETAL);
}


bool CGeomNode::IsEnvMapped()
{
	return GET_FLAG(NODEFLAG_ENVMAPPED);
}


bool CGeomNode::IsUVWibbled()
{
	return GET_FLAG(NODEFLAG_UVWIBBLE);
}

bool CGeomNode::IsBillboard()
{
	return GET_FLAG(NODEFLAG_BILLBOARD);
}


#ifdef	__NOPT_ASSERT__
bool CGeomNode::WasRendered()
{
	return GET_FLAG(NODEFLAG_WAS_RENDERED);
}
#endif

void CGeomNode::SetLeaf(bool yes)
{
	SET_FLAG(NODEFLAG_LEAF);
}


void CGeomNode::SetObject(bool yes)
{
	SET_FLAG(NODEFLAG_OBJECT);
}


void CGeomNode::SetActive(bool yes)
{
	SET_FLAG(NODEFLAG_ACTIVE);
}


void CGeomNode::SetTransformed(bool yes)
{
	SET_FLAG(NODEFLAG_TRANSFORMED);
}


void CGeomNode::SetSky(bool yes)
{
	SET_FLAG(NODEFLAG_SKY);
}


void CGeomNode::SetZPush0(bool yes)
{
	SET_FLAG(NODEFLAG_ZPUSH0);
}


void CGeomNode::SetZPush1(bool yes)
{
	SET_FLAG(NODEFLAG_ZPUSH1);
}


void CGeomNode::SetZPush2(bool yes)
{
	SET_FLAG(NODEFLAG_ZPUSH2);
}


void CGeomNode::SetZPush3(bool yes)
{
	SET_FLAG(NODEFLAG_ZPUSH3);
}


void CGeomNode::SetNoShadow(bool yes)
{
	SET_FLAG(NODEFLAG_NOSHADOW);
}


void CGeomNode::SetUVWibbled(bool yes)
{
	SET_FLAG(NODEFLAG_UVWIBBLE);
}


void CGeomNode::SetVCWibbled(bool yes)
{
	SET_FLAG(NODEFLAG_VCWIBBLE);
}


void CGeomNode::SetColored(bool yes)
{
	SET_FLAG(NODEFLAG_COLOURED);
}


void CGeomNode::SetSkinned(bool yes)
{
	SET_FLAG(NODEFLAG_SKINNED);
}


void CGeomNode::SetBillboard(bool yes)
{
	SET_FLAG(NODEFLAG_BILLBOARD);
}


void CGeomNode::SetSkeletal(bool yes)
{
	SET_FLAG(NODEFLAG_SKELETAL);
}


void CGeomNode::SetEnvMapped(bool yes)
{
	SET_FLAG(NODEFLAG_ENVMAPPED);
}


void CGeomNode::SetBlackFog(bool yes)
{
	SET_FLAG(NODEFLAG_BLACKFOG);
}


uint32 CGeomNode::GetChecksum()
{
	return m_checksum;
}


void CGeomNode::SetChecksum(uint32 checksum)
{
	m_checksum = checksum;
}


void CGeomNode::SetBoundingSphere(float x, float y, float z, float R)
{
	m_bounding_sphere[0] = x;
	m_bounding_sphere[1] = y;
	m_bounding_sphere[2] = z;
	m_bounding_sphere[3] = R;
}


void CGeomNode::SetBoundingBox(float x, float y, float z)
{
	m_bounding_box[0] = x;
	m_bounding_box[1] = y;
	m_bounding_box[2] = z;
}


uint8 *CGeomNode::GetDma()
{
	return (m_flags & NODEFLAG_LEAF) ? u1.mp_dma : NULL;
}


void CGeomNode::SetDma(uint8 *pDma)
{
	m_flags |= NODEFLAG_LEAF;
	u1.mp_dma = pDma;
}


void CGeomNode::SetDmaTag(uint8 *pDma)
{
	m_flags |= NODEFLAG_LEAF;
	u2.m_dma_tag_lo32 = dma::call<<28 | (((uint16 *)pDma)[1]&0x3FF)<<16 | 0;
	u1.mp_dma = pDma;
}


CGeomNode *CGeomNode::GetChild()
{
	return (m_flags & NODEFLAG_LEAF) ? NULL : u1.mp_child;
}


void CGeomNode::SetChild(CGeomNode *pChild)
{
	m_flags &= ~NODEFLAG_LEAF;
	u1.mp_child = pChild;
}

void CGeomNode::AddChild(CGeomNode *pChild)
{
	if (!(m_flags & NODEFLAG_LEAF) && u1.mp_child)
	{
		Dbg_Assert(!pChild->mp_sibling);

		CGeomNode *p_last_child = u1.mp_child;
		while (p_last_child->mp_sibling)
		{
			p_last_child = p_last_child->mp_sibling;
		}

		p_last_child->mp_sibling = pChild;
	} else {
		u1.mp_child = pChild;
	}

	m_flags &= ~NODEFLAG_LEAF;
}


CGeomNode *CGeomNode::GetSibling()
{
	return mp_sibling;
}


void CGeomNode::SetSibling(CGeomNode *pSibling)
{
	mp_sibling = pSibling;
}

void CGeomNode::SetSlaveLOD(CGeomNode *pLOD)
{
    CGeomNode *pLOD_list = this;

	while (pLOD_list->mp_next_LOD) {
		if (pLOD_list->mp_next_LOD->m_LOD_far_dist > pLOD->m_LOD_far_dist) {
			pLOD->mp_next_LOD = pLOD_list->mp_next_LOD;
			pLOD_list->mp_next_LOD = pLOD;
			return;
		}

		pLOD_list = pLOD_list->mp_next_LOD;
	}

	// If we get here, then we are at the end of the list
	pLOD_list->mp_next_LOD = pLOD; 
}
	
void CGeomNode::SetLODFarDist(float LOD_far_dist)
{
	m_LOD_far_dist = LOD_far_dist;
}

float CGeomNode::GetLODFarDist()
{
	return m_LOD_far_dist;
}

sGroup *CGeomNode::GetGroup()
{
	return u3.mp_group;
}


void CGeomNode::SetGroup(sGroup *pGroup)
{
	u3.mp_group = pGroup;
}


Mth::Matrix& CGeomNode::GetMatrix()
{
	Dbg_MsgAssert(IsTransformed(), ("trying to access matrix of a non-transformed CGeomNode"));
	Dbg_MsgAssert(u2.mp_transform, ("null matrix pointer"));
	Dbg_MsgAssert(!(m_flags&NODEFLAG_LEAF), ("Transformed leaf node\n"));
	return *u2.mp_transform;
}


void CGeomNode::SetMatrix(Mth::Matrix *p_mat)
{
	Dbg_MsgAssert(!(m_flags&NODEFLAG_LEAF), ("Transformed leaf node\n"));
	u2.mp_transform = (Mth::Matrix *)p_mat;
	SetTransformed(p_mat ? true : false);
}


void CGeomNode::SetUVWibblePointer(float *pData)
{
	mp_uv_wibble = pData;
}


void CGeomNode::SetVCWibblePointer(uint32 *pData)
{
	mp_vc_wibble = pData;
}


void CGeomNode::SetColor(uint32 rgba)
{
	if (rgba == 0x80808080)
	{
		// clear the colored falg if set to the neutral color
		// as things render a lot quicker that way
		// also, applying the color gives a slightly different
		// value from not applying any color
		SetColored(false);
	}
	
	m_colour = rgba;
}


uint32 CGeomNode::GetColor()
{
	if (IsColored())
	{
		return m_colour;
	} else {
		return 0x80808080;
	}
}


void CGeomNode::SetLightGroup(CLightGroup *p_light_group)
{
	if (!IsLeaf())
	{
		u3.mp_light_group = p_light_group;
	}
}


CLightGroup * CGeomNode::GetLightGroup()
{
	if (!IsLeaf())
	{
		return u3.mp_light_group;
	} else {
		return NULL;
	}
}


void CGeomNode::SetVisibility(uint8 mask)
{
	m_flags &= ~ALL_VISIBILITY_BITS;		// Take out old visibility bits
	m_flags |= (mask << VISIBILITY_FLAG_BIT);
}


uint8 CGeomNode::GetVisibility()
{
	return (m_flags & ALL_VISIBILITY_BITS) >> VISIBILITY_FLAG_BIT;
}


void CGeomNode::SetBoneIndex(sint8 index)
{
	m_bone_index = index;
}


sint8 CGeomNode::GetBoneIndex()
{
	return m_bone_index;
}

void	CGeomNode::CountMetrics(CGeomMetrics *p_metrics)
{
	p_metrics->m_total++;
	if (IsObject()) 
	{
		p_metrics->m_object++;				
	}
	if (IsLeaf())
	{
		p_metrics->m_leaf++;
		p_metrics->m_verts += dma::GetNumVertices(u1.mp_dma);
		p_metrics->m_polys += dma::GetNumTris(u1.mp_dma);
	}
	else
	{
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			p_child->CountMetrics(p_metrics);
		}
	}
	
}


static Mth::Vector	*p_verts = NULL; 

void	CGeomNode::RenderWireframe(int mode)
{
#ifdef	__PLAT_NGPS__
	int lines = 0;
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
	p_verts = new Mth::Vector[100000];
	Mem::Manager::sHandle().PopContext();
	RenderWireframeRecurse(mode, lines);
	delete p_verts;
	p_verts = NULL;
#endif
}

void	CGeomNode::RenderWireframeRecurse(int mode, int &lines)
{

#ifdef	__PLAT_NGPS__
	int sectors = 0;
	int vert_count=0;

	if (IsLeaf())
	{
		if (lines < 95000)  // clmap number of lines, otherwise we crash
		{
			sectors++;
		
			//
			// Do renderable geometry
			int verts = GetNumVerts();
			if (verts>2)
			{
				vert_count += verts;  
				  
				#define	peak 500
				int n=GetNumPolys();
				int r,g,b;
				r=g=b=0;
				if (n <= peak )
				{
					r = (255 * (n)) / peak;  	// r ramps up (black to red)	
				}
				else if (n <= peak * 2)
				{
					r = 255;
					b = (255 * (n - peak) / (peak));	// b&g ramps up (to white)
					g = b;
					
				}
				else
				{
					r = g = b = 255;
				}
				uint32 rgb = (b<<16)|(g<<8)|r;	
	
				
				if (mode == 4)
				{
					// generate a random looking number that's basically a hash of some
					// values in the node, so it stays the same
					uint32 xxx = ((uint32)(this) * verts  * n);
					NxPs2::BeginLines3D(0x80000000 + (0x00ffffff & xxx));		
				}
				else
				{
					NxPs2::BeginLines3D(0x80000000 + (0x00ffffff & rgb));		
				}
				
				GetVerts(p_verts);
				Mth::Vector *p_vert = p_verts+1;
				for (int i = 1; i < verts; i++)
				{
					NxPs2::DrawLine3D((*p_vert)[X],(*p_vert)[Y],(*p_vert)[Z],p_vert[-1][X],p_vert[-1][Y],p_vert[-1][Z]);								
					p_vert++;
					lines++;
				} // end for
		
				NxPs2::EndLines3D();
			} // end if
		}				
	}
	else
	{
		if (WasRendered())
		{
			for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
			{
				p_child->RenderWireframeRecurse(mode, lines);
			}
		}
	}
#endif
}


int	CGeomNode::GetNumVerts()
{
	int num_verts = 0;

	if (IsLeaf())
	{
		num_verts = dma::GetNumVertices(u1.mp_dma);

	}
	else
	{
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				num_verts += mp_next_LOD->GetNumVerts();
			}
		}

		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			num_verts += p_child->GetNumVerts();
		}
	}

	return num_verts;
}


int	CGeomNode::GetNumPolys()
{
	int num_polys = 0;

	if (IsLeaf())
	{
		num_polys = dma::GetNumTris(u1.mp_dma);

	}
	else
	{
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				num_polys += mp_next_LOD->GetNumPolys();
			}
		}

		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			num_polys += p_child->GetNumPolys();
		}
	}

	return num_polys;
}


int	CGeomNode::GetNumBasePolys()
{
	int num_base_polys = 0;

	if (IsLeaf())
	{
		if ((m_flags & NODEFLAG_ZPUSH) == 0)		// check it's the base layer
		{
			num_base_polys = dma::GetNumTris(u1.mp_dma);
		}
	}
	else
	{
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				num_base_polys += mp_next_LOD->GetNumBasePolys();
			}
		}

		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			num_base_polys += p_child->GetNumBasePolys();
		}
	}

	return num_base_polys;
}


int	CGeomNode::GetNumObjects()
{
	int num_objects = 0;

	// Check self
	if (IsObject())
	{
		num_objects++;
	}

	// Recursively check children
	if (!IsLeaf())
	{
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			num_objects += p_child->GetNumObjects();
		}
	}

	return num_objects;
}


CGeomNode*	CGeomNode::GetObject(int num, bool root)
{
	static int found_so_far;

	if (root)
	{
		found_so_far = -1;
		//Dbg_Message("Start looking for #%d", num);
	}

	// Check self
	if (IsObject())
	{
		if (++found_so_far == num)
		{
			//Dbg_Message("Found object #%d", num);
			return this;
		}

		//Dbg_Message("Found object #%d; looking for #%d", found_so_far, num);
	}

	CGeomNode *p_found = NULL;

	// Recursively check children
	if (!IsLeaf())
	{
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			p_found = p_child->GetObject(num, false);
			if (p_found)
			{
				break;
			}
		}

	}

	if (root)
	{
		Dbg_Assert(p_found);
	}

	return p_found;
}


#ifdef __PLAT_WN32__

void CGeomNode::Preprocess(uint8 *p_baseAddress)
{
	// recursively process any children
	CGeomNode *p_child, *p_sibling;
	if (!(m_flags & NODEFLAG_LEAF))
	{
		for (p_child=u1.mp_child; p_child; p_child=p_sibling)
		{
			p_sibling = p_child->mp_sibling;
			p_child->Preprocess(p_baseAddress);
		}
	}

	// recursively process any LODs
	if (mp_next_LOD)
	{
		mp_next_LOD->Preprocess(p_baseAddress);
	}

	if (!(m_flags & NODEFLAG_LEAF))
	{
		// convert transform pointer to offset
		if (u2.mp_transform)
		{
			u2.mp_transform = (Mth::Matrix *) ( (uint8 *)u2.mp_transform - p_baseAddress );
		}
		else
		{
			u2.mp_transform = (Mth::Matrix *)(-1);
		}
	}

	if (m_flags & NODEFLAG_LEAF)
	{
		// convert dma pointer to offset
		if (u1.mp_dma)
		{
			u1.mp_dma = (uint8 *)( u1.mp_dma - p_baseAddress );
		}
		else
		{
			u1.mp_dma = (uint8 *)(-1);
		}
	}
	else
	{
		// convert child pointer to offset
		if (u1.mp_child)
		{
			u1.mp_child = (CGeomNode *) ( (uint8 *)u1.mp_child - p_baseAddress );
		}
		else
		{
			u1.mp_child = (CGeomNode *)(-1);
		}
	}

	// leave group checksum alone

	// convert sibling pointer to offset
	if (mp_sibling)
	{
		mp_sibling = (CGeomNode *) ( (uint8 *)mp_sibling - p_baseAddress );
	}
	else
	{
		mp_sibling = (CGeomNode *)(-1);
	}

	// convert LOD pointer to offset
	if (mp_next_LOD)
	{
		mp_next_LOD = (CGeomNode *) ( (uint8 *)mp_next_LOD - p_baseAddress );
	}
	else
	{
		mp_next_LOD = (CGeomNode *)(-1);
	}

	// convert uv wibble pointer to offset
	if (m_flags & NODEFLAG_UVWIBBLE)
	{
		if (!(m_flags & NODEFLAG_ENVMAPPED) || !(m_flags & NODEFLAG_LEAF))		// can't wibble environment-mapped leaves
		{
			mp_uv_wibble = (float *) ( (uint8 *)mp_uv_wibble - p_baseAddress );
		}
		else
		{
			mp_uv_wibble = (float *)(-1);
		}
	}

	// convert vc wibble pointer to offset
	if (m_flags & NODEFLAG_VCWIBBLE)
	{
		// first convert mesh info
		uint32 *p_meshdata = mp_vc_wibble;
		int seq, num_seqs, vert, num_verts;

		num_seqs = *p_meshdata++;
		for (seq=0; seq<num_seqs; seq++)
		{
			// convert sequence pointer
			*(uint32 **)p_meshdata = (uint32 *) ( (uint8 *)*p_meshdata - p_baseAddress );
			p_meshdata++;

			// convert rgba pointers
			num_verts = *p_meshdata++;
			for (vert=0; vert<num_verts; vert++)
			{
				*(uint32 **)p_meshdata = (uint32 *) ( (uint8 *)*p_meshdata - p_baseAddress );
				p_meshdata++;
			}
		}

		// now the wibble pointer
		mp_vc_wibble = (uint32 *) ( (uint8 *)mp_vc_wibble - p_baseAddress );
	}
	else
	{
		mp_vc_wibble = (uint32 *)(-1);
	}

}

#endif


#ifdef __PLAT_NGPS__

static int SNL_Depth = 0;
												  
void		StripNiceLeaves(CGeomNode *p_parent)
{
	SNL_Depth++;
	CGeomNode *p_child = p_parent->GetChild();
	// iterate over siblings without recursion
	while (p_child)	
	{
		
		printf ("%2d:",SNL_Depth);
		for (int i=i;i<SNL_Depth;i++)
			printf ("    ");
		printf ("0x%08x\n",p_child->GetFlags());
		
		CGeomNode *p_next_child = p_child->GetSibling();
		
		// if child not a leaf, then just recurse down
		if (! (p_child->GetFlags() & NODEFLAG_LEAF))
		{
			StripNiceLeaves(p_child);
		}
		else
		{
		
			// it's a leaf node, so maybe strip it, and then carry on to the siblings
			if (((p_child->GetFlags() & (NODEFLAG_ACTIVE		|
				 NODEFLAG_SKY			|
				 NODEFLAG_TRANSFORMED	|
				 NODEFLAG_LEAF			|
				 NODEFLAG_OBJECT		|
				 NODEFLAG_COLOURED		|
				 NODEFLAG_SKELETAL		|
				 NODEFLAG_BILLBOARD		|
				 NODEFLAG_UVWIBBLE		|
				 NODEFLAG_VCWIBBLE		|
				 NODEFLAG_ENVMAPPED))

			==  (NODEFLAG_ACTIVE		|
				 NODEFLAG_LEAF)))
			
			{
				// strip it out (eventually add it to a quick-render list)
				if (p_parent->GetChild() == p_child)
				{
					// just need to fix up the parent
					p_parent->SetChild(p_next_child);
				}
				else
				{
					// find the elder sibling and fix up his sibling
					CGeomNode *p_elder_sibling = p_parent->GetChild();
					while (p_elder_sibling->GetSibling() != p_child)
					{
						p_elder_sibling = p_elder_sibling->GetSibling();
						Dbg_Assert(p_elder_sibling);	// catch infinite loop
					}
					p_elder_sibling->SetSibling(p_next_child);							
				}
				// p_child is now an orphan
				// TODO - add to quick render list ...
			}
		}
		p_child = p_next_child;
	}	
	SNL_Depth--;
}



CGeomNode* CGeomNode::sProcessInPlace(uint8* pPipData, uint32 loadFlags)
{
	Dbg_MsgAssert(((int)(pPipData) & 0xf) == 0,("pPipData (0x%x) not multiple of 16 \n"
													,(int)pPipData));

	// Jump to beginning of CGeomNode data
	pPipData += *(uint32 *)pPipData;
	
	// from header, get address of root node
	CGeomNode *p_root_node = (CGeomNode *) (pPipData + *(uint32 *)pPipData);

	Dbg_MsgAssert(((int)(p_root_node) & 0xf) == 0,("p_root_node (0x%x) not multiple of 16, .SCN.PS2 file corrupt... \n"
													,(int)p_root_node));



	// recursively process the tree
	#ifdef	__NOPT_ASSERT__
//	int leaves = 
	#endif
	p_root_node->ProcessNodeInPlace(pPipData);
	#ifdef	__NOPT_ASSERT__
//	printf (">>>>> Number of leaves in geom PIP file = %d\n",leaves);
	#endif


	//StripNiceLeaves(p_root_node);	

	// add the root node to the world or to the database
	p_root_node->BecomesChildOf((loadFlags & LOADFLAG_RENDERNOW) ? &sWorld : &sDatabase);

	// set the root node's sky flag if loaded as a sky
	if (loadFlags & LOADFLAG_SKY)
	{
		p_root_node->SetSky(true);
	}

	// return the root node pointer
	return p_root_node;
}

void * CGeomNode::sGetHierarchyArray(uint8 *pPipData, int& size)
{
	Dbg_MsgAssert(((int)(pPipData) & 0xf) == 0,("pPipData (0x%x) not multiple of 16 \n"
													,(int)pPipData));

	uint32 *pPip32Data = (uint32 *) pPipData;

	pPip32Data++;		// skip CGeomNode offset

	// Get array pointer and size
	void *p_array = (pPipData + *(pPip32Data++));
	size = *pPip32Data;

	if (size)
	{
		return p_array;
	} else {
		return NULL;
	}
}

void CGeomNode::AddToTree(uint32 loadFlags)
{
	BecomesChildOf((loadFlags & LOADFLAG_RENDERNOW) ? &sWorld : &sDatabase);

	// set the root node's sky flag if loaded as a sky
	if (loadFlags & LOADFLAG_SKY)
	{
		SetSky(true);
	}
}

void CGeomNode::Cleanup()
{
	// remove from any node it belongs to
	RemoveFrom(&sWorld);
	RemoveFrom(&sDatabase);
}


CGeomNode* CGeomNode::CreateInstance(Mth::Matrix *pMat, CGeomNode *p_parent)
{
	// copy node
	CGeomNode *p_node = new CGeomNode(*this);

	// flag as an instance
	p_node->m_flags |= NODEFLAG_INSTANCE;

	// set matrix
	p_node->SetMatrix(pMat);

	// add to world
	if (p_parent)
	{
		p_node->BecomesChildOf(p_parent);
	}
	else
	{
		p_node->BecomesChildOf(&sWorld);
	}

	// activate!
	p_node->SetActive(true);

	// return new node
	return p_node;
}


// skeletal version
CGeomNode* CGeomNode::CreateInstance(Mth::Matrix *pMat, int numBones, Mth::Matrix *pBoneTransforms, CGeomNode *p_parent)
{
	// copy node
	CGeomNode *p_node = new CGeomNode(*this);

	// flag as a skeletal instance
	p_node->m_flags |= NODEFLAG_SKELETAL|NODEFLAG_INSTANCE;

	// set matrix and bone data
	p_node->SetMatrix(pMat);
	p_node->m_num_bones = numBones;
	p_node->u4.mp_bone_transforms = pBoneTransforms;
	p_node->m_field = 0;

	// add to world
	if (p_parent)
	{
		p_node->BecomesChildOf(p_parent);
	}
	else
	{
		p_node->BecomesChildOf(&sWorld);
	}

	// activate!
	p_node->SetActive(true);

	// return new node
	return p_node;
}


void CGeomNode::DeleteInstance()
{
	Dbg_MsgAssert(m_flags&NODEFLAG_INSTANCE, ("attempt to delete a non-instance"));

	bool removed;
	removed = RemoveFrom(&sWorld);
	removed = removed || RemoveFromChildrenOf(&sWorld);		// Check all cloned scenes, too

	Dbg_MsgAssert(removed, ("Couldn't remove instanced CGeomNode from CGeomNode tree"));

	delete this;
}

CGeomNode* CGeomNode::CreateCopy(CGeomNode *p_parent, bool root)
{
	// copy node
	CGeomNode *p_node = new CGeomNode(*this);

	// clear some pointers
	p_node->mp_sibling = NULL;

	// add to world if this is the top node
	if (root)
	{
		if (p_parent)
		{
			p_node->BecomesChildOf(p_parent);
		}
		else
		{
			p_node->BecomesChildOf(&sWorld);
		}
	}

	// activate!
	p_node->SetActive(true);

	// Also copy data and tree underneath
	if (IsLeaf())
	{
		// Copy the dma data
		Dbg_Assert(u1.mp_dma);
		p_node->u1.mp_dma = new uint8[dma::GetDmaSize(u1.mp_dma)];
		Dbg_Assert(!((uint32) p_node->u1.mp_dma & 0xf))
		dma::Copy(u1.mp_dma, p_node->u1.mp_dma);
	} else {
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				p_node->mp_next_LOD = mp_next_LOD->CreateCopy(NULL, false);
			}
		}

		// Recursively copy children
		p_node->u1.mp_child = NULL;
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			p_node->AddChild(p_child->CreateCopy(NULL, false));
		}
	}

	// return new node
	return p_node;
}

void WaitForRendering();

// Recursivly deletes an object and all its children
void CGeomNode::DeleteCopy(bool root)
{
	// Garrett: Kinda hacky, but gets the job done.  Make sure we aren't rendering before
	// we release DMA buffers
	WaitForRendering();  

	Dbg_MsgAssert(!(m_flags & NODEFLAG_INSTANCE), ("attempt to delete an instance"));

	if (root)
	{
		bool removed;
		removed = RemoveFrom(&sWorld);
		removed = removed || RemoveFromChildrenOf(&sWorld);		// Check all cloned scenes, too

		Dbg_MsgAssert(removed, ("Couldn't remove copied CGeomNode from CGeomNode tree"));
	}

	// Also delete data and tree underneath
	if (IsLeaf())
	{
		// Delete the dma data
		Dbg_Assert(u1.mp_dma);
		delete [] u1.mp_dma;
	} else {
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				mp_next_LOD->DeleteCopy(false);
			}
		}

		// Recursively delete children
		// (Mick) restructured this because DeleteCopy will delete the object it is run on
		CGeomNode *p_child=u1.mp_child;
		while ( p_child)
		{
			CGeomNode *p_next=p_child->mp_sibling;
			p_child->DeleteCopy(false);
			p_child = p_next;
		}
	}

	delete this;	   // <<<< Warning:  deletes 'this'
}

void CGeomNode::Translate(const Mth::Vector & delta_pos)
{
	Dbg_MsgAssert(!(m_flags & NODEFLAG_INSTANCE), ("attempt to translate an instance"));

	// Update bounding sphere
	m_bounding_sphere[X] += delta_pos[X];
	m_bounding_sphere[Y] += delta_pos[Y];
	m_bounding_sphere[Z] += delta_pos[Z];

#if 0
	// Test for Park Editor
	Dbg_Message("Translating by (%.8f, %.8f, %.8f)", delta_pos[X], delta_pos[Y], delta_pos[Z]);
	int delta_x = (int) (delta_pos[X] * SUB_INCH_PRECISION);
	int delta_y = (int) (delta_pos[Y] * SUB_INCH_PRECISION);
	int delta_z = (int) (delta_pos[Z] * SUB_INCH_PRECISION);
	Dbg_MsgAssert(!(delta_x & 0xF), ("Fraction in X: (%x, %x, %x)", delta_x, delta_y, delta_z));
	Dbg_MsgAssert(!(delta_y & 0xF), ("Fraction in Y: (%x, %x, %x)", delta_x, delta_y, delta_z));
	Dbg_MsgAssert(!(delta_z & 0xF), ("Fraction in Z: (%x, %x, %x)", delta_x, delta_y, delta_z));
#endif

	// Update whole tree
	if (IsLeaf())
	{
		// Get the dma data
		Dbg_Assert(u1.mp_dma);

		if (dma::GetBitLengthXYZ(u1.mp_dma) == 32)
		{
			// Calculate int versions
			int delta_x = (int) (delta_pos[X] * SUB_INCH_PRECISION);
			int delta_y = (int) (delta_pos[Y] * SUB_INCH_PRECISION);
			int delta_z = (int) (delta_pos[Z] * SUB_INCH_PRECISION);

			int num_verts = dma::GetNumVertices(u1.mp_dma);

			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
			int32 *p_verts = new int32[num_verts * 4];
			dma::ExtractXYZs(u1.mp_dma, (uint8 *) p_verts);		

			for (int i = 0; i < num_verts; i++) {
				p_verts[(i * 4) + 0] += delta_x;
				p_verts[(i * 4) + 1] += delta_y;
				p_verts[(i * 4) + 2] += delta_z;
			}

			dma::ReplaceXYZs(u1.mp_dma, (uint8 *) p_verts);		

			delete [] p_verts;

			Mem::Manager::sHandle().PopContext();
		}
		else
		{
			Dbg_MsgAssert(dma::GetBitLengthXYZ(u1.mp_dma)==16, ("oh dear, should've been either 16 or 32..."));
		}
	} else {
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				mp_next_LOD->Translate(delta_pos);
			}
		}

		// Recursively update children
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			p_child->Translate(delta_pos);
		}
	}
}

void CGeomNode::RotateY(const Mth::Vector & world_origin, Mth::ERot90 rot_y, bool root)
{
	Dbg_MsgAssert(!(m_flags & NODEFLAG_INSTANCE), ("attempt to translate an instance"));

	if (root)
	{
		Translate(-world_origin);
	}

	// Update bounding box
	// Since they are relative to width and height, we just need to possibly swap X and Z
	if ((int) rot_y & 1)
	{
		float temp = m_bounding_box[X];
		m_bounding_box[X] = m_bounding_box[Z];
		m_bounding_box[Z] = temp;
	}

	// Also rotate bounding sphere position
	m_bounding_sphere.RotateY90(rot_y);

	// Update whole tree
	if (IsLeaf())
	{
		// Delete the dma data
		Dbg_Assert(u1.mp_dma);

		int num_verts = dma::GetNumVertices(u1.mp_dma);

		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
		int32 *p_verts = new int32[num_verts * 4];
		dma::ExtractXYZs(u1.mp_dma, (uint8 *) p_verts);		

		for (int i = 0; i < num_verts; i++) {
			Mth::RotateY90(rot_y,
						   p_verts[(i * 4) + 0],
						   p_verts[(i * 4) + 1],
						   p_verts[(i * 4) + 2]);
		}

		dma::ReplaceXYZs(u1.mp_dma, (uint8 *) p_verts);		

		delete [] p_verts;

		Mem::Manager::sHandle().PopContext();
	} else {
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				mp_next_LOD->RotateY(world_origin, rot_y, false);
			}
		}

		// Recursively update children
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			p_child->RotateY(world_origin, rot_y, false);
		}
	}

	if (root)
	{
		Translate(world_origin);
	}
}

void CGeomNode::Scale(const Mth::Vector & world_origin, const Mth::Vector & scale, bool root)
{
	Dbg_MsgAssert(!(m_flags & NODEFLAG_INSTANCE), ("attempt to scale an instance"));

	if (root)
	{
		Translate(-world_origin);
	}

	// Update bounding box
	m_bounding_box[X] *= scale[X];
	m_bounding_box[Y] *= scale[Y];
	m_bounding_box[Z] *= scale[Z];

#if 0	// Don't do this since it is a local scale
	// Also scale bounding sphere position
	// Update bounding sphere
	m_bounding_sphere[X] *= scale[X];
	m_bounding_sphere[Y] *= scale[Y];
	m_bounding_sphere[Z] *= scale[Z];
#endif

	// Garrett: The radius calculation is not accurate, but so far, it works.  It will always round up.
	// We have to assume the largest radius
	float radius_scale = sqrtf((scale[X] * scale[X]) + (scale[Y] * scale[Y]) + (scale[Z] + scale[Z]));
	m_bounding_sphere[W] *= radius_scale;
	// But see if the bounding box radius is smaller
	float box_radius = sqrtf((m_bounding_box[X] * m_bounding_box[X]) + (m_bounding_box[Y] * m_bounding_box[Y]) + (m_bounding_box[Z] * m_bounding_box[Z]));
	//Dbg_Message("Calculated sphere radius %f; Bounding box radius %f", m_bounding_sphere[W] , box_radius);
	m_bounding_sphere[W] = Mth::Min(box_radius, m_bounding_sphere[W]);

	// Update whole tree
	if (IsLeaf())
	{
		// Get the dma data
		Dbg_Assert(u1.mp_dma);

		// Calculate int versions
		int scale_x = (int) (scale[X] * SUB_INCH_PRECISION);
		int scale_y = (int) (scale[Y] * SUB_INCH_PRECISION);
		int scale_z = (int) (scale[Z] * SUB_INCH_PRECISION);

		// Check to see that we didn't go to zero
		Dbg_MsgAssert(scale_x, ("CGeomNode::Scale(): X scale went to 0 in integer conversion. Original value %f", scale[X]));
		Dbg_MsgAssert(scale_y, ("CGeomNode::Scale(): Y scale went to 0 in integer conversion. Original value %f", scale[Y]));
		Dbg_MsgAssert(scale_z, ("CGeomNode::Scale(): Z scale went to 0 in integer conversion. Original value %f", scale[Z]));

		int num_verts = dma::GetNumVertices(u1.mp_dma);

		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
		int32 *p_verts = new int32[num_verts * 4];
		dma::ExtractXYZs(u1.mp_dma, (uint8 *) p_verts);		

		// SUB_INCH_PRECISION must be a whole number
		Dbg_Assert( ((int) (SUB_INCH_PRECISION * 1000.0f)) == (((int) SUB_INCH_PRECISION) * 1000) );

		for (int i = 0; i < num_verts; i++) {
			p_verts[(i * 4) + 0] = (p_verts[(i * 4) + 0] * scale_x) / ((int) SUB_INCH_PRECISION);
			p_verts[(i * 4) + 1] = (p_verts[(i * 4) + 1] * scale_y) / ((int) SUB_INCH_PRECISION);
			p_verts[(i * 4) + 2] = (p_verts[(i * 4) + 2] * scale_z) / ((int) SUB_INCH_PRECISION);
		}

		dma::ReplaceXYZs(u1.mp_dma, (uint8 *) p_verts);		

		delete [] p_verts;

		Mem::Manager::sHandle().PopContext();
	} else {
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				mp_next_LOD->Scale(world_origin, scale, false);
			}
		}

		// Recursively update children
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			p_child->Scale(world_origin, scale, false);
		}
	}

	if (root)
	{
		Translate(world_origin);
	}
}

void 		CGeomNode::GetVerts(Mth::Vector *p_verts, bool root)
{
	// Not thread safe
	static int vert_index;
	if (root)
	{
		vert_index = 0;
	}

	if (IsLeaf())
	{
		// Get the dma data
		Dbg_Assert(u1.mp_dma);
		int num_verts = dma::GetNumVertices(u1.mp_dma);

		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

		int32 *p_raw_verts = (int32 *) &p_verts[vert_index];
		//int32 *p_raw_verts = new int32[num_verts * 4];
		dma::ExtractXYZs(u1.mp_dma, (uint8 *) p_raw_verts);		

		// Convert center to integer
		// (Mick:  Changed to always have a center offset, as even 32 bit coordinates have one
		// previous, the code assumed that 32 bit coordinate were absolute world coords, and so
		// did not add in the "center" offset
		int32 center[4];
		dma::ConvertFloatToXYZ(center,  m_bounding_sphere);

		for (int i = 0; i < num_verts; i++, vert_index++) 
		{
			dma::ConvertXYZToFloat(p_verts[vert_index], &(p_raw_verts[i * 4]), center);
		}

//		delete [] p_raw_verts;

		Mem::Manager::sHandle().PopContext();
	} else {
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				mp_next_LOD->GetVerts(p_verts, false);
			}
		}

		// Get children verts
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			p_child->GetVerts(p_verts, false);
		}
	}
}

void 		CGeomNode::SetVerts(Mth::Vector *p_verts, bool root)
{
	// Not thread safe
	static int vert_index;
	if (root)
	{
		vert_index = 0;
	}

	if (IsLeaf())
	{
		// Get the dma data
		Dbg_Assert(u1.mp_dma);
		int num_verts = dma::GetNumVertices(u1.mp_dma);

		// Recalculate the bounding data BEFORE setting the DMA data, since it will change the
		// center of the bounding sphere.
		recalculate_bounding_data(&p_verts[vert_index], num_verts);

		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

		int32 *p_raw_verts = new int32[num_verts * 4];

		// Convert center to integer
		int32 center[4];
		if (dma::GetBitLengthXYZ(u1.mp_dma) < 32)
		{
			dma::ConvertFloatToXYZ(center,  m_bounding_sphere);
		}
		

		for (int i = 0; i < num_verts; i++, vert_index++) {
			if (dma::GetBitLengthXYZ(u1.mp_dma) == 32)
			{
				dma::ConvertFloatToXYZ(&(p_raw_verts[i * 4]), p_verts[vert_index]);
			} else {
				dma::ConvertFloatToXYZ(&(p_raw_verts[i * 4]), p_verts[vert_index], center);
			}
		}

		// Replaces only XYZs (not W)
		dma::ReplaceXYZs(u1.mp_dma, (uint8 *) p_raw_verts, true);		

		delete [] p_raw_verts;

		Mem::Manager::sHandle().PopContext();
	} else {
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				mp_next_LOD->SetVerts(p_verts, false);
			}
		}

		// Set children verts
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			p_child->SetVerts(p_verts, false);
		}

		// Recalculate the bounding data at this level using all the verts (assuming this is
		// going to be the object node).
		Dbg_MsgAssert(IsObject(), ("Don't know how to recalculate the bounding data for this node: Not an object"));
		int num_verts = GetNumVerts();
		recalculate_bounding_data(p_verts, num_verts);
	}
}

void 		CGeomNode::recalculate_bounding_data(Mth::Vector *p_verts, int num_verts)
{
	Mth::Vector min, max, center, half_vec;
	float radius;
	int i;

	// calculate bounding box for the mesh
	min[0] = min[1] = min[2] = 1e30f;
	max[0] = max[1] = max[2] = -1e30f;

	for (i=0; i<num_verts; i++)
	{
		if (p_verts[i][X] < min[X])
			min[X] = p_verts[i][X];
		if (p_verts[i][Y] < min[Y])
			min[Y] = p_verts[i][Y];
		if (p_verts[i][Z] < min[Z])
			min[Z] = p_verts[i][Z];

		if (p_verts[i][X] > max[X])
			max[X] = p_verts[i][X];
		if (p_verts[i][Y] > max[Y])
			max[Y] = p_verts[i][Y];
		if (p_verts[i][Z] > max[Z])
			max[Z] = p_verts[i][Z];
	}

	// calculate bounding sphere for the mesh
	center = (max+min)*0.5f;
	radius = 0.0f;
	for (i=0; i<num_verts; i++)
	{
		float len = (p_verts[i] - center).Length();
		if (len > radius)
			radius = len;
	}

	// Update bounding box data
	half_vec = 0.5f * (max - min);
	m_bounding_box[X] = half_vec[X];
	m_bounding_box[Y] = half_vec[Y];
	m_bounding_box[Z] = half_vec[Z];

	// Update bounding sphere data
	m_bounding_sphere = center;
	m_bounding_sphere[W] = radius;
}

void 		CGeomNode::GetColors(uint32 *p_colors, bool root)
{
	// Not thread safe
	static int vert_index;
	if (root)
	{
		vert_index = 0;
	}

	if (IsLeaf())
	{
		// Get the dma data
		Dbg_Assert(u1.mp_dma);
		int num_verts = dma::GetNumVertices(u1.mp_dma);

		dma::ExtractRGBAs(u1.mp_dma, (uint8 *) &(p_colors[vert_index]));		
		vert_index += num_verts;
	} else {
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				mp_next_LOD->GetColors(p_colors, false);
			}
		}

		// Get children colors
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			p_child->GetColors(p_colors, false);
		}
	}
}

void 		CGeomNode::SetColors(uint32 *p_colors, bool root)
{
	// Not thread safe
	static int vert_index;
	if (root)
	{
		vert_index = 0;
	}

	if (IsLeaf())
	{
		// Set the dma data
		Dbg_Assert(u1.mp_dma);
		int num_verts = dma::GetNumVertices(u1.mp_dma);

		dma::ReplaceRGBAs(u1.mp_dma, (uint8 *) &(p_colors[vert_index]));		
		vert_index += num_verts;
	} else {
		// Check if we have LODs
		if (IsObject())
		{
			if (mp_next_LOD)
			{
				mp_next_LOD->SetColors(p_colors, false);
			}
		}

		// Set children colors
		for (CGeomNode *p_child=u1.mp_child; p_child; p_child=p_child->mp_sibling)
		{
			p_child->SetColors(p_colors, false);
		}
	}
}

/*
	vc wibble material data block format
	------------------------------------

	int num_seqs;
	for (seq=0; seq<num_seqs; seq++)
	{
		int num_keys;
		int phase_shift;
		for (key=0; key<num_keys; key++)
		{
			int start_time;
			float colour[4];
		}
	}


	vc wibble mesh data block format
	--------------------------------

	int num_seqs;
	for (seq=0; seq<num_seqs; seq++)
	{
		uint32 *p_sequence;
		int num_wibbled_verts;
		for (vert=0; vert<num_wibbled_verts; vert++)
		{
			uint32 *p_rgba;
		}
	}

*/

void CGeomNode::WibbleVCs()
{
	int seq, num_seqs, key, num_keys, time, start_time, end_time, period, vert, num_verts, phase_shift;
	uint32 rgba, *p_rgba;
	float *p_key;
	float t,r,g,b,a;
	uint32 *p_meshdata, *p_matdata;

	p_meshdata = mp_vc_wibble;

	// get # animation sequences
	num_seqs = *p_meshdata++;

	for (seq=0; seq<num_seqs; seq++)
	{
		p_matdata = *(uint32 **)p_meshdata;
		p_meshdata++;

		// get # keyframes for this sequence
		num_keys = *p_matdata++;

		// get phase-shift
		phase_shift = *p_matdata++;

		// time parameters
		start_time	= p_matdata[0];
		end_time	= p_matdata[num_keys*5-5];
		period		= end_time - start_time;
		time		= start_time + (render::sTime /*(int)Tmr::GetTime()*/ + phase_shift) % period;

		// keep time in range
		if (time < start_time) time = start_time;
		if (time > end_time) time = end_time;

		// locate the keyframe
		for (key=num_keys-1,p_key=(float *)p_matdata+5*num_keys-5; key>=0; key--,p_key-=5)
		{
			if (time >= *(int *)p_key)
			{
				break;
			}
		}

		// parameter expressing how far we are between between this keyframe and the next
		t = (float)(time - ((int *)p_key)[0]) / (float)(((int *)p_key)[5] - ((int *)p_key)[0]);

		#if 0
		// for debugging a TT bug...
		for (int i=1; i<=9; i++)
			Dbg_MsgAssert(p_key[i]>=0 && p_key[i]<=255, ("sTime=%d, num_keys=%d, key=%d, p_key[1]=%g, p_key[2]=%g, p_key[3]=%g, p_key[4]=%g, p_key[5]=%g, p_key[6]=%g, p_key[7]=%g, p_key[8]=%g, p_key[9]=%g, time=%d, start_time=%d, end_time=%d, period=%d, t=%g", \
														  render::sTime,	\
														  num_keys,			\
														  key,				\
														  p_key[1],			\
														  p_key[2],			\
														  p_key[3],			\
														  p_key[4],			\
														  p_key[5],			\
														  p_key[6],			\
														  p_key[7],			\
														  p_key[8],			\
														  p_key[9],			\
														  time,				\
														  start_time,		\
														  end_time,			\
														  period,			\
														  t));
		#endif

		// interpolate between colours
		r = (1.0f-t) * p_key[1] + t * p_key[6];
		g = (1.0f-t) * p_key[2] + t * p_key[7];
		b = (1.0f-t) * p_key[3] + t * p_key[8];
		a = (1.0f-t) * p_key[4] + t * p_key[9];

		Dbg_MsgAssert(r>=0 && r<=255 && g>=0 && g<=255 && b>=0 && b<=255 && a>=0 && a<=255,
					  ("wibbled colour r=%g, g=%g, b=%g, a=%g", r,g,b,a));

		rgba  =	(uint32)(sint32)r     & 0x000000FF |
				(uint32)(sint32)g<<8  & 0x0000FF00 |
				(uint32)(sint32)b<<16 & 0x00FF0000 |
				(uint32)(sint32)a<<24;

		// get # vertices wibbled by this sequence
		num_verts = *p_meshdata++;

		// loop over wibbled vertices, overwriting colour in dma data
		for (vert=0; vert<num_verts; vert++)
		{
			p_rgba = *(uint32 **)p_meshdata;
			p_meshdata++;
			*p_rgba = rgba;
		}
	}
}


Mth::Vector CGeomNode::GetBoundingSphere()
{
	Mth::Vector sphere;
	CGeomNode *p_node = this;

	// recurse till we find a sensible sphere
	while (!(p_node->m_flags & NODEFLAG_LEAF) && p_node->m_bounding_sphere[3] >= 1.0e+10f)
	{
		p_node = p_node->u1.mp_child;
	}

	sphere = p_node->m_bounding_sphere;

	// then account for any siblings
	while (p_node->mp_sibling)
	{
		p_node = p_node->mp_sibling;

		Mth::Vector x0, x1;
		if (p_node->m_bounding_sphere[3] > sphere[3])
		{
			x0 = p_node->m_bounding_sphere; 
			x1 = sphere;
		}
		else
		{
			x0 = sphere;
			x1 = p_node->m_bounding_sphere;
		}
		float r0=x0[3], r1=x1[3];

		// (x0,r0) is the larger sphere, (x1,r1) the smaller
		// d is the vector between centres
		Mth::Vector d = x1 - x0;

		// square of distance between centres
		float D2 = DotProduct(d,d);

		// (r0-r1)^2
		float R2 = (r0-r1)*(r0-r1);

		// m = max { (r1-r0+|d|)/2, 0 }
		float m = 0.0f;
		float D = sqrtf(D2);
		if (R2-D2 < 0)
		{
			m = (r1-r0 + sqrtf(D2)) * 0.5f;
		}

		// normalise d
		if (D>0)
			d *= 1.0f/D;

		sphere    = x0 + m * d;
		sphere[3] = r0 + m;
	}

	return sphere;
}

Mth::CBBox CGeomNode::GetBoundingBox()
{
	CGeomNode *p_node = this;

	while (!(p_node->m_flags & NODEFLAG_LEAF) && p_node->m_bounding_sphere[3] >= 1.0e+10f)
	{
		p_node = p_node->u1.mp_child;
	}

	Mth::Vector min_p(p_node->m_bounding_sphere);
	Mth::Vector max_p(p_node->m_bounding_sphere);

	//Dbg_Message("Bounding sphere (%.8f, %.8f, %.8f, %.8f)", p_node->m_bounding_sphere[X], p_node->m_bounding_sphere[Y], p_node->m_bounding_sphere[Z], p_node->m_bounding_sphere[W]);
	//Dbg_Message("Bounding box (%.8f, %.8f, %.8f)", p_node->m_bounding_box[X], p_node->m_bounding_box[Y], p_node->m_bounding_box[Z]);

	min_p[X] -= p_node->m_bounding_box[0];
	min_p[Y] -= p_node->m_bounding_box[1];
	min_p[Z] -= p_node->m_bounding_box[2];
	min_p[W] = 1.0f;

	max_p[X] += p_node->m_bounding_box[0];
	max_p[Y] += p_node->m_bounding_box[1];
	max_p[Z] += p_node->m_bounding_box[2];
	min_p[W] = 1.0f;

	return Mth::CBBox(min_p, max_p);
}



// interface to uv wibble control


void CGeomNode::SetUVWibbleParams(float u_vel, float u_amp, float u_freq, float u_phase,
								  float v_vel, float v_amp, float v_freq, float v_phase)
{
	Dbg_Assert(mp_uv_wibble);

	mp_uv_wibble[0] = u_vel;
	mp_uv_wibble[1] = v_vel;
	mp_uv_wibble[2] = u_freq;
	mp_uv_wibble[3] = v_freq;
	mp_uv_wibble[4] = u_amp;
	mp_uv_wibble[5] = v_amp;
	mp_uv_wibble[6] = u_phase;
	mp_uv_wibble[7] = v_phase;
}



void CGeomNode::UseExplicitUVWibble(bool yes)
{
	SET_FLAG(NODEFLAG_EXPLICIT_UVWIBBLE);
}



void CGeomNode::SetUVWibbleOffsets(float u_offset, float v_offset)
{
	Dbg_Assert(mp_uv_wibble);

	mp_uv_wibble[8] = u_offset;
	mp_uv_wibble[9] = v_offset;
}



void CGeomNode::SetUVOffset(uint32 material_name, int pass, float u_offset, float v_offset)
{
	Dbg_MsgAssert(0, ("SetUVOffset not supported for CGeomNodes"));
}


void CGeomNode::SetUVMatrix(uint32 material_name, int pass, Mth::Matrix &mat)
{
	Dbg_MsgAssert(0, ("SetUVOffset not supported for CGeomNodes"));
}




#endif









} // namespace NxPs2




