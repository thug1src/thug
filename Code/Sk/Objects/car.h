/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Object (OBJ)											**
**																			**
**	File name:		objects/car.h	    									**
**																			**
**	Created: 		10/15/00	-	mjd										**
**																			**
*****************************************************************************/

#ifndef __OBJECTS_CAR_H
#define __OBJECTS_CAR_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

#include <sk/objects/movingobject.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Script
{
	class CScript;
	class CStruct;
}
				 
namespace Obj
{

	class CGeneralManager;

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

void CreateCar( CGeneralManager *p_obj_man, int nodeNum );
void CreateCar( CGeneralManager* p_obj_man, Script::CStruct* pNodeData);

class CCar : public CMovingObject 
{		

public:
					CCar ( void );
	virtual			~CCar ( void );
	void			InitCar( CGeneralManager* p_obj_man, Script::CStruct* pNodeData );
	bool			CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	
protected:
	void			update_wheels();
	void			display_wheels();
	void			debug_wheels();
	void			DoGameLogic();	
};

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

void CalculateCarHierarchyMatrices( Gfx::CSkeleton* pSkeleton,
                                    Nx::CModel *p_renderedModel, 
									float carRotationX, 
									float m_wheelRotationX, 
									float m_wheelRotationY);

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Obj

#endif	// __OBJECTS_CAR_H
