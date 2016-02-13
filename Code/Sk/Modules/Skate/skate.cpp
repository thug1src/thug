/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Skate Module (SKATE) 									**
**																			**
**	File name:		modules/skate.cpp										**
**																			**
**	Created by:		06/07/2000	-	spg										**
**																			**
**	Description:	Skate Module											**
**																			**
*****************************************************************************/

// start autoduck documentation
// @DOC skate
// @module skate | None
// @subindex Scripting Database
// @index script | skate

// define this to call the movie updatign from the Skate::Mdl update, instead of the fronend up[date
#define	__MOVIES_FROM_SKATE_UPDATE__	

                           
/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/modules/skate/skate.h>

#ifndef __PLAT_XBOX__
#ifndef __PLAT_NGC__
    #include <sifdev.h>
#endif
#endif

#include <core/defines.h>
#include <core/math.h>
#include <core/singleton.h>
#include <core/task.h>

#include <core/string/stringutils.h>

#include <gfx/gfxman.h>
#include <gfx/nx.h>
#include <gfx/nxmiscfx.h>
#include <gfx/nxparticlemgr.h>

#include <gfx/2D/ScreenElement2.h>
#include <gfx/2D/ScreenElemMan.h>

#include <gel/objman.h>
#include <gel/module.h>
#include <gel/mainloop.h>
#include <gel/components/suspendcomponent.h>
#include <gel/components/vibrationcomponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/trickcomponent.h>
#include <gel/components/shadowcomponent.h>
#include <gel/components/railmanagercomponent.h>
#include <gel/music/music.h>
#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/prefs/prefs.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/string.h>
#include <gel/scripting/symboltable.h>
#include <gel/soundfx/soundfx.h>

#include <sk/engine/sounds.h>

#include <sk/gamenet/gamenet.h>

#include <sk/scripting/cfuncs.h>
#include <sk/scripting/nodearray.h>

#include <sk/modules/FrontEnd/FrontEnd.h>
#include <sk/modules/skate/gameflow.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/goalmanager.h>
#include <sk/modules/skate/competition.h>
#include <sk/modules/skate/horse.h>
#include <sk/modules/Viewer/Viewer.h>

#include <sk/objects/crown.h>
#include <sk/objects/moviecam.h>
#include <sk/objects/playerprofilemanager.h>
#include <sk/objects/emitter.h>
#include <sk/objects/proxim.h>
#include <sk/objects/rail.h>
#include <sk/objects/records.h>
#include <sk/objects/restart.h>
#include <sk/objects/skater.h>
#include <sk/objects/skatercareer.h>
#include <sk/objects/skaterprofile.h>
#include <sk/objects/skatertricks.h>
#include <sk/objects/trickObject.h>

#ifdef TESTING_GUNSLINGER
#include <sk/objects/navigation.h>
#endif

#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skaterstatehistorycomponent.h>
#include <sk/components/skaterendruncomponent.h>
#include <sk/components/skaterbalancetrickcomponent.h>
#include <sk/components/RailEditorComponent.h>

#include <sk/parkEditor2/edmap.h>

#include <sys/sys.h>
#include <sys/mem/memman.h>
#include <sys/timer.h>
#include <sys/file/filesys.h>
#include <sys/file/pre.h>
#include <sys/config/config.h>
#include <sys/replay/replay.h>

extern bool	skip_startup;


namespace Front
{
	extern void SetTimeTHPS4(int minutes, int seconds);
    extern void HideTimeTHPS4();
}

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Mdl
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

DefineSingletonClass( Skate, "Skate Module" );

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**							   Private Classes								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

// When going into observer mode the local skater gets destroyed, and is later
// restored from his profile. The created tricks are not stored in the profile,
// so we are storing them out seperately.
static Script::CStruct *spCATData=NULL;
static Script::CStruct *spStatData=NULL;

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Skate::s_logic_code ( const Tsk::Task< Skate >& task )
{
	GameNet::Manager* gamenet_man = GameNet::Manager::Instance();
    
    Dbg_AssertType ( &task, Tsk::Task< Skate > );

	Skate&	mdl = task.GetData();

	mdl.DoUpdate();

	if( gamenet_man->OnServer() && ( mdl.GetNumSkaters() > 0 ))
	{
		mdl.CheckSkaterCollisions();
	}

	if ( mdl.GetGameMode()->IsFrontEnd() )
	{
	}
	else if( ( mdl.GetGameMode()->GetNameChecksum() == CRCD(0x6ef8fda0,"netking") ) ||
			 ( mdl.GetGameMode()->GetNameChecksum() ==  CRCD(0x5d32129c,"king") ))
	{
        if( !mdl.GetGameMode()->EndConditionsMet())
		{
			GameNet::PlayerInfo* king;
			
			king = gamenet_man->GetKingOfTheHill();
			if( king && !king->m_Skater->IsPaused())
			{
				Mdl::Score* score;
				int current_score, total_score, goal_score;
	
				score = king->m_Skater->GetScoreObject();
				current_score = score->GetTotalScore();
				total_score = current_score + (int) (	( Tmr::FrameLength() * 60.0f ) * 
														( Tmr::vRESOLUTION / 60.0f ));
				score->SetTotalScore( total_score );

				goal_score = mdl.GetGameMode()->GetTargetScore();

				if( mdl.GetGameMode()->IsTeamGame())
				{
					total_score = gamenet_man->GetTeamScore( king->m_Team );
				}
				else
				{
					total_score = score->GetTotalScore();
				}

				// When within 10 seconds of victory, play a timer sound periodically
				mdl.BeepTimer( (float) ( goal_score - total_score) / 1000.0f,
							   10.0f,
							   10.0f,
							   "timer_runout_beep");
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Skate::v_start_cb ( void )
{
	// TODO:  Should assert that the top-down heap is empty
	// before the permanent prefiles are loaded
//	Script::RunScript( "DumpHeaps" );
	
	// these prefiles will sit on the top-down heap permanently
	// this is done before other stuff gets put on the top-down
	// heap
	Script::RunScript( "load_permanent_prefiles" );

	m_stat_override = 0.0f;
	   
	Mlp::Manager * mlp_manager =  Mlp::Manager::Instance(); 
                           
	Obj::Proxim_Init();						//
	Obj::CEmitterManager::sInit();

	m_logic_task->SetMask(1<<1);
												   
    // Initialise logic and display tasks
	mlp_manager->AddLogicTask ( *m_logic_task );
    
	GetGameMode()->Reset();

    Script::RunScript("init_loading_bar");
    Script::RunScript("startup_loading_screen");

	// By default we are both client and server
	//gamenet_manager->m_Flags.SetMask( GameNet::mSERVER | GameNet::mCLIENT );
    
	// Run the script for loading sounds that are permanently loaded into sound RAM...
    Script::RunScript("LoadPermSounds");

	// Run the script for loading the skater animations
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	
	Mem::PushMemProfile("Permanent Assets");
	Script::RunScript( "load_permanent_assets" );
	Mem::PopMemProfile(/*"Permanent Assets"*/);

	Mem::Manager::sHandle().PopContext();

	// initialize the pro skater profiles here
	GetPlayerProfileManager()->Init();

	// add the trick checksums used to the lookup table
	mp_trickChecksumTable->Init();

	Script::RunScript("default_system_startup");

	if ( !Config::CD() )
	{
		if ( !skip_startup )
		{
			// Run the personal startup script.
			Script::RunScript("Call_Personal_StartUp_Script");
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::v_stop_cb ( void )
{
    m_logic_task->Remove();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::HideSkater( Obj::CSkater* skater, bool should_hide )
{
	if( should_hide )
	{
		skater->Pause();
	}
	else
	{
		skater->UnPause();
	}

	skater->Hide( should_hide );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int				Skate::GetNextUnusedSkaterHeapIndex( bool for_local_skater )
{
	int start_index, heap_index, num_tested;
	bool taken;

	// Ok... kinda complicated, but it was the simplest solution I could come up with that solves the problem.
	// The issue is that we only have vMAX_SKATERS skater heaps.  And we always want to reserve skater heap 0 
	// for our local skater.  But we might be observing a game that has vMAX_SKATERS, in which case you will
	// not have a heap for the local skater at all.  But if we observe a server that has ( vMAX_SKATERS - 1 )
	// skaters and join late, we want to make sure that our skater (the last one to be loaded) gets skater 
	// heap 0
	start_index = 1;
	if( for_local_skater )
	{
		start_index = 0;
	}

	heap_index = start_index;
	for( num_tested = 0; num_tested < (int) GetGameMode()->GetMaximumNumberOfPlayers(); num_tested ++ )
	{
		taken = false;
		for( uint i = 0; i < GetNumSkaters(); i++ )
		{
			if( m_skaters.GetItem(i) != NULL )
			{
				Obj::CSkater *pSkater = m_skaters.GetItem(i)->GetData();
				if( pSkater->GetHeapIndex() == heap_index )
				{
					taken = true;
					break;
				}
			}
		}
		if( taken == false )
		{
			break;
		}

		heap_index = ( heap_index + 1 ) % GetGameMode()->GetMaximumNumberOfPlayers();
	}
	
	// We assume that these skater heaps are totally empty
	// so if something is aleft on them, then assert

#ifdef	__NOPT_ASSERT__	
	Mem::Heap *skater_heap = Mem::Manager::sHandle().SkaterHeap(heap_index);
	Dbg_MsgAssert( skater_heap != NULL, ( "Invalid skater heap : %d\n", heap_index ));
	if (skater_heap->mUsedBlocks.m_count)
	{
		printf ("Skater heap has %d used blocks still on it, it should be empty\n",skater_heap->mUsedBlocks.m_count);
#ifndef __PLAT_NGC__
		MemView_AnalyzeHeap(skater_heap);
		MemView_DumpHeap(skater_heap);
#endif		// __PLAT_NGC__
		Dbg_MsgAssert(0,("Skater heap has %d used blocks still on it, it should be empty\n",skater_heap->mUsedBlocks.m_count));
	}

	Mem::Heap *geom_heap = Mem::Manager::sHandle().SkaterGeomHeap(heap_index);
	if (geom_heap->mUsedBlocks.m_count)
	{
		printf ("SkaterGeom heap has %d used blocks still on it, it should be empty\n",geom_heap->mUsedBlocks.m_count);
#ifndef __PLAT_NGC__
		MemView_AnalyzeHeap(geom_heap);
		MemView_DumpHeap(geom_heap);
#endif		// __PLAT_NGC__
		Dbg_MsgAssert(0,("SkaterGeom heap has %d used blocks still on it, it should be empty\n",geom_heap->mUsedBlocks.m_count));
	}
	
	#endif
	

	return heap_index;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CSkater*	Skate::add_skater ( Obj::CSkaterProfile* pSkaterProfile, bool local_client, int obj_id, int player_num )
{
	Lst::Node< Obj::CSkater > *node;
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	int heap_index;
    
	// GJ:  copy the face texture (if it has one) from the local client's skater profile
	// we need to do this because the code that sends the face texture data as a separate
	// network message hasn't been implemented yet.
	//Dbg_MsgAssert( local_client, ( "Gary's kludge for face textures only works on local client" ) );
	if ( local_client )
	{
		Gfx::CModelAppearance* pModelAppearance = pSkaterProfile->GetAppearance();
		Obj::CSkaterProfile* pOriginalProfile;
		if (CFuncs::ScriptInSplitScreenGame( NULL, NULL ))
		{
			// (Mick) Kluge modified for 2 players
			// get the correct profile
			pOriginalProfile = mp_PlayerProfileManager->GetProfile( player_num );
		}
		else
		{
			// Otherwise, use the old kluge
			pOriginalProfile = mp_PlayerProfileManager->GetProfile( 0 );
		}
		
		
		Gfx::CModelAppearance* pOriginalAppearance = pOriginalProfile->GetAppearance();

		*pModelAppearance = *pOriginalAppearance;
	}
	
	// Change our ID to what the server says it should be to avoid ID collision amongs clients
	if( local_client )
	{
		Obj::CSkater* skater;

		Script::RunScript( "show_panel_stuff" );
		skater = GetLocalSkater();
		if( skater )
		{   
			// In Split-screen games, we'll have two local skaters
			if( CFuncs::ScriptInSplitScreenGame( NULL, NULL ))
			{
				if( skater->GetID() == (uint32) obj_id )
				{
					return skater;
				}
#ifdef __PLAT_NGC__
				else
				{
					File::PreMgr* pre_mgr = File::PreMgr::Instance();
					pre_mgr->LoadPre( "skaterparts.pre", false);
				}
#endif
			}
			else
			{
				// In Net games, we'll only have one, but his ID can change in order to avoid
				// ID collision over the net.  So, if someone is trying to add another local skater with a different
				// ID, assume that they just want to assign a new ID to our already-existing local skater
				if( skater->GetID() != (uint32) obj_id )
				{   
					Score* score;
	
					node = skater->GetLinkNode();
					node->SetPri( obj_id );
					node->Remove();
					m_skaters.AddUniqueSequence( node );
	
////////////////////////////////////////////////////////////////////////////////////					
// Changing the id means we have to destroy and re-create the particle systems
// as they are referenced by the id of the skater.					
					Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterHeap(skater->GetHeapIndex()));
					Script::RunScript( "DestroySkaterParticles", NULL, skater );

					skater->SetID( obj_id );
					
					Script::RunScript( "InitSkaterParticles", NULL, skater );
					Mem::Manager::sHandle().PopContext();
////////////////////////////////////////////////////////////////////////////////////					
					
					score = skater->GetScoreObject();
					score->SetSkaterId( obj_id );
				}
				
				return skater;
			}
		}
		else
		{
			Obj::CCompositeObject* cam_obj;

			// If we're about to re-create the skater, delete the skatercam as it is about to
			// be re-created.  This is to handle the case where someone goes into observer mode.
			// In this case, we destroy the skater, but leave the camera. So now we're going back
			// to the front end and it's re-creating the skater.
			cam_obj = (Obj::CCompositeObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0x967c138c,"skatercam0"));
			if( cam_obj )
			{
				cam_obj->SetLockOff();
				delete cam_obj;
			}
		}
	}
	else
	{
		// Stop vibration when loading client skaters in net games
		if ( gamenet_man->InNetGame() )
		{
			Obj::CSkater* p_skater;

			p_skater = GetLocalSkater();
			if( p_skater )
			{
				Obj::CVibrationComponent* p_vibration_component = GetVibrationComponentFromObject(p_skater);
				if (p_vibration_component)
				{
					p_vibration_component->StopAllVibration();
				}
			}
		}
	}

	if ( gamenet_man->InNetGame() )
	{
		Dbg_MsgAssert( GetNumSkaters() < GetGameMode()->GetMaximumNumberOfPlayers(),( "Trying to add too many players %d : %d", GetNumSkaters(), GetGameMode()->GetMaximumNumberOfPlayers()));
	}

    Mem::PushMemProfile("Skater"); // Could possibly replace this with the name of the skater, when we have one, so can compare in multi player

		
	Dbg_MsgAssert(( obj_id >= 0 ) && ( obj_id < vMAX_SKATERS ),( "Invalid skater object id\n" ));
	
	heap_index = GetNextUnusedSkaterHeapIndex( local_client );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterHeap(heap_index));

	Obj::CSkater* skater;

	Dbg_MsgAssert( pSkaterProfile,( "No profile specified" ));

	// Garrett: We may need to pause the music here if we get glitches on the network server
	//Pcm::PauseMusic(1);

	skater = new Obj::CSkater ( player_num, GetObjectManager(),
								local_client, obj_id, heap_index, !GetGameMode()->IsFrontEnd() );
	//Pcm::PauseMusic(0);
	skater->Init(pSkaterProfile);

	// Initialize the skater's special items
	if( !skater->IsLocalClient())
	{
		Script::RunScript( "ClientSkaterInit", NULL, skater );
	}

    // This bit of code will order our skaters in the linked list according to their object id
	// so the order will be consistent across all clients
	node = skater->GetLinkNode();
	node->SetPri( skater->GetID() );
	m_skaters.AddUniqueSequence( node );

	if ( GetGameMode()->IsFrontEnd() )
	{
		// front end doesn't have a server, so we need
		// to treat it as a special case.
		skip_to_restart_point( skater );
		Script::RunScript("reset_skateshop_skater");
	}
	
    // check if we should bind this skater to a different controller
	// printf("skaterIndex = %i, player_num = %i\n", mp_controller_preferences[player_num].skaterIndex, player_num);
	UpdateSkaterInputHandlers();

    // restore CAT's
    if( skater->IsLocalClient() && ( skater->GetSkaterNumber() == 0 ))
    {
        if (spCATData)
        {
            printf("CAT: Restoring Created Tricks\n");
            skater->LoadCATInfo(spCATData);
            delete spCATData;
    		spCATData=NULL;
        }
        
        // restore stat goal data too!
        if (spStatData)
        {
            printf("CAT: Restoring Stat Goal data\n");
            skater->LoadStatGoalInfo(spStatData);
            delete spStatData;
    		spStatData=NULL;
        }
    }

    // update trick mappings if this is the second skater in a split screen game
	if ( skater->GetID() == 0x00000001 && CFuncs::ScriptInSplitScreenGame( NULL, NULL ) )
	{
		// Obj::CPlayerProfileManager* pProfileMan = GetPlayerProfileManager();
		// Obj::CSkaterProfile* pProfile = pProfileMan->GetProfile( 1 );
		
		Obj::CTrickComponent* p_trick_component = GetTrickComponentFromObject(skater);
		Dbg_Assert(p_trick_component);
		p_trick_component->UpdateTrickMappings( pSkaterProfile );
	}

	Mem::PopMemProfile(/*"Skater"*/);
	Mem::Manager::sHandle().PopContext();

    return skater;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::GetNextStartingPointData( Mth::Vector* pos, Mth::Matrix* matrix, int obj_id )
{   
    int node;

	pos->Set( 0.0f, 0.0f, 0.0f, 0.0f );
	matrix->Ident();

	node = find_restart_node( obj_id );

    // ...and jump to it
	if (node != -1)
	{
		Mth::Vector rot;

		printf ("Got restart node %d\n",node);
		
		SkateScript::GetAngles( node, &rot );
		if ( rot[ X ] || rot[ Y ] || rot[ Z ] )
		{
			// 3DSMAX_ANGLES Mick: 3/19/03 - Changed all rotation orders to X,Y,Z
			matrix->RotateX( rot[ X ] );
			matrix->RotateY( rot[ Y ] );
			matrix->RotateZ( rot[ Z ] );
		}

		SkateScript::GetPosition( node, pos );
	}
}

/******************************************************************/
/* Find the zero-based restart node. So index 0 is Player1		  */
/*                                                                */
/******************************************************************/

int Skate::find_restart_node( int index )
{
	int node = -1;
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	// get appropriate restart point...
	if( IsMultiplayerGame())
	{
		if( GetGameMode()->IsTeamGame())
		{
			if( GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netlobby" ))
			{
				node = Obj::GetRestartNode( Script::GenerateCRC( "team" ), 0 );
			}
			else if( GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netctf" ))
			{
				GameNet::PlayerInfo* player;
				
				player = gamenet_man->GetLocalPlayer();
				if( player )
				{
					switch( player->m_Team )
					{
						case GameNet::vTEAM_RED:
							node = SkateScript::FindNamedNode( "TRG_CTF_Restart_Red" );
							break;
						case GameNet::vTEAM_BLUE:
							node = SkateScript::FindNamedNode( "TRG_CTF_Restart_Blue" );
							break;
						case GameNet::vTEAM_GREEN:
							node = SkateScript::FindNamedNode( "TRG_CTF_Restart_Green" );
							break;
						case GameNet::vTEAM_YELLOW:
							node = SkateScript::FindNamedNode( "TRG_CTF_Restart_Yellow" );
							break;
					}
				}
			}
			else
			{
				index = gamenet_man->GetSkaterStartingPoint( index );
				node = Obj::GetRestartNode( Script::GenerateCRC( "Multiplayer" ), index );
				// if we did not find one, then try the first generic restart
				if (node == -1)
				{
					printf ("\n\nWARNING - no multiplayer restart point, trying player1\n\n");
					// If no Playern, then try player1
					node = Obj::GetRestartNode( Script::GenerateCRC( "Player1" ), 0 );
				}
			}
		}
		// In the skateshop, use the Player1, Player2 restart points
		else if( GetGameMode()->IsFrontEnd())
		{
			char buff[32];

			sprintf( buff, "Player%d", index + 1 );
			node = Obj::GetRestartNode( Script::GenerateCRC( buff ), 0 );
		}
		else if ( GetGameMode()->IsTrue( "is_horse" ) )
		{
			// grab the current horse node
			node = Obj::GetRestartNode( Script::GenerateCRC( "horse" ), GetHorse()->GetCurrentRestartIndex() );
		}
		else
		{
			index = gamenet_man->GetSkaterStartingPoint( index );
			node = Obj::GetRestartNode( Script::GenerateCRC( "Multiplayer" ), index );
			// if we did not find one, then try the first generic restart
			if (node == -1)
			{
				printf ("\n\nWARNING - no multiplayer restart point, trying player1\n\n");
				// If no Playern, then try player1
				node = Obj::GetRestartNode( Script::GenerateCRC( "Player1" ), 0 );
			}
		}
	}
	else
	{
		node = Obj::GetRestartNode( Script::GenerateCRC( "Player1" ), index );
	}
		
	// If No player 1, then use generic
	if (node == -1)
	{		
		printf ("\n\nWARNING - no Player1 restart point, trying Generic\n\n");
		node = Obj::GetRestartNode( 0, 0 );
	}

	// If No player 1, then use generic
	if( node == -1 )
	{
		printf( "\n\nWARNING - No restart points found\n\n" );
	}

	return node;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::skip_to_restart_point( Obj::CSkater* skater, int node, bool walk )
{
	if (node == -1)
	{
		node = find_restart_node( skater->GetID() );
	}

	// ...and jump to it
	if (node != -1)
	{
		skater->SkipToRestart( node, walk );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::move_to_restart_point( Obj::CSkater* skater )
{
	skip_to_restart_point(skater, -1);
	
/*  Dan: MoveToRestart is deprecated.  We'll use skip instead.
	int node;
	
	node = find_restart_node( skater->GetID() );
    
	if (node != -1)
	{
		skater->MoveToRestart(node);   					   
	}
*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Skate::remove_skater( Obj::CSkater* skater )
{   	
	Dbg_Assert( skater );

    // (Mick) Now check for bailboard, and kill it
	// Bit of a patch really, but a fairly safe high level one.
	// (need a more general solution for killing model that has
	// instances...)
	
	Obj::CCompositeObject*p_bailboard = (Obj::CCompositeObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0x884ef81b, "bailboard") + skater->GetID());
	if( p_bailboard )
	{
		p_bailboard->MarkAsDead();
		Obj::CCompositeObjectManager::Instance()->FlushDeadObjects();
	}
	
	// destroy any car the non-local skater might have
	if( !skater->IsLocalClient() )
	{
		Script::CStruct* p_params = new Script::CStruct;
		p_params->AddChecksum(CRCD(0x5b24faaa, "SkaterId"), skater->GetID());
		Script::RunScript(CRCD(0x5b3fdccb, "remove_car_from_non_local_skater"), p_params);
		delete p_params;
	}

    // save off CAT params
    if( skater->IsLocalClient() && ( skater->GetSkaterNumber() == 0 ))
    {
        if (spCATData)
    	{
    		Dbg_Assert( 0 );
            delete spCATData;
    		spCATData=NULL;
    	}
        
        printf("CAT: Storing Created Trick data\n");
        spCATData=new Script::CStruct;
        skater->AddCATInfo(spCATData);

        // save stat goal info too...
        if (spStatData)
    	{
    		Dbg_Assert( 0 );
            delete spStatData;
    		spStatData=NULL;
    	}
        
        printf("CAT: Storing Stat Goal data\n");
        spStatData=new Script::CStruct;
        skater->AddStatGoalInfo(spStatData);
    }
    
	skater->GetLinkNode()->Remove();
	//Obj::Unlock( skater );
	//Obj::Destroy( skater );
	delete skater;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Skate::SkatersAreIdle()
{
	 //GameNet::Manager* gamenet_man = GameNet::Manager::Instance();

#ifdef __USER_GARY__
	// GJ:  this assert was firing off for some reason.
	// it doesn't seem to have any negative impact, but i'd
	// like to look at it later, when i've got more time
//	Dbg_Assert( GetGameMode()->EndConditionsMet() );
#endif

	bool all_idle = true;

	for ( uint32 i = 0; i < GetNumSkaters(); i++ )
	{
		Obj::CSkater* pSkater = GetSkater( i );
		Dbg_Assert( pSkater );

		if ( GetGameMode()->IsTrue( "is_horse" ) )
		{
			// only need to check the current horse skater
			if ( GetHorse()->GetCurrentSkaterId() != (int) pSkater->GetID() )
			{
				continue;
			}
		}

		Obj::CSkaterEndRunComponent* p_skater_endrun_component = GetSkaterEndRunComponentFromObject(pSkater);
		Dbg_Assert(p_skater_endrun_component);
		if ( !p_skater_endrun_component->RunHasEnded() )
		{
			all_idle = false;
			if( pSkater->IsLocalClient())
			{
				p_skater_endrun_component->EndRun();
			}                     
		}
	}
	
	// In network games, don't base it on whether skater objects are idle as this is unreliable.
	// Instead, base it on "GameIsOver()" which uses reliable transport messages
	/*if( gamenet_man->InNetGame())
	{
		return gamenet_man->GameIsOver();
	}*/

	return all_idle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Skate::FirstTrickStarted()
{
	// you should only call this function in horse mode
	Dbg_Assert( GetGameMode()->IsTrue("is_horse") );
	
	Obj::CSkater* pSkater = GetSkaterById( GetHorse()->GetCurrentSkaterId() );
	Dbg_Assert( pSkater );
															  
	Obj::CTrickComponent* pTrickComponent = GetTrickComponentFromObject(pSkater);
	Dbg_Assert( pTrickComponent );

	if ( pTrickComponent->FirstTrickStarted() )
	{
		// if the trick has started, then end the run, yo!
		Obj::CSkaterEndRunComponent* p_skater_endrun_component = GetSkaterEndRunComponentFromObject(pSkater);
		Dbg_Assert(p_skater_endrun_component);
		p_skater_endrun_component->EndRun();
	}

	return pTrickComponent->FirstTrickStarted();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Skate::FirstTrickCompleted()
{
	// you should only call this function in horse mode
	Dbg_Assert( GetGameMode()->IsTrue("is_horse") );
	
	Obj::CSkater* pSkater = GetSkaterById( GetHorse()->GetCurrentSkaterId() );
	Dbg_Assert( pSkater );
	
	Obj::CTrickComponent* pTrickComponent = GetTrickComponentFromObject(pSkater);
	Dbg_Assert( pTrickComponent );
	
	return pTrickComponent->FirstTrickCompleted();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::PauseGameFlow( bool paused )
{
	

	Dbg_AssertPtr( mp_gameFlow );
	mp_gameFlow->Pause( paused );	
}

//////////////////////////////////////////////////////////////////////
// void Skate::BeepTimer(float time, float beep_time, float beep_speed, const char *p_script_name)
// given a time, run a script if that time is less than beep_time, and again, with increasing
// frequency.  Note use of static vars, means this is a single use fn.  Intetended for a single timer
// time = time remaining in seconds
// beep_time = time at which to start beeping
// beep_speed = relative speed at whcih beeps increase 10.0 is good
// p_script_name = script to run for each beep
//
// example:
//
//			BeepTimer(time,10.0f, 10.0f,"timer_runout_beep");			
//

void Skate::BeepTimer(float time, float beep_time, float beep_speed, const char *p_script_name)
{
	static float next_beep_time;
	if (time > 0.01f && time <= beep_time)
	{
		if (time<next_beep_time)
		{
			Script::RunScript( p_script_name, NULL, NULL );				
			float sub = next_beep_time/beep_speed;
			if  (sub<0.2f)				// limit the delay between beeps 
			{
				sub=0.2f;
			}
			next_beep_time -= sub;			// Make Urgent smaller, but less so as time remain, so frequency rises
		}
	}
	else
	{
		next_beep_time = beep_time;
	}
}			

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void 	do_CheckModelActive(Obj::CObject *pOb, void *pVoidData)
{	
	Obj::CCompositeObject* pCompositeObject = (Obj::CCompositeObject*)pOb;
	Obj::CSuspendComponent* pSuspendComponent = GetSuspendComponentFromObject( pCompositeObject );
	if ( pSuspendComponent )
	{
		pSuspendComponent->CheckModelActive();
	}
}
 

void Skate::DoUpdate()
{

#ifdef	__MOVIES_FROM_SKATE_UPDATE__	
	if ( !Mdl::FrontEnd::Instance()->GamePaused() && !Mdl::CViewer::sGetViewMode() )
	{
		if ( mp_movieManager->IsRolling() )
		{
			if ( mp_movieManager->GetActiveCamera() )
			{
				// hardcoded to viewport 0...  if we need the other
				// viewports to play movies then we can store it
				// in the details...
				Nx::CViewportManager::sSetCamera( 0, mp_movieManager->GetActiveCamera() );
			}

			// hides the loading screen, if any
			if ( m_lastFrameWasMovieCamera )
			{
				Script::RunScript( CRCD(0xc5c9373a,"hide_loading_screen") );
			}
			mp_movieManager->Update();
			m_lastFrameWasMovieCamera = true;
		}
		else
		{
			// the camera just finished
			if ( m_lastFrameWasMovieCamera )
			{
				Script::RunScript( CRCD(0x15674315,"restore_skater_camera") );
				m_lastFrameWasMovieCamera = false;
			}
		}
		
		// play any extra object anims that might be playing
		// (this might be phased out later)
		if ( mp_objectAnimManager->IsRolling() )
		{
			mp_objectAnimManager->Update();
		}
	}
#endif
	
	
	
	// Fills in a little structure with some stuff so that SkipLogic is really fast.
	pre_calculate_object_update_info();
	
	// called once per frame
	Script::RunScript( CRCD(0x1aef674f,"Game_Update") );

	// Mick: PATCH																		 
	// if the game is paused, then manually update the visibility of the game object
	// as we might be moving the camera around (Like in the view-goals screen)
	if ( Mdl::FrontEnd::Instance()->GamePaused())
	{
		GetObjectManager()->ProcessAllObjects(do_CheckModelActive);
	}

// 

	#if 1
	
	// Temp hack, for each viewport, run proxim-update on it, hooking it up to the appropiate camera
	
	// Perhaps a better solution would be to have a non-suspendable component
	// that is attached to the skater, and grabs the viewport/camera itself
	// But what about when the game is paused?	
	if( GetLocalSkater())
	{
		int viewports = Nx::CViewportManager::sGetNumActiveViewports();
		switch (viewports)
		{
			case 1:
				if (Nx::CViewportManager::sGetActiveCamera(0))
				{
					Proxim_Update(1<<0, GetLocalSkater(), Nx::CViewportManager::sGetActiveCamera(0)->GetPos());
				}
				else
				{
					#ifdef	__NOPT_ASSERT__
					printf ("WARNING: 1 viewport, but no active camera 0\n");
					#endif
				}
				break;
			case 2:
				if (Nx::CViewportManager::sGetCamera(0))
				{
					Proxim_Update(1<<0, Obj::CCompositeObjectManager::Instance()->GetObjectByID(0), Nx::CViewportManager::sGetCamera(0)->GetPos());
				}
				// Need to check that the second skater exists, as well as the viewport
				if (Nx::CViewportManager::sGetCamera(1) && Obj::CCompositeObjectManager::Instance()->GetObjectByID(1))
				{
					Proxim_Update(1<<1, Obj::CCompositeObjectManager::Instance()->GetObjectByID(1), Nx::CViewportManager::sGetCamera(1)->GetPos());
				}
				break;
			default:
				Dbg_MsgAssert(0,("Don't handle %d viewport proxim updating",viewports));
		}
	}
	else if (CFuncs::ScriptIsObserving(NULL, NULL))		// TT12029: Run without object in observer mode
	{
		int viewports = Nx::CViewportManager::sGetNumActiveViewports();
		switch (viewports)
		{
			case 1:
				if (Nx::CViewportManager::sGetActiveCamera(0))
				{
					Obj::Proxim_Update(1<<0, NULL, Nx::CViewportManager::sGetActiveCamera(0)->GetPos());
				}
				else
				{
					#ifdef	__NOPT_ASSERT__
					printf ("WARNING: 1 viewport, but no active camera 0\n");
					#endif
				}
				break;
			default:
				Dbg_MsgAssert(0,("Don't handle %d viewport proxim updating in observer mode",viewports));
		}
	}
	else
	{
		#ifdef	__NOPT_ASSERT__
		// printf ("WARNING: No local skater, so no Proxim updates\n");
		#endif
	}
	#endif	




	
	if ( m_levelChanged )
	{
#ifdef __DEFERRED_CLEANUP_TEST__
		CFuncs::ScriptPrintMemInfo( NULL, NULL );
#endif
		m_levelChanged = false;
	}

	// don't think i need this anymore
	if (GetGameMode()->IsFrontEnd())
	{
		// if the front end is active, then don't update
		return;
	}

	// =================== TIMER STUFF ===========================
	// display the appropriate time if necessary
	// (we don't use or show a clock in certain game modes)
	Game::CGoalManager* pGoalManager = Game::GetGoalManager();
	if ( !Replay::RunningReplay() && pGoalManager->ShouldUseTimer() )
	{
		Tmr::Time time_left = (pGoalManager->GetGoalTime() + 999) / 1000;

		// int time_left = m_time_limit - Tmr::InSeconds( front_end->GetGameTime());

		if( time_left < 1 )
		{
			time_left = 0;
		}

		int seconds = time_left % 60;
		int minutes = time_left / 60;
		
		Front::SetTimeTHPS4(minutes, seconds);
		
		//m_panelMgr->SetTime(minutes, seconds);
		
/*		if (GetGameMode()->ShouldTimerBeep())
		{
			//float time = (float)m_time_limit - front_end->GetGameTime()/1000.0f;
			float time = float(time_left);
			BeepTimer(time,10.0f, 10.0f,"timer_runout_beep");			
		}
*/
	}
	else
	{
		Front::HideTimeTHPS4();
	}

	// make sure everyone's controller is still plugged in
#ifdef __PLAT_XBOX__
	int is_changing_levels = Script::GetInteger( CRCD(0xe997db9f,"is_changing_levels"), Script::ASSERT );
	int check_controllers = Script::GetInteger( CRCD(0x77a5662c,"check_for_unplugged_controllers"), Script::ASSERT );
	if ( check_controllers && !is_changing_levels )
	{
		if ( m_first_input_received )
		{
			Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();
			Front::CScreenElement* p_root_window = p_screen_elem_man->GetElement( CRCD(0x56a1eae3,"root_window")  , Front::CScreenElementManager::DONT_ASSERT );
			if ( p_root_window )
			{
				// check that the box isn't already up
				Front::CScreenElement* p_error_box = p_screen_elem_man->GetElement( CRCD(0x2edb780f,"controller_unplugged_dialog_anchor") , Front::CScreenElementManager::DONT_ASSERT );
				if ( !p_error_box )
				{
					bool controller_unplugged = false;
					for ( int i = 0; i < vMAX_SKATERS; i++ )
					{
						Obj::CSkater* pSkater = GetSkater( i );
						if ( pSkater && pSkater->IsLocalClient() )
						{
							Inp::Handler< Obj::CInputComponent >* pHandler = pSkater->mp_input_component->GetInputHandler();
							if ( !pHandler->m_Device->IsPluggedIn() )
							{
								// m_controller_unplugged_frame_count++;
								controller_unplugged = true;
								if ( m_controller_unplugged_frame_count > Script::GetInteger( CRCD(0xf7fabb50,"controller_unplugged_frame_count"), Script::ASSERT ) )
								{
									m_controller_unplugged_frame_count = 0;
									Script::CStruct* pScriptParams = new Script::CStruct();
									GameNet::Manager* gamenet_man = GameNet::Manager::Instance();
									if ( gamenet_man->InNetGame() )
									{
										Game::CGoalManager* pGoalManager = Game::GetGoalManager();
										Dbg_Assert( pGoalManager );
										pGoalManager->DeactivateAllGoals();
										pGoalManager->UninitializeAllGoals();

										pScriptParams->AddChecksum( NONAME, CRCD(0xeaa48e14,"leaving_net_game") );
										Skate::LeaveServer();
										if ( gamenet_man->OnClient() )
										{
											gamenet_man->CleanupPlayers();
											gamenet_man->ClientShutdown();
										}
									}
	
									// get the device that's unplugged
									int device_num = pHandler->m_Device->GetPort();
									
									pScriptParams->AddInteger( CRCD(0xc9428a08,"device_num"), device_num );
									Script::RunScript( CRCD(0xf77aca62,"controller_unplugged"), pScriptParams );
									delete pScriptParams;
	
									break;
								}
							}
						}
					}
					if ( controller_unplugged )
						m_controller_unplugged_frame_count++;
					else
						m_controller_unplugged_frame_count = 0;
				}
			}
		}
	}
#endif		// __PLAT_NGC__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::SetTimeLimit( int seconds )
{
	m_time_limit = seconds;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetTimeLimit | sets the time limit
// @parmopt int | seconds | 120 | number of seconds for time limit
bool ScriptSetTimeLimit(Script::CStruct *pParams, Script::CScript *pScript) {
    int seconds = 120;

    pParams->GetInteger( "seconds", &seconds );

    Skate* skate = Skate::Instance();

    skate->SetTimeLimit( seconds );
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Skate::GetTimeLimit( void )
{
	return m_time_limit;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::UpdateGameFlow()
{
	Dbg_Assert( mp_movieManager );

#ifndef	__MOVIES_FROM_SKATE_UPDATE__	
	if ( !Mdl::FrontEnd::Instance()->GamePaused() && !Mdl::CViewer::sGetViewMode() )
	{
		if ( mp_movieManager->IsRolling() )
		{
			if ( mp_movieManager->GetActiveCamera() )
			{
				// hardcoded to viewport 0...  if we need the other
				// viewports to play movies then we can store it
				// in the details...
				Nx::CViewportManager::sSetCamera( 0, mp_movieManager->GetActiveCamera() );
			}

			// hides the loading screen, if any
			if ( m_lastFrameWasMovieCamera )
			{
				Script::RunScript( CRCD(0xc5c9373a,"hide_loading_screen") );
			}
			mp_movieManager->Update();
			m_lastFrameWasMovieCamera = true;
		}
		else
		{
			// the camera just finished
			if ( m_lastFrameWasMovieCamera )
			{
				Script::RunScript( CRCD(0x15674315,"restore_skater_camera") );
				m_lastFrameWasMovieCamera = false;
			}
		}
		
		// play any extra object anims that might be playing
		// (this might be phased out later)
		if ( mp_objectAnimManager->IsRolling() )
		{
			mp_objectAnimManager->Update();
		}
	}
#endif

	Dbg_AssertPtr( mp_gameFlow );
	mp_gameFlow->Update();
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Skate::Skate ( void )
{   
	m_first_input_received = false;
	m_controller_unplugged_frame_count = 0;
	
	m_logic_task = new Tsk::Task< Skate > ( Skate::s_logic_code, *this );

	net_setup();

	mp_PlayerProfileManager = new Obj::CPlayerProfileManager;
	mp_PlayerProfileManager->Reset();

	mp_gameFlow = new CGameFlow;
	mp_goalManager = new Game::CGoalManager;

//    mp_obj_manager = new Obj::CGeneralManager;
	// Bit of a patch, but the engine now needs to be told what is managing the moving objects		
	// When Skate.cpp is broken up, then the mp_obj_manager needs extracting into some more generic "game object" manager
	// which Nx can query, if it really needs to													
	Nx::CEngine::sSetMovableObjectManager(GetObjectManager());

    mp_gameMode = new Game::CGameMode;
	mp_career = new Obj::CSkaterCareer;				// new local careeer
	
	mp_railManager = new Obj::CRailManager;

#	ifdef TESTING_GUNSLINGER
	mp_navManager = new Obj::CNavManager;
#	endif
	
	mp_trickChecksumTable = new Obj::CTrickChecksumTable;

	mp_trickObjectManager = new Obj::CTrickObjectManager;
	mp_trickObjectManager->DeleteAllTrickObjects();

	m_recording = false;
	m_playing = false;
	m_fd = 0;

	m_gameInProgress = false;
	m_levelLoaded = false;
	SetGameType( Script::GenerateCRC("freeskate") );

	// initialize the splitscreen preferences
    mp_splitscreen_preferences = new Prefs::Preferences;
	mp_splitscreen_preferences->Load( Script::GenerateCRC("default_splitscreen_preferences" ) );

	mp_competition = new CCompetition;
	mp_horse = new CHorse;
	mp_movieManager = new Obj::CMovieManager;
	mp_objectAnimManager = new Obj::CMovieManager;

	mp_gameRecords = new Records::CGameRecords(Obj::CSkaterCareer::vMAX_CAREER_LEVELS);

	m_shadow_mode = Gfx::vSIMPLE_SHADOW;
	
	// Initialise the controller preferences.
	// These might get changed by an autoload off the memory card.
	for (int i=0; i<vMAX_SKATERS; ++i)
	{
		m_device_server_map[i] = -1;
		
		mp_controller_preferences[i].skaterIndex = i;
		mp_controller_preferences[i].VibrationOn=true;
		SetAutoKick(i,true);
		SetSpinTaps(i,false);
	}	
	// so that autolaunch works
	m_device_server_map[0] = 0;



}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Skate::~Skate ( void )
{   
	delete mp_gameRecords;
	delete mp_competition;
	delete mp_horse;
	delete mp_movieManager;
	delete mp_objectAnimManager;
	delete mp_railManager;
	delete mp_career;
//    delete mp_obj_manager;

	net_shutdown();

	Dbg_Assert( mp_gameFlow );
	delete mp_gameFlow;

	Dbg_Assert( mp_gameMode );
    delete mp_gameMode;

	Dbg_Assert( mp_goalManager );
	delete mp_goalManager;
	
	Dbg_Assert( mp_PlayerProfileManager );
	delete mp_PlayerProfileManager;

    delete m_logic_task;
    delete mp_splitscreen_preferences;

    delete mp_trickChecksumTable;
    delete mp_trickObjectManager;
} 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::SetSpinTaps(int index, bool state)
{
	Dbg_MsgAssert(index>=0 && index<vMAX_SKATERS,("Bad index"));
	mp_controller_preferences[index].SpinTapsOn=state;
	Obj::CSkater *p_skater = GetSkater(index);
	if (p_skater)
	{
		GetSkaterCorePhysicsComponentFromObject(p_skater)->m_spin_taps=state;
	}
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::SetAutoKick(int index, bool state)
{
	Dbg_MsgAssert(index>=0 && index<vMAX_SKATERS,("Bad index"));
	mp_controller_preferences[index].AutoKickOn=state;
	Obj::CSkater *p_skater = GetSkater(index);
	if (p_skater)
	{
		GetSkaterCorePhysicsComponentFromObject(p_skater)->m_auto_kick=state;
	}
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::SetVibration(int index, bool state)
{
	Dbg_MsgAssert(index>=0 && index<vMAX_SKATERS,("Bad index"));
	mp_controller_preferences[index].VibrationOn=state;
	Obj::CSkater *p_skater = GetSkater(index);
	if (p_skater)
	{
		Obj::CVibrationComponent* p_vibration_component = GetVibrationComponentFromObject(p_skater);
		if (p_vibration_component)
		{
			p_vibration_component->SetActiveState(state);
		}
	}
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CSkater* 	Skate::GetLocalSkater( void )
{
	unsigned int i;

	for( i = 0; i < GetNumSkaters(); i++ )
	{
		if( m_skaters.GetItem(i) != NULL )
		{
			Obj::CSkater *pSkater = m_skaters.GetItem(i)->GetData();
			if( pSkater->IsLocalClient())
			{
				return pSkater;
			}
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CSkater 	*Skate::GetSkater( uint num )
{
	
	if( GetNumSkaters() <= num )
	{
		return NULL;
	}   
	else 
	{
		Dbg_Assert( m_skaters.GetItem( num ) );
        return m_skaters.GetItem( num )->GetData();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CSkater *Skate::GetSkaterById( uint32 id )
{
	for (uint i = 0; i < GetNumSkaters(); i++)
	{
		if( m_skaters.GetItem(i) != NULL )
		{
			Obj::CSkater *pSkater = m_skaters.GetItem(i)->GetData();
			if (pSkater->GetID() == id)
				return pSkater;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CGeneralManager*	Skate::GetObjectManager( void )
{
//	return mp_obj_manager;
	return	Obj::CCompositeObjectManager::Instance();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CTrickObjectManager*	Skate::GetTrickObjectManager( void )
{
	return mp_trickObjectManager;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CPlayerProfileManager*	Skate::GetPlayerProfileManager( void )
{
	return mp_PlayerProfileManager;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Prefs::Preferences* Skate::GetSplitScreenPreferences( void )
{
	return mp_splitscreen_preferences;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float Skate::GetHandicap( int id )
{
	Prefs::Preferences* pPreferences = GetSplitScreenPreferences();

	Script::CStruct* pStructure = NULL;
	
	switch ( id )
	{
		case 0:
			pStructure = pPreferences->GetPreference( Script::GenerateCRC("player1_handicap") );
			break;
		case 1:
			pStructure = pPreferences->GetPreference( Script::GenerateCRC("player2_handicap") );
			break;
		default:
			Dbg_MsgAssert( 0, ( "Out of range handicap id %d", id ) );
			break;
	}

	Dbg_Assert( pStructure );

	int val = 0;
	pStructure->GetInteger( "time", &val );	// stored in the time field...  ugly!
	return val;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// The Skate::Cleanup function is essentially cleaning up
// the "Session" objects.
// It does not unload any assets, just deletes objects that were valid for the session 

void	Skate::Cleanup()
{
	uint32 i;

	printf("Skate::Cleanup() - Deleting all session specific objects\n");

// Resetting the skater is complicated
// in that we have to lock him, to make sure he is not deleted
// and then reset him manually, as we can't just delete and re-create him (yet)	
	for (i=0;i<GetNumSkaters();i++)
	{
		Obj::CSkater* skater;
		skater = GetSkater(i);
		if (skater)
		{
			skater->SetLockOn();					// Sets the "lock" flag, so he won't get deleted by DestroyAllObjects
			if( skater->IsLocalClient())
			{
				skater->ResetPhysics(false, true);
				// stop any movies associated with each skater
				this->GetMovieManager()->ClearMovieQueue();
			}
			skater->DeleteResources();	   // blood, gap checkslistm shodow
			//skater->SwitchOffShadow();
			skater->RemoveFromCurrentWorld();
		}
	}

// Clear the movies used for moving object anims	
	GetMovieManager()->ClearMovieQueue();
	GetObjectAnimManager()->ClearMovieQueue();

	// clear out the viewer object, if any
	Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();
	if ( pViewer )
	{
		pViewer->RemoveViewerObject();   	
	}

// Destroy all level objects.  Basically anything created with the node array
	GetObjectManager()->DestroyAllObjects();
	GetObjectManager()->FlushDeadObjects();

// For now we destory all the composite objects here as well
//	Obj::CCompositeObjectManager::Instance()->UnlockAllObjects();
//	Obj::CCompositeObjectManager::Instance()->DestroyAllObjects();
//	Obj::CCompositeObjectManager::Instance()->FlushDeadObjects();


	// Now remove the lock on the skaters' objects
	for (i=0;i<GetNumSkaters();i++)
	{
		Obj::CSkater* skater;
		skater = GetSkater(i);
		if (skater)
		{
			skater->SetLockOff();					// Clears the "lock" flag
			skater->AddToCurrentWorld();
		}
	}

	// The gamenet managers objects (crown, compass... )
	// have now been destroyed so Nullify references to them
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	gamenet_man->CleanupObjects();

	// Little sanity check, to make sure everyhting except the skater is gone	
//	sint pTypes[]={SKATE_TYPE_SKATER,-1};
//	GetObjectManager()->AssertIfObjectsRemainApartFrom(pTypes);

	// unlock the skaters, as they will be the only remaining objects	
	//GetObjectManager()->UnlockAllObjects();

	// Delete any leftover spawned scripts.
	// (NB, might not want to do this, if the gameflow has spawned scripts?
	// but we probably do, as most of it will be session specific scripts	
	Script::DeleteSpawnedScripts();

	// Delete all the trick objects that were created for this session
	mp_trickObjectManager->DeleteAllTrickObjects();
	
	// destroy the list of gapchecks (checklist of all gaps in the level)
	// (maybe we would keep this from session to session?)
//	m_gapcheck_list.DestroyAllNodes();
		
	// Destroy the new style particle systems....
    // or not, as they are now all destroyed by the controlling objects, or the object manager
    // so, if we did destroy them here, then the blood splat would stop working.
	Nx::destroy_all_temp_particles( );

	// Destroy rails, they get re-created from the node array
	GetRailManager()->Cleanup();
	
	// Destroy Proximity nodes, similar to rails	
	Obj::Proxim_Cleanup();

	// Destroy EmitterObjects   
	Obj::CEmitterManager::sCleanup();

	// cleanup particles
	Dbg_Printf( "Destroying particle systems..." );
	Nx::CEngine::sGetParticleManager()->Cleanup();

	// This is a debugging function to see what's
	// currently in the anim cache (at this point,
	// only a handful of skater anims should be
	// in the cache)
//	Nx::PrintAnimCache();
}


// Cleaup stuff that might have been created in the park editor test play
void Skate::CleanupForParkEditor()
{
	mp_trickObjectManager->DeleteAllTrickObjects();
	Nx::KillAllTextureSplats();
	Nx::destroy_all_temp_particles( );
	GetRailManager()->Cleanup();
	Obj::Proxim_Cleanup();
	Obj::CEmitterManager::sCleanup();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::Pause( bool pause )
{
	
	if ( pause )
	{
		 Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
		sfx_manager->PauseSounds( );
//		Pcm::PauseMusic( 1 );
//		Pcm::PauseStream( 1 );
	}
	else
	{
		// unpause music if game is active:
		if ( IsGameInProgress( ) )
		{
//			Pcm::PauseMusic( 0 );
//			Pcm::PauseStream( 0 );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::RequestGameEnd()
{
	m_gameInProgress = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Game::CGameMode *Skate::GetGameMode(void)
{
	return mp_gameMode;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Game::CGoalManager* Skate::GetGoalManager(void)
{
	return mp_goalManager;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CSkaterProfile* Skate::GetCurrentProfile( Script::CStruct *pParams )
{
	Dbg_MsgAssert( mp_PlayerProfileManager,( "No skater profile manager\n" ));

	int skaterNum;
	if ( pParams && pParams->GetInteger( "skater", &skaterNum ) )
	{
		return mp_PlayerProfileManager->GetProfile( skaterNum );
	}
	else
	{
		return mp_PlayerProfileManager->GetCurrentProfile();
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CSkaterProfile* Skate::GetProfile( int skaterNum )
{
	Dbg_MsgAssert( mp_PlayerProfileManager,( "No skater profile manager\n" ));

	return mp_PlayerProfileManager->GetProfile( skaterNum );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::SetGameType(uint32 gameType)
{
// Don't set this right away...	
//	GetGameMode()->LoadGameType( gameType );
	m_requested_game_type = gameType;
				 
// Instead, remember it, and
// commit the change when we do the "setgamestate on"
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::SetCurrentGameType()
{
    printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<Changing Game Mode\n");
	GetGameMode()->LoadGameType( m_requested_game_type );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::OpenLevel(uint32 level_script)
{
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
//	CloseLevel();

	Mem::PushMemProfile("LaunchLevel");


	if( gamenet_man->InNetGame())
	{
		if( gamenet_man->OnServer())
		{
			Lst::Search< GameNet::PlayerInfo > sh;
			GameNet::PlayerInfo* player;

			for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
			{
				player->MarkAsNotReady( 0 );
			}
		}
	}


	if (level_script == Script::GenerateCRC("custom_park"))
	{
        ;
	}
	else
	{
#ifdef	__NOPT_ASSERT__
		// Make sure it exists, otherwise we crash obscurely...
		Script::CSymbolTableEntry *p_entry=Script::Resolve(level_script);
		Dbg_MsgAssert(p_entry,("Level script %x (%s) not found",level_script,Script::FindChecksumName(level_script)));
#endif
		
		Script::RunScript(level_script);
	}

	m_levelLoaded = true;
	m_cur_level = level_script;

	if( gamenet_man->InNetGame())
	{
		int i;
		for( i = 0; i < GameNet::vMAX_LOCAL_CLIENTS; i++ )
		{
			Lst::Search< Net::Conn > sh;
			Net::Conn* server_conn;
			Net::Client* client;
	
			client = gamenet_man->GetClient( i );
			if( client )
			{
				server_conn = client->FirstConnection( &sh );
				if( server_conn )
				{
					server_conn->UpdateCommTime( Tmr::Seconds( 12 ));	// update the current comm time so it doesn't time out after
																		// loading the skater
				}
			}
		}

		if( gamenet_man->OnServer())
		{
			Lst::Search< Net::Conn > sh;
			Net::Conn* conn;
			Net::Server* server;

			server = gamenet_man->GetServer();
			if( server )
			{
				for( conn = server->FirstConnection( &sh ); conn; 
						conn = server->NextConnection( &sh ))
				{
					conn->UpdateCommTime( Tmr::Seconds( 8 ));	// update the current comm time so it doesn't time out after
																// loading the skater
				}
			}
		}
		else
		{
			// If we have some queued up new players, we'll respond to the ready query
			// after we finish loading them. Otherwise, if this is just a standard change level
			// call, tell the server we're finished loading it and ready to communicate
			Lst::Search< GameNet::NewPlayerInfo > sh;

			if( gamenet_man->FirstNewPlayerInfo( sh ) == NULL )
			{
				gamenet_man->RespondToReadyQuery();
			}
		}
	}

	Mem::PopMemProfile(/*"LaunchLevel"*/);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// RestartLevel is just intended to be called when a level is restarted
// it does not actually restart the level, it just sets a few flags
// and does something with disabling the the viewer log that's only related to screenshot mode 

void Skate::RestartLevel()
{
	// clear the end run flag
	ClearEndRun();
	
	// GJ:  in a network lobby, the skater is loaded AFTER the
	// level so this return wouldn't work...  come to think
	// of it, what's it needed for?
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	if ( !GetNumSkaters() && !gamenet_man->InNetGame() )
	{
		Ryan("BAD BAD BAD\n");
		return;
	}

	if (!m_levelLoaded)
	{
		Ryan("BAD BAD BAD\n");
		return;
	}

	// the game is now on
	if ( GetGameMode()->IsFrontEnd() )
	{
		// don't let the career timer go until
		// we're actually in a real level,
		// otherwise, we'll get "Requested MenuScreen
		// not active" asserts
		m_gameInProgress = false;

	}
	else
	{
		m_gameInProgress = true;

//		 Mdl::RwViewer * rwviewer_mod =  Mdl::RwViewer::Instance();
//		rwviewer_mod->DisableMainLogic( false );		// reset
	}

	if ( gamenet_man->InNetGame() )
	{
		#if 0
		// unpause the game if we're in the network lobby
		Mdl::FrontEnd* frontend = Mdl::FrontEnd::Instance();
		frontend->SetActive(false);
		#endif
	}

	// Mick:  tell the career mode that we are just starting this level
	// so it will remember what the flags were at the start of the level	
	GetCareer()->StartLevel();
	
	// print debug info
	GetTrickObjectManager()->PrintContents();	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::ChangeLevel( uint32 level_id )
{
	 //GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	// changes level while preserving the skaters

// Mick - moved this in ScriptCleanup
//    Game::CGoalManager* p_GoalManager = Game::GetGoalManager();
//    Dbg_MsgAssert( p_GoalManager, ( "couldn't get GoalManager\n" ) );
//    p_GoalManager->LevelUnload();
//    p_GoalManager->DeactivateAllGoals();
//    p_GoalManager->RemoveAllGoals();

	printf ("-----------------------------------------Skate::ChangeLevel(%d)\n",level_id);

	//uint32 i;
	Script::CStruct* pTempStructure;

	// TODO:  is this necessary?   since we push individual heaps below
	// maybe, if there is anyhting else that might allocate stuff 
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());

	// requests the new level...
	pTempStructure = new Script::CStruct;
	pTempStructure->Clear();
	pTempStructure->AddComponent( Script::GenerateCRC("level"), ESYMBOLTYPE_NAME, (int)level_id );
	Script::RunScript( "request_level", pTempStructure );
	delete pTempStructure;

	m_prev_level = m_cur_level;
	m_cur_level=level_id;
	
	// removes the skaters from the existing world
	// (skate::cleanup func used to take care of this
	// but now it's in ScriptCleanup
	/*if( !gamenet_man->InNetGame())
	{
		for ( i = 0; i < GetNumSkaters(); i++ )
		{
			Obj::CSkater* pSkater = GetSkater( i );
			pSkater->RemoveFromCurrentWorld();
		}
	}*/

	
	// cleans up everything but the skaters
	pTempStructure = new Script::CStruct;
	pTempStructure->Clear();
	// If we're going back to the skateshop, clean up any extra skater heaps we may have allocated
	if( level_id != Script::GenerateCRC( "load_skateshop" ))
	{
		pTempStructure->AddComponent( NONAME, ESYMBOLTYPE_NAME, (int)Script::GenerateCRC("preserve_skaters") );
	}
	CFuncs::ScriptCleanup( pTempStructure, NULL );
	delete pTempStructure;
	
	m_levelChanged = true;
	
	//if( gamenet_man->InNetGame())
	//{
		//Script::RunScript( "create_score_menu" );
	//}

	// restarts the gameflow
	Dbg_AssertPtr( mp_gameFlow );
	mp_gameFlow->Reset( Script::GenerateCRC( "ChangeLevelGameFlow" ) );
	
	Mem::Manager::sHandle().PopContext();
}


void Skate::ResetLevel()
{
	GameNet::Manager* gamenet_man = GameNet::Manager::Instance();

	// Cleanup existing level specific stuff
	Cleanup();

	// Re-trigger all the objects in the level										  
	CFuncs::ScriptParseNodeArray(NULL,NULL);
	
	// clear the end run flag
	ClearEndRun();

	// now that the trick object database is set,
	// we can apply any graffiti state queued up in the trick
	// object manager
	GetTrickObjectManager()->ApplyObserverGraffitiState();

	Game::CGoalManager* pGoalManager = Game::GetGoalManager();
	if( gamenet_man->InNetGame())
	{                                                         
		pGoalManager->InitializeAllMinigames();
		pGoalManager->UnBeatAllGoals();
	}
	else
	{
		pGoalManager->InitializeAllGoals();
	}

	pGoalManager->CreateGoalLevelObjects();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


void Skate::ResetSkaters(int node, bool walk)
{
    // Skip skaters to restart points (will also reset their physics states)
    for (uint32 i=0;i<GetNumSkaters();i++)
    {
        Obj::CSkater* pSkater = GetSkater( i );
        Dbg_MsgAssert ( pSkater,( "No skater" ));
        if ( pSkater->IsLocalClient() )
        {
             skip_to_restart_point( pSkater, node, walk );			
        }

        // We Resync() the skater here, as he has just restarted
        // he also moves on the client
        // however, the client does not know he has to restart yet
        // so will still be sending old positions
        // we need to tell the clients all the restart
        // and then not take any positions from them until they acknowledge 
        // that they have carried out this restart
        // This would involved calling Resync each frame until they acknowledge having restarted
        // or we could just resync every frame for a period of time
        // however, that seems rather dodgy, as you have no way of knowing
        // how long you should resync for 
		pSkater->Resync();

        // So... how do we do that then Steve?

    }
    ClearEndRun();
}		

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::LaunchGame()
{
	 Mdl::FrontEnd * front =  Mdl::FrontEnd::Instance();
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	front->PauseGame(false);

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());

	printf ("\nLAUNCH GAME CALLED with %d skaters \n\n",GetNumSkaters());			   
			   
	//if (!GetNumSkaters())
	if( !gamenet_man->GetLocalPlayer())
	{
		Dbg_Printf( "*** NO LOCAL PLAYER\n" );
		// If we are starting a game with possibly more than two players
		// then we want to allocate 	
		Dbg_Printf( "*** Max players: %d\n", GetGameMode()->GetMaximumNumberOfPlayers());
		if (GetGameMode()->GetMaximumNumberOfPlayers() > NUM_PERM_SKATER_HEAPS)
		{
			Mem::Manager::sHandle().InitSkaterHeaps(GetGameMode()->GetMaximumNumberOfPlayers());
		}
	
		// start the game flow
		Dbg_AssertPtr( mp_gameFlow );
		mp_gameFlow->Reset( Script::GenerateCRC( "InitializeGameFlow" ) );
		mp_gameFlow->Update();
		printf( "\nLeaving Update\n" );
	}
	else
	{
		// Reset everything in the level to its initial state
		ResetLevel();

		// GJ:  we are intentionally not rerunning the intro script here
		m_gameInProgress = true;

		// start the game flow
		Dbg_AssertPtr( mp_gameFlow );
		mp_gameFlow->Reset( Script::GenerateCRC( "StandardGameFlow" ) );
		printf("Restarting game flow\n");
	}

	// Clear the king of the hill
	GameNet::PlayerInfo* player;
	Lst::Search< GameNet::PlayerInfo > sh;
	if(( player = gamenet_man->GetKingOfTheHill()))
	{
		player->MarkAsKing( false );
	}
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		player->ClearCTFState();
	}

	if( GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netlobby" ))
	{
		gamenet_man->CreateTeamFlags( GetGameMode()->NumTeams());
	}

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::CheckSkaterCollisions( void )
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	Obj::CSkater* subject;
	int i;
	
	if( gamenet_man->GameIsOver())
	{
		return;
	}

	// Only perform the collision logic if player collision is enabled
	if( ( gamenet_man->PlayerCollisionEnabled() == false ) &&
		( GetGameMode()->GetNameChecksum() != CRCD(0xbff33600,"netfirefight")) &&
		( GetGameMode()->GetNameChecksum() != CRCD(0x3d6d444f,"firefight")) &&
		( GetGameMode()->GetNameChecksum() != CRCD( 0x6ef8fda0, "netking" )) &&
		( GetGameMode()->GetNameChecksum() != CRCD( 0x5d32129c, "king" )) &&
		( GetGameMode()->GetNameChecksum() != CRCD( 0x6c5ff266, "netctf" )))
	{
		return;
	}

	// Loop through all other skaters and check for collisions
	for( i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
	{
		subject = GetSkater( i );
		if(	subject == NULL )
		{
			continue;
		}
		
		subject->mp_skater_state_history_component->CollideWithOtherSkaters( 0 );
	}

	// If we're in king of the hill mode and the crown is on the ground
	// perform collision checks to see if someone snags it
	if(	( GetGameMode()->GetNameChecksum() == CRCD( 0x6ef8fda0, "netking" )) ||
		( GetGameMode()->GetNameChecksum() == CRCD( 0x5d32129c, "king" )))
	{   
		Obj::CCrown* crown;

		crown = gamenet_man->GetCrown();
		if( crown && !crown->OnKing())
		{
			for( i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
			{
				subject = GetSkater( i );
				if(	subject == NULL )
				{
					continue;
				}
				
				if( subject->mp_skater_state_history_component->CheckForCrownCollision())
				{
					GameNet::PlayerInfo* player;
					Lst::Search< GameNet::PlayerInfo > sh;
		
					player = gamenet_man->GetPlayerByObjectID( subject->GetID() );
					Dbg_Assert( player );

					// It is important that we mark the king immediately (rather than through a
					// network message) since we do logic based on the "current" king
					player->MarkAsKing( true );
					for( player = gamenet_man->FirstPlayerInfo( sh, true ); player;
							player = gamenet_man->NextPlayerInfo( sh, true ))
					{
						// Already marked the king for the local player (above)
						if( player->IsLocalPlayer())
						{
							continue;
						}
						
						GameNet::MsgByteInfo msg;
						Net::MsgDesc msg_desc;
						Net::Server* server;

						server = gamenet_man->GetServer();
						Dbg_Assert( server );
						
						msg.m_Data = subject->GetID();

						msg_desc.m_Data = &msg;
						msg_desc.m_Length = sizeof( GameNet::MsgByteInfo );
						msg_desc.m_Id = GameNet::MSG_ID_NEW_KING;
						msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
						msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
						server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
					}
					break;
				}
			}	   
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::ObserveNextSkater( void )
{
	GameNet::PlayerInfo* player;
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

    player = gamenet_man->GetNextPlayerToObserve();
	gamenet_man->ObservePlayer( player );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char* Skate::GetSkaterDisplayName( int id )
{	
	for ( uint32 j = 0; j < GetNumSkaters(); j++ )
	{
		if ( (int) GetSkater(j)->GetID() == id )
		{
			return GetSkater(j)->GetDisplayName();
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Skate::ShouldBeAbsentNode( Script::CStruct* pNode )
{
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	Dbg_Assert( pNode );
	
	return ( pNode->ContainsFlag( 0x68910ac6 /*"AbsentInNetGames"*/ ) && 
			 ( IsMultiplayerGame() || gamenet_man->InNetMode()));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Skate::IsMultiplayerGame( void )
{
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	return ( gamenet_man->InNetGame() || GetGameMode()->GetInitialNumberOfPlayers() > 1 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Skate::SetShadowMode( Gfx::EShadowType shadowMode )
{
	Obj::CSkater* skater;
	Lst::Search< Obj::CSkater > sh;

	m_shadow_mode = shadowMode;

	for( skater = sh.FirstItem( m_skaters ); skater; skater = sh.NextItem())
	{
		Obj::CShadowComponent* p_shadow_component = GetShadowComponentFromObject(skater);
		Dbg_Assert(p_shadow_component);
		p_shadow_component->SwitchOnSkaterShadow();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Gfx::EShadowType Skate::GetShadowMode( void ) const
{
	return m_shadow_mode;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetGameType | Sets the type of game to be played. Examples: <nl>
// SetGameType FreeSkate <nl>
// SetGameType SingleSession
// @uparm name | Game type
bool ScriptSetGameType(Script::CStruct *pParams, Script::CScript *pScript)
{
	Skate* skate = Skate::Instance();
	uint32 game_type;
	pParams->GetChecksum(NONAME, &game_type, true);
	skate->SetGameType(game_type);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | TestGameType | Tests if current game type is equal to the 
// game type specified as the first parameter
// @uparm name | Game type
bool ScriptTestGameType(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 game_type;
	pParams->GetChecksum(NONAME, &game_type, true);
	
	Skate* skate = Skate::Instance();
	return (game_type == skate->GetGameMode()->GetNameChecksum());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptInTeamGame(Script::CStruct *pParams, Script::CScript *pScript)
{
	Skate* skate = Skate::Instance();

	return skate->GetGameMode()->IsTeamGame();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | TestRequestedGameType |
// This is so that I can tell what the last value sent to SetGameType
// was, since SetGameType will not set the game type straight away,
// so TestGameType will not work.
// @uparm name | Game type
bool ScriptTestRequestedGameType(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 game_type;
	pParams->GetChecksum(NONAME, &game_type, true);
	
	Skate* skate = Skate::Instance();
	return (game_type == skate->GetGameType());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ChangeLevel | 
// @parm string | level | The level to change to
bool ScriptChangeLevel(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	bool show_warning;
	uint32 level;

	pParams->GetChecksum(Script::GenerateCRC("level"), &level, true);
	if( level == Script::GenerateCRC("use_preferences") )
	{
		level = gamenet_man->GetLevelFromPreferences();
	}
		
	gamenet_man->SetLevel( level );
	show_warning = pParams->ContainsComponentNamed( "show_warning" );

#ifdef __NOPT_ASSERT__
	printf("Send message to ChangeLevel level=%s here!\n", Script::FindChecksumName(level) );
#endif

	Pcm::StopMusic();
	
	Net::Server* server;
	GameNet::MsgChangeLevel msg;
	GameNet::PlayerInfo* player;
	Lst::Search< GameNet::PlayerInfo > sh;

	server = gamenet_man->GetServer();
	Dbg_Assert( server );

	msg.m_Level = level;
	msg.m_ShowWarning = (char) show_warning;
	
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; 
			player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		GameNet::MsgReady ready_msg;
		Net::MsgDesc msg_desc;
		
		ready_msg.m_Time = Tmr::GetTime();

		if( player->IsLocalPlayer() == false )
		{
			if( level == CRCD(0xb664035d,"Load_Sk5Ed_gameplay"))
			{
				server->StreamMessage( player->GetConnHandle(), GameNet::MSG_ID_LEVEL_DATA, Ed::CParkManager::COMPRESSED_MAP_SIZE, 
									   Ed::CParkManager::sInstance()->GetCompressedMapBuffer(), "level data", 
									   GameNet::vSEQ_GROUP_PLAYER_MSGS, false, true );
						   
				server->StreamMessage( player->GetConnHandle(), GameNet::MSG_ID_RAIL_DATA, Obj::GetRailEditor()->GetCompressedRailsBufferSize(), 
									   Obj::GetRailEditor()->GetCompressedRailsBuffer(), "rail data", 
									   GameNet::vSEQ_GROUP_PLAYER_MSGS, false, true );
						   
			}
		}

		msg_desc.m_Data = &msg;
		msg_desc.m_Length = sizeof(GameNet::MsgChangeLevel);
		msg_desc.m_Id = GameNet::MSG_ID_CHANGE_LEVEL;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
		server->EnqueueMessage( player->GetConnHandle(), &msg_desc );

		if( player->IsLocalPlayer() == false )
		{
			if( gamenet_man->UsingCreatedGoals())
			{
				gamenet_man->LoadGoals( level );
				server->StreamMessage( player->GetConnHandle(), GameNet::MSG_ID_GOALS_DATA, gamenet_man->GetGoalsDataSize(),
							   gamenet_man->GetGoalsData(), "goals data", GameNet::vSEQ_GROUP_PLAYER_MSGS, false, true );
			}
		}

		// Don't send them any non-important messages until they're finished loading
		msg_desc.m_Data = &ready_msg;
		msg_desc.m_Length = sizeof( GameNet::MsgReady );
		msg_desc.m_Id = GameNet::MSG_ID_READY_QUERY;
		server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
				
		player->SetReadyQueryTime( ready_msg.m_Time );
	}

	// In net games, the changing of levels is deferred. No need to send immediately - we can
	// wait until the enqueued message is naturally added to the outgoing stream.  In 2p mode,
	// however, the scripts expect this message to be sent and handled in a one-frame window
	// so we need to send it immediately
	if ( ! gamenet_man->InNetGame())
	{
		server->SendData();		// Mick, (true) because we want to send it immediatly
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LaunchLevel | launches currently requested level
bool ScriptLaunchLevel(Script::CStruct *pParams, Script::CScript *pScript)
{
	Skate* skate_mod = Skate::Instance();
	uint32 level = skate_mod->m_requested_level;
	skate_mod->OpenLevel(level);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RequestLevel | 
// @uparm name | level to request
bool ScriptRequestLevel(Script::CStruct *pParams, Script::CScript *pScript)
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	Skate* skate_mod = Skate::Instance();
	uint32 level;

	
//	printf ("\n%s\n",pScript->GetScriptInfo());
	
	
	if( !pParams->GetChecksum(NONAME, &level, false))
	{
		const char* level_name;
		char checksum_name[128];

		if( pParams->GetString( NONAME, &level_name, Script::ASSERT ))
		{
			sprintf( checksum_name, "load_%s", level_name );
			Dbg_Printf( "Got checksum name of %s\n", checksum_name );
			level = Script::GenerateCRC( checksum_name );
		}
	}

	if( level == Script::GenerateCRC("use_preferences") )
	{
		level = gamenet_man->GetLevelFromPreferences();
		gamenet_man->SetLevel( level );
	}

	skate_mod->m_requested_level = level;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | Retry | retry current level
bool ScriptRetry(Script::CStruct *pParams, Script::CScript *pScript)
{
	Skate* skate_mod = Skate::Instance();
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	skate_mod->LaunchGame();	
	Mem::Manager::sHandle().PopContext();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LaunchGame | 
bool ScriptLaunchGame(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	Skate::Instance()->LaunchGame();	
	Mem::Manager::sHandle().PopContext();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptFillRankingScreen(Script::CStruct *pParams, Script::CScript *pScript)
{
	 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	Lst::Search< GameNet::PlayerInfo > sh;
	GameNet::PlayerInfo* player;
	Lst::Head< GameNet::ScoreRank > rank_list;
	Lst::Search< GameNet::ScoreRank > rank_sh;
	GameNet::ScoreRank* rank, *next;
	bool in_goal_attack;
	int i;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());

	in_goal_attack = skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netgoalattack" );

	if( skate_mod->GetGameMode()->IsTeamGame())
	{
		for( i = 0; i < skate_mod->GetGameMode()->NumTeams(); i++ )
		{
			if( gamenet_man->NumTeamMembers( i ) == 0 )
			{
				continue;
			}
			char team_name_str[64];
						
			rank = new GameNet::ScoreRank;
			rank->m_IsKing = false;
			rank->m_ColorIndex = i + 2;
			rank->m_TeamId = i;
			rank->m_TotalScore = gamenet_man->GetTeamScore( i );
			
			sprintf( team_name_str, "team_%d_name", i + 1 );
			sprintf( rank->m_Name, "\\c%d%s %s", rank->m_ColorIndex, Script::GetString( team_name_str ), Script::GetString( "total_str" ));
						
			rank->SetPri( gamenet_man->GetTeamScore( i ));
			rank_list.AddNode( rank );
		}

		// Now loop through the sorted list in order of highest to lowest score and print them out
		i = 0;
		for( rank = rank_sh.FirstItem( rank_list ); rank; rank = next )
		{   
			Script::CStruct* p_item_params;
			char score_str[64];
	
			next = rank_sh.NextItem();
	
			for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
			{
				char player_score_str[128];
				int player_score;
                
				if( player->m_Team != rank->m_TeamId )
				{
					continue;
				}

				if( in_goal_attack )
				{
					Game::CGoalManager* pGoalManager;
			 
					pGoalManager = Game::GetGoalManager();
					player_score = pGoalManager->NumGoalsBeatenBy( player->m_Skater->GetID());
				}
				else
				{
					player_score = gamenet_man->GetPlayerScore( player->m_Skater->GetID());
				}
				

				if( player->IsLocalPlayer())
				{
					sprintf( player_score_str, "\\c%d> %s", rank->m_ColorIndex, player->m_Name );
				}
				else
				{
					sprintf( player_score_str, "\\c%d%s", rank->m_ColorIndex, player->m_Name );
				}
				
				p_item_params = new Script::CStruct;	
				p_item_params->AddString( "text", player_score_str );
				// p_item_params->AddChecksum( "id", 123456 + i );
				p_item_params->AddChecksum( "parent", Script::GenerateCRC( "player_list_menu" ));

				Script::RunScript("player_menu_add_item", p_item_params);
				delete p_item_params;
		
				p_item_params = new Script::CStruct;	
				if(	skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netking" ) ||
					skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "king" ))
				{
					sprintf( score_str, "%.2d:%.2d", Tmr::InSeconds( player_score ) / 60, Tmr::InSeconds( player_score ) % 60 );
				}
				else
				{
					sprintf( score_str, "%d", player_score );
				}
				p_item_params->AddString( "text", score_str );
				p_item_params->AddChecksum( "font", Script::GenerateCRC( "dialog" ) );
				p_item_params->AddChecksum( "id", 234567 + i );
				p_item_params->AddChecksum( "parent", Script::GenerateCRC( "rankings_list_menu" ));
		
				Script::RunScript("score_menu_add_item",p_item_params);
				delete p_item_params;

				i++;
			}
			p_item_params = new Script::CStruct;	
			p_item_params->AddString( "text", rank->m_Name );
			// p_item_params->AddChecksum( "id", 123456 + i );
			p_item_params->AddChecksum( NONAME, Script::GenerateCRC( "team_score" ) );
			p_item_params->AddChecksum( "parent", Script::GenerateCRC( "player_list_menu" ));
	
			Script::RunScript("player_menu_add_item",p_item_params);
			delete p_item_params;
	
			p_item_params = new Script::CStruct;	
			if( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netking" ) ||
				skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "king" ))
			{
				sprintf( score_str, "\\c%d%.2d:%.2d", rank->m_ColorIndex, Tmr::InSeconds( rank->GetPri()) / 60, Tmr::InSeconds( rank->GetPri()) % 60 );
			}
			else
			{
				sprintf( score_str, "\\c%d%d", rank->m_ColorIndex, rank->GetPri());
			}
			
			p_item_params->AddString( "text", score_str );
			p_item_params->AddChecksum( "id", 234567 + i );
			p_item_params->AddChecksum( "font", Script::GenerateCRC( "dialog" ) );
			p_item_params->AddChecksum( NONAME, Script::GenerateCRC( "team_score" ) );
			p_item_params->AddChecksum( "parent", Script::GenerateCRC( "rankings_list_menu" ));
	
			Script::RunScript("score_menu_add_item",p_item_params);
			delete p_item_params;
	
			delete rank;
			i++;
		}
	}
	else
	{
		for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
		{
			int player_score;

			if( in_goal_attack )
			{
				Game::CGoalManager* pGoalManager;
		 
				pGoalManager = Game::GetGoalManager();
				player_score = pGoalManager->NumGoalsBeatenBy( player->m_Skater->GetID());
			}
			else
			{
				player_score = gamenet_man->GetPlayerScore( player->m_Skater->GetID());
			}
				
			rank = new GameNet::ScoreRank;
			strcpy( rank->m_Name, player->m_Skater->GetDisplayName() );
	
			// Lists sort based on node's priority so set the node's priority to that of
			// the player's score and add him to the list
			rank->SetPri( player_score );
			rank_list.AddNode( rank );
		}
	
		// Now loop through the sorted list in order of highest to lowest score and print them out
		i = 0;
		for( rank = rank_sh.FirstItem( rank_list ); rank; rank = next )
		{   
			Script::CStruct* p_item_params;
			char score_str[64];
	
			next = rank_sh.NextItem();
	
			//Script::RunScript( "prepare_server_menu_for_new_children" );
			
			p_item_params = new Script::CStruct;	
			p_item_params->AddString( "text", rank->m_Name );
			// p_item_params->AddChecksum( "id", 123456 + i );
			p_item_params->AddChecksum( "parent", Script::GenerateCRC( "player_list_menu" ));
	
			Script::RunScript("player_menu_add_item",p_item_params);
			delete p_item_params;
	
			p_item_params = new Script::CStruct;	
			if( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netking" ) ||
				skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "king" ))
			{
				sprintf( score_str, "%.2d:%.2d", Tmr::InSeconds( rank->GetPri()) / 60, Tmr::InSeconds( rank->GetPri()) % 60 );
			}
			else
			{
				sprintf( score_str, "%d", rank->GetPri());
			}

			p_item_params->AddString( "text", score_str );
			p_item_params->AddChecksum( "id", 234567 + i );
			p_item_params->AddChecksum( "font", Script::GenerateCRC( "dialog" ) );
			p_item_params->AddChecksum( "parent", Script::GenerateCRC( "rankings_list_menu" ));
	
			Script::RunScript("score_menu_add_item",p_item_params);
			delete p_item_params;
	
			delete rank;
			i++;
		}
	}

	Mem::Manager::sHandle().PopContext();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// K: This function used to use GetMaximumNumberOfPlayers,
// but I changed it to use p_gamenet_man->GetMaxPlayers so that only
// the required amount of memory is allocated, hence allowing bigger
// parks that allow fewer players.
// p_gamenet_man->GetMaxPlayers will return the num players as chosen from the Players menu
// when starting a network game.

// @script | InitSkaterHeaps | 
bool ScriptInitSkaterHeaps(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	
	
	int num_skater_heaps_required=1;
	
    Skate *p_skate = Skate::Instance();
	if (p_skate->GetGameType()==CRCD(0x5c8f5d66,"freeskate2p"))
	{
		num_skater_heaps_required=2;
	}
	else
	{
		GameNet::Manager * p_gamenet_man = GameNet::Manager::Instance();
		num_skater_heaps_required=p_gamenet_man->GetMaxPlayers();
	}
		
	Dbg_Printf( "\nInitializing %d Skater Heaps\n", num_skater_heaps_required);
			   
	// If we are startinga  game with possibly more than two players
	// then we want to allocate     
	if( num_skater_heaps_required > NUM_PERM_SKATER_HEAPS )
	{
		Mem::Manager::sHandle().DeleteSkaterHeaps();
		Mem::Manager::sHandle().InitSkaterHeaps(num_skater_heaps_required);
	}

	Mem::Manager::sHandle().PopContext();
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetLevel | resets the level
bool ScriptResetLevel(Script::CStruct *pParams, Script::CScript *pScript)
{
	Skate* skate_mod = Skate::Instance();

	// A hacky way of preserving the running script, since skate's cleanup will destroy all
	// spawned scripts.
	bool was_spawned = false;
	if( pScript )
	{
		was_spawned = pScript->mIsSpawned;
		pScript->mIsSpawned = false;
	}
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	skate_mod->ResetLevel();	
	Mem::Manager::sHandle().PopContext();
	
	if( pScript )
	{
		pScript->mIsSpawned = was_spawned;
	}

	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#if 0

static char * maybe_new(int n, bool new_record)
{
	char *p = Script::GetScriptString(n);
	if (new_record)
	{
		sprintf (p,"&c1");
		p+=3;
	}
	return p;	
}

// insert a single line into the script strings
void populate_line(int &index, Records::CRecord *pRecord)
{
		sprintf(maybe_new(index++,pRecord->GetNewRecord()),"%s",Str::PrintThousands(pRecord->GetValue()));
		sprintf(maybe_new(index++,pRecord->GetNewRecord()),"%d",pRecord->GetNumber());
		sprintf(maybe_new(index++,pRecord->GetNewRecord()),pRecord->GetInitials());
}

#ifndef __PLAT_NGC__
// insert a single line into the script strings
static void populate_line_secs(int &index, Records::CRecord *pRecord)
{
		#if (ENGLISH == 0)		
		sprintf(maybe_new(index++,pRecord->GetNewRecord()),"%3.2f %s",(float)pRecord->GetValue()/100.0f,Script::GetLocalString( "skate_str_secs"));
		#else		
		sprintf(maybe_new(index++,pRecord->GetNewRecord()),"%3.2f secs",(float)pRecord->GetValue()/100.0f);
		#endif
		sprintf(maybe_new(index++,pRecord->GetNewRecord()),"%d",pRecord->GetNumber());
		sprintf(maybe_new(index++,pRecord->GetNewRecord()),pRecord->GetInitials());
}


static void  populate_table(int &index, Records::CRecordTable *pRecordTable)
{

	int entries = pRecordTable->GetSize();
	for (int i = 0; i < entries; i++)
	{
		populate_line(index, pRecordTable->GetRecord(i));		
	}
}
#endif		// __PLAT_NGC__

#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Update the records with whatever we just did this session 
void Skate::UpdateRecords()
{
	m_new_record = false;
	
	int level = GetCareer()->GetLevel();

	// get the records for this level	
	Records::CLevelRecords *pRecords = GetGameRecords()->GetLevelRecords(level);

   	// find the skater, get the score landed:
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	Obj::CSkater *pSkater = skate_mod->GetLocalSkater(); 
	if( pSkater == NULL )
	{
		return;
	}          

	Mdl::Score * pScore = ( pSkater->GetScoreObject() );
	/*
	HUD::PanelMgr* panel_mgr = HUD::PanelMgr::Instance();
	HUD::Panel* pPanel;
	pPanel = panel_mgr->GetPanelBySkaterId(pSkater->GetID(), false);
	*/

	// Insert the score into the high score table if it is high enough	 
	m_new_record |=  (-1 != pRecords->GetHighScores()->MaybeNewEntry(pScore->GetTotalScore(),0));												  
	
	m_new_record |=  (-1 != pRecords->GetBestCombos()->MaybeNewEntry(pScore->GetBestCombo(),0));

	Obj::CSkaterBalanceTrickComponent* p_skater_balance_trick_component = GetSkaterBalanceTrickComponentFromObject(pSkater);
	Dbg_Assert(p_skater_balance_trick_component);
	
	int this_grind = (int)(p_skater_balance_trick_component->mGrind.GetMaxTime()*100);
	int this_manual = (int)(p_skater_balance_trick_component->mManual.GetMaxTime()*100);
	int	this_lip = (int)(p_skater_balance_trick_component->mLip.GetMaxTime()*100);
	// int this_combo = 0;
	int this_combo = pScore->GetLongestCombo();

	m_new_record |= pRecords->GetLongestGrind()->MaybeNewRecord(this_grind,0);	
	m_new_record |= pRecords->GetLongestManual()->MaybeNewRecord(this_manual,0);	
	m_new_record |= pRecords->GetLongestLipTrick()->MaybeNewRecord(this_lip,0);		
	m_new_record |= pRecords->GetLongestCombo()->MaybeNewRecord(this_combo,0);		
}

#define	MAX_COMBO_LINES  200			// more that we need...

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifndef __PLAT_NGC__
#if 0
static float text_width(const char *p_string, const char *p_font)
{
	Fnt::Drawer drawer;
	drawer.SetFont(p_font);
	drawer.SetText(p_string);
	return drawer.GetWidth();
}
#endif
#endif		// __PLAT_NGC__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Utility function to get the info out of the level records
// and into the script strings
void Skate::GetRecordsText(int level)
{
#if 0
#ifndef __PLAT_NGC__
	

	// get the records for this level	
	Records::CLevelRecords *pRecords = GetGameRecords()->GetLevelRecords(level);

	int index = 0;
	
		// find the skater, get the score landed:
	 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	Obj::CSkater *pSkater = skate_mod->GetLocalSkater();
	Mdl::Score * pScore = ( pSkater->GetScoreObject() );
	/*
	HUD::PanelMgr* panel_mgr = HUD::PanelMgr::Instance();
	HUD::Panel* pPanel;
	pPanel = panel_mgr->GetPanelBySkaterId(pSkater->GetID(), false);
	Dbg_MsgAssert(pPanel,("Could not get panel for skater on records screen"));
	*/

	int this_grind = (int)(pSkater->mGrind.GetMaxTime()*100);
	int this_manual = (int)(pSkater->mManual.GetMaxTime()*100);
	int	this_lip = (int)(pSkater->mLip.GetMaxTime()*100);
	
	// get total score for this level																		
	sprintf(Script::GetScriptString(index++),"%s",Str::PrintThousands(pScore->GetTotalScore()));		 // 0
	// get highest score from high score table
	sprintf(Script::GetScriptString(index++),"%s",Str::PrintThousands(pRecords->GetHighScores()->GetRecord(0)->GetValue()));	  // 1
	
	
	#if (ENGLISH == 0)
	
	sprintf(Script::GetScriptString(index++),"%3.2f %s",(float)this_grind/100.0f,Script::GetLocalString( "skate_str_secs"));					  // 2
	sprintf(Script::GetScriptString(index++),"%3.2f %s",(float)this_manual/100.0f,Script::GetLocalString( "skate_str_secs"));					  // 3
	sprintf(Script::GetScriptString(index++),"%3.2f %s",(float)this_lip/100.0f,Script::GetLocalString( "skate_str_secs"));						  // 4
	sprintf(Script::GetScriptString(index++),"%d %s",pPanel->GetNumLongestCombo(),Script::GetLocalString( "skate_str_tricks"));			// 5
	sprintf(Script::GetScriptString(index++),Script::GetLocalString( "skate_str_points_tricks"),Str::PrintThousands(pPanel->GetScoreBestCombo()),pPanel->GetNumBestCombo());	// 6

	#else
	
	sprintf(Script::GetScriptString(index++),"%3.2f secs",(float)this_grind/100.0f);					  // 2
	sprintf(Script::GetScriptString(index++),"%3.2f secs",(float)this_manual/100.0f);					  // 3
	sprintf(Script::GetScriptString(index++),"%3.2f secs",(float)this_lip/100.0f);						  // 4
	sprintf(Script::GetScriptString(index++),"1 tricks");			// 5
	sprintf(Script::GetScriptString(index++),"2 points in 2 tricks");	// 6
	//sprintf(Script::GetScriptString(index++),"%d tricks",pPanel->GetNumLongestCombo());			// 5
	//sprintf(Script::GetScriptString(index++),"%s points in %d tricks",Str::PrintThousands(pPanel->GetScoreBestCombo()),pPanel->GetNumBestCombo());	// 6

	#endif

	// What we want is to get the last N lines of combo text, word wrapped
	// to the correct size for the font we will use on the statistics screen
	//
	// What we have is the text for the individual tricks, and the number of tricks.
	//
	// - Set up a textdrawer with the correct size and font
	// - add text to it, and to a string, until full
	// - copy string into a ScriptString, and repeat

	// find font we will be using by looking in the appropiate property 
	Script::CStruct * p_record_font_props = Script::GetStructure("statistics_font_props");
	const char *p_font;
	p_record_font_props->GetText("font",&p_font);	
	Script::CStruct * p_record_box3_props = Script::GetStructure("statistics_box_3_props");
	int	box_width;
	p_record_box3_props->GetInteger("box_w",&box_width);
	  	
	
	Str::String line_table[MAX_COMBO_LINES];
	int line = 0;
	float current_w = 0;
	int num_combos = 0;
	//int num_combos = pPanel->GetNumBestCombo();
	for (int trick =0; trick < num_combos; trick++)
	{
		const char * trick_chars = "blah";
		//const char * trick_chars = pPanel->GetStringBestCombo(trick);
		float trick_w = text_width(trick_chars, p_font);
		float new_w = current_w + trick_w;
		if (current_w > 0.0f && new_w > box_width)
		{
			line++;
			Dbg_MsgAssert(line < MAX_COMBO_LINES,("Far to many combo lines"));
			current_w = 0.0f;
		}
		if (current_w == 0.0f)
		{
			line_table[line] = trick_chars;
		}
		else
		{
			// combine the two strings into one string by sprintfing them into a temporary buffer
			char temp[1024];	  	// good job we have a huge stack!!
			sprintf (temp,"%s%s",line_table[line].getString(),trick_chars);
			Dbg_MsgAssert(strlen(temp) < 1024,("line insanely too long"));  // not taking any chances
			line_table[line] = temp;	 // this will delete the old string contents
		}
		current_w += trick_w;
	}
//	int num_lines = line+1;

	line = 0;
	while (index < 20)
	{
		sprintf(Script::GetScriptString(index++),line_table[line++].getString());		
	}

/* functions we can use
	int  	GetScoreBestCombo();
	int		GetNumBestCombo();
	const char	* GetStringBestCombo(int trick);
	int  	GetScoreLongestCombo();
	int		GetNumLongestCombo();
*/

	index = 20;

	populate_table(index, pRecords->GetHighScores());								  			// 20-34
	populate_table(index, pRecords->GetBestCombos());											// 35-49
	populate_line_secs(index,pRecords->GetLongestGrind());											// 50-52
	populate_line_secs(index,pRecords->GetLongestManual());											// 53-55
	populate_line_secs(index,pRecords->GetLongestLipTrick());										// 56-58
	populate_line(index,pRecords->GetLongestCombo());											// 59-61
#endif		// __PLAT_NGC__
#endif
}
																			 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::UpdateSkaterInputHandlers()
{	
	printf("&&&&&&&&&&&&&&&&&&&&&&&&&& UpdateSkaterInputHandlers called\n");
	
	for ( int i = 0; i < vMAX_SKATERS; i++ )
	{
		Obj::CSkater* pSkater = GetSkater( i );
		if ( pSkater && pSkater->IsLocalClient() )
		{
			printf("found a local skater - binding controller\n");
			int heap_index = pSkater->GetHeapIndex();
			Dbg_MsgAssert( heap_index >= 0 && heap_index < vMAX_SKATERS, ( "heap index %i out of range", heap_index ) );
			
			Dbg_Assert(GetInputComponentFromObject(pSkater));
			GetInputComponentFromObject(pSkater)->BindToController(m_device_server_map[heap_index]);
			
			// Dan: Pretty sure this method is wrong.
			//char skatecam[16];
			//sprintf(skatecam,"SkaterCam%d",i);
			//if (Obj::CCompositeObject *p_obj
				//= static_cast< Obj::CCompositeObject* >(Obj::CCompositeObjectManager::Instance()->GetObjectByID(Script::GenerateCRC(skatecam))))
			if (Obj::CCompositeObject *p_obj = pSkater->GetCamera())
			{
				Dbg_Assert(GetInputComponentFromObject(p_obj));
				GetInputComponentFromObject(p_obj)->BindToController(m_device_server_map[heap_index]);
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Fills in a little structure with some stuff so that SkipLogic is really fast.
void Skate::pre_calculate_object_update_info()
{
	if (Nx::CViewportManager::sGetNumActiveViewports() > 1 || ! Nx::CViewportManager::sGetActiveCamera(0))
	{
		// Nothing should be suspended if multiplayer, so leave the flag true.
		m_precalculated_object_update_info.mDoNotSuspendAnything=true;
	}
	else
	{
		m_precalculated_object_update_info.mDoNotSuspendAnything=false;
		m_precalculated_object_update_info.mActiveCameraPosition = Nx::CViewportManager::sGetActiveCamera(0)->GetPos();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Prefs::Preferences* GetPreferences( Script::CStruct* pParams, bool assert_on_fail )
{   
	uint32 checksum;
	pParams->GetChecksum( "prefs", &checksum, true );

	return GetPreferences( checksum, assert_on_fail );

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Prefs::Preferences* GetPreferences( uint32 checksum, bool assert_on_fail )
{	
	if ( checksum == Script::GenerateCRC("network") )
	{
		 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
		return gamenet_man->GetNetworkPreferences();
	}
	else if ( checksum == Script::GenerateCRC("taunt") )
	{
		 GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();
		return gamenet_man->GetTauntPreferences();
	}
	else if ( checksum == Script::GenerateCRC("splitscreen") )
	{
		 Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
		return skate_mod->GetSplitScreenPreferences();
	}

	if ( assert_on_fail )
	{
		Dbg_MsgAssert( 0, ( "Couldn't find preferences for %s", Script::FindChecksumName(checksum) ) );
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Just a simple debugging interface function to call the main rail 
// manager's DebugRender() function
void			Rail_DebugRender()
{
	Mdl::Skate* skate_mod =  Mdl::Skate::Instance();
	if (!skate_mod->GetDrawRails())
	{
		return;
	}

	skate_mod->GetRailManager()->DebugRender();

	for (Obj::CRailManagerComponent* p_rail_manager_component = static_cast< Obj::CRailManagerComponent* >(Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType(CRC_RAILMANAGER));
		p_rail_manager_component;
		p_rail_manager_component = static_cast< Obj::CRailManagerComponent* >(p_rail_manager_component->GetNextSameType()))
	{
		Obj::CCompositeObject* p_movable_object = p_rail_manager_component->GetObject();

		// form a transformation matrix
		Mth::Matrix total_mat = p_movable_object->GetMatrix();
		total_mat[X][W] = 0.0f;
		total_mat[Y][W] = 0.0f;
		total_mat[Z][W] = 0.0f;
		total_mat[W] = p_movable_object->GetPos();
		total_mat[W][W] = 1.0f;

		
		Obj::CRailManager *p_railman = p_rail_manager_component->GetRailManager();								   
		if (p_railman)
		{
			p_railman->DebugRender(&total_mat);			
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// return the current gap checklist for the level that we are in														 
Obj::CGapChecklist* Skate::GetGapChecklist()
{
	Obj::CGapChecklist* p_gap_checklist = GetCareer()->GetGapChecklist(); 
	Dbg_Assert(p_gap_checklist);
	return p_gap_checklist;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Skate::ShouldAllocateNetMiscHeap( void )
{
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	return ( gamenet_man->InNetGame() && ( m_cur_level != Script::GenerateCRC( "Load_Skateshop" ))) ;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Skate::ShouldAllocateInternetHeap( void )
{
	GameNet::Manager * gamenet_man =  GameNet::Manager::Instance();

	if( gamenet_man->InNetMode())
	{
		return true;
	}

	// GJ:  need to temporarily remove LOAD_CAS, because 
	// the internet heap is coming in after the unloadable
	// anims, and the unloadable anims need to be the last
	// thing on the topdown heap
	if( ( m_requested_level == Script::GenerateCRC( "load_skateshop" ) ) 
		|| ( m_requested_level == Script::GenerateCRC( "load_CAS" )))
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Skate::FirstInputReceived()
{
	m_first_input_received = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mdl
