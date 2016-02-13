#include <core/defines.h>
#include <gel/mainloop.h>
#include <gel/objtrack.h>
#include <gel/event.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/component.h>
#include <gel/object/compositeobjectmanager.h>
#include <sk/ParkEditor2/ParkEd.h>
#include <sk/scripting/cfuncs.h>
#include <sk/modules/FrontEnd/FrontEnd.h>
#include <gfx/nxviewman.h>
#include <gfx/2D/ScreenElemMan.h>
#include <gfx/2D/TextElement.h>
#include <gel/components/cameracomponent.h>
#include <sk/components/raileditorcomponent.h>
#include <sk/components/goaleditorcomponent.h>

#include <sk/modules/skate/skate.h>

#include <sk/ParkEditor2/clipboard.h>
#include <core/string/stringutils.h>

//#define DONT_USE_CURSOR_META

namespace Ed
{




const float CParkEditor::vMAX_CAM_DIST				= 1500.0f;
const float CParkEditor::vMIN_CAM_DIST				= 500.0f;
const float CParkEditor::vCAM_DIST_INC				= 16.0f;
const float CParkEditor::vCAM_TARGET_ELEVATION		= 300.0f;

DefineSingletonClass( CParkEditor, "Park editor module" );




CParkEditor::CParkEditor() :
	mp_park_manager(true)
{
	m_logic_task 	= new Tsk::Task< CParkEditor > ( CParkEditor::s_logic_code, *this );
	m_display_task 	= new Tsk::Task< CParkEditor > ( CParkEditor::s_display_code, *this, Tsk::BaseTask::Node::vDISPLAY_TASK_PRIORITY_CPARKEDITOR_DISPLAY );
	m_input_handler = new Inp::Handler< CParkEditor > ( 0,  CParkEditor::s_input_logic_code, *this);

	m_movement_vel = 480.0f; //8.0f;		// default movement velocity in inches per second
	m_rotate_vel = 120.0f;		// 120 degrees per second. In 3 seconds you should complete 1 revolution

	mp_camera = NULL;
	m_camDist = 1400.0f;
	m_camAngle = 0.0f; //225.0f;
	m_camAngleVert = 10.0f;

	m_cursor_state = vMOVEMENT;
	m_state = vINACTIVE;
	m_paused = false;

	mp_cursor = NULL;
	mp_menu_manager = NULL;
	
	m_pct_resources_used = 1.0f;
	m_last_main_heap_free=0;
	
	m_tod_script=0;

	mp_play_mode_gap_manager=NULL;

	m_allow_defrag=false;
		
	RegisterWithTracker(NULL);
}




CParkEditor::~CParkEditor()
{
}




void CParkEditor::Initialize(bool editMode)
{
	ParkEd("CParkEditor::Initialize()");
	//printf("CParkEditor::Initialize\n");
	
	mp_park_manager->Initialize();
	
	if (editMode && !mp_cursor)
	{
		mp_cursor = new CCursor();
		mp_menu_manager = new CUpperMenuManager();
	}
}




void CParkEditor::Rebuild(bool justRebuildFloor, bool clearMap, bool makeCursor)
{
	ParkEd("CParkEditor::Rebuild(justRebuildFloor = %d, clearMap = %d)", justRebuildFloor, clearMap);
	
	if (justRebuildFloor)
	{
		mp_park_manager->RebuildFloor();
	}
	else
	{
		if (mp_cursor)
			delete mp_cursor;
		mp_cursor = NULL;
		mp_park_manager->RebuildWholePark(clearMap);
	}	   
	
	if (makeCursor && !mp_cursor)
	{
		mp_cursor = new CCursor();
	}
	
	// Fix to TT1437, where the 1P restart piece was able to be placed outside the map if X pressed really quickly
	// just after nuking a park.
	if (mp_cursor)
	{
		mp_cursor->ForceInBounds();
	}
		
	// Regenerate the node array when the park is rebuilt (usually only happens when we are going to play the level)
	mp_park_manager->RebuildNodeArray();
	
	// and re register the gaps so that any old gaps are removed from the career.
	CGapManager::sInstance()->RegisterGapsWithSkaterCareer();			
}




void CParkEditor::SetState(EEditorState desiredState)
{
	ParkEd("CParkEditor::SetState(desiredState = %d)", desiredState);
	
	if (desiredState != vKEEP_SAME_STATE)
	{
		m_state = desiredState;
	}	
	
	// this prevents menu input from placing a piece
	m_commands.ClearAll();
	if (mp_cursor)
		mp_cursor->SetVisibility(m_state == vEDITING);

	if (desiredState == vTEST_PLAY || desiredState == vREGULAR_PLAY)
	{
	   	// XXX
		Ryan("$$$MakeEditorStuffVisible(false)\n");
		MakeEditorStuffVisible(false); 
		
		CGapManager::sInstance()->RegisterGapsWithSkaterCareer();	
	}
	else
	{
		// XXX
		Ryan("$$$MakeEditorStuffVisible(true)\n");
		MakeEditorStuffVisible(true); 	
	}

	Spt::SingletonPtr<Mdl::FrontEnd> p_front_end;	
	if (desiredState == vEDITING)
	{
		Mdl::Skate::Instance()->CleanupForParkEditor();
	
		p_front_end->SetAutoRepeatTimes(320, 100);
		mp_park_manager->HighlightMetasWithGaps(false, NULL);
	}
	else
	{
		// same as above, but what the hell
		p_front_end->SetAutoRepeatTimes(320, 50);
	}
	update_analog_stick_menu_control_state();
}




void CParkEditor::MakeEditorStuffVisible(bool visible)
{
	ParkEd("MakeEditorStuffVisible(visible = %d)", visible);
	
	// it would be nice to sort out the encapsulation sometime...	
	Ed::CParkManager::Instance()->GetGenerator()->HighlightAllPieces(false);
	Ed::CParkManager::Instance()->GetGenerator()->SetGapPiecesVisible(visible);
	Ed::CParkManager::Instance()->GetGenerator()->SetRestartPiecesVisible(visible);
	
	if (visible)
	{
		Script::RunScript("pause_trick_text");
		Script::RunScript("GoalManager_HidePoints");
		Script::RunScript("GoalManager_HideGoalPoints");
		Script::RunScript("SK4_Hide_Death_Message");
	}
	else
	{
		Script::RunScript("unpause_trick_text");
		Script::RunScript("GoalManager_ShowPoints");
		Script::RunScript("GoalManager_ShowGoalPoints");
	}
}




void CParkEditor::AccessDisk(bool save, int fileSlot, bool blockRebuild)
{
	ParkEd("CParkEditor::AccessDisk()");
	
	if (save)
	{
		CParkManager::sInstance()->AccessDisk(true, fileSlot);
	}
	else
	{
		int old_theme = CParkManager::sInstance()->GetTheme();
		
		if (m_state != vINACTIVE && !blockRebuild)
		{
			// need to destroy cursor geometry now, otherwise code will think park has changed
			Dbg_Assert(mp_cursor);
			mp_cursor->DestroyGeometry();
		}
		if (mp_cursor)
		{
			mp_cursor->DestroyAnyClipboardCursors();
		}	
		
		CParkManager::sInstance()->AccessDisk(false, fileSlot);
		if (m_state != vINACTIVE && !blockRebuild)
		{
			if (CParkManager::sInstance()->GetTheme() != old_theme)
			{
				Script::CStruct params;
				params.AddChecksum("level", CRCD(0xe8b4b836,"Load_Sk5Ed"));
				
				Script::RunScript("change_level", &params);
			}
			else
			{
				CParkManager::sInstance()->RebuildWholePark(false);
				mp_cursor->ChangePieceInSet(0);
				mp_park_manager->HighlightMetasWithGaps(false, NULL);
			}
		}
	}
}




void CParkEditor::PostMemoryCardLoad(uint8 * p_map_buffer, int oldTheme)
{
	bool needs_rebuild = (m_state == vEDITING || m_state == vTEST_PLAY);
	
	if (needs_rebuild)
	{
		// need to destroy cursor geometry now, otherwise code will think park has changed
		Dbg_Assert(mp_cursor);
		mp_cursor->DestroyGeometry();
	}
	// PATCH:  Copy buffer back over itself to set the flags
	Ed::CParkManager::Instance()->SetCompressedMapBuffer(p_map_buffer);
	if (needs_rebuild)
	{
		// XXX
		Ryan("old theme %d, new theme %d\n", oldTheme, CParkManager::sInstance()->GetTheme());
		if (CParkManager::sInstance()->GetTheme() != oldTheme)
		{
			Script::CStruct params;
			params.AddChecksum("level", CRCD(0xe8b4b836,"Load_Sk5Ed"));

			Script::RunScript("change_level", &params);
		}
		else
		{
			CParkManager::sInstance()->RebuildWholePark(false);
			mp_cursor->ChangePieceInSet(0);
			mp_park_manager->HighlightMetasWithGaps(false, NULL);
		}
	}
}


// K: Added to allow cleanup of the park editor heap during play
void CParkEditor::DeleteCursor()
{
	if (mp_cursor)
	{
		delete mp_cursor;
		mp_cursor=NULL;
	}
}

// Creates a copy of the gap manager on the script heap.
// CGapManager::sInstance() will still return the original gap manager afterwards however.
// This function is needed because to free up memory before playing a park, everything on the
// park editor heap is deleted and then the heap is deleted, and the original gap manager is on that heap.
// So a copy is made for play mode, since the gap manager is needed when a gap fires.
void CParkEditor::CreatePlayModeGapManager()
{
	Dbg_MsgAssert(mp_play_mode_gap_manager==NULL,("Expected mp_play_mode_gap_manager to be NULL"));
	Ed::CGapManager *p_original_gap_manager = CGapManager::sInstance();
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	mp_play_mode_gap_manager=new Ed::CGapManager(NULL);
	
	// Use the overloaded assignement operator to make the copy.
	*mp_play_mode_gap_manager = *p_original_gap_manager;
	
	Mem::Manager::sHandle().PopContext();
	
	
	// The constructor of CGapManager sets the sp_instance pointer, so set it back to the original so that
	// things don't get confused.
	p_original_gap_manager->SetInstance();
}

void CParkEditor::SwitchToPlayModeGapManager()
{
	Dbg_MsgAssert(CGapManager::sInstanceNoAssert() == NULL,("Expected CGapManager::sInstance() to be NULL"));
	Dbg_MsgAssert(mp_play_mode_gap_manager,("NULL mp_play_mode_gap_manager"));
	mp_play_mode_gap_manager->SetInstance();
}	

void CParkEditor::DeletePlayModeGapManager()
{
	if (mp_play_mode_gap_manager)
	{
		delete mp_play_mode_gap_manager;
		mp_play_mode_gap_manager=NULL;
	}
	Dbg_MsgAssert(CGapManager::sInstanceNoAssert() == NULL,("Expected CGapManager::sInstance() to be NULL"));
}

void CParkEditor::PlayModeGapManagerChecks()
{
	//printf("CGapManager::sInstance = 0x%08x\n",CGapManager::sInstanceNoAssert());
	//printf("mp_play_mode_gap_manager = 0x%08x\n",mp_play_mode_gap_manager);
}

void CParkEditor::Cleanup()
{
	ParkEd("CParkEditor::Cleanup()");

	m_state = vINACTIVE;
	if (mp_cursor)
	{
		delete mp_cursor;
		mp_cursor = NULL;
	}
	if (mp_menu_manager)
	{
		delete mp_menu_manager;
		mp_menu_manager = NULL;
	}
	
	mp_park_manager->Destroy(CParkGenerator::DESTROY_PIECES_AND_SECTORS);
	
	Obj::CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors = false;
	Obj::GetRailEditor()->DestroyRailGeometry();
	Obj::GetRailEditor()->DestroyPostGeometry();
	Obj::CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors = true;

	printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ DEATH OR GLORY 2 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
}

void CParkEditor::SetAppropriateCamera()
{
	if (mp_cursor && mp_cursor->InRailPlacementMode())
	{
		CFuncs::SetActiveCamera(CRCD(0x5b509ad3,"raileditor"), 0, false);
	}
	else
	{
		CFuncs::SetActiveCamera(CRCD(0x64716bee,"parked_cam"), 0, false);
	}	
}

void CParkEditor::do_piece_select_commands()
{
	if (m_commands.TestMask(mPREV_SET))
	{
		mp_cursor->ChangeSet(-1); 
		turn_on_or_update_piece_menu();
		Obj::CTracker* p_tracker = Obj::CTracker::Instance();
		p_tracker->LaunchEvent(Script::GenerateCRC("parked_menu_up"), Script::GenerateCRC("set_menu_container"));
		//mp_menu_manager->SetSourceMeta(mp_cursor->GetChecksum());
	}
	if (m_commands.TestMask(mNEXT_SET))
	{
		mp_cursor->ChangeSet(1); 
		turn_on_or_update_piece_menu();
		Obj::CTracker* p_tracker = Obj::CTracker::Instance();
		p_tracker->LaunchEvent(Script::GenerateCRC("parked_menu_down"), Script::GenerateCRC("set_menu_container"));
		//mp_menu_manager->SetSourceMeta(mp_cursor->GetChecksum());
	}
	if (m_commands.TestMask(mPREV_PIECE))
	{
		mp_cursor->ChangePieceInSet(-1); 
		turn_on_or_update_piece_menu();
		Obj::CTracker* p_tracker = Obj::CTracker::Instance();
		p_tracker->LaunchEvent(Script::GenerateCRC("parked_menu_left"), Script::GenerateCRC("piece_menu_container"));
		//mp_menu_manager->SetSourceMeta(mp_cursor->GetChecksum());
	}
	if (m_commands.TestMask(mNEXT_PIECE))
	{
		mp_cursor->ChangePieceInSet(1); 
		turn_on_or_update_piece_menu();
		Obj::CTracker* p_tracker = Obj::CTracker::Instance();
		p_tracker->LaunchEvent(Script::GenerateCRC("parked_menu_right"), Script::GenerateCRC("piece_menu_container"));
		//mp_menu_manager->SetSourceMeta(mp_cursor->GetChecksum());
	}
}

void CParkEditor::regular_command_logic()
{
	Dbg_MsgAssert(mp_cursor,("NULL mp_cursor"));

	Mth::Matrix cam_matrix;
	cam_matrix.Ident();
	cam_matrix.SetPos(Mth::Vector(0.0f, 0.0f, m_camDist, 1.0f));
	cam_matrix.RotateX(Mth::DegToRad(-m_camAngleVert));
	cam_matrix.RotateY(Mth::DegToRad(m_camAngle));
	cam_matrix.Translate(Mth::Vector(mp_cursor->m_pos[X], vCAM_TARGET_ELEVATION, mp_cursor->m_pos[Z], 0.0f));
	
	Obj::CCompositeObject * p_obj = (Obj::CCompositeObject *) Obj::CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0x64716bee,"parked_cam"));
	if (p_obj)
	{
		//printf("Setting cam pos\n");
		//CFuncs::SetActiveCamera(CRCD(0x64716bee,"parked_cam"), 0, false);
		
		//Obj::CCameraComponent * p_cam_comp = GetCameraComponentFromObject(p_obj); 			
		p_obj->SetPos(cam_matrix[Mth::POS]);
		p_obj->SetMatrix(cam_matrix);
	}


	if (!m_paused)
	{
		// XXX
		//Ryan("enraged monkey\n");

		// We unhighlight the meta-pieces intersecting the cursor before
		// we change or move it.
		if (!mp_cursor->InAreaSelectMode())
		{
			mp_cursor->HighlightIntersectingMetas(false); 
		}	
		
		Tmr::Time current_time = Tmr::GetTime();
		float elapsed_seconds = (int) (current_time - m_last_time) / 1000.0f;	
		m_last_time = current_time;
		
		static const	float	scale = ( 1.0f / ( 128.0f - Inp::vANALOGUE_TOL ));
		
		m_leftStick.m_Y =
			( m_leftStick.m_Y >  Inp::vANALOGUE_TOL ) ? ( m_leftStick.m_Y - Inp::vANALOGUE_TOL ) * scale :
			( m_leftStick.m_Y < -Inp::vANALOGUE_TOL ) ? ( m_leftStick.m_Y + Inp::vANALOGUE_TOL ) * scale : 0.0f;
		m_leftStick.m_X =
			( m_leftStick.m_X >  Inp::vANALOGUE_TOL ) ? ( m_leftStick.m_X - Inp::vANALOGUE_TOL ) * scale :
			( m_leftStick.m_X < -Inp::vANALOGUE_TOL ) ? ( m_leftStick.m_X + Inp::vANALOGUE_TOL ) * scale : 0.0f;
			
		float rel_shift_x = m_leftStick.m_X * elapsed_seconds * m_movement_vel;
		float rel_shift_z = m_leftStick.m_Y * elapsed_seconds * m_movement_vel;
		float angle_rad = m_camAngle * Mth::PI / 180.0f;
		float shift_x = cosf(angle_rad) * rel_shift_x + sinf(angle_rad) * rel_shift_z;
		float shift_z = cosf(angle_rad) * rel_shift_z - sinf(angle_rad) * rel_shift_x;
	
		m_rightStick.m_Y =
			( m_rightStick.m_Y >  Inp::vANALOGUE_TOL ) ? ( m_rightStick.m_Y - Inp::vANALOGUE_TOL ) * scale :
			( m_rightStick.m_Y < -Inp::vANALOGUE_TOL ) ? ( m_rightStick.m_Y + Inp::vANALOGUE_TOL ) * scale : 0.0f;
		m_rightStick.m_X =
			( m_rightStick.m_X >  Inp::vANALOGUE_TOL ) ? ( m_rightStick.m_X - Inp::vANALOGUE_TOL ) * scale :
			( m_rightStick.m_X < -Inp::vANALOGUE_TOL ) ? ( m_rightStick.m_X + Inp::vANALOGUE_TOL ) * scale : 0.0f;
	
#ifdef __PLAT_NGC__DAN
		if ( !( m_commands.TestMask(mZOOM_CAMERA_IN) ) )
#endif		// __PLAT_NGC__
		{
			m_camAngle += m_rightStick.m_X * elapsed_seconds * m_rotate_vel;	
		}
		if (m_camAngle < 0) m_camAngle += 360.0f;
		else if (m_camAngle >= 360.0) m_camAngle -= 360.0f;
#ifdef __PLAT_NGC__DAN
		if ( !( m_commands.TestMask(mZOOM_CAMERA_IN) ) )
#endif		// __PLAT_NGC__
		{
			m_camAngleVert -= m_rightStick.m_Y * elapsed_seconds * m_rotate_vel;	
		}
		if (m_camAngleVert < 10.0f) m_camAngleVert = 10.0f;
		else if (m_camAngleVert > 90.0) m_camAngleVert = 90.0f;

		/*
			PlayEdPlaceSound for placing items (might have wrong sound hooked up right now)
			PlayEdEraseSound  for removing items
			PlayRaiseGroundSound for raising
			PlayLowerGroundSound for lowering
			PlayRotatePieceSound for rotating both directions
		*/
		
		int rot_inc = 0;
		if (m_commands.TestMask(mROTATE_CW))
		{
			Script::RunScript("PlayRotatePieceSound");
			if (mp_cursor->InGapMode())
				mp_cursor->AdjustGap(-1, 0, 0);
			else
				rot_inc = -1;
		}
		if (m_commands.TestMask(mROTATE_CCW))
		{
			Script::RunScript("PlayRotatePieceSound");
			if (mp_cursor->InGapMode())
				mp_cursor->AdjustGap(-1, 0, 0);
			else
				rot_inc = 1;
		}
		if (m_commands.TestMask(mPLACE_PIECE))
		{
			if (mp_cursor->InGapMode())
			{
				bool success = mp_cursor->AttemptGap();
				if ((success || mp_cursor->IsSittingOnGap()) && !mp_cursor->HalfFinishedGap())
				{
					Script::RunScript("PlayEdPlaceSound");
					// XXX
					Ryan("the hell, you say\n");
					Script::RunScript("parked_setup_gap_menu");
				}
				else
					Script::RunScript("PlayEdBuzzSound");
					
				m_allow_defrag=true;
			}
			else if (mp_cursor->InAreaSelectMode())
			{
				switch (mp_cursor->AttemptAreaSelect())
				{
				case 1:
					// Maybe have a different sound for the first corner ?
					Script::RunScript("PlayEdPlaceSound");
					break;
				case 2:
					Script::RunScript("PlayEdPlaceSound");
					Script::RunScript("parked_setup_area_select_menu");
					break;
				default:
					Script::RunScript("PlayEdBuzzSound");
					break;
				}
				// Note: Not setting m_allow_defrag here, because otherwise it may defrag
				// as soon as the user places one of the area corners, which becomes a pain
				// especially if trying to select a set of pieces to delete to free up memory.
			}
			else if (mp_cursor->InPasteMode())
			{
				mp_cursor->PasteCurrentClipboard();
				m_allow_defrag=true;
			}
			else	
			{
				if (mp_cursor->AttemptStamp())
					Script::RunScript("PlayEdPlaceSound");
				else
					Script::RunScript("PlayEdBuzzSound");
				m_allow_defrag=true;
			}
		}
		if (m_commands.TestMask(mREMOVE_PIECE))
		{
			if (mp_cursor->InGapMode())
			{
				if (mp_cursor->AttemptRemoveGap())
				{
					Script::RunScript("PlayEdEraseSound");
				}
				else
					Script::RunScript("PlayEdBuzzSound");
			}
			else if (mp_cursor->InAreaSelectMode())
			{
				mp_cursor->ClearAreaSelection();
				m_allow_defrag=true;
			}
			else	
			{
				if (mp_cursor->AttemptRemove())
				{
					Script::RunScript("PlayEdEraseSound");
				}
				else
				{
					Script::RunScript("PlayEdBuzzSound");
				}
				m_allow_defrag=true;
			}
			// and re register the gaps so that any old gaps are removed from the career.
			CGapManager::sInstance()->RegisterGapsWithSkaterCareer();			
		}
		
		do_piece_select_commands();
		
		if (m_commands.TestMask(mRAISE_FLOOR))
		{
			if (!mp_cursor->InGapMode())
			{
				if (mp_cursor->ChangeFloorHeight(1))
					Script::RunScript("PlayRaiseGroundSound");
				else
					Script::RunScript("PlayEdBuzzSound");
			}
			m_allow_defrag=true;
		}
		if (m_commands.TestMask(mLOWER_FLOOR))
		{
			if (!mp_cursor->InGapMode())
			{
				if (mp_cursor->ChangeFloorHeight(-1))
					Script::RunScript("PlayLowerGroundSound");
				else
					Script::RunScript("PlayEdBuzzSound");
			}
			m_allow_defrag=true;
		}

#ifdef __PLAT_NGC__DAN
		if ( m_commands.TestMask(mZOOM_CAMERA_IN) )
		{
			// Use y component of stick to zoom camera in or out.
			m_camDist += vCAM_DIST_INC * m_rightStick.m_Y;
			if (m_camDist > vMAX_CAM_DIST)
				m_camDist = vMAX_CAM_DIST;
			if (m_camDist < vMIN_CAM_DIST)
				m_camDist = vMIN_CAM_DIST;
		}
#else
		if (m_commands.TestMask(mZOOM_CAMERA_OUT) && !mp_cursor->InGapMode())
		{
			m_camDist += vCAM_DIST_INC;
			if (m_camDist > vMAX_CAM_DIST)
				m_camDist = vMAX_CAM_DIST;
		}
		if (m_commands.TestMask(mZOOM_CAMERA_IN) && !mp_cursor->InGapMode())
		{
			m_camDist -= vCAM_DIST_INC;
			if (m_camDist < vMIN_CAM_DIST)
				m_camDist = vMIN_CAM_DIST;
		}
#endif		// __PLAT_NGC__
		
		if (m_commands.TestMask(mINCREASE_GAP_LEFT))
		{
			if (mp_cursor->InGapMode())
				mp_cursor->AdjustGap(0, 1, 0);
		}
		if (m_commands.TestMask(mINCREASE_GAP_RIGHT))
		{
			if (mp_cursor->InGapMode())
				mp_cursor->AdjustGap(0, 0, 1);
		}
		if (m_commands.TestMask(mDECREASE_GAP_LEFT))
		{
			if (mp_cursor->InGapMode())
				mp_cursor->AdjustGap(0, -1, 0);
		}
		if (m_commands.TestMask(mDECREASE_GAP_RIGHT))
		{
			if (mp_cursor->InGapMode())
				mp_cursor->AdjustGap(0, 0, -1);
		}


		// If an operation has occurred that has modified collidable geometry,
		// run through all the existing created-rails and update their posts so that they
		// maintain ground contact.
		if (m_commands.TestMask(mPLACE_PIECE) ||
			m_commands.TestMask(mREMOVE_PIECE) ||
			m_commands.TestMask(mRAISE_FLOOR) ||
			m_commands.TestMask(mLOWER_FLOOR))
		{
			Obj::CRailEditorComponent *p_rail_editor=Obj::GetRailEditor();
			p_rail_editor->AdjustYs();
			p_rail_editor->UpdateRailGeometry();
			p_rail_editor->UpdatePostGeometry();
			
			Obj::GetGoalEditor()->RefreshGoalPositionsUsingCollisionCheck();
		}
		
		mp_cursor->Update(shift_x, shift_z, rot_inc);
		//printf("cursor position: (%.2f,%.2f)\n", mp_cursor->m_target_pos[X], mp_cursor->m_target_pos[Z]);		
		
		if (mp_cursor->InAreaSelectMode())
		{
			mp_cursor->RefreshSelectionArea();
		}
		else if (mp_cursor->InRailPlacementMode())
		{
			mp_cursor->DestroyGeometry();
		}
		else
		{
			// Highlight the pieces intersecting the cursor after it has changed or moved
			mp_cursor->HighlightIntersectingMetas(true); 
		}	
	}	
}

void CParkEditor::rail_placement_logic()
{
	Dbg_MsgAssert(mp_cursor,("NULL mp_cursor"));

	if (!m_paused)
	{
		do_piece_select_commands();
	}
}
	
void CParkEditor::Update()
{
	if (!m_paused)
	{
		if (m_state == vEDITING)
		{
			Script::CStruct params;
			params.AddChecksum(NONAME, Script::GenerateCRC("hide"));
			CFuncs::ScriptPauseSkaters(&params, NULL);
		}
		else if (m_state == vTEST_PLAY)
		{
			CFuncs::ScriptUnPauseSkaters(NULL, NULL);
		}
	}

	if (m_state == vEDITING && mp_cursor)
	{
		if (!m_paused)
		{
			if (mp_cursor->InRailPlacementMode())
			{
				if (Obj::GetRailEditor()->IsSuspended())
				{
					Script::RunScript(CRCD(0x2c3b17ea,"SwitchOnRailEditor"));
				}
				
				rail_placement_logic();
				
				if (!mp_cursor->InRailPlacementMode())
				{
					Script::RunScript(CRCD(0x32428a49,"SwitchOffRailEditor"));
				}
			}
			else
			{
				if (!Obj::GetRailEditor()->IsSuspended())
				{
					Script::RunScript(CRCD(0x32428a49,"SwitchOffRailEditor"));
				}
				regular_command_logic();
				if (mp_cursor->InRailPlacementMode())
				{
					Script::RunScript(CRCD(0x2c3b17ea,"SwitchOnRailEditor"));
				}
			}	
		}

		if (!m_paused)		
		{
			CParkGenerator *p_generator = mp_park_manager->GetGenerator();
			CParkGenerator::MemUsageInfo usage_info = p_generator->GetResourceUsageInfo();
	
			#ifdef	__NOPT_ASSERT__
			if (Script::GetInteger(CRCD(0xcc55a87b,"show_parked_main_heap_free")))
			{
				printf("usage_info.mMainHeapFree = %d   usage_info.mParkHeapFree=%d\n",usage_info.mMainHeapFree,usage_info.mParkHeapFree);
			}	
			#endif
			

			if (usage_info.mMainHeapFree >= 0)
			{
				m_pct_resources_used=1.0f-((float)usage_info.mMainHeapFree)/((float)p_generator->GetResourceSize("main_heap_base"));
			}
			else
			{
				#ifdef	__NOPT_ASSERT__
				printf("Memory overflow! Gone over the end of the gauge by %d bytes\n",-usage_info.mMainHeapFree);
				#endif
				m_pct_resources_used=1.0f;
			}	

			Mem::Heap *p_bottom_up_heap = Mem::Manager::sHandle().BottomUpHeap();
			Mem::Heap *p_top_down_heap = Mem::Manager::sHandle().TopDownHeap();
			#ifdef	__NOPT_ASSERT__
			if (Script::GetInteger(CRCD(0xe2ca7dfc,"show_actual_heap_status")))
			{
				printf("Bottom up = %d  Top down = %d\n",p_bottom_up_heap->mFreeMem.m_count,p_top_down_heap->mp_region->MemAvailable());
			}	
			#endif
			
			// top_down_pct is a metric based on the amount of top down heap available. The reason for the 2.0 is to
			// make the top_down_pct be >= 1.0 if there is less than TOP_DOWN_REQUIRED_MARGIN available, and < 1.0
			// if there is more than that available.
			float top_down_pct=2.0f-(((float)p_top_down_heap->mp_region->MemAvailable()) / ((float)TOP_DOWN_REQUIRED_MARGIN));
			if (top_down_pct > m_pct_resources_used)
			{
				m_pct_resources_used = top_down_pct;
			}	


			float park_heap_pct=1.0f-((float)usage_info.mParkHeapFree)/((float)p_generator->GetResourceSize("park_heap_base"));
			if (park_heap_pct > m_pct_resources_used)
			{
				m_pct_resources_used=park_heap_pct;
			}	

			m_last_main_heap_free=usage_info.mMainHeapFree;

			//printf("2: m_pct_resources_used = %f\n",m_pct_resources_used);
			
			//float rail_point_pct = (float) usage_info.mTotalRailPoints / (float) (p_generator->GetResourceSize("out_railpoint_pool") - 25);
			int component_use_est = usage_info.mTotalClonedPieces * 7 + usage_info.mTotalRailPoints * 9 + usage_info.mTotalLinkedRailPoints;
			int base_component_use = p_generator->GetResourceSize("component_use_base");
			float component_pct = (float) (component_use_est - base_component_use) / (float) (p_generator->GetResourceSize("max_components") - base_component_use);
			if (m_pct_resources_used < component_pct)
				m_pct_resources_used = component_pct;
			//printf("component_use_est=%d\n",component_use_est);
			//ParkEd("3: m_pct_resources_used=%f",m_pct_resources_used);
			
			int vector_use_est = usage_info.mTotalClonedPieces * 2 + usage_info.mTotalRailPoints * 2;
			int base_vector_use = p_generator->GetResourceSize("vector_use_base");
			float vector_pct = (float) (vector_use_est - base_vector_use) / (float) (p_generator->GetResourceSize("max_vectors") - base_vector_use);
			if (m_pct_resources_used < vector_pct)
				m_pct_resources_used = vector_pct;
			//printf("vector_use_est=%d\n",vector_use_est);	
			//ParkEd("4: m_pct_resources_used=%f",m_pct_resources_used);
			//printf("3: m_pct_resources_used = %f\n",m_pct_resources_used);

			float pieces_pct=(float)mp_park_manager->GetDMAPieceCount() / (float)p_generator->GetResourceSize("max_dma_pieces");
			if (pieces_pct > m_pct_resources_used)
			{
				m_pct_resources_used = pieces_pct;
			}
			
			//printf("4: m_pct_resources_used = %f IsParkFull=%d\n",m_pct_resources_used,IsParkFull());
			
			float shown_pct_resources_used = m_pct_resources_used;
			if (shown_pct_resources_used < 0.0f) shown_pct_resources_used = 0.0f;
			else if (shown_pct_resources_used > 1.0f) shown_pct_resources_used = 1.0f;



			bool do_defragment=false;
			
			// Don't defrag if in rail placement mode, cos the resulting mode switch causes an assert.
			if (mp_cursor->InRailPlacementMode())
			{
				m_allow_defrag=false;
			}
			// m_allow_defrag is a flag that gets set by any operation that may have increased
			// memory usage, such as placing a piece, or raising the ground.
			// It is required to prevent an infinite loop of defrags which could occur if defrags
			// were allowed every frame.
			
			if (shown_pct_resources_used >= 1.0f && p_top_down_heap->mp_region->MemAvailable() < TOP_DOWN_REQUIRED_MARGIN)
			{
				// We're in trouble, there may not be enough top down heap for UpdateSuperSectors
				// to execute. This can happen if trying to raise a large chunk of ground.
				if (p_bottom_up_heap->mFreeMem.m_count + p_top_down_heap->mp_region->MemAvailable() > (TOP_DOWN_REQUIRED_MARGIN + TOP_DOWN_REQUIRED_MARGIN_LEEWAY))
				{
					// ... but there would be more than enough if all the fragments could be utilised,
					// so do a defragment to see if that helps.
					if (m_allow_defrag)
					{
						do_defragment=true;
					}	
				}
			}	

			
			// The old logic from THPS4, which would only defrag if this flag was set.
			/*
			if (usage_info.mIsFragmented)
			{
				if (m_allow_defrag)
				{
					do_defragment=true;
				}	
			}
			*/

			m_allow_defrag=false;
			
			if (do_defragment)
			{
				#ifdef __PLAT_NGC__
				// The regular defragment script often does not fully cure the fragmentation, which
				// is a problem on the NGC because of its lower memory. So do a full reload instead.
				Script::SpawnScript(CRCD(0x57c05a9a,"reload_park"));
				#else
					// XXX
					Ryan("defragging, memory free (largest block) is %d\n", usage_info.mMainHeapFree);
					SetPaused(true);
					// run a script to defragment memory
					#ifdef __NOPT_ASSERT__
					Script::CScript *p_script=Script::SpawnScript("parked_defragment_memory");
					p_script->SetCommentString("Spawned by CParkEditor::Update()");
					#else
					Script::SpawnScript("parked_defragment_memory");
					#endif
				#endif
				return;
			}



			// percent_bar_colored_part
			Front::CScreenElementManager* p_elem_man = Front::CScreenElementManager::Instance();
			Front::CScreenElement *p_pct_bar = p_elem_man->GetElement(CRCD(0xcd9c729a, "percent_bar_colored_part"), Front::CScreenElementManager::DONT_ASSERT).Convert();
			if (p_pct_bar)
				p_pct_bar->SetScale(shown_pct_resources_used, 1.0f, false, Front::CScreenElement::FORCE_INSTANT);

			if (Script::GetInteger(CRCD(0x7a9c1acd,"parked_show_memory_stats")))
			{
				Script::RunScript(CRCD(0xb935948d,"CreateMemStatsScreenElements"));
				
				Front::CScreenElementManager* p_elem_man = Front::CScreenElementManager::Instance();
				Front::CTextElement *p_park_free_elem = (Front::CTextElement *) p_elem_man->GetElement(CRCD(0xa5a33eb9, "parked_mem_stats_park_free"), Front::CScreenElementManager::ASSERT).Convert();
				char park_free_text[128];
				sprintf(park_free_text, "park heap free:\\c2 %s", Str::PrintThousands(usage_info.mParkHeapFree));
				p_park_free_elem->SetText(park_free_text);
	
				Front::CTextElement *p_main_free_elem = (Front::CTextElement *) p_elem_man->GetElement(CRCD(0xcd38c218, "parked_mem_stats_main_free"), Front::CScreenElementManager::ASSERT).Convert();
				char main_free_text[128];
				if (usage_info.mIsFragmented)
					sprintf(main_free_text, "main heap free:\\c2 %s \\c1FRAG", Str::PrintThousands(usage_info.mMainHeapFree));
				else
					sprintf(main_free_text, "main heap free:\\c2 %s", Str::PrintThousands(usage_info.mMainHeapFree));
				p_main_free_elem->SetText(main_free_text);
	
				Front::CTextElement *p_last_op_elem = (Front::CTextElement *) p_elem_man->GetElement(CRCD(0x9dd4532f, "parked_mem_stats_last_op"), Front::CScreenElementManager::ASSERT).Convert();
				char last_op_text[128];
				sprintf(last_op_text, "last op size:\\c2 %d", usage_info.mMainHeapUsed - usage_info.mLastMainUsed);
				p_last_op_elem->SetText(last_op_text);
	
				Front::CTextElement *p_other_elem = (Front::CTextElement *) p_elem_man->GetElement(CRCD(0xb42704a3, "parked_mem_stats_other"), Front::CScreenElementManager::ASSERT).Convert();
				char other_text[128];
				sprintf(other_text, "components free:\\c2 %d %d", 
						p_generator->GetResourceSize("max_components") - component_use_est - base_component_use,
						p_generator->GetResourceSize("max_vectors") - vector_use_est - base_vector_use);
				p_other_elem->SetText(other_text);
	
				Front::CTextElement *p_pct_elem = (Front::CTextElement *) p_elem_man->GetElement(CRCD(0x65f2394f, "parked_mem_stats_pct"), Front::CScreenElementManager::ASSERT).Convert();
				char pct_text[128];
				sprintf(pct_text, "pct:\\c3 %.2f", m_pct_resources_used * 100.0f);
				p_pct_elem->SetText(pct_text);
	
				Front::CTextElement *p_pieces_remaining_elem = (Front::CTextElement *) p_elem_man->GetElement(CRCD(0x3da56dac, "parked_mem_pieces_remaining"), Front::CScreenElementManager::ASSERT).Convert();
				char pieces_remaining_text[128];
				sprintf(pieces_remaining_text, "pieces remaining:\\c2 %d", p_generator->GetResourceSize("max_dma_pieces") - mp_park_manager->GetDMAPieceCount());
				p_pieces_remaining_elem->SetText(pieces_remaining_text);

				Front::CTextElement *p_main_used_elem = (Front::CTextElement *) p_elem_man->GetElement(CRCD(0xf95160c8, "parked_mem_stats_main_used"), Front::CScreenElementManager::ASSERT).Convert();
				char main_used_text[128];
				sprintf(main_used_text, "main heap used:\\c2 %s", Str::PrintThousands(usage_info.mMainHeapUsed));
				p_main_used_elem->SetText(main_used_text);
			}
			else
			{
				Script::RunScript(CRCD(0x7a449180,"DestroyMemStatsScreenElements"));
			}	

			// Update the clipboard usage bar.			
			/*
			Front::CScreenElement *p_clipboard_percent_bar = p_elem_man->GetElement(CRCD(0xf07100ce,"clipboard_percent_bar_colored_part"), Front::CScreenElementManager::DONT_ASSERT).Convert();
			if (p_clipboard_percent_bar)
			{
				p_clipboard_percent_bar->SetScale(mp_park_manager->GetClipboardProportionUsed(), 1.0f, false, Front::CScreenElement::FORCE_INSTANT);
			}
			*/
		} // end if (!m_paused)
	}
	else
	{
		m_commands.ClearAll();
	}  
}

void CParkEditor::SwitchMenuPieceToMostRecentClipboard()
{
	CCursor::sInstance()->SwitchMenuPieceToMostRecentClipboard();
	
	turn_on_or_update_piece_menu();
	Obj::CTracker* p_tracker = Obj::CTracker::Instance();
	p_tracker->LaunchEvent(Script::GenerateCRC("parked_menu_up"), Script::GenerateCRC("set_menu_container"));
}

// Returns true if there is enough memory to copy or paste an area of the given size.
bool CParkEditor::RoomToCopyOrPaste(int w, int l)
{
	#ifdef __PLAT_NGC__
	Mem::Heap *p_top_down_heap = Mem::Manager::sHandle().TopDownHeap();
	if (p_top_down_heap->mp_region->MemAvailable() < TOP_DOWN_REQUIRED_MARGIN)
	{
		return false;
	}	
	#endif
	
	// Assumes that in the worst case 1 cell uses up 40K, for example when a bunch of fountain pieces are raised
	// to the maximum height.
	return w * l * 40000 < m_last_main_heap_free;
}

void CParkEditor::SetPaused(bool paused)
{
	// XXX
	Ryan("some fucker has made my pause state = %d\n", paused);
	m_paused = paused;

	if (mp_menu_manager)
	{
		if (paused)
		{
			Script::RunScript("parked_kill_wait_message");
			mp_menu_manager->Disable();
		}
		else
		{
			mp_menu_manager->Enable();
			turn_on_or_update_piece_menu();
		}
	}

	if (mp_cursor)
	{
		if (mp_cursor->InRailPlacementMode())
		{
			mp_cursor->SetVisibility(false);
		}
		else
		{
			if (paused)
			{
				mp_cursor->SetVisibility(false);
			}		
			else
			{
				mp_cursor->SetVisibility(true);
			}		
		}	
		mp_cursor->RefreshHelperText();
	}	

	update_analog_stick_menu_control_state();
}




void CParkEditor::turn_on_or_update_piece_menu()
{
	int menu_set_number;
	int set_num = mp_cursor->GetSelectedSet(&menu_set_number);
	// XXX
	Ryan("piece set number is %d\n", set_num);
	Dbg_Assert(mp_menu_manager);
	mp_menu_manager->SetPieceSet(&mp_park_manager->GetPieceSet(set_num), set_num, menu_set_number);
	
	if (mp_cursor)
	{
		mp_cursor->RefreshHelperText();
	}	
}




void CParkEditor::update_analog_stick_menu_control_state()
{
	Spt::SingletonPtr<Mdl::FrontEnd> p_front_end;	
	if (m_state == vEDITING && !m_paused)
	{
		p_front_end->SetAnalogStickActiveForMenus(false);
	}
	else
		p_front_end->SetAnalogStickActiveForMenus(true);
}




void CParkEditor::pass_event_to_listener(Obj::CEvent *pEvent)
{
	if (m_state != vEDITING || !mp_cursor || m_paused)
		return;
	
	uint32 type = pEvent->GetType();
	uint32 target = pEvent->GetTarget();

	if (target != CRCD(0x56a1eae3, "root_window"))
		return;

#ifdef __PLAT_NGC__
	if ( m_commands.TestMask( mINCREASE_GAP_RIGHT ) ) return;
	if ( m_commands.TestMask( mDECREASE_GAP_RIGHT ) ) return;
	if ( m_commands.TestMask( mINCREASE_GAP_LEFT ) ) return;
	if ( m_commands.TestMask( mDECREASE_GAP_LEFT ) ) return;
#endif		// __PLAT_NGC__

	switch(type)
	{
		case Obj::CEvent::TYPE_PAD_UP:
			m_commands.SetMask( mPREV_SET );
			break;
		case Obj::CEvent::TYPE_PAD_DOWN:
			m_commands.SetMask( mNEXT_SET );
			break;
		case Obj::CEvent::TYPE_PAD_LEFT:
			m_commands.SetMask( mPREV_PIECE );
			break;
		case Obj::CEvent::TYPE_PAD_RIGHT:
			m_commands.SetMask( mNEXT_PIECE );
			break;
		default:
			break;
	}
	//Ryan("I got an event!!\n");
}




void CParkEditor::v_start_cb()
{
	
	Ryan("CParkEditor2::v_start_cb\n");

	Mlp::Manager * mlp_manager = Mlp::Manager::Instance(); 
	mlp_manager->AddLogicTask ( *m_logic_task );
	Inp::Manager * inp_manager = Inp::Manager::Instance(); 
	inp_manager->AddHandler( *m_input_handler );
	mlp_manager->AddDisplayTask ( *m_display_task );
}




void CParkEditor::v_stop_cb()
{
	Ryan("CParkEditor2::v_stop_cb\n");

	m_logic_task->Remove();
	m_input_handler->Remove();
}




void CParkEditor::s_input_logic_code ( const Inp::Handler < CParkEditor >& handler )
{
	CParkEditor &mdl = handler.GetData();
	// first clear last frame's data
	mdl.m_commands.ClearAll();

	//if (!mdl.mp_map || mdl.GameGoingOrOutsideEditor() || mdl.InMenu() || !mdl.m_initialized) return;
	
	// translate m_Input data to module-specific commands
	mdl.m_rightStick.m_X = handler.m_Input->m_Event[Inp::Data::vA_RIGHT_X] - 128;
	mdl.m_rightStick.m_Y = handler.m_Input->m_Event[Inp::Data::vA_RIGHT_Y] - 128;
	mdl.m_leftStick.m_X = handler.m_Input->m_Event[Inp::Data::vA_LEFT_X] - 128;
	mdl.m_leftStick.m_Y = handler.m_Input->m_Event[Inp::Data::vA_LEFT_Y] - 128;

#ifdef __PLAT_NGC__
	if( handler.m_Input->m_Makes & Inp::Data::mD_TRIANGLE )
	{
		mdl.m_commands.SetMask( mROTATE_CCW );
	}
#else
	if( handler.m_Input->m_Makes & Inp::Data::mD_SQUARE )
	{
		mdl.m_commands.SetMask( mROTATE_CCW );
	}
#endif		// __PLAT_NGC__
	if( handler.m_Input->m_Makes & Inp::Data::mD_CIRCLE )
	{
		mdl.m_commands.SetMask( mROTATE_CW );
	}
	if( handler.m_Input->m_Makes & Inp::Data::mD_X )
	{
		mdl.m_commands.SetMask( mPLACE_PIECE );
	}
#ifdef __PLAT_NGC__
	if( handler.m_Input->m_Makes & Inp::Data::mD_SQUARE )
	{
		mdl.m_commands.SetMask( mREMOVE_PIECE );
	}
#else
	if( handler.m_Input->m_Makes & Inp::Data::mD_TRIANGLE )
	{
		mdl.m_commands.SetMask( mREMOVE_PIECE );
	}
#endif		// __PLAT_NGC__
#ifdef __PLAT_NGC__
	if( !(handler.m_Input->m_Buttons & Inp::Data::mD_Z) )
	{
		if( handler.m_Input->m_Makes & Inp::Data::mD_L1 )
		{
			mdl.m_commands.SetMask( mRAISE_FLOOR );
		}
		if( handler.m_Input->m_Makes & Inp::Data::mD_R1 )
		{
			mdl.m_commands.SetMask( mLOWER_FLOOR );
		}
	}
#else
	if( handler.m_Input->m_Makes & Inp::Data::mD_L1 )
	{
		mdl.m_commands.SetMask( mRAISE_FLOOR );
	}
	if( handler.m_Input->m_Makes & Inp::Data::mD_L2 )
	{
		mdl.m_commands.SetMask( mLOWER_FLOOR );
	}
#endif		// __PLAT_NGC__
#ifdef __PLAT_NGC__
	if( ( handler.m_Input->m_Makes & Inp::Data::mD_UP ) && ( handler.m_Input->m_Buttons & Inp::Data::mD_R1 ) )
	{
		mdl.m_commands.SetMask( mINCREASE_GAP_RIGHT );
	}
	if( ( handler.m_Input->m_Makes & Inp::Data::mD_DOWN ) && ( handler.m_Input->m_Buttons & Inp::Data::mD_R1 ) )
	{
		mdl.m_commands.SetMask( mDECREASE_GAP_RIGHT );
	}
	if( ( handler.m_Input->m_Makes & Inp::Data::mD_UP ) && ( handler.m_Input->m_Buttons & Inp::Data::mD_L1 ) )
	{
		mdl.m_commands.SetMask( mINCREASE_GAP_LEFT );
	}
	if( ( handler.m_Input->m_Makes & Inp::Data::mD_DOWN ) && ( handler.m_Input->m_Buttons & Inp::Data::mD_L1 ) )
	{
		mdl.m_commands.SetMask( mDECREASE_GAP_LEFT );
	}
#else
	#ifdef __PLAT_XBOX__
		if( handler.m_Input->m_Makes & Inp::Data::mD_L1 )
		{
			mdl.m_commands.SetMask( mINCREASE_GAP_RIGHT );
		}
		if( handler.m_Input->m_Makes & Inp::Data::mD_L2 )
		{
			mdl.m_commands.SetMask( mDECREASE_GAP_RIGHT );
		}
		if( handler.m_Input->m_Makes & Inp::Data::mD_BLACK )
		{
			mdl.m_commands.SetMask( mINCREASE_GAP_LEFT );
		}
		if( handler.m_Input->m_Makes & Inp::Data::mD_WHITE )
		{
			mdl.m_commands.SetMask( mDECREASE_GAP_LEFT );
		}
	#else
		if( handler.m_Input->m_Makes & Inp::Data::mD_R1 )
		{
			mdl.m_commands.SetMask( mINCREASE_GAP_RIGHT );
		}
		if( handler.m_Input->m_Makes & Inp::Data::mD_R2 )
		{
			mdl.m_commands.SetMask( mDECREASE_GAP_RIGHT );
		}
		if( handler.m_Input->m_Makes & Inp::Data::mD_L1 )
		{
			mdl.m_commands.SetMask( mINCREASE_GAP_LEFT );
		}
		if( handler.m_Input->m_Makes & Inp::Data::mD_L2 )
		{
			mdl.m_commands.SetMask( mDECREASE_GAP_LEFT );
		}
	#endif
	
#endif		// __PLAT_NGC__
#ifdef __PLAT_NGC__
	if( handler.m_Input->m_Buttons & Inp::Data::mD_Z )
	{
		if( handler.m_Input->m_Buttons & Inp::Data::mD_L1 )
		{
			mdl.m_commands.SetMask( mZOOM_CAMERA_IN );
		}
		if( handler.m_Input->m_Buttons & Inp::Data::mD_R1 )
		{
			mdl.m_commands.SetMask( mZOOM_CAMERA_OUT );
		}
	}
#else
#ifdef __PLAT_XBOX__
	if( handler.m_Input->m_Buttons & Inp::Data::mD_BLACK )
	{
		mdl.m_commands.SetMask( mZOOM_CAMERA_OUT );
	}
	if( handler.m_Input->m_Buttons & Inp::Data::mD_WHITE )
	{
		mdl.m_commands.SetMask( mZOOM_CAMERA_IN );
	}
#else
	if( handler.m_Input->m_Buttons & Inp::Data::mD_R1 )
	{
		mdl.m_commands.SetMask( mZOOM_CAMERA_OUT );
	}
	if( handler.m_Input->m_Buttons & Inp::Data::mD_R2 )
	{
		mdl.m_commands.SetMask( mZOOM_CAMERA_IN );
	}
#endif		// __PLAT_XBOX__	
#endif		// __PLAT_NGC__
}




void CParkEditor::s_logic_code ( const Tsk::Task< CParkEditor >& task )
{
	

	CParkEditor &mdl = task.GetData();
	//if (mdl.mp_commandParams)
	//	mdl.runCommand();	
	//if (!mdl.mp_map || mdl.GameGoingOrOutsideEditor() || !mdl.m_initialized) return;

	/*
	// put skater up in air HACK
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	Obj::CSkater *pSkater = skate_mod->GetLocalSkater();
	if (pSkater)
		pSkater->m_pos.Set(4000.0f, 4000.0f, 0.0f);
	*/
	
	Tmr::Time elapsed_time;
	static Tmr::Time last_time = 0;
	
	elapsed_time = Tmr::ElapsedTime( last_time );
	last_time = last_time + elapsed_time;   
	
	mdl.Update();
}




void CParkEditor::s_display_code ( const Tsk::Task< CParkEditor >& task )
{
	//CParkEditor &mdl = task.GetData();
	//if (!mdl.mp_map || mdl.GameGoingOrOutsideEditor() || !mdl.m_initialized) return;
	
	//mdl.Draw();
}

void CParkEditor::BindParkEditorToController( int controller )
{
	Spt::SingletonPtr< Inp::Manager > inp_manager;
	inp_manager->ReassignHandler( *m_input_handler, controller );
}

Mth::Vector& CParkEditor::GetCursorPos()
{
	Dbg_MsgAssert(mp_cursor,("NULL mp_cursor"));
	return mp_cursor->m_pos;
}

void CParkEditor::DestroyClipboardsWhichAreTooBigToFit()
{
	if (mp_cursor)
	{
		mp_cursor->DestroyClipboardsWhichAreTooBigToFit();
	}	
}

void CParkEditor::SetTimeOfDayScript(uint32 tod_script)
{
	m_tod_script=tod_script;
}	

CCursor *CCursor::sp_instance = NULL;
const float CCursor::MOTION_PCT_INC = .15f;
const float CCursor::ROT_DEG_INC = 15.0f;




CCursor::CCursor() :
	m_cell_dims(0, 0, 0),
	m_target_pos(0.0f, 0.0f, 0.0f),
	m_pos(0.0f, 0.0f, 0.0f),
	m_hard_rot(Mth::ROT_0),
	m_target_rot(0.0f),
	m_rot(0.0f),
	mp_meta(NULL)
{
	m_num_corners_selected=0;
	m_initialised_highlight=false;
	
	mp_clipboards=NULL;
	mp_current_clipboard=NULL;
	
	m_hard_rot = Mth::ROT_0;

	m_motion_pct = 1.0f;

	m_selected_set = 0;
	m_menu_set_number = 0;
	
	m_current_gap.numCompleteHalves = 0;
	m_current_gap.mCancelFlags=0;
	
	m_mode = NONE;
	
	CParkManager::CPieceSet &piece_set = m_manager->GetPieceSet(m_selected_set, &m_menu_set_number);
	set_source_meta(piece_set.mEntryTab[piece_set.mSelectedEntry].mNameCrc);

	m_gap_suffix_counter=1;
	
	sp_instance = this;
}

CCursor::~CCursor()
{
	#ifndef DONT_USE_CURSOR_META
	if (mp_meta)
	{
		mp_meta->MarkUnlocked();
		m_manager->DestroyConcreteMeta(mp_meta, CParkManager::mDONT_DESTROY_PIECES_ABOVE);
	}
	
	#endif

	destroy_clipboards();
	
	sp_instance = NULL;
}

// K: Added to test something
void CCursor::DeleteMeta()
{
	if (mp_meta)
	{
		delete mp_meta;
		mp_meta=NULL;
	}
}
		
void CCursor::destroy_clipboards()
{
	CClipboard *p_clipboard=mp_clipboards;
	while (p_clipboard)
	{
		CClipboard *p_next=p_clipboard->mpNext;
		delete p_clipboard;
		p_clipboard=p_next;
	}	
	mp_current_clipboard=NULL;
}

// Called when the park is resized.
void CCursor::DestroyClipboardsWhichAreTooBigToFit()
{
	CClipboard *p_last=NULL;
	CClipboard *p_clipboard=mp_clipboards;
	while (p_clipboard)
	{
		CClipboard *p_next=p_clipboard->mpNext;
		
		GridDims area;
		p_clipboard->GetArea(this,&area);

		if (area.GetW() > m_manager->GetParkNearBounds().GetW() ||
			area.GetW() > m_manager->GetParkNearBounds().GetL() ||
			area.GetL() > m_manager->GetParkNearBounds().GetW() ||
			area.GetL() > m_manager->GetParkNearBounds().GetL())
		{
			if (p_last)
			{
				p_last->mpNext=p_next;
			}	
			if (p_clipboard==mp_clipboards)
			{
				mp_clipboards=p_next;
			}
				
			delete p_clipboard;
			p_clipboard=p_next;
		}
		else
		{
			p_last=p_clipboard;
			p_clipboard=p_next;
		}	
	}	
	
	mp_current_clipboard=mp_clipboards;

	// Refresh since mp_current_clipboard has changed.
	ChangePieceInSet(0);
}

void CCursor::DestroyGeometry()
{
	if (mp_meta)
	{
		mp_meta->MarkUnlocked();
		m_manager->DestroyConcreteMeta(mp_meta, CParkManager::mDONT_DESTROY_PIECES_ABOVE);
		mp_meta = NULL;
	}
}

void CCursor::DestroyAnyClipboardCursors()
{
	CClipboard *p_clipboard=mp_clipboards;
	while (p_clipboard)
	{
		p_clipboard->DestroyMetas();
		p_clipboard=p_clipboard->mpNext;
	}	
}

void CCursor::Update(float shiftX, float shiftZ, int rotInc)
{
	/*
		==================================================
		ROTATION SECTION
		==================================================
	*/
	
	if (rotInc > 0)
	{
		m_hard_rot = Mth::ERot90((m_hard_rot + 1) & 3);
		m_target_rot += 90.0f;
		if (m_target_rot > 360.0f)
		{
			m_target_rot -= 360.0f;
			m_rot -= 360.0f;
		}
		m_cell_dims.SetWHL(m_cell_dims.GetL(), m_cell_dims.GetH(), m_cell_dims.GetW());
	}
	else if (rotInc < 0)
	{
		m_hard_rot = Mth::ERot90((m_hard_rot - 1) & 3);
		m_target_rot -= 90.0f;
		if (m_target_rot < 0.0f)
		{
			m_target_rot += 360.0f;
			m_rot += 360.0f;
		}
		m_cell_dims.SetWHL(m_cell_dims.GetL(), m_cell_dims.GetH(), m_cell_dims.GetW());
	}

	/*
		==================================================
		CELL POSITIONING SECTION
		==================================================
	*/
	
	const float near_zero = .0001f;
	//const float tolerance = .5f;
	
	float shift_x_squared = shiftX * shiftX;
	float shift_z_squared = shiftZ * shiftZ;

	bool actual_shift = (shift_x_squared > near_zero * near_zero || shift_z_squared > near_zero * near_zero);
	
	// can't shift to new cell if cursor currently in motion
	if (actual_shift && m_motion_pct <= 0.0f)
	{
		if (shift_z_squared > 5.8284f * shift_x_squared)
		{
			if (shiftZ < 0.0f)
				change_cell_pos(0, -1);
			else
				change_cell_pos(0, 1);
			m_motion_pct = 1.0f;
		}
		else if (shift_x_squared > 5.8284f * shift_z_squared)
		{
			if (shiftX < 0.0f)
				change_cell_pos(-1, 0);
			else
				change_cell_pos(1, 0);
			m_motion_pct = 1.0f;
		}
		else
		{
			if (shiftZ < 0.0f)
				change_cell_pos(0, -1);
			else
				change_cell_pos(0, 1);
	
			if (shiftX < 0.0f)
				change_cell_pos(-1, 0);
			else
				change_cell_pos(1, 0);
			m_motion_pct = 1.0f;
		}
	}
	else
	{
		change_cell_pos(0, 0);
	}
	
	#if 0
	printf("cell (%d,%d), shown (%.2f,%.2f)\n", 
		   m_cell_dims.GetX(), m_cell_dims.GetZ(), 
		   m_pos[X], m_pos[Z]);
	#endif
	
	/*
		==================================================
		ANIMATION SECTION
		==================================================
	*/

	//m_cell_dims.PrintContents();
	
	// derive m_target_pos from m_cell_dims
	
	Mth::Vector world_dims;
	m_target_pos = m_manager->GridCoordinatesToWorld(m_cell_dims, &world_dims);
	m_target_pos[X] += world_dims[X] / 2.0f;
	m_target_pos[Z] += world_dims[Z] / 2.0f;

	if (m_motion_pct > 0.0f)
	{
		m_pos[X] += (m_target_pos[X] - m_pos[X]) * MOTION_PCT_INC / m_motion_pct;
		m_pos[Z] += (m_target_pos[Z] - m_pos[Z]) * MOTION_PCT_INC / m_motion_pct;
		m_pos[Y] = m_target_pos[Y];
		m_motion_pct -= MOTION_PCT_INC;
		if (m_motion_pct < 0.0f) m_motion_pct = 0.0f;
	}
	else
	{
		m_pos = m_target_pos;
		m_motion_pct = 0.0f;
	}
	
	if (m_rot < m_target_rot)
	{
		m_rot += ROT_DEG_INC;
		if (m_rot > m_target_rot)
			m_rot = m_target_rot;
	}
	else if (m_rot > m_target_rot)
	{
		m_rot -= ROT_DEG_INC;
		if (m_rot < m_target_rot)
			m_rot = m_target_rot;
	}

	/*	
	Script::CStruct *p_params=new Script::CStruct;
	p_params->AddVector(CRCD(0x7f261953,"pos"),m_pos[X],m_pos[Y],m_pos[Z]);
	Script::RunScript(CRCD(0x9796fc9f,"ParkEdCursorPos"),p_params);
	delete p_params;
	*/
	
	#ifndef DONT_USE_CURSOR_META
	// Mick:  TT#3804
	// Adding the vector fixes Z-Fighting between cursor and identical piece
	if (m_mode==PASTE)
	{
		if (mp_current_clipboard)
		{
			//printf("m_clipboard_y=%d\n",m_clipboard_y);
			mp_current_clipboard->ShowMetas(m_pos + Mth::Vector(-1.5f,+1.5f,-1.5f), m_rot, m_clipboard_y);
		}	
	}
	else
	{
		DestroyAnyClipboardCursors();
		if (m_mode != RAIL_PLACEMENT)
		{
			Dbg_Assert(mp_meta);
			mp_meta->SetSoftRot(m_pos + Mth::Vector(-1.5f,+1.5f,-1.5f), m_rot);		  // Mick: Changed to move up, so water does not vanish on GC
		}	
	}	
	#endif
}



bool CCursor::AttemptStamp()
{
	Dbg_Assert(mp_meta);
	ParkEd("CCursor::AttemptStamp()");
	
	if (!m_manager->CanPlacePiecesIn(m_cell_dims))
	{
		return false;
	}
	
	// If we are trying to put down a restart point then we need to see if there are 
	// any free slots avaiable	
	CParkGenerator::RestartType restart_type = m_manager->IsRestartPiece(mp_meta->GetNameChecksum());
	if (restart_type != CParkGenerator::vEMPTY)
	{
		//printf("Attempting to stamp a restart piece type %d\n", restart_type);			
		if (! m_manager->GetGenerator()->FreeRestartExists(restart_type))
		{
			// no free slots for this restart point
			printf ("No free slots exist for this type of restart point\n");
			return false;
		}
	}
		
	#ifndef DONT_USE_CURSOR_META
	CAbstractMetaPiece *p_abstract = m_manager->GetAbstractMeta(mp_meta->GetNameChecksum());
	Dbg_MsgAssert(p_abstract, ("couldn't find abstract piece %s", Script::FindChecksumName(mp_meta->GetNameChecksum())));
	CConcreteMetaPiece *p_concrete = m_manager->CreateConcreteMeta(p_abstract);
	p_concrete->SetRot(m_hard_rot);
	m_manager->AddMetaPieceToPark(m_cell_dims, p_concrete);
	#endif
	
	m_manager->RebuildFloor();
	
	return true;
}

bool CCursor::AttemptRemove()
{
	ParkEd("CCursor::AttemptRemove()");
	
	GridDims area;
	if (m_mode==PASTE)
	{
		if (mp_current_clipboard)
		{
			mp_current_clipboard->GetArea(this,&area);
		}	
		else
		{
			return false;
		}	
	}
	else
	{
		area=m_cell_dims;
	}	
	
	return m_manager->DestroyMetasInArea(area);
}




/*
	A callback called by CParkManager::DestroyConcreteMeta()
*/
void CCursor::InformOfMetapieceDeletion(CConcreteMetaPiece *pMeta)
{
	int gap_half;
	GridDims meta_area = pMeta->GetArea();
	CGapManager::GapDescriptor *p_gap_desc = m_manager->GetGapDescriptor(meta_area, &gap_half);
	if (p_gap_desc && is_gap_descriptor_same_as_current(p_gap_desc))
		m_current_gap.numCompleteHalves = 0;
}




bool CCursor::AttemptGap()
{
	Spt::SingletonPtr<CParkEditor> p_editor;
	if (p_editor->IsParkFull())
		return false;
	
	CMapListTemp meta_list = m_manager->GetMetaPiecesAt(m_cell_dims);
	if (meta_list.IsEmpty()) 
	{
		// XXX
		Ryan("no gap for you!!!!\n");
		return false;
	}
	CConcreteMetaPiece *p_meta = meta_list.GetList()->GetConcreteMeta();
	// XXX
	Ryan("attempting gap at: ");
	m_cell_dims.PrintContents();
	
	int half;
	CGapManager::GapDescriptor *pDesc = m_manager->GetGapDescriptor(m_cell_dims, &half);
	if (pDesc)
	{
		// gap already attached at this location
		if (m_current_gap.numCompleteHalves == 1)
		{
			// user already working on gap, so fail
			return false;
		}
	}
	else if (p_meta)
	{
		// metapiece found at this location without gap attached
		if (m_current_gap.numCompleteHalves == 1)
			// remove gap half placed so far. It'll get rebuilt later.
			m_manager->RemoveGap(m_current_gap);
		else
			// make sure not 2
			m_current_gap.numCompleteHalves = 0;

		int active_gap_half = m_current_gap.numCompleteHalves;
		
		GridDims gap_area = p_meta->GetArea();
		
		// start gap 
		m_current_gap.loc[active_gap_half] = gap_area;
		m_current_gap.rot[active_gap_half] = 0;
		m_current_gap.leftExtent[active_gap_half] = 0;
		m_current_gap.rightExtent[active_gap_half] = 0;
		m_current_gap.score = 100;
		m_current_gap.numCompleteHalves = active_gap_half + 1;
		m_current_gap.tabIndex = -1;
	
		if (active_gap_half == 0)
		{
			// new gap, so give default name
			if (m_gap_suffix_counter > 1)
			{
				// K: Make sure the default names are different otherwise when registered in the 
				// CSkaterCareer it will not get added cos it will think it is the same gap
				// as the last.
				sprintf(m_current_gap.text, "gap%d",m_gap_suffix_counter);
			}
			else
			{
				strcpy(m_current_gap.text, "gap");
			}
			++m_gap_suffix_counter;
		}
		
		if (!m_manager->AddGap(m_current_gap))
		{
			m_current_gap.numCompleteHalves = 0;
			return false;
		}
		m_manager->RebuildFloor();
		if (m_current_gap.numCompleteHalves > 0)
			m_manager->HighlightMetasWithGaps(true, &m_current_gap);

		return true;
	}	
	
	return false;
}




bool CCursor::AttemptRemoveGap()
{
	if (m_current_gap.numCompleteHalves == 1)
	{
		// remove gap being worked on
		m_manager->RemoveGap(m_current_gap);
		m_current_gap.numCompleteHalves = 0;
		m_manager->RebuildFloor();
		return true;
	}

	int half;
	CGapManager::GapDescriptor *pDesc = m_manager->GetGapDescriptor(m_cell_dims, &half);
	if (pDesc)
	{
		// remove gap at this position
		m_manager->RemoveGap(*pDesc);
		m_manager->RebuildFloor();
		m_manager->HighlightMetasWithGaps(true, NULL);
		return true;
	}

	return false;
}




bool CCursor::AdjustGap(int rot, int leftChange, int rightChange)
{
	// grab gap at this location
	int half;
	CGapManager::GapDescriptor *p_desc = m_manager->GetGapDescriptor(m_cell_dims, &half);
	if (!p_desc)
	{
		// no gap, so leave
		return false;
	}

	if (!is_gap_descriptor_same_as_current(p_desc))
		// we are currently working on a gap and it's not the same as the one at this location, so fail
		return false;
	
	CGapManager::GapDescriptor desc_temp = *p_desc;
	bool needs_change = false;

	if (rot != 0)
	{
		desc_temp.rot[half] -= rot;
		desc_temp.leftExtent[half] = 0;
		desc_temp.rightExtent[half] = 0;
		needs_change = true;
	}

	if (leftChange != 0)
	{
		desc_temp.leftExtent[half] += leftChange;
		if (desc_temp.leftExtent[half] < 0)
			desc_temp.leftExtent[half] = 0;
		else if (desc_temp.leftExtent[half] > 6)
			desc_temp.leftExtent[half] = 6;
		needs_change = true;
	}
	
	if (rightChange != 0)
	{
		desc_temp.rightExtent[half] += rightChange;
		if (desc_temp.rightExtent[half] < 0)
			desc_temp.rightExtent[half] = 0;
		if (desc_temp.rightExtent[half] > 6)
			desc_temp.rightExtent[half] = 6;
		needs_change = true;
	}

	if (needs_change)
	{
		m_manager->RemoveGap(*p_desc);
		desc_temp.rot[half] &= 3;

		GridDims park_bounds = m_manager->GetParkNearBounds();
		
		// make sure gap boundaries are within map
		if (desc_temp.rot[half] == 0)
		{
			if (desc_temp.loc[half].GetZ() - desc_temp.rightExtent[half] < park_bounds.GetZ())
				desc_temp.rightExtent[half]--;
			if (desc_temp.loc[half].GetZ() + desc_temp.loc[half].GetL() + desc_temp.leftExtent[half] > park_bounds.GetZ() + park_bounds.GetL())
				desc_temp.leftExtent[half]--;
		}
		else if (desc_temp.rot[half] == 2)
		{
			if (desc_temp.loc[half].GetZ() - desc_temp.leftExtent[half] < park_bounds.GetZ())
				desc_temp.leftExtent[half]--;
			if (desc_temp.loc[half].GetZ() + desc_temp.loc[half].GetL() + desc_temp.rightExtent[half] > park_bounds.GetZ() + park_bounds.GetL())
				desc_temp.rightExtent[half]--;
		}
		else if (desc_temp.rot[half] == 1)
		{
			if (desc_temp.loc[half].GetX() - desc_temp.rightExtent[half] < park_bounds.GetX())
				desc_temp.rightExtent[half]--;
			if (desc_temp.loc[half].GetX() + desc_temp.loc[half].GetW() + desc_temp.leftExtent[half] > park_bounds.GetX() + park_bounds.GetW())
				desc_temp.leftExtent[half]--;
		}
		else if (desc_temp.rot[half] == 3)
		{
			if (desc_temp.loc[half].GetX() - desc_temp.leftExtent[half] < park_bounds.GetX())
				desc_temp.leftExtent[half]--;
			if (desc_temp.loc[half].GetX() + desc_temp.loc[half].GetW() + desc_temp.rightExtent[half] > park_bounds.GetX() + park_bounds.GetW())
				desc_temp.rightExtent[half]--;
		}

		// XXX
		for (half = 0; half < 2; half++)
			Ryan("gap details for half %d (%d,%d,%d),(%d,%d,%d) left=%d right=%d\n", half,
				 desc_temp.loc[half].GetX(), desc_temp.loc[half].GetY(), desc_temp.loc[half].GetZ(),
				 desc_temp.loc[half].GetW(), desc_temp.loc[half].GetH(), desc_temp.loc[half].GetL(),
				 desc_temp.leftExtent[half], desc_temp.rightExtent[half]);

		m_manager->AddGap(desc_temp);
		m_manager->RebuildFloor();
		
		if (m_current_gap.numCompleteHalves == 1)
		{
			m_current_gap = desc_temp;
		}
	}

	return true;
}




void CCursor::SetGapNameAndScore(const char *pGapName, int score)
{
	if (pGapName)
	{
		Dbg_MsgAssert(strlen(pGapName) <= 31, ("too many characters in gap name"));
		if (*pGapName == '\0')
			strcpy(m_current_gap.text, "x");
		else
			strcpy(m_current_gap.text, pGapName);
	}
	if (score >= 0)
	{
		m_current_gap.score = score;
	}
	Dbg_Assert(m_current_gap.tabIndex >= 0);
	CGapManager::sInstance()->SetGapInfo(m_current_gap.tabIndex, m_current_gap);
}

void CCursor::SetGapCancelFlags(Script::CStruct *p_cancelFlags)
{
	if (!p_cancelFlags)
	{
		return;
	}
	
	uint32 cancel_flags=0;
	Script::CComponent *p_comp=p_cancelFlags->GetNextComponent();
	while (p_comp)
	{
		Dbg_MsgAssert(p_comp->mNameChecksum==0 && p_comp->mType==ESYMBOLTYPE_NAME,("Gap cancel flag structure must contain only names"));
		cancel_flags |= Script::GetInteger(p_comp->mChecksum);
		p_comp=p_cancelFlags->GetNextComponent(p_comp);
	}	
	
	m_current_gap.mCancelFlags=cancel_flags;
	
	Dbg_Assert(m_current_gap.tabIndex >= 0);
	CGapManager::sInstance()->SetGapInfo(m_current_gap.tabIndex, m_current_gap);
}


const char *CCursor::GetGapName()
{
	return m_current_gap.text;
}


int CCursor::AttemptAreaSelect()
{
	Dbg_MsgAssert(m_mode==AREA_SELECT,("Called AttemptAreaSelect() with m_mode!=AREA_SELECT"));
	Dbg_MsgAssert(m_num_corners_selected<=2,("Bad m_num_corners_selected of %d",m_num_corners_selected));

	if (m_num_corners_selected==0)
	{
		mp_area_corners[0]=m_cell_dims;
		mp_area_corners[1]=m_cell_dims;
		m_num_corners_selected=1;
		ForceSelectionAreaHighlightRefresh();
	}
	else if (m_num_corners_selected==1)
	{
		GridDims area;
		GetAreaSelectDims(&area);
		if (area.GetW() <= CLIPBOARD_MAX_W && area.GetL() <= CLIPBOARD_MAX_L)
		{
			m_num_corners_selected=2;
		}	
	}
	
	return m_num_corners_selected;
}

void CCursor::ClearAreaSelection()
{
	GridDims area;
	GetAreaSelectDims(&area);
	
	m_manager->HighlightMetasInArea(area,false);
	
	m_num_corners_selected=0;
	m_initialised_highlight=false;
}

void CCursor::ContinueAreaSelection()
{
	m_num_corners_selected=1;
	// Need to force it to re-highlight the selection area next time around
	// because when quitting the paused menu something resets all the highlights.
	ForceSelectionAreaHighlightRefresh();
}

void CCursor::DeleteSelectedPieces()
{
	if (!m_num_corners_selected)
	{
		return;
	}
		
	GridDims dims;
	GetAreaSelectDims(&dims);

	// First true means do not destroy any flags or restarts
	// Second true means only destroy if completely within the area.
	m_manager->DestroyMetasInArea(dims, true, true);

	ContinueAreaSelection();
}

void CCursor::ResetSelectedHeights()
{
	if (!m_num_corners_selected)
	{
		return;
	}
		
	GridDims dims;
	GetAreaSelectDims(&dims);

	m_manager->ResetFloorHeights(dims);
	
	ContinueAreaSelection();
}
	
void CCursor::get_selected_area_coords(uint8 *p_x1, uint8 *p_z1, uint8 *p_x2, uint8 *p_z2)
{
	uint8 x1=mp_area_corners[0].GetX();
	uint8 z1=mp_area_corners[0].GetZ();
	uint8 x2=mp_area_corners[1].GetX();
	uint8 z2=mp_area_corners[1].GetZ();
	
	if (x2 < x1)
	{
		uint8 temp=x1;
		x1=x2;
		x2=temp;
	}

	if (z2 < z1)
	{
		uint8 temp=z1;
		z1=z2;
		z2=temp;
	}
	
	*p_x1=x1;
	*p_z1=z1;
	*p_x2=x2;
	*p_z2=z2;
}

void CCursor::GetAreaSelectDims(GridDims *p_dims)
{
	if (m_num_corners_selected==0)
	{
		p_dims->SetXYZ(0,0,0);
		p_dims->SetWHL(0,0,0);
		return;
	}	
	
	uint8 x1,z1,x2,z2;
	get_selected_area_coords(&x1,&z1,&x2,&z2);
	
	p_dims->SetXYZ(x1,0,z1);
	p_dims->SetWHL(x2-x1+1,1,z2-z1+1);
}

bool CCursor::DestroyMetasInCurrentArea()
{
	GridDims area;
	GetAreaSelectDims(&area);

	return CParkManager::sInstance()->DestroyMetasInArea(area);
}

void CCursor::DeleteOldestClipboard()
{
	// Find p_last, the last clipboard in the list.
	CClipboard *p_clip=mp_clipboards;
	CClipboard *p_last=NULL;
	CClipboard *p_next_to_last=NULL;
	while (p_clip)
	{
		p_next_to_last=p_last;
		p_last=p_clip;
		
		p_clip=p_clip->mpNext;
	}
	if (!p_last)
	{
		// Cannot do anything because there are no clipboards to delete
		return;
	}
		
	delete p_last;
	
	if (p_next_to_last)
	{
		p_next_to_last->mpNext=NULL;
	}
	else
	{
		mp_clipboards=NULL;
	}
}

bool CCursor::SelectionAreaTooBigToCopy()
{
	GridDims area;
	GetAreaSelectDims(&area);

	if (area.GetW() > CLIPBOARD_MAX_W || area.GetL() > CLIPBOARD_MAX_L)
	{
		return true;
	}
	
	Spt::SingletonPtr<CParkEditor> p_editor;
	if (!p_editor->RoomToCopyOrPaste(area.GetW(), area.GetL()))
	{
		return true;
	}	
	
	return false;
}
		
bool CCursor::CopySelectionToClipboard()
{
	if (SelectionAreaTooBigToCopy())
	{
		return false;
	}
		
	GridDims area;
	GetAreaSelectDims(&area);
	
	// TODO TODO TODO
	// If not enough rail points to copy the rails, check to see if any of the existing clipboards contain
	// rails. If so, delete enough of them to free up enough points. If still not enough points, bail out.
	// If enough, carry on with the next loop.
	Mth::Vector corner_pos;
	Mth::Vector area_dims;
	corner_pos=Ed::CParkManager::Instance()->GridCoordinatesToWorld(area,&area_dims);
	int num_rail_points_needed=Obj::GetRailEditor()->CountPointsInArea(corner_pos[X], corner_pos[Z], 
																	   corner_pos[X]+area_dims[X], corner_pos[Z]+area_dims[Z]);
	if (num_rail_points_needed > Obj::GetRailEditor()->GetNumFreePoints())
	{
		return false;
	}	



	if (GetNumClipboards()==MAX_CLIPBOARDS)
	{
		// All the CClipboard's in the pool have already been allocated.
		
		// Find the oldest clipboard and delete it.
		DeleteOldestClipboard();
		
		// That will have freed up space for one more CClipboard
		// so the new CClipboard below should not assert.
		// If it does then something strange is going on, like MAX_CLIPBOARDS is 0 or something.
	}
		
	CClipboard *p_clipboard=new CClipboard;

	
	
	// CopySelectionToClipboard will use up CClipboardEntry's, of which there are only
	// a certain number allocated in a pool.
	// So CopySelectionToClipboard may fail, even though the CClipboard pool may not be full.
	
	// Keep deleting old clipboards until CopySelectionToClipboard succeeds.
	// Note that the above newly allocated p_clipboard has not been put in the list
	// yet so it will not get deleted by accident here.
	while (!p_clipboard->CopySelectionToClipboard(area))
	{
		if (!mp_clipboards)
		{
			// Run out of clipboards to delete! If this happens then CopySelectionToClipboard
			// must be failing because the total number of pieces the user wants to copy 
			// exceeds the total number of CClipboardEntry's available in the pool, which
			// is defined by MAX_CLIPBOARD_METAS in clipboard.h
			break;
		}			
		DeleteOldestClipboard();
	}	
	
	if (p_clipboard->IsEmpty())
	{
		// Nothing got copied into the clipboard, which means either the user tried to copy a 
		// blank area or the above attempts at copying failed due to lack of memory.
		
		delete p_clipboard;
		p_clipboard=NULL;
		
		return false;
	}	
	
	// Add the new clipboard to the list.
	p_clipboard->mpNext=mp_clipboards;
	mp_clipboards=p_clipboard;
	return true;
}	

int CCursor::GetNumClipboards()
{
	int n=0;
	CClipboard *p_clipboard=mp_clipboards;
	while (p_clipboard)
	{
		++n;
		p_clipboard=p_clipboard->mpNext;
	}
	return n;	
}

void CCursor::PasteCurrentClipboard()
{
	if (mp_current_clipboard)
	{
		mp_current_clipboard->Paste(this);
	}
}

CClipboard *CCursor::get_clipboard(int index)
{
	CClipboard *p_clipboard=mp_clipboards;
	
	for (int i=0; i<index; ++i)
	{
		if (p_clipboard)
		{
			p_clipboard=p_clipboard->mpNext;
		}
		else
		{
			break;
		}	
	}
	
	return p_clipboard;
}

void CCursor::set_current_clipboard(int index)
{
	DestroyAnyClipboardCursors();		
	mp_current_clipboard=get_clipboard(index);
	
	/*
	if (mp_current_clipboard)
	{
		m_cell_dims.SetWHL(mp_current_clipboard->GetWidth(),1,mp_current_clipboard->GetLength());
		if (m_hard_rot & 1)
		{
			m_cell_dims.SetWHL(m_cell_dims.GetL(), m_cell_dims.GetH(), m_cell_dims.GetW());
		}
	}	
	*/
}

void CCursor::remove_clipboard(int index)
{
	CClipboard *p_remove=get_clipboard(index);
	if (!p_remove)
	{
		return;
	}
	
	CClipboard *p_search=mp_clipboards;
	CClipboard *p_last=NULL;
	while (p_search)
	{
		if (p_search==p_remove)
		{
			if (p_last)
			{
				p_last->mpNext=p_remove->mpNext;
			}
			else
			{
				mp_clipboards=p_remove->mpNext;
			}
			delete p_remove;
			break;
		}
				
		p_last=p_search;
		p_search=p_search->mpNext;
	}	
}

void CCursor::RefreshSelectionArea()
{
	switch (m_num_corners_selected)
	{
	case 0:
		break;
	case 1:
		if (!m_initialised_highlight)
		{
			GridDims area;
			GetAreaSelectDims(&area);
			
			m_manager->HighlightMetasInArea(area,true);
			
			m_initialised_highlight=true;
		}	
		else
		{
			if (m_cell_dims.GetX() != mp_area_corners[1].GetX() ||
				m_cell_dims.GetZ() != mp_area_corners[1].GetZ())
			{
				GridDims area;
				GetAreaSelectDims(&area);

				m_manager->HighlightMetasInArea(area,false);

				mp_area_corners[1]=m_cell_dims;

				GetAreaSelectDims(&area);

				m_manager->HighlightMetasInArea(area,true);
			}
		}
		break;
	default:
		break;		
	}
}

// Find all the pieces intersecting the cursor
// and turn the highlight effect on or off
// (In THPS3, the highlight was a wireframe.  Here I think we are going to try coloring it red)
void CCursor::HighlightIntersectingMetas(bool on)
{
	if (m_mode==PASTE)
	{
		if (mp_current_clipboard)
		{
			mp_current_clipboard->HighlightIntersectingMetas(this,on);
			return;
		}	
	}
	
	// Get the list of (concrete) meta pieces that intersect the
	// dimension of the cursor's m_cell_dims
	CMapListTemp metas_at_pos = m_manager->GetMetaPiecesAt(m_cell_dims);
	if (metas_at_pos.IsEmpty())
	{
		// Nothing intersecting, so do nothing and return
		return;
	}

	// Iterate over the list of metapieces we just found, and highlight each one
	// (Whcih will actually highlight the pieces
	CMapListNode *p_entry = metas_at_pos.GetList();
	while(p_entry)
	{
		p_entry->GetConcreteMeta()->Highlight(on);		
		p_entry = p_entry->GetNext();
	}
}

void CCursor::SetVisibility(bool visible)
{
	//Dbg_Assert(mp_meta);
	if (mp_meta)
	{
		mp_meta->SetVisibility(visible);
	}	
	
	if (!visible)
	{
		if (mp_current_clipboard)
		{
			mp_current_clipboard->DestroyMetas();
		}
	}		
	
	m_initialised_highlight=false;
}



void CCursor::ChangePieceInSet(int dir)
{
	CParkManager::CPieceSet &piece_set = m_manager->GetPieceSet(m_selected_set, &m_menu_set_number);
	int selected_entry = piece_set.mSelectedEntry;
	selected_entry += dir;
	if (selected_entry < 0)
		selected_entry = piece_set.mTotalEntries - 1;
	else if (selected_entry >= piece_set.mTotalEntries)
		selected_entry = 0;
	piece_set.mSelectedEntry = selected_entry;

	if (piece_set.mIsClipboardSet)
	{
		set_current_clipboard(piece_set.mSelectedEntry);
	}
	else
	{
		set_source_meta(piece_set.mEntryTab[selected_entry].mNameCrc);
	}	
}

void CCursor::ChangeSet(int dir)
{
	for (int count = m_manager->GetTotalNumPieceSets(); count > 0; count--)
	{
		m_selected_set += dir;
		if (m_selected_set < 0)
			m_selected_set = m_manager->GetTotalNumPieceSets() - 1;
		if (m_selected_set >= m_manager->GetTotalNumPieceSets())
			m_selected_set = 0;
		//printf("set is: %s\n", m_palette_set[m_selected_set].mpName);
		if (m_manager->GetPieceSet(m_selected_set, &m_menu_set_number).mVisible) break;
	}
	CParkManager::CPieceSet &piece_set = m_manager->GetPieceSet(m_selected_set, &m_menu_set_number);
	if (piece_set.mIsClipboardSet)
	{
		set_current_clipboard(piece_set.mSelectedEntry);
		// Destroy the regular cursor meta
		DestroyGeometry();
		
		
		change_mode(PASTE);
	}
	else
	{
		set_source_meta(piece_set.mEntryTab[piece_set.mSelectedEntry].mNameCrc);
	}	
}

void CCursor::SwitchMenuPieceToMostRecentClipboard()
{
	if (!GetNumClipboards())
	{
		return;
	}
		
	int num_sets=m_manager->GetTotalNumPieceSets();
	CParkManager::CPieceSet *p_piece_set=NULL;
	
	for (int i=0; i<num_sets; ++i)
	{
		CParkManager::CPieceSet &piece_set = m_manager->GetPieceSet(i, &m_menu_set_number);
		if (piece_set.mIsClipboardSet)
		{
			m_selected_set=i;
			p_piece_set=&piece_set;
			break;
		}
	}
	Dbg_MsgAssert(p_piece_set,("Clipboard piece set not found?"));
	
	p_piece_set->mSelectedEntry=0;
	set_current_clipboard(p_piece_set->mSelectedEntry);
	// Destroy the regular cursor meta
	DestroyGeometry();
	change_mode(PASTE);
	
	// Fix to TT14349: Crash bug if copying tree pieces at the edge of the largest map by highlighting starting at the 
	// lower right corner of the set of trees.
	// Caused by the cursor being out of bounds for a frame, hence causing it to read outside the map array.
	ForceInBounds();
}

bool CCursor::ChangeFloorHeight(GridDims dims, int height, int dir, bool uniformHeight)
{
	//printf("CCursor::ChangeFloorHeight: ");
	
	/*
		Uneven floor:
			If going up, raise to height of topmost
			If going down, only lower ones on same level
	*/

	//printf("height=%d uniform_height=%d\n",height,uniformHeight);
	/*
		Now, 
	*/

	if (dims.GetW() > CLIPBOARD_MAX_W || dims.GetL() > CLIPBOARD_MAX_L)
	{
		return false;
	}
	
	//Spt::SingletonPtr<CParkEditor> p_editor;
	//if (p_editor->IsParkFull())
	//{
	//	return false;
	//}
		
	bool success = true;
	
		
	if (m_manager->SlideColumn(dims, dims.GetY(), height, (dir > 0), !uniformHeight))
	{
		//Ryan("Slide column, dir = %d, dims=", dir);
		//dims.PrintContents();
		
		change_cell_pos(0, 0);
		Spt::SingletonPtr<CParkEditor> p_editor;
		if (m_manager->RebuildFloor(p_editor->IsParkFull()))
		{
			m_manager->HighlightMetasWithGaps(false, NULL);
		}
		else
		{
			// out of memory, so reverse slide
			m_manager->UndoSlideColumn();
			m_manager->RebuildFloor();
			success = false;
		}
	}
	else
		success = false;

	return success;
}

bool CCursor::ChangeFloorHeight(int dir)
{
	GridDims dims;
	switch (m_mode)
	{
	case AREA_SELECT:
		if (m_num_corners_selected)
		{
			GetAreaSelectDims(&dims);
		}
		else
		{
			dims=m_cell_dims;
		}	
		break;
	case PASTE:
		if (mp_current_clipboard)
		{
			mp_current_clipboard->GetArea(this,&dims);
		}
		break;
	default:
		dims=m_cell_dims;
		break;
	}	

	// If the dims exceeds the area of the park, then do not raise or lower.
	// This can happen if the park is been resized to its smallest possible size and then a big pool
	// piece selected. If the raise was allowed to go ahead, it would result in the fence raising too. (TT11825)
	GridDims park_bounds = CParkManager::sInstance()->GetParkNearBounds();	
	if (dims.GetW() > park_bounds.GetW() || dims.GetL() > park_bounds.GetL())
	{
		return false;
	}	
	
	
	bool uniform_height;
	int height = m_manager->GetFloorHeight(dims, &uniform_height);

	// If pasting, destroy the clipboard metas, otherwise the dry run that ChangeFloorHeight
	// does to determine whether it is safe to raise or lower will incorrectly think it
	// is safe when it is not. I think the function generate_floor_pieces_from_height_map
	// gets confused by the presence of the clipboard metas. (TT7495 & TT6152)
	if (m_mode==PASTE)
	{
		if (mp_current_clipboard)
		{
			mp_current_clipboard->DestroyMetas();
		}
	}		
	bool ret_val=ChangeFloorHeight(dims,height,dir,uniform_height);
	
	if (m_mode==AREA_SELECT)
	{
		ForceSelectionAreaHighlightRefresh();
	}
	return ret_val;	
}




bool CCursor::IsSittingOnGap()
{
	int half;
	CGapManager::GapDescriptor *pDesc = m_manager->GetGapDescriptor(m_cell_dims, &half);
	if (pDesc)
		return true;
	return false;
}




CCursor *CCursor::sInstance(bool assert)
{
	Dbg_Assert(!assert || sp_instance);
	return sp_instance;
}




void CCursor::ForceInBounds()
{
	if (m_cell_dims[X] < m_manager->GetParkNearBounds().GetX())
	{
		m_cell_dims[X] = m_manager->GetParkNearBounds().GetX();
	}
	if (m_cell_dims[X] + m_cell_dims[W] > m_manager->GetParkNearBounds().GetX() + m_manager->GetParkNearBounds().GetW())
	{
		m_cell_dims[X] = m_manager->GetParkNearBounds().GetX() + m_manager->GetParkNearBounds().GetW() - m_cell_dims[W];
	}
	if (m_cell_dims[Z] < m_manager->GetParkNearBounds().GetZ())
	{
		m_cell_dims[Z] = m_manager->GetParkNearBounds().GetZ();
	}
	if (m_cell_dims[Z] + m_cell_dims[L] > m_manager->GetParkNearBounds().GetZ() + m_manager->GetParkNearBounds().GetL())
	{
		m_cell_dims[Z] = m_manager->GetParkNearBounds().GetZ() + m_manager->GetParkNearBounds().GetL() - m_cell_dims[L];
	}
	
	if (m_mode==PASTE)
	{
		if (mp_current_clipboard)
		{
			GridDims area;
			mp_current_clipboard->GetArea(this,&area);
						
			int dx=0;
			int park_x_left=m_manager->GetParkNearBounds().GetX();
			int park_x_right=m_manager->GetParkNearBounds().GetX()+m_manager->GetParkNearBounds().GetW()-1;
			sint8 x_left=area.GetX();
			if (x_left < park_x_left)
			{
				dx=park_x_left-x_left;
			}
			sint8 x_right=area.GetX();
			x_right+=area.GetW()-1;	
			if (x_right > park_x_right)
			{
				dx=park_x_right-x_right;
			}	
			m_cell_dims[X]+=dx;
			
			int dz=0;
			int park_z_top=m_manager->GetParkNearBounds().GetZ();
			int park_z_bottom=m_manager->GetParkNearBounds().GetZ()+m_manager->GetParkNearBounds().GetL()-1;
			
			sint8 z_top=area.GetZ();
			if (z_top < park_z_top)
			{
				dz=park_z_top-z_top;
			}
			sint8 z_bottom=area.GetZ();
			z_bottom+=area.GetL()-1;	
			if (z_bottom > park_z_bottom)
			{
				dz=park_z_bottom-z_bottom;
			}	
			
			m_cell_dims[Z]+=dz;
		}	
	}
	
}




void CCursor::change_cell_pos(int incX, int incZ)
{
	//printf("change_cell_pos:\n");
	//printf("Before:");
	//m_cell_dims.PrintContents();
	
	ForceInBounds();
	GridDims old_pos = m_cell_dims;
	
	m_cell_dims[X] += incX;
	m_cell_dims[Z] += incZ;

	ForceInBounds();

	if (m_mode==PASTE)
	{
		if (mp_current_clipboard)
		{
			m_clipboard_y=mp_current_clipboard->FindMaxFloorHeight(this);
			m_cell_dims[Y]=m_clipboard_y;
		}	
		else
		{
			m_cell_dims[Y]=0;
		}	
	}
	else
	{
		int current_floor_height = m_manager->GetFloorHeight(m_cell_dims);
		int floor_height_old_pos = m_manager->GetFloorHeight(old_pos);
		m_cell_dims[Y] = (current_floor_height > floor_height_old_pos) ? current_floor_height : floor_height_old_pos;
	}
		
	//Ryan("cursor at %d,%d\n", m_cell_dims[X], m_cell_dims[Z]);	 

	//printf("After:");
	//m_cell_dims.PrintContents();
	
	#if 0
	printf("park bounds: %d,%d,%d,%d, cell_dims: %d,%d\n", 
		   m_manager->GetParkNearBounds().GetX(), m_manager->GetParkNearBounds().GetZ(),
		   m_manager->GetParkNearBounds().GetW(), m_manager->GetParkNearBounds().GetL(),
		   m_cell_dims[W], m_cell_dims[L]);
	#endif
	
	if (m_mode == GAP || m_mode == GAP_ADJUST)
	{
		ECursorMode new_mode = GAP;
		
		// see if cursor is over a metapiece with attached gap
		CMapListTemp meta_list = m_manager->GetMetaPiecesAt(m_cell_dims);
		if (!meta_list.IsEmpty())
		{
			int half;
			GridDims area = meta_list.GetList()->GetConcreteMeta()->GetArea();
			CGapManager::GapDescriptor *p_desc = m_manager->GetGapDescriptor(area, &half);
			if (p_desc)
			{
				if (m_current_gap.numCompleteHalves != 1)
				{
					m_current_gap = *p_desc;
				}
				new_mode = GAP_ADJUST;
			}
		}
		
		change_mode(new_mode);
		
		if (m_current_gap.numCompleteHalves > 0)
			m_manager->HighlightMetasWithGaps(true, &m_current_gap);
		else
			m_manager->HighlightMetasWithGaps(true, NULL);
	}
}
	



void CCursor::set_source_meta(uint32 nameChecksum)
{
	#ifndef DONT_USE_CURSOR_META
	if (mp_meta)
	{
		mp_meta->MarkUnlocked();
		m_manager->DestroyConcreteMeta(mp_meta, CParkManager::mDONT_DESTROY_PIECES_ABOVE);
		mp_meta = NULL;
	}
	
	CAbstractMetaPiece *p_source_meta = m_manager->GetAbstractMeta(nameChecksum);
	Dbg_MsgAssert(p_source_meta, ("couldn't find metapiece named %s", Script::FindChecksumName(nameChecksum)));
	
	if (p_source_meta->IsRailPlacement())
	{
		m_cell_dims.SetWHL(1, 1, 1);	
	}
	else
	{
		mp_meta = m_manager->CreateConcreteMeta(p_source_meta, true);
		mp_meta->MarkLocked();
		m_cell_dims.SetWHL(mp_meta->GetArea().GetW(), mp_meta->GetArea().GetH(), mp_meta->GetArea().GetL());
		if (m_hard_rot & 1)
			m_cell_dims.SetWHL(m_cell_dims.GetL(), m_cell_dims.GetH(), m_cell_dims.GetW());
	}
	#else
	m_cell_dims.SetWHL(1, 1, 1);	
	#endif

	ClearAreaSelection();

	if (p_source_meta->IsGapPlacement())
	{
		change_mode(GAP);
		//m_manager->HighlightMetasWithGaps(true, NULL);
	}
	else if (p_source_meta->IsAreaSelection())
	{
		change_mode(AREA_SELECT);
		m_manager->HighlightMetasWithGaps(false, NULL);
	}
	else if (p_source_meta->IsRailPlacement())
	{
		change_mode(RAIL_PLACEMENT);
		m_manager->HighlightMetasWithGaps(false, NULL);
	}
	else
	{
		change_mode(REGULAR);
		m_manager->HighlightMetasWithGaps(false, NULL);
	}
		
	//m_manager->AddMetaPieceToPark(GridDims(0, 8, 0), mp_meta);

	//printf("selected set %d, entry %d/%d\n", m_selected_set, m_palette_set[m_selected_set].mSelectedEntry, m_palette_set[m_selected_set].mTotalEntries);
}

void CCursor::change_mode(ECursorMode newMode)
{
	if (m_mode != newMode)
	{
		m_mode = newMode;
		RefreshHelperText();
	}
}




void CCursor::RefreshHelperText()
{
	Script::CStruct params;
	if (m_mode == GAP_ADJUST)
		params.AddChecksum(CRCD(0x6835b854,"mode"), CRCD(0xe75a3a17,"gap_adjust"));
	else if (m_mode == GAP)
		params.AddChecksum(CRCD(0x6835b854,"mode"), CRCD(0x780a5cf3,"gap_regular"));
	else if (m_mode == RAIL_PLACEMENT)
		params.AddChecksum(CRCD(0x6835b854,"mode"), CRCD(0xffd81c08,"rail_placement"));
	else
		params.AddChecksum(CRCD(0x6835b854,"mode"), CRCD(0xb58efc2b,"regular"));
	Script::RunScript("parked_set_helper_text_mode", &params);
}




bool CCursor::is_gap_descriptor_same_as_current(CGapManager::GapDescriptor *pDesc)
{
	if (m_current_gap.numCompleteHalves == 1 && 
		(pDesc->loc[0].GetX() != m_current_gap.loc[0].GetX() ||
		 pDesc->loc[0].GetY() != m_current_gap.loc[0].GetY() ||
		 pDesc->loc[0].GetZ() != m_current_gap.loc[0].GetZ()))
	{
		// we are currently working on a gap and it's not the same as the one at this location, so fail
		return false;
	}

	return true;
}
	



CUpperMenuManager::CUpperMenuManager()
{
	//Rulon: Commented this out to prevent menu set items from being added twice
	//Enable();

	mp_current_set = NULL;
	m_current_entry_in_set = 0;
	m_current_set_index = 0;
}




CUpperMenuManager::~CUpperMenuManager()
{
	Disable();
}




void CUpperMenuManager::SetSourceMeta(uint32 nameChecksum)
{
	//Script::RunScript("parked_make_piece_menu");
	
	CAbstractMetaPiece *p_source_meta = m_manager->GetAbstractMeta(nameChecksum);
	Dbg_MsgAssert(p_source_meta, ("couldn't find metapiece named %s", Script::FindChecksumName(nameChecksum)));
	
	Script::CStruct params;
	params.AddChecksum("metapiece_id", nameChecksum);
	if (p_source_meta->IsSingular())
	{
		params.AddChecksum("sector", nameChecksum);
		Script::RunScript("parked_make_piece_menu_item", &params);			
	}
	else
	{
		Script::CArray sectors_array;
		p_source_meta->BuildElement3dSectorsArray(&sectors_array);			
		params.AddArray("sectors", &sectors_array);
		Script::RunScript("parked_make_piece_menu_item", &params);			
		Script::CleanUpArray(&sectors_array);
	}
}




void CUpperMenuManager::SetPieceSet(CParkManager::CPieceSet *pSet, int set_number, int menuSetNumber)
{
	ParkEd("CUpperMenuManager::SetPieceSet(pSet = %p, set_number = %d)", pSet, set_number);
	
	Dbg_Assert(pSet);
	
	bool is_clipboard_set=false;
	int num_clipboards=0;
	if (pSet->mIsClipboardSet)
	{
		is_clipboard_set=true;
		num_clipboards=CCursor::sInstance()->GetNumClipboards();
		CCursor::sInstance()->ClearAreaSelection();
	}

	Obj::CTracker* p_tracker = Obj::CTracker::Instance();
	if (mp_current_set)
	{
		//uint32 name_crc = pSet->mEntryTab[m_current_entry_in_set].mNameCrc;
		uint32 name_crc = mp_current_set->mEntryTab[m_current_entry_in_set].mNameCrc;
		
		ParkEd("unfocusing item in piece menu %s", Script::FindChecksumName(name_crc));
		if (p_tracker->GetObject(name_crc))
			p_tracker->LaunchEvent(Obj::CEvent::TYPE_UNFOCUS, name_crc);
	}
	
	ParkEd("current set is %p\n", mp_current_set);
	if (pSet != mp_current_set)
	{
		//printf("Script::RunScript(\"parked_make_piece_menu\");");
		Script::RunScript("parked_make_piece_menu");
		
		for (int i = 0; i < pSet->mTotalEntries; i++)
		{
			uint32 name_checksum = pSet->mEntryTab[i].mNameCrc;
			
			if (is_clipboard_set)
			{
				Script::CStruct params;
				params.AddChecksum("metapiece_id", name_checksum);
				if (i<num_clipboards)
				{
					params.AddInteger("ClipBoardIndex",i+1);
				}
				else
				{
					params.AddChecksum(NONAME,CRCD(0x66da03b3,"EmptyClipBoard"));
				}	
				Script::RunScript("parked_make_piece_menu_item", &params);
			}
			else
			{
				CAbstractMetaPiece *p_source_meta = m_manager->GetAbstractMeta(name_checksum);
				Dbg_MsgAssert(p_source_meta, ("couldn't find metapiece named %s", Script::FindChecksumName(name_checksum)));
				
				Script::CStruct params;
				params.AddChecksum("metapiece_id", name_checksum);
				if (p_source_meta->IsSingular())
				{
					params.AddChecksum("sector", name_checksum);
					Script::RunScript("parked_make_piece_menu_item", &params);			
				}
				else
				{
					Script::CArray sectors_array;
					p_source_meta->BuildElement3dSectorsArray(&sectors_array);			
					params.AddArray("sectors", &sectors_array);
					Script::RunScript("parked_make_piece_menu_item", &params);			
					Script::CleanUpArray(&sectors_array);
				}
			}	
		}

		Script::RunScript("parked_make_set_menu");
		
		Script::CStruct params;
		params.AddInteger("set_number", menuSetNumber);
		params.AddInteger("last_set_number", m_current_set_index);
		Script::RunScript("parked_lock_piece_and_set_menus", &params);
		
		mp_current_set = pSet;
		m_current_set_index = menuSetNumber;
	}

	// m_current_entry_in_set
	
	//Dbg_Assert(m_current_entry_in_set >= 0 && m_current_entry_in_set < 
	
	ParkEd("focusing item in piece menu %s", Script::FindChecksumName(pSet->mEntryTab[pSet->mSelectedEntry].mNameCrc));
	if (p_tracker->GetObject(pSet->mEntryTab[pSet->mSelectedEntry].mNameCrc))
		p_tracker->LaunchEvent(Obj::CEvent::TYPE_FOCUS, pSet->mEntryTab[pSet->mSelectedEntry].mNameCrc);
	m_current_entry_in_set = pSet->mSelectedEntry;

	/* Make the slider reflect the selection window */
	
	int first_in_window = m_current_entry_in_set - 3;
	int last_in_window = m_current_entry_in_set + 3;
	if (pSet->mTotalEntries <= 7)
	{
		first_in_window = 0;
		last_in_window = pSet->mTotalEntries - 1;
	}
	else if (first_in_window < 0)
	{
		last_in_window += -first_in_window;
		first_in_window = 0;
	}
	else if (last_in_window >= pSet->mTotalEntries)
	{
		first_in_window += pSet->mTotalEntries - last_in_window - 1;
		last_in_window = pSet->mTotalEntries - 1;
	}
	
	Front::CScreenElementManager* p_element_manager = Front::CScreenElementManager::Instance();
	Front::CScreenElementPtr p_slider = p_element_manager->GetElement(Script::GenerateCRC("piece_slider_orange"), Front::CScreenElementManager::ASSERT);
	float slider_x, slider_y;
	p_slider->GetLocalPos(&slider_x, &slider_y);
	float slider_start = 375.0f * first_in_window / pSet->mTotalEntries;
	float slider_end = 375.0f * (last_in_window + 1) / pSet->mTotalEntries;
	p_slider->SetPos(slider_start, slider_y, Front::CScreenElement::FORCE_INSTANT);
	p_slider->SetScale((slider_end - slider_start) / 4.0f, 1.0f, false, Front::CScreenElement::FORCE_INSTANT);

	Front::CScreenElementPtr p_name_text = p_element_manager->GetElement(Script::GenerateCRC("piece_menu_name_text"), Front::CScreenElementManager::ASSERT);
	if (pSet->mEntryTab[m_current_entry_in_set].mpName)
		((Front::CTextElement *) p_name_text.Convert())->SetText(pSet->mEntryTab[m_current_entry_in_set].mpName);
	else
		((Front::CTextElement *) p_name_text.Convert())->SetText("---");
}




void CUpperMenuManager::Enable()
{
	ParkEd("CUpperMenuManager::Enable()");
	//printf("CUpperMenuManager::Enable()");
	
	Script::RunScript("parked_make_piece_menu");			
	Script::RunScript("parked_make_set_menu");			
	
	for (int i = 0; i < m_manager->GetTotalNumPieceSets(); i++)
	{
		CParkManager::CPieceSet &set = m_manager->GetPieceSet(i);
		if (set.mVisible)
		{
			Script::CStruct params;
			params.AddString("set_name", set.mpName);
			Script::RunScript("parked_make_set_menu_item", &params);
		}
	}

	// will force menu to be rebuilt in SetPieceSet()
	mp_current_set = NULL;
}




void CUpperMenuManager::Disable()
{
	ParkEd("CUpperMenuManager::Disable()");
	
	Script::RunScript("parked_destroy_piece_menu");
	Script::RunScript("parked_destroy_set_menu");
	mp_current_set= NULL;
}

bool ScriptSetParkEditorTimeOfDay(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 tod_script=0;
	pParams->GetChecksum(CRCD(0x4c72ed98,"tod_script"),&tod_script);
	Ed::CParkEditor::Instance()->SetTimeOfDayScript(tod_script);
	return true;
}

bool ScriptGetParkEditorTimeOfDayScript(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 tod_script=Ed::CParkEditor::Instance()->GetTimeOfDayScript();
	if (tod_script)
	{
		pScript->GetParams()->AddChecksum(CRCD(0x4c72ed98,"tod_script"),tod_script);
		return true;
	}	
	return false;
}

// Added so that when choosing to create a goal, the node array can be initialised, otherwise the
// cursor will not be able to detect kill polys (unless a test play had been done)
bool ScriptRebuildParkNodeArray(Script::CStruct *pParams, Script::CScript *pScript)
{
	Ed::CParkManager::sInstance()->RebuildNodeArray();	
	return true;
}
	
bool ScriptFreeUpMemoryForPlayingPark(Script::CStruct *pParams, Script::CScript *pScript)
{
	// The following will free up everything on the park editor heap, then delete the
	// heap itself.

	Ed::CParkEditor::Instance()->CreatePlayModeGapManager();
	
	Ed::CParkEditor::Instance()->DeleteCursor();
	// Note: Only destroying pieces, not the cloned sectors, otherwise the whole park
	// will disappear from beneath the skater.
	Ed::CParkManager::sInstance()->Destroy(Ed::CParkGenerator::DESTROY_ONLY_PIECES);
	
	Ed::CParkEditor::Instance()->SwitchToPlayModeGapManager();
	
	return true;
}

bool ScriptCalibrateMemoryGauge(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Old
	return true;
}

bool ScriptGetParkEditorCursorPos(Script::CStruct *pParams, Script::CScript *pScript)
{
	CParkEditor* p_editor = CParkEditor::Instance();
	Mth::Vector pos=p_editor->GetCursorPos();
	pScript->GetParams()->AddVector(CRCD(0x7f261953,"pos"),pos[X],pos[Y],pos[Z]);
	return true;
}

bool ScriptSwitchToParkEditorCamera(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Create the park editor camera if it does not exist, then switch to it.
	Obj::CCompositeObject * p_obj = (Obj::CCompositeObject *) Obj::CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0x64716bee,"parked_cam"));
	if (!p_obj)
	{
		Script::CStruct * p_cam_params = new Script::CStruct;
		p_cam_params->AddChecksum("name",CRCD(0x64716bee,"parked_cam"));
		
		Obj::CCompositeObjectManager::Instance()->CreateCompositeObjectFromNode(
											Script::GetArray("parkedcam_composite_structure"),p_cam_params);
		
		delete	p_cam_params;
	}
	
	CParkEditor::Instance()->SetAppropriateCamera();	

	return true;
}
	
/*
	Parameters:
	
	state
	command
	
	States:
	-edit
		Command:
		-initialize
		-clear whole park
		-regenerate from compressed map
		-build floor
	-test_play
	-off
	
	Will
*/
bool ScriptSetParkEditorState(Script::CStruct *pParams, Script::CScript *pScript)
{
	ParkEd("CCursor::ScriptSetParkEditorState()");
	#if DEBUG_THIS_DAMN_THING
	Script::PrintContents(pParams, 4);
	#endif
	
	CParkEditor* p_editor = CParkEditor::Instance();
	
	CParkEditor::EEditorState new_state = CParkEditor::vKEEP_SAME_STATE;
	pParams->GetChecksum("state", (uint32 *) &new_state);
	Dbg_MsgAssert(new_state == CParkEditor::vEDITING ||
				  new_state == CParkEditor::vINACTIVE ||
				  new_state == CParkEditor::vREGULAR_PLAY ||
				  new_state == CParkEditor::vTEST_PLAY, 
				  ("invalid park editor state"));

	CParkEditor::EEditorCommand command = CParkEditor::vNO_COMMAND;
	pParams->GetChecksum("command", (uint32 *) &command);
	Dbg_MsgAssert(command == CParkEditor::vINITIALIZE ||
				  command == CParkEditor::vCLEAR ||
				  command == CParkEditor::vREGENERATE_FROM_MAP ||
				  command == CParkEditor::vBUILD_FLOOR ||
				  command == CParkEditor::vNO_COMMAND,
				  ("invalid park editor command"));
	
	if (new_state == CParkEditor::vINACTIVE)
	{
		p_editor->SetState(new_state);					
		p_editor->Cleanup();
		p_editor->SetPaused(false);
	}
	else
	{
		if (command == CParkEditor::vINITIALIZE)
		{
			p_editor->Initialize((new_state == CParkEditor::vEDITING));
		}
		else
		{
			if (command == CParkEditor::vCLEAR)
				p_editor->Rebuild(false, true, (new_state == CParkEditor::vEDITING));
			else if (command == CParkEditor::vREGENERATE_FROM_MAP)
				p_editor->Rebuild(false, false, (new_state == CParkEditor::vEDITING));
			else if (command == CParkEditor::vBUILD_FLOOR)
				p_editor->Rebuild(true, false, (new_state == CParkEditor::vEDITING));			
		}
		p_editor->SetState(new_state);					
	}

	return true;
}




bool ScriptSetParkEditorPauseMode(Script::CStruct *pParams, Script::CScript *pScript)
{
	ParkEd("CCursor::ScriptSetParkEditorPauseMode()");
	#if DEBUG_THIS_DAMN_THING
	Script::PrintContents(pParams, 4);
	#endif
	
	CParkEditor* p_editor = CParkEditor::Instance();
	
	if (pParams->ContainsFlag("pause"))
		p_editor->SetPaused(true);
	else if (pParams->ContainsFlag("unpause"))
		p_editor->SetPaused(false);
	
	return true;
}




bool ScriptCustomParkMode(Script::CStruct *pParams, Script::CScript *pScript)
{
	CParkEditor* p_editor = CParkEditor::Instance();
	if (pParams->ContainsFlag("editing"))
	{
		return p_editor->EditingCustomPark();
	}
	else if (pParams->ContainsFlag("testing"))
	{
		return p_editor->TestingCustomPark();
	}
	else if (pParams->ContainsFlag("just_using"))
	{
		return p_editor->UsingCustomPark();
	}
	
	Dbg_MsgAssert(0, ("need a parameter -- 'editing' or 'just_using'"));
	return false;
}

bool ScriptSetParkEditorMaxPlayers(Script::CStruct *pParams, Script::CScript *pScript)
{
	int max_players=1;
	pParams->GetInteger(NONAME,&max_players);
	Dbg_MsgAssert(max_players>=1 && max_players<=8,("\n%s\nBad value of %d sent to SetParkEditorMaxPlayers",pScript->GetScriptInfo(),max_players));
	
	CParkManager::Instance()->GetGenerator()->SetMaxPlayers(max_players);

	return true;
}	

bool ScriptGetParkEditorMaxPlayers(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->AddInteger("MaxPlayers",CParkManager::Instance()->GetGenerator()->GetMaxPlayers());
	return true;
}	

bool ScriptGetParkEditorMaxPlayersPossible(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->AddInteger("MaxPlayersPossible",CParkManager::Instance()->GetGenerator()->GetMaxPlayersPossible());
	return true;
}	


// a helper function used by ScriptCanCleanlyResizePark(), ScriptResizePark()
GridDims ComputeResizedDims(Script::CStruct *pParams)
{
	// these are the current bounds, may be replaced with new info
	GridDims new_bounds = CParkManager::sInstance()->GetParkNearBounds();	
	
	// assume current w,l unless new parms passed in
	int w = new_bounds.GetW();
	int l = new_bounds.GetL();
	pParams->GetInteger("w", &w);
	pParams->GetInteger("l", &l);

	// these diffs may be replaced below
	int w_diff = w - (int) new_bounds.GetW();
	int l_diff = l - (int) new_bounds.GetL();

	if (pParams->GetInteger("inc_w", &w_diff))
		w = new_bounds.GetW() + w_diff;
	if (pParams->GetInteger("inc_l", &l_diff))
		l = new_bounds.GetL() + l_diff;
	
	new_bounds[X] = new_bounds[X] - w_diff / 2;
	new_bounds[Z] = new_bounds[Z] - l_diff / 2;
	new_bounds[W] = w;
	new_bounds[L] = l;

	
	return new_bounds;
}

// Prevents rail points or goal peds being placed too close to the edge of the park.
bool IsWithinParkBoundaries(Mth::Vector pos, float margin)
{
	GridDims park_bounds = CParkManager::sInstance()->GetParkNearBounds();	
	Mth::Vector corner_pos;
	Mth::Vector area_dims;
	corner_pos=Ed::CParkManager::Instance()->GridCoordinatesToWorld(park_bounds,&area_dims);

	if (pos[X] > corner_pos[X]+margin && pos[X] < corner_pos[X]+area_dims[X]-margin &&
		pos[Z] > corner_pos[Z]+margin && pos[Z] < corner_pos[Z]+area_dims[Z]-margin)
	{
		return true;
	}
	
	return false;
}		


bool ScriptCanCleanlyResizePark(Script::CStruct *pParams, Script::CScript *pScript)
{
	GridDims new_bounds;
	
	Script::CStruct *p_new_bounds=NULL;
	if (pParams->GetStructure(CRCD(0xb4f88771,"NewBounds"),&p_new_bounds))
	{
		int x,z,w,l;
		p_new_bounds->GetInteger(CRCD(0x7323e97c,"x"),&x);
		p_new_bounds->GetInteger(CRCD(0x9d2d8850,"z"),&z);
		p_new_bounds->GetInteger(CRCD(0xe39cf4ed,"w"),&w);
		p_new_bounds->GetInteger(CRCD(0x69f93d01,"l"),&l);
		new_bounds.SetXYZ(x,0,z);
		new_bounds.SetWHL(w,1,l);
		
		bool can_resize=true;
		if (CParkManager::sInstance()->AreMetasOutsideBounds(new_bounds))
		{
			can_resize=false;
		}
		
		Mth::Vector corner_pos;
		Mth::Vector area_dims;
		corner_pos=Ed::CParkManager::Instance()->GridCoordinatesToWorld(new_bounds,&area_dims);
		if (Obj::GetRailEditor()->ThereAreRailPointsOutsideArea(corner_pos[X]+PARK_BOUNDARY_MARGIN, 
																corner_pos[Z]+PARK_BOUNDARY_MARGIN,
																corner_pos[X]+area_dims[X]-PARK_BOUNDARY_MARGIN, 
																corner_pos[Z]+area_dims[Z]-PARK_BOUNDARY_MARGIN))
		{
			can_resize=false;
		}

		if (Obj::GetGoalEditor()->ThereAreGoalsOutsideArea(corner_pos[X], corner_pos[Z], corner_pos[X]+area_dims[X], corner_pos[Z]+area_dims[Z]))
		{
			can_resize=false;
		}
		
		return can_resize;	
	}

	return false;
}


void ResizePark(GridDims newBounds)
{
	CParkManager::sInstance()->RebuildInnerShell(newBounds);
	CParkManager::sInstance()->RebuildFloor();

	Mth::Vector corner_pos;
	Mth::Vector area_dims;
	corner_pos=Ed::CParkManager::Instance()->GridCoordinatesToWorld(newBounds,&area_dims);
	Obj::GetRailEditor()->ClipRails(corner_pos[X]+PARK_BOUNDARY_MARGIN, corner_pos[Z]+PARK_BOUNDARY_MARGIN,
									corner_pos[X]+area_dims[X]-PARK_BOUNDARY_MARGIN, corner_pos[Z]+area_dims[Z]-PARK_BOUNDARY_MARGIN,
									Obj::CRailEditorComponent::DELETE_POINTS_OUTSIDE);
	Obj::GetGoalEditor()->DeleteGoalsOutsideArea(corner_pos[X], corner_pos[Z], corner_pos[X]+area_dims[X], corner_pos[Z]+area_dims[Z]);
	
	CParkEditor::Instance()->DestroyClipboardsWhichAreTooBigToFit();
}

bool ScriptResizePark(Script::CStruct *pParams, Script::CScript *pScript)
{
	static GridDims stored_dims;
	
	GridDims new_bounds;
	
	Script::CStruct *p_new_bounds=NULL;
	if (pParams->GetStructure(CRCD(0xb4f88771,"NewBounds"),&p_new_bounds))
	{
		int x,z,w,l;
		p_new_bounds->GetInteger(CRCD(0x7323e97c,"x"),&x);
		p_new_bounds->GetInteger(CRCD(0x9d2d8850,"z"),&z);
		p_new_bounds->GetInteger(CRCD(0xe39cf4ed,"w"),&w);
		p_new_bounds->GetInteger(CRCD(0x69f93d01,"l"),&l);
		new_bounds.SetXYZ(x,0,z);
		new_bounds.SetWHL(w,1,l);
	}
	else
	{
		new_bounds = ComputeResizedDims(pParams);
	}
	
	
	if (pParams->ContainsFlag("delayed"))
	{
		stored_dims = new_bounds;
	}
	else 
	{
		if (pParams->ContainsFlag("use_stored"))
		{
			new_bounds = stored_dims;
		}
		if (!CParkManager::sInstance()->EnoughMemoryToResize(new_bounds))
		{
			Script::RunScript("parked_show_out_of_memory_message");
			return false;
		}
				
		ResizePark(new_bounds);
		// Refresh the rail geometry so that UpdateSuperSectors gets called on them.
		//Obj::GetRailEditor()->RefreshGeometry();
	}
	
	return true;
}


bool ScriptGetCurrentParkBounds(Script::CStruct *pParams, Script::CScript *pScript)
{
	GridDims bounds = CParkManager::sInstance()->GetParkNearBounds();	
	pScript->GetParams()->AddInteger(CRCD(0x7323e97c,"x"),bounds.GetX());
	pScript->GetParams()->AddInteger(CRCD(0x9d2d8850,"z"),bounds.GetZ());
	pScript->GetParams()->AddInteger(CRCD(0xe39cf4ed,"w"),bounds.GetW());
	pScript->GetParams()->AddInteger(CRCD(0x69f93d01,"l"),bounds.GetL());
	return true;
}	

bool ScriptCanChangeParkDimension(Script::CStruct *pParams, Script::CScript *pScript)
{
	GridDims current_bounds = CParkManager::sInstance()->GetParkNearBounds();
	GridDims max_bounds = CParkManager::sInstance()->GetParkFarBounds();
	
	Script::CArray *p_dim_array = NULL;
	if (pParams->GetArray(NONAME, &p_dim_array))
	{
		int w = p_dim_array->GetInt(0);
		int l = p_dim_array->GetInt(1);
		if (w != current_bounds.GetW() || l != current_bounds.GetL())
			return true;
	}
	else
	{
		// otherwise, decrease
		bool test_for_increase = pParams->ContainsFlag("up");
	
		if (pParams->ContainsFlag("width"))
		{
			if (test_for_increase && current_bounds.GetW() < max_bounds.GetW()) return true;
			else if (!test_for_increase && current_bounds.GetW() > 16) return true;
		}
		else
		{
			if (test_for_increase && current_bounds.GetL() < max_bounds.GetL()) return true;
			else if (!test_for_increase && current_bounds.GetL() > 16) return true;
		}
	}
		
	return false;
}




bool ScriptSaveParkToDisk(Script::CStruct *pParams, Script::CScript *pScript)
{
	int slot = 0;
	pParams->GetInteger("slot", &slot, Script::ASSERT);
	// XXX
	Ryan("saving park at slot %d\n", slot);
	CParkEditor* p_editor = CParkEditor::Instance();
	p_editor->AccessDisk(true, slot, false);
	return true;
}




bool ScriptLoadParkFromDisk(Script::CStruct *pParams, Script::CScript *pScript)
{
	int slot = 0;
	pParams->GetInteger("slot", &slot, Script::ASSERT);

	bool block_rebuild = pParams->ContainsFlag("block_rebuild");

	// XXX
	Ryan("loading park in slot %d\n", slot);
	CParkEditor* p_editor = CParkEditor::Instance();
	p_editor->AccessDisk(false, slot, block_rebuild);
	return true;
}




bool ScriptIsParkUnsaved(Script::CStruct *pParams, Script::CScript *pScript)
{
	return !CParkManager::sInstance()->IsMapSavedLocally();
}




bool ScriptFireCustomParkGap(Script::CStruct *pParams, Script::CScript *pScript)
{
	int gap_index;
	pParams->GetInteger("gap_index", &gap_index, true);

	// XXX
	Ryan("ScriptFireCustomParkGap() %d\n", gap_index);
	
	CGapManager::sInstance()->LaunchGap(gap_index, pScript);
	return true;
}

bool ScriptSetEditedParkGapInfo(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *p_name = NULL;
	pParams->GetString("name", &p_name);
	int score = -1;
	pParams->GetInteger("score", &score);
	
	CCursor::sInstance()->SetGapNameAndScore(p_name, score);

	Script::CStruct *p_cancel_flags=NULL;
	pParams->GetStructure(CRCD(0x49ec3f9,"cancel_flags"),&p_cancel_flags);
	CCursor::sInstance()->SetGapCancelFlags(p_cancel_flags);
	return true;
}




bool ScriptGetEditedParkGapName(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->AddString("name", CCursor::sInstance()->GetGapName());

	return true;
}

bool ScriptParkEditorSelectionAreaTooBigToCopy(Script::CStruct *pParams, Script::CScript *pScript)
{
	return CCursor::sInstance()->SelectionAreaTooBigToCopy();
}

bool ScriptCopyParkEditorSelectionToClipboard(Script::CStruct *pParams, Script::CScript *pScript)
{
	bool success=CCursor::sInstance()->CopySelectionToClipboard();
	CCursor::sInstance()->ClearAreaSelection();	
	return success;
}

bool ScriptSwitchParkEditorMenuPieceToMostRecentClipboard(Script::CStruct *pParams, Script::CScript *pScript)
{
	CParkEditor::Instance()->SwitchMenuPieceToMostRecentClipboard();
	return true;
}

bool ScriptCutParkEditorAreaSelection(Script::CStruct *pParams, Script::CScript *pScript)
{
	bool success=CCursor::sInstance()->CopySelectionToClipboard();
	if (success)
	{
		CCursor::sInstance()->DeleteSelectedPieces();
		CCursor::sInstance()->ResetSelectedHeights();
	}
	CCursor::sInstance()->ClearAreaSelection();	
	return success;
}

bool ScriptParkEditorAreaSelectionDeletePieces(Script::CStruct *pParams, Script::CScript *pScript)
{
	CCursor::sInstance()->DeleteSelectedPieces();
	return true;
}

bool ScriptContinueParkEditorAreaSelection(Script::CStruct *pParams, Script::CScript *pScript)
{
	CCursor::sInstance()->ContinueAreaSelection();
	return true;
}
	
bool ScriptGetEditorTheme(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->AddInteger(CRCD(0x688a18f7,"theme"), CParkManager::sInstance()->GetTheme());
	return true;
}

bool ScriptSetEditorTheme(Script::CStruct *pParams, Script::CScript *pScript)
{
	int theme=0;
	pParams->GetInteger(CRCD(0x688a18f7,"theme"),&theme);
	CParkManager::sInstance()->SetTheme(theme);
	return true;
}

bool ScriptGetEditorMaxThemes(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->AddInteger(CRCD(0x7ddc0da8,"max_themes"), MAX_THEMES);
	return true;
}

bool ScriptGetCustomParkName(Script::CStruct *pParams, Script::CScript *pScript)
{
	int truncate_char_num = -1;
	pParams->GetInteger(NONAME, &truncate_char_num);
	
	const char *p_name = CParkManager::sInstance()->GetParkName();
	
	if (truncate_char_num == -1)
	{
		if (p_name)
			pScript->GetParams()->AddString("name", p_name);
		else
			pScript->GetParams()->AddString("name", "unnamed park");
	}
	else
	{
		char out_name[64];
		for (int i = 0; i < truncate_char_num; i++)
			out_name[i] = p_name[i];
		out_name[truncate_char_num] = '\0';
		pScript->GetParams()->AddString("name", out_name);
	}
	return true;
}




bool ScriptSetCustomParkName(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *p_name = NULL;
	pParams->GetString("name", &p_name, Script::ASSERT);
	CParkManager::sInstance()->SetParkName(p_name);
	return true;
}


// @script | IsCustomPark | returns true if this is a custom park
bool ScriptIsCustomPark(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	return Ed::CParkEditor::Instance()->UsingCustomPark();
}



bool ScriptBindParkEditorToController( Script::CStruct* pParams, Script::CScript* pScript )
{
	int controller;
	pParams->GetInteger( NONAME, &controller, Script::ASSERT );
	CParkEditor* p_editor = CParkEditor::Instance();
	p_editor->BindParkEditorToController( controller );
	return true;
}

#ifdef __PLAT_NGC__
// Required in order that the park rebuild when defragmenting during resizing does not
// reset the map heights.
// (Used in the script parked_defragment_memory_for_resize in sk5ed_scripts)
bool ScriptWriteCompressedMapBuffer(Script::CStruct *pParams, Script::CScript *pScript)
{
	CParkManager::sInstance()->WriteCompressedMapBuffer();
	return true;
}

bool ScriptRequiresDefragment(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mem::Heap *p_top_down_heap = Mem::Manager::sHandle().TopDownHeap();
	if (p_top_down_heap->mp_region->MemAvailable() < TOP_DOWN_REQUIRED_MARGIN)
	{
		return true;
	}
	
        Mem::Heap *p_main_heap = Mem::Manager::sHandle().BottomUpHeap();
        if (p_main_heap->mFreeMem.m_count > 200000)
	{
		return true;
	}

	return false;
}

bool ScriptTopDownHeapTooLow(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mem::Heap *p_top_down_heap = Mem::Manager::sHandle().TopDownHeap();
	if (p_top_down_heap->mp_region->MemAvailable() < TOP_DOWN_REQUIRED_MARGIN)
	{
		return true;
	}
	return false;
}	

#endif


}
