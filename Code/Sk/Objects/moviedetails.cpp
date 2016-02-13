//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       moviedetails.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  06/17/2002
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/objects/moviedetails.h>

#include <gel/components/modelcomponent.h>
								
#include <gfx/bonedanim.h>
#include <gfx/camera.h>
#include <gfx/nxviewman.h>

#include <gel/assman/assman.h>
#include <gel/objman.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/symboltable.h>

#include <sk/modules/skate/skate.h>
#include <gel/object/compositeobject.h>

#include <gfx/skeleton.h>

#include <sk/modules/FrontEnd/FrontEnd.h>

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/

namespace Obj
{

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

/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMovieDetails::CMovieDetails() : Lst::Node<CMovieDetails>( this )
{
	mp_exitParams = NULL;

	// defaults to not skippable
	m_skippable = false;
	m_holdOnLastFrame = false;

	m_allowPause = false;

	mp_camera = new Gfx::Camera;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMovieDetails::~CMovieDetails()
{
	if ( mp_exitParams )
	{
		delete mp_exitParams;
	}
   
	if ( mp_camera )
	{
		if (!Nx::CViewportManager::sMarkCameraForDeletion(mp_camera))
		{
			delete mp_camera;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMovieDetails::SetParams( Script::CStruct* pParams )
{
	Dbg_Assert( pParams );

	Dbg_MsgAssert( m_exitScript == 0, ( "exit script was already assigned to %s", Script::FindChecksumName( m_name ) ) );

	if ( !mp_exitParams )
	{
		mp_exitParams = new Script::CStruct;
	}

	mp_exitParams->Clear();

	if ( pParams->GetChecksum( CRCD(0x1a005041,"exitScript"), &m_exitScript, Script::NO_ASSERT ) )
	{
		Script::CStruct* pSubParams = NULL;
		if ( pParams->GetStructure( CRCD(0x894fd988,"exitParams"), &pSubParams, Script::NO_ASSERT ) )
		{
			mp_exitParams->AppendStructure( pSubParams );
		}
	}

	int skippable = 0;
	pParams->GetInteger( CRCD(0x8dde72c0,"skippable"), &skippable, Script::NO_ASSERT );
	m_skippable = skippable;

	// if we want the movie to continue to hold on the last frame (i.e. don't go on to the next anim)
	m_holdOnLastFrame = pParams->ContainsFlag( CRCD(0xeb300c29,"play_hold") );

	int allow_pause = 0;
	pParams->GetInteger( CRCD(0xb8ca12c5,"allow_pause"), &allow_pause, Script::NO_ASSERT );
	m_allowPause = ( allow_pause != 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* CMovieDetails::GetParams()
{
	Script::CStruct* pReturnParams = new Script::CStruct();
	pReturnParams->AddInteger( CRCD(0x8dde72c0,"skippable"), m_skippable );
	pReturnParams->AddInteger( CRCD(0xb8ca12c5,"allow_pause"), m_allowPause );
	return pReturnParams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMovieDetails::Cleanup()
{
	// run the exit script

//	if ( m_exitScript != 0 )
	{
//		Dbg_Message( "Running exit script (%s)\n", Script::FindChecksumName(m_exitScript) );

//		Script::RunScript( m_exitScript, mp_exitParams, NULL );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovieDetails::Abort( bool only_if_skippable )
{
	if ( !only_if_skippable )
	{
		m_aborted = true;
	}
	if ( m_skippable )
	{
		m_aborted = true;
	}

	return m_aborted;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovieDetails::SetSkippable( bool skippable )
{
	m_skippable = skippable;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovieDetails::SetPauseMode( bool pause_mode )
{
	m_shouldPause = pause_mode;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovieDetails::SetName( uint32 name )
{
	m_name = name;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CMovieDetails::GetName()
{
	return m_name;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovieDetails::CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CObjectAnimDetails::CObjectAnimDetails() : CMovieDetails()
{
//	Gfx::CBonedAnimFrameData*	mp_frameData;
	
	mp_frameData = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CObjectAnimDetails::~CObjectAnimDetails()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CObjectAnimDetails::InitFromStructure( Script::CStruct* pParams )
{
	this->SetParams( pParams );

	if ( !pParams->ContainsFlag( CRCD(0xee721991,"virtual_cam") ) )
	{
		Ass::CAssMan* ass_man = Ass::CAssMan::Instance();
		Gfx::CBonedAnimFrameData* pData = (Gfx::CBonedAnimFrameData*)ass_man->GetAsset( this->GetName(), false );
		if ( !pData )
		{
			Dbg_Message( "Warning: couldn't find movie with name %s", Script::FindChecksumName( this->GetName() ) );
			return false;
		}
		
		this->set_frame_data( pData, 
							pParams->ContainsFlag( CRCD(0x5ea0e211,"loop") ) ? Gfx::LOOPING_CYCLE : Gfx::LOOPING_HOLD,
							pParams->ContainsFlag( CRCD(0xf8cfd515,"backwards") ) );
	}
	else
	{
		int numFrames = 0;
		pParams->GetInteger( CRCD(0x019176c5,"frames"), &numFrames, Script::ASSERT );
		this->set_movie_length( numFrames / 60.0f );
	}

	// by default, pause movie when game is paused 
	this->SetPauseMode( true );
	   
	// now that the new movie is about to start,
	// reset all of its custom keys
	this->ResetCustomKeys();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CObjectAnimDetails::Update()
{
	update_moving_objects();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CObjectAnimDetails::set_frame_data( Gfx::CBonedAnimFrameData* pFrameData, Gfx::EAnimLoopingType loopingType, bool reverse )
{
	Dbg_Assert( pFrameData );

	mp_frameData = pFrameData;

	if ( !reverse )
	{
		m_animController.PlaySequence( 0, 0.0f, pFrameData->GetDuration(), loopingType, 0.0f, 1.0f );
	}
	else
	{
// 26Mar03 JCB - When playing backwards, start on the final frame, not beyond the end of the anim.
//
//		m_animController.PlaySequence( 0, pFrameData->GetDuration(), 0.0f, loopingType, 0.0f, 1.0f );
		m_animController.PlaySequence( 0, pFrameData->GetDuration() - (1.0f/60.0f), 0.0f, loopingType, 0.0f, 1.0f );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CObjectAnimDetails::set_movie_length( float duration )
{
	mp_frameData = NULL;

	m_animController.PlaySequence( 0, 0.0f, duration, Gfx::LOOPING_HOLD, 0.0f, 1.0f );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CObjectAnimDetails::ResetCustomKeys()
{
	// eventually, this will go in the decompressed version of the
	// frame data, rather than the frame data itself...
	if ( mp_frameData )
	{
		mp_frameData->ResetCustomKeys();
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CObjectAnimDetails::process_custom_keys(float startTime, float endTime, bool end_time_inclusive)
{
	// This code currently assumes that you're not going to pause the
	// camera.  If so, then you run the risk of calling these user keys
	// more than once...

	// TODO:  Handle this gracefully!

	if ( mp_frameData )
	{
		Dbg_Assert( mp_camera );

		// It depends on the direction also...
		mp_frameData->ProcessCustomKeys( startTime, endTime, mp_camera, end_time_inclusive );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CObjectAnimDetails::update_moving_objects()
{
	float oldTime = m_animController.GetCurrentAnimTime();

	m_animController.Update();
	
	// Print out some useful information...
	//if ( Script::GetInt( CRCD(0x2f510f45,"moviecam_debug"), false ) )
	{
		//Dbg_Message( "Moving object anim %s @ %f seconds (%d frames)", Script::FindChecksumName(m_name), m_animController.GetCurrentAnimTime(), (int)(m_animController.GetCurrentAnimTime() * 60.0f) );
	}

	// process associated keys since last frame
	process_custom_keys( oldTime, m_animController.GetCurrentAnimTime(), false );

	Mth::Quat* pQuat = new Mth::Quat[mp_frameData->GetNumBones()];
	Mth::Vector* pVector = new Mth::Vector[mp_frameData->GetNumBones()];
	mp_frameData->GetInterpolatedCameraFrames( pQuat, pVector, m_animController.GetCurrentAnimTime() );

	// get the position of each bone
	for ( int i = 0; i < mp_frameData->GetNumBones(); i++ )
	{
		// grab the frame from the animation controller
		Dbg_Assert( mp_frameData );
		
		uint32 objName;
		objName = mp_frameData->GetBoneName( i );

		Obj::CObject* pObject = Obj::ResolveToObject( objName );
		Dbg_MsgAssert( pObject, ( "Can't find lookup target %s", Script::FindChecksumName( objName ) ) );
		if ( pObject )
		{
			Obj::CCompositeObject* pCompositeObject = (Obj::CCompositeObject*)pObject;

			// update the object's orientation
			Mth::Matrix theMatrix;
			
			Mth::Vector nullTrans;
			nullTrans.Set(0.0f,0.0f,0.0f,1.0f);		// Mick: added separate initialization
			Mth::QuatVecToMatrix( pQuat + i, &nullTrans, &theMatrix );
			
			pCompositeObject->SetMatrix( theMatrix );
			pCompositeObject->SetDisplayMatrix( theMatrix );

			// update the object's position
			pCompositeObject->m_pos = pVector[i];

			if ( Script::GetInt( CRCD(0x2f510f45,"moviecam_debug") ) )
			{
				printf( "Object %s @ time %f:  ", Script::FindChecksumName(objName), m_animController.GetCurrentAnimTime() );
				pCompositeObject->m_pos.PrintContents();
				pCompositeObject->m_matrix.PrintContents();
			}
		
			// if we want to change the position/rotation,
			// then we also need to update the model component
			CModelComponent* pModelComponent = GetModelComponentFromObject( pCompositeObject );
			if ( pModelComponent )
			{
				pModelComponent->Update();
			}
		}
		else
		{
			printf( "object animation:  couldn't find object %s\n", Script::FindChecksumName(objName) );
		}
	}
	
	delete[] pQuat;
	delete[] pVector;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CObjectAnimDetails::IsComplete()
{
	if ( m_aborted )
	{
		return true;
	}

	if ( m_holdOnLastFrame )
	{
		return false;
	}

	return m_animController.IsAnimComplete();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CObjectAnimDetails::IsHeld()
{
	/* Added by Ken. Used to determine when camera anims used in	  */
	/* the skate shop front-end have got to their last frame, so that */
	/* scripts can trigger things to happen such as the video menu    */
	/* making the video cassettes pan out to the foreground.          */
	
	return ( m_holdOnLastFrame && m_animController.IsAnimComplete() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CObjectAnimDetails::OverridesCamera()
{
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterCamDetails::CSkaterCamDetails() : CMovieDetails()
{
	mp_frameData = NULL;
	
	// defaults to no target
	m_hasTarget = false;
	m_targetID = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterCamDetails::~CSkaterCamDetails()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCamDetails::InitFromStructure( Script::CStruct* pParams )
{
	this->SetParams( pParams );

	if ( !pParams->ContainsFlag( CRCD(0xee721991,"virtual_cam") ) )
	{
		Ass::CAssMan* ass_man = Ass::CAssMan::Instance();
		Gfx::CBonedAnimFrameData* pData = (Gfx::CBonedAnimFrameData*)ass_man->GetAsset( this->GetName(), false );
		if ( !pData )
		{
			Dbg_Message( "Warning: couldn't find movie with name %s", Script::FindChecksumName( this->GetName() ) );
			return false;
		}
		
		this->set_frame_data( pData, 
							pParams->ContainsFlag( CRCD(0x5ea0e211,"loop") ) ? Gfx::LOOPING_CYCLE : Gfx::LOOPING_HOLD,
							pParams->ContainsFlag( CRCD(0xf8cfd515,"backwards") ) );
	}
	else
	{
		int numFrames = 0;
		pParams->GetInteger( CRCD(0x019176c5,"frames"), &numFrames, Script::ASSERT );
		this->set_movie_length( numFrames / 60.0f );
	}

	// by default, pause movie when game is paused 
	this->SetPauseMode( true );
	   
	// look for target parameters
	this->set_target_params( pParams );
	
	// now that the new movie is about to start,
	// reset all of its custom keys
	this->ResetCustomKeys();

	update_camera();

	// process initial keys, end time inclusive...
	// this to fix an FOV glitch on the first frame
	process_custom_keys( m_animController.GetCurrentAnimTime(), m_animController.GetCurrentAnimTime(), true );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCamDetails::Update()
{
	update_camera_time();

	update_camera();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCamDetails::set_target_params( Script::CStruct* pParams )
{
	Dbg_Assert( pParams );

	m_hasTarget = pParams->GetChecksum( CRCD(0x531e4d28,"targetID"), &m_targetID, Script::NO_ASSERT );
	
	if ( m_hasTarget )
	{
		// target offset, if any
		m_targetOffset.Set(0.0f,0.0f,0.0f);
		pParams->GetVector( CRCD(0x906642bf,"targetOffset"), &m_targetOffset, Script::NO_ASSERT );
		m_hasTargetOffset = ( m_targetOffset[X] != 0.0f || m_targetOffset[Y] != 0.0f || m_targetOffset[Z] != 0.0f );

		// position offset, if any
		m_positionOffset.Set(0.0f,0.0f,0.0f);
		m_hasPositionOffset = pParams->GetVector( CRCD(0xa00d3d6e,"positionOffset"), &m_positionOffset, Script::NO_ASSERT );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCamDetails::clear_target_params()
{
	m_hasTarget = false;
	m_targetID = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCamDetails::set_frame_data( Gfx::CBonedAnimFrameData* pFrameData, Gfx::EAnimLoopingType loopingType, bool reverse )
{
	Dbg_Assert( pFrameData );

	mp_frameData = pFrameData;

	if ( !reverse )
	{
		m_animController.PlaySequence( 0, 0.0f, pFrameData->GetDuration(), loopingType, 0.0f, 1.0f );
	}
	else
	{
// 26Mar03 JCB - When playing backwards, start on the final frame, not beyond the end of the anim.
//
//		m_animController.PlaySequence( 0, pFrameData->GetDuration(), 0.0f, loopingType, 0.0f, 1.0f );
		m_animController.PlaySequence( 0, pFrameData->GetDuration() - (1.0f/60.0f), 0.0f, loopingType, 0.0f, 1.0f );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCamDetails::set_movie_length( float duration )
{
	mp_frameData = NULL;

	m_animController.PlaySequence( 0, 0.0f, duration, Gfx::LOOPING_HOLD, 0.0f, 1.0f );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCamDetails::ResetCustomKeys()
{
	// eventually, this will go in the decompressed version of the
	// frame data, rather than the frame data itself...
	if ( mp_frameData )
	{
		mp_frameData->ResetCustomKeys();
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCamDetails::get_current_frame(Mth::Quat* pQuat, Mth::Vector* pTrans)
{
	Dbg_Assert( pQuat );
	Dbg_Assert( pTrans );

	// grab the frame from the animation controller
	Dbg_Assert( mp_frameData );
	Dbg_Assert( mp_frameData->GetNumBones() == 1 );
	mp_frameData->GetInterpolatedCameraFrames( pQuat, pTrans, m_animController.GetCurrentAnimTime() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCamDetails::process_custom_keys(float startTime, float endTime, bool end_time_inclusive)
{
	// This code currently assumes that you're not going to pause the
	// camera.  If so, then you run the risk of calling these user keys
	// more than once...

	// TODO:  Handle this gracefully!

	if ( mp_frameData )
	{
		Dbg_Assert( mp_camera );

		// It depends on the direction also...
		mp_frameData->ProcessCustomKeys( startTime, endTime, mp_camera, end_time_inclusive );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCamDetails::update_camera_time()
{
	float oldTime = m_animController.GetCurrentAnimTime();

	m_animController.Update();

	// Not sure if this is still needed for the sound code
	// Maybe we can add the functionality automatically to the Gfx::Camera?
	mp_camera->StoreOldPos();
	
	// process associated keys since last frame
	process_custom_keys( oldTime, m_animController.GetCurrentAnimTime(), false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCamDetails::update_camera()
{
	Dbg_Assert( mp_camera );

	Mth::Vector& camPos = mp_camera->GetPos();
	Mth::Matrix& camMatrix = mp_camera->GetMatrix();
	
	Mdl::FrontEnd* pFront = Mdl::FrontEnd::Instance();

	for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		if ( pFront->GetInputHandler( i )->m_Input->m_Makes & Inp::Data::mD_X )
		{
			// abort if skippable
			Abort( true );
		}
	}
	
	if ( mp_frameData )
	{
		Mth::Quat theQuat;
		Mth::Vector theTrans;

		get_current_frame( &theQuat, &theTrans );
		
		// update the camera position
		camPos = theTrans;

		// update the camera orientation
		Mth::Vector nullTrans;
		nullTrans.Set(0.0f,0.0f,0.0f,1.0f);		// Mick: added separate initialization
		Mth::QuatVecToMatrix( &theQuat, &nullTrans, &camMatrix );
	}
	else
	{
		// clear out the cam matrix to the identity
		// if there's no associated animation.
		// the assumption is that the following m_hasTarget
		// code block will overrride it with the correct data...
		camPos = Mth::Vector( 0.0f, 0.0f, 0.0f );
		camMatrix.Ident();
		
		// if there's no frame data, then we should
		// reset the camera to some default FOV,
		// or else it will use the last FOV used
		float fov_in_degrees = Script::GetFloat( CRCD(0x99529205, "camera_fov") );
		mp_camera->SetHFOV(fov_in_degrees);
	}
	
	if ( m_hasTarget )
	{
		if ( m_targetID != CRCD(0xc588eebc,"world") )
		{
			// calculate the orientation from the target
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			Obj::CGeneralManager* pObjectManager = skate_mod->GetObjectManager();
			Dbg_Assert( pObjectManager );
			Obj::CObject* pObject = pObjectManager->GetObjectByID( m_targetID );
			Dbg_MsgAssert( pObject, ( "Can't find lookup target %s", Script::FindChecksumName(m_targetID) ) );
			if ( pObject )
			{
				//Dbg_MsgAssert( static_cast<Obj::CCompositeObject*>( pObject ), ( "This function only works on moving objects." ) ); 
				Obj::CCompositeObject* pCompositeObject = (Obj::CCompositeObject*)pObject;
	
				// override it with the moving object's position
				Mth::Vector pos = pCompositeObject->GetPos();
				
				// patch for positions that don't have 1.0 in the final column...
				// not sure why this would be the case, but it seems to happen
				// with the skater...  (possibly a result of skater's mp_physics
				// disappearing)
				pos[W] = 1.0f;
	
				// not properly working yet...
				if ( m_hasTargetOffset )
				{    
					Mth::Matrix mat = pCompositeObject->GetMatrix();
					mat[Mth::POS] = pos;				                                                         
					mat.TranslateLocal( m_targetOffset );
					pos = mat[Mth::POS];
				}
	
				if ( m_hasPositionOffset )
				{
					camPos = pos;
	
					Mth::Matrix mat = pCompositeObject->GetMatrix();
					mat[Mth::POS] = pos;
					mat.TranslateLocal( m_positionOffset );
					camPos = mat[Mth::POS];
				}
	
				// set rotation from the camera's position (always up)
				Mth::Vector atVec = camPos - pos;
				atVec.Normalize();
				
				camMatrix.Ident();
				camMatrix[Mth::AT]		= atVec;
				camMatrix[Mth::RIGHT]	= Mth::CrossProduct( camMatrix[Mth::UP], camMatrix[Mth::AT] );
				camMatrix[Mth::RIGHT].Normalize();
				camMatrix[Mth::UP]		= Mth::CrossProduct( camMatrix[Mth::AT], camMatrix[Mth::RIGHT] );
			}
		}
		else
		{
			// World coordinates are a lot simpler
			Mth::Vector pos = m_targetOffset;
			camPos = m_positionOffset;
			Mth::Vector atVec = camPos - pos;
			atVec[W] = 1.0f;
			atVec.Normalize();
			camMatrix.Ident();
			camMatrix[Mth::AT]		= atVec;
			camMatrix[Mth::RIGHT]	= Mth::CrossProduct( camMatrix[Mth::UP], camMatrix[Mth::AT] );
			camMatrix[Mth::RIGHT].Normalize();
			camMatrix[Mth::UP]		= Mth::CrossProduct( camMatrix[Mth::AT], camMatrix[Mth::RIGHT] );
		}		
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCamDetails::CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( checksum )
	{
		case 0xa8d5a188:		// SetTargetObject
			{
				Dbg_Message( "*** Calling SetTargetObject" );
				
				set_target_params( pParams );
				
				return true;
			}
		case 0xd7241a6c:		// ClearTargetObject
			{
				Dbg_Message( "*** Calling ClearTargetObject" );
				
				clear_target_params();
				
				return true;
			}
			break;
	}

	return CMovieDetails::CallMemberFunction( checksum, pParams, pScript );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCamDetails::IsComplete()
{
	if ( m_aborted )
	{
		return true;
	}

	if ( m_holdOnLastFrame )
	{
		return false;
	}

	return m_animController.IsAnimComplete();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCamDetails::IsHeld()
{
	/* Added by Ken. Used to determine when camera anims used in	  */
	/* the skate shop front-end have got to their last frame, so that */
	/* scripts can trigger things to happen such as the video menu    */
	/* making the video cassettes pan out to the foreground.          */
	
	return ( m_holdOnLastFrame && m_animController.IsAnimComplete() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCamDetails::OverridesCamera()
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj
