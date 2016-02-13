//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxModel.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  1/8/2002
//****************************************************************************

#ifndef	__GFX_P_NX_MODEL_H__
#define	__GFX_P_NX_MODEL_H__
    
#include "gfx/nxmodel.h"
#include "gfx/Ngc/nx/instance.h"
                   
namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/////////////////////////////////////////////////////////////////////////////////////
//
// Here's a machine specific implementation of the CModel
    
class CNgcModel : public CModel
{
public:
						CNgcModel();
	virtual 			~CNgcModel();

	NxNgc::CInstance    *GetInstance() { return mp_instance; }
	void				SetInstance( NxNgc::CInstance * p ) { mp_instance = p; }
private:				// It's all private, as it is machine specific
//	bool				plat_load_mesh( CMesh* pMesh );
//	bool				plat_unload_mesh();
//	bool				plat_set_render_mode(ERenderMode mode);
//	bool				plat_set_color(uint8 r, uint8 g, uint8 b, uint8 a);
//	bool				plat_set_visibility(uint32 mask);
//	bool				plat_set_active( bool active );
//    bool				plat_set_scale( float scaleFactor );
//    bool				plat_replace_texture( char* p_srcFileName, char* p_dstFileName );
//	bool				plat_render(Mth::Matrix* pRootMatrix, Mth::Matrix* ppBoneMatrices, int numBones);

	bool				plat_init_skeleton( int numBones );

	bool				plat_prepare_materials( void );
	bool				plat_refresh_materials( void );

	Mth::Vector 		plat_get_bounding_sphere();
	void				plat_set_bounding_sphere( const Mth::Vector& boundingSphere );

	NxNgc::CInstance	*mp_instance;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // Nx

#endif 

