//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       ModelBuilder.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/16/2001
//****************************************************************************

#ifndef __GFX_MODELBUILDER_H
#define __GFX_MODELBUILDER_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <gel/object.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

namespace Nx
{
    class CModel;
};
                
namespace Gfx
{
	class CModelAppearance;
	class CSkeleton;
	
	const uint32 vCHECKSUM_CREATE_MODEL_FROM_APPEARANCE = 0x91c1ec93;	// create_model_from_appearance
	const uint32 vCHECKSUM_COLOR_MODEL_FROM_APPEARANCE = 0xd2c041c2;	// color_model_from_appearance
	const uint32 vCHECKSUM_SCALE_MODEL_FROM_APPEARANCE = 0xaee1af7d;	// scale_model_from_appearance
	const uint32 vCHECKSUM_HIDE_MODEL_FROM_APPEARANCE = 0xb1a55798;		// hide_model_from_appearance


/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

class CModelBuilder	: public Obj::CObject
{
	public:
		CModelBuilder( bool useAssetManager, uint32 texDictOffset );

	public:
		virtual void	BuildModel( CModelAppearance* pAppearance, Nx::CModel* pModel, Gfx::CSkeleton* pSkeleton, uint32 scriptName = vCHECKSUM_CREATE_MODEL_FROM_APPEARANCE );
		virtual bool	CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript );
	
	protected:
		virtual bool	model_add_geom( uint32 partChecksum );
		virtual bool	model_clear_geom( uint32 partChecksum );
		virtual bool	model_hide_geom( uint32 partChecksum, bool hidden );
		virtual bool	geom_modulate_color( Script::CStruct* pParams );
		virtual bool	geom_set_uv_offset( uint32 partChecksum, uint32 matChecksum, int pass );
		virtual bool	geom_allocate_uv_matrix_params( uint32 matChecksum, int pass );
		virtual bool	geom_replace_texture( uint32 partChecksum );
		virtual bool	model_clear_all_geoms();
		virtual bool	model_remove_polys();
		virtual bool	model_reset_scale();
		virtual bool	model_apply_body_shape();
		virtual bool	model_apply_bone_scale( uint32 partChecksum );
		virtual bool	model_apply_object_scale();
		virtual bool	model_finalize();
		virtual bool	model_run_script( uint32 partChecksum, uint32 targetChecksum );
		
	protected:
		// temporary pointer to the model, so that CallMemberFunctions
		// can operate on the Nx::CModel.  This only exists for the
		// duration of the BuildModel() function call.
		static Nx::CModel*			mp_model;
		static Gfx::CSkeleton*		mp_skeleton;
		static CModelAppearance*	mp_appearance;
		bool						m_useAssetManager;
		uint32						m_texDictOffset;
};

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

} // namespace Obj

#endif	// __GFX_MODELBUILDER_H
