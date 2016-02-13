//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       ViewerObj.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  1/21/2002
//****************************************************************************

#ifndef __VIEWER_OBJ_H
#define __VIEWER_OBJ_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/support.h>

#include <gel/inpman.h>
#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif

#include <gel/net/net.h>

#include <sk/objects/movingobject.h>

/*****************************************************************************
**							Forward Declarations						**
*****************************************************************************/

namespace Net
{
	class Server;
};

namespace Nx
{
    class CModel;
};

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Obj
{
	class CSkeletonComponent;

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

// a viewer object is similar to a moving object,
// but without the physics.  a viewer object always
// exists in the viewer, although it doesn't necessarily
// have renderable model data associated with it

class CViewerObject : public CMovingObject
{
public:

enum ECommands
{
    mLOAD_SKATER        = nBit( 1 ),
	mLOAD_PEDESTRIAN	= nBit( 2 ),
    mGOTO_NEXT_ANIM		= nBit( 3 ),
    mGOTO_PREV_ANIM		= nBit( 4 ),
    mGOTO_FIRST_ANIM	= nBit( 5 ),
    mINCREASE_SPEED 	= nBit( 6 ),
    mDECREASE_SPEED     = nBit( 7 ),
    mINCREMENT_FRAME 	= nBit( 8 ),
    mDECREMENT_FRAME    = nBit( 9 ),
    mSHOW_PANEL    		= nBit( 10 ),
    mHIDE_PANEL    		= nBit( 11 ),
    mTOGGLE_PANEL    	= nBit( 12 ),
	mSTRAFE_UP   	    = nBit( 13 ),
	mSTRAFE_DOWN        = nBit( 14 ),
	mROLL_LEFT          = nBit( 15 ),
	mROLL_RIGHT         = nBit( 16 ),
	mTOGGLE_ROTATE_MODE = nBit( 17 ),
};

public:
    CViewerObject( Net::Server* pServer );
    virtual     ~CViewerObject();
	void		SetRotation( float rotZ, float rotX );
	void        LoadModel( Script::CStruct* pParams );
	void		UnloadModel();
	void		SetAnim( uint32 animName );
	void		ReloadAnim( const char* pAnim, uint32 animName );
    
protected:
	void		DoGameLogic();
    void        Update();
	void		UpdateWheels();
    
protected:
	void		ChangeAnimSpeed( float animSpeed );
	void		IncrementFrame( bool forwards );

protected:
	static 		Net::MsgHandlerCode		s_handle_set_anim;
	static 		Net::MsgHandlerCode		s_handle_set_anim_speed;
	static 		Net::MsgHandlerCode		s_handle_increment_frame;
	static		Net::MsgHandlerCode		s_handle_reload_anim;
	static		Net::MsgHandlerCode		s_handle_reload_cam_anim;
	static		Net::MsgHandlerCode		s_handle_sequence_preview;

    static		Inp::Handler< CViewerObject >::Code 	s_input_logic_code;

    Inp::Handler< CViewerObject >*	mp_input_handler;
    Tsk::Task< CViewerObject >*     mp_logic_task;

	Flags<ECommands>				m_commands;

protected:
	Net::Server*					mp_server;
    float       					m_animSpeed;
	bool							m_paused;
	bool							m_showPanel;
};

} // namespace Obj

#endif	// __VIEWER_OBJ_H

