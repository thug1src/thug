///////////////////////////////////////////////////////////////////////////////
// p_NxSector.cpp

#include 	"Gfx/nx.h"
#include 	"Gfx/NGPS/p_NxGeom.h"
#include 	"Gfx/NGPS/p_NxSector.h"
#include 	"Gfx/NxMiscFX.h"
#include 	<gfx/image/imagebasic.h>
#include 	"gfx\ngps\nx\geomnode.h"

#include 	"gel/collision/collision.h"

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Sector::CPs2Sector() :
	mp_plat_object(NULL), 
	m_rgba(Image::RGBA(128, 128, 128, 128)),
	m_shatter(false)
{
	m_world_matrix.Ident();
	m_active = true;						// default to be active....
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Sector::~CPs2Sector()
{
	if (mp_geom)
	{
		//CPs2Geom *p_ps2_geom = static_cast<CPs2Geom *>(mp_geom);
		//p_ps2_geom->SetEngineObject(NULL);		// Hack so that CGeom doesn't try to free the CGeomNode
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CSector
// and we will also have a CXboxSector, CNgcSector, even a CPcSector
// maybe in the future we will have a CPS3Sector?

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Sector::plat_set_color(Image::RGBA rgba)
{
	// Set values
	m_rgba = rgba;

	// Engine call here
	if (mp_plat_object)
	{
		mp_plat_object->SetColored(true);
		mp_plat_object->SetColor(*((uint32 *) &rgba));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA CPs2Sector::plat_get_color() const
{
	return m_rgba;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Sector::plat_clear_color()
{
#if 0
	// Set to white
	plat_set_color(Image::RGBA(128, 128, 128, 128));
#endif
	// Engine call here
	if (mp_plat_object)
	{
		mp_plat_object->SetColored(false);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Sector::plat_set_visibility(uint32 mask)
{
	// Engine call here
	if (mp_plat_object)
	{
		mp_plat_object->SetVisibility(mask & 0xFF);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CPs2Sector::plat_get_visibility() const
{
	if ( mp_plat_object )
	{
		return mp_plat_object->GetVisibility() | 0xFFFFFF00;		// To keep format compatible
	}
	else
	{
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Sector::plat_set_active(bool on)	
{
	// Set values
	if (m_active != on)
	{
		m_active = on;

		if (mp_plat_object)
		{
			// Engine call here
			mp_plat_object->SetActive(on);
		}
	}
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2Sector::plat_is_active() const
{
	return m_active;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::CBBox & CPs2Sector::plat_get_bounding_box() const
{
	// Garrett: TODO: change to renderable bounding box
	Dbg_Assert(mp_coll_sector);
	Dbg_Assert(mp_coll_sector->GetGeometry());
	return mp_coll_sector->GetGeometry()->GetBBox();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Sector::plat_set_world_position(const Mth::Vector& pos)
{
	 // Set values
	 m_world_matrix[W] = pos;

	 // Engine call here
	 // TEMP, just update matrix of CGeomNode
	 update_engine_matrix();
 }

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &	CPs2Sector::plat_get_world_position() const
{
	return m_world_matrix[W];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Sector::plat_set_y_rotation(Mth::ERot90 rot)
{
	// Engine call here
	// Garrett: TEMP just set the world matrix
	Mth::Matrix orig_rot_mat(m_world_matrix), rot_mat;
	float rad = (float) ((int) rot) * (Mth::PI * 0.5f);
	CreateRotateYMatrix(rot_mat, rad);

	orig_rot_mat[W] = Mth::Vector(0.0f, 0.0f, 0.0f, 1.0f);

	rot_mat = orig_rot_mat * rot_mat;
	//Dbg_Message("[X] (%f, %f, %f, %f)", rot_mat[X][X], rot_mat[X][Y],  rot_mat[X][Z],  rot_mat[X][W]);
	//Dbg_Message("[Y] (%f, %f, %f, %f)", rot_mat[Y][X], rot_mat[Y][Y],  rot_mat[Y][Z],  rot_mat[Y][W]);
	//Dbg_Message("[Z] (%f, %f, %f, %f)", rot_mat[Z][X], rot_mat[Z][Y],  rot_mat[Z][Z],  rot_mat[Z][W]);
	//Dbg_Message("[W] (%f, %f, %f, %f)", rot_mat[W][X], rot_mat[W][Y],  rot_mat[W][Z],  rot_mat[W][W]);
	
	m_world_matrix[X] = rot_mat[X];
	m_world_matrix[Y] = rot_mat[Y];
	m_world_matrix[Z] = rot_mat[Z];

	update_engine_matrix();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Sector::update_engine_matrix()
{
	if ( mp_plat_object )
	{
		mp_plat_object->SetMatrix(&m_world_matrix);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Sector::plat_set_shatter(bool on)	
{
	// Set values
	m_shatter = on;

	// Engine call here
	if( on && mp_geom )
	{
		Shatter( mp_geom );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2Sector::plat_get_shatter() const
{
	return m_shatter;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSector * CPs2Sector::plat_clone(bool instance, CScene *p_dest_scene)
{
	// Copy into new sector
	CPs2Sector *p_new_sector = new CPs2Sector(*this);

	// Copy engine data
	if (p_new_sector && mp_plat_object)
	{
		if (instance)
		{
			p_new_sector->mp_plat_object = mp_plat_object->CreateInstance(&p_new_sector->m_world_matrix);
		} else {
			p_new_sector->mp_plat_object = mp_plat_object->CreateInstance(&p_new_sector->m_world_matrix);
			Dbg_Message("CPs2Sector: Can't create copy of CGeomNode yet");
			//p_new_sector->mp_plat_object = mp_plat_object->CreateCopy();
		}
	}

	return p_new_sector;
}

} // Namespace Nx  			
				
				
