#ifndef __SK_PARKEDITOR2_PARKED_H
#define __SK_PARKEDITOR2_PARKED_H

#include <core/task.h>
#include <core/math/vector.h>
#include <gel/inpman.h>
#include <gel/Event.h>
#include <gel/modman.h>
#include <gfx/camera.h>
#include <sk/ParkEditor2/EdMap.h>

// At least this much is required on the top down heap during editing to ensure that
// UpdateSuperSectors will have enough space to execute when raising large areas of ground.
#ifdef __PLAT_NGC__
#define TOP_DOWN_REQUIRED_MARGIN 1000000
#define TOP_DOWN_REQUIRED_MARGIN_LEEWAY 100000
#else
#define TOP_DOWN_REQUIRED_MARGIN 1000000
// A defrag will only occur if freeing up the fragments will make the top down heap have this much
// more than the above margin. Could probably make this zero, just included to make sure it doesn't
// try to defrag too much.
#define TOP_DOWN_REQUIRED_MARGIN_LEEWAY 2000
#endif


class Coord
{
public:
	float m_X;
	float m_Y;
};


namespace Ed
{

enum
{
	MAX_THEMES=5
};

class CCursor;
class CUpperMenuManager;
class CClipboard;



class CParkEditor : public Mdl::Module, public Obj::CEventListener 
{
	
	DeclareSingletonClass( CParkEditor );
	
	CParkEditor();
	~CParkEditor();

public:

	enum EEditorState
	{
		vINACTIVE								= 0xd443a2bc,	// "off"
		vEDITING								= 0x7eca21e5,	// "edit"
		vTEST_PLAY								= 0xa40f0b8a,	// "test_play"
		vREGULAR_PLAY							= 0xcd3682c1,	// "regular_play"
		
		// use this as parameter only
		vKEEP_SAME_STATE						= 0,
	};

	enum EEditorCommand
	{
		vINITIALIZE								= 0x27f2117d,	// "initialize"
		vCLEAR									= 0x1a4e0ef9,	// "clear"
		vREGENERATE_FROM_MAP					= 0x2a907cbc,	// "regenerate_from_map"
		vBUILD_FLOOR							= 0x1625eda,	// "build_floor"

		vNO_COMMAND								= 0,
	};

	void										Initialize(bool edit_mode);
	void 										Rebuild(bool justRebuildFloor, bool clearMap, bool makeCursor);
	void										SetState(EEditorState desiredState);
	void										AccessDisk(bool save, int slot, bool blockRebuild);
	void 										PostMemoryCardLoad(uint8 * p_map_buffer, int oldTheme);
	void										Cleanup();
	
	void										SetAppropriateCamera();
	
	// K: Added to allow cleanup of the park editor heap during play
	void										DeleteCursor();

	void										Update();

	void										SwitchMenuPieceToMostRecentClipboard();
	
	void										SetPaused(bool paused);
	void 										MakeEditorStuffVisible( bool visible);
	
	bool										UsingCustomPark() {return m_state != vINACTIVE;}
	// returns true if actually editing right now
	bool										EditingCustomPark() {return m_state == vEDITING;}
	bool										TestingCustomPark() {return m_state == vTEST_PLAY;}

	// returns true if park is full
	bool	   									IsParkFull() {return m_pct_resources_used >= 1.0f;} 								
	bool										RoomToCopyOrPaste(int w, int l);

	void										BindParkEditorToController( int controller );
	Mth::Vector&								GetCursorPos();
	
	void										DestroyClipboardsWhichAreTooBigToFit();
	
	void										SetTimeOfDayScript(uint32 tod_script);
	uint32										GetTimeOfDayScript() {return m_tod_script;}
	
	void										CreatePlayModeGapManager();
	void										SwitchToPlayModeGapManager();
	void										DeletePlayModeGapManager();
	void										PlayModeGapManagerChecks();
	
private:	

	void										do_piece_select_commands();
	void										regular_command_logic();
	void										rail_placement_logic();
	
	int											m_last_main_heap_free;
	
	// A flag which is set whenever an operation is done that could increase memory usage.
	// This permits a defrag to be done if required. The flag is reset every frame.
	// The flag is required to prevent an infinite loop of defrags.
	bool 										m_allow_defrag;
	
	void										turn_on_or_update_piece_menu();
	void										update_analog_stick_menu_control_state();
	
	void										pass_event_to_listener(Obj::CEvent *pEvent);

	enum Commands
	{
		// some are aliases
		
		mROTATE_CW         	= nBit( 1 ),
		mROTATE_CCW        	= nBit( 2 ),
		mPREV_PIECE        	= nBit( 3 ),
		mNEXT_PIECE        	= nBit( 4 ),
		mPLACE_PIECE       	= nBit( 5 ),
		mREMOVE_PIECE      	= nBit( 6 ),
		mRAISE_FLOOR      	= nBit( 7 ),
		mLOWER_FLOOR      	= nBit( 8 ),
		mPREV_SET        	= nBit( 9 ),
		mNEXT_SET        	= nBit( 10 ),
		mZOOM_CAMERA_OUT	= nBit( 13 ),
		mZOOM_CAMERA_IN		= nBit( 14 ),

		mROTATE_GAP_CW		= nBit( 1 ),
		mROTATE_GAP_CCW		= nBit( 2 ),
		mPLACE_GAP       	= nBit( 5 ),
		mREMOVE_GAP       	= nBit( 6 ),
#ifdef __PLAT_NGC__
		mINCREASE_GAP_LEFT	= nBit( 0 ),
		mDECREASE_GAP_LEFT	= nBit( 15 ),
		mINCREASE_GAP_RIGHT	= nBit( 11 ),
		mDECREASE_GAP_RIGHT	= nBit( 12 ),
#else
		mINCREASE_GAP_LEFT	= nBit( 15 ),
		mDECREASE_GAP_LEFT	= nBit( 16 ),
		mINCREASE_GAP_RIGHT	= nBit( 11 ),
		mDECREASE_GAP_RIGHT	= nBit( 12 ),
#endif		// __PLAT_NGC__
	};

	static const float				vMAX_CAM_DIST;
	static const float				vMIN_CAM_DIST;
	static const float				vCAM_DIST_INC;
	static const float				vCAM_TARGET_ELEVATION;
	
	static	Tsk::Task< CParkEditor >::Code		s_logic_code;
	static	Tsk::Task< CParkEditor >::Code		s_display_code;
	static	Inp::Handler< CParkEditor >::Code  	s_input_logic_code;

	Tsk::Task< CParkEditor >*					m_logic_task;	
    Tsk::Task< CParkEditor >*					m_display_task;
    Inp::Handler< CParkEditor >*				m_input_handler;
	
	float										m_movement_vel;	// in inches per sec
	float										m_rotate_vel;	// in degrees per sec
    
	Coord                      			m_rightStick;
    Coord                      			m_leftStick;
    Flags<Commands>                 			m_commands;
	
	Gfx::Camera*								mp_camera;
	float										m_camAngle;
	float										m_camAngleVert;
	float										m_camDist;
	
	enum CursorState
	{
												vMOVEMENT,
												vGAP_MOVE,
												vGAP_ADJUST,
	};
	
	CursorState									m_cursor_state;
	
	void										v_start_cb( void );
	void										v_stop_cb( void );
    	
	Spt::SingletonPtr<CParkManager>				mp_park_manager;

	Mth::Vector									m_cursor_pos;
	CCursor *									mp_cursor;
	CUpperMenuManager *							mp_menu_manager;

	Tmr::Time									m_last_time;

	EEditorState								m_state;
	bool										m_paused;
	
	float										m_pct_resources_used;
	
	// This is the name of the time-of-day script which gets passed to the
	// script_change_tod script in the parameter tod_action.
	// (see timeofday.q for the above scripts)
	// The time of day gets set in Sk5Ed_Startup and in parked_test_play (both in sk5ed_scripts.q)
	// and in GameFlow_StartRun
	uint32										m_tod_script;
	
	CGapManager *								mp_play_mode_gap_manager;
	/* Debugging */
};




class CCursor
{
	friend class CParkEditor;

public:												
												CCursor();
												~CCursor();

	void										DeleteMeta();
	void 										DestroyGeometry();
	void										DestroyAnyClipboardCursors();
	void										Update(float shiftX, float shiftZ, int rotInc);
	bool										AttemptStamp();
	bool										AttemptRemove();
	void 										InformOfMetapieceDeletion(CConcreteMetaPiece *pMeta);
	bool										AttemptGap();
	bool										AttemptRemoveGap();
	bool 										AdjustGap(int rot, int leftChange, int rightChange);
	void										SetGapNameAndScore(const char *pGapName, int score);
	void										SetGapCancelFlags(Script::CStruct *p_cancelFlags);
	
	const char *								GetGapName();

	int											AttemptAreaSelect();
	void										ClearAreaSelection();
	void										DeleteSelectedPieces();
	void										ResetSelectedHeights();
	void										ContinueAreaSelection();
	void										GetAreaSelectDims(GridDims *p_dims);
	bool										DestroyMetasInCurrentArea();
	void										RefreshSelectionArea();
	void										ForceSelectionAreaHighlightRefresh() {m_initialised_highlight=false;}
	
	void										ChangePieceInSet(int dir);
	void										ChangeSet(int dir);
	void										SwitchMenuPieceToMostRecentClipboard();
	
	bool										ChangeFloorHeight(GridDims dims, int height, int dir, bool uniformHeight);
	bool										ChangeFloorHeight(int dir);
	void 										HighlightIntersectingMetas(bool on);
	void										SetVisibility(bool visible);

	int											GetSelectedSet(int *pMenuSetNumber) {*pMenuSetNumber = m_menu_set_number; return m_selected_set;}
	uint32										GetChecksum() {return mp_meta->GetNameChecksum();}

	enum ECursorMode
	{
		NONE,
		REGULAR,
		GAP,
		GAP_ADJUST,
		AREA_SELECT,
		PASTE,
		RAIL_PLACEMENT,
	};

	ECursorMode									GetCursorMode() {return m_mode;}
	bool										InGapMode() {return (m_mode == GAP || m_mode == GAP_ADJUST);}
	bool										InAreaSelectMode() {return m_mode==AREA_SELECT;}
	bool										InRailPlacementMode() {return m_mode==RAIL_PLACEMENT;}
	bool										InPasteMode() {return m_mode==PASTE;}
	Mth::ERot90									GetRotation() {return m_hard_rot;}
	const GridDims &							GetPosition() {return m_cell_dims;}
	bool										SelectionAreaTooBigToCopy();
	bool										CopySelectionToClipboard();
	void										DeleteOldestClipboard();
	
	void										PasteCurrentClipboard();
	int											GetNumClipboards();
	int											GetClipboardY() {return m_clipboard_y;}
		
	bool										IsSittingOnGap();
	bool										HalfFinishedGap() {return m_current_gap.numCompleteHalves == 1;}

	// K: Made this public so that it can be called from CParkEditor::Rebuild
	void										ForceInBounds();

	void										DestroyClipboardsWhichAreTooBigToFit();

	static	CCursor *							sInstance(bool assert = true);

protected:

	void										destroy_clipboards();
	void										get_selected_area_coords(uint8 *p_x1, uint8 *p_z1, uint8 *p_x2, uint8 *p_z2);
	
	void										change_cell_pos(int incX, int incZ);
	void										set_source_meta(uint32 nameChecksum);
	void										change_mode(ECursorMode newMode);
	void										RefreshHelperText();

	bool										is_gap_descriptor_same_as_current(CGapManager::GapDescriptor *pDesc);
	
	static const float							MOTION_PCT_INC;
	static const float							ROT_DEG_INC;
	
	Spt::SingletonPtr<CParkManager>				m_manager;

	GridDims									m_cell_dims;
	
	float										m_motion_pct;
	float										m_motion_pct_inc;
	
	Mth::Vector									m_target_pos;
	Mth::Vector									m_pos;
	bool										m_am_moving;

	Mth::ERot90									m_hard_rot;
	float										m_target_rot;
	float										m_rot; // rotation on y, in degrees


	CConcreteMetaPiece *						mp_meta;
	
	// Clipboard stuff
	uint8										m_num_corners_selected; // 0,1 or 2
	GridDims									mp_area_corners[2];
	bool										m_initialised_highlight;
	
	// List of clipboards
	CClipboard *								mp_clipboards;
	// This is the clip that will get placed when in paste mode.
	CClipboard *								mp_current_clipboard;
	CClipboard *								get_clipboard(int index);
	void										set_current_clipboard(int index);
	void										remove_clipboard(int index);
	// The y that must be added to the y of each clipboard entry such that
	// they align with the ground as best as possible.
	int											m_clipboard_y;
	

	
	
	int											m_selected_set;
	int											m_menu_set_number;

	ECursorMode									m_mode;

	/* Gap Stuff */

	CGapManager::GapDescriptor					m_current_gap;
	uint32										m_gap_suffix_counter;
	
	static CCursor *							sp_instance;
};




class CUpperMenuManager
{
public:
												CUpperMenuManager();
												~CUpperMenuManager();

	void 										SetSourceMeta(uint32 nameChecksum);
	void										SetPieceSet(CParkManager::CPieceSet *pSet, int set_number, int menuSetNumber);
	void										Enable();
	void										Disable();

protected:

	Spt::SingletonPtr<CParkManager>				m_manager;
	CParkManager::CPieceSet *					mp_current_set;
	int											m_current_set_index;
	int											m_current_entry_in_set;
};


#define PARK_BOUNDARY_MARGIN 10.0f
bool IsWithinParkBoundaries(Mth::Vector pos, float margin);


bool ScriptSetParkEditorTimeOfDay(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetParkEditorTimeOfDayScript(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRebuildParkNodeArray(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptFreeUpMemoryForPlayingPark(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCalibrateMemoryGauge(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetParkEditorCursorPos(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSwitchToParkEditorCamera(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetParkEditorState(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetParkEditorPauseMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCustomParkMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetParkEditorMaxPlayers(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetParkEditorMaxPlayers(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetParkEditorMaxPlayersPossible(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCanCleanlyResizePark(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptResizePark(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetCurrentParkBounds(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCanChangeParkDimension(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSaveParkToDisk(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLoadParkFromDisk(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptIsParkUnsaved(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptFireCustomParkGap(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetEditedParkGapInfo(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetEditedParkGapName(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptParkEditorSelectionAreaTooBigToCopy(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCopyParkEditorSelectionToClipboard(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSwitchParkEditorMenuPieceToMostRecentClipboard(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCutParkEditorAreaSelection(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptParkEditorAreaSelectionDeletePieces(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptContinueParkEditorAreaSelection(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetEditorTheme(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetEditorTheme(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetEditorMaxThemes(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptGetCustomParkName(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetCustomParkName(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptBindParkEditorToController(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptIsCustomPark(Script::CStruct *pParams, Script::CScript *pScript);
#ifdef __PLAT_NGC__
bool ScriptWriteCompressedMapBuffer(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRequiresDefragment(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptTopDownHeapTooLow(Script::CStruct *pParams, Script::CScript *pScript);
#endif


}
#endif
