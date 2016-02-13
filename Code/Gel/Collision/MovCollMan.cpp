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
**	File name:		gel\collision\movcollman.cpp   							**
**																			**
**	Created by:		04/08/02	-	grj										**
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

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/



namespace Nx
{

Lst::Head<CCollObj> CMovableCollMan::s_collision_list;
//CCollObj * CMovableCollMan::s_collision_array[MAX_COLLISION_OBJECTS];
//int CMovableCollMan::s_array_size = 0;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CMovableCollMan::sAddCollision(CCollObj *p_collision, Obj::CCompositeObject *p_object)
{
	Lst::Node<CCollObj> *node = new Lst::Node<CCollObj>(p_collision);
	s_collision_list.AddToTail(node);
	//s_collision_array[s_array_size++] = p_collision;
	//Dbg_Assert(s_array_size <= MAX_COLLISION_OBJECTS);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CMovableCollMan::sRemoveCollision(CCollObj *p_collision)
{
	Lst::Node<Nx::CCollObj> *node_coll, *node_next;

	for(node_coll = s_collision_list.GetNext(); node_coll; node_coll = node_next)
	{
		node_next = node_coll->GetNext();

		if (node_coll->GetData() == p_collision)
		{
			delete node_coll;
		}
	}
}


} // namespace Nx

