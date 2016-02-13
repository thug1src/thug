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
**	File name:		MovCollMan.h											**
**																			**
**	Created: 		04/08/2002	-	grj										**
**																			**
*****************************************************************************/

#ifndef	__GEL_MOVCOLLMAN_H
#define	__GEL_MOVCOLLMAN_H

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

namespace Nx
{

class CCollObj;
class CCollObjTriData;

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class CMovableCollMan
{
public:
	//static void				sInit();
	//static void				sCleanup();

	static void				sAddCollision(CCollObj *p_collsion, Obj::CCompositeObject *p_object);
	static void				sRemoveCollision(CCollObj *p_collsion);

	static Lst::Head<CCollObj> *sGetCollisionList();
	static CCollObj **			sGetCollisionArray();

protected:
	enum
	{
		MAX_COLLISION_OBJECTS = 100,
	};

	static Lst::Head<CCollObj>	s_collision_list;
	static CCollObj *			s_collision_array[MAX_COLLISION_OBJECTS];		// If the speed is needed
	static int					s_array_size;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Lst::Head<CCollObj> *CMovableCollMan::sGetCollisionList()
{
	return &s_collision_list;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline CCollObj **			CMovableCollMan::sGetCollisionArray()
{
	return s_collision_array;
}

} // namespace Nx

#endif	//	__GEL_MOVCOLLMAN_H
