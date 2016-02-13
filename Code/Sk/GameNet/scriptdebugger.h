#ifndef __GAMENET_SCRIPTDEBUGGER_H__
#define	__GAMENET_SCRIPTDEBUGGER_H__

#include <core/defines.h>

namespace GameNet
{

#define	vSERVER_IP_VARIABLE	"VIEWER_IP"

enum
{
	vSCRIPT_DEBUG_PORT = 10050, // Needs to be distinct from vEXPORT_COMM_PORT or any others
};	

enum
{
	vSCRIPT_DEBUG_GROUP = 10
};

enum
{	
	//	32 is the first available user-defined message id
	vMSG_ID_DBG_PACKET							= 32, // PC -> Game
	
	vMSG_ID_DBG_CHECKSUM_NAMES_BUFFER_SIZE		= 33, // Game -> PC
	vMSG_ID_DBG_CHECKSUM_NAMES					= 34, // Game -> PC
	vMSG_ID_DBG_CSCRIPT_LIST					= 35, // Game -> PC
	vMSG_ID_DBG_WATCH_THIS						= 36, // PC -> Game
	vMSG_ID_DBG_WATCH_SCRIPT					= 37, // PC -> Game
	vMSG_ID_DBG_SCRIPT_CREATED					= 38, // Game -> PC
	vMSG_ID_DBG_SCRIPT_DIED						= 39, // Game -> PC
	vMSG_ID_DBG_CSCRIPT_INFO					= 40, // Game -> PC
	
	vMSG_ID_DBG_COMPRESSION_LOOKUP_TABLE		= 41, // Game -> PC
	vMSG_ID_DBG_BASIC_CSCRIPT_INFO				= 42, // Game -> PC
	vMSG_ID_DBG_STOP							= 43, // PC -> Game
	vMSG_ID_DBG_STEP_INTO						= 44, // PC -> Game
	vMSG_ID_DBG_STEP_OVER						= 45, // PC -> Game
	vMSG_ID_DBG_GO								= 46, // PC -> Game
	vMSG_ID_DBG_SCRIPT_UPDATE_DELAY				= 47, // PC -> Game
	vMSG_ID_DBG_CLEAR_SCRIPT_WATCHES			= 48, // PC -> Game
	vMSG_ID_DBG_REFRESH							= 49, // PC -> Game
	vMSG_ID_DBG_SEND_CSCRIPT_LIST				= 50, // PC -> Game
	
	vMSG_ID_DBG_SEND_WATCH_LIST					= 51, // PC -> Game
	vMSG_ID_DBG_WATCH_LIST						= 52, // Game -> PC
	vMSG_ID_DBG_STOP_WATCHING_THIS_INDEX		= 53, // PC -> Game
	vMSG_ID_DBG_STOP_WATCHING_THIS_CSCRIPT		= 54, // PC -> Game
	vMSG_ID_DBG_SEND_COMPOSITE_OBJECT_INFO		= 55, // PC -> Game
	vMSG_ID_DBG_COMPOSITE_OBJECT_INFO			= 56, // Game -> PC
	
	vMSG_ID_DBG_MOUSE_POSITION					= 57, // PC -> Game
	vMSG_ID_DBG_MOUSE_ON_SCREEN					= 58, // PC -> Game
	vMSG_ID_DBG_MOUSE_OFF_SCREEN				= 59, // PC -> Game
	vMSG_ID_DBG_MOUSE_LEFT_BUTTON_DOWN			= 60, // PC -> Game
	vMSG_ID_DBG_MOUSE_LEFT_BUTTON_UP			= 61, // PC -> Game
	vMSG_ID_DBG_MOUSE_RIGHT_BUTTON_DOWN			= 62, // PC -> Game
	vMSG_ID_DBG_MOUSE_RIGHT_BUTTON_UP			= 63, // PC -> Game
	
	vMSG_ID_DBG_COMPOSITE_OBJECT_LIST			= 64, // Game -> PC
	vMSG_ID_DBG_SEND_COMPOSITE_OBJECT_LIST		= 65, // PC -> Game
	vMSG_ID_DBG_SET_OBJECT_UPDATE_MODE			= 66, // PC -> Game
	vMSG_ID_DBG_VIEW_OBJECT						= 67, // PC -> Game
	
	vMSG_ID_DBG_SEND_SCRIPT_GLOBAL_INFO			= 68, // PC -> Game
	vMSG_ID_DBG_SCRIPT_GLOBAL_INFO				= 69, // Game -> PC
	
	vMSG_ID_DBG_EXCEPTION_INFO					= 70, // Game -> PC
	vMSG_ID_DBG_RUNNING_SCRIPT					= 71, // Game -> PC
};

}
#endif	// __GAMENET_SCRIPTDEBUGGER_H__