///////////////////////////////////////////////////////////////////////////////
// NxSector.h



#ifndef	__GFX_NXSECTOR_H__
#define	__GFX_NXSECTOR_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include 	<gfx/image/imagebasic.h>

namespace Mth {
	class Vector;
	class CBBox;
}

namespace Nx
{

////////////////////////////////////////////////////////////////
// The Sector Story:
//
// To the designer, A "Sector" (previously a "world sector" is an
// object in 3D Studio Max.  Typically this might be a tree, a wall
// a section of a building, a parked car or any other static part of the world
//
// The designer wants to be able to:
// -  load the sector as part of the scene, see it and collide with it
// -  reference the sector by name
// -  Turn it on and off (both collision and display)
// -  make it visible and invisible in particular viewports for LOD reasons
// -  change its color
// -  change various attributes, like UV wibble speed.  
//
// To the engine programmer, the sector will be split into various bits of data
// for optimization and platform specific reasons.  The geometry might be in different chunks
// and the collision data might be seperate from the rendering data.  
// The act of "turning off" a sector will be very different from platform to platform
//
// To get round this problem, we have a CSector class which acts as a facade
// to the actual machine specific sector data.
//
// When the engine loads a scene, it will create a list of CSectors, one for each
// sector in the scene.  
// Each CSector will actually be a platfrom specific class derived from CSector,
// like: CPs2Sector, CXboxSector or CNGC Sector
// The derived class (like, CPs2Sector), contains the platform specific data that
// identifies the sector in the engine.  On the PS2, this might have a list of the 
// meshes that make up the sector, maybe a list of the materials, probably a pointer to the 
// collision data, and maybe a pointer to a structure containing sector flags.
// CSector provides functions like SetVisibility(), which call a platform specific
// function (PlatSetVisibility()), which calls the actual engine specific
// function, which performs whatever is necessary at a low level to make the 
// sector visible/invisible (on the PS2, this might involve relinking a DMA chain,
// on other platforms, it might just involve flipping a bit.)
//
// The "engine programmers" have to implement and maintain the platform
// specific CPs2Sector (and CXBox.., etc) implementation.
//
// The "game programmers" simply use the CSector class, and will never need to know 
// anything about the derived class, and their code will work across all three platforms.  
//
// The End
/////////////////////////////////////////////////////////////////////////////////

// Forward declaration
class CGeom;
class CScene;
class CCollStatic;
#ifdef	__PLAT_NGPS__		
class CPs2Scene;
#endif //	__PLAT_NGPS__		

// The CSector class is the platform independent abstract base class
// of the platform specific CSector classes
class	CSector
{
public:
// The basic interface to the sector
// this is the machine independent part	
// machine independ range checking, etc can go here	
							CSector();
	virtual					~CSector();

	uint32					GetChecksum() const;
	void					SetChecksum(uint32 c);

	CGeom *					GetGeom() const;
	void					SetGeom(CGeom *p_geom);

	void					SetColor(Image::RGBA rgba);
	Image::RGBA				GetColor() const;
	void					ClearColor();
	
	void 					SetLightGroup(uint32 light_group) {m_light_group = light_group;}
	uint32 					GetLightGroup() {return m_light_group;}

	void					SetVisibility(uint32 mask);
	uint32					GetVisibility() const;

	void					SetActive(bool on);
	bool					IsActive() const;

	void					SetActiveAtReplayStart(bool on);
	bool					GetActiveAtReplayStart() const;
	void					SetVisibleAtReplayStart(bool on);
	bool					GetVisibleAtReplayStart() const;
	void					SetStoredActive(bool on);
	bool					GetStoredActive() const;
	void					SetStoredVisible(bool on);
	bool					GetStoredVisible() const;
	
	const Mth::CBBox &		GetBoundingBox() const;

	void					SetWorldPosition(const Mth::Vector& pos);
	const Mth::Vector &		GetWorldPosition() const;

	// Rotation is applied to meshes permanently
	void					SetYRotation(Mth::ERot90 rot);
	Mth::ERot90				GetYRotation() const;			// Gets applied rotation

	// Scale is applied to meshes permanently
	void					SetScale(const Mth::Vector& scale);
	Mth::Vector				GetScale() const;				// Gets scale

	// Change collision data of a sector
	int						GetNumCollVertices() const;
	void					GetRawCollVertices(Mth::Vector *p_vert_array) const;
	void					SetRawCollVertices(const Mth::Vector *p_vert_array);

	void					SetCollidable(bool on);
	bool					IsCollidable() const;

	void					SetShatter(bool on);
	bool					GetShatter() const;

	bool					AddCollSector(Nx::CCollStatic *p_coll_sector);
	CCollStatic *			GetCollSector() const;

	void					SetOccluder(bool on);
	
	// Wibble functions
	void					SetUVWibbleParams(float u_vel, float u_amp, float u_freq, float u_phase,
											  float v_vel, float v_amp, float v_freq, float v_phase);
	void					UseExplicitUVWibble(bool yes);
	void					SetUVWibbleOffsets(float u_offset, float v_offset);
	
protected:
	// Internal sector flags.
	enum {
		mCOLLIDE					= 0x0001,
		mCLONE						= 0x0002,
		mADD_TO_SUPER_SECTORS		= 0x0004,					// Cloned sector needs adding to Super Sectors
		mMARKED_FOR_DELETION		= 0x0008,					// Cloned object no longer used but not deleted yet
		mREMOVE_FROM_SUPER_SECTORS	= 0x0010,					// Take out of Super Sectors w/o deleting
		mINVALID_SUPER_SECTORS		= 0x0020,					// Tells if SuperSectors needs updating
		mIN_SUPER_SECTORS			= 0x0040,					// Tells if in SuperSectors
		// These 4 used by replay code
		mACTIVE_AT_REPLAY_START		= 0x0080,					// Whether the sector is active at the start of the replay
		mVISIBLE_AT_REPLAY_START	= 0x0100,					// Whether the sector is visible at the start of the replay
		mSTORED_ACTIVE				= 0x0200,					// Stored active status, so that it can be restored at the end of the replay
		mSTORED_VISIBLE				= 0x0400,					// Stored visible status, so that it can be restored at the end of the replay
	};

	uint32					get_flags() const;				// Get internal flags
	void					set_flags(uint32 flags);		// Set internal flags
	void					clear_flags(uint32 flags);		// Clear internal flags

	uint32					m_checksum;
	uint32					m_flags;
	uint32					m_light_group;		// checksum of light group as specified in node array

	Mth::ERot90				m_rot_y;		// since rotation already applied below p-line, we keep the value above
	Mth::Vector				m_scale;		// since scale already applied below p-line, we keep the value above

	CGeom *					mp_geom;		// The geometry of the sector
	CCollStatic *			mp_coll_sector;	// Collision object for the sector

private:
	CSector *				clone(bool instance, bool add_to_super_sectors, CScene *p_dest_scene = NULL);	// must only be called through CScene

	// The virtual functions will have a stub implementation
	// in nxsector.cpp
	//virtual	void			plat_set_color(Image::RGBA rgba);
	//virtual	Image::RGBA		plat_get_color() const;
	//virtual	void			plat_clear_color();

	//virtual	void			plat_set_visibility(uint32 mask);
	//virtual	uint32			plat_get_visibility() const;

	//virtual	void			plat_set_active(bool on);
	//virtual	bool			plat_is_active() const;

	//virtual const Mth::CBBox &	plat_get_bounding_box() const;

	//virtual void			plat_set_world_position(const Mth::Vector& pos);
	//virtual const Mth::Vector &	plat_get_world_position() const;

	//virtual void			plat_set_y_rotation(Mth::ERot90 rot);

	virtual	void			plat_set_shatter(bool on);
	virtual	bool			plat_get_shatter() const;

	virtual CSector *		plat_clone(bool instance, CScene *p_dest_scene);

	// CScene needs access to clone() and internal flags
	friend CScene;
#ifdef	__PLAT_NGPS__		
	friend CPs2Scene;
#endif //	__PLAT_NGPS__		
};

}

#endif // 

