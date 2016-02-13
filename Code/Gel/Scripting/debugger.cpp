#include "gel/scripting/debugger.h"
#include "sk/gamenet/scriptdebugger.h"
#include "gel/mainloop.h"
#include "gel/scripting\symboltable.h"
#include "gel/scripting\checksum.h"
#include "gel/scripting\script.h"
#include "gel/scripting\symboltype.h"
#include "gel/scripting\array.h"
#include "gel/scripting\struct.h"
#include "gel/scripting\utils.h"
#include "gel/scripting\component.h"
#include "gel/scripting\scriptcache.h"
#include "gel/object/compositeobjectmanager.h"
#include "gel/components/modelcomponent.h"
#include "gfx/nxviewman.h"
#include "sk/modules/skate/skate.h"
#include "sk/objects/skater.h"
#include "sk/objects/skatercam.h"
#include "gel/event.h"


#ifdef __NOPT_ASSERT__


namespace Dbg
{

uint32 gpDebugInfoBuffer[SCRIPT_DEBUGGER_BUFFER_SIZE/4];

static Script::CScript *s_find_cscript_given_id(uint32 id)
{
	Script::CScript *p_script=Script::GetNextScript(NULL);
	while (p_script)
	{
		if (p_script->GetUniqueId()==id)
		{
			return p_script;
		}	
		p_script=Script::GetNextScript(p_script);
	}	
	return NULL;
}

static Script::CScript *s_find_cscript_given_name(uint32 name)
{
	Script::CScript *p_script=Script::GetNextScript(NULL);
	while (p_script)
	{
		if (p_script->RefersToScript(name))
		{
			return p_script;
		}	
		p_script=Script::GetNextScript(p_script);
	}	
	return NULL;
}

static Obj::CCompositeObject *s_find_nearest_object(int screen_x, int screen_y, float search_radius)
{
	Nx::CViewport *p_viewport = Nx::CViewportManager::sGetActiveViewport();
	Dbg_MsgAssert(p_viewport, ("Can't find an active viewport"));
	
	Lst::Search<Obj::CObject> sh;
	Obj::CObject *p_ob = (Obj::CObject*) sh.FirstItem( Obj::CCompositeObjectManager::Instance()->GetRefObjectList() );
	Obj::CCompositeObject *p_nearest_ob=NULL;
	float min_d=1000000.0f;
	while (p_ob)
	{
		float x,y;
		Nx::ZBufferValue z;
		p_viewport->TransformToScreenCoord(((Obj::CCompositeObject*)p_ob)->GetPos(),x,y,z);
		
		float dx=x-screen_x;
		float dy=y-screen_y;
		float d=sqrtf(dx*dx+dy*dy);
		if (d<search_radius && d<min_d)
		{
			min_d=d;
			p_nearest_ob=(Obj::CCompositeObject*)p_ob;
		}	
		
		p_ob = sh.NextItem();
	}
	
	return p_nearest_ob;
}

static Nx::ERenderMode s_change_render_mode(Obj::CCompositeObject *p_ob, Nx::ERenderMode new_render_mode)
{
	Nx::ERenderMode old_render_mode=Nx::vNONE;
	
	if (p_ob)
	{
		Obj::CModelComponent *p_model=(Obj::CModelComponent*)p_ob->GetComponent(CRCD(0x286a8d26,"Model"));
		if (p_model)
		{
			Nx::CModel *p_cmodel=p_model->GetModel();
			if (p_cmodel)
			{
				old_render_mode=p_cmodel->GetRenderMode();
				p_cmodel->SetRenderMode(new_render_mode);
			}
		}
	}
	
	return old_render_mode;
}

DefineSingletonClass( CScriptDebugger, "Script debugger module" );

CScriptDebugger::CScriptDebugger()
{
	// Note: Split message id's start at 1 because 0 is used to indicate no message.
	m_split_message_id=1;
	
	m_state=SCRIPT_DEBUGGER_STATE_IDLE;
	mp_checksum_names_current_pos=NULL;
	mp_checksum_names_end=NULL;
	mp_server=NULL;
	mp_logic_task=NULL;
	
	m_num_watched_scripts=0;
	
	m_mouse_on_screen=false;
	m_got_valid_mouse_position=false;
	m_mouse_x=0;
	m_mouse_y=0;
	mp_currently_selected_object=NULL;
	m_original_object_render_mode=Nx::vNONE;
	m_left_button_down=false;	
	m_right_button_down=false;	

	m_state_when_left_pressed.mTime=0;
	m_state_when_left_pressed.mMouseX=0;
	m_state_when_left_pressed.mMouseY=0;
	m_state_when_left_pressed.mCamPhi=0.0f;
	m_state_when_left_pressed.mCamTheta=0.0f;
	
	m_state_when_right_pressed.mCamRadius=0.0f;
	m_state_when_right_pressed.mMouseX=0;
	m_state_when_right_pressed.mMouseY=0;
	
	mp_viewed_object=NULL;
	setup_default_camera_viewing_position();
	
	m_in_object_single_step_mode=false;
	m_single_step_object_id=0;
	
	m_object_update_total_bytes_sent=0;
	m_frames_since_last_object_update=0;
}

CScriptDebugger::~CScriptDebugger()
{
	if (mp_server)
	{
		delete mp_server;
	}	
	if (mp_logic_task)
	{
		delete mp_logic_task;
	}	
}

void CScriptDebugger::v_start_cb ( void )
{
// Stick it all on the network heap, as it's kind of network releated
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());


	Net::Manager * p_net_man = Net::Manager::Instance();
	mp_server = p_net_man->CreateNewAppServer( 0, "Script debugger", 4, GameNet::vSCRIPT_DEBUG_PORT,
												inet_addr( p_net_man->GetLocalIP()), Net::App::mACCEPT_FOREIGN_CONN | Net::App::mDYNAMIC_RESEND );
	
	// Note: If the default task priority is used then the 'view object' functionality does not work.
	// s_logic_code needs to be called after all the normal camera code has executed, so that the camera
	// position can be overridden. Hence a priority lower than the default needs to be used. 
	mp_logic_task = new Tsk::Task< CScriptDebugger > ( CScriptDebugger::s_logic_code, *this, Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_SCRIPT_DEBUGGER );

	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
	mlp_manager->AddLogicTask( *mp_logic_task );
	
	mp_server->m_Dispatcher.AddHandler(	Net::MSG_ID_CONNECTION_REQ, 
										s_handle_connection_request, 
										Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this );
										
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_WATCH_THIS, 
										s_handle_watch_this, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_WATCH_SCRIPT, 
										s_handle_watch_script, 
										Net::mHANDLE_LATE, this);
										
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_STOP, 
										s_handle_stop, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_STEP_INTO, 
										s_handle_step_into, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_STEP_OVER, 
										s_handle_step_over, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_GO, 
										s_handle_go, 
										Net::mHANDLE_LATE, this);

	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_CLEAR_SCRIPT_WATCHES, 
										s_handle_clear_script_watches, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_REFRESH, 
										s_handle_refresh, 
										Net::mHANDLE_LATE, this);
										
	mp_server->m_Dispatcher.AddHandler(	Net::MSG_ID_DISCONN_REQ, 
										s_handle_disconnect, 
										Net::mHANDLE_LATE, this);

	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_SEND_CSCRIPT_LIST, 
										s_handle_send_cscript_list, 
										Net::mHANDLE_LATE, this);

	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_SEND_WATCH_LIST, 
										s_handle_send_watch_list, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_STOP_WATCHING_THIS_INDEX, 
										s_handle_stop_watching_this_index, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_STOP_WATCHING_THIS_CSCRIPT, 
										s_handle_stop_watching_this_cscript, 
										Net::mHANDLE_LATE, this);

	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_SEND_COMPOSITE_OBJECT_INFO, 
										s_handle_send_composite_object_info, 
										Net::mHANDLE_LATE, this);

	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_MOUSE_POSITION, 
										s_handle_mouse_position, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_MOUSE_ON_SCREEN, 
										s_handle_mouse_on_screen, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_MOUSE_OFF_SCREEN, 
										s_handle_mouse_off_screen, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_MOUSE_LEFT_BUTTON_DOWN, 
										s_handle_mouse_left_button_down, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_MOUSE_LEFT_BUTTON_UP, 
										s_handle_mouse_left_button_up, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_MOUSE_RIGHT_BUTTON_DOWN, 
										s_handle_mouse_right_button_down, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_MOUSE_RIGHT_BUTTON_UP, 
										s_handle_mouse_right_button_up, 
										Net::mHANDLE_LATE, this);

										
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_SEND_COMPOSITE_OBJECT_LIST, 
										s_handle_send_composite_object_list, 
										Net::mHANDLE_LATE, this);
										
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_SET_OBJECT_UPDATE_MODE, 
										s_handle_set_object_update_mode, 
										Net::mHANDLE_LATE, this);
	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_VIEW_OBJECT, 
										s_handle_view_object, 
										Net::mHANDLE_LATE, this);

	mp_server->m_Dispatcher.AddHandler(	GameNet::vMSG_ID_DBG_SEND_SCRIPT_GLOBAL_INFO, 
										s_handle_send_script_global_info, 
										Net::mHANDLE_LATE, this);
										
	m_paused = false;
	
	Net::Manager* net_man = Net::Manager::Instance();
	net_man->SetBandwidth( 200000 );

	Mem::Manager::sHandle().PopContext();
	
}

void CScriptDebugger::v_stop_cb ( void )
{
	mp_logic_task->Remove();
	mp_server->m_Dispatcher.Deinit();
}

void CScriptDebugger::transmit_cscript_list()
{
	Script::CStruct *p_list=new Script::CStruct;

	//int total_num_return_addresses=0;
	//int total_scripts_using_big_buffer=0;
	int num_scripts=0;
	Script::CScript *p_script=Script::GetNextScript(NULL);
	while (p_script)
	{
		//total_num_return_addresses+=p_script->GetNumReturnAddresses();
		//if (p_script->UsingBigLoopBuffer())
		//{
		//	++total_scripts_using_big_buffer;
		//}
			
		p_script=Script::GetNextScript(p_script);
		++num_scripts;
	}	
	p_list->AddInteger(CRCD(0x62f3d8fa,"NumScripts"),num_scripts);
	//p_list->AddInteger(CRCD(0xed4b5c6e,"NumScriptsUsingBigBuffer"),total_scripts_using_big_buffer);
	//p_list->AddFloat(CRCD(0x4f73b852,"AverageNumReturnAddresses"),((float)total_num_return_addresses)/num_scripts);


	// Add the script cache info.	
	Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
	Dbg_MsgAssert(p_script_cache,("NULL p_script_cache"));
	p_script_cache->GetDebugInfo(p_list);

	p_script=Script::GetNextScript(NULL);
	while (p_script)
	{
		Script::CStruct *p_script_info=new Script::CStruct;
		
		p_script_info->AddChecksum("m_unique_id",p_script->GetUniqueId());
		
		//p_script_info->AddInteger(CRCD(0x46b0d38c,"NumReturnAddresses"),p_script->GetNumReturnAddresses());
		//p_script_info->AddInteger(CRCD(0x477f712b,"UsingBigCallstackBuffer"),p_script->UsingBigCallstackBuffer());
		
		Obj::CObject *p_object=p_script->mpObject;
		if (p_object)
		{
			p_script_info->AddChecksum("Object",p_object->GetID());
		}	
		
		if (p_script->mIsSpawned)
		{
			p_script_info->AddChecksum(NONAME,CRCD(0xf697fda7,"Spawned"));
		}
		
		#ifdef	__SCRIPT_EVENT_TABLE__		
			if (p_script->GetEventHandlerTable())
			{
				p_script->GetEventHandlerTable()->GetDebugInfo(p_script_info);
			}
		#endif

		
		// Convert to microseconds by dividing by 150
		p_script_info->AddInteger("CPUTime",p_script->m_last_time/150);

		// Create a new structure component for p_script_info, and name it using the name of the script.
		// Then add this component to the p_list structure using AppendComponentPointer.
		// The reason it is done this way rather than simply using AddStructurePointer is that
		// AddStructurePointer will remove any previously added components of the same name.
		// There may be many instances of the same script running, so we want those to all appear.
		// AppendComponentPointer will add the new component to the structure and will leave any
		// existing components untouched. (it was written specially for this function)
		Script::CComponent *p_comp=new Script::CComponent;
		p_comp->mType=ESYMBOLTYPE_STRUCTURE;
		p_comp->mpStructure=p_script_info;
		
		uint32 base_script=p_script->GetBaseScript();
		if (base_script)
		{
			p_comp->mNameChecksum=base_script;
		}
		else	
		{
			// Occasionally the base_script might be zero (sometimes when scrolling
			// through certain menus)
			// Name it using the unique id of the script.
			p_comp->mNameChecksum=p_script->GetUniqueId();
		}

		p_list->AppendComponentPointer(p_comp);
				
		p_script=Script::GetNextScript(p_script);
	}	

	int structure_bytes_written=Script::WriteToBuffer(p_list,(uint8*)gpDebugInfoBuffer,SCRIPT_DEBUGGER_BUFFER_SIZE);
	delete p_list;
	
	Net::MsgDesc msg;
	msg.m_Data = gpDebugInfoBuffer;
	msg.m_Length = structure_bytes_written;
	msg.m_Id = GameNet::vMSG_ID_DBG_CSCRIPT_LIST;
	StreamMessage(&msg);
}

void CScriptDebugger::transmit_watch_list()
{
	SWatchInfo *p_watch_info=(SWatchInfo*)gpDebugInfoBuffer;
	Dbg_MsgAssert(m_num_watched_scripts*sizeof(SWatchInfo)<=SCRIPT_DEBUGGER_BUFFER_SIZE,("gpDebugInfoBuffer overflow"));

	for (int i=0; i<m_num_watched_scripts; ++i)
	{
		p_watch_info->mScriptName=mp_watched_scripts[i].mChecksum;
		p_watch_info->mStopImmediately=mp_watched_scripts[i].mStopScriptImmediately;
		++p_watch_info;
	}		

	Net::MsgDesc msg;
	msg.m_Data = gpDebugInfoBuffer;
	msg.m_Length = m_num_watched_scripts*sizeof(SWatchInfo);
	msg.m_Id = GameNet::vMSG_ID_DBG_WATCH_LIST;
	StreamMessage(&msg);
}

void CScriptDebugger::transmit_composite_object_list()
{
	Script::CStruct *p_list=new Script::CStruct;
	
	Lst::Search<Obj::CObject> sh;
	Obj::CObject *p_ob = (Obj::CObject*) sh.FirstItem( Obj::CCompositeObjectManager::Instance()->GetRefObjectList() );
	while (p_ob)
	{
		Script::CStruct *p_object_info=new Script::CStruct;
		p_object_info->AddInteger("CPUTime",((Obj::CCompositeObject*)p_ob)->GetUpdateTime());
		if (p_ob->GetID())
		{
			p_list->AddStructurePointer(p_ob->GetID(),p_object_info);
		}
		else
		{
			p_list->AddStructurePointer("Skater",p_object_info);
		}	
		
		p_ob = sh.NextItem();
	}
	
	int structure_bytes_written=Script::WriteToBuffer(p_list,(uint8*)gpDebugInfoBuffer,SCRIPT_DEBUGGER_BUFFER_SIZE);
	delete p_list;
	
	Net::MsgDesc msg;
	msg.m_Data = gpDebugInfoBuffer;
	msg.m_Length = structure_bytes_written;
	msg.m_Id = GameNet::vMSG_ID_DBG_COMPOSITE_OBJECT_LIST;
	Dbg::CScriptDebugger::Instance()->StreamMessage(&msg);
}

void CScriptDebugger::transmit_compression_lookup_table()
{   
	Script::CArray *p_table=Script::GetArray(0x35115a20/*WriteToBuffer_CompressionLookupTable_8*/);
	Dbg_MsgAssert(p_table->GetType()==ESYMBOLTYPE_NAME,("Bad array type"));
	Dbg_MsgAssert(p_table->GetSize()<=256,("Bad array size"));
	
    Net::MsgDesc msg;
	msg.m_Data = p_table->GetArrayPointer();
	msg.m_Length = p_table->GetSize()*4;
	msg.m_Id = GameNet::vMSG_ID_DBG_COMPRESSION_LOOKUP_TABLE;
	StreamMessage(&msg);

	p_table=Script::GetArray(0x25231f42/*WriteToBuffer_CompressionLookupTable_16*/);
	Dbg_MsgAssert(p_table->GetSize()==0,("Need to update transmit_compression_lookup_table to handle WriteToBuffer_CompressionLookupTable_16, since it is no longer empty ..."));
}	

SWatchedScript *CScriptDebugger::find_watched_script(uint32 checksum)
{
	for (int i=0; i<m_num_watched_scripts; ++i)
	{
		if (mp_watched_scripts[i].mChecksum==checksum)
		{
			return &mp_watched_scripts[i];
		}
	}		
	return NULL;
}

void CScriptDebugger::add_to_watched_scripts(uint32 checksum, bool stopScriptImmediately)
{
	Script::CScript *p_script=s_find_cscript_given_name(checksum);
	if (p_script)
	{
        p_script->WatchInDebugger(stopScriptImmediately);
	}
		
	SWatchedScript *p_watched_script=find_watched_script(checksum);
	if (p_watched_script)
	{
		p_watched_script->mStopScriptImmediately=stopScriptImmediately;
		return;
	}

	Dbg_MsgAssert(m_num_watched_scripts<MAX_WATCHED_SCRIPTS,("Too many watched scripts"));
	mp_watched_scripts[m_num_watched_scripts].mStopScriptImmediately=stopScriptImmediately;
	mp_watched_scripts[m_num_watched_scripts].mChecksum=checksum;
	++m_num_watched_scripts;
	
	transmit_watch_list();
}

void CScriptDebugger::refresh()
{
	transmit_compression_lookup_table();
	transmit_watch_list();
	transmit_cscript_list();
	transmit_composite_object_list();
	
	Script::CScript *p_script=Script::GetNextScript(NULL);
	while (p_script)
	{
		if (p_script->BeingWatchedInDebugger())
		{
			// DEFINITELY_TRANSMIT means override the optimization whereby a packet is not
			// sent if it's checksum matches that of the last sent.
			p_script->TransmitInfoToDebugger(Script::DEFINITELY_TRANSMIT);
		}	
		p_script=Script::GetNextScript(p_script);
	}	
}

void CScriptDebugger::stop_watching_script(int index)
{
	if (index<0 || index >= m_num_watched_scripts)
	{
		return;
	}	

	if (m_num_watched_scripts)
	{
		for (int i=index; i<m_num_watched_scripts-1; ++i)
		{
			mp_watched_scripts[i]=mp_watched_scripts[i+1];
		}
		--m_num_watched_scripts;
	}
	
	// Refresh the window in the debugger by transmitting the new watch list.
	transmit_watch_list();
}

int CScriptDebugger::send_composite_object_info(Obj::CCompositeObject* p_obj)
{
	int num_bytes_sent=0;
	
	if (p_obj)
	{
		Script::CStruct *p_info=new Script::CStruct;
		p_obj->GetDebugInfo(p_info);
		
		Dbg_MsgAssert(SCRIPT_DEBUGGER_BUFFER_SIZE>4,("Oops"));
		gpDebugInfoBuffer[0]=p_obj->GetID();
		uint8 *p_buf=(uint8*)(gpDebugInfoBuffer+1);
		int structure_bytes_written=Script::WriteToBuffer(p_info,p_buf,SCRIPT_DEBUGGER_BUFFER_SIZE-4);
		delete p_info;
		
		Net::MsgDesc msg;
		msg.m_Data = gpDebugInfoBuffer;
		msg.m_Length = 4+structure_bytes_written;
		msg.m_Id = GameNet::vMSG_ID_DBG_COMPOSITE_OBJECT_INFO;

		StreamMessage(&msg);
		num_bytes_sent=msg.m_Length;
		
		printf("Sent composite object info for '%s'\n",Script::FindChecksumName(p_obj->GetID()));
	}	
	return num_bytes_sent;
}

void CScriptDebugger::send_composite_object_info(uint32 id)
{
	send_composite_object_info((Obj::CCompositeObject*)Obj::CCompositeObjectManager::Instance()->GetObjectByID( id ));
}

void CScriptDebugger::set_object_update_mode(SObjectUpdateMode *pObjectUpdateMode)
{
	// Make sure the game is not paused, which it will be if in single step mode.
	Dbg::CScriptDebugger::Instance()->ScriptDebuggerUnpause();
	
	switch (pObjectUpdateMode->mMode)
	{
		case Dbg::EVERY_FRAME:
		{
			Obj::CCompositeObject* p_obj=(Obj::CCompositeObject*)Obj::CCompositeObjectManager::Instance()->GetObjectByID(pObjectUpdateMode->mObjectId);
			if (p_obj)
			{
				p_obj->SetDebuggerEveryFrameFlag(pObjectUpdateMode->mOn);
			}	
			break;
		}	
		case Dbg::SINGLE_STEP:
			if (pObjectUpdateMode->mOn)
			{
				// Set the single step mode flag to true, which means that the next time in to
				// s_logic_code it will send a packet of info for this object, then pause the game.
				// When paused, it will still respond to incoming messages, so the next time
				// the user clicks the 'single step' button it will call this function, hence
				// unpausing the game for one frame until it gets in to s_logic_code again, where
				// it will send another packet of info, then pause again etc.
				m_in_object_single_step_mode=true;
				m_single_step_object_id=pObjectUpdateMode->mObjectId;
			}	
			else
			{
				m_in_object_single_step_mode=false;
				m_single_step_object_id=0;
			}
			break;
		default:	
			break;
	}	
}

void CScriptDebugger::view_object(SViewObject *pViewObject)
{
	Obj::CCompositeObject *p_obj=NULL;
	
	if (pViewObject->mDoViewObject)
	{
		p_obj=(Obj::CCompositeObject*)Obj::CCompositeObjectManager::Instance()->GetObjectByID(pViewObject->mObjectId);
	}
	mp_viewed_object=p_obj;
	
	setup_default_camera_viewing_position();
}

void CScriptDebugger::send_any_single_step_object_info()
{
	if (m_in_object_single_step_mode)
	{
		Obj::CCompositeObject* p_obj=(Obj::CCompositeObject*)Obj::CCompositeObjectManager::Instance()->GetObjectByID(m_single_step_object_id);
		if (p_obj)
		{
			// Send a packet of info, then pause the game until the user clicks the 'single step'
			// button again, which will cause CScriptDebugger::set_object_update_mode to be called.
			send_composite_object_info(p_obj);
			Dbg::CScriptDebugger::Instance()->ScriptDebuggerPause();
		}
	}	
}

void CScriptDebugger::send_script_global_info(uint32 id)
{
	// Look up the symbol to see if it is the name of some global.
	Script::CSymbolTableEntry *p_entry=Script::LookUpSymbol(id);
	if (p_entry)
	{
		int message_size=0;		
		uint8 *p_buf=(uint8*)gpDebugInfoBuffer;

		// Write in the name, type, and source file name.		
		Dbg_MsgAssert(SCRIPT_DEBUGGER_BUFFER_SIZE > 8,("SCRIPT_DEBUGGER_BUFFER_SIZE is like way too small dude"));
		*(uint32*)p_buf=id;
		p_buf+=4;
		*(uint32*)p_buf=p_entry->mType;
		p_buf+=4;
		message_size+=8;
		
		const char *p_source_filename=Script::FindChecksumName(p_entry->mSourceFileNameChecksum);
		int len=strlen(p_source_filename)+1;
		Dbg_MsgAssert(SCRIPT_DEBUGGER_BUFFER_SIZE-message_size > len,("SCRIPT_DEBUGGER_BUFFER_SIZE is too small"));
		
		strcpy((char*)p_buf,p_source_filename);
		p_buf+=len;
		message_size+=len;

		// For pure data types, write in a structure containing the value.
		switch (p_entry->mType)
		{
			case ESYMBOLTYPE_INTEGER:
			case ESYMBOLTYPE_FLOAT:
			case ESYMBOLTYPE_STRING:
			case ESYMBOLTYPE_LOCALSTRING:
			case ESYMBOLTYPE_PAIR:
			case ESYMBOLTYPE_VECTOR:
			case ESYMBOLTYPE_STRUCTURE:
			case ESYMBOLTYPE_ARRAY:
			case ESYMBOLTYPE_NAME:
			{
				// Copy the name and value into a structure for sending to the
				// script debugger for display.
				Script::CStruct *p_struct=new Script::CStruct;
		
				// Create a structure component and fill it in with the value of the global.
				Script::CComponent *p_source=new Script::CComponent;
				p_source->mType=p_entry->mType;
				p_source->mUnion=p_entry->mUnion; // What mUnion represents depends on mType
				
				// Now use CopyComponent to make a copy. This needs to be done because mUnion may
				// be a pointer to some entity, and we don't want that to be deleted when p_struct
				// gets deleted.
				Script::CComponent *p_new=new Script::CComponent;
				Script::CopyComponent(p_new,p_source);
				// Delete p_source now that we've got a copy.
				p_source->mUnion=0;
				delete p_source;
				
				// Name the component using id so that it appears as expected in the structure.
				// Ie, if Foo is a global with value 6, then it will appear as Foo=6 in the structure.
				p_new->mNameChecksum=id;
				p_struct->AddComponent(p_new);
		
				
				int structure_bytes_written=Script::WriteToBuffer(p_struct,p_buf,SCRIPT_DEBUGGER_BUFFER_SIZE-message_size);
				delete p_struct;
				
				p_buf+=structure_bytes_written;
				message_size+=structure_bytes_written;
				break;
			}	
			default:	
				break;
		}			
		
		// Send the info to the debugger.
		Net::MsgDesc msg;
		msg.m_Data = gpDebugInfoBuffer;
		msg.m_Length = message_size;
		msg.m_Id = GameNet::vMSG_ID_DBG_SCRIPT_GLOBAL_INFO;
		StreamMessage(&msg);
	}	
}

void CScriptDebugger::stop_debugging()
{
	m_num_watched_scripts=0;
	
	Script::CScript *p_script=Script::GetNextScript(NULL);
	while (p_script)
	{
		p_script->StopWatchingInDebugger();
		p_script=Script::GetNextScript(p_script);
	}	

	stop_all_every_frame_object_updates();
	
	ScriptDebuggerUnpause();
	
	switch_off_mouse_cursor();
}

int	CScriptDebugger::s_handle_connection_request( Net::MsgHandlerContext* context )
{   
	Dbg_Assert( context );

	Dbg_Printf( "Got Join Request from ip %x port %d\n", context->m_Conn->GetIP(), context->m_Conn->GetPort());

	// We'll just accept local clients
	if( context->m_Conn->IsRemote())
	{
		// Rule out redundancy
		if( context->m_PacketFlags & Net::mHANDLE_FOREIGN )
		{
			Net::Conn* conn;

			// Are we currently accepting players?
			if( context->m_App->AcceptsForeignConnections() == false )
			{
				return Net::HANDLER_MSG_DONE;
			}
			
			// Create a more permanent connection
			conn = context->m_App->NewConnection( context->m_Conn->GetIP(), context->m_Conn->GetPort());
			conn->SetBandwidthType( Net::Conn::vNARROWBAND );//Net::Conn::vLAN );
			conn->SetSendInterval( 0 );
			
			context->m_Conn->Invalidate();
			context->m_Conn = conn;	// the rest of the chain will use this new, valid connection
		}
	}


	// Make sure that the debugger is updated with the latest info.
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->refresh();
	
	return Net::HANDLER_CONTINUE;
}

// This function is kind of redundant at the moment, since there is now a seperate s_handle_watch_script
// and there will probably soon be a s_handle_watch_object.
int	CScriptDebugger::s_handle_watch_this( Net::MsgHandlerContext* context )
{   
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	
	Dbg_MsgAssert(context->m_MsgLength==4,("Unexpected length of %d for vMSG_ID_DBG_WATCH_THIS", context->m_MsgLength));
	
	uint32 checksum=*(uint32*)context->m_Msg;

	// Determine what kind of thing the checksum is referring to ...
	
	Script::CSymbolTableEntry *p_sym=Script::LookUpSymbol(checksum);
	if (p_sym)
	{
		switch (p_sym->mType)
		{
			case ESYMBOLTYPE_QSCRIPT:
			{
				// false means do not stop the script immediately when detected.
				p_this->add_to_watched_scripts(checksum,false);
				break;
			}	
			default:
				break;
		}
	}

	return Net::HANDLER_CONTINUE;
}

int	CScriptDebugger::s_handle_watch_script( Net::MsgHandlerContext* context )
{   
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	
	Dbg_MsgAssert(context->m_MsgLength==12,("Unexpected length of %d for vMSG_ID_DBG_WATCH_SCRIPT", context->m_MsgLength));
	
	uint32 *p_data=(uint32*)context->m_Msg;   			
	bool stop_script_immediately=p_data[0];
	
	uint32 type_of_checksum=p_data[1];
	uint32 checksum=p_data[2];

	if (type_of_checksum==CHECKSUM_IS_SCRIPT_ID)
	{
		Script::CScript *p_script=s_find_cscript_given_id(checksum);  
		if (p_script)
		{
			p_script->WatchInDebugger(stop_script_immediately);
			// Make sure the debugger at least gets some info, otherwise it won't get
			// any if the scripts update function is not called.
			p_script->TransmitInfoToDebugger();
		}	
		return Net::HANDLER_CONTINUE;
	}

	p_this->add_to_watched_scripts(checksum,stop_script_immediately);
	
	return Net::HANDLER_CONTINUE;	
}

int	CScriptDebugger::s_handle_stop( Net::MsgHandlerContext* context )
{   
	Dbg_MsgAssert(context->m_MsgLength==4,("Unexpected length of %d for vMSG_ID_DBG_STOP", context->m_MsgLength));
	uint32 id=*(uint32*)context->m_Msg;
	
	Script::CScript *p_script=s_find_cscript_given_id(id);
	if (!p_script)
	{
		return Net::HANDLER_CONTINUE;
	}

	p_script->DebugStop();
	
	return Net::HANDLER_CONTINUE;
}

int	CScriptDebugger::s_handle_step_into( Net::MsgHandlerContext* context )
{   
	Dbg_MsgAssert(context->m_MsgLength==4,("Unexpected length of %d for vMSG_ID_DBG_STOP", context->m_MsgLength));
	uint32 id=*(uint32*)context->m_Msg;

	Script::CScript *p_script=s_find_cscript_given_id(id);
	if (!p_script)
	{
		return Net::HANDLER_CONTINUE;
	}
	
	p_script->DebugStepInto();
	
	return Net::HANDLER_CONTINUE;
}

int	CScriptDebugger::s_handle_step_over( Net::MsgHandlerContext* context )
{   
	Dbg_MsgAssert(context->m_MsgLength==4,("Unexpected length of %d for vMSG_ID_DBG_STOP", context->m_MsgLength));
	uint32 id=*(uint32*)context->m_Msg;

	Script::CScript *p_script=s_find_cscript_given_id(id);
	if (!p_script)
	{
		return Net::HANDLER_CONTINUE;
	}
	
	p_script->DebugStepOver();
	
	return Net::HANDLER_CONTINUE;
}

int	CScriptDebugger::s_handle_go( Net::MsgHandlerContext* context )
{   
	Dbg_MsgAssert(context->m_MsgLength==4,("Unexpected length of %d for vMSG_ID_DBG_STOP", context->m_MsgLength));
	uint32 id=*(uint32*)context->m_Msg;

	Script::CScript *p_script=s_find_cscript_given_id(id);
	if (!p_script)
	{
		return Net::HANDLER_CONTINUE;
	}
	
	p_script->DebugGo();
	
	return Net::HANDLER_CONTINUE;
}

// This gets called whenever the "Clear script watches" menu option in the script debugger
// is chosen.
int	CScriptDebugger::s_handle_clear_script_watches( Net::MsgHandlerContext* context )
{   
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	
	// Clear the list of scripts being watched.
	p_this->m_num_watched_scripts=0;
	// Make sure the list gets updated in the debugger.
	p_this->transmit_watch_list();
	
	// Then run through all existing scripts making sure none of them are transmitting any more.
	Script::CScript *p_script=Script::GetNextScript(NULL);
	while (p_script)
	{
		p_script->StopWatchingInDebugger();
		
		// Also force any window open in the debugger for this script to close, by
		// sending a script-died message.
		uint32 id=p_script->GetUniqueId();
		Net::MsgDesc msg;
		msg.m_Data = &id;;
		msg.m_Length = 4;
		msg.m_Id = GameNet::vMSG_ID_DBG_SCRIPT_DIED;
		p_this->StreamMessage(&msg);
		
		p_script=Script::GetNextScript(p_script);
	}	
	
	return Net::HANDLER_CONTINUE;
}

int	CScriptDebugger::s_handle_refresh( Net::MsgHandlerContext* context )
{   
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->refresh();
	
	return Net::HANDLER_CONTINUE;
}

int	CScriptDebugger::s_handle_disconnect( Net::MsgHandlerContext* context )
{   
	Dbg_Printf("Got disconnect message from monitor.exe\n");
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->stop_debugging();
	
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_send_cscript_list( Net::MsgHandlerContext* context )
{
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->transmit_cscript_list();
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_send_watch_list( Net::MsgHandlerContext* context )
{
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->transmit_watch_list();
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_send_composite_object_list( Net::MsgHandlerContext* context )
{
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->transmit_composite_object_list();
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_set_object_update_mode( Net::MsgHandlerContext* context )
{
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->set_object_update_mode((SObjectUpdateMode*)context->m_Msg);
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_view_object( Net::MsgHandlerContext* context )
{
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->view_object((SViewObject*)context->m_Msg);
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_stop_watching_this_index( Net::MsgHandlerContext* context )
{
	uint32 index=*(uint32*)context->m_Msg;

	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->stop_watching_script(index);
		
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_stop_watching_this_cscript( Net::MsgHandlerContext* context )
{
	uint32 id=*(uint32*)context->m_Msg;
	
	Script::CScript *p_script=s_find_cscript_given_id(id);
	if (p_script)
	{
		p_script->StopWatchingInDebugger();
	}	
		
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_send_composite_object_info( Net::MsgHandlerContext* context )
{
	uint32 id=*(uint32*)context->m_Msg;
	
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->send_composite_object_info(id);
	
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_send_script_global_info( Net::MsgHandlerContext* context )
{
	uint32 id=*(uint32*)context->m_Msg;
	
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->send_script_global_info(id);
	
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_mouse_position( Net::MsgHandlerContext* context )
{
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	
	// Ignore the message if the m_mouse_on_screen flag is not set, because that would
	// mean it was a late message, received after the mouse had gone off screen.
	if (p_this->m_mouse_on_screen)
	{
		sint16 *p_coords=(sint16*)context->m_Msg;
		int x=p_coords[0];
		int y=p_coords[1];
	
		p_this->m_mouse_x=x;	
		p_this->m_mouse_y=y;	
		p_this->m_got_valid_mouse_position=true;
		
		Script::CStruct *p_params=new Script::CStruct;
		p_params->AddInteger("x",x);
		p_params->AddInteger("y",y);
		Script::RunScript("UpdateDebuggerMousePosition",p_params);
		delete p_params;
	}	
	
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_mouse_off_screen( Net::MsgHandlerContext* context )
{
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->switch_off_mouse_cursor();
	
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_mouse_left_button_down( Net::MsgHandlerContext* context )
{
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->left_button_down();
	
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_mouse_left_button_up( Net::MsgHandlerContext* context )
{
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->left_button_up();
	
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_mouse_right_button_down( Net::MsgHandlerContext* context )
{
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->right_button_down();
	
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_mouse_right_button_up( Net::MsgHandlerContext* context )
{
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->m_right_button_down=false;
	
	return Net::HANDLER_CONTINUE;
}

int CScriptDebugger::s_handle_mouse_on_screen( Net::MsgHandlerContext* context )
{
	// Set the m_mouse_on_screen flag so that mouse position messages are not ignored.
	CScriptDebugger *p_this=(CScriptDebugger*)context->m_Data;
	p_this->m_mouse_on_screen=true;
	
	// Initialise these flags to false so that the buttons have to be released first.
	p_this->m_left_button_down=false;
	p_this->m_right_button_down=false;
	
	return Net::HANDLER_CONTINUE;
}

void CScriptDebugger::EnqueueMessage(Net::MsgDesc *p_message)
{
	if (mp_server)
	{
		mp_server->EnqueueMessage(Net::HANDLE_ID_BROADCAST,p_message);
	}	
}

void CScriptDebugger::StreamMessage(Net::MsgDesc *p_message)
{
	if (mp_server)
	{
		mp_server->StreamMessage(	Net::HANDLE_ID_BROADCAST,
									p_message->m_Id,
									p_message->m_Length,
									p_message->m_Data,
									"",
									GameNet::vSCRIPT_DEBUG_GROUP );
	}									
}

void CScriptDebugger::send_sub_packet(Net::MsgDesc *p_message, uint32 size, char *p_source,
									  int packetIndex, int totalNumPackets)
{
	if (!mp_server)
	{
		return;
	}	
	
	Dbg_MsgAssert(size <= SPLIT_MESSAGE_MAX_SIZE,("Sub packet size too big"));
	
	static char sp_packet_data[sizeof(SSplitMessageHeader)+SPLIT_MESSAGE_MAX_SIZE];
	SSplitMessageHeader *p_header=(SSplitMessageHeader*)sp_packet_data;
		
	p_header->mOriginalMessageId=p_message->m_Id;
	p_header->mOriginalMessageSize=p_message->m_Length;
	p_header->mThisMessageId=m_split_message_id;
	p_header->mThisMessageIndex=packetIndex;
	p_header->mThisMessageSize=size;
	p_header->mTotalNumMessages=totalNumPackets;
	
	//printf("Id=%x, index=%d, total packets=%d\n",p_header->mThisMessageId,
	//p_header->mThisMessageIndex,
	//p_header->mTotalNumMessages);
	
	memcpy(p_header+1,p_source,size);
	
	Net::MsgDesc msg;
	msg.m_Id = GameNet::vMSG_ID_DBG_PACKET;
	msg.m_Data = sp_packet_data;
	msg.m_Length = sizeof(SSplitMessageHeader)+size;
	
	printf("Transmitting sub packet, size=%d\n",size);
	
	mp_server->EnqueueMessage(Net::HANDLE_ID_BROADCAST,&msg);
}

// This can be used in place of EnqueueMessage or StreamMessage when the message
// data is large, ie bigger than the maximum packet size that EnqueueMessage will allow,
// which is about 1300 bytes.
// The advantage of EnqueueMessage is that the messages arrive very quickly at the other end.
// The disadvantage is that it asserts for packet sizes > 1300 bytes or so.
// The advantage of StreamMessage is that it does allow for large packets.
// The disadvantage is that the packets can take a long time to arrive, since it guarantees that they
// arrive in the correct order, a feature that the script debugger does not always need.
// This means that composite object info cannot be sent every frame, because the packets start piling
// up at the PS2 side and it eventually asserts.

// SplitAndEnqueueMessage tries to get the advantages of both Enqueue and Stream.
// It allows for large packets because it splits them up into smaller packets and sends each of them
// using EnqueueMessage.
void CScriptDebugger::SplitAndEnqueueMessage(Net::MsgDesc *p_message)
{
	if (!mp_server)
	{
		return;
	}	

	printf("Splitting message of size %d\n",p_message->m_Length);
	
	int num_packets=p_message->m_Length / SPLIT_MESSAGE_MAX_SIZE;
	int total_num_packets=num_packets;
	int remainder=p_message->m_Length % SPLIT_MESSAGE_MAX_SIZE;
	if (remainder)
	{
		++total_num_packets;
	}
	
	char *p_source_data=(char*)p_message->m_Data;
	for (int i=0; i<num_packets; ++i)
	{
		send_sub_packet(p_message,SPLIT_MESSAGE_MAX_SIZE,p_source_data,i,total_num_packets);
		p_source_data+=SPLIT_MESSAGE_MAX_SIZE;
	}	
	
	if (remainder)
	{
		send_sub_packet(p_message,remainder,p_source_data,total_num_packets-1,total_num_packets);
	}
		
	++m_split_message_id;
}

void CScriptDebugger::process_net_tasks_loop( void )
{
	m_paused = true;
	while( m_paused )
	{
		Tmr::Time start_loop_time;

		start_loop_time = Tmr::GetTime();
		check_for_timeouts();
		mp_server->GetReceiveDataTask().vCall();
		mp_server->GetProcessDataTask().vCall();
		mp_server->GetSendDataTask().vCall();
		mp_server->GetNetworkMetricsTask().vCall();
		while(( Tmr::GetTime() - start_loop_time ) < 17 );
	}
}

void CScriptDebugger::ScriptDebuggerPause( void )
{
	//Dbg_Printf( "Pausing Game\n" );
	process_net_tasks_loop();
}

void CScriptDebugger::ScriptDebuggerUnpause( void )
{
	//Dbg_Printf( "Unpausing Game\n" );
	m_paused = false;
}

void CScriptDebugger::TransmitExceptionInfo(Obj::SException *p_exception, Obj::CCompositeObject* p_obj)
{
	#if 0
	if (mp_server)
	{
		//SCRIPT_DEBUGGER_BUFFER_SIZE
		gpDebugInfoBuffer[0]=(uint32)(p_obj->GetID());
		gpDebugInfoBuffer[1]=p_exception->ExceptionNameChecksum;
		gpDebugInfoBuffer[2]=p_exception->ScriptChecksum;
		
		Net::MsgDesc msg;
		msg.m_Data = gpDebugInfoBuffer;
		msg.m_Length = 3*4;
		msg.m_Id = GameNet::vMSG_ID_DBG_EXCEPTION_INFO;
		Dbg::CScriptDebugger::Instance()->EnqueueMessage(&msg);
	}
	#endif
}

// SPEEDOPT: This gets called by the CScript code whenever the script changes.
// It may need to be optimized ... or maybe not.
SWatchedScript *CScriptDebugger::GetScriptWatchInfo(uint32 checksum)
{
	for (int i=0; i<m_num_watched_scripts; ++i)
	{
		if (mp_watched_scripts[i].mChecksum==checksum)
		{
			return &mp_watched_scripts[i];
		}
	}		
	
	return NULL;
}	

void CScriptDebugger::check_for_timeouts( void )
{
	Lst::Search< Net::Conn > sh;
	for( Net::Conn *conn = mp_server->FirstConnection(&sh); conn; conn = mp_server->NextConnection(&sh) )
	{
		if (conn->GetNumResends() > 3)
		{
			conn->Invalidate();
			mp_server->TerminateConnection( conn );
			printf("Dropping PC connection due to exceeding 3 resends ...\n");
		}
	}
}

// A third of a second
#define DOUBLE_CLICK_TIME_WINDOW (150000000/3)
void CScriptDebugger::left_button_down()
{
	m_left_button_down=true;
	
	// If a double-click is done then automatically view the object.
	Tmr::CPUCycles time=Tmr::GetTimeInCPUCycles();
	if (time-m_state_when_left_pressed.mTime < DOUBLE_CLICK_TIME_WINDOW)
	{
		if (mp_currently_selected_object)
		{
			SViewObject v;
			v.mDoViewObject=true;
			v.mObjectId=mp_currently_selected_object->GetID();
			view_object(&v);
			
			m_left_button_down=false;
		}
	}
	m_state_when_left_pressed.mTime=time;
	
	m_state_when_left_pressed.mMouseX=m_mouse_x;
	m_state_when_left_pressed.mMouseY=m_mouse_y;
	m_state_when_left_pressed.mCamPhi=m_cam_phi;
	m_state_when_left_pressed.mCamTheta=m_cam_theta;
	
	if (0)//mp_viewed_object)
	{
		// Mouse clicks do not send object info when viewing an object. Instead, holding the left button
		// down allows the mouse to rotate the camera around the object.
	}
	else
	{
		// Run a script to make the mouse text do something so that we know the click has had an effect.
		Script::RunScript("DoMouseClickEffect");
		send_composite_object_info(mp_currently_selected_object);
	}	
}

void CScriptDebugger::left_button_up()
{
	m_left_button_down=false;
}

void CScriptDebugger::right_button_down()
{
	m_right_button_down=true;
	
	m_state_when_right_pressed.mMouseX=m_mouse_x;
	m_state_when_right_pressed.mMouseY=m_mouse_y;
	m_state_when_right_pressed.mCamRadius=m_cam_radius;
}

enum
{
	MOUSE_PICK_SEARCH_RADIUS=70
};
	
void CScriptDebugger::update_mouse_cursor()
{
	// The m_got_valid_mouse_position is to prevent glitches due to m_mouse_on_screen being
	// set but no mouse position message received yet.
	if (m_mouse_on_screen && m_got_valid_mouse_position)
	{
		Obj::CCompositeObject *p_ob=NULL;
															
		if (0)//mp_viewed_object)
		{
			// If we are viewing an object, then the mouse will be used to rotate the camera view
			// around the object, so don't search for a new object cos the text will just get in the way.
		}
		else
		{
			p_ob=s_find_nearest_object(	m_mouse_x,
										m_mouse_y,
										MOUSE_PICK_SEARCH_RADIUS);
		}
		
		if (p_ob != mp_currently_selected_object)
		{
			s_change_render_mode(mp_currently_selected_object,m_original_object_render_mode);
			
			mp_currently_selected_object=p_ob;
			m_original_object_render_mode=s_change_render_mode(p_ob,Nx::vBBOX);
		}
		
		if (mp_currently_selected_object)
		{
			Script::CStruct *p_struct=new Script::CStruct;
			p_struct->AddInteger("x",m_mouse_x);
			p_struct->AddInteger("y",m_mouse_y);
			p_struct->AddString("text",Script::FindChecksumName(mp_currently_selected_object->GetID()));
			Script::RunScript("SetMouseText",p_struct);
			delete p_struct;
		}	
		else
		{
			Script::RunScript("DestroyMouseText");
		}
	}		
}

void CScriptDebugger::switch_off_mouse_cursor()
{
	// Destroy the mouse cursor screen elements, then set m_mouse_on_screen to false 
	// so that any late mouse position messages get ignored.
	Script::RunScript("DestroyMouseCursor");
	m_mouse_on_screen=false;
	m_got_valid_mouse_position=false;
	// Reset these too so that there is no confusion.
	m_left_button_down=false;
	m_right_button_down=false;
	
	
	if (mp_currently_selected_object)
	{
		s_change_render_mode(mp_currently_selected_object,m_original_object_render_mode);
	}
	mp_currently_selected_object=NULL;	
	m_original_object_render_mode=Nx::vNONE;
	
}

void CScriptDebugger::do_any_every_frame_object_updates()
{
	++m_frames_since_last_object_update;
	
	// Restrict the rate at which data is sent to be less than 200K per second, as a safeguard.
	if (m_object_update_total_bytes_sent * 60 / m_frames_since_last_object_update > 200000)
	{
		return;
	}
		
	m_object_update_total_bytes_sent=0;
	m_frames_since_last_object_update=0;
	
	Lst::Search<Obj::CObject> sh;
	Obj::CCompositeObject *p_ob = (Obj::CCompositeObject*) sh.FirstItem( Obj::CCompositeObjectManager::Instance()->GetRefObjectList() );
	while (p_ob)
	{
		if (p_ob->GetDebuggerEveryFrameFlag())
		{
			m_object_update_total_bytes_sent+=send_composite_object_info(p_ob);
		}
		p_ob = (Obj::CCompositeObject*)sh.NextItem();
	}
}

void CScriptDebugger::stop_all_every_frame_object_updates()
{
	Lst::Search<Obj::CObject> sh;
	Obj::CCompositeObject *p_ob = (Obj::CCompositeObject*) sh.FirstItem( Obj::CCompositeObjectManager::Instance()->GetRefObjectList() );
	while (p_ob)
	{
		p_ob->SetDebuggerEveryFrameFlag(false);
		p_ob = (Obj::CCompositeObject*)sh.NextItem();
	}
}

void CScriptDebugger::setup_default_camera_viewing_position()
{
	int type=SKATE_TYPE_UNDEFINED;
	if (mp_viewed_object)
	{
		type=mp_viewed_object->GetType();
	}
		
	switch (type)
	{
	case SKATE_TYPE_PED:
	case SKATE_TYPE_SKATER:
		m_cam_radius=100.0f;
		m_cam_theta=1.107f;
		m_cam_focus_offset=40;
		break;
	case SKATE_TYPE_CAR:
		m_cam_radius=200.0f;
		m_cam_theta=1.326f;
		m_cam_focus_offset=40;
		break;
	default:
		m_cam_radius=100.0f;
		m_cam_theta=1.107f;
		m_cam_focus_offset=0;
		break;
	}
	m_cam_phi=0.0f;

	m_state_when_left_pressed.mCamPhi=m_cam_phi;
	m_state_when_left_pressed.mCamTheta=m_cam_theta;
	m_state_when_right_pressed.mCamRadius=m_cam_radius;
	
	// If the viewed object has a bounding radius defined then add that to the initial camera radius
	// too, so that the camera does not start off inside big things.
	if (mp_viewed_object)
	{
		Obj::CModelComponent *p_model_component=(Obj::CModelComponent*)mp_viewed_object->GetComponent(CRCD(0x286a8d26,"Model"));
		if (p_model_component)
		{
			Nx::CModel *p_model=p_model_component->GetModel();
			if (p_model)
			{
				Mth::Vector r=p_model->GetBoundingSphere();
				
				// Apply any scaling, needed for the big alligator in the zoo.
				Mth::Vector scale=p_model->GetScale();
				float s=scale.GetX();
				if (scale.GetY()>s) s=scale.GetY();
				if (scale.GetZ()>s) s=scale.GetZ();
				
				m_cam_radius+=r.Length()*s;
			}	
		}
	}	
}

// This will override the camera position and angles so as to view the mp_viewed_object.
void CScriptDebugger::update_camera()
{
	// Note: mp_viewed_object is a smart pointer, so will become NULL if the object dies.
	if (!mp_viewed_object)
	{
		// Nothing to see
		return;
	}	

	// Get the skater's camera.
	Mdl::Skate * p_skate_mod =  Mdl::Skate::Instance();
	Obj::CSkater *p_skater = p_skate_mod->GetSkater(0);
	if (!p_skater)
	{
		// No skater
		return;
	}
		
	Gfx::Camera *p_skater_cam=p_skater->GetActiveCamera();
	if (!p_skater_cam)
	{
		// No skater camera
		return;
	}	


	if (m_mouse_on_screen && m_got_valid_mouse_position)
	{
		if (m_left_button_down)
		{
			m_cam_theta=m_state_when_left_pressed.mCamTheta+Script::GetFloat("MouseRotationUpDownFactor")*(m_mouse_y-m_state_when_left_pressed.mMouseY);
			
			if (m_cam_theta > Mth::PI-0.01f)
			{
				m_cam_theta = Mth::PI-0.01f;
			}	
			else if (m_cam_theta < 0.01f)
			{
				m_cam_theta = 0.01f;
			}	
			
			m_cam_phi=m_state_when_left_pressed.mCamPhi+Script::GetFloat("MouseRotationLeftRightFactor")*(m_mouse_x-m_state_when_left_pressed.mMouseX);
			if (m_cam_phi > Mth::PI*2.0f)
			{
				m_cam_phi -= Mth::PI*2.0f;
			}
			else if (m_cam_phi < -Mth::PI*2.0f)
			{
				m_cam_phi += Mth::PI*2.0f;
			}
		}
			
		if (m_right_button_down)
		{
			m_cam_radius=m_state_when_right_pressed.mCamRadius+Script::GetFloat("MouseZoomFactor")*(m_mouse_y-m_state_when_right_pressed.mMouseY);
			
			if (m_cam_radius < 0.01f)
			{
				m_cam_radius=0.01f;
			}	
		}
	}

	Mth::Vector cam_radius_vector;
	cam_radius_vector[Y]=cosf(m_cam_theta);
	float s=sinf(m_cam_theta);
	cam_radius_vector[X]=s*cosf(m_cam_phi);
	cam_radius_vector[Z]=s*sinf(m_cam_phi);
	cam_radius_vector.Normalize(); // Just to be sure
	cam_radius_vector *= m_cam_radius;

	// Rotate using the object's matrix so that the camera's frame of
	// reference is that of the object, otherwise the camera will not turn with the object.
	cam_radius_vector *= mp_viewed_object->GetDisplayMatrix();
	
	Mth::Vector focus_pos=mp_viewed_object->GetPos();
	focus_pos[Y]+=m_cam_focus_offset;
	Mth::Vector camera_pos=focus_pos+cam_radius_vector;

	camera_pos[W] = 1.0f;
	p_skater_cam->SetPos(camera_pos);

	Mth::Matrix mat;
	mat.Ident();
	mat[Z]=focus_pos-camera_pos;
	mat[Z].Normalize();
	mat[X]	= Mth::CrossProduct( mat[Y], mat[Z] );
	mat[X].Normalize();
	mat[Y]	= Mth::CrossProduct( mat[Z], mat[X] );
	mat[Y].Normalize();

	Mth::Matrix& frame_matrix = p_skater_cam->GetMatrix();
	frame_matrix[X][X] = -mat[0][0];
	frame_matrix[X][Y] = -mat[0][1];
	frame_matrix[X][Z] = -mat[0][2];
	frame_matrix[Y][X] = mat[1][0];
	frame_matrix[Y][Y] = mat[1][1];
	frame_matrix[Y][Z] = mat[1][2];
	frame_matrix[Z][X] = -mat[2][0];
	frame_matrix[Z][Y] = -mat[2][1];
	frame_matrix[Z][Z] = -mat[2][2];
}

void CScriptDebugger::s_logic_code ( const Tsk::Task< CScriptDebugger >& task )
{
	CScriptDebugger&	mdl = task.GetData();
	Dbg_AssertType ( &task, Tsk::Task< CScriptDebugger > );
	
	mdl.check_for_timeouts();

	mdl.update_camera();

	mdl.update_mouse_cursor();	

	mdl.do_any_every_frame_object_updates();
	mdl.send_any_single_step_object_info();
	
	switch (mdl.m_state)
	{
	case SCRIPT_DEBUGGER_STATE_IDLE:
	{
		break;
	}
			
	default:
		break;
	}			
}

} // namespace Dbg
#endif
