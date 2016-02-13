//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       MotionComponent.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  11/5/2002
//****************************************************************************

// start autoduck documentation
// @DOC motioncomponent
// @module motioncomponent | None
// @subindex Scripting Database
// @index script | motioncomponent

#include <gel/components/motioncomponent.h>
#include <gel/components/pedlogiccomponent.h>
#include <gel/components/lockobjcomponent.h>
#include <gel/components/modelcomponent.h>

#include <core/math/slerp.h>
									
#include <gel/collision/collision.h>
#include <gel/object/compositeobject.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

#include <sk/engine/feeler.h>
#include <sk/objects/pathob.h>
#include <sk/objects/pathman.h>
#include <sk/scripting/nodearray.h>
#include <sk/objects/followob.h>



namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
enum
{
	UNITS_IPS,
	UNITS_FPS,
	UNITS_MPH,
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// This static function is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
CBaseComponent* CMotionComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CMotionComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CMotionComponent::CMotionComponent() : CBaseComponent()
{
	SetType( CRC_MOTION );

	mp_pathOb = NULL;
	
	mp_path_object_tracker = NULL;

	m_point_stick_to_ground = true;

	Reset();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CMotionComponent::~CMotionComponent()
{
	if ( mp_pathOb )
	{
		delete mp_pathOb;
		mp_pathOb = NULL;
	}

	// Note: No need to unregister from mp_path_object_tracker, because the path tracker
	// uses smart pointers.
	
	if ( mp_slerp )
	{
		delete mp_slerp;
		mp_slerp = NULL;
	}
	
	if (mp_follow_ob)
	{
		delete mp_follow_ob;
		mp_follow_ob=NULL;
	}	

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CMotionComponent::InitFromStructure( Script::CStruct* pParams )
{
	int nodeNum = -1;
	pParams->GetInteger( CRCD(0xe50d6573,"nodeIndex"), &nodeNum, Script::NO_ASSERT );
	m_start_node = m_current_node = nodeNum;
	OrientToNode(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CMotionComponent::ProcessWait( Script::CScript * pScript )
{
	switch (pScript->GetWaitType())
	{
		case Script::WAIT_TYPE_OBJECT_MOVE:
			if ( !IsMoving() )
			{
				pScript->ClearWait();
			}
			break;
		
		case Script::WAIT_TYPE_OBJECT_JUMP_FINISHED:
			if ( !(m_movingobj_status & MOVINGOBJ_STATUS_JUMPING) )
			{
				pScript->ClearWait();
			}
			break;

		case Script::WAIT_TYPE_OBJECT_ROTATE:
			if ( !IsRotating() )
			{
				pScript->ClearWait();
			}
			break;
		case Script::WAIT_TYPE_OBJECT_STOP_FINISHED:
			if ( !( m_vel_z > 0.0f ) )
			{
				pScript->ClearWait();
			}
			break;
		default:
			Dbg_MsgAssert(0,("\n%s\nWait type of %d not supported by CMotionComponent",pScript->GetScriptInfo(),pScript->GetWaitType()));
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CMotionComponent::Update()
{

	m_time = Tmr::FrameLength();	
	
	if ( m_movingobj_status & MOVINGOBJ_STATUS_HOVERING )
	{
		// Having these functions next to each other here does mean that objects cannot be
		// moving and hovering at the same time, but currently nothing requires that.
		UndoHover();
		ApplyHover();
	}

	if ( m_movingobj_status & MOVINGOBJ_STATUS_ROTXYZ )
	{
		RotUpdate( );
	}
	
	if ( m_movingobj_status & MOVINGOBJ_STATUS_QUAT_ROT )
	{
		QuatRot( );
	}

	if ( m_movingobj_status & MOVINGOBJ_STATUS_MOVETO )
	{
		if ( Move( ) )
		{
			m_movingobj_status &= ~MOVINGOBJ_STATUS_MOVETO;
		}
	}
	else if ( m_movingobj_status & MOVINGOBJ_STATUS_ON_PATH )
	{
		FollowPath( );
	}
	else if ( m_movingobj_status & MOVINGOBJ_STATUS_FOLLOWING_LEADER )
	{
		Dbg_MsgAssert( !(m_movingobj_status & MOVINGOBJ_STATUS_QUAT_ROT), ("Cannot have object rotating when it is following a leader"));
		Dbg_MsgAssert( !(m_movingobj_status & MOVINGOBJ_STATUS_ROTXYZ), ("Cannot have object rotating when it is following a leader"));
		FollowLeader( );
	}

	if ( m_movingobj_status & MOVINGOBJ_STATUS_JUMPING )
	{
		DoJump();
		GetObject()->m_pos = m_jump_pos;
		// m_pos[Y]=m_jump_y;
	}


	// TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
	// BIT OF A PATCH, as the skater has a motion component
	// but we don't want to set his display matrix directly here.
	if (GetObject()->GetID() > 15)
	{
		GetObject()->SetDisplayMatrix(GetObject()->GetMatrix());
	}
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMotionComponent::IsMoving()
{   
	return ( m_movingobj_status & ( MOVINGOBJ_STATUS_ON_PATH | MOVINGOBJ_STATUS_MOVETO ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMotionComponent::IsRotating()
{	
	return ( m_movingobj_status &  ( MOVINGOBJ_STATUS_ROTXYZ | MOVINGOBJ_STATUS_QUAT_ROT ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void s_get_velocity( Script::CStruct* pParams, float* pVel )
{
	if ( pParams->GetFloat( NONAME, pVel ) )
	{
		// default is mph:
		if ( pParams->ContainsFlag( 0xaad7c80f ) ) // 'fps'
		{
			*pVel = FEET_TO_INCHES( *pVel );
		}
		else if ( !( pParams->ContainsFlag( 0xa18b8f32 ) ) ) // 'ips'
		{
			// miles per hour then...
			*pVel = MPH_TO_IPS( *pVel );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void s_get_acceleration( Script::CStruct* pParams, float* pAccel )
{
	if ( pParams->GetFloat( NONAME, pAccel ) )
	{
		// default is mphps:
		if ( pParams->ContainsFlag( 0xf414a5ea ) ) // 'fpsps'
		{
			*pAccel = FEET_TO_INCHES( *pAccel );
		}
		else if ( !( pParams->ContainsFlag( 0x7644323b ) ) ) // 'ipsps'
		{
			// miles per hour per second then...
			*pAccel = MPH_TO_IPS( *pAccel );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::UndoHover( void )
{   
	Mth::Vector pos=GetObject()->GetPos();
	pos[Y]=m_y_before_applying_hover;
	GetObject()->SetPos(pos);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::ApplyHover( void )
{   
	uint32 t=Tmr::ElapsedTime(0)%m_hover_period;
	m_hover_offset=m_hover_amp*sinf(t*2*3.141592653f/m_hover_period);
	Mth::Vector pos=GetObject()->GetPos();
	m_y_before_applying_hover=pos[Y];
	pos[Y]+=m_hover_offset;
	GetObject()->SetPos(pos);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMotionComponent::IsHovering( void )
{
	return (m_movingobj_status & MOVINGOBJ_STATUS_HOVERING) && 
			// If it is doing a Quat rot, then pretend it is not hovering.
			// This is so that the rocking buoy in the shipyard is not indicated as
			// being hovering, otherwise the replay code will show it as just hovering
			// up and down. The replay code optimizes hovering objects by not recording their
			// precise positions and orientations every frame. We need that for the buoy though.
		   !(m_movingobj_status &  (MOVINGOBJ_STATUS_QUAT_ROT));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::GetHoverOrgPos( Mth::Vector* p_orgPos )
{
	Dbg_MsgAssert(p_orgPos,("NULL p_orgPos"));
	Mth::Vector pos=GetObject()->GetPos();
	p_orgPos->Set(pos[X],m_y_before_applying_hover,pos[Z]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent::EMemberFunctionResult CMotionComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | Obj_IsMoving | true if moving
		case 0x21accf0d: 		// Obj_IsMoving
			return ( IsMoving() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE );
			break;
	
        // @script | Obj_IsRotating | true if rotating
		case 0x7bc461ae: 		// Obj_IsRotating
			return ( IsRotating() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE );
			break;

        // @script | Obj_StorePos | stores the current position
		case 0x8e52abea: 		// Obj_StorePos
			m_stored_pos = GetObject()->GetPos();
			break;

        // @script | Obj_StoreNode | stores the next waypoint in the path
		case 0xc8cc6820: 		// Obj_StoreNode
			Dbg_MsgAssert( GetPathOb(),( "\n%s\nObject trying to store waypoint when not on a path.", pScript->GetScriptInfo( ) ));
			m_stored_node = GetPathOb()->m_node_to;
			break;

        // @script | Obj_StopMoving |
		case 0x68e1f022: 		// Obj_StopMoving
			m_movingobj_status &= ~( MOVINGOBJ_STATUS_MOVETO | MOVINGOBJ_STATUS_ON_PATH );
			break;

        // @script | Obj_StopRotating |
		case 0xdfbe3db3: 		// Obj_StopRotating
			m_movingobj_status &= ~( MOVINGOBJ_STATUS_ROTX | MOVINGOBJ_STATUS_ROTY |
				MOVINGOBJ_STATUS_ROTZ | MOVINGOBJ_STATUS_QUAT_ROT | MOVINGOBJ_STATUS_LOOKAT );
			break;

        // @script | Obj_SetPathVelocity | 
        // @uparm 1.0 | velocity
        // @flag mph | default units
        // @flag ips |
        // @flag fps | 
		case 0xfb603ea9:		// Obj_SetPathVelocity
			s_get_velocity( pParams, &m_max_vel );
			break;
		
        // @script | Obj_SetPathMinStopVel | Speed at which friction
        // overcomes momentum and the object stops. If left at zero,
        // the object will take forever to stop... A good value for
        // a car is 1 or 2 mph. Pedestrians are more 'sticky': make
        // it 3 or 4 mph for them. Only used if you call Obj_StopAlongPath
        // @uparm 1.0 | velocity
        // @flag mph | default units
        // @flag ips |
        // @flag fps | 
		case 0x2ca63cdf:		// Obj_SetPathMinStopVel
			s_get_velocity( pParams, &m_min_stop_vel );
			break;
		
        // @script | Obj_SetPathAcceleration | used for when the object
        // is stopped, and needs to get up to speed (set by Obj_SetVelocity).
        // @uparm 1.0 | acceleration
        // @flag mphps | default units
        // @flag ipsps |
        // @flag fpsps | 
		case 0xd6697ad0:		// Obj_SetPathAcceleration
			s_get_acceleration( pParams, &m_acceleration );
			break;

        // @script | Obj_SetPathDeceleration | used if the velocity is
        // decreased while the object is traveling, to slow down to the new speed
        // @uparm 1.0 | acceleration
        // @flag mphps | default units
        // @flag ipsps |
        // @flag fpsps | 		
		case 0xa67fc783:		// Obj_SetPathDeceleration
			s_get_acceleration( pParams, &m_deceleration );
			break;
		
        // @script | Obj_Rotate | rotate object
        // @parmopt float | time | 0.0 | in seconds - if not included, instant rotation
        // @parmopt vector | relative | (0, 0, 0) | rotate relative to the object
        // @parmopt vector | absolute | (0, 0, 0) | rotate oriented along world axes
        // @flag FLAG_MAX_COORDS | with this flag, coordinates will be converted to 
        // correspond to the axes in Max ( Z up, -Y forward, and X right. )
		case 0x5cf1b7d4:  // "Obj_Rotate"
			QuatRot_Init( pParams );
			break;
		
        // @script | Obj_RotX | rotate about x axis
        // @parmopt float | angle | | If not specified, object will rotate forever
        // @parmopt float | speed | | Speed of rotation in degrees per second. <nl>
        // Required unless deltaROT is ZERO
        // @parmopt float | acceleration | | Acceleration in degrees per
        // second per second.  If not set, velocity is set instantly
        // @parmopt float | deceleration | | If specified, velocity is set instantly.
        // Deceleration in degrees per
        // second per second.  Target stops rotating when it reaches the target
        // angle specified, or when speed reaches ZERO
        // @flag FLAG_MAX_COORDS | With this flag, coordinates will be converted to 
        // correspond to the axes in Max ( Z up, -Y forward, and X right. )
		case 0x0b231a9d:  // "Obj_RotX"
			RotAxis_Init( pParams, pScript, X );
			break;

        // @script | Obj_RotY | rotate about Y axis
        // @parmopt float | angle | | If not specified, object will rotate forever
        // @parmopt float | speed | | Speed of rotation in degrees per second. <nl>
        // Required unless deltaROT is ZERO
        // @parmopt float | acceleration | | Acceleration in degrees per
        // second per second.  If not set, velocity is set instantly
        // @parmopt float | deceleration | | If specified, velocity is set instantly.
        // Deceleration in degrees per
        // second per second.  Target stops rotating when it reaches the target
        // angle specified, or when speed reaches ZERO
        // @flag FLAG_MAX_COORDS | With this flag, coordinates will be converted to 
        // correspond to the axes in Max ( Z up, -Y forward, and X right. )
		case 0x7c242a0b:  // "Obj_RotY"
			RotAxis_Init( pParams, pScript, Y );
			break;

        // @script | Obj_RotZ | rotate about z axis
        // @parmopt float | angle | | If not specified, object will rotate forever
        // @parmopt float | speed | | Speed of rotation in degrees per second. <nl>
        // Required unless deltaROT is ZERO
        // @parmopt float | acceleration | | Acceleration in degrees per
        // second per second.  If not set, velocity is set instantly
        // @parmopt float | deceleration | | If specified, velocity is set instantly.
        // Deceleration in degrees per
        // second per second.  Target stops rotating when it reaches the target
        // angle specified, or when speed reaches ZERO
        // @flag FLAG_MAX_COORDS | With this flag, coordinates will be converted to 
        // correspond to the axes in Max ( Z up, -Y forward, and X right. )
		case 0xe52d7bb1:  // "Obj_RotZ"
			RotAxis_Init( pParams, pScript, Z );
			break;

        // @script | Obj_PathHeading | Defaults to on. Sets the
        // heading of the object in the direction of the path
        // @flag off | use to set off
		case 0xd8f4bc32:  // "Obj_PathHeading"
			if ( pParams->ContainsFlag( 0xd443a2bc ) ) // "off"
			{
				m_movingobj_flags |= MOVINGOBJ_FLAG_INDEPENDENT_HEADING;
			}
			else
			{
				m_movingobj_flags &= ~MOVINGOBJ_FLAG_INDEPENDENT_HEADING;
			}
			break;

        // @Script | Obj_SetConstantHeight | sets object height to whatever
        // height the object is currently at
        // @flag off | turn it off (default is on)
		case 0xaa3cb476:  // "Obj_SetConstantHeight"
			if ( pParams->ContainsFlag( 0xd443a2bc ) ) // "off"
			{
				m_movingobj_flags &= ~MOVINGOBJ_FLAG_CONSTANT_HEIGHT;
			}
			else
			{
				m_movingobj_flags |= MOVINGOBJ_FLAG_CONSTANT_HEIGHT;
				pParams->GetFloat( NONAME, &GetObject()->m_pos[ Y ] );
			}
			break;

        // @script | Obj_GetNextObjOnPath | Searches forward along the path the object is on for a max
		// of Range feet, and if another object on the path is encountered, its name is put into a parameter
		// called Ob. If no object is found, no Ob parameter will be created, and if it existed before, it
		// will be removed.
        // @parmopt float | Range | 0.0 | Distance to search, in feet
		case 0xb75589ce: // Obj_GetNextObjOnPath	
		{	
			float range=100.0f;
			pParams->GetFloat(CRCD(0x6c78a5b6,"Range"),&range);
			range*=12.0f;
			
			pScript->GetParams()->RemoveComponent( CRCD(0xffff9a1c,"Ob") );
			
			CCompositeObject* p_closest_ob = GetNextObjectOnPath( range );

			if ( p_closest_ob )
				pScript->GetParams()->AddChecksum( CRCD(0xffff9a1c,"Ob"), p_closest_ob->GetID() );			
			break;
		}

        // @script | Obj_GetRandomLink | Chooses a random node that the object's current node is 
		// linked to, and puts its link number into a parameter called Link.
		// If the object's current node is not linked to anything, no Link parameter will be created.
		case 0x6866312f: // Obj_GetRandomLink
		{
			uint32 num_links = SkateScript::GetNumLinks( m_current_node );
			if (num_links)
			{
				// Note: The +1 is because we're using the convention of numbering the
				// links from 1 to n rather than 0 to n-1
				pScript->GetParams()->AddInteger(CRCD(0xc953660e,"Link"),Mth::Rnd(num_links)+1);
			}
			break;
		}
			
        // @script | Obj_RandomPathMode | Causes Obj_FollowPath commands to
        // pick random links as moving objects traverse paths
        // @flag off | use to turn off - default is on
		case 0x434ea1fb:	// Obj_RandomPathMode
			if ( pParams->ContainsFlag( 0xd443a2bc ) ) // "off"
			{
				m_movingobj_flags &= ~MOVINGOBJ_FLAG_RANDOM_PATH_MODE;
			}
			else
			{
				m_movingobj_flags |= MOVINGOBJ_FLAG_RANDOM_PATH_MODE;
			}
			break;

        // @script | Obj_SetPathTurnDist | The distance from a waypoint 
        // at which the object starts turning
        // @uparm 1.0 | distance in feet
        // @flag inches | use units of inches for distance
		case 0x76003d82: // "Obj_SetPathTurnDist"
		{
			float enterTurnDist;
			if ( pParams->GetFloat( NONAME, &enterTurnDist ) )
			{
				if ( !pParams->ContainsFlag( 0xf915b4f3 ) ) // "inches"
				{
					enterTurnDist = FEET_TO_INCHES( enterTurnDist );
				}
				EnsurePathobExists( GetObject() );
				GetPathOb()->m_enter_turn_dist = enterTurnDist;
			}
			break;
		}
				
        // @script | Obj_FollowPath | Must also have Obj_SetVelocity set.
        // If the node has no links, the object will move to the node and
        // stop. Linked in a circle: object follows loop until stopped.
        // Linked to a finite path: walks to the end of the path. Returns
        // true for Obj_IsMoving until path complete. If used in conjunction
        // with Obj_MoveTo functions, will turn moveto function off, and
        // vice versa
        // @parm name | Name | name of node at beginning of path
		case 0xbfb0031d:  // "Obj_FollowPath"
			m_movingobj_flags &= ~MOVINGOBJ_FLAG_STOP_ALONG_PATH;
			FollowPath_Init( pParams );
			break;

        // @script | Obj_LookAtNode | 
        // @parm name | Name | the name of the node you want to look at
        // @parmopt name | lockAxis | | can lock any two axes (LOCK_X, LOCK_Y, LOCK_Z)
        // @parmopt float | time | vel/acceleration | in seconds
        // @parmopt float | speed | 360  | angular velocity in degrees per second
        // @parmopt float | acceleration | 0 | angular acceleration in degrees per second
        // per second 
        // @flag FLAG_MAX_COORDS | With this flag, coordinates will be converted to 
        // correspond to the axes in Max ( Z up, -Y forward, and X right. )
		case 0x103bd253:  // "Obj_LookAtNode"
			LookAtNode_Init( pParams, pScript );
			break;

		// @script | Obj_LookAtNodeLinked | looks at the next node
        // @parmopt int | linkNum | 1 | The link to look at.
		case 0x6419ceff: // "Obj_LookAtNodeLinked"
			LookAtNodeLinked_Init( pParams, pScript );
			break;

        // @script | Obj_LookAtNodeStored | looks at stored node
        // @parmopt name | lockAxis | | can lock any two axes (LOCK_X, LOCK_Y, LOCK_Z)
        // @parmopt float | time | vel/acceleration | in seconds
        // @parmopt float | speed | 360  | angular velocity in degrees per second
        // @parmopt float | acceleration | 0 | angular acceleration in degrees per second
        // per second 
		// @parmopt float | AngleThreshold | 0 | If the object is already looking within this
		// many degrees of the correct direction, it will not bother turning.
        // @flag FLAG_MAX_COORDS | With this flag, coordinates will be converted to 
        // correspond to the axes in Max ( Z up, -Y forward, and X right. )
		case 0xa5a2d218: // "Obj_LookAtNodeStored"
		{
			Mth::Vector lookAtPos;
			SkateScript::GetPosition( m_stored_node, &lookAtPos );
			LookAt_Init( pParams, lookAtPos );
			break;
		}

        // @script | Obj_LookAtPos |
        // @uparm (0, 0, 0) | position vector
        // @parmopt name | lockAxis | | can lock any two axes (LOCK_X, LOCK_Y, LOCK_Z)
        // @parmopt float | time | vel/acceleration | in seconds
        // @parmopt float | speed | 360  | angular velocity in degrees per second
        // @parmopt float | acceleration | 0 | angular acceleration in degrees per second
        // per second
		case 0x74c30242:  // "Obj_LookAtPos"
			LookAtPos_Init( pParams, true );
			break;

        // @script | Obj_LookAtPosStored | look at stored pos
        // @parmopt name | lockAxis | | can lock any two axes (LOCK_X, LOCK_Y, LOCK_Z)
        // @parmopt float | time | vel/acceleration | in seconds
        // @parmopt float | speed | 360  | angular velocity in degrees per second
        // @parmopt float | acceleration | 0 | angular acceleration in degrees per second
        // per second
		// @parmopt float | AngleThreshold | 0 | If the object is already looking within this
		// many degrees of the correct direction, it will not bother turning.
		case 0xe8d121cb: // "Obj_LookAtPosStored"
			LookAt_Init( pParams, m_stored_pos );
			break;

		// @script | Obj_LookAtRelPos | look at position relative to object position
        // @uparm (0, 0, 0) | relative position vector
        // @parmopt name | lockAxis | | can lock any two axes (LOCK_X, LOCK_Y, LOCK_Z)
        // @parmopt float | time | vel/acceleration | in seconds
        // @parmopt float | speed | 360  | angular velocity in degrees per second
        // @parmopt float | acceleration | 0 | angular acceleration in degrees per second
        // per second
        case 0x33df698f:  // "Obj_LookAtRelPos"
			LookAtPos_Init( pParams, false );
			break;

        // @script | Obj_StopAlongPath | accurately stop and object
        // @uparm 1.0 | distance in feet.  Note: doesn't use deceleration
        // set in Obj_SetDeceleration
		case 0xfe71453f:  // "Obj_StopAlongPath"
			m_stop_dist = 0;
			m_stop_dist_traveled = 0;
			if ( !m_min_stop_vel )
			{
				m_min_stop_vel = 15;
			}
			pParams->GetFloat( NONAME, &m_stop_dist );
			if ( !pParams->ContainsFlag( 0xf915b4f3 ) ) // inches
			{
				m_stop_dist = FEET_TO_INCHES( m_stop_dist );
				m_vel_z_start = m_vel_z;
			}
			if ( !m_stop_dist )
			{
				m_vel_z = 0;
			}
			m_movingobj_flags |= MOVINGOBJ_FLAG_STOP_ALONG_PATH;
			break;
		
        // @script | Obj_StartAlongPath | Only needs to be used if you've
        // stopped an object along a path, to restart the object moving. 
		case 0xbd6c6807:  // "Obj_StartAlongPath"
			m_movingobj_flags &= ~MOVINGOBJ_FLAG_STOP_ALONG_PATH;
			break;
		
        // @script | Obj_FollowPathLinked | Will cause the object to
        // follow the path that it is linked to.
        // When calling Obj_FollowPathLinked, you can add an
        // optional parameter: <nl>
        // originalNode 
        // to follow a linked path to the node that created the object. <nl>
        // If random mode is on, a random link from the original
        // node will be used
        // @flag originalNode | follow a linked path to the node that 
        // created the object
		case 0x0da2d86d:  // "Obj_FollowPathLinked"
			m_movingobj_flags &= ~MOVINGOBJ_FLAG_STOP_ALONG_PATH;
			FollowPathLinked_Init( pParams, pScript );
			break;

        // @script | Obj_FollowPathStored | Follow the path from current
        // position to the position of the waypoint last stored
        // in the script call to Obj_StoreNode (when a pedestrian, for example, has to jump
        // out of the way of the player, he stores his current position and the current node
        // that he's heading for... then he walks back to his position, and finally ends up
        // heading to the waypoint he was heading for before he left the path to jump out of the way).
		case 0xcc19c48a: // "Obj_FollowPathStored"
			FollowStoredPath_Init( pParams );
			break;

        // @script | Obj_SetGroundOffset | 
		case 0xbf9fffe5: // "Obj_SetGroundOffset"
			Dbg_Message( "Obj_SetGroundOffset is obsolete!" );
            break;
		
        // @script | Obj_MoveToPosStored | move to world position stored
        // using Obj_StorePos
        // @parmopt float | time | | default unit is milliseconds
        // @parmopt float | speed | | used instead of time and acceleration
        // @parmopt float | acceleration | | change in speed per second
		case 0x8b669799: // "Obj_MoveToPosStored"
			Move_Init( pParams, pScript, m_stored_pos );
			break;

        // @script | Obj_MoveForward | Moves a distance in the direction the object is facing.
        // @parmopt float | dist | | Distance to move, in inches.
        // @parmopt float | time | | default unit is milliseconds
        // @parmopt float | speed | | used instead of time and acceleration
        // @parmopt float | acceleration | | change in speed per second
		case 0xf389285d: // Obj_MoveForward
		{
			float dist=0.0f;
			pParams->GetFloat(CRCD(0x7e832f08,"Dist"),&dist);
			Move_Init( pParams, pScript, GetObject()->GetPos() + GetObject()->m_matrix[Z] * dist );	 
			break;
		}

        // @script | Obj_MoveLeft | Moves a distance at 90 degrees to the direction the object is facing.
        // @parmopt float | dist | | Distance to move, in inches.
        // @parmopt float | time | | default unit is milliseconds
        // @parmopt float | speed | | used instead of time and acceleration
        // @parmopt float | acceleration | | change in speed per second
		case 0x5ce058fd: // Obj_MoveLeft
		{
			float dist=0.0f;
			pParams->GetFloat(CRCD(0x7e832f08,"Dist"),&dist);
			Move_Init( pParams, pScript, GetObject()->GetPos() + GetObject()->m_matrix[X] * dist );	 
			break;
		}
			
        // @script | Obj_MoveToNode | move to any node by node name
        // @parm name | name | node name
        // @parmopt float | time | | default unit is milliseconds
        // @parmopt float | speed | | used instead of time and acceleration
        // @flag mph | time in mph
        // @flag IPS | time in IPS
        // @flag fps | time in fps
        // @parmopt float | acceleration | | change in speed per second
		case 0x8819dd8b:  // "Obj_MoveToNode"
			MoveToNode_Init( pParams, pScript );
			break;
			
        // @script | Obj_MoveToPos | moves to the world position specified
        // by the vector
        // @uparm (0, 0, 0) | position vector 
        // @parmopt float | time | | default unit is milliseconds
        // @parmopt float | speed | | used instead of time and acceleration
        // @parmopt float | acceleration | | change in speed per second
		case 0x84ec6600:  // "Obj_MoveToPos"
			MoveToPos_Init( pParams, pScript, true );
			break;

        // @script | Obj_MoveToRelPos | moves to a position relative to the object
        // @uparm (0, 0, 0) | position vector 
        // @parmopt float | time | | default unit is milliseconds
        // @parmopt float | speed | | used instead of time and acceleration
        // @parmopt float | acceleration | | change in speed per second
		case 0xea81a32b:  // "Obj_MoveToRelPos"
			MoveToPos_Init( pParams, pScript, false );
			break;
		
        // @script | Obj_MoveToLink | Moves to the 1st node that this object is linked to 
        // @parmopt float | time | | default unit is milliseconds
        // @parmopt float | speed | | used instead of time and acceleration
        // @parmopt float | acceleration | | change in speed per second
		// @parm int | LinkNum | link number
        case 0x3bcaac3f:  // "Obj_MoveToLink"
			MoveToLink_Init( pParams, pScript );
			break;
	
        // @script | Obj_StickToGround | 
        // @flag on | turn on (optional - default value is on)
        // @flag off | turn off (default is on)
        // @parm float | distAbove | distance above the object to check
        // for collision
        // @parm float | distBelow | distance below the object to check
        // for collision
		case 0xc5eca638:  // "Obj_StickToGround"
			pParams->GetFloat( 0xa8a5a3c7, &m_col_dist_above ); // "distAbove"
			pParams->GetFloat( 0x182d87a7, &m_col_dist_below ); // "distBelow"
			if ( pParams->ContainsFlag( 0xd443a2bc ) ) // "off"
			{
				m_movingobj_flags &= ~MOVINGOBJ_FLAG_STICK_TO_GROUND;
			}
			else
			{
				m_movingobj_flags |= MOVINGOBJ_FLAG_STICK_TO_GROUND;
				if ( ( m_col_dist_above == 0 ) && ( m_col_dist_below == 0 ) )
				{
					Dbg_Message( "\n%s\nTrying to stick to the ground, distAbove and distBelow are zero!!!", pScript->GetScriptInfo() );
					return CBaseComponent::MF_FALSE;
				}
				StickToGround();
			}
			break;

        // @script | Obj_WaitJumpFinished | Waits for jump to finish, which is when the object 
		// has fallen to the same height it was at when the Obj_Jump command was issued.
		case 0x1e094b84:  // Obj_WaitJumpFinished
		{
			pScript->SetWait(Script::WAIT_TYPE_OBJECT_JUMP_FINISHED,this);
			break;
		}

        // @script | Obj_WaitRotate | wait for rotation to complete
		case 0xbdc4c878:  // Obj_WaitRotate
			pScript->SetWait(Script::WAIT_TYPE_OBJECT_ROTATE,this);
			break;

        // @script | Obj_WaitMove | use to wait for move to complete
		case 0xdbfdd02f:  // Obj_WaitMove
			pScript->SetWait(Script::WAIT_TYPE_OBJECT_MOVE,this);
			break;

		// @script | Obj_WaitStop | use to wait for the object to come to a complete stop
		case CRCC( 0x8d95f1e1, "Obj_WaitStop" ):
			pScript->SetWait(Script::WAIT_TYPE_OBJECT_STOP_FINISHED,this);
			break;

		// @script | Obj_SkipToRestart | skips the object to a restart point
		case 0x3407ad3d: // Obj_SkipToRestart
		{
			uint32 node_name=0;
			Mth::Vector node_pos;
			
			pParams->GetChecksum(NONAME,&node_name);
			int node_index = SkateScript::FindNamedNode(node_name);
			Script::CStruct* p_node_data = SkateScript::GetNode(node_index);
			
			SkateScript::GetPosition(p_node_data, &node_pos);
			GetObject()->SetPos( node_pos );
			this->OrientToNode(p_node_data);
		
			// refresh the model with the new pos/orientation
			Obj::CModelComponent* pModelComponent = (Obj::CModelComponent*)GetModelComponentFromObject( GetObject() );
			pModelComponent->Update();
		}
		break;

		case 0xe606eba2: // Obj_Hover
		{
			if (pParams->ContainsFlag(CRCD(0xd443a2bc,"Off")))
			{
				UndoHover();
				m_movingobj_status &= ~MOVINGOBJ_STATUS_HOVERING;
			}
			else
			{
				m_hover_offset=0;
				
				pParams->GetFloat(CRCD(0xc9fde32c,"Amp"),&m_hover_amp);
				float f=1.0f;
				pParams->GetFloat(CRCD(0xa80bea4a,"Freq"),&f);
				m_hover_period=(int)(1000.0f/f);
				// Add 10% of randomness so that groups of hovering things slowly
				// go out of phase with each other rather than hovering in unison.
				int random_number = Mth::Rnd(m_hover_period*20/10);
				m_hover_period += random_number-m_hover_period/10;

//				printf( "hover period = %d random number = %d\n", m_hover_period, random_number );

				if ( m_hover_period <= 0 )
				{
					// GJ:  make sure that hover period is a valid value
					Dbg_MsgAssert( false, ( "Potential divide-by-zero m_hover_period=%d at %s...  tell Mick!", m_hover_period, pScript->GetScriptInfo() ) );
					m_hover_period = 1;
				}
				
				m_y_before_applying_hover=GetObject()->GetPos()[Y];
				m_movingobj_status |= MOVINGOBJ_STATUS_HOVERING;
			}	
			break;
		}
		
        // @script | Obj_GetSpeed | Gets the object's current speed and puts it into a parameter called speed.
		// By default the units will be miles per hour.
		// @flag ips | Make the speed value be in inches per second
		// @flag fps | Make the speed value be in feet per second
		case 0xe76d73d2: // Obj_GetSpeed 
		{
			float speed=0.0f;
			if (m_movingobj_status & MOVINGOBJ_STATUS_MOVETO)
			{
				speed=m_moveto_speed;
			}
			else if ( GetLockObjComponentFromObject(GetObject()) && GetLockObjComponentFromObject(GetObject())->IsLockEnabled() )
			{
				Mth::Vector d=GetObject()->m_pos - GetObject()->m_old_pos;
				float m_time = Tmr::FrameLength();	 	// dodgy
				if (m_time)
				{
					speed=d.Length()/m_time;
				}	
			}
			else
			{
				speed=GetObject()->m_vel.Length();
			}
			
			// speed is now in inches per sec.
			
			if ( pParams->ContainsFlag( 0xa18b8f32 ) ) // "ips"
			{
				// Nothing to do.
			}
			else if ( pParams->ContainsFlag( 0xaad7c80f ) ) // "fps"
			{
				// Convert to feet per second
				speed/=FEET_TO_INCHES(1.0f);
			} 
			else
			{
				// Convert to miles per hour
				speed/=MPH_TO_INCHES_PER_SECOND(1.0f);
			}
				
			pScript->GetParams()->AddFloat("Speed",speed);
			break;
		}		


        // @script | Obj_Jump | Makes the object jump.
		// @parmopt float | Speed | 150.0 | The upwards speed of the jump
		// @parmopt float | Gravity | -360.0 | The gravity applied to the jump
		// @parmopt vector | heading | | use this heading as jump direction rather than
		// jumping straight up
		// @parmopt float | col_dist_above | 6 | distance used for collision checks above the object
		// @parmopt float | col_dist_below | 6 | distance used for collision checks below the object
		// @flag use_current_heading | use the object's current Z vector as jump direction
		case 0x1ae46261:  // Obj_Jump
		{
			m_jump_speed = 150.0f;
			pParams->GetFloat( CRCD(0xf0d90109,"Speed"), &m_jump_speed );
			m_jump_original_speed = m_jump_speed;

			m_jump_gravity = -360.0f;
			pParams->GetFloat( CRCD(0xa5e2da58,"Gravity"), &m_jump_gravity );

			m_jump_start_pos = GetObject()->m_pos;

			m_jump_col_dist_above = 6.0f;
			pParams->GetFloat( CRCD( 0xecc2c699, "col_dist_above" ), &m_jump_col_dist_above, Script::NO_ASSERT );
			m_jump_col_dist_below = 6.0f;
			pParams->GetFloat( CRCD( 0x5c4ae2f9, "col_dist_below" ), &m_jump_col_dist_below, Script::NO_ASSERT );
			
			if ( pParams->GetVector( CRCD( 0xfd4bc03e, "heading" ), &m_jump_heading, Script::NO_ASSERT ) )
			{
				m_jump_use_heading = true;
				m_jump_speed *= m_jump_heading[Y];
				m_jump_heading[Y] = 0.0f;
			}
			else if ( pParams->ContainsFlag( CRCD( 0xfecffe49, "use_current_heading" ) ) )
			{
				// m_jump_heading = Mth::Vector(0, 0, 1);
				m_jump_heading = GetObject()->GetDisplayMatrix()[Z];
				m_jump_use_heading = true;
				m_jump_speed *= m_jump_heading[Y];
				m_jump_heading[Y] = 0.0f;				
			}
			else
			{				
				m_jump_use_heading = false;
			}

			m_jump_use_land_height = false;
			if ( pParams->GetFloat( CRCD( 0xa1810c27, "land_height" ), &m_jump_land_height, Script::NO_ASSERT ) )
			{
				m_jump_use_land_height = true;
			}

			m_movingobj_status |= MOVINGOBJ_STATUS_JUMPING;
			break;
		}
		
        // @script | Obj_FollowLeader | Makes the object follow a fixed distance behind another object.
        // @parm name | Name | Name of the object to follow
        // @parmopt float | Distance | 100 | Distance behind the leader in inches
		// @flag OrientY | Tilt the object to be at right angles to the path direction
		// @flag Off | Switch off following
		// @flag LeaveYUnaffected | The y coordinate of the object will be left unaffected, so
		// that the object can be clamped to the ground and not follow the skater into the air
		// for example.
		case 0x2fad370d: // Obj_FollowLeader
			FollowLeader_Init( pParams );
			break;


		

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::EnsurePathobExists( CCompositeObject* pCompositeObject )
{
	if ( !mp_pathOb )
	{
		mp_pathOb = new CPathOb( pCompositeObject );
		Dbg_MsgAssert( mp_pathOb,( "Couldn't allocate pathob." ));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPathOb* CMotionComponent::GetPathOb()
{
	return mp_pathOb;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Added by Ken. If the object is following a path, this will return the node
// index of the node it is heading towards. Returns -1 otherwise.
int CMotionComponent::GetDestinationPathNode()
{	
	if ( !(m_movingobj_status & MOVINGOBJ_STATUS_ON_PATH) )
	{
		return -1;
	}	

	if ( !mp_pathOb )
	{
		return -1;
	}	
	//Dbg_MsgAssert(mp_pathOb,("NULL GetPathOb() ?"));
	return mp_pathOb->m_node_to;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CMotionComponent::GetPreviousPathNode()
{	
	if ( !(m_movingobj_status & MOVINGOBJ_STATUS_ON_PATH) )
	{
		return -1;
	}	

	if ( !mp_pathOb )
	{
		return -1;
	}	
	//Dbg_MsgAssert(mp_pathOb,("NULL GetPathOb() ?"));
	return mp_pathOb->m_node_from;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CMotionComponent::Reset()
{
	m_turn_ang_target.Set( );
	m_cur_turn_ang.Set( );
	m_turn_speed_target.Set( );
	m_turn_speed.Set( );
	m_delta_turn_speed.Set( );
	m_angular_friction.Set( );
	m_orig_pos.Set( );
	
	m_moveto_pos.Set( );
	m_moveto_dist = 0.0f;
	m_moveto_dist_traveled = 0.0f;
	m_moveto_speed = 0.0f;
	m_moveto_speed_target = 0.0f;
	m_moveto_acceleration = 0.0f;
	
	m_acceleration = 0.0f;
	m_deceleration = 0.0f;
	m_max_vel = 0.0f;
	m_vel_z = 0.0f;
	m_stop_dist = 0.0f;
	m_stop_dist_traveled = 0.0f;
	m_vel_z_start = 0.0f;
	m_min_stop_vel = 0.0f;
	
	m_rot_time_target = 0.0f;
	m_rot_time = 0.0f;
	
	// don't think this is dangerous...
	m_movingobj_status = 0;
	m_movingobj_flags = 0;
	
	// Reset the Triangle collision caches
	
	m_last_triangle.Init();
	m_car_left_rear_last_triangle.Init();
	m_car_right_rear_last_triangle.Init();
	m_car_mid_front_last_triangle.Init();

	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::SetCorrectRotationDirection( int rotAxis )
{
	
	m_turn_speed[ rotAxis ] = Mth::Abs( m_turn_speed[ rotAxis ] );
	m_turn_speed_target[ rotAxis ] = Mth::Abs( m_turn_speed_target[ rotAxis ] );
	m_delta_turn_speed[ rotAxis ] = Mth::Abs( m_delta_turn_speed[ rotAxis ] );
	m_angular_friction[ rotAxis ] = Mth::Abs( m_angular_friction[ rotAxis ] );
	if ( m_turn_ang_target[ rotAxis ] < 0 )
	{
		m_turn_speed[ rotAxis ] *= -1.0f;
		m_turn_speed_target[ rotAxis ] *= -1.0f;
		m_delta_turn_speed[ rotAxis ] *= -1.0f;
		m_angular_friction[ rotAxis ] *= -1.0f;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::InitPath( int nodeNumber )
{
	if (mp_path_object_tracker)
	{
		mp_path_object_tracker->StopTrackingObject( GetObject() );
	}	

	mp_path_object_tracker=Obj::CPathMan::Instance()->TrackObject( GetObject(), nodeNumber );	
	// Note: mp_path_object_tracker could be NULL
	
	EnsurePathobExists( GetObject() );
	GetPathOb()->NewPath( nodeNumber, GetObject()->GetPos() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::FollowPath_Init( Script::CStruct* pParams )
{   
	uint32 nodeChecksum;
	if ( pParams->GetChecksum( 0xa1dc81f9, &nodeChecksum ) ) // "name"
	{
		int nodeNum = SkateScript::FindNamedNode( nodeChecksum );
		InitPath( nodeNum );
		m_movingobj_status |= MOVINGOBJ_STATUS_ON_PATH;
		// these can't coexist:
		m_movingobj_status &= ~MOVINGOBJ_STATUS_MOVETO;
	}
	else
	{
		Dbg_MsgAssert( 0,( "Must specify name of node in Obj_FollowPath, using name = nodeName." ));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMotionComponent::PickRandomWaypoint()
{
	return ( m_movingobj_flags & MOVINGOBJ_FLAG_RANDOM_PATH_MODE );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::HitWaypoint( int nodeIndex )
{	
	uint32 scriptChecksum;
	Script::CStruct* pNodeData;
	pNodeData = SkateScript::GetNode( nodeIndex );
	Dbg_MsgAssert( pNodeData,( "Waypoint gave us a bad node index." ));
	m_current_node = nodeIndex;
	
	if ( pNodeData->GetChecksum( 0x3aefe377, &scriptChecksum ) ) 	// "SpawnObjScript"
	{
		Script::CStruct* pScriptParams = NULL;
		pNodeData->GetStructure( CRCD(0x7031f10c,"Params"), &pScriptParams );
#ifdef __NOPT_ASSERT__
		Script::CScript* p_script=GetObject()->SpawnScriptPlease( scriptChecksum, pScriptParams );
		p_script->SetCommentString("Created by CMotionComponent::HitWaypoint");
#else
		GetObject()->SpawnScriptPlease( scriptChecksum, pScriptParams );
#endif
		return;
	}
	
	if ( pNodeData->GetChecksum( 0x86125749, &scriptChecksum ) ) 	// "SwitchObjScript"
	{
		Script::CStruct* pScriptParams = NULL;
		pNodeData->GetStructure( CRCD(0x7031f10c,"Params"), &pScriptParams );
		GetObject()->SwitchScript( scriptChecksum, pScriptParams );
		return;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMotionComponent::LookAt_Init( Script::CStruct* pParams, const Mth::Vector& lookAtPos )
{	
	int deltaDegreesPerSecond = 360;
	int deltaDeltaDegreesPerSecond = 0;
	int lockAxis = ( 1 << Y );
	float time = 0.0f;
	
	if ( !( m_movingobj_status & MOVINGOBJ_STATUS_LOOKAT ) )
	{
		m_movingobj_status |= MOVINGOBJ_STATUS_LOOKAT;
		m_movingobj_status &= ~MOVINGOBJ_STATUS_ROTXYZ;
		m_movingobj_flags &= ~( MOVINGOBJ_FLAG_CONST_ROTX | MOVINGOBJ_FLAG_CONST_ROTY | MOVINGOBJ_FLAG_CONST_ROTZ );
	}

	pParams->GetFloat( 0x906b67ba, &time ); // "time"
	if ( !time )
	{
		pParams->GetInteger( 0xf0d90109, &deltaDegreesPerSecond );		// 'speed'
		pParams->GetInteger( 0x85894a84, &deltaDeltaDegreesPerSecond );	// 'acceleration'
		pParams->GetInteger( 0xa4aecb20, &lockAxis );  // "lockAxis"
	}
	
	float threshold=0.0f;
	pParams->GetFloat(0xf73a4efd,&threshold); // AngleThreshold
	threshold=DEGREES_TO_RADIANS(threshold);
	
	bool rotation_required=false;
	int i;
	for ( i = 0; i < 3; i++ )
	{
		if ( lockAxis & ( 1 << i ) )
		{
			if ( SetUpLookAtPos( lookAtPos, GetObject()->GetPos(), i == Z ? Y : Z, i, threshold ) )
			{
				if ( time )
				{
					m_delta_turn_speed[ i ] = 0.0f;
					m_turn_speed[ i ] = m_turn_ang_target[ i ] / time;
				}
				else
				{
					m_delta_turn_speed[ i ] = DEGREES_TO_RADIANS( deltaDeltaDegreesPerSecond );
					if ( !m_delta_turn_speed[ i ] )
						m_turn_speed[ i ] = DEGREES_TO_RADIANS( deltaDegreesPerSecond );
					else
					{
						m_turn_speed[ i ] = 0.0f;
						m_turn_speed_target[ i ] = 0.0f;
					}
					SetCorrectRotationDirection( i );
				}
				rotation_required=true;
			}
		}
	}
	return rotation_required;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::QuatRot_Setup( Script::CStruct* pParams, const Mth::Matrix& rot )
{
	if ( !mp_slerp )
	{
		mp_slerp = new Mth::SlerpInterpolator;
	}

    Mth::Matrix tempMatrix = rot;
	mp_slerp->setMatrices( &GetObject()->GetMatrix(), &tempMatrix );
	
	m_rot_time = 0.0f;
	m_rot_time_target = 0.0f;
	pParams->GetFloat( 0x906b67ba, &m_rot_time_target ); // "time" ( in seconds )
	m_movingobj_status |= MOVINGOBJ_STATUS_QUAT_ROT;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::QuatRot_Init( Script::CStruct* pParams )
{
	Mth::Vector rot( 0.0f, 0.0f, 0.0f );
	if ( pParams->GetVector( 0x91a4c826, &rot ) )  // relative
	{
		if ( pParams->ContainsFlag( 0x26af9dc9 ) ) // "FLAG_MAX_COORDS"
		{
			rot[ Z ] = -rot[ Z ];
			rot.ConvertToMaxCoords( );
		}
		rot.DegreesToRadians( );
		Mth::Matrix temp = GetObject()->GetMatrix();
		temp.RotateLocal( rot );
		QuatRot_Setup( pParams, temp );
	}
	else if ( pParams->GetVector( 0x592089a9, &rot ) ) //absolute
	{
		if ( pParams->ContainsFlag( 0x26af9dc9 ) ) // "FLAG_MAX_COORDS"
		{
			rot[ Z ] = -rot[ Z ];
			rot.ConvertToMaxCoords( );
		}
		rot.DegreesToRadians( );
		Mth::Matrix temp( rot[ X ], rot[ Y ], rot[ Z ] );
		QuatRot_Setup( pParams, temp );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMotionComponent::SetUpLookAtPos( const Mth::Vector& lookToPos, const Mth::Vector& currentPos, int headingAxis, int rotAxis, float threshold )
{
	Mth::Vector pathHeading = lookToPos - currentPos;
	
	m_cur_turn_ang[ rotAxis ] = 0.0f;
	m_turn_ang_target[ rotAxis ] = Mth::GetAngle( GetObject()->GetMatrix(), pathHeading, headingAxis, rotAxis );
	if ( fabs(m_turn_ang_target[ rotAxis ]) > threshold )
	{
		m_movingobj_status |= ( MOVINGOBJ_STATUS_ROTX << rotAxis );
/*		Dbg_MsgAssert( ( ( !( m_movingobj_status & MOVINGOBJ_STATUS_ON_PATH ) ) ||
						( m_movingobj_flags & MOVINGOBJ_FLAG_INDEPENDENT_HEADING ) ) ||
						rotAxis == Z,( "Can't rotate on X or Y axis while using auto-heading on path." ));*/
		return true;
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::RotAxis_Init( Script::CStruct* pParams, Script::CScript* pScript, int rotAxis )
{
	
	float mult = 1.0f;

	// Mick - Just being safe here.  	
	Dbg_MsgAssert( rotAxis == X || rotAxis == Y || rotAxis== Z,( "Illegal rotAxis %d", rotAxis ));
	
	
	// get out of lookat mode... and shit.
	if ( m_movingobj_status & MOVINGOBJ_STATUS_LOOKAT )
	{
		m_movingobj_status &= ~MOVINGOBJ_STATUS_LOOKAT;
		m_movingobj_status &= ~MOVINGOBJ_STATUS_ROTXYZ;
	}
	if ( pParams->ContainsFlag( 0x26af9dc9 ) ) // "FLAG_MAX_COORDS"
	{
		switch ( rotAxis )
		{
			case ( X ):
				mult = -1.0f;
				break;

			case ( Y ):
				rotAxis = Z;
//				mult = -1.0f;
				break;

			case ( Z ):
				rotAxis = Y;
				break;
			
			default:
				Dbg_MsgAssert( 0,( "Illegal rotAxis" ));
				break;
		}
	}

	// clear this in case it isn't set...
	m_angular_friction[ rotAxis ] = 0.0f;
	m_delta_turn_speed[ rotAxis ] = 0.0f;
	m_turn_ang_target[ rotAxis ] = 0.0f;

	if ( pParams->GetFloat( 0xff7ebaf6, &m_turn_ang_target[ rotAxis ] ) ) // "angle"
	{
		m_turn_ang_target[ rotAxis ] = mult * DEGREES_TO_RADIANS( m_turn_ang_target[ rotAxis ] );
		m_movingobj_flags &= ~( MOVINGOBJ_FLAG_CONST_ROTX << rotAxis );
		m_cur_turn_ang[ rotAxis ] = 0.0f;
		m_turn_speed[ rotAxis ] = 0.0f;
		if ( !m_turn_ang_target[ rotAxis ] )
		{
			// stop the rotation!
			m_movingobj_status &= ~( MOVINGOBJ_STATUS_ROTX << rotAxis );
			return;
		}
	}
	
	if ( pParams->GetFloat( 0xf59ff7d7, &m_angular_friction[ rotAxis ] ) ) // "deceleration"
	{
		// K: If a deceleration is specified, the 'speed' value specified is the target speed.
		// The current m_turn_speed remains unchanged so that it slows down to the target speed.
		// (TT427)
		m_angular_friction[ rotAxis ] = DEGREES_TO_RADIANS( m_angular_friction[ rotAxis ] );
		if ( pParams->GetFloat( 0xf0d90109, &m_turn_speed_target[ rotAxis ] ) ) // "speed"
		{
			m_turn_speed_target[ rotAxis ] = DEGREES_TO_RADIANS( m_turn_speed_target[ rotAxis ] );
		}
		else
		{
			Dbg_MsgAssert( 0,( "\n%s\nMust specify speed for Obj_Rot functions (degrees per second).", pScript->GetScriptInfo( ) ));
		}
	}
	else if ( pParams->GetFloat( 0x85894a84, &m_delta_turn_speed[ rotAxis ] ) ) // acceleration
	{
		m_delta_turn_speed[ rotAxis ] = DEGREES_TO_RADIANS( m_delta_turn_speed[ rotAxis ] );
		if ( pParams->GetFloat( 0xf0d90109, &m_turn_speed_target[ rotAxis ] ) ) // "speed"
		{
			m_turn_speed_target[ rotAxis ] = DEGREES_TO_RADIANS( m_turn_speed_target[ rotAxis ] );
		}
		else
		{
			Dbg_MsgAssert( 0,( "\n%s\nMust specify speed for Obj_Rot functions (degrees per second).", pScript->GetScriptInfo( ) ));
		}
		if ( !( m_movingobj_status & ( MOVINGOBJ_STATUS_ROTX << rotAxis ) ) )
		{
			m_turn_speed[ rotAxis ] = 0.0f;
		}
	}
	else
	{
		if ( pParams->GetFloat( 0xf0d90109, &m_turn_speed[ rotAxis ] ) ) // "speed"
		{
			m_turn_speed[ rotAxis ] = DEGREES_TO_RADIANS( m_turn_speed[ rotAxis ] );
		}
		else
		{
			Dbg_MsgAssert( 0,( "\n%s\nMust specify speed for Obj_Rot functions (degrees per second).", pScript->GetScriptInfo( ) ));
		}
	}
	if ( !( m_turn_ang_target[ rotAxis ] || m_angular_friction[ rotAxis ] ) )
	{
		m_movingobj_flags |= ( MOVINGOBJ_FLAG_CONST_ROTX << rotAxis );
	}
	if ( !( m_movingobj_flags & ( MOVINGOBJ_FLAG_CONST_ROTX << rotAxis ) ) )
	{
		SetCorrectRotationDirection( rotAxis );
	}
	m_movingobj_status |= ( MOVINGOBJ_STATUS_ROTX << rotAxis );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::LookAtNodeLinked_Init( Script::CStruct* pParams, Script::CScript* pScript )
{	
	int numLinks;
	numLinks = SkateScript::GetNumLinks( m_current_node );
	if ( !numLinks )
	{
		Dbg_MsgAssert( 0,( "\n%s\nNo node linked to object calling Obj_LookAtNodeLinked", pScript->GetScriptInfo( ) ));
		return;
	}
	
	int link_num=1;
	pParams->GetInteger( 0xe85997d0, &link_num ); // "linkNum"
	// Designers number the links from 1 to n, so convert to programmer form
	link_num -= 1;
	
	// Print a warning message instead of asserting, to save having to reboot.
	if (link_num<0 || link_num>=numLinks)
	{
		#ifdef __NOPT_ASSERT__
		printf("!!!!!!!!!! Bad LinkNum of %d sent to Obj_LookAtNodeLinked, range is 1 to %d\n",link_num+1,numLinks);
		#endif
		link_num=0;
	}	
	
	Mth::Vector lookAtPos;
	SkateScript::GetPosition( SkateScript::GetLink( m_current_node, link_num ), &lookAtPos );
	LookAt_Init( pParams, lookAtPos );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::LookAtNode_Init( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 checksum = 0;
	
	pParams->GetChecksum( 0xa1dc81f9, &checksum ); // "name"
	if ( !checksum )
	{
		Dbg_MsgAssert( 0,( "\n%s\nNeed node name in Obj_LookAtNode", pScript->GetScriptInfo( ) ));
		return;
	}
	Mth::Vector lookAtPos;
	SkateScript::GetPosition( SkateScript::FindNamedNode( checksum ), &lookAtPos );
	LookAt_Init( pParams, lookAtPos );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::LookAtPos_Init( Script::CStruct* pParams, bool absolute )
{
	
	Mth::Vector vec( 0, 0, 0 );
	pParams->GetVector( NONAME, &vec );
	if ( pParams->ContainsFlag( 0x26af9dc9 ) ) // "FLAG_MAX_COORDS"
	{
		vec.ConvertToMaxCoords( );
	}
	if ( !absolute )
	{
		vec += GetObject()->GetPos();
	}
	LookAt_Init( pParams, vec );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::Rot( int rotAxis, float deltaRot )
{
	switch ( rotAxis )
	{
		case ( X ):
			GetObject()->m_matrix.RotateXLocal( deltaRot );
			GetObject()->m_matrix.OrthoNormalizeAbout( X );
			break;
		
		case ( Y ):
			GetObject()->m_matrix.RotateYLocal( deltaRot );
			GetObject()->m_matrix.OrthoNormalizeAbout( Y );
			break;
		
		case ( Z  ):
			GetObject()->m_matrix.RotateZLocal( deltaRot );
			GetObject()->m_matrix.OrthoNormalizeAbout( Z );
			break;
		
		default:
			Dbg_MsgAssert( 0,( "illegal rot axis." ));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::RotateToFacePos( const Mth::Vector pos )
{   
	Mth::Vector desiredHeading;
	desiredHeading = pos - GetObject()->GetPos();
	float deltaY = Mth::GetAngle( GetObject()->GetMatrix(), desiredHeading );
	GetObject()->m_matrix.RotateYLocal( deltaY );
	GetObject()->m_matrix.OrthoNormalizeAbout( Y );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Follow the path that's linked to the current waypoint (node) that the object is sitting at.
void CMotionComponent::FollowPathLinked_Init( Script::CStruct* pParams, Script::CScript* pScript )
{   
	int numLinks;
	int nodeNum;

	if ( pParams->ContainsFlag( 0x422e1fce ) ) // originalNode
	{
		nodeNum = m_start_node;
	}
	else
	{
		nodeNum = m_current_node;
	}
	
	numLinks = SkateScript::GetNumLinks( nodeNum );
	if ( !numLinks )
	{
		Dbg_MsgAssert( 0,( "\n%s\nNo node linked to object (from node %d) calling Obj_FollowPathLinked", pScript->GetScriptInfo( ), nodeNum ));
		return;
	}
	if ( ( numLinks > 1 ) && PickRandomWaypoint( ) )
	{
		InitPath( SkateScript::GetLink( nodeNum, Mth::Rnd( numLinks ) ) );
	}
	else
	{
		InitPath( SkateScript::GetLink( nodeNum, 0 ) );
	}
	m_movingobj_status |= MOVINGOBJ_STATUS_ON_PATH;
	// these can't coexist:
	m_movingobj_status &= ~MOVINGOBJ_STATUS_MOVETO;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Follow the path from current position to the position of the waypoint last stored
// in the script call to Obj_StoreNode (when a pedestrian, for example, has to jump
// out of the way of the player, he stores his current position and the current node
// that he's heading for... then he walks back to his position, and finally ends up
// heading to the waypoint he was heading for before he left the path to jump out of the way).
void CMotionComponent::FollowStoredPath_Init( Script::CStruct* pParams )
{   
	InitPath( m_stored_node );
	m_movingobj_status |= MOVINGOBJ_STATUS_ON_PATH;
	// these can't coexist:
	m_movingobj_status &= ~MOVINGOBJ_STATUS_MOVETO;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Physics for when the moving object is following a path.
void CMotionComponent::DoPathPhysics( void )
{	
	if ( m_movingobj_status & MOVINGOBJ_STATUS_ON_PATH )
	{
		// if we're stopping:
		if ( m_movingobj_flags & MOVINGOBJ_FLAG_STOP_ALONG_PATH )
		{
			if ( m_stop_dist_traveled >= m_stop_dist )
			{
				return;
			}
			// keep slowing down...
			m_stop_dist_traveled += m_vel_z * m_time;
			if ( ( !m_stop_dist ) || ( m_stop_dist_traveled >= m_stop_dist ) )
			{
				m_vel_z = 0;
			}
			else
			{
				float percent = m_stop_dist_traveled / m_stop_dist;
				m_vel_z = ( m_vel_z_start * ( 1.0f - percent ) ) + ( m_min_stop_vel * ( percent ) );
			}
			return;
		}
		
		// Adjust velocity...
		if ( m_vel_z < m_max_vel )
		{
			if ( m_acceleration )
			{
				m_vel_z += ( m_acceleration * m_time );
				if ( m_vel_z > m_max_vel )
				{
					m_vel_z = m_max_vel;
				}
			}
			else
			{
				m_vel_z = m_max_vel;
			}
		}
		else if ( m_vel_z > m_max_vel )
		{
			if ( m_deceleration )
			{
				m_vel_z -= ( m_deceleration * m_time );
				if ( m_vel_z < m_max_vel )
				{
					m_vel_z = m_max_vel;
				}
			}
			else
			{
				m_vel_z = m_max_vel;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#if 0

// UNUSED...

// BB: I have added a new version of this function below.  I moved the code
// from the CallMemberFunction func because I needed to do the same thing from c.
// I have left this here for reference, just in case.
CCompositeObject* CMotionComponent::GetNextObjectOnPath( float range )
{
	if (!(m_movingobj_status & MOVINGOBJ_STATUS_ON_PATH) || !GetPathOb() || !mp_path_object_tracker)
	{
		return NULL;
	}	
	
	float distance_covered=0.0f;
	Mth::Vector path_pos=m_pos;
	int next_node=GetPathOb()->m_node_to;
	
	// Copy the non-NULL object pointers from the tracker to a small local array for speed,
	// since they may need to be stepped through several times and the tracker's array
	// contains lots of NULL's
	#define MAX_OBS 20
	CCompositeObject* pp_objects[MAX_OBS];
	int num_objects=0;
	
	const CSmtPtr<CCompositeObject>* pp_object_list=mp_path_object_tracker->GetObjectList();
	for (int i=0; i<CPathObjectTracker::MAX_OBJECTS_PER_PATH; ++i)
	{
		if (pp_object_list[i])
		{
			Dbg_MsgAssert(num_objects<MAX_OBS,("Too many objects on path, need to increase MAX_OBS in CMovingObject::GetNextObjectOnPath"));
			pp_objects[num_objects++]=pp_object_list[i].Convert();
		}
	}		
	
	CCompositeObject* p_nearest=NULL;
	while (true)
	{
		float min_dist=0.0f;
		
		for (int i=0; i<num_objects; ++i)
		{
			if (GetMotionComponentFromObject(pp_objects[i])->GetDestinationPathNode()==next_node)
			{
				float d=Mth::Distance(pp_objects[i]->m_pos,path_pos);
				if (distance_covered+d < range)
				{
					if (!p_nearest || d<min_dist)
					{
						min_dist=d;
						p_nearest=pp_objects[i];
					}	
				}
			}
		}	
				
		if (p_nearest)
		{
			break;
		}	
		
		Script::CStruct* p_node=SkateScript::GetNode(next_node);
		Dbg_MsgAssert(p_node,("NULL p_node"));
		Mth::Vector node_pos;
		SkateScript::GetPosition(p_node,&node_pos);
		
		distance_covered+=Mth::Distance(path_pos,node_pos);
		path_pos=node_pos;

		// Get the next linked node
		Script::CArray* p_links=NULL;
		p_node->GetArray(0x2e7d5ee7/*Links*/,&p_links);
		
		if (!p_links)
		{
			break;
		}
		
		next_node=p_links->GetInteger(0);	
	}
	
	return p_nearest;
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// returns TRUE if instant move, FALSE otherwise.
bool CMotionComponent::Move_Init( Script::CStruct* pParams, Script::CScript* pScript, Mth::Vector pos, int nodeNum )
{   
	int units = UNITS_MPH;
	float time;
	float speed;

	m_moveto_pos = pos;
	m_moveto_dist = Mth::Distance( m_moveto_pos, GetObject()->GetPos() );
	m_moveto_acceleration = 0.0f;
	
	// initialize everything:
	m_orig_pos = GetObject()->GetPos();
	m_moveto_dist_traveled = 0.0f;
	m_movingobj_status |= MOVINGOBJ_STATUS_MOVETO;
	// can't follow path and moveto at the same time:
	m_movingobj_status &= ~MOVINGOBJ_STATUS_ON_PATH;
	m_moveto_node_num = -1;
	if ( pParams->GetFloat( 0x906b67ba, &time ) ) // "time"
	{
		// time is in seconds:
		Dbg_MsgAssert( time,( "\n%s\nCan't have zero for time...", pScript->GetScriptInfo( ) ));
		m_moveto_speed = m_moveto_dist / time;
	}
	else if ( pParams->GetFloat( 0xf0d90109, &speed ) ) // "speed"
	{	// use speed and acceleration:
		if ( pParams->ContainsFlag( 0x2ce7ee02 ) ) // "mph"
		{
			units = UNITS_MPH;
		}
		else if ( pParams->ContainsFlag( 0xaad7c80f ) ) // "fps"
		{
			units = UNITS_FPS;
		} 
		else if ( pParams->ContainsFlag( 0xa18b8f32 ) ) //IPS
		{
			units = UNITS_IPS;
		}

		pParams->GetFloat( 0x85894a84, &m_moveto_acceleration ); // "acceleration"

		if ( units == UNITS_MPH )
		{
			speed = MPH_TO_IPS( speed );
			m_moveto_acceleration = MPH_TO_IPS( m_moveto_acceleration );
		}
		else if ( units == UNITS_FPS )
		{
			speed *= 12.0f;
			m_moveto_acceleration *= 12.0f;
		}
		
		if ( !m_moveto_acceleration )
		{
			m_moveto_speed = speed;
		}
		else
		{
			m_moveto_speed = 0.0f;
			m_moveto_speed_target = speed;
		}
	}
	else
	{
		// instantaneous move:
		GetObject()->SetPos(m_moveto_pos);
		m_movingobj_status &= ~MOVINGOBJ_STATUS_MOVETO;
		
		if ( nodeNum != -1 )
		{
			if ( pParams->ContainsFlag( 0x90a91232 ) ) // "orient"
			{
				Script::CStruct* pNodeData = SkateScript::GetNode( nodeNum );
				OrientToNode( pNodeData );
			}
		}
		
		// GJ:  Need to re-implement this on THPS5...
		// (used to fix quick-jump bug on Xbox/NGC,
		// where LODing code wasn't refreshing the model's
		// position)

#if 0
#ifndef __PLAT_NGPS__
		// Rebuild display matrix. Gary - you should look at this post THPS4 and generalize.
		Mth::Matrix rootMatrix;
		rootMatrix = m_display_matrix;
		rootMatrix[Mth::POS] = m_pos;
		rootMatrix[Mth::POS][W] = 1.0f;
		if( GetModel() )
		{
			GetModel()->Render( &rootMatrix, true, GetSkeleton() );

			// Also update the shadow if relevant.
			if( mp_shadow && mp_shadow->GetShadowType() != Gfx::vDETAILED_SHADOW )
			{
				update_shadow();
			}
		}
#endif
#endif

		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::MoveToNode_Init( Script::CStruct* pParams, Script::CScript* pScript )
{   
	Mth::Vector pos;
	
	// if we're moving to a named node:
	uint32 checksum = 0;
	pParams->GetChecksum( 0xa1dc81f9, &checksum ); // "name"
	if ( checksum )
	{
		int nodeNum = SkateScript::FindNamedNode( checksum );
		SkateScript::GetPosition( nodeNum, &pos );
		if ( Move_Init( pParams, pScript, pos, nodeNum ) )
		{
			HitWaypoint( nodeNum );
		}
		else
		{
			m_moveto_node_num = nodeNum;
		}
		return;
	}
	Dbg_MsgAssert( 0,( "\n%s\nObj_MoveToNode requires node name specification (name = *)", pScript->GetScriptInfo( ) ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::MoveToPos_Init( Script::CStruct* pParams, Script::CScript* pScript, bool absolute )
{   
	Mth::Vector vec( 0, 0, 0 );
	pParams->GetVector( NONAME, &vec );
	if ( pParams->ContainsFlag( 0x26af9dc9 ) ) // "FLAG_MAX_COORDS"
	{
		vec.ConvertToMaxCoords( );
	}
	if ( !absolute )
	{
		vec += GetObject()->GetPos();
	}
	Move_Init( pParams, pScript, vec );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::MoveToLink_Init( Script::CStruct* pParams, Script::CScript* pScript )
{   
	int linkNum = 0;
	int numLinks;
	numLinks = SkateScript::GetNumLinks( m_current_node );
	if ( !numLinks )
	{
		Dbg_MsgAssert( 0,( "Obj_MoveToLink requires a link to the object's current node (last node hit in path)." ));
		return;
	}
	if ( pParams->GetInteger( 0xe85997d0, &linkNum ) ) // "linkNum"
	{
		linkNum -= 1;
	}
	else if ( ( numLinks > 1 ) && ( pParams->ContainsFlag( 0x90a1c52a ) ) ) // "randomLink"
	{
		linkNum = Mth::Rnd( numLinks );
	}
	Mth::Vector pos;
	Dbg_MsgAssert( linkNum < numLinks,( "\n%s\nLink num is greater than the number of links.", pScript->GetScriptInfo( ) ));
	int nodeNum;
	nodeNum = SkateScript::GetLink( m_current_node, linkNum );
	SkateScript::GetPosition( nodeNum, &pos );
	if ( Move_Init( pParams, pScript, pos ) )
	{
		HitWaypoint( nodeNum );
	}
	else
	{
		m_moveto_node_num = nodeNum;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMotionComponent::Move( void )
{	
	if ( m_moveto_acceleration )
	{
		m_moveto_speed += m_moveto_acceleration * Tmr::FrameRatio( );
		if ( m_moveto_speed >= m_moveto_speed_target )
		{
			m_moveto_speed = m_moveto_speed_target;
			m_moveto_acceleration = 0.0f;
		}
	}
	m_moveto_dist_traveled += m_time * m_moveto_speed;
	if ( m_moveto_dist_traveled >= m_moveto_dist )
	{
		GetObject()->m_pos = m_moveto_pos;
		if ( m_moveto_node_num != -1 )
		{
			HitWaypoint( m_moveto_node_num );
		}
		return true;
	}
	Mth::Vector newPos = m_moveto_pos - m_orig_pos;
	newPos *= ( m_moveto_dist_traveled / m_moveto_dist );
	GetObject()->m_pos = m_orig_pos + newPos;
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::OrientToNode(Script::CStruct* pNodeData)
{
	Mth::Vector rot;
	GetObject()->m_matrix.Ident( );	   		// Set identity before we decided if we need to rotate or not, in case we don't.
	SkateScript::GetAngles( pNodeData, &rot );
	if ( rot[ X ] || rot[ Y ] || rot[ Z ] )
	{
		// AML: This was rotating Z, then X, then Y
		//		See me if there are problems, and I'll change
		//		the exporter to do Z,X,Y

		//m_matrix.RotateZ( rot[ Z ] );
		GetObject()->GetMatrix().RotateX( rot[ X ] );
		GetObject()->GetMatrix().RotateY( rot[ Y ] );
		GetObject()->GetMatrix().RotateZ( rot[ Z ] );
	}	
	//printf ("OrientToNode %d, (%f,%f,%f)\n",nodeNum,rot[X],rot[Y],rot[Z]);
	
	// sync up the display matrix to the physics matrix
	GetObject()->GetDisplayMatrix() = GetObject()->m_matrix;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::QuatRot( void )
{	
	float percent;
	m_rot_time += m_time;
	if ( ( !m_rot_time_target ) || ( m_rot_time >= m_rot_time_target ) )
	{
		percent = 1.0f;
		m_movingobj_status &= ~MOVINGOBJ_STATUS_QUAT_ROT;
	}
	else
	{
		percent = m_rot_time / m_rot_time_target;
	}
	
	Dbg_MsgAssert( mp_slerp,( "Missing mp_slerp..." ));
    mp_slerp->getMatrix( &GetObject()->m_matrix, percent );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::FollowPath( void )
{   
	// temp hack to separate new peds from old path ob stuff
	Obj::CPedLogicComponent* pPedLogicComp = GetPedLogicComponentFromObject( GetObject() );
	if ( pPedLogicComp )
		return;

	Dbg_MsgAssert( GetPathOb(),( "Shouldn't be in FOLLOW_WAYPOINTS state without pathob." ));

    Mth::Vector movement = -GetObject()->GetPos();				
				
	// change velocity:
	DoPathPhysics();

	// do path traversal:
	if ( m_vel_z )
	{
		/*
		Mth::Vector d=GetPathOb()->m_wp_pos_to-GetPathOb()->m_wp_pos_from;
		m_shadow_normal.Set(0.0f,1.0f,0.0f);
		m_shadow_normal=Mth::CrossProduct(m_shadow_normal,d);
		m_shadow_normal=Mth::CrossProduct(m_shadow_normal,d);
		m_shadow_normal.Normalize();
		*/
		
		// send in our forward direction, distance traveled...
		//if ( GetPathOb()->TraversePath( is_being_skitched, m_vel_z * m_time, 
		//	( m_movingobj_flags & MOVINGOBJ_FLAG_INDEPENDENT_HEADING ) ? NULL : &m_matrix[ Z ] ) )
		if ( GetPathOb()->TraversePath( m_vel_z * m_time, 
			( m_movingobj_flags & MOVINGOBJ_FLAG_INDEPENDENT_HEADING ) ? NULL : &GetObject()->m_matrix[ Z ] ) )
		{
			// path is done... turn off flag:
			m_movingobj_status &= ~MOVINGOBJ_STATUS_ON_PATH;
			m_vel_z = 0.0f;
		}

		if ( !( m_movingobj_flags & MOVINGOBJ_FLAG_INDEPENDENT_HEADING ) )
		{
			// set up the matrix according to the nav point heading...
			GetPathOb()->SetHeading( &GetObject()->m_matrix[ Z ] );
			if ( m_movingobj_flags & MOVINGOBJ_FLAG_NO_PITCH_PLEASE )
			{
				if ( ( GetObject()->m_matrix[ Z ] )[ Y ] )
				{
					( GetObject()->m_matrix[ Z ] )[ Y ] = 0.0f;
					GetObject()->m_matrix[ Z ].Normalize( );
				}
			}
			// this heading may be changed in a moment, when we stick
			// the object to the ground... if it isn't stuck, we should
			// normalize the matrix around that heading...
			if ( !( m_movingobj_flags & MOVINGOBJ_FLAG_STICK_TO_GROUND ) )
			{
				// make sure our matrix is orthagonal:
				GetObject()->m_matrix[ X ] = Mth::CrossProduct( GetObject()->m_matrix[ Y ], GetObject()->m_matrix[ Z ]);
				GetObject()->m_matrix[ X ].Normalize();
				// This will work for things whose Y axis is always up...
				// May have to add flags for things that rotate along Z axis or
				// far along X axis:
				GetObject()->m_matrix[ Z ] = Mth::CrossProduct( GetObject()->m_matrix[ X ], GetObject()->m_matrix[ Y ] );
				GetObject()->m_matrix[ Z ].Normalize( );
			}
		}
		if ( m_movingobj_flags & MOVINGOBJ_FLAG_STICK_TO_GROUND )
		{
			// Just move it to the nav pos ( except for y axis ) then stick it to the ground...
			float origY = GetObject()->m_pos[ Y ];
			GetObject()->m_pos = GetPathOb()->m_nav_pos;
			GetObject()->m_pos[ Y ] = origY;

			StickToGround();
		}
		else
		{
			float origY = GetObject()->m_pos[ Y ];
			GetObject()->m_pos = GetPathOb()->m_nav_pos;
			if ( m_movingobj_flags & MOVINGOBJ_FLAG_CONSTANT_HEIGHT )
			{
				GetObject()->m_pos[ Y ] = origY;
			}
		}
	}

	movement += GetObject()->m_pos;				// actually calculate movement	
	GetObject()->m_vel = movement / m_time; 		// calculate the velocity, so we can impart velocity in things we hit, etc
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// call SetUpLookAtPos first, or one of the other rotate
// setup functions, then call this until it returns true:
bool CMotionComponent::Rotate( int rotAxis )
{	

	bool done = false;

	// update angular acceleration/angular velocity:
	if ( m_delta_turn_speed[ rotAxis ] )
	{
		m_turn_speed[ rotAxis ] += m_delta_turn_speed[ rotAxis ] * m_time;
		if ( ( Mth::Abs( m_turn_speed[ rotAxis ] ) > Mth::Abs( m_turn_speed_target[ rotAxis ] ) )
			&& ( ( m_turn_speed[ rotAxis ] > 0.0f ) == ( m_turn_speed_target[ rotAxis ] > 0.0f ) ) )
		{
			// stop angular acceleration...settle on the target:
			m_turn_speed[ rotAxis ] = m_turn_speed_target[ rotAxis ];
		}
	}
	
	if ( m_angular_friction[ rotAxis ] )
	{
		float friction = m_angular_friction[ rotAxis ] * m_time;
		// Ken: Changed this so that it decelerates down to the m_turn_speed_target rather than 0
		if ( Mth::Abs( friction ) > Mth::Abs( m_turn_speed[ rotAxis ] - m_turn_speed_target[ rotAxis ] ) )
		{
			// Settle on the target speed
			m_turn_speed[ rotAxis ] = m_turn_speed_target[ rotAxis ];
			// Switch off the friction so that it deos not slow down any more
			m_angular_friction[ rotAxis ] = 0.0f;
			
			Dbg_Message( "done rotating due to deceleration... %s angle traversed = %f", 
				rotAxis == X ? "X" : ( rotAxis == Y ? "Y" : "Z" ), RADIANS_TO_DEGREES( m_cur_turn_ang[ rotAxis ] ) );
			if (m_turn_speed_target[ rotAxis ]==0.0f)
			{
				return true; // done rotating...
			}
			else
			{
				return false;  
			}	
		}
		else
		{
			m_turn_speed[ rotAxis ] -= friction;
		}
	}
	float cur_turn_dist = m_turn_speed[ rotAxis ] * m_time;
	
	if ( !( m_movingobj_flags & ( MOVINGOBJ_FLAG_CONST_ROTX << rotAxis ) ) )
	{
		bool positive = m_turn_ang_target[ rotAxis ] > m_cur_turn_ang[ rotAxis ];
		m_cur_turn_ang[ rotAxis ] += cur_turn_dist;
		if ( positive != ( m_turn_ang_target[ rotAxis ] > m_cur_turn_ang[ rotAxis ] ) )
		{
			//done rotating!
			done = true;
			// and deltaYRot should be altered so as to hit 0 exactly:
			cur_turn_dist += m_turn_ang_target[ rotAxis ] - m_cur_turn_ang[ rotAxis ];
	//		m_cur_turn_ang[ rotAxis ] = m_turn_ang_target[ rotAxis ];
		}
	}

	Rot( rotAxis, cur_turn_dist );
	
	return ( done );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::RotUpdate( void )
{   
	// rotate states:
	for ( int i = 0; i < 3; i++ )
	{
		if ( m_movingobj_status & ( MOVINGOBJ_STATUS_ROTX << i ) )
		{
			if ( Rotate( i ) )
			{
				m_movingobj_status &= ~( MOVINGOBJ_STATUS_ROTX << i );
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


static bool s_do_collision_check(const Mth::Vector *p_v0, const Mth::Vector *p_v1, Mth::Vector *p_collisionPoint, 
								 STriangle *p_lastTriangle)
{
	// First, see if there is a collision with the last triangle, and if there is, use that.
	Mth::Vector d=*p_v1-*p_v0;
	float alpha=0.0f;
	if (Nx::CCollObj::sRayTriangleCollision(p_v0, &d,
											&p_lastTriangle->mpVertices[0],
											&p_lastTriangle->mpVertices[1],
											&p_lastTriangle->mpVertices[2], &alpha))
	{
		*p_collisionPoint = *p_v0+d*alpha;
		return true;
	}
	else
	{
		CFeeler feeler;
		if (feeler.GetCollision(*p_v0,*p_v1, false))
		{
			Nx::CCollObj* p_col_obj=feeler.GetSector();
			Dbg_MsgAssert(p_col_obj->IsTriangleCollision(),("Not triangle collision !!!"));
	
			Nx::CCollObjTriData* p_tri_data=p_col_obj->GetGeometry();
			Dbg_MsgAssert(p_tri_data,("NULL p_tri_data"));
	
			int face_index=feeler.GetFaceIndex();
			p_lastTriangle->mpVertices[0]=p_col_obj->GetVertexPos(p_tri_data->GetFaceVertIndex(face_index, 0));
			p_lastTriangle->mpVertices[1]=p_col_obj->GetVertexPos(p_tri_data->GetFaceVertIndex(face_index, 1));
			p_lastTriangle->mpVertices[2]=p_col_obj->GetVertexPos(p_tri_data->GetFaceVertIndex(face_index, 2));
	
			// if there is a collision, then snap to it
			*p_collisionPoint = feeler.GetPoint();
	
			return true;
		}
	}
	return false;	
}


bool CMotionComponent::StickToGround()
{
	if ( ( !m_col_dist_above ) && ( !m_col_dist_below ) )
	{
		return false;
	}	

	if ( m_point_stick_to_ground )
	{
		// standard stick to ground
		// (also need a car version,
		// which will be a separate component)

		Mth::Vector v0, v1;

		// make sure our X vector is correct (got Z from the nav matrix...)
		GetObject()->m_matrix[ X ] = Mth::CrossProduct( GetObject()->m_matrix[ Y ], GetObject()->m_matrix[ Z ]);
		GetObject()->m_matrix[ X ].Normalize();

		v0.Set( 0.0f, FEET_TO_INCHES( fabs( m_col_dist_above ) ), 0.0f );
		v1.Set( 0.0f, -FEET_TO_INCHES( fabs( m_col_dist_below ) ), 0.0f );

		v0 += GetObject()->m_pos;
		v1 += GetObject()->m_pos;

		// get disatnce to ground
		// and snap the skater to it

		Mth::Vector collision_point;
		if (s_do_collision_check(&v0, &v1, &collision_point, &m_last_triangle))
		{
			GetObject()->m_pos=collision_point;
			return true;
		}
		return false;	
	}
	else
	{
		// if it's a car, then it's big
		Mth::Vector v0, v1, vectForward, vectLeft, leftCollidePoint, rightCollidePoint;
		bool leftCollision = false;
		bool rightCollision = false;
		CFeeler		feeler;

		GetObject()->m_matrix[ Y ].Set( 0.0f, 1.0f, 0.0f );
		// make sure our X vector is correct (got Y and Z from the nav matrix...)
		GetObject()->m_matrix[ X ] = Mth::CrossProduct( GetObject()->m_matrix[ Y ], GetObject()->m_matrix[ Z ]);
		GetObject()->m_matrix[ X ].Normalize();

		vectForward = GetObject()->m_matrix[ Z ] * FEET_TO_INCHES( 6.0f );
		vectLeft = GetObject()->m_matrix[ X ] * FEET_TO_INCHES( 2.5f );

		GetObject()->m_pos -= vectForward;
		GetObject()->m_pos += vectLeft;

		v0 = v1 = GetObject()->m_pos;
		v0[ Y ] += FEET_TO_INCHES( m_col_dist_above );
		v1[ Y ] -= FEET_TO_INCHES( m_col_dist_below );

		// find a collision point under the left rear:
		if (s_do_collision_check(&v0, &v1, &leftCollidePoint, &m_car_left_rear_last_triangle))
		{
			leftCollision = true;
		}
		
		GetObject()->m_pos -= vectLeft * 2;

		v0 = v1 = GetObject()->m_pos;
		v0[ Y ] += FEET_TO_INCHES( m_col_dist_above );
		v1[ Y ] -= FEET_TO_INCHES( m_col_dist_below );

		// find a collision point under the right rear:
		if (s_do_collision_check(&v0, &v1, &rightCollidePoint, &m_car_right_rear_last_triangle))
		{
			rightCollision = true;
			if ( leftCollision )
			{
				GetObject()->m_matrix[ X ] = leftCollidePoint - rightCollidePoint;
				GetObject()->m_matrix[ X ].Normalize( );
			}
		}
		
		GetObject()->m_pos += vectForward * 2;
		GetObject()->m_pos += vectLeft;

		v0 = v1 = GetObject()->m_pos;
		v0[ Y ] += FEET_TO_INCHES( m_col_dist_above );
		v1[ Y ] -= FEET_TO_INCHES( m_col_dist_below );

		GetObject()->m_pos -= vectForward;

		Mth::Vector mid_front_collide_point;
		if (s_do_collision_check(&v0, &v1, &mid_front_collide_point, &m_car_mid_front_last_triangle))
		{
			if ( rightCollision && leftCollision )
			{
				GetObject()->m_pos[ Y ] = leftCollidePoint[ Y ] + rightCollidePoint[ Y ];
				GetObject()->m_pos[ Y ] /= 2;
				GetObject()->m_pos[ Y ] += mid_front_collide_point.GetY();
				GetObject()->m_pos[ Y ] /= 2;

				Mth::Vector heading;

				heading = leftCollidePoint + rightCollidePoint;
				heading /= 2;
				heading -= mid_front_collide_point;
				heading *= -1;
				heading.Normalize( );

				GetObject()->m_matrix[ Z ] += heading;
				GetObject()->m_matrix[ Z ] /= 2;

				GetObject()->m_matrix[ Y ] = Mth::CrossProduct( GetObject()->m_matrix[ Z ], GetObject()->m_matrix[ X ] );
				GetObject()->m_matrix[ Y ].Normalize( );

				return true;
			}
		}

		// (Mick) This assertion has been removed, as cars sometimes drive where the skater cannot go, and hence into no collision		
		//	Dbg_MsgAssert( 0, ( "StickToGround car %s was driving on an area with no collision", Script::FindChecksumName(GetID()) ) );

		return false;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::GetDebugInfo( Script::CStruct* p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert( p_info, ( "NULL p_info sent to CMotionComponent::GetDebugInfo" ) );
	CBaseComponent::GetDebugInfo(p_info);
	
	p_info->AddChecksum( CRCD(0x9e6b250,"m_movingobj_status"), m_movingobj_status );
	p_info->AddChecksum( CRCD(0xe5dff85a,"m_movingobj_flags"), m_movingobj_flags );
	p_info->AddFloat( CRCD(0xc2d3b58d,"m_moveto_speed"), m_moveto_speed );
	p_info->AddFloat( CRCD(0x3a3ae81a,"m_acceleration"), m_acceleration );
	p_info->AddFloat( CRCD(0x4a2c5549,"m_deceleration"), m_deceleration );
	p_info->AddFloat( CRCD(0x472b8f9b,"m_max_vel"), m_max_vel );
	p_info->AddFloat( CRCD(0xc37e4e25,"m_vel_z"), m_vel_z );
	p_info->AddFloat( CRCD(0xcab44eaf,"m_stop_dist"), m_stop_dist );

	// p_info->AddFloat( "m_col_dist_above", m_col_dist_above );
	// p_info->AddFloat( "m_col_dist_below", m_col_dist_below );
	// p_info->AddVector( "m_last_triangle_v0", m_last_triangle_v0 );
	// p_info->AddVector( "m_last_triangle_v1", m_last_triangle_v1 );
	// p_info->AddVector( "m_last_triangle_v2", m_last_triangle_v2 );
	// p_info->AddVector( "m_last_normal", m_last_normal );
	p_info->AddInteger( CRCD(0xa13363b0,"m_point_stick_to_ground"), m_point_stick_to_ground );
	
	p_info->AddFloat( CRCD(0xac5b4824,"m_moveto_dist"), m_moveto_dist );
	p_info->AddFloat( CRCD(0x3e3570a6,"m_moveto_dist_traveled"), m_moveto_dist_traveled );
	p_info->AddFloat( CRCD(0x93e73df1,"m_moveto_speed_target"), m_moveto_speed_target );
	p_info->AddFloat( CRCD(0x6819fd8d,"m_moveto_acceleration"), m_moveto_acceleration );	
	
	p_info->AddInteger( CRCD(0xd6d94326,"m_moveto_node_num"), m_moveto_node_num );
	
	// p_info->AddVector( "m_turn_ang_target", m_turn_ang_target );
	// p_info->AddVector( "m_cur_turn_ang", m_cur_turn_ang );
	// p_info->AddVector( "m_turn_speed_target", m_turn_speed_target );
	p_info->AddVector( CRCD(0x9d27cc70,"m_turn_speed"), m_turn_speed );
	// p_info->AddVector( "m_delta_turn_speed", m_delta_turn_speed );
	// p_info->AddVector( "m_angular_friction", m_angular_friction );
	p_info->AddVector( CRCD(0xfa4307e,"m_orig_pos"), m_orig_pos );
	
	// p_info->AddFloat( "m_stop_dist_traveled", m_stop_dist_traveled );
	// p_info->AddFloat( "m_vel_z_start", m_vel_z_start );
	// p_info->AddFloat( "m_min_stop_vel", m_min_stop_vel );
	
	p_info->AddVector( CRCD(0xc3e1cded,"m_stored_pos"), m_stored_pos );
	p_info->AddInteger( CRCD(0x56e54ee5,"m_stored_node"), m_stored_node );
	
	// CPathObjectTracker*		mp_path_object_tracker;
	
	// Mth::SlerpInterpolator*	mp_slerp;
	// p_info->AddFloat( "m_rot_time_target", m_rot_time_target );
	p_info->AddFloat( CRCD(0x622c6c9b,"m_rot_time"), m_rot_time );
	
	p_info->AddInteger( CRCD(0x80dd9065,"m_start_node"), m_start_node );
	p_info->AddInteger( CRCD(0xa80dc42,"m_current_node"), m_current_node );
	
	if (mp_pathOb)
	{
		Script::CStruct* p_pathOb_params = new Script::CStruct();
		mp_pathOb->GetDebugInfo( p_pathOb_params	);
		p_info->AddStructure( CRCD(0x81270c1f,"mp_pathOb"), p_pathOb_params );
		delete p_pathOb_params;
	}	
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCompositeObject* CMotionComponent::GetNextObjectOnPath( float range )
{
	CCompositeObject* p_closest_ob=NULL;

	if ( mp_path_object_tracker )
	{		
		const CSmtPtr<CCompositeObject>* pp_object_list = mp_path_object_tracker->GetObjectList();

		float min_dist = 0.0f;
		for (int i=0; i<CPathObjectTracker::MAX_OBJECTS_PER_PATH; ++i)
		{
			if (pp_object_list[i])
			{
				CCompositeObject* p_ob=pp_object_list[i].Convert();
	
				if (p_ob!=GetObject() && 
					Mth::DotProduct(GetObject()->m_matrix[Z],p_ob->GetMatrix()[Z]) >= 0.0f &&
					Mth::DotProduct(GetObject()->m_matrix[Z],p_ob->GetPos()-GetObject()->GetPos()) >= 0.0f
					)
				{
					if (GetMotionComponentFromObject(p_ob)->GetDestinationPathNode()==GetDestinationPathNode() &&
						GetMotionComponentFromObject(p_ob)->GetPreviousPathNode()!=GetPreviousPathNode())
					{
						// Make vehicles on converging paths not collide, otherwise
						// a pile up happens at the start of the car paths in the school
						// Note: Actually, this didn't fix it ... might be a script bug?
					}
					else
					{
						float d=Mth::Distance( GetObject()->GetPos(), p_ob->GetPos());
						if( d <= range )
						{
							if( !p_closest_ob || d < min_dist )
							{
								min_dist = d;
								p_closest_ob = p_ob;
							}
						}
					}	
				}
			}
		}
	}
	return p_closest_ob;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


void CMotionComponent::FollowLeader( void )
{   
	Dbg_MsgAssert( mp_follow_ob,( "Shouldn't be in follow-leader state without mp_follow_ob." ));
	
	mp_follow_ob->GetNewPathPointFromObjectBeingFollowed();
	
	float old_y = GetObject()->m_pos[Y];
	mp_follow_ob->CalculatePositionAndOrientation(&GetObject()->m_pos,&GetObject()->m_matrix);
	if (m_leave_y_unaffected_when_following_leader)
	{
		GetObject()->m_pos[Y]=old_y;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::FollowLeader_Init( Script::CStruct* pParams )
{
	if (pParams->ContainsFlag(CRCD(0xd443a2bc,"Off")))
	{
		m_movingobj_status &= ~MOVINGOBJ_STATUS_FOLLOWING_LEADER;
		if (mp_follow_ob)
		{
			delete mp_follow_ob;
			mp_follow_ob=NULL;
		}
		return;
	}		
		
	m_leave_y_unaffected_when_following_leader=pParams->ContainsFlag("LeaveYUnaffected");
	
	m_movingobj_status |= MOVINGOBJ_STATUS_FOLLOWING_LEADER;
	
	// these can't coexist:
	m_movingobj_status &= ~MOVINGOBJ_STATUS_MOVETO;

	//	GetMotionComponent()->m_movingobj_status &= ~MOVINGOBJ_STATUS_LOCKED_TO_OBJECT;
	Obj::CLockObjComponent* pLockObjComponent = GetLockObjComponentFromObject(GetObject());
	pLockObjComponent->EnableLock( false );

	if ( !mp_follow_ob )
	{
		mp_follow_ob = new CFollowOb;
	}	
	mp_follow_ob->Reset();
	
	float distance=0.0f;
	pParams->GetFloat(CRCD(0xe36d657e,"Distance"),&distance);
	mp_follow_ob->SetDistance(distance);
	
	uint32 name=0;
	pParams->GetChecksum(CRCD(0xa1dc81f9,"Name"),&name);
	mp_follow_ob->SetLeaderName(name);
	
	if (pParams->ContainsFlag(CRCD(0xe19e310a,"OrientY")))
	{
		mp_follow_ob->OrientY();
	}	
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMotionComponent::DoJump( void )
{
	Mth::Vector new_pos;

	// Gfx::AddDebugLine( m_jump_start_pos, m_pos, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 1 );
	float distance = m_jump_speed * m_time + ( m_jump_gravity * 0.5f * m_time * m_time );
	// printf("m_jump_speed = %f, m_jump_gravity = %f\n, m_jump_original_speed = %f, m_jump_heading = (%f, %f, %f)\n", m_jump_speed, m_jump_gravity, m_jump_original_speed, m_jump_heading[X], m_jump_heading[Y], m_jump_heading[Z]);
	new_pos = GetObject()->m_pos;
	if ( m_jump_use_heading )
	{		
		// first figure the pos if we stayed on our original heading
		new_pos += ( m_jump_heading * m_jump_original_speed * m_time );
		new_pos[Y] = GetObject()->m_pos[Y];
	}
	new_pos[Y] += distance;

	m_jump_speed += m_jump_gravity * m_time;

	// do land checks if we're on our way down
	if ( distance < 0.0f )
	{
		if ( m_jump_use_land_height )
		{
			if ( new_pos[Y] < m_jump_land_height )
			{
				m_movingobj_status &= ~MOVINGOBJ_STATUS_JUMPING;
				new_pos[Y] = m_jump_land_height;
			}
		}
		else
		{
			// use collision checks
			CFeeler feeler;
			feeler.m_start = new_pos + Mth::Vector( 0, m_jump_col_dist_above, 0 );
			feeler.m_end = new_pos + Mth::Vector( 0, -m_jump_col_dist_below, 0 );
			if ( feeler.GetCollision() )
			{
				m_movingobj_status &= ~MOVINGOBJ_STATUS_JUMPING;
				if ( m_jump_start_pos[X] != GetObject()->m_pos[X] || m_jump_start_pos[Z] != GetObject()->m_pos[Z] )
				{
					// printf("StickToGround %s\n",Script::FindChecksumName(GetID()));
					// GetMotionComponent()->StickToGround();
					// m_jump_start_pos = m_pos;
				
					/*if ( new_pos[Y] < m_jump_start_pos[Y])
					{
						new_pos[Y] = m_jump_start_pos[Y];
						GetMotionComponent()->m_movingobj_status &= ~MOVINGOBJ_STATUS_JUMPING;
					}*/
				}
				else
				{
					// new_pos[Y] = m_jump_start_pos[Y];
					// GetMotionComponent()->m_movingobj_status &= ~MOVINGOBJ_STATUS_JUMPING;
				}
			}
		}
	}
	m_jump_pos = new_pos;
}



	
}
