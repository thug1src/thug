//////////////////////////////////////////
// Emitter.h

#ifndef __OBJECTS_Emitter_H
#define __OBJECTS_Emitter_H

#include <core\hashtable.h>
#include <core\math\geometry.h>

namespace Nx
{
	class	CSector;
}

namespace Gfx
{
	class	Camera;
}

namespace Script
{
	class	CScript;
}

namespace Obj
{

	class	CSkater;
	class CEmitterManager;

class CEmitterObject : public Spt::Class
{
public:
	enum EShape
	{
		vSHAPE_BBOX = 0,		// Axis-aligned bounding box
		vSHAPE_NON_AA_BBOX,		// Non-axis-aligned bounding box
		vSHAPE_SPHERE,
	};

	///////////
	// Functions
	void				InitGeometry(Nx::CSector *p_sector);
	bool				CheckInside(const Mth::Vector &object_pos) const;
	Mth::Vector 		GetClosestPoint(const Mth::Vector &object_pos) const;

	// Checksum conversion functions
	static EShape		sGetShapeFromChecksum(uint32 checksum);

protected:
	///////////
	// Variables
	int			m_node;				// Number of the node in the trigger array
	int 		m_link;				// link number
	uint32		m_name;				// checksum of name of the node in the trigger array
	Mth::Vector m_pos;				// position of the node
	Mth::Vector m_angles;			// angles of the node
	bool 		m_active;			// Emitter segment starting at this node: active or not?

	//int			m_outside_flags;	// bitmask of flags, 0 = inside the radius, 1 = outside

	EShape 		m_shape_type;

	// Sphere data
	float		m_radius;
	float		m_radius_squared;

	// Geometry Data
	Nx::CSector *mp_sector;

	// BBox data
	Mth::CBBox	m_bbox;
	
	float		m_last_distance_squared;
	
	uint32 		m_script;

	//CEmitterObject *m_pNext;


	// friends
	friend CEmitterManager;
};

 

class CEmitterManager
{
public:
	static void				sInit();
	static void				sCleanup();
	static void				sAddEmitter(int node, bool active);
	static void				sAddedAll();
	static CEmitterObject * sGetEmitter(uint32 checksum);

	// activate/deactivate Emitter:
	static void				sSetEmitterActive(int node, int active);

	//static void				sUpdateEmitters(CSkater *pSkater, Gfx::Camera* camera);

protected:
	static Lst::HashTable<CEmitterObject> *	sp_emitter_lookup;	
};



} 

			
#endif			// #ifndef __OBJECTS_Emitter_H
			


