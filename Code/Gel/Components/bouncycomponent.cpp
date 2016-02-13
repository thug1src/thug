//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       BouncyComponent.cpp
//* OWNER:          Mick West
//* CREATION DATE:  10/17/2002
//****************************************************************************

//#define	__DEBUG_BOUNCY__

#include <gel/components/Bouncycomponent.h>

#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/component.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <gel/objsearch.h>
#include <gel/objman.h>

#include <core/math/slerp.h>

#include <sk/engine/feeler.h>

#include <gel/collision/collision.h>

//#include <sk/objects/movingobject.h>		// TODO:  remove - only used for getting the bouncing box via collision
#include <sk/modules/skate/skate.h>				// and this
#include <sk/objects/skater.h>				// and this

namespace Obj
{

#define SWITCH_STATE( x )					m_state = x

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent *	CBouncyComponent::s_create()
{
	return static_cast<CBaseComponent*>(new CBouncyComponent);	
}
    
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBouncyComponent::CBouncyComponent() : CBaseComponent()
{
	SetType(CRC_BOUNCY);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBouncyComponent::~CBouncyComponent()
{

	if ( mp_bounce_slerp )
		delete mp_bounce_slerp;
	if (mp_collide_script_params)
		delete mp_collide_script_params;
	if (mp_bounce_script_params)
		delete mp_bounce_script_params;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Returns 
//	0 (MF_FALSE) if executed and returned false
//  1 (MF_TRUE)  if executed and returned true
//  2 (MF_NOT_EXECUTRED)    if not executed

CBaseComponent::EMemberFunctionResult CBouncyComponent::CallMemberFunction( uint32 Checksum, Script::CScriptStructure *pParams, Script::CScript *pScript )
{
	
	switch ( Checksum )
	{

        // @script | BouncyObj_PlayerCollisionOn | turn player collision on
		case ( 0xf9055d81 ): // "BouncyObj_PlayerCollisionOn"
			m_bouncyobj_flags &= ~BOUNCYOBJ_FLAG_PLAYER_COLLISION_OFF;
			return MF_TRUE;

        
        // @script | BouncyObj_PlayerCollisionOff | turn player collision off
		case ( 0xf64ef88e ): // "BouncyObj_PlayerCollisionOff"
			m_bouncyobj_flags |= BOUNCYOBJ_FLAG_PLAYER_COLLISION_OFF;
			return MF_TRUE;

        // @script | BouncyObj_Go | The vector can be anything you want...
        // the initial velocity.  It's optional, without it the object will
        // fall from it's current location.  If it's already on the ground
        // probably nothing will happen
        // @uparmopt (0, 0, 0) | vector
		case ( 0xfea0e952 ): // "BouncyObj_Go"
		{
			Mth::Vector vel;
			vel.Set( );
			pParams->GetVector( NONAME, &vel );
			if ( m_state == BOUNCYOBJ_STATE_BOUNCING )
			{
				Dbg_Message( "\n%s\nWarning:  Calling BouncyObj_Go when object is already bouncing.", pScript->GetScriptInfo( ) );
			}
			if ( pParams->ContainsFlag( 0x26af9dc9 ) ) // "FLAG_MAX_COORDS"
			{
				vel.ConvertToMaxCoords( );
			}
			bounce( vel );
			return MF_TRUE;
		}
	}
	return MF_NOT_EXECUTED;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CBouncyComponent::InitFromStructure( Script::CStruct* pParams )
{

	m_time = 0.01666666f;			// Patch!!!!

	m_state = BOUNCYOBJ_STATE_INIT;

	m_bounce_mult = 0.9f;
	m_bouncyobj_flags = 0;

	// Bouncy object properties:
	m_random_dir = 10;
	pParams->GetInteger( 0x767f323f, &m_random_dir ); // RandomDirectionOnBounce
	
	m_bounce_mult = 0.6f;
	pParams->GetFloat( 0xaf6034df, &m_bounce_mult ); // BounceMult
	
	m_min_bounce_vel = 3;
	pParams->GetFloat( 0xff08ee44, &m_min_bounce_vel );  // MinBounceVel
	m_min_bounce_vel = FEET_TO_INCHES( m_min_bounce_vel );
	
	m_rot_per_axis = 360;
	pParams->GetInteger( 0x33ee064f, &m_rot_per_axis ); // ConstRot
	
	m_bounce_rot = 45;
	pParams->GetInteger( 0xde14151d, &m_bounce_rot ); // BounceRot
	
	m_rest_orientation_type=0;
	pParams->GetInteger( 0xc0f30f40, &m_rest_orientation_type ); // RestOrientationType

	m_gravity = 32;
	pParams->GetFloat( 0xa5e2da58, &m_gravity ); // Gravity
	m_gravity = FEET_TO_INCHES( m_gravity );
	
	m_bounciness = 1.0f;
	pParams->GetFloat( 0x373d4e7d, &m_bounciness ); // Bounciness
	
	m_bounce_sound=0;
	pParams->GetChecksum( 0x1fb2f60c, &m_bounce_sound ); // BounceSound
	
	m_hit_sound=0;
	pParams->GetChecksum( 0x29e2e9e0, &m_hit_sound ); // HitSound
	
	m_up_mag = 32.0f;
	pParams->GetFloat( 0x3eca5c95, &m_up_mag ); // UpMagnitude
	m_up_mag = FEET_TO_INCHES( m_up_mag );
	
	m_destroy_when_done = !pParams->ContainsFlag( 0x65ba5a75 ); // NoDestroy
	
	m_min_initial_vel = 15; // feet per second...
	pParams->GetFloat( 0x96763c79, &m_min_initial_vel ); // MinInitialVel
	m_min_initial_vel = FEET_TO_INCHES( m_min_initial_vel );
	
	// Set up collision

	uint32	col_type_checksum = Nx::vCOLL_TYPE_NONE;
	pParams->GetChecksum("CollisionMode", &col_type_checksum);
	#if 0

	
	// TODO - handle getting bouncing box from a collision component
	Nx::CollType col_type = Nx::CCollObj::sConvertTypeChecksum(col_type_checksum);
	
	if (col_type != Nx::vCOLL_TYPE_NONE)
	{
		m_bounce_collision_radius=0.0f;
		m_skater_collision_radius_squared=0.0f;
//		InitCollision(col_type, p_coll_tri_data);
		// Calc radius from bounding box.
		if (((CMovingObject*)(GetObject()))->GetCollisionObject())
		{
			Nx::CCollObjTriData *p_tri_data=((CMovingObject*)(GetObject()))->GetCollisionObject()->GetGeometry();
			Dbg_MsgAssert(p_tri_data,("NULL p_tri_data"));
			Mth::CBBox bbox=p_tri_data->GetBBox();
			m_bounce_collision_radius=Mth::Distance(bbox.GetMax(),bbox.GetMin())/2.0f;
			m_skater_collision_radius_squared=m_bounce_collision_radius*m_bounce_collision_radius;
		}
	}
	else
	#endif
	
	{
		m_bounce_collision_radius=0.0f;
		m_skater_collision_radius_squared=50.0f;
		pParams->GetFloat("CollisionRadius",&m_skater_collision_radius_squared);
		m_skater_collision_radius_squared*=m_skater_collision_radius_squared;
	}
	

	// We need to copy in any script struct parameters, as they might have generated temporarily
	pParams->GetChecksum(CRCD(0x58cdc0f0,"CollideScript"),&m_collide_script);
	Script::CStruct * p_collide_script_params = NULL;
	pParams->GetStructure(CRCD(0x24031741,"CollideScriptParams"),&p_collide_script_params);		
	if (p_collide_script_params)
	{
		mp_collide_script_params = new Script::CStruct;
		*mp_collide_script_params += *p_collide_script_params;
	}
	
	pParams->GetChecksum(CRCD(0x2d075f90,"BounceScript"),&m_bounce_script);
	Script::CStruct * p_bounce_script_params = NULL;
	pParams->GetStructure(CRCD(0x346b0c03,"BounceScriptParams"),&p_bounce_script_params);		
	if (p_bounce_script_params)
	{
		mp_bounce_script_params = new Script::CStruct;
		*mp_bounce_script_params += *p_bounce_script_params;
	}
	
	
//	((CMovingObject*)(GetObject()))->RunNodeScript(0x2d075f90/*BounceScript*/,0x346b0c03/*BounceScriptParams*/);
	


	m_pos = GetObject()->m_pos;
	m_matrix = GetObject()->m_matrix;


}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CBouncyComponent::Update()
{

	Mdl::Skate *	skate_mod =  Mdl::Skate::Instance();
	uint32 numSkaters = skate_mod->GetNumSkaters( );

	m_time = Tmr::FrameLength();

	if ( !( m_bouncyobj_flags & BOUNCYOBJ_FLAG_PLAYER_COLLISION_OFF ) )
	{
		if ( m_state != BOUNCYOBJ_STATE_BOUNCING || ( ( !m_destroy_when_done ) && m_bounce_count ) )
		{
			uint32 i;
			for ( i = 0; i < numSkaters; i++ )
			{
				CSkater *pSkater = skate_mod->GetSkater( i );
				
				// Very simple collision check ...
				// Seems to work fine.
				if ( Mth::DistanceSqr( m_pos, pSkater->m_pos ) < m_skater_collision_radius_squared )
				{
					// Trigger any collide-script specified in the object's node.
					
							   
					#if 0
					// TODO - handle this RunNodeScript - IF IT IS ACTUALLY EVER USED!!!!!
					((CMovingObject*)GetObject())->RunNodeScript(0x58cdc0f0/*CollideScript*/,0x24031741/*CollideScriptParams*/);
					#else
					// This is also rather messy.  Seems like it would be better
					// to have this handled by an exception
					if (m_collide_script)
					{
						Script::CScript *p_script=Script::SpawnScript(m_collide_script,mp_collide_script_params);
						#ifdef __NOPT_ASSERT__
						p_script->SetCommentString("CBouncyComponent collide script");
						#endif
						Dbg_MsgAssert(p_script,("NULL p_script"));
						p_script->mpObject=GetObject();;
						p_script->Update();
					}
					#endif			   

					// Trigger the bounce.
					Mth::Vector vel;
					vel = pSkater->GetVel( );
					bounce_from_object_vel( vel, pSkater->m_pos );
					continue;
				}
			}
		}
	}
	
    switch ( m_state )
	{
		case ( BOUNCYOBJ_STATE_INIT ):
			m_state = BOUNCYOBJ_STATE_IDLE;
			break;
			
		case ( BOUNCYOBJ_STATE_IDLE ):
			break;

		case ( BOUNCYOBJ_STATE_BOUNCING ):
			
			// TODO:  The following is needed only to update the position of the object
			// This should be done by updating the postion of the model component
			#if 0
			// TODO - delete this code if it is not used
			((CMovingObject*)GetObject())->MovingObj_Update( );    
			#endif
			
			do_bounce( );
			break;
						
		default:
			Dbg_MsgAssert( 0,( "Unknown substate." ));
			break;
	}


}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


#define DTR DEGREES_TO_RADIANS

float get_closest_allowable_angle( float current, float allowableAngle )
{
	
	float ang;
	float halfAllowable = allowableAngle / 2.0f;
	ang = DTR( -180.0f );
	ang += halfAllowable;
	while ( ang < DTR( 180.0f ) )
	{
		if ( current < ang )
		{
			return ( ang - ( halfAllowable ) );
		}
		ang += allowableAngle;
	}
	return ( ang - halfAllowable );
}

void CBouncyComponent::land_on_top_or_bottom( Mth::Vector &rot )
{
	
	// x to nearest acceptable:
	if ( ( rot[ X ] > DTR( 90.0f ) ) || ( rot[ X ] < DTR( -90.0f ) ) )
		rot[ X ] = DTR( 180.0f );
	else
		rot[ X ] = 0.0f;
	// z to nearest acceptable:
	if ( ( rot[ Z ] > DTR( 90.0f ) ) || ( rot[ Z ] < DTR( -90.0f ) ) )
		rot[ Z ] = DTR( 180.0f );
	else
		rot[ Z ] = 0.0f;
}

void CBouncyComponent::land_on_any_face( Mth::Vector &rot )
{
	
	rot[ X ] = get_closest_allowable_angle( rot[ X ], DTR( 90.0f ) );
	rot[ Y ] = get_closest_allowable_angle( rot[ Y ], DTR( 90.0f ) );
	rot[ Z ] = get_closest_allowable_angle( rot[ Z ], DTR( 90.0f ) );
}

#define CONE_ANGLE	110.0f

void CBouncyComponent::land_traffic_cone( Mth::Vector &rot )
{
	
/*	if ( rot[ X ] < -DTR( CONE_ANGLE / 2.0f ) )
		rot[ X ] = -DTR( CONE_ANGLE );
	else if ( rot[ X ] > DTR( CONE_ANGLE / 2.0f ) )
		rot[ X ] = DTR( CONE_ANGLE );
	else*/
		rot[ X ] = 0;

	if ( rot[ Z ] < -DTR( CONE_ANGLE / 2.0f ) )
		rot[ Z ] = -DTR( CONE_ANGLE );
	else if ( rot[ Z ] > DTR( CONE_ANGLE / 2.0f ) )
		rot[ Z ] = DTR( CONE_ANGLE );
	else
		rot[ Z ] = 0;

//	rot[ Y ] = 0;
}

void CBouncyComponent::set_up_quats( void )
{
	
	Mth::Vector rot;// = currentRot;
	m_matrix.GetEulers( rot );
	Dbg_MsgAssert( rot[ X ] <= DTR( 180.0f ),( "Trig functions range from 0 to 360, not -180 to 180." ));
	Dbg_MsgAssert( rot[ Y ] <= DTR( 180.0f ),( "Trig functions range from 0 to 360, not -180 to 180." ));
	Dbg_MsgAssert( rot[ Z ] <= DTR( 180.0f ),( "Trig functions range from 0 to 360, not -180 to 180." ));
	
	m_quatrot_time_elapsed = 0.0f;

	// find out the amount we have to rotate:
	switch ( m_rest_orientation_type )
	{
		case ( 1 ):  // Upside down, or rightside up:
			land_on_top_or_bottom( rot );
			break;

		case ( 2 ):
			land_on_any_face( rot );
			break;

		case ( 3 ):
			land_traffic_cone( rot );
			break;
			
		case ( 0 ):
		default:
			Dbg_MsgAssert( 0,( "Unknown rest orientation type: %d", m_rest_orientation_type ));
			return;
			break;
	}
	if ( !mp_bounce_slerp )
	{
		mp_bounce_slerp = new Mth::SlerpInterpolator;
	}
	
	Mth::Matrix temp( rot[ X ], rot[ Y ], rot[ Z ] );
	mp_bounce_slerp->setMatrices( &m_matrix, &temp );
	m_quat_initialized = true;
}

#define QUAT_ROT_TIME 0.5f

bool CBouncyComponent::rotate_to_quat( void )
{
	
	float percent;
	m_quatrot_time_elapsed += m_time;
	if ( m_quatrot_time_elapsed >= QUAT_ROT_TIME )
	{
		percent = 1.0f;
	}
	else
	{
		percent = m_quatrot_time_elapsed / QUAT_ROT_TIME;
	}

	mp_bounce_slerp->getMatrix( &m_matrix, percent );
	return ( percent == 1.0f );
}

#define MAX_SKATER_VEL 1000.0f  // not exact... just about what the max would be.

void CBouncyComponent::do_bounce( void )
{
	m_old_pos = m_pos;
	
	int temp;
	switch ( m_substate )
	{
		case ( 0 ):
		
			// generate a new constant rotation:
			temp = Mth::Rnd( 8 );
			m_rotation[ X ] = DEGREES_TO_RADIANS( ( float ) ( ( m_rot_per_axis >> 2 ) + ( Mth::Rnd( ( m_rot_per_axis >> 2 ) * 3 ) ) ) );
			if ( temp & 1 )
			{
				m_rotation[ X ] *= -1.0f;
			}
			m_rotation[ Y ] = DEGREES_TO_RADIANS( ( float ) ( ( m_rot_per_axis >> 2 ) + ( Mth::Rnd( ( m_rot_per_axis >> 2 ) * 3 ) ) ) );
			if ( temp & 2 )
			{
				m_rotation[ Y ] *= -1.0f;
			}
			m_rotation[ Z ] = DEGREES_TO_RADIANS( ( float ) ( ( m_rot_per_axis >> 2 ) + ( Mth::Rnd( ( m_rot_per_axis >> 2 ) * 3 ) ) ) );
			if ( temp & 4 )
			{
				m_rotation[ Z ] *= -1.0f;
			}
			m_rotation[ W ] = 1.0f;
			m_quat_initialized = false;
			m_substate++;
		// intentional fall through:
		case ( 1 ):
		{
			Mth::Vector rot;
			m_pos += m_vel * m_time;
			if ( m_quat_initialized )
			{
				rotate_to_quat( );
			}
			else
			{
				rot = m_rotation;
				rot *= m_time;
				m_matrix.RotateLocal( rot );
			}
			
			
			// add the radius of the object so we don't stick through the ground:
			Mth::Vector velNormalized = m_vel;
			velNormalized.Normalize( );
			Mth::Vector colPoint = m_pos + ( velNormalized * m_bounce_collision_radius );
			m_vel[ Y ] -= m_gravity * m_time;
			CFeeler feeler;
			feeler.SetLine(m_old_pos, colPoint);
			if ( feeler.GetCollision())
			{
				#if 0
				// TODO - make it work, or remove if not used
				((CMovingObject*)(GetObject()))->RunNodeScript(0x2d075f90/*BounceScript*/,0x346b0c03/*BounceScriptParams*/);
				#else
				// This is also rather messy.  Seems like it would be better
				// to have this handled by an exception
				if (m_bounce_script)
				{
					Script::CScript *p_script=Script::SpawnScript(m_bounce_script,mp_bounce_script_params);
					Dbg_MsgAssert(p_script,("NULL p_script"));
					#ifdef __NOPT_ASSERT__
					p_script->SetCommentString("CBouncyComponent bounce script");
					#endif
					p_script->mpObject=GetObject();;
					p_script->Update();
				}
				#endif

				m_bounce_count++;
				Mth::Vector v;
				Mth::Vector r;
				float dP;
				v = m_pos - m_old_pos;
				Mth::Vector n;
				n = feeler.GetNormal();
				dP = Mth::DotProduct( v, n );
				r = ( dP * 2.0f ) * n;
				r -= v;
				float origVel = m_vel.Length( );
				m_vel = -r;
				m_vel.Normalize( origVel * m_bounce_mult );
				m_vel.RotateY( DTR( ( float ) ( Mth::Rnd( 2 * m_random_dir ) - m_random_dir ) ) );
				m_pos = feeler.GetPoint() - ( velNormalized * m_bounce_collision_radius );
				if ( fabsf( m_vel[ Y ] ) < ( m_min_bounce_vel + ( 2.0f *( m_gravity * m_time ) ) ) )
				{
					if ( m_rest_orientation_type )
					{
						if ( !m_quat_initialized )
						{
							set_up_quats( );
						}
						m_substate++;
						break;
					}
					
					
					#if 0
					// TODO - make it work, or remove if not used
					// This "HigerLevel" is probably redundant for bouncy obs
					((CMovingObject*)(GetObject()))->HigherLevel( false );
					#endif					
					
					SWITCH_STATE( BOUNCYOBJ_STATE_IDLE );
					if ( m_destroy_when_done )
					{
						GetObject()->MarkAsDead( );
					}
					else
					{
						#if 0
						// TODO - get exceptions working
						GetExceptionComponentFromObject( GetObject() )->FlagException( 0x61682835 ); // DoneBouncing
						#endif
					}
					return;
				}
				
				if ( origVel > m_min_bounce_vel * 2.0f )
				{
					if ( m_bounce_sound )
					{
						// adjust volume depending on magnitude of current velocity:
						#if 0
						// TODO - Get Bouncy object sounds working with components
						float vol;
						vol = 100.0f * ( origVel / ( MAX_SKATER_VEL * m_bounciness ) );
						float pitch = 80.0f + ( float ) Mth::Rnd( 40 );
						((CMovingObject*)(GetObject()))->PlaySound_VolumeAndPan( m_bounce_sound, vol, pitch );
						#endif
					}

					// bounce rot:
					if ( m_bounce_rot )
					{
						rot[ X ] = DEGREES_TO_RADIANS( ( float )( Mth::Rnd( m_bounce_rot * 2 ) - m_bounce_rot ) );
						rot[ Y ] = DEGREES_TO_RADIANS( ( float )( Mth::Rnd( m_bounce_rot * 2 ) - m_bounce_rot ) );
						rot[ Z ] = DEGREES_TO_RADIANS( ( float )( Mth::Rnd( m_bounce_rot * 2 ) - m_bounce_rot ) );
						m_matrix.RotateLocal( rot );
					}
					m_substate = 0;
				}
				else if ( m_rest_orientation_type )
				{
					if ( !m_quat_initialized )
					{
						// rotate to the resting orientation:
						set_up_quats( );
					}
				}
			}
			break;
		}

		case ( 2 ):
			if ( rotate_to_quat( ) )
			{
				#if 0
				((CMovingObject*)(GetObject()))->HigherLevel( false );
				#endif
				
				SWITCH_STATE( BOUNCYOBJ_STATE_IDLE );
				if ( m_destroy_when_done )
				{
					GetObject()->MarkAsDead( );
				}
			}
			break;
			
		default:
			Dbg_Message( 0, "Woah!  Fire matt immediately." );
			break;
	}

	#ifdef	__DEBUG_BOUNCY__	
//	printf ("%d: %s: (%.2f,%.2f,%.2f)\n",__LINE__,__PRETTY_FUNCTION__,m_pos[X],m_pos[Y],m_pos[Z]); 
	#endif
	
	GetObject()->m_pos = m_pos;
	GetObject()->m_matrix = m_matrix;
	GetObject()->SetDisplayMatrix(m_matrix);
	
	
}

void CBouncyComponent::bounce( const Mth::Vector &vel )
{

	#ifdef	__DEBUG_BOUNCY__	
//	printf ("%d: %s: (%.2f,%.2f,%.2f),(%.2f,%.2f,%.2f)\n",__LINE__,__PRETTY_FUNCTION__,vel[X],vel[Y],vel[Z]); 
	#endif
	
	m_vel = vel;
	if ( m_hit_sound )
	{
		// adjust volume depending on magnitude of current velocity:
		#if 0
		// TODO - sound components for bouncy object
		float vol;
		vol = 100.0f * ( m_vel.Length( ) / MAX_SKATER_VEL );
		float pitch = 80.0f + ( float ) Mth::Rnd( 40 );
		((CMovingObject*)(GetObject()))->PlaySound_VolumeAndPan( m_hit_sound, vol, pitch );
		#endif
	}
	if ( m_bounciness != 1.0f )
	{
		m_vel *= m_bounciness;
	}
	m_bounce_count = 0;
	#if 0
	// TODO - is HigherLevel even needed here?  or just cut and paste from some other object?
	((CMovingObject*)(GetObject()))->HigherLevel( true );
	#endif
	SWITCH_STATE( BOUNCYOBJ_STATE_BOUNCING );
	#if 0
	// TODO - exceptions working as a component
	GetExceptionComponentFromObject( GetObject() )->FlagException( 0x7dc30ba6 ); // Bounce
	#endif
}


// Given an object velocity, then calculate the velocity to bounce by
void CBouncyComponent::bounce_from_object_vel( const Mth::Vector &vel, const Mth::Vector &pos )
{

	#ifdef	__DEBUG_BOUNCY__	
//	printf ("%d: %s: (%.2f,%.2f,%.2f),(%.2f,%.2f,%.2f)\n",__LINE__,__PRETTY_FUNCTION__,vel[X],vel[Y],vel[Z],pos[X],pos[Y],pos[Z]); 
	#endif
	
	Mth::Vector vel2 = vel;
	
	float mag;
	mag = vel2.Length( );
	if ( mag < m_min_initial_vel )
	{
		mag = m_min_initial_vel;
		vel2.Normalize( mag );
	}
	Mth::Vector dir = m_pos - pos;
	dir.Normalize( mag );
	vel2 += dir * ( ( ( float ) ( 50 + Mth::Rnd( 100 ) ) ) / 100.0f );
	vel2.Normalize( mag );
	vel2[ Y ] = ( m_up_mag * ( mag + MAX_SKATER_VEL ) ) / ( MAX_SKATER_VEL * 2.0f );
	bounce(vel2);
}

void CBouncyComponent::GetDebugInfo( Script::CStruct* p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CBouncyComponent::GetDebugInfo"));
	// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  

	p_info->AddChecksum("m_collide_script",m_collide_script);
	p_info->AddStructure("mp_collide_script_params",mp_collide_script_params);

	p_info->AddChecksum("m_bounce_script",m_bounce_script);
	p_info->AddStructure("mp_bounce_script_params",mp_bounce_script_params);
#endif				 
}
	
}
