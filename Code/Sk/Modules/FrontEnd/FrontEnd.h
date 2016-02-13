/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:																**
**																			**
**	Module:															        **
**																			**
**	File name:												                **
**																			**
**	Created: 		09/12/2000	-	rjm										**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __FRONTEND_H
#define __FRONTEND_H

#include <core/support.h>
#include <core/task.h>
#include <core/string/cstring.h>

#include <gel/module.h>
#include <gel/inpman.h>

#include <gfx/camera.h>
#include <gfx/image/imagebasic.h>

#include <sys/timer.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Script
{
	class CStruct;
	class CScript;
}

namespace Mdl
{



/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class FrontEnd;

class FrontEnd : public  Module
{
	
	DeclareSingletonClass( FrontEnd );

public:
	enum
	{ 
		vMAIN_MENU 			= 0x6249f70e, // "main_menu"
		vGAME_MENU			= 0x69efa940, // "game_menu"
		vPARKED_MENU		= 0xab42b78b, // "parked_menu"
	};
	
	enum EControllerMode
	{
		INACTIVE,
		ACTIVE,
		MAP_TO_FIRST,
	};

	
	void 							PauseGame(bool pause);
	bool 							GamePaused() {return m_paused;}
	// This function is used to temporarily switch the paused task list on and off. You should reset it to its previous
	// state afterwards. Use SetActive() whenever you can.
	void 							SetPausedTasks(bool is_active);

	bool							PadsPluggedIn();
	void 							UseFirstPadForPausedMenu() {m_use_second_pad_for_paused_menu=false; m_using_one_pad_for_paused_menu=true;}
	void 							UseSecondPadForPausedMenu() {m_use_second_pad_for_paused_menu=true; m_using_one_pad_for_paused_menu=true;}

	void 							SetAutoRepeatTimes(Tmr::Time kickoff, Tmr::Time repeat) {m_auto_repeat_time[0] = kickoff; m_auto_repeat_time[1] = repeat;}
	void							GetAutoRepeatTimes(Tmr::Time &kickoff, Tmr::Time &repeat) {kickoff = m_auto_repeat_time[0]; repeat = m_auto_repeat_time[1];}
	void 							SetControllerMode(int controllerNum, EControllerMode mode) {m_pad_info[controllerNum].mMode = mode;}
	void 							SetControllerTempBlock(bool tempBlock) {m_temp_block_pad_input = tempBlock;}

	void							AddEntriesToEventButtonMap(Script::CStruct *pParams);

	void 							AddKeyboardHandler( int max_string_length );
	void 							RemoveKeyboardHandler( void );
	void 							SetAnalogStickActiveForMenus( bool isActive );

private:

	FrontEnd( void );
	virtual							~FrontEnd( void );

	void 							update(bool game_is_paused);
	uint32							grab_timed_event_type(uint mask, int index);
	uint32							turn_mask_into_event_type(uint mask, int count, int index, uint buttons);
	
	enum Commands 
	{
        mSELECTUP   			= nBit(0),
        mSELECTDOWN 			= nBit(1),
        mXDOWN 					= nBit(2),
		mTRIANGLEDOWN			= nBit(3),
		mSTARTDOWN				= nBit(4),
		mTOGGLE					= nBit(5),
		mSELECTLEFT				= nBit(6),
		mSELECTRIGHT			= nBit(7),
	};

	static	Tsk::Task< FrontEnd >::Code		s_logic_code;
	static	Tsk::Task< FrontEnd >::Code		s_handle_keyboard_code;
	static	Tsk::Task< FrontEnd >::Code		s_display_code;
	static	Inp::Handler< FrontEnd >::Code  s_input_logic_code;

	void							v_start_cb( void );
	void							v_stop_cb( void );
    	
	Tsk::Task< FrontEnd >*			m_logic_task;	
	Tsk::Task< FrontEnd >*			m_handle_keyboard_task;
	bool							m_use_second_pad_for_paused_menu;
	bool							m_using_one_pad_for_paused_menu;
	
   	Inp::Handler< FrontEnd >*		mp_input_handlers[SIO::vMAX_DEVICES];
public:
	// Used by ScriptAnyControllerPressed in cfuncs.cpp
	Inp::Handler< FrontEnd >*		GetInputHandler( int index ) { return mp_input_handlers[index]; }
	void							UpdateInputHandlerMappings();
	int								GetInputHandlerMapping( int device_num );

	int								m_device_server_map[SIO::vMAX_DEVICES];

private:

	bool							m_paused;
	bool							m_allowDeactivate;

	// first is "kickoff" time, second is "repeat" time
	Tmr::Time						m_auto_repeat_time[2];
	int								m_max_kb_string_length;

	enum EPadState
	{
		NOT_DOWN,
		SLOW_REPEAT,
		FAST_REPEAT,
	};
	
	struct PadInfo
	{
		EPadState					mState;
		Tmr::Time					mAutorepeatCountdown;
		EControllerMode				mMode;
	};
	
	PadInfo							m_pad_info[SIO::vMAX_DEVICES];
	bool							m_temp_block_pad_input;

	enum 
	{
		MAX_BUTTON_EVENT_MAP_ENTRIES	= 32,
		// use in mEventType
		DEAD_ENTRY						= 0,
	};
	
	struct EventMapEntry
	{
		uint						mDigitalButtonIndex;
		uint32						mEventType;
	};
	
	EventMapEntry					m_digital_button_event_map[32];
	
	bool							m_check_for_auto_input;
	bool							m_using_auto_input;
	uint32							m_auto_input_script_id;
};

bool ScriptLaunchMenuScreen(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetMenuAutoRepeatTimes(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetMenuPadMappings(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetButtonEventMappings(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetAnalogStickActiveForMenus(Script::CStruct *pParams, Script::CScript *pScript);

} // namespace FrontEnd

#endif


