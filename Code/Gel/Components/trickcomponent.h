//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       TrickComponent.h
//* OWNER:          ???
//* CREATION DATE:  ??/??/??
//****************************************************************************

#ifndef __COMPONENTS_TRICKCOMPONENT_H__
#define __COMPONENTS_TRICKCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <sk/objects/skater.h>
#include <sk/objects/trickobject.h>

#ifndef __OBJECT_BASECOMPONENT_H__
#include <gel/object/basecomponent.h>
#endif

#ifndef __GEL_INPMAN_H
#include <gel/inpman.h>
#endif

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_TRICK CRCD(0x270f56e1,"Trick")
#define		GetTrickComponent() ((Obj::CTrickComponent*)GetComponent(CRC_TRICK))
#define		GetTrickComponentFromObject(pObj) ((Obj::CTrickComponent*)(pObj)->GetComponent(CRC_TRICK))

namespace Script
{
    class CScript;
    class CStruct;
    class CComponent;
}
              
namespace Mdl
{
	class Score;
};
	
namespace Obj
{

class CSkaterProfile;
class CSkaterBalanceTrickComponent;
class CSkaterFlipAndRotateComponent;
class CSkaterStateComponent;
class CSkaterRunTimerComponent;

// An element of the trick queue.
struct STrick
{
	uint32 ArrayChecksum;	// Checksum of the script array, eg AirTricks
	uint Index;				// Index within the array of the trick.
	
	// These next two are only used by manual tricks and special grind tricks.
	// These have a finite duration. For example, if a manual is queued up early on
	// in a jump, it may expire before the player lands.
	uint32 Time;			// The time when the trick was triggered.
	uint32 Duration;		// How long after Time that the trick will last for.
	bool UseSpecialTrickText; // Whether to use the yellow text when displaying the trick.
};

enum EEventType
{
	// All the events in the buffer have this type initially.
	EVENT_NONE,
	
	EVENT_BUTTON_PRESSED,
	EVENT_BUTTON_RELEASED,
	
	EVENT_BUTTON_PRESSURE_CHANGE,
	
	EVENT_ON_RAIL,
	EVENT_OFF_RAIL,
};

#define MAX_EVENTS 100
struct SButtonEvent
{
	uint32 Time;
	EEventType EventType;
	uint32 ButtonNameChecksum;
	int ButtonPressure;
	bool MaybeUsed;
	uint32 Used;
};

#define USED_BY_REGULAR_TRICK (1<<0)
#define USED_BY_EXTRA_TRICK (1<<1)
#define USED_BY_MANUAL_TRICK (1<<2)
#define USED_BY_EXTRA_GRIND_TRICK (1<<3)

#define USE_SPECIAL_TRICK_TEXT true

class CTrickComponent : public CBaseComponent
{
	// Used to detect infinite recursion of RunTrick
	uint32 m_num_runtrick_recursions;
	
	Mdl::Score *mp_score;
	
	CInputComponent* 				mp_input_component;
	CSkaterBalanceTrickComponent*	mp_skater_balance_trick_component;
	CSkaterCorePhysicsComponent*	mp_skater_core_physics_component;
	CSkaterFlipAndRotateComponent*	mp_skater_flip_and_rotate_component;
	CSkaterStateComponent*			mp_skater_state_component;
    CStatsManagerComponent*			mp_stats_manager_component;
	
	Script::CStruct*				get_trigger_structure(Script::CStruct *p_struct);
	Script::CStruct*				get_alternate_trigger_structure(Script::CStruct *p_struct);
	
public:
	CSkater*						GetSkater (   ) { return static_cast< CSkater* >(GetObject()); }
	
	void UpdateTrickMappings( Obj::CSkaterProfile* pSkaterProfile );

	Mdl::Score *GetScoreObject();
	void SetAssociatedScore(Mdl::Score *p_score) {mp_score=p_score;}

	// Gets called from the CTrickComponent constructor, and also from CSkater::ResetEverything()
	void Clear();

	void RecordButtons();
	void Debounce ( int button, float time );

	// The Button Event buffer.
	SButtonEvent mpButtonEvents[MAX_EVENTS];
	// How many events are in the buffer. When the skater is first created this will be zero,
	// but will increase until it equals MAX_EVENTS then will stay equal to MAX_EVENTS from then on.
	uint16 mNumEvents;
	// Index of the last event in the buffer.
	uint16 mLastEvent;

	// If a trick gets triggered, the buttons which triggered it will be stored here.
	// Eg, for a kickflip, button 0 will be Square (checksum of), and button 1 will be Left.
	// They are stored so that the extra-trick trigger can use one of the parent trick's buttons,
	// rather than being hard wired. This is needed in case the player re-maps the parent trick to
	// a different button combination.
	#define MAX_TRICK_BUTTONS 3
	uint32 mp_trick_button[MAX_TRICK_BUTTONS];
	// Reads the button names and time duration out of a 'Trigger' structure.
	uint32 get_buttons_and_duration(Script::CComponent *p_comp, int num_buttons, uint32 *p_button_names);
	void record_last_tricks_buttons(uint32 *p_button_names);

	float mButtonDebounceTime[PAD_NUMBUTTONS];
	bool mButtonState[PAD_NUMBUTTONS];
	
	bool GetButtonState(uint32 Checksum);

	int mNumButtonsToIgnore;
	#define MAX_BUTTONS_TO_IGNORE 2
	uint32 mpButtonsToIgnore[MAX_BUTTONS_TO_IGNORE];
	void ButtonRecord(uint Button, bool Pressed);
	
	bool TriggeredInLastNMilliseconds(uint32 ButtonNameChecksum, uint32 Duration, uint32 IgnoreMask=0xffffffff);
	bool BothTriggeredNothingInBetween(uint32 Button1, uint32 Button2, uint32 Duration, uint32 IgnoreMask);
	bool QueryEvents(Script::CStruct *pQuery, uint32 UsedMask=USED_BY_REGULAR_TRICK, uint32 IgnoreMask=0xffffffff);
	void ResetMaybeUsedFlags();
	void ConvertMaybeUsedToUsed(uint32 UsedMask);
	void RemoveOldButtonEvents(uint32 Button, uint32 OlderThan);
	SButtonEvent *AddEvent(EEventType EventType);
	void ClearEventBuffer();
	void DumpEventBuffer();
	
	// If set, then the next call to Display will use the yellow text.
	bool mUseSpecialTrickText;

	// This maps trick slots to trick names. Each pro has their own set of default
	// mappings, stored in the profile.
	// The default mappings are defined in script, eg the structure HawkTricks.
	Script::CStruct *mpTrickMappings;

	//////////////////////////////////////////////////////////
	// 					Trick queue stuff
	//////////////////////////////////////////////////////////
	
	int mFrontTrick;	// The trick that is at the front of the queue, ie, next to be executed.
	int mBackTrick;		// The trick at the back of the queue, ie the last to be added.
	// mFrontTrick and mBackTrick are both -1 to indicate no tricks in queue.
	#define TRICK_QUEUE_SIZE 100
	STrick mpTricks[TRICK_QUEUE_SIZE];
	// Called by the ClearTrickQueue script command.
	void ClearTrickQueue();
	
	void AddTrick(uint32 ArrayChecksum, uint Index, bool UseSpecialTrickText=false);
	void RemoveFrontTrick();
	void ClearTricksFrom(uint32 ArrayChecksum);

	bool TrickIsDefined(Script::CStruct *pTrick);
	bool RunTrick(Script::CStruct *pTrick, uint32 optionalFlag=0, Script::CStruct *pExtraParams=NULL);

	// Called by the DoNextTrick script command.
	void TriggerNextQueuedTrick(uint32 scriptToRunFirst=0, Script::CStruct *p_scriptToRunFirstParams=NULL, Script::CStruct* pExtraParams=NULL);
	
	int mNumQueueTricksArrays;
	// Storing array checksums rather than pointers to the arrays, in case they get reloaded.
	#define MAX_QUEUE_TRICK_ARRAYS 10
	uint32 mpQueueTricksArrays[MAX_QUEUE_TRICK_ARRAYS];
	void ClearQueueTricksArrays();
	
	// Checksum of any special tricks array specified by the last SetQueueTricks script command.
	uint32 mSpecialTricksArrayChecksum;
	
	// This can be set by the SetQueueTricks command, and allows events that have already been
	// used by certain tricks, eg extra grinds, to be not ignored.
	uint32 mDoNotIgnoreMask;
	
	void MaybeAddTrick(uint32 ArrayChecksum, bool UseSpecialTrickText=false, uint32 DoNotIgnoreMask=0);
	
	// Always called every frame.
	void AddTricksToQueue();

	//////////////////////////////////////////////////////////
	// 					Manual stuff
	//////////////////////////////////////////////////////////
	
	int mNumManualTrickArrays;
	// Storing array checksums rather than pointers to the arrays, in case they get reloaded.
	#define MAX_MANUAL_TRICK_ARRAYS 10
	uint32 mpManualTrickArrays[MAX_MANUAL_TRICK_ARRAYS];
	
	// Checksum of any special manual tricks specified by the SetManualTricks script command.
	uint32 mSpecialManualTricksArrayChecksum;
	
	bool mGotManualTrick;
	// A special one entry 'queue' for manual tricks.
	STrick mManualTrick;
	// Called by the DoNextManualTrick script command.
	void TriggerAnyManualTrick(Script::CStruct *pExtraParams=NULL);
	
	// Checks a single array to see if any trick needs to be put in the manual queue.
	// Called from MaybeQueueManualTrick()
	void CheckManualTrickArray(uint32 ArrayChecksum, uint32 IgnoreMask=0xffffffff, bool UseSpecialTrickText=false);
	
	// These two functions are always called every frame.
	void MaybeQueueManualTrick();
	void MaybeExpireManualTrick();
	// Called by the ClearManualTrick script command.
	void ClearManualTrick();

	//////////////////////////////////////////////////////////
	// 					Extra-grind stuff
	//////////////////////////////////////////////////////////
	
	int mNumExtraGrindTrickArrays;
	// Storing array checksums rather than pointers to the arrays, in case they get reloaded.
	#define MAX_EXTRA_GRIND_TRICK_ARRAYS 10
	uint32 mpExtraGrindTrickArrays[MAX_EXTRA_GRIND_TRICK_ARRAYS];
	
	uint32 mSpecialExtraGrindTrickArrayChecksum;
	
	bool mGotExtraGrindTrick;
	// A special one entry 'queue' for extra grind tricks.
	STrick mExtraGrindTrick;
	
	// Called from CSkater::MaybeStickToRail()
	bool TriggerAnyExtraGrindTrick(bool Right, bool Parallel, bool Backwards, bool Regular);
	
	// For checking an single array. Called from MaybeQueueExtraGrindTrick()
	void MaybeQueueExtraGrindTrick(uint32 ArrayChecksum, bool UseSpecialTrickText=false);
	
	// These two functions are always called every frame.
	void MaybeQueueExtraGrindTrick();
	void MaybeExpireExtraGrindTrick();
	// Called by the ClearExtraGrindTrick script command.
	void ClearExtraGrindTrick();
	
	////////////////////////////////////////////////////////////////
	//
	// 'Extra' trick stuff.
	//
	////////////////////////////////////////////////////////////////
	
	// Whether we've got extra tricks to check for.
	bool mGotExtraTricks;
	// Whether to check for extra tricks forever.
	bool mExtraTricksInfiniteDuration;
	// If not infinite duration, this is how long after the start time to check for extra tricks for.
	uint32 mExtraTricksDuration;
	// The start time.
	uint32 mExtraTricksStartTime;
	
	// How many array checksums are in the array.
	int mNumExtraTrickArrays;
	#define MAX_EXTRA_TRICK_ARRAYS 10
	uint32 mpExtraTrickArrays[MAX_EXTRA_TRICK_ARRAYS];
	// Bitfields indicating which tricks in each array are excluded. (Hence each array can't have more than 32 tricks in it)
	// Used when an extra trick can branch to another trick in the same array, but we
	// don't want the same trick again, eg rail balance to another rail balance.
	uint32 mpExcludedExtraTricks[MAX_EXTRA_TRICK_ARRAYS];

	// Array checksum & exclusion bitfield for the special extra tricks, which are only checked for when special.
	uint32 mSpecialExtraTricksArrayChecksum;
	uint32 mExcludedSpecialExtraTricks;
	
	bool TriggerAnyExtraTrick(uint32 ArrayChecksum, uint32 ExcludedTricks);
	void TriggerAnyExtraTricks();
	void SetExtraTricks(Script::CStruct *pParams, Script::CScript *pScript);
	void KillExtraTricks();
	uint32 CalculateIgnoreMask(uint32 ArrayChecksum, const char *pIgnoreName);
	void ExcludeExtraTricks(const char *pIgnoreName);
	bool IsExcluded(Script::CStruct *pTrick, const char *pIgnoreName);

	bool mBashingEnabled;
	
	void HandleBashing();
	float GetBashFactor();

	// Name of the current trick, which gets displayed when the trick is triggered.
	char mpTrickName[MAX_TRICK_NAME_CHARS+1];
	int mTrickScore;

	// Name of the deferred trick, which gets displayed only if a 'display blockspin' or ClearPanel_Landed is done.
	char mpDeferredTrickName[MAX_TRICK_NAME_CHARS+1];
	int mDeferredTrickScore;
	Mdl::Score::Flags mDeferredTrickFlags;

	float mTallyAngles;
	
	// for knowing when to stop horse mode
	bool m_first_trick_started;
	bool m_first_trick_completed;
	
	int mNumTricksInCombo;
	void IncrementNumTricksInCombo();
    void ClearMiscellaneous();
	
	void SetGraffitiTrickStarted( bool started );
	bool GraffitiTrickStarted() { return m_graffiti_trick_started; }
	
	// for knowing when to accumulate graffiti objects
	bool m_graffiti_trick_started;
	
	void TrickOffObject( uint32 node_name );
		
	void SetFirstTrickStarted( bool started ) { m_first_trick_started = started; }
	bool FirstTrickStarted() { return m_first_trick_started; }
	bool FirstTrickCompleted() { return m_first_trick_completed; }
	
	uint32 WritePendingTricks( uint32* p_buffer, uint32 max_size );
	
	Obj::CPendingTricks m_pending_tricks;

    CTrickComponent();
    virtual ~CTrickComponent();

public:
    virtual void            		Update();
    virtual void            		Finalize();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
};

inline Mdl::Score* CTrickComponent::GetScoreObject()
{
	Dbg_MsgAssert(mp_score,("NULL pScore"));
	return mp_score;
}	

bool ScriptDoNextTrick( Script::CStruct *pParams, Script::CScript *pScript );


}

#endif
