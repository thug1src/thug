//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       gameobj.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  11/02/2000
//****************************************************************************

#ifndef __OBJECTS_GAMEOBJ_H
#define __OBJECTS_GAMEOBJ_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

namespace Script
{
	class CStruct;
}

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Obj
{
	class CCompositeObject;
	class CGeneralManager;

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

void CreateParticleEmitter( CGeneralManager* p_obj_man, Script::CStruct* pNodeData );
CCompositeObject* CreateGameObj( CGeneralManager* p_obj_man, Script::CStruct* pNodeData );
void CreateLevelObj( CGeneralManager* p_obj_man, Script::CStruct* pNodeData );
void CreateParticleObject( CGeneralManager* p_obj_man, Script::CStruct* pNodeData );

} // namespace Obj

#endif	// __OBJECTS_GAMEOBJ_H
