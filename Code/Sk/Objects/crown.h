/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		skate3													**
**																			**
**	Module:			Crown			 										**
**																			**
**	File name:		Crown.h													**
**																			**
**	Created by:		08/06/01	-	SPG										**
**																			**
**	Description:	King of the Hill crown object							**
**																			**
*****************************************************************************/

#ifndef __OBJECTS_CROWN_H
#define __OBJECTS_CROWN_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/macros.h>
#include <core/math.h>

#include <sk/objects/movingobject.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace Obj
{

class CSkater;

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class  CCrown : public CMovingObject 
{		
public:
	static const float vCROWN_RADIUS;

public:
	CCrown ( void );
	virtual	~CCrown ( void );
	
	void			InitCrown( CGeneralManager* p_obj_man, Script::CStruct* pNodeData );
	void			PlaceOnKing( CSkater* king );
	void			RemoveFromWorld( void );
	void			RemoveFromKing( void );
	Mth::Vector& 	GetPosition( void ) {return m_pos;}
	void			SetPosition( Mth::Vector& pos ) { m_pos = pos; }
	bool			OnKing( void );
	void			DoGameLogic();

protected:	
	CSkater* 		mp_king;
	uint32			m_head_bone_index;
	bool			m_on_king;
		
//	static	Tsk::Task< CCrown >::Code s_logic_code;
//	Tsk::Task< CCrown >*	mp_logic_task;
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

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj

#endif	// __OBJECTS_CROWN_H


