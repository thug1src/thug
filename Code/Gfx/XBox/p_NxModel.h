//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxModel.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  1/8/2002
//****************************************************************************

#ifndef	__GFX_P_NX_MODEL_H__
#define	__GFX_P_NX_MODEL_H__
    
#include "gfx/nxmodel.h"
#include "gfx/xbox/nx/instance.h"
                   
namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/////////////////////////////////////////////////////////////////////////////////////
//
// Here's a machine specific implementation of the CModel
    
class CXboxModel : public CModel
{
public:
						CXboxModel();
	virtual 			~CXboxModel();
	NxXbox::CInstance	*GetInstance( void )								{ return mp_instance; }
	void				SetInstance( NxXbox::CInstance *p_instance )		{ mp_instance = p_instance; }

private:				// It's all private, as it is machine specific
	virtual Mth::Vector	plat_get_bounding_sphere( void );
	virtual void		plat_set_bounding_sphere( const Mth::Vector& boundingSphere );

	bool				plat_init_skeleton( int num_bones );

	NxXbox::CInstance	*mp_instance;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // Nx

#endif 
