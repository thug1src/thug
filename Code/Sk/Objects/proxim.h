//////////////////////////////////////////
// Proxim.h

#ifndef __OBJECTS_Proxim_H
#define __OBJECTS_Proxim_H

#include <core\math\geometry.h>

#include <gfx/camera.h>

namespace Nx
{
	class	CSector;
}

namespace Script
{
	class	CScript;
}

namespace Obj
{

	class	CSkater;

class CProximNode : public Spt::Class
{
public:
	enum EShape
	{
		vSHAPE_SPHERE = 0,
		vSHAPE_BBOX,			// Axis-aligned bounding box
		vSHAPE_NON_AA_BBOX,		// Non-axis-aligned bounding box
		vSHAPE_CYLINDER,		// Cylinder
		vSHAPE_GEOMETRY,		// Geometry data
	};

	///////////
	// Functions
	void		SetActive(bool active);
	void		InitGeometry(Nx::CSector *p_sector);
	bool		HasGeometry() const;
	bool		CheckInside(const Mth::Vector &object_pos) const;
	Mth::Vector GetClosestIntersectionPoint(const Mth::Vector &object_pos) const;

	// Checksum conversion functions
	static EShape sGetShapeFromChecksum(uint32 checksum);

	///////////
	// Variables
	int			m_node;				// Number of the node in the trigger array
	int 		m_link;				// link number
	uint32		m_name;				// checksum of name of the node in the trigger array
	Mth::Vector m_pos;				// position of the node
	Mth::Vector m_angles;			// angles of the node
	bool 		m_active;			// Proxim segment starting at this node: active or not?

	uint32 		m_proxim_object;	// checksum of name, if this is a ProximObject
	uint32		m_active_objects;	// bit number corresponds to originating object ID
	uint32		m_active_scripts;	// bit number corresponds to originating object ID

	int			m_outside_flags;	// bitmask of flags, 0 = inside the radius, 1 = outside

	int 		m_type;
	EShape 		m_shape;

	// Sphere data
	float		m_radius;
	float		m_radius_squared;

	// Geometry Data
	Nx::CSector *mp_sector;

	// BBox data
	Mth::CBBox	m_bbox;
	
	float		m_last_distance_squared;
	
	uint32 		m_script;

	CProximNode *m_pNext;

};

 

void Proxim_AddNode(int node, bool active);
void Proxim_Init();
void Proxim_Cleanup();
void Proxim_AddedAll();
// activate/deactivate Proxim:
void Proxim_SetActive( int node, int active);
void Proxim_Update ( uint32 trigger_mask, CObject* p_script_target, const Mth::Vector& pos );

class CProximManager : public Spt::Class
{
private:
	static const int vMAX_NUM_PROXIM_TRIGGERS = 8 * sizeof(uint32);
	
public:
	CProximManager() { m_inside_flag_valid = false; }

	static CProximNode * sGetNode(uint32 checksum);

	CProximNode*  m_pFirstNode;	
		
	uint32 GetProximTriggerMask ( int viewport );
	
	bool IsInsideFlagValid (   ) { return m_inside_flag_valid; }
	bool IsInside (   ) { return m_inside_flag; }
	
private:
	bool m_inside_flag;
	bool m_inside_flag_valid;
	
	friend void Proxim_Update ( uint32 trigger_mask, CObject* p_script_target, const Mth::Vector& pos );
	friend CProximNode;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint32 CProximManager::GetProximTriggerMask ( int viewport )
{
	return 1 << viewport;
}

} 
			
#endif // #ifndef __OBJECTS_Proxim_H
			


