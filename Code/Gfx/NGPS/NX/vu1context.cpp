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
**	File name:		vu1context.cpp											**
**																			**
**	Created by:		00/00/00	-	mrd										**
**																			**
**	Description:	sends contextual setup data to vu1 float registers		**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/math.h>
#include <sys/timer.h>
#include "geomnode.h"
#include "vu1context.h"
#include "vu1code.h"
#include "render.h"
#include "dma.h"
#include "vif.h"
#include "gif.h"
#include "gs.h"
#include "light.h"

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace NxPs2
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

CVu1Context::CVu1Context()
{
	Dbg_Assert("Don't use this constructor!");
}


void CVu1Context::Init()
{
	uint8 *dma_ploc = dma::pLoc;

	//dma::pLoc = (uint8 *)&m_stmask;
	dma::pLoc = (uint8 *)&m_stmask;
	vu1::Loc = 0;

	// set vifcodes
	vif::STMASK(0);				// mp_light_group = NULL; in a very cheeky way
	vif::STCYCL(1,1);
	vif::UNPACK(0,V4_32,STD_CTXT_SIZE+1,REL,UNSIGNED,0);

	// set m_giftag
	gif::Tag1(gs::NOP, 1, PACKED, 0, 0, 0, STD_CTXT_SIZE, VU1_ADDR(L_VF12));

	//SetExtended(false);

	dma::pLoc = dma_ploc;
}


void CVu1Context::InitExtended()
{
	uint8 *dma_ploc = dma::pLoc;

	//dma::pLoc = (uint8 *)&m_stmask;
	dma::pLoc = (uint8 *)&m_stmask;
	vu1::Loc = 0;

	// set vifcodes
	vif::STMASK(0);				// mp_light_group = NULL; in a very cheeky way
	vif::STCYCL(1,1);
	vif::UNPACK(0,V4_32,EXT_CTXT_SIZE+1,REL,UNSIGNED,0);

	// set m_giftag
	gif::Tag1(gs::NOP, 1, PACKED, 0, 0, 0, EXT_CTXT_SIZE, VU1_ADDR(L_VF12));

	SetExtended(true);

	update_lights();

	dma::pLoc = dma_ploc;
}


CVu1Context::CVu1Context(const CVu1Context &ctxt)
{
	// Mick: Since this does not contain any pointers, it's safe to just copy it
	// this cuts down on unwarrented initialization of the various vectors and stuff
	// and should be fast as we are quad word aligned.
	// Garrett: Even though we have a pointer to a light group, it is still OK to
	// use memcpy() since this class doesn't manage the instance it points to.

	if (ctxt.IsExtended())
	{
		memcpy(this, &ctxt, (EXT_CTXT_SIZE+7)*16);
	}
	else
	{
		// reduced version
		memcpy(this, &ctxt, (STD_CTXT_SIZE+7)*16);
	}

}


void CVu1Context::StandardSetup(Mth::Matrix &localToWorld)
{
	// get the local to camera matrix
	m_matrix = localToWorld * render::WorldToCamera;

	// get the massaged matrix values
	Mth::Matrix inv = m_matrix;
	inv.InvertUniform();
	m_translation = -inv[3] * SUB_INCH_PRECISION;
	m_translation[0] = float(int(m_translation[0]));
	m_translation[1] = float(int(m_translation[1]));
	m_translation[2] = float(int(m_translation[2]));

	Mth::Matrix adjusted_matrix = m_matrix;
	//m_adjustedMatrix = m_matrix;
	adjusted_matrix[3] = -m_translation * m_matrix * RECIPROCAL_SUB_INCH_PRECISION;

	// generate the vu1 context data
	//m_localToFrustum  = m_adjustedMatrix * render::CameraToFrustum;
	Mth::Matrix temp;
	sceVu0MulMatrix((sceVu0FVECTOR*)&temp,
					(sceVu0FVECTOR*)&render::CameraToFrustum,
					(sceVu0FVECTOR*)&adjusted_matrix);   	
	//m_localToFrustum = temp;
	
//	m_localToViewport = m_localToFrustum * render::FrustumToViewport;
	sceVu0MulMatrix((sceVu0FVECTOR*)&m_localToViewport,
					(sceVu0FVECTOR*)&render::FrustumToViewport,
					(sceVu0FVECTOR*)&temp);

	// Update light directions
	if (IsExtended())
	{
		update_lights();
	}
}



CVu1Context *CVu1Context::Localise()
{
	dma::SetList(NULL);
	CVu1Context *p_new_context = (CVu1Context*)dma::pLoc;

	if (IsExtended())
	{
		memcpy(dma::pLoc, this, sizeof(CVu1Context));
		dma::pLoc += sizeof(CVu1Context);
	}
	else
	{
		// reduced version
		memcpy(dma::pLoc, this, (STD_CTXT_SIZE+7)*16);
		dma::pLoc += (STD_CTXT_SIZE+7)*16;
	}

	return p_new_context;
}




CVu1Context *CVu1Context::LocaliseExtend()
{
	dma::SetList(NULL);
	CVu1Context *p_new_context = (CVu1Context*)dma::pLoc;

	if (IsExtended())
	{
		memcpy(dma::pLoc, this, sizeof(CVu1Context));
	}
	else
	{
		memcpy(dma::pLoc, this, (STD_CTXT_SIZE+7)*16);
		p_new_context->InitExtended();
	}
	dma::pLoc += sizeof(CVu1Context);

	return p_new_context;
}




void CVu1Context::HierarchicalSetup(Mth::Matrix &localToParent)
{
	// get the local to camera matrix
	m_matrix = localToParent * m_matrix;

	// get the massaged matrix values
	Mth::Matrix temp = m_matrix;
	m_translation = -(temp.InvertUniform())[3] * SUB_INCH_PRECISION;
	m_translation[0] = float(int(m_translation[0]));
	m_translation[1] = float(int(m_translation[1]));
	m_translation[2] = float(int(m_translation[2]));

	Mth::Matrix adjusted_matrix = m_matrix;
	//m_adjustedMatrix = m_matrix;
	adjusted_matrix[3] = -m_translation * m_matrix * RECIPROCAL_SUB_INCH_PRECISION;

	// generate the vu1 context data
//	m_localToFrustum  = m_adjustedMatrix * render::CameraToFrustum;
	sceVu0MulMatrix((sceVu0FVECTOR*)&temp,
					(sceVu0FVECTOR*)&render::CameraToFrustum,
					(sceVu0FVECTOR*)&adjusted_matrix);   	
	//m_localToFrustum = temp;
	
//	m_localToViewport = m_localToFrustum * render::FrustumToViewport;
	sceVu0MulMatrix((sceVu0FVECTOR*)&m_localToViewport,
					(sceVu0FVECTOR*)&render::FrustumToViewport,
					(sceVu0FVECTOR*)&temp);

	// Update light directions
	if (IsExtended())
	{
		update_lights();
	}
}

// Mick: This is taking 1.5% of our precious CPU time
void CVu1Context::update_lights()
{
	Mth::Matrix temp;
	temp[W].Set(0.0f,0.0f,0.0f,1.0f);

	// light vectors
	if (mp_light_group)
	{
		temp[0] = mp_light_group->GetDirection(0);
		temp[1] = mp_light_group->GetDirection(1);
		temp[2] = mp_light_group->GetDirection(2);
	
		temp *= render::WorldToCamera;
		temp.Transpose();
		
		sceVu0MulMatrix((sceVu0FVECTOR*)&m_lightVecsX,
						(sceVu0FVECTOR*)&temp,
						(sceVu0FVECTOR*)&m_matrix);   	
	
		m_ambientColour = mp_light_group->GetAmbientColor() * 1.0f/128.0f;
		m_lightColour0  = mp_light_group->GetDiffuseColor(0) * 1.0f/128.0f;
		m_lightColour1  = mp_light_group->GetDiffuseColor(1) * 1.0f/128.0f;
		m_lightColour2  = mp_light_group->GetDiffuseColor(2) * 1.0f/128.0f;
	}
	else
	{
		temp[0] = CLightGroup::sGetDefaultDirection(0);
		temp[1] = CLightGroup::sGetDefaultDirection(1);
		temp[2] = CLightGroup::sGetDefaultDirection(2);

		temp *= render::WorldToCamera;
		temp.Transpose();
		
		sceVu0MulMatrix((sceVu0FVECTOR*)&m_lightVecsX,
						(sceVu0FVECTOR*)&temp,
						(sceVu0FVECTOR*)&m_matrix);   	
	
		m_ambientColour = CLightGroup::sGetDefaultAmbientColor() * 1.0f/128.0f;
		m_lightColour0  = CLightGroup::sGetDefaultDiffuseColor(0) * 1.0f/128.0f;
		m_lightColour1  = CLightGroup::sGetDefaultDiffuseColor(1) * 1.0f/128.0f;
		m_lightColour2  = CLightGroup::sGetDefaultDiffuseColor(2) * 1.0f/128.0f;
	}

	
}


void CVu1Context::SetupAsSky()
{
	// get the local to camera matrix
	m_matrix = render::WorldToCamera;

	// get the massaged matrix values
	m_matrix[3].Set(0,0,0,1);
	m_translation.Set(0,0,0,1);

	Mth::Matrix adjusted_matrix = m_matrix;
	//m_adjustedMatrix = m_matrix;
	adjusted_matrix.ScaleLocal(Mth::Vector(RECIPROCAL_SUB_INCH_PRECISION, RECIPROCAL_SUB_INCH_PRECISION, RECIPROCAL_SUB_INCH_PRECISION, 1.0f));

	// generate the vu1 context data
	//m_viewportScale   = render::SkyViewportScale;
	//m_viewportOffset  = render::SkyViewportOffset;
	//m_localToFrustum  = adjusted_matrix * render::CameraToFrustum;
	m_localToViewport = adjusted_matrix * render::CameraToFrustum * render::SkyFrustumToViewport;
}


void CVu1Context::AddZPush(float zPush)
{
	m_localToViewport[0][2] += m_localToViewport[0][3] * zPush * 16.0f * render::InverseViewportScale[3];
	m_localToViewport[1][2] += m_localToViewport[1][3] * zPush * 16.0f * render::InverseViewportScale[3];
	m_localToViewport[2][2] += m_localToViewport[2][3] * zPush * 16.0f * render::InverseViewportScale[3];
	m_localToViewport[3][2] += m_localToViewport[3][3] * zPush * 16.0f * render::InverseViewportScale[3];
}


void CVu1Context::WibbleUVs(float *pData, uint Explicit)
{
	if (Explicit)
	{
		m_texOffset[0] = pData[8];
		m_texOffset[1] = pData[9];
	}
	else
	{
		// Get number of vblanks, but reduce it to 24 bits so we don't lose accuracy in the float conversion.
		// This will introduce a glitch every 6 days, but that's preferrable to the loss of accuracy.
		int vblanks = Tmr::GetVblanks() & 0x00FFFFFF;

		// convert to seconds as float
		float t = (float)vblanks * (1.0f/60.0f);
		float angle;

		// u offset
		angle = pData[2] * t + pData[6];
		angle -= (2.0f*Mth::PI) * (float)(int)(angle * (0.5f/Mth::PI));		// reduce angle mod 2pi
		m_texOffset[0] = pData[0] * t + pData[4] * sinf(angle);
		m_texOffset[0] += 8.0f;
		m_texOffset[0] -= (float)(((int)m_texOffset[0] >> 4) << 4);			// reduce offset mod 16
		if (m_texOffset[0] < 0.0f)											// and put it in the range -8 to +8
			m_texOffset[0] += 8.0f;
		else
			m_texOffset[0] -= 8.0f;

		// v offset
		angle = pData[3] * t + pData[7];
		angle -= (2.0f*Mth::PI) * (float)(int)(angle * (0.5f/Mth::PI));		// reduce angle mod 2pi
		m_texOffset[1] = pData[1] * t + pData[5] * sinf(angle);
		m_texOffset[1] += 8.0f;
		m_texOffset[1] -= (float)(((int)m_texOffset[1] >> 4) << 4);			// reduce offset mod 16
		if (m_texOffset[1] < 0.0f)											// and put it in the range -8 to +8
			m_texOffset[1] += 8.0f;
		else
			m_texOffset[1] -= 8.0f;

	}
}


void CVu1Context::SetShadowVecs(float y)
{
	m_texScale  = Mth::Vector(0.0078125f, 0.0078125f, 0.0f, 0.0f);
	m_texOffset = Mth::Vector(0.5f-0.0078125f*(render::ShadowCameraPosition[X]+RECIPROCAL_SUB_INCH_PRECISION*m_translation[X]),
							  0.5f-0.0078125f*(render::ShadowCameraPosition[Z]+RECIPROCAL_SUB_INCH_PRECISION*m_translation[Z]),
							  -((y+36.0f)+RECIPROCAL_SUB_INCH_PRECISION*m_translation[Y]),
							  0.0f);
}


void CVu1Context::SetColour(CGeomNode *pNode)
{
	uint32 rgba = pNode->GetColor();
   	m_texScale[X] = (float)(rgba     & 0xFF) * 0.0078125f;
	m_texScale[Y] = (float)(rgba>> 8 & 0xFF) * 0.0078125f;
	m_texScale[Z] = (float)(rgba>>16 & 0xFF) * 0.0078125f;
	m_texScale[W] = (float)(rgba>>24       ) * 0.0078125f;
}


Mth::Vector *CVu1Context::GetColour()
{
	return &m_texScale;
}


void CVu1Context::SetLights(CLightGroup *pLightGroup)
{
	mp_light_group = pLightGroup;
	update_lights();
}


void CVu1Context::SetReflectionVecs(sint16 uScale, sint16 vScale)
{
	Mth::Matrix temp;
	float uf,vf;
	uf = float(uScale)*(0.5f/256.0f);
	vf = float(vScale)*(0.5f/256.0f);

	temp[0] = Mth::Vector(uf, 0,  0,  0);
	temp[1] = Mth::Vector(0,  vf, 0,  0);
	temp[2] = Mth::Vector(0,  0,  1.4,0);
	temp.Transpose();

	sceVu0MulMatrix((sceVu0FVECTOR*)&m_reflVecsX,
					(sceVu0FVECTOR*)&temp,
					(sceVu0FVECTOR*)&m_matrix);   	
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace NxPs2




