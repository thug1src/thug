//****************************************************************************
//* MODULE:         Sk/Scripting
//* FILENAME:       CFuncs.cpp
//* OWNER:          Kendall Harrison
//* CREATION DATE:  9/14/2000
//****************************************************************************

// start autoduck documentation
// @DOC cfuncs
// @module cfuncs | None
// @subindex Scripting Database
// @index script | cfuncs

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/scripting/cfuncs.h>
#include <sk/scripting/skfuncs.h>

#include <sk/objects/pathman.h>
//#include <libscf.h>

#include <core/defines.h>
#include <core/math.h>
#include <core/thread.h>
#include <core/singleton.h>
#include <core/macros.h>
#include <core/String/stringutils.h>

#include <sys/demo.h>
#include <sys/timer.h>
#include <sys/File/PRE.h>
#include <sys/file/filesys.h>
#include <sys/File/pip.h>
#include <sys/mem/memman.h>
#include <sys/mem/memtest.h>
#include <sys/sioman.h>
#include <sys/sio/keyboard.h>
#include <sys/mcman.h>
#include <sys/config/config.h>
#include <sys/replay/replay.h>

#ifdef __PLAT_NGPS__
//    #include <sif.h>
//    #include <sifdev.h>
    #include <libcdvd.h>
	//#include <libscf.h>
	#include <gfx/ngps/nx/nx_init.h>
#endif

#ifdef __PLAT_NGC__
#include <sys/ngc/p_render.h>
#include <gfx/ngc/nx/nx_init.h>
#endif

#include <gfx/gfxman.h>
#include <gfx/bonedanim.h>
#include <gfx/camera.h>
#include <gfx/casutils.h>
#include <gfx/debuggfx.h>
#include <gfx/gfxutils.h>
#include <gfx/modelbuilder.h>
#include <gfx/skeleton.h>
#include <gfx/nx.h>
#include <gfx/NxSector.h>
#include <gfx/NxModel.h>
#include <gfx/NxTexMan.h>
#include <gfx/NxLoadScreen.h>
#include <gfx/NxLightMan.h>
#include <gfx/NxViewMan.h>
#include <gfx/NxMiscFX.h>
#include <gfx/NxGeom.h>

#include <gel/modman.h>
#include <gel/mainloop.h>
#include <gel/objman.h>
#include <gel/inpman.h>
#include <gel/soundfx/soundfx.h>
#include <gel/music/music.h>
#include <gel/movies/movies.h>
#include <gel/net/net.h>
#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>
#include <gel/prefs/prefs.h>
#include <gel/assman/assman.h>
#include <gel/objtrack.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/rigidbodycomponent.h>
#include <gel/components/cameracomponent.h>
#include <gel/components/proximtriggercomponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/skeletoncomponent.h>
#include <gel/components/soundcomponent.h>
#include <gel/components/streamcomponent.h>
#include <gel/environment/terrain.h>


#include <gel/scripting/script.h> 
#include <gel/scripting/symboltype.h> 
#include <gel/scripting/struct.h> 
#include <gel/scripting/array.h> 
#include <gel/scripting/symboltable.h> 
#include <gel/scripting/checksum.h> 
#include <gel/scripting/parse.h> 
#include <gel/scripting/component.h>
#include <gel/scripting/eval.h>
#include <gel/scripting/string.h>
#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/event.h>
 
#include <sk/scripting/gs_file.h> 
#include <sk/scripting/nodearray.h> 

#include <sk/objects/car.h>
#include <sk/objects/gameobj.h>
#include <sk/objects/gap.h>
#include <sk/objects/moviecam.h>
#include <sk/objects/ped.h>
#include <sk/objects/proxim.h>
#include <sk/objects/emitter.h>
#include <sk/objects/rail.h>
#include <sk/objects/records.h>
#include <sk/objects/restart.h>
#include <sk/objects/skater.h>
#include <sk/objects/skaterprofile.h>
#include <sk/objects/skatercareer.h>
#include <sk/objects/playerprofilemanager.h>
#include <sk/objects/skatercam.h>
#include <sk/objects/trickobject.h>
#include <sk/objects/viewerobj.h>

#ifdef TESTING_GUNSLINGER
#include <sk/objects/navigation.h>
#endif

#include <sk/gamenet/gamenet.h>
#include <sk/modules/skate/competition.h>
#include <sk/modules/skate/GameFlow.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/GoalManager.h>
#include <sk/modules/skate/horse.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/FrontEnd/FrontEnd.h>
//#include <sk/modules/FrontEnd/SoundOptions.h>
//#include <sk/modules/FrontEnd/TrickOptions.h>
#include <sk/modules/Viewer/Viewer.h>
#include <gel/scripting/Debugger.h>
#include <gfx/nxparticlemgr.h>

#include <sk/ParkEditor2/ParkEd.h>

#include <sk/components/raileditorcomponent.h>

#ifdef __PLAT_XBOX__
#include <sk/gamenet/xbox/p_auth.h>
#endif

#include <sk/language.h>

bool DumpFunctionTimesTrigger=false;

void MemViewToggle();

namespace Script
{
	extern void DumpLastStructs();
}

namespace Gfx
{
	void DebuggeryLines_CleanUp( void );		// just for debugging
}

#ifdef	__PLAT_NGPS__
namespace NxPs2
{
#ifdef	__NOPT_ASSERT__
extern uint32		gPassMask1; 		// 1<<6 | 1<<7  (0x40, 0x80)
extern uint32		gPassMask0; 		// 1<<6 | 1<<7  (0x40, 0x80)
#endif 
}
#endif // __PLAT_NGPS__


// This is the level specific qb file.
#if 0
static uint32 s_level_specific_qb=0;
#else
// extended to have more than one
#define	MAX_LEVEL_QBS  2
static uint32 s_level_specific_qbs[MAX_LEVEL_QBS];
#endif


///////////////////////////////////////////////////////////////////
// some temp debugging functions
#define	DUMP_LEN	1024
static void * dump_handle;	
static char p_buffer[1024];
static char *p_pos;

int dumping_printfs = 0;		

bool dump_open(const char *name)
{	
	dump_handle = File::Open( name, "wb");		
	p_pos = p_buffer;

	if (dump_handle != NULL)
	{
		dumping_printfs = 1;
	}

	return ( dump_handle != NULL );
}

void dump_flush()
{
	int len = p_pos - p_buffer;
	if (len)
	{
		File::Write( p_buffer, len, 1, dump_handle );     		
		p_pos = p_buffer;
	}
}

// print to file, replacing lf with cr/lf, so I can read them in notepad
void dump_printf(char *p)
{
	int len = strlen(p);
	if ((int)(p_pos + len) > (int)(p_buffer + DUMP_LEN-16))
	{
		dump_flush();
	}
	
	for (int i= 0; i<len;i++)
	{
		if (*p != 0x0a)
		{
			*p_pos ++ = *p++;     
		}
		else
		{
			p++;
			*p_pos++ = 0x0d;
			*p_pos++ = 0x0a;
		}
	}
}

void dump_close()
{
	dump_flush();
	File::Close( dump_handle );
	dumping_printfs = 0;
}

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace CFuncs
{

using namespace Script;
	
/*****************************************************************************
**								  Externals									**
*****************************************************************************/



/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

enum{
	NODE_ACTION_CREATE,
	NODE_ACTION_KILL,
	NODE_ACTION_SET_VISIBLE,
	NODE_ACTION_SET_INVISIBLE,
	NODE_ACTION_SHATTER,
	NODE_ACTION_CHECK_IF_ALIVE,
	NODE_ACTION_SET_COLOR,
};

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

//static Tmr::Time		s_start_network_test_time = 0;
//static Tsk::Task< Tmr::Time >*	s_network_test_task;
static Tsk::Task< int >*		s_second_controller_check_task = NULL;

// GJ:  viewer mode stuff...  will eventually be moved to CViewer class, hopefully.
static int s_view_mode = 0;
static uint32	s_last_broadcast_cheat = 0;
														
/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*
static void		test_network_task( const Tsk::Task< Tmr::Time >& task )
{
	// Wait a second before testing so the user can see the status box
	if(( Tmr::GetTime() - s_start_network_test_time ) < Tmr::Seconds( 1 ))
	{
		return;
	}
	
	Net::Manager * net_man =  Net::Manager::Instance();
	bool properly_setup;
	 
	properly_setup = net_man->NetworkEnvironmentSetup();
	if( !properly_setup )
	{
		if( net_man->GetLastError() == Net::vRES_ERROR_DEVICE_NOT_HOT )
		{
			Script::CStruct* pStructure;
			
			pStructure = new Script::CStruct;
			
			pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_error_not_connected" ));
			Script::RunScript( "create_net_startup_error_dialog", pStructure );

			delete pStructure;
		}
		else if( ( net_man->GetLastError() == Net::vRES_ERROR_DEVICE_NOT_CONNECTED ) ||
				 ( net_man->GetLastError() == Net::vRES_ERROR_UNKNOWN_DEVICE ))
		{
			Script::CStruct* pStructure;
			
			pStructure = new Script::CStruct;
			
			pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_error_not_detected" ));
			Script::RunScript( "create_net_startup_error_dialog", pStructure );

			delete pStructure;
		}
		else if( net_man->GetLastError() == Net::vRES_ERROR_DHCP )
		{
			Script::CStruct* pStructure;
			
			pStructure = new Script::CStruct;
			
			pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_error_dhcp_error" ));
			Script::RunScript( "create_net_startup_error_dialog", pStructure );

			delete pStructure;
		}
		else if( net_man->GetLastError() == Net::vRES_ERROR_DEVICE_CHANGED )
		{
			Script::CStruct* pStructure;
			
			pStructure = new Script::CStruct;
			
			pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_error_changed_device" ));
			Script::RunScript( "create_net_startup_error_dialog", pStructure );

			delete pStructure;
		}
		else
		{
			Script::CStruct* pStructure;
			
			pStructure = new Script::CStruct;
			
			pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_error_general_error" ));
			Script::RunScript( "create_net_startup_error_dialog", pStructure );

			delete pStructure;
		}
	}
	else
	{   
		Script::CStruct* pParams;

		Script::RunScript( "dialog_box_exit" );
		//Script::RunScript( "launch_network_select_menu" );
		
        pParams = new Script::CStruct;

		pParams->AddChecksum( "change_gamemode", Script::GenerateCRC( "change_gamemode_net" ));
		Script::RunScript( "launch_select_skater_menu", pParams );
		
		delete pParams;
	}

	delete s_network_test_task;
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static	void	s_second_controller_check_code( const Tsk::Task< int > &task )
{
	// Get the second controller.
	 SIO::Manager * pSioMan =  SIO::Manager::Instance();
	SIO::Device *pFirstController=pSioMan->GetDeviceByIndex(0);
	SIO::Device *pSecondController=pSioMan->GetDeviceByIndex(1);
	
#ifdef __PLAT_XBOX__
	int num_plugged_devices;
	SIO::Device *pThirdController=pSioMan->GetDeviceByIndex(2);
	SIO::Device *pFourthController=pSioMan->GetDeviceByIndex(3);
	
	num_plugged_devices = 0;
	if( pFirstController->IsPluggedIn())
	{
		num_plugged_devices++;
	}
	if( pSecondController->IsPluggedIn())
	{
		num_plugged_devices++;
	}
	if( pThirdController->IsPluggedIn())
	{
		num_plugged_devices++;
	}
	if( pFourthController->IsPluggedIn())
	{
		num_plugged_devices++;
	}

	if( num_plugged_devices >= 2 )
	{
		Script::RunScript( "enable_two_player_option" );
	}	
	else
	{
		Script::RunScript( "disable_two_player_option" );
	}         
#else
	if (pSecondController->IsPluggedIn() && pFirstController->IsPluggedIn())
	{
		Script::RunScript( "enable_two_player_option" );
	}	
	else
	{
		Script::RunScript( "disable_two_player_option" );
	}         
#endif
#ifdef __PLAT_XBOX__
	DWORD result;
	static Tmr::Time s_last_link_test;
	
	if(( Tmr::GetTime() - s_last_link_test ) >= Tmr::Seconds( 1 ))
	{
		result = XNetGetEthernetLinkStatus();
        if(( result & XNET_ETHERNET_LINK_ACTIVE ) == 0 )
		{
			Script::RunScript( "disable_system_link_option" );
		}
		else
		{
			Script::RunScript( "enable_system_link_option" );
		}
		s_last_link_test = Tmr::GetTime();
	}
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*
static bool SetupNetworkDeviceDialogHandler( Front::EDialogBoxResult Result )
{
	
	switch (Result)
	{
		case Front::DB_YES:
		{
			Mdl::FrontEnd* front = Mdl::FrontEnd::Instance();
			Front::MenuFactory* menu_factory = Front::MenuFactory::Instance();
			Front::DialogBox* dlg;
			 
			dlg = front->GetDialogBox();
			dlg->GoBack();
					
			Front::MenuEvent event;
			event.SetTypeAndTarget( Front::MenuEvent::vLINK, Script::GenerateCRC("net_network_setup_menu") );
			menu_factory->LaunchEvent(&event);
			
			return false;
		}
		default:
			break;
	}		

	return true;
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

bool ScriptDummyCommand(Script::CStruct *pParams, Script::CScript *pScript)
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// C++ version of the script command	
bool SetActiveCamera(uint32 id, int viewport, bool move_to_current)
{
	Obj::CCompositeObject *p_obj = (Obj::CCompositeObject *)Obj::CCompositeObjectManager::Instance()->GetObjectByID(id);
	if (!p_obj)
	{
		return false;
	}
	
	Obj::CCameraComponent * p_cam_comp = GetCameraComponentFromObject(p_obj); 
	
	if (p_cam_comp)
	{
		// Move this camera to the current camera position.  Useful for seamlessly 
		// transitioning between cameras
		if (move_to_current)
		{
			Dbg_MsgAssert(Nx::CViewportManager::sGetActiveCamera(viewport), ("No active camera for viewport %d\n",viewport));
			p_obj->m_matrix = Nx::CViewportManager::sGetActiveCamera(viewport)->GetMatrix();
			p_obj->m_pos = Nx::CViewportManager::sGetActiveCamera(viewport)->GetPos();			
			p_cam_comp->Update();  // bit dodgy...  force the camera to update itself from the parent object
		}
		
		Nx::CViewportManager::sSetCamera( viewport, p_cam_comp->GetCamera() );
	}
	else
	{
		Dbg_Message("WARNING:  Trying to set camera from object with no camera component");
	}

#if 0
// this is not needed any more, as the proxim trigger component has been deprecated in favour 
// of using whatever camera directly	
	// deactivate all proxim trigger components with this viewport on other objects with cameras and activate any proxim trigger on the new camera object
	Obj::CProximTriggerComponent* p_prox_comp
		= static_cast< Obj::CProximTriggerComponent* >(Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType(CRC_PROXIMTRIGGER));
	while (p_prox_comp)
	{
		Obj::CCameraComponent* p_peer_cam_comp = GetCameraComponentFromObject(p_prox_comp->GetObject());
		
		if (p_peer_cam_comp)
		{
			if (p_peer_cam_comp == p_cam_comp)
			{
				p_prox_comp->SetActive();
				p_prox_comp->SetViewport(viewport);
			}
			else
			{
				if (p_prox_comp->GetViewport() == viewport)
				{
					p_prox_comp->SetInactive();
				}
			}
		}
		
		p_prox_comp = static_cast< Obj::CProximTriggerComponent* >(p_prox_comp->GetNextSameType());
	}

	// K: If a different camera is switched to when the game is paused it could be that the geometry
	// it is looking at was switched off by the previous camera. So refresh the proxim component
	// to make sure it switches back on.
	// (This fixes a bug when scrolling through the view-gaps menu, where geometry was switching off)
	p_prox_comp=GetProximTriggerComponentFromObject(p_obj);
	if (p_prox_comp)
	{
		p_prox_comp->ProximUpdate();
	}	
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/* Waits for n CScript::Update calls.                             */
/******************************************************************/

// @script | Wait | Waits for a period of time, where the time can be specified in various units. <nl>
// For example:  Wait 1200       By default, the units are milliseconds, so 
// this will wait for 1.2 seconds <nl>
// Wait 2.1 seconds     This will wait for 2.1 seconds. <nl>
// Wait 60 frames       This will wait for 60 sixtieths of a second, regardless of the framerate. <nl>
// Wait 60 gameframes       This will wait for exactly 60 logic calls, 
// so the actual time it takes will depend on the current framerate. <nl>
// In all the above, the units do not have to be plural, so one could type 
// ‘Wait 1 second’ instead of ‘Wait 1 seconds’
// @parm float |  | number of units to wait
// @flag seconds | measure time in seconds
// @flag frames | measure time in frames
// @flag gameframes | measure time in gameframes 
bool ScriptWait(Script::CStruct *pParams, Script::CScript *pScript)
{
	float Period=0.0f;
	pParams->GetFloat(NONAME,&Period);
	
	if (pParams->ContainsFlag(0xd029f619/*"seconds"*/) || pParams->ContainsFlag(0x49e0ee96/*"second"*/))
	{
		pScript->WaitTime((int)(Period*1000.0f));
		return true;
	}	

	if (pParams->ContainsFlag(0xfc37df05/*"gameframe"*/) || pParams->ContainsFlag(0xb99ae3d6/*"gameframes"*/) || pParams->ContainsFlag(0xdcd4ce73/*"game"*/))
	{
		pScript->Wait((int)Period);
		return true;
	}	

	if (pParams->ContainsFlag(0x4a07c332/*"frame"*/) || pParams->ContainsFlag(0x19176c5/*"frames"*/))
	{
		pScript->WaitTime((int)(Period*1000.0f/60.0f)); // This should NOT be changed for PAL, as Tmr::GetTime accounts for PAL
		return true;
	}		

	if (pParams->ContainsFlag(0xecaa3345/*"one_per_frame"*/))
	{
		pScript->WaitOnePerFrame((int)Period);
		return true;
	}	
	

	
	pScript->WaitTime((int)Period);
	return true;
}

/******************************************************************/
/*                                                                */
/*  Added for testing if statements, may be useful anyway though  */
/******************************************************************/

// @script | IsZero | added for testing if statements.  may be useful
bool ScriptIsZero(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	
	int Val=0;
	pParams->GetInteger(NONAME,&Val);
	if (Val==0)
	{
		return true;
	}
	else
	{
		return false;
	}		
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | CastToInteger | If a parameter with the passed name exists and is a float,
// this will cast it down to an integer.
// So for example, if x=3.141, after calling CastToInteger x, x will be 3.
// @uparmopt name | The name of the parameter to be cast down
bool ScriptCastToInteger(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 param_name=0;
	if (!pParams->GetChecksum(NONAME,&param_name))
	{
		return false;
	}
	
	float v=0.0f;
	if (!pScript->GetParams()->GetFloat(param_name,&v))
	{
		return false;
	}

	// Need to remove the parameter first, otherwise we will end up with both an integer
	// and a float parameter with the same name. Components are only automatically replaced if
	// the new component has the same name and type.
	pScript->GetParams()->RemoveComponent(param_name);
	pScript->GetParams()->AddInteger(param_name,(int)v);
	return true;
}

// @script bool | StringToInteger | If a parameter with the passed name exists and is a string,
// this will convert it to a integer.
// So for example, if x="123", after calling StringToInteger x, x will be 123.
// @uparmopt name | The name of the string parameter to be converted.
bool ScriptStringToInteger(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 param_name=0;
	if (!pParams->GetChecksum(NONAME,&param_name))
	{
		return false;
	}
	
	const char *p_string="";
	if (!pScript->GetParams()->GetString(param_name,&p_string))
	{
		return false;
	}

	// Need to remove the parameter first, otherwise we will end up with both an integer
	// and a string parameter with the same name. Components are only automatically replaced if
	// the new component has the same name and type.
	pScript->GetParams()->RemoveComponent(param_name);
	
	int v=0;
	int len=strlen(p_string);
	const char *p_ch=p_string;
	for (int i=0; i<len; ++i)
	{
		Dbg_MsgAssert(*p_ch>='0' && *p_ch<='9',("\n%s\nNon-numeric char in '%s'",pScript->GetScriptInfo(),p_string));
		v=v*10+*p_ch++-'0';
	}
	
	pScript->GetParams()->AddInteger(param_name,(int)v);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | IntegerEquals | Returns true if the two values equal
// @parmopt int | a | 0 | First value
// @parmopt int | b | 0 | Second value
bool ScriptIntegerEquals(Script::CStruct *pParams, Script::CScript *pScript)
{
	int a = 0;
	pParams->GetInteger( CRCD(0x174841bc,"a"), &a );
	
	int b = 0;
	pParams->GetInteger( CRCD(0x8e411006,"b"), &b );
	
	return ( a == b );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | ChecksumEquals | Returns true if two checksums are equal
// @parmopt name | a | 0 (checksum value) | First name
// @parmopt name | b | 0 (checksum value) | Second name
bool ScriptChecksumEquals(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	uint32 a=0;
	pParams->GetChecksum("a",&a);
	uint32 b=0;
	pParams->GetChecksum("b",&b);

	return a==b;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | StringEquals | Returns true if two strings are equal
// @parm string | a | First string
// @parm string | b | Secnond string
bool ScriptStringEquals(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	const char* a;
	pParams->GetText("a",&a,true);
	const char* b;
	pParams->GetText("b",&b,true);

	return Script::GenerateCRC(a)==Script::GenerateCRC(b);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | ArrayContains | Returns true if the array contains an
// element equal to the variable. <nl>
// Example:  ArrayContains array=some_int_array contains=4 <nl>
// ArrayContains array=some_string_array contains="some_string"
// @parm array | array | The array to search
// @parm | contains | The element to search for.  Must match array type
bool ScriptArrayContains(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	Script::CArray* pArray;
	pParams->GetArray( "array", &pArray, true );

	uint8 array_type = pArray->GetType();

	for ( uint32 i = 0; i < pArray->GetSize(); i++ )
	{
		switch ( array_type )
		{
			case ESYMBOLTYPE_INTEGER:
			{
				int desiredInteger;
				pParams->GetInteger( "contains", &desiredInteger, true );
				if ( desiredInteger == pArray->GetInt( i ) )
					return true;
				break;
			}
			case ESYMBOLTYPE_NAME:
			{
				uint32 desiredChecksum;
				pParams->GetChecksum( "contains", &desiredChecksum, true );
				if ( desiredChecksum == pArray->GetNameChecksum( i ) )
					return true;
				break;
			}
			case ESYMBOLTYPE_STRING:
			{
				const char* pDesiredString;
				pParams->GetText( "contains", &pDesiredString, true );
				// Changed to use stricmp instead of GenerateCRC because GenerateCRC sees \ and / as
				// being the same character.
				if (stricmp(pDesiredString, pArray->GetString( i )) == 0)
					return true;
				break;
			}
			default:
				Dbg_MsgAssert( 0, ( "This data type is not yet supported by ArrayContains" ) );			
				break;
		}
	}

	// item was not found
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetArraySize | returns the number of elements in the specified structure
// @parm | array_size | return value for array size
bool ScriptGetArraySize(Script::CStruct *pParams, Script::CScript *pScript)
{
	Script::CArray* pArray;
	
	pParams->GetArray( NONAME, &pArray, Script::ASSERT );

	// Add support for higher dimensional arrays
	uint32 index = 0;
	if(pParams->GetInteger(CRCD(0xba453b9,"index1"),(int*)&index, Script::NO_ASSERT))
	{
		pArray = pArray->GetArray(index);

		if(pParams->GetInteger(CRCD(0x92ad0203,"index2"),(int*)&index, Script::NO_ASSERT))
		{
			pArray = pArray->GetArray(index);

			if(pParams->GetInteger(CRCD(0xe5aa3295,"index3"),(int*)&index, Script::NO_ASSERT))
			{
				pArray = pArray->GetArray(index);
			}
		}
	}

	pScript->GetParams()->AddInteger( "array_size", pArray->GetSize() );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetArrayElement | Changes the value of an element of an array.
// @parm name | ArrayName | The name of the array in the current script's parameters whose
// value is to be changed.
// @parm int | Index | The index of the array element whose value is to be changed.
// @parm int | NewValue | The new value for the element. The type of this value must match the
// type of the array, so it is not necessarily an integer. If the type does not match that of
// the array it will assert.
bool ScriptSetArrayElement(Script::CStruct *pParams, Script::CScript *pScript)
{
	// TODO: Most of this code should really be in the CArray class. Need to go over that
	// class to make it easier to manipulate arrays safely. (resize, etc)
	
	// Get a pointer to the CArray that is to be modified.
	// It will be in the script's parameters.
	uint32 array_name=0;
	pParams->GetChecksum("ArrayName",&array_name,Script::ASSERT);
	Script::CArray *p_array=NULL;
	pScript->GetParams()->GetArray(array_name,&p_array,Script::ASSERT);
	
	// Get the index of the element to be changed.
	uint32 index=0;
	pParams->GetInteger("Index",(int*)&index,Script::ASSERT);

	// Find the new value which is to be written into the array.	
	Script::CComponent *p_new_value=pParams->FindNamedComponentRecurse(CRCD(0x4b40630d,"NewValue"));
	Dbg_MsgAssert(p_new_value,("\n%s\nMissing NewValue parameter in call to SetArrayElement",pScript->GetScriptInfo()));
	// Resolve the componentn in case the new value is referring to some global.
	Script::ResolveNameComponent(p_new_value);
	
	// Now make a copy of the component. This is because it may contain a pointer, eg a pointer to
	// a struct, vector, string etc, and that entity may only exist temporarily.
	// CopyComponent will create a new pointer which will be safe to delete later.
	Script::CComponent *p_copy=new Script::CComponent;
	Script::CopyComponent(p_copy,p_new_value);

	// Check that the type of the new value is consistent with the type of the array.
	if (p_array->GetType()==ESYMBOLTYPE_FLOAT && 
		p_copy->mType==ESYMBOLTYPE_INTEGER)
	{
		// It's OK to write an integer value into an array of floats so don't assert in this case.
	}
	else
	{
		Dbg_MsgAssert(p_copy->mType==p_array->GetType(),("Array type of %s does not match the NewValue type of %s in call to SetArrayElement",Script::GetTypeName(p_array->GetType()),Script::GetTypeName(p_copy->mType)));
	}
	
	// Write the new value into the array.
	switch (p_array->GetType())
	{
	case ESYMBOLTYPE_INTEGER:
		p_array->SetInteger(index,p_copy->mIntegerValue);
		break;
    case ESYMBOLTYPE_FLOAT:
		if (p_copy->mType==ESYMBOLTYPE_FLOAT)
		{
			p_array->SetFloat(index,p_copy->mFloatValue);
		}
		else
		{
			Dbg_MsgAssert(p_copy->mType==ESYMBOLTYPE_INTEGER,("Eh ?"));
			p_array->SetFloat(index,(float)p_copy->mIntegerValue);
		}
		break;
    case ESYMBOLTYPE_NAME:
		p_array->SetChecksum(index,p_copy->mChecksum);
		break;
	// Now for the types that are referenced by a pointer.
	// In each of these cases, first the existing pointer in the array gets deleted.
	// The the pointer to the array of pointers is got, and the new pointer written in.
	// Cannot use the regular SetString, SetVector etc functions because they all assert if
	// the existing pointer is not NULL, to catch memory leaks.
    case ESYMBOLTYPE_STRING:
	{
		Script::DeleteString(p_array->GetString(index));
		char **pp_strings=(char**)p_array->GetArrayPointer();
		// Need to do a bounds check since not using SetString
		Dbg_MsgAssert(index<p_array->GetSize(),("Bad index of %d sent to SetArrayElement, array size is %d",index,p_array->GetSize()));
		pp_strings[index]=p_copy->mpString;
		break;
	}	
    case ESYMBOLTYPE_LOCALSTRING:
	{
		Script::DeleteString(p_array->GetString(index));
		char **pp_strings=(char**)p_array->GetArrayPointer();
		Dbg_MsgAssert(index<p_array->GetSize(),("Bad index of %d sent to SetArrayElement, array size is %d",index,p_array->GetSize()));
		pp_strings[index]=p_copy->mpLocalString;
		break;
	}	
    case ESYMBOLTYPE_PAIR:
	{
		delete p_array->GetPair(index);
		Script::CPair **pp_pairs=(Script::CPair**)p_array->GetArrayPointer();
		Dbg_MsgAssert(index<p_array->GetSize(),("Bad index of %d sent to SetArrayElement, array size is %d",index,p_array->GetSize()));
		pp_pairs[index]=p_copy->mpPair;
		break;
	}	
    case ESYMBOLTYPE_VECTOR:
	{
		delete p_array->GetVector(index);
		Script::CVector **pp_vectors=(Script::CVector**)p_array->GetArrayPointer();
		Dbg_MsgAssert(index<p_array->GetSize(),("Bad index of %d sent to SetArrayElement, array size is %d",index,p_array->GetSize()));
		pp_vectors[index]=p_copy->mpVector;
		break;
	}	
    case ESYMBOLTYPE_STRUCTURE:
	{
		delete p_array->GetStructure(index);
		Script::CStruct **pp_structs=(Script::CStruct**)p_array->GetArrayPointer();
		Dbg_MsgAssert(index<p_array->GetSize(),("Bad index of %d sent to SetArrayElement, array size is %d",index,p_array->GetSize()));
		pp_structs[index]=p_copy->mpStructure;
		break;
	}	
    case ESYMBOLTYPE_ARRAY:
	{
		Script::CArray *p_old_array=p_array->GetArray(index);
		Script::CleanUpArray(p_old_array);
		delete p_old_array;
		Script::CArray **pp_arrays=(Script::CArray**)p_array->GetArrayPointer();
		Dbg_MsgAssert(index<p_array->GetSize(),("Bad index of %d sent to SetArrayElement, array size is %d",index,p_array->GetSize()));
		pp_arrays[index]=p_copy->mpArray;
		break;
	}	
	default:
		Dbg_MsgAssert(0,("Unsupported array type of %s in SetArrayElement",Script::GetTypeName(p_array->GetType())));
		break;		
	}
	
	// Now delete the temporary component, but set any pointer in it to NULL first since the
	// pointer has been given to the array.
	p_copy->mUnion=0;
	delete p_copy;
	
	return true;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Get2DarrayData | 
bool ScriptGet2DArrayData(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 array_name=0;
	pParams->GetChecksum(CRCD(0x8f61d602,"ArrayName"),&array_name,Script::ASSERT);
	Script::CArray *p_array1=NULL;
	if(!pScript->GetParams()->GetArray(array_name,&p_array1,Script::NO_ASSERT))
	{
		p_array1 = Script::GetArray( array_name,Script::ASSERT);
	}

	// Get the index of the element
	uint32 index1=0;
	pParams->GetInteger(CRCD(0xba453b9,"Index1"),(int*)&index1,Script::ASSERT);

	// Get the index of the element
	uint32 index2=0;
	pParams->GetInteger(CRCD(0x92ad0203,"Index2"),(int*)&index2,Script::ASSERT);


	Dbg_MsgAssert(index1<p_array1->GetSize(),("Bad index of %d sent to Get2DArrayData, array size is %d",index1,p_array1->GetSize()));
	Script::CArray *p_array2=p_array1->GetArray(index1);

	Dbg_MsgAssert(index2<p_array2->GetSize(),("Bad index of %d sent to Get2DArrayData, array size is %d",index2,p_array2->GetSize()));

	switch (p_array2->GetType())
	{
	case ESYMBOLTYPE_INTEGER:
		pScript->GetParams()->AddInteger( CRCD(0x6820459a,"val"), p_array2->GetInteger(index2) );
		break;
    case ESYMBOLTYPE_FLOAT:
		pScript->GetParams()->AddFloat( CRCD(0x6820459a,"val"), p_array2->GetFloat(index2) );
		break;
    case ESYMBOLTYPE_NAME:
		pScript->GetParams()->AddChecksum( CRCD(0x6820459a,"val"), p_array2->GetChecksum(index2) );
		break;
    case ESYMBOLTYPE_STRING:
	{
		pScript->GetParams()->AddString( CRCD(0x6820459a,"val"), p_array2->GetString(index2) );
		break;
	}	
    case ESYMBOLTYPE_LOCALSTRING:
	{
		pScript->GetParams()->AddString( CRCD(0x6820459a,"val"), p_array2->GetString(index2) );
		break;
	}	
    case ESYMBOLTYPE_PAIR:
	{	
		Script::CPair *p_pair = p_array2->GetPair(index2);
		pScript->GetParams()->AddPair( CRCD(0x6820459a,"val"), p_pair->mX, p_pair->mY );
		break;
	}	
    case ESYMBOLTYPE_VECTOR:
	{
		Script::CVector *p_vector = p_array2->GetVector(index2);
		
		pScript->GetParams()->AddVector( CRCD(0x6820459a,"val"), p_vector->mX, p_vector->mY, p_vector->mZ );
		break;
	}	
    case ESYMBOLTYPE_STRUCTURE:
	{
		pScript->GetParams()->AddStructure( CRCD(0x6820459a,"val"), p_array2->GetStructure(index2) );
		break;
	}	
    case ESYMBOLTYPE_ARRAY:
	{
		pScript->GetParams()->AddArray( CRCD(0x6820459a,"val"), p_array2->GetArray(index2) );
		break;
	}	
	default:
		Dbg_MsgAssert(0,("Unsupported array type of %s",Script::GetTypeName(p_array2->GetType())));
		break;		
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Get3DarrayData | 
bool ScriptGet3DArrayData(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 array_name=0;
	pParams->GetChecksum(CRCD(0x8f61d602,"ArrayName"),&array_name,Script::ASSERT);
	Script::CArray *p_array1=NULL;
	if(!pScript->GetParams()->GetArray(array_name,&p_array1,Script::NO_ASSERT))
	{
		p_array1 = Script::GetArray( array_name,Script::ASSERT);
	}

	// Get the index of the element
	uint32 index1=0;
	pParams->GetInteger(CRCD(0xba453b9,"Index1"),(int*)&index1,Script::ASSERT);

	// Get the index of the element
	uint32 index2=0;
	pParams->GetInteger(CRCD(0x92ad0203,"Index2"),(int*)&index2,Script::ASSERT);

	// Get the index of the element
	uint32 index3=0;
	pParams->GetInteger(CRCD(0xe5aa3295,"Index3"),(int*)&index3,Script::ASSERT);

	Dbg_MsgAssert(index1<p_array1->GetSize(),("Bad index of %d sent to Get3DArrayData, array size is %d",index1,p_array1->GetSize()));
	Script::CArray *p_array2=p_array1->GetArray(index1);

	Dbg_MsgAssert(index2<p_array2->GetSize(),("Bad index of %d sent to Get3DArrayData, array size is %d",index2,p_array2->GetSize()));
	Script::CArray *p_array3=p_array2->GetArray(index2);

	Dbg_MsgAssert(index3<p_array3->GetSize(),("Bad index of %d sent to Get3DArrayData, array size is %d",index3,p_array3->GetSize()));
	

	switch (p_array3->GetType())
	{
	case ESYMBOLTYPE_INTEGER:
		pScript->GetParams()->AddInteger( CRCD(0x6820459a,"val"), p_array3->GetInteger(index3) );
		break;
    case ESYMBOLTYPE_FLOAT:
		pScript->GetParams()->AddFloat(CRCD(0x6820459a,"val"), p_array3->GetFloat(index3) );
		break;
    case ESYMBOLTYPE_NAME:
		pScript->GetParams()->AddChecksum( CRCD(0x6820459a,"val"), p_array3->GetChecksum(index3) );
		break;
    case ESYMBOLTYPE_STRING:
	{
		pScript->GetParams()->AddString( CRCD(0x6820459a,"val"), p_array3->GetString(index3) );
		break;
	}	
    case ESYMBOLTYPE_LOCALSTRING:
	{
		pScript->GetParams()->AddString( CRCD(0x6820459a,"val"), p_array3->GetString(index3) );
		break;
	}	
    case ESYMBOLTYPE_PAIR:
	{	
		Script::CPair *p_pair = p_array3->GetPair(index3);
		pScript->GetParams()->AddPair( CRCD(0x6820459a,"val"), p_pair->mX, p_pair->mY );
		break;
	}	
    case ESYMBOLTYPE_VECTOR:
	{
		Script::CVector *p_vector = p_array3->GetVector(index3);
		
		pScript->GetParams()->AddVector( CRCD(0x6820459a,"val"), p_vector->mX, p_vector->mY, p_vector->mZ );
		break;
	}	
    case ESYMBOLTYPE_STRUCTURE:
	{
		pScript->GetParams()->AddStructure( CRCD(0x6820459a,"val"), p_array3->GetStructure(index3) );
		break;
	}	
    case ESYMBOLTYPE_ARRAY:
	{
		pScript->GetParams()->AddArray( CRCD(0x6820459a,"val"), p_array3->GetArray(index3) );
		break;
	}	
	default:
		Dbg_MsgAssert(0,("Unsupported array type of %s",Script::GetTypeName(p_array3->GetType())));
		break;		
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetNDarrayData | 
bool ScriptGetNDArrayData(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 array_name=0;
	pParams->GetChecksum(CRCD(0x8f61d602,"ArrayName"),&array_name,Script::ASSERT);
	Script::CArray *p_array=NULL;
	if(!pScript->GetParams()->GetArray(array_name,&p_array,Script::NO_ASSERT))
	{
		p_array = Script::GetArray( array_name,Script::ASSERT);
	}

	uint32 indices_name=CRCD(0xffc8e484,"Indices");
	//pParams->GetChecksum("Indices",&indices_name,Script::ASSERT);
	Script::CArray *p_indices=NULL;
	pScript->GetParams()->GetArray(indices_name,&p_indices,Script::ASSERT);

	uint32 i = 0;
	uint32 index=0;
	for(; i < p_indices->GetSize()-1; ++i)
	{
		// Get the index of the element
		index = p_indices->GetInteger( i );
		
		p_array = p_array->GetArray(index);
	}
	index = p_indices->GetInteger( i );
	
	switch (p_array->GetType())
	{
	case ESYMBOLTYPE_INTEGER:
		pScript->GetParams()->AddInteger( CRCD(0x6820459a,"val"), p_array->GetInteger(index) );
		break;
    case ESYMBOLTYPE_FLOAT:
		pScript->GetParams()->AddFloat( CRCD(0x6820459a,"val"), p_array->GetFloat(index) );
		break;
    case ESYMBOLTYPE_NAME:
		pScript->GetParams()->AddChecksum( CRCD(0x6820459a,"val"), p_array->GetChecksum(index) );
		break;
    case ESYMBOLTYPE_STRING:
	{
		pScript->GetParams()->AddString( CRCD(0x6820459a,"val"), p_array->GetString(index) );
		break;
	}	
    case ESYMBOLTYPE_LOCALSTRING:
	{
		pScript->GetParams()->AddString( CRCD(0x6820459a,"val"), p_array->GetString(index) );
		break;
	}	
    case ESYMBOLTYPE_PAIR:
	{	
		Script::CPair *p_pair = p_array->GetPair(index);
		pScript->GetParams()->AddPair( CRCD(0x6820459a,"val"), p_pair->mX, p_pair->mY );
		break;
	}	
    case ESYMBOLTYPE_VECTOR:
	{
		Script::CVector *p_vector = p_array->GetVector(index);
		
		pScript->GetParams()->AddVector( CRCD(0x6820459a,"val"), p_vector->mX, p_vector->mY, p_vector->mZ );
		break;
	}	
    case ESYMBOLTYPE_STRUCTURE:
	{
		pScript->GetParams()->AddStructure( CRCD(0x6820459a,"val"), p_array->GetStructure(index) );
		break;
	}	
    case ESYMBOLTYPE_ARRAY:
	{
		pScript->GetParams()->AddArray( CRCD(0x6820459a,"val"), p_array->GetArray(index) );
		break;
	}	
	default:
		Dbg_MsgAssert(0,("Unsupported array type of %s",Script::GetTypeName(p_array->GetType())));
		break;		
	}
	return true;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AddParams | appends the given structure to the list of params
// @uparmopt {} | structure to append to params
bool ScriptAddParams(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->AppendStructure(pParams);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RemoveComponent | removes a component from the script's list of params
// @uparm name | name of component to remove
bool ScriptRemoveComponent(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 name;
	if ( !pParams->GetChecksum("name", &name, Script::NO_ASSERT) )
	{
		pParams->GetChecksum(NONAME, &name, Script::ASSERT);
	}

	pScript->GetParams()->RemoveComponent(name);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static uint32 s_get_bottom_up_free(bool includeFrag)
{
	Mem::Manager& mem_man = Mem::Manager::sHandle();
	Mem::Heap* p_heap = mem_man.GetHeap( CRCD(0xc7800b0,"BottomUpHeap") );
	Mem::Region* p_region = p_heap->ParentRegion();

	if (includeFrag)
	{
		return p_heap->mFreeMem.m_count + p_region->MemAvailable();
	}
	return p_region->MemAvailable();
}

// @script | SetScriptString | 
bool ScriptSetScriptString(Script::CStruct *pParams, Script::CScript *pScript)
{
	int index=0;
	pParams->GetInteger(CRCD(0x7f8c98fe,"index"),&index);
	
	const char *p_string="";
	pParams->GetString(CRCD(0x61414d56,"string"),&p_string);
	
	Script::SetScriptString(index,p_string);
	return true;
}

// @script | GetScriptString | 
bool ScriptGetScriptString(Script::CStruct *pParams, Script::CScript *pScript)
{
	int index=0;
	pParams->GetInteger(CRCD(0x7f8c98fe,"index"),&index);
	
	pScript->GetParams()->AddString(CRCD(0x61414d56,"string"),Script::GetScriptString(index));
	return true;
}
	
// @script | KenTest1 | old test of ken's
bool ScriptKenTest1(Script::CStruct *pParams, Script::CScript *pScript)
{
	static bool s_got_main_heap_free_edit=false;
	static uint32 s_main_heap_free_edit=0;
	
	if (pParams->ContainsFlag(CRCD(0x1a4e0ef9,"Clear")))
	{
		s_got_main_heap_free_edit=false;
		s_main_heap_free_edit=0;
	}	
	
	if (!s_got_main_heap_free_edit)
	{
		s_main_heap_free_edit = s_get_bottom_up_free(true);
		printf("Stored s_main_heap_free_edit\n");
		s_got_main_heap_free_edit=true;
		return true;
	}	
	
#	if !defined( __PLAT_NGC__ ) || ( defined( __PLAT_NGC__ ) && !defined( __NOPT_FINAL__ ) )
	uint32 main_heap_free_play = s_get_bottom_up_free(false);
	
	printf("main_heap_pad = %d\n", s_main_heap_free_edit - main_heap_free_play + 200000);
#endif		// __NOPT_FINAL__
	
	return true;
}

// Used when creating default file names in the files menu.
// It appends the passed suffix, but if the result will be a string longer than MaxChars
// then it will move the suffix back so that the total number of chars is never more than
// MaxChars.
bool ScriptAppendSuffix(Script::CStruct *pParams, Script::CScript *pScript)
{
	enum
	{
		BUFFER_SIZE=100,
	};	
	char p_new_text[BUFFER_SIZE];
	
	const char *p_text="";
	pParams->GetString(CRCD(0xc4745838,"Text"), &p_text);
	int original_text_length=strlen(p_text);
	Dbg_MsgAssert(original_text_length<BUFFER_SIZE,("Text '%s' too long",p_text));
	strcpy(p_new_text,p_text);
	
	int max_chars=15;
	pParams->GetInteger(CRCD(0xa6931595,"MaxChars"), &max_chars);
	Dbg_MsgAssert(max_chars<BUFFER_SIZE,("max_chars too big"));
	
	
	int suffix=1;
	pParams->GetInteger(CRCD(0x4a4f7821,"suffix"),&suffix);
	
	char p_suffix[20];
	sprintf(p_suffix, " %d",suffix);
	int suffix_length=strlen(p_suffix);
	Dbg_MsgAssert(suffix_length<max_chars,("max_chars of %d too small for suffix '%s'",suffix_length,p_suffix));
	
	if (original_text_length + suffix_length > max_chars)
	{
		p_new_text[max_chars-suffix_length]=0;
	}
	strcat(p_new_text, p_suffix);
	

	uint32 new_text_name=0;
	pParams->GetChecksum(CRCD(0x50060694,"NewTextName"),&new_text_name);
	
	pScript->GetParams()->AddString(new_text_name, p_new_text);
	return true;
}

// @script | FindNearestRailPoint | 
// Used by the rail editor for snapping the cursor to the nearest rail point.
// Note that for this to work CParkGenerator::GenerateOutRailSet needs to have been called at some point first.
// If it were not called, this would only find the nearest edited-rail point.
bool ScriptFindNearestRailPoint(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mth::Vector pos;
	pParams->GetVector(CRCD(0x7f261953,"Pos"),&pos,Script::ASSERT);
	
	float min_dist=150.0f;
	pParams->GetFloat(CRCD(0x213395f1,"MinDist"),&min_dist);
	float min_dist2=min_dist*min_dist;

	uint32 ignore_rail_id=0;
	pParams->GetChecksum(CRCD(0x288a6efe,"IgnoreRailId"),&ignore_rail_id);

	int ignore_rail_point_index=-1;
	pParams->GetInteger(CRCD(0xe4d305a7,"IgnoreRailPointIndex"),&ignore_rail_point_index);


	// Find the nearest edited-rail point	
	Mth::Vector edited_rail_nearest_point;
	float edited_rail_nearest_dist2=100000000.0f;
	uint32 rail_id=0;
	int rail_point_index=0;
	Obj::GetRailEditor()->FindNearestRailPoint(pos,
											   &edited_rail_nearest_point, &edited_rail_nearest_dist2,
											   &rail_id,&rail_point_index,
											   ignore_rail_id, ignore_rail_point_index);
	
	// Then find the nearest rail point on the park editor level geometry.
	// CParkGenerator::GenerateOutRailSet needs to have been called at some point first for this to
	// find anything.
	// Not calling GenerateOutRailSet here because it may take a while to execute.
	Mth::Vector level_nearest_point;
	float level_nearest_dist2=100000000.0f;
	if (pParams->ContainsFlag(CRCD(0x9c247a94,"CheckLevelGeometry")))
	{
		Ed::CParkManager::sInstance()->GetGenerator()->FindNearestRailPoint(pos,&level_nearest_point,&level_nearest_dist2);
	}	
	
	if (edited_rail_nearest_dist2 < min_dist2 ||
		level_nearest_dist2 < min_dist2)
	{
		Mth::Vector nearest_point;
		if (edited_rail_nearest_dist2 < level_nearest_dist2)
		{
			nearest_point=edited_rail_nearest_point;
			
			pScript->GetParams()->AddChecksum(CRCD(0xa61e7cd9,"rail_id"),rail_id);
			pScript->GetParams()->AddInteger(CRCD(0xab3c14,"rail_point_index"),rail_point_index);
		}
		else
		{
			nearest_point=level_nearest_point;
		}	
		pScript->GetParams()->AddVector(CRCD(0xdf970a05,"NearestPos"),nearest_point[X],nearest_point[Y],nearest_point[Z]);
		return true;
	}
	
	return false;
}

bool ScriptGetTime(Script::CStruct *pParams, Script::CScript *pScript)
{
#ifdef __PLAT_NGPS__
/*
	sceCdCLOCK rtc;
	sceScfGetLocalTimefromRTC(&rtc);

	pScript->GetParams()->AddInteger("Hours",rtc.hour);	
	pScript->GetParams()->AddInteger("Minutes",rtc.minute);	
	pScript->GetParams()->AddInteger("Seconds",rtc.second);	
*/	
#endif
	return true;
}

bool ScriptGetDate(Script::CStruct *pParams, Script::CScript *pScript)
{
#ifdef __PLAT_NGPS__
/*
	sceCdCLOCK rtc;
	sceScfGetLocalTimefromRTC(&rtc);

	pScript->GetParams()->AddInteger("Day",rtc.day);	
	pScript->GetParams()->AddInteger("Month",rtc.month);	
	pScript->GetParams()->AddInteger("Year",rtc.year);	
*/	
#endif
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetObNearestScreenCoord(Script::CStruct *pParams, Script::CScript *pScript)
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptRandomize(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mth::InitialRand(Tmr::ElapsedTime(0));
	return true;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | KenTest2 | old test of Ken's
bool ScriptKenTest2(Script::CStruct *pParams, Script::CScript *pScript)
{
	//Ed::CParkEditor::Instance()->PlayModeGapManagerChecks();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static Tmr::Time sStartTime=0;
bool ScriptResetTimer(Script::CStruct *pParams, Script::CScript *pScript)
{
	sStartTime=Tmr::ElapsedTime(0);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptTimeGreaterThan(Script::CStruct *pParams, Script::CScript *pScript)
{
	float t=0.0f;
	pParams->GetFloat(NONAME,&t);
	
	if (Tmr::ElapsedTime(sStartTime)>t*1000.0f)
	{
		return true;
	}
	return false;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetStartTime(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->AddInteger(CRCD(0xd16b61e6, "StartTime"), Tmr::GetTime());
	return true;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetElapsedTime(Script::CStruct *pParams, Script::CScript *pScript)
{
	int start_time;
	pParams->GetInteger(CRCD(0xd16b61e6, "StartTime"), &start_time);
	pScript->GetParams()->AddInteger(CRCD(0x3eb3566b, "ElapsedTime"), Tmr::ElapsedTime(start_time));
	return true;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RemoveParameter | Removes the parameter with the given name. If no such parameter
// exists, it will do nothing.
// @uparmopt name | The name of the parameter to be removed
bool ScriptRemoveParameter(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 name=0;
	pParams->GetChecksum(NONAME,&name);
	
	#ifdef __NOPT_ASSERT__
	const char *p_foo=NULL;
	Dbg_MsgAssert(!pParams->GetString(NONAME,&p_foo),("\n%s\nRemoveParameter requires a name, not a string",pScript->GetScriptInfo()));
	#endif
	
	CStruct *p_params=pScript->GetParams();
	Dbg_MsgAssert(p_params,("NULL p_params ??"));
	p_params->RemoveComponent(name);
	p_params->RemoveFlag(name);
	return true;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetRandomValue | Chooses a random number in a specified range. The returned value can 
// be selected to be either an integer or a floating point value.
// @parmopt name | name | 0 | The name to be given to the random value parameter. For example, 
// name=foo will make the random number be assigned to a parameter called foo.
// @parmopt float | a | 0.0 | The first value in the range.
// @parmopt float | b | 0.0 | The second value in the range. The first value does not have to be smaller than the second.
// @parmopt int | Resolution | 1000 | The number of possible floating point values that could be returned.
// For example, if the range is a=1, b=2 and the resolution is set to 3, it will return either 1, 1.5 or 2
// @flag Integer | Return an integer value in the given range, inclusive of the end values.
// For example, if the range is a=1, b=3 it will return either 1, 2 or 3. Note that a and b need to be 
// integers instead of floats in this case. 
bool ScriptGetRandomValue(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 param_name=0;
	pParams->GetChecksum(CRCD(0xa1dc81f9,"Name"),&param_name);
		
	if (pParams->ContainsFlag(CRCD(0xa5f054f4,"Integer")))
	{
		int a=0;
		int b=0;
		pParams->GetInteger(CRCD(0x174841bc,"a"),&a);
		pParams->GetInteger(CRCD(0x8e411006,"b"),&b);
		if (a>b)
		{
			int t=a;
			a=b;
			b=t;
		}	
		pScript->GetParams()->AddInteger(param_name,a+Mth::Rnd(b-a+1));
	}
	else
	{
		int resolution=1000;
		pParams->GetInteger(CRCD(0x22cf075,"Resolution"),&resolution);
		Dbg_MsgAssert(resolution>=2,("Resolution needs to be at least 2"));

		float a=0.0f;
		float b=0.0f;
		pParams->GetFloat(CRCD(0x174841bc,"a"),&a);
		pParams->GetFloat(CRCD(0x8e411006,"b"),&b);
		
		pScript->GetParams()->AddFloat(param_name,a+Mth::Rnd(resolution)*(b-a)/(resolution-1));
	}
		
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetConfig | Allows various things in the config module to be changed at runtime.
// @parmopt name | Language | | Can be one of English, French or German
// @flag CD | Use CD for file loading
// @flag NotCD | Use host for file loading
// @flag GotExtraMemory | Switch on use of extra memory
// @flag NoExtraMemory | Switch off use of extra memory
// @flag Pal | Be in PAL mode
// @flag NTSC | Be in NTSC mode
// @parmopt int | FrameRate | | Frame rate to use, 50 or 60
bool ScriptSetConfig(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 language_checksum=0;
	pParams->GetChecksum(CRCD(0x2b248e4a,"Language"),&language_checksum);
	
	switch (language_checksum)
	{
	case 0xd37cfdff: // English
		Config::gLanguage=Config::LANGUAGE_ENGLISH;
		break;
	case 0x508a31a1: // French
		Config::gLanguage=Config::LANGUAGE_FRENCH;
		break;
	case 0x5dcbbf5e: // German
		Config::gLanguage=Config::LANGUAGE_GERMAN;
		break;
	case 0xcbe70acb: // Spanish
		Config::gLanguage=Config::LANGUAGE_FRENCH;
		break;
	case 0xa8469630: // Italian
		Config::gLanguage=Config::LANGUAGE_GERMAN;
		break;
	default:
		break;
	}
	
	if (pParams->ContainsFlag(CRCD(0xba297025,"cd")))
	{
		if (!Config::gCD)
		{
			File::InstallFileSystem();
		}
		Config::gCD=true;
	}	
	if (pParams->ContainsFlag(CRCD(0x2b6d7b32,"notcd")))
	{
		if (Config::gCD)
		{
			File::UninstallFileSystem();
		}
		Config::gCD=false;
	}	
	
	if (pParams->ContainsFlag(CRCD(0x38d23fd4,"GotExtraMemory")))
	{
		Config::gGotExtraMemory=true;
	}
	if (pParams->ContainsFlag(CRCD(0xaf11b50f,"NoExtraMemory")))		
	{
		Config::gGotExtraMemory=false;
	}
	
	if (pParams->ContainsFlag(CRCD(0x6cad3928,"Pal")))		
	{
		Config::gDisplayType=Config::DISPLAY_TYPE_PAL;
	}
	if (pParams->ContainsFlag(CRCD(0x86137a88,"NTSC")))		
	{
		Config::gDisplayType=Config::DISPLAY_TYPE_NTSC;
	}
	
	int frame_rate=60;
	if (pParams->GetInteger(CRCD(0xee2bc65f,"FrameRate"),&frame_rate))
	{
		Config::gFPS=frame_rate;
	}
	
	return true;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PrintConfig | Prints the current system configuration, whether it is CD, PAL, etc.
bool ScriptPrintConfig(Script::CStruct *pParams, Script::CScript *pScript)
{
	printf("\n");
	printf("System Configuration:\n\n");
	printf("Hardware       = %s\n",Config::GetHardwareName());
	printf("Display Type   = %s, %d fps\n",Config::GetDisplayTypeName(),Config::FPS());
	printf("Language       = %s\n",Config::GetLanguageName());
	printf("Territory      = %s\n",Config::GetTerritoryName());
	printf("CD             = %s\n",Config::CD()?"True":"False");
	printf("GotExtraMemory = %s\n",Config::GotExtraMemory()?"True":"False");
	printf("\n");
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | German | Returns true if the current language is German
bool ScriptGerman(Script::CStruct *pParams, Script::CScript *pScript)
{
	return Config::GetLanguage()==Config::LANGUAGE_GERMAN;
}

// @script | French | Returns true if the current language is French
bool ScriptFrench(Script::CStruct *pParams, Script::CScript *pScript)
{
	return Config::GetLanguage()==Config::LANGUAGE_FRENCH;
}

// @script | Spanish | Returns true if the current language is Spanish
bool ScriptSpanish(Script::CStruct *pParams, Script::CScript *pScript)
{
	return Config::GetLanguage()==Config::LANGUAGE_SPANISH;
}

// @script | Italian | Returns true if the current language is Italian
bool ScriptItalian(Script::CStruct *pParams, Script::CScript *pScript)
{
	return Config::GetLanguage()==Config::LANGUAGE_ITALIAN;
}

// @script | English | Returns true if the current language is English
bool ScriptEnglish(Script::CStruct *pParams, Script::CScript *pScript)
{
	return Config::GetLanguage()==Config::LANGUAGE_ENGLISH;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#ifdef		__USE_PROFILER__			
static Tmr::CPUCycles sStopwatchStartTime=0;
#endif

bool ScriptResetStopwatch(Script::CStruct *pParams, Script::CScript *pScript)
{
#ifdef		__USE_PROFILER__			
	sStopwatchStartTime=Tmr::GetTimeInCPUCycles();
#endif	
	return true;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptPrintStopwatchTime(Script::CStruct *pParams, Script::CScript *pScript)
{
#ifdef		__USE_PROFILER__			
	Tmr::CPUCycles diff=Tmr::GetTimeInCPUCycles()-sStopwatchStartTime; 
	printf("Stopwatch time = %.3f seconds\n",(float)diff/150000000.0f);
#endif	
	return true;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CustomSkaterFilenameDefined | Doesn't appear to be supported
bool ScriptCustomSkaterFilenameDefined(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	 Mdl::Skate * pSkate =  Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile=pSkate->GetCurrentProfile();
	Dbg_MsgAssert(pSkaterProfile,("NULL pSkaterProfile"));
	
	if (strcmp(pSkaterProfile->GetCASFileName(),"Unimplemented")==0)
	{
		return false;
	}
	else
	{
		return true;
	}
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetCustomSkaterFilename | Gets the filename for the current CAS
bool ScriptGetCustomSkaterFilename(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Mdl::Skate * pSkate =  Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile=pSkate->GetCurrentProfile();
	Dbg_MsgAssert(pSkaterProfile,("NULL pSkaterProfile"));
	
	pScript->GetParams()->AddString(CRCD(0xf36c1878,"CASFileName"),pSkaterProfile->GetCASFileName());
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetCustomSkaterFilename | Sets the filename for the current CAS
bool ScriptSetCustomSkaterFilename(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mdl::Skate * pSkate =  Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile=pSkate->GetCurrentProfile();
	Dbg_MsgAssert(pSkaterProfile,("NULL pSkaterProfile"));

	const char *p_file_name="Unimplemented";
	pParams->GetString(NONAME,&p_file_name);
	pSkaterProfile->SetCASFileName(p_file_name);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EditingPark | Returns true of we're in the park editor
bool ScriptEditingPark(Script::CStruct *pParams, Script::CScript *pScript)
{
#if 0
	Ed::ParkEditor * pParkEd =  Ed::ParkEditor::Instance();
	if (pParkEd->IsInitialized() && !pParkEd->GameGoingOrOutsideEditor())
	{
		// We're in the park editor, editing a park.
		return true;
	}
#endif
		
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#define PARK_NAME_BUF_SIZE 128 
static char spParkName[PARK_NAME_BUF_SIZE]={0};

// @script | GetParkName | Gets the current park name
bool ScriptGetParkName(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->AddString(CRCD(0xcc23bddb,"ParkName"),spParkName);
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetParkName | Sets the current park name
bool ScriptSetParkName(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *p_name="";
	pParams->GetString(NONAME,&p_name);
	Dbg_MsgAssert(strlen(p_name)<PARK_NAME_BUF_SIZE,("Park name '%s' too long",p_name));

	strcpy(spParkName,p_name);	
	return false;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ParkEditorThemeWasSwitched | Check if theme changed
bool ScriptParkEditorThemeWasSwitched(Script::CStruct *pParams, Script::CScript *pScript)
{
#if 0
	Ed::ParkEditor * pParkEd =  Ed::ParkEditor::Instance();
	return pParkEd->m_themeWasAutoSwitched;
#endif
	return false;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static bool MenuIsShown(uint32 id)
{
/*
	 Front::MenuFactory * pMenuFactory =  Front::MenuFactory::Instance();
	Front::MenuElement *pMenuElement=pMenuFactory->GetMenuElement(id,false); // false means don't assert

	if (pMenuElement && pMenuElement->m_isShown)
		return true;
	else
		return false;
*/

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MenuIsShown | Checks array of names (pArray) is shown
bool ScriptMenuIsShown(Script::CStruct *pParams, Script::CScript *pScript)
{
	// If there is an array of names, check them.		
	Script::CArray *pArray=NULL;
	if (pParams->GetArray(NONAME,&pArray))
	{
		Dbg_MsgAssert(pArray,("Eh? NULL pArray?"));
		for (uint32 i=0; i<pArray->GetSize(); ++i)
		{
			if (MenuIsShown(pArray->GetNameChecksum(i)))
			{
				return true;
			}
		}
	}
	
	uint32 id=0;
	if (pParams->GetChecksum(NONAME,&id))
	{
		if (MenuIsShown(id))
		{
			return true;
		}	
	}
			
	return false;			
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static bool MenuIsSelected(uint32 id)
{
	
	/*
	 Front::MenuFactory * pMenuFactory =  Front::MenuFactory::Instance();
	Front::MenuElement *pMenuElement=pMenuFactory->GetMenuElement(id,false); // false means don't assert

	if (pMenuElement && pMenuElement->m_isSelected)
		return true;
	else
		return false;
	*/
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ForEachIn | For each member of the array, run the specified script
// @parm name | Do | The name of the script to run
// @uparm [] | The array on which to run
// @parmopt structure | Params | | any extra parameters that need to be
// merged onto each of those in the array
bool ScriptForEachIn(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 ScriptChecksum=0;
	pParams->GetChecksum(CRCD(0x62ba3f6a,"Do"),&ScriptChecksum);
	Dbg_MsgAssert(ScriptChecksum,("\n%s\nScript name missing in call to ForEachIn\n(eg, ForEachIn AnArray Do=AScript)",pScript->GetScriptInfo()));
	
	Script::CArray *pArray=NULL;
	pParams->GetArray(NONAME,&pArray);
	Dbg_MsgAssert(pArray,("\n%s\nMissing array parameter in call to ForEachIn",pScript->GetScriptInfo()));
	Dbg_MsgAssert(pArray->GetSize()==0 || pArray->GetType()==ESYMBOLTYPE_STRUCTURE,("\n%s\nForEachIn only supports arrays of structures at the moment",pScript->GetScriptInfo()));
	
	
	// Get any extra parameters that need to be merged onto each of those in the array.
	Script::CStruct *pExtraParams=NULL;
	pParams->GetStructure(CRCD(0x7031f10c,"params"),&pExtraParams);
	
	if (pExtraParams)
	{
		// Used for holding the merged parameters.
		Script::CStruct *pTotalParams=new Script::CStruct;
		
		for (uint32 i=0; i<pArray->GetSize(); ++i)
		{
			pTotalParams->Clear();
			
			CStruct *p_array_struct=pArray->GetStructure(i);
			
			if (p_array_struct)
			{
				*pTotalParams+=*p_array_struct;
			}	
			
			if (pExtraParams)
			{
				*pTotalParams+=*pExtraParams;
			}	
	
			Script::RunScript(ScriptChecksum,pTotalParams,pScript->mpObject);
		}
		
		// Delete the temporary structure.
		delete pTotalParams;
	}
	else
	{
		for (uint32 i=0; i<pArray->GetSize(); ++i)
		{
			Script::RunScript(ScriptChecksum,pArray->GetStructure(i),pScript->mpObject);
		}
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SizeOf | 
// Puts the size (num elements in) of the passed array into a parameter called ArraySize
// @uparm [] | The array of which we're finding the size
bool ScriptSizeOf(Script::CStruct *pParams, Script::CScript *pScript)
{
	Script::CArray *p_array=NULL;
	pParams->GetArray(NONAME,&p_array);
	
	int size=0;
	if (p_array)
	{
		size=p_array->GetSize();
	}	
	pScript->GetParams()->AddComponent(Script::GenerateCRC("ArraySize"),ESYMBOLTYPE_INTEGER,size);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetElement | Retrieves the array element pointed to by "Index"
// and places it in the param called "Element"
// @uparm [] | the array
// @parmopt int | Index | 0 | The index value of the array element
bool ScriptGetElement(Script::CStruct *pParams, Script::CScript *pScript)
{
	Script::CArray *p_array=NULL;
	pParams->GetArray(NONAME,&p_array);

	int index=0;
	pParams->GetInteger("Index",&index);

	pScript->GetParams()->RemoveComponent(0xbebfa1c6/*Element*/);	
	
	if (p_array)
	{
		if (index<0 || index>=(int)p_array->GetSize())
		{
		   #ifdef __NOPT_ASSERT__
		   printf("\n%s\nWarning ! Index %d out of range. Array has %d elements\n",pScript->GetScriptInfo(),index,p_array->GetSize());
		   #endif
		   return false;
		}
		
		switch (p_array->GetType())
		{
		case ESYMBOLTYPE_STRUCTURE:
		{
			Script::CStruct *p_new=new Script::CStruct;
			p_new->AppendStructure(p_array->GetStructure(index));
			pScript->GetParams()->AddComponent(0xbebfa1c6/*Element*/,ESYMBOLTYPE_STRUCTUREPOINTER,(int)p_new);
			break;
		}	
		case ESYMBOLTYPE_ARRAY:
			pScript->GetParams()->AddComponent(0xbebfa1c6/*Element*/,p_array->GetArray(index));
			break;
		case ESYMBOLTYPE_INTEGER:
			pScript->GetParams()->AddComponent(0xbebfa1c6/*Element*/,ESYMBOLTYPE_INTEGER,p_array->GetInt(index));
			break;
			
		default:
			printf("GetElement currently only supports arrays of structures, arrays or integers ... \n(Requested type=%d)\n",p_array->GetType());
			break;
		}		
	}
	return true;
}
		   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetNextArrayElement | Increments the value in Index and uses it to retrieve that
// element from the array, and puts it in Element
// If no Index parameter exists, it will retrieve the first element of the array, and 
// create Index and set it to zero.
// If Index goes off the end of the array, it will remove the Index parameter and
// any existing Element parameter, and return false.
// Hence calling GetNextArrayElement repeatedly will cycle through the array elements.
// @uparm [] | the array
bool ScriptGetNextArrayElement(Script::CStruct *pParams, Script::CScript *pScript)
{
	Script::CArray *p_array=NULL;
	pParams->GetArray(NONAME,&p_array);

	int index=-1;
	pScript->GetParams()->GetInteger(CRCD(0x7f8c98fe,"Index"),&index);

	pScript->GetParams()->RemoveComponent(0x7f8c98fe/*Index*/);
	pScript->GetParams()->RemoveComponent(0xbebfa1c6/*Element*/);
	
	if (!p_array)
	{
		return false;
	}
	
	int size=p_array->GetSize();
	
	if (index<0)
	{
		// No index parameter existed, so set index to the first element of the array.
		index=0;
		
		if (size==0)
		{
			// The array is empty though, so add nothing to the params.
			return false;
		}	
	}
	else
	{
		// An index existed already, so increment it.
		++index;
		if (index>=size)
		{
			// Run out of array elements!
			return false;
		}
	}

	pScript->GetParams()->AddInteger(CRCD(0x7f8c98fe,"Index"),index);

	switch (p_array->GetType())
	{
	case ESYMBOLTYPE_STRUCTURE:
		pScript->GetParams()->AddStructure(0xbebfa1c6/*Element*/,p_array->GetStructure(index));
		break;
	case ESYMBOLTYPE_ARRAY:
		pScript->GetParams()->AddArray(0xbebfa1c6/*Element*/,p_array->GetArray(index));
		break;
	case ESYMBOLTYPE_INTEGER:
		pScript->GetParams()->AddInteger(0xbebfa1c6/*Element*/,p_array->GetInteger(index));
		break;
	case ESYMBOLTYPE_FLOAT:
		pScript->GetParams()->AddFloat(0xbebfa1c6/*Element*/,p_array->GetFloat(index));
		break;
	case ESYMBOLTYPE_NAME:
		pScript->GetParams()->AddChecksum(0xbebfa1c6/*Element*/,p_array->GetChecksum(index));
		break;
	case ESYMBOLTYPE_STRING:
		pScript->GetParams()->AddString(0xbebfa1c6/*Element*/,p_array->GetString(index));
		break;
	case ESYMBOLTYPE_LOCALSTRING:
		pScript->GetParams()->AddLocalString(0xbebfa1c6/*Element*/,p_array->GetLocalString(index));
		break;
	default:
		printf("GetNextArrayElement does not support arrays of type '%s'\n",GetTypeName(p_array->GetType()));
		break;
	}		
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetRandomArrayElement | Chooses a random element of the passed array, and
// puts it in Element. It will put the index of the chosen element into a parameter called Index.
// If the array is empty, it will remove any existing Element and Index parameter and return false.
// @uparm [] | the array
bool ScriptGetRandomArrayElement(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->RemoveComponent(CRCD(0xbebfa1c6,"Element"));
	pScript->GetParams()->RemoveComponent(CRCD(0x7f8c98fe,"index"));

	Script::CArray *p_array=NULL;
	pParams->GetArray(NONAME,&p_array);

	if (!p_array)
	{
		return false;
	}
	
	int size=p_array->GetSize();
	if (size==0)
	{
		return false;
	}	

	int index=Mth::Rnd(size);
	// Wack in the index too cos it is handy to know.
	pScript->GetParams()->AddInteger(CRCD(0x7f8c98fe,"index"),index);	
		
	switch (p_array->GetType())
	{
	case ESYMBOLTYPE_STRUCTURE:
		pScript->GetParams()->AddStructure(CRCD(0xbebfa1c6,"Element"),p_array->GetStructure(index));
		break;
	case ESYMBOLTYPE_ARRAY:
		pScript->GetParams()->AddArray(CRCD(0xbebfa1c6,"Element"),p_array->GetArray(index));
		break;
	case ESYMBOLTYPE_INTEGER:
		pScript->GetParams()->AddInteger(CRCD(0xbebfa1c6,"Element"),p_array->GetInteger(index));
		break;
	case ESYMBOLTYPE_FLOAT:
		pScript->GetParams()->AddFloat(CRCD(0xbebfa1c6,"Element"),p_array->GetFloat(index));
		break;
	case ESYMBOLTYPE_NAME:
		pScript->GetParams()->AddChecksum(CRCD(0xbebfa1c6,"Element"),p_array->GetChecksum(index));
		break;
	default:
		printf("GetRandomArrayElement does not support arrays of type '%s'\n",GetTypeName(p_array->GetType()));
		break;
	}		
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PermuteArray | Creates a new array equal to the passed array with its elements
// randomly permuted. The new array is put into a parameter named using the passed NewArrayName.
// @parm array | Array | The source array
// @parm name | NewArrayName | The name to be given to the new array parameter.
// @flag MakeNewFirstDifferFromOldLast | Makes the first element of the new array definitely
// be different from the old last element. This is so that the old permutation can be
// concatenated with the new and no consecutive elements will be the same.
// Could be useful for playing songs in a random order with no repeats.
// @flag PermuteSource | If this flag is specified then it will be the source array itself
// that will get permuted, no new array will get created.
// So the NewArrayName parameter is not required in this case, it will be ignored.
bool ScriptPermuteArray(Script::CStruct *pParams, Script::CScript *pScript)
{
	Script::CArray *p_source_array=NULL;
	pParams->GetArray("Array",&p_source_array);
	
	if (!p_source_array)
	{
		#ifdef __NOPT_ASSERT__
		printf("\n%s\nPermuteArray requires an Array parameter\n",pScript->GetScriptInfo());
		#endif
		return false;
	}

	CArray *p_array_to_be_permuted=p_source_array;
	bool create_new_array=!pParams->ContainsFlag("PermuteSource");
	
	uint32 new_array_name=0;
	CArray *p_new_array=NULL;
	
	if (create_new_array)
	{
		pParams->GetChecksum("NewArrayName",&new_array_name);
		if (!new_array_name)
		{
			#ifdef __NOPT_ASSERT__
			printf("\n%s\nPermuteArray requires a NewArrayName parameter\n",pScript->GetScriptInfo());
			#endif
			return false;
		}	
		
		p_new_array=new CArray;
		CopyArray(p_new_array,p_source_array);
		p_array_to_be_permuted=p_new_array;
	}	
	
	
	
	int size=p_array_to_be_permuted->GetSize();
	if (size)
	{
		uint32 *p_array_data=p_array_to_be_permuted->GetArrayPointer();
		Dbg_MsgAssert(p_array_data,("NULL p_array_data ?"));
		
		int num_swaps=size*10;
		
		uint32 old_last=p_array_data[size-1];
		
		for (int i=0; i<num_swaps; ++i)
		{
			int a=Mth::Rnd(size);
			int b=Mth::Rnd(size);
			
			uint32 temp=p_array_data[a];
			p_array_data[a]=p_array_data[b];
			p_array_data[b]=temp;
		}
		
		// Ensure that the new first element is not the same as the old last one,
		// so that when using the function to jumble up a list of soundtracks say,
		// there will never be two consecutive ones the same.
		if (pParams->ContainsFlag("MakeNewFirstDifferFromOldLast") && size>1 && p_array_data[0]==old_last)
		{
			int a=0;
			int b=1+Mth::Rnd(size-1);
			
			uint32 temp=p_array_data[a];
			p_array_data[a]=p_array_data[b];
			p_array_data[b]=temp;
		}
	}
	
	if (create_new_array)
	{
		pScript->GetParams()->AddArrayPointer(new_array_name,p_new_array);
	}
		
	return true;
}
	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetGammaValues( Script::CStruct *pParams, Script::CScript *pScript )
{
#	if defined( __PLAT_XBOX__ )
	 Gfx::Manager * gfx_man =  Gfx::Manager::Instance();
	float gamma_r;
	float gamma_g;
	float gamma_b;
	gfx_man->GetGammaNormalized( &gamma_r, &gamma_g, &gamma_b );

	pScript->GetParams()->RemoveComponent( "red" );
	pScript->GetParams()->RemoveComponent( "green" );
	pScript->GetParams()->RemoveComponent( "blue" );

	pScript->GetParams()->AddInteger( "red", ( (int)( gamma_r * 100.0f ) ));
	pScript->GetParams()->AddInteger( "green", ( (int)( gamma_g * 100.0f ) ));
	pScript->GetParams()->AddInteger( "blue", ( (int)( gamma_b * 100.0f ) ));
#	endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ApplyChangeGamma | This gets called whenever one of 
// the color values in the gamma menu changes.
// Called from script in gamemenu.q
bool ScriptApplyChangeGamma( Script::CStruct *pParams, Script::CScript *pScript )
{
	
#	if defined( __PLAT_XBOX__ )	
	// get the current values
	 Gfx::Manager * gfx_man =  Gfx::Manager::Instance();
	float gamma_r;
	float gamma_g;
	float gamma_b;
	gfx_man->GetGammaNormalized( &gamma_r, &gamma_g, &gamma_b );

	uint32 color;
	pParams->GetChecksum( "color", &color, Script::ASSERT );

	float change;
	pParams->GetFloat( "change", &change, Script::ASSERT );
	switch ( color )
	{
	case CRCC( 0x59ea070, "red" ):
		if (( change > 0.0f && gamma_r < 1.0f ) || ( change < 0.0f && gamma_r > 0.0f ))
			gamma_r += change;
		break;
	case CRCC( 0x2f6511de, "green" ):
		if (( change > 0.0f && gamma_g < 1.0f ) || ( change < 0.0f && gamma_g > 0.0f ))
			gamma_g += change;
		break;
	case CRCC( 0x61c9354b, "blue" ):
		if (( change > 0.0f && gamma_b < 1.0f ) || ( change < 0.0f && gamma_b > 0.0f ))
			gamma_b += change;
		break;
	default:
		Dbg_MsgAssert( 0, ("ApplyChangeGamma called on unknown color") );
	}
	
	gfx_man->SetGammaNormalized( gamma_r, gamma_g, gamma_b );
	
/*	// grab any new values from the params
	int r, g, b;
	if ( pParams->GetInteger( "red", &r, Script::NO_ASSERT ) )
		gamma_r = (float)(r) / 100.0f;
	if ( pParams->GetInteger( "green", &g, Script::NO_ASSERT ) )
		gamma_g = (float)(g) / 100.0f;
	if ( pParams->GetInteger( "blue", &b, Script::NO_ASSERT ) )
		gamma_b = (float)(b) / 100.0f;
*/

#	endif
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GotParam | Checks if the script was passed the specified parameter
// @uparm name | The name of the param to check for
bool ScriptGotParam(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	uint32 FlagChecksum=0;
	pParams->GetChecksum(NONAME,&FlagChecksum);
	Dbg_MsgAssert(FlagChecksum,("GotParam command requires a flag name"));
	
	// Get the script's parameters, and see if they contain the aforementioned flag therein.
	Script::CStruct *pScriptParams=pScript->GetParams();
	Dbg_MsgAssert(pScriptParams,("NULL pScriptParams"));
	return pScriptParams->ContainsComponentNamed(FlagChecksum);
}

Tmr::Time gButtonDebounceTime[PAD_NUMBUTTONS];  // PAD_NUMBUTTONS defined in skater.h for some reason (instead of some controller header)

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ControllerDeboune |
// For example: <nl>
// Debounce X time = 2.0 <nl>
// This will ignore the X button until either the time has elapsed,
// or the X button has been released and subsequently re-pressed. <nl>
// This prevents the press of X from carrying over from one screen to 
// the next, and removes the need to have a delay, whilst still allowing 
// the user to actively X past things. 
// @uparm name | the button
// @parmopt float | time | 1.0 | 
bool ScriptControllerDebounce( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	uint32 Checksum = 0;
	float time=1.0f;
	pParams->GetFloat(0x906b67ba,&time);	   // "time"
	if ( pParams->GetChecksum( NONAME, &Checksum ) )
	{
		int Button = Inp::GetButtonIndex(Checksum);	
		gButtonDebounceTime[Button] = (Tmr::Time)(Tmr::GetTime() + (time * 1000));
	}
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void ResetDebounce( void )
{
	
	int i;
	for ( i = 0; i < PAD_NUMBUTTONS; i++ )
	{
		gButtonDebounceTime[ i ] = 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CheckButton(Inp::Handler< Mdl::FrontEnd >* pHandler, uint32 button)
{
	

	if (!pHandler)
	{
		return false;
	}	
	
	if (!pHandler->m_Input)
	{
		return false;
	}	
	
	int whichButton = Inp::GetButtonIndex( button );
	if ( gButtonDebounceTime[ whichButton ] > Tmr::GetTime( ) )
	{
		return ( false );
	}
	switch ( button )
	{
		case ( 0xbc6b118f ): // Up
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_UP] )
			{
				return ( true );
			}
			break;
		case ( 0xe3006fc4 ): // Down
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_DOWN] )
			{
				return ( true );
			}
			break;
		case ( 0x85981897 ): // Left
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_LEFT] )
			{
				return ( true );
			}
			break;
		case ( 0x4b358aeb ): // Right
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_RIGHT] )
			{
				return ( true );
			}
			break;
		case ( 0xb7231a95 ): // UpLeft
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_UP] )
			{
				if ( pHandler->m_Input->m_Event[Inp::Data::vA_LEFT] )
				{
					return ( true );
				}
			}
			break;
		case ( 0xa50950c5 ): // UpRight
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_UP] )
			{
				if ( pHandler->m_Input->m_Event[Inp::Data::vA_RIGHT] )
				{
					return ( true );
				}
			}
			break;
		case ( 0xd8847efa ): // DownLeft
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_DOWN] )
			{
				if ( pHandler->m_Input->m_Event[Inp::Data::vA_LEFT] )
				{
					return ( true );
				}
			}
			break;
		case ( 0x786b8b68 ): // DownRight
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_DOWN] )
			{
				if ( pHandler->m_Input->m_Event[Inp::Data::vA_RIGHT] )
				{
					return ( true );
				}
			}
			break;
		case ( 0x2b489a86 ): // Circle
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_CIRCLE] )
			{
				return ( true );
			}
			break;
		case ( 0x321c9756 ): // Square
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_SQUARE] )
			{
				return ( true );
			}
			break;
		case ( 0x7323e97c ): // X
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_X] )
			{
				return ( true );
			}
			break;
		case ( 0x20689278 ): // Triangle
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_TRIANGLE] )
			{
				return ( true );
			}
			break;
		case ( 0x26b0c991 ): // L1
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_L1] )
			{
				return ( true );
			}
			break;
		case ( 0xbfb9982b ): // L2
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_L2] )
			{
				return ( true );
			}
			break;
		case ( 0xf2f1f64e ): // R1
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_R1] )
			{
				return ( true );
			}
			break;
		case ( 0x6bf8a7f4 ): // R2
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_R2] )
			{
				return ( true );
			}
			break;
        case ( 0x767a45d7 ): // black
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_BLACK] )
			{
				return ( true );
			}
			break;
        case ( 0xbd30325b ): // white
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_WHITE] )
			{
				return ( true );
			}
			break;
        case ( 0x9d2d8850 ): // Z
			if ( pHandler->m_Input->m_Event[Inp::Data::vA_Z] )
			{
				return ( true );
			}
			break;

		default:
			break;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ControllerPressed | returns true if the specified button is pressed
// @uparm name | the button to check
// @uparmopt 0 | controller index
bool ScriptControllerPressed(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mdl::FrontEnd* pFront = Mdl::FrontEnd::Instance();

	int controller;
	uint32 button = 0;
	if ( pParams->GetChecksum( NONAME, &button ) )
	{
		if ( pParams->GetInteger( NONAME, &controller, Script::NO_ASSERT ) )
		{
			Dbg_MsgAssert( controller < SIO::vMAX_DEVICES && controller >= 0, ( "ControllerPressed called with bad controller index" ) );
			if ( CheckButton( pFront->GetInputHandler( controller ), button ) )
				return true;
			else
				return false;
		}
		else
		{
			for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
			{
				if ( CheckButton( pFront->GetInputHandler( i ), button ) )
					return true;
			}
			return false;
		}
	}
	return false;
}

// @script | GetAnalogueInfo | Reads the analogue sticks, ans returns the floats LeftX, LeftY, RightX and RightY.
// These each range from -1 to 1.
// @parmopt int | Controller | 0 | Controller index
bool ScriptGetAnalogueInfo(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mdl::FrontEnd* pFront = Mdl::FrontEnd::Instance();

	int controller=0;
	pParams->GetInteger( CRCD(0xb30d9965,"Controller"), &controller, Script::NO_ASSERT );
	
	Dbg_MsgAssert( controller < SIO::vMAX_DEVICES && controller >= 0, ( "ControllerPressed called with bad controller index" ) );
	Inp::Handler< Mdl::FrontEnd > *p_handler=pFront->GetInputHandler( controller );
	CControlPad pad;
	pad.Update(p_handler->m_Input);
	
	pScript->GetParams()->AddFloat(CRCD(0x303067f1,"LeftX"),pad.m_scaled_leftX);
	pScript->GetParams()->AddFloat(CRCD(0x47375767,"LeftY"),pad.m_scaled_leftY);
	pScript->GetParams()->AddFloat(CRCD(0x694df774,"RightX"),pad.m_scaled_rightX);
	pScript->GetParams()->AddFloat(CRCD(0x1e4ac7e2,"RightY"),pad.m_scaled_rightY);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSfxVolume | SetSfxVolume [100] <nl>
// Param: volumeLevel <nl>
// range: 0 to 100 (the main mixing volume, affecting all soundfx) <nl>
// default: 100 <nl> <nl>
// Notes: the volume is set to 100 on startup, and can only be modified with this command
// @uparmopt 100.0 | volume
bool ScriptSetVolume( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	float Volume = 100.0f;
    
	pParams->GetFloat( NONAME, &Volume );

	 Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
	sfx_manager->SetMainVolume( Volume );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetMusicVolume | 
// @uparmopt 100.0 | volume level
bool ScriptSetMusicVolume( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	float Volume = 100.0f;
    
	pParams->GetFloat( NONAME, &Volume );

	Pcm::SetVolume( Volume );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetMusicStreamVolume | 
// @uparmopt 100.0 | volume level
bool ScriptSetMusicStreamVolume( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	float Volume = 100.0f;
    
	pParams->GetFloat( NONAME, &Volume );

	Pcm::SetMusicStreamVolume( Volume );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StopMusic | 
bool ScriptStopMusic( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	Pcm::StopMusic( );
	return ( true );
}

#define CHECKSUM_FADE	0xb5aa125f // fade

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PauseMusic | 
bool ScriptPauseMusic( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	int pausePlease = 1;
	pParams->GetInteger( NONAME, &pausePlease );
	Pcm::PauseMusic( pausePlease );
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PauseStream | 
// @uparm 1 | 0 to unpause, 1 to pause
bool ScriptPauseStream( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	int pausePlease = 1;
	pParams->GetInteger( NONAME, &pausePlease );
	Pcm::PauseStream( pausePlease );
	return ( true );
}
		   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadMusicHeader | parameter is name of header/wad file (minus extension) 
// Do this once on startup. 
// Example:  LoadMusicHeader "music\music"
// @uparm "string" | name of header/wad file
bool ScriptLoadMusicHeader( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	const char *pName = NULL;
	pParams->GetText( NONAME, &pName );
	if ( pName )
	{
		// Mick:  Moved to BottomUpHeap, as it is a lot bigger now, and only loaded at startup
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
		bool done =  ( Pcm::LoadMusicHeader( pName ) );
		Mem::Manager::sHandle().PopContext();
		return done;
		
	}
	return ( false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadStreamHeader | 
// @uparm "string" | name of header/wad file (minus extension)
bool ScriptLoadStreamHeader( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	const char *pName = NULL;
	pParams->GetText( NONAME, &pName );
	if ( pName )
	{
		// Mick:  Moved to BottomUpHeap, as it is a lot bigger now, and only loaded at startup
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
		bool done = ( Pcm::LoadStreamHeader( pName ) );
		Mem::Manager::sHandle().PopContext();
		return done;
	}
	return ( false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StreamIsAvailable | only available in the skateshop
// @parm int | | which stream 
bool ScriptStreamIsAvailable( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	if ( !skate_mod->GetGameMode()->IsFrontEnd() )
	{
		Dbg_MsgAssert( 0, ( "StreamIsAvailable script function is only valid in the skateshop." ) );
	}
	int whichStream;
	if ( pParams->GetInteger( NONAME, &whichStream ) )
	{
		if ( whichStream >= Pcm::GetNumStreamsAvailable( ) )
		{
			Dbg_MsgAssert( 0, ( "\n%s\nAsking for StreamIsAvailable on stream %d, past valid range ( 0 to %d ).", pScript->GetScriptInfo( ),  whichStream, Pcm::GetNumStreamsAvailable( ) - 1 ) );
			return ( false );
		}
		return ( Pcm::StreamAvailable( whichStream ) );
	}
	else
	{
		return ( Pcm::StreamAvailable( ) );
	}
}

#define CHECKSUM_FLAG_PERM	0x389129e4	// "FLAG_PERM"

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AddMusicTrack | adds track to list - plays randomly during game <nl>
// Note: The songs that go into the data/music folder (and are added to the
// CD in them 'music' directory) as well as the ambient tracks in data/ambient
// (and in 'ambient' directory on CD) should have a filename of 8 characters
// or less, as otherwise the names get converted upon building a CD
// @uparmopt "string" | filename in quotes ( .wav file in data/music ) 
// @uparmopt name | filename 
// @flag flag_perm | to add the song to the permanent soundtrack list on startup...
// otherwise track is a level-specific ambient type. <nl>
bool ScriptAddMusicTrack( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	const char *pTrackName = NULL;
	const char *pTrackTitle = NULL;
	// default to ambient tracks...
	pParams->GetText( NONAME, &pTrackName );
	pParams->GetText( "TrackTitle", &pTrackTitle );
	int whichList = Pcm::TRACKLIST_LEVEL_SPECIFIC;
	if ( pParams->ContainsFlag( CHECKSUM_FLAG_PERM ) )
	{
		whichList = Pcm::TRACKLIST_PERM;
	}
	
	if ( pTrackName )
	{
		Pcm::AddTrackToPlaylist( pTrackName, whichList, pTrackTitle );
	}
	return ( true );
}


// @script | ChangeTrackState | Disables or enables one of the permanent music tracks.
// @uparmopt 0 | The index of the track
// @flag Off | Disable the specified track. If omitted, it will enable the track.
bool ScriptChangeTrackState(Script::CStruct *pParams, Script::CScript *pScript)
{
	int track_num=0;
	pParams->GetInteger(NONAME,&track_num);
	
	Pcm::SetTrackForbiddenStatus(track_num,pParams->ContainsFlag("Off"),Pcm::TRACKLIST_PERM);
	return true;
}
	
// @script | TrackEnabled | Returns true if the specified track in the permanent track list is enabled.
// @uparmopt 0 | The index of the track
bool ScriptTrackEnabled(Script::CStruct *pParams, Script::CScript *pScript)
{
	int track_num=0;
	pParams->GetInteger(NONAME,&track_num);

	if (Pcm::GetNumTracks(Pcm::TRACKLIST_PERM) == 0)
	{
		return false;
	}	
	
	Dbg_MsgAssert( track_num>=0 && track_num < Pcm::GetNumTracks(Pcm::TRACKLIST_PERM),( "\n%s\nBad track number of %d sent to TrackEnabled, num tracks = %d", pScript->GetScriptInfo(), track_num, Pcm::GetNumTracks(Pcm::TRACKLIST_PERM) ));
	
	uint64 list1,list2;
    Pcm::GetPlaylist(&list1, &list2);
    
    if (track_num<64)
    {
        if (list1 & (((uint64)1) << track_num))
        {
            return false;
        }
    }        
    else
    {
        if (list2 & (((uint64)1) << (track_num-64)))
        {
            return false;
        }
    }
	
	return true;	
}

bool ScriptMusicIsPaused(Script::CStruct *pParams, Script::CScript *pScript)
{
	return Pcm::MusicIsPaused();
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ClearMusicTrackList | 
// @flag FLAG_PERM | optionally add FLAG_PERM to clear 
// the permanent list, otherwise clear level specific list
bool ScriptClearMusicTrackList( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	int whichList = Pcm::TRACKLIST_LEVEL_SPECIFIC;
	if ( pParams->ContainsFlag( CHECKSUM_FLAG_PERM ) )
	{
		whichList = Pcm::TRACKLIST_PERM;
	}
	Pcm::ClearPlaylist( whichList );
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SkipMusicTrack | 
// Play next track in the list ( or if in random mode, pick another random track )
bool ScriptSkipMusicTrack( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	Pcm::SkipMusicTrack( );
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetMusicMode |
// Play music or ambient sounds?
// @uparm 1 | 1 for music on, 0 for music off/ ambience on
bool ScriptSetMusicMode( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	int musicOn = 1;  // 1 for music on, 0 for music off/ambience on
	pParams->GetInteger( NONAME, &musicOn );
	Pcm::SetActiveTrackList( musicOn ? Pcm::TRACKLIST_PERM : Pcm::TRACKLIST_LEVEL_SPECIFIC );
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetRandomMode | 
// Play tracks in Random mode or play tracks in order.
bool ScriptSetRandomMode( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	int randomModeOn = 1;
	pParams->GetInteger( NONAME, &randomModeOn );
	Pcm::SetRandomMode( randomModeOn );
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetMusicLooping | 
// Loops playing track.
// @uparm 1 | 1 enables looping, 0 disables looping
bool ScriptSetMusicLooping( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	int loopingModeOn = 1;
	pParams->GetInteger( NONAME, &loopingModeOn );
	Pcm::SetLoopingMode( loopingModeOn );
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetCurrentTrack | 
// Gets the index of the currently playing music track.
bool ScriptGetCurrentTrack( Script::CStruct *pParams, Script::CScript *pScript )
{
    int track = Pcm::GetCurrentTrack();
    pScript->GetParams()->AddInteger( "current_track", track );
    return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PlayMovie | Play a movie from data\movies on PC, or \movies on CD
// @uparm "string" | movie name
bool ScriptPlayMovie( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	const char *pMovieName;


	if ( pParams->GetText( NONAME, &pMovieName, true ) )
	{
		Flx::PlayMovie( pMovieName );
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnloadAnim | 
// @parm name | descChecksum | asset name
// @parmopt name | refChecksum | 0 | skeleton name
bool ScriptUnloadAnim( Script::CStruct *pParams, Script::CScript *pScript )
{
	// get the assman
	Ass::CAssMan * ass_man = Ass::CAssMan::Instance();

	const char* pAnimName = NULL;
	pParams->GetText( "name", &pAnimName, Script::ASSERT );

	Ass::CAsset* pAsset = ass_man->GetAssetNode( Script::GenerateCRC(pAnimName), false );

	if ( pAsset )
	{
		ass_man->DestroyReferences( pAsset );
		ass_man->UnloadAsset( pAsset );
		
//		Dbg_Message( "Unloaded asset %s", 
//					 pAnimName );
	}
	else
	{
//		Dbg_Message( "Couldn't find asset %s to unload", 
//					 pAnimName );
	}


#if 0
	// can't do it this way, because it 
	// deletes the reference,
	// not the actual asset itself...

	uint32 animName = 0;
	uint32 refChecksum = 0;

	pParams->GetChecksum( "descChecksum", &animName, Script::ASSERT );

	//printf("Called UnloadAnim on %s\n",Script::FindChecksumName(animName));
	
	if ( !pParams->GetChecksum( "refChecksum", &refChecksum, Script::NO_ASSERT ) )
	{
		refChecksum = ass_man->GetReferenceChecksum();
	}

	// make sure the reference checksum is not 0,
	// if a reference checksum was not specified	
	Dbg_MsgAssert( refChecksum != 0, ( "No ref checksum" ) );

	// kills the original asset
	Ass::CAsset* pAsset = ass_man->GetAssetNode( refChecksum + animName, false );
	if ( pAsset )
	{
		ass_man->DestroyReferences( pAsset );
		ass_man->UnloadAsset( pAsset );
#ifdef __NOPT_ASSERT__
		Dbg_Message( "Unloaded asset %s %s", 
					 Script::FindChecksumName( refChecksum ),
					 Script::FindChecksumName( animName ) );
#endif
	}
	else
	{
#ifdef __NOPT_ASSERT__
		Dbg_Message( "Couldn't find asset %s %s to unload", 
					 Script::FindChecksumName( refChecksum ),
					 Script::FindChecksumName( animName ) );
#endif
	}
#endif

   return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadAsset | 
// @parm string | name | asset name
bool ScriptLoadAsset( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Ass::CAssMan * ass_man =  Ass::CAssMan::Instance();

	const char* p_asset_filename;
	if ( pParams->GetText( (const char*)"name", &p_asset_filename, false )
		 || pParams->GetText( NONAME, &p_asset_filename, true ) )
	{
		// either (name="XXX") or ("XXX") is required
	}

#ifdef __USER_GARY__
//	Dbg_Message( "*** Loading asset %s, perm=%d\n", p_asset_filename, ass_man->GetDefaultPermanent() );
#endif

//	if( !ass_man->LoadOrGetAsset( p_asset_filename, false, ass_man->GetDefaultPermanent() ) )
	if( !ass_man->LoadOrGetAsset( p_asset_filename, false, false, ass_man->GetDefaultPermanent(), 0, NULL, pParams ) )
	{
		if ( Script::GetInteger( CRCD(0x25dc7904,"AssertOnMissingAssets") ) )
		{
			Dbg_MsgAssert( false,( "Could not load asset %s", p_asset_filename ));
		}
		else
		{
			return true;
		}
	}																			 

	uint32 descChecksum = 0;
	if ( pParams->GetChecksum( (const char*)"desc", &descChecksum, false )
		 || pParams->GetChecksum( NONAME, &descChecksum, false ) )
	{
		// either (desc=nnn) or (nnn) is required
		ass_man->AddRef(p_asset_filename, descChecksum, 0);	 
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadAnim | loads the specified animation
// @parm string | name | the file name of the animation
// @parm string | desc | description
// @flag async | load animation asynchronously
// returns true if a reference is generated.  false if a reference already exists
bool ScriptLoadAnim( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Ass::CAssMan * ass_man =  Ass::CAssMan::Instance();

	// an anim is just like an asset, except it has a reference added to it
		
	const char* p_asset_filename;
	uint32 descChecksum = 0;
	pParams->GetText( CRCD(0xa1dc81f9,"name"), &p_asset_filename );

	pParams->GetChecksum( CRCD(0x4d20d794,"descChecksum"), &descChecksum, true );

	bool async = false;
	if( pParams->ContainsFlag( CRCD(0x90e07c79,"async") ))
	{
		async = true;
	}

	bool use_pip = false;
	if( pParams->ContainsFlag( CRCD(0x23028e29,"use_pip") ))
	{
		use_pip = true;
	}

	return ass_man->LoadAnim( p_asset_filename, descChecksum, ass_man->GetReferenceChecksum(), async, use_pip );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadSkeleton | loads the specified skeleton
// @parm string | name | the file name of the skeleton
// @parm string | desc | description
bool ScriptLoadSkeleton( Script::CStruct *pParams, Script::CScript *pScript )
{
	Ass::CAssMan* ass_man =  Ass::CAssMan::Instance();

	const char* nameBuf;
	pParams->GetText( (const char*)"name", &nameBuf, Script::ASSERT );

	uint32 skeletonName;
	pParams->GetChecksum( NONAME, &skeletonName, Script::ASSERT );
	
//    Gfx::CSkeletonData* pSkeletonData = (Gfx::CSkeletonData*)ass_man->LoadOrGetAsset(nameBuf, false, ass_man->GetDefaultPermanent());
    Gfx::CSkeletonData* pSkeletonData = (Gfx::CSkeletonData*)ass_man->LoadOrGetAsset(nameBuf, false, false, ass_man->GetDefaultPermanent(), 0);
	if ( !pSkeletonData )
	{
		Dbg_MsgAssert(0,("Failed to load skeleton %s",nameBuf));
	}

	// Now add the reference to it, if one was requested
	// this gets combined with the "reference checksum"
	if (!ass_man->AssetLoaded(skeletonName))
	{
		ass_man->AddRef(nameBuf, skeletonName, 0);	 
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptAssManSetReferenceChecksum( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Ass::CAssMan * ass_man =  Ass::CAssMan::Instance();

	uint32 checksum;
	if ( !pParams->GetChecksum( NONAME, &checksum ) )
	{
		pParams->GetInteger( NONAME, (int*)&checksum, true );
	}

	ass_man->SetReferenceChecksum( checksum );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptAssManSetDefaultPermanent( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Ass::CAssMan * ass_man =  Ass::CAssMan::Instance();

	int permanent;
	pParams->GetInteger( NONAME, &permanent, true );

	ass_man->SetDefaultPermanent( permanent );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#define CHECKSUM_PITCH		0xd8604126	// "pitch"
#define CHECKSUM_DROPOFF	0xff2020ec	// "dropoff"
#define CHECKSUM_VOLUME		0xf6a36814	// "vol"

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadSound | 
// @uparm "string" | The name of the sound file
// @parmopt float | pitch | 100.0 | 
// @parmopt float | volume | 100.0 | 
// @parmopt float | dropoff | 0.0 | 
// @flag PosUpdateWithDoppler | positional update with doppler
// @flag NoReverb | reverb effect disabled for this sound
bool ScriptLoadSound( Script::CStruct *pParams, Script::CScript *pScript )
{
	const char *pSfxName;
	if ( pParams->GetText( NONAME, &pSfxName, true ) )
	{
		float volume = 100.0f;
		float pitch = 100.0f;
		float dropoff = 0.0f;
		int flags = 0;
		pParams->GetFloat( CHECKSUM_PITCH, &pitch );
		if ( pParams->GetFloat( CHECKSUM_DROPOFF, &dropoff ) )
		{
			dropoff = FEET_TO_INCHES( dropoff );
		}
		pParams->GetFloat( CHECKSUM_VOLUME, &volume );
		
		if ( pParams->ContainsFlag( CHECKSUM_FLAG_PERM ) )
		{
			flags |= SFX_FLAG_LOAD_PERM;
		}
		if ( pParams->ContainsFlag( 0xcf15c120 ) ) //"PosUpdateWithDoppler"
		{
			flags |= SFX_FLAG_POSITIONAL_UPDATE_WITH_DOPPLER;
		}
		else if ( pParams->ContainsFlag( CRCD(0x8b87176f,"NoReverb") ) )
		{
			flags |= SFX_FLAG_NO_REVERB;
		}
		//else if ( pParams->ContainsFlag( 0x14f9c4fe ) ) // "PosUpdate"
		//{
		//	flags |= SFX_FLAG_POSITIONAL_UPDATE;
		//}
			
		 Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
		if ( !sfx_manager->LoadSound( pSfxName, flags, dropoff, pitch, volume ) )
		{
//			printf( "sound %s couldn't load\n", pSfxName );
		}
	}
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PlayStream | 
// @uparm name | stream name
// @parmopt float | volume | 100.0 | volume of both left and right channel in percent (overrides volL and volR)
// @parmopt float | volL | 100.0 | volume of left channel
// @parmopt float | volR | 100.0 | volume of right channel
// @parmopt float | pitch | 100.0 | pitch value
// @parmopt int | priority | 50 | priority; higher priority streams can stop lower priority streams.
// @parmopt name | id | 0 | control ID (used instead of stream name for script control purposes)
// @parmopt int | lipsync | 0 | set to 1 to load lipsync data
bool ScriptPlayStream( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	uint32 streamNameChecksum = 0;
	if ( pParams->GetChecksum( NONAME, &streamNameChecksum ) )
	{
		float volume = 100.0f;
		float volL = 100.0f;
		float volR = 100.0f;
		float pitch = 100.0f;
		int priority = STREAM_DEFAULT_PRIORITY;
		uint32 id = 0;
		int lipsync = 0;
		pParams->GetFloat( "volL", &volL );
		pParams->GetFloat( "volR", &volR );
		if (pParams->GetFloat( CHECKSUM_VOLUME, &volume ))
		{
			volL = volume;
			volR = volume;
		}
		pParams->GetFloat( CHECKSUM_PITCH, &pitch );
		pParams->GetInteger( 0x9d5923d8, &priority ); // priority
		pParams->GetChecksum( 0x40c698af, &id ); // id
		pParams->GetInteger( CRCD(0xf5c3922b,"lipsync") , &lipsync );

		if (lipsync == 1)
		{
			bool loaded = Pcm::CStreamFrameAmpManager::sLoadFrameAmp(streamNameChecksum);
			if (!loaded)
			{
				Dbg_MsgAssert( 0,( "Couldn't load lipsync data for stream %s", Script::FindChecksumName(streamNameChecksum)));
				return false;
			}
		}

		Sfx::sVolume	vol( Sfx::VOLUME_TYPE_BASIC_2_CHANNEL );
		vol.SetChannelVolume( 0, volL );
		vol.SetChannelVolume( 1, volR );
		Pcm::PlayStream( streamNameChecksum, &vol, pitch, priority, id );

		return ( true );
	}
	Dbg_MsgAssert( 0,( "\n%s\nMust specify stream name.", pScript->GetScriptInfo( ) ));
	return ( false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StopStream | stops current stream from playing
// @uparm name | stream name (or control ID, if one was supplied with PlayStream)
bool ScriptStopStream( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	uint32 id;
	if (pParams->GetChecksum( NONAME, &id ))
	{
		Pcm::StopStreamFromID( id );
	}
	else
	{
		Pcm::StopStreams( -1 );		// Stop all streams
	}

	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsStreamPlaying | Returns true if the named stream is currently playing
// @uparm name | stream name (or control ID, if one was supplied with PlayStream)
bool ScriptIsStreamPlaying( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 id = 0;
    
	if (pParams->GetChecksum( NONAME, &id ))
	{
		return Pcm::StreamPlaying(id);
	}
	else
	{
		Dbg_MsgAssert(0, ("No stream specified"));
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetStreamParams | Set the volume or pitch of a playing stream
// @uparm name | stream name (or control ID, if one was supplied with PlayStream)
// @parmopt float | vol | 100.0 | volume of both left and right channel (overrides volL and volR)
// @parmopt float | volL | 100.0 | volume of left channel
// @parmopt float | volR | 100.0 | volume of right channel
// @parmopt float | pitch | 100.0 | pitch
bool ScriptSetStreamParams( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 id = 0;
    
	if (pParams->GetChecksum( NONAME, &id ))
	{
		float Volume;
		float VolL;
		float VolR;
		float Pitch;

		bool adjust_vol;

		adjust_vol = pParams->GetFloat( CRCD(0x5e285a66,"VolL"), &VolL );
		adjust_vol = pParams->GetFloat( CRCD(0xa4276705,"VolR"), &VolR ) || adjust_vol;
		if (pParams->GetFloat( CRCD(0xf6a36814,"Vol"), &Volume ))
		{
			VolL = Volume;
			VolR = Volume;
			adjust_vol = true;
		}

		bool result = true;

		if (adjust_vol)
		{
			Sfx::sVolume vol;
			vol.SetSilent();
			vol.SetChannelVolume( 0, VolL );
			vol.SetChannelVolume( 1, VolR );

			result = Pcm::SetStreamVolumeFromID(id, &vol);
		}

		if (pParams->GetFloat( CRCD(0xd8604126,"Pitch"), &Pitch ))
		{
			result = Pcm::SetStreamPitchFromID(id, Pitch) && result;
		}

		if (!result)
		{
			Dbg_MsgAssert(0, ("Can't find stream %s", Script::FindChecksumName(id)));
		}

		return result;
	}
	else
	{
		Dbg_MsgAssert(0, ("No stream specified"));
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PlayTrack | 
// @uparm "string" | filename in quotes (no extension, file should be a 
// .wav file in data/music)
// @uparmopt 0 | The index of the track. For use if the name is not known.
// @parmopt int | loop | 0 | continuously play the track if set to non-zero
bool ScriptPlayTrack( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	const char *songName = NULL;
	char p_calculated_track_name[MAX_TRACKNAME_STRING_LENGTH];
    int track_num=999;
	int loop = 0;
	
    if (!pParams->GetText( NONAME, &songName ))
	{
        pParams->GetInteger(NONAME,&track_num);
        songName=Pcm::GetTrackName(track_num,Pcm::TRACKLIST_PERM);
        
		Dbg_MsgAssert(strlen("MUSIC\\VAG\\SONGS\\")+strlen(songName)<MAX_TRACKNAME_STRING_LENGTH,("Track name '%s' too long",songName));
		sprintf(p_calculated_track_name,"MUSIC\\VAG\\SONGS\\%s",songName);
		songName=p_calculated_track_name;
	}	

	pParams->GetInteger(CRCD(0x5ea0e211,"loop"), &loop);

    if ( songName )
	{
		printf("PlayTrack songName ===================== %s\n", songName);
        Pcm::StopMusic( );
        Pcm::PlayTrack( songName, loop );
        Pcm::SetCurrentTrack( track_num );
	}
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PlayMusicStream | Plays a stereo stream through the music channel
// @uparm name | music stream name (in music.wad file)
// @parmopt float | volume | -1.0 | volume in percent; negative value uses music volume
bool ScriptPlayMusicStream( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	uint32 streamNameChecksum = 0;
	if ( pParams->GetChecksum( NONAME, &streamNameChecksum ) )
	{
		float volume = -1.0f;
		pParams->GetFloat( CHECKSUM_VOLUME, &volume );
		Pcm::PlayMusicStream( streamNameChecksum, volume );
		return ( true );
	}
	Dbg_MsgAssert( 0,( "\n%s\nMust specify stream name.", pScript->GetScriptInfo( ) ));
	return ( false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadStreamFrameAmp | Loads stream frame amplitude data for lipsync
// @uparm name | stream name
bool ScriptLoadStreamFrameAmp( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 streamNameChecksum = 0;
	if ( pParams->GetChecksum( NONAME, &streamNameChecksum ) )
	{
		return Pcm::CStreamFrameAmpManager::sLoadFrameAmp(streamNameChecksum);
	}
	Dbg_MsgAssert( 0,( "\n%s\nMust specify stream name.", pScript->GetScriptInfo( ) ));
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | FreeStreamFrameAmp | Frees stream frame amplitude data for lipsync
// @uparm name | stream name
bool ScriptFreeStreamFrameAmp( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 streamNameChecksum = 0;
	if ( pParams->GetChecksum( NONAME, &streamNameChecksum ) )
	{
		return Pcm::CStreamFrameAmpManager::sFreeFrameAmp(streamNameChecksum);
	}
	Dbg_MsgAssert( 0,( "\n%s\nMust specify stream name.", pScript->GetScriptInfo( ) ));
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PlaySound | 
// @uparm name | sound name
// @parmopt float | vol | 100.0 | volume of both left and right channel (overrides volL and volR)
// @parmopt float | volL | 100.0 | volume of left channel
// @parmopt float | volR | 100.0 | volume of right channel
// @parmopt float | pitch | 100.0 | 
// @parmopt name | id | 0 | control ID (used instead of sound name for script control purposes)
bool ScriptPlaySound(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 SoundChecksum = 0;
	uint32 id = 0;
	float Volume = 100.0f;
	float VolL = 100.0f;
	float VolR = 100.0f;
	float Pitch = 100.0f;
    
	pParams->GetChecksum( NONAME, &SoundChecksum );
	pParams->GetFloat( "VolL", &VolL );
	pParams->GetFloat( "VolR", &VolR );
	if (pParams->GetFloat( "Vol", &Volume ))
	{
		VolL = Volume;
		VolR = Volume;
	}
	pParams->GetFloat( "Pitch", &Pitch );
	pParams->GetChecksum( CRCD(0x40c698af,"id"), &id );

	Dbg_MsgAssert( SoundChecksum,( "PlaySound requires the name of the sound" ));
	
	Sfx::sVolume vol;
	vol.SetSilent();
	vol.SetChannelVolume( 0, VolL );
	vol.SetChannelVolume( 1, VolR );
	
	Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
	if( !sfx_manager->PlaySound( SoundChecksum, &vol, Pitch, id ))
	{
#ifdef __NOPT_ASSERT__
		printf( "failed to play sound %s", Script::FindChecksumName( SoundChecksum ) );
#endif	
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StopSound | 
// @uparm name | sound name (or control ID, if one was supplied with PlaySound)
bool ScriptStopSound( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 id;
	if (pParams->GetChecksum( NONAME, &id ))
	{
		Sfx::CSfxManager* sfx_manager = Sfx::CSfxManager::Instance();
		bool result = sfx_manager->StopSound(id);

		if (!result)
		{
			Dbg_Message( "Couldn't find sound %s to stop", Script::FindChecksumName( id ) );
		}

		return result;
	}
	else
	{
		Dbg_MsgAssert(0, ("No sound specified"));
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StopAllSounds | 
bool ScriptStopAllSounds(Script::CStruct *pParams, Script::CScript *pScript)
{
	Sfx::CSfxManager* sfx_manager = Sfx::CSfxManager::Instance();
    sfx_manager->StopAllSounds();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSoundParams | Set the volume and pitch of a playing sound
// @uparm name | sound name (or control ID, if one was supplied with PlaySound)
// @parmopt float | vol | 100.0 | volume of both left and right channel (overrides volL and volR)
// @parmopt float | volL | 100.0 | volume of left channel
// @parmopt float | volR | 100.0 | volume of right channel
// @parmopt float | pitch | 100.0 | pitch of sound
bool ScriptSetSoundParams( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 id = 0;
    
	if (pParams->GetChecksum( NONAME, &id ))
	{
		float Volume;
		float VolL = 100.0f;
		float VolR = 100.0f;
		float Pitch = 100.0f;

		bool adjust_vol;

		adjust_vol = pParams->GetFloat( CRCD(0x5e285a66,"VolL"), &VolL );
		adjust_vol = pParams->GetFloat( CRCD(0xa4276705,"VolR"), &VolR ) || adjust_vol;
		if (pParams->GetFloat( CRCD(0xf6a36814,"Vol"), &Volume ))
		{
			VolL = Volume;
			VolR = Volume;
			adjust_vol = true;
		}

		if (pParams->GetFloat( CRCD(0xd8604126,"Pitch"), &Pitch ) && !adjust_vol)
		{
			Dbg_MsgAssert(0, ("Must set volume when setting pitch"));

		}

		Sfx::sVolume vol;
		vol.SetSilent();
		vol.SetChannelVolume( 0, VolL );
		vol.SetChannelVolume( 1, VolR );

		Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
		if( !sfx_manager->SetSoundParams( id, &vol, Pitch ))
		{
	#ifdef __NOPT_ASSERT__
			Dbg_MsgAssert(0, ("Can't find sound %s", Script::FindChecksumName(id)));
	#endif // __NOPT_ASSERT__
		}
	}
	else
	{
		Dbg_MsgAssert(0, ("No sound specified"));
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsSoundPlaying | Returns true if the named sound is currently playing
// @uparm name | sound name (or control ID, if one was supplied with PlaySound)
bool ScriptIsSoundPlaying( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 id = 0;
    
	if (pParams->GetChecksum( NONAME, &id ))
	{
		Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
		return sfx_manager->SoundIsPlaying(id);
	}
	else
	{
		Dbg_MsgAssert(0, ("No sound specified"));
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSfxReverb | 1st param: reverbLevel <nl>
// default 0 (will turn reverb off) <nl>
// range: 0 to 100 percent <nl>
// 2nd param: mode (see possible values in reverb.q) <nl>
// default is 0 (first value in reverb.q) <nl> <nl>
// Notes: 2nd param (mode) only matters if 1st param (reverbLevel) is 
// above 0. On a script Cleanup command, the global reverb is turned off.
// @uparmopt 0.0 | Reverb (cannot be greater than 100)
// @uparmopt name | reverb mode
bool ScriptSetReverb( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	float reverb = 0.0f;
	int reverbMode = 0;

	// if the parameter isn't found, it should default to zero!	
	pParams->GetFloat( NONAME, &reverb );
	if ( reverb > 100.0f )
	{
		reverb = 100.0f;
		Dbg_Message( "Reverb greater than 100.  Clipping." );
	}
	
	// if the parameter isn't found, it should default to zero!	
	pParams->GetInteger( CRCD(0x6835b854,"mode"), &reverbMode );
	
	 Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
	sfx_manager->SetReverb( reverb, reverbMode, pParams->ContainsFlag( "instant" ) );
	
	return ( true );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSfxDropoff | SetSfxDropoff [100] <nl>
// Param: dropoff distance (in feet) <nl>
// range: anything above zero <nl>
// default is 100 <nl> <nl>
// Notes: only affects positional soundfx. Also, the global
// dropoff distance is set back to 100 (or whatever you want me
// to set it to) on a script Cleanup command. That way, if somebody 
// forgets to call SetSfxDropoff at the beginning of their level, it 
// will always be this default instead of staying at whatever level
// the previously loaded level had it set to
// @uparm 1.0 | Dropoff distance
bool ScriptSetDropoff( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	float dropoffDist = DEFAULT_DROPOFF_DIST;

	pParams->GetFloat( NONAME, &dropoffDist );
	if ( dropoffDist <= 0.0f )
	{
		Dbg_MsgAssert( 0.0f,( "Can't have dropoff zero or less." ));
		return ( false );
	}
	
	 Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
	sfx_manager->SetDefaultDropoffDist( FEET_TO_INCHES( dropoffDist ) );
	
	return ( true );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/



void	set_all_colors(Nx::CScene * p_scene, Image::RGBA rgb, uint32 light_group)
{
	Lst::HashTable< Nx::CSector > * p_sector_list = p_scene->GetSectorList();
	

	
	if (p_sector_list)
	{
		p_sector_list->IterateStart();	
		Nx::CSector *p_sector = p_sector_list->IterateNext();		
		if (light_group)
		{
			while(p_sector)
			{
				if (p_sector->GetLightGroup() == light_group)
				{
					p_sector->SetColor(rgb);
				}
				p_sector = p_sector_list->IterateNext();
			}
		}
		else
		{
			while(p_sector)
			{
				p_sector->SetColor(rgb);
				p_sector = p_sector_list->IterateNext();
			}
		}
	}
}


// Given a value, a target and a range, then scale the distance from the target by the amount
// so, 
float compress_value(float value, float target, float amount)
{
	float v =  target + (value-target) * amount;
	if (v<0.0f) v = 0.0f;
	if (v>255.0f) v = 255.0f;
	return v;
}

void	compress_all_vertex_colors(Nx::CScene * p_scene, float target, float amount, uint32 light_group)
{
	Lst::HashTable< Nx::CSector > * p_sector_list = p_scene->GetSectorList();
	
	Image::RGBA rgb;
	
	
	if (p_sector_list)
	{
		p_sector_list->IterateStart();	
		Nx::CSector *p_sector = p_sector_list->IterateNext();		
		while(p_sector)
		{

			Nx::CGeom 	*p_geom = p_sector->GetGeom();
//			Dbg_MsgAssert(p_geom,("sector does not have geometry"))
			
			if (p_geom && (!light_group || light_group == p_sector->GetLightGroup()))
			{
				//
				// Do renderable geometry
				int verts = p_geom->GetNumRenderVerts();
			
				if (verts)
				{
	//				Mth::Vector	*p_verts = new Mth::Vector[verts];
					Image::RGBA	*p_colors = new Image::RGBA[verts];
	//				p_geom->GetRenderVerts(p_verts);
					//p_source_geom->GetRenderColors(p_colors);
					
					// Note: getting the original render colors will allocate lots of memory
					// to store all the origianl colors the firs tiem it is called
					p_geom->GetOrigRenderColors(p_colors);
				
					Image::RGBA *p_color = p_colors;
					if (target != 0.0 || amount != 0.0)
					{
						for (int i = 0; i < verts; i++)
						{
							rgb.a = p_color->a; 
							rgb.r =  (uint8) compress_value((float) p_color->r,target,amount);						
							rgb.g =  (uint8) compress_value((float) p_color->g,target,amount);						
							rgb.b =  (uint8) compress_value((float) p_color->b,target,amount);						
							*p_color = rgb;		//(*p_color & 0xff000000) | color;
							p_color++;
						} // end for
					}
			
					// Set the colors on the actual new geom, not on the source...		
					p_geom->SetRenderColors(p_colors);
					
	//				delete [] p_verts;
					delete [] p_colors;
				} // end if
				else
				{
					// debuggery
					//p_geom->SetColor(Image::RGBA(0,0,100,0));
				}
			}
			p_sector = p_sector_list->IterateNext();
		}
	}
}



void	nudge_vertex_colors(Nx::CScene * p_scene, float amount)
{
	Lst::HashTable< Nx::CSector > * p_sector_list = p_scene->GetSectorList();
	
	Image::RGBA rgb;
	
	if (p_sector_list)
	{
		p_sector_list->IterateStart();	
		Nx::CSector *p_sector = p_sector_list->IterateNext();		
		while(p_sector)
		{

			Nx::CGeom 	*p_geom = p_sector->GetGeom();
//			Dbg_MsgAssert(p_geom,("sector does not have geometry"))
			
			if (p_geom)
			{
				//
				// Do renderable geometry
				int verts = p_geom->GetNumRenderVerts();
			
				if (verts)
				{
					Mth::Vector	*p_verts = new Mth::Vector[verts];
					Image::RGBA	*p_colors = new Image::RGBA[verts];
					p_geom->GetRenderVerts(p_verts);
					//p_source_geom->GetRenderColors(p_colors);
					
					// Note: getting the original render colors will allocate lots of memory
					// to store all the origianl colors the firs tiem it is called
					p_geom->GetOrigRenderColors(p_colors);
				
					Image::RGBA *p_color = p_colors;
					Mth::Vector *p_vert = p_verts;
					for (int i = 0; i < verts; i++)
					{
			//			CalculateVertexLighting(*p_vert, *p_color);
						
						rgb.a = p_color->a; 

						Mth::Vector col;
						col.Set(p_color->r, p_color->g, p_color->b);
					
											 
						// create a pseudo random number based on the vertex position
						// so non-common verts in the same position will get the same result
						// which is the only way you can get a smooth result							 
						int i_random =  (int)((*p_vert)[X]*16.0f) * (int)((*p_vert)[Y]*16.0f) ^ (int)((*p_vert)[Z]*16.0f);
						float r1 = (float)(i_random & 0x7fff) / 32768.0f;
						
						float	nudge = 1.0f + ( 2.0f * r1 * amount ) - amount;
						
						col *= 	nudge;
						if (col[X] < 0.0f) col[X] = 0.0f;
						if (col[X] > 255.0f) col[X] = 255.0f;
						if (col[Y] < 0.0f) col[Y] = 0.0f;
						if (col[Y] > 255.0f) col[Y] = 255.0f;
						if (col[Z] < 0.0f) col[Z] = 0.0f;
						if (col[Z] > 255.0f) col[Z] = 255.0f;
						
						rgb.r = (uint8)col[X];
						rgb.g = (uint8)col[Y];
						rgb.b = (uint8)col[Z];
						 
						*p_color = rgb;		//(*p_color & 0xff000000) | color;
						//*(uint32*)p_color = Mth::Rnd(32767);		//(*p_color & 0xff000000) | color;
			
						p_color++;
						p_vert++;
					} // end for
			
					// Set the colors on the actual new geom, not on the source...		
					p_geom->SetRenderColors(p_colors);
					
					delete [] p_verts;
					delete [] p_colors;
				} // end if
				else
				{
					// debuggery
					//p_geom->SetColor(Image::RGBA(0,0,100,0));
				}
			}
			p_sector = p_sector_list->IterateNext();
		}
	}
}




bool ScriptSetSceneColor( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	uint32 color;
	
	pParams->GetInteger( "color", (int*)&color, TRUE );
	uint32 sky = color;
	pParams->GetInteger( "sky", (int*)&sky );
	uint32 light_group =0;
	pParams->GetChecksum( "lightgroup", (uint32*)&light_group );	
	Nx::CScene	*p_scene = Nx::CEngine::sGetMainScene();
	
	Image::RGBA rgb;
	rgb.r = (uint8)((color)&0xff);
	rgb.g = (uint8)((color>>8)&0xff);
	rgb.b = (uint8)((color>>16)&0xff);
	rgb.a = 0x80;
	
	Image::RGBA sky_rgb;
	sky_rgb.r = (uint8)((sky)&0xff);
	sky_rgb.g = (uint8)((sky>>8)&0xff);
	sky_rgb.b = (uint8)((sky>>16)&0xff);
	sky_rgb.a = 0x80;
	
	
	if (p_scene)
	{
		// For main scene, just set the root color
		#ifdef	__PLAT_NGPS__

		// if doing a global or outdoor change to some color
		// then reset all the colors first		
		// to reset things that were set by individual SetObjectColor  commands
		// like the Chris's best line goal in NJ
		if 	(color != 0x808080 && (light_group == CRCD(0xe3714dc1,"outdoor") || light_group == 0))
		{
			Image::RGBA clear_rgb(0x80,0x80,0x80,0x80);
			set_all_colors(p_scene, clear_rgb, light_group);						
		}
		
		// if not the outdoor or default group, or if we are turing lighting off with 0x808080
		// then set it on all nodes
		// otherwise we just set the root color
		if ((light_group != CRCD(0xe3714dc1,"outdoor") && light_group != 0 ) || color == 0x808080 
			/*|| p_scene->GetID() ==CRCD(0xefc68238,"cloned")*/)
		#endif
		{
			set_all_colors(p_scene, rgb, light_group);
		}
		
		// Set the root color of the scene.  A PS2 specific optimization
		// which can be ignored by other platforms.		
		if (light_group == CRCD(0xe3714dc1,"outdoor") || light_group == 0)
		{
			p_scene->SetMajorityColor(rgb);
		}
	}
	
	// K: If the park editor shell scene exists, update its colours too.
	p_scene=Nx::CEngine::sGetScene(Ed::CParkManager::Instance()->GetShellSceneID());
	if (p_scene)
	{
		// Note, cut and paste from above
		#ifdef	__PLAT_NGPS__
		// if not the outdoor or default group, or if we are turing lighting off with 0x808080
		// then set it on all nodes
		// otherwise we just set the root color
		if ((light_group != CRCD(0xe3714dc1,"outdoor") && light_group != 0 ) || color == 0x808080)
		#endif
		{
			set_all_colors(p_scene, rgb, light_group);
		}
		
		// Set the root color of the scene.  A PS2 specific optimization
		// which can be ignored by other platforms.		
		if (light_group == CRCD(0xe3714dc1,"outdoor") || light_group == 0)
		{
			p_scene->SetMajorityColor(rgb);
		}
	}

	
	p_scene = Nx::CEngine::sGetSkyScene();
	
	if (p_scene)
	{
		set_all_colors(p_scene, sky_rgb, light_group);
	}
	
	// we also need to set the color of all the level objects
	// assume they are all "outdoors"
	if (light_group == CRCD(0xe3714dc1,"outdoor") || light_group == 0)
	{
		Obj::CModelComponent *p_component = static_cast<Obj::CModelComponent*>( Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( CRC_MODEL ));
		while( p_component )
		{
			p_component->ApplySceneLighting(rgb);
			p_component = static_cast<Obj::CModelComponent*>( p_component->GetNextSameType());
		}
	}	
	
	return true;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool ScriptCompressVC( Script::CStruct *pParams, Script::CScript *pScript )
{

	float target = 0.0f;
	pParams->GetFloat( "target", (float*)&target, TRUE );
	float amount = 0.0f;
	pParams->GetFloat( "percent", (float*)&amount );
	amount = (100.0f - amount)/100.0f;
	uint32 light_group =0;
	pParams->GetChecksum( "lightgroup", (uint32*)&light_group );	

	Nx::CScene	*p_scene = Nx::CEngine::sGetMainScene();
	if (p_scene)
	{
		compress_all_vertex_colors(p_scene, target, amount, light_group);
	}
	p_scene = Nx::CEngine::sGetSkyScene();
	if (p_scene)
	{
		compress_all_vertex_colors(p_scene, target, amount, light_group);
	}
	return true;
}

// @script bool | FakeLights | Adjust lighting for a node-based SceneLight.
// @parm name | id | The name of the node, if only one needs adjusting.
// @parm name | Prefix | The prefix of a set of nodes, if lots need adjusting.
// @parmopt integer | Time | 0 | The time period, in frames, over which the execution of the command
// will be spread so as not to use up too much CPU time in one frame.
// @parmopt float | inner_radius | none | The new radius of the light in inches
// @parmopt float | outer_radius | none | The new radius of the light in inches
// @parmopt float | percent | none | The new intensity percentage
// @parmopt integer | red | none | The new red component of the light color (also green, blue)
bool ScriptFakeLights( Script::CStruct *pParams, Script::CScript *pScript )
{
	// If an id is supplied, we want to apply the effect to just one light.
	uint32 id = 0; 
	if( pParams->GetChecksum( CRCD(0x40c698af,"id"), &id ))
	{
		Nx::CLightManager::sFakeLight(id,pParams);
		return true;
	}

	// Get the period in frames over which the command will execute.
	int time_period=0;
	pParams->GetInteger(CRCD(0x906b67ba,"time"),&time_period);
	
	// K: Added this bit to allow a set of nodes with a given prefix to be adjusted.
	uint32 prefix=0;	
    if ( pParams->GetChecksumOrStringChecksum( CRCD(0x6c4e7971,"Prefix"), &prefix ) )
	{
		uint16 num_nodes = 0;
		const uint16 *p_nodes = SkateScript::GetPrefixedNodes( prefix, &num_nodes );
		
		Nx::CLightManager::sFakeLights(p_nodes,num_nodes,time_period,pParams);
		return true;
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool ScriptNudgeVC( Script::CStruct *pParams, Script::CScript *pScript )
{

	float amount = 0.0f;
	pParams->GetFloat( "percent", (float*)&amount, true );
	amount = (amount)/100.0f;
	Nx::CScene	*p_scene = Nx::CEngine::sGetMainScene();
	if (p_scene)
	{
		nudge_vertex_colors(p_scene,amount);
	}
	return true;
}

// Center the viewer camera on the scene
// (Previously this would center the current camera on the scene, which had unintended results)
void	center_camera_on_scene(Nx::CScene * p_scene, float scale, float x_rot, float y_rot, float z_rot, uint32 id=0)
{

	// get a bounding box of the world, and center the camera on that	
	Mth::CBBox bbox;
	
	Lst::HashTable< Nx::CSector > * p_sector_list = p_scene->GetSectorList();
	if (p_sector_list)
	{
		p_sector_list->IterateStart();	
		Nx::CSector *p_sector = p_sector_list->IterateNext();		
		while(p_sector)
		{

			if ((!id || id == p_sector->GetChecksum()) && p_sector->IsActive())
			{
				
				Nx::CGeom 	*p_geom = p_sector->GetGeom();
				if (p_geom)
				{
					//
					// Do renderable geometry
					int verts = p_geom->GetNumRenderVerts();
					if (verts)
					{
						Mth::Vector	*p_verts = new Mth::Vector[verts];
						p_geom->GetRenderVerts(p_verts);
						Mth::Vector *p_vert = p_verts;
						for (int i = 0; i < verts; i++)
						{
							bbox.AddPoint(*p_vert);
							//printf ("%f,%f,%f,%f\n",p_vert[0][X],p_vert[0][Y],p_vert[0][Z],p_vert[0][W]);
//							if (i>0)
//							{
//								Gfx::AddDebugLine(p_vert[0],p_vert[-1],0xffffff,100);
//							}
							p_vert++;
						} // end for
				
						delete [] p_verts;
					} // end if
				}
			}
			p_sector = p_sector_list->IterateNext();
		}
	}


	
	
	// Move the camera to the object, and move it back by the width of the object (min 100 ft)
//    Gfx::Camera* pCam = Nx::CViewportManager::sGetCamera( 0 );

// Mick:  Now we attempt to get the viewer camera object

	Obj::CCompositeObject *p_obj = (Obj::CCompositeObject *)Obj::CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0xeb17151b,"viewer_cam"));
	if (!p_obj)
	{
		return;
	}
	
								 
	Dbg_MsgAssert(GetCameraComponentFromObject(p_obj),("Viewer Camera Object Missing camera component"));
	
	
	Mth::Vector min = bbox.GetMin();
	Mth::Vector max = bbox.GetMax();
	Mth::Vector mid = (min + max)/2.0f;
	float	diagonal = (max - min).Length();
	if (diagonal < 1000)
	{
		diagonal = 1000;
	}


	diagonal *= scale;

	// Note we are moving the parent object, the camera component takes its pos from this
	p_obj->GetMatrix().Ident();
	p_obj->GetMatrix().RotateX(Mth::DegToRad(x_rot));
	p_obj->GetMatrix().RotateY(Mth::DegToRad(y_rot));
	p_obj->GetMatrix().RotateZ(Mth::DegToRad(z_rot));

	// printf ("diagonal = %f, mid = [%f,%f,%f]\nbox = [%f,%f,%f] - [%f,%f,%f]\n",diagonal,mid[X],mid[Y],mid[Z],min[X],min[Y],min[Z],max[X],max[Y],max[Z]  );	

	p_obj->SetPos(mid + scale * diagonal * p_obj->GetMatrix().GetAt());

	
}


// @script | CenterCamera |  Center the camera on the rendered geometry, or an individual object
// @parmopt float | scale | 0.9 | 
// @parmopt float | X | -45 | Angle to rotate about X 
// @parmopt float | Y | 45 | Angle to rotate about Y
// @parmopt float | Z | 0 | Angle to rotate about X
// @parmopt	checksum | id | 0 | checksum of and individual object to center on
// Notes:  The Camera will be centered on the center of the bounding box
// of all exported geometry.  It will be moved back by the size of the diagonal
// of the bounding box, and rotated by the X,Y,Z angles

bool ScriptCenterCamera( Script::CStruct *pParams, Script::CScript *pScript )
{
	// This is only interend to be used in the fly around modes
	// so kill it during menus

	if (!Mdl::CViewer::sGetViewMode())
	{
		return false;
	}


	float scale = 0.9f;
	pParams->GetFloat( "scale", &scale );
	float X = -45.0f;;
	pParams->GetFloat( "x", &X );
	float Y = 45.0f;
	pParams->GetFloat( "y", &Y );
	float Z = 0.0f;
	pParams->GetFloat( "z", &Z );
	uint32 id = 0; 
	pParams->GetChecksum( "id", &id );




	Nx::CScene	*p_scene = Nx::CEngine::sGetMainScene();
	if (p_scene)
	{
		center_camera_on_scene(p_scene, scale, X, Y, Z, id);
	}
	return true;
}





#define CHECKSUM_MAXPITCH			0xfa3e14c5	// maxpitch
#define CHECKSUM_MINPITCH			0x1c5ebb24	// minpitch
#define CHECKSUM_MAXVOL				0x0693daaf	// maxvol
#define CHECKSUM_MINVOL				0x4391992d	// minvol
#define CHECKSUM_TERRAIN			0x3789ac4e  // terrain
#define CHECKSUM_TABLE				0x09d670b9  // table

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadTerrainSounds | Loads all the associated sounds for a terrain type of a level.
// @parm name | terrain | Name of terrain

bool ScriptLoadTerrainSounds( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 terrain_checksum;

	if (!pParams->GetChecksum(CHECKSUM_TERRAIN, &terrain_checksum))
	{
		printf("Expected terrain checksum\n");
		return false;
	}

	ETerrainType terrain = Env::CTerrainManager::sGetTerrainFromChecksum(terrain_checksum);
	Env::CTerrainManager::sLoadTerrainSounds(terrain);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Goto | jumps to the specified script
// @Uparm name | The name of the script 
// @parmopt structure | Params | | parameter list to pass to the new script
bool ScriptGoto(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	uint32 ScriptChecksum=0;
	pParams->GetChecksum(NONAME,&ScriptChecksum);
	Dbg_MsgAssert(ScriptChecksum,("Goto command requires a script name"));
	
	Script::CStruct *pArgs=NULL;
	pParams->GetStructure(CRCD(0x7031f10c,"Params"),&pArgs);
	pScript->SetScript(ScriptChecksum,pArgs,pScript->mpObject);
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GotoPreserveParams | This has the same effect as goto, 
// except all the parameters that were passed to the current script 
// will get passed to the new script
// @Uparm name | The name of the script to goto
// @parmopt structure | Params | | additional parameters to pass to script
bool ScriptGotoPreserveParams(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	uint32 ScriptChecksum=0;
	pParams->GetChecksum(NONAME,&ScriptChecksum);
	Dbg_MsgAssert(ScriptChecksum,("GotoPreserveParams command requires a script name"));
	
	// Extract the old script parameters.
	// Note: Need to do it this way rather than pass pScript->GetParams() to pScript->SetScript,
	// because SetScript will clear the pScript's params before using the passed params, but
	// since the passed params ARE the pScript's params, this will mean they will get zeroed.
	Script::CStruct *pTemp=new Script::CStruct;
	pTemp->AppendStructure(pScript->GetParams());
	
	// If any more params are specified, merge them on too.
	Script::CStruct *pMoreParams=NULL;
	pParams->GetStructure(CRCD(0x7031f10c,"Params"),&pMoreParams);
	pTemp->AppendStructure(pMoreParams);
	
	
	pScript->SetScript(ScriptChecksum,pTemp,pScript->mpObject);
	delete pTemp;
	
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GotoRandomScript | This will abort the current script
// and jump to a random one of the scripts specified in an array. <nl>
// For example: <nl>
// GotoRandomScript [ScriptA ScriptB ScriptC]
// @uparm [] | Array of scripts to choose from

bool ScriptGotoRandomScript(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	
	Script::CArray *pArray=NULL;
	pParams->GetArray(NONAME,&pArray);
	if (pArray && pArray->GetSize())
	{
		switch (pArray->GetType())
		{
			case ESYMBOLTYPE_NAME:
				pScript->SetScript(pArray->GetNameChecksum(Mth::Rnd(pArray->GetSize())),NULL,pScript->mpObject);				
				break;
			default:
				Dbg_MsgAssert(0,("GotoRandomScript requires an array of script names."));
				break;
		}		
	}
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PrintStruct | Prints the contents of the passed structure
// @uparm structure | The structure to print
bool ScriptPrintStruct(Script::CStruct *pParams, Script::CScript *pScript)
{
	Script::PrintContents(pParams);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static CComponent sTempComponent;

static int sFormatText(CStruct *p_dest_struct, CStruct *p_format)
{
	#define FORMATTED_TEXT_MAX_LEN 1000
	static char s_printf_buffer[FORMATTED_TEXT_MAX_LEN+1];

	Dbg_MsgAssert(p_format,("NULL p_format"));
	
	uint32 string_name_checksum=0;
	p_format->GetChecksum(CRCD(0xff0db407,"TextName"),&string_name_checksum);
	
	// If passed a ChecksumName, the checksum of the formatted text will
	// be calculated and put into a parameter with that name.
	uint32 checksum_name_checksum=0;
	p_format->GetChecksum(CRCD(0x76fa7a8f,"ChecksumName"),&checksum_name_checksum);

	int result = 1;
	
	const char *p_format_text="";
	if (!p_format->GetString(NONAME,&p_format_text))
	{
		printf("Expected text format parameters to contain a format string\n");
		return 0;
	}	
	
	char *p_dest=s_printf_buffer;
	const char *p_scan=p_format_text;
	int space_left=FORMATTED_TEXT_MAX_LEN;
	while (*p_scan)
	{
		Dbg_MsgAssert(space_left,("Overflowed formatted text buffer"));

		if (p_scan[0]=='\\' && p_scan[1]=='%')
		{
			// Make \% equate to %, so that we can have % chars in the string.
			++p_scan;
			*p_dest++=*p_scan++;
			--space_left;
		}	
		else if (*p_scan=='%')
		{
			++p_scan; // Skip over the %
			
			// Find the component in p_format which has the same name as the name following
			// the % sign.
			#define PARAM_NAME_MAX_LEN 50
			char p_param_name[PARAM_NAME_MAX_LEN+1];
			int c=0;
			char *p_param_name_dest=p_param_name;

			if (*p_scan=='%')
			{
				// Two percents mean that what follows is a name longer than one char.
				++p_scan;
				
				while ((*p_scan>='a' && *p_scan<='z') ||
					   (*p_scan>='A' && *p_scan<='Z') ||
					   (*p_scan>='0' && *p_scan<='9') ||
					   *p_scan=='_')
				{
					Dbg_MsgAssert(c<PARAM_NAME_MAX_LEN,("Text formatting param name too long in '%s'",p_format_text));
					*p_param_name_dest++=*p_scan++;
					++c;
				}	
			}
			else
			{
				// One percent means that the name is defined by just one char.
				// This is to allow formatting to be used to append strings, for example
				// "%sBlaa"
				*p_param_name_dest++=*p_scan++;
			}
				
			*p_param_name_dest=0;
			// Got the param name, so look it up in the p_format structure.
			CComponent *p_unresolved_comp=p_format->FindNamedComponentRecurse(GenerateCRC(p_param_name));
			
			if (!p_unresolved_comp)
			{
				printf("Missing text formatting parameter '%s'\n",p_param_name);
				return 0;
			}	

			// p_unresolved_comp may have type Name, which may resolve to some global. So use
			// the ResolveNameComponent to resolve it.
			// Using sTempComponent because ResolveNameComponent may modify the component by
			// perhaps setting it's mpStructure or mpArray to equal that of some global symbol.
			// It would be bad to modify the contents of p_unresolved_comp because it is
			// in the passed structure, which will get cleaned up later, resulting in the deletion
			// of the global symbol's pointer, causing horrible crashes.
			sTempComponent.mType=p_unresolved_comp->mType;
			sTempComponent.mUnion=p_unresolved_comp->mUnion;
			ResolveNameComponent(&sTempComponent);
			
			char p_temp[100];
			switch (sTempComponent.mType)
			{
				case ESYMBOLTYPE_INTEGER:
				{
					if (p_format->ContainsFlag(CRCD(0xf138085f,"UseCommas")))
					{
						strcpy(p_temp,Str::PrintThousands(sTempComponent.mIntegerValue));
					}
					else
					{
						sprintf(p_temp,"%d",sTempComponent.mIntegerValue);
					}	
					int len=strlen(p_temp);
					Dbg_MsgAssert(len<=space_left,("Overflowed formatted text buffer"));
					strcpy(p_dest,p_temp);
					p_dest+=len;
					space_left-=len;
					break;
				}	
				case ESYMBOLTYPE_FLOAT:
				{
					int decimal_places=3;
					char format_string[16];
					p_format->GetInteger(CRCD(0xd8e2e09f,"DecimalPlaces"),&decimal_places);
					Dbg_MsgAssert(decimal_places>=0 && decimal_places<10,("decimal_places must be less than 10"));
					sprintf( format_string, "%%.%df", decimal_places );
					sprintf(p_temp,format_string,sTempComponent.mFloatValue);
					int len=strlen(p_temp);
					Dbg_MsgAssert(len<=space_left,("Overflowed formatted text buffer"));
					strcpy(p_dest,p_temp);
					p_dest+=len;
					space_left-=len;
					break;
				}	
				case ESYMBOLTYPE_PAIR:
				{
					CPair *p_pair=sTempComponent.mpPair;
					Dbg_MsgAssert(p_pair,("NULL p_pair"));
					
					sprintf(p_temp,"(%.3f,%.3f)",p_pair->mX,p_pair->mY);
					int len=strlen(p_temp);
					Dbg_MsgAssert(len<=space_left,("Overflowed formatted text buffer"));
					strcpy(p_dest,p_temp);
					p_dest+=len;
					space_left-=len;
					break;
				}	
				case ESYMBOLTYPE_VECTOR:
				{
					CVector *p_vector=sTempComponent.mpVector;
					Dbg_MsgAssert(p_vector,("NULL p_vector"));
					
					sprintf(p_temp,"(%.3f,%.3f,%.3f)",p_vector->mX,p_vector->mY,p_vector->mZ);
					int len=strlen(p_temp);
					Dbg_MsgAssert(len<=space_left,("Overflowed formatted text buffer"));
					strcpy(p_dest,p_temp);
					p_dest+=len;
					space_left-=len;
					break;
				}	
				case ESYMBOLTYPE_NAME:
				{
					// we set the return result to 2, so we can assert in ScriptFormatText
					// so we can see where in the script we are
					result = 2;
					sprintf(p_temp,"%s",FindChecksumName(sTempComponent.mChecksum));
					int len=strlen(p_temp);
					Dbg_MsgAssert(len<=space_left,("Overflowed formatted text buffer"));
					strcpy(p_dest,p_temp);
					p_dest+=len;
					space_left-=len;
					break;
				}	
				case ESYMBOLTYPE_STRING:
				case ESYMBOLTYPE_LOCALSTRING:
				{
					const char *p_source="";
					if (sTempComponent.mType==ESYMBOLTYPE_STRING)
					{
						p_source=sTempComponent.mpString;
					}	
					else
					{
						p_source=sTempComponent.mpLocalString;
					}	
					
					while (*p_source)
					{
						Dbg_MsgAssert(space_left,("Overflowed formatted text buffer"));
						*p_dest++=*p_source++;
						--space_left;
					}	
					break;
				}	
				default:
				{
					Dbg_MsgAssert(0,("Error when formatting '%s'\n'%s' has data type '%s' which is not supported yet.\n",p_format_text,p_param_name,Script::GetTypeName(sTempComponent.mType)));
					break;
				}	
			}	
		}
		else
		{
			*p_dest++=*p_scan++;
			--space_left;
		}	
	}
	*p_dest=0;

	Dbg_MsgAssert(p_dest_struct,("NULL p_dest_struct"));
	if (string_name_checksum)
	{
		p_dest_struct->AddString(string_name_checksum,s_printf_buffer);
	}	
	if (checksum_name_checksum)
	{
		p_dest_struct->AddChecksum(checksum_name_checksum,Script::GenerateCRC(s_printf_buffer));
	}	
	return result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | FormatText | Fills in any % characters in the passed format string with the passed
// parameters, and assigns the new string to a parameter with the given name.
// For example:
// FormatText TextName=Line1 "Hello %f %b" f="Mum" b=23
// will create the string "Hello Mum 23" and assign it to Line1
// The name following the % must consist of a single character. This is to allow formatting to
// be used to append strings, eg "%sBlaa"
// If two % signs are used, then the name that follows may consist of more than one char,
// eg "%%foo blaa"
//
// Currently supported data types are integer, float, string, local string, vector and pair.
// If any of the parameters are missing it will give an error message and will not create the
// final string. It will not assert though.
//
// @parmopt name | TextName | | The name to be given to the new formatted string
// @parmopt name | ChecksumName | | If this is specified, the checksum of the formatted text
// will be calculated and added using ChecksumName as its name.
// @uparm string | The source format string
// @flag UseCommas | This will make it print any integer using commas to separate thousands for clarity.
bool ScriptFormatText(Script::CStruct *pParams, Script::CScript *pScript)
{
	if (sFormatText(pScript->GetParams(),pParams) == 2)
	{
	#ifdef	__NOPT_ASSERT__
//		Dbg_MsgAssert(0,("%s\nCannot use checksum names in FormatText",pScript->GetScriptInfo()));
		printf ("%s\n### WARNING ### - Cannot use checksum names in FormatText",pScript->GetScriptInfo());
	#endif
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool s_get_string_from_params(char* p_buffer, Script::CStruct *pParams, Script::CScript* pScript)
{
	Dbg_Assert( p_buffer );

	const char *p_text="Default printf text";
	if (pParams->GetText(NONAME,&p_text))
	{
		CStruct *p_format=new CStruct;
		p_format->AddChecksum(GenerateCRC("TextName"),GenerateCRC("PrintfText"));
		p_format->AppendStructure(pParams);
		
		if (sFormatText(p_format,p_format))
		{
			p_format->GetString("PrintfText",&p_text);
		}
		else
		{
			Dbg_Warning("\n%s\nError formatting text for printf",pScript->GetScriptInfo());
			p_text="";
		}

		sprintf(p_buffer, "%s\n",p_text);
		
		delete p_format;   		// p_format actually contains the string we are printing, so delete it AFTER the printf
		
		return true;
	}

	int IntVal=0;
	if (pParams->GetInteger(NONAME,&IntVal))
	{
		sprintf(p_buffer, "%d\n",IntVal);

		return true;
	}
		
	#ifdef __NOPT_ASSERT__
	uint32 Checksum=0;    
	if (pParams->GetChecksum(NONAME,&Checksum))
	{
		sprintf(p_buffer, "[Checksum] %s\n",Script::FindChecksumName(Checksum));

		return true;
	}
		
	sprintf(p_buffer, "Empty printf at %s", pScript->GetScriptInfo());
	#endif
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Printf | Printf "Default printf text" Prints the passed 
// text onto the EE0 output on the target manager.
// Printf can will also print the checksum name of any passed checksum, for example:
// printf blaa    // Will print: [Checksum] blaa
// @uparmopt "text" | some text to print
// @uparmopt 1 | an int to print
// @uparmopt name | checksum to print
// @flag UseCommas | This will make it print any integer using commas to separate thousands for clarity.
bool ScriptPrintf(Script::CStruct *pParams, Script::CScript *pScript)
{
	char buffer[1024];

	s_get_string_from_params(buffer, pParams, pScript);
	
	printf( buffer );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptAssert |
bool ScriptScriptAssert(Script::CStruct *pParams, Script::CScript *pScript)
{
	char buffer[1024];

	s_get_string_from_params(buffer, pParams, pScript);
	
	Dbg_MsgAssert( 0, ( "%s\nSCRIPT ASSERT:  %s", pScript->GetScriptInfo(), buffer ) );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PrintScriptInfo |
bool ScriptPrintScriptInfo(Script::CStruct *pParams, Script::CScript *pScript)
{
	#ifdef	__NOPT_ASSERT__
	printf("+++ScriptInfo+++++++++++++++++++++++++\n");
	
	// firsts prints out the associated string,
	// using the "printf" script function
	ScriptPrintf(pParams, pScript);
	
	// then it asserts
	printf("%s",pScript->GetScriptInfo());
	
	printf("++++++++++++++++++++++++++++++++++++++\n");
	#endif	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetHackFlag | Sets special hackflag
// @uparmopt 1 | integer flag (1 or 0)
static bool HackFlag=false;
bool ScriptSetHackFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	int v=1;
	pParams->GetInteger(NONAME,&v);
	HackFlag=v?true:false;
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | HackFlagIsSet | Returns current value of hackflag
bool ScriptHackFlagIsSet(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	return HackFlag;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PreloadModel | Preloads model
// Example: PreloadModel name="ModelName" or name="levels/skateshop/thps4board_01/thps4board_01.mdl"
// @parm string | name | The model name
bool ScriptPreloadModel(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char* pModelName;
	pParams->GetText( "name", &pModelName, Script::ASSERT );

	// now preload any programmatically-loaded peds
	Nx::CModel* pDummy = Nx::CEngine::sInitModel();
	Dbg_Assert( pDummy );
	
	bool forceTexDictLookup = pParams->ContainsFlag( CRCD(0x6c37fdc7,"AllowReplaceTex") );

	Str::String fullModelName;
	fullModelName = Gfx::GetModelFileName(pModelName, ".mdl");
	pDummy->AddGeom( fullModelName.getString(), 0, true, 0, forceTexDictLookup );
		
	Nx::CEngine::sUninitModel( pDummy );
	
	return true;
}
		   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*
// @script | LoadLevelGeometry | Loads a bsp file. Assumes the current 
// directory is \skate3\data, so a valid string would be "levels\aus\aus.bsp". 
// Can also have an optional "Sky="xxx.bsp" parameter for loading the 
// sky dome with a level file. The Level and Sky parameters are optional, 
// but you must have at least one of them (So you can load just a sky, 
// or just a level, if you want).
// @parmopt string | Sky | | .bsp file for sky dome
// @parmopt string | Level | | .bsp file for level
// @parmopt string | Pre_set | | load assets from PRE file
bool ScriptLoadLevelGeometry(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	 File::PreMgr * pre_mgr =  File::PreMgr::Instance();
	
	const char*	p_level = NULL;
	const char*	p_sky 	= NULL;

	pParams->GetText( "Level",&p_level );
	pParams->GetText( "Sky", &p_sky );

	Nx::CTexDict *p_tex_dict, *p_sky_tex_dict;
	Nx::CScene *p_scene, *p_sky_scene;

	if (p_sky)
	{
		p_sky_tex_dict = Nx::CTexDictManager::sLoadTextureDictionary(p_sky);
		Dbg_Assert(p_sky_tex_dict);
		p_sky_scene = Nx::CEngine::sLoadScene(p_sky, p_sky_tex_dict, false, true);	// mark as sky that doesn't go in SuperSectors
		Dbg_Assert(p_sky_scene);
	}
	
	if (p_level)
	{
		//p_tex_dict = Nx::CEngine::sLoadTextures(p_level);
		p_tex_dict = Nx::CTexDictManager::sLoadTextureDictionary(p_level);
		Dbg_Assert(p_tex_dict);
		p_scene = Nx::CEngine::sLoadScene(p_level, p_tex_dict);
		Dbg_Assert(p_scene);
		p_scene->LoadCollision(p_level);
	}
	
	return true;
}

*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptUnloadAllLevelGeometry(Script::CStruct *pParams, Script::CScript *pScript)
{
	Nx::CEngine::sUnloadAllScenesAndTexDicts();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptLoadScene(Script::CStruct *pParams, Script::CScript *pScript)
{
	bool is_dictionary = pParams->ContainsFlag(CRCD(0x3d559e7d,"is_dictionary"));  		// piece dictionary, like the park editor
	bool is_sky = pParams->ContainsFlag(CRCD(0xb5bd28d8,"is_sky"));						// a sky dome
	bool is_net = pParams->ContainsFlag(CRCD(0x417149c8,"is_net"));						// the _net version
	bool add_super_sectors = ! pParams->ContainsFlag(CRCD(0x8943dbd8,"no_supersectors"));	// optionally ignore supersectors
	
	const char *p_scene_name = NULL;
	pParams->GetText( CRCD(0x26861025,"scene"),&p_scene_name,true );
	Dbg_MsgLog(("Loading Scene %s ...",p_scene_name));
	printf ("Loading Scene %s ...",p_scene_name);
	

	// First load the texture dictionary
	char	texture_dict_name[128];
	sprintf(texture_dict_name,"levels\\%s\\%s%s.tex",p_scene_name,p_scene_name,is_net?"_net":"");


	
	#ifdef	__NOPT_ASSERT__
	const char *p_fallback = "nj_sky";
	// Some debug code to load the NJ sky if default_sky does not exists
	if (stricmp(texture_dict_name,"levels\\default_sky\\default_sky.tex")==0)
	{
		printf ("TRYING TO LAOD DEFUALT SKY\n");
		printf ("TRYING TO LAOD DEFUALT SKY\n");
		printf ("TRYING TO LAOD DEFUALT SKY\n");
		printf ("TRYING TO LAOD DEFUALT SKY\n");
		// it's the default sky, so check if it exists
		if (!File::Exist(texture_dict_name))
		{
			printf ("NO EXIST\n");
		
			p_scene_name = p_fallback;			
			sprintf(texture_dict_name,"levels\\%s\\%s.tex",p_scene_name,p_scene_name);
		}
	}
	#endif
	
	
	Nx::CTexDict * p_tex_dict = Nx::CTexDictManager::sLoadTextureDictionary(texture_dict_name,true);
	Dbg_MsgAssert(p_tex_dict,("ERROR loading tex dict for %s\n",texture_dict_name));

	// a level ending in _sky is automatically a sky dome	
	if (Str::StrStr(p_scene_name,"_sky"))
	{
		is_sky = true;
	}
	
	// Skys always have no supersectors
	if (is_sky)
	{
		add_super_sectors = false;		
	}
	
	// and dictionaries always have no supersectors
	if (is_dictionary)
	{
		add_super_sectors = false;		
	}
	
	#ifdef	__NOPT_ASSERT__
	Nx::CScene * p_scene = 
	#endif
	Nx::CEngine::sLoadScene(p_scene_name, p_tex_dict, add_super_sectors,is_sky,is_dictionary,is_net);	// mark as sky that doesn't go in SuperSectors
	Dbg_MsgAssert(p_scene,("ERROR loading scene for %s\n",p_scene_name));
	printf ("... done\n");
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptAddScene(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *p_scene_name = NULL;
	const char *p_add_name = NULL;
	pParams->GetText( "scene",&p_scene_name );
	pParams->GetText( "add",&p_add_name );
	if (p_scene_name && p_add_name)
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());

		#ifdef	__NOPT_ASSERT__
		Nx::CScene * p_scene =
		#endif
		Nx::CEngine::sAddScene(p_scene_name, p_add_name);
		Dbg_MsgAssert(p_scene,("ERROR adding scene for %s\n",p_scene_name));

		Mem::Manager::sHandle().PopContext();
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// we have a seperate script command for loading collision
// so we can control the loading of pre files
bool ScriptLoadCollision(Script::CStruct *pParams, Script::CScript *pScript)
{

	const char *p_scene_name = NULL;
	pParams->GetText( "scene",&p_scene_name );
	bool is_net = pParams->ContainsFlag(CRCD(0x417149c8,"is_net"));						// the _net version
	if (p_scene_name)
	{
		Nx::CScene * p_scene = Nx::CEngine::sGetScene(p_scene_name);
		Dbg_MsgAssert(p_scene,("Trying to load collision for scene: %s\n",p_scene_name));
		Dbg_MsgLog(("Trying to load collision for scene: %s\n",p_scene_name));
		p_scene->LoadCollision(p_scene_name, is_net);	
	}
	return true;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptAddCollision(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *p_scene_name = NULL;
	const char *p_add_name = NULL;
	pParams->GetText( "scene",&p_scene_name );
	pParams->GetText( "add",&p_add_name );
	if (p_scene_name && p_add_name)
	{
		Nx::CScene * p_scene = Nx::CEngine::sGetScene(p_scene_name);
		Dbg_MsgAssert(p_scene,("Trying to add collision file %s to scene: %s\n", p_add_name, p_scene_name));
		p_scene->AddCollision(p_add_name);
	}
	return true;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptUnloadScene(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *p_scene_name = NULL;
	bool matching_names;
	pParams->GetText( "scene",&p_scene_name );

	matching_names = false;
	if (p_scene_name)
	{
		Nx::CScene *p_scene = Nx::CEngine::sGetScene(p_scene_name);
		if (p_scene)
		{
			matching_names = true;
		}
		else
		{
			p_scene = Nx::CEngine::sGetMainScene();
		}

		if (p_scene)
		{
			Nx::CTexDict *p_tex_dict = p_scene->GetTexDict();

			Nx::CEngine::sUnloadScene(p_scene);
			// must unload dictionary after scene
			if (p_tex_dict)
			{
				Nx::CTexDictManager::sUnloadTextureDictionary(p_tex_dict);
			}
		}
	}

	return matching_names;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptQuickReload(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *p_scene_name = NULL;
	pParams->GetText( "scene",&p_scene_name );
	if (p_scene_name)
	{
		Nx::CEngine::sQuickReloadGeometry(p_scene_name);
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptCreateFromNode( Script::CStruct *pNode )
{	
//	Dbg_MsgLog(("CreateFromNodeIndex nodeIndex=%d",nodeIndex));
	
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	Obj::CRailManager *p_rail_man = skate_mod->GetRailManager();
	Dbg_MsgAssert(p_rail_man,("Missing rail manager"));					 
					 
//	Script::CArray *pNodeArray = Script::GetArray( CRCD(0xc472ecc5,"NodeArray") );
//	Dbg_MsgAssert( pNodeArray,( "No NodeArray found" ));

	// We still need the node index for some things (rails, proxim)
	// nut now we get it from the node
	int nodeIndex = 0;
	pNode->GetInteger(CRCD(0xe50d6573,"NodeIndex"), &nodeIndex);

//	Script::CStruct *pNode=pNodeArray->GetStructure( nodeIndex );
	Dbg_MsgAssert( pNode,( "NULL pNode" ));

	// If this is a net game, don't load objects that were meant to be left out for 
	// performance/bandwidth/gameplay reasons
	if ( skate_mod->ShouldBeAbsentNode( pNode ) )
	{
		return true;
	}

#ifdef __NOPT_ASSERT__
	Mem::Allocator* pCurrentHeapContext = Mem::Manager::sHandle().GetContextAllocator();
	if ( !( pCurrentHeapContext == Mem::Manager::sHandle().CutsceneBottomUpHeap()
		 || pCurrentHeapContext == Mem::Manager::sHandle().BottomUpHeap() ) )
	{
		// GJ:  this function used to push the bottom up heap context explicitly,
		// but it was causing problems with the cutscene code, which
		// wants to put stuff on the cutscene heap.  however, there's
		// no reason to push the bottom up heap, since the bottom up heap
		// was always the default context anyway...  i think.
		Dbg_MsgAssert( 0, ( "Was expecting bottomupheap context" ) );
	}
#endif

	/*
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	*/

	uint32 ClassChecksum = 0;
	pNode->GetChecksum( 0x12b4e660 /*"Class"*/, &ClassChecksum );

	// check for obsolete trigger objects first
	if ( ClassChecksum == 0xe594f0a2 )	// trigger
	{
		uint32 checksumName = 0;
		if ( pNode->GetChecksum( 0xa1dc81f9, &checksumName ) ) // checksum 'name'
		{
#ifdef __NOPT_ASSERT__
			printf("WARNING:  Trigger objects are obsolete %s\n", Script::FindChecksumName(checksumName) );
#endif		
		}
	}

	bool success = true;

	switch (ClassChecksum)
	{
		case 0xe47f1b79: //vehicle
			Dbg_MsgLog(("Vehicle"));
			Mem::PushMemProfile("Vehicles");
			Obj::CreateCar( skate_mod->GetObjectManager(), pNode );
			Mem::PopMemProfile();
			break;
	
		case 0xa0dfac98:  //pedestrian
		case 0x061a741e:  //ped
			{
				Dbg_MsgLog(("Pedestrian"));
				Mem::PushMemProfile("Pedestrians");
				Obj::CreatePed( skate_mod->GetObjectManager(), pNode );
				Mem::PopMemProfile();
			}
			break;

		case 0xef59c100:  //gameobject
			{
				Dbg_MsgLog(("gameobject"));
				Mem::PushMemProfile("Game Objects");
				Obj::CreateGameObj( skate_mod->GetObjectManager(), pNode );
				Mem::PopMemProfile();
			}
			break;
			
		case 0x19b1e241: // ParticleEmitter
			{
				Dbg_MsgLog(("ParticleEmitter"));
				Mem::PushMemProfile("Particle Systems");
				Obj::CreateParticleEmitter( skate_mod->GetObjectManager(), pNode );
				Mem::PopMemProfile();
			}
			break;
		case 0x9e7d469e: // ParticleObject
			{
				Dbg_MsgLog(("ParticleObject"));
				Mem::PushMemProfile("Particle Systems");
				Obj::CreateParticleObject( skate_mod->GetObjectManager(), pNode );
				Mem::PopMemProfile();
			}
			break;

		case 0xb7b3bd86:  // LevelObject
			{
				Dbg_MsgLog(("LevelObject"));
				// create a level object here
				Mem::PushMemProfile("LevelObjects");
				Obj::CreateLevelObj( skate_mod->GetObjectManager(), pNode );
				Mem::PopMemProfile();
				// Note, we used to turn off the original sector here
				// this is now done to all LevelObjects in ParseNodeArray
			}
			break;

//		case 0xe594f0a2:  // Trigger (geometry containing triggers...)
		case 0xbf4d3536:  // LevelGeometry
		{
			Dbg_MsgLog(("LevelGeometry"));
			uint32 checksumName = 0;
			if ( pNode->GetChecksum( 0xa1dc81f9, &checksumName ) ) // checksum 'name'
			{
//				Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksumName);
				// We only want to get sectors from the main scene, as that is what the node array applies to															
				Nx::CScene * p_main_scene = Nx::CEngine::sGetMainScene();
				Nx::CSector *p_sector = p_main_scene->GetSector(checksumName);
				if (p_sector)
				{
					Dbg_MsgAssert(p_sector,("sGetSector(0x%x) returned NULL (%s)",checksumName,Script::FindChecksumName(checksumName)));
					p_sector->SetVisibility(0xffffffff);
					p_sector->SetActive(true);
					p_sector->SetCollidable(true);

					// If this node is an occluder, let the occlusion code know that this occluder is now enabled.
					if( pNode->ContainsFlag( 0x4e549fe1 /*"Occluder"*/ ))
					{
						Nx::CEngine::sEnableOcclusionPoly( checksumName, true );
					}
				}
				else
				{
					printf(" WARNING: sGetSector(0x%x) returned NULL (%s)\n",checksumName,Script::FindChecksumName(checksumName));
				}
				//Bsp::ClearWorldSectorFlag( checksumName, mSD_KILLED | mSD_NON_COLLIDABLE | mSD_INVISIBLE  | mSD_INVISIBLE2);
			}
			break;
 		}

		case 0x8e6b02ad:  // railnode
			Dbg_MsgLog(("railnode"));
			p_rail_man->SetActive( nodeIndex, true, pNode->ContainsFlag( "FLAG_WHOLE_RAIL" ) );
			break;
		case CRCC(0x30c19600, "ClimbingNode"):
			Dbg_MsgLog(("climbingnode"));
			p_rail_man->SetActive( nodeIndex, true, pNode->ContainsFlag( "FLAG_WHOLE_RAIL" ) );
			break;
		case 0x8470f2e:  // proximnode
			Dbg_MsgLog(("proximnode"));
			Obj::Proxim_SetActive( nodeIndex, true );
			break;			
		case 0xd64dc7c2:  // emitterobject
			Dbg_MsgLog(("emitterobject"));
			Obj::CEmitterManager::sSetEmitterActive( nodeIndex, true );
			break;			
		default:
			Dbg_MsgLog(("default"));
			success = false;
			break;
	}
//	Mem::Manager::sHandle().PopContext();
	return success;
}


bool ScriptCreateFromNodeIndex( int nodeIndex )
{	
	return ScriptCreateFromNode(SkateScript::GetNode(nodeIndex));
}


struct MaybeKillObjectInfo
{
	uint32 nodeNum;
	Obj::CObject *forbiddenObject;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void MaybeKillObject( Obj::CObject *pOb, void *pVoidData )
{
	
	Dbg_MsgAssert(pOb,("NULL pOb"));
	Dbg_MsgAssert(pVoidData,("NULL pVoidData"));

	MaybeKillObjectInfo *pInfo;
	pInfo = ( MaybeKillObjectInfo * )pVoidData;
	
	uint32 nodeNum = pInfo->nodeNum;
	uint32	nodeName = SkateScript::GetNodeNameChecksum(nodeNum);
	
	if ( pOb->GetID() == nodeName )
	{
		Dbg_MsgAssert( pOb != pInfo->forbiddenObject,( "Can't issue a kill command from within the script of the object you're killing:  Use Die command." ));
		pOb->DestroyIfUnlocked( );
//		pOb->MarkAsDead();
	}
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptKillFromNodeIndex( int nodeIndex, Script::CScript *pScript )
{			
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
			
	Obj::CRailManager *p_rail_man = skate_mod->GetRailManager();
	Dbg_MsgAssert(p_rail_man,("Missing rail manager"));					 
					     
	Script::CArray *pNodeArray = Script::GetArray( CRCD(0xc472ecc5,"NodeArray") );
	Dbg_MsgAssert( pNodeArray,( "No NodeArray found" ));

	Script::CStruct *pNode=pNodeArray->GetStructure( nodeIndex );
	Dbg_MsgAssert( pNode,( "NULL pNode" ));

	uint32 ClassChecksum = 0;
	pNode->GetChecksum( 0x12b4e660 /*"Class"*/, &ClassChecksum );


									
	switch (ClassChecksum)
	{
		case 0xe47f1b79: // vechicle
		case 0xa0dfac98: // pedestrian
		case 0x061a741e: // ped
		case 0xef59c100: // gameobject
		case 0x19b1e241: // ParticleEmitter
		case 0x9e7d469e: // ParticleObject
		case 0xb7b3bd86: // LevelObject
		case 0x5b8ab877: // skater
		{
			MaybeKillObjectInfo info;
			info.forbiddenObject = pScript->mpObject;
			info.nodeNum = nodeIndex;
			
			skate_mod->GetObjectManager()->ProcessAllObjects( MaybeKillObject, &info );
			break;
		}

		case CRCC(0x30c19600, "ClimbingNode"):
			p_rail_man->SetActive( nodeIndex, false, pNode->ContainsFlag( "FLAG_WHOLE_RAIL" ) );
			break;		

		case 0x8e6b02ad:  // railnode
			p_rail_man->SetActive( nodeIndex, false, pNode->ContainsFlag( "FLAG_WHOLE_RAIL" ) );
			break;		

		case 0x8470f2e:  // proximnode
			Obj::Proxim_SetActive( nodeIndex, false );
			break;			

		case 0xd64dc7c2:  // emitterobject
			Obj::CEmitterManager::sSetEmitterActive( nodeIndex, false );
			break;			

//		case 0xe594f0a2:  // Trigger (geometry containing triggers...)
		case 0xbf4d3536:  // LevelGeometry
		{
			uint32 checksumName = 0;
			if ( pNode->GetChecksum( 0xa1dc81f9, &checksumName ) ) // checksum 'name'
			{
//				Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksumName);				
				// We only want to get sectors from the main scene, as that is what the node array applies to															
				Nx::CScene * p_main_scene = Nx::CEngine::sGetMainScene();
				Nx::CSector *p_sector = p_main_scene->GetSector(checksumName);
				Dbg_MsgAssert(p_sector,("sGetSector(0x%x) returned NULL (%s)",checksumName,Script::FindChecksumName(checksumName)));
				p_sector->SetActive(false);
				//Bsp::SetWorldSectorFlag(  checksumName, mSD_KILLED );

				// If this node is an occluder, let the occlusion code know that this occluder is no longer enabled.
				if( pNode->ContainsFlag( 0x4e549fe1 /*"Occluder"*/ ))
				{
					Dbg_MsgAssert(!ScriptInSplitScreenGame(NULL, NULL),
								  ("Can't turn occlusion polys on or off during a 2 player game"));
					Nx::CEngine::sEnableOcclusionPoly( checksumName, false );
				}
			}
			break;
 		}

		default:
			return ( false );
			break;
	}
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptKillFromNodeNameChecksum( uint32 nodeNameChecksum, Script::CScript *pScript )
{

	return ( ScriptKillFromNodeIndex( SkateScript::FindNamedNode( nodeNameChecksum ), pScript ) );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool DoNodeActionFromNodeNum( int nodeNum, int action, Script::CScript *pScript, Script::CStruct *pParams )
{
    GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

// In net games, check if the node is flagged to be absent
// and just ignore the command it fo	
	if( gamenet_man->InNetMode())
	{
		// Rather inefficient.  We should get away from refering to nodes by index
		if (skate_mod->ShouldBeAbsentNode( SkateScript::GetNode(nodeNum) ) )
		{
			// just ignore the action
			return false;
		}
	}

	// note rather dodgy way of determining the viewport of a script
	// if it has an object, and tht object is the second skater, 
	// and we have two viewports
	// use viewport 1, otherwise default to viewport 0	
	int viewport_number = 0;
	if (pScript->mpObject && !gamenet_man->InNetGame() && Nx::CViewportManager::sGetNumActiveViewports() == 2)
	{
		Obj::CSkater *pSkater = skate_mod->GetSkater(1);
		if (pScript->mpObject == pSkater )		
		{
			viewport_number = 1;
		}
	}
	
	
    switch ( action )
	{
		case ( NODE_ACTION_CREATE ):
			return ( ScriptCreateFromNodeIndex( nodeNum ) );
		case ( NODE_ACTION_KILL ):
			return ( ScriptKillFromNodeIndex( nodeNum, pScript ) );
		case ( NODE_ACTION_SET_VISIBLE ):
			return ( ScriptSetVisibilityFromNodeIndex( nodeNum, false, viewport_number ) );
		case ( NODE_ACTION_SET_INVISIBLE ):
			return ( ScriptSetVisibilityFromNodeIndex( nodeNum, true, viewport_number ) );
		case ( NODE_ACTION_SET_COLOR ):
			return ( ScriptSetColorFromNodeIndex( nodeNum, pParams ) );
		case NODE_ACTION_SHATTER:
		{   
			ScriptSetVisibilityFromNodeIndex( nodeNum, false, viewport_number );
			return ScriptShatterFromNodeIndex( nodeNum );
		}
		case NODE_ACTION_CHECK_IF_ALIVE:
		{
			return ( ScriptCheckExistenceFromNodeIndex( nodeNum ) );
		}
		default:
			Dbg_MsgAssert( 0,( "Fire Matt." ));
			return ( false );
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Used to perform actions to nodes by prefix, name checksum, or links... General version
// (applicable to nodes that create Environmental Objects, Rails, and all types of CMovingObjects.
bool DoNodeAction( Script::CStruct *pParams, Script::CScript *pScript, int action )
{
	
	uint32 nameChecksum;
	uint32 prefixChecksum;
	//const char *pPrefix;
	int i;
	bool useCurrentLinks = false;
	if ( pParams->ContainsFlag( 0x2e7d5ee7 ) || // checksum 'links'
		( useCurrentLinks = pParams->ContainsFlag( 0xbceb479a ) ) ) // checksum 'currentlinks'
	{
		Dbg_MsgAssert(0,("%s:  DoNodeAction with links or currentlinks is deprecated",pScript->GetScriptInfo()));
	
		/*
		int numLinks;
		Script::CStruct *pNode = NULL;
		if ( pParams->GetChecksumOrStringChecksum( 0xa1dc81f9, &nameChecksum ) ) // checksum 'name'
		{
			if ( useCurrentLinks )
			{
				Dbg_Message( "\n%s\nWarning:  The 'currentLinks' flag only works in an object script on the calling object.", pScript->GetScriptInfo( ) );
			}
			pNode = SkateScript::GetNode( SkateScript::FindNamedNode( nameChecksum ) );
			Dbg_MsgAssert( pNode,( "\n%s\nNo node named %s found", pScript->GetScriptInfo( ), Script::FindChecksumName( nameChecksum ) ));
		}
		else
		{
			if ( pScript->mNode != -1 )
			{
				pNode = SkateScript::GetNode( pScript->mNode );
				Dbg_MsgAssert( pNode,( "\n%s\nnode %d not found", pScript->GetScriptInfo( ), pScript->mNode ));
			}
			else
			{
				if ( pScript->mpObject )
				{
					if ( useCurrentLinks )
					{
						pNode = SkateScript::GetNode( ((Obj::CMovingObject *) pScript->mpObject.Convert())->m_current_node );
						Dbg_MsgAssert( pNode,( "\n%s\n object's node %d is invalid", pScript->GetScriptInfo( ), ((Obj::CMovingObject *) pScript->mpObject.Convert())->m_start_node ));
					}
					else
					{
						pNode = SkateScript::GetNode( ((Obj::CMovingObject *) pScript->mpObject.Convert())->m_start_node );
						Dbg_MsgAssert( pNode,( "\n%s\n object's node %d is invalid", pScript->GetScriptInfo( ), ((Obj::CMovingObject *) pScript->mpObject.Convert())->m_start_node ));
					}
				}
				else
				{
					Dbg_MsgAssert( 0,( "\n%s\nNo valid nodes to be found anywherest, dumass.", pScript->GetScriptInfo( ) ));
					return ( false );
				}
			}
		}
		Dbg_MsgAssert( pNode,( "What the fuck?  How did this motherfuckin' happen?" ));
		numLinks = SkateScript::GetNumLinks( pNode );
		for ( i = 0; i < numLinks; i++ )
		{
			if ( DoNodeActionFromNodeNum( SkateScript::GetLink( pNode, i ), action, pScript ) )
			{
				if ( action == NODE_ACTION_CHECK_IF_ALIVE )
				{
					return ( true );
				}
			}
		}
		*/

		return ( true );
	}
    //if ( pParams->GetText( 0x6c4e7971, &pPrefix ) ) // checksum 'prefix'
    if ( pParams->GetChecksumOrStringChecksum( 0x6c4e7971, &prefixChecksum ) ) // checksum 'prefix'
	{
		// Create with a prefix specified:
		uint16 numNodes = 0;
	  //  const uint16 *pMatchingNodes = SkateScript::GetPrefixedNodes( pPrefix, &numNodes );
		const uint16 *pMatchingNodes = SkateScript::GetPrefixedNodes( prefixChecksum, &numNodes );
		for ( i = 0; i < numNodes; i++ )
		{
			if ( DoNodeActionFromNodeNum( pMatchingNodes[ i ], action, pScript, pParams ) )
			{
				if ( action == NODE_ACTION_CHECK_IF_ALIVE )
				{
					return ( true );
				}
			}
		}
		return ( true );
	}
	if ( pParams->GetChecksumOrStringChecksum( 0xa1dc81f9, &nameChecksum ) ) // checksum 'name'
	{
		// Create with a name specified:
		return ( DoNodeActionFromNodeNum( SkateScript::FindNamedNode( nameChecksum ), action, pScript, pParams ) );
	}

	// if they're calling "Invisible" with no parameters, intending to turn 
	// visibility off on an object from within the object's script, they should
	// be calling "Obj_Invisible" (or Obj_Visible, or Die, create not applicable
	// since the object would have to have  already been created to be calling
	// something.)
	Dbg_Message( "\n%s\nSyntax not recognized... object script may need to call Obj_ version. (Ask Matt).", pScript->GetScriptInfo( ) );
	return ( false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptNodeExists( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 node_checksum=0;
	pParams->GetChecksum(NONAME,&node_checksum,true);
	
	return SkateScript::NodeExists(node_checksum);
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptCreateFromStructure( Script::CStruct *pParams, Script::CScript *pScript )
{
	return ScriptCreateFromNode( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Create | creates an object node
// @flag links | pass in to perform action to nodes by links
// @flag currentlinks | pass in to perform action to nodes by links
// @parmopt name | name | | The name of the node
// @parmopt name | prefix | | Create with a prefix specified
bool ScriptCreate( Script::CStruct *pParams, Script::CScript *pScript )
{

 #ifdef	__PLAT_NGPS__		
//		snProfSetRange( -1, (void*)0, (void*)-1);
//		snProfSetFlagValue(0x01);
 #endif


	// Flush dead objects before we create any new ones
	ScriptFlushDeadObjects(NULL,NULL);	
	
	bool result = ( DoNodeAction( pParams, pScript, NODE_ACTION_CREATE ) );

 #ifdef	__PLAT_NGPS__		
//		snProfSetRange( 4, (void*)NULL, (void*)-1);
 #endif		
 
	return result;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetObjectColor | color the object a particular color
// @parm name | name | The name of the piece of level geometry to change color
// @parm int | color | The color to set the object to (as a hexadecimal ABGR value)
bool ScriptSetObjectColor( Script::CStruct *pParams, Script::CScript *pScript )
{

	bool result = ( DoNodeAction( pParams, pScript, NODE_ACTION_SET_COLOR ) );
	
	return result;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Kill | kills an object node
// @flag links | pass in to perform action to nodes by links
// @flag currentlinks | pass in to perform action to nodes by links
// @parmopt name | name | | The name of the node
// @parmopt name | prefix | | kill with a prefix specified
bool ScriptKill( Script::CStruct *pParams, Script::CScript *pScript )
{

	return ( DoNodeAction( pParams, pScript, NODE_ACTION_KILL ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Visible | make object visible
// @flag links | pass in to perform action to nodes by links
// @flag currentlinks | pass in to perform action to nodes by links
// @parmopt name | name | | The name of the node
// @parmopt name | prefix | | act on objects with specified prefix
bool ScriptVisible( Script::CStruct *pParams, Script::CScript *pScript )
{
	return ( DoNodeAction( pParams, pScript, NODE_ACTION_SET_VISIBLE ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Invisible | make object invisible
// @flag links | pass in to perform action to nodes by links
// @flag currentlinks | pass in to perform action to nodes by links
// @parmopt name | name | | The name of the node
// @parmopt name | prefix | | act on objects with specified prefix
bool ScriptInvisible( Script::CStruct *pParams, Script::CScript *pScript )
{
	return ( DoNodeAction( pParams, pScript, NODE_ACTION_SET_INVISIBLE ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Shatter | Shatter an object
// @parmopt float | area | 288.0 | the subdivision area above which
// polys will be recursively subdivided. Be careful with this
// parameter - making it too small will cause the buffer to
// overflow and assert. The units are square inches <nl>
// The default value may change
// @parmopt float | variance | 0.0 | the amount the velocity
// of each shard can be (randomly) above the supplied velocity. 
// So a value of 1.0 would mean a shard could travel at twice the supplied <nl>
// velocity <nl> The default value may change.
// @parmopt float | spread | 1.0 | determines the degree to which the
// shards will fly apart. The way I figure this is to take the center
// of the bounding box for the object, and then move backwards (spread
// * velocity). This position is then used to figure the start velocity
// for each shard. So the lower the spread value, the less the start
// point moves back, and the more the shards will fly apart. (I can
// draw a diagram if this isn't clear). <nl>
// The default value may change
// @parmopt float | life | 4.0 | how long the shatter atomic will exist for
// (seconds) <nl> The default value may change
// @parmopt | bounce | -10,000 | Default value may change
// @parmopt | scale | 1.0 | Scaling factor for the shards
// @flag use_skater_vel | Use skater velocity to determine
// if object should shatter
// @parm | vel_x | default velocity of shard on x-axis (in inches per second)
// @parm | vel_y | default velocity of shard on y-axis (in inches per second) 
// @parm | vel_z | default velocity of shard on z-axis (in inches per second)
bool ScriptShatter( Script::CStruct *pParams, Script::CScript *pScript )
{
	// Need to read some params from the script here.
	float		area		= 0.0f;
	float		variance	= 0.0f;
	float		spread		= 0.0f;
	float		life		= 0.0f;
	float		bounce		= 0.0f;
	float		bounce_amp	= 0.0f;
	float		scale		= 1.0f;
	Mth::Vector	velocity;

	if( pParams->ContainsFlag( 0x9f98989e /*"use_skater_vel"*/ ))
	{
		pParams->GetFloat( "scale", &scale );

		Obj::CSkater* p_skater = static_cast <Obj::CSkater*>( pScript->mpObject.Convert() );
		Dbg_Assert( p_skater );

		velocity = p_skater->GetVel() * scale;
	}
	else
	{
		// Have to supply vel_x, y, and z.
		pParams->GetFloat( "vel_x", &velocity[X], true );
		pParams->GetFloat( "vel_y", &velocity[Y], true );
		pParams->GetFloat( "vel_z", &velocity[Z], true );
		velocity[W] = 0.0f;	// To prevent assert on using unitialized vectors 
	}

	pParams->GetFloat( "area",			&area );
	pParams->GetFloat( "variance",		&variance );
	pParams->GetFloat( "spread",		&spread );
	pParams->GetFloat( "life",			&life );
	pParams->GetFloat( "bounce",		&bounce );
	pParams->GetFloat( "bounce_amp",	&bounce_amp );

	//////////////////////////////////////////////////////////////////////////////////////////
	// Mick 09/18/02:  PATCH
	// setting the shatter area too low (generally to 1000) generates an excessive amount
	// of polygons in the form of immediate mode primitive, which eat up the DMA buffer on the PS2
	// So I'm going to limit the area to 2000 in the general case, and to 5000 in multi player games
	float min_area = 2000.0f;				// single player limit
	if (Mdl::Skate::Instance()->IsMultiplayerGame())
	{
		min_area = 5000.0f;
	}
	
	if (area < min_area)
	{
		#ifdef	__NOPT_ASSERT__
		printf("WARNING:  area %.1f too small for THPS4 PS2 engine, clamping to %.1f\n",area,min_area);
		#endif
		area = min_area;
	}
	// END PATCH
	//////////////////////////////////////////////////////////////////////////////////////////

	// Set up values for this shatter.
	Nx::ShatterSetParams( velocity, area, variance, spread, life, bounce, bounce_amp );

	return DoNodeAction( pParams, pScript, NODE_ACTION_SHATTER );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CleanupBeforeParseNodeArray()
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

	Obj::Proxim_Init();					
	Obj::CEmitterManager::sInit();
	skate_mod->GetRailManager()->Cleanup();		// probably not needed, as will be done in ScriptCleanup
	skate_mod->GetTrickObjectManager()->DeleteAllTrickObjects();

	// Scene lights are created during the node array parse, so clear them up here ready.
	Nx::CLightManager::sClearSceneLights();
	Nx::CLightManager::sClearVCLights();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ParseNodeArray | Parses the loaded node array 
bool ScriptParseNodeArray( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mem::PushMemProfile("ParseNodeArray");

	CleanupBeforeParseNodeArray();	

	Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

	// Scan through the node array creating all things that need to be created.
	Script::CArray *pNodeArray=Script::GetArray(CRCD(0xc472ecc5,"NodeArray"));
	Dbg_MsgAssert(pNodeArray,("No NodeArray found in ParseNodeArray"));

	// Rails are now added seperately
	Mem::PushMemProfile("Rail Nodes");
	Obj::CRailManager *p_rail_man = skate_mod->GetRailManager();
	Dbg_MsgAssert(p_rail_man,("Missing rail manager"));					 
	p_rail_man->AddRailsFromNodeArray(pNodeArray );					 									  
	Mem::PopMemProfile();
	
	// Nav nodes are added separately.
#	ifdef TESTING_GUNSLINGER
	Mem::PushMemProfile( "Nav Nodes" );
	Obj::CNavManager* p_nav_man = skate_mod->GetNavManager();
	Dbg_MsgAssert( p_nav_man, ( "Missing Nav Manager" ));
	p_nav_man->AddNavsFromNodeArray( pNodeArray );
	Mem::PopMemProfile();
#	endif

	// We only want to get sectors from the main scene, as that is what the node array applies to															
	Nx::CScene * p_main_scene = Nx::CEngine::sGetMainScene();
									  
	bool	fail = false;
	uint32 i;	 
	for (i=0; i<pNodeArray->GetSize(); ++i)
	{
		Script::CStruct *pNode=pNodeArray->GetStructure(i);
		Dbg_MsgAssert(pNode,("NULL pNode"));

		uint32 ClassChecksum = 0;
		pNode->GetChecksum( 0x12b4e660 /*"Class"*/, &ClassChecksum );


		// First the nodes that need some special processing
		// for the "net game" state
		switch (ClassChecksum)
		{						   
			// Note - Rails are no longer added here, but in the AddRailsFromNodeArray, above
			
			case 0xb7b3bd86:  // LevelObject
			{
				// A "LevelObject" is like "LevelGeometry", in that it is a 3DStudio Max object
				// which is part of the scene (like, a wall or a house)
				// However, it's going to be movable, so it has to exist in "local" coordinate
				// space, centered around the origin.
				// When it is exported, the original geometry is just tranlated to the origin, and left there
				// later we will create a clone of this object, which will be be movable instance
				// but the original is left alone. So, we always turn off the original instance
				// of a LevelObject, before we create any instances. The creation is done via
				// ScriptCreateFromNodeIndex, either below (if createdAtStart), 
				// or via a script command at some indeterminate time in the future.
				uint32 checksumName = 0;
				pNode->GetChecksum( 0xa1dc81f9 /*Name*/, &checksumName);
//				Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksumName);
				Nx::CSector *p_sector = p_main_scene->GetSector(checksumName);
				if (p_sector)
				{
						p_sector->SetActive(false);
				}
				

				break;
			}
			
			case 0x8e6b02ad:  // railnode
			{
				pNode->RemoveComponent(CRCD(0x9d2d0915,"angles"));
				pNode->RemoveComponent(CRCD(0x7321a8d6,"type")); 	// Type is no longer used, as "TerrainType" suffices
				break;
			}

			case CRCC(0x30c19600, "ClimbingNode"):
			{
				break;
			}
			
			case 0xbf4d3536:  // LevelGeometry
			{
				
				////////////////////////////////////////////////////////////////////////////////////
				// Test:  if it's a level object, then remove the angles, collisionmode and position
				pNode->RemoveComponent(CRCD(0x2d7e583b,"collisionmode"));
//				pNode->RemoveComponent(CRCD(0xb9d31b0a,"position"));
//				pNode->RemoveComponent(CRCD(0x9d2d0915,"angles"));
				////////////////////////////////////////////////////////////////////////////////////

				
				// first get the sector indicated by this level geometry
				uint32 checksumName = 0;
				pNode->GetChecksum( 0xa1dc81f9 /*Name*/, &checksumName);
				if (checksumName == 0)
				{
					Script::PrintContents(pNode);
					printf("FAILED BECAUSE Node %d is LevelGeometry, but has no Name=....\n",i);
					fail = true;
				}
				//Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksumName);
				Nx::CSector *p_sector = p_main_scene->GetSector(checksumName);
				
				//Dbg_MsgAssert(p_sector,("sGetSector(0x%x) returned NULL (%s)",checksumName,Script::FindChecksumName(checksumName)));
				
				if (p_sector)
				{
					 
					// The park editor takes care of it's own visibility								
					// 

					   
					if (Ed::CParkEditor::Instance()->UsingCustomPark()) 		// is it a custom park???
					{
						if ( !( pNode->ContainsFlag( 0x7c2552b9/*"CreatedAtStart"*/  ) ) || skate_mod->ShouldBeAbsentNode( pNode ) )
						{
							// if this assertion fires, you are probably try to extend the park
							// editor to have some sectors be not "createdatstart"
							// which is not currently supported, as the park editor sets up it;s
							// own visibility and "active" state for things like:
							//   - restart nodes
							//   - the cursor
							//   - gap pieces
							// so you would need to re-work it so they were correctly flagged 
							// as no created at start, and then remove this whole "UsingCustomPark" special case
							// This is also done in the logic around the call to ScriptCreateFromNodeIndex, below, in this function.
							Dbg_MsgAssert(0,("Park ed levelgeom not set created at start"));
						}					
					}
					else
					{
						// The sector will already be active and visible, since no flags will be set...
						// but if we're not created at start, set the non-collide and invisible flags:
						if ( !( pNode->ContainsFlag( 0x7c2552b9/*"CreatedAtStart"*/  ) ) || skate_mod->ShouldBeAbsentNode( pNode )
							 )
						{
							p_sector->SetActive(false);
						}
						else
						{
							// Mick: as ParseNodeArray can get called multiple times
							// on the same node array
							// we need to clear any KILLED and INVIVIBLE flags
							// that might have been set in the previous session
							p_sector->SetVisibility(0xffffffff);
							p_sector->SetActive(true);
							p_sector->SetCollidable(true);
						}
					}
					
					// If it's an occluder, then we need to flag it as such
					if (  pNode->ContainsFlag( 0x4e549fe1/*"Occluder"*/   )	)
					{
//						printf ("Occluding node %d, <%s>\n",i,Script::FindChecksumName(checksumName));
						// This object is an occluder
						// so need to flag it as so
						p_sector->SetOccluder(true);				
						
						// For now, we make all occluders invisble, so we can see through them
						//printf ("WARNING:   Occluders are being made invisible!!!!!!!\n");											
						//printf ("WARNING:   Occluders are being made invisible!!!!!!!\n");											
						//printf ("WARNING:   Occluders are being made invisible!!!!!!!\n");											
						p_sector->SetActive(false);

						// If this occluder is not created at start, disable it.
						if( !pNode->ContainsFlag( 0x7c2552b9 /*"CreatedAtStart"*/  ))
						{
							Nx::CEngine::sEnableOcclusionPoly( checksumName, false );
						}
					}
					
					// Set the light group (usually will be "outdoor")
					uint32	light_group = CRCD(0xe3714dc1,"outdoor");			 // default to OutDoors
					pNode->GetChecksum(CRCD(0x7e4813a7,"lightgroup"),&light_group);
					p_sector->SetLightGroup(light_group);
				}
				else
				{
					// Only fail in non-multiplayer games
					// as it's quite valid for a sector to not exist in a network game
					// as they are stripped out at SceneConv time 
					if (!skate_mod->IsMultiplayerGame())
					{
						//printf("sGetSector(0x%x) returned NULL (%s)",checksumName,Script::FindChecksumName(checksumName));
						printf("FAILED BECAUSE: Cannot find level geometry or collision for node %d \nMaybe there are spaces in or after the name of the Max object (%s)?\nOr maybe the .QN file does not match the .SCN file checked into Perforce?\n"
									   ,i,Script::FindChecksumName(checksumName));
						fail = true;
					}
				}
	
				bool is_trick_object = pNode->ContainsComponentNamed( "TrickObject" );
	
				if (!Config::CD())
				{
					bool debug_graffiti = Script::GetInt( "create_all_trick_objects", false );
					if ( debug_graffiti )
						is_trick_object = true;
				}		

				// Only add trick objects is they are not flagged as absentinnetgames
				if ( !skate_mod->ShouldBeAbsentNode( pNode ) && is_trick_object )
				{
					Mem::PushMemProfile("Trick Object Clusters");
					// get the node name
					uint32 checksumName;
					pNode->GetChecksum( "name", &checksumName, true );
	
					uint32 clusterName = checksumName;
					pNode->GetChecksum( "Cluster", &clusterName, false );
					skate_mod->GetTrickObjectManager()->AddTrickCluster( clusterName );
						
					// add the environment object to this cluster
					skate_mod->GetTrickObjectManager()->AddTrickObjectToCluster( checksumName, clusterName );
					Mem::PopMemProfile();
				}
				break;
			}
			default:
				break;
		}	
		
		// The following nodes are capable of being ignored in net games		
		
		if ( skate_mod->ShouldBeAbsentNode( pNode ) )
		{
			// Don't load, as it's a net game
		}	
		else 
		{
		
			if (ClassChecksum == 0xbf4d3536 && Ed::CParkEditor::Instance()->UsingCustomPark())  // LevelGeometry
			{
				// Level geometry in park editor is ignored, see above.
			}
			else
			{
				
				if (ClassChecksum == 0x8470f2e) // checksum 'ProximNode'
				{
					Mem::PushMemProfile("Proximity Nodes");
					Obj::Proxim_AddNode( i, pNode->ContainsFlag( 0x7c2552b9/*"CreatedAtStart"*/ ) );
					Mem::PopMemProfile();
				}
				
				if (ClassChecksum == 0xd64dc7c2) // checksum 'EmitterObject'
				{
					Mem::PushMemProfile("Emitter Objects");
					Obj::CEmitterManager::sAddEmitter( i, pNode->ContainsFlag( 0x7c2552b9/*"CreatedAtStart"*/ ) );
					Mem::PopMemProfile();
				}

				if ( pNode->ContainsFlag( 0x7c2552b9 /*"CreatedAtStart"*/ ) )
				{
					ScriptCreateFromNodeIndex( i );
				}
			}
		}				
	}	
	
	Dbg_MsgAssert(!fail, ("ParseNodeArray FAILED, see above for reasons labled FAILED BECAUSE \n"));
	
	Obj::Proxim_AddedAll();
	p_rail_man->AddedAll();
	Mdl::Skate::Instance()->GetGapChecklist()->FindGaps();

	/*
	// STUPID HACK
	Ed::ParkEditor* park_ed = Ed::ParkEditor::Instance();
	if (park_ed->IsInitialized() && park_ed->GameGoingOrOutsideEditor())
		park_ed->MakeStuffInvisibleForGameplay();
	*/
	
// Mick:
// Level lights need to be created AFTER the node array is parsed, so that the 
// lightgroups are set on the level geometry
// otherwise geometry nodes that occur after this one will default to "outdoor"	

	for (i=0; i<pNodeArray->GetSize(); ++i)
	{
		Script::CStruct *pNode=pNodeArray->GetStructure(i);
		Dbg_MsgAssert(pNode,("NULL pNode"));

		uint32 ClassChecksum = 0;
		pNode->GetChecksum( 0x12b4e660 /*"Class"*/, &ClassChecksum );


		// First the nodes that need some special processing
		// for the "net game" state
		if (ClassChecksum == CRCD(0xa0e52802,"LevelLight"))
		{						   
			// Will need to check a parameter here to see if the light is flagged to affect objects.
			bool affects_objects	= !pNode->ContainsFlag( CRCD( 0xe1509e4f, "ExcludeSkater" ));
			bool affects_geometry	= !pNode->ContainsFlag( CRCD( 0x6f050e18, "ExcludeLevel" ));

			// Get the name checksum of this node, since this will be used as the link between the node and the Scene Light.
			uint32 name_checksum = SkateScript::GetNodeNameChecksum( i );

			Mth::Vector pos;
			SkateScript::GetPosition( pNode, &pos );

			// Default values.
			float intensity		= 100.0f;
			float inner_radius	= 300.0f;
			float outer_radius	= 300.0f;

			pNode->GetFloat( CRCD( 0x2689291c, "Brightness" ), &intensity );	
			pNode->GetFloat( CRCD( 0x8d2c287f, "InnerRadius" ), &inner_radius );
			pNode->GetFloat( CRCD( 0x833950cc, "OuterRadius" ), &outer_radius );

			if( !pNode->ContainsFlag( CRCD( 0x7c2552b9, "CreatedAtStart" )))
			{
				// This light is not supposed to be active at startup.
			}

			// Lights default to pure white in the absence of any color information.
			Image::RGBA col( 0xFF, 0xFF, 0xFF, 0x00 );

			Script::CArray *p_array;
			pNode->GetArray( CRCD( 0x99a9b716, "Color" ), &p_array );
			if( p_array )
			{
				col.r = p_array->GetInteger( 0 );
				col.g = p_array->GetInteger( 1 );
				col.b = p_array->GetInteger( 2 );
			}

			if( affects_objects )
			{
				Nx::CSceneLight *p_new_light = Nx::CLightManager::sAddSceneLight();
				if( p_new_light )
				{
					p_new_light->SetNameChecksum( name_checksum );
					p_new_light->SetLightPosition( pos );

					// Set the color, radius and intensity.
					p_new_light->SetLightIntensity( intensity * 0.01f );
					p_new_light->SetLightColor( col );
					p_new_light->SetLightRadius( outer_radius );
				}
				else
				{
					Dbg_MsgAssert(0,("Too many Scene lights"));
				}
			}

			if( affects_geometry )
			{
				Nx::CLightManager::sAddVCLight(name_checksum,pos,intensity,col,outer_radius);
			}
		}
	}	
	

	Mem::PopMemProfile(/*"ParseNodeArray"*/);

	return true;
}	

static char LastNodeArrayLoaded[128];

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadNodeArray | Will load node array from the file specified 
// @uparm "string" | File name to load
bool ScriptLoadNodeArray(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mem::PushMemProfile("LoadNodeArray");
	
	// Remove any spawned scripts that might have been left over from the previous level.
	Script::DeleteSpawnedScripts();
		
	
	const char *pFileName=NULL;
	pParams->GetText(NONAME,&pFileName);
	Dbg_MsgAssert(pFileName,("LoadNodeArray requires a file name."));

	strcpy(LastNodeArrayLoaded,pFileName);

	// Load the QB
	SkateScript::LoadQB(pFileName);
	
	// Ensure it has a node array in it
	Script::CArray* pNodeArray=Script::GetArray(CRCD(0xc472ecc5,"NodeArray"));
	Dbg_MsgAssert(pNodeArray,("No NodeArray found in %s",pFileName));

	for ( uint32 i = 0; i < pNodeArray->GetSize(); i++ )
	{
		Script::CStruct* pStructure = pNodeArray->GetStructure(i);
		pStructure->AddInteger("nodeIndex", i);
	}

	if (pParams->ContainsFlag("park_editor"))
	{
		// PARKED4:
		// Nothing needs doing here, we don't preload any models in the park editor
	}
	else
	{
		// not a park editor QB, parse as normal
//		ScriptPreloadModels( NULL, NULL );
	}
	Mem::PopMemProfile(/*"LoadNodeArray"*/);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ReLoadNodeArray | Will reload the last node array you loaded, 
// without parsing it, and without deleting anything. 
// Note, this will load directly from the .qb, so there is NO NEED to rebuild the .pre files. 
// Using this command by itself could be used if you are just altering 
// stuff like trigger scripts.  However, as it does not respawn anything, 
// you have the potential to mess things up.  Give it a go though, you can 
// always just do a Select+Circle to make sure. 
bool ScriptReLoadNodeArray(Script::CStruct *pParams, Script::CScript *pScript)
{	
	
	// We try to use the Debug heap, as we assume we will be fragmenting memory like a jackass
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
	
	const char *pFileName=LastNodeArrayLoaded;
	// Load the QB
	SkateScript::LoadQB(pFileName);
	
	// Ensure it has a node array in it
	Script::CArray *pNodeArray=Script::GetArray(CRCD(0xc472ecc5,"NodeArray"));
	
	Dbg_MsgAssert(pNodeArray,("No NodeArray found in %s",pFileName));
		
	// renumber node indices
	for ( uint32 i = 0; i < pNodeArray->GetSize(); i++ )
	{
		Script::CStruct* pStructure = pNodeArray->GetStructure(i);
		pStructure->AddInteger("nodeIndex", i);
	}
	
	Mem::Manager::sHandle().PopContext();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetBackgroundColor | Sets the background color. Each component 
// is in the range 0-255.  The alpha value currently is not used. 
// @parmopt int | r | 0 | Red component
// @parmopt int | g | 0 | Green component
// @parmopt int | b | 0 | Blue component
// @parmopt int | alpha | 0 | Alpha value
bool ScriptSetBackgroundColor(Script::CStruct *pParams, Script::CScript *pScript)
{
	    
	int Red=0;
	int Green=0;
	int Blue=0;
	int Alpha=0;
	pParams->GetInteger("r",&Red);
	pParams->GetInteger("g",&Green);
	pParams->GetInteger("b",&Blue);
	pParams->GetInteger("alpha",&Alpha);
    
/*	
	gfx_man->sBackgroundColor.r 	= Red;
	gfx_man->sBackgroundColor.g = Green;
	gfx_man->sBackgroundColor.b 	= Blue;
	gfx_man->sBackgroundColor.a = Alpha;   
*/
	printf ("STUBBED ScriptSetBackgroundColor\n");	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetBSPAmbientColor | Sets the ambient color for the BSP 
// (The environment, or the non-moving objects). 
// NOTE:  This should be set to 0,0,0, as any other setting will result 
// in much slowdown.  However, you can set it temporarily if you 
// want to try out effects quickly. 
// @parmopt int | r | 0 | Red component
// @parmopt int | g | 0 | Green component
// @parmopt int | b | 0 | Blue component
// @parmopt int | alpha | 0 | Alpha value
bool ScriptSetBSPAmbientColor(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_Message( "SetBSPAmbientColor is obsolete" );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetDFFAmbientColor | Sets the ambient color for the DFFs 
// (The skater, cars and suchlike.).
// @parmopt int | r | 0 | Red component
// @parmopt int | g | 0 | Green component
// @parmopt int | b | 0 | Blue component
// @parmopt int | alpha | 0 | Alpha value
bool ScriptSetDFFAmbientColor(Script::CStruct *pParams, Script::CScript *pScript)
{			
	Dbg_Message( "SetDFFAmbientColor is obsolete" );
	return true;
}
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetDFFDirectColor | Sets the directional light color 
// for the DFFs (The skater, cars and suchlike.).   (Note:  temporary, 
// until we get a better lighting setup). 
// @parmopt int | r | 0 | Red component
// @parmopt int | g | 0 | Green component
// @parmopt int | b | 0 | Blue component
// @parmopt int | alpha | 0 | Alpha value
bool ScriptSetDFFDirectColor(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_Message( "SetDFFDirectColor is obsolete" );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetDynamicLightModulationFactor | A value of 0.0 means no 
// effect (light/dark polys have no effect). 
// A value of 1.0 means the maximum possible effect from light/dark polys 
// These values will stay set until changed, so designers will want to
// put these in their level startup scripts. 
// @parm float | value | 
// @parmopt flag | ambient | | 
// @parmopt bool | directional | | 
bool ScriptSetDynamicLightModulationFactor( Script::CStruct* pParams, Script::CScript* pScript )
{
	

	bool	ok;
	int		index	= 0;
	float	value	= 0.0f;
	if( pParams->GetFloat( "value", &value ))
	{
		if( pParams->ContainsFlag( "ambient" ))
		{
			Nx::CLightManager::sSetAmbientLightModulationFactor(value);
		}
		else
		{
			ok = pParams->GetInteger( "directional", &index );
			Nx::CLightManager::sSetDiffuseLightModulationFactor(index, value);
		}
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetClippingDistances | Sets the clipping distances in inches. 
// “Near” should generally be left at 12, “Far” should be whatever is 
// appropriate for your level. Example: 
// SetClippingDistances Near=12 Far=8000  
// @parm float | Near |
// @parm float | Far | 
bool ScriptSetClippingDistances(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	float Near=Gfx::Camera::vDEFAULT_FARZ;
	float Far=Gfx::Camera::vDEFAULT_NEARZ;
	pParams->GetFloat("Near",&Near);
	pParams->GetFloat("Far",&Far);

	// This is a bit of a patch,
	// cycle over the two possible cameras, and set the clipping dist
	for (int cam = 0; cam < 2; cam ++)
	{
		
		Gfx::Camera *p_cam = Nx::CViewportManager::sGetCamera(cam);
		if (p_cam)
		{
			p_cam->SetNearFarClipPlanes(Near,Far);
		}
	}
	//Gfx::Camera::SetNearFarClipPlanes(Near,Far);


	// Also Gamecube uses different near and far z clip planes, and has a fog far plane independent of the
	// camera far plane. So indicate here what the fog far plane for Gamecube should be.
//#	ifdef __PLAT_NGC__
//	NsRender::setFogFar( Far );
//#	endif // __PLAT_NGC__



	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetTrivialFarZClip | Sets the Z clipping distance 
// SetTrivialFarZClip on        --- to turn it on
// SetTrivialFarZClip off       --- to turn it off 
// @flag on | Pass this in to set the trivial Z clipping
// distance to true or false
bool ScriptSetTrivialFarZClip(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	bool on = false;

	if( pParams->ContainsFlag( "on" ))
	{
		on = true;
	}

	 Gfx::Manager * gfx_manager =  Gfx::Manager::Instance();
	gfx_manager->SetTrivialFarZClip( on );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EnableFog | 
bool ScriptEnableFog( Script::CStruct *pParams,
					  Script::CScript *pScript )
{
	Nx::CFog::sEnableFog(true);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DisableFog | 
bool ScriptDisableFog( Script::CStruct *pParams,
					  Script::CScript *pScript )
{
	Nx::CFog::sEnableFog(false);

	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetFogDistance | Sets the "near" distance (in inches) 
// at which fogging will begin.  (Polys closer than the near distance
// will not be fogged.  Beyond the near distance, they will become
// increasingly fogged until they reach maximum fogging at the far
// clip distance -- see SetClippingDistances).
// @parm float | distance | distance in inches
bool ScriptSetFogDistance( Script::CStruct *pParams,
						   Script::CScript *pScript )
{
	
	float fog_dist = 0.0f;
	pParams->GetFloat( "distance", &fog_dist );

	Nx::CFog::sSetFogNearDistance(fog_dist);

	// Garrett: Not sure if this is needed anymore, but leaving it there
	// in case some other platforms need it.
	// This is a bit of a patch,
	// cycle over the two possible cameras, and set the clipping dist
	for ( int cam = 0; cam < 2; cam++ )
	{
		Gfx::Camera *p_cam = Nx::CViewportManager::sGetCamera(cam);
		if ( NULL != p_cam )
		{
			p_cam->SetFogNearPlane( fog_dist );
		}
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetFogExponent | Sets the fog exponent.  The exponent
// value determines the "spread" of the fog, or how far out you
// must go before you reach maximum fog.  This component of fog
// will probably change in the near future.
// @parm float | exponent | fog exponent
bool ScriptSetFogExponent( Script::CStruct *pParams,
						   Script::CScript *pScript )
{
	float exponent;
	if (!pParams->GetFloat( "exponent", &exponent ))
	{
		Dbg_MsgAssert(0, ("Can't find parameter 'exponent'"));
	}

	Nx::CFog::sSetFogExponent(exponent);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetFogColor | Sets the color that will be used for
// atmospheric fogging.  Color components are in the range 0-255. 
// The alpha component is in the range of 0-128.
// @parmopt int | r | 255 | red component
// @parmopt int | g | 255 | green component
// @parmopt int | b | 255 | blue component
// @parmopt int | a | 128 | alpha component (opacity of fog)
bool ScriptSetFogColor( Script::CStruct *pParams,
						Script::CScript *pScript )
{
	
	int red = 255;
	int green = 255;
	int blue = 255;
	int alpha = 128;
	pParams->GetInteger( "r", &red );
	pParams->GetInteger( "g", &green );
	pParams->GetInteger( "b", &blue );
	pParams->GetInteger( "a", &alpha );

	Image::RGBA rgba(red, green, blue, alpha);

	Nx::CFog::sSetFogRGBA( rgba );
		
	return true;
}

// @script | SetUVWibbleParams | Sets the UV wibble parameters of a sector
// @parm name | sector | name of sector
// @parm float | u_vel   | u velocity
// @parm float | u_amp   | u amplitude
// @parm float | u_freq  | u frequency
// @parm float | u_phase | u phase
// @parm float | v_vel   | v velocity
// @parm float | v_amp   | v amplitude
// @parm float | v_freq  | v frequency
// @parm float | v_phase | v phase
bool ScriptSetUVWibbleParams(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Get sector checksum
	uint32 checksum;
	if (!pParams->GetChecksum("sector", &checksum))
	{
		Dbg_MsgAssert(0, ("Can't find parameter sector"));
	}

	// Find sector
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksum);
	Dbg_MsgAssert(p_sector, ("Can't find sector %x in world", checksum));

	// Extract all the parameters
    float u_velocity;
    float u_amplitude;
    float u_frequency;
    float u_phase;
    float v_velocity;
    float v_amplitude;
    float v_frequency;
    float v_phase;
	bool found_all;

	found_all  = pParams->GetFloat("u_vel", &u_velocity);
	found_all &= pParams->GetFloat("u_amp", &u_amplitude);
	found_all &= pParams->GetFloat("u_freq", &u_frequency);
	found_all &= pParams->GetFloat("u_phase", &u_phase);
	found_all &= pParams->GetFloat("v_vel", &v_velocity);
	found_all &= pParams->GetFloat("v_amp", &v_amplitude);
	found_all &= pParams->GetFloat("v_freq", &v_frequency);
	found_all &= pParams->GetFloat("v_phase", &v_phase);

	Dbg_MsgAssert(found_all, ("Missing one or more of the wibble parameters.  Must fill them all out."));

	p_sector->SetUVWibbleParams(u_velocity, u_amplitude, u_frequency, u_phase, v_velocity, v_amplitude, v_frequency, v_phase);

	return true;
}

// @script | EnableExplicitUVWibble | Enables explicit setting of the UV wibble offsets.
// To set the offsets, call SetUVWibbleOffsets.
// @parm name | sector | name of sector
bool ScriptEnableExplicitUVWibble(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Get sector checksum
	uint32 checksum;
	if (!pParams->GetChecksum("sector", &checksum))
	{
		Dbg_MsgAssert(0, ("Can't find parameter sector"));
	}

	// Find sector
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksum);
	Dbg_MsgAssert(p_sector, ("Can't find sector %x in world", checksum));

	p_sector->UseExplicitUVWibble(true);

	return true;
}

// @script | DisableExplicitUVWibble | Disables explicit setting of the UV wibble offsets.
// @parm name | sector | name of sector
bool ScriptDisableExplicitUVWibble(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Get sector checksum
	uint32 checksum;
	if (!pParams->GetChecksum("sector", &checksum))
	{
		Dbg_MsgAssert(0, ("Can't find parameter sector"));
	}

	// Find sector
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksum);
	Dbg_MsgAssert(p_sector, ("Can't find sector %x in world", checksum));

	p_sector->UseExplicitUVWibble(false);

	return true;
}

// @script | SetUVWibbleOffsets | Sets the UV wibble offsets explicitly
// @parm name | sector | name of sector
// @parm float | u_off | u offset
// @parm float | v_off | v offset
bool ScriptSetUVWibbleOffsets(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Get sector checksum
	uint32 checksum;
	if (!pParams->GetChecksum("sector", &checksum))
	{
		Dbg_MsgAssert(0, ("Can't find parameter sector"));
	}

	// Find sector
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksum);
	Dbg_MsgAssert(p_sector, ("Can't find sector %x in world", checksum));

	// Extract all the parameters
    float u_offset;
    float v_offset;
	bool found_all;

	found_all  = pParams->GetFloat("u_off", &u_offset);
	found_all &= pParams->GetFloat("v_off", &v_offset);

	Dbg_MsgAssert(found_all, ("Missing one or more of the wibble offset parameters.  Must fill them all out."));

	p_sector->SetUVWibbleOffsets(u_offset, v_offset);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetPreferenceString(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Script::CStruct* pStructure;
	Prefs::Preferences* pPreferences;
	uint32 field_id, pref_type;
	const char* ui_string;
	CStruct* pass_back_params;
    
	pass_back_params = pScript->GetParams();

	pParams->GetChecksum( NO_NAME, &field_id );
	pParams->GetChecksum( "pref_type", &pref_type );

	pPreferences = Mdl::GetPreferences(pref_type); // This will assert if it cannot find the prefs
	
	pStructure = pPreferences->GetPreference( field_id );
	Dbg_MsgAssert(pStructure, ("\n%s\nCan't get preference 0x%x (%s)",pScript->GetScriptInfo(),field_id, Script::FindChecksumName(field_id)));
	
	//pParams->AddStructure( "preference", pStructure );
	pStructure->GetString( "ui_string", &ui_string );
	pass_back_params->AddString( "ui_string", ui_string );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetPreferencePassword(Script::CStruct *pParams, Script::CScript *pScript)
{
	Script::CStruct* pStructure;
	Prefs::Preferences* pPreferences;
	uint32 field_id, pref_type;
	const char* ui_string;
	CStruct* pass_back_params;
	static char	s_password_string[32];
	uint32 i;
    
	pass_back_params = pScript->GetParams();

	pParams->GetChecksum( NO_NAME, &field_id );
	pParams->GetChecksum( "pref_type", &pref_type );

	pPreferences = Mdl::GetPreferences(pref_type); // This will assert if it cannot find the prefs

	pStructure = pPreferences->GetPreference( field_id );
	//pParams->AddStructure( "preference", pStructure );
	pStructure->GetString( "ui_string", &ui_string );
	for( i = 0; i < strlen( ui_string ); i++ )
	{
		s_password_string[i] = '*';
	}
	
	Dbg_Assert( i < 32 );
	s_password_string[i] = '\0';

	pass_back_params->AddString( "password_string", s_password_string );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetPreferenceChecksum(Script::CStruct *pParams, Script::CScript *pScript)
{
	Script::CStruct* pStructure;
	Prefs::Preferences* pPreferences;
	uint32 field_id, checksum, pref_type;
	CStruct* pass_back_params;
    
	pass_back_params = pScript->GetParams();

	pParams->GetChecksum( NO_NAME, &field_id );
	pParams->GetChecksum( "pref_type", &pref_type );

	pPreferences = Mdl::GetPreferences(pref_type); // This will assert if it cannot find the prefs

	pStructure = pPreferences->GetPreference( field_id );
	//pParams->AddStructure( "preference", pStructure );
	pStructure->GetChecksum( "checksum", &checksum );
	pass_back_params->AddChecksum( "checksum", checksum );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetPreference | sets the prefs to those specified in
// the params structure
// @parm name | field | the field id
// @parm structure | params | 
bool ScriptSetPreference(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	// gets either network or splitscreen prefs, as appropriate
	Prefs::Preferences* pPreferences = Mdl::GetPreferences( pParams );
	
	uint32 field_id;
	pParams->GetChecksum( "field", &field_id, true );

	Script::CStruct* pSubParams;
	pParams->GetStructure( "params", &pSubParams, true );
	pPreferences->SetPreference( field_id, pSubParams );

#ifdef __NOPT_ASSERT__
	Script::PrintContents(pSubParams);
#endif
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PreferenceEquals | returns true if the actual preference value
// for the given field equals the test value
// @parm name | field | the field id
// @parm name | equals | test value
bool ScriptPreferenceEquals(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	// gets either network or splitscreen prefs, as appropriate
	Prefs::Preferences* pPreferences = Mdl::GetPreferences( pParams );

	uint32 field_id;
	pParams->GetChecksum("field", &field_id, true);

	uint32 test_value;
	pParams->GetChecksum( "equals", &test_value, true );

	Script::CStruct* pStructure = pPreferences->GetPreference( field_id );
	Dbg_Assert(pStructure);

	uint32 actual_value;
	pStructure->GetChecksum( "checksum", &actual_value, true );

	return ( test_value == actual_value );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetCamera | 
bool ScriptResetCamera(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	// This is a bit of a patch,
	// cycle over the two possible cameras, and reset them
	for (int cam = 0; cam < 2; cam ++)
	{
		
		Gfx::Camera *p_cam = Nx::CViewportManager::sGetCamera(cam);
		if (p_cam)
		{
			printf ("TODO: ScriptResetCamera not implemented\n");
		}
	}
	//Gfx::Camera::SetNearFarClipPlanes(Near,Far);
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetVolumeFromValue | sets the volume according to the slider
// @uparm sfxvol | the volume to set (either sfxvol or cdvol)
// @parm name | id | the slider id used to change the volume
bool ScriptSetVolumeFromValue( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	/*
	uint32 whichParam;
	pParams->GetChecksum( NONAME, &whichParam, true );

	uint32 slider_id;
	pParams->GetChecksum( "id", &slider_id, true );

	Front::MenuFactory* menu_factory = Front::MenuFactory::Instance();
	Front::SliderMenuElement* pSliderElement = static_cast<Front::SliderMenuElement*>( menu_factory->GetMenuElement( slider_id, true ) );
	Dbg_Assert( pSliderElement );

	switch ( whichParam )
	{
		case 0x8d6f6554: // "sfxvol"
		{
			 Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
			sfx_manager->SetMainVolume( ( float )( 10 * pSliderElement->GetValue( ) ) );
		}
		break;

		case 0xa5162bd2: // "cdvol"
		{
			// still set the main music volume even if in the front end!!!
			Pcm::SetVolume( ( float )( 10 * pSliderElement->GetValue( ) ) );

			// make sure the music is playing and shit...
			Pcm::PauseMusic( false );
		}
		break;

		default:
		{
			// unrecognized sound parameter
			Dbg_MsgAssert( 0, ( "Unrecognized sound parameter" ) );
		}
		break;
	}
	*/
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetValueFromVolume | 
// @uparm sfxvol | the volume you're interested in (either sfxvol, cdvol, or cutvol)
bool ScriptGetValueFromVolume( Script::CStruct *pParams, Script::CScript *pScript )
{
	

	uint32 whichParam;
	pParams->GetChecksum( NONAME, &whichParam, true );

	switch ( whichParam )
	{
		case 0x8d6f6554: // "sfxvol"
		{
			 Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
			pScript->GetParams()->AddComponent( Script::GenerateCRC( "value" ), ESYMBOLTYPE_INTEGER, ( (int) sfx_manager->GetMainVolume() ) / 10 );
		}
		break;
		
		case 0xa5162bd2: // "cdvol"
		{
			pScript->GetParams()->AddComponent( Script::GenerateCRC( "value" ), ESYMBOLTYPE_INTEGER, ( (int) Pcm::GetVolume() ) / 10 );
		}
		break;

		case 0xe32f3525: // "cutvol"
		{
			pScript->GetParams()->AddComponent( Script::GenerateCRC( "value" ), ESYMBOLTYPE_INTEGER, ( (int) Pcm::GetMusicStreamVolume() ) / 10 );
		}
		break;

		default:
		{
			// unrecognized sound parameter
			Dbg_MsgAssert( 0, ( "Unrecognized sound parameter" ) );
		}
		break;
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSliderValue | 
// @parm name | id | the slider id
// @parm int | value | value to assign to slider
bool ScriptSetSliderValue( Script::CStruct *pParams, Script::CScript *pScript )
{
	

	/*
	uint32 slider_id;
	pParams->GetChecksum( "id", &slider_id, true );

	int value;
	pParams->GetInteger( "value", &value, true );
	
	Front::MenuFactory* menu_factory = Front::MenuFactory::Instance();
	Front::SliderMenuElement* pSliderElement = static_cast<Front::SliderMenuElement*>( menu_factory->GetMenuElement( slider_id, true ) );
	Dbg_Assert( pSliderElement );
	pSliderElement->SetValue( value );
	*/

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetIconTexture | 
// @parm name | id | menuelement
// @parm string | texture | the texture name
bool ScriptSetIconTexture(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	/*
	uint32 id;
	pParams->GetChecksum("id", &id, true);
	
	Front::MenuFactory* menu_factory = Front::MenuFactory::Instance();
	Front::IconMenuElement* pIconElement = static_cast<Front::IconMenuElement*>( menu_factory->GetMenuElement( id, true ) );

	if ( pIconElement )
	{
		const char* textureName;
		pParams->GetText("texture", &textureName, true);
		pIconElement->SetTexture( textureName );
	}
	*/

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


// @script | SetMovementVelocity | Sets the movement (translational) 
// speed of the camera in the viewer. The number is in terms of inches per second. 
// @parmopt float | vel | 400 | velocity in inches per second
bool ScriptSetMovementVelocity( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	float vel = 400;
	pParams->GetFloat( NONAME, &vel );

	Mdl::CViewer::sSetMovementVelocity( vel );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetRotateVelocity | Sets the rotational speed of the 
// camera in the viewer. The number is in terms of degrees per second. 
// @parmopt float | vel | 120 | Rotational speed in degrees per second
bool ScriptSetRotateVelocity( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	float vel = 120;
	pParams->GetFloat( NONAME, &vel );

	Mdl::CViewer::sSetRotateVelocity( vel );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | OnReload | 
// This is a script function that doesn't do anything, it is just used to indicate what should
// happen if the script gets reloaded at runtime. It needs to be the first 
bool ScriptOnReload(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetEngine | resets the engine
bool ScriptResetEngine(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	 Mlp::Manager * mlp_manager =  Mlp::Manager::Instance();
	mlp_manager->QuitLoop();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleMetrics | Toggle metrics - use before ToggleMemMetrics
bool ScriptToggleMetrics(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	 Gfx::Manager * gfx_manager =  Gfx::Manager::Instance();
	gfx_manager->ToggleMetrics();
    //Script::RunScript( "show_poly_count" );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleNetMetrics | 
bool ScriptToggleNetMetrics(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	 GameNet::Manager * gamenet_manager =  GameNet::Manager::Instance();
	gamenet_manager->ToggleMetrics();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleVRAMViewer | Toggle VRAM viewer
bool ScriptToggleVRAMViewer(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	 Gfx::Manager * gfx_manager =  Gfx::Manager::Instance();
	gfx_manager->ToggleVRAMViewer();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DumpVRAMUsage | Spits out a file detailing VRAM usage 
// separately for all four contexts
bool ScriptDumpVRAMUsage(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	 Gfx::Manager * gfx_manager =  Gfx::Manager::Instance();
	gfx_manager->DumpVRAMUsage();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleLightViewer | Toggle the light viewer
bool ScriptToggleLightViewer(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_Message( "ToggleLightViewer is obsolete" );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | debugrendermode | currently unsupported
bool ScriptSetDebugRenderMode(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_Message( "debugrendermode is obsolete" );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | debugtoggletextureupload | currently unsupported
bool ScriptToggleTextureUpload(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_Message( "debugtoggletextureupload is obsolete" );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | debugtoggletexturedraw | currently unsupported
bool ScriptToggleTextureDraw(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_Message( "debugtoggletexturedraw is obsolete" );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | debugtogglepointdraw | currently unsupported
bool ScriptTogglePointDraw(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_Message( "debugtogglepointdraw is obsolete" );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | debugrendertest1 | old function?  returns true if mbr_nsDebugTest1
// (check cfuncs.cpp) is set
bool ScriptRenderTest1(Script::CStruct *pParams, Script::CScript *pScript)
{
	
#ifdef __PLAT_NGPS__
//	mbr_nsDebugTest1=(mbr_nsDebugTest1)?0:1;
#endif
	return true;
}
// @script | debugrendertest2 | old function?  returns true if mbr_nsDebugTest2
// (check cfuncs.cpp) is set
bool ScriptRenderTest2(Script::CStruct *pParams, Script::CScript *pScript)
{

#ifdef __PLAT_NGPS__
//	mbr_nsDebugTest2=(mbr_nsDebugTest2)?0:1;
#endif
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetViewMode | Debugging function to toggle the camera mode,
// switching between normal, free with fixed skater, and free with moving skater
// usually triggers with Select+X (See buttonscripts.q)
bool ScriptSetViewMode(Script::CStruct *pParams, Script::CScript *pScript)
{
	int viewMode;
	pParams->GetInteger( NONAME, &viewMode, Script::ASSERT );

	Dbg_Assert( viewMode >= 0 && viewMode < Obj::NUM_VIEW_MODES );

	s_view_mode = viewMode;
	Mdl::CViewer::sSetViewMode( s_view_mode );
	// Handle toggling to different cameras based on view mode
	
	switch (s_view_mode )
	{
		case Obj::GAMEPLAY_SKATER_ACTIVE:
			SetActiveCamera(CRCD(0x967c138c,"skatercam0"),0, false);			
			break;
		default:
			SetActiveCamera(CRCD(0xeb17151b,"viewer_cam"),0, true);						
			break;		
	}
	
		
	Obj::CSkater* pSkater;
	for ( int i = 0; i < 8; i++ )
	{
		 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
		pSkater = skate_mod->GetSkater(i);						   
		if ( pSkater )			// Skater might not exist
		{
			pSkater->SetViewMode( (Obj::EViewMode)s_view_mode );
		}
	}
	
	// clear out the viewer object, if any
	Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();
	if ( pViewer )
	{
		pViewer->RemoveViewerObject();   	
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleMetricItem | ToggleMetricItem will turn on or off 
// individual items in the metric display.
// The “item” parameter is one of: <nl>
// METRIC_TIME <nl>
// METRIC_ARENAUSAGE <nl>            
// METRIC_TOTALPOLYS <nl>
// METRIC_POLYSPROC <nl>
// METRIC_VERTS <nl>
// METRIC_RESOURCEALLOCS <nl>
// METRIC_TEXTUREUPLOADS <nl>
// METRIC_VU1 <nl>
// METRIC_DMA1 <nl>
// METRIC_DMA2 <nl>
// METRIC_VBLANKS <nl>
// METRIC_DRAWTIME <nl>
// METRIC_IHANDLERTIME <nl>
// METRIC_SKYCACHE <nl>
// METRIC_VIDEOMODE <nl>
// METRIC_VRAMUSAGE <nl>
// METRIC_MEMUSED <nl>
// METRIC_MEMFREE <nl>
// METRIC_REGIONINFO <nl>
// @parmopt int | item | 0 | 
bool ScriptToggleMetricItem( Script::CStruct *pParams, Script::CScript *pScript )
{

	
/*
	uint32 metricFlags = 0;
	pParams->GetInteger( "item", ( int * )&metricFlags );

	 Gfx::Manager * gfx_manager =  Gfx::Manager::Instance();

	Gfx::Metrics* pMetrics = gfx_manager->GetMetrics();
	pMetrics->ToggleMetricItem( metricFlags );
*/
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleMemMetrics | This will replace the PS2 specific
// metrics with a list of the all the memory heaps, together with
// the amount of fragmentation, and the amount of free memory. Note, 
// you have to use ToggleMetrics to actually see the metrics before
// using ToggleMemMetrics <nl>
// See the online doc for a full explanation of the various heaps
bool ScriptToggleMemMetrics( Script::CStruct *pParams, Script::CScript *pScript )
{
	/* 
	 Gfx::Manager * gfx_manager =  Gfx::Manager::Instance();
	Gfx::Metrics* pMetrics = gfx_manager->GetMetrics();
	pMetrics->ToggleMemMetrics();
	*/
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// @script | KillAllTextureSplats | Destroys all exisiting texture splats
bool ScriptKillAllTextureSplats( Script::CStruct *pParams, Script::CScript *pScript )
{
	Nx::KillAllTextureSplats();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ProximCleanup | Destroys all proxim nodes
bool ScriptProximCleanup(Script::CStruct *pParams, Script::CScript *pScript)
{
	// TT12597: Clear out the proxim node pointers in the stream and sound components
	// new fast way, just go directly to the components, if any
	Obj::CSoundComponent* p_sound_component = static_cast<Obj::CSoundComponent*>( Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( CRC_SOUND ));
	while( p_sound_component )
	{
		p_sound_component->ClearProximNode();
		
		p_sound_component = static_cast<Obj::CSoundComponent*>( p_sound_component->GetNextSameType());
	}

	Obj::CStreamComponent* p_stream_component = static_cast<Obj::CStreamComponent*>( Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( CRC_STREAM ));
	while( p_stream_component )
	{
		p_stream_component->ClearProximNode();
		
		p_stream_component = static_cast<Obj::CStreamComponent*>( p_stream_component->GetNextSameType());
	}

	Obj::Proxim_Cleanup();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Cleanup | Destroys current world, texture dictionaries, objects and railnodes.
// @flag preserve_skaters | Set this to preserve the skaters
bool ScriptCleanup(Script::CStruct *pParams, Script::CScript *pScript)
{
	//	DumpUnwindStack( 20, 0 );
	// Wait for any async rendering to finish before we proceed	
	Nx::CEngine::sFinishRendering();
 	#ifdef	__PLAT_NGPS__
	NxPs2::AllocateExtraDMA(false);
	#endif

	Replay::DeallocateReplayMemory();
	
// goalmanager stuff moved here from Skate::ChangeLevel
    Game::CGoalManager* p_GoalManager = Game::GetGoalManager();
    Dbg_MsgAssert( p_GoalManager, ( "couldn't get GoalManager\n" ) );
    
    p_GoalManager->DeactivateAllGoals();
    p_GoalManager->RemoveAllGoals();

	// PARKED4:
	Ed::CParkEditor* p_park_editor = Ed::CParkEditor::Instance();
	p_park_editor->Cleanup();
	p_park_editor->DeletePlayModeGapManager();
	
	Mdl::Skate * skate_mod =  Mdl::Skate::Instance();	
	bool preserve;
	bool was_spawned;

	Dbg_Printf( "Performing Cleanup\n" );

	Gfx::DebuggeryLines_CleanUp( );	// so we remove debug lines between levels

	preserve = false;
	if( pParams )
	{
		preserve = pParams->ContainsFlag("preserve_skaters");
	}	

// ********************************************************************************************
// ********************************************************************************************
// ********************************************************************************************
	// A hacky way of preserving the running script, since skate's cleanup will destroy all
	// spawned scripts.
	was_spawned = false;
	if( pScript )
	{
		was_spawned = pScript->mIsSpawned;
		pScript->mIsSpawned = false;
	}
	// Delete's session objects managed by the "Skate::" code module	
	skate_mod->Cleanup( );   
	if( pScript )
	{
		pScript->mIsSpawned = was_spawned;
	}
	
	// Destroy other Session Objects
// These can be recreated, no assets are unloaded

	//skate_mod->GetGoalManager()->RemoveAllGoals();	

	Dbg_Printf( "Destroying imposters..." );
	Nx::CEngine::sGetImposterManager()->Cleanup();

	Dbg_Printf( "Destroying misc fx..." );
	Nx::MiscFXCleanup();

	Dbg_Printf( "Clearing node-name hash table ..." );
	SkateScript::ClearNodeNameHashTable();

	// Turn off fogging...let the next level turn it back on if it wants it
	Nx::CFog::sEnableFog(false);

	Script::KillStoppedScripts();


	Dbg_Printf( "Cleaning up terrain soundFX manager." );
	Env::CTerrainManager::sReset();
	Dbg_Printf( "Done.\n" );

				   
// ********************************************************************************************
// ********************************************************************************************
// ********************************************************************************************
// Actual unloading of assets starts here 
	
// unload sond effects (just resets the SRAM pointer)				   
	Dbg_Printf( "Cleaning up soundFX manager." );
	 Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
	sfx_manager->CleanUp( );		// Sfx manager uses a very primitive memory management system ... check it out..
	Dbg_Printf( "Done.\n" );

// Unload object models and animations													 
	printf ("unloading assman tables ...........\n");
	 Ass::CAssMan * ass_manager =  Ass::CAssMan::Instance();
	ass_manager->UnloadAllTables( );
	printf ("done unloading assman ...........\n");

// ********************************************************************************************
// ********************************************************************************************
// ********************************************************************************************
	Dbg_Printf( "Unloading NodeArray .QB (level .qn file)" );
	Script::CSymbolTableEntry *p_sym=Script::LookUpSymbol(CRCD(0xc472ecc5,"NodeArray"));
	if (p_sym)
	{
		SkateScript::UnloadQB(p_sym->mSourceFileNameChecksum);
	}	
	
// ********************************************************************************************
// ********************************************************************************************
// ********************************************************************************************
// Unload the level specific qb.
	// Note: It's ok if s_level_specific_qb is 0, UnloadQB just won't do anything if it can't find it.
	#if 0
	SkateScript::UnloadQB(s_level_specific_qb);
	s_level_specific_qb=0;
	#else
	for (int qb=0;qb<MAX_LEVEL_QBS;qb++)
	{
		SkateScript::UnloadQB(s_level_specific_qbs[qb]);
		s_level_specific_qbs[qb]=0;
	}
	#endif
	

// ********************************************************************************************
// ********************************************************************************************
// ********************************************************************************************
	//  Unload the level geometry
					
	printf ("Starting to unload Geometry from the engine .....\n");
	ScriptUnloadAllLevelGeometry(NULL,NULL);
	printf ("Done unloading Geometry from the engine .....\n");					 

// ********************************************************************************************
// ********************************************************************************************
// ********************************************************************************************
	// Maybe unload the skaters		 
	if (preserve)
	{
		//ScriptUnhookSkaters(NULL,NULL);	    // just tell the skaters that there is no world loaded  (maybe not needed)
	
						/*
		// NASTY (TEMPORARY) PATCH
		// since the skater is preserved, but the animations have been deleted
		// then we need to recreate the references to them (what was the "animarray")	
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
		if( skate_mod->GetLocalSkater())
		{
			Obj::CAnimationComponent* pAnimComponent = GetAnimationComponentFromObject( skate_mod->GetLocalSkater() );
			pAnimComponent->SetAnimArray( Script::GenerateCRC("animload_human") );
		}
		Mem::Manager::sHandle().PopContext();
		*/
	
	}
	else	
	{
		printf("Clean up without preserve skaters\n");
		//skate_mod->UnloadSkaters();
		// as we're leaving a level,
		// clean up any additional heaps we might have allocated for the other skaters...
		Mem::Manager::sHandle().DeleteSkaterHeaps();
	}


	// Unload any pip pre files that are no longer being used (eg, collision)
	const char *p_pre_name=Pip::GetNextLoadedPre();
	while (p_pre_name)
	{
		const char *p_next=Pip::GetNextLoadedPre(p_pre_name);
		if (!Pip::PreFileIsInUse(p_pre_name))
		{
			Pip::UnloadPre(p_pre_name);
		}
		p_pre_name=p_next;
	}	
		
	// unload the skater parts pre file if it is currently loaded
	File::PreMgr* pre_mgr = File::PreMgr::Instance();
	pre_mgr->UnloadPre( "skaterparts.pre", true );			
	
	// Clear away any old cameras
	Nx::CViewportManager::sDeleteMarkedCameras();

	// Clear any currently running FakeLights commands.
	Nx::CLightManager::sClearCurrentFakeLightsCommand();

	// Clear away any non-permanent heaps
	Nx::FlushParticleTextures(false);
	
	
	// cleanup pathman memory
	Obj::CPathMan* pPathMan = Obj::CPathMan::Instance();
	pPathMan->DeallocateObjectTrackerMemory();

	if( skate_mod->ShouldAllocateNetMiscHeap())
	{
		Dbg_Printf( "****** LEVEL: 0x%x  0x%x 0x%x\n", skate_mod->m_requested_level, skate_mod->m_cur_level, skate_mod->m_prev_level );
		Mem::Manager::sHandle().InitNetMiscHeap();
	}
	else
	{
		Mem::Manager::sHandle().DeleteNetMiscHeap();
	}


	Mem::Manager& mem_man = Mem::Manager::sHandle();
	
	// destroy the custom heaps, if they exist
	// (TODO:  Move this to script, since it really
	// only applies to the skateshop)
	mem_man.DeleteNamedHeap( Script::GenerateCRC("preview"), false );

	#ifdef	__NOPT_ASSERT__
	// sanity checks for fragmentation
	Mem::Heap* heap = mem_man.BottomUpHeap();
	int fragmentation = heap->mFreeMem.m_count;
	
	#if 1	
	if (fragmentation > 10000)
	{
		// not guaranteed to do anything useful.... might crash the debugger (Mick)
		printf ("Dumping Fragments.....\n");
		MemView_DumpFragments(heap);
		printf ("Done Dumping Fragments.....\n");
	}
	#endif
	
	Dbg_MsgAssert(fragmentation < 10000, ("Excessive bottom up fragmentation (%d) after cleanup",fragmentation)); 
	
#ifndef __PLAT_NGC__
	heap = mem_man.TopDownHeap();
	fragmentation = heap->mFreeMem.m_count;
	if (fragmentation > 10000)
	{
		// not guaranteed to do anything useful.... might crash the debugger (Mick)
		printf ("Dumping Fragments.....\n");
		MemView_DumpFragments(heap);
		printf ("Done Dumping Fragments.....\n");
	}
	
	Dbg_MsgAssert(fragmentation < 10000, ("Excessive top down fragmentation (%d) after cleanup",fragmentation)); 
#endif		// __PLAT_NGC__
	#endif

#	if defined( __PLAT_XBOX__ ) && defined( __NOPT_ASSERT__ )
#	define AddStr(a,b)	( pstrOut += wsprintf( pstrOut, a, b ))
#	define KB			( 1024 )
	MEMORYSTATUS stat;
	char strOut[1024], *pstrOut;

    // Get the memory status.
    GlobalMemoryStatus( &stat );

    // Setup the output string.
    pstrOut = strOut;
//	AddStr( "%4d total kb of virtual memory.\n", stat.dwTotalVirtual / KB );
//	AddStr( "%4d  free kb of virtual memory.\n", stat.dwAvailVirtual / KB );
    AddStr( "%4d total kb of physical memory.\n", stat.dwTotalPhys / KB );
    AddStr( "%4d  free kb of physical memory.\n", stat.dwAvailPhys / KB );
//	AddStr( "%4d  percent of memory is in use.\n", stat.dwMemoryLoad );
    OutputDebugString( strOut );
#	endif
	
#	ifdef __PLAT_NGPS__
	if( skate_mod->ShouldAllocateInternetHeap())
	{
		Mem::Manager::sHandle().InitInternetHeap();
	}
	else
	{
		Mem::Manager::sHandle().DeleteInternetHeap();
	}
#	endif // __PLAT_NGPS__

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadQB | Loads the specified QB file.
// @uparm "data\scripts\ken.qb" | File name of the qb to load
// @uparmopt LevelSpecific | Whether this is the level specific script file
bool ScriptLoadQB(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *p_file_name=NULL;
	pParams->GetString(NONAME,&p_file_name);
	Dbg_MsgAssert(p_file_name,("LoadQB requires a file name."));

	if (pParams->ContainsFlag("LevelSpecific"))
	{
		#if 0
		if (s_level_specific_qb)
		{
			// Only one level specific qb can exist currently, so make sure
			// that the old one is removed, just in case someone did two
			// LoadQB's in a row.
			SkateScript::UnloadQB(s_level_specific_qb);
		}	
		s_level_specific_qb=Crc::GenerateCRCFromString(p_file_name);
		#else
		// Now allowing multiple level specific QBs, so 
		// just look for a free slot
		int qb;
		for (qb=0;qb<MAX_LEVEL_QBS;qb++)
		{
			if (!s_level_specific_qbs[qb])
			{
				s_level_specific_qbs[qb] = Crc::GenerateCRCFromString(p_file_name);
				break;
			}
		}
		
		Dbg_MsgAssert(qb < MAX_LEVEL_QBS, ("Can't have more that %d level specifc qbs (loading %s)\n",MAX_LEVEL_QBS,p_file_name)); 
		
				
		#endif
		
		// Load the QB
		SkateScript::LoadQB(p_file_name,ASSERT_IF_DUPLICATE_SYMBOLS);
	}
	else
	{
		// This will not assert if duplicate symbols. This is needed for when a 
		// LoadQB command is put in a button-script as a convenient way to reload
		// a qb. If it asserted for duplicate symbols, it would assert due to the
		// qb already being loaded.
		SkateScript::LoadQB(p_file_name);
	}	
	
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnloadQB | Unloads the specified QB file.
// @uparm "data\scripts\ken.qb" | File name of the qb to unload
bool ScriptUnloadQB(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *p_file_name=NULL;
	pParams->GetString(NONAME,&p_file_name);
	Dbg_MsgAssert(p_file_name,("LoadQB requires a file name."));

	// Unload the QB
	uint32 qb_checksum=Crc::GenerateCRCFromString(p_file_name);
	SkateScript::UnloadQB(qb_checksum);

	// If it was the level specific qb, which is normally removed automatically
	// by ScriptCleanup above.
	// Since they removed it manually, reset s_level_specific_qb so that it is consistent.
	#if 0	
	if (qb_checksum==s_level_specific_qb)
	{
		s_level_specific_qb=0;
	}
	#else
	int qb;
	for (qb=0;qb<MAX_LEVEL_QBS;qb++)
	{
		if (qb_checksum == s_level_specific_qbs[qb])
		{
			s_level_specific_qbs[qb] = 0;
		}
	}
	#endif
		
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleRenderMode | 
bool ScriptToggleRenderMode(Script::CStruct *pParams, Script::CScript *pScript)
{
	Nx::CEngine::sToggleRenderMode();	

	return true;
}


// @script | SetRenderMode | 
bool ScriptSetRenderMode(Script::CStruct *pParams, Script::CScript *pScript)
{

	int mode = 0;
	pParams->GetInteger(CRCD(0x6835b854,"mode"),&mode);
	Nx::CEngine::sSetRenderMode(mode);	

	return true;
}

// @script | SetWireframeMode | 
bool ScriptSetWireframeMode(Script::CStruct *pParams, Script::CScript *pScript)
{
	int mode = 0;
	pParams->GetInteger(CRCD(0x6835b854,"mode"),&mode);
	Nx::CEngine::sSetWireframeMode(mode);	

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DebugRenderIgnore | 
// @parmopt int | ignore_1 | 0 | bitmask of face flags where we ignore this face if bit is 1
// @parmopt int | ignore_0 | 0 | bitmask of face flags where we ignore this face if bit is 0
bool ScriptDebugRenderIgnore(Script::CStruct *pParams, Script::CScript *pScript)
{

	int ignore_0 = 0;	
	int ignore_1 = 0;
	pParams->GetInteger("ignore_1",&ignore_1);
	pParams->GetInteger("ignore_0",&ignore_0);
						  
	Nx::CEngine::sSetDebugIgnore(ignore_1,ignore_0);	

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetVRAMPackContext | Currently, there are 4 possible 
// contexts, 0 through 3, with 0 being the default. There is a script 
// command SetVRAMPackContext which takes a number between 0 and 3. 
// All packing handled by the intelligent packer will then only pack 
// in blocks with a matching context.
// @uparmopt 0 | context number (int)
bool ScriptSetVRAMPackContext(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	int context = 0;
	pParams->GetInteger( NONAME, &context );

#	ifdef __PLAT_NGPS__
//	RpSkyNXSetVRAMPackContext( (uint32)context );
#	endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScreenShot | save a screenshot with specified filename
// @parmopt string | filename | screen | place to save screenshot
bool ScriptScreenShot(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *fileName;
	bool ret;

	ret = pParams->GetText( "filename", &fileName );
	if ( !ret )
		fileName = "screen";

	 Gfx::Manager * gfx_manager =  Gfx::Manager::Instance();
	gfx_manager->ScreenShot( fileName );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// @script | DumpShots | dump memory card screenshots to PC
bool ScriptDumpShots(Script::CStruct *pParams, Script::CScript *pScript)
{
	#ifdef	__PLAT_NGPS__
	Gfx::Manager * gfx_manager =  Gfx::Manager::Instance();
	gfx_manager->DumpMemcardScreeenshots( );
	#endif

	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IfDebugOn | returns true if debug on
bool ScriptIfDebugOn(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Dbg_MsgAssert(pScript,("NULL pScript ?"));
	pScript->SwitchOnIfDebugging();
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IfDebugOff | returns true if debug off
bool ScriptIfDebugOff(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Dbg_MsgAssert(pScript,("NULL pScript ?"));
	pScript->SwitchOffIfDebugging();
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CD | returns true if CD present
bool ScriptCD(Script::CStruct *pParams, Script::CScript *pScript)
{
	return Config::CD();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | NotCD | returns true if CD not present
bool ScriptNotCD(Script::CStruct *pParams, Script::CScript *pScript)
{
	return !Config::CD();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Bootstrap | returns true if this is a bootstrap demo
bool ScriptBootstrap(Script::CStruct *pParams, Script::CScript *pScript)
{
	return Config::Bootstrap();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CasArtist | returns true if cas_artist int is set anywhere
bool ScriptCasArtist(Script::CStruct *pParams, Script::CScript *pScript)
{
	bool is_cas_artist = Script::GetInt( "cas_artist", false );

	return is_cas_artist;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | NotCasArtist | returns true if cas_artist int is not set
bool ScriptNotCasArtist(Script::CStruct *pParams, Script::CScript *pScript)
{
	return !ScriptCasArtist( pParams, pScript );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Gunslinger | returns true if a Gunslinger build
bool ScriptGunslinger(Script::CStruct *pParams, Script::CScript *pScript)
{
#	ifdef TESTING_GUNSLINGER
	return true;
#	else
	return false;
#	endif
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

enum
{
	ACTION_SET_FLAG,
	ACTION_CLEAR_FLAG,
	ACTION_QUERY_FLAG,
	ACTION_SET_OBJECT_FLAG,    // different than the script flags.
	ACTION_CLEAR_OBJECT_FLAG,  // different than the script flags.
	ACTION_FLAG_EXCEPTION_BY_CHECKSUM,
	ACTION_SEE_IF_OBJECT_EXISTS,
};

struct SActionData
{
	int Flag;
	int Action;
	bool ReturnValue;
	bool ObjectFound;
};

struct SActionData_NameChecksum
{
	uint32 ObjectNameChecksum;
	SActionData actionData;
};

struct SActionData_NodeIndex
{
	int NodeIndex;
	SActionData actionData;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void DoAction( Obj::CCompositeObject* pOb, SActionData* pData )
{
	pData->ObjectFound = true;
	switch ( pData->Action )
	{
		case ( ACTION_SET_FLAG ):
			pOb->mScriptFlags |= ( 1 << pData->Flag );
			pData->ReturnValue = true;
			break;
		case ( ACTION_CLEAR_FLAG ):
			pOb->mScriptFlags &= ~( 1 << pData->Flag );
			pData->ReturnValue = true;
			break;
		case ( ACTION_QUERY_FLAG ):
			if ( pOb->mScriptFlags & ( 1 << pData->Flag ) )
			{
				pData->ReturnValue = true;
			}
			break;
		case ( ACTION_SET_OBJECT_FLAG ):
			pOb->SetFlags(((Obj::CObject *) pOb)->GetFlags() | ( pData->Flag ));	 // Note:  Flags are passed as a bitfield
			break;
		case ( ACTION_CLEAR_OBJECT_FLAG ):
			pOb->SetFlags(((Obj::CObject *) pOb)->GetFlags() & ~( pData->Flag ));
			break;
		case ( ACTION_FLAG_EXCEPTION_BY_CHECKSUM ):
			{
				pOb->SelfEvent((pData->Flag));
				pData->ReturnValue = true;
			}
			break;
		case ( ACTION_SEE_IF_OBJECT_EXISTS ):
			break;
		default:
			Dbg_MsgAssert( 0,( "Unknown action %d", pData->Action ));
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void Action_ByNameChecksum( Obj::CObject* pOb, void* pVoidData )
{
	
	Dbg_MsgAssert(pOb,("NULL pOb"));
	Dbg_MsgAssert(pVoidData,("NULL pVoidData"));
	
	SActionData_NameChecksum *pData=(SActionData_NameChecksum*)pVoidData;

	// Mick - find objects based either on their node name (soon to go away)
	// or by their ID (which is set to the node name for composite objects)	
	if ( pOb->GetID() == pData->ObjectNameChecksum )
	{
		DoAction( (Obj::CCompositeObject*)pOb, &pData->actionData );
	}
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void Action_ByNodeIndex(Obj::CObject* pOb, void* pVoidData)
{
	
	Dbg_MsgAssert(pOb,("NULL pOb"));
	Dbg_MsgAssert(pVoidData,("NULL pVoidData"));
	
	SActionData_NodeIndex *pData=(SActionData_NodeIndex*)pVoidData;

	uint32	nodeName = SkateScript::GetNodeNameChecksum(pData->NodeIndex);
	
	if (pOb->GetID() == nodeName )
	{
		DoAction( (Obj::CCompositeObject*)pOb, &pData->actionData );
	}	
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ClearFlag | clears a flag 
// @uparm 1 | the flag to clear
// @parmopt name | name | | the name of the object
// @parmopt string | prefix | | prefix value
bool ScriptClearFlag(Script::CStruct *pParams, Script::CScript *pScript )
{
	
	return ( ScriptDoAction( pParams, pScript, ACTION_CLEAR_FLAG ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SendFlag | Sets a specified flag in a named object. All
// objects have a set of 32 flags. For example: <nl>
// SendFlag 14 name =TRG_Car03 ; no quotes <nl>
// This will search all the objects until it finds the one called
// TRG_Car03, and will then set its flag 14. The flag value must
// be between 0 and 31 inclusive. To aid readability, names should
// be used for the flags, and there is a file flags.q where the
// definitions should be put
// @uparm 1 | the flag to set
// @parmopt name | name | | the name of the object
// @parmopt string | prefix | | prefix value
bool ScriptSendFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	return ( ScriptDoAction( pParams, pScript, ACTION_SET_FLAG ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | QueryFlag | you can query a flag on an object (or several
// objects if using links parameter or prefix = "Something_" parameter
// instead of name = Something_Unique). In the case of one object,
// QueryFlag will return true or false depending on if the flag is set
// or not. In the case of several objects, the function will return true
// if any of the objects specified have that flag set, false otherwise
// @uparm 1 | the flag to query
// @parmopt name | name | | the name of the object
// @parmopt string | prefix | | prefix value
bool ScriptQueryFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	return ( ScriptDoAction( pParams, pScript, ACTION_QUERY_FLAG ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SendException | use the SendException command in the same
// way you would use the SendFlag command:  you can specify name=whatever_object
// or links or prefix="whatever_" (just like with SendFlag). 
// @uparm Exception | the flag 
// @parmopt name | name | | object name
// @parmopt string | prefix | | prefix value
bool ScriptFlagException(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	return ( ScriptDoAction( pParams, pScript, ACTION_FLAG_EXCEPTION_BY_CHECKSUM ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsAlive | Takes the same arguments as SendFlag etc... Returns
// true if the object currently exists
// @parm name | name | object name
bool ScriptCheckIfAlive( Script::CStruct *pParams, Script::CScript *pScript )
{

	return ( DoNodeAction( pParams, pScript, NODE_ACTION_CHECK_IF_ALIVE ) );
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void SetUpActionData( SActionData *pData, int flag, int action )
{
	
	Dbg_MsgAssert( pData,( "Fire matt the idiot" ));
	pData->Flag = flag;
	pData->Action = action;
	pData->ObjectFound = false;
	pData->ReturnValue = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Used to perform actions to nodes by prefix, name checksum, or links -- but this version
// only operates on the nodes that create CMovingObjects... For the general version (applicable
// to nodes that create Environmental Objects, Rails, etc... AS WELL as CMovingObjects, see
// DoNodeAction( )
bool ScriptDoAction(Script::CStruct *pParams, Script::CScript *pScript, int action )
{
	
	int flag = 0;
	if (action == ACTION_FLAG_EXCEPTION_BY_CHECKSUM)
	{
	#ifdef __NOPT_ASSERT__
		bool GotFlag=
	#endif	
		pParams->GetChecksum(NONAME,(uint32*)&flag);
		Dbg_MsgAssert(GotFlag,("\n%s\nSendException  require a chechecksum name to be specified.\n(NOT AN INTEGER)",pScript->GetScriptInfo()));
	}
	else
	{
	#ifdef __NOPT_ASSERT__
		bool GotFlag=
	#endif	
		pParams->GetInteger(NONAME,&flag);
		Dbg_MsgAssert(GotFlag,("\n%s\nFlag commands require a flag to be specified.\n(Either an integer or a name defined to be an integer)",pScript->GetScriptInfo()));
		Dbg_MsgAssert(flag>=0 && flag<32,("\n%s\nBad flag value of %d, value must be between 0 and 31",pScript->GetScriptInfo(),flag));
	
	}
	uint32 nameChecksum;
	const char *pPrefix;
	int i;
	
	
	bool useCurrentLinks = false;
	if ( pParams->ContainsFlag( 0x2e7d5ee7 ) || // checksum 'links'
		( useCurrentLinks = pParams->ContainsFlag( 0xbceb479a ) ) ) // checksum 'currentlinks'
	{
	
		Dbg_MsgAssert(0,("\n%s\n'links' and 'currentlinks' are deprecated\n",pScript->GetScriptInfo()));
	}
	
	if ( pParams->GetChecksum( 0xa1dc81f9, &nameChecksum) )  // 'name'
	{
		SActionData_NameChecksum Data;
		Data.ObjectNameChecksum = nameChecksum;
		SetUpActionData( &Data.actionData, flag, action );
		 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
		skate_mod->GetObjectManager()->ProcessAllObjects(Action_ByNameChecksum,&Data);
		Dbg_MsgAssert( Data.actionData.ObjectFound,( "\n%s\nDidn't find object specified '%s'.", pScript->GetScriptInfo( ), Script::FindChecksumName( nameChecksum ) ) );
		return ( Data.actionData.ReturnValue );
	}
    if ( pParams->GetText( 0x6c4e7971, &pPrefix ) ) // checksum 'prefix'
	{
		// Create with a prefix specified:
		uint16 numNodes = 0;
		const uint16 *pMatchingNodes = SkateScript::GetPrefixedNodes( pPrefix, &numNodes );
		 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
		SActionData_NodeIndex Data;
		SetUpActionData( &Data.actionData, flag, action );
		for ( i = 0; i < numNodes; i++ )
		{
			Data.NodeIndex = pMatchingNodes[ i ];
			skate_mod->GetObjectManager()->ProcessAllObjects(Action_ByNodeIndex,&Data);
		}
		Dbg_MsgAssert( Data.actionData.ObjectFound,( "\n%s\nDidn't find any objects with prefix %s.", pScript->GetScriptInfo( ), pPrefix ));
		return ( Data.actionData.ReturnValue );
	}
#if 0  // put in if ever necessary:
	// set the flag on ourself:
	if ( pScript->mpObject )
	{
		switch ( action )
		{
			case ( ACTION_SET_FLAG ):
				pScript->mpObject->mScriptFlags |= ( 1 << flag );
				return ( true );
			case ( ACTION_CLEAR_FLAG ):
                pScript->mpObject->mScriptFlags &= ~( 1 << flag );
				return ( true );
			case ( ACTION_QUERY_FLAG ):
				return ( pScript->mpObject->mScriptFlags & ( 1 << flag );
			default:
				Dbg_MsgAssert( 0,( "Unknown flag action %d", action ));
				break;
		}
	}
	else
	{
		Dbg_MsgAssert( 0,( "\n%s\nNeed to specify an object, or call this command on a script associated with an object.", pScript->GetScriptInfo( ) ));
	}
#endif
	Dbg_MsgAssert( 0,( "\n%s\nMust specify a name, links, or prefix please.", pScript->GetScriptInfo( ) ));
	return ( false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptSetVisibilityFromNodeIndex( int nodeIndex, bool invisible, int viewport_number )
{	
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

	Script::CArray *pNodeArray = Script::GetArray( CRCD(0xc472ecc5,"NodeArray") );
	Dbg_MsgAssert( pNodeArray,( "No NodeArray found" ));

	Script::CStruct *pNode=pNodeArray->GetStructure( nodeIndex );
	Dbg_MsgAssert( pNode,( "NULL pNode" ));

	// If this is a net game, don't show/hide objects that were meant to be left out for 
	// performance/bandwidth/gameplay reasons
	if ( skate_mod->ShouldBeAbsentNode( pNode ) )
	{
		return true;
	}

	uint32 ClassChecksum = 0;
	pNode->GetChecksum( 0x12b4e660 /*"Class"*/, &ClassChecksum );
	
	switch (ClassChecksum)
	{
		case 0xe47f1b79: // vehicle
		case 0xa0dfac98: // pedestrian
		case 0x61a741e:  // ped
		case 0xef59c100: // gameobject
		case 0x5b8ab877: // skater
		case 0x19b1e241: // ParticleEmitter	  
		case 0x9e7d469e: // ParticleObject  
		{
			SActionData_NodeIndex Data;
			Data.NodeIndex = nodeIndex;
			if ( invisible )
			{
				SetUpActionData( &Data.actionData, Obj::CMovingObject::vINVISIBLE, ACTION_SET_OBJECT_FLAG );
			}
			else
			{
				SetUpActionData( &Data.actionData, Obj::CMovingObject::vINVISIBLE, ACTION_CLEAR_OBJECT_FLAG );
			}
			Data.NodeIndex = nodeIndex;
			skate_mod->GetObjectManager()->ProcessAllObjects(Action_ByNodeIndex,&Data);
			if ( !Data.actionData.ObjectFound )
			{
				Dbg_Message( "Vis/Invis:  Object from node %d not found.", nodeIndex );
			}
			break;
		}
		case 0xb7b3bd86:  // LevelObject
		case 0xbf4d3536:  // LevelGeometry
		{
			uint32 checksumName = 0;
			//Dbg_MsgAssert( ( "No world in viewer module..." ));
			
			if ( pNode->GetChecksum( 0xa1dc81f9, &checksumName ) ) // checksum 'name'
			{
//				Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksumName);
				// We only want to get sectors from the main scene, as that is what the node array applies to															
				Nx::CScene * p_main_scene = Nx::CEngine::sGetMainScene();
				Nx::CSector *p_sector = p_main_scene->GetSector(checksumName);
				Dbg_MsgAssert(p_sector,("sGetSector(0x%x) returned NULL (%s)",checksumName,Script::FindChecksumName(checksumName)));

				uint32 orig_flag = p_sector->GetVisibility();
				uint32 flag = 0x1; 
				if (viewport_number == 1)
				{
					flag = 0x2; 
				}
				if ( invisible )
				{
					p_sector->SetVisibility(orig_flag & ~flag);		// clear flags
					//Bsp::SetWorldSectorFlag(  checksumName, flag );
				}
				else
				{
					p_sector->SetVisibility(orig_flag | flag);		// set flags
					//Bsp::ClearWorldSectorFlag(  checksumName, flag );
				}
			}
			break;
 		}

//		case 0xe594f0a2:  // Trigger (geometry containing triggers...)
		case 0x8e6b02ad:  // railnode
		case CRCC(0x30c19600, "ClimbingNode"):
		default:
			return ( false );
			break;
	}
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptCheckExistenceFromNodeIndex( int nodeIndex )
{	
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

	Script::CArray *pNodeArray = Script::GetArray( CRCD(0xc472ecc5,"NodeArray") );
	Dbg_MsgAssert( pNodeArray,( "No NodeArray found" ));

	Script::CStruct *pNode=pNodeArray->GetStructure( nodeIndex );
	Dbg_MsgAssert( pNode,( "NULL pNode" ));

	uint32 ClassChecksum = 0;
	pNode->GetChecksum( 0x12b4e660 /*"Class"*/, &ClassChecksum );
	
	switch (ClassChecksum)
	{
		case 0xe47f1b79: // vehicle
		case 0xa0dfac98: // pedestrian
		case 0x61a741e:  // ped
		case 0xef59c100: // gameobject
		case 0x5b8ab877: // skater
		case 0xb7b3bd86: // LevelObject
		case 0x19b1e241: // ParticleEmitter
		case 0x9e7d469e: // ParticleObject
		
		{
			SActionData_NodeIndex Data;
			Data.NodeIndex = nodeIndex;
			SetUpActionData( &Data.actionData, 0, ACTION_SEE_IF_OBJECT_EXISTS );
			Data.NodeIndex = nodeIndex;
			skate_mod->GetObjectManager()->ProcessAllObjects(Action_ByNodeIndex,&Data);
			if ( !Data.actionData.ObjectFound )
			{
				return ( false );
			}
			return ( true );
			break;
		}
		case 0xbf4d3536:  // LevelGeometry
		{
			uint32 checksumName = 0;
			//Dbg_MsgAssert( ( "No world in viewer module..." ));
			if ( pNode->GetChecksum( 0xa1dc81f9, &checksumName ) ) // checksum 'name'
			{
//				Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksumName);				
				// We only want to get sectors from the main scene, as that is what the node array applies to															
				Nx::CScene * p_main_scene = Nx::CEngine::sGetMainScene();
				Nx::CSector *p_sector = p_main_scene->GetSector(checksumName);
				Dbg_MsgAssert(p_sector,("sGetSector(0x%x) returned NULL (%s)",checksumName,Script::FindChecksumName(checksumName)));
				return (p_sector->IsActive());
				//return ( Bsp::WorldSectorIsAlive( checksumName ) );
			}
			break;
 		}

		case 0xe594f0a2:  // Trigger (geometry containing triggers...)
			break;
			
		case CRCC(0x30c19600, "ClimbingNode"):
		case 0x8e6b02ad:  // railnode
			return ( skate_mod->GetRailManager()->IsActive( nodeIndex ) );
			break;
			
		default:
			return ( false );
			break;
	}
	return ( true );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptSetColorFromNodeIndex( int nodeIndex, Script::CStruct *pParams )
{	

	uint32 color = 0x808080;
	
	pParams->GetInteger( CRCD(0x99a9b716,"color"), (int*)&color, TRUE );
	
	Image::RGBA rgb;
	rgb.r = (uint8)((color)&0xff);
	rgb.g = (uint8)((color>>8)&0xff);
	rgb.b = (uint8)((color>>16)&0xff);
	rgb.a = 0x80;

	Script::CArray *pNodeArray = Script::GetArray( CRCD(0xc472ecc5,"NodeArray") );
	Dbg_MsgAssert( pNodeArray,( "No NodeArray found" ));

	Script::CStruct *pNode=pNodeArray->GetStructure( nodeIndex );
	Dbg_MsgAssert( pNode,( "NULL pNode" ));

	uint32 ClassChecksum = 0;
	pNode->GetChecksum( 0x12b4e660 /*"Class"*/, &ClassChecksum );
	
	switch (ClassChecksum)
	{
		case 0xbf4d3536:  // LevelGeometry
		{
			uint32 checksumName = 0;
			//Dbg_MsgAssert( ( "No world in viewer module..." ));
			if ( pNode->GetChecksum( 0xa1dc81f9, &checksumName ) ) // checksum 'name'
			{
				Nx::CScene * p_main_scene = Nx::CEngine::sGetMainScene();
				Nx::CSector *p_sector = p_main_scene->GetSector(checksumName);
				Dbg_MsgAssert(p_sector,("sGetSector(0x%x) returned NULL (%s)",checksumName,Script::FindChecksumName(checksumName)));
				p_sector->SetColor(rgb);
			
			}
			break;
 		}
		default:
			return ( false );
			break;
	}
	return ( true );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptShatterFromNodeIndex( int nodeIndex )
{
	
	
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

// Mick:  If pre-running scripts in a net game, we might shatter a lot of glass
// which might run out of memory
// so don't actually shatter the sector if that's what we are doing

	if( skate_mod->LaunchingQueuedScripts())
	{
		return true;
	}

	Script::CArray *pNodeArray = Script::GetArray( CRCD(0xc472ecc5,"NodeArray") );
	Dbg_MsgAssert( pNodeArray,( "No NodeArray found" ));

	Script::CStruct *pNode=pNodeArray->GetStructure( nodeIndex );
	Dbg_MsgAssert( pNode,( "NULL pNode" ));

	// If this is a net game, don't show/hide objects that were meant to be left out for 
	// performance/bandwidth/gameplay reasons
	if ( skate_mod->ShouldBeAbsentNode( pNode ) )
	{
		return true;
	}

	uint32 ClassChecksum = 0;
	pNode->GetChecksum( 0x12b4e660 /*"Class"*/, &ClassChecksum );

	switch( ClassChecksum )
	{
		case 0xb7b3bd86:  // LevelObject
		case 0xbf4d3536:  // LevelGeometry
		{
			//Dbg_MsgAssert( p_world,( "No world in viewer module..." ));

			uint32 checksumName = 0;
			if( pNode->GetChecksum( 0xa1dc81f9, &checksumName )) // checksum 'name'
			{
//				Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksumName);				
				// We only want to get sectors from the main scene, as that is what the node array applies to															
				Nx::CScene * p_main_scene = Nx::CEngine::sGetMainScene();
				Nx::CSector *p_sector = p_main_scene->GetSector(checksumName);
				Dbg_MsgAssert(p_sector,("sGetSector(0x%x) returned NULL (%s)",checksumName,Script::FindChecksumName(checksumName)));
				p_sector->SetShatter(true);
				//Bsp::ShatterWorldSector(  checksumName );
			}
			break;
 		}

		default:
		{
			return false;
			break;
		}
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MakeSkaterGoto |
// This will cause the main skater script to jump to the
// specified script. So a script triggered from a trigger poly
// can use this to force the main skater script to jump to a script.
// It will choose the skater that triggered the poly in that case. <nl>
// <nl>
// A script not associated with an object can also use this, in which
// case it will set the script of skater 0. <nl>
// <nl>
// The main skater script itself could also use it, but there would be
// no point since it can use Goto. <nl>
// <nl>
// This should be used with care, since making the skater jump to a
// script when he's in the middle of doing something could confuse him.
// Eg, forcing him to jump to OnGroundAI when he's on a rail will make
// an assert go off.
// @uparm name | Script name
// @parmopt structure | params | | parameters to be passed to new script
bool ScriptMakeSkaterGoto(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	
	Script::CStruct *pParamsToPass=NULL;
	pParams->GetStructure(CRCD(0x7031f10c,"Params"),&pParamsToPass);
	
	uint32 ScriptChecksum=0;
	pParams->GetChecksum(NONAME,&ScriptChecksum);
	Dbg_MsgAssert(ScriptChecksum,("\n%s\nMakeSkaterGoto requires a script name, eg MakeSkaterGoto Blaa",pScript->GetScriptInfo()));

	Obj::CSkater *pSkater=NULL;
	if (pScript && pScript->mpObject && pScript->mpObject->GetType()==SKATE_TYPE_SKATER)	
	{
		// If the script using this has an object associated with it, and that object
		// is a skater, then use that skater.
		// This will happen in the case of a trigger script using this command.
		pSkater=(Obj::CSkater*)pScript->mpObject.Convert();
	}
	else
	{
		 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
		int Skater=0;
		if( pParams->GetInteger("Skater",&Skater))
		{
			pSkater = skate_mod->GetSkater(Skater);
		}
		else
		{
			pSkater = skate_mod->GetLocalSkater();
		}
	}
		
	if (pSkater && pSkater->IsLocalClient())
	{
		pSkater->JumpToScript(ScriptChecksum,pParamsToPass);
	}
		
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MakeSkaterGotoSub | interrupts whatever script the skater is doing,
// calls new script, and then returns to the original script.
// @uparm name | script to goto
// @parmopt structure | Params | | parameters to pass to new script
// @parmopt int | Skater | | 
bool ScriptMakeSkaterGosub(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Script::CStruct *pParamsToPass=NULL;
	pParams->GetStructure(CRCD(0x7031f10c,"Params"),&pParamsToPass);
	
	uint32 ScriptChecksum=0;
	pParams->GetChecksum(NONAME,&ScriptChecksum);
	Dbg_MsgAssert(ScriptChecksum,("\n%s\nMakeSkaterGosub requires a script name, eg MakeSkaterGosub Blaa",pScript->GetScriptInfo()));

	Obj::CSkater *pSkater=NULL;
	if (pScript && pScript->mpObject && pScript->mpObject->GetType()==SKATE_TYPE_SKATER)	
	{
		// If the script using this has an object associated with it, and that object
		// is a skater, then use that skater.
		// This will happen in the case of a trigger script using this command.
		pSkater=(Obj::CSkater*)pScript->mpObject.Convert();
	}
	else
	{
		 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
		int Skater=0;
		pParams->GetInteger("Skater",&Skater);
		pSkater = skate_mod->GetSkater(Skater);						   
	}
		
	if (pSkater)
	{
		pSkater->CallScript(ScriptChecksum,pParamsToPass);
	}
		
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SpawnScript | This will create & run a new script which will
// run -in parallel- until it finishes, when it will die. The new script
// is not associated with any object. The calling script is not affected
// in any way. A typical use of a spawned script would be to play a sound,
// pause, play another sound, etc, for example when an object dies. This
// way the object itself can die straight away
// @uparm name | the name of the script to spawn
// @parmopt structure | Params | | Parameter structure to pass to new script
// @parmopt name | Id | | an id to assign to the spawned script, so it can 
// be killed by KillSpawnedScript
// @flag NotSessionSpecific | This will cause the script to not get deleted when the current
// level (session) ends.
bool ScriptSpawnScript(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 ScriptChecksum=0;
	pParams->GetChecksum(NONAME,&ScriptChecksum);
	Dbg_MsgAssert(ScriptChecksum,("\n%s\nMissing script name in SpawnScript command.",pScript->GetScriptInfo()));
	bool net_enabled, permanent;
	Script::CStruct *pScriptParams=NULL;
	pParams->GetStructure(CRCD(0x7031f10c,"Params"),&pScriptParams);
	
	// The spawned script can optionally be given an Id, so that it can be deleted
	// by KillSpawnedScript.
	uint32 Id=0;
	pParams->GetChecksum("Id",&Id);

	uint32 CallbackScript=0;
	pParams->GetChecksum("Callback",&CallbackScript);
	Script::CStruct *pCallbackParams=NULL;
	pParams->GetStructure("CallbackParams",&pCallbackParams);
		
	//net_enabled = 0;
	//permanent = 0;
	//pParams->GetInteger( "NetEnabled", &net_enabled );
	//pParams->GetInteger( "Permanent", &permanent );
	net_enabled = pParams->ContainsFlag( "NetEnabled" );
	permanent = pParams->ContainsFlag( "Permanent" );
	
	int not_session_specific=0;
	pParams->GetInteger( "NotSessionSpecific", &not_session_specific);
	
	// copy the parent's node
	#ifdef __NOPT_ASSERT__
	Script::CScript *p_script=Script::SpawnScript(ScriptChecksum,pScriptParams,CallbackScript,pCallbackParams,
													pScript->mNode,
													Id,
													net_enabled,
													permanent,
													not_session_specific); 	
	p_script->SetCommentString("Spawned by script command SpawnScript");
	p_script->SetOriginatingScriptInfo(pScript->GetCurrentLineNumber(),pScript->mScriptChecksum);
	#else
	Script::SpawnScript(ScriptChecksum,pScriptParams,CallbackScript,pCallbackParams,
						pScript->mNode,
						Id,
						net_enabled,
						permanent,
						not_session_specific); 	
	#endif	
	return true;
}	


			
// @script | SpawnSound | This will create & run a new script which will
// run -in parallel- until it finishes, when it will die. The new script
// is not associated with any object. The calling script is not affected
// in any way. A typical use of a spawned script would be to play a sound,
// pause, play another sound, etc, for example when an object dies. This
// way the object itself can die straight away
//
// Note, this is the same as SpawnScript, except if the current script is
// attached to an object, then we do the same as Obj_SpawnScript
// So we get attached to the object
//
// @uparm name | the name of the script to spawn
// @parmopt structure | Params | | Parameter structure to pass to new script
// @parmopt name | Id | | an id to assign to the spawned script, so it can 
// be killed by KillSpawnedScript
bool ScriptSpawnSound(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	if (pScript->mpObject)
	{
		uint32 scriptChecksum;
		if ( pParams->GetChecksum( NONAME, &scriptChecksum ) )
		{
			// The spawned script can optionally be given an Id, so that it can be deleted
			// by KillSpawnedScript.
			uint32 Id=0;
			// keep the same ID as the parent if not specified...
			Id = Script::FindSpawnedScriptID(pScript);
			pParams->GetChecksum("Id",&Id);
			Script::CScriptStructure *pScriptParams = NULL;
			pParams->GetStructure( "Params", &pScriptParams );
			#ifdef __NOPT_ASSERT__	
			Script::CScript *p_script=pScript->mpObject->SpawnScriptPlease( scriptChecksum, pScriptParams, Id );
			p_script->SetCommentString("Created by SpawnSound");
			p_script->SetOriginatingScriptInfo(pScript->GetCurrentLineNumber(),pScript->mScriptChecksum);
			#else
			pScript->mpObject->SpawnScriptPlease( scriptChecksum, pScriptParams, Id );
			#endif
		}
	}
	else
	{
		return ScriptSpawnScript(pParams,pScript);		
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SpawnSkaterScript | This will create & run a new script
// on the skater which will run in parallel until it finishes, when it
// will die. The calling script is not affected in any way. 
// @uparm name | the name of the script to call
// @parmopt structure | Params | | parameter list to pass to new script
// @parmopt name | Id | | an id to assign to the new script so it can 
// be killed by KillSpawnedScript
bool ScriptSpawnSkaterScript(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	// Mostly the same as the code for ScriptSpawnScript
	uint32 ScriptChecksum=0;
	pParams->GetChecksum(NONAME,&ScriptChecksum);
	Dbg_MsgAssert(ScriptChecksum,("\n%s\nMissing script name in SpawnSkaterScript command.",pScript->GetScriptInfo()));
	
	Script::CStruct *pScriptParams=NULL;
	pParams->GetStructure(CRCD(0x7031f10c,"Params"),&pScriptParams);
	
	// The spawned script can optionally be given an Id, so that it can be deleted
	// by KillSpawnedScript.
	uint32 Id=0;
	pParams->GetChecksum("Id",&Id);

	uint32 CallbackScript=0;
	pParams->GetChecksum("Callback",&CallbackScript);
	Script::CStruct *pCallbackParams=NULL;
	pParams->GetStructure("CallbackParams",&pCallbackParams);
	
	Script::CScript *pNewScript=Script::SpawnScript(ScriptChecksum,pScriptParams,CallbackScript,pCallbackParams,pScript->mNode,Id); 	// copy the parent's node
	#ifdef __NOPT_ASSERT__
	pNewScript->SetCommentString("Spawned by script command SpawnSkaterScript");
	pNewScript->SetOriginatingScriptInfo(pScript->GetCurrentLineNumber(),pScript->mScriptChecksum);
	#endif	
	
	// Now set the object pointer to be the skater.
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	Obj::CSkater *pSkater = skate_mod->GetLocalSkater();						   
	
	pNewScript->mpObject=pSkater;
	
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | KillSpawnedScript | can be passed a script name or an id
// @parmopt name | Name | | the name of the script
// @parmopt name | Id | | the id of the script
bool ScriptKillSpawnedScript(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	uint32 ScriptChecksum=0;
	if (pParams->GetChecksum("Name",&ScriptChecksum))
	{
		// Got a script name, so kill all spawned scripts that ran that script.
		Script::KillSpawnedScriptsThatReferTo(ScriptChecksum);
		return true;
	}

	uint32 Id=0;										   
	if (pParams->GetChecksum("Id",&Id))
	{
		// They specified an Id, so kill all spawned scripts with this Id.
		Script::KillSpawnedScriptsWithId(Id);
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PauseSkaters | This will cause all skaters to freeze.
// They will continue where they left off when UnPauseSkaters is called. 
// Note that this command will not pause any spawned scripts associated
// with the skaters, because otherwise it will pause the camera animation
// script too, which is where the command will probably be used most
bool ScriptPauseSkaters( Script::CStruct *pParams, Script::CScript *pScript )
{
	Replay::WritePauseSkater();
	
	bool hide = pParams->ContainsFlag("hide");	
	
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	uint32 NumSkaters=skate_mod->GetNumSkaters();
	for (uint32 i=0; i<NumSkaters; ++i)
	{
		Obj::CSkater *pSkater = skate_mod->GetSkater(i);
		if (pSkater && pSkater->IsLocalClient()) // Hmm, assert instead?
		{
			pSkater->Pause();
			pSkater->Hide(hide);
		}	
	}	
	
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnPauseSkaters | opposite effect of PauseSkaters
bool ScriptUnPauseSkaters( Script::CStruct *pParams, Script::CScript *pScript )
{
	Replay::WriteUnPauseSkater();
	
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	uint32 NumSkaters=skate_mod->GetNumSkaters();
	for (uint32 i=0; i<NumSkaters; ++i)
	{
		Obj::CSkater *pSkater = skate_mod->GetSkater(i);
		if (pSkater)
		{
			pSkater->UnPause();
			pSkater->Hide(false);
		}	
	}	
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PauseSkater | this pauses an individual skater
// @uparm 0 | skater id
bool ScriptPauseSkater( Script::CStruct *pParams, Script::CScript *pScript )
{
	bool hide = pParams->ContainsFlag("hide");	

	int skater_id;
	pParams->GetInteger( NONAME, &skater_id, true );
	
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	Obj::CSkater *pSkater = skate_mod->GetSkater(skater_id);
	if (pSkater)
	{
		pSkater->Pause();
		pSkater->Hide(hide);
	}	

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnPauseSkater | Unpauses an individual skater
// @uparm 0 | skater id
bool ScriptUnPauseSkater( Script::CStruct *pParams, Script::CScript *pScript )
{
	

	int skater_id;
	pParams->GetInteger( NONAME, &skater_id, true );

	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	Obj::CSkater *pSkater = skate_mod->GetSkater(skater_id);
	if (pSkater)
	{
		pSkater->UnPause();
		pSkater->Hide(false);
	}	

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PauseGame | Similar to "PauseSkaters", it will pause
// the skaters, and everything else in the world, yet still allow
// you to launch messages and spawn new scripts
bool ScriptPauseGame( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Mdl::FrontEnd * front =  Mdl::FrontEnd::Instance();
	front->PauseGame(true);
	// if the script was a spawned script
	// then we want to unpause it, so we can keep running after the game has paused
	Script::UnpauseSpawnedScript(pScript);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnPauseGame | 
bool ScriptUnPauseGame( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Mdl::FrontEnd * front =  Mdl::FrontEnd::Instance();
	front->PauseGame(false);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsGamePaused | 
bool ScriptIsGamePaused( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Mdl::FrontEnd * front =  Mdl::FrontEnd::Instance();
	if( front->GamePaused() )
    {
        return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PauseObjects |
bool ScriptPauseObjects(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Pause all composite objects
	Obj::CCompositeObjectManager::Instance()->Pause(true);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnpauseObjects |
bool ScriptUnPauseObjects(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Unpause all composite objects
	Obj::CCompositeObjectManager::Instance()->Pause(false);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PauseSpawnedScripts |
bool ScriptPauseSpawnedScripts(Script::CStruct *pParams, Script::CScript *pScript)
{
	Script::PauseSpawnedScripts(true);		

	if ( pScript->mIsSpawned )
	{
		Dbg_MsgAssert( 0, ( "Can't pause spawned scripts from a spawned script" ) );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnPauseSpawnedScripts |
bool ScriptUnPauseSpawnedScripts(Script::CStruct *pParams, Script::CScript *pScript)
{
	Script::PauseSpawnedScripts(false);		

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetClock | 
bool ScriptResetClock( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	int time_limit;

	// Clients instead get their time limit from the server
	if( gamenet_man->OnServer())
	{
		time_limit = skate_mod->GetGameMode()->GetTimeLimit();
		skate_mod->SetTimeLimit( time_limit );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PadsPluggedIn | Returns true if at least one pad is plugged in.
bool ScriptPadsPluggedIn( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Mdl::FrontEnd * p_front =  Mdl::FrontEnd::Instance();
	 return p_front->PadsPluggedIn();
}	 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DoFlash | Doflash just draws a big poly over the viewport
// and then modulates the color and alpha of that poly over time. There
// is no support for Blendmodes yet. If you are doing a player specific 
// flash remember to call it from a skater script so that it works in 2
// player splitscreen
// @parmopt float | duration | 0.0 | 
// @parmopt string | texture | | texture name
// @parmopt float | z | 0.0 | z
// @flag fullscreen |
// @flag behind_panel |
// @flag additive |
// @flag subtractive |
// @parmopt int | skater | | for specifying skater when not in skater script
// @flag ignore_pause |
// @parm int | start_r | start red index
// @parm int | start_g | start green index
// @parm int | start_b | start blue index
// @parm int | start_a | start alpha value
// @parm int | end_r | end red index
// @parm int | end_g | end green index
// @parm int | end_b | end blue index
// @parm int | end_a | end alpha value
bool ScriptDoFlash( Script::CStruct *pParams, Script::CScript *pScript )
{
	

	float		duration		= 0.0f;
	float		z				= 0.0f;
	uint32		flags			= 0;
	bool		fullscreen		= false;
	int			skater_num		= -1;
	const char*	p_texture_name	= NULL;

	pParams->GetFloat( "duration", &duration );
	
	if( duration > 0.0f )
	{
		pParams->GetText( "texture", &p_texture_name );

		pParams->GetFloat( "z", &z );

//		if( pParams->ContainsFlag( 0xdf6436d2 /*"fullscreen"*/ ))
//		{
//			flags |= Flash::FLAG_FULLSCREEN;
//			fullscreen = true;
//		}	
		if( pParams->ContainsFlag( 0x70ffc649 /*"behind_panel"*/ ))
		{
			flags |= Nx::SCREEN_FLASH_FLAG_BEHIND_PANEL;
		}	
		if( pParams->ContainsFlag( 0x19c43cf6 /*"additive"*/ ))
		{
			flags |= Nx::SCREEN_FLASH_FLAG_ADDITIVE;
		}	
		else if( pParams->ContainsFlag( 0x387c9ed6 /*"subtractive"*/ ))
		{
			flags |= Nx::SCREEN_FLASH_FLAG_SUBTRACTIVE;
		}	

		if( !fullscreen )
		{
			pParams->GetInteger( "skater", &skater_num );
			if( skater_num == -1 )
			{
				// No skater parameter provided, so assume running from a skater script.
				Obj::CSkater* p_skater = static_cast <Obj::CSkater*>( pScript->mpObject.Convert() );
				Dbg_Assert( p_skater );
				skater_num = p_skater->GetSkaterNumber();
			}
		}

		if( pParams->ContainsFlag( 0xdf6436d2 /*"ignore_pause"*/ ))
		{
			flags |= Nx::SCREEN_FLASH_FLAG_IGNORE_PAUSE;
		}	

		int val = 0;
		Image::RGBA from, to;

		pParams->GetInteger( "start_r",	&val );
		from.r = val;
		pParams->GetInteger( "start_g",	&val );
		from.g = val;
		pParams->GetInteger( "start_b",	&val );
		from.b = val;
		pParams->GetInteger( "start_a",	&val );
		from.a = val;

		pParams->GetInteger( "end_r",	&val );
		to.r = val;
		pParams->GetInteger( "end_g",	&val );
		to.g = val;
		pParams->GetInteger( "end_b",	&val );
		to.b = val;
		pParams->GetInteger( "end_a",	&val );
		to.a = val;

		if( fullscreen )
		{
		}
		else
		{
			Nx::AddScreenFlash( skater_num, from, to, duration, z, flags, p_texture_name );

			// Get the panel for player.
			//HUD::PanelMgr* panel_mgr = HUD::PanelMgr::Instance();
			//HUD::Panel* p_panel = panel_mgr->GetPanelBySkaterId( skater_num );
			//p_panel->AddFlash( from, to, duration, z, flags, p_texture_name );
		}
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StartServer | Starts a server using the IP stored in
// the script variable LocalIP, which you also have to define. You
// can also define a script variable called ServerName to name your
// server (i.e. ServerName = “MyServer” with a max of 15 characters
bool ScriptStartServer(Script::CStruct *pParams, Script::CScript *pScript )
{
	
	
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	// set the appropriate memory context
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());

	skate_mod->StartServer();

	if (gamenet_man->InInternetMode())
	{
		// defer loading the level/starting the game
		// until we have connected to the matchmaker
		//gamenet_man->RequestMatchmakerConnect();
		
		// SG: 9-17-02. Post the game AFTER we're in our first level
		//gamenet_man->PostGame();
	}
	else if (gamenet_man->InLanMode())
	{
		// if we're in LAN mode, start the game immediately
	}
		
	// pop the memory context
	Mem::Manager::sHandle().PopContext();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LeaveServer | 
bool ScriptLeaveServer(Script::CStruct *pParams, Script::CScript *pScript )
{   
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

	skate_mod->LeaveServer();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | FindServers | broadcast out to the local subnet for servers
bool ScriptFindServers(Script::CStruct *pParams, Script::CScript *pScript )
{
	// Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	
	//skate_mod->FindServers();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | JoinServer | Join the server in session at the given IP address and port
// @uparm "string" | ip address
// @uparm 1 | port
bool ScriptJoinServer(Script::CStruct *pParams, Script::CScript *pScript )
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
    
	if( gamenet_man->OnServer())
	{
		uint32 i;

		// If playing split screen, create N clients (one for each viewport
		// otherwise, just create one client
//		gamenet_man->SpawnClient( false, true, 0 );

		uint32 numPlayers = skate_mod->GetGameMode()->GetInitialNumberOfPlayers();

		for( i = 0; i < numPlayers; i++ )
		{   
			Net::Client* client;

			client = gamenet_man->SpawnClient( false, true, true, i );
			
			//skate_mod->JoinServer( false, 0, 0, true, i );
			gamenet_man->JoinServer( false, 0, 0, i );
			skate_mod->AddNetworkMsgHandlers( client, i );
			if( gamenet_man->InNetGame())
			{
				Script::RunScript( "entered_network_game" );
			}
		}
	}
	else
	{
		/*int max_players;
		pParams->GetInteger( Script::GenerateCRC( "MaxPlayers" ), (int *) &max_players );
		skate_mod->GetGameMode()->SetMaximumNumberOfPlayers( max_players );

		Dbg_Printf( "**** SETTING MAX PLAYERS TO %d\n", max_players );
		Dbg_Assert( skate_mod->GetGameMode()->GetMaximumNumberOfPlayers() == (uint32) max_players );*/

#ifdef __PLAT_XBOX__
		IN_ADDR host_addr;
		int port = 0;
		bool observe_only;

		pParams->GetInteger( Script::GenerateCRC( "Address" ), (int *) &host_addr.s_addr );
		pParams->GetInteger( Script::GenerateCRC( "Port" ), &port );
		observe_only = ( gamenet_man->GetJoinMode() == GameNet::vJOIN_MODE_OBSERVE );
	
		if( port != 0 )
		{
			gamenet_man->SpawnClient( false, false, true, 0 );
			gamenet_man->JoinServer( observe_only, (unsigned long) host_addr.s_addr, port, 0 );
		}
#else
		const char *server_ip = NULL;
		int port = 0;
		bool observe_only;
		
		pParams->GetText( NONAME, &server_ip );
		pParams->GetInteger( NONAME, &port );
		observe_only = ( gamenet_man->GetJoinMode() == GameNet::vJOIN_MODE_OBSERVE );
	
		if( server_ip && ( port != 0 ))
		{
			gamenet_man->SpawnClient( false, false, true, 0 );
			gamenet_man->JoinServer( observe_only, inet_addr( server_ip ), port, 0 );
		}
#endif
	}
    
	Mem::Manager::sHandle().PopContext();
    	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetNetworkMode | sets the network mode
// @uparm 0 | mode
bool ScriptSetNetworkMode( Script::CStruct *pParams, Script::CScript *pScript )
{
	int mode = 0;
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	mode = 0;
	pParams->GetInteger( NONAME, &mode );
	if( mode == 0 )
	{
		gamenet_man->SetNetworkMode( GameNet::vNO_NET_MODE );
	}
	else if( mode == 1 )
	{
		gamenet_man->SetNetworkMode( GameNet::vLAN_MODE );
	}
	else if( mode == 2 )
	{
		gamenet_man->SetNetworkMode( GameNet::vINTERNET_MODE );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MemViewToggle | 
bool ScriptMemViewToggle( Script::CStruct *pParams, Script::CScript *pScript )
{
	MemViewToggle();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ProfileTasks | profiles task for the number of frames specified
// @uparm 1 | number of frames
bool ScriptProfileTasks( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Mlp::Manager * mlp_manager =  Mlp::Manager::Instance();
	int Val=1;
	pParams->GetInteger(NONAME,&Val);	
	printf ("\n\nProfiling task for %d frames:\n",Val);
	mlp_manager->ProfileTasks(Val);
	
	return true;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UseNetworkPreferences | 
bool ScriptUseNetworkPreferences( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->UsePreferences();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptTestNetSetup( Script::CStruct *pParams, Script::CScript *pScript )
{
	Net::Manager * net_man =  Net::Manager::Instance();
	bool properly_setup;
	 
	properly_setup = net_man->NetworkEnvironmentSetup();
	if( !properly_setup )
	{
		if( net_man->GetLastError() == Net::vRES_ERROR_DEVICE_NOT_HOT )
		{
			pParams->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_error_not_connected" ));
			Script::RunScript( "create_net_startup_error_dialog", pParams );
		}
		else if( net_man->GetLastError() == Net::vRES_ERROR_DEVICE_NOT_CONNECTED )
		{
			pParams->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_error_device_error" ));
			Script::RunScript( "create_net_startup_error_dialog", pParams );
		}
		else if( net_man->GetLastError() == Net::vRES_ERROR_UNKNOWN_DEVICE )
		{
			pParams->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_error_not_detected" ));
			Script::RunScript( "create_net_startup_error_dialog", pParams );
		}
		else if( net_man->GetLastError() == Net::vRES_ERROR_DHCP )
		{
			pParams->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_error_dhcp_error" ));
			Script::RunScript( "create_net_startup_error_dialog", pParams );
		}
		else if( net_man->GetLastError() == Net::vRES_ERROR_DEVICE_CHANGED )
		{
			pParams->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_error_changed_device" ));
			Script::RunScript( "create_net_startup_error_dialog", pParams );
		}
		else
		{
			pParams->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_error_general_error" ));
			Script::RunScript( "create_net_startup_error_dialog", pParams );
		}
	}
	else
	{   
		uint32 success_script;

		pParams->GetChecksum( "success_script", &success_script, true );
		Dbg_Printf( "***** SUCCESS! RUNNING SCRIPT %p\n", success_script );
		Script::RunScript( success_script, pParams );
		
		/*Script::CStruct* pParams;

		Script::RunScript( "dialog_box_exit" );
		//Script::RunScript( "launch_network_select_menu" );
		
        pParams = new Script::CStruct;

		pParams->AddChecksum( "change_gamemode", Script::GenerateCRC( "change_gamemode_net" ));
		Script::RunScript( "launch_select_skater_menu", pParams );
		
		delete pParams;*/
	}

	return properly_setup;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptNeedToTestNetSetup( Script::CStruct *pParams, Script::CScript *pScript )
{
	Net::Manager * net_man =  Net::Manager::Instance();
	
	return net_man->NeedToTestNetworkEnvironment();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptCanChangeDevices( Script::CStruct *pParams, Script::CScript *pScript )
{
	Net::Manager * net_man =  Net::Manager::Instance();

	return net_man->CanChangeDevices();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ConnectToInternet | 
bool ScriptConnectToInternet( Script::CStruct *pParams, Script::CScript *pScript )
{
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	uint32 success, failure;

	success = 0;
	failure = 0;

	pParams->GetChecksum( CRCD(0x90ff204d,"success"), &success );
	pParams->GetChecksum( CRCD(0xde64fc3e,"failure"), &failure );

	return gamenet_man->ConnectToInternet( success, failure );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptCancelConnectToInternet( Script::CStruct* pParams, Script::CScript* pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->CancelConnectToInternet();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptCancelLogon( Script::CStruct *pParams, Script::CScript *pScript )
{
#ifdef __PLAT_XBOX__
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	gamenet_man->mpAuthMan->CancelLogon();
	return true;
#else
	return true;
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptIsOnline( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Net::Manager * net_man =  Net::Manager::Instance();

	return net_man->IsOnline();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DisconnectFromInternet | 
bool ScriptDisconnectFromInternet( Script::CStruct *pParams, Script::CScript *pScript )
{
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	uint32 callback_script = 0;

	pParams->GetChecksum( CRCD(0x86068bd9,"callback"), &callback_script );
	gamenet_man->DisconnectFromInternet( callback_script );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StopAllScripts | This will cause all scripts in existence
// to stop (including the one calling this function). This function may
// be useful later when doing cleanup type stuff. Note that though this
// command will stop all scripts in existence at the moment it is called,
// it won't prevent new scripts being run by the C-code
bool ScriptStopAllScripts(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Script::StopAllScripts();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetMenuElementText | sets the specified menu element
// @uparmopt "Blaa" | text to use for menu element - "Blaa" is the actual default
bool ScriptSetMenuElementText(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	/*
	uint32 Id=0;
	pParams->GetChecksum("Id",&Id);
	Dbg_MsgAssert(Id,("SetMenuElementText requires an Id"));
	
	const char *pText="Blaa";
	pParams->GetText(NONAME,&pText);
	
	Front::MenuEvent event;
	event.SetTypeAndTarget(Front::MenuEvent::vSETCONTENTS,Id);
	Script::CStruct *pData = event.GetData();		
	pData->AddComponent(Script::GenerateCRC("string"), ESYMBOLTYPE_STRING, pText);
	 Front::MenuFactory * pMenuFactory =  Front::MenuFactory::Instance();
	pMenuFactory->LaunchEvent(&event);
	*/
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static bool FirstTimeThisIsCalled_Flag=true;
// @script | FirstTimeThisIsCalled | returns true the first time this is called
bool ScriptFirstTimeThisIsCalled(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	if (FirstTimeThisIsCalled_Flag)
	{
		FirstTimeThisIsCalled_Flag=false;
		return true;
	}
	return false;		
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EnableActuators | 
// @uparm 1 | 1 for enable - 0 for disable.  1 is the default
bool	ScriptEnableActuators(Script::CStruct *pParams, Script::CScript *pScript)
{	
	int on = 1;	
	pParams->GetInteger(NONAME,&on);
	Inp::Manager * input_man =  Inp::Manager::Instance();
	Mdl::Skate * pSkate =  Mdl::Skate::Instance();
	for (int i=0; i< SIO::vMAX_DEVICES; ++i)
	{
		if ( pSkate->mp_controller_preferences[i].VibrationOn )
		{
			if (on)
			{
				input_man->EnableActuator(i);
			}
			else
			{
				input_man->DisableActuator(i);
			}
		}
	}

	return true;
}
					  

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | InNetGame | returns true if we're in a net game
bool ScriptInNetGame(Script::CStruct *pParams, Script::CScript *pScript)
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	 
	return gamenet_man->InNetGame();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsObserving | You can now use the "IsObserving" script
// command to perform logic (ex. hide objects) if the player is
// Observing a network game.
bool ScriptIsObserving( Script::CStruct *pParams, Script::CScript *pScript )
{
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	GameNet::PlayerInfo* player;
	GameNet::NewPlayerInfo* new_player;
	Lst::Search< GameNet::NewPlayerInfo > sh;

	if(( player = gamenet_man->GetLocalPlayer()))
	{
		return player->IsObserving();
	}
		
	// If we're observing, we need to remove our skater
	for( new_player = gamenet_man->FirstNewPlayerInfo( sh ); new_player; new_player = gamenet_man->NextNewPlayerInfo( sh ))
	{
		if( new_player->Flags & GameNet::PlayerInfo::mLOCAL_PLAYER )
		{
			if( new_player->Flags & GameNet::PlayerInfo::mOBSERVER )
			{
				return true;
			}
		}
	}

	return( gamenet_man->GetJoinMode() == GameNet::vJOIN_MODE_OBSERVE );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SkatersAreReady | true if all skaters connected to the server
// are done loading
bool ScriptSkatersAreReady(Script::CStruct *pParams, Script::CScript *pScript)
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	
	// poll the server to find out whether the skaters are done loading
	if( gamenet_man->ReadyToPlay() && 
		( ( skate_mod->GetNumSkaters() > 0 ) ||
		  ( ScriptIsObserving( NULL, NULL ))))
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSlomo | sets slomo to specified value
// @uparmopt 1.0 | value 
bool ScriptSetSlomo(Script::CStruct *pParams, Script::CScript *pScript)
{
	float slomo = 1.0f;
	pParams->GetFloat(NONAME,&slomo);
	Tmr::SetSlomo(slomo);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetArenaSize | Obsolete function
bool ScriptSetArenaSize(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_Message( "SetArenaSize is obsolete" );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetParticleSysVisibility | no longer supported.
bool ScriptSetParticleSysVisibility( Script::CStruct *pParams, Script::CScript *pScript )
{
	printf ("STUBBED: ScriptSetParticleSysVisibility\n");
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | TogglePlayerNames | 
bool ScriptTogglePlayerNames( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->TogglePlayerNames();
	if( !gamenet_man->ShouldDrawPlayerNames())
	{
		Script::RunScript( "destroy_all_player_names" );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/*************************                          *****************************************/

// @script | SetCurrentGameType | sets the current game type
bool ScriptSetCurrentGameType( Script::CStruct *pParams, Script::CScript *pScript )
{

	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	skate_mod->SetCurrentGameType();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DumpNetMessageStats | displays all net message stats
bool ScriptDumpNetMessageStats( Script::CStruct *pParams, Script::CScript *pScript )
{
	Net::Conn* conn;
	Lst::Search< Net::Conn > sh;
	Net::App* app;
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
#ifdef __NOPT_ASSERT__
	 Net::Manager * net_man =  Net::Manager::Instance();
#endif		// __NOPT_ASSERT__
	int i, j, total_size, total_num, size, num;
	Net::Metrics* metrics;

	app = gamenet_man->GetServer();
	if( app == NULL )
	{
		app = gamenet_man->GetClient( 0 );
	}

	if( app == NULL )
	{
		return true;
	}

	i = 0;
    for( conn = app->FirstConnection( &sh ); conn; conn = app->NextConnection( &sh ))
	{
		metrics = conn->GetInboundMetrics();
		Dbg_Printf( "Conn %d inbound stats:\n", i );
		total_size = 0;
		total_num = 0;
		for( j = 0; j < 256; j++ )
		{
			total_size += metrics->GetTotalMessageData( j );
			total_num += metrics->GetTotalNumMessagesOfId( j );
		}

		// Guard against dbz
		if( total_size == 0 )
		{
			total_size = 1;
		}

		for( j = 0; j < 256; j++ )
		{
			size = metrics->GetTotalMessageData( j );
			num = metrics->GetTotalNumMessagesOfId( j );

			if( num > 0 )
			{
#				ifdef __NOPT_ASSERT__
				Dbg_Printf( "[%d-%s] Num: %d Size: %d Pct: %02f\n", j, net_man->GetMessageName( j ),
						num, size, (float) size/(float) total_size );
#				endif // __NOPT_ASSERT__
			}
		}

		metrics = conn->GetOutboundMetrics();
		Dbg_Printf( "Conn %d outbound stats:\n", i );
		total_size = 0;
		total_num = 0;
		for( j = 0; j < 256; j++ )
		{
			total_size += metrics->GetTotalMessageData( j );
			total_num += metrics->GetTotalNumMessagesOfId( j );
		}

		// Guard against dbz
		if( total_size == 0 )
		{
			total_size = 1;
		}

		for( j = 0; j < 256; j++ )
		{
			size = metrics->GetTotalMessageData( j );
			num = metrics->GetTotalNumMessagesOfId( j );

			if( num > 0 )
			{
#				ifdef __NOPT_ASSERT__
				Dbg_Printf( "[%d-%s] Num: %d Size: %d Pct: %02f\n", j, net_man->GetMessageName( j ),
						num, size, (float) size/(float) total_size );
#				endif // __NOPT_ASSERT__
			}
		}
		i++;
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetServerMode | sets the server mode
// @flag off | 
bool  ScriptSetServerMode(Script::CStruct *pParams, Script::CScript *pScript)
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	bool off = pParams->ContainsFlag("off");
    
	gamenet_man->SetServerMode( !off );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | NotifyBailDone | 
bool ScriptNotifyBailDone( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	Net::Client* client;	

	Obj::CSkater *pSkater=NULL;
	if( pScript && pScript->mpObject && pScript->mpObject->GetType() == SKATE_TYPE_SKATER )	
	{
		// If the script using this has an object associated with it, and that object
		// is a skater, then use that skater.
		pSkater= (Obj::CSkater *) pScript->mpObject.Convert();
	}

	if( pSkater && pSkater->IsLocalClient())
	{
		client = gamenet_man->GetClient( pSkater->GetSkaterNumber());
		if( client )
		{
			Net::MsgDesc msg_desc;

			msg_desc.m_Id = GameNet::MSG_ID_BAIL_DONE;
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
			client->EnqueueMessageToServer( &msg_desc );
		}
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DisplayLoadingScreen | shows the loading screen 
// @uparm "string" | name of the screen to display
// @uparmopt time | the amount of time to display loading bar (default is 0, meaning no loading bar)
// @flag Freeze | just freeze current screen
// @flag Blank | clear screen to black
bool ScriptDisplayLoadingScreen( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	const char	*pScreen = "images\\xxxxx";	
	pParams->GetText(NONAME,&pScreen);
	
	Nx::CLoadScreen::sDisplay( (char*) pScreen, pParams->ContainsFlag(CRCD(0xb96e0be5,"Freeze")),pParams->ContainsFlag(CRCD(0xc3d43b9a,"blank")));

	float duration;
	if (pParams->GetFloat(NONAME,&duration))
	{
		Nx::CLoadScreen::sStartLoadingBar(duration);
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | HideLoadingScreen | hides the loading screen 
bool ScriptHideLoadingScreen( Script::CStruct *pParams, Script::CScript *pScript )
{
	//LoadScreen::Hide();
	Nx::CLoadScreen::sHide();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EnterObserverMode | changes to observer mode (for net game)
bool ScriptEnterObserverMode(Script::CStruct *pParams, Script::CScript *pScript)
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->RequestObserverMode();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ObserveNextSkater | 
bool ScriptObserveNextSkater( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

	skate_mod->ObserveNextSkater();

    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AllowPause | 
// @flag off | do not allow pause
bool ScriptAllowPause(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	bool on = true;
	if (pParams->ContainsFlag("off"))
	{
		on = false;
	}		

	printf ("WARNING: ScriptAllowPause is STUBBED\n");	
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RefreshServerList | gets a new server list 
bool ScriptRefreshServerList(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->RefreshServerList( pParams->ContainsFlag( "force_refresh" ));
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StartServerList | 
bool ScriptStartServerList(Script::CStruct *pParams, Script::CScript *pScript)
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->StartServerList();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptStartLobbyList( Script::CStruct* pParams, Script::CScript* pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	if(	gamenet_man->GetServerListState() == GameNet::vSERVER_LIST_STATE_SHUTDOWN )
	{
		gamenet_man->SetNextServerListState( GameNet::vSERVER_LIST_STATE_STARTING_LOBBY_LIST );
		gamenet_man->SetServerListState( GameNet::vSERVER_LIST_STATE_INITIALIZE );
	}
	else
	{
		gamenet_man->SetServerListState( GameNet::vSERVER_LIST_STATE_STARTING_LOBBY_LIST );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StopServerList | 
bool ScriptStopServerList(Script::CStruct *pParams, Script::CScript *pScript)
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->StopServerList();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | FreeServerList | 
bool ScriptFreeServerList(Script::CStruct *pParams, Script::CScript *pScript)
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->FreeServerList();
	
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PauseGameFlow | suspends gameflow - should only be called from
// gameflow.q
bool ScriptPauseGameFlow(Script::CStruct *pParams, Script::CScript *pScript)
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	skate_mod->PauseGameFlow( true );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnPauseGameFlow | 
bool ScriptUnpauseGameFlow(Script::CStruct *pParams, Script::CScript *pScript)
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	skate_mod->PauseGameFlow( false );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | InFrontEnd | returns true if we are in the front end
bool ScriptInFrontEnd(Script::CStruct *pParams, Script::CScript *pScript)
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	return skate_mod->GetGameMode()->IsFrontEnd();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | InSplitScreenGame | returns true if we are in a split screen game
bool ScriptInSplitScreenGame(Script::CStruct *pParams, Script::CScript *pScript)
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	return skate_mod->IsMultiplayerGame() && !gamenet_man->InNetGame() && !skate_mod->GetGameMode()->IsFrontEnd();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | InMultiPlayerGame | 
bool ScriptInMultiplayerGame(Script::CStruct *pParams, Script::CScript *pScript)
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	
	return skate_mod->IsMultiplayerGame();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetFireballLevel(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	int level;
	 
	Script::CStruct* params = pScript->GetParams();

 	level = skate_mod->GetGameMode()->GetFireballLevel();
	params->AddInteger( CRCD(0x651533ec,"level"), level );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GameModeEquals | returns true if the current game mode matches
// the specified game mode
// @uparm name | game mode to check. More than one may be specified, eg GameModeEquals is_singlesession is_creategoals
bool ScriptGameModeEquals(Script::CStruct *pParams, Script::CScript *pScript)
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	 
 	CComponent *pComp=pParams->GetNextComponent();
	while (pComp)
	{
		if (pComp->mNameChecksum==0 && pComp->mType==ESYMBOLTYPE_NAME)
		{
			if (skate_mod->GetGameMode()->IsTrue(pComp->mChecksum))
			{
				return true;
			}
		}
		pComp=pParams->GetNextComponent(pComp);
	}	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | OnServer | returns true if we are on a server
bool ScriptOnServer(Script::CStruct *pParams, Script::CScript *pScript)
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	return gamenet_man->OnServer();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetCurrentLevel | returns level=load_foun  (or whatever)
// so you can use it with LoadLevel, without going through LoadRequestedLevel
bool ScriptGetCurrentLevel(Script::CStruct *pParams, Script::CScript *pScript)
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	pScript->GetParams()->AddComponent( Script::GenerateCRC("level"), ESYMBOLTYPE_NAME, (int)skate_mod->m_requested_level );
	
	Game::CGoalManager* p_goal_manager = Game::GetGoalManager();
	Dbg_MsgAssert(p_goal_manager,("NULL p_goal_manager ?"));
	pScript->GetParams()->AddChecksum(CRCD(0x16a0b364,"level_structure"),p_goal_manager->GetLevelStructureName());
	
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RestartLevel | RestartLevel is just intended to be called
// when a level is restarted it does not actually restart the level,
// it just sets a few flags and does something with disabling the the
// viewer log that's only related to screenshot mode 
bool ScriptRestartLevel(Script::CStruct *pParams, Script::CScript *pScript)
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	skate_mod->RestartLevel();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleScores | toggles game scores
bool ScriptToggleScores(Script::CStruct *pParams, Script::CScript *pScript)
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->ToggleScores();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | XTriggered | 
bool ScriptXTriggered(Script::CStruct *pParams, Script::CScript *pScript)
{
    Dbg_MsgAssert( 0, ( "Obsolete function" ) );
    return false;

//	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
//	return skate_mod->XTriggered();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UsePad | sets the specified skater to use the pad
// @uparmopt 0 | skater id (default is 0)
bool ScriptUsePad(Script::CStruct *pParams, Script::CScript *pScript)
{
    Dbg_MsgAssert( 0, ( "Obsolete function" ) );
    return false;
	
/*
    int skaterId = 0;
	pParams->GetInteger( NONAME, &skaterId );

	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	skate_mod->UsePad( skaterId );
	return true;
*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsTrue | Checks to see whether some global constant equals
// 1 or 0.  (If it's not defined anywhere, then it returns 0.) <nl>
// Usage: <nl>
// if IsTrue run_viewer <nl>
//    printf "I am in viewer mode" <nl>
// else <nl>
//    printf "I am in skateshop mode" <nl>
// endif <nl>
// if IsTrue test_balls <nl>
// where test_balls is defined in "yourname".q. <nl>
//    printf "I am testing my balls here" <nl>
// endif 
// @uparm name | global constant to check
bool ScriptIsTrue(Script::CStruct *pParams, Script::CScript *pScript)
{
	int Integer=0;
	pParams->GetInteger( NONAME, &Integer, false );
	
	return Integer;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GameFlow | 
// @uparm name | script name
bool ScriptGameFlow(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 ScriptChecksum=0;
	pParams->GetChecksum(NONAME,&ScriptChecksum);
	Dbg_MsgAssert(ScriptChecksum,("\n%s\nGameFlow requires a script name, eg Gameflow Blaa",pScript->GetScriptInfo()));

	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	Dbg_AssertPtr( skate_mod->mp_gameFlow );
	skate_mod->mp_gameFlow->Reset( ScriptChecksum );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptLastBroadcastedCheatWas(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 cheat_flag;

	pParams->GetChecksum(CRCD(0xae94c183,"cheat_flag"), &cheat_flag, true );

	return( s_last_broadcast_cheat == cheat_flag );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptClearCheats(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	Obj::CSkaterCareer* career;

	career = skate_mod->GetCareer();
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x13d5b5db,"CHEAT_ON_1")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x8adce461,"CHEAT_ON_2")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0xfddbd4f7,"CHEAT_ON_3")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x63bf4154,"CHEAT_ON_4")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x14b871c2,"CHEAT_ON_5")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x8db12078,"CHEAT_ON_6")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0xfab610ee,"CHEAT_ON_7")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x6a090d7f,"CHEAT_ON_8")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x1d0e3de9,"CHEAT_ON_9")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x37cbee45,"CHEAT_ON_10")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x40ccded3,"CHEAT_ON_11")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0xd9c58f69,"CHEAT_ON_12")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0xaec2bfff,"CHEAT_ON_13")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x30a62a5c,"CHEAT_ON_14")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x47a11aca,"CHEAT_ON_15")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0xdea84b70,"CHEAT_ON_16")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0xa9af7be6,"CHEAT_ON_17")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x39106677,"CHEAT_ON_18")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x4e1756e1,"CHEAT_ON_19")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x1ce6bd86,"CHEAT_ON_20")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x6be18d10,"CHEAT_ON_21")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0xf2e8dcaa,"CHEAT_ON_22")));
	career->UnSetGlobalFlag( Script::GetInteger(CRCD(0x85efec3c,"CHEAT_ON_23")));

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptBroadcastCheat(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	Lst::Search< Net::Conn > sh;
	Net::Conn* conn;
	Net::Server* server;
	uint32 cheat_flag;
	char on;

	pParams->GetChecksum(CRCD(0xae94c183,"cheat_flag"), &cheat_flag, true );
	Dbg_Assert( gamenet_man->OnServer());
	Dbg_Assert( gamenet_man->InNetGame());
	if( pParams->ContainsComponentNamed(CRCD(0xf649d637,"on")))
	{
		on = 1;
	}
	else
	{
		on = 0;
	}

	// Intentionally sprinkled in this function so that hackers have a more-difficult time to 
	// nullify it
	s_last_broadcast_cheat = cheat_flag;
	server = gamenet_man->GetServer();
	for( conn = server->FirstConnection( &sh ); conn; conn = server->NextConnection( &sh ))
	{
		if( conn->IsRemote())
		{
			Net::MsgDesc msg_desc;
			GameNet::MsgToggleCheat msg;
	
			msg.m_Cheat = cheat_flag;
			msg.m_On = on;
	
			msg_desc.m_Id = GameNet::MSG_ID_TOGGLE_CHEAT;
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_Data = &msg;
			msg_desc.m_Length = sizeof( GameNet::MsgToggleCheat );
			msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
		
			server->EnqueueMessage( conn->GetHandle(), &msg_desc );
			s_last_broadcast_cheat = cheat_flag;
		}
	}

	s_last_broadcast_cheat = cheat_flag;
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptCheatAllowed(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 cheat_flag;

	pParams->GetChecksum(CRCD(0xae94c183,"cheat_flag"), &cheat_flag, true );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | JoinWithPassword | joins the server with the specified password
// @parm string | string | password string
bool ScriptJoinWithPassword(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
    
	const char* p_string;
	pParams->GetText( "string", &p_string, true );

    gamenet_man->ReattemptJoinWithPassword((char*) p_string );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SendChatMessage | 
// @parm string | string | the message to send
bool ScriptSendChatMessage(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	const char* p_string = NULL;
	pParams->GetText( "string", &p_string );
	if( p_string == NULL )
	{
		return false;
	}

#if ( ENGLISH == 0 )
	if ( ( p_string != NULL ) && ( strlen( p_string ) > 0 ) && ( stricmp( p_string, Script::GetLocalString( "kc_str_empty" ) ) ) )
#else
	if ( ( p_string != NULL ) && ( strlen( p_string ) > 0 ) && ( stricmp( p_string, "--EMPTY--" ) ) )
#endif
	{
		 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
		gamenet_man->SendChatMessage( (char*) p_string );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | InSlapGame | true if we are in a slap or netslap game
bool ScriptInSlapGame( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

	if( ( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netslap" )) ||
		(skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "slap" )))
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetScreenMode | Sets the way the screen will be split up into separate
// viewports in the game. If there is more than one viewport, and multiple skaters
// are active, then each skater will get his own viewport
// @uparm One_Camera | the mode to use - one of the following: <nl>
// One_Camera <nl>
// Split_Vertical <nl>
// Split_Horizontal <nl>
// Split_Quarters
bool ScriptSetScreenMode(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

	Nx::ScriptSetScreenMode( pParams, pScript );

	Obj::CSkater* pSkater;
	Mdl::Score* pScore;
	for( uint i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
	{
		pSkater = skate_mod->GetSkater(i);						   
		if ( pSkater )			// Skater might not exist
		{
			pScore = pSkater->GetScoreObject();
			pScore->RepositionMeters();
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetScreenModeFromGameMode | sets the appropriate screen mode based on the game mode
bool ScriptSetScreenModeFromGameMode( Script::CStruct *pParams, Script::CScript *pScript )
{           
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	Nx::ScreenMode requested_mode, current_mode;

	Obj::CSkater* pSkater;
	Mdl::Score* pScore;
	for( uint i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
	{
		pSkater = skate_mod->GetSkater(i);						   
		if ( pSkater && pSkater->IsLocalClient())			// Skater might not exist
		{
			printf ("cfuncs %d: SUTUBBBEDDDDDDDDDDDDDDDDDDDDDDDD\n",__LINE__);
			
			/*							
			Gfx::Camera *p_camera = pSkater->GetSkaterCam();
			if (p_camera)
			{
				Nx::CViewportManager::sSetCameraAllScreenModes(pSkater->GetHeapIndex(), p_camera);
			}
			*/
		}
	}
	
	// set up the appropriate screen mode
	requested_mode = skate_mod->GetGameMode()->GetScreenMode();
	current_mode = Nx::CViewportManager::sGetScreenMode();
	if(! ((	( requested_mode == Nx::vSPLIT_V ) &&
			( current_mode == Nx::vSPLIT_H )) ||
			(( requested_mode == Nx::vSPLIT_H ) &&
			( current_mode == Nx::vSPLIT_V ))))
	{
		Nx::CViewportManager::sSetScreenMode( requested_mode );
	}

	for( uint i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
	{
		pSkater = skate_mod->GetSkater(i);						   
		if ( pSkater && pSkater->IsLocalClient())			// Skater might not exist
		{
			pScore = pSkater->GetScoreObject();
			pScore->RepositionMeters();
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadPendingPlayers | 
bool ScriptLoadPendingPlayers( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->LoadPendingPlayers();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LaunchQueuedScripts | 
bool ScriptLaunchQueuedScripts( Script::CStruct *pParams, Script::CScript *pScript )
{
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->LaunchQueuedScripts();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
// extend the current parameters to include:  string = "WOW"

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetInitialsString | gets the initials string for the player
bool ScriptGetInitialsString( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mdl::Skate * pSkate =  Mdl::Skate::Instance();
	Records::CGameRecords *pGameRecords=pSkate->GetGameRecords();
	
	pScript->GetParams()->AddComponent( Script::GenerateCRC( "string" ), ESYMBOLTYPE_STRING, pGameRecords->GetDefaultInitials()->Get() );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetInitialsString | sets the initials string for the player
// @parm string | string | initials string
bool ScriptSetInitialsString( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	 Mdl::Skate * pSkate =  Mdl::Skate::Instance();
	Records::CGameRecords *pGameRecords=pSkate->GetGameRecords();	
	const char *pInitials = "ERR";
	pParams->GetText("string",&pInitials);
	pGameRecords->SetDefaultInitials(pInitials);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AttachToSkater | (DEPRECATED) Make the current script be running on the skater <nl>
// used for the gameflow, where we are not always sure if there is a skater there <nl>
// use this in place of SpawnSkaterScript in the gameflow <nl>
// if "AttachToSkater End" is called then will detatch it <nl>
// must be in mathcing pairs <nl>
// otherwise the gameflow gets messed up <nl>
// when the skater is deleted (as the gameflow gets stopped and deleted) <nl>
// @flag End | 
bool	ScriptAttachToSkater(  Script::CStruct *pParams, Script::CScript *pScript )
{

	Dbg_MsgAssert(0,("ScriptAttachToSkater should not be called, as it is old and dangerous\n"));	
	
	if (pParams->ContainsFlag("End"))
	{
		pScript->mpObject = NULL; 
	}
	else
	{
		 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
		Obj::CSkater *pSkater = skate_mod->GetLocalSkater();
		Dbg_MsgAssert(pSkater,("AttachToSkater called before skater exists"));
		pScript->mpObject = pSkater; 
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | TryCheatString | 
// @parm string | string | cheat string to try
bool	ScriptTryCheatString(  Script::CStruct *pParams, Script::CScript *pScript )
{
	
	char  b[128];
	const char *p;
	pParams->GetText("string",&p,true);
	if (strlen(p) < 6)
	{
		Script::RunScript("cheat_playsound_bad");
		return false;
	}
	
	sprintf(b,"12345678901234567890"); 		// just in case.....
	sprintf(b,p);
	
	// (Mick)									  
	// We want to generate a 64 bit CRC
	// so we take 3 checksums, starting from 3 different places
	// and add the first one to the second two
	// and use those two numbers as a 64 bit checksum
	// The idea here is to create a number, from which is is difficult to
	// go back to the original string
	// (to do this, you need a fairlry long string, otherwise, you could just try them all
	// now we do 1 second worth of PS2 precessing the generate the key,
	// so even on a computer 1000 times as fast, it would take ~30 years to try all six letter keys
	// So, barring a flaw in my algorithm, should be pretty safe even with short keys 
	
	uint32 check0 = Script::GenerateCRC(p);	  // get checksum
	
	uint32 check1 = 0;	
	for (int i = 0;i< 100000;i++)			  // a hundred thousand calls, to mess with Ken
	{
		sprintf (b,"%d",i+check0);			  // print a different number at the start
		b[strlen(b)]='X';					  // link it to the rest of the string
		char *p1 = b;
		while (*p1)
		{
			check1 += 1023 * (*p1++);				  // and just add up the bytes, oh, how sweet...
		}
		check0 = Script::GenerateCRC(b);	  // oh, I slay myself...
	}
	uint32 check2 = Script::GenerateCRC(&p[strlen(p)/3]);	
	uint32 check3 = Script::GenerateCRC(b);	   // use the final string, so no correlation with check2
	
//	printf ("%x,%x,%x\n",check1,check2,check3);	 // three pretty dang random looking numbers

	check2 ^= check1;							 // munched into two
	check3 ^= check1;							 // (hopefully not a fatal mistake)
	
//	printf ("%x,%x,%x\n",check1,check2,check3);
	// Print it out, so we can cut and paste
	printf ("\n{c1=%d c2=%d CheatScript=cheat_xxx },  ; %s\n\n",check2, check3,p); 
	// then search through the cheat array to see if we have a match


	#ifdef	__PLAT_XBOX__				 	
	Script::CArray *pCheatArray = Script::GetArray( "Cheat_Array_Xbox" );
	#endif
	#ifdef	__PLAT_NGC__				 	
	Script::CArray *pCheatArray = Script::GetArray( "Cheat_Array_Gamecube" );
	#endif
	#ifdef	__PLAT_NGPS__				 	
	Script::CArray *pCheatArray = Script::GetArray( "Cheat_Array_PS2" );
	#endif
	
	Dbg_MsgAssert( pCheatArray,( "No Cheat_Array found" ));
	int cheats = pCheatArray->GetSize();
	bool cheated = false;
	for (int cheat = 0;cheat<cheats;cheat++)
	{
		Script::CStruct *pStruct = pCheatArray->GetStructure(cheat);
		Dbg_MsgAssert( pStruct,( "Cheat Array messed up" ));
		uint32 c1,c2;
		uint32 cheat_script;
		pStruct->GetInteger("c1",(int*)&c1,true);
		pStruct->GetInteger("c2",(int*)&c2,true);
		pStruct->GetChecksum("cheatscript",&cheat_script,true);
		if (c1 == check2 && c2 == check3)
		{
			Script::RunScript("cheat_playsound_good");
			Script::RunScript(cheat_script);
			cheated = true;
		}
	}
	
	if (!cheated)
	{
			Script::RunScript("cheat_playsound_bad");
	}


								
	return false;								
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LevelIs | true if the level matches the specified level
// @uparm name | level name
bool ScriptLevelIs(  Script::CStruct *pParams, Script::CScript *pScript )
{
	

	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	
	// GJ:  this may not work in park editor mode
	uint32 level_name;
	pParams->GetChecksum( NONAME, &level_name, true );
	
	return ( skate_mod->m_cur_level == level_name );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StartNetworkLobby | 
bool ScriptStartNetworkLobby( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	gamenet_man->StartNetworkLobby();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ObserversAllowed | 
bool ScriptObserversAllowed( Script::CStruct *pParams, Script::CScript *pScript )
{
	 Net::Manager * net_man =  Net::Manager::Instance();
	
	// Modem games cannot host observers
	return( net_man->GetConnectionType() != Net::vCONN_TYPE_MODEM );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | NumPlayersAllowed | 
// @uparm name | number of players allowed
bool ScriptNumPlayersAllowed( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	
	uint32 checksum = 0;
	 Net::Manager * net_man =  Net::Manager::Instance();

	// Non-Modem games have no max player restriction
	if( net_man->GetConnectionType() != Net::vCONN_TYPE_MODEM )
	{
		return true;
	}
    
	pParams->GetChecksum( NONAME, &checksum );
	if( checksum == Script::GenerateCRC( "num_4" ))
	{
		return false;
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AutoDNS | true if auto DNS
bool ScriptAutoDNS( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	Script::CStruct* pStructure;
	Prefs::Preferences* pPreferences;
	uint32 auto_dns_checksum;
	 
	pPreferences = gamenet_man->GetNetworkPreferences();

	pStructure = pPreferences->GetPreference( Script::GenerateCRC("auto_dns") );
	pStructure->GetChecksum( "Checksum", &auto_dns_checksum, true );
	return ( auto_dns_checksum == Script::GenerateCRC( "boolean_true" ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UsingDefaultMasterServers | true if using the default master
// servers
bool ScriptUsingDefaultMasterServers( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	Script::CStruct* pStructure;
	Prefs::Preferences* pPreferences;
	uint32 default_servers;
	 
	pPreferences = gamenet_man->GetNetworkPreferences();

	pStructure = pPreferences->GetPreference( Script::GenerateCRC("use_default_master_servers") );
	pStructure->GetChecksum( "Checksum", &default_servers, true );
	return ( default_servers == Script::GenerateCRC( "boolean_true" ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UsingDHCP | true if using DHCP
bool ScriptUsingDHCP( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	Script::CStruct* pStructure;
	Prefs::Preferences* pPreferences;
	uint32 ip_assignment;
	 
	pPreferences = gamenet_man->GetNetworkPreferences();

	pStructure = pPreferences->GetPreference( Script::GenerateCRC("broadband_type") );
	pStructure->GetChecksum( "Checksum", &ip_assignment, true );
	return ( ip_assignment == Script::GenerateCRC( "ip_dhcp" ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | InInternetMode | 
bool ScriptInInternetMode( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	return gamenet_man->InInternetMode();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EnteringNetGame | true if we're in a network game but not 
// read to play
bool ScriptEnteringNetGame( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

	 return(( gamenet_man->InNetGame()) &&
			( skate_mod->m_prev_level == Script::GenerateCRC( "Load_Skateshop" )));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DeviceChosen | true if a device type has been chosen
bool ScriptDeviceChosen( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	uint32 checksum;
	Prefs::Preferences* pPreferences;
	Script::CStruct* pStructure;

	pPreferences = gamenet_man->GetNetworkPreferences();
	Dbg_Assert( pPreferences );

	pStructure = pPreferences->GetPreference( Script::GenerateCRC("device_type") );
	pStructure->GetChecksum( "checksum", &checksum, true );

	return ( checksum != Script::GenerateCRC("device_none"));
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GameIsOver | true if game is over
bool ScriptGameIsOver( Script::CStruct *pParams, Script::CScript *pScript )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	
	return gamenet_man->GameIsOver();
}

static char 	s_level_name[64];

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetLevelName | gets the level name into the above static, so it can be used for the DumpHeaps memory profile dump 
bool ScriptSetLevelName( Script::CStruct *pParams, Script::CScript *pScript )
{
	const char* pName;
	pParams->GetText( NONAME, &pName, true );
	sprintf(s_level_name,pName);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetPS2 | For testing the reset button handler when
// the Sony Network adapter is loaded
bool ScriptResetPS2( Script::CStruct *pParams, Script::CScript *pScript )
{

#	ifdef __PLAT_NGPS__
	int stat;

	Dbg_Printf( "Resetting PS2\n" );
	
	while( sceDevctl("dev9x:", DDIOC_OFF, NULL, 0, NULL, 0 ) < 0 );
	// PS2 power off
	while( !sceCdPowerOff( &stat ) || stat );
#	endif // __PLAT_NGPS__

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetHD | For testing the reset button handler when
// the Sony Network adapter is loaded
bool ScriptResetHD( Script::CStruct *pParams, Script::CScript *pScript )
{
	Dbg_Printf( "Resetting PS2\n" );

#	ifdef __PLAT_NGPS__
	int stat;
	sceDevctl("pfs:", PDIOC_CLOSEALL, NULL, 0, NULL, 0);
	while (sceDevctl("hdd:", HDIOC_DEV9OFF, NULL, 0, NULL, 0) < 0);
	while (sceDevctl("dev9x:", DDIOC_OFF, NULL, 0, NULL, 0) < 0);
    while (!sceCdPowerOff(&stat) || stat);
#	endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PAL | returns true if we're in PAL mode
bool ScriptPAL( Script::CStruct *pParams, Script::CScript *pScript )
{
	
#ifdef	PAL
	return true;
#else
	return false;
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Change | changes the symbol to the new value specified.  will do nothing
// if called on the following: a symbol that doesn't already exist, a script symbol, c-function
// or member function symbols <nl>
// should really only be called on strings, floats, and ints <nl>
// example: Change Foo=7
// @uparm name | the name of the symbol to change
// @uparm value | the value to assign to the symbol (make sure type is correct)
// @flag Resolve | If specified, resolve the right hand side if it is a name of a global, so as to lose
// the reference to that global. Ie, Change a=b Resolve where a=6 and b=3.141 will change a to be 3.141,
// and it will remain so even if b is then changed to something else.
bool ScriptChangeSymbolValue(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Extract the first component from pParams, which is used to define the name of the
	// symbol to change and the new value to give to it.
	CComponent *pComp=pParams->GetNextComponent();
	Dbg_MsgAssert(pComp,("\n%s\nChange function requires a symbol name and new value, eg Change Foo=7",pScript->GetScriptInfo()));
	Dbg_MsgAssert(pComp->mNameChecksum,("\n%s\nChange function requires a symbol name and new value, eg Change Foo=7",pScript->GetScriptInfo()));
	if (!pComp)
	{
		return false;
	}
		
	// Do a few checks on the old symbol.
	CSymbolTableEntry *pOld=Script::LookUpSymbol(pComp->mNameChecksum);
	
	// Require that the old symbol must exist, otherwise continuing will create the symbol and it will never get deleted,
	// causing fragmentation & memory leaks etc.
	if (!pOld)
	{
		Dbg_MsgAssert(0,("\n%s\nTried to change the value of the non-existent symbol '%s'",pScript->GetScriptInfo(),FindChecksumName(pComp->mNameChecksum)));
		return false;
	}	
	// Don't allow script symbols to be changed. If this were allowed then all existing CScripts that were running that
	// script would have to get halted to avoid their PC's becoming invalid, which would require some extra code
	// and seems kind of a silly thing to want to do anyway.
	if (pOld->mType==ESYMBOLTYPE_QSCRIPT)
	{
		Dbg_MsgAssert(0,("\n%s\nTried to change the value of the script '%s'",pScript->GetScriptInfo(),FindChecksumName(pComp->mNameChecksum)));
		return false;
	}	
	// Don't allow c-function or member function symbols to be changed, cos once changed they'll never be able to be put back.
	if (pOld->mType==ESYMBOLTYPE_CFUNCTION)
	{
		Dbg_MsgAssert(0,("\n%s\nTried to change the value of the c-function '%s'",pScript->GetScriptInfo(),FindChecksumName(pComp->mNameChecksum)));
		return false;
	}	
	if (pOld->mType==ESYMBOLTYPE_MEMBERFUNCTION)
	{
		Dbg_MsgAssert(0,("\n%s\nTried to change the value of the member function '%s'",pScript->GetScriptInfo(),FindChecksumName(pComp->mNameChecksum)));
		return false;
	}	
	
	// Hmmm, in theory other types could be changed, but they shouldn't be cos it'll cause fragmentation.
	switch (pOld->mType)
	{
		case ESYMBOLTYPE_INTEGER:
        case ESYMBOLTYPE_FLOAT:
        case ESYMBOLTYPE_NAME:
			break;
		default:
			Dbg_MsgAssert(0,("\n%s\nCannot change the value of '%s' because it has type '%s', and reallocating it will cause memory fragmentation. So there.",pScript->GetScriptInfo(),FindChecksumName(pComp->mNameChecksum),GetTypeName(pOld->mType)));
			break;
	}	
	
	// Remove the old symbol.
	Script::CleanUpAndRemoveSymbol(pOld);

	// Create it afresh.
	CSymbolTableEntry *pNew=Script::CreateNewSymbolEntry(pComp->mNameChecksum); 

	CComponent temp;
	temp.mType=pComp->mType;
	temp.mUnion=pComp->mUnion;

	// If they are changing a value to some name, then support the option of resolving that name,
	// in case it might be a global.
	// For example, suppose a=6, and b=3.141
	// 'Change a=b' will change a from 6 to the checksum b. This means it will preserve the reference to b,
	// so in the C code if a GetFloat("a") is done it will return 3.141, but if b is changed to 28.3, 
	// GetFloat("a") will now return 28.3, because it preserves the reference to b.
	// However, 'Change a=b Resolve' will change a to be the float 3.141. Even if b is now changed, a has
	// lost the reference to b and will stay as 3.141.
	if (temp.mType==ESYMBOLTYPE_NAME)
	{
		if (pParams->ContainsFlag(CRCD(0x991a7bc3,"Resolve")))
		{
			CSymbolTableEntry *p_global=Script::Resolve(temp.mChecksum);
			if (p_global)
			{
				temp.mType=p_global->mType;
				temp.mUnion=p_global->mUnion;
			}
		}
	}
	
	// Copy the new value into the symbol just created.
	switch (temp.mType)
	{
		case ESYMBOLTYPE_INTEGER:
            pNew->mType=ESYMBOLTYPE_INTEGER;
            pNew->mIntegerValue=temp.mIntegerValue;
            break;

        case ESYMBOLTYPE_FLOAT:
            pNew->mType=ESYMBOLTYPE_FLOAT;
            pNew->mFloatValue=temp.mFloatValue;
            break;

        case ESYMBOLTYPE_NAME:
            pNew->mType=ESYMBOLTYPE_NAME;
            pNew->mChecksum=temp.mChecksum;
            break;
			
        default:
            Dbg_MsgAssert(0,("\n%s\nChange function does not support component type '%s'",pScript->GetScriptInfo(),GetTypeName(temp.mType)));
            break;
	}

	// This is to prevent an assert in the CComponent destructor
	temp.mUnion=0;
	
	return true;									   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DumpScripts | Prints out the names of all the currently existing scripts.
bool ScriptDumpScripts( Script::CStruct *pParams, Script::CScript *pScript )
{
	bool just_stack = pParams->ContainsFlag("just_stack");
	
	if (!just_stack)
		Script::DumpScripts();
	
	// XXX
	#ifdef	__NOPT_ASSERT__
	printf("Script stack is: %s\n", pScript->GetScriptInfo());
	#endif
	
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | TimeUp | true if time is up
bool ScriptTimeUp( Script::CStruct *pParams, Script::CScript *pScript )
{
     Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

    return skate_mod->GetGameMode()->ShouldUseClock() && ( skate_mod->GetGameMode()->GetTimeLeft() <= 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LaunchViewer | 
bool ScriptLaunchViewer( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::Manager * mdl_manager =  Mdl::Manager::Instance();
	Mdl::CViewer* viewer_mod;
	static bool initialized = false;
	
	if( initialized )
	{
		return true;
	}

	viewer_mod = new Mdl::CViewer;

	mdl_manager->RegisterModule ( *viewer_mod );
	mdl_manager->StartModule( *viewer_mod );
	initialized = true;
	
	
	// Add the parameters for the skater camera component		
	// so it can get me as the target
	Script::CStruct * p_cam_params = new Script::CStruct;
	p_cam_params->AddChecksum("name",CRCD(0xeb17151b,"viewer_cam"));
	
	// Also create the camera components here.
	Obj::CCompositeObject *p_cam_object = Obj::CCompositeObjectManager::Instance()->CreateCompositeObjectFromNode(
										Script::GetArray("viewercam_composite_structure"),p_cam_params);
	
	delete	p_cam_params;
	p_cam_object->SetLockOn();		// Lock it so it does not get deleted between levels


	return true;
}

// @script | LaunchViewer | 
bool ScriptLaunchScriptDebugger( Script::CStruct* pParams, Script::CScript* pScript )
{
	#ifdef __NOPT_ASSERT__
	static bool initialized = false;
	
	if( initialized )
	{
		return true;
	}

	Mdl::Manager::Instance()->StartModule( *Dbg::CScriptDebugger::Instance() );
	
	initialized = true;
	#endif
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetViewerModel | 
bool ScriptSetViewerModel( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();

	s_view_mode = (int)Obj::VIEWER_SKATER_PAUSED;
	
	// sync the viewer up to cfuncs.cpp's version of s_view_mode
	pViewer->sSetViewMode( s_view_mode );
	// Handle toggling to different cameras based on view mode
	
	switch (s_view_mode )
	{
		case Obj::GAMEPLAY_SKATER_ACTIVE:
			SetActiveCamera(CRCD(0x967c138c,"skatercam0"),0, false);			
			break;
		default:
			SetActiveCamera(CRCD(0xeb17151b,"viewer_cam"),0, true);						
			break;		
	}


	Obj::CSkater* pSkater;
	for ( int i = 0; i < 8; i++ )
	{
		 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
		pSkater = skate_mod->GetSkater(i);						   
		if ( pSkater )			// Skater might not exist
		{
			pSkater->SetViewMode( (Obj::EViewMode)s_view_mode );
		}
	}
	
	if ( pViewer )
	{
		pViewer->ResetCameraToViewerObject();
		
		Script::PrintContents(pParams);

		pViewer->AddViewerObject(pParams);

		/*
		Obj::CViewerObject* pViewerObject = pViewer->GetViewerObject();
		if ( pViewerObject )
		{
			pViewerObject->UnloadModel();
			
			pViewerObject->LoadModel( pParams );
		}
		*/
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetViewerAnim | 
bool ScriptSetViewerAnim( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();

	if ( pViewer )
	{
		uint32 animName;
		pParams->GetChecksum( NONAME, &animName, true );

		Obj::CViewerObject* pViewerObject = pViewer->GetViewerObject();
		if ( pViewerObject )
		{
			pViewerObject->SetAnim( animName );
		}
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptSetViewerLODDist | 
bool ScriptSetViewerLODDist( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();

	if ( pViewer )
	{
		Script::CPair thePair;
		if ( !pParams->GetPair( NONAME, &thePair, Script::NO_ASSERT ) )
		{
			Dbg_Message( "Looking for pair in the format:  SetViewerLODDist (1,500.0f)" );
			return false;
		}

		Obj::CViewerObject* pViewerObject = pViewer->GetViewerObject();
		if ( pViewerObject )
		{
			GetModelComponentFromObject( pViewerObject )->SetModelLODDistance( (int)thePair.mX, thePair.mY );
		}
	}

	return true;
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

bool ScriptGetViewerObjectID( Script::CStruct* pParams, Script::CScript* pScript )
{
	// clear out the viewer object, if any
	Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();
	if ( pViewer )
	{
		Obj::CCompositeObject* pViewerObject = pViewer->GetViewerObject();
		pScript->GetParams()->AddChecksum( CRCD(0x0830ecaf,"objID"), pViewerObject->GetID() );
		pScript->GetParams()->AddChecksum( CRCD(0x52280066,"viewerObjectId"), pViewerObject->GetID() );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptReloadViewerAnim( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();

	if ( pViewer )
	{
		Obj::CViewerObject* pViewerObject = pViewer->GetViewerObject();
		if ( pViewerObject )
		{
			uint32 animName;
			const char* fileName;

			if ( pParams->GetChecksum( NONAME, &animName )
				 && pParams->GetText( NONAME, &fileName ) )
			{
				pViewerObject->ReloadAnim( fileName, animName );
			}
			else
			{
				Dbg_Message( "Need 2 parameters to viewer anim:  filename string, and anim checksum" );
			}
		}
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AddRestartsToMenu | adds all restart points to the menu
bool ScriptAddRestartsToMenu( Script::CStruct* pParams, Script::CScript* pScript )
{
	
	Script::CStruct* p_restart_params;
	int 	node = -1;
	int 	entry = 0;
	do
	{
		node = Obj::GetRestartNode(0, 0, node);
		if (node != -1)
		{
			Script::CArray *pNodeArray=Script::GetArray(CRCD(0xc472ecc5,"NodeArray"));
			Script::CScriptStructure *pNode=pNodeArray->GetStructure(node);
			const char *pName;
			if (!pNode->GetText("RestartName",&pName))
			{
				pName = "Unnamed restart";
			}
			p_restart_params = new Script::CStruct;	
			p_restart_params->AddString("text",pName);
			p_restart_params->AddChecksum("id",123456 + entry);
			p_restart_params->AddChecksum("no_bg",CRCD(0x9f67b2c8,"no_bg"));
			p_restart_params->AddChecksum("centered",CRCD(0x2a434d05,"centered"));
			p_restart_params->AddChecksum("pad_choose_script",Script::GenerateCRC("skip_to_selected_restart"));
			
			// create the parameters that are passed to the X script
			Script::CStruct *p_script_params= new Script::CStruct;
			p_script_params->AddInteger("node_number",node);	
			p_restart_params->AddStructure("pad_choose_params",p_script_params);			

			/*if (!Script::GetInt("SimpleRestarts",false))
			{
				p_restart_params->AddChecksum("focus_script",Script::GenerateCRC("preview_restart"));
				// also pass it too the Focus script
				p_restart_params->AddStructure("focus_params",p_script_params);						
			}*/
			
            // scale the restarts
            float initial_scale;
            pParams->GetFloat( "initial_scale", &initial_scale, Script::ASSERT );
            p_restart_params->AddFloat( "scale", initial_scale );
            //p_restart_params->AddChecksum( "unfocus_script", Script::GenerateCRC( "scale_down_restart" ) );
			
			Script::RunScript("theme_menu_add_item",p_restart_params);
			delete p_restart_params;
			delete p_script_params;

			entry++;
		}
	} while (node != -1);

	if (entry == 0)
	{
			p_restart_params = new Script::CStruct;	
			p_restart_params->AddString("text","No restarts in level");
			p_restart_params->AddChecksum("id",123456 + entry);
			p_restart_params->AddChecksum("no_bg",CRCD(0x9f67b2c8,"no_bg"));
			p_restart_params->AddChecksum("centered",CRCD(0x2a434d05,"centered"));
			p_restart_params->AddChecksum("pad_choose_script",Script::GenerateCRC("skip_to_selected_restart"));
			Script::RunScript("theme_menu_add_item",p_restart_params);
			delete p_restart_params;
	}

	
	
	
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptAddWarpPointsToMenu( Script::CStruct* pParams, Script::CScript* pScript ) {
    Script::CStruct* p_restart_params;
    
    // grab array of warp points
    Script::CArray *p_Array = NULL;

    pParams->GetArray( "nodes", &p_Array );
    Dbg_MsgAssert(p_Array, ("AddWarpPointsToMenu requires an array of nodes"));
    // better only be names
    Dbg_MsgAssert(p_Array->GetType()==ESYMBOLTYPE_NAME,("\n%s\nnodes: Array must be of names",pScript->GetScriptInfo()));
    
    
    // add nodes to menu    
    for (uint32 i=0; i<p_Array->GetSize(); ++i) {
		// Obj_MoveToNode name = <nodename> Orient NoReset
        
        p_restart_params = new Script::CStruct;
        
        Script::CScriptStructure *pNode = SkateScript::GetNode( SkateScript::FindNamedNode( p_Array->GetChecksum( i ) ) );
        const char *pName;
        if (!pNode->GetText("RestartName",&pName))
        {
            pName = "Unnamed restart";
        }        
        
        p_restart_params->AddString( "text", pName );
        p_restart_params->AddChecksum("pad_choose_script",Script::GenerateCRC("WarpSkater"));

        // create the parameters that are passed to the script
        Script::CStruct *p_script_params = new Script::CStruct;
        p_script_params->AddChecksum( "nodename", p_Array->GetChecksum( i ) );
        p_restart_params->AddStructure( "pad_choose_params", p_script_params );

        Script::RunScript( "make_text_sub_menu_item", p_restart_params );
        delete p_restart_params;

    }
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptRunScriptOnObject | allows you to access Obj_ type functions, from a global script
bool ScriptRunScriptOnObject( Script::CStruct* pParams, Script::CScript* pScript )
{
	// searches for object by name

	uint32 obj_id;
	pParams->GetChecksum("id", &obj_id, true);
    
	uint32 scriptName;
	pParams->GetChecksum(NONAME, &scriptName, true);

	Script::CStruct* pSubParams = NULL;
	pParams->GetStructure(CRCD(0x7031f10c,"params"), &pSubParams);

	pScript->GetParams()->RemoveComponent(CRCD(0xa2b033fd,"UniqueID"));	
	
	 // this returns all objects, not just game objects and peds
	Obj::CObject* pObject = Obj::ResolveToObject( obj_id );

	if ( pObject )
	{
		/*
		Script::CScript *pNewScript=Script::SpawnScript(scriptName,pSubParams,0,NULL,scriptName);
		#ifdef __NOPT_ASSERT__
		pNewScript->SetCommentString("Spawned by script command RunScriptOnObject");
		pNewScript->SetOriginatingScriptInfo(pScript->GetCurrentLineNumber(),pScript->mScriptChecksum);
		#endif	
		
		pNewScript->mpObject = pObject;
		
		pScript->GetParams()->AddChecksum("UniqueID",pNewScript->GetUniqueId());
//		Script::RunScript( scriptName, pSubParams, obj_and_id.pObj );

	*/
	
		if (pObject->GetScript())
		{
			// (Mick) If this object has a script, but it's got a NULL mpObject
			// that means that script has been cleared in the current game loop
			// but not got around to being deleted.
			// it's quite safe to re-use it, we just need to set the object on it
			if (pObject->GetScript()->mpObject == NULL)
			{
				pObject->GetScript()->mpObject = pObject;
			}
		
			Dbg_MsgAssert(pObject->GetScript()->mpObject == pObject,("%s\nscript object %p not same as object %p",pScript->GetScriptInfo(),pObject->GetScript()->mpObject.Convert(),pObject));
		}
		// (Mick) Instead of spawning a script, call it on the main script
		// Which allows us to set exceptions and event handlers
		// It's also a lot quicker
		// (Might be problem with parameter conflicts?)
		pObject->CallScript(scriptName, pSubParams);
		
		
		return true;
	
		
	}
	else
	{
		#ifdef	__NOPT_ASSERT__
		printf( "Warning: Couldn't find object %s on which to run script %s\n", Script::FindChecksumName(obj_id), Script::FindChecksumName(scriptName) );
		#endif
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RunScriptOnComponentType | 
bool ScriptRunScriptOnComponentType( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 componentType;
	pParams->GetChecksum( CRCD(0xb6015ea8,"component"), &componentType, Script::ASSERT );

	uint32 target;
	pParams->GetChecksum( CRCD(0xb990d003,"target"), &target, Script::ASSERT );

	Script::CStruct* pSubParams = NULL;
	pParams->GetStructure( CRCD(0x7031f10c,"params"), &pSubParams, Script::NO_ASSERT );

	// new fast way, just go directly to the components, if any
	Obj::CBaseComponent *p_component = Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( componentType );
	while( p_component )
	{
		p_component->CallMemberFunction( target, pSubParams, pScript );
		
		p_component = p_component->GetNextSameType();		
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Debounce | This will ignore the specified button
// until either the time has elapsed, or the button has been
// released and subsequently re-pressed. 
// For X, this prevents the press of X from carrying over from one
// screen to the next, and removes the need to have a delay,
// whilst still allowing the user to actively X past things
// @uparm X | button name
// @parmopt float | time | 1.0 | time to wait (in seconds)
// @parmopt int | clear | 0 | if 1, then clear the current pressed and triggered states
	
bool ScriptDebounce( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 ButtonChecksum=0;
	pParams->GetChecksum(NONAME,&ButtonChecksum);
	if (!ButtonChecksum)
	{
		return true;
	}	
	
	float time=1.0f;
	pParams->GetFloat(CRCD(0x906b67ba, "time"),&time);

	int	clear = 0;
	pParams->GetInteger(CRCD(0x1a4e0ef9, "clear"),&clear);
	
	Obj::CInputComponent* p_input_component
		= static_cast< Obj::CInputComponent* >(Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType(CRC_INPUT));
	while (p_input_component)
	{
		p_input_component->Debounce(ButtonChecksum, time, clear);
		p_input_component = static_cast< Obj::CInputComponent* >(p_input_component->GetNextSameType());
	}
		
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetStatOverride | Set a global stat override for debugging
// set it to 0 to use normal stats
bool ScriptSetStatOverride( Script::CStruct* pParams, Script::CScript* pScript )
{
	float	value = 0.0f;
	pParams->GetFloat(NONAME,&value);
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	skate_mod->SetStatOverride(value);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleRails | Toggle the debug display of rails on and off
bool ScriptToggleRails( Script::CStruct* pParams, Script::CScript* pScript )
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	skate_mod->SetDrawRails(!skate_mod->GetDrawRails());
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleRigidBodyDebug | Toggle the debug display of rigidbody characteristics
bool ScriptToggleRigidBodyDebug( Script::CStruct* pParams, Script::CScript* pScript )
{
	Obj::CRigidBodyComponent::sToggleDrawRigidBodyDebugLines(); 
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CheckForHoles | check the scene file for holes in geometry, and visually display them
bool ScriptCheckForHoles( Script::CStruct* pParams, Script::CScript* pScript )
{
	Nx::CEngine::sDebugCheckForHoles();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | OnXbox |
bool ScriptOnXbox( Script::CStruct* pParams, Script::CScript* pScript )
{
#ifdef __PLAT_XBOX__
	return true;
#else
	return false;
#endif	// __PLAT_XBOX__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GotoXboxDashboard |
bool ScriptGotoXboxDashboard( Script::CStruct* pParams, Script::CScript* pScript )
{
#	ifdef __PLAT_XBOX__
	LD_LAUNCH_DASHBOARD ld;
    ZeroMemory( &ld, sizeof(ld) );

	// In the case where we are rebooting to the memory management section of the dashboard,
	// this value should contain the total number of blocks needed.
	uint32 total_blocks_needed = 0;

	if( pParams->ContainsFlag( 0x1592cbca /*"memory"*/ ))
	{
		pParams->GetInteger(CRCD(0xb204db86,"total_blocks_needed"),(int*)&total_blocks_needed);
		
		ld.dwReason		= XLD_LAUNCH_DASHBOARD_MEMORY;
		ld.dwParameter1	= 'U';
		ld.dwParameter2	= total_blocks_needed;
	}
	else
	{
		ld.dwReason = XLD_LAUNCH_DASHBOARD_MAIN_MENU;
	}

    XLaunchNewImage( NULL, PLAUNCH_DATA( &ld ) );
#	endif	// __PLAT_XBOX__
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptSystemLinkEnabled( Script::CStruct* pParams, Script::CScript* pScript )
{
#ifdef __PLAT_XBOX__
	DWORD result;
	
	result = XNetGetEthernetLinkStatus();
	if(( result & XNET_ETHERNET_LINK_ACTIVE ) == 0 )
	{
		return false;
	}
#endif	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Particle system functions.
// @script | CreateParticleSystem | Creates a particle system.
// @parm name | name | the name of the particle system. (Should be Particle_UnNamed if created from a node)
// @parmopt int | max | 256 | maximum number of particles this system can display.
// @parmopt name | texture | | the name of the texture to apply. No name means use flat shaded.
// @parmopt name | emitscript | | the name of the script to call for emission.
// @parmopt name | updatescript | | the name of the script to call to update position etc.
// @parmopt name | blendmode | diffuse | The blendmode: blend/add/sub/modulate/brighten & 
// fixblend/fixadd/fixsub/fixmodulate/fixbrighten & diffuse (no blend at all). Defaults to diffuse.
// @parmopt int | fix | 0 | Fixed alpha value. 128 is normal. Range is 0-255.
// @parmopt name | params | | Sets a parameter block for use with the created particle scripts.
// @parmopt name | type | flat | Sets the type of particle (flat/shaded/smooth/glow). Defaults to flat.
// @parmopt int | segments | 8 | The number of segments in a segmented particle, such as 'Glow'.
// @parmopt float | split | 0.5 | Sets the split point in a segmented particle. 0 = split at center. 1 means split at outer edge.
// @parmopt int | history | 1 | Number of history lists to keep. Minimum is 1 when using trail particle types.
// @parmopt int | perm | 0 | set to 1 if particle system stays between levels (like the skaters)
// update script to grab bone/node positions when attaching.
//
// 
bool ScriptCreateParticleSystem( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 name = 0;
	pParams->GetChecksum("Name",&name);

	int max = 256;
	pParams->GetInteger("Max",&max);

	int max_streams = 2;
	pParams->GetInteger("MaxStreams",&max_streams);

	uint32 texture = 0;
	pParams->GetChecksum("Texture",&texture);

	uint32 blendmode = 0x515e298e;		// Defaults to diffuse.
	pParams->GetChecksum("blendmode",&blendmode);
	
	uint32 type = 0xaab555bb;		// Defaults to flat.
	pParams->GetChecksum("type",&type);
	
	int fix = 0;
	pParams->GetInteger("fix",&fix);

	int segments = 8;
	pParams->GetInteger("segments",&segments);

	float split = 0.5f;
	pParams->GetFloat("split",&split);

	int history = 1;
	pParams->GetInteger("history",&history);

	int perm = 0;
	pParams->GetInteger("perm",&perm);

	// Create a particle. Will be internally added to the process list, so we don't need to do
	// anything after it has been created.
	Nx::CParticle * p_particle = Nx::create_particle( name, type, max, max_streams, texture, blendmode, fix, segments, split, history, perm );
	
	Script::CStruct* pSubParams = NULL;
	pParams->GetStructure( "params", &pSubParams, Script::NO_ASSERT );

	uint32 emitscript = 0;
	if ( pParams->GetChecksum("emitscript",&emitscript) )
	{
		p_particle->set_emit_script( emitscript, pSubParams );
	}

								
//	p_particle->SetActive( false );
//								
//	uint32 active = 0;
//	if ( pParams->GetChecksum("active",&active) )
//	{
//		p_particle->SetActive( active );
//	}
	
	uint32 updatescript = 0;
	if ( pParams->GetChecksum("updatescript",&updatescript) )
	{
		p_particle->set_update_script( updatescript, pSubParams );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetScript | Sets the emission script.
// @parm name | name | the name of the particle system.
// @parmopt name | emitscript | | the name of the script to call for emission.
// @parmopt name | updatescript | | the name of the script to call to update position etc.
// @parmopt name | params | | Sets a parameter block for use with the created particle scripts.
bool ScriptSetScript( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 name = 0;
	pParams->GetChecksum("Name",&name);

	Script::CStruct* pSubParams = NULL;
	pParams->GetStructure( "params", &pSubParams, Script::NO_ASSERT );

	Nx::CParticle * p_particle = Nx::get_particle( name );
	Dbg_MsgAssert( p_particle, ( "Couldn't find the requested particle system." ) );

	uint32 emitscript = 0;
	if ( pParams->GetChecksum("emitscript",&emitscript) )
	{
		p_particle->set_emit_script( emitscript, pSubParams );
	}
	
	uint32 updatescript = 0;
	if ( pParams->GetChecksum("updatescript",&updatescript) )
	{
		p_particle->set_update_script( updatescript, pSubParams );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DestroyParticleSystem | Destroys a particle system.
// @parm name | name | the name of the particle system.
// @parmopt int | ifempty | 0 | Set this to 1 if you want the particle system to be destroyed only when empty.
bool ScriptDestroyParticleSystem( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 name = 0;
	pParams->GetChecksum("Name",&name);

	int ifempty = 0;
	pParams->GetInteger("IfEmpty",&ifempty);

	if ( ifempty )
	{
		Nx::destroy_particle_when_empty( name );
	}
	else
	{
		Nx::destroy_particle( name );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EmptyParticleSystem | Empties the particle system immediately (sets number of particles to 0. 
// @parm name | name | the name of the particle system.
bool ScriptEmptyParticleSystem( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 name = 0;
	pParams->GetChecksum("Name",&name);

	Nx::CParticle * p_particle = Nx::get_particle( name );
	Dbg_MsgAssert( p_particle, ( "Couldn't find the requested particle system." ) );

	p_particle->SetNumParticles( 0 );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ParticleExists | Returns whether the specified particle system exists
// @parm name | name | the name of the particle system.
bool ScriptParticleExists( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 name = 0;
	if ( !pParams->GetChecksum("Name",&name ) )
	{
		pParams->GetChecksum(NONAME,&name,Script::ASSERT);
	}

	Nx::CParticle * p_particle = Nx::get_particle( name );

	if ( p_particle )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StructureContains | true if the structure contains an element
// or flag with the specified name
// @parm structure | structure | the structure to check
// @uparm name | the named param to look for
bool ScriptStructureContains( Script::CStruct* pParams, Script::CScript* pScript )
{
	Script::CStruct* p_struct;
	if ( pParams->GetStructure( "structure", &p_struct, Script::NO_ASSERT ) )
	{	
		// printf("got a structure\n");
		uint32 name;
		pParams->GetChecksum( NONAME, &name, Script::NO_ASSERT );
		Dbg_MsgAssert( name, ( "StructureContains called without a param name" ) );
		// printf("checking for a name of %x\n", name );
		if ( p_struct->ContainsComponentNamed( name ) )
		{
			return true;
		}
		else
			return p_struct->ContainsFlag( name );
	}
	// printf("didn't get a structure\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetBonePosition | allows you to access Obj_ type functions, from a global script
bool ScriptGetBonePosition( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mth::Vector bonePos;
	bool success = false;
	
	// searches for object by m_id
	uint32 obj_id;
	pParams->GetChecksum( CRCD(0x40c698af,"id"), &obj_id, Script::ASSERT );
	
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();	
	Obj::CGeneralManager* pObjectManager = skate_mod->GetObjectManager();
	Dbg_Assert( pObjectManager );

	Obj::CObject* pObject = pObjectManager->GetObjectByID( obj_id );
	if ( pObject )
	{
		Dbg_MsgAssert( static_cast<Obj::CMovingObject*>( pObject ), ( "This function only works on moving objects." ) ); 
		Obj::CMovingObject* pMovingObject = (Obj::CMovingObject*)pObject;

		uint32 boneName;
		pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, Script::ASSERT );
		
		Obj::CSkeletonComponent* p_skeleton_component = GetSkeletonComponentFromObject(pMovingObject);
		Dbg_Assert(p_skeleton_component);
		success = p_skeleton_component->GetBoneWorldPosition( boneName, &bonePos );
	}
	
	// make sure we have somewhere to return the data
	Dbg_Assert( pScript && pScript->GetParams() );
	pScript->GetParams()->AddFloat( CRCD(0x7323e97c,"x"), bonePos[X] );
	pScript->GetParams()->AddFloat( CRCD(0x424d9ea,"y"), bonePos[Y] );
	pScript->GetParams()->AddFloat( CRCD(0x9d2d8850,"z"), bonePos[Z] );

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ShouldEmitParticles | tests whether the specified particle system is active
bool ScriptShouldEmitParticles( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 name;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &name, Script::ASSERT );

	Nx::CParticle * p_particle = Nx::get_particle( name );

	if ( p_particle )
	{
		return p_particle->IsEmitting();
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ParticlesOn | turns on the specified particle system
bool ScriptParticlesOn( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 name;
	pParams->GetChecksum( "name", &name, Script::ASSERT );

	Nx::CParticle * p_particle = Nx::get_particle( name );

	if ( p_particle )
	{
		p_particle->SetEmitting( true );
	}
	else
	{
//		Dbg_MsgAssert( 0, ( "Couldn't find particle system to turn on" ) );
	}

	return ( p_particle != NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ParticlesOff | turns off the specified particle system
bool ScriptParticlesOff( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 name;
	pParams->GetChecksum( "name", &name, Script::ASSERT );

	Nx::CParticle * p_particle = Nx::get_particle( name );

	if ( p_particle )
	{
		p_particle->SetEmitting( false );
	}
	else
	{
//		Dbg_MsgAssert( 0, ( "Couldn't find particle system to turn off" ) );
	}

	return ( p_particle != NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MangleChecksums | adds 2 checksums together and returns the result in "mangled_id"
// @parm name | a | First checksum
// @parm name | b | Second checksum
// @parm name | mangled_id | The returned value for the mangled checksum
bool ScriptMangleChecksums( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 a;
	if ( !pParams->GetChecksum( CRCD(0x174841bc,"a"), &a, Script::NO_ASSERT ) )
	{
		pParams->GetInteger( CRCD(0x174841bc,"a"), (int*)&a, Script::ASSERT );
	}
	
	uint32 b;
	if ( !pParams->GetChecksum( CRCD(0x8e411006,"b"), &b, Script::NO_ASSERT ) )
	{
		pParams->GetInteger( CRCD(0x8e411006,"b"), (int*)&b, Script::ASSERT );
	}
	
	uint32 mangled_id = a + b;
	Dbg_Assert( pScript && pScript->GetParams() );
	pScript->GetParams()->AddChecksum( CRCD(0xfba40626,"mangled_id"), mangled_id );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AppendSuffixToChecksum | Appends a string suffix to a checksum and returns the result in "appended_id"
// @parm name | Base | The base checksum
// @parm string | SuffixString | The suffix string
bool ScriptAppendSuffixToChecksum( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 base_checksum;
	pParams->GetChecksum(CRCD(0x3f4b019e, "Base"), &base_checksum, Script::ASSERT);
	
	const char* suffix_string;
	pParams->GetString(CRCD(0x94542eea, "SuffixString"), &suffix_string, Script::ASSERT);
	
	pScript->GetParams()->AddChecksum(CRCD(0xafb911c6, "appended_id"), Crc::ExtendCRCWithString(base_checksum, suffix_string));
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RotateVector | rotates a vector by the specified amount
// @parm float | x | X value of the vector to be rotated (also, the return value)
// @parm float | y | Y value of the vector to be rotated (also, the return value)
// @parm float | z | Z value of the vector to be rotated (also, the return value)
// @parm float | rx | Amount to rotate on X, in degrees
// @parm float | ry | Amount to rotate on Y, in degrees
// @parm float | rz | Amount to rotate on Z, in degrees
bool ScriptRotateVector( Script::CStruct* pParams, Script::CScript* pScript )
{
	// get original vector:
	Mth::Vector vec;
	pParams->GetFloat( "x", &vec[X], Script::ASSERT );
	pParams->GetFloat( "y", &vec[Y], Script::ASSERT );
	pParams->GetFloat( "z", &vec[Z], Script::ASSERT );
	vec[W] = 1.0f;

	// get amount to rotate (in degrees)
	float rx = 0.0f;
	float ry = 0.0f;
	float rz = 0.0f;
	pParams->GetFloat( "rx", &rx, Script::NO_ASSERT );
	pParams->GetFloat( "ry", &ry, Script::NO_ASSERT );
	pParams->GetFloat( "rz", &rz, Script::NO_ASSERT );

	// rotate the vector
	Mth::Matrix mat;
	mat.Ident();
	mat.RotateXLocal( Mth::DegToRad(rx) );
	mat.RotateYLocal( Mth::DegToRad(ry) );
	mat.RotateZLocal( Mth::DegToRad(rz) );
	vec = mat.Transform( vec );

	// return the vector
	pScript->GetParams()->AddFloat( "x", vec[X] );
	pScript->GetParams()->AddFloat( "y", vec[Y] );
	pScript->GetParams()->AddFloat( "z", vec[Z] );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsPS2 | Returns true if the current hardware is PS2 (proview/devkit/regular).
bool ScriptIsPS2( Script::CStruct* pParams, Script::CScript* pScript )
{
	if (( Config::GetHardware() == Config::HARDWARE_PS2 ) ||
		( Config::GetHardware() == Config::HARDWARE_PS2_PROVIEW ) ||
		( Config::GetHardware() == Config::HARDWARE_PS2_DEVSYSTEM ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsNGC | Returns true if the current hardware is GameCube.
bool ScriptIsNGC( Script::CStruct* pParams, Script::CScript* pScript )
{
	if ( ( Config::GetHardware() == Config::HARDWARE_NGC ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsXBOX | Returns true if the current hardware is Xbox.
bool ScriptIsXBOX( Script::CStruct* pParams, Script::CScript* pScript )
{
	if ( ( Config::GetHardware() == Config::HARDWARE_XBOX ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsWin32 | Returns true if the current hardware is Win32.
bool ScriptIsWIN32( Script::CStruct* pParams, Script::CScript* pScript )
{
	if ( ( Config::GetHardware() == Config::HARDWARE_WIN32 ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetPlatform | Puts the name of the current platform as a checksum into a parameter
// called Platform. The value will be either ps2, xbox, ngc or win32
// Handy for use in script switch statements, rather than having to have nested if's
bool ScriptGetPlatform( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 platform=0;
	
	switch (Config::GetHardware())
	{
	case Config::HARDWARE_PS2:
	case Config::HARDWARE_PS2_PROVIEW:
	case Config::HARDWARE_PS2_DEVSYSTEM:
		platform=0x988a3508; // ps2
		break;
	case Config::HARDWARE_NGC:
		platform=0xbcf00d45; // ngc
		break;
	case Config::HARDWARE_XBOX:
		platform=0x87d839b8; // xbox
		break;
	case Config::HARDWARE_WIN32:
		platform=0x4af45e2e; // win32
		break;
	default:
		break;
	}
	
	pScript->GetParams()->AddChecksum("Platform",platform);		
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptIsPal( Script::CStruct* pParams, Script::CScript* pScript )
{
	return Config::PAL();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PushMemProfile | push an arbitarily named memory profile (Mick's use only)
bool ScriptPushMemProfile( Script::CStruct* pParams, Script::CScript* pScript )
{
	
	const char* pContext;
	pParams->GetText( NONAME, &pContext, true );
	Mem::PushMemProfile((char*)pContext);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PopMemProfile | pop a mem profile that was pushed (Mick's use only)
bool ScriptPopMemProfile( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mem::PopMemProfile();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | TogglePass | Toggle which passes are displayed
bool ScriptTogglePass( Script::CStruct* pParams, Script::CScript* pScript )
{
	#ifdef	__PLAT_NGPS__
	#ifdef	__NOPT_ASSERT__
	static 	int pass = 0;
	pass++;

	// Let us put a pass value in that will override the toggling value  
	pParams->GetInteger(CRCD(0x318f2bdb,"Pass"),&pass);
	
	if (pass == 5)
	{
		pass = 0;
	}
	if (pass)
	{
		NxPs2::gPassMask1 = 0xc0; 			// 1<<6 | 1<<7  (0x40, 0x80)
		NxPs2::gPassMask0 = (pass-1)<<6;		
	}
	else
	{
		NxPs2::gPassMask1 = 0;
		NxPs2::gPassMask0 = 0;
	}
	#endif
	#endif
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetScreen | Sets default screen aspect ratio and camera
// @parm float | Aspect | aspect ratio, usually 16/9 (1.777777777) or 4/3 (1.3333333333)
// @parm float | Angle | screen angle in degrees usally 72, or 80 for 16:9 
// @parm int | Letterbox | 0 for non-letterbox, 1 for letterboxed.  Normally used with 4/3 aspect 
bool	ScriptSetScreen( Script::CStruct* pParams, Script::CScript* pScript )
{
	float	Aspect;
	float	Angle;
	
	if (pParams->GetFloat("Aspect",&Aspect))
	{
		Nx::CViewportManager::sSetScreenAspect(Aspect);
	}
	
	if (pParams->GetFloat("Angle",&Angle))
	{
		Nx::CViewportManager::sSetScreenAngle(Angle);
	}
	
	int letterbox;
	if (pParams->GetInteger("Letterbox",&letterbox))
	{
		Nx::CEngine::sSetLetterbox(letterbox);
	}
	
	return true;	   
	   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetUpperCaseString( Script::CStruct* pParams, Script::CScript* pScript )
{
	const char* initial_string;
	pParams->GetString( NONAME, &initial_string, Script::ASSERT );

	char new_string[128];
	strcpy( new_string, initial_string );

	Str::UpperCase( new_string );
	pScript->GetParams()->AddString( "UpperCaseString", new_string );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetGameMode( Script::CStruct* pParams, Script::CScript* pScript )
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	uint32 gameMode = skate_mod->GetGameMode()->GetNameChecksum();

	pScript->GetParams()->AddChecksum( "GameMode", gameMode );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptStartKeyboardHandler( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::FrontEnd* front = Mdl::FrontEnd::Instance();
	int max_length;

	pParams->GetInteger( "max_length", &max_length, true );
	front->AddKeyboardHandler( max_length );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptEnableKeyboard( Script::CStruct* pParams, Script::CScript* pScript )
{
#ifdef __PLAT_NGPS__
	SIO::EnableKeyboard( true );
#endif
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptDisableKeyboard( Script::CStruct* pParams, Script::CScript* pScript )
{
#ifdef __PLAT_NGPS__
	SIO::EnableKeyboard( false );
#endif
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptStopKeyboardHandler( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::FrontEnd* front = Mdl::FrontEnd::Instance();

	front->RemoveKeyboardHandler();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CreateIndexArray | creates an array of ints, where each element
// is equal to its own index
// @uparm 1 | size of array
bool ScriptCreateIndexArray( Script::CStruct* pParams, Script::CScript* pScript )
{
	int size;
	pParams->GetInteger( NONAME, &size, Script::ASSERT );

	if ( size < 1 )
		Dbg_MsgAssert( 0, ( "CreateIndexArray called with size less than 1" ) );

	Script::CArray *p_index_array = new Script::CArray();
	p_index_array->SetSizeAndType( size, ESYMBOLTYPE_INTEGER );

	for ( int i = 0; i < size; i++ )
		p_index_array->SetInteger( i, i );

	pScript->GetParams()->AddArray( "index_array", p_index_array );
	Script::CleanUpArray( p_index_array );
	delete p_index_array;
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetRescaledTargetValue | 
// @parm float | min |  
// @parm float | max | 
// @parm float | target | 
// @parm float | result | 
bool ScriptGetRescaledTargetValue( Script::CStruct* pParams, Script::CScript* pScript )
{
	float min;
	float max;
	float target;
	float result;

	pParams->GetFloat( "min", &min, true );
	pParams->GetFloat( "max", &max, true );
	pParams->GetFloat( "target", &target, true );

	result = ( target - min ) / ( max - min );

	if ( pParams->ContainsFlag( "sin_curve" ) )
	{
		result = asinf( result ) * 4096.0f / ( 2.0f * Mth::PI );
	
		if ( pParams->ContainsFlag( "backwards" ) )
		{
			result = 2048.0f - result;
		}
	}

//	Dbg_Message( "min = %f max = %f target = %f result = %f", min, max, target, result );

	// this finds the scaled value between 0.0 and 1.0
	pScript->GetParams()->AddFloat( "result", result );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MenuIsSelected | Checks names in pArray to find selected
bool ScriptMenuIsSelected(Script::CStruct *pParams, Script::CScript *pScript)
{
	// If there is an array of names, check them.		
	Script::CArray *pArray=NULL;
	if (pParams->GetArray(NONAME,&pArray))
	{
		Dbg_MsgAssert(pArray,("Eh? NULL pArray?"));
		for (uint32 i=0; i<pArray->GetSize(); ++i)
		{
			if (MenuIsSelected(pArray->GetNameChecksum(i)))
			{
				return true;
			}
		}
	}
	
	uint32 id=0;
	if (pParams->GetChecksum(NONAME,&id))
	{
		if (MenuIsSelected(id))
		{
			return true;
		}	
	}
			
	return false;			
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetNodeName | Gets the node name of the object running this script, and puts
// it into a parameter called NodeName.
// If there is no such object, it will not add NodeName and will return false.
bool ScriptGetNodeName(Script::CStruct *pParams, Script::CScript *pScript)
{
	if (pScript->mpObject)
	{
		Obj::CMovingObject *p_pos_obj = static_cast<Obj::CMovingObject*>(pScript->mpObject.Convert());
		if (p_pos_obj)
		{
			if (p_pos_obj->GetID())
			{
				pScript->GetParams()->AddChecksum("NodeName",p_pos_obj->GetID());
				return true;
			}
		}
	}			

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetTesterScript | Sets and starts running the tester script. This is a script which
// will never die and will always be having its update function called every frame.
// Used for auto testing levels.
// SetTesterScript may be called more than once. Any new call will stop the current tester script
// and start the new one.
// If the tester script hits an endscript it still won't die, it will just stick on the endscript
// and not do anything. (This uses negligible processing time)
// @uparm name | The name of the script to run.
bool ScriptSetTesterScript(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 tester_script=0;
	CStruct *p_params=NULL;
	
	pParams->GetChecksum(NONAME,&tester_script);
	pParams->GetStructure(CRCD(0x7031f10c,"params"),&p_params);
	
	 Mdl::Skate * pSkate =  Mdl::Skate::Instance();

	Dbg_MsgAssert(pSkate->mp_gameFlow,("NULL pSkate->mp_gameFlow ??"));
	pSkate->mp_gameFlow->SetTesterScript(tester_script, p_params);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | KillTesterScript | Kills any tester script that is running. 
// Returns true if a tester script was running before, false otherwise.
bool ScriptKillTesterScript(Script::CStruct *pParams, Script::CScript *pScript)
{
	 Mdl::Skate * pSkate =  Mdl::Skate::Instance();

	Dbg_MsgAssert(pSkate->mp_gameFlow,("NULL pSkate->mp_gameFlow ??"));
	return pSkate->mp_gameFlow->KillTesterScript();
}									 

// Memory manager related functions

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MemPushContext | 
// @uparm 1 | value
bool ScriptMemPushContext( Script::CStruct *pParams, Script::CScript *pScript )
{
//	MemViewToggle();
	
	// 0 is the special case name
	int Val=0;			   			// default to 0
	if ( pParams->GetInteger(NONAME,&Val,false) )
	{
		if ( Val == 0 )
		{
			// is the bottom up heap
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
		}
		else
		{
			Dbg_MsgAssert( 0, ( "Must specify heap name (instead of number) for MemPushContext (ex: BottomUpHeap)" ) );
		}
	}
	else
	{
		// otherwise, look it up by name
		uint32 whichHeap;
		pParams->GetChecksum( NONAME, &whichHeap, true );
		Mem::Manager& mem_man = Mem::Manager::sHandle();
		Mem::Heap* pHeap = mem_man.GetHeap( whichHeap );
		Dbg_Assert( pHeap );

		Mem::Manager::sHandle().PushContext( pHeap );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MemPopContext |
bool ScriptMemPopContext( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mem::Manager::sHandle().PopContext();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MemInitHeap |
// @parm name | name | the name of heap to create
// @parm int | size | the size of heap to create
bool ScriptMemInitHeap(Script::CStruct *pParams, Script::CScript *pScript)
{	
	const char* pHeapName;
	pParams->GetText( "name", &pHeapName, true );

	int heapSize;
	pParams->GetInteger( "size", &heapSize, true );

	uint32 heapName;
	heapName = Script::GenerateCRC( pHeapName );

	Mem::Manager& mem_man = Mem::Manager::sHandle();
	Dbg_MsgAssert( !mem_man.NamedHeap(heapName, false), ( "named heap %s already exists", pHeapName ) )
	mem_man.InitNamedHeap( heapName, heapSize, pHeapName );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MemDeleteHeap |
// @parm name | name | the name of heap to delete
bool ScriptMemDeleteHeap(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char* pHeapName;
	pParams->GetText( "name", &pHeapName, true );

	Mem::Manager& mem_man = Mem::Manager::sHandle();	
	return mem_man.DeleteNamedHeap( Script::GenerateCRC( pHeapName ), false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptMemThreadSafe( Script::CStruct *pParams, Script::CScript *pScript )
{
	if (pParams->ContainsFlag( Script::GenerateCRC( "off" )))
	{
		Mem::SetThreadSafe( false );
	}
	else
	{
		Mem::SetThreadSafe( true );
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AnalyzeHeap | analyzes specified heap
// @uparmopt BottomUpHeap | heap to analyze
bool ScriptAnalyzeHeap( Script::CStruct *pParams, Script::CScript *pScript )
{
	// GJ:  I am planning to refactor this!
	Mem::Manager& mem_man = Mem::Manager::sHandle();
	Mem::Heap* pHeap = NULL;
	uint32 whichHeap = Script::GenerateCRC("BottomUpHeap");
	if ( pParams )
	{
		pParams->GetChecksum( NONAME, &whichHeap );
	}

	pHeap = mem_man.GetHeap( whichHeap );

#ifndef __PLAT_NGC__
	if ( pHeap )
	{
		MemView_AnalyzeHeap( pHeap );
	}
#endif		// __PLAT_NGC__

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PrintMemInfo | displays mem info about specified heap
// @uparmopt BottomUpHeap | which heap to print
bool ScriptPrintMemInfo( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mem::Manager& mem_man = Mem::Manager::sHandle();
	Mem::Heap* pHeap = NULL;
	char buf[64];
	uint32 whichHeap = Script::GenerateCRC("BottomUpHeap");
	if ( pParams )
	{
		pParams->GetChecksum( NONAME, &whichHeap );
	}
	
	pHeap = mem_man.GetHeap( whichHeap );
	strcpy( buf, mem_man.GetHeapName( whichHeap ) );

	if ( pHeap )
	{
		printf(  "\n" );

		printf( "%s:\n", buf );
		printf( "Used %dK (%d) Peak %dK (%d)\n",
				pHeap->mUsedMem.m_count / 1024,
				pHeap->mUsedBlocks.m_count,
				pHeap->mUsedMem.m_peak / 1024,
				pHeap->mUsedBlocks.m_peak );
		printf( "Frag %dK (%d) Peak %dK (%d)\n",
				pHeap->mFreeMem.m_count / 1024,
				pHeap->mFreeBlocks.m_count,
				pHeap->mFreeMem.m_peak / 1024,
				pHeap->mFreeBlocks.m_peak );

		printf(  "\n" );
	}
	
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DisplayFreeMem | displays free mem info of specified heap
bool ScriptDisplayFreeMem( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mem::Manager& mem_man = Mem::Manager::sHandle();

	Mem::Heap* heap;
   
	Script::CStruct* params;

	params = new Script::CStruct;
	for (heap = mem_man.FirstHeap(); heap != NULL; heap = mem_man.NextHeap(heap))
	{		
			Mem::Region* region = heap->ParentRegion();			
			
			params->Clear();
			
			params->AddChecksum( "id", Script::GenerateCRC( heap->GetName() ) );
			params->AddInteger( "free_mem", region->MemAvailable() );
			params->AddInteger( "min_free_mem", region->MinMemAvailable() );

			Script::RunScript( "UpdateDisplayFreeMemory", params );
			
	}

	delete params;
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DumpHeaps | prints out heap contents
// @flag ExactValues | Prints the exact heap sizes in bytes.
bool ScriptDumpHeaps( Script::CStruct *pParams, Script::CScript *pScript )
{
#	if !defined( __PLAT_NGC__ ) || ( defined( __PLAT_NGC__ ) && !defined( __NOPT_FINAL__ ) )

	Script::DumpLastStructs();

	Mem::Manager& mem_man = Mem::Manager::sHandle();

#	ifndef __PLAT_XBOX__
#ifndef __PLAT_NGC__
	char buf[512];
#endif		// __PLAT_NGC__
#	endif

	Mem::Heap* heap;

#	ifndef __PLAT_XBOX__
#ifndef __PLAT_NGC__
	dump_open("blah.txt");
#endif		// __PLAT_NGC__
#	endif

	if (pParams->ContainsFlag(CRCD(0xd65b9ac7,"ExactValues")))
	{
		printf("Name              Used    Frag    Free    Min Blocks\n");
		printf("--------------- ------ ------- ------- ------ ------\n");
		for (heap = mem_man.FirstHeap(); heap != NULL; heap = mem_man.NextHeap(heap))
		{		
				Mem::Region* region = heap->ParentRegion();			
				printf( "%12s: %8d %7d %7d %7d  %5d \n",
						heap->GetName(),
						heap->mUsedMem.m_count ,
						heap->mFreeMem.m_count ,
						region->MemAvailable() ,
						region->MinMemAvailable() ,
						heap->mUsedBlocks.m_count
						);										
		}
	}	
	else
	{
		printf("Name            Used  Frag  Free   Min  Blocks\n");
		printf("--------------- ----- ----- ---- ------ ------\n");
		for (heap = mem_man.FirstHeap(); heap != NULL; heap = mem_man.NextHeap(heap))
		{		
				Mem::Region* region = heap->ParentRegion();			
				printf( "%12s: %5dK %4dK %4dK %4dK  %5d \n",
						heap->GetName(),
						heap->mUsedMem.m_count / 1024,
						heap->mFreeMem.m_count / 1024,
						region->MemAvailable() / 1024,
						region->MinMemAvailable() / 1024,
						heap->mUsedBlocks.m_count
						);										
		}
	}

#	ifndef __PLAT_XBOX__
#ifndef __PLAT_NGC__
	dump_close();
#endif		// __PLAT_NGC__
#	endif
	
	Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
	printf( "\nSound mem free %d K  (%d bytes)\n", 
		 sfx_manager->MemAvailable() / 1024, sfx_manager->MemAvailable() );

#	ifndef __PLAT_XBOX__
#ifndef __PLAT_NGC__
	char fileName[256];
	int i = 0;
	while ( TRUE )
	{
		// The filename is formed from the level name a number, and the free memory
		// you should sort them by DATE, and not by filename
		// 
		sprintf( fileName, "dumps\\%s_dump_%02d_%s.txt",
						   s_level_name,
//						   __nDATE__,
					//	   __nTIME__,
						   i,
						   Str::PrintThousands((mem_man.BottomUpHeap()->ParentRegion()->MemAvailable())) );

		printf ("Filename = <%s>, %d chars\n",fileName,strlen(fileName));
						   
		if ( FALSE == File::Exist( fileName ))
			break;

		i++;
	}
#endif		// __PLAT_NGC__
#	endif // __PLAT_XBOX__

#	ifndef __PLAT_XBOX__
#ifndef __PLAT_NGC__
	if( dump_open(fileName))
	{
		sprintf(buf,"Name            Used  Frag  Free   Blocks\n");
		dump_printf(buf);
		sprintf(buf,"--------------- ----- ----- ------ ------\n");
		dump_printf(buf);
		for (heap = mem_man.FirstHeap(); heap != NULL; heap = mem_man.NextHeap(heap))
		{		
				Mem::Region* region = heap->ParentRegion();			
				sprintf(buf, "%12s: %5dK %4dK %4dK   %5d \n",
						heap->GetName(),
						heap->mUsedMem.m_count / 1024,
						heap->mFreeMem.m_count / 1024,
						region->MemAvailable() / 1024,
						heap->mUsedBlocks.m_count
						);										
				dump_printf(buf);
		}
		sprintf( buf, "\nSound mem free %d K  (%d bytes)\n\n\n", 
			 sfx_manager->MemAvailable() / 1024, sfx_manager->MemAvailable() );
		dump_printf(buf);
	
		sprintf(buf,"Summary\n----------\n\n");
		dump_printf(buf);
		Mem::DumpMemProfile(3);
		sprintf(buf,"\n\nDetails\n----------\n\n");
		dump_printf(buf);
		Mem::DumpMemProfile(100);

		dump_close();
	}
#endif		// __PLAT_NGC__
#	endif //__PLAT_XBOX__

	printf("\nScript pool usage:\n");
	printf("CStruct:           %5d of %5d, max %5d\n",CStruct::SGetNumUsedItems(),CStruct::SGetTotalItems(),CStruct::SGetMaxUsedItems());
	printf("CComponent:        %5d of %5d, max %5d\n",CComponent::SGetNumUsedItems(),CComponent::SGetTotalItems(),CComponent::SGetMaxUsedItems());
	printf("CSymbolTableEntry: %5d of %5d, max %5d\n",CSymbolTableEntry::SGetNumUsedItems(),CSymbolTableEntry::SGetTotalItems(),CSymbolTableEntry::SGetMaxUsedItems());
	printf("CVector:           %5d of %5d, max %5d\n",CVector::SGetNumUsedItems(),CVector::SGetTotalItems(),CVector::SGetMaxUsedItems());
	printf("CPair:             %5d of %5d, max %5d\n",CPair::SGetNumUsedItems(),CPair::SGetTotalItems(),CPair::SGetMaxUsedItems());
	printf("CArray:            %5d of %5d, max %5d\n",CArray::SGetNumUsedItems(),CArray::SGetTotalItems(),CArray::SGetMaxUsedItems());
	printf("CScript:           %5d of %5d, max %5d\n",CScript::SGetNumUsedItems(),CScript::SGetTotalItems(),CScript::SGetMaxUsedItems());
	Mem::CCompactPool *p_hash = Mem::PoolManager::SGetPool(Mem::PoolManager::vHASH_ITEM_POOL);
	printf("CHashItem:         %5d of %5d, max %5d\n",p_hash->GetNumUsedItems(),p_hash->GetTotalItems(),p_hash->GetMaxUsedItems());
	printf("\n(HashItems are init in main.cpp, others are in init.cpp)\n");
#endif		// __NOPT_FINAL__

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DumpFragments | prints out heap fragments
bool ScriptDumpFragments( Script::CStruct *pParams, Script::CScript *pScript )
{
	int maxFragmentation = 10000;
	pParams->GetInteger( NONAME, &maxFragmentation, Script::NO_ASSERT );

	if ( pParams->ContainsFlag( "K" ) )
	{
		maxFragmentation *= 1024;
	}

#ifdef __NOPT_ASSERT__
	Mem::Manager& mem_man = Mem::Manager::sHandle();
	
	// sanity checks for fragmentation
	Mem::Heap* heap = mem_man.BottomUpHeap();
	int fragmentation = heap->mFreeMem.m_count;
	
	if (fragmentation > maxFragmentation)
	{
		// not guaranteed to do anything useful.... might crash the debugger (Mick)
		printf ("Dumping Fragments.....\n");
		MemView_DumpFragments(heap);
		printf ("Done Dumping Fragments.....\n");
	}
	
	Dbg_MsgAssert(fragmentation < maxFragmentation, ("Excessive bottom up fragmentation (%d) after cleanup (max=%d)",fragmentation,maxFragmentation)); 
	
	heap = mem_man.TopDownHeap();
	fragmentation = heap->mFreeMem.m_count;
	if (fragmentation > maxFragmentation)
	{
		// not guaranteed to do anything useful.... might crash the debugger (Mick)
		printf ("Dumping Fragments.....\n");
		MemView_DumpFragments(heap);
		printf ("Done Dumping Fragments.....\n");
	}
	
	Dbg_MsgAssert(fragmentation < maxFragmentation, ("Excessive top down fragmentation (%d) after cleanup (max=%d)",fragmentation,maxFragmentation)); 
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptClearStruct( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 structName;
	pParams->GetChecksum( "struct", &structName, Script::ASSERT );

	Script::CStruct* pStruct = Script::GetStructure( structName, Script::ASSERT );
	pStruct->Clear();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptAppendStruct( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 structName;
	pParams->GetChecksum( "struct", &structName, Script::ASSERT );

	uint32 fieldName;
	pParams->GetChecksum( "field", &fieldName, Script::ASSERT );
	
	Script::CStruct* pAppendParams;
	pParams->GetStructure( "params", &pAppendParams, Script::ASSERT );

	Script::CStruct* pSubStruct = new Script::CStruct;
	pSubStruct->AppendStructure( pAppendParams );
	
	Script::CStruct* pStruct = Script::GetStructure( structName, Script::ASSERT );
	pStruct->AddStructurePointer( fieldName, pSubStruct ); 

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptScriptExists( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 script_name_checksum=0;
	pParams->GetChecksum(NONAME,&script_name_checksum);

	CSymbolTableEntry *p_entry=Resolve(script_name_checksum);
	if (p_entry && p_entry->mType==ESYMBOLTYPE_QSCRIPT)
	{
		return true;
	}
	return false;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptSpawnSecondControllerCheck( Script::CStruct* pParams, Script::CScript* pScript )
{
	 Mlp::Manager * mlp_manager =  Mlp::Manager::Instance();
	int not_used;

	if( s_second_controller_check_task != NULL )
	{
		return true;
	}

	not_used = 0;
	s_second_controller_check_task = new Tsk::Task< int > ( s_second_controller_check_code, not_used );
	mlp_manager->AddLogicTask ( *s_second_controller_check_task );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptStopSecondControllerCheck( Script::CStruct* pParams, Script::CScript* pScript )
{
	Dbg_Assert( s_second_controller_check_task != NULL );

	delete s_second_controller_check_task;
	s_second_controller_check_task = NULL;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LoadExecPS2 | 
// @parm string | elf | elf file to load
// if this fails on a TOOL turn off SN profiling in main.cpp
// and check the media type in prepare_to_exit
bool ScriptLoadExecPS2( Script::CStruct *pParams, Script::CScript *pScript )
{

#ifdef __PLAT_NGPS__
	const char* p_string;
	pParams->GetText( "elf", &p_string, true );
    Sys::LoadExec( p_string );
#endif // __PLAT_NGPS__
	
    return true;		// actually will never get here}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*
- TODO - factor out the common functionality of ExitDemo into LoadExecPS2

the LoadExecPS2() references lists some common gotchas:

The total size of the character strings for the filename and all of the
arguments must not exceed 256 bytes. 

Note the following points before calling LoadExecPS2(). 
* Terminate or delete all threads other than the thread that will 
  execute LoadExecPS2(). 
* Cancel all callbacks and interrupt handlers. 
* Execute scePadEnd(). 
* Execute sceSifExitCmd().
*/

bool ScriptExitDemo( Script::CStruct*, Script::CScript* )
{
#ifdef __PLAT_NGPS__
    Sys::ExitDemo();
#endif

	return true;		// actually will never get here
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptIsArray( Script::CStruct* pParams, Script::CScript* pScript )
{
	Script::CArray* pTest = NULL;
	if ( pParams->GetArray( NONAME, &pTest, Script::NO_ASSERT ) )
		return true;
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UseUserSoundtrack | xbox only - uses a certain soundtrack
// @uparm 1 | the soundtrack number
bool ScriptUseUserSoundtrack( Script::CStruct* pParams, Script::CScript* pScript )
{
	Dbg_MsgAssert( Config::GetHardware() == Config::HARDWARE_XBOX, ( "UseUserSoundtrack should only be called on the XBox" ) );
	
	int soundtrack;
	pParams->GetInteger( NONAME, &soundtrack, Script::ASSERT );
	Pcm::UseUserSoundtrack( soundtrack );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UseStandardSoundtrack | xbox only - use the standard soundtrack
bool ScriptUseStandardSoundtrack( Script::CStruct* pParams, Script::CScript* pScript )
{
	Dbg_MsgAssert( Config::GetHardware() == Config::HARDWARE_XBOX, ( "UseStandardSoundtrack should only be called on XBox" ) );
	Pcm::UseStandardSoundtrack();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | DisableReset | ngc only - disable the reset button. If the user presses reset when disabled, a screen will be displayed until it is enabled, and it will reset.
bool ScriptDisableReset( Script::CStruct* pParams, Script::CScript* pScript )
{
#ifdef __PLAT_NGC__
	//printf("DisableReset ...\n");
	NxNgc::EngineGlobals.disableReset = true;
#endif		// __PLAT_NGC__ 

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EnableReset | ngc only - enable the reset button. If reset was pressed while disabled, it will now reset.
bool ScriptEnableReset( Script::CStruct* pParams, Script::CScript* pScript )
{
#ifdef __PLAT_NGC__
	//printf("WARNING! EnableReset ...\n");

	NxNgc::EngineGlobals.disableReset = false;
#endif		// __PLAT_NGC__ 

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetToIPL | ngc only - resets to IPL screen.
bool ScriptResetToIPL( Script::CStruct* pParams, Script::CScript* pScript )
{
#ifdef __PLAT_NGC__
	NxNgc::EngineGlobals.resetToIPL = true;
#endif		// __PLAT_NGC__ 

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptInitAnimCompressTable( Script::CStruct* pParams, Script::CScript* pScript )
{
	const char* pFileName;
	pParams->GetText( NONAME, &pFileName, Script::ASSERT );

	uint32 compressTableName;
	pParams->GetChecksum( NONAME, &compressTableName, Script::ASSERT );

	char filename[256];
	sprintf( filename, "%s", pFileName );//, Nx::CEngine::sGetPlatformExtension() );

	switch ( compressTableName )
	{
		case 0xc6a56fe3:	// q48
		{
			Gfx::InitQ48Table( filename );
		}
		break;
		
		case 0xc06ead08:	// t48
		{
			Gfx::InitT48Table( filename );
		}
		break;

		default:
			Dbg_MsgAssert( 0, ( "Unrecognized compress table name %s", Script::FindChecksumName(compressTableName) ) );
			break;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Test_Composite is a test array of structures
// that are the components of the composite object, and the parameters passed to it
/*
test_composite = [
    {
        component = model
        model = "gameobjects\skate\letter_a\letter_a.mdl"
    }
    
    {
        component = exceptions
    }
    
    {
        component = bouncyphysics
        bounce = 2
    }
   {
    components = [model, exceptions, explodingphysics]
    model = "gameobjects\skate\letter_a\letter_a.mdl"
    bounce = 2
   
} 
]
*/
						
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptCreateCompositeObject( Script::CStruct* pParams, Script::CScript* pScript )
{
	// Required array of structures containing "component="
	Script::CArray*	p_array = NULL;
	pParams->GetArray(CRCD(0x11b70a02,"components"),&p_array,Script::ASSERT);

	// Optional extra structure of parameters that override any provided in the array	
	Script::CStruct* p_struct = NULL;
	pParams->GetStructure(CRCD(0x7031f10c,"params"),&p_struct);
	
	Obj::CCompositeObjectManager::Instance()->CreateCompositeObjectFromNode(p_array, p_struct);
	
	return true;
}	  

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the id of an object, and a structure, 
bool ScriptRefreshObject( Script::CStruct* pParams, Script::CScript* pScript )
{


	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AutoRail |
// auto-generates rails based of the level's collidable geometry
// @parmopt float | min_rail_edge_angle | 30.0 |
// rails are created only on edges as least as sharp as this angle
// @parmopt float | max_rail_angle_of_assent | 45.0 |
// rails are created only on edges less steep than this angle
// @parmopt float | min_railset_length | 36.0 |
// autorail attempts to join contiguous edges together in order to determine the effective length of a curved rail;
// once this is done, rails shorter than this length in inches are dropped
// @parmopt float | min_edge_length | 0.0 |
// edges below this length in inches are not considered for rail creation, even if they could be part of a curved rail
// @parmopt float | max_corner_in_railset | 50.0 |
// maximum angle between two edges for them to be connected as a curved rail
// @parmopt float | connection_slop | 0.1 |
// slop distance allowed between rail endpoints below which those rails will be snapped togeather
// @parmopt float | farthest_degenerate_rail | 12.0 |
// auto-generated rails	within this distance in inches of an old rail will not be created
// @parmopt float | max_degenerate_rail_angle | 20.0 |
// if the angle between an auto-generated rail and a nearly old rail is below this angle, the auto-generated rail will not be created
// @parmopt float | max_low_curb_height | 8.0 |
// curbs below this height in inches are not given rails; controls the low-curb collision feeler length
// @parmopt float | vertical_feeler_length | 60.0 |
// length in inches of the vertical collision feeler
// @parmopt float | crossbar_feeler_length | 12.0 |
// length in inches of the crossbar collision feeler
// @parmopt float | crossbar_feeler_elevation | 12.0 |
// height in inches of the crossbar collision feeler
// @parmopt float | curb_feeler_angle_of_assent | 30.0	|
// low-curb collision feeler radiates from the rail at this angle above the bordering faces
// @parmopt float | feeler_increment | 36.0 |
// collision feelers are used at each step of this distance in inches along the rail
// @flag no_feelers |
// turns off the checking of the volume around rails by collision feelers
// otherwise this checking is done in order to insure that a rail is not cramped or on a low curb
// @flag no_vertical_feeler |
// turns off the collision feeler which radiates upward from the rail
// @flag no_crossbar_feeler |
// turns off the collision feeler which lies just above the rail and runs perpendicular to it
// @flag no_low_curb_feeler |
// turns off the collision feelers which radiate from the rail just above the bordering faces;
// these collision feelers are used to insure rails are not created on low curbs
// @flag overwrite |
// deletes old rails before auto-generating rails
bool ScriptAutoRail( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::Skate::Instance()->GetRailManager()->AutoGenerateRails(pParams);
	return true;			  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptInside(Script::CStruct* pParams, Script::CScript* pScript)
{
	Dbg_MsgAssert(Mdl::Skate::Instance()->mpProximManager->IsInsideFlagValid(), ("Inside called outside of a proxim trigger script or within a proxim trigger script with a ProximObject"));
    return Mdl::Skate::Instance()->mpProximManager->IsInside(); 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptSetSpecialBarColors(Script::CStruct* pParams, Script::CScript* pScript)
{
    int skater_num;
    pParams->GetInteger( "skater", &skater_num, Script::ASSERT );
    
    Mdl::Skate * pSkate = Mdl::Skate::Instance();
    Obj::CSkater* pSkater = pSkate->GetSkater( skater_num );
    Mdl::Score *pScore = pSkater->GetScoreObject();

    pScore->setSpecialBarColors();
    return true;
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	ScriptGetMetrics(Script::CStruct* pParams, Script::CScript* pScript)
{
// get various metrics into a structure for display
// You can run this on a spawned script to see in the viewer

	Script::CStruct *p_metrics = new Script::CStruct();
	Nx::CEngine::sGetMetrics(p_metrics);
	
	Script::CStruct *p_script_params = pScript->GetParams(); 				   
	p_script_params->AddStructure("Metrics",p_metrics);
	
	return true;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	ScriptMoveNode(Script::CStruct* pParams, Script::CScript* pScript)
{
   	// Given a name and a position, move the node to that position
	// This can only be applied to one node at once, as it's an absolute position
	// So we just do everything here.

	uint32	name;															
	if ( ! pParams->GetChecksum(CRCD(0xa1dc81f9,"name"),&name) )
	{
		printf ("Missing Name\n");
		return false;
	}

	int node_num = SkateScript::FindNamedNode( name, false );
	if( node_num == -1 )
	{
		Dbg_Printf( "MoveNode failed. Could not find node %p in the scene\n", name );
		return false;
	}
	Script::CStruct *pNode = SkateScript::GetNode( node_num );
	
	Mth::Vector new_pos, old_pos;
	
  	if ( !pParams->GetVector(CRCD(0xb9d31b0a,"position"),&new_pos)
		 && !pParams->GetVector(CRCD(0x7f261953,"pos"),&new_pos) )
	{
		printf("Missing position parameter in MoveNode\n");
		return false;
	}

	if ( !pNode->GetVector(CRCD(0xb9d31b0a,"position"),&old_pos)
		 && !pNode->GetVector(CRCD(0x7f261953,"pos"),&old_pos) )
	{
		printf("node is missing position\n");
		return false;
	}
		
	// Move the original node 
	// GJ:  i'm pretty sure that the following is incorrect,
	// and that the Z component needs to be negated, but since we don't
	// actually reference the "position" component after the object has
	// been created, the bug has slipped through the cracks...  we're
	// switching over to the new-style "pos" vector anyway
//	pNode->AddVector(CRCD(0xb9d31b0a,"position"),new_pos[X],new_pos[Y],new_pos[Z]);
	pNode->AddVector(CRCD(0x7f261953,"pos"),new_pos[X],new_pos[Y],new_pos[Z]);

	// Now look for the node type, and handle the mode for each type of thing

	Obj::CCompositeObject * p_object = (Obj::CCompositeObject *) Obj::CCompositeObjectManager::Instance()->GetObjectByID(name);

	if (p_object)
	{
		// Might also have to set the pos of the components
		// as they can store internal absolute positions
		// and the pos of the object is only used for display
		p_object->SetPos(new_pos);
	}

	uint32 ClassChecksum = 0;
	pNode->GetChecksum( 0x12b4e660 /*"Class"*/, &ClassChecksum );
	switch (ClassChecksum)
	{
		case 0xe47f1b79: //vehicle
			break;
	
		case 0xa0dfac98:  //pedestrian
		case 0x61a741e:  // ped
//			Obj::CreatePed( skate_mod->GetObjectManager(), nodeIndex );
			break;

		case 0xef59c100:  //gameobject
			break;
			
		case 0x19b1e241: // ParticleEmitter		
			{
				Nx::CParticle * p_particle = Nx::get_particle( name );
				if (p_particle)
				{
					p_particle->Refresh();
				}
			}
			break;

		case 0xb7b3bd86:  // LevelObject
			{
			}
			break;

		case 0xbf4d3536:  // LevelGeometry
		{
			/*Dbg_MsgLog(("LevelGeometry"));
			uint32 checksumName = 0;
			if ( pNode->GetChecksum( 0xa1dc81f9, &checksumName ) ) // checksum 'name'
			{
//				Nx::CSector *p_sector = Nx::CEngine::sGetSector(checksumName);
				// We only want to get sectors from the main scene, as that is what the node array applies to															
				Nx::CScene * p_main_scene = Nx::CEngine::sGetMainScene();
				Nx::CSector *p_sector = p_main_scene->GetSector(checksumName);
				if (p_sector)
				{
					Dbg_MsgAssert(p_sector,("sGetSector(0x%x) returned NULL (%s)",checksumName,Script::FindChecksumName(checksumName)));
					p_sector->SetWorldPosition(new_pos);
				}
				else
				{
					printf(" WARNING: sGetSector(0x%x) returned NULL (%s)\n",checksumName,Script::FindChecksumName(checksumName));
				}
				//Bsp::ClearWorldSectorFlag( checksumName, mSD_KILLED | mSD_NON_COLLIDABLE | mSD_INVISIBLE  | mSD_INVISIBLE2);
			}*/
			break;
 		}

		case CRCC(0x30c19600, "ClimbingNode"):
		case 0x8e6b02ad:  // railnode
			Mdl::Skate::Instance()->GetRailManager()->MoveNode( node_num, new_pos );
			break;			
		case 0x8470f2e:  // proximnode
//			Obj::Proxim_SetActive( nodeIndex, true );
			break;			
		default:
			return ( false );
			break;
	}
	
	return true;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
					 
// @script | SetActiveCamera |
// Sets the active camera to be the camera component of this object
// @parmopt checksum | id | 0 | 
// @parmopt integer | viewport | 0 |
// @flag move |	  move the new camera to the position of the old camera
bool	ScriptSetActiveCamera(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32	id=0;
	pParams->GetChecksum(CRCD(0x40c698af,"id"),&id);
	int	viewport=0;
	pParams->GetInteger(CRCD(0x9fd29151,"viewport"),&viewport);
	bool move = pParams->ContainsFlag(CRCD(0x10c1c887,"move"));

	return SetActiveCamera(id,viewport, move); 	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptSin(Script::CStruct* pParams, Script::CScript* pScript)
{
    float x,sin;
    pParams->GetFloat( NONAME , &x, Script::ASSERT );
    
    sin = sinf(Mth::DegToRad(x));

    Script::CStruct *p_script_params = pScript->GetParams(); 				   
	p_script_params->AddFloat("sin",sin);
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptCos(Script::CStruct* pParams, Script::CScript* pScript)
{
    float x,cos;
    pParams->GetFloat( NONAME , &x, Script::ASSERT );
    
    cos = cosf(Mth::DegToRad(x));

    Script::CStruct *p_script_params = pScript->GetParams(); 				   
	p_script_params->AddFloat("cos",cos);
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptTan(Script::CStruct* pParams, Script::CScript* pScript)
{
    float x,tan;
    pParams->GetFloat( NONAME , &x, Script::ASSERT );
    
    tan = tanf(Mth::DegToRad(x));

    Script::CStruct *p_script_params = pScript->GetParams(); 				   
	p_script_params->AddFloat("tan",tan);
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptASin(Script::CStruct* pParams, Script::CScript* pScript)
{
    float x,asin;
    pParams->GetFloat( NONAME , &x, Script::ASSERT );
    
    asin = asinf(x);
    asin = Mth::RadToDeg(asin);

    Script::CStruct *p_script_params = pScript->GetParams(); 				   
	p_script_params->AddFloat("asin",asin);
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptACos(Script::CStruct* pParams, Script::CScript* pScript)
{
    float x,acos;
    pParams->GetFloat( NONAME , &x, Script::ASSERT );
    
    acos = acosf(x);
    acos = Mth::RadToDeg(acos);

    Script::CStruct *p_script_params = pScript->GetParams(); 				   
	p_script_params->AddFloat("acos",acos);
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptATan(Script::CStruct* pParams, Script::CScript* pScript)
{
    float x,atan;
    pParams->GetFloat( NONAME , &x, Script::ASSERT );
    
    atan = atanf(x);
    atan = Mth::RadToDeg(atan);

    Script::CStruct *p_script_params = pScript->GetParams(); 				   
	p_script_params->AddFloat("atan",atan);
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Load tracking info from a file, and display it as immediate mode lines
bool ScriptShowTracking(Script::CStruct* pParams, Script::CScript* pScript)
{
	Gfx::DebugGfx_CleanUp();
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());

	bool do_bonks = pParams->ContainsFlag(CRCD(0xb426bf5a,"bonks"));														   
	bool do_grinds = pParams->ContainsFlag(CRCD(0x14240297,"grinds"));														   
	bool do_verts = pParams->ContainsFlag(CRCD(0x969d3af6,"verts"));														   
	bool do_lines = pParams->ContainsFlag(CRCD(0xb0fe7369,"lines"));					

	// if nothing specificed, then do everything												  
	if (!do_bonks && !do_grinds && !do_verts && !do_lines)
	{
		do_bonks = do_grinds = do_verts	= do_lines = true;
	}
														   
	const char *p_name="";
	pParams->GetString(NONAME,&p_name);
	if (p_name[1] == ':')
	{
		// skip over any 'c:' the user might be specifying
		// hopefully they don't try to load things off q:, or something
		printf ("WARNING: stripped drive name off file\n");
		p_name+=2;
	}
	int size=0;
	void * p_file;
	if (File::Exist(p_name))
	{
		p_file = File::LoadAlloc(p_name,&size);
	
		printf ("Loaded %s, size = %d, at %p\n",p_name,size,p_file);
	
		Mem::Manager::sHandle().PopContext();
	}
	else
	{
		printf ("Can't find file: %s\n",p_name);
		return false;
	}
	
	int bonks = 0;
	int grinds = 0;				 
	int verts = 0;				 
	int lines = 0;				 
	char *p = (char*)p_file;
	while (p < ((char*)p_file+size))
	{
		if (
			    p[0] == 0x0a
			&&	p[1] == 'T'
			&&	p[2] == 'r'
			&&	p[3] == 'a'
			&&	p[4] == 'c'
			&&	p[5] == 'k'
			&&	p[6] == 'i'
			&&	p[7] == 'n'
			&&	p[8] == 'g'
			)
		{
			int type = p[9] - '0';
			p += 11;
			float f[6];
			for (int i=0;i<6;i++)
			{
				f[i] = (float)atof(p);
				while (*p=='-' || *p=='.' || (*p>='0' && *p<='9'))
				{
					p++;
				}
				p++;  // skip the comma (or half the EOL)
			}
			p--;	// back up!
//			printf ("%d: %f,%f,%f - %f,%f,%f,%f\n",type,f[0],f[1],f[2],f[3],f[4],f[5]);

			Mth::Vector start = Mth::Vector(f[0],f[1],f[2]); 
			Mth::Vector end = Mth::Vector(f[3],f[4],f[5]);
			
			uint32 color = 0xff00ff;
			switch (type)
			{
				case 1:
					bonks++;
					if (!do_bonks) goto sorry;
					color = 0xffffff;	  // white = Bonk
					start -= (end-start)*10;
					break;
				case 2:
					grinds++;
					if (!do_grinds) goto sorry;
					color = 0x0000ff;	  // Red = Grind
					//start -= (end-start)*2;
					break;
				case 3: 
					verts++;
					if (!do_verts) goto sorry;
					color = 0xffff00;	  // Cyan = Vert
					start -= (end-start)*2;
					break;
				case 4: 
					lines++;
					if (!do_lines) goto sorry;
					color = 0x008000;	  // Green = Line
					start -= (end-start)*4;
					break;
				default:
					color = 0xffff;
			}
			Gfx::AddDebugArrow(start, end,color,color,0);
sorry:		
		;	
		}
	
	
		p++;
	}
	
	printf ("Bonks = %d,  Grinds = %d, verts= %d, lines = %d\n",bonks, grinds, verts, lines);
	
	Mem::Free(p_file);	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsGrind | 
bool ScriptIsGrind(Script::CStruct* pParams, Script::CScript* pScript)
{
	return pParams->ContainsFlag(CRCD(0x255ed86f, "Grind"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetColorBufferClear | 
// @parmopt integer | clear | 0 |
bool ScriptSetColorBufferClear(Script::CStruct* pParams, Script::CScript* pScript)
{
	int	clear = 0;
	pParams->GetInteger( CRCD( 0x1a4e0ef9, "clear" ), &clear );

	Nx::CEngine::sSetColorBufferClear( clear > 0 );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ShowCamOffset |
// @parm name | Name | The name of the node we want to get the offset from
// Printfs the cam offset in EE0
bool ScriptShowCamOffset(Script::CStruct* pParams, Script::CScript* pScript)
{
	#ifdef	__NOPT_ASSERT__
	// Given a node name
	// then calculate a good targetOffset and positionOffset
	// so we can mode the camera into that position later

	// Some common code to either relative, or world specific
	Mth::Matrix cam_matrix = Nx::CViewportManager::sGetActiveCamera()->GetMatrix();
	Mth::Vector cam_pos = Nx::CViewportManager::sGetActiveCamera()->GetPos();			

	// get a line segment extending fromthe camera forward (note -z)
	Mth::Line cam_fwd;
	cam_fwd.m_start = cam_pos;
	cam_fwd.m_end = cam_pos - cam_matrix[Z] * 10000;

	
	uint32 name = 0;
	if (pParams->GetChecksum(CRCD(0xa1dc81f9,"name"),&name))
	{
		
		if (name != CRCD(0xc588eebc,"world"))
		{
			// get the node from the node array, and get the position
			int node = SkateScript::FindNamedNode(name);
			if (node != -1)
			{
				Script::CStruct *pNode = SkateScript::GetNode(node);
				Mth::Vector	pos;
				pNode->GetVector(CRCD(0x7f261953,"pos"),&pos);
	
	
				Mth::Matrix cam_matrix = Nx::CViewportManager::sGetActiveCamera()->GetMatrix();
				Mth::Vector cam_pos = Nx::CViewportManager::sGetActiveCamera()->GetPos();			
	
	
				// get a line segment extending fromthe camera forward (note -z)
				Mth::Line cam_fwd;
				cam_fwd.m_start = cam_pos;
				cam_fwd.m_end = cam_pos - cam_matrix[Z] * 10000;
				
				Mth::Line node_up;
				node_up.m_start = pos;
				node_up.m_end = pos + Mth::Vector(0,10000,0);
	
				Mth::Vector pa, pb;			
				float mua,mub;
				if (Mth::LineLineIntersect( cam_fwd, node_up, &pa, &pb, &mua, &mub, false ))
				{
					Gfx::AddDebugArrow(pos, pa,0xff0000,0,100);			
	//				Gfx::AddDebugArrow(pa, pb ,0xff00  ,0,100);			
				}
				else
				{
					printf ("Can't calculate intersection, move camera a bit\n");
				}			
				
				//Need to find the offset from this point to the line of sight of the camera
				// we have  cam_pos = position of camera
				//          pos     = position of node
				//          pa      = position of the focus
				
				Mth::Vector target_offset = pa-pos;
				Mth::Vector position_offset = cam_pos - pa;
				
				/*
				printf ("\nUnrotated - Use if focussing relative to the ground\n");																									 
				printf ("		targetOffset=(%.1f, %.1f, %.1f)\n",target_offset[X],target_offset[Y],target_offset[Z]);
				printf ("		positionOffset=(%.1f, %.1f, %.1f)\n",position_offset[X],position_offset[Y],position_offset[Z]);
				*/
				
				// Now we have to transform it by the inverse fo the matrix representing the orientation fo the node
				Obj::CCompositeObject *p_obj = (Obj::CCompositeObject *)Obj::CCompositeObjectManager::Instance()->GetObjectByID(name);
				if (!p_obj)
				{
					printf ("Can't find the oject this node represents\n");
					return false;
				}
				// Get the matrix
				Mth::Matrix obj_mat = p_obj->GetMatrix();
				// Invert it, as we want to do the opposite of what we will do with the actual values
				obj_mat.Invert();
				// apply this, to get the values
				target_offset = obj_mat.TransformAsPos(target_offset);
				position_offset = obj_mat.TransformAsPos(position_offset);
				
//				printf ("\nRotated (Use if focussing relative to the object's orientation)\n");																									 
				printf ("		targetid=%s\n",Script::FindChecksumName(name)); 
				printf ("		targetOffset=(%.1f, %.1f, %.1f)\n",target_offset[X],target_offset[Y],target_offset[Z]);
				printf ("		positionOffset=(%.1f, %.1f, %.1f)\n",position_offset[X],position_offset[Y],position_offset[Z]);
	
				return true;
			}			
		}
	}
	
	// Looks like we want world coordinates, so print up targetOffset as a point 100 feet in front of the camera
	// and positionoffset as the actual point of the camera
	printf ("		targetid=world\n"); 
	printf ("		targetOffset=(%.1f, %.1f, %.1f)\n",cam_fwd.m_end[X],cam_fwd.m_end[Y],cam_fwd.m_end[Z]);
	printf ("		positionOffset=(%.1f, %.1f, %.1f)\n",cam_pos[X],cam_pos[Y],cam_pos[Z]);
	
	
	
	#endif
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


// @script | SetEventHandler | register this script to listen for the given event and gosub to the given script in response to the event
// @parm name | Ex | the name of the event listened for
// @parm script | Scr | the script to be run in response to the event
// @parmopt name | Group | the name of this event listeners group; used to identify which listeners to clear when clearing listener groups
// @parmopt structure | Params | this structure is added to the parameter list of the script
// @parmopt flag | exception | set if this causes an exception, rather than a goto
bool ScriptSetEventHandler(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 ex = 0;
	pParams->GetChecksum(CRCD(0xf8728bec,"Ex"), &ex, true);
	uint32 scr = 0;
	pParams->GetChecksum(CRCD(0xa6d2d890,"Scr"), &scr, true);
	uint32 group = CRCD(0x1ca1ff20,"default");
	pParams->GetChecksum(CRCD(0x923fbb3a,"group"), &group);	
	bool exception = pParams->ContainsFlag(CRCD(0x80367192,"exception"));
	Script::CStruct *pExtraParams=NULL;
	pParams->GetStructure(CRCD(0x7031f10c,"params"),&pExtraParams);
	pScript->SetEventHandler(ex,scr,group,exception,pExtraParams);	
	return true;
}

bool ScriptClearEventHandler(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 type;
	pParams->GetChecksum(NO_NAME, &type, Script::ASSERT);
	pScript->RemoveEventHandler(type);
	return true;
}
			
bool ScriptClearEventHandlerGroup(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 group = Obj::CEventHandlerTable::vDEFAULT_GROUP;
	pParams->GetChecksum(NO_NAME, &group);
	pScript->RemoveEventHandlerGroup(group);
	return true;
}
		
// @script | OnExceptionRun | run the specified script on exception
// @uparm name | script name to run
// can be called without a parameter to clear it
bool ScriptOnExceptionRun(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 OnExceptionScriptChecksum = 0;
	pParams->GetChecksum( NONAME, &OnExceptionScriptChecksum);
	pScript->SetOnExceptionScriptChecksum(OnExceptionScriptChecksum);
	return true;
}

bool ScriptOnExitRun(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 OnExitScriptChecksum = 0;
	pParams->GetChecksum( NONAME, &OnExitScriptChecksum);
	pScript->SetOnExitScriptChecksum(OnExitScriptChecksum);
	return true;
}


// @script | Block | Stop execution of this script, allowing to still be woken up by events and exceptions
bool ScriptBlock(Script::CStruct* pParams, Script::CScript* pScript)
{
	pScript->Block();
	return true;
}

// @script | PrintEventHandlerTable | Prints the event handler table of the current script
bool ScriptPrintEventHandlerTable ( Script::CStruct* pParams, Script::CScript* pScript )
{
	pScript->PrintEventHandlerTable();
	return true;
}


// @script | AllocateSplitScreenDMA | (PS2 only) allocated extra memory for DMA usage
bool ScriptAllocateSplitScreenDMA ( Script::CStruct* pParams, Script::CScript* pScript )
{
	#ifdef	__PLAT_NGPS__
	
	Nx::CEngine::sFinishRendering();
 	NxPs2::AllocateExtraDMA(true);

	#endif
	
	return true;
}



// @script | AddSkaterEarly | Create and load a skater, so when we autolaunch a level, it's there.  DEBUG ONLY
bool ScriptAddSkaterEarly( Script::CStruct* pParams, Script::CScript* pScript )
{

	if (Mdl::Skate::Instance()->GetNumSkaters() == 0)
	{
		Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile(0);
		Mdl::Skate::Instance()->add_skater( pSkaterProfile, true, 0, 0);
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptPreLoadStreamDone( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 streamId;
	pParams->GetChecksum( NONAME, &streamId, Script::ASSERT );
	return Pcm::PreLoadStreamDone( streamId );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptStartPreLoadedStream( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 streamId;
	pParams->GetChecksum( CRCD(0x8a68ab90,"streamId"), &streamId, Script::ASSERT );
	float vol = 100;
	pParams->GetFloat( CRCD(0x46653221,"volume"), &vol, Script::NO_ASSERT );
	Sfx::sVolume* p_volume = new Sfx::sVolume();
	p_volume->SetChannelVolume( 0, vol );
	p_volume->SetChannelVolume( 1, vol );
	bool rc = Pcm::StartPreLoadedStream( streamId, p_volume );
	delete p_volume;
	return rc;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptFinishRendering( Script::CStruct* pParams, Script::CScript* pScript )
{
	Nx::CEngine::sFinishRendering();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace CFuncs
