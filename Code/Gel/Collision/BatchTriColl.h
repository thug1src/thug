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
**	File name:		BatchTriColl.h											**
**																			**
**	Created: 		04/12/2002	-	grj										**
**																			**
*****************************************************************************/

#ifndef	__GEL_BATCHTRICOLL_H
#define	__GEL_BATCHTRICOLL_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/math.h>
#include <core/math/geometry.h>

#include <gel/collision/collenums.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#ifdef __PLAT_NGPS__
//#define BATCH_TRI_COLLISION
#endif

namespace Nx
{

#ifdef BATCH_TRI_COLLISION

class CCollObj;
class CollData;

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class CBatchTriColl
{
public:
	Mth::Line					m_test_line;
	Mth::Vector					m_test_line_dir;
	CCollObj *					mp_collision_obj;
	float						m_distance;
	FaceIndex					m_face_index;

	static CollData *			sp_coll_data;			// We should only have one at a time
};

// Results from collision
struct SCollOutput
{
	float distance;
	int pad1;
	int pad2;
	int index;
};

class CBatchTriCollMan
{
public:
	static bool					sInit(CollData *p_coll_data);
	static void					sFinish();

	static void					sAddTriCollision(const Mth::Line &test_line, const Mth::Vector &test_line_dir,
												 CCollObj *p_collision_obj, FaceIndex face_index);
	static bool					sRemoveTriCollision(CBatchTriColl *p_tri_collsion);

	static void					sStartNewTriCollisions();
	static volatile bool		sIsTriCollisionDone();
	static volatile bool		sIsNested();
	static bool					sWaitTriCollisions();

#ifdef __PLAT_NGPS__
	static bool					sUseVU0Micro();
	static void					sDisableVU0Micro();
#endif

protected:
	enum
	{
		MAX_BATCH_COLLISIONS = 40,
	};

	// Collision callback
	static int					s_collision_done(int);
	static void					s_process_results();

	static void					s_switch_buffers();

	static volatile bool		s_processing;
	static volatile bool		s_result_processing;
	static volatile int			s_nested;				// Indicates nesting level, so normal collision must be used
	static volatile bool		s_found_collision;

	static CBatchTriColl		s_tri_collision_array[MAX_BATCH_COLLISIONS][2];		// Double buffered
	static int					s_current_array;
	static int					s_array_size;
	static int					s_next_idx_to_batch;

	static int					s_num_collisions;
	static SCollOutput			s_collision_results[MAX_BATCH_COLLISIONS];

private:
#ifdef __PLAT_NGPS__
	static int 					s_collision_handler_id;// for interrupt callback
	static bool					s_use_vu0_micro;
#endif
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline volatile bool			CBatchTriCollMan::sIsTriCollisionDone()
{
	return !s_processing;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline volatile bool			CBatchTriCollMan::sIsNested()
{
	return s_nested > 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void						CBatchTriCollMan::s_switch_buffers()
{
	s_array_size = 0;
	s_next_idx_to_batch = 0;
	s_current_array = s_current_array ^ 1;
}

#endif // BATCH_TRI_COLLISION

} // namespace Nx

#endif	//	__GEL_BATCHTRICOLL_H
