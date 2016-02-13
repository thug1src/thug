#ifndef __GEL_SCRIPTING_DEBUGGER_H__
#define __GEL_SCRIPTING_DEBUGGER_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __GEL_MODULE_H
#include <gel/module.h>
#endif

#ifndef __NET_H__
#include <gel/net/net.h>
#endif

#ifndef __NETSERV_H__
#include <gel/net/server/netserv.h>
#endif

#ifndef	__GFX_NXMODEL_H__
#include <gfx/nxmodel.h>
#endif

#ifndef __GEL_OBJPTR_H
#include <gel/objptr.h>
#endif

// This is outside the #ifdef __NOPT_ASSERT__ so that they can be used in the PC code,
// which includes this file but does not have __NOPT_ASSERT__ defined.
namespace Obj
{
	class CObject;
	class CCompositeObject;
	struct SException;
}

namespace Dbg
{

// Points to the fixed area in high memory where the game code stores stuff for the
// script debugger to read using the target manager API.
// (Note: The script debugger app includes this file, so has access to these defines)
// Currently, the first thing stored there is a SChecksumNameInfo structure.
// The game code will assert if this value does not match the symbol _script_debugger_start
enum
{
	SCRIPT_DEBUGGER_MEMORY_START=0x06E00080
};

// Contains pointers to the start and end of the big list of names whose checksums are
// loaded. (Eg, every word in every q file currently loaded)
// This lets the script debugger register all those checksums when it starts up, so that
// it can resolve checksums to names. 
struct SChecksumNameInfo
{
	char **mppStart;
	char **mppEnd;
};



// Used in the vMSG_ID_DBG_WATCH_SCRIPT packet.
enum
{
	CHECKSUM_IS_SCRIPT_NAME,
	CHECKSUM_IS_SCRIPT_ID,
};

// An array of these gets passed from the PS2 to the debugger in the
// vMSG_ID_DBG_WATCH_LIST meesage
struct SWatchInfo
{
	uint32 mScriptName;
	bool mStopImmediately;
};

enum EObjectUpdateMode
{
	EVERY_FRAME,
	SINGLE_STEP,
};	
	
// One of these gets passed to the PS2 from the debugger in the
// vMSG_ID_DBG_SET_OBJECT_UPDATE_MODE message.
struct SObjectUpdateMode
{
	uint32 mObjectId;
	EObjectUpdateMode mMode;
	bool mOn;
};

// One of these gets passed to the PS2 from the debugger in the
// vMSG_ID_DBG_VIEW_OBJECT message.

struct SViewObject
{
	uint32 mObjectId;
	bool mDoViewObject;
};

struct SSplitMessageHeader
{
	char mOriginalMessageId;
	uint32 mOriginalMessageSize;
	
	int mTotalNumMessages;
	
	uint32 mThisMessageId;
	uint32 mThisMessageSize;
	int mThisMessageIndex;
};

enum
{
	// This makes each sub-packet be as big as possible. Netapp.cpp will assert if the total
	// packet size is >= Net::Manager::vMAX_PACKET_SIZE - 64
	// The sub-packet includes a SSplitMessageHeader, hence that size has to be subtracted too.
	SPLIT_MESSAGE_MAX_SIZE=Net::Manager::vMAX_PACKET_SIZE - 65 - sizeof(SSplitMessageHeader),
};

}

#ifdef __NOPT_ASSERT__

namespace Net
{
	class MsgDesc;
}	

namespace Dbg
{

enum
{
	SCRIPT_DEBUGGER_BUFFER_SIZE=60*1024, // Must be a multiple of 4
};	

enum EWatchType
{
	NONE=0,
	SCRIPT,
	SCRIPT_GLOBAL,
	OBJECT_ID,
	EXCEPTION,
};

#define MAX_WATCHED_SCRIPTS 100
struct SWatchedScript
{
	uint32 mChecksum;
	bool mStopScriptImmediately;
};

struct SStateWhenLeftPressed
{
	int mMouseX;
	int mMouseY;
	float mCamTheta;
	float mCamPhi;
	Tmr::CPUCycles mTime;
};

struct SStateWhenRightPressed
{
	int mMouseX;
	int mMouseY;
	float mCamRadius;
};

class CScriptDebugger  : public Mdl::Module
{
	DeclareSingletonClass( CScriptDebugger );

	Net::Server *mp_server;
	// Used by the SplitAndEnqueueMessage function to give an id to the message
	// that is being split. Each of the sub-messages that the message is split into
	// is given this id so that they can be reconstructed at the other end.
	// It increments by 1 each time it is used.
	uint32 m_split_message_id;
	void send_sub_packet(Net::MsgDesc *p_message, uint32 size, char *p_source,
						 int packetIndex, int totalNumPackets);
	
	enum EState
	{
		SCRIPT_DEBUGGER_STATE_IDLE,
	};
		
	EState m_state;
	char *mp_checksum_names_current_pos;
	char *mp_checksum_names_end;

	int m_num_watched_scripts;
	bool m_paused;
	SWatchedScript mp_watched_scripts[MAX_WATCHED_SCRIPTS];
	
	bool m_mouse_on_screen;
	bool m_got_valid_mouse_position;
	int m_mouse_x;
	int m_mouse_y;
	Obj::CCompositeObject *mp_currently_selected_object; // TODO: Change to be a smart pointer !!!
	Nx::ERenderMode m_original_object_render_mode;
	bool m_left_button_down;	
	SStateWhenLeftPressed m_state_when_left_pressed;
	
	bool m_right_button_down;	
	SStateWhenRightPressed m_state_when_right_pressed;
	
	void left_button_down();
	void left_button_up();
	void right_button_down();

	void update_mouse_cursor();
	void switch_off_mouse_cursor();
	
	Obj::CSmtPtr<Obj::CCompositeObject> mp_viewed_object;
	bool m_in_object_single_step_mode;
	uint32 m_single_step_object_id;

	int m_object_update_total_bytes_sent;
	int m_frames_since_last_object_update;
	
	float m_cam_radius;
	float m_cam_theta;
	float m_cam_phi;
	float m_cam_focus_offset;
	void setup_default_camera_viewing_position();
	void update_camera();
	
	void v_start_cb ( void );
	void v_stop_cb ( void );
	
	static Tsk::Task< CScriptDebugger >::Code s_logic_code;       
	Tsk::Task< CScriptDebugger > *mp_logic_task;
	
	static Net::MsgHandlerCode s_handle_connection_request;
	static Net::MsgHandlerCode s_handle_watch_this;
	static Net::MsgHandlerCode s_handle_watch_script;

	static Net::MsgHandlerCode s_handle_stop;
	static Net::MsgHandlerCode s_handle_step_into;
	static Net::MsgHandlerCode s_handle_step_over;
	static Net::MsgHandlerCode s_handle_go;
	static Net::MsgHandlerCode s_handle_clear_script_watches;
	static Net::MsgHandlerCode s_handle_refresh;
	static Net::MsgHandlerCode s_handle_disconnect;
	static Net::MsgHandlerCode s_handle_send_cscript_list;
	static Net::MsgHandlerCode s_handle_send_watch_list;
	static Net::MsgHandlerCode s_handle_stop_watching_this_index;
	static Net::MsgHandlerCode s_handle_stop_watching_this_cscript;
	static Net::MsgHandlerCode s_handle_send_composite_object_info;
	static Net::MsgHandlerCode s_handle_mouse_position;
	static Net::MsgHandlerCode s_handle_mouse_on_screen;
	static Net::MsgHandlerCode s_handle_mouse_off_screen;
	static Net::MsgHandlerCode s_handle_mouse_left_button_down;
	static Net::MsgHandlerCode s_handle_mouse_left_button_up;
	static Net::MsgHandlerCode s_handle_mouse_right_button_down;
	static Net::MsgHandlerCode s_handle_mouse_right_button_up;
	static Net::MsgHandlerCode s_handle_send_composite_object_list;
	static Net::MsgHandlerCode s_handle_set_object_update_mode;
	static Net::MsgHandlerCode s_handle_view_object;
	static Net::MsgHandlerCode s_handle_send_script_global_info;

	void add_to_watched_scripts(uint32 checksum, bool stopScriptImmediately);
	SWatchedScript *find_watched_script(uint32 checksum);
	void refresh();
	void stop_debugging();
	void stop_watching_script(int index);
	int  send_composite_object_info(Obj::CCompositeObject* p_obj);
	void send_composite_object_info(uint32 id);
	void send_script_global_info(uint32 id);
	void do_any_every_frame_object_updates();
	void stop_all_every_frame_object_updates();
	void send_any_single_step_object_info();
	
	void set_object_update_mode(SObjectUpdateMode *pObjectUpdateMode);
	void view_object(SViewObject *pViewObject);
	
	void process_net_tasks_loop( void );
	void check_for_timeouts( void );
	
	void transmit_cscript_list();
	void transmit_watch_list();
	void transmit_composite_object_list();
	void transmit_compression_lookup_table();

public:
	CScriptDebugger();
	virtual	~CScriptDebugger();
	
	void EnqueueMessage(Net::MsgDesc *p_message);
	void StreamMessage(Net::MsgDesc *p_message);
	
	void SplitAndEnqueueMessage(Net::MsgDesc *p_message);
	
	void ScriptDebuggerPause( void );
	void ScriptDebuggerUnpause( void );

	void TransmitExceptionInfo(Obj::SException *p_exception, Obj::CCompositeObject* p_obj);
	SWatchedScript *GetScriptWatchInfo(uint32 checksum);
};

extern uint32 gpDebugInfoBuffer[];

} // namespace Dbg
#endif // __NOPT_ASSERT__

#endif // #ifndef __GEL_SCRIPTING_DEBUGGER_H__

