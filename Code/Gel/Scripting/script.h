#ifndef	__SCRIPTING_SCRIPT_H
#define	__SCRIPTING_SCRIPT_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef	__SCRIPTING_SCRIPTDEFS_H
// TODO: Remove this at some point, only necessary for the CScriptStructure define, to make everything compile.
#include <gel/scripting/scriptdefs.h>
#endif

#ifndef __SYS_TIMER_H
#include <sys/timer.h> // For Tmr::Time
#endif

#ifndef __SYS_MEM_POOLABLE_H
#include <sys/mem/poolable.h>
#endif

#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif


#include <gel/RefCounted.h>


namespace Obj
{
	class CBaseComponent;
	class CEvent;
	class CEventHandlerTable;
}
	
namespace Script
{

// TODO: Remove this at some point, only necessary to make the other game-code headers compile without having
// go through them all including vecpair.h
// Currently they assume everything is declared in script.h
class CPair;

class CStruct;
class CSymbolTableEntry;
struct SStructScript;

// The maximum level of Begin-Repeat nesting.
#define MAX_NESTED_BEGIN_REPEATS 10
#define NESTED_BEGIN_REPEATS_SMALL_BUFFER_SIZE 1

// The maximum number of nested calls to other scripts.                              
#define MAX_RETURN_ADDRESSES 16
// The number of SReturnAddress structures in the small buffer in each CScript
// Mostly 2 is all that is needed, so a buffer of MAX_RETURN_ADDRESSES is only allocated when needed.
#define RETURN_ADDRESSES_SMALL_BUFFER_SIZE 2

// Return values from CScript::Update()
enum EScriptReturnVal
{
    ESCRIPTRETURNVAL_FINISHED,
    ESCRIPTRETURNVAL_BLOCKED,
	ESCRIPTRETURNVAL_WAITING,
    ESCRIPTRETURNVAL_ERROR,
	ESCRIPTRETURNVAL_STOPPED_IN_DEBUGGER,
	ESCRIPTRETURNVAL_FINISHED_INTERRUPT,
};

// Values for CScript::m_wait_type
enum EWaitType
{
	WAIT_TYPE_NONE=0,
	WAIT_TYPE_COUNTER,
	WAIT_TYPE_TIMER,
	WAIT_TYPE_BLOCKED,
	WAIT_TYPE_OBJECT_MOVE,
	WAIT_TYPE_OBJECT_ANIM_FINISHED,
	WAIT_TYPE_OBJECT_JUMP_FINISHED,
	WAIT_TYPE_OBJECT_STOP_FINISHED,
	WAIT_TYPE_OBJECT_ROTATE,
	WAIT_TYPE_STREAM_FINISHED,
	WAIT_TYPE_ONE_PER_FRAME,
};

enum ESingleStepMode
{
	OFF=0,
	WAITING,
	STEP_INTO,
	STEP_OVER,
};

enum
{
	DEFINITELY_TRANSMIT=1
};
	
// Begin-repeat loop structure.    
struct SLoop
{
    // Pointer to the start of the loop, which is the
    // token following the Begin.
    uint8 *mpStart;
	
	// Pointer to the end of the loop, ie the next instruction after the loop.
	uint8 *mpEnd;
    
	bool mGotCount;			// Whether it is a finite loop with a count value.
	bool mNeedToReadCount;	// Whether the count value following the repeat needs to be read.
    int mCount;				// Counts down to zero, and skips past the repeat once it reaches zero.
};

// Saves the original location & other info when calling a subroutine.
struct SReturnAddress
{
    uint32 mScriptNameChecksum;
    CStruct *mpParams;
    uint8 *mpReturnAddress;
	Obj::CObject *mpObject;
	SLoop *mpLoop;
	#ifdef __NOPT_ASSERT__
	uint32 m_if_status;
	#endif
	EWaitType mWaitType;
	Obj::CBaseComponent *mpWaitComponent;
    int mWaitTimer;
	Tmr::Time mStartTime;
	Tmr::Time mWaitPeriod;
	bool mInterrupted;
};

// Script class.
// To run a script, one must create one of these, then call the SetScript member function to
// set which script it is to run.
//
// Then the Update function must be called repeatedly.
// The script will never die by itself, it must be deleted.
//
// Poolable to speed up allocation & deallocation.
// MEMOPT sizeof(CScript) was 1080 last time I checked, due to the arrays of 
// loops, return addresses and if's. Could maybe allocate these off pools instead, if the number
// of CScript's ever gets big enough to be using up a substantial amount of memory.
// There would be a slight speed hit if pools were used, but probably not much, 
// allocating/deallocating off a pool is pretty quick.
#ifdef __PLAT_WN32__
class CScript
#else
class CScript : public Mem::CPoolable<CScript>
#endif
{
	// CScripts are linked into a list.
	CScript *mp_next;
	CScript *mp_previous;
	friend CScript *GetNextScript(CScript *p_script=NULL);
	
	// If this CScript got setup using a SStructScript (via the SetScript member function below)
	// then this will be a pointer to a copy of the script, and mp_pc will point somewhere into it.
	// Otherwise mp_struct_script will be left as NULL.
	uint8 *mp_struct_script;
	
    // The program counter.
    uint8 *mp_pc;
	
	// Holds the parameters for passing to function calls.
	CStruct *mp_function_params;

    // The input parameters, which get accessed within the script using the <,> operator.
    CStruct *mp_params;
	
    // Begin-Repeat loop stuff.
    // NULL if not in a loop, otherwise points into the following array.
    SLoop *mp_current_loop;
	// If the number of loops needs to be bigger than NESTED_BEGIN_REPEATS_SMALL_BUFFER_SIZE then
	// mp_loops will point to a dynamically allocated array of MAX_NESTED_BEGIN_REPEATS SLoop structures,
	// otherwise mp_loops will equal mp_loops_small_buffer
    SLoop mp_loops_small_buffer[NESTED_BEGIN_REPEATS_SMALL_BUFFER_SIZE];
	SLoop *mp_loops;
	
	

    // Call stack.
    int m_num_return_addresses;
    SReturnAddress mp_return_addresses_small_buffer[RETURN_ADDRESSES_SMALL_BUFFER_SIZE];
	// If the num return addresses needs to be bigger than RETURN_ADDRESSES_SMALL_BUFFER_SIZE then
	// mp_return_addresses will point to a dynamically allocated array of MAX_RETURN_ADDRESSES SReturnAddress structures,
	// otherwise mp_return_addresses will equal mp_return_addresses_small_buffer
	SReturnAddress *mp_return_addresses;
	
	#ifdef __NOPT_ASSERT__
	// Used to determine when to step-over a script command when the script
	// is being stepped through in the debugger.
	int m_last_num_return_addresses_when_halted;
	#endif
	
    // Used by the Wait function to cause a script to hiccup.
    int m_wait_timer;
	
	// Used by the WaitTime function to cause the script to wait for an
	// absolute amount of time.
	Tmr::Time m_start_time;
	Tmr::Time m_wait_period;

	// How the script is waiting.
	EWaitType m_wait_type;
	Obj::CBaseComponent *mp_wait_component;

	

	// a helpful member added by Ryan
	uint32	m_unique_id;
	// used for computing unique IDs
	static int s_next_unique_id;
	
	// Gets set to true if the script was forced to call another script by a call to Interrupt.
	// This will get stored in the call stack.
	// Needed to prevent the call to Update() that Interrupt does from continuing the execution of
	// the interrupted script when the called script finishes.
	bool m_interrupted;

	void call_script(uint32 newScriptChecksum, uint8 *p_script, CStruct *p_params, Obj::CObject *p_object, bool interrupt=false);
	void set_script(uint32 scriptChecksum, uint8 *p_script, CStruct *p_params, Obj::CObject *p_object);
	
	// This flag is to prevent multiple calls to process_waits in nested calls to CScript::Update(),
	// such as when a script command causes an interrupt to occur on itself.
	bool m_skip_further_process_waits;	
	
	void process_waits();
	
	void load_function_params();
	bool run_member_function(uint32 functionName, Obj::CObject *p_substituteObject=NULL);
	bool run_cfunction(bool (*p_cfunc)(CStruct *pParams, CScript *pCScript));
	uint32 get_name();
	
	bool execute_command();
	
	
	#ifdef __NOPT_ASSERT__
	bool 			m_if_debug_on;
	uint32 			m_if_status; 			// For detecting spurious else's
	#endif


	#ifdef __NOPT_ASSERT__
	// Used by script debugger code.
	uint32 m_last_transmitted_info_checksum;
	bool m_being_watched_in_debugger;
	ESingleStepMode m_single_step_mode;
	enum
	{
		MAX_COMMENT_STRING_CHARS=63,
	};
	char mp_comment_string[MAX_COMMENT_STRING_CHARS+1];
	int m_originating_script_line_number;
	uint32 m_originating_script_name;
	float m_last_instruction_time_taken;
	float m_total_instruction_time_taken;
	
	#ifdef __PLAT_WN32__
	uint32 *write_callstack_entry(uint32 *p_buf, int bufferSize, uint32 scriptNameChecksum, uint8 *p_PC, CStruct *p_params, Obj::CObject *p_ob) {}
	void check_if_needs_to_be_watched_in_debugger() {}
	void advance_pc_to_next_line_and_halt() {}
	#else
	uint32 *write_callstack_entry(uint32 *p_buf, int bufferSize, uint32 scriptNameChecksum, uint8 *p_PC, CStruct *p_params, Obj::CObject *p_ob);
	void check_if_needs_to_be_watched_in_debugger();
	void advance_pc_to_next_line_and_halt();
	#endif
	#endif
	
	void execute_if();
	void execute_else();
	void execute_endif();
	
	
	void skip_to_after_endswitch();
	void execute_switch();
	
	void execute_begin();
	void execute_repeat();
	void execute_break();
	
	bool execute_return();

public:

	#ifdef __NOPT_ASSERT__
	#ifdef __PLAT_WN32__
	void SetCommentString(const char *p_comment) {}
	void SetOriginatingScriptInfo(int lineNumber, uint32 scriptName) {}
	void TransmitInfoToDebugger(bool definitely_transmit=false) {}
	void TransmitBasicInfoToDebugger() {}
	void WatchInDebugger(bool stopScriptImmediately) {}
	bool BeingWatchedInDebugger() {return false;}
	void StopWatchingInDebugger() {}
	void DebugStop() {}
	void DebugStepInto() {}
	void DebugStepOver() {}
	void DebugGo() {}
	#else	
	void SetCommentString(const char *p_comment);
	void SetOriginatingScriptInfo(int lineNumber, uint32 scriptName);
	// This gets called for all CScript's every frame.
	// It transmits lots of info about the script to the script debugger running on the PC
	void TransmitInfoToDebugger(bool definitely_transmit=false);
	void TransmitBasicInfoToDebugger();
	void WatchInDebugger(bool stopScriptImmediately);
	bool BeingWatchedInDebugger() {return m_being_watched_in_debugger;}
	void StopWatchingInDebugger();
	void DebugStop();
	void DebugStepInto();
	void DebugStepOver();
	void DebugGo();
	
	int GetNumReturnAddresses() {return m_num_return_addresses;}
	bool UsingBigCallstackBuffer();
	bool UsingBigLoopBuffer();
	#endif
	#endif

#ifdef	__SCRIPT_EVENT_TABLE__		
	// Event Handler/Exception Stuff
	void				SetEventHandler(uint32 ex, uint32 scr, uint32 group, bool exception, CStruct* p_params);
	void  				SetEventHandlers(Script::CArray *pArray, EReplaceEventHandlers replace = DONT_REPLACE_HANDLERS);
	void				RemoveEventHandler(uint32 type);
	void				RemoveEventHandlerGroup(uint32 group);
	void 				SetOnExceptionScriptChecksum(uint32 OnExceptionScriptChecksum);
	uint32 				GetOnExceptionScriptChecksum() const; 
	void 				SetOnExitScriptChecksum(uint32 OnExceptionScriptChecksum);
	uint32 				GetOnExitScriptChecksum() const; 
	bool				PassTargetedEvent(Obj::CEvent *pEvent, bool broadcast = false);	// return true if object still valid
#endif

	void				PrintEventHandlerTable (   );
	
	CStruct *GetParams() {Dbg_MsgAssert(mp_params,("NULL mp_params ?")); return mp_params;}

	int	mNode;		// Number of the node that caused this script to be spawned, -1 if none specific

    // If NULL then the script is a free script, eg StartUp.
    // If non-NULL, then the script is associated with some object.
    Obj::CObjectPtr mpObject;

    // Checksum of the name of this script.
    uint32 mScriptChecksum;

	void SetWait(EWaitType type, Obj::CBaseComponent *p_component);
	void ClearWait();
	EWaitType GetWaitType() {return m_wait_type;}


	bool mIsSpawned:1;	// True if this is a spawned script.
	// Note: These next members only apply if the script is spawned.
	bool mNotSessionSpecific:1; // If this is true then the spawned script will not get
								// deleted by DeleteSpawnedScripts
	bool mPaused:1;
	bool mPauseWithObject:1;	// If this is true then the spawned script will pause when its object's ShouldUpdatePauseWithObjectScripts
								// returns false.  CCompositeObjects return	false when they are paused.
	uint32 mId;
	// An optional callback script, which gets run as soon as the spawned script completes.
	uint32 mCallbackScript;
	CStruct *mpCallbackScriptParams;

	CScript();
    ~CScript();

    void SetScript(uint32 scriptChecksum, CStruct *p_params=NULL, Obj::CObject *p_object=NULL);
    void SetScript(const char *p_scriptName, CStruct *p_params=NULL, Obj::CObject *p_object=NULL);
    void SetScript(const SStructScript *p_structScript, CStruct *p_params=NULL, Obj::CObject *p_object=NULL);
	
	CStruct *MakeParamsSafeFromDeletionByClearScript(CStruct *p_params);
	void ClearScript();
	void ClearEventHandlerTable();
	bool RefersToScript(uint32 checksum);
	bool RefersToObject(Obj::CObject *p_object);
	void RemoveReferencesToObject(Obj::CObject *p_object);
	
	bool AllScriptsInCallstackStillExist();
	uint32 GetBaseScript();
	void Restart();
	EScriptReturnVal Interrupt(uint32 newScriptChecksum, CStruct *p_params=NULL);
	
    EScriptReturnVal Update();
    bool Finished();

	void Block();
	void UnBlock();
	bool getBlocked();

	void Clear();
    void Wait(int numFrames);
    void WaitOnePerFrame(int numFrames);
	void WaitTime(Tmr::Time period);
	bool GotScript();

	uint32 GetUniqueId() {return m_unique_id;}
	
	// Stops the script from being associated with an object, ie, sets mpObject to NULL.
	// Allows the script to continue executing though, so the script must not contain any
	// further member functions, otherwise the code will assert.
	// This function also sets mWaitType=WAIT_TYPE_NONE; to ensure the Update function does
	// not assert if it is waiting for the object to finish doing something.
	void DisassociateWithObject(Obj::CObject *pObjectToDisconnectFrom);
	
	#ifdef __NOPT_ASSERT__
	const char *GetScriptInfo();
	int GetCurrentLineNumber(void);
	const char *GetSourceFile(void);
	
	// and let's have it for debugging for now, in case people (Mick!) leave it in by mistake and screw up the build
	const inline char *GetScriptInfo( void ) const { return ""; }
	#else // #ifdef __NOPT_ASSERT__
#	ifdef __PLAT_XBOX__
	const inline char *GetScriptInfo( void ) const { return ""; }
#	endif // ifdef __PLAT_XBOX__

	#ifdef __PLAT_WN32__
	const char *GetScriptInfo() const {return "";}
	#endif
	
	#endif // #ifdef __NOPT_ASSERT__
	
	void SwitchOnIfDebugging();
	void SwitchOffIfDebugging();

// These are pseudo-private, for debugging profiling only
	#ifdef __NOPT_ASSERT__
	int				m_last_time;			// time spend in last update	
	int 			m_total_time;			// total time spent updating
	#endif
	
#ifdef	__SCRIPT_EVENT_TABLE__		
private:	
	Obj::CEventHandlerTable *	mp_event_handler_table;
	
	// Script to run when there is an exception	
	// (needed here, since we are getting rid of CExceptionComponent, to replace it with one 
	// single event system
	uint32 			mOnExceptionScriptChecksum;
	uint32 			mOnExitScriptChecksum;
public:
	Obj::CEventHandlerTable * GetEventHandlerTable() {return mp_event_handler_table;}
	Obj::CRefCounted			m_reference_counter;		// having a reference counter allows us to have a safe smart pointer	
#endif	 



};

void SendScript( uint32 scriptChecksum, CStruct *p_params, Obj::CObject *p_object );

void RunScript(const char *p_scriptName, CStruct *p_params=NULL, Obj::CObject *p_object=NULL,
				bool netScript = false );
void RunScript(uint32 scriptChecksum, CStruct *p_params=NULL, Obj::CObject *p_object=NULL,
				bool netScript = false, const char *p_scriptName=NULL );

void DeleteSpawnedScripts();
void UpdateSpawnedScripts();
void PauseSpawnedScripts(bool status);
void UnpauseSpawnedScript(CScript* p_script);
uint32 NumSpawnedScriptsRunning();
bool	ScriptExists(uint32 scriptNameChecksum);

CScript* SpawnScript(uint32 scriptChecksum, CStruct *p_script_params=NULL, 
					 uint32 callbackScript=NO_NAME, CStruct *p_callbackParams=NULL,
					 int node = -1, uint32 id=0,
					 bool netEnabled = false, bool permanent = false, bool not_session_specific=false, bool pause_with_object=false );
CScript* SpawnScript(const char *pScriptName, CStruct *p_script_params=NULL,
					 uint32 callbackScript=NO_NAME, CStruct *p_callbackParams=NULL,
					 int node = -1, uint32 id=0,
					 bool netEnabled = false, bool permanent = false, bool not_session_specific=false, bool pause_with_object=false );
void KillSpawnedScript(CScript *p_script);
void KillSpawnedScriptsThatReferTo(uint32 checksum);
void KillSpawnedScriptsWithId(uint32 id);
void KillSpawnedScriptsWithObject(Obj::CObject *p_object);
void KillSpawnedScriptsWithObjectAndId(Obj::CObject *p_object, uint32 id, CScript *p_callingScript);
void KillSpawnedScriptsWithObjectAndName(Obj::CObject *p_object, uint32 name, CScript *p_callingScript);
uint32 FindSpawnedScriptID(CScript *p_script);
CScript* FindSpawnedScriptWithID(uint32 id);
void SendSpawnScript( uint32 scriptChecksum, Obj::CObject *p_object, int node, bool permanent );


CScript *GetScriptWithUniqueId(uint32 id);

// These should not be here, but should be in the game-specific scripting directory.
// Just putting them here for the moment to get things to compile ...
class CArray;

// TODO: This just included to allow compilation of GenerateCRC, remove later.
uint32 GenerateCRC(const char *p_string);

void StopAllScripts();
void StopAllScriptsUsingThisObject(Obj::CObject *p_object);
void StopScriptsUsingThisObject(Obj::CObject *p_object, uint32 scriptCrc);
void StopScriptsUsingThisObject_Proper(Obj::CObject *p_object, uint32 scriptCrc);
void KillStoppedScripts();
void DumpScripts();

uint32 GetNumCScripts();

CScript *GetCurrentScript();

#ifdef	__NOPT_ASSERT__
const char * GetCurrentScriptInfo();
#endif

} // namespace Script

#endif	// __SCRIPTING_SCRIPT_H

