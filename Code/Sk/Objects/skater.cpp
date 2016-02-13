/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**							    											**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Objects (OBJ) 											**
**																			**
**	File name:		skater.cpp												**
**																			**
**	Created:		01/25/00												**
**																			**
**	Description:	Skater													**
**															
****************************************************************************/
  								 									  
// start autoduck documentation
// @DOC skater        																	  
// @module skater | None
// @subindex Scripting Database
// @index script | skater

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/singleton.h>
#include <core/math.h>
#include <core/math/slerp.h>

#include <sys/timer.h>
#include <sys/sioman.h>

#include <gel/objtrack.h>
#include <gel/assman/assman.h>
#include <gel/collision/collcache.h>
#include <gel/inpman.h>
#include <gel/mainloop.h>
#include <gel/environment/terrain.h>
#include <gel/soundfx/soundfx.h>
#include <gel/net/client/netclnt.h>
#include <gel/net/server/netserv.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/lockobjcomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/motioncomponent.h>
#include <gel/components/shadowcomponent.h>
#include <gel/components/skeletoncomponent.h>
#include <gel/components/specialitemcomponent.h>
#include <gel/components/suspendcomponent.h>
#include <gel/components/trickcomponent.h>
#include <gel/components/nodearraycomponent.h>
#include <gel/components/railmanagercomponent.h>
#include <gel/components/skitchcomponent.h>
#include <gel/components/cameracomponent.h>
#include <gel/components/skatercameracomponent.h>
#include <gel/components/vibrationcomponent.h>
#include <gel/components/proximtriggercomponent.h>
#include <gel/components/triggercomponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/collisioncomponent.h>
#include <gel/components/walkcomponent.h>
#include <gel/components/movablecontactcomponent.h>
#include <gel/components/ribboncomponent.h>
#include <gel/components/statsmanagercomponent.h>
#include <gel/components/walkcameracomponent.h>
#ifdef TESTING_GUNSLINGER
#include <gel/components/ridercomponent.h>
#include <gel/components/weaponcomponent.h>
#endif


#include <gfx/camera.h>
#include <gfx/shadow.h>
#include <gfx/debuggfx.h>				// for debug lines
#include <gfx/gfxutils.h>
#include <gfx/nxmodel.h>
#include <gfx/nxlight.h>
#include <gfx/nxlightman.h>
#include <gfx/2D/ScreenElemMan.h>
#include <gfx/2D/SpriteElement.h>
#include <gfx/2D/ScreenElement2.h>
#include <gfx/skeleton.h>
#include <gfx/nx.h>						// for sGetMovableObjects
#include <gfx/NxMiscFX.h>

                                   
#include <sk/modules/skate/skate.h> // for SKATE_TYPE_*
#include <sk/modules/FrontEnd/FrontEnd.h>
#include <sk/modules/skate/competition.h>	  	// for CCompetition
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/goalmanager.h>   // for special goal
#include <sk/modules/skate/CreateATrick.h>

#include <sk/ParkEditor2/ParkEd.h>

#include <sk/objects/crown.h>
#include <sk/objects/moviecam.h>
#include <sk/objects/proxim.h>
#include <sk/objects/rail.h>
#include <sk/objects/objecthook.h>
#include <sk/objects/restart.h>
#include <sk/objects/skater.h>
#include <sk/objects/skaterprofile.h>
#include <sk/objects/skatercareer.h>
#include <sk/objects/playerprofilemanager.h>
#include <sk/components/skaterstancepanelcomponent.h>
#include <sk/components/skaterloopingsoundcomponent.h>
#include <sk/components/skatersoundcomponent.h>
#include <sk/components/skatergapcomponent.h>
#include <sk/components/skaterrotatecomponent.h>
#include <sk/components/skaterphysicscontrolcomponent.h>
#include <sk/components/skaterlocalnetlogiccomponent.h>
#include <sk/components/skaternonlocalnetlogiccomponent.h>
#include <sk/components/skaterscorecomponent.h>
#include <sk/components/skatermatrixqueriescomponent.h>
#include <sk/components/skaterfloatingnamecomponent.h>
#include <sk/components/skaterstatehistorycomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skateradjustphysicscomponent.h>
#include <sk/components/skaterfinalizephysicscomponent.h>
#include <sk/components/skatercleanupstatecomponent.h>
#include <sk/components/skaterendruncomponent.h>
#include <sk/components/skaterbalancetrickcomponent.h>
#include <sk/components/skaterflipandrotatecomponent.h>
#include <sk/components/skaterstatecomponent.h>
#include <sk/components/skaterruntimercomponent.h>

#include <sk/engine/feeler.h>
#include <sk/engine/contact.h>

#include <sk/gamenet/gamenet.h>

#include <sk/scripting/cfuncs.h>

#include <sys/profiler.h>
#include <sys/replay/replay.h>

#include <core/math/geometry.h>

#include <gel/music/music.h>

#include <gel/collision/collision.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/component.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/checksum.h>
#include <sk/scripting/nodearray.h>

#ifdef __PLAT_NGC__
#include "gfx/ngc/nx/nx_init.h"
#endif		// __PLAT_NGC__


#define	FLAGEXCEPTION(X)	SelfEvent(X)


////////////////////////////////////////////////////////////////////////

inline void	dodgy_test()
{
//		 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
//		if (skate_mod->GetSkater(0))
//		{
//			skate_mod->GetSkater(0)->DodgyTest();
//		}
}

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

extern uint64	Gfx_LastVBlank;

namespace Gfx
{
	void DebuggeryLines_CleanUp( void );		// just for debugging
}

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/

// All the CSkater code sits in the Obj namespace, along with other objects,
// like peds and cars
namespace Obj
{

bool DebugSkaterScripts=false;			// Set to true to dump out a lot of info about the skater's script
										// generally you would set it with the script command SkaterDebugOn/Off

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/


/////////////////////////////////////////////////////////////////////////////
// Physics patching, so physics vars can come from different structs
// or globals, in a nice relaxed manner

Script::CStruct * get_physics_struct()
{
	// if (Script::GetInteger(CRCD(0x9e86d1c1,"use_bike_physics")))
	// {
		// return Script::GetStructure(CRCD(0x9d728121,"bike_physics"));
	// }
	// else if (Script::GetInteger(CRCD(0x9ae06a83,"use_walk_physics")))
	// {
		// return Script::GetStructure(CRCD(0x8cbc89dd,"skater_physics"));
	// }
	// else
	// {
		return Script::GetStructure(CRCD(0x8cbc89dd,"skater_physics"));
	// }
}
			  
float GetPhysicsFloat(uint32 id, Script::EAssertType assert)
{

	Script::CStruct	*p_struct = get_physics_struct();	
	float x;
	if (p_struct->GetFloat(id,&x))
	{
		return x;
	}

	return Script::GetFloat(id,assert);
}

int GetPhysicsInt(uint32 id, Script::EAssertType assert)
{
	
	Script::CStruct	*p_struct = get_physics_struct();	
	int x;
	if (p_struct->GetInteger(id,&x))
	{
		return x;
	}

	return Script::GetInteger(id,assert);
}

// End of physics patching
/////////////////////////////////////////////////////////////////////////////


// This Test generally not called
// can stick code in here that will be called from dodgy_test
// whcih gets called from every printf
// generaly you'd have printfs enabled via something like DEBUG_POSITION
void 	CSkater::DodgyTest()
{
		CFeeler feeler;
		feeler.SetEnd(m_pos);
		feeler.SetStart(m_pos + Mth::Vector(0.0f,100.0f,0.0f,0.0f));
		if (feeler.GetCollision())
		{
#	if !defined( __PLAT_NGC__ ) || ( defined( __PLAT_NGC__ ) && !defined( __NOPT_FINAL__ ) )
			float above = feeler.GetPoint()[Y] - m_pos[Y];
			printf(" collision %.2f above me, normal = (%.2f,%.2f,%.2f)\n",above,feeler.GetNormal()[X],feeler.GetNormal()[Y],feeler.GetNormal()[Z]);
#endif		// __NOPT_FINAL__
		}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// K: This is a factored out code snippet that is used quite a bit.
// It get's a pointer to this skater's score object and asserts if it's NULL.
Mdl::Score *CSkater::GetScoreObject()
{
	Dbg_Assert(mp_skater_score_component);
	return mp_skater_score_component->GetScore();
}	

// Just move the skater to the correct position and orientation for
// this restart point
// used to set intial position of other skaters on clients
void CSkater::MoveToRestart(int node)
{
	Dbg_Assert(mp_skater_core_physics_component);
	
	m_matrix.Ident();
	Script::CStruct *pNode = NULL;
	if (node >= 0)
	{
		Script::CArray *pNodeArray=Script::GetArray(CRCD(0xc472ecc5,"NodeArray"));
		pNode=pNodeArray->GetStructure(node);
		Dbg_MsgAssert(pNode,( "null restart"));
		SkateScript::GetPosition( node, &m_pos );
		#ifdef	CHEAT_RUBBER
		m_rubber_pos = m_pos;
		m_rubber_vel.Set();
		#endif
		
		m_old_pos = m_pos;
		mp_skater_core_physics_component->m_safe_pos = m_pos;
		
		SetTeleported();

		Mth::Vector angles;
		SkateScript::GetAngles(pNode, &angles);
		m_matrix.SetFromAngles(angles);
		
		mp_skater_core_physics_component->ResetLerpingMatrix();
		mp_skater_core_physics_component->m_display_normal = mp_skater_core_physics_component->m_last_display_normal = mp_skater_core_physics_component->m_current_normal = m_matrix[Y];
		
		#ifdef		DEBUG_DISPLAY_MATRIX
		dodgy_test(); printf("%d: Setting display_matrix[Y][Y] to %f, [X][X] to %f\n",__LINE__,mp_skater_core_physics_component->m_lerping_display_matrix[Y][Y],mp_skater_core_physics_component->m_lerping_display_matrix[X][X]);
		#endif
	
		// set the shadow to stick to his feet, assuming we are on the ground
		// Note: These are used by the simple shadow, not the detailed one.
		Dbg_Assert(mp_shadow_component)
		mp_shadow_component->SetShadowPos( m_pos );
		mp_shadow_component->SetShadowNormal( m_matrix[Y] );
	
		// update for the camera
		m_display_matrix = mp_skater_core_physics_component->m_lerping_display_matrix;
	}
}

void CSkater::SkipToRestart(int node, bool walk)
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

#	ifdef	DEBUG_TRIGGERS			
	dodgy_test(); printf("SkipToRestart(%d)\n",node);
#	endif

    GetSkaterPhysicsControlComponent()->NotifyReset();
	
	if( m_local_client )
	{
		// The Skater might have been in the middle of a move that disabled his input, so re-enable it
		// and also enable the camera control
		mp_input_component->EnableInput();
		// Also player should ot be paused
		UnPause();

        if (mp_stats_manager_component)
        {
            mp_stats_manager_component->m_restarted_this_frame = true;
        }

        /*		
        // If the player had locked the camera in lookaround, clear it back.
		if ( mp_skaterCamera )
		{
			mp_skaterCamera->ClearLookaroundLock();
		}
		*/
		//printf ("STUBBED: Skater.cpp line %d - not clearing lookaroundlock\n",__LINE__);

		// Reset the skater back to a collidable state
		if( gamenet_man->InNetGame())
		{   
			CFuncs::ScriptNotifyBailDone( NULL, NULL );
			mp_skater_core_physics_component->SetFlagFalse( IS_BAILING );
		}
	}

    if (mp_vibration_component)
	{
		mp_vibration_component->Reset();
	}

	if (node != -1)
	{
		MoveToRestart(node);
	}
	       
	ResetPhysics(walk);

	dodgy_test(); //printf("Setting animation flipped to %d in SkipToRestart\n",mp_skater_core_physics_component->GetFlag(FLIPPED));
	
	if (IsLocalClient())
	{
		// reset any flippedness of animation
		GetSkaterFlipAndRotateComponent()->ApplyFlipState();
	}

	// Only local skaters should run the script
	if( m_local_client )
	{
		if (!m_viewing_mode)
		{
			UpdateCameras( true );
			
			Obj::CCompositeObject *p_obj = GetCamera();
			if (p_obj)
			{
				p_obj->Update();	 // Not the best way of doing it...
			}

			
		}
		if (node != -1)
		{
			Script::CStruct *pNode = NULL;	
			Script::CArray *pNodeArray=Script::GetArray(CRCD(0xc472ecc5,"NodeArray"));
			pNode=pNodeArray->GetStructure(node);
			// Now run any TriggerScript associated with this node
			uint32 script;
			if (pNode->GetChecksum(0x2ca8a299,&script))
			{
	#			ifdef	DEBUG_TRIGGERS			
				dodgy_test(); printf("%d: Restart Node Script triggered, script name %s, node %d\n",(int)Tmr::GetVblanks(),(char*)Script::FindChecksumName(script),node);
	#			endif			
				SpawnAndRunScript(script,node);
			}
		}	  	
	}
			
	SetTeleported();

	if(	( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0x6c5ff266,"netctf") ) ||
		( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0x6ef8fda0,"netking") ) ||
		( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0x5d32129c,"king") ))
	{
		Net::Client* client;
		GameNet::PlayerInfo* player;
		Net::MsgDesc msg_desc;

		client = gamenet_man->GetClient( m_skater_number );
		Dbg_Assert( client );
		player = gamenet_man->GetPlayerByObjectID( GetID());
		Dbg_Assert( player );
		player->MarkAsRestarting();

		msg_desc.m_Id = GameNet::MSG_ID_MOVED_TO_RESTART;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
		client->EnqueueMessageToServer( &msg_desc );
	}

}

//////////////////////////////////////////////////////////////////////////
// START of "Stat" code


float	CSkater::GetStat(EStat stat)
{
	

	float value;			   

	float	override;
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	override = skate_mod->GetStatOverride();
	
	if (override != 0.0f)
	{
		value = override;
	}
	else
	{
		if (m_stat[stat] == -1.0f)
		{
			value =  GetPhysicsFloat(CRCD(0x6357d57c,"Skater_Default_Stats"));
		}
		else
		{
			value =  m_stat[stat];
		}
	}

	// In a network game, always set stats to 10	
//	if( GameNet::Manager::Instance()->InNetGame() )
//	{
//		value = 10.0f;
//	}

	if( GameNet::Manager::Instance()->InNetGame())
	{
		GameNet::PlayerInfo* player;

		player = GameNet::Manager::Instance()->GetPlayerByObjectID( GetID());
		if( player )
		{
			if( player->IsKing())
			{
				value = 1.0f;
			}
			else if( player->HasCTFFlag())
			{
				value = 6.0f;
			}
			else
			{
				value = 10.0f;
			}
		}
		else
		{
			value = 10.0f;
		}
	}
	else if( CFuncs::ScriptInSplitScreenGame( NULL, NULL ))
	{
		GameNet::PlayerInfo* player;

		player = GameNet::Manager::Instance()->GetPlayerByObjectID( GetID());
		if( player )
		{
			if( player->IsKing())
			{
				value = 1.0f;
			}
		}
	}

	if( GameNet::Manager::Instance()->InNetGame())
	{
		GameNet::PlayerInfo* player;

		player = GameNet::Manager::Instance()->GetPlayerByObjectID( GetID());
		if( player )
		{
			if( player->IsKing())
			{
				value = 1.0f;
			}
			else if( player->HasCTFFlag())
			{
				value = 6.0f;
			}
			else
			{
				value = 10.0f;
			}
		}
		else
		{
			value = 10.0f;
		}
	}
	else if( CFuncs::ScriptInSplitScreenGame( NULL, NULL ))
	{
		GameNet::PlayerInfo* player;

		player = GameNet::Manager::Instance()->GetPlayerByObjectID( GetID());
		if( player )
		{
			if( player->IsKing())
			{
				value = 1.0f;
			}
		}
	}

	if (CHEAT_STATS_13)
	{
		value = 15.0f;
	}

	Dbg_Assert(mp_skater_score_component);
	if (mp_skater_score_component->GetScore()->GetSpecialState())
	{
		value += 3.0f;
	}

	return value;
	
}


void	CSkater::SetStat(EStat stat, float value)
{

	// factor in the handicap if any...
	if ( CFuncs::ScriptInSplitScreenGame( NULL, NULL ) )
	{
		 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
		value += skate_mod->GetHandicap( m_id );

		// clamp it to the min, max values
		if ( value > 10.0f ) value = 10.0f;
		if ( value < 0.0f ) value = 0.0f;
	}
	
	m_stat[stat] = value;
}


// Gets a stat value given a pointer to the stat structure.
float	CSkater::GetScriptedStat(Script::CStruct *pSS)
{
	
	Script::CPair range;
	Script::CPair switch_range;
	int stat;
	float limit;

    Dbg_MsgAssert(pSS,("NULL pSS sent to GetScriptedStat"));
	
	if (!pSS->GetPair(NONAME,&range))
	{
		Dbg_MsgAssert(0,("range pair not found in structure."));		
	}

	float stat_value;
	if (!pSS->GetInteger(NONAME,&stat))
	{
		stat_value = 10.0f;	   		// default to 10 if no stats
	}
	else
	{
		stat_value = GetStat((CSkater::EStat)stat);
	}
	
	float lower = range.mX;
	float upper = range.mY;  
	float len = upper - lower;
	float value = lower + (len * stat_value / 10);


	if (mp_skater_core_physics_component->IsSwitched())		// if skating in switch stance
	{
		if (pSS->GetPair(0x9016b4e7,&switch_range))	   		// switch
		{
			float	switch_stat = GetStat(STATS_SWITCH);   	// returns 0..13
			float 	mult = switch_range.mX + (switch_range.mY - switch_range.mX) * switch_stat/10.0f;
			
			if (mult <0.0f) mult = 0.0f;
			if (mult >1.0f) mult = 1.0f;		// this ensures we are never Better in switch

			value *= mult;			
			
		}
	}
	
	Game::GOAL_MANAGER_DIFFICULTY_LEVEL level = Mdl::Skate::Instance()->GetGoalManager()->GetDifficultyLevel();
	if (level != Game::GOAL_MANAGER_DIFFICULTY_MEDIUM)
	{
		Script::CPair diff;	
		if (pSS->GetPair(CRCD(0xba8fb854,"diff"),&diff))
		{
			if (level == Game::GOAL_MANAGER_DIFFICULTY_LOW)
			{
				value *= diff.mX;
			}
			else if (level == Game::GOAL_MANAGER_DIFFICULTY_HIGH)
			{
				value *= diff.mY;
			}
		}
	}	
	
	// if there is a limit, then clamp the value to it
	// accounting for the direction of the range, this may be a high limit
	// or a low limit, but applies to the value for higher values of stat_value.
	// so if lower > upper, then it's a low limit
	if (pSS->GetFloat(0x8069179f/*"limit"*/,&limit))
	{
		if (lower < upper)
		{
			if (value > limit)
			{
				value = limit;
			}
		}
		else
		{
			if (value < limit)
			{
				value = limit;
			}
		}
	}
	else
	{
		// there is no limit, so we do nothing.
	}
	
	return value;
}

// Gets a stat value given the name of the stat, where the stat is defined as a global structure.
float	CSkater::GetScriptedStat(const uint32 checksum)
{
	
   // Get the structure that contains the range, stat and limit
	Script::CStruct *pSS = NULL; 
	Script::CStruct *pPhysics; 
	// Try to get it from the skater/biker physics structure first
	pPhysics = get_physics_struct();
	if (! pPhysics->GetStructure(checksum,&pSS))
	{
		pSS= Script::GetStructure(checksum);
	}
	
	Dbg_MsgAssert(pSS, ("State %s not found", Script::FindChecksumName(checksum)));

	// These asserts are also in the other GetScriptedStat,
	// but included here too so that it can print the name of the structure.
	#ifdef __NOPT_ASSERT__
	Script::CPair range;
	if (!pSS->GetPair(NONAME,&range))
	{
		Dbg_MsgAssert(0,("range pair not found in %s.",Script::FindChecksumName(checksum)));		
	}

	int stat;
	if (!pSS->GetInteger(NONAME,&stat))
	{
		Dbg_MsgAssert(0,("stat not found in %s.",Script::FindChecksumName(checksum)));		
	}
	#endif
		
	return GetScriptedStat(pSS);
}

// Gets a stat value given the name of the stat, where the stat is defined as a global structure.


// Gets a stat value given the name of the stat within a structure pSetOfStats.
// This is used in manual.cpp.
// Different sets of control & wobble stats are stored in structures in physics.q
float CSkater::GetScriptedStat(uint32 Checksum, Script::CStruct *pSetOfStats)
{
	Dbg_MsgAssert(pSetOfStats,("NULL pSetOfStats."));
	Script::CStruct *pSS=NULL;
		pSetOfStats->GetStructure(Checksum,&pSS);
	Dbg_MsgAssert(pSS,("Could not find stat called %s in pSetOfStats",Script::FindChecksumName(Checksum)));
	
	return GetScriptedStat(pSS);
}


// END of "Stat" code
/////////////////////////////////////////////////////////////////////////////

void CSkater::MoveToRandomRestart( void )
{
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	int node, retry;
	
	

	// Try to go to a different restart point than last time.  But still, limit
	// the number of retries just in case there is only one restart node in the level
	retry = 0;
	do
	{
		node = skate_mod->find_restart_node( Mth::Rnd( Mdl::Skate::vMAX_SKATERS ));
		retry++;
	} while(( node == m_last_restart_node ) && ( retry < 20 ));

	m_last_restart_node = node;

	Dbg_Printf( "In MoveToRandomRestart: Node %d\n", node );
    
	if (node != -1)
	{
		SkipToRestart(node);   					   
	}
}

void CSkater::UpdateCameras( bool instantly )
{
//	printf ("%d: SUTUBBBEDDDDDDDDDDDDDDDDDDDDDDDD  UpdateCameras(%d)\n",__LINE__,instantly);	
	if (instantly)
	{
		SetTeleported( false );
	}
}

void CSkater::UpdateCamera( bool instantly )
{
//	printf ("%d: SUTUBBBEDDDDDDDDDDDDDDDDDDDDDDDD  UpdateCamera(%d)\n",__LINE__,instantly);	
	if (instantly)
	{
		SetTeleported( false );
	}
}

Gfx::Camera* CSkater::GetActiveCamera( void )
{

	return Nx::CViewportManager::sGetActiveCamera(m_skater_number);
}
	
CCompositeObject* CSkater::GetCamera()
{
	uint32 camera_name = CRCD(0x967c138c,"SkaterCam0");
	if (m_skater_number == 1)
	{
		camera_name = CRCD(0xe17b231a,"SkaterCam1");
	}
	return static_cast< CCompositeObject* >(CCompositeObjectManager::Instance()->GetObjectByID(camera_name));
}

void CSkater::SetViewMode( EViewMode view_mode )
{
	
	 Mdl::FrontEnd* front = Mdl::FrontEnd::Instance();
	//HUD::PanelMgr* panel_mgr = HUD::PanelMgr::Instance();

	if( m_viewing_mode == view_mode )
	{
		return;
	}
	

	// don't want to do it to skaters that don't have this component
	// (which would be the nonlocal skaters)	
	if (!GetSkaterPhysicsControlComponent())
	{
		return;
	}
	
	m_viewing_mode = view_mode;
			  
	switch ( m_viewing_mode )
	{
		// Normal skaing around with camera
		case GAMEPLAY_SKATER_ACTIVE:
		{
			// Regular gameplay, panel and console on
			if (GetPhysicsInt(CRCD(0x9816e1e3,"ScreenShotMode")))
			{
				dodgy_test(); printf("UnPausing game\n");
				front->PauseGame(false);
			}
		}
		break;

		// select+X pressed once, skater is frozen										  
		case VIEWER_SKATER_PAUSED:
		{
			if (GetPhysicsInt(CRCD(0x9816e1e3,"ScreenShotMode")))
			{
				dodgy_test(); printf("Freezing game\n");
				front->PauseGame(true);
			}
			else
			{
				GetSkaterPhysicsControlComponent()->SuspendPhysics(true);
			}
		}
		break;
		
		// select+X pressed again, skater skates around					 
		case VIEWER_SKATER_ACTIVE:
		{
			// pause the  game, turn off console
			if (GetPhysicsInt(CRCD(0x9816e1e3,"ScreenShotMode")))
			{
				// Dan: should we skip VIEWER_SKATER_ACTIVE then ScreenShotMode is true?
			}
			else
			{
				GetSkaterPhysicsControlComponent()->SuspendPhysics(false);
			}
		}
		break;
		
		
		default:
		Dbg_Assert( 0 );
	}
}

void CSkater::Pause()
{
	if (!mPaused && m_local_client )
	{
		MARK;
		
		// Pause the base object
		CMovingObject::Pause();

		// Switch off the meters. They will come back on by themselves when the skater is unpaused
		// and his logic is called again.
		Dbg_Assert(mp_skater_score_component)	   // Might not have a panel in the front end
		
		// NOTE:  These two lines make the balance meter vanish in "Screenshot" mode
		// But are needed for regular game pausing
		mp_skater_score_component->GetScore()->SetBalanceMeter(false);
		mp_skater_score_component->GetScore()->SetManualMeter(false);
		
		Script::RunScript(CRCD(0x1277e3fd,"pause_run_timer"), NULL);
		
		// Pause the input device (such as vibrations)
		if (mp_input_component)
		{
			mp_input_component->PauseDevice();
		}
        
		// Pause the game sounds
		Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
		sfx_manager->PauseSounds();
	}
}

void CSkater::UnPause()
{
	if (mPaused && m_local_client)
	{
		MARK;
		
		CMovingObject::UnPause();
		
		if (mp_input_component)
		{
			mp_input_component->UnPauseDevice();
		}

		CSkaterLoopingSoundComponent *p_sound_loop_component = GetSkaterLoopingSoundComponent();
		if (p_sound_loop_component)
		{
			p_sound_loop_component->UnPause();
		}
	}	
}

// Ken: Factored this out so that the replay code can call it too.
void CSkater::UpdateShadow(const Mth::Vector& pos, const Mth::Matrix& matrix)
{
#ifndef __PLAT_NGPS__
	static Mth::Vector ground_dir( 0.8f, -0.8f, 0.3f );
	static Mth::Vector air_dir( 0.0f, -1.0f, 0.0f );
	
	// If lights are active, set the ground direction to be that of the primary light.
	if( GetModel())
	{
		Nx::CModelLights *p_lights = GetModel()->GetModelLights();
		if( p_lights )
		{
			ground_dir = p_lights->GetLightDirection( 0 ) * -1.0f;

			if( ground_dir[Y] > -0.65f )
			{
				// Lighting direction is too shallow, leading to overly extended shadows. Limit direction.
				// In the new vector, we know we want the [Y] component to be -0.65, so it follows that we
				// want ( [X]^2 + [Z]^2 ) to be ( 1.0 - ( -0.65 ^2 )), or 0.5775. First, figure out the
				// current value of ( [X]^2 + [Z]^2 ).
				float xz_squared	= ( ground_dir[X] * ground_dir[X] ) + ( ground_dir[Z] * ground_dir[Z] );
				float multiple		= sqrtf( 0.5775f / xz_squared );
				ground_dir[X]		= ground_dir[X] * multiple;
				ground_dir[Y]		= -0.65f;
				ground_dir[Z]		= ground_dir[Z] * multiple;
			}
		}
	}
	
	bool ground = true;
	if( mp_skater_physics_control_component && ( mp_skater_physics_control_component->IsSkating()))
	{
		ground = mp_skater_core_physics_component->GetState() == GROUND;
	}
	else if( mp_walk_component )
	{
		ground = mp_walk_component->GetState() != CWalkComponent::WALKING_AIR;
	}
	
	// Interpolate between the two shadow directions based on Skater state.
	if( ground )
	{
		m_air_timer = ( m_air_timer > 0 ) ? ( m_air_timer - 1 ) : 0;
	}
	else
	{
		m_air_timer = ( m_air_timer < 16 ) ? ( m_air_timer + 1 ) : 16;
	}

	Mth::Vector dir = Mth::Lerp(ground_dir, air_dir, m_air_timer * (1.0f / 16.0f));
	// Mth::Vector dir = ground_dir + (( air_dir - ground_dir ) * ((float)m_air_timer / 16.0f ));
	mp_shadow_component->SetShadowDirection( dir );
#ifdef __PLAT_NGC__
	NxNgc::EngineGlobals.skater_shadow_dir.x = dir[X];
	NxNgc::EngineGlobals.skater_shadow_dir.y = dir[Y];
	NxNgc::EngineGlobals.skater_shadow_dir.z = dir[Z];
	NxNgc::EngineGlobals.skater_height = GetSkaterStateComponent()->GetHeight();

#endif		// __PLAT_NGC__ 

#endif

	/*
	GJ:  The following has been moved to CShadowComponent::Update()
	
	Mth::Vector shadow_target_pos = pos + ( matrix.GetUp() * 36.0f );

	if ( pShadowComponent->mp_shadow )
	{
		if (pShadowComponent->mp_shadow->GetShadowType()==Gfx::vSIMPLE_SHADOW)
		{
			pShadowComponent->mp_shadow->UpdatePosition(m_shadow_pos,m_matrix,pShadowComponent->m_shadow_normal);
		}
		else
		{
			pShadowComponent->mp_shadow->UpdatePosition(shadow_target_pos); // at this point m_pos is the same as m_pos
		}
	}
	*/
}

// K: Added for use by replay code so that it can playback pad vibrations
SIO::Device *CSkater::GetDevice()
{
	// Stubbed for now
	// Dbg_MsgAssert(m_input_handler,("NULL m_input_handler"));
	// Dbg_MsgAssert(m_input_handler->m_Device,("NULL m_input_handler->m_Device ?"));
	// return m_input_handler->m_Device;
	return NULL;
}

int			CSkater::GetHeapIndex( void )
{
	return m_heap_index;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

CSkater::CSkater ( int player_num, CBaseManager* p_obj_manager, bool local_client, int obj_id,
						int heap_num, bool should_tristrip ) : m_link_node( this ), m_local_client( local_client )
{
	m_type = SKATE_TYPE_SKATER;
	
	m_in_world = false;
	m_heap_index = heap_num;
	m_id = obj_id;

	// Mick:  Set up the Skater and Skater2 aliases	
	// Note we use heap_num rather than player_num, as player_num is unreliable in network games
	if (heap_num == 0)
	{
		Obj::CTracker::Instance()->AddAlias(CRCD(0x5b8ab877,"Skater"),this);		
	}
	if (heap_num == 1)
	{
		Obj::CTracker::Instance()->AddAlias(CRCD(0x6ed3fa7,"Skater2"),this);				
	}
	
	// low priority (but not lower than cameras) ensures that moving objects we may skate on or
	// cars we may drive will be updated before the skater each frame
	m_node.SetPri(-500);
	p_obj_manager->RegisterObject(*this);
	
	m_skater_number = player_num;
	
	DUMPI(m_skater_number);
	DUMPI(m_id);

	SetProfileColor(0x00c0c0);				// yellow = skater
	
	mp_animation_component=NULL;
	mp_model_component=NULL;
	mp_shadow_component=NULL;
	mp_trick_component=NULL;
	mp_vibration_component=NULL;
	mp_input_component=NULL;
	mp_skater_sound_component=NULL;
	mp_skater_gap_component=NULL;
	mp_skater_score_component=NULL;
	mp_skater_core_physics_component=NULL;
    mp_stats_manager_component=NULL;
	mp_walk_component=NULL;
	mp_skater_physics_control_component=NULL;

	// create the following components before CMovingObject creates components
	
	Script::CStruct* component_struct = new Script::CStruct;
	
	component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERSTATE);
	CreateComponentFromStructure(component_struct);
																  
	if (m_local_client)
	{
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_INPUT);
		CreateComponentFromStructure(component_struct);
		mp_input_component = GetInputComponent();
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERSCORE);
		CreateComponentFromStructure(component_struct);
		mp_skater_score_component = GetSkaterScoreComponent();

		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERMATRIXQUERIES);
		CreateComponentFromStructure(component_struct);

		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_TRICK);
		CreateComponentFromStructure(component_struct);
		mp_trick_component = GetTrickComponent();
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERPHYSICSCONTROL);
		CreateComponentFromStructure(component_struct);
		mp_skater_physics_control_component=GetSkaterPhysicsControlComponent();

		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERCOREPHYSICS);
		CreateComponentFromStructure(component_struct);
		mp_skater_core_physics_component = GetSkaterCorePhysicsComponent();

		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERROTATE);
		CreateComponentFromStructure(component_struct);

		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERGAP);
		CreateComponentFromStructure(component_struct);
		mp_skater_gap_component = GetSkaterGapComponent();
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERADJUSTPHYSICS);
		CreateComponentFromStructure(component_struct);
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_TRIGGER);
		CreateComponentFromStructure(component_struct);
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERFINALIZEPHYSICS);
		CreateComponentFromStructure(component_struct);
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERCLEANUPSTATE);
		CreateComponentFromStructure(component_struct);
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_WALK);
		CreateComponentFromStructure(component_struct);
		mp_walk_component = GetWalkComponent();
		
#		ifdef TESTING_GUNSLINGER
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_RIDER);
		CreateComponentFromStructure(component_struct);

		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_WEAPON);
		CreateComponentFromStructure(component_struct);
#		endif

		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERLOCALNETLOGIC);
		CreateComponentFromStructure(component_struct);
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERENDRUN);
		CreateComponentFromStructure(component_struct);
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERBALANCETRICK);
		CreateComponentFromStructure(component_struct);
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERLOOPINGSOUND);
		CreateComponentFromStructure(component_struct);

        component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_STATSMANAGER);
		CreateComponentFromStructure(component_struct);
        mp_stats_manager_component = GetStatsManagerComponent();
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_MOVABLECONTACT);
		CreateComponentFromStructure(component_struct);
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERRUNTIMER);
		CreateComponentFromStructure(component_struct);
	}
	else
	{
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERSCORE);
		CreateComponentFromStructure(component_struct);
		mp_skater_score_component = GetSkaterScoreComponent();

		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERNONLOCALNETLOGIC);
		CreateComponentFromStructure(component_struct);
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERCLEANUPSTATE);
		CreateComponentFromStructure(component_struct);
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERFLOATINGNAME);
		CreateComponentFromStructure(component_struct);

		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERENDRUN);
		CreateComponentFromStructure(component_struct);
		
		component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERLOOPINGSOUND);
		CreateComponentFromStructure(component_struct);
	}
	
	component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERSTATEHISTORY);
	CreateComponentFromStructure(component_struct);
	mp_skater_state_history_component = GetSkaterStateHistoryComponent();
	
	component_struct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_SKATERFLIPANDROTATE);
	CreateComponentFromStructure(component_struct);
									
    delete component_struct;
}

void CSkater::Construct ( Obj::CSkaterProfile* pSkaterProfile)
{
	// only skater 0 should update the camera in the
	// skateshop (otherwise, reloading skater 1 would
	// cause a momentary flicker)
	Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	bool should_create_cam = ( !skate_mod->GetGameMode()->IsFrontEnd() ) || ( m_skater_number == 0 );

	if( m_local_client && should_create_cam )
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());

		uint32	name = CRCD(0x967c138c,"SkaterCam0");
		if (m_skater_number == 1)
		{
			name = CRCD(0xe17b231a,"SkaterCam1");
		}
		
			   
		CCompositeObject *p_cam_object = (CCompositeObject*)CCompositeObjectManager::Instance()->GetObjectByID(name);
		if (p_cam_object)
		{
			// Camera already exists, was being used as an observer camera,
			// so just attache it back to this skater
			GetSkaterCameraComponentFromObject(p_cam_object)->SetSkater( this );
			GetWalkCameraComponentFromObject(p_cam_object)->SetSkater( this );
		}
		else
		
		{
			// Add the parameters for the skater camera component		
			// so it can get me as the target
			Script::CStruct * p_component_params = new Script::CStruct;
			p_component_params->AddChecksum(CRCD(0x4c48da58,"CameraTarget"), GetID());
			// p_component_params->AddChecksum(CRCD(0x1313e3c,"ProximTriggerTarget"), GetID());
			p_component_params->AddChecksum(CRCD(0xa1dc81f9,"name"),name);
	
			p_cam_object = CCompositeObjectManager::Instance()->CreateCompositeObjectFromNode(
													Script::GetArray("skatercam_composite_structure"),p_component_params);
			delete	p_component_params;
		}		
		
		GetWalkComponent()->SetAssociatedCamera(p_cam_object);
		CFuncs::SetActiveCamera(name, m_skater_number, false);
		
#if 0 
// proximtrigger components are deprecated  	
		// point the viewer camera's proximity trigger at the skater
		// if it exists, which it might not, if the viewer is not active (Like for the XBox)
		CObject* p_viewer_cam = CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0xeb17151b,"viewer_cam"));
		if (p_viewer_cam)
		{
			GetProximTriggerComponentFromObject(static_cast< CCompositeObject* >(p_viewer_cam))->SetScriptTarget(this);
		}
#endif		
		
		// Lock the object so it won't get deleted.
		p_cam_object->SetLockOn();
		
			// Set default mode, so "behind" etc, is set tosomething sensible 
		GetSkaterCameraComponentFromObject(p_cam_object)->SetMode(CSkaterCameraComponent::SKATERCAM_MODE_NORMAL_MEDIUM, 0.0f);

		/*
		CCameraComponent *p_cam_component				= new CCameraComponent();
		CSkaterCameraComponent *p_skatercam_component	= new CSkaterCameraComponent();

		// We want the CameraComponent processed *after* the SkaterCameraComponent.
		p_cam_object->AddComponent( p_skatercam_component );
		p_cam_object->AddComponent( p_cam_component );

		p_skatercam_component->InitFromStructure( NULL );
		p_cam_component->InitFromStructure( NULL );

		p_skatercam_component->SetSkater( this );
		*/
		
		Mem::Manager::sHandle().PopContext();
	}

	// Set up the AI script.
	Dbg_MsgAssert(mp_script==NULL,("mp_script not NULL?"));

	mp_script=new Script::CScript;


// SkaterInit used to be run there --------------> *	
	
	Script::CStruct* pSkaterInfoParams = Script::GetStructure(CRCD(0x962e5cf7,"skater_asset_info"));

	uint32 skeleton_name = CRCD(0x5a9d2a0a,"human");
	if (pSkaterInfoParams)
	{
		pSkaterInfoParams->GetChecksum("skater_skeleton",&skeleton_name,Script::NO_ASSERT);
	}
    
//--------------------------------------------------------
	// components need to be created in a specific
	// order...  the model needs to come as late as
	// possible, so that all the other components can
	// play around with its display matrix.  (maybe 
	// should have some kind of priority system for 
	// components)?
	
	MovingObjectCreateComponents();

	Dbg_MsgAssert( !GetAnimationComponent(), ( "Animation component already exists" ) );
	Script::CStruct* pAnimationStruct = new Script::CStruct;
	pAnimationStruct->AddChecksum( CRCD(0xb6015ea8,"component"), CRC_ANIMATION );
	pAnimationStruct->AddChecksum( CRCD(0x5b8c6dc2,"AnimEventTableName"), CRCD(0x3fec31a6,"SkaterAnimEventTable") );
	CreateComponentFromStructure(pAnimationStruct, NULL);
    delete pAnimationStruct;
	mp_animation_component = GetAnimationComponent();
		
	Dbg_MsgAssert( !GetSkeletonComponent(), ( "Skeleton component already exists" ) );
	Script::CStruct* pSkeletonStruct = new Script::CStruct;
	pSkeletonStruct->AddChecksum( CRCD(0xb6015ea8,"component"), CRC_SKELETON );
    pSkeletonStruct->AddChecksum( CRCD(0x222756d5,"skeleton"), skeleton_name );
    pSkeletonStruct->AddInteger( CRCD(0xd3982061,"max_bone_skip_lod"), 0 );
	CreateComponentFromStructure(pSkeletonStruct, NULL);
    delete pSkeletonStruct;
	
	#if 0
	// Dan: testing
	Script::CStruct* pStruct = new Script::CStruct;
	pStruct->AddChecksum(CRCD(0xb6015ea8,"component"), CRC_RIBBON);
    pStruct->AddChecksum(CRCD(0xcab94088,"bone"), CRCD(0x7ee14cfe,"Bone_Wrist_L"));
    pStruct->AddInteger(CRCD(0x69feef91,"num_links"), 15);
    pStruct->AddFloat(CRCD(0x9f4625c2,"link_length"), 5.0f);
    pStruct->AddChecksum(CRCD(0x99a9b716,"color"), MAKE_RGB(0, 0, 255));
	CreateComponentFromStructure(pStruct, NULL);
    delete pStruct;
	#endif
	
//--------------------------------------------------------
 	
	uint32 anim_script_name = CRCD(0x501949bd,"animload_human");
	if (pSkaterInfoParams)
	{
		pSkaterInfoParams->GetChecksum(CRCD(0x8b7488e,"skater_anims"),&anim_script_name,Script::NO_ASSERT);
	}
	mp_animation_component->SetAnims( anim_script_name );
	mp_animation_component->EnableBlending( true );
	
	// Create an empty model 
	Dbg_MsgAssert( !GetModelComponent(), ( "Model component already exists" ) );
	Script::CStruct* pModelStruct = new Script::CStruct;
	pModelStruct->AddChecksum( NONAME, CRCD(0x10079f2d,"UseModelLights") );
	
	// Enables the shadow volume on any meshes that get added in the future
	int shadowVolumeEnabled = 0;
	if ( m_local_client )
	{
		shadowVolumeEnabled = true;
	}
	pModelStruct->AddInteger( CRCD(0x2a1f92c0,"ShadowVolume"), shadowVolumeEnabled ); 
	
	pModelStruct->AddChecksum( CRCD(0xb6015ea8,"component"), CRC_MODEL );
	CreateComponentFromStructure(pModelStruct, NULL);
	delete pModelStruct;
	mp_model_component = GetModelComponent();
	
	// create the model
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterGeomHeap(m_heap_index));
	
	Ass::CAssMan * ass_man =  Ass::CAssMan::Instance();
	bool defaultPermanent = ass_man->GetDefaultPermanent();
	ass_man->SetDefaultPermanent( false );
	
	// load up the model from a profile
	mp_model_component->InitModelFromProfile( pSkaterProfile->GetAppearance(), false, m_heap_index );
	
	ass_man->SetDefaultPermanent( defaultPermanent );
	
	Mem::Manager::sHandle().PopContext();

	// Create Model lights
	// (This has been moved to the CModelComponent)
//	GetModel()->CreateModelLights();
//	Nx::CModelLights *p_lights = GetModel()->GetModelLights();
//	p_lights->SetPositionPointer(&m_pos);

	// now that the model has been created,
	// create a shadow component for the skater
	Dbg_MsgAssert( !GetShadowComponent(), ( "Shadow component already exists" ) );
	Script::CStruct* pShadowStruct = new Script::CStruct;
	pShadowStruct->AddChecksum( CRCD(0xb6015ea8,"component"), CRC_SHADOW );
    pShadowStruct->AddChecksum( CRCD(0x9ac24b18,"shadowType"), (CRCD(0x76a54cd1,"detailed")) );
	CreateComponentFromStructure(pShadowStruct, NULL);
    delete pShadowStruct;
	mp_shadow_component = GetShadowComponent();

	if ( m_local_client )
	{
		Dbg_MsgAssert( !GetVibrationComponent(), ( "Vibration component already exists" ) );
		Script::CStruct* p_vibration_struct = new Script::CStruct;
		p_vibration_struct->AddChecksum( CRCD(0xb6015ea8,"component"), CRC_VIBRATION );
		CreateComponentFromStructure(p_vibration_struct, NULL);
		delete p_vibration_struct;
		mp_vibration_component = GetVibrationComponent();

		Dbg_MsgAssert( !GetSkaterStancePanelComponent(), ( "StancePanel component already exists" ) );
		Script::CStruct* p_stancepanel_struct = new Script::CStruct;
		p_stancepanel_struct->AddChecksum( CRCD(0xb6015ea8,"component"), CRC_SKATERSTANCEPANEL );
		CreateComponentFromStructure(p_stancepanel_struct, NULL);
		delete p_stancepanel_struct;
	}
	else
	{
		mp_vibration_component = NULL;
	}
	
	if ( m_local_client )
	{
		Dbg_MsgAssert( !GetSkaterSoundComponent(), ( "SkaterSound component already exists" ) );
		Script::CStruct* p_skater_sound_struct = new Script::CStruct;
		p_skater_sound_struct->AddChecksum( CRCD(0xb6015ea8,"component"), CRC_SKATERSOUND );
		CreateComponentFromStructure(p_skater_sound_struct, NULL);
		delete p_skater_sound_struct;
		mp_skater_sound_component = GetSkaterSoundComponent();
	}
	
	// Finalize Components
	Finalize();									
	
	// Set up the controller preferences.
	Dbg_MsgAssert(m_skater_number>=0 && m_skater_number<Mdl::Skate::vMAX_SKATERS,("Eh? Bad m_skater_number"));
	
	if (IsLocalClient())
	{
		mp_skater_core_physics_component->m_auto_kick = skate_mod->mp_controller_preferences[m_skater_number].AutoKickOn;
		mp_skater_core_physics_component->m_spin_taps = skate_mod->mp_controller_preferences[m_skater_number].SpinTapsOn;
		mp_vibration_component->SetActiveState(skate_mod->mp_controller_preferences[m_skater_number].VibrationOn);
	}


	// Mick:  Run the SkaterInit Script
	// this used to be done before all the components are created, but it makes more sense to do it after
	// and the old way boke after I moved the "SetSkaterCamOverride" into SkaterCameraComponent
	if ( m_local_client )
	{
		// Start running the ground AI.
		#ifdef __NOPT_ASSERT__
		mp_script->SetCommentString("Created in CSkater constructor, assigned to CSkater::mp_script");
		#endif
		mp_script->SetScript("SkaterInit",NULL,this);
	}

	
	UpdateSkaterInfo( pSkaterProfile );

	UpdateStats( pSkaterProfile );
	
	if ( m_local_client )
	{
		GetSkaterSoundComponent()->SetMaxSpeed(GetScriptedStat(CRCD(0xcc5f87aa,"Skater_Max_Max_Speed_Stat")));
	}
	

	/*
	// K: For the debug info above the skater.
	mp_drawer=new Fnt::Drawer();
	mp_drawer->SetRenderState(Image::Drawer::vRS_ZBUFFER | Image::Drawer::vRS_ALPHABLENDING);
	mp_drawer->SetFont("small.fnt");
	mp_drawer->SetJust(Image::vJUST_CENTER_X,Image::vJUST_CENTER_Y);
	mp_drawer->SetShadowState(true,0,0);
	*/

	if (mp_trick_component)
	{
		// K: Create and fill in the trick mappings structure.
		mp_trick_component->UpdateTrickMappings( pSkaterProfile );

		// Initialise the event buffer.
		mp_trick_component->ClearEventBuffer();
		
		// Initialise the trick queue
		mp_trick_component->ClearTrickQueue();
		mp_trick_component->ClearManualTrick();
		mp_trick_component->ClearExtraGrindTrick();

		mp_trick_component->SetAssociatedScore(mp_skater_score_component->GetScore());
	}
	if (mp_skater_gap_component)
	{
		mp_skater_gap_component->SetAssociatedScore(mp_skater_score_component->GetScore());
	}
	
	mSparksRequireRail=true;


	Obj::CCollisionComponent* p_collision_component = GetCollisionComponent();
	if ( p_collision_component )
	{
		if (p_collision_component->GetCollision())
		{
			// Garrett: Make skater non-collidable for now
			p_collision_component->GetCollision()->SetObjectFlags(mSD_NON_COLLIDABLE);
		}
	}
	
	// Init collision cache for CFeelers
	if (m_local_client)
	{
		// mp_collision_cache = Nx::CCollCacheManager::sCreateCollCache();
	}
	else
	{
		// mp_collision_cache = NULL;
	}

	// make sure it doesn't flash at the origin for a brief moment,
	mp_model_component->FinalizeModelInitialization();

	// this needs to happen after the m_id has been assigned
	Script::RunScript( CRCD(0xae5438af,"InitSkaterParticles"), NULL, this );

	// safety-check to make sure he doesn't start out in the blair witch position
	// (because of our animation LOD-ing system)
	// (must be done before the models get initialized)
    mp_animation_component->PlaySequence(CRCD(0x1ca1ff20,"default"));
    
    if (m_local_client)
	{
		// add created tricks
		for (int i=0; i < Game::vMAX_CREATED_TRICKS; i++)
		{
			m_created_trick[i] = new Game::CCreateATrick;
		}
        Script::RunScript( CRCD(0xbdc1c89,"spawn_add_premade_cats_to_skater"), NULL, this );
        
		// setup the correct physics state
		CCompositeObject::CallMemberFunction(CRCD(0x5c038f9b,"SkaterPhysicsControl_SwitchWalkingToSkating"), NULL, NULL);
	}
}

void CSkater::Init(Obj::CSkaterProfile* pSkaterProfile)
{
	Construct(pSkaterProfile);

	GameNet::Manager * gamenet_manager =  GameNet::Manager::Instance();

// Note, both tasks are registered as LOW_PRIORITY, so they come after everything else
// this ensures the movement of the skater is in sync with objects the skater stands on
	
	if( m_local_client && CFuncs::ScriptInMultiplayerGame( NULL, NULL ))
	{
		Net::App* client;
	
		client = gamenet_manager->GetClient( m_skater_number );
		Dbg_Assert( client );
		   
		client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SCORE, Mdl::Score::s_handle_score_message, 0, this );
		client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SKATER_COLLIDE_WON, CSkaterStateHistoryComponent::sHandleCollision, 0, this );
		client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SKATER_COLLIDE_LOST, CSkaterStateHistoryComponent::sHandleCollision, 0, this );
		client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SKATER_PROJECTILE_HIT_TARGET, CSkaterStateHistoryComponent::sHandleProjectileHit, 0, this );
		client->m_Dispatcher.AddHandler( GameNet::MSG_ID_SKATER_HIT_BY_PROJECTILE, CSkaterStateHistoryComponent::sHandleProjectileHit, 0, this );
		client->m_Dispatcher.AddHandler( GameNet::MSG_ID_STEAL_MESSAGE, CSkaterLocalNetLogicComponent::sHandleStealMessage, 0, this );
	}
		
	// moved to CSkaterLocalNetLogicComponent
	// m_last_update_time = 0;
    
	Obj::CSuspendComponent* pSuspendComponent = GetSuspendComponent();
	if ( pSuspendComponent )
	{
		pSuspendComponent->m_lod_dist[0] = 3000;	// was 500, but in multi-player, we generally want to see subtle animations animating
													// and in single player, you should always be close to the camera anyway
		pSuspendComponent->m_lod_dist[1] = 5000; 	// we pretty much always want to see the other player, even at a great distance
		pSuspendComponent->m_lod_dist[2] = 10000000;
		pSuspendComponent->m_lod_dist[3] = 10000000;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkater::UpdateStats( CSkaterProfile* pSkaterProfile )
{		
	// GJ:  apply the stats contained in the skater profile;  we'll need
	// need to override them after this function, if we're applying cheats
	
	this->SetStat( STATS_AIR, pSkaterProfile->GetStatValue((CRCD(0x439f4704,"air"))) );
	this->SetStat( STATS_RUN, pSkaterProfile->GetStatValue((CRCD(0xaf895b3f,"run"))) );
	this->SetStat( STATS_OLLIE, pSkaterProfile->GetStatValue((CRCD(0x9b65d7b8,"ollie"))) );
	this->SetStat( STATS_SPEED, pSkaterProfile->GetStatValue((CRCD(0xf0d90109,"speed"))) );
	this->SetStat( STATS_SPIN, pSkaterProfile->GetStatValue((CRCD(0xedf5db70,"spin"))) );
	this->SetStat( STATS_FLIPSPEED, pSkaterProfile->GetStatValue((CRCD(0x6dcb497c,"flip_speed"))) );
	this->SetStat( STATS_SWITCH, pSkaterProfile->GetStatValue((CRCD(0x9016b4e7,"switch"))) );
	this->SetStat( STATS_RAILBALANCE, pSkaterProfile->GetStatValue((CRCD(0xf73a13e3,"rail_balance"))) );
	this->SetStat( STATS_LIPBALANCE, pSkaterProfile->GetStatValue((CRCD(0xae798769,"lip_balance"))) );
	this->SetStat( STATS_MANUAL, pSkaterProfile->GetStatValue((CRCD(0xb1fc0722,"manual_balance"))) );

#ifdef __USER_GARY__
	dodgy_test(); printf("Air    %2.2f\n",GetStat(STATS_AIR));		
	dodgy_test(); printf("Run    %2.2f\n",GetStat(STATS_RUN));		
	dodgy_test(); printf("Ollie  %2.2f\n",GetStat(STATS_OLLIE));		
	dodgy_test(); printf("Speed  %2.2f\n",GetStat(STATS_SPEED));		
	dodgy_test(); printf("Spin   %2.2f\n",GetStat(STATS_SPIN));		
	dodgy_test(); printf("Flip   %2.2f\n",GetStat(STATS_FLIPSPEED));		
	dodgy_test(); printf("Switch %2.2f\n",GetStat(STATS_SWITCH));		
	dodgy_test(); printf("Rail   %2.2f\n",GetStat(STATS_RAILBALANCE));		
	dodgy_test(); printf("Lip    %2.2f\n",GetStat(STATS_LIPBALANCE));		
	dodgy_test(); printf("Manual %2.2f\n",GetStat(STATS_MANUAL));
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkater::UpdateSkaterInfo( Obj::CSkaterProfile* pSkaterProfile )
{
	// get some info from the skater profile
	// while we still have access to it...
	Dbg_MsgAssert( pSkaterProfile,( "No profile supplied" ));

	m_pushStyle = pSkaterProfile->GetChecksumValue( (CRCD(0xc15dbf86,"pushstyle")) );
	m_isGoofy = pSkaterProfile->GetChecksumValue( (CRCD(0x7d02bcc3,"stance")) ) == ( CRCD(0x287fcd4e,"goofy") );
	m_isPro = pSkaterProfile->IsPro();
	m_displayName = pSkaterProfile->GetDisplayName();
	
	// needed for playing correct streams
	pSkaterProfile->GetInfo()->GetInteger( CRCD(0x3f813177,"is_male"), &m_isMale, Script::ASSERT );

	const char* pFirstName;
	pSkaterProfile->GetInfo()->GetText( CRCD(0x562e3ecd,"first_name"), &pFirstName, Script::ASSERT );
	Dbg_MsgAssert( strlen( pFirstName) < MAX_LEN_FIRST_NAME,
				   ( "First name %s is too long (max = %d chars)", m_firstName, MAX_LEN_FIRST_NAME ) );
	strcpy( m_firstName, pFirstName );
	
	m_skaterNameChecksum = pSkaterProfile->GetSkaterNameChecksum();
	
	if (IsLocalClient())
	{
		mp_skater_core_physics_component->SetFlag(FLIPPED, !m_isGoofy);
		GetSkaterFlipAndRotateComponent()->ApplyFlipState();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkater::~CSkater ( void )
{ 
// Taken care of by the shadow component's destructor
//	Obj::CShadowComponent* pShadowComponent = GetShadowComponent();
//	if ( pShadowComponent )
//	{
//		pShadowComponent->SwitchOffShadow();
//	}

	// if (mp_collision_cache)
	// {
		// Nx::CCollCacheManager::sDestroyCollCache(mp_collision_cache);
	// }
	
	// We must unpause the controller before we destruct ourselves.  Otherwise, the
	// paused/unpaused toggle will become out of sync with any future skater which
	// uses this controller.
	if (mPaused && m_local_client && mp_input_component)
	{
		mp_input_component->GetDevice()->UnPause();
		mp_input_component->GetDevice()->StopAllVibrationIncludingSaved();
	}

	DeleteResources();	   

	Script::RunScript( CRCD(0xf429e0ac,"DestroySkaterParticles"), NULL, this );

	for (int i=0; i < Game::vMAX_CREATED_TRICKS; i++)
    {
        delete m_created_trick[i];
    }

	// (Mick) If there is a camera still associated with this skater, then delete it
	// this will be either SkaterCam0 or SkaterCam1, and only on local clients 	
	// (Steve) only delete player 2's camera. For now, at least, player 1's camera
	// is persistent because it is used in observer mode
	if (IsLocalClient() && ( m_skater_number == 1 ))
	{
		CCompositeObject *p_cam = GetCamera();
		if (p_cam)
		{
			//delete p_cam;
			p_cam->MarkAsDead();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Reset skater state (physics, score, et al.), such as when you're retrying
void CSkater::Reset()
{
	// MARK;
	
	Dbg_Assert(mp_skater_score_component);
	mp_skater_score_component->Reset();
	
	// clear max times (records) for the various balance tricks
	Dbg_Assert(GetSkaterBalanceTrickComponent());
	GetSkaterBalanceTrickComponent()->ClearMaxTimes();	
	
	ResetAnimation();
	
	Dbg_Assert(mp_skater_core_physics_component);
	int i;
	for ( i = 0; i < NUM_ESKATERFLAGS; i++ )
	{
		mp_skater_core_physics_component->SetFlag( ( ESkaterFlag )i, true );
		mp_skater_core_physics_component->SetFlag( ( ESkaterFlag )i, false );
	}

	// Reset the pad:
	Dbg_Assert(mp_input_component);
	mp_input_component->GetControlPad().Reset();
}


// DeleteResources will delete things the skater has allocated that
// we do not want to stick around when we do a cleanup of everything else
// we should really split the skater into temporary and permanent parts
void CSkater::DeleteResources(void)
{
	if (IsLocalClient())
	{
		mp_skater_gap_component->ClearActiveGaps();
		
		GetSkaterBalanceTrickComponent()->ClearBalanceParameters();
	}
	
	// we don't want to carry over special items
	// from one level to another (they would
	// fragment memory anyway)
	
	Obj::CSpecialItemComponent* pSpecialItemComponent = GetSpecialItemComponent();
	Dbg_Assert(pSpecialItemComponent);
	pSpecialItemComponent->DestroyAllSpecialItems();

	// need to get rid of any animations
	// that the skater is currently playing,
	// or else there will be CBlendChannels
	// fragmenting the bottom-up heap.
	mp_animation_component->Reset();
	
	
#ifdef	__SCRIPT_EVENT_TABLE__		
	if (mp_script)
	{
		delete mp_script;
		mp_script = NULL;
		
	}
#else
	// Clear the event receiver associated with this objects
	// They will get set up again next time we add or remove one
	mp_event_handler_table->unregister_all(this);
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


void CSkater::ResetStates()
{
	Dbg_Assert( IsLocalClient());

	GetSkaterPhysicsControlComponent()->SuspendPhysics(false);
	
	GetSkaterFlipAndRotateComponent()->Reset();
	
//	#ifdef __PLAT_NGC__
//	mDoDeferredRotateAfter=false;
//	mDeferredFlipAndRotate=false;
//	#endif

	// Ken stuff, reset other misc stuff such as the event buffer, trick queue, blaa blaa
	
//	GetExceptionComponent()->ClearExceptions();

	Dbg_Assert( mp_trick_component );
	mp_trick_component->ClearQueueTricksArrays();
	mp_trick_component->ClearManualTrick();
	mp_trick_component->ClearExtraGrindTrick();
	mp_trick_component->ClearEventBuffer();
	mp_trick_component->ClearTrickQueue();
	mp_trick_component->ClearMiscellaneous();
	
	GetSkaterStateComponent()->Reset();

	// If set this will make the call to UpdateModel() use the display matrix flipped.
	// Used when running the lip-trick out-anim, to stop it appearing flipped until
	// the anim has finished.
	mp_model_component->mFlipDisplayMatrix=false;
	
	// Make sure the meters are off.
	Dbg_Assert( mp_skater_score_component )
	mp_skater_score_component->GetScore()->SetBalanceMeter(false);
	mp_skater_score_component->GetScore()->SetManualMeter(false);
	
	// Reset the panel
//	Mdl::Score *pScore=GetScoreObject();
//	pScore->BailRequest();	// effectively cancels the current score pot

	// Reset doing-trick flag so that he doesn't bail on the drop-in if he died whilst
	// doing a trick.
	
	if (mp_vibration_component)
	{
		mp_vibration_component->StopAllVibration();
	}	

	// set the shadow to stick to his feet, assuming we are on the ground
	// Note: These are used by the simple shadow, not the detailed one.
	mp_shadow_component->SetShadowPos( m_pos );
	mp_shadow_component->SetShadowNormal( m_matrix[Y] );

	mSparksRequireRail=true;
	
	GetSkaterStancePanelComponent()->Reset();
}

void CSkater::ResetAnimation()
{
	Dbg_Assert( IsLocalClient());
    
	// Put the board orientation back to normal
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	Net::Client* client;

	client = gamenet_man->GetClient( GetHeapIndex());
	Dbg_Assert( client );

	CSkaterFlipAndRotateComponent* p_flip_and_rotate_component = GetSkaterFlipAndRotateComponent();
	p_flip_and_rotate_component->RotateSkateboard(GetID(), false, client->m_Timestamp, IsLocalClient());
	p_flip_and_rotate_component->ApplyFlipState();
	
	// Make sure the board is visible
	mp_model_component->HideGeom( CRCD(0xa7a9d4b8,"board"), false, IsLocalClient() );
	
	/*
	// Make sure all atomics are visible
	// (GJ:  Not really sure why the previous board unhiding is still necessary)
	pModelComponent->HideGeom((CRCD(0xc4e78e22,"all")), false, IsLocalClient());
	
	// unhiding all the atomics will conflict with the "invisible" cheat	
	*/
	  
	// getting into certain physics states
	// will reset the animation, so we want to 
	// make sure we've always got some animation
	// to be playing...
	mp_animation_component->Reset();

//	GJ:  don't start playing a new animation, because
//  it's possible that the animations have been unloaded
//	mp_animation_component->PlaySequence( CRCD(0x1ca1ff20,"default") );
//	mp_animation_component->SetLoopingType( Gfx::LOOPING_CYCLE, true );
}

void CSkater::ResetInitScript(bool walk, bool in_cleanup)
{
// Mick: Finally update the "SkaterInit" script, so it does any flag clearing that 
// is needed by the game
// we do this now, otherwise some other script could issue a MakeSkaterGoto, and clear this script
// so it would never get run.
	if ( !mp_script )
	{
		mp_script = new Script::CScript;
	}
	
	Script::CStruct* p_params = NULL;
	if (walk || in_cleanup)
	{
		p_params = new Script::CStruct;
		if (walk)
		{
			p_params->AddChecksum(NO_NAME, CRCD(0x726e85aa,"Walk"));
		}
		if (in_cleanup)
		{
			p_params->AddChecksum(NO_NAME, CRCD(0x7b738580,"InCleanup"));
		}
	}
	
	mp_script->SetScript(CRCD(0xe740f458,"SkaterInit"), p_params, this);
	if( IsLocalClient())
	{
		mp_script->Update();
	}
	if (p_params)
	{
		delete p_params;
	}
}

// Reset physics flags, position, velocity, etc...
void CSkater::ResetPhysics(bool walk, bool in_cleanup)
{
	// MARK;
	Dbg_Assert( IsLocalClient());

	// Massive function to reset all skater's members so that
	// replay becomes deterministicalistic
	// ResetEverything( );
	
	// switch back to walking
	if (mp_skater_physics_control_component->IsWalking())
	{
		Script::CStruct* p_params = new Script::CStruct;
		p_params->AddChecksum(CRCD(0x91d0d784, "NewScript"), CRCD(0x1dde4804, "OnGroundAi"));
		JumpToScript(CRCD(0x93a4e402, "Switch_OnGroundAI"), p_params);
		delete p_params;
	}
	
	// make sure the OnExitRun script gets run before DeleteResources clears the script
	if (mp_script && mp_script->GetOnExitScriptChecksum())
	{
		uint32 on_exit_script_name_checksum = mp_script->GetOnExitScriptChecksum();
		mp_script->SetOnExitScriptChecksum(0);
		mp_script->Interrupt(on_exit_script_name_checksum);
	}
	
	mp_skater_core_physics_component->Reset();

	GetSkaterCameraComponentFromObject(GetCamera())->ResetMode();

	ResetStates();
	DeleteResources();
	ResetAnimation();		  
	ResetInitScript(walk, in_cleanup);
	
	GetSkaterEndRunComponent()->ClearIsEndingRun();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkater::SkaterEquals( Script::CStruct* pStructure )
{
	// TODO:  Maybe stick this data inside the skater's tags,
	// so that we can create new fields without having to
	// update this function?

	Dbg_MsgAssert( pStructure,( "No structure supplied" ));

	if ( pStructure->ContainsFlag( CRCD(0xd82f8ac8,"is_pro") ) )
	{
		if ( !m_isPro )
			return false;
	}

	if ( pStructure->ContainsFlag( CRCD(0x5a3264bb,"is_custom") ) )
	{
		if ( m_isPro )
			return false;
	}

	if ( pStructure->ContainsFlag( CRCD(0x3f813177,"is_male") ) )
	{
		if ( !m_isMale )
			return false;
	}
	
	if ( pStructure->ContainsFlag( CRCD(0xe5c2f84c,"is_female") ) )
	{
		if ( m_isMale )
			return false;
	}
	
	uint32 is_named;
	if ( pStructure->GetChecksum( CRCD(0xc2e3d008,"is_named"), &is_named ) )
	{
		if ( is_named != m_skaterNameChecksum )
			return false;
	}

	uint32 stance;
	if ( pStructure->GetChecksum( CRCD(0x7d02bcc3,"stance"), &stance ) )
	{
		uint32 myStance = ( m_isGoofy ? CRCD(0x287fcd4e,"goofy") : CRCD(0xb58efc2b,"regular") );
		if ( stance != myStance )
			return false;
	}

	// if we've gotten to the end of the function,
	// that means that all of the criteria matched
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkater::CallMemberFunction( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript )
{
	
	Dbg_MsgAssert(pScript,("NULL pScript"));
	
	return _function0( Checksum, pParams, pScript );
}

bool CSkater::_function0( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript )
{
	
	switch (Checksum)
	{
        // @script | InLocalSkater | returns true if the skater is a local client
		case 0x7015bc33: // IsLocalSkater
		{
			return IsLocalClient();
		}
		
        // @script | MoveToRandomRestart | moves the skater to a random
        // restart node
		case 0xbf450bab: // MoveToRandomRestart
		{
			MoveToRandomRestart();
			break;
		}

        // @script | SkaterIsNamed | true if skater name matches specified name
        // @uparm name | skater name
		case 0x1c5612ee: // SkaterIsNamed
		{
			uint32 checksum;
			pParams->GetChecksum( NONAME, &checksum, Script::ASSERT );
			return ( checksum == m_skaterNameChecksum );
		}
		break;
		
		// @script | GetCameraId | sets the skater's camera id to CameraId
		case CRCC(0x97c8285a,"GetCameraId"):
		{
			uint32 camera_name = CRCD(0x967c138c,"SkaterCam0");
			if (m_skater_number == 1)
			{
				camera_name = CRCD(0xe17b231a,"SkaterCam1");
			}
			pScript->GetParams()->AddChecksum(CRCD(0x7a039889,"CameraId"), camera_name);
			break;
		}

        // @script | RemoveSkaterFromWorld | 
		case 0xbce85494:	// RemoveSkaterFromWorld
		{
			 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

			skate_mod->HideSkater( this, true );
			return true;
		}
		break;

        // @script | AddSkaterToWorld | 
		case 0x0c09c1b7:	// AddSkaterToWorld
		{
			 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

			skate_mod->HideSkater( this, false );
			//AddToCurrentWorld();
			return true;
		}
		break;

		#if 0
		case 0x7fa39d17:	// ResetLookAround
		{
			Dbg_Message("Skater:ResetLookAround does not work");
			if (GetSkaterCameraComponent())
			{
				GetSkaterCameraComponent()->ResetLookAround();	
			}
			return true;
		}
		break;
		#endif
		
		case 0x72aecfc0:	// ResetRigidBodyCollisionRadiusBoost
			ResetRigidBodyCollisionRadiusBoost();
			break;

		default:
			return _function1( Checksum, pParams, pScript );
			break;
	}
	return true;
}
			
bool CSkater::_function1( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript )
{
	
	
	switch (Checksum)
	{
		/////////////////////////////////////////////////////////////////////////////////////////////
        // @script | KillSkater | Killskater will reset the skater
        // to a node, and set his speed to 0. If this command is
        // run from a node, and that node is linked to a restart
        // node, then the skater will skip to that restart node,
        // and the script in that restart node will be executed
		// @parmopt name | Node | | name of node to jump to
		// @parmopt name | prefix | | restart at nearest node matching this prefix
		// @flag RestartWalking | restart the skater in walk physics mode
		case 0x1b572abf: // KillSkater 
		{
			// default just clear skater skater
			// and reset to origin		
			
			bool walk = pParams->ContainsComponentNamed(CRCD(0xd6f06bf6,"RestartWalking"));
			
			// If there is a "node" parameter, then skip to that 
			// Mick, optionally add a "prefix" parameter, and skip to the nearest node that matches
			// this prefix (will need a new node funtion:
			//  int	GetNearestNodeByPrefix(const char *p_prefix, const Mth::Vector &pos)

			const char *p_prefix = NULL;
			uint32	prefix_name = 0;
			if (!pParams->GetChecksum(CRCD(0x6c4e7971,"prefix"),&prefix_name))
			{
				if (pParams->GetString(CRCD(0x6c4e7971,"prefix"),&p_prefix))
				{
					prefix_name = Crc::GenerateCRCFromString(p_prefix);
				}
			}
			if (prefix_name)
			{
				int	target_node = SkateScript::GetNearestNodeByPrefix(prefix_name,m_pos);
				Dbg_MsgAssert(target_node != -1,("%s\nNo restart node found with prefix 0x%x, %s",pScript->GetScriptInfo(),prefix_name, p_prefix));
				SkipToRestart(target_node, walk);  
				break;
			}
			
			
			uint32 node_name=0;
			if (pParams->GetChecksum(CRCD(0x7a8017ba,"Node"),&node_name))
			{
				int target_node = SkateScript::FindNamedNode(node_name);
				Dbg_MsgAssert(target_node != -1,("%\n Tried to kill skater to non-existant node",pScript->GetScriptInfo()));
				SkipToRestart(target_node, walk); 
				break;
			}
			else
			{
				
				int node = pScript->mNode;
				Dbg_MsgAssert(node !=  -1,( "%s\nKillSkater called from non node script with no 'node' parameter",pScript->GetScriptInfo()));			
				{
					// Otherwise, check the links to see if we are linked to a restart point
				
					int links = SkateScript::GetNumLinks(node);
					int link;
					for (link = 0;link<links;link++)
					{
						int linked_node = SkateScript::GetLink(node,link);
						if (IsRestart(linked_node))
						{
							// if the node is linked to a restart point
							// then we just skip to that restart point
							SkipToRestart(linked_node, walk); 
							break;   
						}
					}
					if (link == links)
					{
						#if 1
						if (Ed::CParkEditor::Instance()->UsingCustomPark()) 		// is it a custom park???
						{
							Mdl::Skate* skate_mod = Mdl::Skate::Instance();
							skate_mod->skip_to_restart_point( this, -1, walk );	// just skip to standard restart node			
						}
						else
						#endif
						{
							Dbg_MsgAssert(0,( "%s\nKillSkater called, but node %d not linked to restart",pScript->GetScriptInfo(),node));			
						}
					}
				}
			}			
			break;
		}
		
        // @script | SparksOn | turn sparks on
        // @flag RailNotRequired | 
		case 0xe62f94a2: // SparksOn
			Replay::WriteSparksOn();
			Script::RunScript( CRCD(0xaf4475de,"sparks_on"), pParams, this );
			mSparksRequireRail=true;
			mSparksOn=true;
			if (pParams->ContainsFlag("RailNotRequired"))
			{
				mSparksRequireRail=false;
			}	
			break;

        // @script | SparksOff | turn sparks off
		case 0x5436a335: // SparksOff
			Replay::WriteSparksOff();
			Script::RunScript( CRCD(0x0dccf5c3,"sparks_off"), pParams, this );
			mSparksOn=false;
			mSparksRequireRail=true;
			break;

        // @script | BloodOff | turn blood off
        // @parm string | bone | bone name
		case 0xc67c18d7: // BloodOff
		{
			const char* p_bone_name;
			if (!pParams->GetText( CRCD(0xcab94088,"bone"), &p_bone_name ))
			{
				Dbg_MsgAssert(0,("%s\nBloodOff requires a bone name"));
				return 0;
			}
			
			// nothing doing......

            break;
		}
   
        // @script | Die | can't be called on the player
		case 0xc6870028:// "Die"
			Dbg_MsgAssert( 0,( "\n%s\nDie can't be called on the player.\nIs somebody putting 'Die' in a trigger script?", pScript->GetScriptInfo( ) ));
			break;

        // @script | ShouldMongo | true if should mongo
		case 0x3e8eb713:	// ShouldMongo
		{
			if( !IsLocalClient())
			{
				return false;
			}
			switch ( m_pushStyle )
			{
				case vALWAYS_MONGO:
					return true;
					break;
				case vMONGO_WHEN_SWITCH:
					return ( mp_skater_core_physics_component->IsSwitched() );
					break;
				case vNEVER_MONGO:
				default:
					return false;
					break;
			}
		}
			break;

        // @script | SetCustomRestart |  Skater command to set the current position as the "custom restart point" 
		// and also to query or clear the status of this restart, based on:
        // @flag set | set the restart
		// @flag clear | clear the restart
		// if no flags are passed, then it will return true if restart point is valid 
		case 0x42832375:			// SetCustomRestart
		{
			if (pParams->ContainsFlag(CRCD(0x19ebda23,"set")))
			{
				m_restart_valid = true;
				m_restart_pos = m_pos;
				m_restart_matrix = m_matrix;			
				m_restart_walking = mp_skater_physics_control_component->IsWalking();
			}
			else if (pParams->ContainsFlag(CRCD(0x1a4e0ef9,"clear")))
			{
				m_restart_valid = false;
				break;
			}
			else
			{
				return m_restart_valid;
			}
			
			//break;
		}
		// Note: Intentional fallthrough!  We want setting the point to have the same effect as skipping to it
		
        // @script | SkipToCustomRestart |  Skater command to jump to previously set custom restart 		
		case 0x5a3c19e9:		// SkipToCustomRestart
		{
			SkipToRestart(-1, m_restart_walking);
			m_pos = m_restart_pos;
			DUMP_SPOSITION
			#ifdef	DEBUG_POSITION
			dodgy_test(); printf("%d: Position = (%.2f,%.2f,%.2f)\n",__LINE__,m_pos[X],m_pos[Y],m_pos[Z]);
			#endif
			m_matrix = m_restart_matrix;
			mp_skater_core_physics_component->m_safe_pos = m_pos;				// for uberfrig
			m_old_pos = m_pos;				// ensure we don't get any nasty triggerings....
			SetTeleported();
			
			mp_skater_core_physics_component->ResetLerpingMatrix();
			#ifdef		DEBUG_DISPLAY_MATRIX
			dodgy_test(); printf("%d: Setting display_matrix[Y][Y] to %f, [X][X] to %f\n",__LINE__,mp_skater_core_physics_component->m_lerping_display_matrix[Y][Y],mp_skater_core_physics_component->m_lerping_display_matrix[X][X]);
			#endif
			mp_skater_core_physics_component->m_display_normal = mp_skater_core_physics_component->m_last_display_normal = mp_skater_core_physics_component->m_current_normal = m_matrix[Y];
			// set the shadow to stick to his feet, assuming we are on the ground
			// Note: These are used by the simple shadow, not the detailed one.
			mp_shadow_component->SetShadowPos( m_pos );
			mp_shadow_component->SetShadowNormal( m_matrix[Y] );
			break;
		}

        // @script | GetStat |  Get a stat value in the float stat_value, so it can be used in a script expression
		// @parm int | Stat | index of stat in stat array, see 
		case 0x7915aa31:			// GetStat
		{
			int stat;
			pParams->GetInteger(CRCD(0xdf4700de,"Stat"),&stat,true);
			pScript->GetParams()->AddFloat(CRCD(0x8eaf7add,"stat_value"),GetStat((EStat)stat));
			break;
		}

        // @script | GetScriptedStat |  Get a stat value in the float stat_value
		// parameters are as used in physics.q
		// and you would generally pass in a structure, like Skater_Max_Speed_Stat
		// rather than the individual values
		case 0x9abe8a21:			// GetScriptedStat
		{
//			Script::PrintContents(pParams);
			Script::CStruct *p_struct = NULL;
			pParams->GetStructure(NONAME,&p_struct,true);
			pScript->GetParams()->AddFloat(CRCD(0x8eaf7add,"stat_value"),GetScriptedStat(p_struct));
			break;
		}
		
		// @script | GetSkaterNumber | Gets the skater number, and puts it into a parameter called
		// SkaterNumber
		case 0xf4c52f72: // GetSkaterNumber 
		{
			pScript->GetParams()->AddInteger(CRCD(0xf3cf5755,"SkaterNumber"),m_skater_number);
			break;
		}
		
		case 0xd38e8b6c: // PlaceBeforeCamera
			{
				if ( GetActiveCamera() )
				{
					Mth::Vector	&cam_pos = GetActiveCamera()->GetPos();
					Mth::Matrix	&cam_mat = GetActiveCamera()->GetMatrix();


					m_matrix.Ident();
					m_pos = cam_pos;
					m_pos[Y] += FEET( 0 );
					m_pos -= cam_mat[Z] * FEET( 12 );
					DUMP_SPOSITION
					m_old_pos = m_pos;			// MoveToRestart 
					mp_skater_core_physics_component->m_safe_pos = m_pos;			// MoveToRestart  
					m_matrix = cam_mat;
					m_matrix[Z] = -m_matrix[Z];
					m_matrix[X] = -m_matrix[X];
					mp_skater_core_physics_component->ResetLerpingMatrix();
					#ifdef		DEBUG_DISPLAY_MATRIX
					dodgy_test(); printf("%d: Setting display_matrix[Y][Y] to %f, [X][X] to %f\n",__LINE__,mp_skater_core_physics_component->m_lerping_display_matrix[Y][Y],mp_skater_core_physics_component->m_lerping_display_matrix[X][X]);
					#endif
					mp_skater_core_physics_component->m_display_normal = mp_skater_core_physics_component->m_last_display_normal = mp_skater_core_physics_component->m_current_normal = m_matrix[Y];
					// set the shadow to stick to his feet, assuming we are on the ground
					// Note: These are used by the simple shadow, not the detailed one.
					mp_shadow_component->SetShadowPos( m_pos );
					mp_shadow_component->SetShadowNormal( m_matrix[Y] );
					// update these for the camera
					m_vel = -cam_mat[Z];  
					m_vel[Y] = 0;  
					m_vel.Normalize(10);  
					UpdateCamera(true);	  	// re-snap the camera to the correct position
					
					#ifdef	DEBUG_VELOCITY
					dodgy_test(); printf("%d: Velocity = (%.2f,%.2f,%.2f) (%f)\n",__LINE__,m_vel[X],m_vel[Y],m_vel[Z],m_vel.Length());
					#endif
				}
			}
			break;

  			Dbg_MsgAssert(0,("Forgot a break?"));		
		
		default:
			return CMovingObject::CallMemberFunction( Checksum, pParams, pScript );
			break;
	}
	return true;
}

// End of CallMemberFunction

void CSkater::SparksOn()
{
	Dbg_Assert( 0 );

	// GJ:  It would be nice to keep all the spark logic in
	// script, but it requires some skater state info in C
	// (like the mSparksRequireRail).  Eventually, we should
	// move this all to script, instead of calling the following
	// script from C code.

	Script::RunScript( CRCD(0xaf4475de,"sparks_on"), NULL, this );
	mSparksOn = true;
}

void CSkater::SparksOff()
{
	// GJ:  It would be nice to keep all the spark logic in
	// script, but it requires some skater state info in C
	// (like the mSparksRequireRail).  Eventually, we should
	// move this all to script, instead of calling the following
	// script from C code.
	if ( mSparksOn )
	{
		Replay::WriteSparksOff();
		Script::RunScript( CRCD(0x430efc31,"TurnOffSkaterSparks"), NULL, this );
		mSparksOn=false;
	}
}
	
// Used by the MakeSkaterGoto script function in cfuncs.cpp
// Causes the skater to jump to the specified script straight away.
void CSkater::JumpToScript(uint32 ScriptChecksum, Script::CStruct *pParams)
{
	Dbg_Assert(IsLocalClient());
		
//	Dbg_MsgAssert(mp_script,("NULL mp_script"));
	if ( !mp_script )
	{
		mp_script = new Script::CScript;
	}

	GetSkaterFlipAndRotateComponent()->DoAnyFlipRotateOrBoardRotateAfters(); // <- See huge comment above definition of this function.
	mp_script->SetScript(ScriptChecksum,pParams,this);
	mp_script->Update();
}
	
void CSkater::JumpToScript(const char *pScriptName, Script::CStruct *pParams)
{
	
	JumpToScript(Script::GenerateCRC(pScriptName),pParams);
}

Mth::Matrix&				CSkater::GetDisplayMatrix( void )
{
	return mp_skater_core_physics_component->m_lerping_display_matrix;
}

void						CSkater::SetDisplayMatrix( Mth::Matrix& matrix )
{
	mp_skater_core_physics_component->m_lerping_display_matrix = matrix;
}

Lst::Node< CSkater >*		CSkater::GetLinkNode( void )
{
	return &m_link_node;
}

void CSkater::Resync( void )
{
	if (!m_local_client)
	{
		GetSkaterNonLocalNetLogicComponent()->Resync();
	}
	
	mp_skater_state_history_component->ResetPosHistory();
	//mp_skater_state_history_component->ResetAnimHistory();
	//mp_skater_state_history_component->SetLatestAnimTimestamp( 0 );
	
}

void CSkater::RemoveFromCurrentWorld( void )
{
	if (IsLocalClient())
	{
		GetSkaterLoopingSoundComponent()->StopLoopingSound();
	}
	
	mp_shadow_component->SwitchOffShadow();
    
	Hide( true );
	m_in_world = false;
}

void	CSkater::AddToCurrentWorld ( void )
{
	mp_shadow_component->SwitchOnSkaterShadow();
	Hide( false );
	m_in_world = true;
}

bool	CSkater::IsInWorld( void )
{
	return m_in_world;
}

const char* CSkater::GetDisplayName()
{
	// set the player's display name for rankings screen,
	// "You got bitch-slapped by X" messages, etc.
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	if( gamenet_man->InNetGame() )
	{
		GameNet::PlayerInfo* pInfo = gamenet_man->GetPlayerByObjectID( m_id );
		Dbg_Assert( pInfo );
		// (Mick) don't assign it to m_displayName, as that can cause memory allocations
		// and fragments the heap
		// ensure you set the appropiate context if you want to do this.
		//m_displayName =  pInfo->m_Name;
		return pInfo->m_Name;
	}
	
	return m_displayName.getString();
}

void CSkater::AddCATInfo(Script::CStruct *pStuff)
{
	Dbg_MsgAssert(pStuff,("NULL pStuff"));
	
    Script::CArray *p_tricks=new Script::CArray;
    p_tricks->SetSizeAndType(Game::vMAX_CREATED_TRICKS,ESYMBOLTYPE_STRUCTURE);
    for (int i=0; i<Game::vMAX_CREATED_TRICKS; ++i)
    {
        Script::CStruct *p_new_trick_struct=new Script::CStruct;
        p_new_trick_struct->AddStructure(CRCD(0x26a7cadf,"other"),m_created_trick[i]->mp_other_params);
        p_new_trick_struct->AddArray(CRCD(0x6b19fc8f,"animations"),m_created_trick[i]->mp_animations);
        p_new_trick_struct->AddArray(CRCD(0x2e628ee6,"rotations"),m_created_trick[i]->mp_rotations);

        //Script::PrintContents( p_new_trick_struct );

        p_tricks->SetStructure(i,p_new_trick_struct);
    }
    pStuff->AddArrayPointer(CRCD(0x1e26fd3e,"Tricks"),p_tricks);
}

void CSkater::LoadCATInfo(Script::CStruct *pStuff)
{
    printf("LoadCATInfo was called\n");
	Dbg_MsgAssert(pStuff,("NULL pStuff"));

    Script::CArray *pCATInfo=NULL;
    pStuff->GetArray(CRCD(0x1e26fd3e,"tricks"),&pCATInfo);

    Dbg_Assert( m_created_trick[1] );

    for (int i=0; i<Game::vMAX_CREATED_TRICKS; ++i)
    {
        Script::CStruct *pTemp=NULL;
        pTemp = pCATInfo->GetStructure(i);
        
        // other params
        Script::CStruct *p_other_params=NULL;
        pTemp->GetStructure(CRCD(0x26a7cadf,"other"),&p_other_params,Script::ASSERT);
        m_created_trick[i]->mp_other_params->AppendStructure( p_other_params );

        // animations
        Script::CArray *p_animation_info=NULL;
        pTemp->GetArray(CRCD(0x6b19fc8f,"animations"),&p_animation_info,Script::ASSERT);
        Script::CopyArray( m_created_trick[i]->mp_animations, p_animation_info );
        
        // rotations
        Script::CArray *p_rotation_info=NULL;
        pTemp->GetArray(CRCD(0x2e628ee6,"rotations"),&p_rotation_info,Script::ASSERT);
        Script::CopyArray( m_created_trick[i]->mp_rotations, p_rotation_info );

        //Script::PrintContents( m_created_trick[i]->mp_other_params );
        //Script::PrintContents( m_created_trick[i]->mp_rotations );
    }

    Script::RunScript(CRCD(0x33fe5817,"kill_load_premade_cats"), NULL);
}

void CSkater::AddStatGoalInfo(Script::CStruct *pStuff)
{
    Dbg_MsgAssert(pStuff,("NULL pStuff"));
	
    Script::CArray *p_stat_goals=new Script::CArray;
    p_stat_goals->SetSizeAndType(Obj::NUM_STATS_GOALS,ESYMBOLTYPE_INTEGER);
    
    for (int i=0; i<Obj::NUM_STATS_GOALS; ++i)
    {
        p_stat_goals->SetInteger(i,mp_stats_manager_component->StatGoalGetStatus(i));
    }
    pStuff->AddArrayPointer(CRCD(0x44b66a9f,"StatGoals"),p_stat_goals);
}

void CSkater::LoadStatGoalInfo(Script::CStruct *pStuff)
{
    Dbg_MsgAssert(pStuff,("NULL pStuff"));

    // Reset Stats Goals and set to proper difficulty
    Script::CStruct* p_params = NULL;
    mp_stats_manager_component->RefreshFromStructure(p_params);

    Script::CArray *pStatGoalInfo=NULL;
    pStuff->GetArray(CRCD(0x44b66a9f,"StatGoals"),&pStatGoalInfo);

    for (int i=0; i<Obj::NUM_STATS_GOALS; ++i)
    {
        mp_stats_manager_component->StatGoalSetStatus(i,pStatGoalInfo->GetInteger(i));
    }
}

void CSkater::AddChapterStatusInfo(Script::CStruct *pStuff)
{
    Dbg_MsgAssert(pStuff,("NULL pStuff"));
	
    pStuff->AddArray(CRCD(0x8e8ee0c0,"ChapterStatus"),Script::GetArray( CRCD(0xdfa17d68,"CHAPTER_STATUS"), Script::ASSERT ));
}

void CSkater::LoadChapterStatusInfo(Script::CStruct *pStuff)
{
    Dbg_MsgAssert(pStuff,("NULL pStuff"));

	uint32 Name = Script::GenerateCRC("ChapterStatus");
    Script::CArray *pChapterStatusInfo=NULL;
    pStuff->GetArray(Name,&pChapterStatusInfo);

    Script::CopyArray(Script::GetArray( CRCD(0xdfa17d68,"CHAPTER_STATUS"), Script::ASSERT ), pChapterStatusInfo);
}

} // namespace Obj
