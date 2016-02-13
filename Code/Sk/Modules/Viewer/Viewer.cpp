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
**	File name:		Viewer.cpp												**
**																			**
**	Created by:		11/28/01	-	SPG										**
**																			**
**	Description:	Viewer module		 									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <gel/inpman.h>
#include <gel/mainloop.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/object/compositeobjectmanager.h>


#include <gfx/nxviewman.h>

#include <modules/viewer/viewer.h>

#include <sk/objects/viewerobj.h>
#include <sk/objects/skater.h>			// just to see if the select button is pressed!!!

#include <gamenet/exportmsg.h>
#include <sk/gamenet/gamenet.h>

#include <scripting/gs_file.h>
#include <gel/scripting/utils.h>

#include <sys/config/config.h>
#include <sys/profiler.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

bool	run_runmenow = false;		// patch for signalling that the runnow script should be run

namespace Mdl
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

float	CViewer::s_movement_vel;	// in inches per sec
float	CViewer::s_rotate_vel;	// in degrees per sec
int		CViewer::s_view_mode=0;

// GJ:  I'm assuming that there's only going to be one CViewer at a time.
// I needed to be able to access this class from cfuncs.cpp, so I added
// this pointer.  Perhaps when Steve gets back from his surgery, we can
// decide a more appropriate interface between cfuncs.cpp and viewer.cpp.
CViewer* CViewer::sp_viewer = NULL;


/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

void        CViewer::s_shift_input_logic_code ( const Inp::Handler < CViewer >& handler )
{
	Dbg_AssertType ( &handler, Inp::Handler< CViewer > );

	CViewer&   mdl = handler.GetData();

	mdl.m_shift_commands.ClearAll();

	if( handler.m_Input->m_Buttons & Inp::Data::mD_SELECT )
	{
		mdl.m_shift_commands.SetMask( handler.m_Input->m_Makes );
		if (Script::GetInt(CRCD(0xf3e055e1,"select_shift")))
		{
			// only mask the buttons if we are using select as a shift button 
			handler.m_Input->MaskDigitalInput( Inp::Data::mD_ALL && ~Inp::Data::mD_LEFT && ~Inp::Data::mD_RIGHT && ~Inp::Data::mD_UP && ~Inp::Data::mD_DOWN);
			handler.m_Input->MaskAnalogInput( Inp::Data::mA_ALL && ~Inp::Data::mA_LEFT && ~Inp::Data::mA_RIGHT && ~Inp::Data::mA_UP && ~Inp::Data::mA_DOWN && ~Inp::Data::mA_LEFT_X && ~Inp::Data::mA_LEFT_Y && ~Inp::Data::mA_RIGHT_X && ~Inp::Data::mA_RIGHT_Y );
		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CViewer::s_input_logic_code ( const Inp::Handler < CViewer >& handler )
{       
	CViewer&	mdl = handler.GetData();

//	printf ("s_view_mode = %d   rightx = %d\n",s_view_mode,mdl.m_right[X]);

// just return if we are not in viewer mode
	if (!s_view_mode)
	{
		return;
	}
							  
	// GJ:  no longer clearing the last frame's data
	// at the beginning of the loop...  this allows
	// the viewer object to add additional m_commands
	// to the viewer (useful for customizing controller
	// configs in different viewer modes)
//	mdl.m_commands.ClearAll();

	// translate m_Input data to module-specific commands
	mdl.m_right[X] = handler.m_Input->m_Event[Inp::Data::vA_RIGHT_X_UNCLAMPED] - 128;
	mdl.m_right[Y] = handler.m_Input->m_Event[Inp::Data::vA_RIGHT_Y_UNCLAMPED] - 128;
	mdl.m_left[X] = handler.m_Input->m_Event[Inp::Data::vA_LEFT_X_UNCLAMPED] - 128;
	mdl.m_left[Y] = handler.m_Input->m_Event[Inp::Data::vA_LEFT_Y_UNCLAMPED] - 128;


	if (Script::GetInt(CRCD(0x89bef647,"viewer_buttons_enabled")))
	{
		if( ( handler.m_Input->m_Buttons & Inp::Data::mD_R1 ) &&
			( handler.m_Input->m_Makes & Inp::Data::mD_R2 ))
		{
			if (!Config::CD())
			{
	//  Mick - took out viewport changing for non-PS2, as it messes up X-Box artists, with only one shoulder button
				#ifdef	__PLAT_NGPS__
				mdl.m_commands.SetMask( mCHANGE_SCREEN_MODE );
				#endif
			}	
		}
		else if( handler.m_Input->m_Buttons & Inp::Data::mD_R1 )
		{
			if( !( handler.m_Input->m_Buttons & Inp::Data::mD_R2 ))
			{
				mdl.m_commands.SetMask( mSTRAFE_UP );
			}
		}
		else if( handler.m_Input->m_Buttons & Inp::Data::mD_R2 )
		{
			mdl.m_commands.SetMask( mSTRAFE_DOWN );
		}
		
	//	if (!Config::CD())
		{
			if( handler.m_Input->m_Makes & Inp::Data::mD_START )
			{
				mdl.m_commands.SetMask( mRESET );
			}
			if( mdl.m_rotation_mode == vROTATE_CAMERA )
			{   
				if( handler.m_Input->m_Makes & Inp::Data::mD_TRIANGLE )
				{
					mdl.m_commands.SetMask( mVIEW_TOP );
				}
				if( handler.m_Input->m_Makes & Inp::Data::mD_SQUARE )
				{
					mdl.m_commands.SetMask( mVIEW_LEFT );
				}
				if( handler.m_Input->m_Makes & Inp::Data::mD_CIRCLE )
				{
					mdl.m_commands.SetMask( mVIEW_RIGHT );
				}
				if( handler.m_Input->m_Makes & Inp::Data::mD_X )
				{
					mdl.m_commands.SetMask( mVIEW_FRONT );
				}
			}
			else if( mdl.m_rotation_mode == vROTATE_OBJECTS )
			{
				if( handler.m_Input->m_Makes & Inp::Data::mD_X )
				{
					mdl.m_commands.SetMask( mRESET_ROTATIONS );
				}
			}
			if( handler.m_Input->m_Buttons & Inp::Data::mD_UP )
			{
				if ( handler.m_Input->m_Makes & Inp::Data::mD_UP )
				{
					mdl.m_commands.SetMask( mLIGHT_FW );
				}
			}
			if( handler.m_Input->m_Buttons & Inp::Data::mD_DOWN )
			{
				if ( handler.m_Input->m_Makes & Inp::Data::mD_DOWN )
				{
					mdl.m_commands.SetMask( mLIGHT_BK );
				}
			}
			if( handler.m_Input->m_Buttons & Inp::Data::mD_RIGHT )
			{
				mdl.m_commands.SetMask( mLIGHT_RT );
			}
			if( handler.m_Input->m_Buttons & Inp::Data::mD_LEFT )
			{
				mdl.m_commands.SetMask( mLIGHT_LT );
			}
			if( handler.m_Input->m_Makes & Inp::Data::mD_L3 )
			{
				mdl.m_commands.SetMask( mTOGGLE_ROTATE_MODE );
			}
			if ( handler.m_Input->m_Buttons & Inp::Data::mD_L1 )
			{
				if ( !( handler.m_Input->m_Buttons & Inp::Data::mD_L2 ))
				{
					mdl.m_commands.SetMask( mROLL_LEFT );
				}
			}
			else if ( handler.m_Input->m_Buttons & Inp::Data::mD_L2 )
			{
				mdl.m_commands.SetMask( mROLL_RIGHT );
			}
	
			
			if ( ! (handler.m_Input->m_Buttons & Inp::Data::mD_SELECT) )
			{
				// Patch on buttonscript calls to center the camera in various ways when in a view mode
				if ( handler.m_Input->m_Makes & Inp::Data::mD_SQUARE )
				{
					Script::RunScript("UserViewerSquare");
				}
				
				if ( handler.m_Input->m_Makes & Inp::Data::mD_CIRCLE )
				{
					Script::RunScript("UserViewerCIRCLE");
				}
				
				if ( handler.m_Input->m_Makes & Inp::Data::mD_TRIANGLE )
				{
					Script::RunScript("UserViewerTRIANGLE");
				}
				
				if ( handler.m_Input->m_Makes & Inp::Data::mD_X )
				{
					Script::RunScript("UserViewerX");
				}
			}		
			
		}	
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CViewer::s_logic_code ( const Tsk::Task< CViewer >& task )
{
	CViewer&	mdl = task.GetData();

	Dbg_AssertType ( &task, Tsk::Task< CViewer > );

	//static const    float   ldelta = 1.0f;
	//static const    float   lcolordelta = .0125f;
	static const    float   scale = ( 1.0f / ( 128.0f - Inp::vANALOGUE_TOL ));

//    Gfx::Camera* pCam = Nx::CViewportManager::sGetCamera( 0 );
//	if ( !pCam )
//	{
//		return;
//	}

	float elapsed_seconds;
	Tmr::Time elapsed_time;
	static Tmr::Time last_time = 0;

	// store the old camera pos before updating anything...
//	pCam->StoreOldPos( );

	elapsed_time = Tmr::ElapsedTime( last_time );
	elapsed_seconds = (int)elapsed_time / 1000.0f;
	last_time = last_time + elapsed_time;   

	if ( mdl.m_commands.TestMask( mTOGGLE_ROTATE_MODE ))
	{
		// Mick: Don't switch to rotating the camera around the object unless there is an object
		if ( mdl.mp_viewerObject && mdl.m_rotation_mode == vROTATE_CAMERA )
		{
			mdl.m_rotation_mode = vROTATE_OBJECTS;
		}
		else
		{
			mdl.m_rotation_mode = vROTATE_CAMERA;
		}      
	}

	mdl.m_right[Y] =
	( mdl.m_right[Y] >  Inp::vANALOGUE_TOL ) ? ( mdl.m_right[Y] - Inp::vANALOGUE_TOL ) * scale :
	( mdl.m_right[Y] < -Inp::vANALOGUE_TOL ) ? ( mdl.m_right[Y] + Inp::vANALOGUE_TOL ) * scale : 0.0f;
	mdl.m_right[X] =
	( mdl.m_right[X] >  Inp::vANALOGUE_TOL ) ? ( mdl.m_right[X] - Inp::vANALOGUE_TOL ) * scale :
	( mdl.m_right[X] < -Inp::vANALOGUE_TOL ) ? ( mdl.m_right[X] + Inp::vANALOGUE_TOL ) * scale : 0.0f;

	mdl.m_left[Y] =
	( mdl.m_left[Y] >  Inp::vANALOGUE_TOL ) ? ( mdl.m_left[Y] - Inp::vANALOGUE_TOL ) * scale :
	( mdl.m_left[Y] < -Inp::vANALOGUE_TOL ) ? ( mdl.m_left[Y] + Inp::vANALOGUE_TOL ) * scale : 0.0f;
	mdl.m_left[X] =
	( mdl.m_left[X] >  Inp::vANALOGUE_TOL ) ? ( mdl.m_left[X] - Inp::vANALOGUE_TOL ) * scale :
	( mdl.m_left[X] < -Inp::vANALOGUE_TOL ) ? ( mdl.m_left[X] + Inp::vANALOGUE_TOL ) * scale : 0.0f;

	Obj::CCompositeObject * p_obj = (Obj::CCompositeObject *) Obj::CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0xeb17151b,"viewer_cam"));
	if (p_obj)
	{
		if ( mdl.m_rotation_mode == vROTATE_CAMERA )
		{
			if (mdl.m_left[Y])
			{
				p_obj->GetMatrix().RotateLocal( Mth::Vector( 1, 0, 0 ),
							mdl.m_left[Y] * elapsed_seconds * mdl.s_rotate_vel * Mth::PI / 180.0f );
			}
			if (mdl.m_left[X])
			{
				// this rotation is *NOT* local, as the designers seem to prefer it this way.
				Mth::Matrix mat = p_obj->GetMatrix();
				mat.RotateY( -mdl.m_left[X] * elapsed_seconds * mdl.s_rotate_vel * Mth::PI / 180.0f );
				p_obj->SetMatrix( mat );
			}
		}
		else if ( mdl.m_rotation_mode == vROTATE_OBJECTS )
		{
			Dbg_Assert( mdl.mp_viewerObject );
			mdl.mp_viewerObject->SetRotation( mdl.m_left[X], mdl.m_left[Y] );
		}


	
		// Move with right analog stick:
		if (mdl.m_right[X] || mdl.m_right[Y])
		{
			Mth::Vector transVec;
			transVec[X] = mdl.m_right[X] * ( elapsed_seconds * mdl.s_movement_vel );
			transVec[Y] = 0.0f;
			transVec[Z] = mdl.m_right[Y] * ( elapsed_seconds * mdl.s_movement_vel );
			transVec[W] = 1.0f;
			s_translate_local( p_obj, transVec );
		}
	}

	if ( mdl.m_commands.TestMask( mSTRAFE_UP ))
	{
		s_translate_local( p_obj, Mth::Vector( 0, elapsed_seconds * mdl.s_movement_vel, 0 ) );
	}
	if ( mdl.m_commands.TestMask( mSTRAFE_DOWN ))
	{
		s_translate_local( p_obj, Mth::Vector( 0, -elapsed_seconds * mdl.s_movement_vel, 0 ) );
	}

	if ( !Script::GetInt( CRCD(0xd8c91578,"DisableViewerRoll"), false ) )
	{
		if ( mdl.m_commands.TestMask( mROLL_LEFT ))
		{
			p_obj->GetMatrix().RotateLocal( Mth::Vector( 0, 0, 1 ),
									  -0.5f * elapsed_seconds * mdl.s_rotate_vel * Mth::PI / 180.0f );
		}
		if ( mdl.m_commands.TestMask( mROLL_RIGHT ))
		{
			p_obj->GetMatrix().RotateLocal( Mth::Vector( 0, 0, 1 ),
									  0.5f * elapsed_seconds * mdl.s_rotate_vel * Mth::PI / 180.0f );
		}
	}
	
	p_obj->Pause(false);		// dang, it's suspended due to game being paused, so need to update manually...
							// THIS IS A PATCH - needs to be more explicity handled.....
	
	
	if ( mdl.m_commands.TestMask( mCHANGE_SCREEN_MODE ))
	{
		Nx::CViewportManager::sSetScreenMode ( (Nx::ScreenMode) (( (int) Nx::CViewportManager::sGetScreenMode() + 1 ) % Nx::vNUM_SCREEN_MODES) );
	}

	// GJ:  clear the frame's data, now that we're done with it
	// (this used to be done at the beginning of the loop)
	mdl.m_commands.ClearAll();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CViewer::s_translate_local( Obj::CCompositeObject* p_obj, const Mth::Vector& vec )
{
	// need to build a temporary matrix,
	// because the rotation and translation
	// are stored separately in the camera
	Mth::Matrix mat = p_obj->GetMatrix();
	mat[Mth::POS] = p_obj->GetPos();
	
	mat.TranslateLocal( vec );
	
	p_obj->SetPos(mat[Mth::POS]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CScript *spawn_if_exists(uint32 script)
{
	if (Script::ScriptExists(script))
	{
		return  Script::SpawnScript(script);
	}
	return NULL;
}

void        CViewer::s_shift_logic_code ( const Tsk::Task< CViewer >& task )
{
	Dbg_AssertType ( &task, Tsk::Task< CViewer > );

	CViewer&   mdl = task.GetData();

	if (!Script::GetInt(CRCD(0xf3e055e1,"select_shift")))
	{
//		if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_SELECT ))
//		{
//			Script::RunScript(CRCD(0x60871393,"UserSelectSelect"));
//		}
	}
	else
	{
		Script::CScript *p_script=NULL;
		if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_TRIANGLE ))
		{
			p_script=spawn_if_exists(CRCD(0xcbb91022,"UserSelectTriangle"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_SQUARE ))
		{
			p_script=spawn_if_exists(CRCD(0xe69691fa,"UserSelectSquare"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_CIRCLE ))
		{
			// (launches the game)
			p_script=spawn_if_exists(CRCD(0xffc29c2a,"UserSelectCircle"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_START ))
		{
			p_script=spawn_if_exists(CRCD(0x51b0e413,"UserSelectStart"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_X ))
		{
			p_script=spawn_if_exists(CRCD(0x746d2598,"UserSelectX"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_L1 ))
		{
			p_script=spawn_if_exists(CRCD(0x81d0a13c,"UserSelectL1"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_L2 ))
		{
			p_script=spawn_if_exists(CRCD(0x18d9f086,"UserSelectL2"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_R1 ))
		{
			p_script=spawn_if_exists(CRCD(0x55919ee3,"UserSelectR1"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_R2 ))
		{
			p_script=spawn_if_exists(CRCD(0xcc98cf59,"UserSelectR2"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_UP ))
		{
			p_script=spawn_if_exists(CRCD(0x1b0b7922,"UserSelectUp"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_DOWN ))
		{
			p_script=spawn_if_exists(CRCD(0xbbc0e36a,"UserSelectDown"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_LEFT ))
		{
			p_script=spawn_if_exists(CRCD(0xdd589439,"UserSelectLeft"));
		}
		else if ( mdl.m_shift_commands.TestMask( Inp::Data::mD_RIGHT ))
		{
			p_script=spawn_if_exists(CRCD(0x7a03c488,"UserSelectRight"));
		}
		   
		if (p_script)
		{
			#ifdef __NOPT_ASSERT__
			p_script->SetCommentString("Spawned from CViewer::s_shift_logic_code()");
			#endif
		}	
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewer::s_handle_quickview( Net::MsgHandlerContext* context )
{
	Net::MsgQuickview* p_msg;
	Script::CStruct* pStructure;

	p_msg = (Net::MsgQuickview*) context->m_Msg;

	Dbg_Printf( "Got quickview request to load file %s....\n", p_msg->m_Filename );

    pStructure = new Script::CStruct;

	pStructure->AddComponent( Script::GenerateCRC( "scene" ), ESYMBOLTYPE_STRING, p_msg->m_Filename );

	Script::RunScript( "ReloadScene", pStructure );

	delete pStructure;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewer::s_handle_incremental_update( Net::MsgHandlerContext* context )
{
	Net::MsgQuickview* p_msg;
	Script::CStruct* pStructure;

	p_msg = (Net::MsgQuickview*) context->m_Msg;

	Dbg_Printf( "Got quickview request to load file %s....\n", p_msg->m_Filename );

    pStructure = new Script::CStruct;

	pStructure->AddComponent( Script::GenerateCRC( "scene" ), ESYMBOLTYPE_STRING, p_msg->m_Filename );

	pStructure->AddComponent( Script::GenerateCRC( "add" ), ESYMBOLTYPE_STRING, p_msg->m_UpdateFilename );
	Script::RunScript( "AddToScene", pStructure );

	delete pStructure;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewer::s_handle_update_material( Net::MsgHandlerContext* context )
{
	Dbg_Printf( "Got material update request....\n" );

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewer::s_handle_rq( Net::MsgHandlerContext* context )
{
	char* path = context->m_Msg;

	if ((strcmp("..\\runnow.qb",path) == 0) || (strcmp("runnow.qb",path) == 0))
	{
		run_runmenow = true;
	}
	else
	{
		Dbg_Printf( "******* GOT PATH %s ********\n", path );
	}
	SkateScript::LoadQB( path );
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewer::s_handle_load_model( Net::MsgHandlerContext* context )
{        
	Dbg_Message( "Got request to load model/anim...\n" );
	
	Mdl::CViewer* pViewer = (Mdl::CViewer*) context->m_Msg;
	Dbg_Assert( pViewer );
	
	Script::CStruct* pStruct = new Script::CStruct;

	Net::MsgViewObjLoadModel* p_msg;
	p_msg = (Net::MsgViewObjLoadModel*) context->m_Msg;
	pStruct->AddString( CRCD(0xb974f2fc,"modelName"), p_msg->m_ModelName );
	pStruct->AddChecksum( CRCD(0x6c2bfb7f,"animName"), p_msg->m_AnimScriptName );
	pStruct->AddChecksum( CRCD(0x09794932,"skeletonName"), p_msg->m_SkeletonName );
	pViewer->AddViewerObject( pStruct );
	
	delete pStruct;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
		 
int CViewer::s_handle_unload_model( Net::MsgHandlerContext* context )
{
	Dbg_Message( "Got request to unload model/anim...\n" );

	Mdl::CViewer* pViewer = (Mdl::CViewer*) context->m_Msg;
	Dbg_Assert( pViewer );
	pViewer->RemoveViewerObject();

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewer::s_handle_run_script_command( Net::MsgHandlerContext* context )
{
	uint32 *p_data=(uint32*)context->m_Msg;
	
	uint32 command_name=*p_data++;
	
	Script::CStruct *p_params=new Script::CStruct;
	Script::ReadFromBuffer(p_params,(uint8*)p_data);
//	Dbg_Printf( "Running script command %p\n", command_name );
	Script::RunScript(command_name,p_params);
	delete p_params;
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewer::AddViewerObject( Script::CStruct* pParams )
{
	if (Config::GotExtraMemory())
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
	}
	else
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	}
		
	// if the viewer object exists already,
	// clear it out
	if ( mp_viewerObject )
	{
		RemoveViewerObject();
	}

	Dbg_Assert( !mp_viewerObject );
	mp_viewerObject = new Obj::CViewerObject( mp_server );
	mp_viewerObject->LoadModel( pParams );
	
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewer::RemoveViewerObject()
{
	if ( mp_viewerObject )
	{
		mp_viewerObject->UnloadModel();
		delete mp_viewerObject;
		mp_viewerObject = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CViewer::v_start_cb ( void )
{
	Inp::Manager* inp_manager = Inp::Manager::Instance();
	Mlp::Manager* mlp_manager = Mlp::Manager::Instance();

	#ifdef		__USE_PROFILER__	
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ProfilerHeap());
	#else
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
	#endif

	// Sorry Steve, but we need the shift input handler on the CD, so I'm going
	// to have to start this module all the time.
	// but on the CD, we don't initialize any of the actual view stuff
	if (!Config::CD())
	{
		
		
	
		mp_server->m_Dispatcher.AddHandler( Net::vMSG_ID_QUICKVIEW, 
											s_handle_quickview, 
											Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN );
		mp_server->m_Dispatcher.AddHandler(	Net::vMSG_ID_UPDATE_MATERIAL, 
											s_handle_update_material, 
											Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN );
		mp_server->m_Dispatcher.AddHandler(	Net::vMSG_ID_REMOTE_Q, 
											s_handle_rq, 
											Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN );
		mp_server->m_Dispatcher.AddHandler( Net::vMSG_ID_INCREMENTAL_UPDATE,
											s_handle_incremental_update,
											Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN );
		mp_server->m_Dispatcher.AddHandler( Net::vMSG_ID_VIEWOBJ_LOAD_MODEL, 
											s_handle_load_model,
											Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this );
		mp_server->m_Dispatcher.AddHandler( Net::vMSG_ID_VIEWOBJ_UNLOAD_MODEL, 
											s_handle_unload_model,
											Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this );
		mp_server->m_Dispatcher.AddHandler(	Net::vMSG_ID_RUN_SCRIPT_COMMAND, 
											s_handle_run_script_command, 
											Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this );
	
	
	
		Dbg_Printf( "Starting Viewer Module....\n" );
	}	

	// We also add the regular input handler and logic handler on CD
	mlp_manager->AddLogicTask( *mp_logic_task );
	inp_manager->AddHandler( *mp_input_handler );
	
	// The "shift_input" handler and logic are just for the user-select buttons
	// specifically the screenshot (Select+Square)
	// generally they are disabled. 
	// see buttonscripts.q for the scripts that get run

	printf("Adding shift input handlers\n)");	
	inp_manager->AddHandler( *mp_shift_input_handler );
	mlp_manager->AddLogicTask( *mp_shift_logic_task );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CViewer::v_stop_cb ( void )
{   
	if ( mp_viewerObject )
	{
    	delete mp_viewerObject;
		mp_viewerObject = NULL;
	}
    
	mp_input_handler->Remove();
	mp_logic_task->Remove();
	mp_shift_logic_task->Remove();
	mp_shift_input_handler->Remove();

	mp_server->m_Dispatcher.Deinit();
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

CViewer::CViewer( void )
{
	if (!Config::CD())
	{
		uint32 local_ip;

		Net::Manager * net_man = Net::Manager::Instance();
			
#ifdef __PLAT_NGC__
		SOInAddr local_addr;
		SOInetAtoN( net_man->GetLocalIP(), &local_addr );
		local_ip = local_addr.addr;
#else
		local_ip = inet_addr( net_man->GetLocalIP());
#endif
		net_man->NetworkEnvironmentSetup();
		mp_server = net_man->CreateNewAppServer( 0, "Skate4 Viewer", 4, Net::vEXPORT_COMM_PORT,
													inet_addr( net_man->GetLocalIP()), Net::App::mACCEPT_FOREIGN_CONN );
	
		Dbg_Assert( mp_server );
	}	


	int viewer_controller = Script::GetInt(CRCD(0x702247a5,"Viewer_controller"), false);

	mp_logic_task = new Tsk::Task< CViewer > ( CViewer::s_logic_code, *this );
	mp_input_handler = new Inp::Handler< CViewer > ( viewer_controller,  s_input_logic_code, *this, Tsk::BaseTask::Node::vNORMAL_PRIORITY );
	
	s_movement_vel = 400.0f;	// default movement velocity in inches per second
	s_rotate_vel = 120.0f;		// 120 degrees per second. In 3 seconds you should complete 1 revolution

	printf("Creating shift input handlers\n)");	
	mp_shift_logic_task = new Tsk::Task< CViewer > ( CViewer::s_shift_logic_code, *this );
	mp_shift_input_handler = new Inp::Handler< CViewer > ( viewer_controller,  CViewer::s_shift_input_logic_code, *this, Tsk::BaseTask::Node::vHANDLER_PRIORITY_VIEWER_SHIFT_INPUT_LOGIC );

	Dbg_Assert( !sp_viewer );
	sp_viewer = this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CViewer::~CViewer( void )
{
	Dbg_Assert( mp_server );

	delete mp_server;
	delete mp_input_handler;
	delete mp_logic_task;
	delete mp_shift_logic_task;
	delete mp_shift_input_handler;

	Dbg_Assert( sp_viewer );
	sp_viewer = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CViewerObject* CViewer::GetViewerObject()
{
	return mp_viewerObject;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CViewer::sSetViewMode(int viewMode)
{
	s_view_mode = viewMode;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void    CViewer::sSetMovementVelocity( float in_per_sec )
{
	s_movement_vel = in_per_sec;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				 
void    CViewer::sSetRotateVelocity( float deg_per_sec )
{
	s_rotate_vel = deg_per_sec;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				 
bool	CViewer::ResetCameraToViewerObject()
{
	if ( !Script::GetInteger( CRCD(0xc965e06f,"ResetCameraToViewerObject"), Script::NO_ASSERT ) )
	{
		return true;
	}

//	Gfx::Camera* pCamera = Nx::CViewportManager::sGetCamera(0);
	Obj::CCompositeObject * p_obj = (Obj::CCompositeObject *) Obj::CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0xeb17151b,"viewer_cam"));

	if ( p_obj )
	{
		// ideally we would want to hide the level/sky when
		// switching to the object viewer, but for now,
		// just put the viewer object in the sky...

		// set the camera
		Mth::Matrix ident;
		ident.Ident();
		p_obj->SetMatrix( ident );

		// the viewer object gets placed at (0, 1000, 0),
		// so put the camera slightly offset from here
		p_obj->SetPos( Mth::Vector(0.0f, 35.0f, 100.0f) );
	}

	return true;
}			

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				 
CViewer* CViewer::sGetViewer()
{
	return sp_viewer;
}
							  
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				 
void CViewer::SetCommand( const uint32 command )
{
	m_commands.SetMask( command );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				 
} // namespace Mdl

