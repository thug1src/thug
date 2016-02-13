/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 2002 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	File name:		shadow.cpp												**
**																			**
**	Description:	Shadow interface code									**
**																			**
*****************************************************************************/

#include <gfx/shadow.h>

#include <gel/scripting/symboltable.h>

#include <gfx/camera.h>
#include <gfx/gfxutils.h>
#include <gfx/nx.h>
#include <gfx/nxmodel.h>
#include <gfx/nxtextured3dpoly.h>

namespace Gfx
{

CShadow::CShadow()
{
	m_type=vUNDEFINED_SHADOW;
}

CShadow::~CShadow()
{
}

CDetailedShadow::CDetailedShadow(Nx::CModel *p_model, Mth::Vector camera_direction, float camera_distance)
{
	m_type=vDETAILED_SHADOW;
	mp_texture = Nx::CEngine::sCreateRenderTargetTexture( 256, 256, 16, 16 );		// maybe platform dependent?
	
	// Note, engine will figure out scene for itself
	Nx::CEngine::sProjectTextureIntoScene( mp_texture, p_model );		

	m_distance = camera_distance;
	m_direction = camera_direction;
	m_direction.Normalize();
	mp_camera = new Camera();
	mp_camera->SetPos(Mth::Vector(0,0,0));
	// construct an orientation matrix for the camera
	// so it is looking in the required direction
	Mth::Matrix orient;
	orient.Ident();		 		// initialize (since it aint)
	orient[Z] = m_direction;
	orient[Z].Normalize();
	
	// X and Y are at right angles to Z  
	orient[X][X] =  orient[Z][Z];
	orient[X][Y] = -orient[Z][X];
	orient[X][Z] = -orient[Z][Y];
	
	orient[Y][X] =  orient[Z][Y];
	orient[Y][Y] = -orient[Z][Z];
	orient[Y][Z] = -orient[Z][X];
	mp_camera->SetMatrix(orient);

	// this loops through all the existing shadows,
	// and sets their shadow flags...  the model
	// also remembers its shadow state, for any
	// subsequent geoms that are added
	p_model->EnableShadow( true ); 
}

	
CDetailedShadow::~CDetailedShadow()
{	
	Nx::CEngine::sStopProjectionTexture( mp_texture );
	
	if ( mp_texture )
	{
		delete mp_texture;
	}

	delete mp_camera;
}


// set the position for the projection camera to point at
void		CDetailedShadow::UpdatePosition(Mth::Vector pos)
{
	mp_camera->SetPos(pos - m_direction * m_distance);
	Nx::CEngine::sSetProjectionTextureCamera( mp_texture, mp_camera );
}



// set the direction in which the projection camera points
void CDetailedShadow::UpdateDirection( const Mth::Vector& dir )
{
	// Store camera position.
	Mth::Vector cam_pos = mp_camera->GetPos();

	// Set new direction and normalize.	
	m_direction = dir;
	m_direction.Normalize();
	
	// Zero out camera position.
	mp_camera->SetPos( Mth::Vector( 0, 0, 0 ));

	// Construct an orientation matrix for the camera so it is looking in the required direction.
	Mth::Matrix orient;
	orient.Identity();
	orient[Z] = m_direction;
	
	// X and Y are at right angles to Z.
	orient[X][X] =  orient[Z][Z];
	orient[X][Y] = -orient[Z][X];
	orient[X][Z] = -orient[Z][Y];
	orient[Y][X] =  orient[Z][Y];
	orient[Y][Y] = -orient[Z][Z];
	orient[Y][Z] = -orient[Z][X];
	mp_camera->SetMatrix( orient );

	// Restore camera position.
	mp_camera->SetPos( cam_pos );

	// Pass camera details on to engine.
	Nx::CEngine::sSetProjectionTextureCamera( mp_texture, mp_camera );
}



CSimpleShadow::CSimpleShadow()
{
	m_type=vSIMPLE_SHADOW;
	m_scale=1.0f;
	m_offset=1.0f;	
	mp_model=NULL;
	/*
	mp_poly=Nx::CEngine::sCreateTextured3dPoly();
	Dbg_MsgAssert(mp_poly,("Could not create textured 3d poly for shadow"));
	mp_poly->SetTexture("pedshadow");
	*/
}

CSimpleShadow::~CSimpleShadow()
{
	/*
	Dbg_MsgAssert(mp_poly,("NULL mp_poly ?"));
	Nx::CEngine::sDestroyTextured3dPoly(mp_poly);
	*/
	
	if (mp_model)
	{
		Nx::CEngine::sUninitModel(mp_model);
		mp_model=NULL;
	}	
}

void CSimpleShadow::SetModel(const char *p_model_name)
{
	if (mp_model)
	{
		Nx::CEngine::sUninitModel(mp_model);
		mp_model=NULL;
	}	

	mp_model=Nx::CEngine::sInitModel();
	
	Dbg_MsgAssert(p_model_name,("NULL p_model_name"));
	
	// TODO: Change to use a geom file instead for PS2, more efficient than mdl ...
	mp_model->AddGeom(Gfx::GetModelFileName(p_model_name, ".mdl").getString(), 0, true);
}

void CSimpleShadow::UpdatePosition(Mth::Vector& parentPos, Mth::Matrix& parentMatrix, Mth::Vector normal)
{
	/*
	Dbg_MsgAssert(mp_poly,("NULL mp_poly ?"));
	float w=Script::GetFloat("shadowwidth");
	mp_poly->SetPos(pos,w,w,normal);
	*/
		
	Mth::Matrix display_matrix;
	display_matrix[Y]=normal;
	display_matrix[Z]=parentMatrix[Z];
	display_matrix[X]=Mth::CrossProduct(display_matrix[Y],display_matrix[Z]);
	
	Mth::Vector scale(m_scale,m_scale,m_scale,1.0f);
	display_matrix.Scale(scale);
	
	// Change requested by LF/Andre:  use the passed normal
	display_matrix.SetPos(parentPos+normal*m_offset);  //WAS: parentPos+parentMatrix[Y]*m_offset
 	
	//Dbg_MsgAssert(mp_model,("NULL mp_model"));
	if (mp_model)
	{
		mp_model->Render(&display_matrix,true,NULL);
	}	
}

void CSimpleShadow::Hide()
{
	//Dbg_MsgAssert(mp_model,("NULL mp_model"));
	if (mp_model)
	{
		mp_model->SetActive(false);
	}	
}

void CSimpleShadow::UnHide()
{
	//Dbg_MsgAssert(mp_model,("NULL mp_model"));
	if (mp_model)
	{
		mp_model->SetActive(true);
	}	
}

}

