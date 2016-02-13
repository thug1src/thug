//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       GoalEditorComponent.h
//* OWNER:          Kendall Harrison
//* CREATION DATE:  3/21/2003
//****************************************************************************

#ifndef __COMPONENTS_GOALEDITORCOMPONENT_H__
#define __COMPONENTS_GOALEDITORCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

// Replace this with the CRCD of the component you are adding
#define		CRC_GOALEDITOR CRCD(0x81f01058,"GoalEditor")

//  Standard accessor macros for getting the component either from within an object, or 
//  given an object				 
#define		GetGoalEditorComponent() ((Obj::CGoalEditorComponent*)GetComponent(CRC_GOALEDITOR))
#define		GetGoalEditorComponentFromObject(pObj) ((Obj::CGoalEditorComponent*)(pObj)->GetComponent(CRC_GOALEDITOR))

namespace Script
{
    class CScript;
    class CStruct;
}

namespace Nx
{
	class CModel;
}

namespace Obj
{
enum
{
	// The number of nodes used by a goal
	MAX_POSITIONS_PER_GOAL=8,
	// Limited to 20 because a ped will need to be created for each goal
	// Note: If this value is changed, it would be a good idea to update VERSION_CREATEDGOALS
	// in memcard.q too, to prevent loading old mem card saves that may have more goals than
	// the new value allows.
	MAX_GOALS_PER_LEVEL=10,
	
	// Note: MAX_POSITIONS_PER_GOAL*MAX_GOALS_PER_LEVEL nodes will get added to the NodeArray
	
	// The total number of goals stored in the GoalEditor composite object.
	MAX_GOALS_TOTAL=50,
};

class CInputComponent;
class CEditorCameraComponent;

class CGoalPos
{
public:
	enum EType
	{
		NONE,
		PED,
		RESTART,
		LETTER,
	};
	
	EType mType;
	Mth::Vector mPos;
	float mHeight;
	float mAngle;
};	

struct SPreDefinedKeyComboSet
{
	uint8 mSetIndex; // Index into the cag_skatetris_key_combos array defined in goal_editor.q
	
	// Note: These three are not really used at the moment. They used to be settable from menus,
	// but are now all left as zero for simplicity, with only a global spin value settable for the
	// whole goal.
	// So if it stays this way, there is no need for this structure, could just have a simple array
	// of set indices instead.
	uint8 mRequirePerfect;
	uint8 mSpin; // Actually the spin/180
	uint8 mNumTaps;
};
	
class CEditGoal
{
	// Identifies which level this goal is for via the level load script name.
	uint32 m_level_script;
	
	enum
	{
		// These needs to be at least 1 bigger than the max_length value passed to
		// the create_onscreen_keyboard script.
		GOAL_NAME_BUFFER_SIZE=50,
		PED_NAME_BUFFER_SIZE=21,
		GOAL_DESCRIPTION_BUFFER_SIZE=100,
		GOAL_WIN_MESSAGE_BUFFER_SIZE=100,
	};
	char mp_goal_name[GOAL_NAME_BUFFER_SIZE];
	char mp_ped_name[PED_NAME_BUFFER_SIZE];
	char mp_goal_description[GOAL_DESCRIPTION_BUFFER_SIZE];
	char mp_win_message[GOAL_WIN_MESSAGE_BUFFER_SIZE];


	uint32 m_goal_type;
	
	CGoalPos mp_item_positions[MAX_POSITIONS_PER_GOAL];
	uint8 m_num_positions_required;
	uint8 m_num_positions_set;
	uint8 m_current_position_index;
	uint8 m_placed_last_position;
	
	uint8 m_has_won_goal;
	
	uint8 m_used; // Indicates whether this slot in the array of CEditGoals is used or not.
	
	// m_level_goal_index has a value from 0 to MAX_GOALS_PER_LEVEL-1
	// From the level goal index is also derived the goal's id string
	// which the goal manager needs when creating the goal ped arrows
	// and the goal id, which is just the checksum of the id string. 
	int m_level_goal_index;
	
	int m_time_limit;
	
	// One of skate, walk, junkercar or rallycar
	uint32 m_control_type;
	
	// The following are goal specific params.
	// Could have stored these in a CStruct instead, but that would have caused varying
	// memory requirements depending on how many goals the user had added, making it harder to test.
	
	// The score value for high-score, high-combo etc.
	int m_score;

	// Skatetris specific
	int m_acceleration_interval;
	float m_acceleration_percent;
	int m_trick_time;
	int m_time_to_stop_adding_tricks;
	int m_max_tricks;
	
	int m_spin;

	// Combo skatetris specific
	bool m_single_combo;
	int m_combo_size;

	// Tricktris specific
	int m_tricktris_block_size;
	int m_tricktris_total_to_win;
	
	enum
	{
		MAX_COMBO_SETS=10
	};	
	SPreDefinedKeyComboSet mp_combo_sets[MAX_COMBO_SETS];
	int m_num_combo_sets;
	// Used for writing/reading the above array to mem card.
	void write_combo_sets(Script::CStruct *p_info);
	void read_combo_sets(Script::CStruct *p_info);
	
	// Generates the goal_tetris_key_combos array and inserts it into p_info, for sending to
	// the goal manager.
	void generate_key_combos_array(Script::CStruct *p_info);
	
	
	enum
	{
		MAX_GAPS=10,
		MAX_GAP_TRICK_NAME_CHARS=50,
	};
	uint8 mp_gap_numbers[MAX_GAPS];
	uint8 m_num_gaps;
	char mp_trick_name[MAX_GAP_TRICK_NAME_CHARS+1];
	
	
	void write_position_into_node(uint index, uint32 nodeName);
	void add_goal_params(Script::CStruct *p_params);
	uint32 generate_marker_object_name(int index);
	uint32 generate_goal_object_name(int index);
	void create_marker(int index);
	void delete_marker(int index);
	bool position_too_close_to_existing_positions(const Mth::Vector& pos);
	bool position_ok_to_place(const Mth::Vector& pos, float height);

	void refresh_position_using_collision_check(uint positionIndex);

public:
	enum EWriteDest
	{
		NOT_WRITING_TO_GOAL_MANAGER=0,
		WRITING_TO_GOAL_MANAGER=1,
	};	
private:
	
	// The dest parameter determines the format in which the combo sets are written out.
	// When generating params for the goal manager, the full goal_tetris_key_combos array is
	// generated. When writing to mem card the chosen pre-defined set numbers are written instead,
	// so that after loading they can still be edited.
	void write_skatetris_params(Script::CStruct *p_info, EWriteDest dest);
	void read_skatetris_params(Script::CStruct *p_info);
	
public:
	CEditGoal();
	~CEditGoal();
	
	void Clear();

	#ifdef SIZE_TEST	
	// For testing, this fills the name buffers, eg mp_goal_name, mp_trick_name etc, to their
	// max size.
	void FillNameBuffers();
	uint32 GetSkateGoalMaxSize();
	#endif
	
	bool Used() {return m_used;}
	void SetUsedFlag(bool used) {m_used=used;}
	
	void SetLevelGoalIndex(uint index) {m_level_goal_index=index;}
	int GetLevelGoalIndex() {return m_level_goal_index;}
	
	uint32 GetId();
	const char *GetIDString();
	
	bool SetPosition(const Mth::Vector& pos, float height, float angle);
	bool GetPosition(Mth::Vector *p_pos, float *p_height, float *p_angle);
	CGoalPos::EType GetPositionType();
	void GetPosition(Mth::Vector *p_pos, int index);
	int GetNumPositionsSet() {return m_num_positions_set;}
	
	bool BackUp(const Mth::Vector& pos, float height, float angle);
	bool GotAllPositions();
	uint GetCurrentLetterIndex();
	bool PlacedLastPosition() {return m_placed_last_position;}
	
	void SetGoalDescription(const char *p_text);
	void SetWinMessage(const char *p_text);
	void SetGoalName(const char *p_name);
	void SetPedName(const char *p_name);
	bool PositionClashesWithExistingPositions(Mth::Vector& pos);
	
	int  GetScore() {return m_score;}
	void SetScore(int score) {m_score=score;}
	void SetTimeLimit(int timeLimit);
	void SetControlType(uint32 controlType);
	
	void EditGoal();

	void WriteIntoStructure(Script::CStruct *p_info);
	void ReadFromStructure(Script::CStruct *p_info);

	void WriteGoalSpecificParams(Script::CStruct *p_info, EWriteDest dest=NOT_WRITING_TO_GOAL_MANAGER);
	void ReadGoalSpecificParams(Script::CStruct *p_params);
	void RemoveGoalSpecificFlag(Script::CStruct *p_params);
	
	uint32 GetType() {return m_goal_type;}
	void SetType(uint32 type);
	void SetLevel(uint32 levelScript) {m_level_script=levelScript;}
	uint32 GetLevel() {return m_level_script;}
	const char *GetGoalName() {return mp_goal_name;}
	const char *GetPedName() {return mp_ped_name;}
	void FlagAsWon() {m_has_won_goal=true;}
	
	void AddKeyComboSet(int setIndex, bool requirePerfect, int spin, int numTaps);
	void RemoveKeyComboSet(int setIndex);
	SPreDefinedKeyComboSet *GetKeyComboSet(int setIndex);
	
	void AddGap(uint8 gap_number);
	void RemoveGap(uint8 gap_number);
	void RemoveGapAndReorder(uint8 gap_number);
	bool GotGap(uint8 gap_number);
	
	void AddGoalToGoalManager(bool markUnbeaten=false);
	void WriteNodePositions();
	
	bool GoalIsOutsideArea(float x0, float z0, float x1, float z1);
	void RefreshPositionsUsingCollisionCheck();
	
	void GetDebugInfo(Script::CStruct* p_info);
};

class CGoalEditorComponent : public CBaseComponent
{
	CEditGoal 	mp_goals[MAX_GOALS_TOTAL];
	CEditGoal 	*mp_current_goal;

	CInputComponent 		*mp_input_component;
	CEditorCameraComponent 	*mp_editor_camera_component;
	void get_pointers_to_required_components();
	Obj::CCompositeObject *get_cursor();
	void refresh_cursor_object();

	void get_pos_from_camera_component(Mth::Vector *p_pos, float *p_height, float *p_angle);
	void update_cursor_collision_type();
	
	CEditGoal *find_goal(const char *p_name);
	CEditGoal *find_goal(uint32 id);
	CEditGoal *find_goal(Script::CStruct *p_params);
	
	void update_camera_pos();
	void update_cursor_position();
	void remove_goal(CEditGoal *p_goal);
	bool too_close_to_another_goal_position(const Mth::Vector &pos, float height, uint32 level, int positionIndex);
	
public:
    CGoalEditorComponent();
    virtual ~CGoalEditorComponent();

public:
	void							ClearAllExceptParkGoals();
	void							ClearOnlyParkGoals();
	
	void							CleanOutUnfinishedGoals();
	void							NewGoal();
	void							SetGoalType(uint32 type);
	void							AddGoalsToGoalManager();
	void							WriteNodePositions();
	int								GetNumGoals();
	bool							ThereAreGoalsOutsideArea(float x0, float z0, float x1, float z1);
	void							DeleteGoalsOutsideArea(float x0, float z0, float x1, float z1);
	int 							CountGoalsForLevel(uint32 level);
	
	void							RefreshGoalPositionsUsingCollisionCheck();
	void							RefreshGapGoalsAfterGapRemovedFromPark(int gapNumber);
	
	
	// For reading/writing to memcard
	void							WriteIntoStructure(Script::CStruct *p_info);
	enum EBoolLoadingParkGoals
	{
		NOT_LOADING_PARK_GOALS=0,
		LOADING_PARK_GOALS=1,
	};	
	void							ReadFromStructure(Script::CStruct *p_info, EBoolLoadingParkGoals loadingParkGoals=NOT_LOADING_PARK_GOALS);
	
	void							WriteIntoStructure(uint32 levelScript, Script::CStruct *p_info);
	uint32							WriteToBuffer(uint32 levelScript, uint8 *p_buffer, uint32 bufferSize);
	uint8 *							ReadFromBuffer(uint32 levelScript, uint8 *p_buffer);
	

	virtual	void 					Finalize();
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );
	virtual void					Hide( bool shouldHide );

	static CBaseComponent*			s_create();
};

//char p[sizeof(CGoalEditorComponent)/0];

void InsertGoalEditorNodes();
Obj::CGoalEditorComponent *GetGoalEditor();

} // namespace Obj

#endif
