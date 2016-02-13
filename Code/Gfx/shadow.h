/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2001 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	Module:			Shadow  (GFX)											**
**																			**
**	File name:		gfx/shadow.h											**
**																			**
**	Created: 		01/25/01	-	dc										**
**																			**
*****************************************************************************/

#ifndef __GFX_SHADOW_H
#define __GFX_SHADOW_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/support.h>
#include <core/math.h>

namespace Nx
{
	class CModel;
	class CTexture;
	class CTextured3dPoly;
}
							 
							 
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Gfx
{

enum EShadowType
{
	vUNDEFINED_SHADOW,
	vDETAILED_SHADOW,
	vSIMPLE_SHADOW
};

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



class	CShadow : public Spt::Class
{
protected:
	EShadowType	m_type;
	
public:
								CShadow();
	virtual						~CShadow();
	virtual void				UpdatePosition(Mth::Vector pos) {}
	virtual void				UpdatePosition(Mth::Vector& parentPos, Mth::Matrix& parentMatrix, Mth::Vector normal) {}
	virtual void				UpdateDirection( const Mth::Vector& dir ) {}
	virtual void				Hide() {}
	virtual void				UnHide() {}
	EShadowType					GetShadowType() {return m_type;}
};

class	CSimpleShadow : public CShadow
{
	//Nx::CTextured3dPoly *mp_poly;
	Nx::CModel	*mp_model;
	float m_scale;
	float m_offset;
	
public:
								CSimpleShadow();
								~CSimpleShadow();
	void						SetScale(float scale) {m_scale=scale;}
	void						SetOffset(float offset) {m_offset=offset;}
	void						SetModel(const char *p_model_name);
	void						UpdatePosition(Mth::Vector& parentPos, Mth::Matrix& parentMatrix, Mth::Vector normal);
	void						Hide();
	void						UnHide();
};


class	Camera;				 
class	CDetailedShadow : public CShadow
{
public:
								CDetailedShadow(Nx::CModel *p_model, Mth::Vector camera_direction = Mth::Vector(0.0f,-1.0f,0.0f), float camera_distance = 72.0f);
								~CDetailedShadow();
	void    					UpdatePosition(Mth::Vector	pos);		// the position of the target
	void    					UpdateDirection( const Mth::Vector& dir );		// the direction in which the camera points

private:
	Camera*						mp_camera;
	Nx::CTexture*				mp_texture;
	Mth::Vector					m_direction;
	float						m_distance;
};
	   

} // namespace Gfx

#endif	// __GFX_SHADOW_H
