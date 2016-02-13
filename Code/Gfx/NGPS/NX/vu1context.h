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
**	Module:			vu1context				 								**
**																			**
**	File name:		vu1context.h											**
**																			**
**	Created by:		00/00/00	-	mrd										**
**																			**
**	Description:	sends contextual setup data to vu1 float registers		**
**																			**
*****************************************************************************/

#ifndef __NGPS_NX_VU1CONTEXT_H
#define __NGPS_NX_VU1CONTEXT_H

#ifdef __PLAT_NGPS__

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace NxPs2
{


#define STD_CTXT_SIZE 5
#define EXT_CTXT_SIZE 16



class CVu1Context
{
public:

	CVu1Context();
	void Init();
	void InitExtended();
	CVu1Context(const CVu1Context &node);
	~CVu1Context() {}

	void			StandardSetup(Mth::Matrix &localToWorld);
	void			HierarchicalSetup(Mth::Matrix &localToParent);
	void			SetupAsSky();
	void			AddZPush(float zPush);
	void			WibbleUVs(float *pData, uint Explicit);
	void			SetShadowVecs(float y);
	void			SetColour(CGeomNode *pNode);
	void			SetAmbient(uint32 rgba);
	void			SetLights(CLightGroup *pLightGroup);
	void			SetReflectionVecs(sint16 uScale, sint16 vScale);
	void			SetExtended(bool yes);

	CVu1Context		*Localise();
	CVu1Context		*LocaliseExtend();


	Mth::Matrix 	*GetMatrix();
	Mth::Vector		*GetTranslation();
	int				GetDma();
	Mth::Vector		*GetColour();
	CLightGroup		*GetLights();
	bool			IsExtended() const;

private:
	void			update_lights();

	// non-dma data
	Mth::Matrix		m_matrix;
	Mth::Vector		m_translation;

	// dma data... don't rearrange this part!
	uint32			m_stmask;
	CLightGroup *	mp_light_group;
	uint32			m_stcycl;
	uint32			m_unpack;
	uint128			m_giftag;
	Mth::Matrix		m_localToViewport;
	Mth::Vector		m_texScale;
	Mth::Vector		m_reflVecsX;
	Mth::Vector		m_reflVecsY;
	Mth::Vector		m_reflVecsZ;
	Mth::Vector		m_lightVecsX;
	Mth::Vector		m_lightVecsY;
	Mth::Vector		m_lightVecsZ;
	Mth::Vector		m_ambientColour;
	Mth::Vector		m_lightColour0;
	Mth::Vector		m_lightColour1;
	Mth::Vector		m_lightColour2;
	Mth::Vector		m_texOffset;

	// keep the size a whole number of a quadwords
};


inline Mth::Matrix *CVu1Context::GetMatrix()
{
	return &m_matrix;
}


inline Mth::Vector *CVu1Context::GetTranslation()
{
	return &m_translation;
}

inline int CVu1Context::GetDma()
{
	return (int)&m_stmask;
}

inline CLightGroup *CVu1Context::GetLights()
{
	return mp_light_group;
}



/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

inline void CVu1Context::SetExtended(bool yes)
{
	if (yes)
	{
		m_stmask |= 1;
	}
	else
	{
		m_stmask &= ~1;
	}
}


inline bool CVu1Context::IsExtended() const
{
	return (m_stmask & 1) ? true : false;
}
										
										
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace NxPs2

#endif	// __PLAT_NGPS__

#endif	// __NGPS_NX_VU1CONTEXT_H


