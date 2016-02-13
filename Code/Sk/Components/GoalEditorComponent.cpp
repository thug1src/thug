//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       GoalEditorComponent.cpp
//* OWNER:          Kendall Harrison
//* CREATION DATE:  3/21/2003
//****************************************************************************

#include <sk/components/GoalEditorComponent.h>
#include <sk/components/EditorCameraComponent.h>
#include <sk/parkeditor2/parked.h>
#include <gel/components/inputcomponent.h>
#include <sk/modules/skate/goalmanager.h>
#include <gel/objman.h>
#include <sk/modules/skate/skate.h>
#include <sk/scripting/nodearray.h>

#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>

#include <gfx/nx.h>
#include <gfx/nxmodel.h>
#include <sk/engine/feeler.h>
#include <gel/objtrack.h>

//#define SIZE_TEST

namespace Obj
{

static uint32 s_convert_level(uint32 level)
{
	if (level==CRCD(0xb664035d,"Load_Sk5Ed_gameplay"))
	{
		// Map Load_Sk5Ed_gameplay back to Load_Sk5Ed 
		level=CRCD(0xe8b4b836,"Load_Sk5Ed");
	}
	
	return level;
}

// Often in the scripts the value returned by GetCurrentLevel is passed on to commands like GetEditedGoalsInfo
// However sometimes the level might be returned as Load_Sk5Ed_gameplay rather than Load_Sk5Ed, so
// this will map it back to Load_Sk5Ed
static uint32 s_get_level_checksum(Script::CStruct *p_params)
{
	uint32 level=0;
	p_params->GetChecksum(CRCD(0x651533ec,"level"),&level);
	
	level=s_convert_level(level);
			
	return level;
}

static uint32 s_get_current_level()
{
	Mdl::Skate *p_skate_module = Mdl::Skate::Instance();
	uint32 current_level=p_skate_module->m_cur_level;
	
	current_level=s_convert_level(current_level);						 
	
	return current_level;
}	
	
static const char *s_generate_letter_model_filename(uint32 goalType, int letterIndex)
{
	const char *p_model_file_name=NULL;
	switch (goalType)
	{
		case 0x54166acd: // Skate
			switch (letterIndex)
			{
				case 0:
					p_model_file_name="gameobjects\\skate\\letter_s\\letter_s.mdl";
					break;
				case 1:
					p_model_file_name="gameobjects\\skate\\letter_k\\letter_k.mdl";
					break;
				case 2:
					p_model_file_name="gameobjects\\skate\\letter_a\\letter_a.mdl";
					break;
				case 3:
					p_model_file_name="gameobjects\\skate\\letter_t\\letter_t.mdl";
					break;
				case 4:
					p_model_file_name="gameobjects\\skate\\letter_e\\letter_e.mdl";
					break;
				default:
					break;
			}		
			break;
		case 0x4ec3cfb5: // Combo
			switch (letterIndex)
			{
				case 0:
					p_model_file_name="gameobjects\\combo\\goal_combo_c\\goal_combo_c.mdl";
					break;
				case 1:
					p_model_file_name="gameobjects\\combo\\goal_combo_o\\goal_combo_o.mdl";
					break;
				case 2:
					p_model_file_name="gameobjects\\combo\\goal_combo_m\\goal_combo_m.mdl";
					break;
				case 3:
					p_model_file_name="gameobjects\\combo\\goal_combo_b\\goal_combo_b.mdl";
					break;
				case 4:
					p_model_file_name="gameobjects\\combo\\goal_combo_o\\goal_combo_o.mdl";
					break;
				default:
					break;
			}		
			break;
		default:
			break;
	}				
	return p_model_file_name;
}

static uint32 s_generate_letter_node_type_checksum(uint32 goalType, int letterIndex)
{
	uint32 checksum=0;
	switch (goalType)
	{
		case 0x54166acd: // Skate
			switch (letterIndex)
			{
				case 0:
					checksum=CRCD(0xf75425ea,"Letter_S");
					break;
				case 1:
					checksum=CRCD(0xe438bdbc,"Letter_K");
					break;
				case 2:
					checksum=CRCD(0x4ed54a2,"Letter_A");
					break;
				case 3:
					checksum=CRCD(0x6930b049,"Letter_T");
					break;
				case 4:
					checksum=CRCD(0x38090bb,"Letter_E");
					break;
				default:
					break;
			}		
			break;
		case 0x4ec3cfb5: // Combo
			switch (letterIndex)
			{
				case 0:
					checksum=CRCD(0x7bf1ba66,"Combo_C");
					break;
				case 1:
					checksum=CRCD(0x7247f64d,"Combo_O");
					break;
				case 2:
					checksum=CRCD(0x9c499761,"Combo_M");
					break;
				case 3:
					checksum=CRCD(0xcf68af0,"Combo_B");
					break;
				case 4:
					checksum=CRCD(0x7247f64d,"Combo_O");
					break;
				default:
					break;
			}		
			break;
		default:
			break;
	}				
	return checksum;
}

static uint32 s_generate_node_name_checksum(int goalIndex, int positionIndex)
{
	char p_name[100];
	p_name[0]=0;
	
	switch (positionIndex)
	{
		case 0:
			sprintf(p_name,"TRG_EditedGoal%d_Pro",goalIndex);
			break;
		case 1:
			sprintf(p_name,"TRG_EditedGoal%d_Restart",goalIndex);
			break;
		default:
			sprintf(p_name,"TRG_EditedGoal%d_Letter_%d",goalIndex,positionIndex-2);
			break;	
	}
	
	return Script::GenerateCRC(p_name);
}

static const Script::CStruct *s_get_default_goal_params(uint32 goalType)
{
	Script::CStruct *p_default_params=NULL;
	
	switch (goalType)
	{
		case 0x54166acd: // Skate
			p_default_params=Script::GetStructure(CRCD(0x3328976a,"Goal_SkateLetters_genericParams"));
			break;
		case 0x4ec3cfb5: // Combo
			p_default_params=Script::GetStructure(CRCD(0xb4f5801d,"Goal_amateurCOMBOline_genericParams"));
			break;
		case 0x6fe44c6d: // HighScore
			p_default_params=Script::GetStructure(CRCD(0xef8f345f,"Goal_HighScore_genericParams"));
			break;
		case 0xec414b76: // HighCombo
			p_default_params=Script::GetStructure(CRCD(0xb520a510,"goal_highcombo_genericParams"));
			break;
		case 0x7fe2238a: // SkateTris
		case 0x0fc6f698: // ComboSkateTris
		case 0xea7ba666: // TrickTris
			p_default_params=Script::GetStructure(CRCD(0x675816b5,"goal_tetris_genericParams"));
			break;
		case 0x61c5d092: // Gap
			p_default_params=Script::GetStructure(CRCD(0x5756a675,"Goal_Gaps_genericParams"));	
			break;
		default:
			break;
	}			
	return p_default_params;
}

static const Script::CStruct *s_get_extra_goal_params(uint32 goalType)
{
	Script::CStruct *p_extra_params=NULL;
	
	switch (goalType)
	{
		case 0x54166acd: // Skate
			p_extra_params=Script::GetStructure(CRCD(0xe41e0e04,"EditedGoal_ExtraParams_Skate"));
			break;
		case 0x4ec3cfb5: // Combo
			p_extra_params=Script::GetStructure(CRCD(0xfecbab7c,"EditedGoal_ExtraParams_Combo"));
			break;
		case 0x6fe44c6d: // HighScore
			p_extra_params=Script::GetStructure(CRCD(0x28c7b799,"EditedGoal_ExtraParams_HighScore"));
			break;
		case 0xec414b76: // HighCombo
			p_extra_params=Script::GetStructure(CRCD(0xab62b082,"EditedGoal_ExtraParams_HighCombo"));
			break;
		case 0x7fe2238a: // SkateTris
			p_extra_params=Script::GetStructure(CRCD(0x38c1d87e,"EditedGoal_ExtraParams_SkateTris"));
			break;
		case 0x0fc6f698: // ComboSkateTris
			p_extra_params=Script::GetStructure(CRCD(0x6626b0d8,"EditedGoal_ExtraParams_ComboSkateTris"));
			break;
		case 0xea7ba666: // TrickTris	
			p_extra_params=Script::GetStructure(CRCD(0xad585d92,"EditedGoal_ExtraParams_TrickTris"));
			break;
		case 0x61c5d092: // Gap
			p_extra_params=Script::GetStructure(CRCD(0x9005703a,"EditedGoal_ExtraParams_Gap"));
			break;
		default:
			break;
	}
	return p_extra_params;
}
			
void InsertGoalEditorNodes()
{
	Script::CArray *p_node_array=GetArray(CRCD(0xc472ecc5,"NodeArray"),Script::ASSERT);
	
	uint32 new_node_index=0;
	if (p_node_array->GetSize()==0)
	{
		// If it's empty, it'll need its type setting too.
		p_node_array->SetSizeAndType(MAX_POSITIONS_PER_GOAL*MAX_GOALS_PER_LEVEL,ESYMBOLTYPE_STRUCTURE);
	}
	else
	{
		new_node_index=p_node_array->GetSize();
		p_node_array->Resize(p_node_array->GetSize() + MAX_POSITIONS_PER_GOAL*MAX_GOALS_PER_LEVEL);
	}	
	
	Script::CStruct *p_new_node=NULL;
	for (int g=0; g<MAX_GOALS_PER_LEVEL; ++g)
	{
		p_new_node=new Script::CStruct;
		p_new_node->AppendStructure(Script::GetStructure(CRCD(0x8f0f67d7,"EditedGoal_Pro_Node"),Script::ASSERT));
		p_new_node->AddChecksum(CRCD(0xa1dc81f9,"Name"),s_generate_node_name_checksum(g,0));
		p_new_node->AddInteger(CRCD(0xe50d6573,"NodeIndex"),new_node_index);
		p_node_array->SetStructure(new_node_index++,p_new_node);
		
		p_new_node=new Script::CStruct;
		p_new_node->AppendStructure(Script::GetStructure(CRCD(0xc9119673,"EditedGoal_Restart_Node"),Script::ASSERT));
		p_new_node->AddChecksum(CRCD(0xa1dc81f9,"Name"),s_generate_node_name_checksum(g,1));
		p_new_node->AddInteger(CRCD(0xe50d6573,"NodeIndex"),new_node_index);
		p_node_array->SetStructure(new_node_index++,p_new_node);
		
		for (int i=0; i<MAX_POSITIONS_PER_GOAL-2; ++i)
		{
			p_new_node=new Script::CStruct;
			p_new_node->AppendStructure(Script::GetStructure(CRCD(0x1aaf51ba,"EditedGoal_Letter_Node"),Script::ASSERT));
			
			p_new_node->AddChecksum(CRCD(0xa1dc81f9,"Name"),s_generate_node_name_checksum(g,i+2));
			p_new_node->AddInteger(CRCD(0xe50d6573,"NodeIndex"),new_node_index);
			
			p_node_array->SetStructure(new_node_index++,p_new_node);
		}	
	}	
}

CEditGoal::CEditGoal()
{
	Clear();
}

CEditGoal::~CEditGoal()
{
}

void CEditGoal::Clear()
{
	m_used=false;
	m_level_script=0;
	mp_goal_name[0]=0;	
	mp_goal_description[0]=0;
	mp_win_message[0]=0;
	mp_ped_name[0]=0;
	m_goal_type=0;
	
	m_num_positions_set=0;
	m_num_positions_required=0;
	m_current_position_index=0;
	m_placed_last_position=false;
	m_has_won_goal=false;
		
	m_score=100;
	m_time_limit=120;
	
	m_acceleration_interval=5;
	m_acceleration_percent=0.1f;
	m_trick_time=3000;
	m_time_to_stop_adding_tricks=5;
	m_max_tricks=11;
	m_spin=360;
	
	m_single_combo=false;
	m_combo_size=2;
	
	m_tricktris_block_size=3;
	m_tricktris_total_to_win=8;

	m_num_combo_sets=0;

	m_control_type=CRCD(0x54166acd,"skate");
	
	m_num_gaps=0;
	mp_trick_name[0]=0;
				
	for (int i=0; i<MAX_POSITIONS_PER_GOAL; ++i)
	{
		mp_item_positions[i].mType=CGoalPos::NONE;
		mp_item_positions[i].mHeight=0.0f;
	}
	m_level_goal_index=0;
}

const char *CEditGoal::GetIDString()
{
	static char sp_id_string[20];
	sprintf(sp_id_string,"CreatedGoal%d",m_level_goal_index);
	return sp_id_string;
}

uint32 CEditGoal::GetId()
{
	return Script::GenerateCRC(GetIDString());
}

void CEditGoal::refresh_position_using_collision_check(uint positionIndex)
{
	Dbg_MsgAssert(positionIndex < MAX_POSITIONS_PER_GOAL,("Bad positionIndex"));
	
	Mth::Vector pos=mp_item_positions[positionIndex].mPos;
	// First, do a drop down check from the current position
	Mth::Vector start=pos;
	start[Y]+=10000.0f;
	Mth::Vector end=pos;
	end[Y]-=10000.0f;
	
	CFeeler feeler;
	feeler.SetStart(start);
	feeler.SetEnd(end);
	if (feeler.GetCollision())
	{
		mp_item_positions[positionIndex].mPos=feeler.GetPoint();
		return;
	}
	
	// No collision found, so try starting from much higher up
	start[Y]+=10000.0f;	
	feeler.SetStart(start);
	feeler.SetEnd(end);
	if (feeler.GetCollision())
	{
		mp_item_positions[positionIndex].mPos=feeler.GetPoint();
	}
}

void CEditGoal::RefreshPositionsUsingCollisionCheck()
{
	for (int i=0; i<m_num_positions_set; ++i)
	{
		refresh_position_using_collision_check(i);
		
		Script::CStruct params;
		params.AddChecksum("Name",generate_goal_object_name(i));
	
		params.AddVector("Pos",mp_item_positions[i].mPos[X],
							   mp_item_positions[i].mPos[Y]+mp_item_positions[i].mHeight,
							   mp_item_positions[i].mPos[Z]);
		
		Script::RunScript(CRCD(0x79582240,"goal_editor_refresh_goal_object_position"),&params);
	}	
}

void CEditGoal::add_goal_params(Script::CStruct *p_params)
{
	Dbg_MsgAssert(p_params,("NULL p_params"));

	p_params->AppendStructure(s_get_default_goal_params(m_goal_type));
	p_params->AppendStructure(s_get_extra_goal_params(m_goal_type));
	
	// Now change the node names to be those of the special nodes created just for edited goals.
    p_params->AddChecksum(CRCD(0x2d7e03d,"trigger_obj_id"),s_generate_node_name_checksum(m_level_goal_index,0));
    p_params->AddChecksum(CRCD(0x3494f170,"restart_node"),s_generate_node_name_checksum(m_level_goal_index,1));
	
	// Then write in other misc params.
	
	switch (m_goal_type)
	{
		case 0x54166acd: // Skate
		{
			p_params->AddChecksum(CRCD(0xba54bd5c,"S_obj_id"),s_generate_node_name_checksum(m_level_goal_index,2));
			p_params->AddChecksum(CRCD(0x150a97c2,"K_obj_id"),s_generate_node_name_checksum(m_level_goal_index,3));
			p_params->AddChecksum(CRCD(0x84ca8b0a,"A_obj_id"),s_generate_node_name_checksum(m_level_goal_index,4));
			p_params->AddChecksum(CRCD(0xb091b445,"T_obj_id"),s_generate_node_name_checksum(m_level_goal_index,5));
			p_params->AddChecksum(CRCD(0x008085f0,"E_obj_id"),s_generate_node_name_checksum(m_level_goal_index,6));
			break;
		}
			
		case 0x4ec3cfb5: // Combo
		{
			Script::CArray *p_array=new Script::CArray;
			p_array->SetSizeAndType(5,ESYMBOLTYPE_STRUCTURE);
			
			Script::CStruct *p_struct=new Script::CStruct;
			p_struct->AddChecksum(CRCD(0x99f6ccbb,"obj_id"),s_generate_node_name_checksum(m_level_goal_index,2));
			p_struct->AddString(CRCD(0xc4745838,"text"),"C");
			p_array->SetStructure(0,p_struct);

			p_struct=new Script::CStruct;
			p_struct->AddChecksum(CRCD(0x99f6ccbb,"obj_id"),s_generate_node_name_checksum(m_level_goal_index,3));
			p_struct->AddString(CRCD(0xc4745838,"text"),"O");
			p_array->SetStructure(1,p_struct);

			p_struct=new Script::CStruct;
			p_struct->AddChecksum(CRCD(0x99f6ccbb,"obj_id"),s_generate_node_name_checksum(m_level_goal_index,4));
			p_struct->AddString(CRCD(0xc4745838,"text"),"M");
			p_array->SetStructure(2,p_struct);

			p_struct=new Script::CStruct;
			p_struct->AddChecksum(CRCD(0x99f6ccbb,"obj_id"),s_generate_node_name_checksum(m_level_goal_index,5));
			p_struct->AddString(CRCD(0xc4745838,"text"),"B");
			p_array->SetStructure(3,p_struct);

			p_struct=new Script::CStruct;
			p_struct->AddChecksum(CRCD(0x99f6ccbb,"obj_id"),s_generate_node_name_checksum(m_level_goal_index,6));
			p_struct->AddString(CRCD(0xc4745838,"text"),"O");
			p_array->SetStructure(4,p_struct);
			
			p_params->AddArrayPointer(CRCD(0xb3c7f1b2,"letter_info"),p_array);
			break;
		}	
		
		default:
			break;
	}

	WriteGoalSpecificParams(p_params,WRITING_TO_GOAL_MANAGER);
	
	if (mp_goal_description[0])
	{
		p_params->AddString(CRCD(0xc5d7e6b,"goal_description"),mp_goal_description);
	}

	if (mp_win_message[0])
	{
		Script::CArray *p_default_success_cam_anims=Script::GetArray(CRCD(0x8070c110,"EditedGoal_success_cam_anims"),Script::ASSERT);
		Dbg_MsgAssert(p_default_success_cam_anims->GetSize()==1,("Expected EditedGoal_success_cam_anims to contain just one structure"));
		
		Script::CArray *p_success_cam_anims=new Script::CArray;
		Script::CopyArray(p_success_cam_anims, p_default_success_cam_anims);
		p_success_cam_anims->GetStructure(0)->AddString(CRCD(0xa7ab3b6d,"cam_anim_text"), mp_win_message);
		p_success_cam_anims->GetStructure(0)->AddChecksum(CRCD(0x531e4d28,"targetid"),s_generate_node_name_checksum(m_level_goal_index,0));
			
		p_params->AddArrayPointer(CRCD(0x27cd32e1,"success_cam_anims"), p_success_cam_anims);
	}

	p_params->AddString(CRCD(0xbfecc45b,"goal_name"),GetIDString());
	
	if (mp_goal_name[0])
	{
		p_params->AddString(CRCD(0x4bc5229d,"view_goals_text"),mp_goal_name);
	}	
		
	if (mp_ped_name[0])
	{
		p_params->AddString(CRCD(0x243b9c3b,"full_name"),mp_ped_name);
	}	
	
	p_params->AddInteger(CRCD(0x906b67ba,"time"),m_time_limit);
	p_params->AddChecksum(CRCD(0x81cff663,"control_type"),m_control_type);		
	
	
	// This is so that the goal manager can tell that this is an edited goal, and hence not save its
	// details to the mem card.
	p_params->AddChecksum(NONAME,CRCD(0x981d3ad0,"edited_goal"));
}

uint32 CEditGoal::generate_marker_object_name(int index)
{
	static char p_name[100];
	sprintf(p_name,"GoalEditorMarkerObject%d",index);
	return Script::GenerateCRC(p_name);
}	

uint32 CEditGoal::generate_goal_object_name(int index)
{
	static char p_name[100];
	
	if (index == 0)
	{
		sprintf(p_name,"TRG_EditedGoal%d_Pro",m_level_goal_index);
	}
	else if (index > 1)
	{
		sprintf(p_name,"TRG_EditedGoal%d_Letter_%d",m_level_goal_index,index-2);
	}
	else
	{
		// Position 1 is the restart, so no object exists.
		return 0;
	}	
	
	return Script::GenerateCRC(p_name);
}	

void CEditGoal::create_marker(int index)
{
	Dbg_MsgAssert(index<MAX_POSITIONS_PER_GOAL,("Bad index"));

	Script::CStruct params;
	
	switch (mp_item_positions[index].mType)
	{
		case CGoalPos::PED:
			params.AddChecksum(CRCD(0x7321a8d6,"Type"),CRCD(0x61a741e,"Ped"));
			break;

		case CGoalPos::RESTART:
			params.AddChecksum(CRCD(0x7321a8d6,"Type"),CRCD(0x1806ddf8,"Restart"));
			params.AddString("Model","gameobjects\\p1_cursor\\p1_cursor.mdl");
			break;
					
		case CGoalPos::LETTER:
		{
			params.AddChecksum(CRCD(0x7321a8d6,"Type"),CRCD(0x71fd11f5,"Letter"));
		
			// The -2 is because the first two positions are the ped and restart
			Dbg_MsgAssert(index>=2,("Bad index"));
			const char *p_letter_model_filename=s_generate_letter_model_filename(m_goal_type,index-2);
			params.AddString("Model",p_letter_model_filename);
			params.AddChecksum(NONAME,CRCD(0x3bd176d9,"RotateAndHover"));
			break;
		}
		default:
			break;
	}
					
	params.AddChecksum("Name",generate_marker_object_name(index));

	params.AddVector("Pos",mp_item_positions[index].mPos[X],
						   mp_item_positions[index].mPos[Y]+mp_item_positions[index].mHeight,
						   mp_item_positions[index].mPos[Z]);
	params.AddFloat("Angle",mp_item_positions[index].mAngle * 180.0f/3.141592654f);
	
	Script::RunScript("goal_editor_create_marker_object",&params);
}

void CEditGoal::delete_marker(int index)
{
	Dbg_MsgAssert(index<MAX_POSITIONS_PER_GOAL,("Bad index"));

	Script::CStruct params;
	params.AddChecksum("Name",generate_marker_object_name(index));
	Script::RunScript("goal_editor_delete_marker_object",&params);
}

bool CEditGoal::position_too_close_to_existing_positions(const Mth::Vector& pos)
{
	float default_min_dist_allowable=Script::GetFloat(CRCD(0x71b20276,"GoalEditor_DefaultMinDistBetweenPositions"));
	float letter_skater_min_dist_allowable=Script::GetFloat(CRCD(0x4e9755c1,"GoalEditor_MinDistBetweenLettersandSkater"));
	
	for (int i=0; i<m_num_positions_set; ++i)
	{
		// Ignore the current cursor position, otherwise a piece won't be able to be placed
		// back at its original position when navigating forward/back with X and triangle.
		if (i != m_current_position_index)
		{
			CGoalPos::EType this_type=mp_item_positions[m_current_position_index].mType;
			CGoalPos::EType that_type=mp_item_positions[i].mType;
			
			float min_dist_allowable=default_min_dist_allowable;
			if ((this_type==CGoalPos::LETTER && that_type==CGoalPos::RESTART) ||
				(this_type==CGoalPos::RESTART && that_type==CGoalPos::LETTER))
			{
				min_dist_allowable=letter_skater_min_dist_allowable;
			}	
			
			Mth::Vector p=mp_item_positions[i].mPos;
			p[Y]+=mp_item_positions[i].mHeight;
			
			if ((p-pos).Length() < min_dist_allowable)
			{
				return true;
			}
		}	
	}
	
	return false;
}

bool CEditGoal::position_ok_to_place(const Mth::Vector& pos, float height)
{
	Mdl::Skate * p_skate_mod =  Mdl::Skate::Instance();
	if (p_skate_mod->m_cur_level == CRCD(0xe8b4b836,"load_sk5ed") )
	{
		// Don't allow the pos to be too close to the park boundary, otherwise the ped can be made to
		// be embedded in the wall.
		if (!Ed::IsWithinParkBoundaries(pos, 30.0f))
		{
			return false;
		}
	}
			
	Mth::Vector pos_with_height=pos;
	pos_with_height[Y]+=height;
	if (position_too_close_to_existing_positions(pos_with_height))
	{
		return false;
	}
	return true;
}

// Used by the GetEditedGoalsInfo script command for use when creating the menu of existing goals.
// Also used by GetCurrentEditedGoalInfo command.
// Also used for writing the goal info to the mem card.
// And used by GetDebugInfo for sending stuff to the script debugger.
void CEditGoal::WriteIntoStructure(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info"));
	
	p_info->AddChecksum(NONAME,CRCD(0x981d3ad0,"edited_goal"));
	
	p_info->AddChecksum(CRCD(0x651533ec,"level"),m_level_script);
	p_info->AddInteger(CRCD(0x60ce75ae,"level_goal_index"),m_level_goal_index);
	p_info->AddChecksum(CRCD(0x9982e501,"goal_id"),GetId());
	p_info->AddInteger(CRCD(0xc51bf6e3,"time_limit"),m_time_limit);
	p_info->AddInteger(CRCD(0x14938611,"won_goal"),m_has_won_goal);
	p_info->AddChecksum(CRCD(0x81cff663,"control_type"),m_control_type);		
	
	p_info->AddString(CRCD(0x4bc5229d,"view_goals_text"),mp_goal_name);
	p_info->AddString(CRCD(0xc5d7e6b,"goal_description"),mp_goal_description);
	p_info->AddString(CRCD(0x5de57284,"win_message"),mp_win_message);
	p_info->AddString(CRCD(0xb9426608,"ped_name"),mp_ped_name);
	p_info->AddChecksum(CRCD(0x6d11ed74,"goal_type"),m_goal_type);
	p_info->AddChecksum(CRCD(0x71fc348b,"pro_name"),s_generate_node_name_checksum(m_level_goal_index,0));
	
	WriteGoalSpecificParams(p_info);

	if (m_num_positions_set)
	{
		Script::CArray *p_position_array=new Script::CArray;
		p_position_array->SetSizeAndType(m_num_positions_set,ESYMBOLTYPE_STRUCTURE);
		for (uint i=0; i<m_num_positions_set; ++i)
		{
			Script::CStruct *p_pos_info=new Script::CStruct;
			p_pos_info->AddVector(CRCD(0x7f261953,"Pos"),mp_item_positions[i].mPos[X],
														 mp_item_positions[i].mPos[Y],
														 mp_item_positions[i].mPos[Z]);
			p_pos_info->AddFloat(CRCD(0xab21af0,"Height"),mp_item_positions[i].mHeight);
			p_pos_info->AddFloat(CRCD(0xff7ebaf6,"Angle"),mp_item_positions[i].mAngle);
			p_position_array->SetStructure(i,p_pos_info);
		}
		p_info->AddArrayPointer(CRCD(0x32df12e0,"node_positions"),p_position_array);
	}	
}

void CEditGoal::ReadFromStructure(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info"));
	
	Clear();
	m_used=true;

	int won_goal=0;
    //p_info->GetInteger(CRCD(0x14938611,"won_goal"),&won_goal);
	m_has_won_goal=won_goal;
	
	m_level_script=s_get_level_checksum(p_info);
	Dbg_MsgAssert(m_level_script,("Zero m_level_script"));
	
	p_info->GetInteger(CRCD(0x60ce75ae,"level_goal_index"),&m_level_goal_index);
	
	p_info->GetChecksum(CRCD(0x6d11ed74,"goal_type"),&m_goal_type);
	Dbg_MsgAssert(m_goal_type,("Zero m_goal_type"));
	// Calling SetType will set up m_num_positions_required
	SetType(m_goal_type);

	p_info->GetInteger(CRCD(0xcd66c8ae,"score"),&m_score);

	p_info->GetInteger(CRCD(0xc51bf6e3,"time_limit"),&m_time_limit);
	p_info->GetChecksum(CRCD(0x81cff663,"control_type"),&m_control_type);
	
	const char *p_string="";
	p_info->GetString(CRCD(0x4bc5229d,"view_goals_text"),&p_string);
	Dbg_MsgAssert(strlen(p_string)<GOAL_NAME_BUFFER_SIZE,("goal name '%s' too long",p_string));
	strcpy(mp_goal_name,p_string);
	
	p_string="";
	p_info->GetString(CRCD(0xc5d7e6b,"goal_description"),&p_string);
	Dbg_MsgAssert(strlen(p_string)<GOAL_DESCRIPTION_BUFFER_SIZE,("goal description '%s' too long",p_string));
	strcpy(mp_goal_description,p_string);

	p_string="";
	p_info->GetString(CRCD(0x5de57284,"win_message"),&p_string);
	Dbg_MsgAssert(strlen(p_string)<GOAL_WIN_MESSAGE_BUFFER_SIZE,("Win message '%s' too long",p_string));
	strcpy(mp_win_message,p_string);
	
	p_string="";
	p_info->GetString(CRCD(0xb9426608,"ped_name"),&p_string);
	Dbg_MsgAssert(strlen(p_string)<PED_NAME_BUFFER_SIZE,("ped name '%s' too long",p_string));
	strcpy(mp_ped_name,p_string);
	
	
	Script::CArray *p_position_array=NULL;
	if (p_info->GetArray(CRCD(0x32df12e0,"node_positions"),&p_position_array))
	{
		Dbg_MsgAssert(p_position_array->GetSize()==m_num_positions_required,("Bad node_positions size for goal '%s'",mp_goal_name));
		
		m_num_positions_set=p_position_array->GetSize();
		for (uint i=0; i<m_num_positions_set; ++i)
		{
			Script::CStruct *p_struct=p_position_array->GetStructure(i);
			p_struct->GetVector(CRCD(0x7f261953,"Pos"),&mp_item_positions[i].mPos);
			p_struct->GetFloat(CRCD(0xab21af0,"Height"),&mp_item_positions[i].mHeight);
			p_struct->GetFloat(CRCD(0xff7ebaf6,"Angle"),&mp_item_positions[i].mAngle);
		}	
	}
		
	ReadGoalSpecificParams(p_info);
}

void CEditGoal::AddGap(uint8 gap_number)
{
	if (GotGap(gap_number))
	{
		return;
	}
		
	if (m_num_gaps>=MAX_GAPS)
	{
		printf("Warning! MAX_GAPS limit reached!\n");
		return;
	}	
	Dbg_MsgAssert(m_num_gaps<MAX_GAPS,("Too many gaps added"));
	mp_gap_numbers[m_num_gaps++]=gap_number;
}

void CEditGoal::RemoveGap(uint8 gap_number)
{
	for (int i=0; i<m_num_gaps; ++i)
	{
		if (mp_gap_numbers[i]==gap_number)
		{
			for (int j=i; j<m_num_gaps-1; ++j)
			{
				mp_gap_numbers[j]=mp_gap_numbers[j+1];
			}
			--m_num_gaps;
			break;
		}
	}			
}

// This is used when a gap is removed from the park editor. Once removed,
// any references to gaps with a gap number higher than the one removed will
// need to be decremented.
void CEditGoal::RemoveGapAndReorder(uint8 gap_number)
{
	RemoveGap(gap_number);
	for (int i=0; i<m_num_gaps; ++i)
	{
		if (mp_gap_numbers[i] > gap_number)
		{
			--mp_gap_numbers[i];
		}
	}
}
	
bool CEditGoal::GotGap(uint8 gap_number)
{
	for (int i=0; i<m_num_gaps; ++i)
	{
		if (mp_gap_numbers[i]==gap_number)
		{
			return true;
		}
	}
	return false;		
}

void CEditGoal::AddKeyComboSet(int setIndex, bool requirePerfect, int spin, int numTaps)
{
	int array_index=0;
	bool found=false;
	for (int i=0; i<m_num_combo_sets; ++i)
	{
		if (mp_combo_sets[i].mSetIndex==setIndex)
		{
			array_index=i;
			found=true;
			break;
		}
	}
	if (!found)
	{
		Dbg_MsgAssert(m_num_combo_sets<MAX_COMBO_SETS,("Too many key combo sets added, may need to increase MAX_COMBO_SETS in goaleditorcomponent.h (currently=%d)",MAX_COMBO_SETS));
		array_index=m_num_combo_sets;
		++m_num_combo_sets;
	}	
	
	mp_combo_sets[array_index].mNumTaps=numTaps;
	mp_combo_sets[array_index].mRequirePerfect=requirePerfect;
	mp_combo_sets[array_index].mSetIndex=setIndex;
	Dbg_MsgAssert((spin%180)==0,("spin must be a multiple of 180"));
	mp_combo_sets[array_index].mSpin=spin/180;
}

void CEditGoal::RemoveKeyComboSet(int setIndex)
{
	bool found=false;
	for (int i=0; i<m_num_combo_sets; ++i)
	{
		if (mp_combo_sets[i].mSetIndex==setIndex)
		{
			// Move the ones above down. Won't take long, they're only 4 bytes each.
			for (int j=i; j<m_num_combo_sets-1; ++j)
			{
				mp_combo_sets[j]=mp_combo_sets[j+1];
			}	
			found=true;
			break;
		}
	}
	if (found)
	{
		--m_num_combo_sets;
	}	
}

SPreDefinedKeyComboSet *CEditGoal::GetKeyComboSet(int setIndex)
{
	for (int i=0; i<m_num_combo_sets; ++i)
	{
		if (mp_combo_sets[i].mSetIndex==setIndex)
		{
			return &mp_combo_sets[i];
		}
	}
	return NULL;
}

// Used when generating the goal_tetris_key_combos array from the mp_combo_sets array,
// for sending to the goal manager.
void CEditGoal::generate_key_combos_array(Script::CStruct *p_info)
{
	if (!m_num_combo_sets)
	{
		printf("!!!!!!!! Warning! No combo sets specified, using emergency_key_combos\n");
		p_info->AddChecksum(CRCD(0x7be1e689,"goal_tetris_key_combos"),CRCD(0x6d641bcb,"emergency_key_combos"));
		return;
	}
		
	Script::CArray *p_predefined_key_combos=Script::GetArray(CRCD(0xba9358a5,"cag_skatetris_key_combos"),Script::ASSERT);

	int size=0;
	
	// Calculate the total size of the goal_tetris_key_combos array.
	for (int i=0; i<m_num_combo_sets; ++i)
	{
		Script::CStruct *p_struct=p_predefined_key_combos->GetStructure(mp_combo_sets[i].mSetIndex);
		Script::CArray *p_key_combos=NULL;
		p_struct->GetArray(CRCD(0x79704516,"key_combos"),&p_key_combos);
		Dbg_MsgAssert(p_key_combos,("Missing key_combos array in element %d of array cag_skatetris_key_combos",i));
		size+=p_key_combos->GetSize();
	}

	Dbg_MsgAssert(size,("Zero size for goal_tetris_key_combos array!"));

	Script::CArray *p_goal_tetris_key_combos=new Script::CArray;
	p_goal_tetris_key_combos->SetSizeAndType(size,ESYMBOLTYPE_STRUCTURE);

	int dest_index=0;
	for (int i=0; i<m_num_combo_sets; ++i)
	{
		Script::CStruct *p_struct=p_predefined_key_combos->GetStructure(mp_combo_sets[i].mSetIndex);
		Script::CArray *p_key_combos=NULL;
		p_struct->GetArray(CRCD(0x79704516,"key_combos"),&p_key_combos);
		Dbg_MsgAssert(p_key_combos,("Missing key_combos array in element %d of array cag_skatetris_key_combos",i));
		
		for (uint32 j=0; j<p_key_combos->GetSize(); ++j)
		{
			Script::CStruct *p_entry=new Script::CStruct;

			if (p_key_combos->GetType()==ESYMBOLTYPE_NAME)
			{
				p_entry->AddChecksum(CRCD(0xacfdb27a,"key_combo"),p_key_combos->GetChecksum(j));
			}
			else
			{
				p_entry->AppendStructure(p_key_combos->GetStructure(j));
			}
			
			// Overlay any extra params the user specified for this set.
			if (mp_combo_sets[i].mNumTaps)
			{
				p_entry->AddInteger(CRCD(0xa4bee6a1,"num_taps"),mp_combo_sets[i].mNumTaps);
			}
			if (mp_combo_sets[i].mRequirePerfect)
			{
				p_entry->AddChecksum(NONAME,CRCD(0x687e2c25,"require_perfect"));
			}
			//if (mp_combo_sets[i].mSpin)
			//{
			//	p_entry->AddInteger(CRCD(0x8c8abd19,"random_spin"),mp_combo_sets[i].mSpin*180);
			//}
			
			// Originally each set could have it's own spin, but it was decided that that was not
			// necessary and that instead a single spin value would be specified for the whole goal.
			if (m_spin)
			{
				// random_spin means the goal manager will actually choose a random spin up to the m_spin value.
				p_entry->AddInteger(CRCD(0x8c8abd19,"random_spin"),m_spin);
			}
				
			p_goal_tetris_key_combos->SetStructure(dest_index++,p_entry);
		}
	}
		
	p_info->AddArrayPointer(CRCD(0x7be1e689,"goal_tetris_key_combos"),p_goal_tetris_key_combos);
}

// Used for writing the mp_combo_sets array to mem card.
// Also used in GetDebugInfo
void CEditGoal::write_combo_sets(Script::CStruct *p_info)
{
	if (!m_num_combo_sets)
	{
		return;
	}
		
	Script::CArray *p_array=new Script::CArray;
	p_array->SetSizeAndType(m_num_combo_sets,ESYMBOLTYPE_STRUCTURE);
	for (int i=0; i<m_num_combo_sets; ++i)
	{
		Script::CStruct *p_struct=new Script::CStruct;
		
		p_struct->AddInteger(CRCD(0xeb7d6498,"set_index"),mp_combo_sets[i].mSetIndex);
		if (mp_combo_sets[i].mRequirePerfect)
		{
			p_struct->AddChecksum(NONAME,CRCD(0x687e2c25,"require_perfect"));
		}
		p_struct->AddInteger(CRCD(0xedf5db70,"spin"),mp_combo_sets[i].mSpin*180);
		p_struct->AddInteger(CRCD(0xa4bee6a1,"num_taps"),mp_combo_sets[i].mNumTaps);
		
		p_array->SetStructure(i,p_struct);
	}	
	p_info->AddArrayPointer(CRCD(0x490f171,"combo_sets"),p_array);
}

void CEditGoal::read_combo_sets(Script::CStruct *p_info)
{
	Script::CArray *p_array=NULL;
	if (!p_info->GetArray(CRCD(0x490f171,"combo_sets"),&p_array))
	{
		return;
	}
		
	m_num_combo_sets=p_array->GetSize();
	Dbg_MsgAssert(m_num_combo_sets<=MAX_COMBO_SETS,("m_num_combo_sets of %d too big",m_num_combo_sets));
	
	for (int i=0; i<m_num_combo_sets; ++i)
	{
		Script::CStruct *p_struct=p_array->GetStructure(i);
		
		int int_val=0;
		p_struct->GetInteger(CRCD(0xeb7d6498,"set_index"),&int_val);
		mp_combo_sets[i].mSetIndex=int_val;
		
		int_val=0;
		p_struct->GetInteger(CRCD(0xedf5db70,"spin"),&int_val);
		Dbg_MsgAssert((int_val%180)==0,("Bad spin of %d",int_val));
		mp_combo_sets[i].mSpin=int_val/180;
		
		int_val=0;
		p_struct->GetInteger(CRCD(0xa4bee6a1,"num_taps"),&int_val);
		mp_combo_sets[i].mNumTaps=int_val;
		
		mp_combo_sets[i].mRequirePerfect=p_struct->ContainsFlag(CRCD(0x687e2c25,"require_perfect"));
	}	
}

// The dest parameter determines the format in which the combo sets are written out.
// When generating params for the goal manager, the full goal_tetris_key_combos array is
// generated. When writing to mem card the chosen pre-defined set numbers are written instead.
void CEditGoal::write_skatetris_params(Script::CStruct *p_info, EWriteDest dest)
{
	p_info->AddInteger(CRCD(0x1181b29,"acceleration_interval"),m_acceleration_interval);
	p_info->AddFloat(CRCD(0x76f65c8,"acceleration_percent"),m_acceleration_percent);
	p_info->AddInteger(CRCD(0xded8540a,"trick_time"),m_trick_time);
	p_info->AddInteger(CRCD(0xa6d96603,"time_to_stop_adding_tricks"),m_time_to_stop_adding_tricks);
	p_info->AddInteger(CRCD(0x89473db7,"max_tricks"),m_max_tricks);
	
	if (dest==WRITING_TO_GOAL_MANAGER)
	{
		// When writing params out for the goal manager, the m_spin value is given to all of
		// the key combos.
		generate_key_combos_array(p_info);
	}
	else
	{
		p_info->AddInteger(CRCD(0xedf5db70,"spin"),m_spin);
		write_combo_sets(p_info);
	}
}

void CEditGoal::read_skatetris_params(Script::CStruct *p_info)
{
	p_info->GetInteger(CRCD(0x1181b29,"acceleration_interval"),&m_acceleration_interval);
	p_info->GetFloat(CRCD(0x76f65c8,"acceleration_percent"),&m_acceleration_percent);
	p_info->GetInteger(CRCD(0xded8540a,"trick_time"),&m_trick_time);
	p_info->GetInteger(CRCD(0xa6d96603,"time_to_stop_adding_tricks"),&m_time_to_stop_adding_tricks);
	p_info->GetInteger(CRCD(0x89473db7,"max_tricks"),&m_max_tricks);
	p_info->GetInteger(CRCD(0xedf5db70,"spin"),&m_spin);
	
	read_combo_sets(p_info);
}

// The dest parameter determines the format in which the combo sets are written out.
// When generating params for the goal manager, the full goal_tetris_key_combos array is
// generated. When writing to mem card the chosen pre-defined set numbers are written instead.
void CEditGoal::WriteGoalSpecificParams(Script::CStruct *p_info, EWriteDest dest)
{
	switch (m_goal_type)
	{
		case 0xea7ba666: // TrickTris
			p_info->AddChecksum(NONAME,CRCD(0xea7ba666,"tricktris"));
			p_info->AddInteger(CRCD(0x45a375c9,"tricktris_block_size"),m_tricktris_block_size);
			p_info->AddInteger(CRCD(0xbfd3a831,"tricktris_total_to_win"),m_tricktris_total_to_win);
			// Needed otherwise the goal will fail straight away for block size 13, total to win 12 (TT5933)
			m_max_tricks=m_tricktris_block_size;
			write_skatetris_params(p_info,dest);
			break;
			
		case 0x0fc6f698: // ComboSkateTris
			p_info->AddChecksum(NONAME,CRCD(0x4ec3cfb5,"combo"));
			p_info->AddInteger(CRCD(0x67dd90ca,"combo_size"),m_combo_size);
			if (m_single_combo)
			{
				p_info->AddChecksum(NONAME,CRCD(0xdf47b144,"single_combo"));
				
				// Needed otherwise the goal will fail straight away for a single combo size of 12 (TT7652)
				m_max_tricks=m_combo_size;
			}
			write_skatetris_params(p_info,dest);
			break;	
			
		case 0x7fe2238a: // SkateTris
			write_skatetris_params(p_info,dest);
			break;
			
		case 0x6fe44c6d: // HighScore
		case 0xec414b76: // HighCombo
		{
			p_info->AddInteger(CRCD(0xcd66c8ae,"score"),m_score);
			if (dest==WRITING_TO_GOAL_MANAGER)
			{
				const char *p_text=NULL;
				if (m_goal_type==CRCD(0xec414b76,"HighCombo"))
				{
					p_text=Script::GetString(CRCD(0x3693bfe4,"edited_high_combo_goal_text"));
				}
				else
				{
					p_text=Script::GetString(CRCD(0xd6efb1b5,"edited_high_score_goal_text"));
				}	
				char p_temp[100];
				sprintf(p_temp,p_text,m_score);
				p_info->AddString(CRCD(0xda441d9a,"goal_text"),p_temp);
			}
			break;
		}

		case 0x61c5d092: // Gap
		{
			p_info->AddString(CRCD(0x730844a3,"required_trick_name"),mp_trick_name);
			
			if (dest==WRITING_TO_GOAL_MANAGER)
			{
				Script::CArray *p_goal_flags=new Script::CArray;
				p_goal_flags->SetSizeAndType(m_num_gaps,ESYMBOLTYPE_NAME);
				char p_foo[20];
				for (int i=0; i<m_num_gaps; ++i)
				{
					sprintf(p_foo,"got_%d",i+1);
					p_goal_flags->SetChecksum(i,Script::GenerateCRC(p_foo));
				}	
				p_info->AddArrayPointer(CRCD(0xcc3c4cc4,"goal_flags"),p_goal_flags);

				
				Script::CArray *p_required_gaps=new Script::CArray;
				p_required_gaps->SetSizeAndType(m_num_gaps,ESYMBOLTYPE_INTEGER);
				
				for (int i=0; i<m_num_gaps; ++i)
				{
					p_required_gaps->SetInteger(i,mp_gap_numbers[i]);
				}	
				p_info->AddArrayPointer(CRCD(0x52d4489e,"required_gaps"),p_required_gaps);
			}
			else
			{
				if (m_num_gaps)
				{
					Script::CArray *p_gaps=new Script::CArray;
					p_gaps->SetSizeAndType(m_num_gaps,ESYMBOLTYPE_INTEGER);
					for (int i=0; i<m_num_gaps; ++i)
					{
						p_gaps->SetInteger(i,mp_gap_numbers[i]);
					}	
					p_info->AddArrayPointer(CRCD(0xd76c173e,"Gaps"),p_gaps);
				}	
			}
			break;
		}
			
		default:
			break;
	}			
}

void CEditGoal::ReadGoalSpecificParams(Script::CStruct *p_params)
{
	switch (m_goal_type)
	{
		case 0xea7ba666: // TrickTris
			p_params->GetInteger(CRCD(0x45a375c9,"tricktris_block_size"),&m_tricktris_block_size);
			p_params->GetInteger(CRCD(0xbfd3a831,"tricktris_total_to_win"),&m_tricktris_total_to_win);
			read_skatetris_params(p_params);
			break;
			
		case 0x0fc6f698: // ComboSkateTris
			p_params->GetInteger(CRCD(0x67dd90ca,"combo_size"),&m_combo_size);
			
			// Note: This only sets the m_single_combo if the flag single_combo is specified,
			// if single_combo is not specified it will leave m_single_combo the way it was.
			// Need this otherwise m_single_combo will get switched off if ReadGoalSpecificParams
			// is used to just set combo_size.
			if (p_params->ContainsFlag(CRCD(0xdf47b144,"single_combo")))
			{
				m_single_combo=true;
			}	
			
			read_skatetris_params(p_params);
			break;
			
		case 0x7fe2238a: // SkateTris
			read_skatetris_params(p_params);
			break;
			
		case 0x6fe44c6d: // HighScore
		case 0xec414b76: // HighCombo
		{
			p_params->GetInteger(CRCD(0xcd66c8ae,"score"),&m_score);
			break;
		}
			
		case 0x61c5d092: // Gap
		{
			const char *p_trick_name=NULL;
			if (p_params->GetString(CRCD(0x730844a3,"required_trick_name"),&p_trick_name))
			{
				Dbg_MsgAssert(strlen(p_trick_name)<=MAX_GAP_TRICK_NAME_CHARS,("trick_name '%s' too long",p_trick_name));
				strcpy(mp_trick_name,p_trick_name);
			}
			
			Script::CArray *p_gaps=NULL;
			p_params->GetArray(CRCD(0xd76c173e,"Gaps"),&p_gaps);
			if (p_gaps)
			{
				Dbg_MsgAssert(p_gaps->GetSize()<=MAX_GAPS,("Gaps array too big, max is %d",MAX_GAPS));
				m_num_gaps=p_gaps->GetSize();
				for (int i=0; i<m_num_gaps; ++i)
				{
					mp_gap_numbers[i]=p_gaps->GetInteger(i);
				}
			}		
			break;
		}
			
		default:
			break;
	}			
}

void CEditGoal::RemoveGoalSpecificFlag(Script::CStruct *p_params)
{
	uint32 flag=0;
	p_params->GetChecksum(NONAME,&flag);
	
	switch (m_goal_type)
	{
		case 0x0fc6f698: // ComboSkateTris
			if (flag==CRCD(0xdf47b144,"single_combo"))
			{
				m_single_combo=false;
			}	
			break;
			
		default:
			break;
	}			
}
	
void CEditGoal::EditGoal()
{
//	Dbg_MsgAssert(m_num_positions_set == m_num_positions_required,("Tried to edit a goal that was not fully defined originally"));
	// Note: The index starts at 1, because the cursor will initially be at index 0, and the cursor object is
	// not a marker object.
	for (int index=1; index<m_num_positions_set; ++index)
	{
		create_marker(index);
	}
	m_current_position_index=0;
	
	m_has_won_goal=false;
}

bool CEditGoal::GotAllPositions()
{
	return m_num_positions_set == m_num_positions_required;
}

bool CEditGoal::SetPosition(const Mth::Vector& pos, float height, float angle)
{
	m_placed_last_position=false;

	if (!position_ok_to_place(pos,height))
	{
		return false;
	}	

	Dbg_MsgAssert(m_current_position_index<MAX_POSITIONS_PER_GOAL,("Bad m_current_position_index"));

	mp_item_positions[m_current_position_index].mPos=pos;
	mp_item_positions[m_current_position_index].mHeight=height;
	mp_item_positions[m_current_position_index].mAngle=angle;
	
	if (m_current_position_index == m_num_positions_required-1)
	{
		m_placed_last_position=true;
	}	
	if (m_current_position_index==0)
	{
		// Just placed the ped position, so update the helper text so as to have the 'back' option
		Script::RunScript(CRCD(0xd066a889,"create_cag_helper_text"));
	}
		
	create_marker(m_current_position_index);
	
	++m_current_position_index;
	
	delete_marker(m_current_position_index);
	
	// Update m_num_positions_set, which is the total number of valid positions set so far.
	if (m_current_position_index >= m_num_positions_set)
	{
		m_num_positions_set=m_current_position_index;
		if (m_num_positions_set == m_num_positions_required)
		{
			m_current_position_index=m_num_positions_required-1;
			return true;
		}	
		
		// Make the new position default to the last position
		mp_item_positions[m_current_position_index].mPos=pos;
		mp_item_positions[m_current_position_index].mHeight=height;
		mp_item_positions[m_current_position_index].mAngle=angle;
		
		// When moving on to the first letter of the skate or combo line, add in a tweak to 
		// the height so that the letter does not appear initially half way through the ground.
		if ((m_goal_type==CRCD(0x4ec3cfb5,"Combo") || m_goal_type==CRCD(0x54166acd,"Skate")) &&  
			 m_current_position_index == 2 )
		{
			mp_item_positions[m_current_position_index].mHeight+=Script::GetFloat(CRCD(0x82a694e,"GoalEditor_LetterHeight"));
		}
	}	
		
	return true;
}

bool CEditGoal::GetPosition(Mth::Vector *p_pos, float *p_height, float *p_angle)
{
	*p_pos=mp_item_positions[m_current_position_index].mPos;
	*p_height=mp_item_positions[m_current_position_index].mHeight;
	*p_angle=mp_item_positions[m_current_position_index].mAngle;
	return true;
}

void CEditGoal::GetPosition(Mth::Vector *p_pos, int index)
{
	Dbg_MsgAssert(m_num_positions_set > index,("Called GetPedPosition with index=%d but m_num_positions_set=%d",index,m_num_positions_set));
	*p_pos=mp_item_positions[index].mPos;
	(*p_pos)[Y]+=mp_item_positions[index].mHeight;
}

CGoalPos::EType CEditGoal::GetPositionType()
{
	if (m_num_positions_set)
	{
		return mp_item_positions[m_current_position_index].mType;
	}
	return CGoalPos::PED;
}

bool CEditGoal::BackUp(const Mth::Vector& pos, float height, float angle)
{
	// Can't back up if at the first position
	if (m_current_position_index == 0)
	{
		return false;
	}

	if (!position_ok_to_place(pos,height))
	{
		return false;
	}
		
	// First, set the position as though X had been pressed, so that a marker is dropped at the current
	// position which can be got back to by pressing X again.
	Dbg_MsgAssert(m_current_position_index<MAX_POSITIONS_PER_GOAL,("Bad m_current_position_index"));

	mp_item_positions[m_current_position_index].mPos=pos;
	mp_item_positions[m_current_position_index].mHeight=height;
	mp_item_positions[m_current_position_index].mAngle=angle;

	create_marker(m_current_position_index);
		
	if (m_current_position_index==m_num_positions_set)
	{
		++m_num_positions_set;
	}
	
	// Then decrement the position index, and delete the marker at that position.
	// The cursor will then appear there on the next call to refresh_cursor_object
	Dbg_MsgAssert(m_current_position_index,("Zero m_current_position_index ?"));
	--m_current_position_index;
	
	delete_marker(m_current_position_index);

	if (m_current_position_index == 0)
	{
		// Returned to the ped position, so update the helper text so as to not have the 'back' option
		Script::CStruct *p_params=new Script::CStruct;
		p_params->AddChecksum(NONAME,CRCD(0x7f1538e0,"PedPosition"));
		Script::RunScript(CRCD(0xd066a889,"create_cag_helper_text"), p_params);
		delete p_params;
	}	

	return true;	
}

uint CEditGoal::GetCurrentLetterIndex()
{
	uint index=0;
	
	switch (m_goal_type)
	{
		case 0x4ec3cfb5: // Combo
		case 0x54166acd: // Skate
		{
			Dbg_MsgAssert(m_current_position_index>=2,("Current position is not that of a letter"));
			index=m_current_position_index-2;
			break;
		}
		default:
			Dbg_MsgAssert(0,("Called GetCurrentLetterIndex() when goal type is %s\n",Script::FindChecksumName(m_goal_type)));
			break;
	}
	
	return index;
}

void CEditGoal::SetType(uint32 type)
{
	m_goal_type=type;
	
	mp_item_positions[0].mType=CGoalPos::PED;
	mp_item_positions[0].mHeight=0.0f;
	mp_item_positions[1].mType=CGoalPos::RESTART;
	mp_item_positions[1].mHeight=0.0f;
	
	const Script::CStruct *p_default_params=s_get_default_goal_params(m_goal_type);
	
	switch (m_goal_type)
	{
		case 0x54166acd: // Skate
		{
			m_num_positions_required=7;
			
			for (int i=2; i<MAX_POSITIONS_PER_GOAL; ++i)
			{
				mp_item_positions[i].mType=CGoalPos::LETTER;
				mp_item_positions[i].mHeight=0.0f;
			}
			break;
		}	
		case 0x4ec3cfb5: // Combo
		{
			m_num_positions_required=7;
			
			for (int i=2; i<MAX_POSITIONS_PER_GOAL; ++i)
			{
				mp_item_positions[i].mType=CGoalPos::LETTER;
				mp_item_positions[i].mHeight=0.0f;
			}
			break;
		}	

		case 0x6fe44c6d: // HighScore
		case 0xec414b76: // HighCombo
		case 0x7fe2238a: // SkateTris
		case 0x0fc6f698: // ComboSkateTris
		case 0xea7ba666: // TrickTris
		case 0x61c5d092: // Gap
		{
			m_num_positions_required=2;
			break;
		}
		
		default:
			break;
	}			
	
	if (p_default_params)
	{
		p_default_params->GetInteger(CRCD(0xc51bf6e3,"time_limit"),&m_time_limit);
		p_default_params->GetInteger(CRCD(0xcd66c8ae,"score"),&m_score);
		
		const char *p_string="";
		p_default_params->GetString(CRCD(0x4bc5229d,"view_goals_text"),&p_string);
		Dbg_MsgAssert(strlen(p_string)<GOAL_NAME_BUFFER_SIZE,("goal name '%s' too long",p_string));
		strcpy(mp_goal_name,p_string);
	}	
}

void CEditGoal::SetGoalDescription(const char *p_text)
{
	Dbg_MsgAssert(p_text,("NULL p_text ?"));
	Dbg_MsgAssert(strlen(p_text)<GOAL_DESCRIPTION_BUFFER_SIZE,("Goal description too long!"));
	strcpy(mp_goal_description,p_text);
}

void CEditGoal::SetWinMessage(const char *p_text)
{
	Dbg_MsgAssert(p_text,("NULL p_text ?"));
	Dbg_MsgAssert(strlen(p_text)<GOAL_WIN_MESSAGE_BUFFER_SIZE,("Win message too long!"));
	strcpy(mp_win_message,p_text);
}

void CEditGoal::SetGoalName(const char *p_name)
{
	Dbg_MsgAssert(p_name,("NULL p_name ?"));
	Dbg_MsgAssert(strlen(p_name)<GOAL_NAME_BUFFER_SIZE,("Goal name too long!"));
	strcpy(mp_goal_name,p_name);
}

void CEditGoal::SetPedName(const char *p_name)
{
	Dbg_MsgAssert(p_name,("NULL p_name ?"));
	Dbg_MsgAssert(strlen(p_name)<PED_NAME_BUFFER_SIZE,("Goal name too long!"));
	strcpy(mp_ped_name,p_name);
}

void CEditGoal::SetTimeLimit(int timeLimit)
{
	m_time_limit=timeLimit;
	
	switch (m_goal_type)
	{
		case 0x7fe2238a: // SkateTris
		case 0x0fc6f698: // ComboSkateTris
		case 0xea7ba666: // TrickTris
			// Make sure that m_time_to_stop_adding_tricks is valid.
			if (m_time_to_stop_adding_tricks > m_time_limit)
			{
				m_time_to_stop_adding_tricks=m_time_limit;
			}
			break;
			
		default:
			break;
	}		
}

void CEditGoal::SetControlType(uint32 controlType)
{
	m_control_type=controlType;
}

void CEditGoal::AddGoalToGoalManager(bool markUnbeaten)
{
	Script::CStruct *p_params=new Script::CStruct;
	add_goal_params(p_params);
	
	Game::CGoalManager* p_goal_manager = Game::GetGoalManager();
	Dbg_MsgAssert(p_goal_manager,("NULL p_goal_manager ?"));
	p_goal_manager->AddGoal(GetId(),p_params);

	Game::CGoal *p_goal=p_goal_manager->GetGoal(GetId());
	// Mark it as seen so that it is selectable in the view-goals menu
	p_goal->SetHasSeen();		

	if (markUnbeaten)
	{
		m_has_won_goal=false;
	}
		
	if (m_has_won_goal)
	{
		p_goal->MarkBeaten();
	}	
	
	delete p_params;
}

void CEditGoal::write_position_into_node(uint index, uint32 nodeName)
{
	Dbg_MsgAssert(index<m_num_positions_set,("Bad index of %d",index));

	Script::CStruct *p_node=SkateScript::GetNode(SkateScript::FindNamedNode(nodeName));
	p_node->AddVector(CRCD(0x7f261953,"Pos"),	mp_item_positions[index].mPos.GetX(),
												mp_item_positions[index].mPos.GetY()+mp_item_positions[index].mHeight,
												mp_item_positions[index].mPos.GetZ());
	p_node->AddVector(CRCD(0x9d2d0915,"Angles"),0,
												mp_item_positions[index].mAngle-3.141592654f,
												0);
												
	// For letter nodes, the type and model filename need to be added.
	if (index >=2 )
	{
		p_node->AddChecksum(CRCD(0x7321a8d6,"Type"),s_generate_letter_node_type_checksum(m_goal_type,index-2));
		p_node->AddString(CRCD(0x286a8d26,"Model"),s_generate_letter_model_filename(m_goal_type,index-2));
	}	
}

void CEditGoal::WriteNodePositions()
{
	for (int i=0; i<m_num_positions_set; ++i)
	{
		write_position_into_node(i,s_generate_node_name_checksum(m_level_goal_index,i));
	}	
}

bool CEditGoal::GoalIsOutsideArea(float x0, float z0, float x1, float z1)
{
	if (m_num_positions_set > 1)
	{
		Dbg_MsgAssert(mp_item_positions[1].mType==CGoalPos::RESTART,("Strange type for goal position 1"));
		float x=mp_item_positions[1].mPos.GetX();
		float z=mp_item_positions[1].mPos.GetZ();
		
		if (x < x0 || x > x1 || z < z0 || z > z1)
		{
			return true;
		}
	}		
	return false;
}

void CEditGoal::GetDebugInfo(Script::CStruct* p_info)
{
#ifdef	__DEBUG_CODE__
	WriteIntoStructure(p_info);
#endif				 
}

#ifdef SIZE_TEST	
void CEditGoal::FillNameBuffers()
{
	int i;
	for (i=0; i<MAX_GAP_TRICK_NAME_CHARS; ++i)
	{
		mp_trick_name[i]='?';
	}
	mp_trick_name[MAX_GAP_TRICK_NAME_CHARS]=0;
	
	for (i=0; i<GOAL_NAME_BUFFER_SIZE-1; ++i)
	{
		mp_goal_name[i]='?';
	}
	mp_goal_name[GOAL_NAME_BUFFER_SIZE-1]=0;

	for (i=0; i<PED_NAME_BUFFER_SIZE-1; ++i)
	{
		mp_ped_name[i]='?';
	}
	mp_ped_name[PED_NAME_BUFFER_SIZE-1]=0;
	
	for (i=0; i<GOAL_DESCRIPTION_BUFFER_SIZE-1; ++i)
	{
		mp_goal_description[i]='?';
	}
	mp_goal_description[GOAL_DESCRIPTION_BUFFER_SIZE-1]=0;
	
	for (i=0; i<GOAL_WIN_MESSAGE_BUFFER_SIZE-1; ++i)
	{
		mp_win_message[i]='?';
	}
	mp_win_message[GOAL_WIN_MESSAGE_BUFFER_SIZE-1]=0;
}

uint32 CEditGoal::GetSkateGoalMaxSize()
{
	Clear();
	SetType(CRCD(0x54166acd,"Skate"));
	Mth::Vector pos;
	SetPosition(pos,0.0f,0.0f);
	pos[X]+=200.0f;
	SetPosition(pos,0.0f,0.0f);
	pos[X]+=200.0f;
	SetPosition(pos,0.0f,0.0f);
	pos[X]+=200.0f;
	SetPosition(pos,0.0f,0.0f);
	pos[X]+=200.0f;
	SetPosition(pos,0.0f,0.0f);
	pos[X]+=200.0f;
	SetPosition(pos,0.0f,0.0f);
	pos[X]+=200.0f;
	SetPosition(pos,0.0f,0.0f);
	Dbg_MsgAssert(GotAllPositions(),("Eh?"));
	FillNameBuffers();
	
	Script::CStruct *p_struct=new Script::CStruct;
	WriteIntoStructure(p_struct);
	
	uint8 *p_temp=(uint8*)Mem::Malloc(10000);
	uint32 size=Script::WriteToBuffer(p_struct,p_temp,10000);
	Mem::Free(p_temp);
	
	delete p_struct;
	
	Clear();
	return size;
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// s_create is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
// s_create	returns a CBaseComponent*, as it is to be used
// by factor creation schemes that do not care what type of
// component is being created
// **  after you've finished creating this component, be sure to
// **  add it to the list of registered functions in the
// **  CCompositeObjectManager constructor  

CBaseComponent* CGoalEditorComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CGoalEditorComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// All components set their type, which is a unique 32-bit number
// (the CRC of their name), which is used to identify the component	
CGoalEditorComponent::CGoalEditorComponent() : CBaseComponent()
{
	SetType( CRC_GOALEDITOR );
	
	ClearAllExceptParkGoals();
	ClearOnlyParkGoals();
	mp_input_component=NULL;
	mp_editor_camera_component=NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CGoalEditorComponent::~CGoalEditorComponent()
{
	Script::RunScript("goal_editor_destroy_cursor");
}

// For writing to memcard
void CGoalEditorComponent::WriteIntoStructure(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info"));
		
	int goal_count=0;
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		// Do not count goals for the park, since those are stored in the park file.
		if (mp_goals[i].Used() && mp_goals[i].GetLevel() != CRCD(0xe8b4b836,"Load_Sk5Ed"))
		{
			++goal_count;
		}
	}		
	if (!goal_count)
	{
		return;
	}
		
	Script::CArray *p_goals_array=new Script::CArray;
	p_goals_array->SetSizeAndType(goal_count,ESYMBOLTYPE_STRUCTURE);
	int goal_index=0;
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		// Does not write out the goals for the park, since those are stored in the park file.
		if (mp_goals[i].Used() && mp_goals[i].GetLevel() != CRCD(0xe8b4b836,"Load_Sk5Ed"))
		{
			Script::CStruct *p_goal_info=new Script::CStruct;
			mp_goals[i].WriteIntoStructure(p_goal_info);
			p_goals_array->SetStructure(goal_index++,p_goal_info);
		}	
	}
	Dbg_MsgAssert(goal_index==goal_count,("Bad goal_index ?"));
	p_info->AddArrayPointer(CRCD(0x38dbe1d0,"Goals"),p_goals_array);
}

// This just writes in the goals for a particular level. Used when sending goal info
// across the network for network goal attack games.
void CGoalEditorComponent::WriteIntoStructure(uint32 levelScript, Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info"));
	
	int num_goals=CountGoalsForLevel(levelScript);
			
	Script::CArray *p_goals_array=new Script::CArray;
	p_goals_array->SetSizeAndType(num_goals,ESYMBOLTYPE_STRUCTURE);
	int goal_index=0;
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && mp_goals[i].GetLevel()==levelScript)
		{
			Script::CStruct *p_goal_info=new Script::CStruct;
			mp_goals[i].WriteIntoStructure(p_goal_info);
			p_goals_array->SetStructure(goal_index++,p_goal_info);
		}	
	}
	Dbg_MsgAssert(goal_index==num_goals,("Bad num_goals ?"));
	
	p_info->AddArrayPointer(CRCD(0x38dbe1d0,"Goals"),p_goals_array);
}

// Writes out the goals for a particular level into a uint8* buffer.
// Used when sending goal info across the network for network goal attack games.
uint32 CGoalEditorComponent::WriteToBuffer(uint32 levelScript, uint8 *p_buffer, uint32 bufferSize)
{
	Dbg_MsgAssert(p_buffer,("NULL p_buffer"));
	
	// Warning! It's feasible that we could run out of memory here ... 
	// The structure could contain around 6K of stuff.	
	Script::CStruct *p_struct=new Script::CStruct;
	WriteIntoStructure( s_convert_level( levelScript ), p_struct);
	uint32 bytes_written=Script::WriteToBuffer(p_struct, p_buffer, bufferSize);
	delete p_struct;
	
	return bytes_written;
}

uint8 *CGoalEditorComponent::ReadFromBuffer(uint32 levelScript, uint8 *p_buffer)
{
	Dbg_MsgAssert(p_buffer,("NULL p_buffer"));

	levelScript=s_convert_level(levelScript);
	
	Script::CStruct *p_struct=new Script::CStruct;
	uint8 *p_end=Script::ReadFromBuffer(p_struct, p_buffer);
	
	// Run through all the goals clearing any that exist for this level.
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && mp_goals[i].GetLevel()==levelScript)
		{
			mp_goals[i].Clear();
		}
	}	
	// Clear this just in case it was one of those that got cleared.
	mp_current_goal=NULL;
	
	Script::CArray *p_goals_array=NULL;
	p_struct->GetArray(CRCD(0x38dbe1d0,"Goals"),&p_goals_array,Script::ASSERT);

	uint32 array_index=0;
	int num_goals_left_to_load=p_goals_array->GetSize();
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (!num_goals_left_to_load)
		{
			break;
		}	
		if (!mp_goals[i].Used())
		{
			mp_goals[i].ReadFromStructure(p_goals_array->GetStructure(array_index++));
			mp_goals[i].SetLevel(levelScript);
			--num_goals_left_to_load;
		}
	}
	Dbg_MsgAssert(num_goals_left_to_load == 0,("Could not load all the goals"));
	
	delete p_struct;
	return p_end;
}

// For reading from memcard
void CGoalEditorComponent::ReadFromStructure(Script::CStruct *p_info, EBoolLoadingParkGoals loadingParkGoals)
{
	Dbg_MsgAssert(p_info,("NULL p_info"));

	if (loadingParkGoals)
	{
		ClearOnlyParkGoals();
	}
	else
	{
		ClearAllExceptParkGoals();
	}
	
	Script::CArray *p_goals_array=NULL;
	p_info->GetArray(CRCD(0x38dbe1d0,"Goals"),&p_goals_array);
	if (!p_goals_array)
	{
		// No Goals array means no goals.
		return;
	}
	
	uint32 array_size=p_goals_array->GetSize();

	for (uint32 array_index=0; array_index<array_size; ++array_index)
	{
		Script::CStruct *p_struct=p_goals_array->GetStructure(array_index);
		if ( (loadingParkGoals && s_get_level_checksum(p_struct) == CRCD(0xe8b4b836,"Load_Sk5Ed")) ||
			 (!loadingParkGoals && s_get_level_checksum(p_struct) != CRCD(0xe8b4b836,"Load_Sk5Ed")))
		{
			for (int i=0; i<MAX_GOALS_TOTAL; ++i)
			{
				if (!mp_goals[i].Used())
				{
					mp_goals[i].ReadFromStructure(p_struct);
					break;
				}
			}				
		}	
	}
	
	mp_current_goal=NULL;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CGoalEditorComponent::InitFromStructure( Script::CStruct* pParams )
{
	// ** Add code to parse the structure, and initialize the component

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// RefreshFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CGoalEditorComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Default to just calline InitFromStructure()
	// but if that does not handle it, then will need to write a specific 
	// function here. 
	// The user might only want to update a single field in the structure
	// and we don't want to be asserting becasue everything is missing 
	
	InitFromStructure(pParams);


}

void CGoalEditorComponent::Hide( bool shouldHide )
{
	if (shouldHide)
	{
		Script::RunScript("goal_editor_destroy_cursor");
	}	
	else
	{
		// Make sure we're using the skater's pad (TT10311)
		Script::CStruct *p_params=new Script::CStruct;
		p_params->AddInteger(CRCD(0x67e6859a,"player"),0);
		mp_input_component->InitFromStructure(p_params);
		delete p_params;
		
		refresh_cursor_object();
	}
}

Obj::CCompositeObject *CGoalEditorComponent::get_cursor()
{
	return (Obj::CCompositeObject*)Obj::CTracker::Instance()->GetObject(CRCD(0x9211b125,"GoalEditorCursor"));
}

void CGoalEditorComponent::refresh_cursor_object()
{
	const char *p_model_filename="";

	Dbg_MsgAssert(mp_current_goal,("NULL mp_current_goal"));
	switch (mp_current_goal->GetPositionType())
	{
		case CGoalPos::RESTART:
			Dbg_MsgAssert(mp_current_goal,("NULL mp_current_goal?"));
			p_model_filename="gameobjects\\p1_cursor\\p1_cursor.mdl";
			break;
		case CGoalPos::LETTER:
			Dbg_MsgAssert(mp_current_goal,("NULL mp_current_goal?"));
			p_model_filename=s_generate_letter_model_filename(mp_current_goal->GetType(),
															  mp_current_goal->GetCurrentLetterIndex());
			break;
		default:
			p_model_filename="gameobjects\\goal_cursor\\goal_cursor.mdl";
			break;
	}
	
	Script::CStruct params;
	if (mp_current_goal->GetPositionType()==CGoalPos::PED)
	{
		params.AddChecksum(CRCD(0x7321a8d6,"Type"),CRCD(0x61a741e,"Ped"));
	}
	else
	{
		params.AddChecksum(CRCD(0x7321a8d6,"Type"),0);
		params.AddString(CRCD(0x286a8d26,"Model"),p_model_filename);
	}
	
	Script::RunScript(CRCD(0x36791e3c,"goal_editor_create_cursor"),&params,GetObject());
	
	// Choose whether the cursor should be allowed to be moved over kill polys.
	// Only the restart position is not allowed to, to allow letters to be placed over kill polys.
	update_cursor_collision_type();
}

void CGoalEditorComponent::get_pos_from_camera_component(Mth::Vector *p_pos, float *p_height, float *p_angle)
{
	Dbg_MsgAssert(mp_editor_camera_component,("NULL mp_editor_camera_component ?"));
	*p_pos=mp_editor_camera_component->GetCursorPos();
	*p_height=mp_editor_camera_component->GetCursorHeight();
	*p_angle=mp_editor_camera_component->GetCursorOrientation();
}

void CGoalEditorComponent::update_cursor_collision_type()
{
	Dbg_MsgAssert(mp_editor_camera_component,("NULL mp_editor_camera_component ?"));
	
	// Restart positions are not allowed to go over kill polys, otherwise the skater will
	// die straight away.
	// Note that the ped position also has to not be allowed to go over kill polys, otherwise
	// as soon as it is placed the restart cursor will be over the kill poly, and hence be stuck.
	if (mp_current_goal->GetPositionType()==CGoalPos::PED ||
		mp_current_goal->GetPositionType()==CGoalPos::RESTART)
	{
		Mdl::Skate * p_skate_mod =  Mdl::Skate::Instance();
		if (p_skate_mod->m_cur_level == CRCD(0xe8b4b836,"load_sk5ed") )
		{
			// In the park editor, allow the cursor to move anywhere, otherwise the cursor
			// will get stuck if initialised on a kill poly.
			mp_editor_camera_component->SetSimpleCollision(true);
		}
		else
		{
			mp_editor_camera_component->SetSimpleCollision(false);
		}	
	}
	else
	{
		mp_editor_camera_component->SetSimpleCollision(true);
	}
}

CEditGoal *CGoalEditorComponent::find_goal(const char *p_name)
{
	uint32 current_level=s_get_current_level();

	Dbg_MsgAssert(p_name,("NULL p_name ?"));
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && 
			mp_goals[i].GetLevel()==current_level && 
			stricmp(mp_goals[i].GetGoalName(),p_name)==0)
		{
			return &mp_goals[i];
		}
	}
	return NULL;		
}

CEditGoal *CGoalEditorComponent::find_goal(uint32 id)
{
	uint32 current_level=s_get_current_level();
	
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && 
			mp_goals[i].GetLevel()==current_level && 
			mp_goals[i].GetId()==id)
		{
			return &mp_goals[i];
		}
	}
	return NULL;		
}

CEditGoal *CGoalEditorComponent::find_goal(Script::CStruct *p_params)
{
	uint32 id=0;
	if (p_params->GetChecksum(CRCD(0x9982e501,"goal_id"),&id))
	{
		return find_goal(id);
	}
	
	const char *p_name=NULL;
	if (p_params->GetString(CRCD(0xbfecc45b,"goal_name"),&p_name))
	{
		return find_goal(p_name);
	}
	
	int index=0;
	if (p_params->GetInteger(CRCD(0x474a6a7f,"goal_index"),&index))
	{
		Dbg_MsgAssert(0,("find_goal called for an index"));
		
		uint32 current_level=s_get_current_level();
		
		for (int i=0; i<MAX_GOALS_TOTAL; ++i)
		{
			if (mp_goals[i].Used() && 
				mp_goals[i].GetLevel()==current_level && 
				mp_goals[i].GetLevelGoalIndex()==index)
			{
				return &mp_goals[i];
			}
		}
		return NULL;
	}
	
	return mp_current_goal;	
}

int	CGoalEditorComponent::GetNumGoals()
{
	int n=0;
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used())
		{
			++n;
		}
	}
	return n;
}

int CGoalEditorComponent::CountGoalsForLevel(uint32 level)
{
	int n=0;
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && (level==0 || mp_goals[i].GetLevel()==level) && mp_goals[i].GotAllPositions())
		{
			++n;
		}
	}
	return n;
}
	
void CGoalEditorComponent::update_camera_pos()
{
	Dbg_MsgAssert(mp_editor_camera_component,("NULL mp_editor_camera_component"));
	Dbg_MsgAssert(mp_current_goal,("NULL mp_current_goal"));
	
	Mth::Vector pos(0.0f,0.0f,0.0f);
	float height=0.0f;
	float angle=0.0f;
	mp_current_goal->GetPosition(&pos,&height,&angle);
	
	mp_editor_camera_component->SetCursorHeight(height);
	mp_editor_camera_component->SetCursorOrientation(angle);
	mp_editor_camera_component->SetCursorPos(pos);
}

void CGoalEditorComponent::update_cursor_position()
{
	Script::CStruct params;
	
	Mth::Vector cursor_pos(0.0f,0.0f,0.0f);		
	float cursor_height=0.0f;
	float cursor_angle=0.0f;
	get_pos_from_camera_component(&cursor_pos,&cursor_height,&cursor_angle);
	cursor_pos[Y]+=cursor_height;
	
	params.AddVector(CRCD(0x7f261953,"Pos"),cursor_pos[X],cursor_pos[Y],cursor_pos[Z]);
	params.AddFloat(CRCD(0xff7ebaf6,"Angle"),mp_editor_camera_component->GetCursorOrientation());
	
	Script::RunScript(CRCD(0xdea7fe56,"goal_editor_update_cursor_position"),&params);
}

void CGoalEditorComponent::remove_goal(CEditGoal *p_goal)
{
	Dbg_MsgAssert(p_goal,("NULL p_goal"));
	
	if (p_goal->Used())
	{
		Game::CGoalManager* p_goal_manager = Game::GetGoalManager();
		Dbg_MsgAssert(p_goal_manager,("NULL p_goal_manager ?"));
		
		if (p_goal_manager->GoalExists(p_goal->GetId()))
		{
			p_goal_manager->RemoveGoal(p_goal->GetId());
		}	
	}	
	p_goal->Clear();
	
	if (mp_current_goal==p_goal)
	{
		mp_current_goal=NULL;
	}
	if (GetNumGoals()==0)
	{
		Dbg_MsgAssert(mp_current_goal==NULL,("mp_current_goal not NULL ?"));
	}	
}

bool CGoalEditorComponent::too_close_to_another_goal_position(const Mth::Vector &pos, float height, uint32 level, int positionIndex)
{
	Mth::Vector pos_with_height=pos;
	pos_with_height[Y]+=height;
	
	float default_min_dist_allowable=Script::GetFloat(CRCD(0x71b20276,"GoalEditor_DefaultMinDistBetweenPositions"));
	
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && 
			mp_goals[i].GetLevel()==level &&
			mp_goals[i].GetNumPositionsSet() > positionIndex)
		{
			Mth::Vector p;
			mp_goals[i].GetPosition(&p, positionIndex);
			p-=pos_with_height;
			if (p.Length() < default_min_dist_allowable)
			{
				return true;
			}
		}
	}
			
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CGoalEditorComponent::Finalize()
{
	// Get the pointers to the other required components.
	
	Dbg_MsgAssert(mp_input_component==NULL,("mp_input_component not NULL ?"));
	mp_input_component = GetInputComponentFromObject(GetObject());
	Dbg_MsgAssert(mp_input_component,("CGoalEditorComponent requires parent object to have an input component!"));

	Dbg_MsgAssert(mp_editor_camera_component==NULL,("mp_editor_camera_component not NULL ?"));
	mp_editor_camera_component = GetEditorCameraComponentFromObject(GetObject());
	Dbg_MsgAssert(mp_editor_camera_component,("CGoalEditorComponent requires parent object to have an EditorCamera component!"));
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// The component's Update() function is called from the CCompositeObject's 
// Update() function.  That is called every game frame by the CCompositeObjectManager
// from the s_logic_code function that the CCompositeObjectManager registers
// with the task manger.
void CGoalEditorComponent::Update()
{
	if (mp_current_goal)
	{
		CControlPad& control_pad = mp_input_component->GetControlPad();
		if (control_pad.m_x.GetTriggered())
		{
			control_pad.m_x.ClearTrigger();
	
			Mth::Vector cursor_pos(0.0f,0.0f,0.0f);
			float cursor_height=0.0f;
			float cursor_angle=0.0f;
			get_pos_from_camera_component(&cursor_pos,&cursor_height,&cursor_angle);
			
			if ((mp_current_goal->GetPositionType() == CGoalPos::RESTART && 
				too_close_to_another_goal_position(cursor_pos,cursor_height,mp_current_goal->GetLevel(),0)) || // 0 = ped
				(mp_current_goal->GetPositionType() == CGoalPos::PED && 
				too_close_to_another_goal_position(cursor_pos,cursor_height,mp_current_goal->GetLevel(),1))) // 1 = restart
			{
				// Can't place restart positions too close to another goal's ped position, or
				// ped positions too close to another goal's restart position.
				Script::RunScript(CRCD(0xaebc7020,"goal_editor_play_placement_fail_sound"),NULL,GetObject());
			}
			else
			{
				if (mp_current_goal->SetPosition(cursor_pos,cursor_height,cursor_angle))
				{
					Script::RunScript(CRCD(0x718b071b,"goal_editor_play_placement_success_sound"),NULL,GetObject());
					// Once a position is set, the cursor model will need to change for the next position.
					refresh_cursor_object();
					update_camera_pos();
				}
				else
				{
					Script::RunScript(CRCD(0xaebc7020,"goal_editor_play_placement_fail_sound"),NULL,GetObject());
				}
			}	
			
			if (mp_current_goal->PlacedLastPosition())
			{
				Script::RunScript(CRCD(0x89119628,"goal_editor_finished_placing_letters"),NULL,GetObject());
			}
		}	
	
		if (control_pad.m_triangle.GetTriggered())
		{
			control_pad.m_triangle.ClearTrigger();
	
			Mth::Vector cursor_pos(0.0f,0.0f,0.0f);
			float cursor_height=0.0f;
			float cursor_angle=0.0f;
			get_pos_from_camera_component(&cursor_pos,&cursor_height,&cursor_angle);
	
			Dbg_MsgAssert(mp_current_goal,("NULL mp_current_goal"));
			if ((mp_current_goal->GetPositionType() == CGoalPos::RESTART && 
				too_close_to_another_goal_position(cursor_pos,cursor_height,mp_current_goal->GetLevel(),0)) || // 0 = ped
				(mp_current_goal->GetPositionType() == CGoalPos::PED && 
				too_close_to_another_goal_position(cursor_pos,cursor_height,mp_current_goal->GetLevel(),1))) // 1 = restart
			{
				// Can't place restart positions too close to another goal's ped position, or
				// ped positions too close to another goal's restart position.
				Script::RunScript(CRCD(0xade100d6,"goal_editor_play_backup_fail_sound"),NULL,GetObject());
			}
			else
			{
				if (mp_current_goal->BackUp(cursor_pos,cursor_height,cursor_angle))
				{
					Script::RunScript(CRCD(0xfa275cc5,"goal_editor_play_backup_success_sound"),NULL,GetObject());
					refresh_cursor_object();
					update_camera_pos();
				}	
				else
				{
					Script::RunScript(CRCD(0xade100d6,"goal_editor_play_backup_fail_sound"),NULL,GetObject());
				}			
			}	
		}	
	}
	
	update_cursor_position();
}

void CGoalEditorComponent::ClearAllExceptParkGoals()
{
	for (uint i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && mp_goals[i].GetLevel() != CRCD(0xe8b4b836,"Load_Sk5Ed"))
		{
			mp_goals[i].Clear();
		}	
	}
	
	mp_current_goal=NULL;
}	

void CGoalEditorComponent::ClearOnlyParkGoals()
{
	for (uint i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && mp_goals[i].GetLevel() == CRCD(0xe8b4b836,"Load_Sk5Ed"))
		{
			mp_goals[i].Clear();
		}	
	}	
	
	mp_current_goal=NULL;
}

void CGoalEditorComponent::CleanOutUnfinishedGoals()
{
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && !mp_goals[i].GotAllPositions())
		{
			mp_goals[i].Clear();
		}
	}
}

void CGoalEditorComponent::NewGoal()
{
	CleanOutUnfinishedGoals();

	uint32 current_level=s_get_current_level();
	
	int level_goal_index=0;
		
	while (true)
	{
		bool found_suitable_index=true;
		for (int i=0; i<MAX_GOALS_TOTAL; ++i)
		{
			if (mp_goals[i].Used() && 
				mp_goals[i].GotAllPositions() && 
				mp_goals[i].GetLevel()==current_level && 
				mp_goals[i].GetLevelGoalIndex()==level_goal_index)
			{
				++level_goal_index;
				found_suitable_index=false;
				break;
			}
		}	
		if (found_suitable_index)
		{
			break;
		}
	}		
	Dbg_MsgAssert(level_goal_index<MAX_GOALS_PER_LEVEL,("Bad level_goal_index!"));
	
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (!mp_goals[i].Used())
		{
			mp_current_goal=&mp_goals[i];
			mp_current_goal->Clear();
			mp_current_goal->SetUsedFlag(true);
		
			mp_current_goal->SetLevel(current_level);
			mp_current_goal->SetLevelGoalIndex(level_goal_index);
			
			refresh_cursor_object();
			return;
		}
	}
								
	Dbg_MsgAssert(0,("Exceeded CGoalEditorComponent::MAX_GOALS_TOTAL of %d",MAX_GOALS_TOTAL));
}

void CGoalEditorComponent::SetGoalType(uint32 type)
{
	Dbg_MsgAssert(mp_current_goal,("NULL mp_current_goal"));
	mp_current_goal->SetType(type);
}
	
bool CGoalEditorComponent::ThereAreGoalsOutsideArea(float x0, float z0, float x1, float z1)
{
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && mp_goals[i].GetLevel()==CRCD(0xe8b4b836,"Load_Sk5Ed"))
		{
			if (mp_goals[i].GoalIsOutsideArea(x0,z0,x1,z1))
			{
				return true;
			}	
		}
	}	
	return false;
}

void CGoalEditorComponent::DeleteGoalsOutsideArea(float x0, float z0, float x1, float z1)
{
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && 
			mp_goals[i].GetLevel()==CRCD(0xe8b4b836,"Load_Sk5Ed") &&
			mp_goals[i].GoalIsOutsideArea(x0,z0,x1,z1))
		{
			remove_goal(&mp_goals[i]);
		}
	}	
}

void CGoalEditorComponent::RefreshGoalPositionsUsingCollisionCheck()
{
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && 
			mp_goals[i].GetLevel()==CRCD(0xe8b4b836,"Load_Sk5Ed"))
		{
			mp_goals[i].RefreshPositionsUsingCollisionCheck();
		}
	}	
}

void CGoalEditorComponent::RefreshGapGoalsAfterGapRemovedFromPark(int gapNumber)
{
	for (int i=0; i<MAX_GOALS_TOTAL; ++i)
	{
		if (mp_goals[i].Used() && 
			mp_goals[i].GetLevel()==CRCD(0xe8b4b836,"Load_Sk5ed") && 
			mp_goals[i].GetType()==CRCD(0x61c5d092,"Gap"))
		{
			mp_goals[i].RemoveGapAndReorder(gapNumber);
		}	
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the "Checksum" of a script command, then possibly handle it
// if it's a command that this component will handle	
CBaseComponent::EMemberFunctionResult CGoalEditorComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | EditedGoalAddGap
		case 0xab710f3: // EditedGoalAddGap
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				int gap_number=0;
				pParams->GetInteger(CRCD(0x3ac9ab5e,"gap_number"),&gap_number);
				p_goal->AddGap(gap_number);
			}
			break;
		}

		// @script | EditedGoalRemoveGap
		case 0x5c155a29: // EditedGoalRemoveGap
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				int gap_number=0;
				pParams->GetInteger(CRCD(0x3ac9ab5e,"gap_number"),&gap_number);
				p_goal->RemoveGap(gap_number);
			}
			break;
		}

		// @script | EditedGoalGotGap
		case 0xfb279560: // EditedGoalGotGap
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				int gap_number=0;
				pParams->GetInteger(CRCD(0x3ac9ab5e,"gap_number"),&gap_number);
				if (p_goal->GotGap(gap_number))
				{
					return CBaseComponent::MF_TRUE;
				}	
			}
			return CBaseComponent::MF_FALSE;
			break;
		}
		
		// @script | AddKeyComboSet
		case 0xcca11c78: // AddKeyComboSet
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				int set_index=0;
				pParams->GetInteger(CRCD(0xeb7d6498,"set_index"),&set_index);
				int spin=0;
				pParams->GetInteger(CRCD(0xedf5db70,"spin"),&spin);
				int num_taps=0;
				pParams->GetInteger(CRCD(0xa4bee6a1,"num_taps"),&num_taps);
				p_goal->AddKeyComboSet(set_index,
									   pParams->ContainsFlag(CRCD(0x687e2c25,"require_perfect")),
									   spin,
									   num_taps);
			}	
			break;
		}
		
		// @script | RemoveKeyComboSet
		case 0x1d30a7b6: // RemoveKeyComboSet
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				int set_index=0;
				pParams->GetInteger(CRCD(0xeb7d6498,"set_index"),&set_index);
				p_goal->RemoveKeyComboSet(set_index);
			}
			break;
		}

		// @script | GetKeyComboSet
		case 0xe5ed63b7: // GetKeyComboSet
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				int set_index=0;
				pParams->GetInteger(CRCD(0xeb7d6498,"set_index"),&set_index);
				
				SPreDefinedKeyComboSet *p_keycombo_set=p_goal->GetKeyComboSet(set_index);
				if (p_keycombo_set)
				{
					pScript->GetParams()->AddInteger(CRCD(0xedf5db70,"Spin"),p_keycombo_set->mSpin*180);
					pScript->GetParams()->AddInteger(CRCD(0xa4bee6a1,"num_taps"),p_keycombo_set->mNumTaps);
					if (p_keycombo_set->mRequirePerfect)
					{
						pScript->GetParams()->AddChecksum(NONAME,CRCD(0x687e2c25,"require_perfect"));
					}	
				}
				else
				{
					return CBaseComponent::MF_FALSE;
				}	
			}
			break;
		}
		
		// @script | RemoveGoalSpecificFlag
		case 0x3ebd2c9: // RemoveGoalSpecificFlag
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				p_goal->RemoveGoalSpecificFlag(pParams);
			}	
			break;
		}
		
		// @script | SetGoalSpecificParams
		case 0x47208c6d: // SetGoalSpecificParams
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				p_goal->ReadGoalSpecificParams(pParams);
			}	
			break;
		}

		// @script | FlagGoalAsWon
		case 0x769309e1: // FlagGoalAsWon
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				p_goal->FlagAsWon();
			}	
			break;
		}
		
		
		// @script | FindUnfinishedGoal
		case 0x88d54a10: // FindUnfinishedGoal
		{
			uint32 type=0;
			pParams->GetChecksum("type",&type);
			
			mp_current_goal=NULL;
			
			for (int i=0; i<MAX_GOALS_TOTAL; ++i)
			{
				if (mp_goals[i].Used() && mp_goals[i].GetType()==type && !mp_goals[i].GotAllPositions())
				{
					mp_current_goal=&mp_goals[i];
				}
			}
			if (!mp_current_goal)
			{
				return CBaseComponent::MF_FALSE;
			}	
			break;	
		}

		case 0xff9e8752: // GoalExists
		{
			if (!find_goal(pParams))
			{
				return CBaseComponent::MF_FALSE;
			}	
			break;
		}									
		
        // @script | NewEditorGoal | 
		case 0x259c59dd: // NewEditorGoal
		{
			NewGoal();
			Dbg_MsgAssert(mp_current_goal,("NULL mp_current_goal ?"));
			//printf("Max skate goal size = %d\n",mp_current_goal->GetSkateGoalMaxSize());
			
			pScript->GetParams()->AddChecksum("goal_id",mp_current_goal->GetId());
			break;
		}	
		
		// @script | RemovedCreatedGoal |
		case 0x1f80b5ae: // RemovedCreatedGoal
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				remove_goal(p_goal);
			}	
			break;
		}

		// @script | GoalHasAllPositionsSet |
		case 0x26848221: // GoalHasAllPositionsSet
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				if (!p_goal->GotAllPositions())
				{
					return CBaseComponent::MF_FALSE;
				}	
			}	
			else
			{
				return CBaseComponent::MF_FALSE;
			}
			break;
		}
			
		// @script | SetCurrentEditorGoal |
		case 0xfdb2c514: // SetCurrentEditorGoal
		{
			mp_current_goal=find_goal(pParams);
			break;
		}
			
        // @script | SetEditorGoalType | 
		case 0x98abbd09: // SetEditorGoalType
		{
			uint32 type=0;
			pParams->GetChecksum("type",&type);
			
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				p_goal->SetType(type);
			}	
			break;
		}

        // @script | SetEditorGoalDescription | 
		case 0x66a33157: // SetEditorGoalDescription
		{
			const char *p_text="";
			pParams->GetString("text",&p_text);
			
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				p_goal->SetGoalDescription(p_text);
			}	
			break;
		}

        // @script | SetEditorGoalWinMessage | 
		case 0x7aabbc09: // SetEditorGoalWinMessage
		{
			const char *p_text="";
			pParams->GetString("text",&p_text);
			
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				p_goal->SetWinMessage(p_text);
			}	
			break;
		}

		// @script | GetCurrentEditedGoalInfo
		case 0x69b872e3: // GetCurrentEditedGoalInfo
		{
			if (mp_current_goal)
			{
				mp_current_goal->WriteIntoStructure(pScript->GetParams());
			}	
			break;
		}
			
		// @script | GetEditedGoalsInfo
		// @parmopt name | Level | If specified, it will only include goals for this level.
		// The level is specified using the name of its load script.
		case 0x96f11bd3: // GetEditedGoalsInfo
		{
			uint32 level=s_get_level_checksum(pParams);
			
			int n=CountGoalsForLevel(level);
			if (n==0)
			{
				break;
			}
				
			Script::CArray *p_info_array=new Script::CArray;
			p_info_array->SetSizeAndType(n, ESYMBOLTYPE_STRUCTURE);
			int index=0;
			
			for (int i=0; i<MAX_GOALS_TOTAL; ++i)
			{
				if (mp_goals[i].Used() && (level==0 || mp_goals[i].GetLevel()==level) && mp_goals[i].GotAllPositions())
				{
					Script::CStruct *p_info=new Script::CStruct;
					mp_goals[i].WriteIntoStructure(p_info);
					p_info_array->SetStructure(index++,p_info);
				}	
			}
			Dbg_MsgAssert(index==n,("Eh?"));
			
			pScript->GetParams()->AddArrayPointer("EditedGoalsInfo",p_info_array);
			break;
		}
			
		// @script | GetNumEditedGoals	
		// @parmopt name | Level | If specified, it will only count goals for this level.
		// The level is specified using the name of its load script.
		case 0x96874358: // GetNumEditedGoals
		{
			uint32 level=s_get_level_checksum(pParams);
			
			int n=CountGoalsForLevel(level);
			if (level==0 && pParams->ContainsFlag(CRCD(0xb99f6330,"ExcludeParkEditorGoals")))
			{
				n-=CountGoalsForLevel(CRCD(0xe8b4b836,"Load_Sk5Ed"));
				Dbg_MsgAssert(n>=0,("More editor goals (%d) than total goals? (%d)",CountGoalsForLevel(CRCD(0xe8b4b836,"Load_Sk5Ed")),CountGoalsForLevel(0)));
			}
			pScript->GetParams()->AddInteger("NumGoals",n);
			break;
		}

		// @script | GetCurrentEditedGoalId
		case 0x5bd8050a: // GetCurrentEditedGoalId
		{
			pScript->GetParams()->RemoveComponent("goal_id");
			if (mp_current_goal)
			{
				pScript->GetParams()->AddChecksum("goal_id",mp_current_goal->GetId());
			}
			break;
		}

		// @script | WriteEditedGoalNodePositions
		case 0x5408ac04: // WriteEditedGoalNodePositions
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				p_goal->WriteNodePositions();
			}	
			break;
		}
		
		// @script | AddEditedGoalToGoalManager
		case 0xb525b113: // AddEditedGoalToGoalManager
		{
			CEditGoal *p_goal=find_goal(pParams);
			if (p_goal)
			{
				p_goal->AddGoalToGoalManager(pParams->ContainsFlag(CRCD(0xc6322a25,"MarkUnbeaten")));
			}	
			break;
		}

		// @script | SetEditorGoalName |
		case 0x4a569426: // SetEditorGoalName
		{
			const char *p_name="";
			if (mp_current_goal && pParams->GetString("Name",&p_name))
			{
				mp_current_goal=find_goal(pParams);
				mp_current_goal->SetGoalName(p_name);
			}	
			break;
		}

		// @script | SetEditorPedName |
		case 0x410821df: // SetEditorPedName
		{
			const char *p_name="";
			if (mp_current_goal && pParams->GetString("Name",&p_name))
			{
				mp_current_goal=find_goal(pParams);
				mp_current_goal->SetPedName(p_name);
			}	
			break;
		}

		// @script | MaxEditedGoalsReached | Returns true if there is no space for any more
		// goals, or if the max goals per level has been reached for this level.
		// @parmopt name | Level | The level, specified using its load script name.
		case 0x60288abe: // MaxEditedGoalsReached
		{
			if (GetNumGoals()==MAX_GOALS_TOTAL)
			{
				// Definitely reached the max if got MAX_GOALS_TOTAL
				return CBaseComponent::MF_TRUE;
			}
			
			// Otherwise, count up how many goals exist for this level, 
			// and compare with MAX_GOALS_PER_LEVEL
			uint32 level=s_get_level_checksum(pParams);
			Dbg_MsgAssert(level,("\n%s\nNo Level specified",pScript->GetScriptInfo()));
			
			if (CountGoalsForLevel(level) >= MAX_GOALS_PER_LEVEL)
			{
				return CBaseComponent::MF_TRUE;
			}
				
			return CBaseComponent::MF_FALSE;
			break;
		}

		// @script | EditGoal |
		case 0x831eca10: // EditGoal
			mp_current_goal=find_goal(pParams);
			if (mp_current_goal)
			{
				mp_current_goal->EditGoal();
				update_camera_pos();
				refresh_cursor_object();
			}
			break;

		// @script | NukeAllGoals	
		case 0xdba8e3fc: // NukeAllGoals
		{
			if (pParams->ContainsFlag(CRCD(0x98439808,"OnlyParkEditorGoals")))
			{
				ClearOnlyParkGoals();
			}
			else
			{
				ClearAllExceptParkGoals();
			}	
			break;
		}
			
		// @script | GetMaxGoalsPerLevel		
		case 0xd7258335: // GetMaxGoalsPerLevel
		{
			pScript->GetParams()->AddInteger(CRCD(0x8bc2e0db,"max_goals"),MAX_GOALS_PER_LEVEL);
			break;
		}

		case 0x9e5a634a: // SetGoalScore
		{
			mp_current_goal=find_goal(pParams);
			if (mp_current_goal)
			{
				int score=0;
				if (pParams->GetInteger(NONAME,&score))
				{
					mp_current_goal->SetScore(score);
				}	
			}
			break;
		}

		// @script | SetGoalTimeLimit
		case 0x7d5073f6: // SetGoalTimeLimit
		{
			mp_current_goal=find_goal(pParams);
			if (mp_current_goal)
			{
				int time=120;
				if (pParams->GetInteger(NONAME,&time))
				{
					mp_current_goal->SetTimeLimit(time);
				}	
			}
			break;
		}

		// @script | SetGoalControlType
		case 0xec316a9c: // SetGoalControlType
		{
			mp_current_goal=find_goal(pParams);
			if (mp_current_goal)
			{
				uint32 control_type=CRCD(0x54166acd,"skate");
				if (pParams->GetChecksum(NONAME,&control_type))
				{
					mp_current_goal->SetControlType(control_type);
				}	
			}
			break;
		}

		// @script | RefreshGoalCursorPosition
		case 0x20b566b1: // RefreshGoalCursorPosition
		{
			if (mp_current_goal)
			{
				Mth::Vector cursor_pos(0.0f,0.0f,0.0f);		
				float angle=0.0f;
				float height=0.0f;
				mp_current_goal->GetPosition(&cursor_pos,&height,&angle);
				
				Script::CStruct params;
				params.AddVector(CRCD(0x7f261953,"Pos"),cursor_pos[X],cursor_pos[Y]+height,cursor_pos[Z]);
				params.AddFloat(CRCD(0xff7ebaf6,"Angle"),angle);
				Script::RunScript(CRCD(0xdea7fe56,"goal_editor_update_cursor_position"),&params);
			}	
			break;
		}
			
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	// the "default" case of the switch statement handles
	// unrecognized functions;  if we make it down here,
	// that means that the component already handled it
	// somehow
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalEditorComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CGoalEditorComponent::GetDebugInfo"));

	int num_goals=GetNumGoals();
	p_info->AddInteger("NumGoals",num_goals);
	if (mp_current_goal)
	{
		Script::CStruct *p_struct=new Script::CStruct;
		mp_current_goal->GetDebugInfo(p_struct);
		p_info->AddStructurePointer("CurrentGoal",p_struct);
	}
	
	if (num_goals)
	{
		Script::CArray *p_array=new Script::CArray;
		p_array->SetSizeAndType(num_goals,ESYMBOLTYPE_STRUCTURE);
		int index=0;
		for (int i=0; i<MAX_GOALS_TOTAL; ++i)
		{
			if (mp_goals[i].Used())
			{
				Script::CStruct *p_struct=new Script::CStruct;
				mp_goals[i].GetDebugInfo(p_struct);
				p_array->SetStructure(index++,p_struct);
			}	
		}
		Dbg_MsgAssert(index==num_goals,("Bad num_goals ?"));
		p_info->AddArrayPointer("Goals",p_array);
	}
		
// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

Obj::CGoalEditorComponent *GetGoalEditor()
{
	Obj::CCompositeObject *p_obj=(Obj::CCompositeObject*)Obj::CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0x81f01058,"GoalEditor"));
	Dbg_MsgAssert(p_obj,("No GoalEditor object"));
	Obj::CGoalEditorComponent *p_goal_editor=GetGoalEditorComponentFromObject(p_obj);
	Dbg_MsgAssert(p_goal_editor,("No goal editor component ???"));
	
	return p_goal_editor;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}
