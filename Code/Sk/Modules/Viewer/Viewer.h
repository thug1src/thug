/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2001 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate4													**
**																			**
**	Module:			Viewer			 										**
**																			**
**	File name:		Viewer.h												**
**																			**
**	Created by:		11/28/01	-	SPG										**
**																			**
**	Description:	Viewer module		 									**
**																			**
*****************************************************************************/

#ifndef __SK_MODULES_VIEWER_H__
#define __SK_MODULES_VIEWER_H__

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/math.h>

#include <gel/module.h>
#include <gel/inpman.h>
#include <gel/net/net.h>
#include <gel/net/server/netserv.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Obj
{
    class CViewerObject;
    class CCompositeObject;
};

namespace Script
{
	class CStruct;
};
			  
namespace Mdl
{
/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class CViewer : public Module
{
public:
	CViewer( void );
	~CViewer( void );

	static int	sGetViewMode() {return s_view_mode; }
	static void	sSetViewMode(int viewMode);
	static void	sSetMovementVelocity( float in_per_sec );
	static void	sSetRotateVelocity( float deg_per_sec );
	
	// GJ:  viewer mode stuff...
	// there's probably a better way to do this...
	static CViewer*		sGetViewer();
	bool				ResetCameraToViewerObject();
	void				AddViewerObject( Script::CStruct* pParams );
	void				RemoveViewerObject();
	Obj::CViewerObject*	GetViewerObject();
	static CViewer*		sp_viewer;
	void				SetCommand( const uint32 command );

	enum ECommands
	{
		mSTRAFE_UP          = nBit( 1 ),
		mSTRAFE_DOWN        = nBit( 2 ),
		mLIGHT_BK			= nBit( 3 ),
		mLIGHT_FW			= nBit( 4 ),
		mLIGHT_LT			= nBit( 5 ),
		mLIGHT_RT			= nBit( 6 ),
		mVIEW_FRONT         = nBit( 7 ),
		mVIEW_LEFT          = nBit( 8 ),
		mVIEW_RIGHT         = nBit( 9 ),
		mVIEW_TOP           = nBit( 10 ),
		mRESET              = nBit( 11 ),
		mCHANGE_SCREEN_MODE = nBit( 12 ),
		mTOGGLE_RENDER_MODE = nBit( 13 ),
		mTOGGLE_ROTATE_MODE	= nBit( 14 ),
		mRESET_ROTATIONS	= nBit( 15 ),
		mROLL_LEFT          = nBit( 16 ),
		mROLL_RIGHT         = nBit( 17 ),
	};

	enum ERotationMode
	{
		vROTATE_CAMERA,
		vROTATE_OBJECTS
	};

private:
	void			v_start_cb ( void );
	void			v_stop_cb ( void );
	
	static 			Net::MsgHandlerCode				s_handle_quickview;
	static 			Net::MsgHandlerCode				s_handle_incremental_update;
	static 			Net::MsgHandlerCode				s_handle_update_material;
	static 			Net::MsgHandlerCode				s_handle_rq;
	static 			Net::MsgHandlerCode				s_handle_load_model;
	static 			Net::MsgHandlerCode				s_handle_unload_model;
	static 			Net::MsgHandlerCode				s_handle_run_script_command;
	
	static			Tsk::Task< CViewer >::Code		s_logic_code;       
	static			Tsk::Task< CViewer >::Code		s_shift_logic_code;       
	
	static			Inp::Handler< CViewer >::Code	s_input_logic_code;
	static			Inp::Handler< CViewer >::Code	s_shift_input_logic_code;
	static			Inp::Handler< CViewer >::Code	s_shift_input_logic_code2;
	
	static 			void							s_translate_local( Obj::CCompositeObject* p_obj, const Mth::Vector& vec );
	

	Inp::Handler< CViewer >*	mp_input_handler;
	Inp::Handler< CViewer >*	mp_shift_input_handler;      
	Tsk::Task< CViewer >*		mp_logic_task;
	Tsk::Task< CViewer >*		mp_shift_logic_task;	

	// Input
    Mth::Vector		m_right;
    Mth::Vector		m_left;
    Flags<ECommands>m_commands;
	Flags<uint>     m_shift_commands;
	ERotationMode	m_rotation_mode;
	static float	s_movement_vel;	// in inches per sec
	static float	s_rotate_vel;	// in degrees per sec
	static int		s_view_mode;	// 0 if inactive
	
	Net::Server*	mp_server;

    Obj::CViewerObject*    mp_viewerObject;
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

CViewer* GetViewer();

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

} // namespace Mdl

#endif	// __SK_MODULES_VIEWER_H__



