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
**	Module:			 			                              				**
**																			**
**	File name:											                    **
**																			**
**	Created by:	    rjm, 9/12/2000											**
**																			**
**	Description:									                        **
**																			**
*****************************************************************************/

// start autoduck documentation
// @DOC frontend
// @module frontend | None
// @subindex Scripting Database
// @index script | frontend
                                    

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/singleton.h>
#include <core/support.h>
#include <core/task.h>

#include <gfx/gfxman.h>
#include <gfx/camera.h>

#include <gel/inpman.h>
#include <gel/module.h>
#include <gel/music/music.h>
#include <gel/soundfx/soundfx.h>
#include <gel/net/net.h>
#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>
#include <gel/mainloop.h>
#include <gel/objtrack.h>
#include <gel/object/compositeobjectmanager.h>

#include <modules/FrontEnd/FrontEnd.h>
#include <sk/modules/skate/skate.h>

#include <objects/skaterprofile.h>

#include <gel/scripting/array.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <scripting/cfuncs.h>

#include <sk/gamenet/gamenet.h>
#include <sys/sio/keyboard.h>
#include <sys/profiler.h>
#include <sys/File/PRE.h>
#include <sys/replay/replay.h>


#include <gfx/2D/ScreenElemMan.h>

#include <sk/objects/skater.h>		   // just for pausing the skater, and for running the "run me now" script

										   
extern bool run_runmenow;

namespace Mdl
{
DefineSingletonClass( FrontEnd, "Frontend module" );


FrontEnd::FrontEnd()
{

	m_logic_task = new Tsk::Task< FrontEnd > ( FrontEnd::s_logic_code, *this, Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_FRONTEND);
	m_handle_keyboard_task = new Tsk::Task< FrontEnd > ( FrontEnd::s_handle_keyboard_code, *this, Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_HANDLE_KEYBOARD);
	
	for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		mp_input_handlers[i] = new Inp::Handler< FrontEnd > ( i, FrontEnd::s_input_logic_code, *this, Tsk::BaseTask::Node::vHANDLER_PRIORITY_FRONTEND_INPUT_LOGIC0);		
		m_pad_info[i].mState =  NOT_DOWN;
		m_pad_info[i].mMode = ACTIVE;
	}
	m_paused = false;

	m_auto_repeat_time[0] = Tmr::vRESOLUTION * 3 / 10;
	m_auto_repeat_time[1] = Tmr::vRESOLUTION * 5 / 100;
	
	
	// m_pad_info[1].mState =  NOT_DOWN;
	
	// m_pad_info[1].mMode = INACTIVE;

	m_temp_block_pad_input = false;

	m_using_auto_input = false;
	m_check_for_auto_input = true;
	m_auto_input_script_id = 0;

	for (int i = 0; i < MAX_BUTTON_EVENT_MAP_ENTRIES; i++)
	{
		m_digital_button_event_map[i].mEventType = DEAD_ENTRY;
	}

	for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
		m_device_server_map[i] = i;
}




FrontEnd::~FrontEnd()
{
	delete m_logic_task;
	
	for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		delete mp_input_handlers[i];
		mp_input_handlers[i] = NULL;
	}
}




void FrontEnd::v_start_cb ( void )
{

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		Mlp::Manager * mlp_manager = Mlp::Manager::Instance(); 
		mlp_manager->AddLogicTask ( *m_logic_task );
		Inp::Manager * inp_manager = Inp::Manager::Instance(); 
		
		for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
		{
			Inp::Handler< FrontEnd >* p_handler = GetInputHandler( i );
			inp_manager->AddHandler( *p_handler );
		}
	Mem::Manager::sHandle().PopContext();	
}




void FrontEnd::v_stop_cb ( void )
{
	m_logic_task->Remove();
	
	for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		Inp::Handler< FrontEnd >* p_handler = GetInputHandler( i );
		p_handler->Remove();
	}
}


bool FrontEnd::PadsPluggedIn()
{
	for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		Dbg_MsgAssert(mp_input_handlers[i],("NULL mp_input_handlers[%d]",i));
		if (mp_input_handlers[i]->m_Device->IsPluggedIn())
		{
			return true;
		}
	}
	return false;		
}



void FrontEnd::AddKeyboardHandler( int max_string_length )
{
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance(); 

	m_max_kb_string_length = max_string_length;
	mlp_manager->AddLogicTask( *m_handle_keyboard_task );
}




void FrontEnd::RemoveKeyboardHandler( void )
{
	m_handle_keyboard_task->Remove();
}




void FrontEnd::SetAnalogStickActiveForMenus( bool isActive )
{
	Spt::SingletonPtr<Inp::Manager> p_manager;
	p_manager->SetAnalogStickOverride(isActive);
}




///////////////////////////////////////////
// (Mick takes responsibility for this)
// This function JUST pauses the game
// this mainly consists of pausing all the tasks (for objects, cars, peds), but
// first it pauses the skate module (for music), and the sio_manager (pad vibration)
//
// The pausing of stuff is fairly comvoluted, here we:
//
// Pause the PCM music
// pause the pad vibration
// pause all messages, but allow future messages to be launched
// pause all spawned scripts, apart from the current one, and future spawned scritps 
//
// pause the skater, but still allow the reading of the pad from script 
//
// Overall, a rather mixed bag of things, which might be best refactored
// but should do for now.....
//
// Note:  is_active == true ... implies the "Front End" is active, and the "game" paused

void FrontEnd::PauseGame(bool paused)
{
	
	// if already in the correct "paused" state, then just return
	if (paused == m_paused)	
	{
		return;
	}
				
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	if (paused)
	{
		// make game be paused
		skate_mod->Pause( );
		
		// Ken: Pause the controller vibrations.
		SIO::Manager * sio_man = SIO::Manager::Instance();
		sio_man->Pause();
	}
	else
	{
		// make game be unpaused
		// Ken: Un-pause the controller vibrations.
		SIO::Manager * sio_man = SIO::Manager::Instance();
		sio_man->UnPause();
	}
	
	// Pause currently spawned scripts (any script with PauseGame in it will unpause itself)		
	Script::PauseSpawnedScripts(paused);		
	
	// Pause or unpause all the skaters
	uint32 NumSkaters = skate_mod->GetNumSkaters();
	
	for (uint32 i = 0; i < NumSkaters; ++i)
	{
		Obj::CSkater *pSkater = skate_mod->GetSkater(i);
		if (pSkater) // Hmm, assert instead?
		{
			if (paused)
			{
				pSkater->Pause();
			}
			else
			{
				pSkater->UnPause();
			}
		}	
	}	
	
	// Pause all composite objects
	Obj::CCompositeObjectManager::Instance()->Pause(paused);
	

	// pause (or unpause) any screen elements in process of morphing
	Front::CScreenElementManager* p_screen_element_manager = Front::CScreenElementManager::Instance();
	p_screen_element_manager->SetPausedState(paused);
	
	m_paused = paused;

	
	if (m_paused)
	{
		// make game be paused
		Mlp::Manager * mlp_manager = Mlp::Manager::Instance(); 
		mlp_manager->SetLogicMask(Mlp::Manager::mGAME_OBJECTS | Mlp::Manager::mGFX_MANAGER);
		Tmr::StoreTimerInfo( );
	}
	else
	{
		// make game be unpaused
		Mlp::Manager * mlp_manager = Mlp::Manager::Instance(); 
		mlp_manager->SetLogicMask(Mlp::Manager::mNONE);		
//		rw_viewer->DisableMainLogic(false);
		// unpause music and soundfx:
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		skate_mod->Pause( false );
		Tmr::RecallTimerInfo( );
	}
}




void FrontEnd::AddEntriesToEventButtonMap(Script::CStruct *pParams)
{
	#ifdef __PLAT_XBOX__
	const char *p_array_name = "xbox";
	#else
	#ifdef __PLAT_NGC__
	const char *p_array_name = "gamecube";
	#else
	const char *p_array_name = "ps2";
	#endif
	#endif
	
	// This seems a very hacky way of doing things.
	// Black and white need to be in position 16 and 17, since they are button indices 16 and 17.
	#ifdef __PLAT_XBOX__
	const char *p_button_names[] =
	{
		"left_trigger_full",
		"right_trigger_full",
		"left_trigger_half",
		"right_trigger_half",
		"y",
		"b",
		"a",
		"x",
		"back",
		"left_stick_button",
		"right_stick_button",
		"null",
		"null",
		"null",
		"null",
		"null",
		"black",
		"white"
	};
	#else
	#ifdef __PLAT_NGC__
	const char *p_button_names[] =
	{
		"left_trigger_full",
		"right_trigger_full",
		"left_trigger_half",
		"right_trigger_half",
		"y",
		"x",
		"a",
		"b",
		"null",
		"z_plus_left_trigger",
		"z_plus_right_trigger",
		"null",
		"null",
		"null",
		"null",
		"null",
		"null",
		"null",
		"z"
	};
	#else
	const char *p_button_names[] =
	{
		"left_trigger2",
		"right_trigger2",
		"left_trigger1",
		"right_trigger1",
		"triangle",
		"circle",
		"x",
		"square",
		"select",
		"left_stick_button",
		"right_stick_button"
	};
	#endif
	#endif
	
	// Cleaner way of allowing varying button names array per platform.
	int num_mapped_buttons = sizeof( p_button_names ) / sizeof( char* );

	uint32 null_crc = Script::GenerateCRC("null");
	
	/*
		Indices:
		
		i: into script array
		j: into button name array
		x: into map array
	*/
	
	Script::CArray *p_map;
	if (!pParams->GetArray(p_array_name, &p_map))
		return;

	for (uint i = 0; i < p_map->GetSize(); i++)
	{
		Script::CArray *p_entry = p_map->GetArray(i);

		uint32 button_crc = p_entry->GetChecksum(0);
		uint32 event_crc = p_entry->GetChecksum(1);

		for (uint j = 0; j < (uint) num_mapped_buttons; j++)
		{
			if (button_crc == Script::GenerateCRC(p_button_names[j]) || button_crc == null_crc)
			{
				#ifdef	__NOPT_ASSERT__
				bool removing_entries = (button_crc == null_crc || event_crc == null_crc);
				#endif
				int x = 0;
				for (; x < MAX_BUTTON_EVENT_MAP_ENTRIES; x++)
				{
					if (button_crc == null_crc && m_digital_button_event_map[x].mEventType == event_crc)
					{
						// in this case, I am removing existing entry (or entries)
						m_digital_button_event_map[x].mEventType = DEAD_ENTRY;
					}
					else if (event_crc == null_crc && m_digital_button_event_map[x].mDigitalButtonIndex == j)
					{
						// in this case, I am removing existing entry (or entries)
						m_digital_button_event_map[x].mEventType = DEAD_ENTRY;
					}
					else if (m_digital_button_event_map[x].mEventType == DEAD_ENTRY)
					{
						// in this case, I am adding a new entry, so need empty slot
						m_digital_button_event_map[x].mDigitalButtonIndex = j;
						m_digital_button_event_map[x].mEventType = event_crc;
						break;
					}
				}
				Dbg_MsgAssert(x < MAX_BUTTON_EVENT_MAP_ENTRIES || removing_entries, ("out of entries, increase size of MAX_BUTTON_EVENT_MAP_ENTRIES"));
				
				break;
			}
		}
	}


	/*
		Paul:
	
		'Y' = PS2 'Triangle'. 
		'X' = PS2 'Circle'.   
		'A' = PS2 'X'.        
		'B' = PS2 'Square'.   
		'Left Trigger pressed down > halfway' = PS2 'L1'. 
		'Right Trigger pressed down > halfway' = PS2 'R1'.
		'Left Trigger' = PS2 'L2'.                        
		'Right Trigger' = PS2 'R2'.                       
		'Z' plus 'L' = PS2 'Left Stick Button'.  
		'Z' plus 'R' = PS2 'Right Stick Button'
		Z button without L & R is PS2 'Select'
	*/

	/*
		Dave:
	
		Xbox start = start
		Xbox back = select
		The 4 xbox color buttons map to the Ps2 buttons in the same location.
		Black and white buttons aren't mapped at all.
		Triggers as per Ngc.
		Left and right stick buttons map directly to Ps2 equivalents
	*/
}




void FrontEnd::s_input_logic_code ( const Inp::Handler < FrontEnd >& handler )
{	   
	Dbg_AssertType ( &handler, Inp::Handler< FrontEnd > );
    
	FrontEnd& mdl = handler.GetData();

	Obj::CTracker* p_tracker = Obj::CTracker::Instance();
	p_tracker->LogTick();
	
	for (int count = 0;; count++)
	{
		uint32 event_type = 0; 
		
		if (count == 0)
		{
			if (mdl.m_check_for_auto_input)
			{
				mdl.m_using_auto_input = (bool) Script::GetInteger("use_auto_menu_input");
				if (mdl.m_using_auto_input)
				{
					Script::CScript *p_script = Script::SpawnScript("auto_menu_input");
					#ifdef __NOPT_ASSERT__
					p_script->SetCommentString("Spawned from FrontEnd::s_input_logic_code");
					#endif
					mdl.m_auto_input_script_id = p_script->GetUniqueId();
				}
				mdl.m_check_for_auto_input = false;
			}
			
			uint32 timed_event_type = mdl.grab_timed_event_type(handler.m_Input->m_Buttons, handler.m_Index);
			if (timed_event_type)
				event_type = timed_event_type;
		}
		
		uint32 mapped_event_type = mdl.turn_mask_into_event_type(handler.m_Input->m_Makes, count, handler.m_Index,handler.m_Input->m_Buttons);
		if (mapped_event_type)
			event_type = mapped_event_type;
		
		if (event_type && mdl.m_pad_info[handler.m_Index].mMode != INACTIVE && !mdl.m_temp_block_pad_input) 
		{
			Script::CStruct event_data;
			int device_num = handler.m_Device->GetPort();
			event_data.AddInteger("device_num", device_num );
			// printf("device_num = %i\n", device_num);

			if (mdl.m_pad_info[handler.m_Index].mMode == ACTIVE)
			{
				// get the controller this came from				
				// event_data.AddInteger("controller", handler.m_Index);
				event_data.AddInteger("controller", 0);
				// event_data.AddInteger( "controller", Mdl::FrontEnd::Instance()->GetInputHandlerMapping( device_num ) );
				// printf( "using controller %i\n", Mdl::FrontEnd::Instance()->GetInputHandlerMapping( device_num ) );
			}
			else
			{
				// printf("using 0\n");
				event_data.AddInteger("controller", 0);
			}
			
			//if (handler.m_Index)
			// printf("firing pad event %s\n", Script::FindChecksumName(event_type));
	
			if (p_tracker->LaunchEvent(event_type, Obj::CEvent::vSYSTEM_EVENT, Obj::CEvent::vSYSTEM_EVENT, &event_data))
			{
				// will get here whenever menu systems responds to one of above occurences
	
				//handler.m_Input->MaskDigitalInput(Inp::Data::mD_ALL);
				//handler.m_Input->MaskAnalogInput(Inp::Data::mA_ALL);
			}
		}

		if (event_type == 0)
			break;
	}
}




void FrontEnd::s_logic_code ( const Tsk::Task< FrontEnd >& task )
{
	Dbg_AssertType ( &task, Tsk::Task< FrontEnd > );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());

	FrontEnd&	mdl = task.GetData();

	mdl.update(mdl.m_paused);
	
	
#		ifdef __USE_PROFILER__
		Sys::CPUProfiler->PushContext( 0, 0, 128 ); 	// blue under green = screen element update
#		endif // __USE_PROFILER__
	
	
	Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();
	p_screen_elem_man->Update();



#		ifdef __USE_PROFILER__
		Sys::CPUProfiler->PopContext(  ); 	
#		endif // __USE_PROFILER__
	
	Mem::Manager::sHandle().PopContext();		
}


void FrontEnd::s_handle_keyboard_code( const Tsk::Task< FrontEnd >& task )
{
	int i, num_chars;
	char makes[256];
	FrontEnd& mdl = task.GetData();
		
	num_chars = SIO::KeyboardRead( makes );

	for( i = 0; i < num_chars; i++ )
	{
		if((( makes[i] >= 32 ) && ( makes[i] <= 126 ) ) || ( makes[i] == SIO::vKB_ENTER ) || ( makes[i] == SIO::vKB_BACKSPACE ))
		{
			Script::CStruct* pParams;
			
			pParams = new Script::CStruct;
			if( makes[i] == SIO::vKB_ENTER )
			{
				pParams->AddChecksum( NONAME, Script::GenerateCRC( "got_enter" ));
			}
			else if( makes[i] == SIO::vKB_BACKSPACE )
			{
				pParams->AddChecksum( NONAME, Script::GenerateCRC( "got_backspace" ));
			}
			else
			{
				char text_string[2];

				text_string[0] = makes[i];
				text_string[1] = '\0';
				pParams->AddString( "text", text_string );
			}
			
			pParams->AddInteger( "max_length", mdl.m_max_kb_string_length );
			Script::RunScript( "handle_keyboard_input", pParams );
			delete pParams;
		}
	}

	// Don't let anyone else handle the keypresses
	SIO::KeyboardClear();
}


/*****************************************************************
*	This friendly little function gets called every frame. 'game_is_paused'
*	is true if the game is paused, or if the player is in the front end.
*
*	I will probably move this function to MainMod.cpp or some other more
*	logical place later on.                                                                
*                                                               
******************************************************************/
void FrontEnd::update(bool game_is_paused)
{	
	/*
		===========================================
		Non-front end Stuff
		===========================================
	*/

	// Turn Keybard handler on and off

	#ifdef	__PLAT_NGPS__	
	if (	game_is_paused										// paused
		|| 	GameNet::Manager::Instance()->InNetGame()			// or a network game
		|| Mdl::Skate::Instance()->m_requested_level == 0x9f2bafb7        	// (0x9f2bafb7 === Load_SkateShop) In skateshop
		|| m_handle_keyboard_task->InList() 					// if the keyboard is on-screen
		)
	{
		SIO::SetKeyboardActive(true);
	}
	else
	{
		SIO::SetKeyboardActive(false);
	}
	#endif
	

	
	// Call these functions every frame:
	Pcm::Update( );	

	Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();
	sfx_manager->Update();

	if ( !game_is_paused )
	{
		// Mick: using the default heap here....
		// as spawned scripts can fill up the front end heap in an unpredictable manner
		// like with the LA earthquake	
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());


#		ifdef __USE_PROFILER__
		Sys::CPUProfiler->PushContext( 128, 0, 0 ); 	// red under green = Positional Sounds
#		endif // __USE_PROFILER__

		sfx_manager->UpdatePositionalSounds( );

#		ifdef __USE_PROFILER__
		Sys::CPUProfiler->PopContext( ); 	// 
#		endif // __USE_PROFILER__

		Mem::Manager::sHandle().PopContext();
	}

	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());

#	ifdef __USE_PROFILER__
	Sys::CPUProfiler->PushContext( 128, 128, 128 ); 	// gray under green = spawned scripts
#	endif // __USE_PROFILER__

	// And call these functions every game frame:
	Script::UpdateSpawnedScripts();
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	skate_mod->UpdateGameFlow();

	// mick: run the special script I use for command line scripting	
	if (run_runmenow)
	{
		run_runmenow = false;
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		Obj::CSkater *pSkater = skate_mod->GetLocalSkater();						   
		Script::RunScript("RunMeNow",NULL,pSkater);
	}


#	ifdef __USE_PROFILER__
	Sys::CPUProfiler->PopContext( ); 	// 
#	endif // __USE_PROFILER__

	Mem::Manager::sHandle().PopContext();
}




uint32 FrontEnd::grab_timed_event_type(uint mask, int index)
{
    char makes[256];
	int i, num_chars;
	uint32 event_type = 0;
		
	/*
		We check for these types of pad presses first, because of the timing element
	*/
	if (mask & Inp::Data::mD_LEFT)
		event_type = Obj::CEvent::TYPE_PAD_LEFT;
	else if (mask & Inp::Data::mD_RIGHT)
		event_type = Obj::CEvent::TYPE_PAD_RIGHT;
	else if (mask & Inp::Data::mD_UP)
		event_type = Obj::CEvent::TYPE_PAD_UP;
	else if (mask & Inp::Data::mD_DOWN)
		event_type = Obj::CEvent::TYPE_PAD_DOWN;	
	
	// Let the pad take precedence over the keyboard
	if(( index == 0 ) && ( event_type == 0 ))
	{
		// First check for keyboard events, if there were any
		num_chars = SIO::KeyboardRead( makes );
	
		if( num_chars > 0 )
		{
			for( i = 0; i < num_chars; i++ )
			{
				if( makes[i] == SIO::vKB_LEFT )
				{
					event_type = Obj::CEvent::TYPE_PAD_LEFT;
					break;
				}
				else if( makes[i] == SIO::vKB_RIGHT )
				{
					event_type = Obj::CEvent::TYPE_PAD_RIGHT;
					break;
				}
				else if( makes[i] == SIO::vKB_UP )
				{
					event_type = Obj::CEvent::TYPE_PAD_UP;
					break;
				}
				else if( makes[i] == SIO::vKB_DOWN )
				{
					event_type = Obj::CEvent::TYPE_PAD_DOWN;
					break;
				}
			}
		}
	}

	if (m_pad_info[index].mState == NOT_DOWN)
	{
		if (event_type)
		{
			// The directional pad was freshly pressed, so we will send an event.
			// Begin next countdown until the pad input "counts" again.
			m_pad_info[index].mAutorepeatCountdown = m_auto_repeat_time[0];
			m_pad_info[index].mState = SLOW_REPEAT;
		}
	}
	else if (event_type)
	{ 		
		// The directional pad was held down last frame	(and this one)
		
		// update countdown	time
		Tmr::Time frame_time = (Tmr::Time) (Tmr::FrameLength() * Tmr::vRESOLUTION);
		if (frame_time < m_pad_info[index].mAutorepeatCountdown)
			m_pad_info[index].mAutorepeatCountdown -= frame_time;
		else
			m_pad_info[index].mAutorepeatCountdown = 0;

		if (m_pad_info[index].mAutorepeatCountdown == 0)
		{
			// countdown finished, time to fire event, reset countdown time
			if (m_pad_info[index].mState == SLOW_REPEAT)
				m_pad_info[index].mState = FAST_REPEAT;
			m_pad_info[index].mAutorepeatCountdown = m_auto_repeat_time[1];
		}
		else
			// don't fire pad event, countdown hasn't finished
			event_type = 0;
	}
	else
	{
		m_pad_info[index].mState = NOT_DOWN;
	}	

	if (m_using_auto_input && index == 0)
	{
		event_type = 0;

		Script::CScript *p_script = Script::GetScriptWithUniqueId(m_auto_input_script_id);
		if (p_script)
		{
			Script::UnpauseSpawnedScript(p_script);
		}
		else
		{
			m_using_auto_input = false;
		}		
	}
	
	return event_type;
}




uint32 FrontEnd::turn_mask_into_event_type(uint mask, int count, int index, uint buttons)
{
    for (int i = 0; i < MAX_BUTTON_EVENT_MAP_ENTRIES; i++)
	{
		if ((mask & (1<<m_digital_button_event_map[i].mDigitalButtonIndex)) && 
			m_digital_button_event_map[i].mEventType != DEAD_ENTRY)
		{
			count--;
			if (count < 0)
				return m_digital_button_event_map[i].mEventType;
		}
	}
	

	// Mick:  Ignore start button as an event if select is pressed and select_shift is true	
	if( ! (buttons & Inp::Data::mD_SELECT) || ! Script::GetInt(CRCD(0xf3e055e1,"select_shift")))
	{
		if( (mask & Inp::Data::mD_START) && count == 0 )
			return Obj::CEvent::TYPE_PAD_START;
	}
	
	if( index == 0 )
	{
		char makes[256];
		int i, num_chars;
		uint32 event_type = 0;

		// First check for keyboard events, if there were any
		num_chars = SIO::KeyboardRead( makes );

		if( num_chars > 0 )
		{
			for( i = 0; i < num_chars; i++ )
			{   
				// If the keyboard menu is showing, don't treat space and enter as X
				if( makes[i] == SIO::vKB_ESCAPE )
				{
					
					Script::CStruct* pParams;

					pParams= new Script::CStruct;
					pParams->AddChecksum( Script::GenerateCRC( "id" ), Script::GenerateCRC( "current_menu_anchor" ));
					if( Obj::ScriptObjectExists( pParams, NULL ) == false )
					{
						event_type = Obj::CEvent::TYPE_PAD_START;
					}
					else
					{
						event_type = Obj::CEvent::TYPE_PAD_BACK;
					}
					
					SIO::KeyboardClear();
					delete pParams;
					break;
				}
				else if(( makes[i] == SIO::vKB_ENTER ) || ( makes[i] == 32 ))
				{
					Script::CStruct* pParams;
					bool menu_up;

					menu_up = false;
					pParams = new Script::CStruct;
					pParams->AddChecksum( Script::GenerateCRC( "id" ), Script::GenerateCRC( "keyboard_anchor" ));
					
					// Enter and space act as "choose" only if you're not currently using the on-screen keyboard
					if( Obj::ScriptObjectExists( pParams, NULL ) == false )
					{
						pParams->Clear();
    					pParams->AddChecksum( Script::GenerateCRC( "id" ), Script::GenerateCRC( "actions_menu" ));
                        
                        // only allow enter in actions menu
                        if( (Obj::ScriptObjectExists( pParams, NULL ) == true) && ( makes[i] == 32 ) )
    					{
                            return 0;
                        }
                        else
                        {
                            pParams->Clear();
    						pParams->AddChecksum( Script::GenerateCRC( "id" ), Script::GenerateCRC( "current_menu_anchor" ));
    						if( Obj::ScriptObjectExists( pParams, NULL ))
    						{
    							menu_up = true;
    						}
    						else
    						{
    							pParams->Clear();
    							pParams->AddChecksum( Script::GenerateCRC( "id" ), Script::GenerateCRC( "dialog_box_anchor" ));
    							if( Obj::ScriptObjectExists( pParams, NULL ))
    							{
    								menu_up = true;
    							}
    						}
    
    						if( menu_up )
    						{
    							event_type = Obj::CEvent::TYPE_PAD_CHOOSE;
    							SIO::KeyboardClear();
    						}
                        }
					}
					delete pParams;
				}
			}
		}

		return event_type;
	}
	
    return 0;
}


void FrontEnd::UpdateInputHandlerMappings()
{
	Spt::SingletonPtr< Inp::Manager > inp_man;
	
	for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		printf("m_device_server_map[%i] = %i\n", i, m_device_server_map[i]);
		Inp::Handler< FrontEnd >* p_handler = GetInputHandler( i );
		if ( p_handler )
		{
			printf("got a handler\n");
			inp_man->ReassignHandler( *p_handler, m_device_server_map[i] );
		}
	}
}

int FrontEnd::GetInputHandlerMapping( int device_num )
{
	return m_device_server_map[device_num];
}


bool ScriptLaunchMenuScreen(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 which_menu;
	if (!pParams->GetChecksum("screen", &which_menu))
		Dbg_MsgAssert(0, ("need screen=... with LaunchMenuScreen command"));

	// XXX
	Ryan("requesting game menu from LaunchMenuScreen\n");
//	FrontEnd* frontend = FrontEnd::Instance();
//	frontend->RequestMenuScreen(which_menu);

	printf ("STUBBED: LaunchMenuScreen\n");
	
	return true;
}




bool ScriptSetMenuAutoRepeatTimes(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Script::CPair times;
	pParams->GetPair(NONAME, &times, Script::ASSERT);

	FrontEnd* p_front_end = FrontEnd::Instance();
	p_front_end->SetAutoRepeatTimes((Tmr::Time) (times.mX * (float) Tmr::vRESOLUTION),
									(Tmr::Time) (times.mY * (float) Tmr::vRESOLUTION));
	return true;
}




bool ScriptSetMenuPadMappings(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Script::CArray *p_map_array;
	pParams->GetArray(NONAME, &p_map_array, true);

	FrontEnd* p_front_end = FrontEnd::Instance();
	for (int i = 0; i < 2; i++)
	{
		uint32 flag = p_map_array->GetChecksum(i);
		FrontEnd::EControllerMode mode = FrontEnd::INACTIVE;
		if (flag == Script::GenerateCRC("inactive"))
			mode = FrontEnd::INACTIVE;
		else if (flag == Script::GenerateCRC("active"))
			mode = FrontEnd::ACTIVE;
		else if (flag == Script::GenerateCRC("use_as_first"))
			mode = FrontEnd::MAP_TO_FIRST;
		else
			Dbg_MsgAssert(0, ("unknown controller mode"));

		p_front_end->SetControllerMode(i, mode);
	}

	return true;
}




bool ScriptSetButtonEventMappings(Script::CStruct *pParams, Script::CScript *pScript)
{
	FrontEnd* p_front_end = FrontEnd::Instance();
	p_front_end->AddEntriesToEventButtonMap(pParams);
	
	if (pParams->ContainsFlag("block_menu_input"))
		p_front_end->SetControllerTempBlock(true);
	else if (pParams->ContainsFlag("unblock_menu_input"))
		p_front_end->SetControllerTempBlock(false);
	
	return true;
}


// @script bool | SetAnalogStickActiveForMenus | Turn on/off analog simulation of digital buttons,
// so you can turn them off during certain menus that use analog for other things. 
// @uparmopt 1 | on or off, 1 or 0
bool ScriptSetAnalogStickActiveForMenus(Script::CStruct *pParams, Script::CScript *pScript)
{
	int	active = 1;
	pParams->GetInteger(NONAME,&active,false);
	FrontEnd::Instance()->SetAnalogStickActiveForMenus((bool)active);
	return true;
}

} // namespace FrontEnd
