//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       EditorCameraComponent.cpp
//* OWNER:          KSH
//* CREATION DATE:  26 Mar 2003
//****************************************************************************

#include <sk/components/editorcameracomponent.h>
#include <sk/components/raileditorcomponent.h>
#include <sk/modules/skate/skate.h>
#include <gel/components/inputcomponent.h>
#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/parse.h>
#include <gel/scripting/symboltable.h>
#include <sk/engine/feeler.h>
#include <gfx/shadow.h>
#include <sk/scripting/nodearray.h>
#include <sys/config/config.h>
#include <sk/parkeditor2/parked.h>

//#define DEBUG_COLLISION

namespace Obj
{


static bool sHitKillPoly=false;
// If a poly's node's trigger script contains any of these then it is considered a Kill poly
static uint32 spKillScripts[]=
{
	CRCC(0xdea3057b,"SK3_KillSkater"),
	CRCC(0xb1058546,"SK3_KillSkater_Water"),
	CRCC(0xb38ed6b,"SK3_KillSkater_Finish"),
	CRCC(0x1310920e,"SK3_KillSkater_Pungee"),
	CRCC(0x979a99ab,"NY_KillWater"),
	CRCD(0x7aae1282,"RU_KillSkater"),
	CRCD(0xcb1b1d9e,"RU_KillSkater2"),
	CRCD(0x84e5c703,"NY_LeavingMessage"),
	CRCD(0xf66fdf6e,"SJ_Televator"),
	CRCD(0xb21175da,"HI_KillSkater_LittleDogs"),
	CRCD(0xd9d60165,"VC_CAG_boundary"),
};

static void s_check_if_kill_poly ( CFeeler* p_feeler )
{
	if (!p_feeler->GetTrigger() || !p_feeler->GetNodeChecksum())
	{
		#ifdef DEBUG_COLLISION
		//printf("Not got trigger or not got node checksum\n");
		#endif
		return;
	}	
	
	int node = SkateScript::FindNamedNode(p_feeler->GetNodeChecksum(), false);// false means don't assert
	if (node < 0)
	{
		// Got a trigger, but no node, so assume it is a kill poly. This case arises in the park editor
		// where a kill piece such as water or lava has been placed but the node array has not been generated yet.
		//sHitKillPoly=true;
		// Note: Commented out, to fix TT8832, where the cursor could get stuck on cars.
		// The PE now allows the cursor to go anywhere, even onto kill polys.
		return;
	}
		
	Script::CArray *p_node_array = Script::GetArray(CRCD(0xc472ecc5, "NodeArray"));
	Script::CStruct *p_node = p_node_array->GetStructure(node);
	
	uint32 trigger_script=0;
	p_node->GetChecksum(CRCD(0x2ca8a299,"TriggerScript"),&trigger_script);
	
	if (trigger_script)
	{
		//printf("trigger_script=%s\n",Script::FindChecksumName(trigger_script));
		if (Script::ScriptContainsAnyOfTheNames(trigger_script,spKillScripts,sizeof(spKillScripts)/sizeof(uint32)))
		{
			sHitKillPoly=true;
		}	
	}	
}

// Checks whether the vector posA -> posB goes through any Kill polys.
// Does not check collidable polys.
static bool s_goes_through_kill_poly(const Mth::Vector &posA, const Mth::Vector &posB)
{
	sHitKillPoly=false;
	
	CFeeler feeler;
	
	feeler.m_start = posA;
	feeler.m_end = posB;

	feeler.SetIgnore(0, mFD_NON_COLLIDABLE | mFD_TRIGGER);
	feeler.SetCallback(s_check_if_kill_poly);
	//feeler.DebugLine(0,255,0);
	feeler.GetCollision();
	
	return sHitKillPoly;
}

bool CEditorCameraComponent::find_y(Mth::Vector start, Mth::Vector end, float *p_y)
{
	#ifdef DEBUG_COLLISION
	//printf("s_find_y: ");
	#endif
	
	CFeeler feeler;
	feeler.SetStart(start);
	feeler.SetEnd(end);
	// Ignore invisible polys, otherwise one can get stuck on top of the hotel in Hawaii,
	// because it is surrounded by horizontal invisible polys.
	//feeler.SetIgnore(mFD_INVISIBLE, 0);	
	
	if (feeler.GetCollision())
	{
		if (m_simple_collision)
		{
			#ifdef DEBUG_COLLISION
			printf("Simple drop down\n");
			#endif
			
			sHitKillPoly=false;
			Mth::Vector point=feeler.GetPoint();
			*p_y=point[Y];
			return true;
		}	
	
		sHitKillPoly=false;
		s_check_if_kill_poly(&feeler);
		if (sHitKillPoly)
		{
			// Can't go onto collidable Kill polys
			#ifdef DEBUG_COLLISION
			printf("Kill poly!\n");
			#endif
			return false;
		}
				
		// Check the steepness
		Mth::Vector normal=feeler.GetNormal();
		// .087=max steepness of 85 degrees
		if (normal[Y] < 0.087f/*sinf(Mth::DegToRad(90.0f-Script::GetFloat(CRCD(0x87b926b4,"MaxSteepness"))))*/)
		{
			// The ground is too steep!
			// Need this check otherwise the skater can be placed halfway down the almost vertical
			// riverbank in NJ
			#ifdef DEBUG_COLLISION
			printf("Too steep!\n");
			#endif
			return false;
		}
		
		#ifdef DEBUG_COLLISION
		//printf("Found y=%f normal[Y]=%f\n",feeler.GetPoint()[Y],normal[Y]);
		#endif
		
		Mth::Vector point=feeler.GetPoint();
		*p_y=point[Y];

		// But, before returning true, check whether there are any kill polys between the start and the
		// collision point.
		if (s_goes_through_kill_poly(start, point))
		{
			#ifdef DEBUG_COLLISION
			printf("Kill poly above found collision!\n");
			#endif
			return false;
		}
				
		return true;
	}
	
	#ifdef DEBUG_COLLISION
	printf("No collision\n");
	#endif
	
	return false;
}

bool CEditorCameraComponent::find_collision_at_offset(Mth::Vector oldCursorPos, float upOffset)
{
	Mth::Vector start=m_cursor_pos;
	start[Y]+=upOffset;
	Mth::Vector end=m_cursor_pos;
	end[Y]+=Script::GetFloat(CRCD(0xf230d521,"EditorCam_CursorCollisionDownOffset"));
	
	bool new_position_is_ok=false;
	
	float y;
	if (find_y(start,end,&y))
	{
		new_position_is_ok=true;
	}
	
	if (new_position_is_ok)
	{
		Mth::Vector from = oldCursorPos;
		Mth::Vector to = m_cursor_pos;
		to[Y]=y;
		to[Y]+=72.0f; // 6 feet above the ground
		// Make it really be horizontal to avoid steep collision check weirdness
		from[Y]=to[Y]; 
		
		if (!m_simple_collision)
		{
			// Do a horizontal collision check to see if the cursor 
			// is going to move through a kill poly
			if (s_goes_through_kill_poly(from, to))
			{
				#ifdef DEBUG_COLLISION
				printf("Goes through kill poly\n");
				#endif
				new_position_is_ok=false;
			}
		}
		
		if (!m_allow_movement_through_walls)
		{
			// Also don't allow moving through walls
			CFeeler feeler;
			feeler.SetIgnore(mFD_NON_COLLIDABLE, 0);
			feeler.SetStart(from);
			feeler.SetEnd(to);
			if (feeler.GetCollision())
			{
				#ifdef DEBUG_COLLISION
				printf("oof\n");
				#endif
				new_position_is_ok=false;
			}
		}	
	}

	if (new_position_is_ok)
	{
		m_cursor_pos[Y]=y;
		return true;
	}
	return false;	
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CEditorCameraComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CEditorCameraComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CEditorCameraComponent::CEditorCameraComponent() : CBaseComponent()
{
	SetType( CRC_EDITORCAMERA );
	
	m_camera_focus_pos.Set();
	m_cam_pos.Set();
	m_cursor_pos.Set();
	m_cursor_u.Set();
	m_cursor_v.Set();
	
	m_radius=0.0f;
	m_radius_min=0.0f;
	m_radius_max=0.0f;
	
	m_angle=0.0f;
	m_tilt_angle=0.0f;
	m_tilt_angle_min=0.0f;
	m_tilt_angle_max=0.0f;
	
	m_cursor_height=0.0f;
	m_min_height=0.0f;
	
	mp_shadow=NULL;
	mp_input_component=NULL;
	
	m_simple_collision=false;
	m_allow_movement_through_walls=false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CEditorCameraComponent::~CEditorCameraComponent()
{
	delete_shadow();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CEditorCameraComponent::InitFromStructure( Script::CStruct* pParams )
{
	// ** Add code to parse the structure, and initialize the component

	m_min_height=0.0f;
	pParams->GetFloat(CRCD(0x9cdf40cd,"min_height"),&m_min_height);
	
	m_radius_min=100.0f;
	pParams->GetFloat(CRCD(0x52eecb98,"min_radius"),&m_radius_min);
	
	m_radius_max=2000.0f;
	pParams->GetFloat(CRCD(0x53e2512c,"max_radius"),&m_radius_max);
	
	m_simple_collision=pParams->ContainsFlag(CRCD(0xa0d7fbab,"SimpleCollision"));
	m_allow_movement_through_walls=pParams->ContainsFlag(CRCD(0xf5680def,"AllowMovementThroughWalls"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// RefreshFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CEditorCameraComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Default to just calline InitFromStructure()
	// but if that does not handle it, then will need to write a specific 
	// function here. 
	// The user might only want to update a single field in the structure
	// and we don't want to be asserting becasue everything is missing 
	
	InitFromStructure(pParams);
}

void CEditorCameraComponent::Hide( bool shouldHide )
{
	if (shouldHide)
	{
		delete_shadow();
	}	
	else
	{
		create_shadow();
	}
}

void CEditorCameraComponent::GetCursorOrientation(Mth::Vector *p_u, Mth::Vector *p_v)
{
	*p_u=m_cursor_u;
	*p_v=m_cursor_v;
}

void CEditorCameraComponent::create_shadow()
{
	if (!mp_shadow)
	{
		mp_shadow = new Gfx::CSimpleShadow;
		mp_shadow->SetScale( SHADOW_SCALE );
		mp_shadow->SetModel( "Ped_Shadow" );
	}	
}

void CEditorCameraComponent::delete_shadow()
{
	if (mp_shadow)
	{
		delete mp_shadow;
		mp_shadow=NULL;
	}	
}

void CEditorCameraComponent::update_shadow()
{
	Mth::Vector cursor_pos=m_cursor_pos;		
	cursor_pos[Y]+=m_cursor_height;

	CFeeler feeler;
	feeler.SetStart(cursor_pos);
	feeler.SetEnd(cursor_pos + Mth::Vector(0.0f,SHADOW_COLLISION_CHECK_DISTANCE,0.0f,0.0f));
	if (feeler.GetCollision())
	{
		create_shadow();
		
		Mth::Matrix mat;
		mat.Ident();

		Mth::Vector shadow_pos=feeler.GetPoint();
		Mth::Vector shadow_normal=feeler.GetNormal();
		mp_shadow->UpdatePosition(shadow_pos,
										 mat,
										 shadow_normal);
	}
	else
	{
		delete_shadow();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEditorCameraComponent::Finalize()
{
	// Get the pointers to the other required components.
	Dbg_MsgAssert(mp_input_component==NULL,("mp_input_component not NULL ?"));
	mp_input_component = GetInputComponentFromObject(GetObject());
	Dbg_MsgAssert(mp_input_component,("CEditorCameraComponent requires parent object to have an input component!"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// The component's Update() function is called from the CCompositeObject's 
// Update() function.  That is called every game frame by the CCompositeObjectManager
// from the s_logic_code function that the CCompositeObjectManager registers
// with the task manger.
void CEditorCameraComponent::Update()
{
	Dbg_MsgAssert(mp_input_component,("NULL mp_input_component"));
	
	// TODO: When in park editor, use different values here, cos they'll need to be
	// tweaked according to the shell geometry.
	m_tilt_angle_min=Script::GetFloat(CRCD(0x7d16d113,"EditorCam_TiltMin"));
	m_tilt_angle_max=Script::GetFloat(CRCD(0x411bee4a,"EditorCam_TiltMax"));
	
	CControlPad& control_pad = mp_input_component->GetControlPad();

	
	// Update m_angle
	float angle_vel=Script::GetFloat(CRCD(0x8242d878,"EditorCam_TurnSpeed"))*control_pad.m_scaled_rightX;
	m_angle+=angle_vel;
	while (m_angle > 2*3.141592654f)
	{
		m_angle -= 2*3.141592654f;
	}	
	while (m_angle < 0.0f)
	{
		m_angle += 2*3.141592654f;
	}	
	
	// Update m_tilt_angle
	angle_vel=-Script::GetFloat(CRCD(0x3cfa3a72,"EditorCam_TiltSpeed"))*control_pad.m_scaled_rightY;
	m_tilt_angle+=angle_vel;
	if (m_tilt_angle > m_tilt_angle_max)
	{
		m_tilt_angle = m_tilt_angle_max;
	}	
	if (m_tilt_angle < m_tilt_angle_min)
	{
		m_tilt_angle = m_tilt_angle_min;
	}	
	
	// Update m_radius
	bool zoom_in_button_pressed=false;
	bool zoom_out_button_pressed=false;
	bool raise_pressed=false;
	bool lower_pressed=false;
	switch (Config::GetHardware())
	{
		case Config::HARDWARE_XBOX:
		{
			uint32 input_mask = mp_input_component->GetInputMask();
			if (input_mask & Inp::Data::mA_BLACK)
			{
				zoom_out_button_pressed=true;
			}	
			if (input_mask & Inp::Data::mA_WHITE)
			{
				zoom_in_button_pressed=true;
			}	
			// Note: See p_siodev.cpp for the low level code that does the weird XBox buttons mappings
			if (Ed::CParkEditor::Instance()->EditingCustomPark())
			{
				// When editing a park, XBox L gives L2, XBox R gives L1
				// We want XBox L to lower, XBox R to raise
				if (input_mask & Inp::Data::mA_L1)
				{
					raise_pressed=true;
				}	
				if (input_mask & Inp::Data::mA_L2)
				{
					lower_pressed=true;
				}	
			}
			else
			{
				// When not editing a park, XBox L gives L1, XBox R gives R1
				// We want XBox L to lower, XBox R to raise
				if (input_mask & Inp::Data::mA_R1)
				{
					raise_pressed=true;
				}	
				if (input_mask & Inp::Data::mA_L1)
				{
					lower_pressed=true;
				}	
			}
			break;
		}
		case Config::HARDWARE_NGC:
		{
			uint32 input_mask = mp_input_component->GetInputMask();
			if (input_mask & Inp::Data::mA_Z)
			{
				if (input_mask & Inp::Data::mA_R1)
				{
					zoom_out_button_pressed=true;
				}
				if (input_mask & Inp::Data::mA_L1)
				{
					zoom_in_button_pressed=true;
				}
			}
			else
			{
				if (input_mask & Inp::Data::mA_R1)
				{
					lower_pressed=true;
				}
				if (input_mask & Inp::Data::mA_L1)
				{
					raise_pressed=true;
				}
			}
			break;
		}
		default:	
		{
			zoom_out_button_pressed=control_pad.m_R1.GetPressed();
			zoom_in_button_pressed=control_pad.m_R2.GetPressed();
			raise_pressed=control_pad.m_L1.GetPressed();
			lower_pressed=control_pad.m_L2.GetPressed();
			break;		
		}	
	}
	
	float radius_vel=Script::GetFloat(CRCD(0x4e58f829,"EditorCam_InOutSpeed"));
	if (zoom_out_button_pressed)
	{
	}	
	else if (zoom_in_button_pressed)
	{
		radius_vel=-radius_vel;
	}
	else
	{
		radius_vel=0.0f;
	}
		
	// Make the speed proportional to the current radius so that the camera can be zoomed
	// out to the max nice and quick.
	radius_vel*=m_radius;
	
	m_radius+=radius_vel;
	if (m_radius < m_radius_min)
	{
		m_radius = m_radius_min;
	}	
	if (m_radius > m_radius_max)
	{
		m_radius = m_radius_max;
	}	
	float zoom_factor=(m_radius-m_radius_min)/(m_radius_max-m_radius_min);
	// zoom_factor is between 0 and 1 and indicates how far out the camera is zoomed.
	// 0 = closest to cursor, 1 = furthest away.


	// Update the height
	float height_vel_min=Script::GetFloat(CRCD(0x604961f4,"EditorCam_UpDownSpeedMin"));
	float height_vel_max=Script::GetFloat(CRCD(0x5c445ead,"EditorCam_UpDownSpeedMax"));
	float height_vel=height_vel_min + zoom_factor * (height_vel_max-height_vel_min);
	
	// If very close in allow very fine movements, needed for rail placement in rail editor.
	// TODO: Fix this so that the change in speed is not so sudden, make it linear like above.
	if (m_radius < 100)
	{
		height_vel=0.2f;
	}
	
	if (raise_pressed)
	{
	}	
	else if (lower_pressed)
	{
		height_vel=-height_vel;
	}
	else
	{
		height_vel=0.0f;
	}	
	
	m_cursor_height+=height_vel;
	float max_height=Script::GetFloat(CRCD(0x61d56fa0,"EditorCam_MaxHeight"));
	if (m_cursor_height > max_height)
	{
		m_cursor_height=max_height;
	}

	// Switch off collision on any created-rail sectors whilst doing collision checks.
	// This is because the rail sectors have a vertical collidable poly, which occasionally does cause
	// collisions, making the cursor ping up into the air. Must be some sort of floating point innaccuracy
	// (or too much accuracy), probably because the collision check vector is straight down the vertical poly.
	Mdl::Skate * p_skate_mod =  Mdl::Skate::Instance();
	if (p_skate_mod->m_cur_level == CRCD(0xe8b4b836,"load_sk5ed"))
	{
		Obj::GetRailEditor()->SetSectorActiveStatus(false);
	}	

	// Make sure that the cursor cannot be raised through a collidable poly, otherwise it
	// can be raised through the stadium roof in slam city jam (TT3142)
	CFeeler feeler;
	feeler.SetStart(m_cursor_pos);
	feeler.SetEnd(m_cursor_pos + Mth::Vector(0.0f, m_cursor_height+10.0f, 0.0f));
	if (feeler.GetCollision())
	{
		m_cursor_height=feeler.GetPoint()[Y]-m_cursor_pos[Y]-10.0f;
	}
	
	// This allows the height to be less than m_min_height, but as soon as it goes
	// over m_min_height it won't be able to go below it again.
	// This is for the rail editor, where the min height needs to be limited whilst
	// still allowing snapping to a vertex.
	if (height_vel < 0.0f && m_cursor_height-height_vel+0.01 >= m_min_height)
	{
		if (m_cursor_height < m_min_height)
		{
			m_cursor_height=m_min_height;
		}
	}	
	if (m_cursor_height < 0.0f)
	{
		m_cursor_height=0.0f;
	}
		
	// Update m_cursor_pos ...

	// First move it horizontally according to the left analogue stick
	m_cursor_u.Set(cosf(m_angle),0.0f,-sinf(m_angle));
	m_cursor_v.Set(sinf(m_angle),0.0f,cosf(m_angle));
	
	float speed_min=Script::GetFloat(CRCD(0x83bde786,"EditorCam_MoveSpeedMin"));
	float speed_max=Script::GetFloat(CRCD(0xbfb0d8df,"EditorCam_MoveSpeedMax"));
	// Make the speed proportional to the zoom factor so that fine movements can be done
	// when zoomed in, whilst fast movements can be made when zoomed out.
	float speed = speed_min + zoom_factor * (speed_max-speed_min);
	
	// If very close in allow very fine movements, needed for rail placement in rail editor.
	// TODO: Fix this so that the change in speed is not so sudden, make it linear like above.
	if (m_radius < 100)
	{
		speed=1.0f;
	}
		
	float horiz=control_pad.m_leftX / 128.0f;
	float vert=control_pad.m_leftY / 128.0f;
	if (fabs(horiz) < 0.01f)
	{
		if (control_pad.m_left.GetPressed())
		{
			horiz=-0.5f;
		}	
		if (control_pad.m_right.GetPressed())
		{
			horiz=0.5f;
		}	
	}
		
	if (fabs(vert) < 0.01f)
	{
		if (control_pad.m_up.GetPressed())
		{
			vert=-0.5f;
		}	
		if (control_pad.m_down.GetPressed())
		{
			vert=0.5f;
		}	
	}	
		
	Mth::Vector vel = m_cursor_u * horiz*speed+
					  m_cursor_v * vert*speed;
	Mth::Vector old_pos=m_cursor_pos;					  
	m_cursor_pos+=vel;

	// Now do a collision check to find the y.	
	if (!find_collision_at_offset(old_pos, Script::GetFloat(CRCD(0xc7210318,"EditorCam_CursorCollisionFirstUpOffset"))))
	{
		// Do an upwards collision check to check for a ceiling above the skater, and do not
		// do the check for higher collision if there is a ceiling. This is to fix a bug where in Slam City
		// the goal cursor would pop up into the stands if hitting the teleport poly in the escalator D corridor.
		CFeeler feeler;
		feeler.SetStart(old_pos);
		Mth::Vector high=old_pos;
		high[Y]+=120.0f;
		feeler.SetEnd(high);
		if (feeler.GetCollision())
		{
			// Undo the movement so that the cursor does not move.
			m_cursor_pos=old_pos;
		}
		else if (!find_collision_at_offset(old_pos, Script::GetFloat(CRCD(0x56c2bff2,"EditorCam_CursorCollisionSecondUpOffset"))))
		{
			// Undo the movement so that the cursor does not move.
			m_cursor_pos=old_pos;
		}
		else
		{
			// Did find higher collision, which is going to cause the cursor to pop to a high place, such
			// as the roof of a building. In this case, zero the cursor height, because it seems weird to
			// have it maintain it's hover height in that case. (TT10272)
			m_cursor_height=m_min_height;
		}	
	}
		
	if (p_skate_mod->m_cur_level == CRCD(0xe8b4b836,"load_sk5ed"))
	{
		Obj::GetRailEditor()->SetSectorActiveStatus(true);
	}	

	
	// Calculate the camera position, which equals the cursor position but has a lag
	// applied so that as the cursor follows the contours of the ground it does not make
	// the camera glitch.
	m_camera_focus_pos[X]=m_cursor_pos[X];
	m_camera_focus_pos[Z]=m_cursor_pos[Z];
	m_camera_focus_pos[Y]+=(m_cursor_pos[Y]+m_cursor_height-m_camera_focus_pos[Y])*Script::GetFloat(CRCD(0x725ad02c,"EditorCam_YCatchUpFactor"));
	
	Mth::Vector tilted_v;
	tilted_v[X]=m_cursor_v[X]*cosf(m_tilt_angle);
	tilted_v[Z]=m_cursor_v[Z]*cosf(m_tilt_angle);
	tilted_v[Y]=sinf(m_tilt_angle);

	Mth::Matrix mat;
	mat[Z]=tilted_v;
	mat[X]=m_cursor_u;
	mat[Y]=Mth::CrossProduct(tilted_v,m_cursor_u);
	mat[X][W]=0.0f;
	mat[Y][W]=0.0f;
	mat[Z][W]=0.0f;
	mat[W].Set();
	GetObject()->SetMatrix(mat);
	
	Mth::Vector start=m_camera_focus_pos;
	/*
	Mth::Vector end;
	float radius=m_radius;
	while (true)
	{
		end=start+tilted_v*radius;

		if (radius > Script::GetFloat(CRCD(0xf612c474,"EditorCam_CursorCollisionEnableDistMax")))
		{
			printf("Not checksing for collision\n");
			break;
		}
		
		Mth::Vector p_rays[4];
		p_rays[0]=start;
		p_rays[1]=start-120*u+Mth::Vector(0.0f,70.0f,0.0f,0.0f);
		p_rays[2]=start+Mth::Vector(0.0f,70.0f,0.0f,0.0f);
		p_rays[3]=start+120*u+Mth::Vector(0.0f,70.0f,0.0f,0.0f);
		if (cursor_occluded(p_rays,4,end))
		{
			radius=radius*0.9f; // TODO: Hmmm, should find out the exact distance ...
		}	
		else
		{
			break;
		}	
	}

	m_cam_pos+=(end-m_cam_pos)*Script::GetFloat(CRCD(0xf151a64,"EditorCam_CameraCatchUpFactor"));
	*/
	// TODO: Fix the camera collision above, then comment out this line for it to have effect.
	m_cam_pos=start + tilted_v*m_radius;

	m_cam_pos[W]=1.0f;
	GetObject()->SetPos(m_cam_pos);
	
	
	update_shadow();
}

bool CEditorCameraComponent::cursor_occluded(Mth::Vector *p_ray_starts, int numRays, Mth::Vector end)
{
	CFeeler feeler;
	for (int i=0; i<numRays; ++i)
	{
		feeler.SetStart(p_ray_starts[i]);
		feeler.SetEnd(end);
		if (feeler.GetMovableCollision())
		{
			return false;
		}	
		if (!feeler.GetCollision())
		{
			return false;
		}
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the "Checksum" of a script command, then possibly handle it
// if it's a command that this component will handle	
CBaseComponent::EMemberFunctionResult CEditorCameraComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		case 0xf362bbbf: // EditorCam_Initialise
			pParams->GetVector(CRCD(0xb9d31b0a,"position"),&m_cursor_pos);
			m_camera_focus_pos=m_cursor_pos;
			m_radius=700.0f;
			pParams->GetFloat(CRCD(0xc48391a5,"radius"),&m_radius);
			m_tilt_angle=0.6f;
			pParams->GetFloat(CRCD(0xe3c07609,"tilt"),&m_tilt_angle);
			m_cursor_height=0.0f;
			pParams->GetFloat(CRCD(0xb8d629ca,"cursor_height"),&m_cursor_height);
			break;

		case 0x69cb3ae1: // EditorCam_SetCursorPos
		{
			Mth::Vector pos;
			pParams->GetVector(CRCD(0xb9d31b0a,"position"),&pos,Script::ASSERT);
			m_cursor_pos=pos;
			m_cursor_height=0.0f;
			
			// Call update to do the ground collision check.
			Update();
			// That might have changed m_cursor_pos[Y], so adjust the height so that the cursor does 
			// appear at the required position.
			m_cursor_height=pos[Y]-m_cursor_pos[Y];
			// Note: If the above call to Update() followed by adjusting the height were not done, then it would not
			// be possible to snap to rail vertices, because rails are uncollidable, so on the next frame the cursor
			// would fall through the rail back to the ground.
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

void CEditorCameraComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CEditorCameraComponent::GetDebugInfo"));
	
	p_info->AddInteger("m_active",m_active);
	p_info->AddVector("m_camera_focus_pos",m_camera_focus_pos.GetX(),m_camera_focus_pos.GetY(),m_camera_focus_pos.GetZ());
	p_info->AddVector("m_cursor_pos",m_cursor_pos.GetX(),m_cursor_pos.GetY(),m_cursor_pos.GetZ());
	p_info->AddFloat("m_cursor_height",m_cursor_height);
	p_info->AddFloat("m_radius",m_radius);
	p_info->AddFloat("m_radius_min",m_radius_min);
	p_info->AddFloat("m_radius_max",m_radius_max);
	p_info->AddFloat("m_angle",m_angle);
	p_info->AddFloat("m_tilt_angle",m_tilt_angle);
	p_info->AddFloat("m_tilt_angle_min",m_tilt_angle_min);
	p_info->AddFloat("m_tilt_angle_max",m_tilt_angle_max);

// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}
