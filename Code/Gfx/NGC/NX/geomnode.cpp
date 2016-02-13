///*****************************************************************************
//**																			**
//**			              Neversoft Entertainment.			                **
//**																		   	**
//**				   Copyright (C) 2000 - All Rights Reserved				   	**
//**																			**
//******************************************************************************
//**																			**
//**	Project:		<project name>											**
//**																			**
//**	Module:			<module name>			 								**
//**																			**
//**	File name:		<filename>.cpp											**
//**																			**
//**	Created by:		00/00/00	-	initials								**
//**																			**
//**	Description:	<description>		 									**
//**																			**
//*****************************************************************************/
//
///*****************************************************************************
//**							  	  Includes									**
//*****************************************************************************/
//
//#include <core/defines.h>
//#include <core/math.h>
//#include "render.h"
//#include "geomnode.h"
//#include <gfx\ngc\nx\nx_init.h>
//
///*****************************************************************************
//**								DBG Information								**
//*****************************************************************************/
//
//namespace NxNgc
//{
//
//
//
///*****************************************************************************
//**								  Externals									**
//*****************************************************************************/
//
///*****************************************************************************
//**								   Defines									**
//*****************************************************************************/
//
///*****************************************************************************
//**								Private Types								**
//*****************************************************************************/
//
///*****************************************************************************
//**								 Private Data								**
//*****************************************************************************/
//
///*****************************************************************************
//**								 Public Data								**
//*****************************************************************************/
//
//CGeomNode CGeomNode::sWorld;
//CGeomNode CGeomNode::sDatabase;
//
///*****************************************************************************
//**							  Private Prototypes							**
//*****************************************************************************/
//
///*****************************************************************************
//**							  Private Functions								**
//*****************************************************************************/
//
///*****************************************************************************
//**							  Public Functions								**
//*****************************************************************************/
//
//
//CGeomNode::CGeomNode()
//{
//	m_flags = 0;
//
//	m_bounding_sphere[0] = 0.0f;
//	m_bounding_sphere[1] = 0.0f;
//	m_bounding_sphere[2] = 0.0f;
//	m_bounding_sphere[3] = (float)1.0e+30;
//
//	mp_properties	= NULL;
//	mp_child		= NULL;
//	mp_sibling		= NULL;
//	mp_dma			= NULL;
//	mp_group		= NULL;
//}
//
//
//void CGeomNode::Init()
//{
//	m_flags = 0;
//
//	m_bounding_sphere[0] = 0.0f;
//	m_bounding_sphere[1] = 0.0f;
//	m_bounding_sphere[2] = 0.0f;
//	m_bounding_sphere[3] = (float)1.0e+30;
//
//	mp_properties	= NULL;
//	mp_child		= NULL;
//	mp_sibling		= NULL;
//	mp_dma			= NULL;
//	mp_group		= NULL;
//}
//
//
//CGeomNode::CGeomNode(const CGeomNode &node)
//{
//	m_flags				= node.m_flags;
//	m_bounding_sphere	= node.m_bounding_sphere;
//	mp_properties		= node.mp_properties;
//	mp_child			= node.mp_child;
//	mp_sibling			= node.mp_sibling;
//	mp_dma				= node.mp_dma;
//	mp_group			= node.mp_group;
//}
//
//
//
//
//#define GET_FLAG(FLAG) (m_flags & (FLAG) ? true : false)
//#define SET_FLAG(FLAG) m_flags = yes ? m_flags|FLAG : m_flags&~FLAG
//
//bool CGeomNode::IsLeaf()
//{
//	return GET_FLAG(NODEFLAG_LEAF);
//}
//
//
//bool CGeomNode::IsObject()
//{
//	return GET_FLAG(NODEFLAG_OBJECT);
//}
//
//
//bool CGeomNode::IsActive()
//{
//	return GET_FLAG(NODEFLAG_ACTIVE);
//}
//
//
//bool CGeomNode::IsTransformed()
//{
//	return GET_FLAG(NODEFLAG_TRANSFORMED);
//}
//
//
//void CGeomNode::SetLeaf(bool yes)
//{
//	SET_FLAG(NODEFLAG_LEAF);
//}
//
//
//void CGeomNode::SetObject(bool yes)
//{
//	SET_FLAG(NODEFLAG_OBJECT);
//}
//
//
//void CGeomNode::SetActive(bool yes)
//{
//	SET_FLAG(NODEFLAG_ACTIVE);
//}
//
//
//void CGeomNode::SetTransformed(bool yes)
//{
//	SET_FLAG(NODEFLAG_TRANSFORMED);
//}
//
//
//uint32 CGeomNode::GetChecksum()
//{
//	return m_checksum;
//}
//
//
//void CGeomNode::SetChecksum(uint32 checksum)
//{
//	m_checksum = checksum;
//}
//
//
//void CGeomNode::SetBoundingSphere(float x, float y, float z, float R)
//{
//	m_bounding_sphere[0] = x;
//	m_bounding_sphere[1] = y;
//	m_bounding_sphere[2] = z;
//	m_bounding_sphere[3] = R;
//}
//
//
//uint8 *CGeomNode::GetDma()
//{
//	return mp_dma;
//}
//
//
//void CGeomNode::SetDma(uint8 *pDma)
//{
//	mp_dma = pDma;
//}
//
//
//CGeomNode *CGeomNode::GetChild()
//{
//	return mp_child;
//}
//
//
//void CGeomNode::SetChild(CGeomNode *pChild)
//{
//	mp_child = pChild;
//}
//
//
//CGeomNode *CGeomNode::GetSibling()
//{
//	return mp_sibling;
//}
//
//
//void CGeomNode::SetSibling(CGeomNode *pSibling)
//{
//	mp_sibling = pSibling;
//}
//
//
//sGroup *CGeomNode::GetGroup()
//{
//	return mp_group;
//}
//
//
//void CGeomNode::SetGroup(sGroup *pGroup)
//{
//	mp_group = pGroup;
//}
//
//
//Mth::Matrix& CGeomNode::GetMatrix()
//{
//	Dbg_MsgAssert(IsTransformed(), ("trying to access matrix of a non-transformed CGeomNode"));
//	Dbg_MsgAssert(mp_properties, ("trying to access matrix of a CGeomNode which has no properties"));
//	return *((Mth::Matrix *)mp_properties);
//}
//
//
//void CGeomNode::SetMatrix(Mth::Matrix *p_mat)
//{
//	mp_properties = (void *)p_mat;
//	SetTransformed(p_mat ? true : false);
//}
//
//
//void *CGeomNode::GetProperties()
//{
//	return mp_properties;
//}
//
//
//void CGeomNode::SetProperties(void *pProperties)
//{
//	mp_properties = pProperties;
//}
//
//
//#ifdef __PLAT_WN32__
//
//void CGeomNode::Preprocess(uint8 *p_baseAddress)
//{
//	// recursively process any children
//	CGeomNode *p_child, *p_sibling;
//	for (p_child=mp_child; p_child; p_child=p_sibling)
//	{
//		p_sibling = p_child->mp_sibling;
//		p_child->Preprocess(p_baseAddress);
//	}
//
//	// convert properties pointer to offset
//	if (mp_properties)
//		mp_properties = (void *) ( (uint8 *)mp_properties - p_baseAddress );
//	else
//		mp_properties = (void *)(-1);
//
//	// convert dma pointer to offset
//	if (mp_dma)
//		mp_dma = (uint8 *)( mp_dma - p_baseAddress );
//	else
//		mp_dma = (uint8 *)(-1);
//
//	// leave group ID alone
//
//	// convert child pointer to offset
//	if (mp_child)
//	{
//		mp_child = (CGeomNode *) ( (uint8 *)mp_child - p_baseAddress );
//	}
//	else
//	{
//		mp_child = (CGeomNode *)(-1);
//	}
//
//	// convert sibling pointer to offset
//	if (mp_sibling)
//	{
//		mp_sibling = (CGeomNode *) ( (uint8 *)mp_sibling - p_baseAddress );
//	}
//	else
//	{
//		mp_sibling = (CGeomNode *)(-1);
//	}
//}
//
//#endif
//
//
//
//CGeomNode* CGeomNode::CreateInstance(Mth::Matrix *p_mat)
//{
//	// copy node
//	CGeomNode *p_node = new CGeomNode(*this);
//
//	// set matrix
//	p_node->SetMatrix(p_mat);
//
////	// add to world
////	p_node->BecomesChildOf(&sWorld);
////
////	// activate!
////	p_node->SetActive(true);
//
//	// return new node
//	return p_node;
//}
//
//
//void CGeomNode::DeleteInstance()
//{
////	RemoveFrom(&sWorld);
//	delete this;
//}
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//
//} // namespace NxNgc





