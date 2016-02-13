///*****************************************************************************
//**																			**
//**			              Neversoft Entertainment.			                **
//**																		   	**
//**				   Copyright (C) 2000 - All Rights Reserved				   	**
//**																			**
//******************************************************************************
//**																			**
//**	Project:		THPS4													**
//**																			**
//**	Module:			NxNgc					 								**
//**																			**
//**	File name:		geometry.h												**
//**																			**
//**	Created by:		02/08/02	-	mrd										**
//**																			**
//**	Description:	classes for Ngc geometry								**
//**																			**
//*****************************************************************************/
//
//#ifndef __GFX_NGC_NX_GEOMETRY_H
//#define __GFX_NGC_NX_GEOMETRY_H
//
///*****************************************************************************
//**							  	  Includes									**
//*****************************************************************************/
//
///*****************************************************************************
//**								   Defines									**
//*****************************************************************************/
//
//
//namespace NxNgc
//{
//	// Forward declarations
//	struct	sGroup;
//
//
//#define NODEFLAG_ACTIVE			(1<<0)
//#define NODEFLAG_LEAF			(1<<1)
//#define NODEFLAG_OBJECT			(1<<2)
//#define NODEFLAG_TRANSFORMED	(1<<3)
//#define NODEFLAG_COLOURED		(1<<4)
//
//#define LOADFLAG_RENDERNOW		(1<<0)
//
//// put these here temporarily
//#define RENDERFLAG_CULL			(1<<0)
//#define RENDERFLAG_CLIP			(1<<1)
//
//
///*****************************************************************************
//**							Class Definitions								**
//*****************************************************************************/
//
//struct STransformContext
//{
//	Mth::Matrix	matrix;
//	Mth::Matrix	adjustedMatrix;
//	Mth::Vector	translation;
//};
//
//
//class CGeomNode
//{
//public:
//
//
//
//	#ifdef __PLAT_NGC__
//	void				Render(STransformContext& transformContext, uint32 renderFlags);
//	#endif
//
//	bool				IsLeaf();
//	bool				IsActive();
//	bool				IsTransformed();
//	bool				IsObject();
//
//	void				SetLeaf(bool yes);
//	void				SetActive(bool yes);
//	void				SetTransformed(bool yes);
//	void				SetObject(bool yes);
//
//	void				SetChecksum(uint32 checksum);
//	void				SetBoundingSphere(float x, float y, float z, float R);	// change args to const Mth::Vector sphere
//	void				SetDma(uint8 *pDma);
//	void				SetChild(CGeomNode *pChild);
//	void				SetSibling(CGeomNode *pSibling);
//	void				SetGroup(sGroup *pGroup);
//	void				SetMatrix(Mth::Matrix *p_mat);
//	void				SetProperties(void *pProperties);
//
//	uint32				GetChecksum();
//	Mth::Matrix&		GetMatrix();
//	void*				GetProperties();
//	CGeomNode*			GetChild();
//	CGeomNode*			GetSibling();
//	uint8*				GetDma();
//	sGroup*				GetGroup();
//
//	#ifdef __PLAT_WN32__
//	void 				Preprocess(uint8 *p_baseAddress);
//	#endif
//
//	#ifdef __PLAT_NGC__
//	static CGeomNode*	sProcessInPlace(uint8 *pPipData, uint32 loadFlags);
//	void				Cleanup();
//	
//	CGeomNode*			CreateInstance(Mth::Matrix *p_mat);
//	void				DeleteInstance();
//	CGeomNode*			CreateCopy();
//	#endif
//	
//	void				Init();
//
//						CGeomNode();
//						CGeomNode(const CGeomNode &node);
//						~CGeomNode() {}
//
//
//						static CGeomNode	sWorld;
//						static CGeomNode	sDatabase;
//
//
//
//private:
//
//	#ifdef __PLAT_NGC__
//	bool				IsOutsideViewVolume(Mth::Vector &s);
//	bool				IsInsideOuterVolume(Mth::Vector &s);
//	bool				NeedsClipping(Mth::Vector &s);
//
//	void				SendVu1Context(STransformContext& tc);
//	void				LinkDma(uint8 *p_newTail);
//
//	void 				ProcessNodeInPlace(uint8 *p_baseAddress);
//
//	void 				BecomesChildOf(CGeomNode *p_parent);
//	void				RemoveFrom(CGeomNode *p_parent);
//
//	#endif
//
//	Mth::Vector			m_bounding_sphere;
//	uint32				m_flags;
//	void *				mp_properties;
//	CGeomNode *			mp_child;
//	CGeomNode *			mp_sibling;
//	uint8 *				mp_dma;
//	sGroup *			mp_group;
//	uint32				m_checksum;
//	uint32				pad[1];
//};
//
//
//
///*****************************************************************************
//**							 Private Declarations							**
//*****************************************************************************/
//
///*****************************************************************************
//**							  Private Prototypes							**
//*****************************************************************************/
//
///*****************************************************************************
//**							  Public Declarations							**
//*****************************************************************************/
//
///*****************************************************************************
//**							   Public Prototypes							**
//*****************************************************************************/
//
///*****************************************************************************
//**								Inline Functions							**
//*****************************************************************************/
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//
//} // namespace NxNgc
//
//#endif	// __GFX_NGC_NX_GEOMETRY_H


