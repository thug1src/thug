/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2002 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Nx														**
**																			**
**	File name:		CollCache.h												**
**																			**
**	Created: 		08/16/2002	-	grj										**
**																			**
*****************************************************************************/

#ifndef	__GEL_COLLCACHE_H
#define	__GEL_COLLCACHE_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/math.h>
#include <core/math/geometry.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Nx
{

class CCollObj;
class CCollStatic;
class CCollObjTriData;
class CCollCacheManager;

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/
struct SCollCacheNode
{
	CCollObj			*mp_coll_obj;		// Collision object
	const Mth::CBBox	*mp_bbox;			// Current bounding box
};

////////////////////////////////////

class CCollCache
{
public:
							CCollCache();
							~CCollCache();

	void					Clear();

	// WARNING: The Update() function takes just as long as a normal collision call.  Make sure you
	// plan on using the cache multiple times.
	void					Update(const Mth::CBBox &bbox);

	// Checks to see if this cache can be used
	bool					Contains(const Mth::CBBox &test_bbox) const;
	bool					Contains(const Mth::Line &test_line) const;

	const SCollCacheNode *	GetCollisionArray() const;
	int						GetNumCollisions() const;
	const SCollCacheNode *	GetStaticCollisionArray() const;
	int						GetNumStaticCollisions() const;
	const SCollCacheNode *	GetMovableCollisionArray() const;
	int						GetNumMovableCollisions() const;

protected:
	enum
	{
		MAX_COLLISION_OBJECTS = 500,
	};

	void					add_static_collision(CCollStatic *p_collision);
	void					add_movable_collision(CCollObj *p_collision);
	void					delete_static_collision(CCollStatic *p_collision);
	void					delete_movable_collision(CCollObj *p_collision);
	void					delete_collision(CCollObj *p_collision);			// In case we don't know what type (less efficient)

	Mth::CBBox				m_bbox;				// bounding box where cache is valid
	SCollCacheNode			m_collision_array[MAX_COLLISION_OBJECTS];
	int						m_array_size;
	int						m_num_static_coll;
	int						m_num_movable_coll;

	// Friends
	friend CCollCacheManager;
};

////////////////////////////////////

class CCollCacheManager
{
public:

	// Allocation functions
	static CCollCache *		sCreateCollCache();
	static void				sDestroyCollCache(CCollCache *p_cache);

	// Collision deletion functions
	static void				sDeleteStaticCollision(CCollStatic *p_collision);
	static void				sDeleteMovableCollision(CCollObj *p_collision);
	static void				sDeleteCollision(CCollObj *p_collision);			// In case we don't know what type (less efficient)
	
	static void				sSetAssertOnCacheMiss ( bool state ) { s_assert_on_cache_miss = state; }
	static bool				sGetAssertOnCacheMiss (   ) { return s_assert_on_cache_miss; }

protected:
	enum
	{
		MAX_COLLISION_CACHES = 8,
	};
	
	static CCollCache *		sp_coll_cache_array[MAX_COLLISION_CACHES];
	static int				s_num_coll_caches;
	
	static bool				s_assert_on_cache_miss;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool					CCollCache::Contains(const Mth::CBBox &test_bbox) const
{
	return m_bbox.Within(test_bbox);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool					CCollCache::Contains(const Mth::Line &test_line) const
{
	return (m_bbox.Within(test_line.m_start) && m_bbox.Within(test_line.m_end));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const SCollCacheNode *CCollCache::GetCollisionArray() const
{
	return m_collision_array;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline int					CCollCache::GetNumCollisions() const
{
	return m_array_size;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const SCollCacheNode *CCollCache::GetStaticCollisionArray() const
{
	return m_collision_array;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline int					CCollCache::GetNumStaticCollisions() const
{
	return m_num_static_coll;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const SCollCacheNode *CCollCache::GetMovableCollisionArray() const
{
	return &(m_collision_array[m_num_static_coll]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline int					CCollCache::GetNumMovableCollisions() const
{
	return m_num_movable_coll;
}

} // namespace Nx

#endif	//	__GEL_COLLCACHE_H
