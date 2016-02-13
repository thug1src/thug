//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       NxGeom.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/27/2002
//****************************************************************************

#include <core/math.h>

#include <gfx/nxgeom.h>
#include <gfx/image/imagebasic.h>

#include <gel/collision/colltridata.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>

namespace Nx
{

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions are provided here:
// so engine implementors can leave certain functionality until later
						
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom* 			CGeom::plat_clone(bool instance, CScene* pDestScene)
{
	printf ("STUB: PlatClone\n");
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom* 			CGeom::plat_clone(bool instance, CModel* pDestModel)
{
	printf ("STUB: PlatClone\n");
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CGeom::plat_load_geom_data(CMesh* pMesh, CModel* pModel, bool color_per_material)
{
	printf ("STUB: PlatLoadGeomData\n");
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CGeom::plat_finalize()
{
	// do nothing...  this is only needed for the PS2 right now
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32			CGeom::plat_get_checksum()
{
	printf ("STUB: PlatGetChecksum\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CGeom::plat_set_color(Image::RGBA rgba)
{
	printf ("STUB: PlatSetColor\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA		CGeom::plat_get_color() const
{
	printf ("STUB: PlatGetColor\n");
	return Image::RGBA(0, 0, 0, 0);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CGeom::plat_clear_color()
{
	printf ("STUB: PlatClearColor\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CGeom::plat_set_material_color(uint32 mat_checksum, int pass, Image::RGBA rgba)
{
	printf ("STUB: PlatSetMaterialColor\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA		CGeom::plat_get_material_color(uint32 mat_checksum, int pass)
{
	printf ("STUB: PlatGetMaterialColor\n");
	return Image::RGBA(0, 0, 0, 0);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CGeom::plat_set_visibility(uint32 mask)
{
	printf ("STUB: PlatSetVisibility\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32			CGeom::plat_get_visibility() const
{
	printf ("STUB: PlatGetVisibility\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CGeom::plat_set_active(bool active)
{
	printf ("STUB: PlatSetActive\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CGeom::plat_is_active() const
{
	printf ("STUB: PlatIsActive\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::CBBox &	CGeom::plat_get_bounding_box() const
{
	static Mth::CBBox stub_bbox;
	printf ("STUB: PlatGetBoundingBox\n");
	return stub_bbox;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Vector CGeom::plat_get_bounding_sphere() const
{
	printf ("STUB: PlatGetBoundingSphere\n");
	return Mth::Vector( 0.0f, 0.0f, 0.0f, 10000.0f );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CGeom::plat_set_world_position(const Mth::Vector& pos)
{
	printf ("STUB: PlatSetWorldPosition\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &	CGeom::plat_get_world_position() const
{
	static Mth::Vector stub_vec;
	printf ("STUB: PlatGetWorldPosition\n");
	return stub_vec;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CGeom::plat_set_orientation(const Mth::Matrix& orient)
{
	printf ("STUB: PlatSetOrientation\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Matrix & CGeom::plat_get_orientation() const
{
	static Mth::Matrix stub_mat;
	printf ("STUB: PlatGetOrientation\n");
	return stub_mat;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 			CGeom::plat_rotate_y(Mth::ERot90 rot)
{
	printf ("STUB: PlatRotateY\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CGeom::plat_set_transform(const Mth::Matrix& transform)
{
	printf ("STUB: PlatSetTransform\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Matrix &	CGeom::plat_get_transform() const
{
	static Mth::Matrix stub_mat;
	printf ("STUB: PlatGetTransform\n");
	return stub_mat;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CGeom::plat_set_scale(const Mth::Vector& scale)
{
	printf ("STUB: PlatSetScale\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector		CGeom::plat_get_scale() const
{
	printf ("STUB: PlatGetScale\n");
	return Mth::Vector(0, 0, 0, 0);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CGeom::plat_set_model_lights(CModelLights *p_model_lights)
{
	printf ("STUB: PlatSetModelLights\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CGeom::plat_render(Mth::Matrix* pRootMatrix, Mth::Matrix* ppBoneMatrices, int numBones)
{
	printf ("STUB: PlatRender\n");
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CGeom::plat_set_bounding_sphere( const Mth::Vector& boundingSphere )
{
	printf ("STUB: PlatSetBoundingSphere\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CGeom::plat_hide_polys( uint32 mask )
{
	printf ("STUB: PlatHidePolys\n");
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CGeom::plat_enable_shadow( bool enabled )
{
//	printf ("STUB: PlatEnableShadow\n");
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// - returns number of renderable verts in a C...Geom
// this should be the same number of verts as is returned/expected by the following functions 
int 		CGeom::plat_get_num_render_verts()								
{
	printf ("STUB: CGeom::plat_get_num_render_verts\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// - gets a single array of the verts positions as regular floating point Mth::Vectors
// p_verts points to memory supplied by the caller
void 		CGeom::plat_get_render_verts(Mth::Vector *p_verts)			
{
	printf ("STUB: CGeom::plat_get_render_verts\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int 		CGeom::plat_get_num_render_polys()								
{
	printf ("STUB: CGeom::plat_get_num_render_polys\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int 		CGeom::plat_get_num_render_base_polys()								
{
	printf ("STUB: CGeom::plat_get_num_render_base_polys\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// - gets a single array of the verts colors as RGBA
// p_colors points to memory supplied by the caller
void 		CGeom::plat_get_render_colors(Image::RGBA *p_colors)		// - gets an array of vertex colors
{
	printf ("STUB: CGeom::plat_get_render_colors\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// - sets the verts after modification
void 		CGeom::plat_set_render_verts(Mth::Vector *p_verts)			  
{
	printf ("STUB: CGeom::plat_set_render_verts\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// - sets the colors after modification
void 		CGeom::plat_set_render_colors(Image::RGBA *p_colors)				
{
	printf ("STUB: CGeom::plat_set_render_colors\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void		CGeom::plat_set_bone_matrix_data( Mth::Matrix* pBoneMatrices, int numBones )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CGeom::plat_set_uv_wibble_params(float u_vel, float u_amp, float u_freq, float u_phase,
											 float v_vel, float v_amp, float v_freq, float v_phase)
{
	printf ("STUB: CGeom::plat_set_uv_wibble_params\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CGeom::plat_use_explicit_uv_wibble(bool yes)
{
	printf ("STUB: CGeom::plat_use_explicit_uv_wibble\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CGeom::plat_set_uv_wibble_offsets(float u_offset, float v_offset)
{
	printf ("STUB: CGeom::plat_set_uv_wibble_offsets\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CGeom::plat_set_uv_wibble_offsets(uint32 mat_checksum, int pass, float u_offset, float v_offset)
{
	printf ("STUB: CGeom::plat_set_uv_wibble_offsets\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CGeom::plat_set_uv_matrix(uint32 mat_checksum, int pass, const Mth::Matrix &mat)
{
	printf ("STUB: CGeom::plat_set_uv_matrix\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CGeom::plat_allocate_uv_matrix_params(uint32 mat_checksum, int pass)
{
	// only needed by the xbox right now...

	return true;
}

/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

// These functions are the platform independent part of the interface to 
// the platform specific code
// parameter checking can go here....
// although we might just want to have these functions inline, or not have them at all?

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom::CGeom()
{
	m_cloned = vORIGINAL;
	mp_coll_tri_data = NULL;
	mp_orig_render_colors = NULL;
	m_multipleColorsEnabled = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom::~CGeom()
{
	if (m_cloned != vORIGINAL)			// delete cloned data
	{
		if (mp_coll_tri_data)
		{
			// Right now, cloning of collision isn't handled at the CGeom level
			//delete mp_coll_tri_data;
		}
	}
	
	if (mp_orig_render_colors)
	{
		delete mp_orig_render_colors;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom*  CGeom::Clone(bool instance, CScene* pDestScene)
{
#ifdef __PLAT_NGPS__
	// Garrett: Want to be able to disable copy just in case it opens up crashes on the Park Editor
	if (Script::GetInt( "disable_clone_copy", false ))
	{
		instance = true;
	}
#endif

	// Create new instance and clone stuff below p-line
	CGeom* p_new_geom = plat_clone(instance, pDestScene);

	if (p_new_geom)
	{
		p_new_geom->m_cloned = (instance) ? vINSTANCE : vCOPY;

		if (mp_coll_tri_data)
		{
#if 1
			// Garrett: Just copying the pointer now, since the CGeom version is just used for reference
			p_new_geom->mp_coll_tri_data = mp_coll_tri_data;
#else
			// Make NULL for now until we can figure out a better way of cloning collision with CSector
			p_new_geom->mp_coll_tri_data = NULL;
			//p_new_geom->mp_coll_tri_data = mp_coll_tri_data->Clone(instance);
			//Dbg_Assert(p_new_geom->mp_coll_tri_data);
#endif
		}
		// Mick:  bit of a patch - attempt to fix crash from deleting thigns twice
		p_new_geom->mp_orig_render_colors = NULL;
	}

	return p_new_geom;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom* CGeom::Clone(bool instance, CModel* pDestModel)
{
	// Create new instance and clone stuff below p-line
	CGeom* p_new_geom = plat_clone(instance, pDestModel);

	if (p_new_geom)
	{
		p_new_geom->m_cloned = (instance) ? vINSTANCE : vCOPY;

		Dbg_MsgAssert(!mp_coll_tri_data, ("Cloning of model geometry doesn't support collision now"));

		// Mick:  bit of a patch - attempt to fix crash from deleting thigns twice
		p_new_geom->mp_orig_render_colors = NULL;
	}

	return p_new_geom;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGeom::LoadGeomData( CMesh* pMesh, CModel* pModel, bool color_per_material )
{
	// All data created here is an instance of something else
	m_cloned = vINSTANCE;
	m_multipleColorsEnabled = color_per_material;

	return plat_load_geom_data( pMesh, pModel, color_per_material );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::Finalize()
{
	plat_finalize();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CGeom::GetChecksum()
{		
	return plat_get_checksum();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetColor(Image::RGBA rgba)
{		
	plat_set_color(rgba);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::ClearColor()
{		
	plat_clear_color();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA	CGeom::GetColor() const
{		
	return plat_get_color();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGeom::SetMaterialColor(uint32 mat_checksum, int pass, Image::RGBA rgba)
{
	return plat_set_material_color(mat_checksum, pass, rgba);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA	CGeom::GetMaterialColor(uint32 mat_checksum, int pass)
{
	return plat_get_material_color(mat_checksum, pass);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetVisibility(uint32 mask)
{
	plat_set_visibility(mask);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CGeom::GetVisibility() const
{
	return plat_get_visibility();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetActive( bool active )
{
	plat_set_active( active );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGeom::IsActive() const
{
	return plat_is_active();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::CBBox & CGeom::GetBoundingBox() const
{
	return plat_get_bounding_box();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector CGeom::GetBoundingSphere() const
{
	return plat_get_bounding_sphere();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetBoundingSphere( const Mth::Vector& boundingSphere )
{
	plat_set_bounding_sphere( boundingSphere );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetWorldPosition(const Mth::Vector& pos)
{
	plat_set_world_position(pos);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector & CGeom::GetWorldPosition() const
{
	return plat_get_world_position();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetOrientation(const Mth::Matrix& orient)
{
	plat_set_orientation(orient);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Matrix & CGeom::GetOrientation() const
{
	return plat_get_orientation();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::RotateY(Mth::ERot90 rot)
{
	plat_rotate_y(rot);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetScale(const Mth::Vector& scale)
{
	plat_set_scale(scale);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CGeom::GetScale() const
{
	return plat_get_scale();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetCollTriData(CCollObjTriData *p_coll_data)
{
	mp_coll_tri_data = p_coll_data;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollObjTriData* CGeom::GetCollTriData() const
{
	return mp_coll_tri_data;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetModelLights(CModelLights *p_model_lights)
{
	plat_set_model_lights(p_model_lights);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGeom::Render( Mth::Matrix* pMatrix, Mth::Matrix* ppBoneMatrices, int numBones )
{
	return plat_render( pMatrix, ppBoneMatrices, numBones );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CGeom::SetBoneMatrixData( Mth::Matrix* pBoneMatrices, int numBones )
{
	plat_set_bone_matrix_data( pBoneMatrices, numBones );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGeom::HidePolys( uint32 mask )
{
	return plat_hide_polys( mask );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGeom::EnableShadow( bool enabled )
{
	return plat_enable_shadow( enabled );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGeom::GetNumRenderVerts()								// - returns number of renderable verts
{
		return plat_get_num_render_verts();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::GetRenderVerts(Mth::Vector *p_verts)				// - gets a single array of the verts
{
		return plat_get_render_verts(p_verts);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::GetRenderColors(Image::RGBA *p_colors)				// - gets an array of vertex colors
{
		return plat_get_render_colors(p_colors);

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Return the total number of renerable polygons 
int	CGeom::GetNumRenderPolys()
{
 	return		plat_get_num_render_polys();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Return the total number of base polygons (excludes those generated for multipass)
int	CGeom::GetNumRenderBasePolys()
{
 	return		plat_get_num_render_base_polys();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Get the original render colors
// if this is the first time this function was called
// then we will allocate memory and store the colors
void CGeom::GetOrigRenderColors(Image::RGBA *p_colors)				// - gets an array of vertex colors
{
	int verts = GetNumRenderVerts();
	if (!mp_orig_render_colors)
	{
		// GJ:  this check was previously used to make sure the malloc caused no bottomup
		// fragmentation during cutscenes.	however, it's no longer needed because of
		// the new cutscene heap.  i'm going to leave the code here commented out, just in case
//		#ifdef	__NOPT_ASSERT__
//		Mdl::Skate* skate_mod = Mdl::Skate::Instance();
//		Dbg_MsgAssert( !skate_mod->GetMovieManager()->IsRolling(), ( "Can't create lights during cutscenes" ) );
//		#endif

		// Mick: Game specific optimization here
		// because the "fake lights" are only used in non-net games
		// we use the frontend/net heap to store the original colors
		// as it is under utilized in a a single player game
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());		
		mp_orig_render_colors = (Image::RGBA*) Mem::Malloc(verts * sizeof(Image::RGBA));
		Mem::Manager::sHandle().PopContext();
		
		GetRenderColors(mp_orig_render_colors);
	}
	memcpy(p_colors,mp_orig_render_colors,verts * sizeof(Image::RGBA));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetRenderVerts(Mth::Vector *p_verts)				// - sets the verts after modification
{
		return plat_set_render_verts(p_verts);

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetRenderColors(Image::RGBA *p_colors)				// - sets the colors after modification
{

		return plat_set_render_colors(p_colors);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetUVWibbleParams(float u_vel, float u_amp, float u_freq, float u_phase,
							  float v_vel, float v_amp, float v_freq, float v_phase)
{
	plat_set_uv_wibble_params(u_vel, u_amp, u_freq, u_phase, v_vel, v_amp, v_freq, v_phase);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::UseExplicitUVWibble(bool yes)
{
	plat_use_explicit_uv_wibble(yes);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGeom::SetUVWibbleOffsets(float u_offset, float v_offset)
{
	plat_set_uv_wibble_offsets(u_offset, v_offset);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGeom::SetUVWibbleOffsets(uint32 mat_checksum, int pass, float u_offset, float v_offset)
{
	return plat_set_uv_wibble_offsets(mat_checksum, pass, u_offset, v_offset);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGeom::SetUVMatrix(uint32 mat_checksum, int pass, const Mth::Matrix &mat)
{
	return plat_set_uv_matrix(mat_checksum, pass, mat);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGeom::AllocateUVMatrixParams(uint32 mat_checksum, int pass)
{
	return plat_allocate_uv_matrix_params(mat_checksum, pass);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGeom::MultipleColorsEnabled()
{
	return m_multipleColorsEnabled;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // Nx
