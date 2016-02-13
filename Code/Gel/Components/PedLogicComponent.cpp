//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       PedLogicComponent.cpp
//* OWNER:          Brad Bulkley
//* CREATION DATE:  1/9/03
//****************************************************************************

// start autoduck documentation
// @DOC pedlogiccomponent
// @module pedlogiccomponent | None
// @subindex Scripting Database
// @index script | pedlogiccomponent

#include <sk/components/skaterloopingsoundcomponent.h>
#include <sk/components/skatersoundcomponent.h>

#include <gel/components/pedlogiccomponent.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/motioncomponent.h>
#include <gel/components/avoidcomponent.h>

#include <gel/environment/terrain.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/symboltable.h>

#include <sk/objects/skater.h>
#include <sk/objects/pathob.h>
#include <sk/objects/pathman.h>
#include <sk/objects/skaterflags.h>
#include <sk/engine/feeler.h>

#include <sk/modules/skate/skate.h>

#include <sk/scripting/nodearray.h>

#include <gfx/debuggfx.h>

namespace Obj
{

#define	vDEFAULT_RANGE (10.0f)
#define vACTION_WEIGHT_RESOLUTION (1000)

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CPedLogicComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CPedLogicComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPedLogicComponent::CPedLogicComponent() : CBaseComponent()
{
	SetType( CRC_PEDLOGIC );

	// reset whiskers
	for ( int i = 0; i < vNUM_WHISKERS; i++ )
		m_whiskers[i] = 0.0;

	ResetBias();
	m_turn_frames = 0;
	m_max_turn_frames = 0; // Script::GetInteger( CRCD( 0x793535f5, "ped_turn_frames" ), Script::ASSERT );
	mp_path_object_tracker = NULL;

	m_col_dist_above = Script::GetFloat( CRCD( 0x379d93d2, "ped_skater_stick_dist_above" ), Script::ASSERT );
	m_col_dist_below = Script::GetFloat( CRCD( 0x8715b7b2, "ped_skater_stick_dist_below" ), Script::ASSERT );

	m_flags = 0;

	m_original_max_vel = 0.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPedLogicComponent::~CPedLogicComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::InitFromStructure( Script::CStruct* pParams )
{
	// ** Add code to parse the structure, and initialize the component
	// m_state = CRCD( 0x23db4aea, "Idle" );s

	pParams->GetInteger( CRCD( 0xe50d6573, "nodeIndex" ), &m_node_from, Script::ASSERT );
	// printf("m_node_from = %i\n", m_node_from);
	SkateScript::GetPosition( m_node_from, &m_wp_from );
	m_normal_lerp = 0.0f;
	m_display_normal = m_last_display_normal = m_current_normal = GetObject()->m_matrix[Y];

	pParams->GetFloat( CRCD( 0x367c4a1b, "StickToGroundDistAbove" ), &m_col_dist_above );
	pParams->GetFloat( CRCD( 0x86f46e7b, "StickToGroundDistBelow" ), &m_col_dist_below );

	m_current_display_matrix = GetObject()->GetDisplayMatrix();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::Update()
{	
	m_time = Tmr::FrameLength();
	
	// BB - Switch on the state and call the appropriate update function.
	// This step could be removed by having the logic for each state in 
	// this function, but this will make it easier to split the component 
	// if need be.  Plus it just looks cleaner.
	switch ( m_state )
	{
		case CRCC( 0x69b05e2c, "generic" ):
			GenericPedUpdate();
			break;
		case CRCC( 0xa85af587, "generic_skater" ):
			GenericSkaterUpdate();
			break;
		case CRCC(0x6367167b,"lip_trick"):
			UpdateLipDisplayMatrix();
			break;
		case CRCC( 0x82b45a67, "generic_standing" ):
			// this is currently handled in script
			break;
	default:
		// Dbg_MsgAssert( 0, ( "PedLogicComponent has unknown state %s", Script::FindChecksumName( m_state ) ) );
		break;
	}
	// Gfx::AddDebugArrow( GetObject()->GetDisplayMatrix()[X] + GetObject()->m_pos, GetObject()->GetDisplayMatrix()[X] * 48 + GetObject()->m_pos, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 1 );
	// Gfx::AddDebugArrow( GetObject()->GetDisplayMatrix()[Y] + GetObject()->m_pos, GetObject()->GetDisplayMatrix()[Y] * 48 + GetObject()->m_pos, MAKE_RGB( 0, 128, 0 ), MAKE_RGB( 0, 0, 128 ), 1 );
	// Gfx::AddDebugArrow( GetObject()->GetDisplayMatrix()[Z] + GetObject()->m_pos, GetObject()->GetDisplayMatrix()[Z] * 48 + GetObject()->m_pos, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 0, 0, 128 ), 1 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CPedLogicComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | Ped_SetLogicState | set the current state of the ped
		// @uparm name | state - eg, generic, generic_skater, etc.
		case CRCC( 0x3685b765, "Ped_SetLogicState" ):
			pParams->GetChecksum( NONAME, &m_state, Script::ASSERT );
			break;
		// @script | Ped_InitPath | initializes the ped on his linked path and starts movement
		case CRCC( 0xe9906815, "Ped_InitPath" ):
		{
			Script::CStruct* pNodeData = SkateScript::GetNode( m_node_from );
			// m_node_to = SkateScript::GetLink( pNodeData, Mth::Rnd( SkateScript::GetNumLinks( pNodeData ) ) );
			
			#ifdef __NOPT_ASSERT__
			int numLinks = SkateScript::GetNumLinks( m_node_from );
			Dbg_MsgAssert( numLinks, ( "Ped_InitPath called on %s, but path node %i has no links!", Script::FindChecksumName( GetObject()->GetID() ), m_node_from ) );
			#endif

			m_node_to = SkateScript::GetLink( pNodeData, 0 );
			SkateScript::GetPosition( m_node_to, &m_wp_to );
			SelectNextWaypoint();
			m_flags |= PEDLOGIC_MOVING_ON_PATH;
			mp_path_object_tracker = Obj::CPathMan::Instance()->TrackPed( GetObject(), m_node_to );
			
			Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
			Dbg_Assert( pMotionComp );
			pMotionComp->m_movingobj_status |= MOVINGOBJ_STATUS_ON_PATH;

			// turn the ped to face the right direction
			Mth::Vector heading = ( m_wp_to - m_wp_from ).Normalize();
			Mth::Matrix display_matrix = GetObject()->GetDisplayMatrix();
			display_matrix[Z] = heading;
			display_matrix.OrthoNormalizeAbout( Z );
			GetObject()->SetDisplayMatrix( display_matrix );
			m_current_display_matrix = display_matrix;

			Mth::Vector current_pos = GetObject()->m_pos;
			for ( int i = 0; i < vPOS_HISTORY_SIZE; i++ )
				m_pos_history[i] = current_pos;

			break;
		}
		// @script | Ped_StartMoving | start moving on path
		case CRCC( 0xd711542, "Ped_StartMoving" ):
		{
			if ( m_flags & PEDLOGIC_STOPPED )
			{
				Script::CScript* pScript = Script::SpawnScript( CRCD( 0xbed339d4, "ped_skater_start_moving" ) );
				pScript->mpObject = GetObject();
				m_flags &= ~PEDLOGIC_STOPPED;
			}
			m_flags |= PEDLOGIC_MOVING_ON_PATH;
			break;
		}
		// @script | Ped_StopMoving | stop moving on path
		case CRCC( 0xcc85adfe, "Ped_StopMoving" ):
			m_flags &= ~PEDLOGIC_MOVING_ON_PATH;
			break;
		// @script | Ped_IsMoving | returns true if the ped is moving on the path
		case CRCC(0x68d298b0,"Ped_IsMoving"):
			return m_flags & PEDLOGIC_MOVING_ON_PATH ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;
		// @script | Ped_SetStickToGroundDist | Set the distances the ped uses to check for ground
		// when sticking.  This check is not performed straight up and down, but at whatever
		// angle the ped is currently standing (his Y vector).
		// @parmopt float | StickToGroundDistAbove | | distance to check above the ped
		// @parmopt float | StickToGroundDistBelow | | distance to check below the ped
		case CRCC( 0x2cc086dd, "Ped_SetStickToGroundDist" ):
			pParams->GetFloat( "distAbove", &m_col_dist_above );
			pParams->GetFloat( "distBelow", &m_col_dist_below );
			printf("StickToGround distances set to %f (above), %f (below)\n", m_col_dist_above, m_col_dist_below);
			break;
		// @script | Ped_SetIsGrinding | used for manually setting the grind state on and off
		// @uparm 0 | 0 for off, anything else for on
		case CRCC( 0xc643e8f5, "Ped_SetIsGrinding" ):
		{
			int is_grinding = 0;
			pParams->GetInteger( NONAME, &is_grinding, Script::ASSERT );
			if ( is_grinding != 0 )
				m_flags |= PEDLOGIC_GRINDING;
			else
				m_flags &= ~PEDLOGIC_GRINDING;
			break;
		}
		// @script | Ped_GetCurrentVelocity | returns variable named velocity...
		// @flag max | get the maximum velocity, rather than the currnt velocity.
		// This is useful in case the ped is currently accelerating or decelerating 
		// to achieve maximum velocity.
		case CRCC( 0x13713760, "Ped_GetCurrentVelocity" ):
		{
			Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
			Dbg_Assert( pMotionComp );
			pScript->GetParams()->AddFloat( CRCD( 0x41272956, "velocity" ), pMotionComp->m_vel_z );
			break;
		}
		// @script | Ped_StoreMaxVelocity | stores the current max velocity, so you can
		// change it and restore it later
		case CRCC( 0xf5627596, "Ped_StoreMaxVelocity" ):
		{
			Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
			Dbg_Assert( pMotionComp );
			m_original_max_vel = pMotionComp->m_max_vel;
			break;
		}
		// @script | Ped_GetOriginalMaxVelocity | returns original_max_velocity stored when 
		// Ped_StoreMaxVelocity was called
		case CRCC( 0xe575efa9, "Ped_GetOriginalMaxVelocity" ):
		{
			pScript->GetParams()->AddFloat( "original_max_velocity", m_original_max_vel );
			break;
		}
		// @script | Ped_Bail |
		case CRCC( 0xee24a410, "Ped_Bail" ):
			Bail();
			break;
		case CRCC( 0xc076d74, "Ped_SetIsBailing" ):
		{
			int v;
			pParams->GetInteger( NONAME, &v, Script::ASSERT );
			if ( pParams->ContainsFlag( CRCD( 0x255ed86f, "grind" ) ) )
			{
				if ( v == 0 )
					m_flags &= ~PEDLOGIC_GRIND_BAILING;
				else
					m_flags |= PEDLOGIC_GRIND_BAILING;				
			}
			else
			{
				if ( v == 0 )
					m_flags &= ~PEDLOGIC_BAILING;
				else
					m_flags |= PEDLOGIC_BAILING;
			}
			break;
		}
		case CRCC( 0xda7be9e2, "Ped_InitVertTrick" ):
		{
			// get the height and gravity
			float height;
			pParams->GetFloat( CRCD( 0xab21af0, "height" ), &height, Script::ASSERT );
			float gravity;
			pParams->GetFloat( CRCD( 0xa5e2da58, "gravity" ), &gravity, Script::ASSERT );
			Dbg_MsgAssert( gravity < 0.0f, ( "Ped_InitVertTrick given positive number for gravity" ) );

			Mth::Vector pt = m_wp_to - GetObject()->m_pos;
			// adjust height to compensate for difference between ped height
			// and height of waypoint
			// height += pt[Y];
			// pt[Y] = 0.0f;

			// figure the time - the gravity is always expressed as negative 
			// acceleration, but since we're figuring the time it will take to
			// fall from the top at velocity=0, we use positive
			// Multiply by 2 to get time up and down
			// d = vi*t + (a*t^2)/2
			float time = 2 * sqrtf( 2 * -height / gravity );

			// compute the inital y velocity, which equals the final y velocity when
			// falling back down (vf^2 = vi^2 + 2ad)
			float vi = sqrtf( 2 * gravity * -height );
			Dbg_Assert( vi > 0 );

			// find the horizontal distance we need to travel
			float distance = pt.Length() / 2;
			// bump it up a bit so we don't miss landing
			distance *= Script::GetFloat( CRCD( 0x2b5bedba, "ped_skater_vert_jump_slop_distance" ), Script::ASSERT );

			// horizontal velocity
			Mth::Vector vh = ( distance / time ) * pt.Normalize();

			// vertical velocity
			Mth::Vector vy = vi * Mth::Vector(0, 1, 0);

			// total speed and heading
			// float jump_speed = ( ( vh + vy ) * time ).Length();
			float jump_speed = ( vh + vy ).Length();
			Mth::Vector heading = ( vh + vy ).Normalize();

			pScript->GetParams()->AddVector( CRCD( 0xfd4bc03e, "heading" ), heading );
			pScript->GetParams()->AddFloat( CRCD( 0x1c4c5690, "jumpSpeed" ), jump_speed );
			pScript->GetParams()->AddFloat( CRCD( 0x66ced87b, "jumpTime" ), time );
			break;
		}
		// @script | Ped_HitWaypoint | force the ped to register a hit on the
		// to waypoint, regardless of his position.  Don't call this unless
		// you know exactly why you're doing it.
		case CRCC( 0x1affffb3, "Ped_HitWaypoint" ):
			HitWaypoint();
			break;
		// @script | Ped_SetIsSkater | 
		// @uparm 1 | 0 if not skater, anything else for skater
		case CRCC( 0x5c794c57, "Ped_SetIsSkater" ):
		{
			int is_skater;
			pParams->GetInteger( NONAME, &is_skater, Script::ASSERT );
			if ( is_skater == 0 )
				m_flags &= ~PEDLOGIC_IS_SKATER;
			else
				m_flags |= PEDLOGIC_IS_SKATER;
			break;
		}
		// @script | Ped_NearVertNode | true if the next node or the to node
		// is vert
		case CRCC( 0x5bb23915, "Ped_NearVertNode" ):
			return ( ( ( m_flags & PEDLOGIC_TO_NODE_IS_VERT )
					 || ( m_flags & PEDLOGIC_NEXT_NODE_IS_VERT )
					 || ( m_flags & PEDLOGIC_FROM_NODE_IS_VERT ) ) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE );
			break;
		case CRCC( 0x56d9f55f, "Ped_PlayJumpSound" ):
		{
			//Dbg_MsgAssert( m_state == CRCD(0xa85af587,"generic_skater"), ( "%s\nPed_PlayJumpSound called on a non-skater ped %s",pScript->GetScriptInfo(),Script::FindChecksumName(GetObject()->GetID()) ) );
			Obj::CSkaterSoundComponent *pSoundComponent = GetSkaterSoundComponentFromObject( GetObject() );
			if ( pSoundComponent )
				pSoundComponent->PlayJumpSound(m_speed_fraction);
			break;
		}
		case CRCC( 0x749de914, "Ped_PlayLandSound" ):
		{
			Dbg_MsgAssert( m_state == CRCD(0xa85af587,"generic_skater"), ( "Ped_PlayLandSound called on a non-ped skater" ) );
			Obj::CSkaterSoundComponent *pSoundComponent = GetSkaterSoundComponentFromObject( GetObject() );
			if ( pSoundComponent )
				pSoundComponent->PlayLandSound(m_speed_fraction);
			break;
		}
		case CRCC( 0x58a588b5, "Ped_GetCurrentNodeNames" ):
		{
			pScript->GetParams()->AddChecksum( CRCD( 0xd14cd5bc, "node_to" ), m_node_to );
			pScript->GetParams()->AddChecksum( CRCD( 0x8141d319, "node_from" ), m_node_from );
			pScript->GetParams()->AddChecksum( CRCD( 0x3c746255, "node_next" ), m_node_next );
			break;
		}
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to C......Component::GetDebugInfo"));
	p_info->AddChecksum( "m_state", m_state );
	
	Script::CArray* pTempArray = new Script::CArray();
	pTempArray->SetSizeAndType( vNUM_WHISKERS, ESYMBOLTYPE_FLOAT );
	for ( int i = 0; i < vNUM_WHISKERS; i++ )
		pTempArray->SetFloat( i, m_whiskers[i] );
	p_info->AddArray( "m_whiskers", pTempArray );
	for ( int i = 0; i < vNUM_WHISKERS; i++ )
		pTempArray->SetFloat( i, m_bias[i] );
	p_info->AddArray( "m_bias", pTempArray );
	Script::CleanUpArray( pTempArray );
	delete pTempArray;

	p_info->AddInteger( "moving_on_path", m_flags & PEDLOGIC_MOVING_ON_PATH );

	p_info->AddInteger( "m_node_from", m_node_from );
	p_info->AddInteger( "m_node_to", m_node_to );
	p_info->AddInteger( "m_node_next", m_node_next );

	p_info->AddVector( "m_wp_from", m_wp_from );
	p_info->AddVector( "m_wp_to", m_wp_to );
	p_info->AddVector( "m_wp_next", m_wp_next );

	p_info->AddInteger( "m_turn_frames", m_turn_frames );
	p_info->AddInteger( "m_max_turn_frames", m_max_turn_frames );

	p_info->AddInteger( "m_is_grinding", m_flags & PEDLOGIC_GRINDING );

	p_info->AddFloat( "m_col_dist_above", m_col_dist_above );
	p_info->AddFloat( "m_col_dist_below", m_col_dist_below );
	
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPedLogicComponent::GenericPedUpdate()
{	
	if ( !( m_flags & PEDLOGIC_MOVING_ON_PATH ) )
		return;

	Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
	Dbg_Assert( pMotionComp );
	// Obj::CPathObjectTracker* p_path_object_tracker = pMotionComp->mp_path_object_tracker;

	bool is_avoiding = CheckForOtherPeds();	
	bool is_turning = UpdateTargetBias();

	// update bias based on path following
	if ( !is_turning && !is_avoiding )
	{
		UpdatePathBias();
	}
	
	// Gfx::AddDebugLine( m_wp_from, m_wp_to, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 1 );
	// Gfx::AddDebugLine( GetObject()->m_pos, m_wp_to, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 1 );

	// move and decay
	pMotionComp->DoPathPhysics();
	UpdatePosition();
	DecayWhiskerBiases();
	pMotionComp->StickToGround();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::GenericSkaterUpdate()
{
	Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
	Dbg_Assert( pMotionComp );
	pMotionComp->DoPathPhysics();
	float distance = m_time * pMotionComp->m_vel_z;
	bool is_jumping = pMotionComp->m_movingobj_status & MOVINGOBJ_STATUS_JUMPING ? true : false;

	// Gfx::AddDebugLine( m_wp_from, m_wp_to, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 1 );
	// Gfx::AddDebugLine( GetObject()->m_pos, m_wp_to, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 1 );
	// Gfx::AddDebugLine( m_pos_history[vPOS_HISTORY_SIZE - 2] + Mth::Vector( 0, 5, 1 ), m_pos_history[vPOS_HISTORY_SIZE - 1] + Mth::Vector( 0, 5, 1 ), MAKE_RGB( 0, 128, 0 ), MAKE_RGB( 0, 128, 0 ) );
	// Gfx::AddDebugArrow( GetObject()->GetDisplayMatrix()[Z] + GetObject()->m_pos, GetObject()->GetDisplayMatrix()[Z] * 24 + GetObject()->m_pos, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 1 );
	
	// check for skater if we're grinding
	if ( m_flags & PEDLOGIC_GRINDING )
	{
		Mdl::Skate* skate_mod = Mdl::Skate::Instance();
		Dbg_Assert( skate_mod );
		Obj::CSkater* pSkater = skate_mod->GetLocalSkater();
		if ( pSkater )
		{
			float d = Mth::DistanceSqr( GetObject()->m_pos, pSkater->GetPos() );
			// printf("skater dist square = %f\n", d );
			if ( d < Script::GetFloat( CRCD( 0xbc2e678a, "ped_skater_min_square_distance_to_skater" ), Script::ASSERT ) )
			{
				GrindBail();
				return;
			}
		}
	}
	
	// see if we've hit the waypoint
	if ( ( m_flags & PEDLOGIC_MOVING_ON_PATH ) || is_jumping )
	{
		Mth::Vector pt;
		if ( ( m_flags & PEDLOGIC_TO_NODE_IS_VERT ) || is_jumping )
		{
			pt = ( GetObject()->m_pos - m_wp_to );
		}
		else
		{
			Mth::Vector wp_to_no_y = m_wp_to;
			wp_to_no_y[Y] = 0.0f;
			Mth::Vector ped_pos_no_y = GetObject()->m_pos;
			ped_pos_no_y[Y] = 0.0f;
			pt = ped_pos_no_y - wp_to_no_y;
		}
		float distance_to_target_square = pt.LengthSqr();
		int min_dist_square = Script::GetInteger( CRCD( 0xfb5d106f, "ped_skater_min_square_distance_to_waypoint" ), Script::ASSERT );
	
		if ( !( m_flags & PEDLOGIC_JUMPING_TO_NODE ) )
		{
			if ( distance_to_target_square < Mth::Sqr( distance ) )
				HitWaypoint();
			else if ( distance_to_target_square < min_dist_square )
				HitWaypoint();
		}

		if ( m_flags & PEDLOGIC_JUMP_AT_TO_NODE )
		{
			int dist_to_crouch = Script::GetInteger( CRCD( 0x52ec432f, "ped_skater_min_square_distance_to_crouch_for_jump" ), Script::ASSERT );
			if ( distance_to_target_square < dist_to_crouch )
			{
				// clear flag!
				m_flags &= ~PEDLOGIC_JUMP_AT_TO_NODE;

				Script::CScript* pScript = Script::SpawnScript( CRCD( 0x9c7ab5f5, "ped_skater_crouch_for_jump" ) );
				pScript->mpObject = GetObject();
			}
		}
	}

	// move
	if ( ( m_flags & PEDLOGIC_MOVING_ON_PATH ) && distance )
	{
		Mth::Vector original_pos = GetObject()->m_pos;
		bool already_adjusted_heading = false;
		if ( ShouldUseBiases() )
		{
			// use biases to update position
			bool is_turning = UpdateTargetBias();
			if ( !is_turning )
			{
				UpdatePathBias();
			}
			UpdatePosition();
			
			// refresh the display matrix if we're jumping, as 
			// AdjustHeading and AdjustNormal won't be called.
			if ( is_jumping )
			{
				RefreshDisplayMatrix();
			}
		}
		else
		{
			// figure the new position
			Mth::Vector new_pos = original_pos + ( distance * ( m_wp_to - GetObject()->m_pos ).Normalize() );
			UpdatePathBias();
			UpdatePosition();			
			GetObject()->m_pos = new_pos;
			AdjustHeading( original_pos );
			already_adjusted_heading = true;
		}

		// stick to ground if we're not grinding or jumping
		if ( !( m_flags & PEDLOGIC_GRINDING )
			 && !( m_flags & PEDLOGIC_GRIND_BAILING )
			 && !is_jumping
			)
		{
			StickToGround();
			// update z vector
			if ( !already_adjusted_heading )
				AdjustHeading( original_pos );
			AdjustNormal();
		}
	}
	else // if ( !( pMotionComp->m_movingobj_status & MOVINGOBJ_STATUS_JUMPING ) )
	{
		RotatePosHistory();
		if ( m_flags & PEDLOGIC_TO_NODE_IS_VERT )
		{
			// AdjustHeading();
		}
		RefreshDisplayMatrix();
		if ( m_flags & PEDLOGIC_DOING_VERT_ROTATION || m_flags & PEDLOGIC_DOING_SPINE )
		{
			DoRotations();
		}
	}

	// update states for looping sounds
	if ( is_jumping )
	{
		UpdateSkaterSoundStates( AIR );
	}
	else if ( m_flags & PEDLOGIC_GRINDING )
	{
		UpdateSkaterSoundStates( RAIL );
	}
	else if ( m_flags & PEDLOGIC_MOVING_ON_PATH )
	{
		UpdateSkaterSoundStates( GROUND );
	}
	else if ( m_flags & PEDLOGIC_DOING_LIP_TRICK )
	{
		UpdateSkaterSoundStates( LIP );
	}
 	
	if ( m_flags & PEDLOGIC_MOVING_ON_PATH )
		m_speed_fraction = ( pMotionComp->m_vel_z / pMotionComp->m_max_vel );
	else
		m_speed_fraction = 0.0f;
	
	Obj::CSkaterLoopingSoundComponent* pLoopingSoundComp = GetSkaterLoopingSoundComponentFromObject( GetObject() );
	if ( pLoopingSoundComp )
	{
		// reset bailing flag
		pLoopingSoundComp->SetIsBailing( m_flags & PEDLOGIC_GRIND_BAILING || m_flags & PEDLOGIC_BAILING );
		pLoopingSoundComp->SetSpeedFraction( m_speed_fraction );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPedLogicComponent::CheckForOtherPeds()
{
	bool is_avoiding = false;
	if ( mp_path_object_tracker )
	{		
		const CSmtPtr<CCompositeObject>* pp_object_list = mp_path_object_tracker->GetObjectList();
		Mth::Matrix display_matrix = GetObject()->GetDisplayMatrix();
		float range = Script::GetFloat( CRCD( 0x7a78f471, "ped_avoid_ped_range" ) );
		float avoid_ped_bias = Script::GetFloat( CRCD( 0xd1e6177b, "ped_avoid_ped_bias" ) );
		Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
		Dbg_Assert( pMotionComp );

		for ( int i = 0; i < CPathObjectTracker::MAX_OBJECTS_PER_PATH; ++i )
		{
			if ( pp_object_list[i] )
			{
				CCompositeObject* p_ob=pp_object_list[i].Convert();
	
				if ( p_ob != GetObject() )
				{
					// ignore peds that are too far above or below
					float max_y_dist = Script::GetFloat( CRCD(0x21b13ffc,"ped_max_y_distance_to_ignore"), Script::ASSERT );
					if ( Mth::Abs( GetObject()->GetPos()[Y] - p_ob->GetPos()[Y] ) > max_y_dist )
					{
						continue;
					}
					
					// ignore peds that are facing the opposite direction and walking away
					float z_dot = Mth::DotProduct( display_matrix[Z], p_ob->GetDisplayMatrix()[Z] );
					if ( z_dot <= 0.0f && Mth::DotProduct( display_matrix[Z], p_ob->GetPos() - GetObject()->GetPos() ) <= 0.0f )
					{
						// printf("walking away from each other\n");
						continue;
					}
					
					// ignore peds that are facing the same direction and going faster than us
					Obj::CMotionComponent* pObMotionComp = GetMotionComponentFromObject( p_ob );
					Dbg_Assert( pObMotionComp );
					if ( z_dot > 0.0f && pObMotionComp->m_max_vel >= pMotionComp->m_max_vel )
					{
						// printf("he's going faster than me\n");
						continue;
					}

					float d = Mth::Distance( GetObject()->GetPos(), p_ob->GetPos() );					
					if ( d <= range )
					{						
						// adjust whisker
						// find the heading
						float theta = acosf( z_dot / ( display_matrix[Z].Length() * p_ob->GetDisplayMatrix()[Z].Length() ) );
						
						// printf("theta = %f\n", theta);
						// reduce to less than 2pi.
						if ( theta > Mth::PI * 2 )
							theta -= Mth::PI * 2 * (int)( theta / ( Mth::PI * 2 ) );
						// if they're coming straight at each other, we'll have to throw in some
						// sideways bias - we don't want them to stop moving or turn around
						// As they start to pass each other, the offset will naturally start
						// to create its own bias
						if ( fabs( theta - Mth::PI ) < Script::GetFloat( CRCD( 0xd2e22cec, "ped_head_on_range" ), Script::ASSERT ) )
						{
							// printf("he's coming right for me!\n");
							// left by default
							theta = Mth::PI / 2;
							// see if we should push them right
							// if ( theta > Mth::PI )
							if ( ( Mth::CrossProduct( display_matrix[Z], p_ob->GetDisplayMatrix()[Z] ) )[Y] < 0 )
								theta *= -1;
						}
						is_avoiding = true;
						AddWhiskerBias( theta, avoid_ped_bias, d, range );
					}
				}
			}
		}
	}
	return is_avoiding;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::UpdatePathBias()
{
	Mth::Vector wp_from_no_y = m_wp_from;
	wp_from_no_y[Y] = 0.0f;
	Mth::Vector wp_to_no_y = m_wp_to;
	wp_to_no_y[Y] = 0.0f;
	Mth::Vector ped_pos_no_y = GetObject()->m_pos;
	ped_pos_no_y[Y] = 0.0f;

	float max_dist_to_path = Script::GetFloat( CRCD( 0xa876aa2c, "ped_max_distance_to_path" ), Script::ASSERT );
	Mth::Vector ft = wp_to_no_y - wp_from_no_y;
	Mth::Vector fp = ped_pos_no_y - wp_from_no_y;

	// need to store this vector so we can check the sign of y...
	// if it's negative, we're on one side of the path, positive for the other
	Mth::Vector ft_cross_fp = Mth::CrossProduct( ft, fp );

	// get the direction we want to push the ped
	Mth::Vector closest_pos_on_path = ( ( Mth::DotProduct( fp, ft ) / ft.LengthSqr() ) * ft ) + m_wp_from;
	closest_pos_on_path[Y] = 0.0f;
	Mth::Vector pos_to_path = closest_pos_on_path - ped_pos_no_y;

	// Gfx::AddDebugArrow( GetObject()->m_pos, closest_pos_on_path, MAKE_RGB( 0, 128, 0 ), MAKE_RGB( 0, 128, 0 ), 1 );

	// Mth::Matrix ob_matrix = GetObject()->m_matrix;
	Mth::Matrix display_matrix = GetObject()->GetDisplayMatrix();
	float angle_to_path = Mth::GetAngle( display_matrix[Z], pos_to_path );
	if ( angle_to_path > 360.0f )
		angle_to_path -= 360.0f * (int)( angle_to_path / 360.0f );
	if ( Mth::CrossProduct( display_matrix[Z], pos_to_path )[Y] > 0 )
		angle_to_path = 360.0f - angle_to_path;

	// float path_length = Mth::Distance( m_wp_from, m_wp_to );
	float path_length = Mth::Distance( wp_from_no_y, wp_to_no_y );
	float current_distance = ft_cross_fp.Length() / path_length;

	// the bias should get stronger as we get farther away, approaching 1 as we 
	// approach the max distance from the path
	float path_bias;
	if ( current_distance > max_dist_to_path )
		path_bias = 1;
	else
		path_bias = 1 - ( ( max_dist_to_path - current_distance ) / max_dist_to_path );

	// reset the bias to get back on the path
	if ( current_distance > Script::GetFloat( CRCD( 0x81085734, "ped_min_distance_to_path" ) ) )
		AdjustBias( Mth::DegToRad( angle_to_path ), path_bias );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPedLogicComponent::UpdateTargetBias()
{
	bool is_turning = false;
	Mth::Vector wp_to_no_y = m_wp_to;
	wp_to_no_y[Y] = 0.0f;
	Mth::Vector ped_pos_no_y = GetObject()->m_pos;
	ped_pos_no_y[Y] = 0.0f;
	Mth::Vector pt = ped_pos_no_y - wp_to_no_y;
	float distance_to_target_square = pt.LengthSqr();
	
	float target_bias = Script::GetFloat( CRCD(0x57342672,"ped_target_node_bias"), Script::ASSERT );
	ResetBias();

	if ( m_node_next == m_node_from )
	{
		float min_square_distance_to_dead_end = Script::GetFloat( CRCD( 0x65b46f32, "ped_walker_min_square_distance_to_dead_end" ), Script::ASSERT );
		if ( distance_to_target_square <= min_square_distance_to_dead_end )
		{
			HitWaypoint();
		}
	}
	else
	{
		int fade_bias_max_distance_square;
		if ( m_flags & PEDLOGIC_IS_SKATER )
			fade_bias_max_distance_square = Script::GetInteger( CRCD( 0x354beda1, "ped_skater_fade_target_bias_max_distance_square" ), Script::ASSERT );
		else
			fade_bias_max_distance_square = Script::GetInteger( CRCD( 0x5b21ab0e, "ped_fade_target_bias_max_distance_square" ), Script::ASSERT );

		if ( m_max_turn_frames && m_turn_frames >= m_max_turn_frames )
		{
			HitWaypoint();
		}
		else if ( m_node_next != -1 && distance_to_target_square < fade_bias_max_distance_square )
		{
			// do we need to reset m_max_turn_frames?
			if ( !m_max_turn_frames )
			{
				// figure how long it will take to get there if we don't turn
				Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
				Dbg_Assert( pMotionComp );
				
				Mth::Vector wp_from_no_y = m_wp_from;
				wp_from_no_y[Y] = 0.0f;
				Mth::Vector wp_to_no_y = m_wp_to;
				wp_to_no_y[Y] = 0.0f;
				Mth::Vector wp_next_no_y = m_wp_next;
				wp_next_no_y[Y] = 0.0f;

				Mth::Vector ft = wp_to_no_y - wp_from_no_y;
				Mth::Vector tn = wp_next_no_y - wp_to_no_y;

				float angle_between_waypoints = Mth::DegToRad( Mth::GetAngle( ft, tn ) );
				float distance_to_waypoint_switch = sqrtf( ( 2 * distance_to_target_square ) * ( 1 + cosf( angle_between_waypoints ) ) );
				int frames = (int)( ( sqrtf( distance_to_waypoint_switch ) / pMotionComp->m_max_vel ) * 60.0f );
				// printf("frames = %i\n", frames);
				m_max_turn_frames = frames;
				if ( m_max_turn_frames <= 0 )
				{
					m_max_turn_frames = 1;
				}
				
				// int frames = (int)( ( sqrtf( distance_to_target_square ) / pMotionComp->m_max_vel ) * 60.0f );
				// m_max_turn_frames = frames / 2;
			}
	
			// add bias for new waypoint and adjust current target bias
			is_turning = true;

			// Guard against potential division by zero here.
			float target_bias_turn_percent;
			if( m_max_turn_frames == 0 )
				target_bias_turn_percent = 0.0f;
			else
				target_bias_turn_percent = 0.5f * ( 1 - (float)( m_max_turn_frames - m_turn_frames ) / (float)m_max_turn_frames );

			m_turn_frames++;
			AddTargetBias( m_wp_next, target_bias * target_bias_turn_percent );
			target_bias -= target_bias * target_bias_turn_percent;
		}
	}
	AddTargetBias( m_wp_to, target_bias );
	return is_turning;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::UpdatePosition()
{
	Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
	Dbg_Assert( pMotionComp );

	float x_bias = GetTotalXBias();
	float z_bias = GetTotalZBias();
	// printf("x_bias = %f, z_bias = %f\n", x_bias, z_bias );

	Mth::Vector last_pos = GetObject()->m_pos;
	
	RotatePosHistory();
	
	float vel = pMotionComp->m_vel_z;
	float distance_to_travel = vel * m_time;
	float x_distance = distance_to_travel * x_bias;
	float z_distance = distance_to_travel * z_bias;
/*	float total_bias = x_bias + z_bias;
	if ( !total_bias )
		return;
	float vel = pMotionComp->m_vel_z;

	float distance_to_travel = vel * m_time;
	float x_distance = distance_to_travel * ( ( total_bias - x_bias ) / total_bias );
	if ( x_bias < 0 )
		x_distance *= -1;
	float z_distance = distance_to_travel * ( ( total_bias - z_bias ) / total_bias );
	if ( z_bias < 0 )
		z_distance *= -1;
*/   
	// printf("x_dist = %f, z_dist = %f\n", x_distance, z_distance);
	// Mth::Matrix mat0 = GetObject()->m_matrix;
	Mth::Matrix mat0 = GetObject()->GetDisplayMatrix();
	Mth::Vector z_vect = mat0[Z] * z_distance;
	Mth::Vector x_vect = mat0[X] * x_distance;
	
	// GetObject()->GetDisplayMatrix()[Z] = new_heading;
	// GetObject()->GetDisplayMatrix().OrthoNormalizeAbout( Z );
	GetObject()->m_pos += x_vect;
	GetObject()->m_pos += z_vect;
	
	// skaters do their own headings
	if ( last_pos != GetObject()->m_pos && !( m_flags & PEDLOGIC_IS_SKATER ) )
		AdjustHeading( last_pos );

/*
	float angle_to_new_heading = atan2( x_distance, z_distance );
	if ( fabs( angle_to_new_heading ) > Script::GetFloat( "ped_max_angle_to_heading", Script::ASSERT ) )
	{
		GetObject()->GetDisplayMatrix().Rotate( mat0[Y], angle_to_new_heading );
		// mat0.Rotate( mat0[Y], angle_to_new_heading );
		// GetObject()->m_matrix.Rotate( mat0[Y], angle_to_new_heading );
		// printf( "angle_to_new_heading = %f\n", Mth::RadToDeg( angle_to_new_heading ) );
	}
*/
	// GetObject()->SetDisplayMatrix( mat0 );
	// GetObject()->m_matrix = mat0;
	// GetObject()->m_pos += adjustment;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::RotatePosHistory()
{
	for ( int i = 0; i < vPOS_HISTORY_SIZE - 1; i++ )
	{
		m_pos_history[i] = m_pos_history[i + 1];
	}
	m_pos_history[vPOS_HISTORY_SIZE - 1] = GetObject()->m_pos;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::AddTargetBias( Mth::Vector target, float target_bias )
{
	Mth::Matrix display_matrix = GetObject()->GetDisplayMatrix();
	
	Mth::Vector ped_pos_no_y = GetObject()->m_pos;
	ped_pos_no_y[Y] = 0.0f;
	target[Y] = 0.0f;

	// Mth::Vector pt = GetObject()->m_pos - target;
	Mth::Vector pt = ped_pos_no_y - target;
	float angle_to_target = Mth::GetAngle( display_matrix[Z], pt );
	angle_to_target += 180.0f;
	if ( angle_to_target > 360.0f )
		angle_to_target -= 360.0f * (int)( angle_to_target / 360.0f );

	if ( ( Mth::CrossProduct( display_matrix[Z], pt ) )[Y] > 0 )
		angle_to_target = 360.0f - angle_to_target;

	// printf("angle_to_target = %f\n", angle_to_target );
	
	// add bias to get to the target - the target_bias is constant
	AdjustBias( Mth::DegToRad( angle_to_target ), target_bias );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::AddWhiskerBias( float angle, float amount, float distance, float range )
{
	// for now, linearly decrease the bias amount within the range
	amount *= ( range - distance ) / range;

	float segment_arc = Mth::DegToRad( 360 / vNUM_WHISKERS );
	for ( int i = 0; i < vNUM_WHISKERS; i++ )
	{
		float angle_between = ( segment_arc * i ) - angle;
		// make sure this is in the right quandrant
		if ( fabs ( angle_between ) >= Mth::PI / 2 && fabs( angle_between ) <= 3 * Mth::PI / 2 )
			continue;

		// float adjusted_amount = fabs( amount * sinf( angle_between ) );
		float adjusted_amount = amount * cosf( angle_between );
		// printf("adding %f to m_whiskers[%i]\n", adjusted_amount, i );
		// add this bias to the whisker
		m_whiskers[i] += adjusted_amount;

		// cap absolute value to 1
		if ( m_whiskers[i] > 1 )
			m_whiskers[i] = 1;
		if ( m_whiskers[i] < -1 )
			m_whiskers[i] = -1;
	}
/*
	float segment_arc = Mth::DegToRad( 360 / vNUM_WHISKERS );
	for ( int i = 0; i < vNUM_WHISKERS; i++ )
	{
		float angle_between = fabs( ( segment_arc * i ) - angle );
		
		// make sure this is less than 180 from the angle we're looking at
		if ( angle_between >= Mth::PI )
			continue;

		// don't bother with maxed bias
		if ( m_whiskers[i] == 1 )
			continue;
		
		// add this bias to the whisker
		m_whiskers[i] += fabs( amount * cos( angle_between ) );

		// cap to 1
		if ( m_whiskers[i] > 1 )
			m_whiskers[i] = 1;
	}
*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::AdjustBias( float angle, float amount )
{
	// printf("AdjustBias( %f, %f )\n", angle, amount );
	// translate the bias to the whisker(s)
	// makes sure the angle isn't greater than 360
	// printf("angle = %f\n", Mth::RadToDeg( angle ) );
	float segment_arc = Mth::DegToRad( 360 / vNUM_WHISKERS );
	for ( int i = 0; i < vNUM_WHISKERS; i++ )
	{		
		float angle_between = ( segment_arc * i ) - angle;
		
		// make sure this is in the right quandrant
		if ( fabs ( angle_between ) >= Mth::PI / 2 && fabs( angle_between ) <= 3 * Mth::PI / 2 )
			continue;

		// float adjusted_amount = fabs( amount * sinf( angle_between ) );
		float adjusted_amount = amount * cosf( angle_between );
		// printf("adding %f to m_bias[%i]\n", adjusted_amount, i );
		// add this bias to the whisker
		m_bias[i] += adjusted_amount;

		// cap absolute value to 1
		if ( m_bias[i] > 1 )
			m_bias[i] = 1;
		if ( m_bias[i] < -1 )
			m_bias[i] = -1;
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DecayWhiskerBiases()
{
	// multiply each whisker by the decay factor
	float decay_factor = Script::GetFloat( CRCD( 0x98cd0c8c, "ped_whisker_decay_factor" ), Script::ASSERT );
	float min_bias = Script::GetFloat( CRCD( 0x4aba9798, "ped_min_bias" ), Script::ASSERT );
	for ( int i = 0; i < vNUM_WHISKERS; i++ )
	{
		m_whiskers[i] *= decay_factor;
		if ( m_whiskers[i] < min_bias )
			m_whiskers[i] = 0;
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CPedLogicComponent::GetTotalXBias()
{
	float total = 0.0f;
	float segment_arc = Mth::DegToRad( 360 / vNUM_WHISKERS );
	for ( int i = 0; i < vNUM_WHISKERS; i++ )
	{
		// float angle = Mth::PI - ( segment_arc * i );
		float angle = segment_arc * i;
		// phase shift 180
		// angle += Mth::PI;
		float whisker_total = m_whiskers[i] + m_bias[i];
		// float whisker_total = m_whiskers[i];
		// float whisker_total = m_bias[i];
    	if ( whisker_total > 1 )
			whisker_total = 1;
		
		total += -sinf( angle ) * whisker_total;
		
		if ( total >= 1 )
			return 1;
		if ( total <= -1 )
			return -1;
	}
	if ( fabs( total ) < Script::GetFloat( CRCD( 0x4aba9798, "ped_min_bias" ), Script::ASSERT ) )
		total = 0;
	// remember that we're 180 degrees out of phase
	return total;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CPedLogicComponent::GetTotalZBias()
{
	float total = 0.0f;
	float segment_arc = Mth::DegToRad( 360 / vNUM_WHISKERS );
	for ( int i = 0; i < vNUM_WHISKERS; i++ )
	{
		float angle = segment_arc * i;
		// phase shift 180
		// angle += Mth::PI;
		float whisker_total = m_whiskers[i] + m_bias[i];
		// float whisker_total = m_whiskers[i];
		// float whisker_total = m_bias[i];
		if ( whisker_total > 1 )
			whisker_total = 1;
		if ( whisker_total < -1 )
			whisker_total = -1;
		// printf("whisker_total[%i] = %f, angle = %f\n", i, whisker_total, angle);
		// 90 degree phase shift
		// total += cosf( angle ) * whisker_total;
		total += cosf( angle ) * whisker_total;
		if ( total >= 1)
			return 1;
		if ( total <= -1 )
			return -1;
	}
	if ( fabs( total ) < Script::GetFloat( CRCD( 0x4aba9798, "ped_min_bias" ), Script::ASSERT ) )
		total = 0;
	// return total;
	return total;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::ResetBias()
{
	for ( int i = 0; i < vNUM_WHISKERS; i++ )
		m_bias[i] = 0;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::HitWaypoint()
{
	// printf( "CPedLogicComponent::HitWaypoint\n" );
	int hit_node = m_node_to;
	int old_from_node = m_node_from;
	
	m_flags &= ~PEDLOGIC_FROM_NODE_IS_VERT;
	m_flags &= ~PEDLOGIC_NEXT_NODE_IS_VERT;
	if ( m_flags & PEDLOGIC_TO_NODE_IS_VERT )
		m_flags |= PEDLOGIC_FROM_NODE_IS_VERT;

	m_flags &= ~PEDLOGIC_JUMPING_TO_NODE;

	m_node_from = m_node_to;
	m_node_to = m_node_next;
	m_wp_from = m_wp_to;
	m_wp_to = m_wp_next;
	
	// handle dead ends for ped walkers
	if ( !( m_flags & PEDLOGIC_IS_SKATER ) && old_from_node == m_node_to )
	{
		Script::CScript* pDeadEndScript = Script::SpawnScript( CRCD( 0xa80f965e, "ped_walker_hit_dead_end" ) );
		pDeadEndScript->mpObject = GetObject();
	}
	
	// update m_to_node_is_vert for distance checks
	m_flags &= ~PEDLOGIC_TO_NODE_IS_VERT;
	// m_to_node_is_vert = false;
	Script::CStruct* pToNodeData = SkateScript::GetNode( m_node_to );

	m_flags &= ~PEDLOGIC_TO_NODE_IS_LIP;

	uint32 to_node_type;
	if ( pToNodeData->GetChecksum( CRCD( 0x17ea37fb, "PedType" ), &to_node_type, Script::NO_ASSERT )
		 && to_node_type == CRCD( 0x54166acd, "skate" ) )
	{
		uint32 to_node_action;
		pToNodeData->GetChecksum( CRCD( 0x963dc198, "SkateAction" ), &to_node_action, Script::ASSERT );
		if ( WaypointIsVert( m_node_to ) )
		{
			m_flags |= PEDLOGIC_TO_NODE_IS_VERT;
			// m_to_node_is_vert = true;
		}
		
		// check for a jump next - we'll need to crouch if we're not
		// already doing some action
		if ( to_node_action == CRCD( 0x7d9d0008, "vert_grab" )
			 || to_node_action == CRCD( 0x584cf9e9, "jump" ) )
		{
			// make sure we're not coming from an action
			Script::CStruct* pFromNodeData = SkateScript::GetNode( m_node_from );
			uint32 from_node_action;
			if ( pFromNodeData->GetChecksum( CRCD( 0x963dc198, "SkateAction" ), &from_node_action, Script::ASSERT ) )
			{
				if ( from_node_action != CRCD( 0x255ed86f, "grind" )
					 && !( m_flags & PEDLOGIC_GRINDING )
					 && !( m_flags & PEDLOGIC_FROM_NODE_IS_VERT ) )
				{
					m_flags |= PEDLOGIC_JUMP_AT_TO_NODE;
				}
			}
		}

		if ( to_node_action == CRCD( 0x554dbd64, "vert_lip" ) )
		{
			m_flags |= PEDLOGIC_TO_NODE_IS_LIP;
		}
	}
	
	SelectNextWaypoint();
	if ( WaypointIsVert( m_node_next ) )
		m_flags |= PEDLOGIC_NEXT_NODE_IS_VERT;

	// we don't want to do any actions if we just came from a vert_grab node
	Script::CStruct* pOldFromNodeData = SkateScript::GetNode( old_from_node );
	uint32 old_skate_action;
	Script::CStruct* pNodeData = SkateScript::GetNode( hit_node );
	Dbg_Assert( pNodeData );
	DoGenericNodeActions( pNodeData );

	if ( !( pOldFromNodeData->GetChecksum( CRCD( 0x963dc198, "skateAction" ), &old_skate_action )
		 && old_skate_action == CRCD( 0x7d9d0008, "Vert_Grab" ) ) )
	{
		uint32 waypoint_type;
		if ( pNodeData->GetChecksum( CRCD( 0x17ea37fb, "PedType" ), &waypoint_type, Script::NO_ASSERT ) )
		{
			switch ( waypoint_type )
			{
			case CRCC( 0x726e85aa, "walk" ):
				DoWalkActions( pNodeData );
				break;
			case CRCC( 0x54166acd, "skate" ):
				DoSkateActions( pNodeData );
				break;
			default:
				Dbg_MsgAssert( 0, ( "Waypoint has bad PedType %s", Script::FindChecksumName( waypoint_type ) ) );
			}
		}
	}

	m_turn_frames = m_max_turn_frames = 0;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPedLogicComponent::WaypointIsVert( int waypoint )
{
	Script::CStruct* pNodeData = SkateScript::GetNode( waypoint );
	uint32 node_action;
	if ( pNodeData->GetChecksum( CRCD( 0x963dc198, "SkateAction" ), &node_action, Script::NO_ASSERT ) )
	{
		if ( node_action == CRCD( 0x7d9d0008, "vert_grab" )
			 || node_action == CRCD( 0x554dbd64, "vert_lip" )
			 || node_action == CRCD( 0xe6dfaec7, "vert_grind" )
			 || node_action == CRCD( 0xda0723da, "vert_land" )
			 || node_action == CRCD( 0xe8f91257, "vert_flip" )
			 || node_action == CRCD( 0xd5b4f014, "vert_jump" ) )
		{
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoWalkActions( Script::CStruct* pNodeData )
{

}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoGenericNodeActions( Script::CStruct* pNodeData )
{
	Dbg_Assert( pNodeData );
	// check for special things we should do at this node
	if ( pNodeData->ContainsFlag( CRCD( 0x74e4bba5, "AdjustSpeed" ) ) )
	{
		if ( ShouldExecuteAction( pNodeData, CRCD( 0x922c5b59, "AdjustSpeedWeight" ) ) )
		{
			float percent;
			pNodeData->GetFloat( CRCD( 0xa04d16ec, "AdjustSpeedPercent" ), &percent, Script::ASSERT );
			AdjustSpeed( percent );
		}
	}
	if ( pNodeData->ContainsFlag( CRCD( 0x55d98ed7, "RunScript" ) ) )
	{
		if ( ShouldExecuteAction( pNodeData, CRCD( 0x86c9ccce, "RunScriptWeight" ) ) )
		{
			uint32 script_name;
			pNodeData->GetChecksum( CRCD( 0x64b4cd9d, "RunScriptName" ), &script_name, Script::ASSERT );
			Script::CScript* pScript = Script::SpawnScript( script_name );
			pScript->mpObject = GetObject();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoSkateActions( Script::CStruct* pNodeData )
{
	// we don't want to do any actions if we're bailing
	if ( ( m_flags & PEDLOGIC_BAILING ) || ( m_flags & PEDLOGIC_GRIND_BAILING ) )
		return;
	
	Dbg_Assert( pNodeData );
	uint32 skate_action;
	if ( pNodeData->GetChecksum( CRCD( 0x963dc198, "SkateAction" ), &skate_action, Script::NO_ASSERT ) )
	{
		switch ( skate_action )
		{
			case CRCC( 0xec1cd520, "continue" ):
				break;
			case CRCC( 0x255ed86f, "grind" ):
			case CRCC( 0xe6dfaec7, "vert_grind" ):
				DoGrind( pNodeData );
				break;
			case CRCC( 0xc870bce5, "grind_off" ):
				DoGrindOff( pNodeData );
				break;
			case CRCC(0xe41199af,"grind_bail"):
				// ignore these nodes...we must have fallen here from a grind
				break;
			case CRCC(0xba1d1e94,"flip_trick"):
				DoFlipTrick( pNodeData );
				break;
			case CRCC( 0xe8f91257, "vert_flip" ):
				DoFlipTrick( pNodeData, true );
				break;
			case CRCC( 0x12797a1b, "grab_trick" ):
				DoGrab( pNodeData );
				break;
			case CRCC( 0x7d9d0008, "vert_grab" ):
				DoGrab( pNodeData, true );
				break;
			case CRCC( 0x554dbd64, "vert_lip" ):
				DoLipTrick( pNodeData );
				break;
			case CRCC( 0xda0723da, "vert_land" ):
				break;
			case CRCC( 0x584cf9e9, "jump" ):
				DoJump( pNodeData );
				break;
			case CRCC( 0xd5b4f014, "vert_jump" ):
				DoJump( pNodeData, true );
				break;
			case CRCC( 0x6d3144bf, "Roll_off" ):
				DoRollOff( pNodeData );
				break;
			case CRCC( 0xef24413b, "manual" ):
				DoManual( pNodeData );
				break;
			case CRCC( 0x9403a0ed, "manual_down" ):
				DoManualDown( pNodeData );
				break;
			case CRCC( 0x46a9e949, "stop" ):
				Stop( pNodeData );
				break;
			default:
				Dbg_MsgAssert( 0, ( "DoSkateActions found unknown action %s\n", Script::FindChecksumName( skate_action ) ) );
				break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::SelectNextWaypoint()
{
	// are we on a grind bail path?
	bool on_grind_bail_path = false;
	Script::CStruct* pNode = SkateScript::GetNode( m_node_from );
	uint32 skate_action;
	if ( pNode->GetChecksum( CRCD( 0x963dc198, "SkateAction" ), &skate_action )
		 && skate_action == CRCD( 0xe41199af, "Grind_Bail" ) )
	{
		on_grind_bail_path = true;
	}
	
	int waypoint;
	int numLinks;
	numLinks = SkateScript::GetNumLinks( m_node_to );

	if ( numLinks == 0 )
	{
		// turn around
		waypoint = m_node_from;
	}
	else if ( numLinks == 1 ) // shortcut if there's only one link
	{
		waypoint = SkateScript::GetLink( m_node_to, 0 );
		
		// special case for jumping straight up in a pipe...land where you started
		if ( waypoint == m_node_from )
		{
			Script::CStruct* pToNodeData = SkateScript::GetNode( m_node_to );
			uint32 skate_action;
			if ( pToNodeData->GetChecksum( CRCD( 0x963dc198, "skateAction" ), &skate_action )
				 && skate_action == CRCD( 0x7d9d0008, "Vert_Grab" ) )
			{
				waypoint = m_node_to;
			}
		}

	}
	else
	{
		// sort the links by priority
		Mth::Vector to_from_vector = m_wp_to - m_wp_from;
		int valid_high_priority_choices[20];
		int valid_low_priority_choices[20];
		Dbg_MsgAssert( numLinks < 20, ( "Too many links (max is 20) from node %i\n", m_node_to ) );
		for ( int i = 0; i < numLinks; i++ )
		{
			valid_high_priority_choices[i] = i;
			valid_low_priority_choices[i] = i;
		}
	
		int num_valid_low_choices = 0;
		int num_valid_high_choices = 0;
		int min_inner_angle = Script::GetInteger( CRCD( 0xfe65d822, "ped_min_inner_path_angle" ), Script::ASSERT );
		
		// store the max angle, so we can return the best choice if we don't find any good ones
		float max_angle = 0.0f;
		int max_angle_index = 0;
		for ( int i = 0; i < numLinks; i++ )
		{
			int test_node = SkateScript::GetLink( m_node_to, i );
	
			if ( test_node != m_node_from )
			{
				Script::CStruct* pNodeData = SkateScript::GetNode( test_node );

				// ignore nodes that aren't PedAI waypoints
				uint32 class_type = 0;
				uint32 waypoint_type = 0;
				pNodeData->GetChecksum( CRCD( 0x12b4e660, "class" ), &class_type );
				pNodeData->GetChecksum( CRCD( 0x7321a8d6, "Type" ), &waypoint_type );
				if ( class_type != CRCD( 0x4c23a77e, "Waypoint" ) || waypoint_type != CRCD( 0xcba10ffa, "PedAI" ) )
					continue;
				
				// ignore Grind_Bail nodes unless we're already on a grind_bail path
				if ( !on_grind_bail_path )
				{
					uint32 skate_action;
					if ( pNodeData->GetChecksum( CRCD( 0x963dc198, "SkateAction" ), &skate_action )
						 && skate_action == CRCD( 0xe41199af, "Grind_Bail" ) )
					{
						continue;
					}
				}
				
				Mth::Vector test_node_position;
				SkateScript::GetPosition( test_node, &test_node_position );
				Mth::Vector to_test_vector = m_wp_to - test_node_position;
				float angle = Mth::GetAngle( to_from_vector, to_test_vector );
				if ( angle > 360.0f )
					angle -= 360.0f * (int)( angle / 360.0f );
	
				// update max angle
				if ( angle > max_angle )
				{
					max_angle = angle;
					max_angle_index = i;
				}
		
				if ( angle >= min_inner_angle )
				{
					// sort nodes by priority
					uint32 priority;
					pNodeData->GetChecksum( CRCD( 0x9d5923d8, "priority" ), &priority, Script::ASSERT );
					switch ( priority )
					{
						case CRCC( 0xde7a971b, "normal" ):
							valid_high_priority_choices[num_valid_high_choices] = i;
							num_valid_high_choices++;
							break;
						case CRCC(0x6d77875e,"low"):
							valid_low_priority_choices[num_valid_low_choices] = i;
							num_valid_low_choices++;
							break;
						default:
							Dbg_MsgAssert( 0, ( "Ped node has invalid priority" ) );
							break;
					}
				}
			}
		}
		
		// if there weren't any good choices, pick the best one
		if ( !( num_valid_high_choices || num_valid_low_choices ) )
		{
			waypoint = SkateScript::GetLink( m_node_to, max_angle_index );
		}
		else
		{
			bool find_low_priority = false;
			// decide if we should pick from a low priority or high priority node
			if ( num_valid_high_choices && num_valid_low_choices )
			{			
				float low_priority_probability = Script::GetFloat( "ped_low_priority_waypoint_probability", Script::ASSERT );
				Dbg_MsgAssert( low_priority_probability < 1, ( "ped_low_priority_waypoint_probability is %f", low_priority_probability ) );
				if ( Mth::Rnd( 100 ) <= ( low_priority_probability * 100 ) )
					find_low_priority = true;
			}
			else if ( num_valid_low_choices )
				find_low_priority = true;
			
			// select random index from valid choices
			if ( find_low_priority )
				waypoint = SkateScript::GetLink( m_node_to, valid_low_priority_choices[Mth::Rnd( num_valid_low_choices )] );
			else
				waypoint = SkateScript::GetLink( m_node_to, valid_high_priority_choices[Mth::Rnd( num_valid_high_choices )] );
		}
	}
	
	// make sure we got something
	if ( waypoint == -1 )
	{
		Dbg_MsgAssert( 0, ( "SelectNextWaypoint couldn't find a valid waypoint" ) );
	}
	else
	{
		m_node_next = waypoint;
		SkateScript::GetPosition( m_node_next, &m_wp_next );
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPedLogicComponent::ShouldExecuteAction( Script::CStruct* pNodeData, uint32 weight_name )
{
	float weight;
	pNodeData->GetFloat( weight_name, &weight, Script::ASSERT );
	Dbg_MsgAssert( weight > 0 && weight <= 1, ( "Waypoint %s has a bad action weight of %f\n", Script::FindChecksumName( GetObject()->GetID() ), weight ) );
	if ( weight > 0.99f )
		return true;
	return ( Mth::Rnd( vACTION_WEIGHT_RESOLUTION ) < ( weight * vACTION_WEIGHT_RESOLUTION ) );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::AdjustSpeed( float percent )
{
	Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
	pMotionComp->m_max_vel *= percent;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::AdjustNormal()
{
	Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
	Dbg_Assert( pMotionComp );
	if ( pMotionComp )
	{
		Obj::STriangle last_triangle = pMotionComp->m_last_triangle;
		Mth::Vector v1 = last_triangle.mpVertices[1] - last_triangle.mpVertices[0];
		Mth::Vector v2 = last_triangle.mpVertices[2] - last_triangle.mpVertices[0];
		Mth::Vector normal = ( Mth::CrossProduct( v1, v2 ) ).Normalize();
		if ( normal[Y] < 0.0f )
			normal = -normal;

		if ( normal != m_current_normal )
		{
			m_current_normal = normal;	// remember this, for detecting if it changes
			m_last_display_normal = m_display_normal;	// remember start position for lerping
			m_normal_lerp = 1.0f;	// set lerp counter
		}

		// if m_normal_lerp is 0.0, then we don't need to do anything, as
		// we should already be there
		if ( m_normal_lerp != 0.0f )	   		// if lerping
		{			
			// If the last display normal is the same as the current normal
			// then we can't interpolate between them
			if ( m_last_display_normal == m_current_normal )
			{
				m_normal_lerp = 0.0f;
			}
			else
			{
				// adjust lerp at constant speed from 1.0 to 0.0, accounting for framerate
				// m_normal_lerp -= NORMAL_LERP_SPEED * (mp_physics->m_time * 60.0f);
				m_normal_lerp -= 0.1f * m_time * 60.0f;
		
				// if gone all the way, then clear lerping values
				// and set m_display_normal to be the current face normal
				if (m_normal_lerp <= 0.0f)
				{
					m_normal_lerp = 0.0f;
					m_display_normal = m_current_normal;
				}
				else
				{
					// Still between old and current normal, so
					// calculate intermediate normal
					m_display_normal = Mth::Lerp( m_current_normal, m_last_display_normal, m_normal_lerp );
					m_display_normal.Normalize();		// Must be normalized...
				}
			}
		}
		
		// Now update the orientation matrix.
		// We need our up (Y) vector to be this vector
		// if it changes, rotate the X and Z vectors to match
		Mth::Matrix display_matrix = GetObject()->GetDisplayMatrix();
		if ( display_matrix[Y] != m_display_normal )
		{
			display_matrix[Y] = m_display_normal;
			display_matrix.OrthoNormalizeAbout( Y );
			GetObject()->SetDisplayMatrix( display_matrix );
			m_current_display_matrix = display_matrix;
		}
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// testing out positional history
void CPedLogicComponent::AdjustHeading()
{
	AdjustHeading( m_pos_history[0] );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::AdjustHeading( Mth::Vector original_pos )
{
	Mth::Matrix mat0 = GetObject()->GetDisplayMatrix();
	Mth::Vector new_pos = GetObject()->m_pos;
	if ( m_flags & PEDLOGIC_TO_NODE_IS_LIP )
	{
		original_pos = m_wp_from;
		new_pos = m_wp_to;
	}

	// Gfx::AddDebugLine( original_pos, new_pos, MAKE_RGB( 0, 128, 0 ), MAKE_RGB( 0, 128, 0 ) );
	
	// change z vector
	mat0[Z] = ( new_pos - original_pos ).Normalize();
	// get new X and Y vectors
	mat0[X] = Mth::CrossProduct( mat0[Y], mat0[Z] );	
	mat0[Y] = Mth::CrossProduct( mat0[Z], mat0[X] );
	GetObject()->SetDisplayMatrix( mat0 );
	m_current_display_matrix = mat0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::RefreshDisplayMatrix()
{
	GetObject()->SetDisplayMatrix( m_current_display_matrix );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPedLogicComponent::do_collision_check( Mth::Vector *p_v0, Mth::Vector *p_v1, Mth::Vector *p_collisionPoint, 
								 Obj::STriangle *p_lastTriangle )
{
	// First, see if there is a collision with the last triangle, and if there is, use that.
	CFeeler feeler;
	if ( feeler.GetCollision( *p_v0, *p_v1, false ) )
	{
		Nx::CCollObj* p_col_obj=feeler.GetSector();
		Dbg_MsgAssert( p_col_obj->IsTriangleCollision(), ( "Not triangle collision !!!" ) );

		Nx::CCollObjTriData* p_tri_data=p_col_obj->GetGeometry();
		Dbg_MsgAssert( p_tri_data, ( "NULL p_tri_data" ) );

		int face_index=feeler.GetFaceIndex();
		p_lastTriangle->mpVertices[0]=p_col_obj->GetVertexPos(p_tri_data->GetFaceVertIndex(face_index, 0));
		p_lastTriangle->mpVertices[1]=p_col_obj->GetVertexPos(p_tri_data->GetFaceVertIndex(face_index, 1));
		p_lastTriangle->mpVertices[2]=p_col_obj->GetVertexPos(p_tri_data->GetFaceVertIndex(face_index, 2));

		// if there is a collision, then snap to it
		*p_collisionPoint = feeler.GetPoint();

		UpdateSkaterSoundTerrain( feeler.GetTerrain() );

		return true;
	}
	return false;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::StickToGround()
{
	// stick to ground modified from the motion component...uses
	// the ped's normal instead of the world's Y
	Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
	Dbg_Assert( pMotionComp );
	
	Mth::Vector v0, v1;

	// make sure our X vector is correct (got Z from the nav matrix...)
	GetObject()->m_matrix[X] = Mth::CrossProduct( GetObject()->m_matrix[Y], GetObject()->m_matrix[Z] );
	GetObject()->m_matrix[X].Normalize();

	Mth::Matrix display_matrix = GetObject()->GetDisplayMatrix();
	v0 = GetObject()->m_pos + ( display_matrix[Y] * m_col_dist_above );
	v1 = GetObject()->m_pos + ( -display_matrix[Y] * m_col_dist_below );
	
	Mth::Vector collision_point;
	if ( do_collision_check( &v0, &v1, &collision_point, &( pMotionComp->m_last_triangle ) ) )
	{
		GetObject()->m_pos = collision_point;
		// GetObject()->GetDisplayMatrix()[Mth::POS] = collision_point;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::UpdateSkaterSoundStates( EStateType state )
{
	Dbg_MsgAssert( m_state == CRCD( 0xa85af587, "generic_skater" ), ( "CPedLogicComponent::UpdateSkaterSoundStates called on a non-skater ped" ) );

	Obj::CSkaterLoopingSoundComponent *pLoopingSoundComponent = GetSkaterLoopingSoundComponentFromObject( GetObject() );
	if ( pLoopingSoundComponent )
		pLoopingSoundComponent->SetState( state );

	Obj::CSkaterSoundComponent *pSoundComponent = GetSkaterSoundComponentFromObject( GetObject() );
	if ( pSoundComponent )
		pSoundComponent->SetState( state );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::UpdateSkaterSoundTerrain( ETerrainType terrain )
{
	Dbg_MsgAssert( m_state == CRCD(0xa85af587,"generic_skater"), ( "CPedLogicComponent::UpdateSkaterSoundTerrain called on non-skater ped" ) );

	// set the terrain type for looping sound
	Obj::CSkaterLoopingSoundComponent* pLoopingSoundComp = GetSkaterLoopingSoundComponentFromObject( GetObject() );
	if ( pLoopingSoundComp )
		pLoopingSoundComp->SetTerrain( terrain );

	Obj::CSkaterSoundComponent *pSoundComponent = GetSkaterSoundComponentFromObject( GetObject() );
	if ( pSoundComponent )
	{
		pSoundComponent->SetTerrain( terrain );
		pSoundComponent->SetLastTerrain( terrain );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPedLogicComponent::ShouldUseBiases()
{
	return ( m_flags & PEDLOGIC_IS_SKATER
			 && !( m_flags & PEDLOGIC_GRINDING )
			 && !( m_flags & PEDLOGIC_GRIND_BAILING )
			 && !( m_flags & PEDLOGIC_TO_NODE_IS_VERT )
			 && !( m_flags & PEDLOGIC_FROM_NODE_IS_VERT )
			 && !( m_flags & PEDLOGIC_JUMPING_TO_NODE )
		   );
}

/******************************************************************/
/*																  */
/*             			Skate Actions!                            */
/*                                                                */
/******************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* CPedLogicComponent::GetSkateActionParams( Script::CStruct* pNodeData )
{
	Script::CStruct* pScriptParams = new Script::CStruct();
	// give higher priority to ped skater data - should this be reversed?	
	pScriptParams->AppendStructure( pNodeData );
	pScriptParams->AppendStructure( GetObject()->GetTags() );
	return pScriptParams;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoGrind( Script::CStruct* pNodeData )
{
	// printf("CPedLogicComponent::DoGrind\n");
	StopSkateActions();
	
	// m_is_grinding = true;
	m_flags |= PEDLOGIC_GRINDING;

	// set the terrain
	uint32 terrain_checksum;
	// TODO: Assert if terrain missing
	if ( pNodeData->GetChecksum( CRCD( 0x54cf8532, "TerrainType" ), &terrain_checksum, Script::NO_ASSERT ) )
	{
		UpdateSkaterSoundTerrain( Env::CTerrainManager::sGetTerrainFromChecksum( terrain_checksum ) );
	}

	Script::CStruct* pScriptParams = GetSkateActionParams( pNodeData );
	Script::CScript* pScript = Script::SpawnScript( CRCD( 0xb7fca430, "ped_skater_grind" ), pScriptParams );
	pScript->mpObject = GetObject();
	delete pScriptParams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
void CPedLogicComponent::GrindUpdate()
{
	m_grind_wobble_frames--;
	if ( m_grind_wobble_frames <= 0 )
	{
		printf("setting the wobble target to %f\n", m_grind_wobble_target);
		// set new wobble target
		Obj::CAnimationComponent* pAnimComp = GetAnimationComponentFromObject( GetObject() );
		Dbg_Assert( pAnimComp );
		pAnimComp->SetWobbleTarget( m_grind_wobble_target, false );
		m_grind_wobble_target = -m_grind_wobble_target;
		m_grind_wobble_frames = Script::GetInteger( "ped_skater_grind_wobble_frames", Script::ASSERT );
	}
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoGrindOff( Script::CStruct* pNodeData )
{
	if ( m_flags & PEDLOGIC_GRINDING )
	{
		// m_is_grinding = false;
		m_flags &= ~PEDLOGIC_GRINDING;
	
		Script::CScript* pScript = Script::SpawnScript( CRCD( 0x84c51e26, "ped_skater_grind_off" ) );
		pScript->mpObject = GetObject();
		// delete pScriptParams;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoJump( Script::CStruct* pNodeData, bool is_vert )
{
	StopSkateActions();
	
	// call Obj_Jump on the object
	Script::CStruct* pScriptParams;
	Script::CScript* pScript;
	if ( is_vert )
	{
		m_flags &= ~PEDLOGIC_MOVING_ON_PATH;
		pScriptParams = GetJumpParams( pNodeData, is_vert );
		pScript = Script::SpawnScript( CRCD( 0x990152d7, "ped_skater_vert_jump" ), pScriptParams );
	}
	else
	{
		if ( pNodeData->ContainsFlag( CRCD( 0xb02567ae, "JumpToNextNode" ) ) )
		{
			m_flags &= ~PEDLOGIC_MOVING_ON_PATH;
			pScriptParams = GetJumpParams( pNodeData, is_vert );
			m_flags |= PEDLOGIC_JUMPING_TO_NODE;
		}
		else
		{
			pScriptParams = GetSkateActionParams( pNodeData );
		}
		pScript = Script::SpawnScript( CRCD( 0x28be3d25, "ped_skater_jump" ), pScriptParams );
	}
	pScript->mpObject = GetObject();
	delete pScriptParams;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoLipTrick( Script::CStruct* pNodeData )
{
	StopSkateActions();
	
	// stop moving on path
	m_flags &= ~PEDLOGIC_MOVING_ON_PATH;

	UpdateLipDisplayMatrix();
	
	Script::CStruct* pScriptParams = GetSkateActionParams( pNodeData );
	Script::CScript* pScript = Script::SpawnScript( CRCD( 0x2fd2b4b8, "ped_skater_lip_trick" ), pScriptParams );
	pScript->mpObject = GetObject();
	delete pScriptParams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::UpdateLipDisplayMatrix()
{
	// set the display matrix
	Mth::Matrix mat0 = GetObject()->GetDisplayMatrix();	
	
	// change z vector
	mat0[Z] = Mth::Vector(0, 1, 0);
	mat0[Y] = m_last_display_normal.Normalize();
	// get new X and Y vectors
	mat0[X] = Mth::CrossProduct( mat0[Y], mat0[Z] ).Normalize();
	mat0[Y] = Mth::CrossProduct( mat0[Z], mat0[X] ).Normalize();
	GetObject()->SetDisplayMatrix( mat0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoManual( Script::CStruct* pNodeData )
{
	StopSkateActions();
	
	Script::CStruct* pScriptParams = GetSkateActionParams( pNodeData );
	Script::CScript* pScript = Script::SpawnScript( CRCD( 0x1462af22, "ped_skater_manual" ), pScriptParams );
	pScript->mpObject = GetObject();
	delete pScriptParams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoManualDown( Script::CStruct* pNodeData )
{
	StopSkateActions();
	
	Script::CStruct* pScriptParams = GetSkateActionParams( pNodeData );
	Script::CScript* pScript = Script::SpawnScript( CRCD( 0x4c0caa11, "ped_skater_manual_down" ), pScriptParams );
	pScript->mpObject = GetObject();
	delete pScriptParams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* CPedLogicComponent::GetJumpParams( Script::CStruct* pNodeData, bool is_vert )
{
	StopSkateActions();

	// stop moving on path 
	m_flags &= ~PEDLOGIC_DOING_VERT_ROTATION;
	m_flags &= ~PEDLOGIC_DOING_SPINE;

	// reset angles
	m_spine_current_angle = 0.0f;
	m_spine_start_angle = 0.0f;
	m_rot_current_angle = 0.0f;

	Script::CStruct* pScriptParams = GetSkateActionParams( pNodeData );
	
	// figure heading and time
	// get the height and gravity
	float height;
	pNodeData->GetFloat( CRCD( 0x838da447, "jumpHeight" ), &height, Script::ASSERT );
	float gravity = Script::GetFloat( CRCD( 0xe224a5d3, "ped_skater_jump_gravity" ), Script::ASSERT );

	float time_total;

	if ( is_vert || pNodeData->ContainsFlag( CRCD( 0xb02567ae, "JumpToNextNode" ) ) )
	{
		Mth::Vector pt = m_wp_to - GetObject()->m_pos;
		pt[Y] = 0.0f;
		
		if ( GetObject()->m_pos[Y] + height < m_wp_to[Y] )
		{
			height = m_wp_to[Y] - GetObject()->m_pos[Y] + Script::GetFloat( CRCD( 0x23f14b13, "ped_skater_jump_to_next_node_height_slop" ), Script::ASSERT );						
			// Script::PrintContents( pNodeData );
			// Dbg_MsgAssert( 0, ( "JumpToNextNode selected but jumpHeight isn't enough to reach target node." ) );
		}		
		
		// adjust height to compensate for difference between ped height
		// and height of waypoint
		// height += pt[Y];
		// pt[Y] = 0.0f;
		
		// figure the time to rise
		// d = vi*t + (a*t^2)/2
		Dbg_Assert( gravity < 0 );
		float time_to_rise = sqrtf( 2 * -height / gravity );
	
		// figure the time to fall
		float time_to_fall = sqrtf( 2 * Mth::Abs( ( GetObject()->m_pos[Y] + height - m_wp_to[Y] ) / gravity ) );
	
		// compute the inital y velocity
		float vi = ( height / time_to_rise ) - ( 0.5f * gravity * time_to_rise );
		Dbg_Assert( vi > 0 );
	
		// find the horizontal distance we need to travel
		float distance = pt.Length();
	
		// horizontal velocity
		time_total = time_to_rise + time_to_fall;
		Mth::Vector vh = ( distance / time_total ) * pt.Normalize();
	
		// vertical velocity
		Mth::Vector vy = vi * Mth::Vector(0, 1, 0);
	
		// total speed and heading
		float jump_speed = ( vh + vy ).Length();
		Mth::Vector heading = ( vh + vy ).Normalize();
	
		pScriptParams->AddVector( CRCD( 0xfd4bc03e, "heading" ), heading );
		pScriptParams->AddFloat( CRCD( 0x1c4c5690, "jumpSpeed" ), jump_speed );
		pScriptParams->AddFloat( CRCD( 0x66ced87b, "jumpTime" ), time_total );
	}
	else
	{
		// just use the jump height
		float jump_speed = sqrtf( 2 * gravity * -height );
		pScriptParams->AddFloat( CRCD(0x1c4c5690,"jumpSpeed"), jump_speed );

		time_total = 2 * sqrtf( 2 * -height * gravity );
	}

	// setup any rotation
	int rot_angle = 0;
	float rot_float;
	if ( pNodeData->ContainsFlag( CRCD( 0xb4077854, "RandomSpin" ) ) )
	{
		// figure the spin angle based on the time we've got
		float min_180_time = Script::GetFloat( CRCD( 0x8b381ab1, "ped_skater_min_180_spin_time" ), Script::ASSERT );
	
		int mult = (int)( time_total / min_180_time );
		// cap the rotation to 720 degrees
		if ( mult > 4 )
			mult = 4;
	
		rot_angle = 180 * Mth::Rnd( mult + 1 );
	}
	else if ( pNodeData->GetFloat( CRCD( 0x96fb50d9, "SpinAngle" ), &rot_float, Script::NO_ASSERT ) )
	{
		// cast to int but make sure it doesn't get rounded down
		rot_angle = (int)( rot_float + 0.01f );
	}
	
	rot_angle = abs( rot_angle );
	if ( rot_angle > 0 )
	{
		// figure fs or bs
		uint32 spin_direction;
		pNodeData->GetChecksum( CRCD( 0xef0b71a0, "SpinDirection" ), &spin_direction, Script::ASSERT );
		
		Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
		bool is_flipped = pAnimationComponent->IsFlipped();
		
		switch ( spin_direction )
		{
			case CRCC( 0x20e1c4a3, "BS" ):
			{
				if ( is_flipped )
				{
					rot_angle *= -1;
				}
				break;
			}
			case CRCC( 0x448d01a7, "FS" ):
			{
				if ( !is_flipped )
				{
					rot_angle *= -1;
				}
				break;
			}
			case CRCC( 0xe7390a8b, "Rand" ):
			{
				// randomly change direction
				int toggle = Mth::Rnd( 2 );
				if ( toggle == 1 )
				{
					rot_angle *= -1;
				}
				break;
			}
			default:
				Dbg_MsgAssert( 0, ( "Unknown SpinDirection" ) );
				break;
		}
		
		m_rot_angle = rot_angle;
		m_flags |= PEDLOGIC_DOING_VERT_ROTATION;
	}

	bool is_spine = pNodeData->ContainsFlag( CRCD( 0x4aa0cff, "SpineTransfer" ) );
	if ( rot_angle % 360 == 0 )
	{
		if ( !is_spine )
			pScriptParams->AddChecksum( NONAME, CRCD( 0x8868e76a, "should_flip" ) );
	}
	else if ( is_spine )
	{
		pScriptParams->AddChecksum( NONAME, CRCD( 0x8868e76a, "should_flip" ) );
	}

	// setup any spine trasnfer
	if ( is_spine )
	{
		m_flags |= PEDLOGIC_DOING_SPINE;
		
		// initialize spine angle to current angle between ped and vert
		Mth::Vector temp_y(0, 1, 0);
		float spine_start = Mth::GetAngle( m_current_display_matrix[Z], temp_y );
		m_spine_start_angle = spine_start + Script::GetFloat( CRCD(0x95e91d23,"ped_skater_spine_rotation_slop") );
	}

	if ( m_flags & PEDLOGIC_DOING_SPINE || m_flags & PEDLOGIC_DOING_VERT_ROTATION )
	{
		m_rot_start_matrix = m_current_display_matrix;
		m_rot_total_time = time_total * Script::GetFloat( CRCD( 0xc1dfc97b, "ped_skater_vert_rotation_time_slop" ), Script::ASSERT );
		m_rot_current_time = 0.0f;
	}

	// check if we're jumping to the next node
	if ( pNodeData->ContainsFlag( CRCD( 0xb02567ae, "JumpToNextNode" ) ) )
	{
		pScriptParams->AddFloat( CRCD( 0xa1810c27, "land_height" ), m_wp_to[Y] );
	}
	
	return pScriptParams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoGrab( Script::CStruct* pNodeData, bool is_vert )
{
	Script::CStruct* pScriptParams;
	if ( is_vert )
	{
		// Obj::CAnimationComponent* pAnimComp = GetAnimationComponentFromObject( GetObject() );
		// printf("flipped = %i\n", pAnimComp->IsFlipped() );

		m_flags &= ~PEDLOGIC_MOVING_ON_PATH;
		pScriptParams = GetJumpParams( pNodeData, is_vert );
		pScriptParams->AddChecksum( NONAME, CRCD( 0x1615618, "is_vert" ) );
	}
	else
	{
		pScriptParams = GetSkateActionParams( pNodeData );

		// see if we're already jumping
		Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
		if ( pMotionComp->m_movingobj_status & MOVINGOBJ_STATUS_JUMPING )
			pScriptParams->AddInteger( CRCD( 0x65b5788d, "is_jumping" ), 1 );
		else
		{
			// figure out how long before we start to fall
			float jumpSpeed = Script::GetFloat( CRCD( 0xf1ff758a, "ped_skater_jump_speed" ), Script::ASSERT );
			float jumpGravity = Script::GetFloat( CRCD( 0xe224a5d3, "ped_skater_jump_gravity" ), Script::ASSERT );
			float time = 2 * sqrtf( Mth::Sqr( jumpSpeed ) / Mth::Sqr( jumpGravity ) );
			pScriptParams->AddInteger( CRCD( 0x66ced87b, "jumpTime" ), time );
			pScriptParams->AddInteger( CRCD( 0x65b5788d, "is_jumping" ), 0 );
		}
	}
		
	Script::CScript* pScript = Script::SpawnScript( CRCD( 0x10585cb3, "ped_skater_grab_trick" ), pScriptParams );
	pScript->mpObject = GetObject();
	
	delete pScriptParams;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoRotations()
{
	m_rot_current_time += m_time;

	float percent = m_time / m_rot_total_time;
	if ( percent > 1.0f )
		percent = 1.0f;

	float spine_target_angle = 180.0f - m_spine_start_angle;
	
	if ( m_flags & PEDLOGIC_DOING_VERT_ROTATION )
		m_rot_current_angle += percent * (float)m_rot_angle;
	if ( m_flags & PEDLOGIC_DOING_SPINE )
		m_spine_current_angle += percent * spine_target_angle;
	
	if ( m_rot_current_time >= m_rot_total_time || percent >= 1.0f )
	{
		if ( m_flags & PEDLOGIC_DOING_VERT_ROTATION )
			m_rot_current_angle = (float)m_rot_angle;
		if ( m_flags & PEDLOGIC_DOING_SPINE )
			m_spine_current_angle = spine_target_angle;

		m_flags &= ~PEDLOGIC_DOING_VERT_ROTATION;
		m_flags &= ~PEDLOGIC_DOING_SPINE;
	}

	// cap spine angle
	if ( m_flags & PEDLOGIC_DOING_SPINE && m_spine_current_angle > spine_target_angle )
	{
		m_spine_current_angle = spine_target_angle;
	}

	Mth::Matrix temp = m_rot_start_matrix;
	temp.RotateLocal( Mth::Vector( Mth::DegToRad( m_spine_current_angle ), Mth::DegToRad( m_rot_current_angle ), 0 ) );
	GetObject()->SetDisplayMatrix( temp );
	m_current_display_matrix = temp;
	m_display_normal = m_last_display_normal = m_current_normal = temp[Y];
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::Stop( Script::CStruct* pNodeData )
{
	StopSkateActions();

	m_flags |= PEDLOGIC_STOPPED;
	
	Script::CStruct* pScriptParams = GetSkateActionParams( pNodeData );
	Script::CScript* pScript = Script::SpawnScript( CRCD( 0x365b2d85, "ped_skater_stop" ), pScriptParams );
	pScript->mpObject = GetObject();
	delete pScriptParams;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::StopSkateActions()
{
	// BB - I moved this from script because it was too damn slow!
	Script::CArray* p_scripts_to_stop = Script::GetArray( CRCD( 0x2ecd9f57, "ped_skater_action_scripts" ), Script::ASSERT );
	int array_size = p_scripts_to_stop->GetSize();
	Obj::CObject* p_object = (Obj::CObject*)GetObject();
	
	for ( int i = 0; i < array_size; i++ )
	{
		uint32 script_to_stop = p_scripts_to_stop->GetChecksum( i );
		Script::StopScriptsUsingThisObject_Proper( p_object, script_to_stop );
	}

	// m_is_grinding = false;
	m_flags &= ~PEDLOGIC_GRINDING;

	m_flags &= ~PEDLOGIC_DOING_VERT_ROTATION;
	m_flags &= ~PEDLOGIC_DOING_SPINE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::Bail()
{
	// kill any running bail scripts
	Script::StopScriptsUsingThisObject_Proper( GetObject(), CRCD(0xe630bf07,"ped_skater_grind_bail") );
	Script::StopScriptsUsingThisObject_Proper( GetObject(), CRCD(0x78131e4f,"ped_skater_generic_bail") );

	// restore the max current velocity before calling this script again!
	if ( m_original_max_vel > 0 )
	{
		Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
		Dbg_Assert( pMotionComp );
		pMotionComp->m_max_vel = m_original_max_vel;
	}
	
	Script::CScript* pScript;
	if ( m_flags & PEDLOGIC_GRINDING )
	{		
		pScript = Script::SpawnScript( CRCD(0xe630bf07,"ped_skater_grind_bail") );
	}
	else 
	{
		pScript = Script::SpawnScript( CRCD(0x78131e4f,"ped_skater_generic_bail") );
	}
	StopSkateActions();
	pScript->mpObject = GetObject();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::GrindBail()
{
	if ( m_flags & PEDLOGIC_GRINDING )
	{
		m_flags &= ~PEDLOGIC_GRINDING;
		m_flags |= PEDLOGIC_GRIND_BAILING;
	
		// select a new next node
		int num_links = SkateScript::GetNumLinks( m_node_to );
	
		// if there's only one link, we have to keep going...shitty
		if ( num_links > 1 )
		{
			// find a grind_bail node!
			for ( int i = 0; i < num_links; i++ )
			{
				int test_node = SkateScript::GetLink( m_node_to, i );
				Script::CStruct* pNodeData = SkateScript::GetNode( test_node );

				uint32 skate_action;
				if ( pNodeData->GetChecksum( CRCD( 0x963dc198, "skateAction" ), &skate_action )
					 && skate_action == CRCD( 0xe41199af, "grind_bail" ) )
				{
					m_node_next = test_node;
					SkateScript::GetPosition( m_node_next, &m_wp_next );
					break;
				}
			}
		}

		// run script!
		Script::CScript* pScript = Script::SpawnScript( CRCD( 0xe630bf07, "ped_skater_grind_bail" ) );
		pScript->mpObject = GetObject();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoRollOff( Script::CStruct* pNodeData )
{
	Script::CScript* pScript = Script::SpawnScript( CRCD( 0xb183d99e, "ped_skater_roll_off" ) );
	pScript->mpObject = GetObject();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPedLogicComponent::DoFlipTrick( Script::CStruct* pNodeData, bool is_vert )
{
	StopSkateActions();
	
	// jump if we need to
	Obj::CMotionComponent* pMotionComp = GetMotionComponentFromObject( GetObject() );
	Dbg_Assert( pMotionComp );
	Script::CStruct* pScriptParams = GetSkateActionParams( pNodeData );

	Script::CStruct *pJumpParams = GetJumpParams( pNodeData, is_vert );
	pScriptParams->AppendStructure( pJumpParams );
	delete pJumpParams;

	if ( is_vert )
	{
		m_flags &= ~PEDLOGIC_MOVING_ON_PATH;		
		pScriptParams->AddChecksum( NONAME, CRCD( 0x1615618, "is_vert" ) );
	}

	// check jumping state
	if ( pMotionComp->m_movingobj_status & MOVINGOBJ_STATUS_JUMPING )
		pScriptParams->AddInteger( CRCD( 0x65b5788d, "is_jumping" ), 1 );
	else
		pScriptParams->AddInteger( CRCD( 0x65b5788d, "is_jumping" ), 0 );

	if ( pNodeData->ContainsFlag( CRCD( 0xb02567ae, "JumpToNextNode" ) ) )
	{
		m_flags &= ~PEDLOGIC_MOVING_ON_PATH;
		m_flags |= PEDLOGIC_JUMPING_TO_NODE;
	}

	Script::CScript *pScript = Script::SpawnScript( CRCD( 0xb83c383c, "ped_skater_flip_trick" ), pScriptParams );
	pScript->mpObject = GetObject();
	delete pScriptParams;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


}
