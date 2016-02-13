//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       ViewerObj.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  1/21/2002
//****************************************************************************

#include <sk/objects/viewerobj.h>
								 
#include <gel/components/animationcomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/skeletoncomponent.h>

#include <gel/mainloop.h>
#include <gel/objman.h>
#include <gel/assman/assman.h>
#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/net/server/netserv.h>

#include <gfx/bonedanim.h>
#include <gfx/gfxutils.h>
#include <gfx/modelappearance.h>
#include <gfx/nx.h>
#include <gfx/nxmodel.h>
#include <gfx/nxhierarchy.h>
#include <gfx/skeleton.h>

#include <sk/gamenet/exportmsg.h>
#include <sk/gamenet/gamenet.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/viewer/viewer.h>
#include <sk/objects/skaterprofile.h>
#include <sk/scripting/cfuncs.h>
#include <sk/scripting/gs_file.h>
#include <sk/scripting/skfuncs.h>

#include <sys/config/config.h>

// TODO:  Remove the rest of the level when the viewer is active								   
// TODO:  Launch a primary sequence by name, pointer, or index
// TODO:  The shoulder buttons should increment the current anim (next/prev anim)
// TODO:  Don't crash if can't find the animation...
// TODO:  Add handlers so that it can launch new anims and such
// TODO:  Camera controls
// TODO:  Be able to go back to the game
// TODO:  Wireframe	mode
// TODO:  Change skater profile, in real time
// TODO:  Take LODs into account
								   
namespace Obj
{

	const float vSPEED_INCREMENT_AMOUNT = 0.25;
	const Mth::Vector vPOS_VIEWEROBJECT( 0.0f, 0.0f, 0.0f );

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewerObject::s_handle_set_anim( Net::MsgHandlerContext* context )
{
	CViewerObject* pViewerObject;
	pViewerObject = (CViewerObject*) context->m_Data;
	Dbg_Assert( pViewerObject );
	
	Net::MsgViewObjSetAnim* p_msg;
	p_msg = (Net::MsgViewObjSetAnim*) context->m_Msg;
	pViewerObject->SetAnim( p_msg->m_AnimName );
	
	Dbg_Message( "Got request to set anim %s...", Script::FindChecksumName(p_msg->m_AnimName) );

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewerObject::s_handle_set_anim_speed( Net::MsgHandlerContext* context )
{
	Dbg_Message( "Got request to set anim speed..." );
	
	CViewerObject* pViewerObject;
	pViewerObject = (CViewerObject*) context->m_Data;
	Dbg_Assert( pViewerObject );

	Net::MsgViewObjSetAnimSpeed* p_msg;
	p_msg = (Net::MsgViewObjSetAnimSpeed*) context->m_Msg;
	pViewerObject->ChangeAnimSpeed( p_msg->m_AnimSpeed );
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewerObject::s_handle_increment_frame( Net::MsgHandlerContext* context )
{
	Dbg_Message( "Got request to increment frame..." );

	CViewerObject* pViewerObject;
	pViewerObject = (CViewerObject*) context->m_Data;
	Dbg_Assert( pViewerObject );
					
	Net::MsgViewObjIncrementFrame* p_msg;
	p_msg = (Net::MsgViewObjIncrementFrame*) context->m_Msg;
	pViewerObject->IncrementFrame( p_msg->m_Forwards );

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewerObject::s_handle_reload_anim( Net::MsgHandlerContext* context )
{
	Dbg_Message( "Got request to reload anim..." );

	CViewerObject* pViewerObject;
	pViewerObject = (CViewerObject*) context->m_Data;
	Dbg_Assert( pViewerObject );
					
	Net::MsgViewObjSetAnimFile* p_msg;
	p_msg = (Net::MsgViewObjSetAnimFile*) context->m_Msg;
	pViewerObject->ReloadAnim( p_msg->m_Filename, p_msg->m_checksum );

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewerObject::s_handle_sequence_preview( Net::MsgHandlerContext* context )
{
	char path[256];
	char* script_name;
	CViewerObject* pViewerObject;
	pViewerObject = (CViewerObject*) context->m_Data;
	Dbg_Assert( pViewerObject );

	script_name = context->m_Msg;
	sprintf( path, "scripts\\animview\\%s.qb", script_name );
	SkateScript::LoadQB( path );
	//Script::RunScriptOnObject("RunMeNow",NULL,pViewerObject);
	Script::CScript *pNewScript=Script::SpawnScript(script_name,NULL,0,NULL);
	pNewScript->mpObject = pViewerObject;
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CViewerObject::s_handle_reload_cam_anim( Net::MsgHandlerContext* context )
{
	Dbg_Message( "Got request to reload cam anim..." );

	Net::MsgViewObjSetAnimFile* p_msg;
	p_msg = (Net::MsgViewObjSetAnimFile*) context->m_Msg;
	
	Script::CStruct* pTempStruct = new Script::CStruct;
								  
	pTempStruct->AddString( NONAME, p_msg->m_Filename );
	pTempStruct->AddChecksum( NONAME, p_msg->m_checksum );

	CFuncs::ScriptReloadSkaterCamAnim( pTempStruct, NULL );

	delete pTempStruct;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CViewerObject::CViewerObject( Net::Server* pServer )
{
	Inp::Manager * inp_manager = Inp::Manager::Instance();
    mp_input_handler = new Inp::Handler< CViewerObject > ( 0,  s_input_logic_code, *this, Tsk::BaseTask::Node::vNORMAL_PRIORITY );
	inp_manager->AddHandler( *mp_input_handler );

    m_animSpeed = 1.0f;
	m_showPanel = true;

	m_matrix.Ident();

	m_paused = false;
	
	mp_server = pServer;

	Dbg_Assert( mp_server );

	mp_server->m_Dispatcher.AddHandler( Net::vMSG_ID_VIEWOBJ_SET_ANIM, 
									  s_handle_set_anim,
									  Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this );
	mp_server->m_Dispatcher.AddHandler( Net::vMSG_ID_VIEWOBJ_SET_ANIM_SPEED, 
									  s_handle_set_anim_speed,
									  Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this );
	mp_server->m_Dispatcher.AddHandler( Net::vMSG_ID_VIEWOBJ_INCREMENT_FRAME, 
									  s_handle_increment_frame,
									  Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this );
	mp_server->m_Dispatcher.AddHandler( Net::vMSG_ID_VIEWOBJ_SET_ANIM_FILE, 
									  s_handle_reload_anim,
									  Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this );
	mp_server->m_Dispatcher.AddHandler( Net::vMSG_ID_VIEWOBJ_SET_CAMANIM_FILE, 
									  s_handle_reload_cam_anim,
									  Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this );
	mp_server->m_Dispatcher.AddHandler( Net::vMSG_ID_VIEWOBJ_PREVIEW_SEQUENCE, 
									  s_handle_sequence_preview,
									  Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this );
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CViewerObject::~CViewerObject()
{
	mp_input_handler->Remove();

    delete mp_input_handler;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void s_remove_asset( const char* pFileName )
{
	// if the file is in the asset manager, unload it so
	// that the artists can display the latest version
	// ("cas_artist" must be set to 1, or else it'll just
	// reload it from the PRE file)
	
	if ( !Script::GetInt( "cas_artist", false ) )
	{
		Dbg_Message( "Warning:  Can't reload asset %s from disk (need to define cas_artist=1)", pFileName );
		return;
	}

	// get the assman
	Ass::CAssMan * ass_man = Ass::CAssMan::Instance();

	// kills the original asset
	Ass::CAsset* pAsset = ass_man->GetAssetNode( Script::GenerateCRC( pFileName ), false );

	if ( pAsset )
	{		
		Dbg_Message( "Unloading asset %s", pFileName );
		ass_man->DestroyReferences( pAsset );
		ass_man->UnloadAsset( pAsset );
	}
	else
	{
		Dbg_Message( "Couldn't find asset %s to unload", pFileName );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewerObject::LoadModel( Script::CStruct* pNodeData )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	skate_mod->GetObjectManager()->RegisterObject( *this );
	MovingObjectCreateComponents();
	MovingObjectInit( pNodeData, skate_mod->GetObjectManager() );

	Nx::CEngine::sFinishRendering();

	if (Config::GotExtraMemory())
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
	}
	else
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	}
		
	Dbg_Assert( pNodeData );
	
	// get rid of old model, if
	// it was stored in the assman
	int skaterProfileNumber;
	const char* pModelName;
	uint32 profileName;

	for ( int i = 0; i < Obj::CModelComponent::vNUM_LODS; i++ )
	{
		char paramName[256];
		sprintf( paramName, "model%d", i ); 

		if ( pNodeData->GetText( paramName, &pModelName, false ) )
		{
			Dbg_Message( "Removing model LOD data from asset manager, in order to replace it with new data" );

			Str::String fullModelName;
			fullModelName = Gfx::GetModelFileName(pModelName, ".skin");		
			s_remove_asset( fullModelName.getString() );

			fullModelName = Gfx::GetModelFileName(pModelName, ".mdl");		
			s_remove_asset( fullModelName.getString() );
		}
	}

	if ( pNodeData->GetText( "modelName", &pModelName, false ) 
		 || pNodeData->GetText( "model", &pModelName, false ) )
	{
		Dbg_Message( "Removing model data from asset manager, in order to replace it with new data" );

		Str::String fullModelName;
		fullModelName = Gfx::GetModelFileName(pModelName, ".skin");		
		s_remove_asset( fullModelName.getString() );
		
		fullModelName = Gfx::GetModelFileName(pModelName, ".mdl");		
		s_remove_asset( fullModelName.getString() );
	}
	else if ( pNodeData->GetChecksum( "profile", &profileName, false ) )
	{
		Dbg_Message( "Warning:  can't remove old model data from asset manager...  asset not reloaded from disk." );
	}
	else if ( pNodeData->GetInteger( "skater_profile_index", &skaterProfileNumber, false ) )
	{
		Dbg_Message( "Warning:  can't remove old model data from asset manager...  asset not reloaded from disk." );

		Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( skaterProfileNumber );
		Script::CStruct* pAppearanceStruct = pSkaterProfile->GetAppearance()->GetStructure();
		Script::CStruct* pNewStruct = new Script::CStruct;
		pNewStruct->AppendStructure( pAppearanceStruct );
		pNodeData->AddStructurePointer( "profile", pNewStruct );
		// don't need to delete the structure, 
		// because it gets added permanently to the struct
	}
	else
	{
		Dbg_MsgAssert( 0, ( "Unrecognized parameters for model loading" ) );
	}

	m_pos = vPOS_VIEWEROBJECT;
	m_matrix.Ident();
	
	Script::RunScript( CRCD(0x24c71bc7,"viewerobj_add_components"), pNodeData, this );

	Finalize();

	Script::RunScript( CRCD(0xbbab79de,"viewerobj_init_model"), pNodeData, this );

	// loop the idle anim, if it exists
	CViewerObject::SetAnim( CRCD(0x23db4aea,"idle") );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewerObject::UnloadModel()
{
	Script::RunScript( "kill_viewer_object_panel" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewerObject::SetAnim( uint32 animName )
{
    if ( GetAnimationComponent() )
    {
        if ( GetAnimationComponent()->AnimExists( animName ) )
        {
            GetAnimationComponent()->PlayPrimarySequence( animName, false, 0.0f, GetAnimationComponent()->AnimDuration( animName ), Gfx::LOOPING_CYCLE);
        
/*
			Script::CStruct* pTempStruct = new Script::CStruct;
			pTempStruct->AddChecksum( "type", Script::GenerateCRC("partialAnim" ) );
			pTempStruct->AddChecksum( "AnimName", Script::GetChecksum("TestPartialAnim" ) );
			pTempStruct->AddChecksum( "from", Script::GenerateCRC("start") );
			pTempStruct->AddChecksum( "to", Script::GenerateCRC("end") );
			pTempStruct->AddChecksum( NONAME, Script::GenerateCRC("cycle") );
			pTempStruct->AddFloat( "speed", 1.0f );
			Script::RunScript( "AddAnimController", pTempStruct, this );
			delete pTempStruct;
*/
		}
        else
        {
#ifdef	__NOPT_ASSERT__
            Dbg_Message( "Unrecognized anim %s", Script::FindChecksumName(animName) );
#endif
        }
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewerObject::SetRotation( float rotZ, float rotX )
{
	m_matrix.Ident();
	m_matrix.RotateZ( rotZ );
	m_matrix.RotateX( rotX );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewerObject::ChangeAnimSpeed( float animSpeed )
{
	m_animSpeed = animSpeed;

	if ( m_animSpeed > 1.0f )
	{
		m_animSpeed = 1.0f;
	}

	if ( m_animSpeed < 0.0f )
	{
		m_animSpeed = 0.0f;
	}

    if ( GetAnimationComponent() )
    {
        GetAnimationComponent()->SetAnimSpeed( m_animSpeed, false );
		
		// update partial anims
		Script::CStruct* pTempStruct = new Script::CStruct;
		pTempStruct->AddFloat( CRCD(0xf0d90109,"speed"), m_animSpeed );
		GetAnimationComponent()->CallMemberFunction( CRCD(0xbd4edd44,"SetPartialAnimSpeed"), pTempStruct, NULL );
		delete pTempStruct;
    }

	if ( GetAnimationComponent() )
	{
		// reset the paused state...
		GetAnimationComponent()->Suspend( false );
	}

	m_paused = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewerObject::IncrementFrame( bool forwards )
{
    if ( GetAnimationComponent() )
    {
		if ( forwards )
        {
            // frame-rate independent
            GetAnimationComponent()->AddTime( 1.0f / 60.0f );
			GetAnimationComponent()->PrintStatus();
        
			Script::CStruct* pTempStruct = new Script::CStruct;
			pTempStruct->AddFloat( CRCD(0x06c9f278,"incVal"), 1.0f / 60.0f );
			GetAnimationComponent()->CallMemberFunction( CRCD(0x6aaeb76f,"IncrementPartialAnimTime"), pTempStruct, NULL );
			delete pTempStruct;
		}
        else
        {
			// reverse direction so that it loops properly
			GetAnimationComponent()->ReverseDirection( false );

			// frame-rate independent
            GetAnimationComponent()->AddTime( -1.0f / 60.0f );
            GetAnimationComponent()->PrintStatus();

			GetAnimationComponent()->ReverseDirection( false );
    		
			GetAnimationComponent()->CallMemberFunction( CRCD(0xf5e2b871,"ReversePartialAnimDirection"), NULL, NULL );
			
			Script::CStruct* pTempStruct = new Script::CStruct;
			pTempStruct->AddFloat( CRCD(0x06c9f278,"incVal"), -1.0f / 60.0f );
			GetAnimationComponent()->CallMemberFunction( CRCD(0x6aaeb76f,"IncrementPartialAnimTime"), pTempStruct, NULL );
			delete pTempStruct;
			
			GetAnimationComponent()->CallMemberFunction( CRCD(0xf5e2b871,"ReversePartialAnimDirection"), NULL, NULL );
		}
		
		// don't want to call the animation component's
		// Update() function, because that changes the
		// time...  instead, call UpdateSkeleton()
		// which syncs the skeleton to the current time
		GetAnimationComponent()->UpdateSkeleton();

		// reset the paused state...
		GetAnimationComponent()->Suspend( true );
	}

	m_paused = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewerObject::ReloadAnim( const char* pFileName, uint32 animName )
{
    if ( GetAnimationComponent() )
    {
        Dbg_Message( "Reloading anim..." );
        Dbg_Message( "Warning!  This function fragments memory.  Use it at your own risk!" );

        // get the assman
        Ass::CAssMan * ass_man = Ass::CAssMan::Instance();

        // kills the original asset
        Ass::CAsset* pAsset = ass_man->GetAssetNode( Script::GenerateCRC( pFileName ), false );
        if ( pAsset )
        {
            ass_man->DestroyReferences( pAsset );
            ass_man->UnloadAsset( pAsset );
        }
        else
        {
#ifdef __NOPT_ASSERT__
            Dbg_Message( "Couldn't find asset %s to unload", Script::FindChecksumName( animName ) );
#endif
        }

        // load anim asset here!!!!
        ass_man->LoadAnim( pFileName, animName, GetAnimationComponent()->GetAnimScriptName(), false, false );

        this->SetAnim( animName );
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewerObject::UpdateWheels()
{
	Nx::CHierarchyObject* pHierarchyObjects = GetModel()->GetHierarchy();
	Dbg_Assert( pHierarchyObjects );

	Gfx::CSkeleton* pCarSkeleton = GetSkeletonComponent()->GetSkeleton();
	Dbg_Assert( pCarSkeleton );

	Dbg_MsgAssert( GetModel()->GetNumObjectsInHierarchy()==pCarSkeleton->GetNumBones(),
				   ( "Skeleton and hierarchy doesn't match (%d hierarchy objs vs %d bones)", 
				    GetModel()->GetNumObjectsInHierarchy(), pCarSkeleton->GetNumBones() ) );

	static float s_wheelRotationX = 0.0f;
	
	// not frame rate independent (doesn't have valid m_time)
	s_wheelRotationX += ( Script::GetFloat("max_wheel_speed", Script::ASSERT) * m_animSpeed );

	// make sure it's within bounds
	while ( s_wheelRotationX > 360.0f )
	{
		s_wheelRotationX -= 360.0f;
	}

	while ( s_wheelRotationX < 0.0f )
	{
		s_wheelRotationX += 360.0f;
	}

	if ( GetModel()->GetNumObjectsInHierarchy() > 5 )
	{
		Mth::Matrix* pMatrices = pCarSkeleton->GetMatrices();
		Dbg_Assert( pMatrices );

		// initialize the setup matrix once per frame
		for ( int i = 0; i < GetModel()->GetNumObjectsInHierarchy(); i++ )
		{						
			*(pMatrices + i) = ( pHierarchyObjects + i )->GetSetupMatrix();
		}

		// apply the appropriate rotations to the wheels
		for ( int i = 2; i < 6; i++ )
		{
			(pMatrices + i)->RotateXLocal( s_wheelRotationX * 2.0f * Mth::PI / 360.0f );
		}

		// get children into object space
		for ( int i = 1; i < 6; i++ )
		{
			*(pMatrices + i) *= *(pMatrices + 0); 
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewerObject::Update()
{
	if ( GetAnimationComponent() )
    {
		if ( m_showPanel )
		{
			if ( GetAnimationComponent()->AnimExists(GetAnimationComponent()->GetCurrentSequence()) )
			{
				// pass the pertinent info to the panel:
				Script::CStruct* pTempStruct = new Script::CStruct;

				char line1[128];
				sprintf( line1, "name %s",
		#ifdef	__NOPT_ASSERT__
						 Script::FindChecksumName( GetAnimationComponent()->GetCurrentSequence() )
		#else
						 "Unknown"
		#endif				  
				  );

				pTempStruct->AddString( "line1", (const char*)&line1 );

				char line2[128];
				sprintf( line2, "speed %f", m_paused ? 0.0f : m_animSpeed );
				pTempStruct->AddString( "line2", (const char*)&line2 );

				char line3[128];
				sprintf( line3, "time %f of %f", 
						 GetAnimationComponent()->GetCurrentAnimTime(), 
						 GetAnimationComponent()->AnimDuration( GetAnimationComponent()->GetCurrentSequence() ) );
				pTempStruct->AddString( "line3", (const char*)&line3 );

				char line4[128];
				sprintf( line4, "frame %d of %d", (int)(GetAnimationComponent()->GetCurrentAnimTime() * 60.0f),
						 (int)(GetAnimationComponent()->AnimDuration( GetAnimationComponent()->GetCurrentSequence() ) * 60.0f) );
				pTempStruct->AddString( "line4", (const char*)&line4 );

				// TODO:  Print out anim size

				Script::RunScript( "draw_viewer_object_panel", pTempStruct );
				delete pTempStruct;
			}
		}
	}

	if ( GetModel()->GetNumObjectsInHierarchy() )
	{
		UpdateWheels();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewerObject::s_input_logic_code ( const Inp::Handler < CViewerObject >& handler )
{       
	CViewerObject& obj = handler.GetData();
 
	// this is the minimum amount of time
	// you need to hold down the left/right keys
	// before it will auto repeat
	int vAUTO_REPEAT_TIME = Script::GetInteger( CRCD(0xe60442d5,"ViewerAutoRepeatTime"), Script::NO_ASSERT );

	// this is how fast it auto repeats
	int vAUTO_REPEAT_SPEED = Script::GetInteger( CRCD(0x2da212e5,"ViewerAutoRepeatSpeed"), Script::NO_ASSERT );

	static int s_auto_repeat_delay = vAUTO_REPEAT_TIME;

	// GJ:  no longer clearing the last frame's data
	// at the beginning of the loop...  this allows
	// us to override the m_commands from an external
	// class (useful for doing custom controller
	// configs for different viewer modes)
//	obj.m_commands.ClearAll();

    if( handler.m_Input->m_Makes & Inp::Data::mD_TRIANGLE )
	{
        obj.m_commands.SetMask( mTOGGLE_PANEL );
	}

	if ( handler.m_Input->m_Makes & Inp::Data::mD_LEFT )
	{
		obj.m_commands.SetMask( mDECREMENT_FRAME );
		s_auto_repeat_delay = vAUTO_REPEAT_TIME;
	}
	else if ( handler.m_Input->m_Makes & Inp::Data::mD_RIGHT )
	{
		obj.m_commands.SetMask( mINCREMENT_FRAME );
		s_auto_repeat_delay = vAUTO_REPEAT_TIME;
	}
	else if ( handler.m_Input->m_Buttons & Inp::Data::mD_LEFT )
	{
		s_auto_repeat_delay--;
		if ( s_auto_repeat_delay <= 0 )
		{
			obj.m_commands.SetMask( mDECREMENT_FRAME );
			s_auto_repeat_delay = vAUTO_REPEAT_SPEED;
		}
	}
	else if ( handler.m_Input->m_Buttons & Inp::Data::mD_RIGHT )
	{
		s_auto_repeat_delay--;
		if ( s_auto_repeat_delay <= 0 )
		{
			obj.m_commands.SetMask( mINCREMENT_FRAME );
			s_auto_repeat_delay = vAUTO_REPEAT_SPEED;
		}
	}
	else
	{
		s_auto_repeat_delay = vAUTO_REPEAT_TIME;
	}
	
	// the viewer uses the global var "viewer_controls_enabled"
	// to decide whether certain viewer controls should be
	// active while in the regular viewer/light viewer.
	// rather than adding a new global var to distinguish
	// between the model viewer/light viewer, i've decided
	// to just have the viewer object handle those viewer
	// controls here
	if( handler.m_Input->m_Makes & Inp::Data::mD_L3 )
	{
		obj.m_commands.SetMask( mTOGGLE_ROTATE_MODE );
	}
	
	if ( handler.m_Input->m_Buttons & Inp::Data::mD_R1 )
	{
		if( !( handler.m_Input->m_Buttons & Inp::Data::mD_R2 ))
		{
			obj.m_commands.SetMask( mSTRAFE_UP );
		}
	}
	else if ( handler.m_Input->m_Buttons & Inp::Data::mD_R2 )
	{
		obj.m_commands.SetMask( mSTRAFE_DOWN );
	}
	
	if ( handler.m_Input->m_Makes & Inp::Data::mD_L1 )
	{
		obj.m_commands.SetMask( mINCREASE_SPEED );
	}
	else if ( handler.m_Input->m_Makes & Inp::Data::mD_L2 )
	{
		obj.m_commands.SetMask( mDECREASE_SPEED );
	}
	
	if ( !GetAnimationComponentFromObject( &obj ) )
	{
		if ( handler.m_Input->m_Buttons & Inp::Data::mD_CIRCLE )
		{
			obj.m_commands.SetMask( mROLL_LEFT );
		}
		else if ( handler.m_Input->m_Buttons & Inp::Data::mD_SQUARE )
		{
			obj.m_commands.SetMask( mROLL_RIGHT );
		}
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CViewerObject::DoGameLogic()
{
	if ( m_commands.TestMask( mTOGGLE_PANEL )	)
	{
		m_showPanel = !m_showPanel;
		
		Script::RunScript( "kill_viewer_object_panel" );
	}

	if ( m_commands.TestMask( mINCREASE_SPEED ) )
	{
		ChangeAnimSpeed( m_animSpeed + vSPEED_INCREMENT_AMOUNT );
	}

	if ( m_commands.TestMask( mDECREASE_SPEED ) )
	{
		ChangeAnimSpeed( m_animSpeed - vSPEED_INCREMENT_AMOUNT );
	}
										
	if ( m_commands.TestMask( mINCREMENT_FRAME ) )
	{
		IncrementFrame( true );
	}

	if ( m_commands.TestMask( mDECREMENT_FRAME ) )
	{
		IncrementFrame( false );
	}

	// yuck:  the following creates a cyclic dependency
	// on viewer...  should get rid of this later.
	if ( m_commands.TestMask( mTOGGLE_ROTATE_MODE ) )
	{
		Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();
		if ( pViewer )
		{
			pViewer->SetCommand( Mdl::CViewer::mTOGGLE_ROTATE_MODE );
		}
	}

	if ( m_commands.TestMask( mSTRAFE_UP ) )
	{
		Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();
		if ( pViewer )
		{
			pViewer->SetCommand( Mdl::CViewer::mSTRAFE_UP );
		}
	}
	
	if ( m_commands.TestMask( mSTRAFE_DOWN ) )
	{
		Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();
		if ( pViewer )
		{
			pViewer->SetCommand( Mdl::CViewer::mSTRAFE_DOWN );
		}
	}
	
	if ( m_commands.TestMask( mROLL_LEFT ) )
	{
		Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();
		if ( pViewer )
		{
			pViewer->SetCommand( Mdl::CViewer::mROLL_LEFT );
		}
	}
	
	if ( m_commands.TestMask( mROLL_RIGHT ) )
	{
		Mdl::CViewer* pViewer = Mdl::CViewer::sGetViewer();
		if ( pViewer )
		{
			pViewer->SetCommand( Mdl::CViewer::mROLL_RIGHT );
		}
	}

	Update();
	
	// GJ:  clear the frame's data, now that we're done with it
	// (this used to be done at the beginning of the loop)
	m_commands.ClearAll();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj
