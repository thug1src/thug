//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxModel.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  1/8/2002
//****************************************************************************

#ifndef	__GFX_P_NX_MODEL_H__
#define	__GFX_P_NX_MODEL_H__
    
#include "gfx/nxmodel.h"


namespace NxPs2
{
	class CInstance;             
	struct sScene;
}
							   
namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/////////////////////////////////////////////////////////////////////////////////////
//
// Here's a machine specific implementation of the CModel
    
class CPs2Model : public CModel
{
                                      
public:
	CPs2Model();
	virtual				~CPs2Model();
	Mth::Matrix*		GetMatrices();

private:				// It's all private, as it is machine specific
	bool				plat_init_skeleton( int numBones );
	Mth::Vector 		plat_get_bounding_sphere();
	void				plat_set_bounding_sphere( const Mth::Vector& boundingSphere );

private:
	Mth::Matrix*		mp_matrices;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // Nx

#endif 
