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
**	Module:			Object (OBJ)											**
**																			**
**	File name:		objects/followob.h    									**
**																			**
**	Created: 		05/02/02	-	ksh										**
**																			**
*****************************************************************************/

#ifndef __OBJECTS_FOLLOWOB_H
#define __OBJECTS_FOLLOWOB_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __CORE_MATH_VECTOR_H
#include <core/math/vector.h>
#endif

//#include <sys/timer.h>

//#include <gfx/gfxman.h>

//#include <gfx/skin.h>

//#include <gel/inpman.h>
//#include <gel/object.h>

//#include <servers/os_coll.h>

//#include <core/math/matrix.h>
//#include <core/math/vector.h>

//#include <sk/objects/movingobject.h>

//#include <gel/scripting/script.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Obj
{

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

#define MAX_FOLLOWOB_PATH_POINTS 64
class  CFollowOb  : public Spt::Class
{
	Mth::Vector mp_path_points[MAX_FOLLOWOB_PATH_POINTS];
	int m_num_path_points;
	int m_most_recent_path_point;
	
	uint32 m_leader_name;
	Mth::Vector m_leader_pos;
	float m_distance;
	bool m_orient_y;
	
public:
	CFollowOb();
	~CFollowOb();
	
	void	Reset();
	void	SetLeaderName(uint32 name) {m_leader_name=name;}
	void	SetDistance(float distance) {m_distance=distance;}
	
	void	GetNewPathPointFromObjectBeingFollowed();
	void	CalculatePositionAndOrientation(Mth::Vector *p_pos, Mth::Matrix *p_orientation);
	void	OrientY() {m_orient_y=true;}
	void	DoNotOrientY() {m_orient_y=false;}
};

} // namespace Obj

#endif	// __OBJECTS_FOLLOWOB_H

