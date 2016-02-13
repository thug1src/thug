///////////////////////////////////////////////////////////////////////////////////////
//
// script.cpp		KSH 1 Nov 2001
//
// CScript class member functions and misc script functions.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <gel/scripting/script.h>
#include <gel/scripting/scriptcache.h>
#include <gel/scripting/tokens.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/parse.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/component.h>
#include <gel/scripting/utils.h>
#include <core/crc.h>
#include <gel/object/basecomponent.h>

#include <sk/modules/skate/skate.h> // for SKATE_TYPE_SKATER
#include <gel/scripting/debugger.h>
#include <gel/net/client/netclnt.h> // For Net::Client class
#ifndef __PLAT_WN32__
#include <sk/gamenet/gamenet.h> // For GameNet::Manager
#endif
#include <sk/gamenet/scriptdebugger.h>

#ifdef	__SCRIPT_EVENT_TABLE__		
#include <gel/Event.h>
#include <gel/objtrack.h>
#endif

//char foo[sizeof(Script::SReturnAddress)/0];

//#define SEND_SCRIPT_NAMES_TO_DEBUGGER

DefinePoolableClass(Script::CScript);

namespace Obj
{
	#ifdef __NOPT_ASSERT__
	extern bool DebugSkaterScripts;
	#endif

}

namespace Script
{

static CScript * sCurrentlyUpdating = NULL;

// Parse.cpp needs this, for getting the script pointer to send to any cfunc it
// encounters when evaluating an expression.
// It also uses it when processing RandomNoRepeat tokens.
CScript *GetCurrentScript()
{
	return sCurrentlyUpdating;
}
	
#ifdef	__NOPT_ASSERT__
// for debugging, return the info of script we are currently updating
// as the function that Asserts by it might not have a pointer 
// to the script 									  
const char  *GetCurrentScriptInfo()
{
	if (sCurrentlyUpdating)
	{
		return sCurrentlyUpdating->GetScriptInfo();
	}
	else
	{
		return "No Script currently updating";
	}
}

#ifdef SEND_SCRIPT_NAMES_TO_DEBUGGER
static void s_send_script_name(uint32 script_name, int numReturnAddresses)
{
	static uint32 sp_ignored_script_names[]=
	{
		CRCC(0x1f8909cb,"timeupscript"),
		CRCC(0x1aef674f,"game_update"),
		CRCC(0x18b3b291,"waitonegameframe"),
		CRCC(0x7c4264dd,"checkforswitchvehicles"),
		CRCC(0x98f4c454,"just_coasting"),
		CRCC(0x30e72e33,"do_random_effect"),
		CRCC(0x7737d7b8,"do_random_effect2"),
		CRCC(0x99443e63,"do_blur_effect"),
		CRCC(0x3e6ab5cb,"do_blur_effect_accept"),
		CRCC(0x9c9dfcf8,"disable_two_player_option"),
		CRCC(0x380ec7a7,"playbrakeanim"),
		CRCC(0x838116e9,"playbrakeidle"),
		CRCC(0x7ae0d8aa,"handbrake"),
		CRCC(0x9f6a63e3,"playturnanimorturnidle"),
		CRCC(0x642587a0,"cessbrake"),
		CRCC(0x603ea7f3,"waitanimfinished"),
		CRCC(0x76792736,"checkfornetbrake"),
		CRCC(0xa4f522e9,"parked_make_set_menu_item"),
		CRCC(0xbeed50b6,"make_text_sub_menu_item"),
		CRCC(0x277b7b33,"parked_make_piece_menu_item"),
		CRCC(0x3ee4627d,"doapush"),
		CRCC(0x493a52f,"helper_text_update_element"),
		CRCC(0x5479735a,"show_panel_message"),
		CRCC(0x7a449180,"DestroyMemStatsScreenElements"),
		CRCC(0x901fefcf,"setexception"),
		CRCC(0xe9242afd,"vibrateoff"),
		CRCC(0xc4c8c400,"switchonboard"),
		CRCC(0xf8d27a88,"switchoffboard"),
		CRCC(0x1195c40a,"bloodparticlesoff"),
		CRCC(0x1946686b,"skaterbloodoff"),
		CRCC(0x243e716,"verifyparam"),
		CRCC(0x2c0c9e7b,"onexceptionrun"),
		CRCC(0x64df5e4c,"WaitAnimWhilstChecking"),
		CRCC(0xcf14cc35,"ClothingLandBounce"),
		CRCC(0x46f7302f,"OnGroundExceptions"),
		CRCC(0x500eb95b,"InAirExceptions"),
		CRCC(0x10da1c88,"ClearExceptions"),
		CRCC(0xaaef4fb5,"ClearOnExceptionRun"),
		CRCC(0xac281817,"model_hide_geom"),
		CRCC(0x3b9039a5,"process_cas_command"),
		CRCC(0xdea7fe56,"goal_editor_update_cursor_position"),
	};
	
	uint32 *p_ignored_script_names=sp_ignored_script_names;
	for (uint32 i=0; i<sizeof(sp_ignored_script_names)/sizeof(uint32); ++i)
	{
		if (*p_ignored_script_names++==script_name)
		{
			return;
		}	
	}

	uint32 p_buf[2];
	p_buf[0]=script_name;
	p_buf[1]=numReturnAddresses;
	
	// Tell the debugger that this script is being run.
	Net::MsgDesc msg;
	msg.m_Data = p_buf;
	msg.m_Length = 8;
	msg.m_Id = GameNet::vMSG_ID_DBG_RUNNING_SCRIPT;
	Dbg::CScriptDebugger::Instance()->StreamMessage(&msg);
}
#endif

#endif


static CScript *sp_scripts=NULL;
static uint32 s_num_cscripts=0;

static bool	s_done_one_per_frame;

int CScript::s_next_unique_id = 0;

uint32 GetNumCScripts()
{
	return s_num_cscripts;
}
	
// For use when external functions want to step through all existing scripts.
// Pass it NULL to get the first CScript.
// Should probably eventually change all the script code to use the iterators in the 
// standard container library at some point I guess.
CScript *GetNextScript(CScript *p_script)
{
	if (p_script==NULL)
	{
		// They want the first CScript
		return sp_scripts;
	}
	else
	{
		return p_script->mp_next;
	}
}

CScript::CScript()
{
	// Zero all the members. CScript is not derived from CClass so we don't get the auto-zeroing.
	uint32 size=sizeof(CScript);
	Dbg_MsgAssert((size&3)==0,("sizeof(CScript) not a multiple of 4 ??"));
	size>>=2;
	uint32 *p_longs=(uint32*)this;
	for (uint32 i=0; i<size; ++i)
	{
		*p_longs++=0;
	}	
	
	// Add to the linked list.			  
	mp_next=sp_scripts;
	if (sp_scripts) 
	{
		sp_scripts->mp_previous=this;
	}	
	sp_scripts=this;

	++s_num_cscripts;

	m_unique_id = s_next_unique_id++;
	
	mNode = -1;
	
	#ifdef __NOPT_ASSERT__										
	m_last_instruction_time_taken=-1.0f;
	m_total_instruction_time_taken=0.0f;
	#endif
	
	#if	0
// if we go over 100 scripts, tell us why
	static uint32 max = 100;
	if (s_num_cscripts > max)
	{
		max = s_num_cscripts;
		printf ("Record number of scripts: %d of %d\nvvvvvvvvvvvvvvvvvvvvvvvvvvv\n",CScript::SGetNumUsedItems(),CScript::SGetTotalItems());
		Script::DumpScripts();
		DumpUnwindStack(20,0);
		printf (" ^^^^^^^^^^^^^^^^^^^^^^^^\nRecord number of scripts: %d of %d\n",CScript::SGetNumUsedItems(),CScript::SGetTotalItems());

	}
	#endif

#ifdef	__SCRIPT_EVENT_TABLE__		
	mp_event_handler_table = NULL;
#endif	 

	mp_return_addresses=mp_return_addresses_small_buffer;
	mp_loops=mp_loops_small_buffer;
}

CScript::~CScript()
{
	ClearScript();
	if (mpCallbackScriptParams)
	{
		delete mpCallbackScriptParams;
	}
	// Check to see if the script has already been deleted
	// perhaps we are deleteing it twice???
	
	Dbg_MsgAssert(mp_next != (CScript *)-1,("mp_next is -1 in script we are trying to delete!!!"));
	Dbg_MsgAssert(mp_previous != (CScript *)-1,("mp_previous is -1 in script we are trying to delete!!!"));
	
	// Remove from the linked list.
	if (mp_previous==NULL) 
	{
		sp_scripts=mp_next;
	}	
	if (mp_next) 
	{
		Dbg_MsgAssert(mp_next!=mp_previous,("mp_next same as mp_prev for script we are deleting"));
		mp_next->mp_previous=mp_previous;
	}	
	if (mp_previous) 
	{
		Dbg_MsgAssert(mp_next!=mp_previous,("mp_next same as mp_prev for script we are deleting"));
		mp_previous->mp_next=mp_next;
	}	

	#ifdef	__NOPT_ASSERT__	
	mp_next = (CScript *)-1;
	mp_previous = (CScript *)-1;
	#endif
	
	// Release any stored RandomNoRepeat or RandomPermute info.
	ReleaseStoredRandoms(this);

	#ifdef	__NOPT_ASSERT__	
	#ifndef __PLAT_WN32__
	if (m_being_watched_in_debugger)
	{
		// Tell the debugger that this script has died.
		Net::MsgDesc msg;
		msg.m_Data = &m_unique_id;;
		msg.m_Length = 4;
		msg.m_Id = GameNet::vMSG_ID_DBG_SCRIPT_DIED;
		Dbg::CScriptDebugger::Instance()->StreamMessage(&msg);
	}	
	#endif
	#endif

#ifdef	__SCRIPT_EVENT_TABLE__		
	if (mp_event_handler_table)
	{
		// Remove the references to the event handler from the tracker

//		mp_event_handler_table->unregister_all(this);
		ClearEventHandlerTable();   // Need this to clean up structs and stuff
	
		if (mp_event_handler_table->m_in_immediate_use_counter)
		{
			// table still in use by its pass_event() function,
			// so leave it up to that function to destroy the table
			mp_event_handler_table->m_valid = false;
		}
		else
		{
			delete mp_event_handler_table;
		}
	}
#endif

	Dbg_MsgAssert(s_num_cscripts,("Zero s_num_cscripts"));
	--s_num_cscripts;
}

#ifdef __NOPT_ASSERT__
#ifndef __PLAT_WN32__

// This gets called whenever mScriptChecksum is changed.
void CScript::check_if_needs_to_be_watched_in_debugger()
{
	Dbg::SWatchedScript *p_watched_script_info=Dbg::CScriptDebugger::Instance()->GetScriptWatchInfo(mScriptChecksum);
	if (p_watched_script_info)
	{
		WatchInDebugger(p_watched_script_info->mStopScriptImmediately);
	}	
}

uint32 *CScript::write_callstack_entry( uint32 *p_buf, int bufferSize, 
										uint32 scriptNameChecksum, 
										uint8 *p_PC,
										CStruct *p_params,
										Obj::CObject *p_ob)
{
	Dbg_MsgAssert(bufferSize >= 8,("write_callstack_entry buffer overflow"));
	*p_buf++=scriptNameChecksum;
	*p_buf++=Script::GetLineNumber(p_PC);
	bufferSize-=8;
		
	// Find out the source file and write it in.
	const char *p_source_file_name="";
	CSymbolTableEntry *p_sym=LookUpSymbol(scriptNameChecksum);
	if (p_sym)
	{
		p_source_file_name=FindChecksumName(p_sym->mSourceFileNameChecksum);
	}
	int len=strlen(p_source_file_name)+1;
	len=(len+3)&~3;
	Dbg_MsgAssert(bufferSize >= len,("write_callstack_entry buffer overflow"));	
	strcpy((char*)p_buf,p_source_file_name);
	p_buf+=len/4;
	bufferSize-=len;

	Dbg_MsgAssert(bufferSize >= 4,("write_callstack_entry buffer overflow"));
	*p_buf++=(uint32)p_ob;
	bufferSize -= 4;
	if (p_ob)
	{
		Dbg_MsgAssert(bufferSize >= 12,("write_callstack_entry buffer overflow"));
		*p_buf++=p_ob->GetType();
		*p_buf++=p_ob->GetID();
		*p_buf++=p_ob->MainScriptIs(this);
		bufferSize -= 12;
		
		// Write out the object's tags.
		int bytes_written=Script::WriteToBuffer(p_ob->GetTags(),(uint8*)p_buf,bufferSize);
		bytes_written=(bytes_written+3)&~3;
		Dbg_MsgAssert(bufferSize >= bytes_written,("write_callstack_entry buffer overflow"));
		p_buf+=bytes_written/4;
		bufferSize-=bytes_written;
	}
	
	// Write out the parameters.
	int bytes_written=Script::WriteToBuffer(p_params,(uint8*)p_buf,bufferSize);
	bytes_written=(bytes_written+3)&~3;
	Dbg_MsgAssert(bufferSize >= bytes_written,("write_callstack_entry buffer overflow"));
	p_buf+=bytes_written/4;
	
	return p_buf;
}

// Sets the comment string, which the script debugger displays.
// This is so that the c-code can describe where the script was created from.
void CScript::SetCommentString(const char *p_comment)
{
	int len=strlen(p_comment);
	if (len>MAX_COMMENT_STRING_CHARS)
	{
		len=MAX_COMMENT_STRING_CHARS;
	}
	for (int i=0; i<len; ++i)
	{
		mp_comment_string[i]=p_comment[i];
	}
	mp_comment_string[len]=0;
}

void CScript::SetOriginatingScriptInfo(int lineNumber, uint32 scriptName)
{
	m_originating_script_line_number=lineNumber;
	m_originating_script_name=scriptName;
}

// This transmits lots of info about the script to the script debugger running on the PC
void CScript::TransmitInfoToDebugger(bool definitely_transmit)
{
	uint32 *p_buf=Dbg::gpDebugInfoBuffer;
	
	int space_left=Dbg::SCRIPT_DEBUGGER_BUFFER_SIZE;
	
	Dbg_MsgAssert(space_left >= 16,("Dbg::gpDebugInfoBuffer overflow"));
	// Stuff that identifies the CScript
	*p_buf++=(uint32)this;
	*p_buf++=m_unique_id;
	*(float*)p_buf=m_last_instruction_time_taken;
	++p_buf;
	*(float*)p_buf=m_total_instruction_time_taken;
	++p_buf;
	
	// The originating script info, ie which script this got spawned from if it was created
	// by a SpawnScript command. Will be two zeros if no such info is available.
	*p_buf++=m_originating_script_line_number;
	*p_buf++=m_originating_script_name;
	space_left-=24;

	// Write out the comment string.	
	int comment_len=strlen(mp_comment_string)+1;
	int comment_space_needed=(comment_len+3)&~3;
	Dbg_MsgAssert(space_left >= comment_space_needed,("Dbg::gpDebugInfoBuffer overflow"));
	char *p_source=mp_comment_string;
	char *p_dest=(char*)p_buf;
	for (int i=0; i<comment_len; ++i)
	{
		*p_dest++=*p_source++;
	}	
	p_buf=(uint32*)(((char*)p_buf)+comment_space_needed);
	space_left-=comment_space_needed;
	
		
	// Write out the function parameters.
	int structure_bytes_written=Script::WriteToBuffer(mp_function_params,(uint8*)p_buf,space_left);
	structure_bytes_written=(structure_bytes_written+3)&~3;
	Dbg_MsgAssert(space_left >= structure_bytes_written,("Dbg::gpDebugInfoBuffer overflow"));
	p_buf+=structure_bytes_written/4;
	space_left-=structure_bytes_written;
	
	// Write out the callstack
	Dbg_MsgAssert(space_left >= 4,("Dbg::gpDebugInfoBuffer overflow"));
	// Adding one because the current script is considered the last entry in the callstack
	// when displayed in the debugger.
	*p_buf++=m_num_return_addresses+1;
	space_left-=4;
	
	uint32 *p_old=p_buf;
    for (int i=0; i<m_num_return_addresses; ++i)
    {
		p_buf=write_callstack_entry(p_buf,space_left,
									mp_return_addresses[i].mScriptNameChecksum,
									mp_return_addresses[i].mpReturnAddress,
									mp_return_addresses[i].mpParams,
									mp_return_addresses[i].mpObject);
		space_left-=(p_buf-p_old)*4;
		p_old=p_buf;											
    }    
	// Output the current script as though it is the last entry in the callstack
	// so that it can be displayed simply in the debugger using the same code that
	// displayed the other callstack entries.
	p_buf=write_callstack_entry(p_buf,space_left,
								mScriptChecksum,
								mp_pc,
								mp_params,
								mpObject.Convert());
	space_left-=(p_buf-p_old)*4;


	Dbg_MsgAssert(space_left >= 20,("Dbg::gpDebugInfoBuffer overflow"));
	
	// Whether spawned or not
	*p_buf++=mIsSpawned;
	
	// Wait state
	*p_buf++=m_wait_type;
	
	// Paused or not
	*p_buf++=mPaused;
	
	// Whether 'session specific' or not
	*p_buf++=mNotSessionSpecific;
	
	// Script debugger needs this so that it knows to expand and highlight the
	// last entry in the callstack when it receives the info.
	*p_buf++=m_single_step_mode;
	
	space_left-=20;
	
	int message_size=((uint8*)p_buf)-((uint8*)Dbg::gpDebugInfoBuffer);
	Dbg_MsgAssert(message_size <= Dbg::SCRIPT_DEBUGGER_BUFFER_SIZE,("Script info message too big !"));
	Dbg_MsgAssert(message_size+space_left == Dbg::SCRIPT_DEBUGGER_BUFFER_SIZE,("What ??"));
	
	uint32 checksum=Crc::GenerateCRCCaseSensitive((const char *)Dbg::gpDebugInfoBuffer,message_size);
	if (checksum != m_last_transmitted_info_checksum || definitely_transmit)
	{
		printf("CScript id=0x%08x Sending script info, message size=%d\n",m_unique_id,message_size);
		
		Net::MsgDesc msg;
		msg.m_Data = Dbg::gpDebugInfoBuffer;
		msg.m_Length = message_size;
		msg.m_Id = GameNet::vMSG_ID_DBG_CSCRIPT_INFO;
		Dbg::CScriptDebugger::Instance()->StreamMessage(&msg);
		
		m_last_transmitted_info_checksum=checksum;
	}	
}

void CScript::TransmitBasicInfoToDebugger()
{
	uint32 *p_buf=Dbg::gpDebugInfoBuffer;
	
	*p_buf++=m_unique_id;
	*p_buf++=mScriptChecksum;
	*p_buf++=Script::GetLineNumber(mp_pc);
		
	// Find out the source file and write it in.
	const char *p_source_file_name="";
	CSymbolTableEntry *p_sym=LookUpSymbol(mScriptChecksum);
	if (p_sym)
	{
		p_source_file_name=FindChecksumName(p_sym->mSourceFileNameChecksum);
	}
	int len=strlen(p_source_file_name)+1;
	len=(len+3)&~3;
	strcpy((char*)p_buf,p_source_file_name);
	p_buf+=len/4;

	int message_size=((uint8*)p_buf)-((uint8*)Dbg::gpDebugInfoBuffer);
	Dbg_MsgAssert(message_size <= Dbg::SCRIPT_DEBUGGER_BUFFER_SIZE,("Script info message too big !"));
	
	//printf("Sending vMSG_ID_DBG_BASIC_CSCRIPT_INFO\n");
	Net::MsgDesc msg;
	msg.m_Data = Dbg::gpDebugInfoBuffer;
	msg.m_Length = message_size;
	msg.m_Id = GameNet::vMSG_ID_DBG_BASIC_CSCRIPT_INFO;
	Dbg::CScriptDebugger::Instance()->StreamMessage(&msg);
}

void CScript::WatchInDebugger(bool stopScriptImmediately)
{
	m_being_watched_in_debugger=true;
	
	if (stopScriptImmediately)
	{
		m_single_step_mode=STEP_INTO;
	}
	else
	{
		m_single_step_mode=OFF;
	}	
}

void CScript::StopWatchingInDebugger()
{
	m_being_watched_in_debugger=false;
}	

void CScript::DebugStop()
{
	bool was_running=m_single_step_mode==OFF;
	m_single_step_mode=WAITING;
	
	if (was_running)
	{
		// This is so that the debugger registers the fact that m_single_step_mode has changed
		// and hence updates the window to indicate that the script has stopped.
		TransmitInfoToDebugger();
	}	
}

void CScript::DebugStepInto()
{
	m_single_step_mode=STEP_INTO;
	Dbg::CScriptDebugger::Instance()->ScriptDebuggerUnpause();	
}

void CScript::DebugStepOver()
{
	m_single_step_mode=STEP_OVER;
	Dbg::CScriptDebugger::Instance()->ScriptDebuggerUnpause();	
}

void CScript::DebugGo()
{
	m_single_step_mode=OFF;
	Dbg::CScriptDebugger::Instance()->ScriptDebuggerUnpause();	
}
#endif // #ifndef __PLAT_WN32__
#endif // #ifdef __NOPT_ASSERT__

void CScript::SetWait(EWaitType type, Obj::CBaseComponent *p_component)
{
	m_wait_type=type;
	Dbg_MsgAssert(mp_wait_component==NULL,("\n%s\nExpected mp_wait_component to be NULL here?",GetScriptInfo()));
	mp_wait_component=p_component;
}

void CScript::ClearWait()
{
	m_wait_type=WAIT_TYPE_NONE;
	mp_wait_component=NULL;
}

void CScript::Block()
{
	m_wait_type=WAIT_TYPE_BLOCKED;
}

void CScript::UnBlock()
{
	m_wait_type=WAIT_TYPE_NONE;
}

bool CScript::getBlocked()
{
	return m_wait_type==WAIT_TYPE_BLOCKED;
}

bool CScript::GotScript()
{
	return mp_pc?true:false;
}

void CScript::SwitchOnIfDebugging()
{
#ifdef __NOPT_ASSERT__
	m_if_debug_on=true;
#endif
}

void CScript::SwitchOffIfDebugging()
{
#ifdef __NOPT_ASSERT__
	m_if_debug_on=false;
#endif
}

void CScript::set_script(uint32 scriptChecksum, uint8 *p_script, CStruct *p_params, Obj::CObject *p_object)
{
	#ifdef	__NOPT_ASSERT__	
	#ifdef SEND_SCRIPT_NAMES_TO_DEBUGGER
	// Tell the debugger that this script is begin run.
	s_send_script_name(scriptChecksum,0);
	#endif
	#endif
	
	#ifdef __NOPT_ASSERT__
	if (Obj::DebugSkaterScripts)
	{
		if (p_object && p_object->GetType()==SKATE_TYPE_SKATER)
		{
			printf("%d: ############### %s ###############  [%i]\n",(int)Tmr::GetRenderFrame(),FindChecksumName(scriptChecksum),m_unique_id);
		}
	}	
	#endif

	CStruct *p_params_copy=MakeParamsSafeFromDeletionByClearScript(p_params);
	
	// Reset the script.
	ClearScript();
	
	Dbg_MsgAssert(mp_function_params==NULL,("mp_function_params not NULL"));
	mp_function_params=new CStruct;
	
	#ifdef __NOPT_ASSERT__ 
	// This is so that the structure can print line number info in case one of
	// the CStruct member function asserts goes off.
	mp_function_params->SetParentScript(this);
	#endif
		
	mScriptChecksum=scriptChecksum;
	#ifdef __NOPT_ASSERT__ 
	check_if_needs_to_be_watched_in_debugger();
	#endif	
    mpObject=p_object;
	
	Dbg_MsgAssert(mp_params==NULL,("mp_params not NULL"));
	mp_params=new CStruct;
	#ifdef __NOPT_ASSERT__ 
	// This is so that the structure can print line number info in case one of
	// the CStruct member function asserts goes off.
	mp_params->SetParentScript(this);
	#endif
	
	if (p_script)
	{
		mp_pc=p_script;	
	}
	else
	{
		Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
		Dbg_MsgAssert(p_script_cache,("NULL p_script_cache"));
	
		mp_pc=p_script_cache->GetScript(scriptChecksum);
		if (!mp_pc)
		{
			#ifdef	__NOPT_ASSERT__
			if (Script::GetInteger(CRCD(0x22d1f89,"AssertOnMissingScripts")))
			{
				Dbg_MsgAssert(false, ("Script %s not found.",FindChecksumName(scriptChecksum)));
			}
			#endif
			if (p_params_copy)
			{
				delete p_params_copy;
			}
			ClearScript();
			ClearEventHandlerTable();
			return;	
		}
	}
	
	// Load the default parameters into mp_params
	mp_pc=AddComponentsUntilEndOfLine(mp_params, mp_pc);
		
	// Now merge what is in p_params onto mp_params.
	if (p_params_copy)
	{
		mp_params->AppendStructure(p_params_copy);
		delete p_params_copy;
	}
	else
	{
		mp_params->AppendStructure(p_params);
	}
	
	if (GetOnExitScriptChecksum())
	{
		// Pass in the name of the exception so that certain exceptions can be ignored.
		Script::CStruct *pFoo=new Script::CStruct;
// Mick - that has no meaning now, as this is triggered by everything
//		pFoo->AddComponent(NONAME, ESYMBOLTYPE_NAME, (int)p_entry->exception);
		// RunScript(GetOnExitScriptChecksum(), pFoo, mpObject );
		// Dan: interrupt instead of run
		uint32 checksum = GetOnExitScriptChecksum();
		SetOnExitScriptChecksum(0);	// clear it once it has been run
		Interrupt(checksum, pFoo);
		delete pFoo;
	}
}

void CScript::SetScript(const SStructScript *p_structScript, CStruct *p_params, Obj::CObject *p_object)
{
	Dbg_MsgAssert(p_structScript,("NULL p_structScript"));
	Dbg_MsgAssert(p_structScript->mNameChecksum,("Zero p_structScript->mNameChecksum"));
	Dbg_MsgAssert(p_structScript->mpScriptTokens,("NULL p_structScript->mpScriptTokens"));
	
	// Calculate the size of the script
	uint32 size=SkipOverScript(p_structScript->mpScriptTokens)-p_structScript->mpScriptTokens;
	
	// Allocate a buffer and make a copy of the script (since the source may get deleted)
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	uint8 *p_new_script=(uint8*)Mem::Malloc(size);
	Mem::Manager::sHandle().PopContext();
	
	uint8 *p_source=p_structScript->mpScriptTokens;
	uint8 *p_dest=p_new_script;
	for (uint32 i=0; i<size; ++i)
	{
		*p_dest++=*p_source++;
	}	
	
	// Call set_script, which will set up mp_pc etc.
	set_script(p_structScript->mNameChecksum,p_new_script,p_params,p_object);
	
	// Now set mp_struct_script, so that the buffer can be deleted later.
	// Note: The setting of mp_struct_script cannot be done before set_script, because set_script calls 
	// ClearScript, which would have deleted mp_struct_script if it was not NULL.
	mp_struct_script=p_new_script;
}

void CScript::SetScript(uint32 scriptChecksum, CStruct *p_params, Obj::CObject *p_object)
{
	set_script(scriptChecksum,NULL,p_params,p_object);
}

void CScript::SetScript(const char *p_scriptName, CStruct *p_params, Obj::CObject *p_object)
{
	SetScript(Crc::GenerateCRCFromString(p_scriptName),p_params,p_object);
}

CStruct *CScript::MakeParamsSafeFromDeletionByClearScript(CStruct *p_params)
{
	// It may be that p_params is mp_params or mp_function_params, or is a substructure of one
	// of those.
	// For example, when a manual is triggered, the passed p_params will be mp_function_params.
	// Since ClearScript() deletes both mp_params and mp_function_params,
	// this will result in p_params getting deleted too.
	// So if either mp_params or mp_function_params references p_params, store the contents
	// of p_params in p_new_params & return it.
	// This is a speed optimization, because it would be slow to always store the contents of
	// p_params. (Copying structures can be slow)
	CStruct *p_new_params=NULL;
	if (p_params)
	{
		if ((mp_function_params && mp_function_params->References(p_params)) || 
			(mp_params && mp_params->References(p_params)))
		{
			p_new_params=new CStruct;
			p_new_params->AppendStructure(p_params);
		}
	}		
	
	return p_new_params;
}

// Stops the script from executing, and deletes the parameters, clears the stack, etc.
// (REQUIREMENT: Must set mp_pc to NULL, so script will be deleted by UpdateSpawnedScripts)
void CScript::ClearScript()
{
	if (mp_struct_script)
	{
		Mem::Free(mp_struct_script);
		mp_struct_script=NULL;
	}

	Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
	Dbg_MsgAssert(p_script_cache,("NULL p_script_cache"));
	if (mp_pc)
	{
		// If a script was being executed, then decrement its usage in the script cache since
		// it is no longer needed by this CScript.
		p_script_cache->DecrementScriptUsage(mScriptChecksum);
	}
	
	mp_pc=NULL;	
	
	if (mp_params)
	{
		delete mp_params;
		mp_params=NULL;
	}
	if (mp_function_params)
	{
		delete mp_function_params;
		mp_function_params=NULL;
	}	
	
    for (int i=0; i<m_num_return_addresses; ++i)
    {
		p_script_cache->DecrementScriptUsage(mp_return_addresses[i].mScriptNameChecksum);
		
        Dbg_MsgAssert(mp_return_addresses[i].mpParams,("NULL mpParams in return stack"));
		if (mp_return_addresses[i].mpParams)
		{
			delete mp_return_addresses[i].mpParams;
			mp_return_addresses[i].mpParams=NULL;
		}	
    }    
	
	m_num_return_addresses=0;
	if (mp_return_addresses != mp_return_addresses_small_buffer)
	{
		Dbg_MsgAssert(mp_return_addresses,("NULL mp_return_addresses ?"));
		Mem::Free(mp_return_addresses);
	}
	mp_return_addresses=mp_return_addresses_small_buffer;


	mp_current_loop=NULL;
	if (mp_loops != mp_loops_small_buffer)
	{
		Dbg_MsgAssert(mp_loops,("NULL mp_loops ?"));
		Mem::Free(mp_loops);
	}
	mp_loops=mp_loops_small_buffer;
	
	
	m_wait_timer=0;
	m_wait_type=WAIT_TYPE_NONE;
	mp_wait_component=NULL;
	
	mpObject=NULL;
	mScriptChecksum=0;

	m_interrupted=false;
	m_skip_further_process_waits=false;
	
		
	#ifdef __NOPT_ASSERT__
	m_if_status=0;
	#endif
}

// Cleans up the script's event handler table.  Call after ClearScript if you are not about to reset the script
// (REQUIREMENT: Must set mp_pc to NULL, so script will be deleted by UpdateSpawnedScripts)
void CScript::ClearEventHandlerTable()
{
	#ifdef	__SCRIPT_EVENT_TABLE__		
	if (mp_event_handler_table)
	{
		// Remove the references to the event handler from the tracker
		mp_event_handler_table->unregister_all(this);
		
		mp_event_handler_table->remove_group(CRCD(0x8b713e0e, "all_groups"));
	}
	#endif
}

// Restarts the script.
void CScript::Restart()
{
	// This is a structure for temporarily storing the old parameters, since the SetScript call will clear
	// everything in the script, including deleting the structure pointed to by mp_params, and the call stack.
	CStruct *p_params=new CStruct;
	
	if (m_num_return_addresses)
	{
		// If there are things in the call stack, then the script that we want to restart is
		// the first one in the call stack, since this is the original script.
		Dbg_MsgAssert(mp_return_addresses[0].mpParams,("NULL p_params in return stack"));
		*p_params=*mp_return_addresses[0].mpParams;
		SetScript(mp_return_addresses[0].mScriptNameChecksum,p_params,mp_return_addresses[0].mpObject);
	}
	else
	{
		// If there is nothing in the call stack, restart the current script.
		*p_params=*mp_params;
		// The parameters are now safe in p_params, so restart the script by reinitialising it.
		SetScript(mScriptChecksum,p_params,mpObject);
	}
		
	// Delete the temporary structure.
	delete p_params;
}

// Stops the script from being associated with an object, ie, sets mpObject to NULL.
// Allows the script to continue executing though, so the script must not contain any
// further member functions, otherwise the code will assert.
// This function also sets m_wait_type=WAIT_TYPE_NONE; to ensure the Update function does
// not assert if it is waiting for the object to finish doing something.
void CScript::DisassociateWithObject(Obj::CObject *pObjectToDisconnectFrom)
{
	if (!pObjectToDisconnectFrom) return;

	#ifdef	__NOPT_ASSERT__
	pObjectToDisconnectFrom->SetLockAssertOff();	
	#endif

	if (mpObject == pObjectToDisconnectFrom)
	{
		mpObject=NULL;
	}
	for (int i=0; i<m_num_return_addresses; ++i)
	{
		if (mp_return_addresses[i].mpObject == pObjectToDisconnectFrom)
		{
			mp_return_addresses[i].mpObject = NULL;
		}
	}		
	
	ClearWait();
}

#ifdef __NOPT_ASSERT__
#ifndef __PLAT_XBOX__

#define SCRIPT_INFO_BUF_SIZE 1000
static char sp_script_info[SCRIPT_INFO_BUF_SIZE];

const char *CScript::GetScriptInfo()
{
	// if called on a NULL object, then return an appropiate string 
	if (this == NULL)
	{
		#ifdef	__NOPT_ASSERT__
		return "No Script, probably CFunc called from RunScript or ForEachIn";
		#else
		return "";
		#endif
	}
	sprintf(sp_script_info,"GetScriptInfo: Missing info");

	if (mScriptChecksum)
	{
		CSymbolTableEntry *p_sym=LookUpSymbol(mScriptChecksum);
	
		if (p_sym)
		{
			if (mp_pc)
			{
				sprintf(sp_script_info,"Line %d of %s, in script '%s'",GetLineNumber(mp_pc),FindChecksumName(p_sym->mSourceFileNameChecksum),FindChecksumName(mScriptChecksum));
			}
		}
		else
		{
			sprintf(sp_script_info,"GetScriptInfo: Script symbol not found. Strange.");
		}	
	}
		
	strcat(sp_script_info,"\nScript call stack:\n");
	CSymbolTableEntry *p_sym=LookUpSymbol(mScriptChecksum);
	if (p_sym)
	{
		strcat(sp_script_info,FindChecksumName(p_sym->mSourceFileNameChecksum));
	}
	else
	{
		strcat(sp_script_info,"Unknown source file");
	}	
	sprintf(sp_script_info+strlen(sp_script_info)," line %d, in ",GetLineNumber(mp_pc));
	strcat(sp_script_info,FindChecksumName(mScriptChecksum));
	strcat(sp_script_info,"\n");
    for (int i=m_num_return_addresses-1; i>=0; --i)
    {
		if (mp_return_addresses[i].mpParams)
		{
			sprintf(sp_script_info+strlen(sp_script_info)," line %d, in ",GetLineNumber(mp_return_addresses[i].mpReturnAddress));
			strcat(sp_script_info,FindChecksumName(mp_return_addresses[i].mScriptNameChecksum));
			strcat(sp_script_info,"\n");
		}	
		else
		{
			strcat(sp_script_info,"-\n");
		}	
    }    
		
		
	Dbg_MsgAssert(strlen(sp_script_info)<SCRIPT_INFO_BUF_SIZE,("sp_script_info buffer overflow"));
	return sp_script_info;
}

int CScript::GetCurrentLineNumber(void)
{
	if (mp_pc)
	{
		return GetLineNumber(mp_pc);
	}
	else
	{
		return 0;
	}	
}

const char *CScript::GetSourceFile(void)
{
	CSymbolTableEntry *p_sym=LookUpSymbol(mScriptChecksum);
	if (p_sym)
	{
		return FindChecksumName(p_sym->mSourceFileNameChecksum);
	}
	else
	{
		return "Unknown";
	}
}		
#else
// Need these to be fast on Xbox because they are used in Dbg_MsgAsserts(), which will call it
// regardless of whether the assert condition is false.
const char *CScript::GetScriptInfo()
{
	return "";
}
int CScript::GetCurrentLineNumber(void)
{
	return 0;
}
const char *CScript::GetSourceFile(void)
{
	return "";
}		
#endif // #ifndef __PLAT_XBOX__
#endif


#ifdef __NOPT_ASSERT__
#ifndef __PLAT_WN32__
// For testing how many scripts end up needing the bigger buffer.
bool CScript::UsingBigCallstackBuffer()
{
	if (mp_return_addresses == mp_return_addresses_small_buffer)
	{
		return false;
	}
	return true;	
}

bool CScript::UsingBigLoopBuffer()
{
	if (mp_loops == mp_loops_small_buffer)
	{
		return false;
	}
	return true;	
}

#endif
#endif

// Used by CScript::Update when calling a script.
// call_script is also used by CScript::Interrupt below.
void CScript::call_script(uint32 newScriptChecksum, uint8 *p_script, CStruct *p_params, Obj::CObject *p_object, bool interrupt)
{			  
	// K: If calling a script when nothing is currently being run, jump to it instead.
	// This is so that the mp_function_params gets created.
	// Fixes a bug where when a 2-player trick attack game was started, it would immediately
	// assert because call_script was called straight away on a newly created CScript.
	// (CGoal::RunCallbackScript would run a script which would do a MakeSkaterGosub, and the
	// skater's script was not running anything at that point)
	if (!mp_pc)
	{
		set_script(newScriptChecksum,p_script,p_params,p_object);
		
		// This may not really be necessary since there is no script to return to that might be affected,
		// but set it anyway for consistency.
		m_interrupted=interrupt;
		return;
	}

	// Calling a subroutine, so store the current script info on the stack.
	
	if (mp_return_addresses == mp_return_addresses_small_buffer && 
		m_num_return_addresses == RETURN_ADDRESSES_SMALL_BUFFER_SIZE)
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
		mp_return_addresses=(SReturnAddress*)Mem::Malloc(MAX_RETURN_ADDRESSES * sizeof(SReturnAddress));
		Mem::Manager::sHandle().PopContext();
		
		for (int i=0; i<RETURN_ADDRESSES_SMALL_BUFFER_SIZE; ++i)
		{
			mp_return_addresses[i]=mp_return_addresses_small_buffer[i];
			/*
			mp_return_addresses[i].mScriptNameChecksum		  = mp_return_addresses_small_buffer[i].mScriptNameChecksum;  
			mp_return_addresses[i].mpParams                   = mp_return_addresses_small_buffer[i].mpParams;             
			mp_return_addresses[i].mpReturnAddress            = mp_return_addresses_small_buffer[i].mpReturnAddress;      
			mp_return_addresses[i].mpObject                   = mp_return_addresses_small_buffer[i].mpObject.Convert();             
			mp_return_addresses_small_buffer[i].mpObject = NULL;
			mp_return_addresses[i].mpLoop                     = mp_return_addresses_small_buffer[i].mpLoop;               
			#ifdef __NOPT_ASSERT__
			mp_return_addresses[i].m_if_status                = mp_return_addresses_small_buffer[i].m_if_status;          
			#endif
			mp_return_addresses[i].mWaitType                  = mp_return_addresses_small_buffer[i].mWaitType;            
			mp_return_addresses[i].mpWaitComponent            = mp_return_addresses_small_buffer[i].mpWaitComponent;      
			mp_return_addresses[i].mWaitTimer                 = mp_return_addresses_small_buffer[i].mWaitTimer;           
			mp_return_addresses[i].mStartTime                 = mp_return_addresses_small_buffer[i].mStartTime;           
			mp_return_addresses[i].mWaitPeriod                = mp_return_addresses_small_buffer[i].mWaitPeriod;          
			mp_return_addresses[i].mInterrupted               = mp_return_addresses_small_buffer[i].mInterrupted;         
			*/
		}
	}
		
    Dbg_MsgAssert(m_num_return_addresses<MAX_RETURN_ADDRESSES,("\n%s\nScript stack overflow when trying to call the script '%s'",GetScriptInfo(),FindChecksumName(newScriptChecksum)));
	
	#ifdef	__NOPT_ASSERT__	
	#ifdef SEND_SCRIPT_NAMES_TO_DEBUGGER
	// Tell the debugger that this script is begin run.
	s_send_script_name(newScriptChecksum,m_num_return_addresses);
	#endif
	#endif
	
	SReturnAddress *p_stack_top=&mp_return_addresses[m_num_return_addresses++];
	
    p_stack_top->mScriptNameChecksum=mScriptChecksum;
    p_stack_top->mpParams=mp_params;
    p_stack_top->mpReturnAddress=mp_pc;
	p_stack_top->mpObject=mpObject;
	p_stack_top->mpLoop=mp_current_loop;
	#ifdef __NOPT_ASSERT__
	p_stack_top->m_if_status=m_if_status;
	#endif
	
	p_stack_top->mWaitType=m_wait_type;
	p_stack_top->mpWaitComponent=mp_wait_component;
	p_stack_top->mWaitTimer=m_wait_timer;
	p_stack_top->mWaitPeriod=m_wait_period;
	p_stack_top->mStartTime=m_start_time;
	
	p_stack_top->mInterrupted=m_interrupted;
    
	// Set the new mScriptChecksum
    mScriptChecksum=newScriptChecksum;
	#ifdef __NOPT_ASSERT__ 
	check_if_needs_to_be_watched_in_debugger();
	#endif	
	
	// Set the new mp_pc
    mp_pc=p_script;
    Dbg_MsgAssert(mp_pc,("NULL p_script sent to call_script"));
	
	// Create a new parameters structure, and fill it in using the default values listed after
	// the script name.
	// Note: Not asserting if mp_params is not NULL, because it probably won't be.
	// The old value of mp_params has been safely stored in the mp_return_addresses array above.
    mp_params=new CStruct;
	#ifdef __NOPT_ASSERT__ 
	// This is so that the structure can print line number info in case one of
	// the CStruct member function asserts goes off.
	mp_params->SetParentScript(this);
	#endif
	mp_pc=AddComponentsUntilEndOfLine(mp_params,mp_pc);
	
	// Now overlay the parameters passed to the script.
	if (p_params)
	{
		*mp_params+=*p_params;
	}	
	
	// Set the object pointer, which may get changed if executing another object's
	// script using the : operator.
	mpObject=p_object;
	
	ClearWait();
	
	#ifdef __NOPT_ASSERT__
	// Not in an if-statement on starting the new routine.
	m_if_status=0;
	#endif
	
	m_interrupted=interrupt;
}

// Uses the above function to implement an interrupt.
// This will jump to the passed script and also do one call to update.
// Once the interrupt script is finished it will return to what it was doing before.
EScriptReturnVal CScript::Interrupt(uint32 newScriptChecksum, CStruct *p_params)
{
	Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
	Dbg_MsgAssert(p_script_cache,("NULL p_script_cache"));
	uint8 *p_script=p_script_cache->GetScript(newScriptChecksum);

	// The true means interrupt the script, which means that the call to Update will not continue
	// execution of the interrupted script when the called script finishes.
	call_script(newScriptChecksum,p_script,p_params,mpObject,true);
	return Update();
}

bool CScript::Finished()
{
	return mp_pc && *mp_pc==ESCRIPTTOKEN_KEYWORD_ENDSCRIPT;
}

#ifdef __NOPT_ASSERT__
static bool sExcludeFromSkaterDebug(uint32 name)
{
	CArray *p_array=GetArray(0xdaa3a3a5/*ExcludeFromSkaterDebug*/,NO_ASSERT);
	if (!p_array)
	{
		return false;
	}
		
	for (uint32 i=0; i<p_array->GetSize(); ++i)
	{
		if (name==p_array->GetChecksum(i))
		{
			return true;
		}
	}
	
	return false;		
}
#endif

// This looks at m_wait_type, does the appropriate wait logic, and resets
// m_wait_type back to WAIT_TYPE_NONE if the wait has finished.
// Otherwise m_wait_type is left the same. 
void CScript::process_waits()
{
	switch (m_wait_type)
	{
		case WAIT_TYPE_NONE:
			break;
			
		case WAIT_TYPE_COUNTER:
			if (m_wait_timer) 
			{
				--m_wait_timer;
			}
			else
			{
				// Finished counting down, so reset wait type 
				// so that script execution continues.
				m_wait_type=WAIT_TYPE_NONE;
			}	
			break;

		case WAIT_TYPE_ONE_PER_FRAME:
			if (m_wait_timer) 
			{
				--m_wait_timer;
			}
			else
			{
				if (s_done_one_per_frame)
				{
					// already done one this frame
				}
				else
				{
					// Finished counting down, so reset wait type 
					// so that script execution continues.
					s_done_one_per_frame = true;
					m_wait_type=WAIT_TYPE_NONE;
				}
			}
			break;

			
		case WAIT_TYPE_TIMER:
		{
//			if (Tmr::ElapsedTime(m_start_time) >= m_wait_period)
			// Mick, instead of using global time, we decrement the timer whenever 
			// the script is updated.  That way paused scripts will be genuinely paused
			//
			// Note, I use m_start_time as a patch variable
			// if it is not zero, I know we have not been round this loop
			// at least once.  
			// If you feel like changing this, then note that m_start_time gets pushed on the stack 
			// when calling another script, you you will need to duplicate that functionality
			// 
			int tick = (int) (Tmr::FrameLength() * 1000.0f);			
			if ((int)m_wait_period < tick)			
			{
				if (m_start_time == 0)	   	// only if we've waited at least one frame
				{
					// Finished counting down, so reset wait type 
					// so that script execution continues.
					m_wait_type=WAIT_TYPE_NONE;
				}
				else
				{
					// finished counting down, but this was the first frame.
					// so we don't want to exit, as we cannot have a zero frame delay if 
					// the time specified was not zero
					// be we want for sure to exit next time around, so set the wait period to 0
					//
					m_wait_period = 0;
				}
			}
			else
			{
				m_wait_period -= tick;
			}
			m_start_time = 0;			// flag that we have waited at least one frame
			break;	
		}	
		case WAIT_TYPE_BLOCKED:	
			// Nothing to do except wait for whatever blocked this script to unblock it.
			break;
			
		// These are the object wait options, which only apply if the script
		// has a parent object.		
		case WAIT_TYPE_OBJECT_MOVE:	
		case WAIT_TYPE_OBJECT_ANIM_FINISHED:	
		case WAIT_TYPE_OBJECT_JUMP_FINISHED:	
		case WAIT_TYPE_OBJECT_STOP_FINISHED:
		case WAIT_TYPE_OBJECT_ROTATE:
		case WAIT_TYPE_STREAM_FINISHED:
			// Call the parent object's wait function.
			Dbg_MsgAssert(mp_wait_component,("\n%s\nNULL mp_wait_component, cannot call ProcessWait",GetScriptInfo()));
			mp_wait_component->ProcessWait(this);
			break;
			
		default:
			Dbg_MsgAssert(0,("\n%s\nBad wait type value of %d",GetScriptInfo(),m_wait_type));
			break;
	}		
}

void CScript::load_function_params()
{
	#ifdef STOPWATCH_STUFF
	pFunctionParamsStopWatch->Start();
	#endif
	
	// Clear the function parameters structure and add the new params, using mp_params to get
	// any parameters referenced using the <,> operators.
	Dbg_MsgAssert(mp_function_params,("NULL mp_function_params"));
	mp_function_params->Clear();
	// AddComponentsUntilEndOfLine will assert if mp_pc is NULL
	mp_pc=AddComponentsUntilEndOfLine(mp_function_params,mp_pc,mp_params);
	
	#ifdef STOPWATCH_STUFF
	pFunctionParamsStopWatch->Stop();
	#endif
}

// Runs the specified member function. Uses mpObject by default, but uses p_substituteObject 
// if it is not NULL. p_substituteObject defaults to NULL if it is not specified.
// Does not affect mp_pc
bool CScript::run_member_function(uint32 functionName, Obj::CObject *p_substituteObject)
{
	Obj::CObject *p_obj;
	// By default member function calls operate on mpObject, but if
	// a substitute object has been specified use that instead.
	if (p_substituteObject)
	{
		p_obj=p_substituteObject;
	}	
	else
	{
		p_obj=mpObject;
	}	
	
	bool return_value=false;
	
	if (p_obj)
	{
		#ifdef STOPWATCH_STUFF
		pUpdateStopWatch->Stop();
		pCFunctionStopWatch->Start();
		#endif
		
		#ifdef STOPWATCH_STUFF
		Tmr::CPUCycles TimeBefore,TimeAfter;
		TimeBefore=Tmr::GetTimeInCPUCycles();
		#endif
		
		return_value=p_obj->CallMemberFunction(functionName,mp_function_params,this);
		
		#ifdef STOPWATCH_STUFF
		TimeAfter=Tmr::GetTimeInCPUCycles();
		float TimeTaken;
		TimeTaken=((int)(TimeAfter-TimeBefore))/150.0f;
		if (TimeTaken>200)
		{
			//SSymbolTableEntry *p_sym=LookUpSymbol(mScriptChecksum);
			//printf("Memb: %s, %f (%s, line %d)\n",FindChecksumName(NameChecksum),TimeTaken,p_sym->pFilename,GetLineNumber(mp_pc));
		}	
		
		p_entry->mAverageExecutionTime = (p_entry->mAverageExecutionTime*p_entry->mNumCalls+TimeTaken)/(p_entry->mNumCalls+1);
		++p_entry->mNumCalls;
		p_entry->mNumCallsInThisFrame=600;
		#endif
		
		#ifdef STOPWATCH_STUFF
		pCFunctionStopWatch->Stop();
		pUpdateStopWatch->Start();
		#endif
	}	
	else
	{
		#ifdef __PLAT_WN32__
		if (functionName != 0xb3c262ec) // "DisassociateFromObject" is okay, no harm in trying to break non-existent association
		{
			Dbg_MsgAssert(0,("\n%s\nTried to call member function %s from a script\nnot associated with a CObject",GetScriptInfo(),FindChecksumName(functionName)));
		}
		#else
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

		// It network games, it's expected that some objects will no longer exist by the time
		// some object member functions are called via remote scripts.  One example being, when
		// someone joins a game-in-progress, they will be instructed to execute a series of scripts
		// and it's possible that the objects which originally spawned those scripts are no 
		// longer in the game or that the client has yet to create skaters for them.  So in 
		// those cases, just print a warning and try to continue processing the script
		if( gamenet_man->InNetGame())
		{   
			Dbg_Warning( "\n%s\nTried to call member function %s from a script\nnot associated with a CObject",GetScriptInfo(),FindChecksumName(functionName));
		}
		else if (functionName != 0xb3c262ec) // "DisassociateFromObject" is okay, no harm in trying to break non-existent association
		{
			Dbg_MsgAssert(0,("\n%s\nTried to call member function %s from a script\nnot associated with a CObject",GetScriptInfo(),FindChecksumName(functionName)));
		}
		#endif
	}
	
	return return_value;
}

// Executes the passed c-function pointer, using the current mp_function_params as parameters.
bool CScript::run_cfunction(bool (*p_cfunc)(CStruct *pParams, CScript *pCScript))
{
	Dbg_MsgAssert(p_cfunc,("\n%s\nNULL p_cfunc",GetScriptInfo()));
	
	#ifdef STOPWATCH_STUFF
	pUpdateStopWatch->Stop();
	pCFunctionStopWatch->Start();
	#endif
	
	#ifdef STOPWATCH_STUFF
	Tmr::CPUCycles TimeBefore,TimeAfter;
	TimeBefore=Tmr::GetTimeInCPUCycles();
	#endif

	/*
	Tmr::Time TimeBefore,TimeAfter;
	TimeBefore=Tmr::GetTimeInCPUCycles();
	*/
	
	bool return_value=(*p_cfunc)(mp_function_params,this);

	/*
	TimeAfter=Tmr::GetTimeInCPUCycles();
	float TimeTaken;
	TimeTaken=((int)(TimeAfter-TimeBefore))/150.0f;
	if (TimeTaken>Script::GetFloat("Threshold"))
	{
		CArray *p_exclude=Script::GetArray("Exclude");
		bool exclude=false;
		for (uint32 i=0; i<p_exclude->GetSize(); ++i)
		{
			if (p_exclude->GetChecksum(i)==name_checksum)
			{
				exclude=true;
				break;
			}
		}
		if (!exclude)
		{
			CSymbolTableEntry *p_sym=LookUpSymbol(mScriptChecksum);
			printf("CFunc: %s, %f (%s, line %d)\n",FindChecksumName(name_checksum),TimeTaken,FindChecksumName(p_sym->mSourceFileNameChecksum),GetLineNumber(mp_pc));
		}	
	}	
	*/

	
	#ifdef STOPWATCH_STUFF
	TimeAfter=Tmr::GetTimeInCPUCycles();
	float TimeTaken;
	TimeTaken=((int)(TimeAfter-TimeBefore))/150.0f;
	if (TimeTaken>200)
	{
		//SSymbolTableEntry *p_sym=LookUpSymbol(mScriptChecksum);
		//printf("CFunc: %s, %f (%s, line %d)\n",FindChecksumName(NameChecksum),TimeTaken,p_sym->pFilename,GetLineNumber(mp_pc));
	}	
	p_entry->mAverageExecutionTime = (p_entry->mAverageExecutionTime*p_entry->mNumCalls+TimeTaken)/(p_entry->mNumCalls+1);
	++p_entry->mNumCalls;
	p_entry->mNumCallsInThisFrame=600;
	#endif
	
	
	#ifdef STOPWATCH_STUFF
	pCFunctionStopWatch->Stop();
	pUpdateStopWatch->Start();
	#endif
	
	return return_value;
}

// Gets the name from mp_pc.
// If mp_pc points to a <,> type name, it will look up that name in mp_params.
// It will advance mp_pc to after the name.
uint32 CScript::get_name()
{
	uint32 name_checksum=0;
	
	switch (*mp_pc)
	{
		case ESCRIPTTOKEN_KEYWORD_ALLARGS:
		{
			++mp_pc;
			mp_params->GetChecksum(NO_NAME,&name_checksum);
			break;
		}
			
		case ESCRIPTTOKEN_ARG:
		{
			++mp_pc;
			Dbg_MsgAssert(*mp_pc==ESCRIPTTOKEN_NAME,("\n%s\nExpected '<' token to be followed by a name.",GetScriptInfo()));
			++mp_pc;
			uint32 arg_checksum=Read4Bytes(mp_pc).mChecksum;
			mp_pc+=4;
			mp_params->GetChecksum(arg_checksum,&name_checksum);
			break;
		}

		case ESCRIPTTOKEN_NAME:
		{
			++mp_pc;
			name_checksum=Read4Bytes(mp_pc).mChecksum;
			mp_pc+=4;
			break;
		}
		
		default:
		{
			Dbg_MsgAssert(0,("\n%s\nUnexpected '%s' token when expecting some sort of name",GetScriptInfo(),GetTokenName((EScriptToken)*mp_pc)));
			break;
		}	
	}
	
	return name_checksum;
}

// Executes the command pointed to by mp_pc, and returns true or false, which is the value to
// be used by any preceding if-statement.
// After executing the command, mp_pc will have changed. It might be pointing to the next command in
// the current script, or to a command in a different script, or it might even be NULL, who knows?
// A 'command' is either a cfunc,member func, or script call, in which the command has the form of
// some sort of name followed by a list of parameters to be sent to it.
// It also covers the direct setting of one of the script's parameters, such as <x>=3
// It also covers expressions enclosed in parentheses that can be used in if's, eg if (1>0)
// Note that like in C, expressions on their own are valid commands, eg (23*67), but unless preceded
// by an if their result will be discarded.
//
// The term 'command' does not cover things like if's, begin-repeat's, or switch statements.
// Generally the logic for interpreting that stuff is done in CScript::Update.
bool CScript::execute_command()
{
	uint8 token=*mp_pc;

	// Check for the pre-processed member function token.
	if (token==ESCRIPTTOKEN_RUNTIME_MEMBERFUNCTION)
	{
		++mp_pc;
		uint32 member_function_checksum=Read4Bytes(mp_pc).mChecksum;
		mp_pc+=4;
		
		load_function_params();
		return run_member_function(member_function_checksum);
	}
		
	// Check for the pre-processed cfunction token.
	if (token==ESCRIPTTOKEN_RUNTIME_CFUNCTION)
	{
		++mp_pc;
        bool (*p_cfunc)(CStruct *pParams, CScript *pCScript)=Read4Bytes(mp_pc).mpCFunction;
		mp_pc+=4;
		
		load_function_params();
		return run_cfunction(p_cfunc);		
	}

	// Check for an expression enclosed in parentheses
	if (token==ESCRIPTTOKEN_OPENPARENTH)
	{
		// Note: Not skipping past the open-parenth token because Evaluate() expects it.
		CComponent *p_comp=new CComponent;
		mp_pc=Evaluate(mp_pc,mp_params,p_comp);
				
		Dbg_MsgAssert(p_comp->mType==ESYMBOLTYPE_INTEGER,("\n%s\nBad type of '%s' returned by expression, expected integer",GetScriptInfo(),GetTypeName(p_comp->mType)));
		bool return_value=p_comp->mIntegerValue;

		CleanUpComponent(p_comp);
		delete p_comp;
		
		return return_value;
	}	
	
	// Otherwise, expect some sort of name, ie Blaa or <Blaa>
	uint32 name=get_name();
	
	// Check if the name is followed by a colon, in which case the name is the id of some object.
	if (*mp_pc==ESCRIPTTOKEN_COLON)
	{
		Obj::CObject *p_substitute_object = NULL;
		// Find the object
#ifndef __PLAT_WN32__
		p_substitute_object=Obj::ResolveToObject(name);
#endif
		Dbg_MsgAssert(p_substitute_object,("\n%s\nCould not resolve '%s' to a CObject instance",GetScriptInfo(),FindChecksumName(name)));
		if( p_substitute_object == NULL )
		{
			return true;
		}

//		if (p_substitute_object == mpObject)
//		{
//			printf("\n\n----------------------------------->\n\%s\nThis script is already running on %s\n",GetScriptInfo(),FindChecksumName(name));
//		}
		
		++mp_pc;
		mp_pc=DoAnyRandomsOrJumps(mp_pc);

		// Now we're expecting some sort of function to run on the object.
		
		// Check for a pre-processed member function
		if (*mp_pc==ESCRIPTTOKEN_RUNTIME_MEMBERFUNCTION)
		{
			++mp_pc;
			uint32 member_function_checksum=Read4Bytes(mp_pc).mChecksum;
			mp_pc+=4;
			
			load_function_params();
			return run_member_function(member_function_checksum,p_substitute_object);
		}

		// No pre-processed member function, so expect some sort of name.
		uint32 function_checksum=get_name();
		
		// Get the parameters that follow the name.
		load_function_params();

		// Look-up what kind of function it is.
		CSymbolTableEntry *p_entry=Resolve(function_checksum);
		
		// if the script is "runmenow" then a syntax error 
		// should just printf a warning and return
		if (!p_entry && mScriptChecksum == 0xfb11e6cd)   // RunMeNow
		{
			#ifdef	__NOPT_ASSERT__
			printf("WARNING !! Confused by '%s', which does not appear to be defined anywhere.\nIf it is a C-function or member function, it needs to be listed in ftables.cpp.",FindChecksumName(function_checksum));
			#endif
			return false;
		}
	
		Dbg_MsgAssert(p_entry,("\n%s\nConfused by '%s', which does not appear to be defined anywhere.\nIf it is a C-function or member function, it needs to be listed in ftables.cpp.",GetScriptInfo(),FindChecksumName(function_checksum)));

		// Expecting the function to be either a member function, or a script.	
		switch (p_entry->mType)
		{
		case ESYMBOLTYPE_QSCRIPT:
		{
			#ifdef __NOPT_ASSERT__
			if (Obj::DebugSkaterScripts)
			{
				if (p_substitute_object->GetType()==SKATE_TYPE_SKATER)
				{
					if (!sExcludeFromSkaterDebug(function_checksum))
					{	 
						printf("%d: Calling %s [%i]\n",(int)Tmr::GetRenderFrame(),FindChecksumName(function_checksum),m_unique_id);
					}	
				}
			}		
			#endif

			Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
			Dbg_MsgAssert(p_script_cache,("NULL p_script_cache"));
			uint8 *p_script=p_script_cache->GetScript(p_entry->mNameChecksum);
			
			call_script(function_checksum,p_script,mp_function_params,p_substitute_object);
			return true;
			break;
		}	
		case ESYMBOLTYPE_MEMBERFUNCTION:
			return run_member_function(function_checksum,p_substitute_object);
			break;
		default:
			Dbg_MsgAssert(0,("\n%s\n'%s' is not a member function or script",GetScriptInfo(),FindChecksumName(function_checksum)));
			return false;
			break;
		}		
	}
	
	// Check if the name is followed by equals.
	// In this case, the name is the name of the parameter we want to set.
	if (*mp_pc==ESCRIPTTOKEN_EQUALS)
	{
		// The get_name call will have looked up any <,> name in the structure, so re-get the
		// preceding name, because we want both x and <x> to mean x.
		mp_pc-=5;
		Dbg_MsgAssert(*mp_pc==ESCRIPTTOKEN_NAME,("\n%s\nEquals must be preceded by a param name",GetScriptInfo()));
		++mp_pc;
		name=Read4Bytes(mp_pc).mChecksum;
		mp_pc+=5; // +5 to skip over the equals too.

		mp_pc=DoAnyRandomsOrJumps(mp_pc);

		// Calculate the value of whatever follows the equals, and store it in p_comp		
		CComponent *p_comp=new CComponent;
		if (*mp_pc==ESCRIPTTOKEN_OPENPARENTH)
		{
			// It's an expression, so evaluate it.
			mp_pc=Evaluate(mp_pc,mp_params,p_comp);
		}
		else
		{
			// It's not an expression, so just load the value in.
			mp_pc=FillInComponentUsingQB(mp_pc,mp_params,p_comp);
		}

		if (p_comp->mType!=ESYMBOLTYPE_NONE)
		{
			// Got some sort of value, so name it and stick it into mp_params.
			p_comp->mNameChecksum=name;
			mp_params->AddComponent(p_comp);
			// Not deleting p_comp because it has been given to mp_params
		}
		else
		{
			// Did not get a value, so clean up p_comp
			delete p_comp;
		}	
		
		return true;
	}
	
	// The name is not followed by a colon or an equals, so it must be a function call.
	// Load in the parameters that follow.
	load_function_params();
	
	// Look up the function to see what it is.
    CSymbolTableEntry *p_entry=Resolve(name);
	
	// if the script is "runmenow" then a syntax error 
	// should just printf a warning and return
	if (!p_entry && mScriptChecksum == 0xfb11e6cd)   // RunMeNow
	{
		#ifdef	__NOPT_ASSERT__
		printf("WARNING !! Confused by '%s', which does not appear to be defined anywhere.\nIf it is a C-function or member function, it needs to be listed in ftables.cpp.",FindChecksumName(name));
		#endif
		return false;
	}

	if (!p_entry)
	{
		#ifdef	__NOPT_ASSERT__
		if (Script::GetInteger(CRCD(0x22d1f89,"AssertOnMissingScripts")))
		{
			Dbg_MsgAssert(p_entry,("\n%s\nConfused by '%s', which does not appear to be defined anywhere.\nIf it is a C-function or member function, it needs to be listed in ftables.cpp.",GetScriptInfo(),FindChecksumName(name)));
		}
		else
		#endif
		{
			#ifdef __PLAT_WN32__
			// Don't printf if compiling on PC, otherwise LevelAssetLister prints lots
			// of warning messages when running the load sound scripts.
			#else
			printf ("WARNING: script %s not found, ignoring in default level.\n",FindChecksumName(name));
			#endif
			return true;
		}
	}
									 

    switch (p_entry->mType)
	{
		case ESYMBOLTYPE_CFUNCTION:
			return run_cfunction(p_entry->mpCFunction);
			break;
		case ESYMBOLTYPE_MEMBERFUNCTION:
			return run_member_function(name);
			break;
		case ESYMBOLTYPE_QSCRIPT:
		{
			#ifdef __NOPT_ASSERT__
			if (Obj::DebugSkaterScripts)
			{
				if (mpObject && mpObject->GetType()==SKATE_TYPE_SKATER)
				{
					if (!sExcludeFromSkaterDebug(name))
					{
						printf("%d: Calling %s [%i]\n",(int)Tmr::GetRenderFrame(),FindChecksumName(name),m_unique_id);
					}	
				}
			}		
			#endif

			Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
			Dbg_MsgAssert(p_script_cache,("NULL p_script_cache"));
			uint8 *p_script=p_script_cache->GetScript(p_entry->mNameChecksum);
		 
			call_script(name,p_script,mp_function_params,mpObject);
			// Script calls always return true.
			return true;
			break;
		}	
		default:
			Dbg_MsgAssert(0,("\n%s\n'%s' cannot be called, because it's a '%s'",GetScriptInfo(),FindChecksumName(name),GetTypeName(p_entry->mType)));
			break;
	}		
	
	return false;
}

void CScript::execute_if()
{
	Dbg_MsgAssert(mp_pc,("NULL mp_pc"));
	Dbg_MsgAssert(*mp_pc==ESCRIPTTOKEN_KEYWORD_IF,("Unexpected *mp_pc='%s', expected keyword 'if'",GetTokenName((EScriptToken)*mp_pc)));

	// Skip over the if token.	
	++mp_pc;
	
	bool negate=false;
	if (*mp_pc==ESCRIPTTOKEN_KEYWORD_NOT)
	{
		++mp_pc;
		negate=true;
	}	
		
	bool return_value=execute_command();
	
	// mp_pc will have changed, and might be NULL.
	if (!mp_pc)
	{
		return;
	}	
	
	if (negate)
	{
		return_value=!return_value;
	}	

	#ifdef __NOPT_ASSERT__
	Dbg_MsgAssert((m_if_status&0x80000000)==0,("\n%s\nToo many nested if's",GetScriptInfo()));
	m_if_status<<=1;
	#endif
	
	// If the return value is false, skip forward to the 'else' or 'endif' statement.
	if (!return_value)
	{
		int in_nested_if=0;

		// Skip mp_pc to the current if's 'else' or 'endif',
		// but ignore the else & endifs belonging to nested ifs.
		#ifdef STOPWATCH_STUFF
		pIfSkipStopWatch->Start();
		#endif
		while (in_nested_if || 
			   !(*mp_pc==ESCRIPTTOKEN_KEYWORD_ELSE || *mp_pc==ESCRIPTTOKEN_KEYWORD_ENDIF))
		{
			Dbg_MsgAssert(*mp_pc!=ESCRIPTTOKEN_KEYWORD_ENDSCRIPT,("endif or else keywords not found after if keyword."));
			
			// Keep track of nested ifs.
			if (*mp_pc==ESCRIPTTOKEN_KEYWORD_IF)
			{
				++in_nested_if;
			}
			else if (*mp_pc==ESCRIPTTOKEN_KEYWORD_ENDIF)
			{
				Dbg_MsgAssert(in_nested_if,("Got endif within true part of if statement without corresponding nested if"));
				--in_nested_if;
			}	
				
			mp_pc=SkipToken(mp_pc);
		}    
		#ifdef __NOPT_ASSERT__
		if (*mp_pc==ESCRIPTTOKEN_KEYWORD_ENDIF)
		{
			m_if_status>>=1;
		}
		#endif
		// Skip over the 'else' or 'endif'
		++mp_pc;
		
		#ifdef STOPWATCH_STUFF
		pIfSkipStopWatch->Stop();
		#endif
	}
	else
	{
		// Otherwise, if the return value was true, mp_pc remains unchanged so that the instructions
		// following the if get executed.
		
		// Record the if-status so that spurious else's can be detected.
		#ifdef __NOPT_ASSERT__
		m_if_status|=1;
		#endif
	}	

	// I'm pretty sure if-debug isn't used, check with Scott
	/*
	#ifdef __NOPT_ASSERT__
	if (m_if_debug_on)
	{
		printf("%s: If %s ",FindChecksumName(mScriptChecksum),FindChecksumName(name_checksum));
		if (return_value)
		{
			printf("TRUE\n");
		}
		else
		{
			printf("FALSE\n");
		}	
	}
	#endif
	*/
}

// On entry, mp_pc must point to an else token.
// Skips forward till it hits an endif, then skips over the endif.
void CScript::execute_else()
{
	Dbg_MsgAssert(mp_pc,("NULL mp_pc"));
	Dbg_MsgAssert(*mp_pc==ESCRIPTTOKEN_KEYWORD_ELSE,("Unexpected *mp_pc='%s', expected keyword 'else'",GetTokenName((EScriptToken)*mp_pc)));
	// else's are only allowed in the true blocks of if-statements.
	// Bit 0 of m_if_status indicates the status of the last if.
	Dbg_MsgAssert(m_if_status&1,("\n%s\nSpurious 'else'",GetScriptInfo()));
	
	#ifdef STOPWATCH_STUFF
	pIfSkipStopWatch->Start();
	#endif
			
	// Skip over the code within the else, since the code before it must have executed.
	int in_nested_if=0;
	while (in_nested_if || *mp_pc!=ESCRIPTTOKEN_KEYWORD_ENDIF)
	{
		Dbg_MsgAssert(*mp_pc!=ESCRIPTTOKEN_KEYWORD_ENDSCRIPT,("endif keyword not found after else keyword."));
		
		// Keep track of nested ifs.
		if (*mp_pc==ESCRIPTTOKEN_KEYWORD_IF)
		{
			++in_nested_if;
		}
		else if (*mp_pc==ESCRIPTTOKEN_KEYWORD_ENDIF)
		{
			Dbg_MsgAssert(in_nested_if,("Got endif within else part of if statement without corresponding nested if"));
			--in_nested_if;
		}	
		
		mp_pc=SkipToken(mp_pc);
	}    
	// We've hit the endif, so skip over it.
	++mp_pc;

	#ifdef __NOPT_ASSERT__
	m_if_status>>=1;
	#endif
	
	#ifdef STOPWATCH_STUFF
	pIfSkipStopWatch->Stop();
	#endif
}

// We can hit an endif either by finishing the 'true' block of an if-statement
// that has no else, or by finishing the 'false' block following an else.
// Either way there is nothing to do, so just skip over it.
void CScript::execute_endif()
{
	Dbg_MsgAssert(mp_pc,("NULL mp_pc"));
	Dbg_MsgAssert(*mp_pc==ESCRIPTTOKEN_KEYWORD_ENDIF,("Unexpected *mp_pc='%s', expected keyword 'endif'",GetTokenName((EScriptToken)*mp_pc)));
	
	#ifdef STOPWATCH_STUFF
	pIfSkipStopWatch->Start();
	#endif
	
	++mp_pc;

	#ifdef __NOPT_ASSERT__
	m_if_status>>=1;
	#endif

	#ifdef STOPWATCH_STUFF
	pIfSkipStopWatch->Stop();
	#endif
}		

// Skips mp_pc forward till it finds an endswitch, skipping over any nested switch
// statements in between.
// Afterwards, mp_pc will point to the token following the endswitch.
void CScript::skip_to_after_endswitch()
{
	Dbg_MsgAssert(mp_pc,("NULL mp_pc"));
	
	while (*mp_pc!=ESCRIPTTOKEN_KEYWORD_ENDSWITCH)
	{
		if (*mp_pc==ESCRIPTTOKEN_KEYWORD_SWITCH)
		{
			// Skip over the switch
			++mp_pc;
			skip_to_after_endswitch();
		}
		else
		{
			mp_pc=SkipToken(mp_pc);
		}
	}
	
	// Skip over the endswitch
	++mp_pc;
}

// Given that mp_pc points to a switch token, this will skip mp_pc forward to the 
// code of the matching case. Or if none matches, it will point to after the endswitch.
void CScript::execute_switch()
{
	Dbg_MsgAssert(mp_pc,("NULL mp_pc"));
	Dbg_MsgAssert(*mp_pc==ESCRIPTTOKEN_KEYWORD_SWITCH,("Unexpected *mp_pc='%s', expected keyword 'switch'",GetTokenName((EScriptToken)*mp_pc)));
	
	#ifdef __NOPT_ASSERT__
	// Remember the old pc value so that the printed line number is correct if 
	// no endswitch is found before the end of file is hit.
	uint8 *start_of_switch_pc=mp_pc;
	#endif

	// Skip over the switch token
	++mp_pc;
	
	// Put the parameters following the switch keyword into mp_function_params
	Dbg_MsgAssert(mp_function_params,("NULL mp_function_params"));
	mp_function_params->Clear();
	mp_pc=AddComponentsUntilEndOfLine(mp_function_params,mp_pc,mp_params);
	
	// Get the first component and make a copy of it, putting it into p_comp.
	// Making a copy cos mp_function_params is needed later to hold the params following
	// each case statement that follows.
	CComponent *p_switch_comp=new CComponent;
	CComponent *p_first_comp=mp_function_params->GetNextComponent();
	if (p_first_comp)
	{
		Dbg_MsgAssert(mp_function_params->GetNextComponent(p_first_comp)==NULL,("\n%s\nswitch argument contains more than one component",GetScriptInfo()));
		CopyComponent(p_switch_comp,p_first_comp);
	}			
	
	// Skip forward till we hit a matching case, or a default, or endswitch
	// But, if we hit another switch statement whilst looking, 
	// skip over it, ignoring its cases.
	bool found=false;
	while (!found)
	{
		switch (*mp_pc)
		{
		case ESCRIPTTOKEN_KEYWORD_SWITCH:
		{
			// Skip over the switch
			++mp_pc;
			skip_to_after_endswitch();
			// mp_pc now points to the token following the ESCRIPTTOKEN_KEYWORD_ENDSCRIPT
			break;
		}
			
		case ESCRIPTTOKEN_KEYWORD_CASE:
		{
			++mp_pc;
			
			mp_function_params->Clear();
			mp_pc=AddComponentsUntilEndOfLine(mp_function_params,mp_pc,mp_params);
			
			CComponent *p_case_comp=mp_function_params->GetNextComponent();
			if (p_case_comp)
			{
				Dbg_MsgAssert(mp_function_params->GetNextComponent(p_case_comp)==NULL,("\n%s\ncase argument contains more than one component",GetScriptInfo()));
				
				if (*p_switch_comp==*p_case_comp)
				{
					found=true;
				}
			}
			
			if (found)
			{
				// We've found a match, but we need to skip over any case statements
				// that immediately follow. This is for when multiple cases want to
				// execute the same chunk of code.
				while (true)
				{
					mp_pc=SkipEndOfLines(mp_pc);
					if (*mp_pc==ESCRIPTTOKEN_KEYWORD_CASE)
					{
						++mp_pc;
						mp_function_params->Clear();
						mp_pc=AddComponentsUntilEndOfLine(mp_function_params,mp_pc,mp_params);
					}							
					else
					{
						break;
					}
				}		
			}
			break;
		}	
		case ESCRIPTTOKEN_KEYWORD_DEFAULT:
		case ESCRIPTTOKEN_KEYWORD_ENDSWITCH:
			++mp_pc;
			found=true;
			break;
		case ESCRIPTTOKEN_KEYWORD_ENDSCRIPT:
			#ifdef __NOPT_ASSERT__
			mp_pc=start_of_switch_pc;
			Dbg_MsgAssert(0,("\n%s\nMissing endswitch",GetScriptInfo()));
			#endif
			break;
		default:
			mp_pc=SkipToken(mp_pc);
			break;
		}	
	}
	
	CleanUpComponent(p_switch_comp);
	delete p_switch_comp;
}

void CScript::execute_begin()
{
	Dbg_MsgAssert(mp_pc,("NULL mp_pc"));
	Dbg_MsgAssert(*mp_pc==ESCRIPTTOKEN_KEYWORD_BEGIN,("Unexpected *mp_pc='%s', expected keyword 'begin'",GetTokenName((EScriptToken)*mp_pc)));
	++mp_pc;
	
	// First, check whether we're about to run out of space on the small statically allocated loop buffer.
	if (mp_current_loop==mp_loops_small_buffer+NESTED_BEGIN_REPEATS_SMALL_BUFFER_SIZE-1)
	{
		// There won't be enough room for the new loop!

		// A quick check, to ensure no memory leaks.
		Dbg_MsgAssert(mp_loops==mp_loops_small_buffer,("Expected mp_loops==mp_loops_small_buffer, %x, %x",mp_loops,mp_loops_small_buffer));
		
		// Dynamically allocate a bigger buffer, and change mp_loops to point to that instead
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
		mp_loops=(SLoop*)Mem::Malloc(MAX_NESTED_BEGIN_REPEATS * sizeof(SLoop));
		Mem::Manager::sHandle().PopContext();
		
		// Then copy over the contents of the small buffer, and update mp_current_loop to point into the new buffer.
		int i;

		for (i=0; i<NESTED_BEGIN_REPEATS_SMALL_BUFFER_SIZE; ++i)
		{
			mp_loops[i]=mp_loops_small_buffer[i];
		}
		mp_current_loop=mp_loops+NESTED_BEGIN_REPEATS_SMALL_BUFFER_SIZE-1;
		
		// But now, we also have to scan through the call stack and update all the mpLoop pointers to
		// point into the new buffer.
		for (i=0; i<m_num_return_addresses; ++i)
		{
			if (mp_return_addresses[i].mpLoop)
			{
				Dbg_MsgAssert(mp_return_addresses[i].mpLoop >= mp_loops_small_buffer && mp_return_addresses[i].mpLoop < mp_loops_small_buffer+NESTED_BEGIN_REPEATS_SMALL_BUFFER_SIZE,("Bad mp_return_addresses[i].mpLoop"));
				
				mp_return_addresses[i].mpLoop=(mp_return_addresses[i].mpLoop-mp_loops_small_buffer)+mp_loops;
			}
		}    
	}
	
	if (mp_current_loop)
	{            
		Dbg_MsgAssert(mp_current_loop-mp_loops<MAX_NESTED_BEGIN_REPEATS-1,("\n%s\nToo many nested begin-repeats",GetScriptInfo()));
		++mp_current_loop;
	}
	else
	{
		mp_current_loop=mp_loops;
	}
		
	// Initialise the new loop ... 
	
	// Record the start for looping back.
	mp_current_loop->mpStart=mp_pc;
	// These get filled in once the repeat is reached.
	mp_current_loop->mpEnd=NULL;
	mp_current_loop->mGotCount=false;
	mp_current_loop->mNeedToReadCount=true;
	mp_current_loop->mCount=0;
}


void CScript::execute_repeat()
{
	Dbg_MsgAssert(mp_pc,("NULL mp_pc"));
	Dbg_MsgAssert(*mp_pc==ESCRIPTTOKEN_KEYWORD_REPEAT,("Unexpected *mp_pc='%s', expected keyword 'repeat'",GetTokenName((EScriptToken)*mp_pc)));
	Dbg_MsgAssert(mp_current_loop,("\n%s\nEncountered repeat with NULL mp_current_loop",GetScriptInfo()));
	
	if (mp_current_loop->mNeedToReadCount)
	{
		// Skip over the repeat token. 
		++mp_pc;
		Dbg_MsgAssert(mp_function_params,("NULL mp_function_params"));
		mp_function_params->Clear();
		mp_current_loop->mpEnd=AddComponentsUntilEndOfLine(mp_function_params,mp_pc,mp_params);
		mp_current_loop->mGotCount=mp_function_params->GetInteger(NO_NAME,&mp_current_loop->mCount);
		if (!mp_current_loop->mGotCount)
		{
			Dbg_MsgAssert(mp_current_loop->mpEnd==mp_pc,("\n%s\nNo count value found following 'repeat'",GetScriptInfo()));
		}		
		
		mp_current_loop->mNeedToReadCount=false;
	}	
		
	if (!mp_current_loop->mGotCount)
	{
		// It's an infinite loop.
		mp_pc=mp_current_loop->mpStart;
	}
	else
	{
		Dbg_MsgAssert(mp_current_loop->mCount,("\n%s\nZero count given to a begin-repeat loop",GetScriptInfo()));
		--mp_current_loop->mCount;
		if (mp_current_loop->mCount)
		{
			// Finite loop, but it has not finished yet, so jump back to the start.
			mp_pc=mp_current_loop->mpStart;
		}
		else
		{
			// The loop has finished!
			
			// Jump PC to the next instruction after the repeat.
			Dbg_MsgAssert(mp_current_loop->mpEnd,("NULL mp_current_loop->pEnd ??")); 
			mp_pc=mp_current_loop->mpEnd;
			
			// Rewind to the previous loop.
			if (mp_current_loop==mp_loops)
			{
				mp_current_loop=NULL;
			}
			else
			{
				--mp_current_loop;
				Dbg_MsgAssert(mp_current_loop>=mp_loops,("\n%s\nBad mp_current_loop",GetScriptInfo()));
			}
		}
	}
}

void CScript::execute_break()
{
	Dbg_MsgAssert(mp_pc,("NULL mp_pc"));
	Dbg_MsgAssert(*mp_pc==ESCRIPTTOKEN_KEYWORD_BREAK,("Unexpected *mp_pc='%s', expected keyword 'break'",GetTokenName((EScriptToken)*mp_pc)));
	Dbg_MsgAssert(mp_current_loop,("\n%s\nEncountered break with NULL mp_current_loop",GetScriptInfo()));
	
	// Step over every token until repeat is reached.

	#ifdef __NOPT_ASSERT__
	int nested_if_count=0;
	#endif	
	int nested_loop_count=0;
	bool in_loop=true;
	// Skip mp_pc to the end of the loop.
	while (in_loop)
	{
		switch (*mp_pc)
		{
		case ESCRIPTTOKEN_KEYWORD_BEGIN:
			++nested_loop_count;
			break;
			
		case ESCRIPTTOKEN_KEYWORD_REPEAT:
			if (nested_loop_count)
			{
				--nested_loop_count;
			}
			else
			{
				in_loop=false;
			}
			break;
			
		#ifdef __NOPT_ASSERT__
		case ESCRIPTTOKEN_KEYWORD_IF:
			++nested_if_count;
			break;
			
		case ESCRIPTTOKEN_KEYWORD_ENDIF:
			if (nested_if_count)
			{
				--nested_if_count;
			}
			else
			{
				// Make sure that the stored if-status is rewound back to its
				// state before the loop was entered, otherwise the
				// 'too many nested ifs' assert will go off erroneously.
				m_if_status>>=1;
			}
			break;
		#endif
		
		case ESCRIPTTOKEN_KEYWORD_ENDSCRIPT:
			Dbg_MsgAssert(0,("\n%s\nRepeat keyword not found after break keyword.",GetScriptInfo()));
			break;
		}
			
		mp_pc=SkipToken(mp_pc);
	}    
	
	// Use AddComponentsUntilEndOfLine just to step over the possible repeat argument.
	// Ugh! Kind of ugly, but shouldn't be too slow because repeat will probably be followed
	// by nothing or just an integer.
	Dbg_MsgAssert(mp_function_params,("NULL mp_function_params"));
	mp_function_params->Clear();
	mp_pc=AddComponentsUntilEndOfLine(mp_function_params,mp_pc);

	
	// Rewind to the previous loop.
	if (mp_current_loop==mp_loops)
	{
		mp_current_loop=NULL;
	}
	else
	{
		--mp_current_loop;
		Dbg_MsgAssert(mp_current_loop>=mp_loops,("\n%s\nBad mp_current_loop",GetScriptInfo()));
	}
}

// Returns from a sub-script by popping the info (mp_pc etc) off the stack.
// If there is nothing on the stack, it sets mp_pc to NULL
// Returns true if the script being returned from was a script triggered by Interrupt.
// This then allows Update() to not continue execution of the script that was interrupted.
bool CScript::execute_return()
{
	// The current script is finished with, so decrement its usage in the script cache.
	Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
	Dbg_MsgAssert(p_script_cache,("NULL p_script_cache"));
	p_script_cache->DecrementScriptUsage(mScriptChecksum);

	bool was_interrupted=m_interrupted;
	
	if (m_num_return_addresses)
	{
		// This script was called from another, either by a regular call or by an interrupt.
		
		--m_num_return_addresses;
		
		// Delete the current mp_params, because it was created when this routine was called.
		Dbg_MsgAssert(mp_params,("\n%s\nNULL p_params",GetScriptInfo()));
		delete mp_params;
		
		// Restore the info stored on the stack.
		SReturnAddress *p_info=mp_return_addresses+m_num_return_addresses;
		
		mScriptChecksum = p_info->mScriptNameChecksum;
		mp_params		= p_info->mpParams;
		mp_pc			= p_info->mpReturnAddress;
		mpObject		= p_info->mpObject;
		mp_current_loop = p_info->mpLoop;
		#ifdef __NOPT_ASSERT__
		m_if_status		= p_info->m_if_status;
		#endif

		m_wait_type		= p_info->mWaitType;
		if (m_wait_type)
		{
			m_wait_timer	= p_info->mWaitTimer;
			m_wait_period	= p_info->mWaitPeriod;
			m_start_time	= p_info->mStartTime;
			mp_wait_component = p_info->mpWaitComponent;
		}	
		
		m_interrupted=p_info->mInterrupted;
		
		#ifdef __NOPT_ASSERT__ 
		check_if_needs_to_be_watched_in_debugger();
		#endif	
		
		// Check that the script being returned to does still exist, ie mp_pc is valid.
		// However, if we're running an embedded script, don't do the check, because the
		// script name may be the name of the embedded script rather than a global script.
		if (!mp_struct_script)
		{
			Dbg_MsgAssert(LookUpSymbol(mScriptChecksum),("Non-existent script '%s' on call stack",FindChecksumName(mScriptChecksum)));
		}
			
		// Actually, we need a better check to see if mp_pc is still valid, because the
		// script we're returning to might have got reloaded and hence moved in memory.
		// Should use a smart pointer.
		// But, doesn't seem to have caused any problems so far, so leave for the moment ...
	}
	else
	{
		// Nothing to return to, so stop the script by setting mp_pc to NULL.
		// This occurs when an endscript is hit.
		mp_pc=NULL;
	}	
	
	return was_interrupted;
}

#ifdef	__NOPT_ASSERT__
#ifndef __PLAT_WN32__
void CScript::advance_pc_to_next_line_and_halt()
{
	if (m_being_watched_in_debugger)
	{
		if (m_single_step_mode==STEP_OVER || 
			m_single_step_mode==STEP_INTO)
		{
			// Record the current return address count so that it can be detected whether
			// the next command is a script call. Needed for stepping over script calls.
			m_last_num_return_addresses_when_halted=m_num_return_addresses;
			
			if (mp_pc)
			{
				mp_pc=SkipEndOfLines(mp_pc);
			}	
			
			TransmitInfoToDebugger();
			m_last_instruction_time_taken=0.0f;
			Dbg::CScriptDebugger::Instance()->ScriptDebuggerPause();
		}
	}
}			
#endif
#endif

//static int sInstructionCount=0;
//static uint64 sLastVBlanks=0;

// Update the script, executing instructions
// REQUIREMENT: is mp_pc is NULL, then return  ESCRIPTRETURNVAL_FINISHED, so the script is deleted by UpdateSpawnedScript
EScriptReturnVal CScript::Update()
{
	#ifdef	__NOPT_ASSERT__
	m_last_instruction_time_taken=0.0f;	
	m_total_instruction_time_taken=0.0f;
	if (m_being_watched_in_debugger)
	{
		TransmitInfoToDebugger();
	}	
	#endif

	#ifdef STOPWATCH_STUFF
	pUpdateStopWatch->Start();
	#endif

	#ifdef	__NOPT_ASSERT__
	Tmr::CPUCycles start_time = Tmr::GetTimeInCPUCycles();
	#endif


	//if (sLastVBlanks!=Tmr::GetVblanks())
	//{
	//	sLastVBlanks=Tmr::GetVblanks();
	//	printf("Instruction count = %d\n",sInstructionCount);
	//	sInstructionCount=0;
	//}	
	

	#ifdef	__NOPT_ASSERT__	
	Obj::CObjectPtr p_obj = NULL;
	if (mpObject)
	{
		p_obj = mpObject;	   			// remember the object we locked
//		mpObject->SetLockAssertOn();
	}
	#endif
	
	m_skip_further_process_waits=false;
	
	// This loop is going to keep executing script commands until either mp_pc
	// becomes NULL somehow, or if some sort of wait command gets executed.
    while (true)
    {
		if (!mp_pc) 
		{
			// Break out if mp_pc became NULL during the execution of the
			// last command. 
			#ifdef STOPWATCH_STUFF
			pUpdateStopWatch->Stop();
			#endif
			sCurrentlyUpdating=NULL;
	#ifdef	__NOPT_ASSERT__	
	
//			if (mpObject)
//			{
//				// check the object we locked is the same one we are unlocking (or deleted)
//				Dbg_MsgAssert(p_obj == mpObject,("\n%s\nFinished Script has changed objects to %p! Original Object %p Still Locked! Tell Mick\n",GetScriptInfo(),mpObject.Convert(),p_obj.Convert()));
//				mpObject->SetLockAssertOff();
//			}
			// If script is waiting, then we might be on a different object
			// if the original object still exists, then unlock it.
			if (p_obj)
			{
				p_obj->SetLockAssertOff();
			}
	#endif																	
			#ifdef	__NOPT_ASSERT__
			start_time = Tmr::GetTimeInCPUCycles() - start_time;
			m_last_time = (int)start_time;	 // experiment with uint etc....
			m_total_time += m_last_time;
			#endif
			
			// Seems like a reasonable thing to do ...
			m_interrupted=false;
			
			return ESCRIPTRETURNVAL_FINISHED;
		}	
		else
		{
			// Make sure sCurrentlyUpdating is correct, because it may have
			// got changed during the execution of the last command.
			sCurrentlyUpdating=this;
		}	
	
		#ifdef	__NOPT_ASSERT__
		bool was_waiting_before;
		if (m_wait_type==WAIT_TYPE_NONE)
		{
			was_waiting_before=false;
		}
		else
		{
			was_waiting_before=true;
		}
		#endif
		
		if (!m_skip_further_process_waits)
		{
			process_waits();
		}	

		// Return straight away if waiting.
		if (m_wait_type!=WAIT_TYPE_NONE)
		{
			#ifdef STOPWATCH_STUFF
			pUpdateStopWatch->Stop();
			#endif
			sCurrentlyUpdating=NULL;

	#ifdef	__NOPT_ASSERT__	
			// If script is waiting, then we might be on a different object
			// if the original object still exists, then unlock it.
			if (p_obj)
			{
				p_obj->SetLockAssertOff();
			}
	#endif	 

	#ifdef	__NOPT_ASSERT__
			start_time = Tmr::GetTimeInCPUCycles() - start_time;
			m_last_time = (int)start_time;	 // experiment with uint etc....
			m_total_time += m_last_time;
	#endif
	   	
			// If an interrupt script hits a blocking command, then clear the interrupted flag.
			// Reason:
			// If an interrupt script contains no blocking functions, then a single call to CScript::Update()
			// would execute the whole script, and as soon as the endscript was reached it was exit the Update()
			// function (due to the interrupt flag being on) hence returning control to the original Update()
			// function that was executing the interrupted script, and all would be well.
			
			// However, if the interrupt script did hit a blocking function, that would cause an early exit from
			// the Update() function, leaving it to the original Update() function, ie the one that was executing the
			// interrupted script, to finish completion of the interrupt script. In that case, we don't want
			// the interrupt flag to be on any more, because otherwise it would cause a premature exit from
			// the original Update() function as soon as the endscript was reached.
			m_interrupted=false;
	
			// Set this to prevent any previous CScript::Update() loop that the return may return to
			// from processing the waits further, otherwise if an interrupt script contains a wait n gameframes
			// it will end up waiting only n-1 gameframes, due to the interrupted scripts Update() loop calling
			// process_waits again.
			m_skip_further_process_waits=true;
			
			if (m_wait_type==WAIT_TYPE_BLOCKED)
			{
				return ESCRIPTRETURNVAL_BLOCKED;
			}
			else
			{
				return ESCRIPTRETURNVAL_WAITING;
			}
		}		
		else
		{
			#ifdef	__NOPT_ASSERT__
			if (was_waiting_before)
			{
				advance_pc_to_next_line_and_halt();
			}	
			#endif
		}		

		// Execute whatever is at mp_pc
		#ifdef	__NOPT_ASSERT__
		Tmr::CPUCycles instruction_start_time = Tmr::GetTimeInCPUCycles();
		#endif
		
		switch (*mp_pc)
		{
		case ESCRIPTTOKEN_KEYWORD_IF:
			//++sInstructionCount;
			// This will execute the function call or expression following the if,
			// and if the command or expression returns false, it will skip
			// mp_pc forward to the token after the else or endif.
			// Otherwise, mp_pc will be left pointing to the first token of the 'true' block.
			execute_if();
			// Note: mp_pc may be NULL at this point
			break;
			
		case ESCRIPTTOKEN_KEYWORD_ELSE:
			//++sInstructionCount;
			// The only time an else token should be hit is at the end of executing
			// the 'true' block following the if.
			// So this will just skip mp_pc over the 'false' block to after the endif.
			execute_else();
			break;
			
		case ESCRIPTTOKEN_KEYWORD_ENDIF:
			//++sInstructionCount;
			// We can hit an endif either by finishing the 'true' block of an if-statement
			// that has no else, or by finishing the 'false' block following an else.
			// Either way there is nothing to do, so just skip over it, but assert if we're
			// not in an if-statement to catch spurious else's.
			execute_endif();
			break;

        case ESCRIPTTOKEN_KEYWORD_SWITCH:
			//++sInstructionCount;
			// Skips mp_pc forward to the token following the matching case. 
			// Or if none matches, it will point to after the endswitch.
			execute_switch();
			break;

        case ESCRIPTTOKEN_KEYWORD_CASE:
		case ESCRIPTTOKEN_KEYWORD_DEFAULT:
        case ESCRIPTTOKEN_KEYWORD_ENDSWITCH:
			//++sInstructionCount;
			// If we hit a case, default or endswitch, it must be due to the completion of the
			// previous case statement's commands.
			// So skip mp_pc forward so that it points to after the endswitch token.
			skip_to_after_endswitch();
			break;
			
        case ESCRIPTTOKEN_KEYWORD_BEGIN:
			//++sInstructionCount;
			// Initialise a new loop counter
			execute_begin();
            break;        

        case ESCRIPTTOKEN_KEYWORD_REPEAT:
			//++sInstructionCount;
			// This will either jump mp_pc back to the start of the loop,
			// or skip it past the repeat if the loop has finished.
			execute_repeat();
            break;

        case ESCRIPTTOKEN_KEYWORD_BREAK:
			//++sInstructionCount;
			// This will skip mp_pc to after the repeat of the current loop
			execute_break();
            break;
		    
        case ESCRIPTTOKEN_KEYWORD_ENDSCRIPT:
			//++sInstructionCount;
			if (execute_return())
			{
				// If we returned from a script call caused by an interrupt, then do not continue
				// with execution of the interrupted script, since an interrupt should not affect the
				// interrupted script.
				return ESCRIPTRETURNVAL_FINISHED_INTERRUPT;
			}	
			// Note: mp_pc may be NULL at this point, if there is no calling script.
            break;
			
        case ESCRIPTTOKEN_KEYWORD_RETURN:
			//++sInstructionCount;
			
			// The return keyword works very similar to hitting the endscript token.
			// The difference is that 'return' will merge the parameters that follow the
			// return keyword onto the parameters of the calling script, if there is one.
			
			// Skip over the return token. 
			++mp_pc;
			
			// Put any parameters that follow into mp_function_params
			Dbg_MsgAssert(mp_function_params,("NULL mp_function_params"));
			mp_function_params->Clear();
			mp_pc=AddComponentsUntilEndOfLine(mp_function_params,mp_pc,mp_params);

			if (execute_return())
			{
				// If we returned from a script call caused by an interrupt, then do not continue
				// with execution of the interrupted script, since an interrupt should not affect the
				// interrupted script.
				// For the same reason we are bailing out here before the returned parameters are merged onto
				// the interrupted scripts params, so that they will not be affected by the interrupt either.
				return ESCRIPTRETURNVAL_FINISHED_INTERRUPT;
			}	
			
			// Note: mp_pc may be NULL at this point, if there is no calling script.

			// Merge the return values onto the parameters of the calling script.
			Dbg_MsgAssert(mp_params,("NULL mp_params ?"));
			mp_params->AppendStructure(mp_function_params);
			break;

        case ESCRIPTTOKEN_KEYWORD_RANDOM:
        case ESCRIPTTOKEN_KEYWORD_RANDOM2:
		case ESCRIPTTOKEN_KEYWORD_RANDOM_NO_REPEAT:
		case ESCRIPTTOKEN_KEYWORD_RANDOM_PERMUTE:
        case ESCRIPTTOKEN_JUMP:
			// Modify mp_pc according to any jump or random operators, and repeat until mp_pc no
			// longer points to a jump or random.
			mp_pc=DoAnyRandomsOrJumps(mp_pc);
			break;

        case ESCRIPTTOKEN_ENDOFLINE:
            ++mp_pc;
            break;
            
	    case ESCRIPTTOKEN_ENDOFLINENUMBER:
			mp_pc+=5; // 1 for the token, 4 for the line number.
            break;
		
        default:
			//++sInstructionCount;
			//printf("Running line %d in script '%s'\n",GetLineNumber(mp_pc),FindChecksumName(mScriptChecksum));
			execute_command();
			// Note: mp_pc may be NULL at this point
            break;
		}	
		
		#ifdef	__NOPT_ASSERT__
		
		if (m_being_watched_in_debugger)
		{
			Tmr::CPUCycles t=Tmr::GetTimeInCPUCycles()-instruction_start_time;
			float last_instruction_time_taken=((float)((int)t))/150.0f;
			
			m_last_instruction_time_taken+=last_instruction_time_taken;
			m_total_instruction_time_taken+=last_instruction_time_taken;
		}		
		
		if (m_single_step_mode==STEP_OVER &&
			m_num_return_addresses > m_last_num_return_addresses_when_halted)
		{
			// If the number of return addresses has increased beyond what it was when last halted
			// in the debugger, then the last command must have been a script call, so do not halt
			// but keep executing instructions until the script call has finished, which will be 
			// indicated by the return address count returning to what it was or lower.
			// (It would be lower if say a goto occurred during the script call)
			
		}
		else
		{
			if (m_wait_type == WAIT_TYPE_NONE)
			{
				advance_pc_to_next_line_and_halt();
			}	
		}	
		#endif	
    }
}

void CScript::Wait(int num_frames)
{
	m_wait_type=WAIT_TYPE_COUNTER;
    m_wait_timer=num_frames;
}

void CScript::WaitOnePerFrame(int num_frames)
{
	m_wait_type=WAIT_TYPE_ONE_PER_FRAME;
    m_wait_timer=num_frames;		 		
}


void CScript::WaitTime(Tmr::Time period)
{
	m_wait_type=WAIT_TYPE_TIMER;
	m_wait_period=period;
	// Record the start time.
	m_start_time=Tmr::GetTime();
}

bool CScript::RefersToScript(uint32 checksum)
{
	if (mScriptChecksum==checksum) 
	{
		return true;
	}	
	
	for (int i=0; i<m_num_return_addresses; ++i)
	{
		if (mp_return_addresses[i].mScriptNameChecksum==checksum) 
		{
			return true;
		}
	}		
	
	return false;
}

// Checks that all the global scripts referred to by this script still exist.
// Used by UnloadQB to stop any script which is missing one of its source scripts.
// This is essential to stop the script from having an invalid program counter pointer.											  
bool CScript::AllScriptsInCallstackStillExist()
{
	if (m_num_return_addresses==0)
	{
		// The call stack is empty, so we only have to consider the script being referred to
		// by mScriptChecksum.
		if (mp_struct_script)
		{
			// mScriptChecksum is not referring to a global script.
			// Rather, the script being run is a local script that was defined in a structure, a copy of which
			// is stored in mp_struct_script, so we know that still exists.
			return true;
		}
		else
		{
			// mScriptChecksum is referring to a global script, so check whether it still exists.
			if (LookUpSymbol(mScriptChecksum))
			{
				return true;
			}
			else
			{
				return false;
			}	
		}
	}

	// There are things in the call stack, so we know that mScriptChecksum (the name of the script
	// currently being executed) is referring to a global script, because local scripts cannot 
	// call local scripts. So check whether the mScriptChecksum symbol still exists.
	if (!LookUpSymbol(mScriptChecksum)) 
	{
		return false;
	}	
	
	// Now we have to check that each of the scripts in the call stack still exist.
	// These must all be global scripts, apart perhaps from the one at index 0. If mp_struct_script is not NULL
	// then the script at index 0 will be a local script that was defined in a structure, not a global script.
	// In that case, its mScriptNameChecksum is not referring to a global script, so must not be checked for
	// existence. (The mScriptNameChecksum will instead be the original parameter name in the structure)
	int i=0;
	if (mp_struct_script)
	{
		// Start checking at index 1 instead, cos we know that the script at index 0 exists. It is contained
		// in mp_struct_script.
		i=1;
	}	
	for (; i<m_num_return_addresses; ++i)
	{
		if (!LookUpSymbol(mp_return_addresses[i].mScriptNameChecksum)) 
		{
			return false;
		}
	}		
	
	return true;
}

bool CScript::RefersToObject(Obj::CObject *p_object)
{
	if (mpObject==p_object) 
	{
		return true;
	}	
	
	for (int i=0; i<m_num_return_addresses; ++i)
	{
		if (mp_return_addresses[i].mpObject==p_object) 
		{
			return true;
		}
	}		
	
	return false;
}

void CScript::RemoveReferencesToObject(Obj::CObject *p_object)
{
	if (mpObject==p_object) 
	{
		mpObject=NULL;
	}	
	
	for (int i=0; i<m_num_return_addresses; ++i)
	{
		if (mp_return_addresses[i].mpObject==p_object) 
		{
			mp_return_addresses[i].mpObject=NULL;
		}
	}		
}

// Returns the base script that is running, not the name of any called scripts that it is currently executing.
uint32 CScript::GetBaseScript()
{
	if (m_num_return_addresses)
	{
		// If in a called script, the base script will be the first thing in the stack.
		return mp_return_addresses[0].mScriptNameChecksum;												 
	}
	else
	{
		// Otherwise return the current script.
		return mScriptChecksum;
	}
}
		
void SendScript( uint32 scriptChecksum, CStruct *p_params, Obj::CObject *p_object )
{
	#ifdef __PLAT_WN32__
	// No GameNet stuff if compiling on PC
	#else
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Net::Client* client;
	GameNet::MsgRunScript msg;
	int size, msg_size;

	if( gamenet_man->InNetGame())
	{
		Net::MsgDesc msg_desc;
		msg.m_ScriptName = scriptChecksum;
		msg.m_ObjID = -1;
		if( p_object )
		{
			msg.m_ObjID = p_object->GetID();
		}

		size = WriteToBuffer(p_params, (uint8 *) msg.m_Data, GameNet::MsgRunScript::vMAX_SCRIPT_PARAMS_LEN );
		Dbg_MsgAssert( size <= GameNet::MsgRunScript::vMAX_SCRIPT_PARAMS_LEN,( "Script too large to send over the net\n" ));
		
		client = gamenet_man->GetClient( 0 );
		Dbg_Assert( client );

		msg_size = ( sizeof( GameNet::MsgRunScript ) - GameNet::MsgRunScript::vMAX_SCRIPT_PARAMS_LEN ) +
					size;
		msg_desc.m_Data = &msg;
		msg_desc.m_Length = msg_size;
		msg_desc.m_Id = GameNet::MSG_ID_RUN_SCRIPT;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
		client->EnqueueMessageToServer( &msg_desc );
	
	}
	#endif
}

#ifdef __NOPT_ASSERT__
static const char *sGetRunScriptName(uint32 scriptChecksum, const char *p_scriptName)
{
	if (p_scriptName)
	{
		return p_scriptName;
	}
	return FindChecksumName(scriptChecksum);
}	
#endif

// Used for running a simple script from start to end.
void RunScript(uint32 scriptChecksum, CStruct *p_params, Obj::CObject *p_object, bool netScript, const char *p_scriptName )
{
	// First, see what type of symbol scriptChecksum is referring to.
    CSymbolTableEntry *p_entry=Resolve(scriptChecksum);
	if (!p_entry)
	{
		#ifdef __NOPT_ASSERT__
		printf("Warning! RunScript could not find '%s'\n",sGetRunScriptName(scriptChecksum,p_scriptName));
		#endif
		return;
	}	
	if (p_entry->mType!=ESYMBOLTYPE_QSCRIPT && 
		p_entry->mType!=ESYMBOLTYPE_CFUNCTION &&
		p_entry->mType!=ESYMBOLTYPE_MEMBERFUNCTION)
	{
		#ifdef __NOPT_ASSERT__
		printf("Warning! RunScript sent '%s' which is not a script, a c-function or a member function. Type=%s\n",sGetRunScriptName(scriptChecksum,p_scriptName),GetTypeName(p_entry->mType));
		#endif
		return;
	}


	if( netScript )
	{
		SendScript( scriptChecksum, p_params, p_object );
	}

	switch (p_entry->mType)
	{
		case ESYMBOLTYPE_CFUNCTION:
		{
			// If the symbol is actually a c-function rather than a script, then run the c-function.
			// This is handy sometimes because it saves having to write a special script just to run 
			// one c-function.
			Dbg_MsgAssert(p_entry->mpCFunction,("NULL pCFunction"));

			// Mick:  We now pass in NULL, as creating a dummy script is very slow			
			(*p_entry->mpCFunction)(p_params,NULL);
			
			break;
		}
		
		case ESYMBOLTYPE_MEMBERFUNCTION:
		{
			Dbg_MsgAssert(p_object,("Tried to run member function '%s' on NULL p_object",Script::FindChecksumName(scriptChecksum)));
	
			// Mick:  We now pass in NULL, as creating a dummy script is very slow			
			p_object->CallMemberFunction(scriptChecksum,p_params,NULL);
			
			break;
		}
		
		case ESYMBOLTYPE_QSCRIPT:
		{
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
			CScript *p_script=new CScript;
			#ifdef __NOPT_ASSERT__
			p_script->SetCommentString("Created by RunScript(...)");
			#endif
			p_script->SetScript(scriptChecksum,p_params,p_object);
			Mem::Manager::sHandle().PopContext();
		
			while (true)
			{
				EScriptReturnVal ret_val=p_script->Update();
				if (ret_val==ESCRIPTRETURNVAL_FINISHED)
				{
					break;
				}
				//Dbg_MsgAssert(0,("\n%s\nScript did not finish when run by RunScript.",p_script->GetScriptInfo()));
				// Script must not get blocked, otherwise it'll hang in this loop forever.
				Dbg_MsgAssert(ret_val!=ESCRIPTRETURNVAL_BLOCKED,("\n%s\nScript got blocked when being run by RunScript.",p_script->GetScriptInfo()));
				// Note: OK if the script returns ESCRIPTRETURNVAL_WAITING, because the wait period will eventually end.
			
			}
			delete p_script;
			break;
		}	
		
		default:
			break;
	}	
}

void RunScript(const char *p_scriptName, CStruct *p_params, Obj::CObject *p_object, bool netScript )
{
	// Quite often, if a script cannot be found then FindChecksumName will not be able to find
	// the checksum name either, so since we know the name here it gets passed along to
	// the other version of RunScript so that the name can be printed in any warning message.
	RunScript(Crc::GenerateCRCFromString(p_scriptName), p_params, p_object, netScript, p_scriptName);
}

/////////////////////////////////////////////////////////////////
//
// 					Spawning script stuff
//
/////////////////////////////////////////////////////////////////

static bool s_updating_scripts = false;
static bool s_delete_scripts_pending = false;

// This gets called from within DeleteSymbols.
void DeleteSpawnedScripts()
{
	// Guard against deleting spawned scripts while we are updating them
	if( s_updating_scripts )
	{
		s_delete_scripts_pending = true;
		return;
	}

	CScript *p_script=GetNextScript();
	while (p_script)
	{
		CScript *p_next=GetNextScript(p_script);
		// Don't kill the script if it is not session-specific.
		// This feature is currently used by one of Steve's net scripts which needs to not
		// be killed by ScriptCleanup
		if (p_script->mIsSpawned && !p_script->mNotSessionSpecific)
		{
			delete p_script;
		}	
		p_script=p_next;
	}	
}

// This gets called from Front::PauseGame
void PauseSpawnedScripts(bool status)
{
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		if (p_script->mIsSpawned)
		{
			p_script->mPaused=status;
		}
		p_script=GetNextScript(p_script);
	}	
}

void UnpauseSpawnedScript(CScript* p_script)
{
	if (p_script && p_script->mIsSpawned)
	{
		p_script->mPaused=false;
	}
}

// Note: Does this need to be fast?
uint32 NumSpawnedScriptsRunning()
{
	uint32 num_spawned_scripts=0;
	
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		if (p_script->mIsSpawned)
		{
			++num_spawned_scripts;
		}
		p_script=GetNextScript(p_script);
	}	
	
	return num_spawned_scripts;
}

// Called once a frame. Currently called from Skate::DoUpdate()
void UpdateSpawnedScripts()
{

	s_updating_scripts = true;
	s_done_one_per_frame = false;
	
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		CScript *p_next;
		// get the next actual spawned script
		p_next=GetNextScript(p_script);		
		// skip any non-spawned scripts
		// since they might get deleted in an unsafe manner
		while (p_next && !p_next->mIsSpawned)
		{
			p_next=GetNextScript(p_next);				
		}
		
		Dbg_MsgAssert(GetNextScript(p_next) != (CScript*)-1,("Next script in spawned list has been deleted sometime earlier!"));
		if (p_next == p_script)
		{
			Dbg_MsgAssert(0,("%s\nLoop in list of spawned scripts!!",p_script->GetScriptInfo()));
		}
		
		// K: Added the || !p_script->GotScript() so that spawned scripts which have been cleared still get
		// cleaned up when the game is paused. This was to fix a bug (TT1453) where cleared scripts where accumulating
		// during the cutscenes. The scripts were the scripts of goal peds, which got cleared when the peds were
		// killed. 
		if (p_script->mIsSpawned && (!p_script->mPaused || !p_script->GotScript()) && (!p_script->mPauseWithObject || !p_script->mpObject || p_script->mpObject->ShouldUpdatePauseWithObjectScripts()))
		{
			if (p_script->Update()==ESCRIPTRETURNVAL_FINISHED)
			{
				// just doing the assertion before we delete the script  
				Dbg_MsgAssert(GetNextScript(p_next) != (CScript*)-1,("%s\nNext script in spawned list has been deleted by this script updating",p_script->GetScriptInfo()));
				// If it had a callback script specified, run it.
				if (p_script->mCallbackScript)
				{
					RunScript(p_script->mCallbackScript,
							  p_script->mpCallbackScriptParams,
							  p_script->mpObject);
				}

				// just doing the assertion before we delete the script  
				Dbg_MsgAssert(GetNextScript(p_next) != (CScript*)-1,
				("Next script in spawned list has been deleted by callback script (%s)",FindChecksumName(p_script->mCallbackScript)));
				// Kill it now that it has finished.
				delete p_script;
			}
			else
			{
				Dbg_MsgAssert(GetNextScript(p_next) != (CScript*)-1,("%s\nNext script in spawned list has been deleted by this script",p_script->GetScriptInfo()));
			}
		}	
			
		p_script=p_next;
		Dbg_MsgAssert(GetNextScript(p_script) != (CScript*)-1,("Next script in spawned list has been deleted by this script"));
	}	


	s_updating_scripts = false;
	if( s_delete_scripts_pending )
	{
		DeleteSpawnedScripts();
		s_delete_scripts_pending = false;
	}


}

// Sned spawn script events to other clients
void SendSpawnScript( uint32 scriptChecksum, Obj::CObject *p_object, int node, bool permanent )
{
	#ifdef __PLAT_WN32__
	// No GameNet stuff if compiling on PC
	#else
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Net::Client* client;
	GameNet::MsgSpawnAndRunScript msg;
    
	if( gamenet_man->InNetGame())
	{
		Net::MsgDesc msg_desc;

		msg.m_ScriptName = scriptChecksum;
		msg.m_ObjID = -1;
		msg.m_Node = node;
		msg.m_Permanent = (char) permanent;
		if( p_object )
		{
			msg.m_ObjID = p_object->GetID();
		}

		client = gamenet_man->GetClient( 0 );
		Dbg_Assert( client );

		msg_desc.m_Data = &msg;
		msg_desc.m_Length = sizeof( GameNet::MsgSpawnAndRunScript );
		msg_desc.m_Id = GameNet::MSG_ID_SPAWN_AND_RUN_SCRIPT;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
		client->EnqueueMessageToServer( &msg_desc );
	}
	#endif
}


CScript *GetScriptWithUniqueId(uint32 id)
{
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		if (p_script->GetUniqueId() == id) 
			return p_script;
		p_script = GetNextScript(p_script);
	}	

	return NULL;
}

// Called from ScriptSpawnScript in cfuncs.cpp
// also called by the triggering code in skater.cpp
// returns the new script if sucessful, asserts if not
// optional "node" parameter is the node number that is
// responsible for spawning this script.
// Also now takes an optional Id, to allow individual spawned script instances to be killed.
CScript* SpawnScript(uint32 scriptChecksum, CStruct *p_scriptParams, uint32 callbackScript, CStruct *p_callbackParams, int node, uint32 id,
					 bool netEnabled, bool permanent, bool not_session_specific, bool pause_with_object )
{
	Dbg_MsgAssert(scriptChecksum,("Zero checksum sent to SpawnScript"));
	
    CSymbolTableEntry *p_entry=Resolve(scriptChecksum);
    if (p_entry)
    {
        if (p_entry->mType==ESYMBOLTYPE_CFUNCTION)
		{
			Dbg_MsgAssert(callbackScript==0,("A callbackScript cannot currently be specified when running SpawnScript on a c-function. (cfunc='%s' callback='%s')",FindChecksumName(scriptChecksum),FindChecksumName(callbackScript)));
			
			// Creating a dummy script to send to the c-function. Normally the c-function would
			// only be called from within a script's update function, and it uses the passed script 
			// pointer in any call to GetScriptInfo if an assert goes off. 
			// So we need to pass a dummy rather than NULL so that it does not crash if it
			// dereferences the pointer.
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
			CScript *p_dummy=new CScript;
			Mem::Manager::sHandle().PopContext();
			
			(*p_entry->mpCFunction)(p_scriptParams,p_dummy);
			
			delete p_dummy;
			return NULL;
		}
		Dbg_MsgAssert(p_entry->mType!=ESYMBOLTYPE_MEMBERFUNCTION,("SpawnScript cannot run the member function '%s'",FindChecksumName(scriptChecksum)));
	}	
	
	
	CScript *p_script=new CScript;
	p_script->SetScript(scriptChecksum, p_scriptParams, NULL);
	#ifdef __NOPT_ASSERT__
	p_script->SetCommentString("Created by SpawnScript");
	#endif
	
	
	p_script->mNode = node;
	p_script->mIsSpawned = true;
	p_script->mId = id;
	p_script->mPaused = false;
	p_script->mNotSessionSpecific=not_session_specific;
	p_script->mPauseWithObject = pause_with_object;
	
	Dbg_MsgAssert(p_script->mpCallbackScriptParams==NULL,("p_script->mpCallbackScriptParams not NULL ?"));
	if (callbackScript)
	{
		p_script->mCallbackScript=callbackScript;
		p_script->mpCallbackScriptParams=new CStruct;
		if (p_callbackParams)
		{
			*p_script->mpCallbackScriptParams+=*p_callbackParams;
		}	
	}
		
	#ifdef __PLAT_WN32__
	// No GameNet stuff if compiling on PC
	#else
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	if( netEnabled && gamenet_man->InNetGame())
	{
		// TODO: Should pass on the not_session_specific flag ...
		SendSpawnScript( scriptChecksum, NULL, node, permanent );
	}
	#endif

	return p_script;
}


bool	ScriptExists(uint32 scriptNameChecksum)
{
	CSymbolTableEntry *p_entry=Resolve(scriptNameChecksum);
	if (p_entry && p_entry->mType==ESYMBOLTYPE_QSCRIPT)
	{
		return true;
	}
	return false;
}

CScript* SpawnScript(const char *p_scriptName, CStruct *p_scriptParams, uint32 callbackScript, CStruct *p_callbackParams, int node, uint32 id, 
					 bool netEnabled, bool permanent, bool not_session_specific, bool pause_with_object )
{
    
	return SpawnScript(Crc::GenerateCRCFromString(p_scriptName),p_scriptParams,callbackScript,p_callbackParams,node,id, netEnabled, permanent, not_session_specific, pause_with_object);
}

// Kills a spawned script, given a pointer to the actual script
void KillSpawnedScript(CScript *p_script)
{
    if (p_script && p_script->mIsSpawned)
	{
		delete p_script;
	}	
}


// Kills all spawned scripts that are either currently running this script directly,
// or have it in their call stack somewhere. (Ie, they will return to it later)
void KillSpawnedScriptsThatReferTo(uint32 checksum)
{
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		CScript *p_next=GetNextScript(p_script);
		if (p_script->mIsSpawned && p_script->RefersToScript(checksum))
		{
//			delete p_script;
			p_script->ClearScript();
			p_script->ClearEventHandlerTable();
		}	
		p_script=p_next;
	}	
}

// Kills all scripts with this particular Id.
void KillSpawnedScriptsWithId(uint32 id)
{
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		CScript *p_next=GetNextScript(p_script);
		if (p_script->mIsSpawned && (p_script->mId==id || p_script->GetUniqueId()==id))
		{
// Mick, instead of deleteing the script, we now just clear it
// this stops it from executing, but leaves it in the linked list
// so it can be cleaned up next time around "UpdateSpawnedScripts"
// Scripts that have ClearScript() called will automatically be deleted
// as they have mp_pc set to NULL
//			delete p_script;
			p_script->ClearScript();
			p_script->ClearEventHandlerTable();
		}	
		p_script=p_next;
	}	
}

// Kills all spawned scripts that have the passed object as their parent object.
// This gets called from within Script::KillAllScriptsWithObject.
void KillSpawnedScriptsWithObject(Obj::CObject *p_object)
{
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		CScript *p_next=GetNextScript(p_script);
		if (p_script->mIsSpawned && p_script->RefersToObject(p_object))
		{
//			delete p_script;
			p_script->ClearScript();
			p_script->ClearEventHandlerTable();
		}	
		p_script=p_next;
	}	
}

// Kills all spawned scripts with a particular Id & parent object.
// Called by Matt's Obj_KillSpawnedScript CObject script member function.
void KillSpawnedScriptsWithObjectAndId(Obj::CObject *p_object, uint32 id, CScript *p_callingScript)
{
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		CScript *p_next=GetNextScript(p_script);
		if (p_script->mIsSpawned && 
			p_script!=p_callingScript &&
			p_script->RefersToObject(p_object) &&
			p_script->mId==id)
		{
//			delete p_script;
			p_script->ClearScript();
			p_script->ClearEventHandlerTable();
		}	
		p_script=p_next;
	}	
}

// Kills all spawned scripts with a particular Id & parent object.
// Called by Matt's Obj_KillSpawnedScript CObject script member function.
void KillSpawnedScriptsWithObjectAndName(Obj::CObject *p_object, uint32 name, CScript *p_callingScript )
{
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		CScript *p_next=GetNextScript(p_script);
		if (p_script->mIsSpawned && 
			p_script!=p_callingScript &&
			p_script->RefersToObject(p_object) &&
			p_script->mScriptChecksum==name)
		{
//			delete p_script;
			p_script->ClearScript();
			p_script->ClearEventHandlerTable();
		}	
		p_script=p_next;
	}	
}

// Returns: 0 if spawned script has no ID, or if the script isn't a spawned script
// Otherwise, returns the ID:
uint32 FindSpawnedScriptID(CScript *p_script)
{
	if (!p_script)
	{
		return 0;
	}
	
	if (!p_script->mIsSpawned)
	{
		return 0;
	}

	return p_script->mId;		
}

CScript* FindSpawnedScriptWithID(uint32 id)
{
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		CScript *p_next=GetNextScript(p_script);
		if (p_script->mIsSpawned && 
			p_script->mId==id)
		{
			return p_script;
		}	
		p_script=p_next;
	}
	
	return NULL;
}


// TODO: This just included to allow compilation of GenerateCRC, remove later.
uint32 GenerateCRC(const char *p_string)
{
	return Crc::GenerateCRCFromString(p_string);
}

// Stops all scripts.
// Currently only used by the StopAllScripts script function, which currently isn't used
// anywhere, but might be handy one day.
void StopAllScripts()
{
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		p_script->ClearScript();
		p_script->ClearEventHandlerTable();
		p_script=GetNextScript(p_script);
	}	
}

// Stops all scripts with a particular object as their parent, but does not delete them.
// They cannot be restarted after being stopped.
// This is basically a way of killing the scripts without actually
// deleting them, so it won't cause any invalid script pointers.
// Currently gets called at the end of the CObject destructor.
void StopAllScriptsUsingThisObject(Obj::CObject *p_object)
{
	StopScriptsUsingThisObject(p_object, 0);
}

// If scriptCrc != 0, then only stop scripts with that ID, otherwise stop all scripts using object
void StopScriptsUsingThisObject(Obj::CObject *p_object, uint32 scriptCrc)
{
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		if (p_script->RefersToObject(p_object) && (!scriptCrc || p_script->mScriptChecksum == scriptCrc))
		{
			p_script->ClearScript();
			p_script->ClearEventHandlerTable();
		}	
		p_script=GetNextScript(p_script);
	}	
}

// TODO: brad - this is a last minute fix to StopScriptsUsingThisObject.
// Rather than fixing the existing function so close to release, I've added
// this version to be called only when we know we want to use it.
void StopScriptsUsingThisObject_Proper(Obj::CObject *p_object, uint32 scriptCrc)
{
	CScript *p_script=GetNextScript();
	while ( p_script )
	{
		if ( p_script->RefersToObject( p_object ) && ( !scriptCrc || p_script->RefersToScript( scriptCrc ) ) )
		{
			p_script->ClearScript();
			p_script->ClearEventHandlerTable();
		}	
		p_script=GetNextScript( p_script );
	}	
}


// Kills any stopped scripts, ie scripts with their mpPC null. These
// would be scripts stopped by the above StopAllScriptsUsingThisObject
void KillStoppedScripts()
{
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		CScript *p_next=GetNextScript(p_script);
		
		// Delete the script if it has stopped (mpPC==NULL), but not if it is a spawned script,
		// because otherwise it will leave a dangling pointer in the array of spawned scripts,
		// which will then get dereferenced in UpdateSpawnedScripts and crash.
		// If the script has stopped, the next call to UpdateSpawnedScripts will delete it, so
		// the script is guaranteed to get cleaned up eventually.
		
		#ifdef	__SCRIPT_EVENT_TABLE__		
		if (!p_script->GotScript() && !p_script->mIsSpawned && !p_script->GetEventHandlerTable())
		#else
		if (!p_script->GotScript() && !p_script->mIsSpawned)
		#endif
		{
			
			
			#ifdef __NOPT_ASSERT__
			printf("Cleaning up script '%s'\n",FindChecksumName(p_script->mScriptChecksum));
			#endif
			
			delete p_script;
		}	
		
		p_script=p_next;
	}	
}

// Run by the DumpScripts script function.
// Prints out the names of all the currently existing scripts.
void DumpScripts()
{
	#ifdef __NOPT_ASSERT__
	printf("###########################################################\n\n");
	printf("All the CScripts that currently exist ...\n\n");
	
	CScript *p_scr=GetNextScript();
	int n = 0;
	while (p_scr)
	{
		printf ("%3d %8d ",p_scr->m_last_time/150, p_scr->m_total_time/150);
		p_scr->m_last_time = 0;
		p_scr->m_total_time = 0;	
					  
		printf ("%3d: ",n++);
		if (p_scr->mIsSpawned)
		{
			printf("S ");
		}		
		else
		{
			printf("  ");
		}

		switch (p_scr->GetWaitType())
		{
		case WAIT_TYPE_NONE:
			printf("None            ");
			break;
		case WAIT_TYPE_COUNTER:
			printf("Counter         ");
			break;
		case WAIT_TYPE_ONE_PER_FRAME:
			printf("one_per_frame   ");
			break;
		case WAIT_TYPE_TIMER:
			printf("Timer           ");
			break;
		case WAIT_TYPE_BLOCKED:
			printf("Blocked         ");
			break;
		case WAIT_TYPE_OBJECT_MOVE:
			printf("Ob Move         ");
			break;
		case WAIT_TYPE_OBJECT_ANIM_FINISHED:
			printf("Ob AnimFinished ");
			break;
		case WAIT_TYPE_OBJECT_JUMP_FINISHED:
			printf("Ob JumpFinished ");
			break;
		case WAIT_TYPE_OBJECT_ROTATE:
			printf("Ob Rotate       ");
			break;
		default:
			printf("Unknown ??????  ");
			break;
		}	

		if (p_scr->Finished())
		{
			printf("[Fin] ");
		}
			
		uint32 checksum=p_scr->GetBaseScript();
		if (checksum)
		{
			CSymbolTableEntry *p_sym=LookUpSymbol(checksum);
			Dbg_MsgAssert(p_sym,("NULL pSym ??"));
	
//			printf("%s: %s\n",FindChecksumName(p_sym->mSourceFileNameChecksum),FindChecksumName(checksum));
			printf("%s\n",FindChecksumName(checksum));
		}
		else
		{
			printf (", Checksum is NULL, probably dead script\n");
		}
		
		p_scr=GetNextScript(p_scr);
	}	
	printf("\n");
	#endif
}


#ifdef	__SCRIPT_EVENT_TABLE__		

/*
	Sets up the table that specifies which scripts to run in response to which events.
	
	See object scripting document.
*/
void CScript::SetEventHandlers(Script::CArray *pArray, EReplaceEventHandlers replace)
{
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
	if (!mp_event_handler_table)
	{
		mp_event_handler_table = new Obj::CEventHandlerTable();
	}
	else
	{
		mp_event_handler_table->unregister_all(this);	
	}
	mp_event_handler_table->add_from_script(pArray, replace);
	
	// Mick:  We still need the compress_table for this way of adding event handlers
	// as the "replace" logic otherwise leads to a rapidly growing table.
	// especially on the on-screen keyboard
	mp_event_handler_table->compress_table();
	
	mp_event_handler_table->register_all(this);

	Mem::Manager::sHandle().PopContext();

}


void	CScript::SetEventHandler(uint32 ex, uint32 scr, uint32 group, bool exception, CStruct *p_params)
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());

	// Need to create the table if not there	
	if (!mp_event_handler_table)
	{
		mp_event_handler_table = new Obj::CEventHandlerTable();
	}
	
	// Add the event to the table
	mp_event_handler_table->AddEvent(ex,scr,group, exception, p_params); 	

	// Inregistering the event receiver, all we are doing is adding an
	// script pointer to the list of scripts that handle this 
	// type of exception
	// there is nothing specific to the actual handler
	// if the script is already in the list, then nothing needs changing. 
	Obj::CTracker::Instance()->RegisterEventReceiver(ex, this); 

	Mem::Manager::sHandle().PopContext();
}



/*
	Removes an entry in the event table with the given type id
*/
void CScript::RemoveEventHandler(uint32 type)
{
	if (!mp_event_handler_table) return;
	mp_event_handler_table->remove_entry(type);
//	mp_event_handler_table->compress_table();

// Refresh the Obj::Tracker

	Obj::CTracker::Instance()->UnregisterEventReceiver(type, this); 
		
}



/*
	Removes all entries in the event table with the given group id
*/
void CScript::RemoveEventHandlerGroup(uint32 group)
{
	if (!mp_event_handler_table) return;
	
	mp_event_handler_table->unregister_all(this);	
	
	mp_event_handler_table->remove_group(group);
//	mp_event_handler_table->compress_table();
	
	mp_event_handler_table->register_all(this);	

}

/*
	Dumps the event table
*/
void CScript::PrintEventHandlerTable (   )
{
	Obj::CEventHandlerTable::sPrintTable(mp_event_handler_table);
}

bool CScript::PassTargetedEvent(Obj::CEvent *pEvent, bool broadcast)
{
	if (mp_event_handler_table)
		mp_event_handler_table->pass_event(pEvent, this, broadcast);
	return true;
}

void 	CScript::SetOnExceptionScriptChecksum(uint32 OnExceptionScriptChecksum) 
{
	mOnExceptionScriptChecksum = OnExceptionScriptChecksum;
}

uint32 	CScript::GetOnExceptionScriptChecksum() const 
{
	return mOnExceptionScriptChecksum;
}

void 	CScript::SetOnExitScriptChecksum(uint32 OnExitScriptChecksum) 
{
	mOnExitScriptChecksum = OnExitScriptChecksum;
}

uint32 	CScript::GetOnExitScriptChecksum() const 
{
	return mOnExitScriptChecksum;
}



#endif

} // namespace Script


