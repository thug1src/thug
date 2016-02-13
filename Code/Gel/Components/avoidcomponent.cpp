//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       AvoidComponent.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/02/02
//****************************************************************************

#include <gel/components/avoidcomponent.h>

#include <core/math.h>
									
#include <gel/object/compositeobject.h>

#include <gel/components/animationcomponent.h>
#include <gel/components/motioncomponent.h>
#include <gel/components/skeletoncomponent.h>

#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <gfx/skeleton.h>

#include <sk/modules/skate/skate.h>
#include <sk/objects/pathob.h>
#include <sk/objects/skater.h>
#include <sk/engine/feeler.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool ped_debug_on()
{
	return ( Script::GetInt( CRCD(0x59f8e435,"ped_debug"), false ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static int get_jump_direction( float rotAng )
{
	// split into forward/left/back/right quadrants 
	// by shifting the angle by 45 degrees

	rotAng += Mth::PI / 4.0f;
	if ( rotAng < 0 )
	{
		rotAng += Mth::PI * 2.0f;
	}

	int direction = (int)( rotAng / ( Mth::PI / 2.0f ) );

	return direction;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static float get_jump_distance( Script::CStruct* pParams, float rotAng )
{
	Dbg_Assert( pParams );

	int direction = get_jump_direction( rotAng );

	float distance = 0.0f;

	switch ( direction )
	{
		case 1:	// left
			pParams->GetFloat( "distLeft", &distance, Script::ASSERT );
			break;

		case 2:	// backward
			pParams->GetFloat( "distBack", &distance, Script::ASSERT );
			break;

		case 3:	// right
			pParams->GetFloat( "distRight", &distance, Script::ASSERT );
			break;
		
		case 0: // forward
		case 4:
		default:
			pParams->GetFloat( "distForward", &distance, Script::ASSERT );
			break;
	}

	return distance;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static bool collides_with( Mth::Vector& vec0, Mth::Vector& vec1 )
{
	CFeeler	feeler;
	
	feeler.m_start = vec0;
	feeler.m_end = vec1;

	return feeler.GetCollision();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*
void CAvoidComponent::draw_collision_circle( int numPoints, float radius, uint32 rgb0 )
{
	Mth::Vector v1, v2;

	Mth::Vector fwd( 0.0f, 0.0f, radius );

	v1 = m_pos + fwd;

	for ( int i = 0; i < numPoints; i++ )
	{
		fwd.RotateY( 2.0f * Mth::PI / (float)numPoints );
		v2 = GetObject()->m_pos + fwd;

		if ( !collides_with( GetObject()->m_pos, v2 ) )
		{
			Gfx::AddDebugLine(v1,v2,rgb0,rgb0,1);
		}

		v1 = v2;
	}
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAvoidComponent::play_appropriate_jump_anim( float rotAng, Script::CStruct* pParams )
{
	// split into forward/left/back/right quadrants 
	// by shifting the angle by 45 degrees

	rotAng += Mth::PI / 4.0f;
	if ( rotAng < 0 )
	{
		rotAng += Mth::PI * 2.0f;
	}

	int direction = (int)( rotAng / ( Mth::PI / 2.0f ) );
	// 0 = forward
	// 1 = left
	// 2 = back
	// 3 = right

	Dbg_Assert( pParams );

	uint32 animChecksum;
	bool flipped;

	switch ( direction )
	{
		case ( 0 ):	// JUMP FORWARD
		case ( 4 ):
//			Dbg_Message( "Ped jumps forward to avoid skater" );
			pParams->GetChecksum( CRCD(0x4de2c9e8,"JumpForward"), &animChecksum, true );
			break;

		case ( 1 ):	// JUMP LEFT
			{
//				Dbg_Message( "Ped jumps left to avoid skater" );
				if ( pParams->GetChecksum( CRCD(0x733da756,"JumpLeft"), &animChecksum, false ) )
				{
					flipped = 0;
				}
				else
				{
					pParams->GetChecksum( CRCD(0xa7a0dd72,"JumpRight"), &animChecksum, true );
					flipped = 1;
				}
				
				Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
				if ( pAnimationComponent )
				{
					pAnimationComponent->FlipAnimation( GetObject()->GetID(), flipped, 0, true );
				}
			}
			break;

		case ( 2 ):	// JUMP BACK
//			Dbg_Message( "Ped jumps back to avoid skater" );
			pParams->GetChecksum( CRCD(0x64948109,"JumpBack"), &animChecksum, true );
			break;

		case ( 3 ):	// JUMP RIGHT
			{
//				Dbg_Message( "Ped jumps right to avoid skater" );
				if ( pParams->GetChecksum( CRCD(0xa7a0dd72,"JumpRight"), &animChecksum, false ) )
				{
					flipped = 0;
				}
				else
				{
					pParams->GetChecksum( CRCD(0x733da756,"JumpLeft"), &animChecksum, true );
					flipped = 1;
				}
				
				Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
				if ( pAnimationComponent )
				{
					pAnimationComponent->FlipAnimation( GetObject()->GetID(), flipped, 0, true );
				}
			}
			break;

		default:
			Dbg_MsgAssert( 0, ( "Ped jumped in unrecognized direction: %d", direction ) );
			return;
			break;
			
	}
	
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );

    if ( pAnimationComponent )
    {
        pAnimationComponent->PlaySequence( animChecksum, 0 );
        m_jumpTime = pAnimationComponent->AnimDuration( animChecksum );
    }
    else
    {
        Dbg_MsgAssert( 0, ( "No animation component" ) );
    }
}
                                                                 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CAvoidComponent::get_ideal_heading()
{
	Mdl::Skate* skate = Mdl::Skate::Instance();
				
	// assume pedestrians only appear in "1 player" game for now...
	// If this assumption becomes incorrect, cycle through skate->GetNumSkaters( )
	// and find the closest.
	CSkater *pSkater = skate->GetLocalSkater();
	Dbg_MsgAssert( pSkater, ( "Ped avoidance only works in 1-player games." ) );
	
	Mth::Vector jumpDir;

	// find the 2 directions perpendicular to the skater's line of motion...
	// and select the direction that will take us further away from the
	// skater
	
	jumpDir = (pSkater->GetDisplayMatrix())[Mth::AT];	
	jumpDir[Y] = 0.0f;
	jumpDir.Normalize( );
	
	Mth::Vector upVect( 0.0f, 1.0f, 0.0f, 0.0f );
	jumpDir = Mth::CrossProduct( jumpDir, upVect );

	// chose the direction that's farther...
	Mth::Vector posPlus;
	Mth::Vector posMinus;
	posPlus = GetObject()->m_pos + jumpDir;
	posMinus = GetObject()->m_pos - jumpDir;
			
	if ( Mth::DistanceSqr( pSkater->m_pos, posPlus ) < Mth::DistanceSqr( pSkater->m_pos, posMinus ) )
	{
		jumpDir[X] = -jumpDir[X];
		jumpDir[Z] = -jumpDir[Z];
	}
	
	return Mth::RadToDeg( Mth::GetAngle( GetObject()->m_matrix, jumpDir ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// This static function is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
CBaseComponent* CAvoidComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CAvoidComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CAvoidComponent::CAvoidComponent() : CBaseComponent()
{
	SetType( CRC_AVOID );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CAvoidComponent::~CAvoidComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CAvoidComponent::InitFromStructure( Script::CStruct* pParams )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CAvoidComponent::Update()
{
#ifdef __NOPT_ASSERT__
//	if ( ped_debug_on() )
	{
		//Gfx::AddDebugCircle( m_pos, 20, 15.0f, 0x0000ffff, 1 );
		//Gfx::AddDebugCircle( m_stored_pos, 20, 15.0f, 0x00ff00ff, 1 );
		//draw_collision_circle( 100, 50.0f, 0xff0000ff );	
	}
#else
	// Doing nothing, so tell code to do nothing next time around
	Suspend(true);
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CAvoidComponent::jump( Mth::Matrix& mat0, Mth::Matrix& mat1, float rotAng, float jumpDist, float maxHerdDistance )
{
	mat1 = mat0;
	
	mat1.RotateYLocal( Mth::DegToRad(rotAng) );

	// test for an extra 15 units (1.25 feet) radius of clearance 
	// around the player to account for his actual body
	// (need to rotate it slightly up, to account for peds
	// that are below ground level...  not sure why this
	// happens, but will check it out in THPS5...)
	const float clearanceDist = 15.0f;
	Mth::Vector collisionTestPos = mat1.Transform( Mth::Vector( 0.0f, 10.0f, jumpDist + clearanceDist ) );
	
#ifdef __NOPT_ASSERT__
/*
	if ( ped_debug_on() )
	{
		Gfx::AddDebugCircle( collisionTestPos, 20, 15.0f, 0x00ff00ff, 10 );
	}
*/
#endif

	if ( collides_with( mat0.GetPos(), collisionTestPos ) )
	{
		Dbg_Message( "Collides with something!" );
		return false;
	}
	else
	{
		// build target matrix
		Mth::Vector targetPos = mat1.Transform( Mth::Vector( 0.0f, 0.0f, jumpDist ) );
		mat1[Mth::POS] = targetPos;

		Mdl::Skate* skate = Mdl::Skate::Instance();
		CSkater *pSkater = skate->GetLocalSkater();
		Dbg_MsgAssert( pSkater, ( "Ped avoidance only works in 1-player games." ) );
		
		Mth::Vector toSkater = targetPos - pSkater->m_pos;
		if ( toSkater.Length() < 30.0f )
		{
			printf( "Too close to skater %f!\n", toSkater.Length() );
			return false;
		}

		// check if we're going too far from the original point
		// TODO:  check for distance from the path, not the point
		// don't allow going too far from the original path,
		// or else you'll be able to herd them into weird
		// positions...
		Mth::Vector totalAvoidVec = targetPos - m_avoidOriginalPos;
		totalAvoidVec[Y] = 0.0f;

		if ( totalAvoidVec.Length() >= maxHerdDistance )
		{
			Dbg_Message( "Too far from the original avoid point %f", totalAvoidVec.Length() );
			return false;
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent::EMemberFunctionResult CAvoidComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch (Checksum)
	{
		// @script | Ped_SelectAvoidPoint | Will cause the pedestrian to jump out of
		// the way of the player.  He will land on a circle, JumpDist away from
		// the last postition stored in Obj_StorePos.  Once he lands the jump,
		// the LandedEscapeJump exception is triggered. 
		// @parmopt name | JumpLeft | jumpLeft | name of anim to play when jumping left
		// @parmopt name | JumpRight | jumpRight | name of anim to play when jumping right
		// @parmopt name | JumpForward | jumpForward | name of anim to play when jumping forward
		// @parmopt name | JumpBack | jumpBack | name of anum to play when jumping back
		// position where the ped jumps to 
		case 0x6157cbc0:	// Ped_SelectAvoidPoint
		{
			// this is the starting point...
			Mth::Matrix mat0;
			mat0 = GetObject()->GetDisplayMatrix();
			mat0[Mth::POS] = GetObject()->m_pos;

			// this is where we will eventually end up
			Mth::Matrix mat1;

			float maxHerdDistance = 100.0f;
			pParams->GetFloat( "maxHerdDistance", &maxHerdDistance, Script::NO_ASSERT );

			float idealHeading = get_ideal_heading();

			if ( jump(mat0, mat1, idealHeading, get_jump_distance( pParams, idealHeading ), maxHerdDistance ) )
			{
			   Dbg_Message( "Selecting ideal jump (ang=%f newpos=(%f, %f, %f)",
							idealHeading,
							mat1[Mth::POS][X],
							mat1[Mth::POS][Y],
							mat1[Mth::POS][Z] );   			
			}

			// GJ:  Try a few more directions before trying 
			// to jump back to the original point...
			else if ( jump(mat0, mat1, idealHeading + 180.0f, get_jump_distance( pParams, idealHeading + 180.0f ), maxHerdDistance) )
			{
				// jump to the other side was successful!
				printf( "Jumping to the other side...\n" );
			}
			else if ( jump(mat0, mat1, idealHeading - 90.0f, get_jump_distance( pParams, idealHeading - 90.0f ), maxHerdDistance) )
			{
				// jump away was successful!
				printf( "Jumping away...\n" );
			}
			else 
			{
				Dbg_Message( "Jumping back towards original point %f %f %f",
							 m_avoidOriginalPos[X],
							 m_avoidOriginalPos[Y],
							 m_avoidOriginalPos[Z] );

				// otherwise, jump back towards original point
				Mth::Vector jumpDir;
				jumpDir = m_avoidOriginalPos - mat0[Mth::POS];

				float rotAng;
				rotAng = Mth::GetAngle( GetObject()->m_matrix, jumpDir );

				bool success = jump( mat0, mat1, rotAng, get_jump_distance(pParams, rotAng), maxHerdDistance );

				if ( !success )
				{
					// jump in place
					mat1 = mat0;

					return CBaseComponent::MF_FALSE;
				}
			}  			

			// don't actually want to rotate the ped
			// (just translate it)
			mat1[Mth::RIGHT] = mat0[Mth::RIGHT];
			mat1[Mth::UP] = mat0[Mth::UP];
			mat1[Mth::AT] = mat0[Mth::AT];

			m_avoidInterpolator.setMatrices( &mat0, &mat1 );
			m_avoidAlpha = 0.0f;

#ifdef __NOPT_ASSERT__
/*
			if ( ped_debug_on() )
			{
				Gfx::AddDebugCircle( mat0[Mth::POS], 20, 15.0f, 0x00ff00ff, 100 );
				Gfx::AddDebugCircle( mat1[Mth::POS], 20, 15.0f, 0x00ff00ff, 100 );
			}
*/
#endif

			Mth::Vector jumpDir;
			jumpDir = mat1[Mth::POS] - mat0[Mth::POS];

			float rotAng;
			rotAng = Mth::GetAngle( GetObject()->m_matrix, jumpDir );

			// rotAng of zero means jump forward, so:
			play_appropriate_jump_anim( rotAng, pParams );
		}
		break;

		// @script | Ped_BackOnOriginalPath |
		case 0xd2aa4d2d:	// Ped_BackOnOriginalPath
		{
			float safeDist;
			safeDist = pParams->GetFloat( "dist", &safeDist, Script::ASSERT );

			Mth::CBBox theBox;
			theBox.SetMin( m_avoidOriginalPos - Mth::Vector( safeDist, safeDist, safeDist ) );
			theBox.SetMax( m_avoidOriginalTargetPos + Mth::Vector( safeDist, safeDist, safeDist ) );

			if ( theBox.Within( GetObject()->m_pos ) )
			{
				printf( "Intersects!\n" ); 
			//	return CBaseComponent::MF_TRUE;
			}
			else
			{
				printf( "Doesn't intersect!\n" ); 
				return CBaseComponent::MF_FALSE;
			}

#if 0
			Mth::Vector originalPath;
			originalPath = m_avoidOriginalTargetPos - m_avoidOriginalPos;

			Mth::Vector upVect( 0.0f, 1.0f, 0.0f, 0.0f );

			Mth::Vector normalToOriginalPath;
			normalToOriginalPath = Mth::CrossProduct( originalPath, upVect );

			// is the distance to the original path
			// choose the direction that's closer...
			Mth::Vector posPlus;
			Mth::Vector posMinus;
			posPlus = GetObject()->m_pos + normalToOriginalPath * safeDist;
			posMinus = GetObject()->m_pos - normalToOriginalPath * safeDist;

			Mth::Line originalLine( m_avoidOriginalTargetPos, m_avoidOriginalPos );
			Mth::Line currentLine1( m_avoidOriginalTargetPos, posPlus ); 
			Mth::Line currentLine2( m_avoidOriginalTargetPos, posMinus ); 

			Mth::Vector my_pt;
			Mth::Vector other_pt;
			float dummyFloat;

			if ( Mth::LineLineIntersect( originalLine, currentLine1, &my_pt, &other_pt, &dummyFloat, &dummyFloat, false ))
			{
				Mth::Vector difference = my_pt - other_pt;
				printf( "intersects with current line 1: %f\n", difference.Length() );
			//	return CBaseComponent::MF_FALSE;
			}
			else if ( Mth::LineLineIntersect( originalLine, currentLine2, &my_pt, &other_pt, &dummyFloat, &dummyFloat, false ))
			{
				Mth::Vector difference = my_pt - other_pt;
				printf( "intersects with current line 2: %f\n", difference.Length() );
			//	return CBaseComponent::MF_FALSE;
			}
			else
			{
				printf( "does not intersect\n" );
		//		return CBaseComponent::MF_FALSE;
			}
#endif
			return CBaseComponent::MF_FALSE;
		}
		break;

		// @script | Ped_MoveTowardsAvoidPoint |
		case 0x41226942:	// Ped_MoveTowardsAvoidPoint
		{
			Mth::Matrix mat0;
			m_avoidAlpha += ( m_jumpTime * ( 1.0f / 60.0f ) * Tmr::FrameRatio() );
			m_avoidInterpolator.getMatrix( &mat0, m_avoidAlpha );
			GetObject()->m_pos = mat0[Mth::POS];
			GetObject()->m_matrix = mat0;
			GetObject()->m_matrix[Mth::POS] = Mth::Vector(0.0f, 0.0f, 0.0f, 1.0f);
		}
		break;

		// @script | Ped_AvoidPointReached |
		case 0x3c7f8f3e:	// Ped_AvoidPointReached
		{
			Dbg_Assert( GetAnimationComponentFromObject( GetObject() ) );
			return ( m_avoidAlpha >= 1.0f && GetAnimationComponentFromObject( GetObject() )->IsAnimComplete() ) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		break;

		case 0x6a4bca91:	// Ped_RememberCurrentPosition
			m_avoidOriginalPos = GetObject()->m_pos;
		break;

		case 0x3fb422e8:	// Ped_RememberNextWaypoint
		{
			Obj::CMotionComponent* pMotionComponent = GetMotionComponentFromObject( GetObject() );
			if ( pMotionComponent && pMotionComponent->GetPathOb() )
			{
				m_avoidOriginalTargetPos = pMotionComponent->GetPathOb()->m_wp_pos_to;
			}
			else
			{
				m_avoidOriginalTargetPos = GetObject()->m_pos;
			}
		}
		break;

		case 0x539a32dc:	// Ped_RememberStickToGround
		{
			Obj::CMotionComponent* pMotionComponent = GetMotionComponentFromObject( GetObject() );
			Dbg_Assert( pMotionComponent );
			m_avoidOriginalStickToGround = ( pMotionComponent->m_movingobj_flags |= MOVINGOBJ_FLAG_STICK_TO_GROUND );
		}	
		break;

		case 0x53bc3b4e:	// Ped_RestoreStickToGround
		{
			Obj::CMotionComponent* pMotionComponent = GetMotionComponentFromObject( GetObject() );
			if ( pMotionComponent )
			{
				if ( m_avoidOriginalStickToGround )
				{
					pMotionComponent->m_movingobj_flags |= MOVINGOBJ_FLAG_STICK_TO_GROUND;
				}
				else
				{
					pMotionComponent->m_movingobj_flags &= ~MOVINGOBJ_FLAG_STICK_TO_GROUND;
				}
			}
		}
		break;

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

void CAvoidComponent::GetDebugInfo( Script::CStruct* p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to C......Component::GetDebugInfo"));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums

	/*
	p_info->AddInteger("m_never_suspend",m_never_suspend);
	p_info->AddFloat("m_suspend_distance",m_suspend_distance);
	*/
	
	// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo( p_info );	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}
