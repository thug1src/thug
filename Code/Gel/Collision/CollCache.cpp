/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Nx								 						**
**																			**
**	File name:		gel\collision\collcache.cpp   							**
**																			**
**	Created by:		08/16/02	-	grj										**
**																			**
**	Description:															**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <gel/collision/collision.h>
#include <gel/collision/movcollman.h>
#include <gel/collision/collcache.h>
#include <engine/SuperSector.h>

#include <gfx/nx.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/


#define PRINT_TIMES 0

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CCollCache::CCollCache()
{
	Clear();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CCollCache::~CCollCache()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollCache::Clear()
{
	m_bbox.Reset();

	m_array_size = 0;
	m_num_static_coll = 0;
	m_num_movable_coll = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollCache::Update(const Mth::CBBox &bbox)
{
#if PRINT_TIMES
	static uint64 s_total_time = 0, s_num_collisions = 0;
	uint64 start_time = Tmr::GetTimeInUSeconds();
#endif

	// Clear old cache first
	Clear();

	// Make line
	Mth::Line is(bbox.GetMin(), bbox.GetMax());

	// initialize starting "max" distance to be max collision distance
	SSec::Manager *ss_man;
	ss_man = Nx::CEngine::sGetNearestSuperSectorManager(is);
//	Dbg_Assert(ss_man);
   	if (!ss_man)
	{
		return;
	}
 
	// Copy bounding box
	m_bbox = bbox;



	CCollStatic** p_coll_obj_list = ss_man->GetIntersectingCollSectors( is );
	
	// Bloat the collision line only for the purposes of deciding against which world 
	// sectors to test the actual collision line
	Mth::CBBox line_bbox;
	Mth::Vector extend;

	extend = bbox.GetMax();
	extend[X] += CCollObj::sLINE_BOX_EXTENT;
	extend[Y] += CCollObj::sLINE_BOX_EXTENT;
	extend[Z] += CCollObj::sLINE_BOX_EXTENT;
	line_bbox.AddPoint(extend);

	extend = bbox.GetMin();
	extend[X] -= CCollObj::sLINE_BOX_EXTENT;
	extend[Y] -= CCollObj::sLINE_BOX_EXTENT;
	extend[Z] -= CCollObj::sLINE_BOX_EXTENT;
	line_bbox.AddPoint(extend);

	// Look for static collision
    CCollStatic* p_coll_obj;
	while((p_coll_obj = *p_coll_obj_list))
	{
		// TODO: Come up with a cleaner BBox check
		//Dbg_Assert(p_coll_obj->GetGeometry());
		//if(line_bbox.Intersect(p_coll_obj->GetGeometry()->GetBBox()))
		if (p_coll_obj->WithinBBox(line_bbox))
		{
			add_static_collision(p_coll_obj);
		}
		p_coll_obj_list++;
	}

	// Now look for movable collision
	Lst::Node< CCollObj > *p_movable_node = CMovableCollMan::sGetCollisionList()->GetNext();
	while(p_movable_node)
	{
		CCollObj *p_coll_obj = p_movable_node->GetData();
		if (p_coll_obj && !(p_coll_obj->m_Flags & (mSD_NON_COLLIDABLE | mSD_KILLED )))
		{
			if (p_coll_obj->WithinBBox(line_bbox))
			{
				add_movable_collision(p_coll_obj);
			}
		}
		p_movable_node = p_movable_node->GetNext();
	}

#if PRINT_TIMES
	uint64 end_time = Tmr::GetTimeInUSeconds();
	s_total_time += end_time - start_time;

	if (++s_num_collisions >= 1000)
	{
		Dbg_Message("Cache Update time %d us", s_total_time);
		s_total_time = s_num_collisions = 0;
	}
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollCache::add_static_collision(CCollStatic *p_collision)
{
	Dbg_Assert(p_collision);
	Dbg_MsgAssert(m_num_movable_coll == 0, ("Can't add static collision to cache after movable collision"));
	Dbg_MsgAssert(m_array_size < MAX_COLLISION_OBJECTS, ("Collision cache full."));

	m_collision_array[m_array_size].mp_bbox = &(p_collision->GetGeometry()->GetBBox());
	Dbg_MsgAssert(m_collision_array[m_array_size].mp_bbox, ("No bounding box found for the static collision"));

	m_collision_array[m_array_size++].mp_coll_obj = p_collision;
	
	m_num_static_coll++;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollCache::add_movable_collision(CCollObj *p_collision)
{
	Dbg_Assert(p_collision);
	Dbg_MsgAssert(m_array_size < MAX_COLLISION_OBJECTS, ("Collision cache full."));

	m_collision_array[m_array_size].mp_bbox = p_collision->get_bbox();
	//Dbg_MsgAssert(m_collision_array[m_array_size].mp_bbox, ("No bounding box found for the movable collision"));

	m_collision_array[m_array_size++].mp_coll_obj = p_collision;

	m_num_movable_coll++;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollCache::delete_static_collision(CCollStatic *p_collision)
{
	for (int i = 0; i < m_num_static_coll; i++)
	{
		if (m_collision_array[i].mp_coll_obj == p_collision)
		{
			// Must copy the whole block down
			for (int j = i + 1; j < m_array_size; j++)
			{
				m_collision_array[j - 1] = m_collision_array[j];
			}
			m_num_static_coll--;
			m_array_size--;
			break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollCache::delete_movable_collision(CCollObj *p_collision)
{
	for (int i = m_num_static_coll; i < m_array_size; i++)
	{
		if (m_collision_array[i].mp_coll_obj == p_collision)
		{
			m_collision_array[i] = m_collision_array[--m_array_size];
			m_num_movable_coll--;
			break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollCache::delete_collision(CCollObj *p_collision)
{
	delete_static_collision(static_cast<CCollStatic *>(p_collision));
	delete_movable_collision(p_collision);
}

////////////////////////////////////

CCollCache *	CCollCacheManager::sp_coll_cache_array[MAX_COLLISION_CACHES];
int				CCollCacheManager::s_num_coll_caches = 0;

bool			CCollCacheManager::s_assert_on_cache_miss;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollCache *	CCollCacheManager::sCreateCollCache()
{
	Dbg_MsgAssert(s_num_coll_caches < MAX_COLLISION_CACHES, ("Too many collision caches"));
	sp_coll_cache_array[s_num_coll_caches] = new CCollCache;

	return sp_coll_cache_array[s_num_coll_caches++];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CCollCacheManager::sDestroyCollCache(CCollCache *p_cache)
{
	bool found = false;

	// Take entry out of array
	for (int i = 0; i < s_num_coll_caches; i++)
	{
		if (sp_coll_cache_array[i] == p_cache)
		{
			// Move last in array to here
			sp_coll_cache_array[i] = sp_coll_cache_array[--s_num_coll_caches];

			found = true;
			break;
		}
	}

	// And do the actual deletion
	delete p_cache;

	Dbg_Assert(found);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CCollCacheManager::sDeleteMovableCollision(CCollObj *p_collision)
{
	// Check each cache
	for (int i = 0; i < s_num_coll_caches; i++)
	{
		sp_coll_cache_array[i]->delete_movable_collision(p_collision);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CCollCacheManager::sDeleteCollision(CCollObj *p_collision)
{
	// Check each cache
	for (int i = 0; i < s_num_coll_caches; i++)
	{
		sp_coll_cache_array[i]->delete_collision(p_collision);
	}
}

} // namespace Nx

