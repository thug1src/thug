///////////////////////////////////////////////////////////////////////////////
// NxSector.cpp

#include "gfx/nx.h"
#include "gfx/nxgeom.h"
#include "gfx/nxsector.h"
#include "gfx/nxflags.h"
#include "gel/collision/collision.h"
#include <sys/replay/replay.h>

namespace	Nx
{
///////////////////////////////////////////////////////////////////
// CSector definitions

// These functions are the platform independent part of the interface to 
// the platform specific code
// parameter checking can go here....
// although we might just want to have these functions inline, or not have them at all?
					 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSector::CSector()
{
	m_flags = mCOLLIDE;
	mp_geom = NULL;
	mp_coll_sector = NULL;
	m_rot_y = Mth::ROT_0;
	uint32	light_group = CRCD(0xe3714dc1,"outdoor");			 // default to OutDoors
	SetLightGroup(light_group);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSector::~CSector()
{
	if (mp_coll_sector && (m_flags & mCLONE))	// cloned collision isn't in scene array
	{
		// Check if in SuperSectors
		if (m_flags & mIN_SUPER_SECTORS)
		{
			Dbg_MsgAssert(0, ("Can't free CSector when it is still on the SuperSector lists"));
		}

		delete mp_coll_sector;
	}

	if (mp_geom /*&& (m_flags & mCLONE)*/)			// cloned geometry isn't in scene array
	{
		delete mp_geom;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	CSector::GetChecksum() const
{
	return m_checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetChecksum(uint32 c)
{
	m_checksum = c;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom *	CSector::GetGeom() const
{
	return mp_geom;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetGeom(CGeom *p_geom)
{
	mp_geom = p_geom;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetColor(Image::RGBA rgba)
{		
	if (mp_geom)
	{
		mp_geom->SetColor(rgba);
	//} else {
	//	plat_set_color(rgba);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::ClearColor()
{		
	if (mp_geom)
	{
		mp_geom->ClearColor();
	//} else {
	//	plat_clear_color();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA	CSector::GetColor() const
{		
	if (mp_geom)
	{
		return mp_geom->GetColor();
	} else {
		return Image::RGBA(0, 0, 0, 0);
	//	return plat_get_color();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetVisibility(uint32 mask)
{
	if (mp_geom)
	{
		uint32 vis=mp_geom->GetVisibility();
		if ( (vis^mask) & 0x01 )
		{
			Replay::WriteSectorVisibleStatus(m_checksum,mask&0x01);
		}	
		mp_geom->SetVisibility(mask);
	//} else {
	//	plat_set_visibility(mask);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	CSector::GetVisibility() const
{
	if (mp_geom)
	{
		return mp_geom->GetVisibility();
	} else {
		return 0;
	//	return plat_get_visibility();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetActive(bool on)
{
	// Do collision also
	if (mp_coll_sector)
	{
		if (on) {
			mp_coll_sector->ClearObjectFlags(mSD_KILLED);
		} else {
			mp_coll_sector->SetObjectFlags(mSD_KILLED);
		}
	}

	if (mp_geom)
	{
		if (mp_geom->IsActive() != on)
		{
			Replay::WriteSectorActiveStatus(m_checksum,on);
		}
		mp_geom->SetActive(on);
	//} else {
	//	plat_set_active(on);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSector::IsActive() const
{
	if (mp_geom)
	{
		return mp_geom->IsActive();
	} else {
		Dbg_Assert(mp_coll_sector);

		return !(mp_coll_sector->GetObjectFlags() & mSD_KILLED);
	//	return plat_is_active();
	}
}

void CSector::SetActiveAtReplayStart(bool on)
{
	if (on)
	{
		m_flags|=mACTIVE_AT_REPLAY_START;
	}
	else
	{
		m_flags&=~mACTIVE_AT_REPLAY_START;
	}
}

bool CSector::GetActiveAtReplayStart() const
{
	return m_flags&mACTIVE_AT_REPLAY_START;
}

void CSector::SetVisibleAtReplayStart(bool on)
{
	if (on)
	{
		m_flags|=mVISIBLE_AT_REPLAY_START;
	}
	else
	{
		m_flags&=~mVISIBLE_AT_REPLAY_START;
	}
}

bool CSector::GetVisibleAtReplayStart() const
{
	return m_flags&mVISIBLE_AT_REPLAY_START;
}

void CSector::SetStoredActive(bool on)
{
	if (on)
	{
		m_flags|=mSTORED_ACTIVE;
	}
	else
	{
		m_flags&=~mSTORED_ACTIVE;
	}
}

bool CSector::GetStoredActive() const
{
	return m_flags&mSTORED_ACTIVE;
}

void CSector::SetStoredVisible(bool on)
{
	if (on)
	{
		m_flags|=mSTORED_VISIBLE;
	}
	else
	{
		m_flags&=~mSTORED_VISIBLE;
	}
}

bool CSector::GetStoredVisible() const
{
	return m_flags&mSTORED_VISIBLE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::CBBox & CSector::GetBoundingBox() const
{
	if (mp_geom)
	{
		return mp_geom->GetBoundingBox();
	} else {
		Dbg_Assert(mp_coll_sector);

		CCollObjTriData *p_coll_data = mp_coll_sector->GetGeometry();

		Dbg_Assert(p_coll_data);

		return p_coll_data->GetBBox();
	//	return plat_get_bounding_box();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetWorldPosition(const Mth::Vector& pos)
{
	// Do collision first
	if (mp_coll_sector)
	{
		mp_coll_sector->SetWorldPosition(pos);
		if (m_flags & mIN_SUPER_SECTORS)
		{
			m_flags |= mINVALID_SUPER_SECTORS;
		}

	}

	// Then Geometry
	if (mp_geom)
	{
		mp_geom->SetWorldPosition(pos);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &	CSector::GetWorldPosition() const
{
	if (mp_geom)
	{
		return mp_geom->GetWorldPosition();
	} else {
		Dbg_Assert(mp_coll_sector);

		return mp_coll_sector->GetWorldPosition();
	//	return plat_get_world_position();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetYRotation(Mth::ERot90 rot)
{
	Mth::ERot90 delta_rot = (Mth::ERot90) (((rot + Mth::NUM_ROTS) - m_rot_y) % Mth::NUM_ROTS);
	//Dbg_Message("SetYRotation: new rot %d sector rot %d delta rot %d", rot, m_rot_y, delta_rot);

	// Do collision first
	if (mp_coll_sector)
	{
		mp_coll_sector->RotateY(GetWorldPosition(), delta_rot);
	}

	if (mp_geom)
	{
		mp_geom->RotateY(delta_rot);
	//} else {
	//	plat_set_y_rotation(delta_rot);
	}

	// Now change value
	m_rot_y = rot;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::ERot90	CSector::GetYRotation() const
{
	return m_rot_y;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetScale(const Mth::Vector & scale)
{
	// Do collision first
	if (mp_coll_sector)
	{
		mp_coll_sector->Scale(GetWorldPosition(), scale);
		if (m_flags & mIN_SUPER_SECTORS)
		{
			m_flags |= mINVALID_SUPER_SECTORS;
		}

	}

	if (mp_geom)
	{
		mp_geom->SetScale(scale);
	}

	// Now change value
	m_scale = scale;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CSector::GetScale() const
{
	return m_scale;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		CSector::GetNumCollVertices() const
{
	if (mp_coll_sector && mp_coll_sector->GetGeometry())
	{
		return mp_coll_sector->GetGeometry()->GetNumVerts();
	}
	else
	{
		Dbg_MsgAssert(0, ("SetRawCollVertices(): Can't find collision geometry"));
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::GetRawCollVertices(Mth::Vector *p_vert_array) const
{
	if (mp_coll_sector && mp_coll_sector->GetGeometry())
	{
		mp_coll_sector->GetGeometry()->GetRawVertices(p_vert_array);
	}
	else
	{
		Dbg_MsgAssert(0, ("SetRawCollVertices(): Can't find collision geometry"));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetRawCollVertices(const Mth::Vector *p_vert_array)
{
	if (mp_coll_sector && mp_coll_sector->GetGeometry())
	{
		mp_coll_sector->GetGeometry()->SetRawVertices(p_vert_array);

		// Invalidate sector in SuperSectors
		if (m_flags & mIN_SUPER_SECTORS)
		{
			Dbg_MsgAssert(!(m_flags & mREMOVE_FROM_SUPER_SECTORS), ("Can't change collision verts of a sector being removed"));
			m_flags |= mINVALID_SUPER_SECTORS;
		}
	}
	else
	{
		Dbg_MsgAssert(0, ("SetRawCollVertices(): Can't find collision geometry"));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetCollidable(bool on)
{
	if (on) {
		m_flags |= mCOLLIDE;
	} else {
		m_flags &= ~mCOLLIDE;
	}

	if (mp_coll_sector)
	{
		if (on) {
			mp_coll_sector->ClearObjectFlags(mSD_NON_COLLIDABLE);
		} else {
			mp_coll_sector->SetObjectFlags(mSD_NON_COLLIDABLE);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSector::IsCollidable() const
{
	return (m_flags & mCOLLIDE) || IsActive();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetOccluder(bool on)
{
	Dbg_MsgAssert(mp_coll_sector,("Flagging a sector as occluder that has no collision info"));
	if (mp_coll_sector)
	{
		if (!on)
		{
			mp_coll_sector->ClearObjectFlags(mSD_OCCLUDER);
		}
		else 
		{
			if (! (mp_coll_sector->GetObjectFlags() & mSD_OCCLUDER))
			{
				mp_coll_sector->SetObjectFlags(mSD_OCCLUDER);
				
				// we've just set some collision as an occluder for the first time
				// so now would be a good time to tell the engine about these new occlusion polygons
				
				CCollStaticTri * p_coll_static = static_cast<CCollStaticTri*>(mp_coll_sector);
				Dbg_MsgAssert(p_coll_static,("World sector has non-static collision???"));
				
				p_coll_static->ProcessOcclusion();
			}
		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::SetShatter(bool on)
{
	Replay::WriteShatter(m_checksum,on);
	plat_set_shatter(on);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSector::GetShatter() const
{
	return plat_get_shatter();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSector::AddCollSector(Nx::CCollStatic *p_coll_sector)
{
	Dbg_Assert(m_checksum == p_coll_sector->GetChecksum());

	if (!mp_coll_sector)
	{
		mp_coll_sector = p_coll_sector;
		if (mp_geom)
		{
			mp_geom->SetCollTriData(p_coll_sector->GetGeometry());
		}

		return true;
	} else {
		Dbg_MsgAssert(0, ("CSector::AddCollSector(): adding second collision sector to %x\n", m_checksum));
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::CCollStatic *	CSector::GetCollSector() const
{
	return mp_coll_sector;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32		CSector::get_flags() const
{
	return m_flags;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CSector::SetUVWibbleParams(float u_vel, float u_amp, float u_freq, float u_phase,
											   float v_vel, float v_amp, float v_freq, float v_phase)
{
	if (mp_geom)
	{
		mp_geom->SetUVWibbleParams(u_vel, u_amp, u_freq, u_phase, v_vel, v_amp, v_freq, v_phase);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CSector::UseExplicitUVWibble(bool yes)
{
	if (mp_geom)
	{
		mp_geom->UseExplicitUVWibble(yes);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CSector::SetUVWibbleOffsets(float u_offset, float v_offset)
{
	if (mp_geom)
	{
		mp_geom->SetUVWibbleOffsets(u_offset, v_offset);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSector::set_flags(uint32 flags)
{
	m_flags |= flags;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSector::clear_flags(uint32 flags)
{
	m_flags &= ~flags;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSector *	CSector::clone(bool instance, bool add_to_super_sectors, CScene *p_dest_scene)
{
	// Create new instance and clone stuff below p-line
	CSector *p_new_sector = plat_clone(instance, p_dest_scene);

	if (p_new_sector)
	{
		// Get new checksum
		p_new_sector->m_checksum = CEngine::sGetUnusedSectorChecksum();

		// Now clone stuff above p-line
		p_new_sector->m_flags	= m_flags | mCLONE;		// mark as clone
		p_new_sector->m_rot_y 	= m_rot_y;

		if (mp_geom)
		{
			p_new_sector->mp_geom = mp_geom->Clone(instance, p_dest_scene);
			Dbg_Assert(p_new_sector->mp_geom);
		}

		if (mp_coll_sector)
		{
			
			// Note, static_cast seems to be essentially a macro
			// so you must pass it a pointer, and not the result of an expression
			// otherwise it will continually re-evaluate that expression
			// which here would casue memory leaks.																			  
			CCollObj *p_coll_obj = mp_coll_sector->Clone(false /*instance*/);	// Garrett: We're not even going to try to instamce it
			if (p_coll_obj)
			{
				p_new_sector->mp_coll_sector = static_cast<CCollStatic *>(p_coll_obj);

				p_new_sector->mp_coll_sector->SetChecksum(p_new_sector->m_checksum);

				p_new_sector->m_flags &= ~mIN_SUPER_SECTORS;		// Not in there yet

				if (add_to_super_sectors)
				{
					p_new_sector->m_flags |= mADD_TO_SUPER_SECTORS;
				}

				Dbg_Assert(p_new_sector->mp_coll_sector);

				// For now, just copy the CCollObjTriData pointer over
				if (p_new_sector->mp_geom)
				{
					// Garrett: Commented assert out because the pointer is now copied in CGeom
					//Dbg_Assert(p_new_sector->mp_geom->GetCollTriData() == NULL);
					p_new_sector->mp_geom->SetCollTriData(p_coll_obj->GetGeometry());
				}

				Dbg_Assert(p_coll_obj->GetGeometry() != mp_coll_sector->GetGeometry());
			}
			else
			{
				p_new_sector->mp_coll_sector = NULL;
				p_new_sector->m_flags &= ~mIN_SUPER_SECTORS;		// Not in there at all
				if (p_new_sector->mp_geom)
				{
					p_new_sector->mp_geom->SetCollTriData(NULL);
				}
			}
		}
		
		return p_new_sector;
	} else {
		return NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions are provided here:
// so engine implementors can leave certain functionality until later
						
#if 0
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::plat_set_color(Image::RGBA rgba)
{
	printf ("STUB: PlatSetColor\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA CSector::plat_get_color() const
{
	printf ("STUB: PlatGetColor\n");
	return Image::RGBA(0, 0, 0, 0);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::plat_clear_color()
{
	printf ("STUB: PlatClearColor\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::plat_set_visibility(uint32 mask)
{
	printf ("STUB: PlatSetVisibility\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	CSector::plat_get_visibility() const
{
	printf ("STUB: PlatGetVisibility\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::plat_set_active(bool on)
{
	printf ("STUB: PlatSetActive\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSector::plat_is_active() const
{
	printf ("STUB: PlatIsActive\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

 void	CSector::plat_set_world_position(const Mth::Vector& pos)
 {
	 printf ("STUB: PlatSetWorldPosition\n");
 }

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::CBBox &	CSector::plat_get_bounding_box() const
{
	static Mth::CBBox stub_bbox;
	printf ("STUB: PlatGetBoundingBox\n");
	return stub_bbox;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &	CSector::plat_get_world_position() const
{
	static Mth::Vector stub_vec;
	printf ("STUB: PlatGetWorldPosition\n");
	return stub_vec;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::plat_set_y_rotation(Mth::ERot90 rot)
{
	printf ("STUB: PlatSetYRotation\n");
}
#endif // 0

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSector::plat_set_shatter(bool on)
{
	printf ("STUB: PlatSetShatter\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSector::plat_get_shatter() const
{
	printf ("STUB: PlatGetShatter\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSector *	CSector::plat_clone(bool instance, CScene *p_dest_scene)
{
	printf ("STUB: PlatClone\n");
	return NULL;
}

}


