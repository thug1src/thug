//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       PathOb.cpp
//* OWNER:          Matt Duncan
//* CREATION DATE:  11/2/2000
//****************************************************************************

/*
	Pathob.cpp
	
	This module contains path-navigation functionality.

	A moving object contains a pointer to this pathob.  When
	the moving object wants to follow a path along waypoints, it
	just initializes this pathob with the current position and tells
	the pathob where it wants to go.  Each frame, the moving object
	calls TraversePath() and sends in the distance travelled, and
	its own heading.  The pathob moves accordingly, but the point remains
	on the plane defined by the points of the path.

	The pathob is its own entity: it's essentially a point in
	space and an orientation.  When the moving object calls TraversePath(),
	the pathob just does the math and figures out where along the path
	to move to.  The moving object takes the new path-ob position, finds
	the ground above or below this position, and possibly sets its heading
	so that it's looking the correct direction down the path.

   	The pathob logic handles a simple path between two points... it also
	handles the curves from one simple path to the next (so the path along
	three waypoints, but instead of a sharp turn the pathob follows a curve
	between the two lines).

	As the pathob hits waypoints, it communicates to the moving object (in
	case the moving object needs to run a script at the waypoint).
*/

#include <objects/pathob.h>

#include <core/debug.h>

#include <gel/components/motioncomponent.h>
#include <gel/inpman.h>
#include <gel/mainloop.h>
#include <gel/object/compositeobject.h>
#include <gel/scripting/script.h>

#include <gfx/debuggfx.h>

#include <sk/scripting/nodearray.h>

#include <sys/timer.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Obj
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

// PATHOB flags:
#define PATHOBFLAG_LINEAR_INTERP			( 1 << 0 )
#define PATHOBFLAG_END_OF_PATH				( 1 << 1 )
#define PATHOBFLAG_SEND_HIT_WAYPOINT_MSG	( 1 << 2 )

#define MAX_RATIO_OF_HYPOTENUSE_TO_OPPOSITE_FOR_TURN_CIRCLE    6

// NOTE: if you change this enum, make sure to update the GetDebugInfo func
enum
{
	PATHOB_STATE_UNINITIALIZED,
	PATHOB_STATE_FOLLOW_LINE,
	PATHOB_STATE_FOLLOW_CURVE,
	PATHOB_STATE_LINEAR_INTERP,
	PATHOB_STATE_IDLE,
};

enum
{
	TURN_METHOD_LINEAR_INTERP,
	TURN_METHOD_CURVE,
	TURN_METHOD_INSTANT_TURN,
};

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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPathOb::CPathOb( CCompositeObject* pObj )
{
	mp_object = pObj;
	m_turn_matrix.Ident();
	
	m_wp_pos_from.Set();
	m_wp_pos_next.Set();
	m_wp_pos_to.Set();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPathOb::~CPathOb( void )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPathOb::Busy( void )
{
	switch ( m_pathob_state )
	{
		case PATHOB_STATE_UNINITIALIZED:
		case PATHOB_STATE_IDLE:
			return false;
		default:
			return true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPathOb::SetUpNavPointLineMotion( void )
{
	// assumes we're already on the line.
	
	// pre-calculations for navigating between the waypoints...
	
	Mth::Vector vect_first_to_pos = m_nav_pos - m_wp_pos_from;
	m_dist_traveled = vect_first_to_pos.Length();
			  
	Mth::Vector vect_first_to_second = m_wp_pos_from - m_wp_pos_to;
	m_dist_to_wp = vect_first_to_second.Length();
    
    m_dist_traveled += m_extra_dist;
	m_extra_dist = 0;
	
	/* Shouldn't need this, now that curve code is independent of turn dist
    if ( m_dist_to_wp < m_current_turn_dist )
	{
		Dbg_MsgAssert( 0,( "Waypoints %d and %d too close or turn radius too large.", m_node_from, m_node_to ));
	}*/
	
	// set up the heading:
	m_turn_matrix[ Z ] = m_wp_pos_to - m_wp_pos_from;
	m_turn_matrix[ Z ].Normalize();
	
	SetUpTurnDist();
	
	return( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CPathOb::SetUpTurningCircle( void )
{
	Mth::Vector vectA, vectB;
	
	// m_nav_pos holds one point on the circle.
	// on the line from m_wp_pos_from to m_wp_pos_to
	// (at the distance of nav pos from m_wp_pos_from)
	// will be the other point on the circle.
	vectB = m_nav_pos - m_wp_pos_from;

	float actualTurnDist = vectB.Length();
						
	m_circle_arrival_pos = m_wp_pos_from;
	vectA = m_wp_pos_to - m_wp_pos_from;
	float distBetweenWps = vectA.Length();
    vectA *= ( actualTurnDist / distBetweenWps );
	m_circle_arrival_pos += vectA;

	// That's all the info from the turning circle we need, if we're doing a linear interpolation:
	if ( m_pathob_flags & PATHOBFLAG_LINEAR_INTERP )
	{
		return TURN_METHOD_LINEAR_INTERP;
	}
	
//	Dbg_MsgAssert( actualTurnDist, "Traveling too fast for turn dist... increase turn dist or decrease velocity." );
	if ( !actualTurnDist )
	{
		return TURN_METHOD_INSTANT_TURN;
	}
	
	// find the radius of the turning circle
	// the angle between the line we're on and the line to
	// the arrival pos on the next line is the same as half
	// the angle that the two radii where the circle touches
	// the waypoint lines form...  use the similar triangles
	// to get the circle radius:
	vectA = m_circle_arrival_pos - m_nav_pos;
	vectA /= 2; // vector from current pos to midpoint along current pos to arrival pos...
	float lengthPosToMidpoint = vectA.Length();

	vectB = m_nav_pos + vectA;
	vectB -= m_wp_pos_from;
	float distWPtoMP = vectB.Length();
	
	if ( ( !distWPtoMP ) | ( ( distWPtoMP * MAX_RATIO_OF_HYPOTENUSE_TO_OPPOSITE_FOR_TURN_CIRCLE ) < lengthPosToMidpoint ) )
	{
		// almost a straight line... use linear interpolation method:
		return TURN_METHOD_LINEAR_INTERP;
	}
	// to understand this, draw the triangles and derive circle radius
	// in terms of the known lengths of the similar triangles:
	m_circle_radius = lengthPosToMidpoint * actualTurnDist;
	m_circle_radius /= distWPtoMP;

	if ( !m_circle_radius )
	{
		// tiny turn radius... snap to the point and shit like that!
		return TURN_METHOD_INSTANT_TURN;
	}
	
	// find the location of the center of the circle...
	// the triangle formed by the current pos, the waypoint pos,
	// and the midpoint between the current pos and the arrival pos
	// is a similar triangle to the current pos, the waypoint pos,
	// and the center of the circle...  therefore the distance from
	// the center of the circle to the waypoint pos can be found:
	float distFromWaypointToCircleCenter = actualTurnDist * actualTurnDist;
	distFromWaypointToCircleCenter /= vectB.Length();
	// take the vector from the waypoint to the midpoint, make it this length,
	// and viola we have the circle center:
	m_circle_center = vectB;
	m_circle_center.Normalize( distFromWaypointToCircleCenter );
	m_circle_center += m_wp_pos_from;

	// test the distance from the current pos... should equal the frickin' radius:
	/*
	{
		Mth::Vector temp = m_circle_center;
		temp -= m_nav_pos;
		Dbg_Message( "dist %f radius %f", temp.Length(), m_circle_radius );
	}*/
	
	if ( m_circle_radius < m_min_turn_circle_radius )
	{
		Dbg_Message( "angle too acute to path between nodes %d and %d", m_node_from, m_node_to );
		Dbg_MsgAssert( 0,( "Turn is too tight for this object...  your waypoints are just too acute for me!" ));
	}

	// find the "rotation axis" (the y axis if we're traveling along the xz plane)
	// and set up a rotation matrix where the axis along the 1st waypoint path [ r0 ]
	// sweeps along the circle as we rotate about the 'up'[ r1 ] axis in the direction if r2...
	m_turn_matrix[ X ] = m_circle_center - m_nav_pos;
	m_turn_matrix[ Z ] = m_circle_center - m_circle_arrival_pos;
	if ( !( m_turn_matrix[ X ].Length() && m_turn_matrix[ Z ].Length() ) )
	{
		return TURN_METHOD_INSTANT_TURN;
	}
	m_turn_matrix[ X ].Normalize();
	m_turn_matrix[ Z ].Normalize();

	// while we have these two vectors, find the angle between them...
	float angCos = Mth::DotProduct( m_turn_matrix[ X ], m_turn_matrix[ Z ] );
#ifdef __PLAT_NGC__
	float	ang = angCos;
	if ( ang > 1.0f ) ang = 1.0f;
	if ( ang < -1.0f ) ang = -1.0f;
	m_arc_ang = acosf( ang );  // angles given in radians!
#else
	m_arc_ang = acosf( angCos );  // angles given in radians!
#endif		// __PLAT_NGC__
	
	// find the total circumference of the arc,
	// which is the circumference of the circle ( ang * radius )
	// multiplied by ang / ( 2 * pi ) [ the fraction of the circle we actually traverse... ]
	m_arc_dist = m_circle_radius * m_arc_ang;
	
	// continue setting up the matrix now...
	m_turn_matrix[ Y ] = Mth::CrossProduct( m_turn_matrix[ X ], m_turn_matrix[ Z ] );
	if ( !m_turn_matrix[ Y ].Length() )
	{
		return TURN_METHOD_INSTANT_TURN;
	}
	m_turn_matrix[ Y ].Normalize();
	m_turn_matrix[ Z ] = Mth::CrossProduct( m_turn_matrix[ X ], m_turn_matrix[ Y ] );
	if ( !m_turn_matrix[ Z ].Length() )
	{
		return TURN_METHOD_INSTANT_TURN;
	}
	m_turn_matrix[ Z ].Normalize();
	
    // Phew!!!  Now we can rotate this matrix along r1 and get the position on
	// the circle by taking r0 multiplied by the circle radius and adding that to
	// the center of the circle... and r2 will be the direction the car is pointed!!
	   
	// now to find the position along the arc, just get the
	// current distance traveled along the arc, divide by
	// the total circumference of the arc, multiply by the
	// angle the arc traverses to get the angle currently
	// traversed...  by rotating our curve matrix along
	// y in this fashion, we can find the position on the
	// circle!
	
	m_dist_traveled = m_extra_dist;
	m_extra_dist = 0;
	
//	Gfx::AddDebugLine( m_nav_pos, m_circle_center, MAKE_RGB( 255, 0, 0 ), MAKE_RGB( 255, 0, 0 ) );
//	Gfx::AddDebugLine( m_circle_center, m_circle_arrival_pos, MAKE_RGB( 0, 0, 255 ), MAKE_RGB( 0, 255, 0 ) );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPathOb::UpdateNavPosAlongPath( void )
{	
	Mth::Vector pointAlongPath;
	
	m_nav_pos = m_wp_pos_from;
	pointAlongPath = m_wp_pos_to - m_wp_pos_from;
	pointAlongPath *= ( m_dist_traveled / m_dist_to_wp );
	m_nav_pos += pointAlongPath;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPathOb::UpdateNavPosAlongCurve( float distTraveled )
{	
	float fractionOfTotal;
	fractionOfTotal = distTraveled / m_arc_dist;

	m_y_rot = m_arc_ang * fractionOfTotal;
	m_turn_matrix.RotateYLocal( m_y_rot );

	Mth::Vector temp;
	temp = m_turn_matrix[ Y ];
	if ( temp[ Y ] > 0.0f )
	{
		m_y_rot *= -1.0f;
	}

	m_nav_pos = m_turn_matrix[ X ] * -m_circle_radius;
	m_nav_pos += m_circle_center;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPathOb::AlignToPath( void )
{   
	m_turn_matrix[ Z ] = m_wp_pos_to - m_nav_pos;
	m_turn_matrix[ Z ].Normalize();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// If the distance between the next two waypoints is less than
// the turn dist, decrease the turn dist and shit (otherwise
// the arrival pos will be on the line but PAST the two waypoints
// rather than between them!
bool CPathOb::SetUpTurnDist( void )
{	
	Mth::Vector temp;
	float wpLength;

	if ( m_enter_turn_dist )
	{
		if ( !SkateScript::GetNumLinks( m_node_to ) )
			return false;
        SkateScript::GetPosition( SkateScript::GetLink( m_node_to, 0 ), &temp );
		temp -= m_wp_pos_to;
		// Need to optimise?  Store this length instead of recalculating it
		// when getting next waypoint!
		wpLength = temp.Length();
		if ( wpLength < m_enter_turn_dist )
		{
			m_current_turn_dist = wpLength / 2.0f;
			temp = m_wp_pos_to - m_nav_pos;
			float distToCurrentWP = temp.Length();
			if ( m_current_turn_dist > distToCurrentWP )
			{
				m_current_turn_dist = distToCurrentWP;
			}
			m_pathob_flags |= PATHOBFLAG_LINEAR_INTERP;
			return true;
		}
	}

	m_pathob_flags &= ~PATHOBFLAG_LINEAR_INTERP;

	m_current_turn_dist = m_enter_turn_dist;
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPathOb::NewPath( int nodeNum, const Mth::Vector &currentPos )
{
	m_pathob_state = PATHOB_STATE_UNINITIALIZED;
	m_pathob_flags = 0;
	m_node_to = nodeNum;
	m_dist_traveled = m_extra_dist = 0;
	m_nav_pos = m_wp_pos_from = currentPos;
	SetUpPath();
	
	while ( Mth::Distance( m_nav_pos, m_wp_pos_to ) < 1.0f )
	{
		if ( !GetNextWaypoint() )
		{
			return;
		}
	}
	
	AlignToPath();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Set up the path according to m_node_from and m_node_to...
// Do this after getting the next waypoint in a path, or when
// beginning a new path.  m_node_from should indicate where we
// are now, m_node_to should be the node index of where we're
// going!
void CPathOb::SetUpPath( void )
{	
	// get data out of waypoints...
	Script::CScriptStructure *pNode = SkateScript::GetNode( m_node_to );
	SkateScript::GetPosition( pNode, &m_wp_pos_to );
	
	if ( !SkateScript::GetNumLinks( m_node_to ) )
	{
		// the path will end on arrival to m_node_to:
		m_pathob_flags |= PATHOBFLAG_END_OF_PATH;
	}
	else
	{
		m_pathob_flags &= ~PATHOBFLAG_END_OF_PATH;
	}

//	Gfx::AddDebugLine( m_wp_pos_from, m_wp_pos_to, MAKE_RGB( 255, 0, 0 ), MAKE_RGB( 0, 0, 255 ) );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPathOb::DrawPath()
{
	Gfx::AddDebugLine( m_wp_pos_from, m_wp_pos_to, MAKE_RGB( 255, 0, 0 ), MAKE_RGB( 0, 0, 255 ) );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPathOb::GetNextWaypoint( void )
{
	
	int numLinks;
	numLinks = SkateScript::GetNumLinks( m_node_to );
	if ( !numLinks )
	{
		return false;
	}
	m_node_from = m_node_to;
	Dbg_Assert( GetMotionComponentFromObject( mp_object ) );
	if ( GetMotionComponentFromObject(mp_object)->PickRandomWaypoint() )
	{
		m_node_to = SkateScript::GetLink( m_node_to, Mth::Rnd( numLinks ) );
	}
	else
	{
		m_node_to = SkateScript::GetLink( m_node_to, 0 );
	}
	m_wp_pos_from = m_wp_pos_to;
	
	SetUpPath();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPathOb::SetHeading( Mth::Vector *p_heading )
{
	// Gets the current heading:	
	*p_heading = m_turn_matrix[ Z ];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CPathOb::GetDistToWaypoint( void )
{
	switch ( m_pathob_state )
	{
		case PATHOB_STATE_UNINITIALIZED:
			//Dbg_Message( "%f line %d", HUGE_DISTANCE, __LINE__ );		
			return HUGE_DISTANCE;
			break;
		
		case PATHOB_STATE_FOLLOW_LINE:
			//Dbg_Message( "%f line %d", m_dist_to_wp - m_dist_traveled, __LINE__ );		
			return ( m_dist_to_wp - m_dist_traveled );
			break;
		
		case PATHOB_STATE_FOLLOW_CURVE:
			if ( m_dist_traveled < ( m_arc_dist / 2 ) )
			{
				//Dbg_Message( "%f line %d", ( m_arc_dist / 2 ) - m_dist_traveled, __LINE__ );		
				return ( ( m_arc_dist / 2 ) - m_dist_traveled );
			}
			else
			{
				Mth::Vector temp = m_wp_pos_to - m_wp_pos_from;
				//Dbg_Message( "%f line %d", temp.Length(), __LINE__ );		
				return temp.Length();
			}
			break;
		
		case PATHOB_STATE_LINEAR_INTERP:
			if ( m_dist_traveled < m_dist_to_wp )
			{
				//Dbg_Message( "%f line %d", m_dist_to_wp - m_dist_traveled, __LINE__ );		
				return ( m_dist_to_wp - m_dist_traveled );
			}
			else
			{
				Mth::Vector temp = m_wp_pos_to - m_wp_pos_from;
				//Dbg_Message( "%f line %d", temp.Length(), __LINE__ );		
				return temp.Length();
			}
			break;
		
		default:
			Dbg_MsgAssert( 0, ( "unhandled pathob state" ) );
			break;
	}
	
	return HUGE_DISTANCE;
}

#if FINDING_GLITCHES_IN_PATH_TRAVERSAL
// this function tracks down glitches in the path traversal...
void CPathOb::FindGlitch( void )
{	
	static float totalDist = 0;
	static int numEntries = 0;
	float thisDist = Distance( m_old_nav_pos, m_nav_pos );
	totalDist += thisDist;
	numEntries++;
	float averageDist = totalDist / numEntries;
	if ( numEntries == 100 )
	{
		numEntries = 50;
		totalDist /= 2;
	}
	if ( fabs( thisDist - averageDist ) > 4.0f )
	{
		Gfx::AddDebugLine( m_nav_pos, m_old_nav_pos, MAKE_RGB( 255, 255, 255 ), MAKE_RGB( 255, 255, 255 ) );
		Dbg_Message( "off by %f dist %f state %d prev state %d", thisDist - averageDist, thisDist, m_pathob_state, m_prev_pathob_state );
	}
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPathOb::PrepareTurn( void )
{	
	int retVal = SetUpTurningCircle();
	switch ( retVal )
	{
		case TURN_METHOD_LINEAR_INTERP:
		{
			// store the current pos in circle center:
			// the angle is too wide (too close to PI radians)...
			// just move along the two lines and gradually turn and shit like that...
			
			m_circle_center = m_nav_pos;
			Mth::Vector temp = m_nav_pos - m_wp_pos_from;
			m_dist_to_wp = temp.Length();
			m_dist_traveled = m_extra_dist;
			m_pathob_state = PATHOB_STATE_LINEAR_INTERP;
			break;
		}
		
		case TURN_METHOD_CURVE:
			m_pathob_state = PATHOB_STATE_FOLLOW_CURVE;
			break;
		
		case TURN_METHOD_INSTANT_TURN:
			// just hit the waypoint, and set up the path for the next one:
			Dbg_Assert( GetMotionComponentFromObject( mp_object ) );
			GetMotionComponentFromObject( mp_object )->HitWaypoint( m_node_from );
			m_pathob_flags &= ~PATHOBFLAG_SEND_HIT_WAYPOINT_MSG;
			m_dist_traveled = m_dist_to_wp - m_dist_traveled;
			// now move along the line...
			SetUpNavPointLineMotion();
			UpdateNavPosAlongPath();
			m_pathob_state = PATHOB_STATE_FOLLOW_LINE;
			break;

		default:
			Dbg_MsgAssert( 0,( "Unknown turn method %d", retVal ));
			break;
	}   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Function TraversePath()
// This function is called by the moving object each frame, and causes the
// pathob to move a little further down the path (by the projection of the moving
// object's forward vector along the line or curve in the path).
// Returns 'TRUE' when the end of the path is reached...
//bool CPathOb::TraversePath( bool is_being_skitched, const float dist, Mth::Vector *pHeading )
bool CPathOb::TraversePath( const float dist, Mth::Vector *pHeading )
{
	//if (is_being_skitched) printf("#");	
	
	if ( m_pathob_state == PATHOB_STATE_IDLE )
	{
		return true;
	}
	
#if FINDING_GLITCHES_IN_PATH_TRAVERSAL
	m_old_nav_pos = m_nav_pos;
#endif
	
	float distTraveled;
	if ( pHeading )
	{
		float cosHeadingToNavPath = Mth::DotProduct( *pHeading, m_turn_matrix[ Z ] );
		if ( !cosHeadingToNavPath )
		{
			//if (is_being_skitched) printf("a");	
			// can't assert on this case... car could be pointing at the sky temporarily!
			distTraveled = 1;
		}
		else
		{		
			//if (is_being_skitched) printf("b");	
			distTraveled = dist * cosHeadingToNavPath;
		}
	}
	else
	{
		//if (is_being_skitched) printf("c");	
		distTraveled = dist;
	}	
	
#if FINDING_GLITCHES_IN_PATH_TRAVERSAL
	m_prev_pathob_state = m_pathob_state;
#endif
		
	switch ( m_pathob_state )
	{
		case PATHOB_STATE_UNINITIALIZED:
			//if (is_being_skitched) printf("d");	
			SetUpNavPointLineMotion();
			m_y_rot = 0;
			m_pathob_state = PATHOB_STATE_FOLLOW_LINE;
			// intentional fall thru...
		case PATHOB_STATE_FOLLOW_LINE:
			//if (is_being_skitched) printf("e");	
			// will convert this to GetDistTraveled(), once the
			// nav point stuff is disconnected from the car stuff
			m_dist_traveled += distTraveled;
			
            if ( m_pathob_flags & PATHOBFLAG_END_OF_PATH )
			{
				// see if we're at the end of the path:
				if ( m_dist_traveled >= m_dist_to_wp )
				{
					m_node_from = m_node_to;
					m_nav_pos = m_wp_pos_from = m_wp_pos_to;
					m_pathob_state = PATHOB_STATE_IDLE;
					return true;
				}
			}
			else if ( m_dist_traveled >= ( m_dist_to_wp - m_current_turn_dist ) )
			{
				// we've reached the waypoint... save the extra!
				//m_extra_dist = m_dist_traveled - ( m_dist_to_wp - m_current_turn_dist );
				if ( !m_current_turn_dist )
				{
					m_nav_pos = m_wp_pos_to;
					Dbg_Assert( GetMotionComponentFromObject( mp_object ) );
					GetMotionComponentFromObject( mp_object )->HitWaypoint( m_node_to );
					if ( !GetNextWaypoint() )
					{
						Dbg_MsgAssert( 0,( "Tell Matt that the code got here, and that he's an idiot but you still love him." ));
						return true;
					}
					else
					{
						m_pathob_state = PATHOB_STATE_UNINITIALIZED;
					}
					break;
				}
				
                // get the circle along the turn, using the turn radius...
				// first, set the nav pos equal to the first point on the circle
				// ( the current position... )
				//m_dist_traveled -= m_extra_dist;
				UpdateNavPosAlongPath();
				
				// then get the next waypoint, which will allow us to figure out
				// the second point along the circle...
				GetNextWaypoint();
				m_pathob_flags |= PATHOBFLAG_SEND_HIT_WAYPOINT_MSG;
				PrepareTurn();
				
				break;
			}
			UpdateNavPosAlongPath();
            break;

		case PATHOB_STATE_FOLLOW_CURVE:
			// moving around the circumference of the circle now!!
			{
				//if (is_being_skitched) printf("f");	
				//Mth::Vector oldpos=m_nav_pos;
				
				// will convert this to GetDistTraveled(), once the
				// nav point stuff is disconnected from the car stuff
				m_dist_traveled += distTraveled;
				//if (is_being_skitched) printf("[dt=%.2f, %.2f]",distTraveled,m_dist_traveled);
				
				if ( ( m_pathob_flags & PATHOBFLAG_SEND_HIT_WAYPOINT_MSG ) &&
					( m_dist_traveled >= ( m_arc_dist / 2.0f ) ) )
				{
					m_pathob_flags &= ~PATHOBFLAG_SEND_HIT_WAYPOINT_MSG;
					Dbg_Assert( GetMotionComponentFromObject( mp_object ) );
					GetMotionComponentFromObject( mp_object )->HitWaypoint( m_node_from );
				}
				if ( m_dist_traveled > m_arc_dist )
				{
					
					m_extra_dist = m_dist_traveled - m_arc_dist;
					//if (is_being_skitched) printf("[mdt=%.2f][ad=%.2f][ed=%.2f]",m_dist_traveled,m_arc_dist,m_extra_dist);
					
					// Calculates m_nav_pos. Ignores m_dist_traveled and m_extra_dist
					UpdateNavPosAlongCurve( distTraveled - m_extra_dist );
					//if (is_being_skitched) printf("[cpd=%.2f]",Mth::Distance(m_nav_pos,oldpos));
					
					// now move along the line...
					
					// Ken: Skitch Glitch fix
					// If this is not zeroed there is an increase in speed for one frame when
					// going from this state to PATHOB_STATE_FOLLOW_LINE
					// Ie, the car moves too far in this transition.
					m_extra_dist=0.0f;
					
					// Recalculates m_dist_traveled using m_nav_pos, then adds m_extra_dist, 
					// then zeros m_extra_dist
					SetUpNavPointLineMotion();
					
					// Calculates m_nav_pos using m_dist_traveled
					UpdateNavPosAlongPath();
					
					m_y_rot = 0;
					m_pathob_state = PATHOB_STATE_FOLLOW_LINE;
					
					//if (is_being_skitched) printf("[%.2f]",Mth::Distance(oldpos,m_nav_pos));
					break;
				}
				// find the angular rotation (angle changes relative to
				// total arc angle as the distance changes relative to
				// the total distance...
				//if (is_being_skitched) printf("{dt=%.2f r=%.2f ad=%.2f ang=%.2f}",distTraveled,m_circle_radius,m_arc_dist,m_arc_ang);
				UpdateNavPosAlongCurve( distTraveled );
				//if (is_being_skitched) printf("[%.2f]",Mth::Distance(oldpos,m_nav_pos));
				
				//temp debuggery:
				/*{
					Mth::Vector tempVect;
					tempVect = m_turn_matrix[ 0 ] * 150;
					tempVect += m_circle_center;
					Gfx::AddDebugLine( tempVect, m_circle_center, MAKE_RGB( 255, 0, 0 ), MAKE_RGB( 0, 0, 0 ) );
					tempVect = m_turn_matrix[ 1 ] * 150;
					tempVect += m_circle_center;
					Gfx::AddDebugLine( tempVect, m_circle_center, MAKE_RGB( 0, 255, 0 ), MAKE_RGB( 0, 0, 0 ) );
					tempVect = m_turn_matrix[ 2 ] * 150;
					tempVect += m_circle_center;
					Gfx::AddDebugLine( tempVect, m_circle_center, MAKE_RGB( 0, 0, 255 ), MAKE_RGB( 0, 0, 0 ) );
				}*/
			}
			break;
		
		case PATHOB_STATE_LINEAR_INTERP:
		{
			//if (is_being_skitched) printf("g");	
			// very minimal turn...
			// move along the current line until we get to the next line...
			// gradually change the heading.
			m_dist_traveled += distTraveled;
			float fracTraveled = m_dist_traveled / m_dist_to_wp;
			
			if ( m_dist_traveled < m_dist_to_wp )
			{
				m_nav_pos = m_circle_center;
				Mth::Vector alongPath = m_wp_pos_from - m_circle_center;
				alongPath *= fracTraveled;
				m_nav_pos += alongPath;
			}
			else
			{
				m_nav_pos = m_wp_pos_from;
				Mth::Vector alongPath = m_circle_arrival_pos - m_wp_pos_from;
				alongPath *= ( fracTraveled - 1 ) / 1;
				m_nav_pos += alongPath;
			}
			
			// set the heading, just a weighted average of the two headings...
			Mth::Vector heading1, heading2;
			heading1 = m_wp_pos_from - m_circle_center;
			heading2 = m_circle_arrival_pos - m_wp_pos_from;
			// fraction traveled over both lines...
			fracTraveled /= 2.0f;
			if ( ( fracTraveled ) >= 0.5f && ( m_pathob_flags & PATHOBFLAG_SEND_HIT_WAYPOINT_MSG ) )
			{
				m_pathob_flags &= ~PATHOBFLAG_SEND_HIT_WAYPOINT_MSG;
				Dbg_Assert( GetMotionComponentFromObject( mp_object ) );
				GetMotionComponentFromObject( mp_object )->HitWaypoint( m_node_from );
			}
			if ( fracTraveled >= 1.0f )
			{
				// set all the variables back so we can traverse the next path:
				fracTraveled = 1;
				
				// Ken: Skitch Glitch fix
				//m_extra_dist = m_dist_traveled - ( 2 * m_dist_to_wp );
				m_extra_dist=0.0f;
				
				SetUpNavPointLineMotion();
				//Dbg_Message( "hmm %f\n", m_dist_traveled );
				m_pathob_state = PATHOB_STATE_FOLLOW_LINE;
				//if (is_being_skitched)
				//{
				//	printf("Going from interp back to line\n");	
				//}	
			}
			m_turn_matrix[ Z ] = ( heading1 * ( 1 - fracTraveled ) );
			// by the time we get to the arrival pos, the heading will be along the next line!
			m_turn_matrix[ Z ] += ( heading2 * ( fracTraveled ) );
			m_turn_matrix[ Z ].Normalize();
			break;
		}

		case PATHOB_STATE_IDLE:
			break;
			
		default:
			Dbg_MsgAssert( 0,( "Unknown substate!" ));
			break;
	}
	
#if FINDING_GLITCHES_IN_PATH_TRAVERSAL
	FindGlitch();
#endif
	
#if DRAW_COLOR_CODED_NAV_PATH
	int rgb;
	switch ( m_pathob_state )
	{
		case PATHOB_STATE_LINEAR_INTERP:
			rgb = MAKE_RGB( 255, 0, 0 );
			break;
			
		case PATHOB_STATE_FOLLOW_CURVE:
			rgb = MAKE_RGB( 0, 255, 0 );
			break;
			
		case PATHOB_STATE_FOLLOW_LINE:
			rgb = MAKE_RGB( 0, 0, 255 );
			break;
		
		case PATHOB_STATE_UNINITIALIZED:
			rgb = 255;
			break;
			
		default:
			rgb = MAKE_RGB( 255, 255, 255 );					  
	}
	m_old_nav_pos[ Y ] += 200;
	m_nav_pos[ Y ] += 200;
	Gfx::AddDebugLine( m_old_nav_pos, m_nav_pos, rgb, rgb );
	m_old_nav_pos[ Y ] -= 200;
	m_nav_pos[ Y ] -= 200;
#endif

	//if (is_being_skitched) printf("#");	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPathOb::GetDebugInfo( Script::CStruct* p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert( p_info, ( "NULL p_info passed to CPathOb::GetDebugInfo" ) );

	p_info->AddVector( "m_nav_pos", m_nav_pos );
	
	// nodes
	p_info->AddInteger( "m_node_from", m_node_from );
	p_info->AddInteger( "m_node_to", m_node_to );
	p_info->AddInteger( "m_node_next", m_node_next );
	p_info->AddStructure( "m_node_from", SkateScript::GetNode( m_node_from ) );
	p_info->AddStructure( "m_node_to", SkateScript::GetNode( m_node_to ) );
	p_info->AddStructure( "m_node_next", SkateScript::GetNode( m_node_next ) );
	
	// waypoint positions:
	p_info->AddVector( "m_wp_pos_from", m_wp_pos_from );
	p_info->AddVector( "m_wp_pos_to", m_wp_pos_to );
	p_info->AddVector( "m_wp_pos_next", m_wp_pos_next );

    // DESIGNER SETTABLE, through script file:
    p_info->AddFloat( "m_enter_turn_dist", m_enter_turn_dist );
	p_info->AddFloat( "m_min_turn_circle_radius", m_min_turn_circle_radius );
	
	// float				m_current_turn_dist;
	
	// navigational info (save runtime calculations...)
	// float				m_dist_to_wp;
	// float				m_dist_traveled;
	
	// could be private:
	// int					m_pathob_flags;

	// float				m_extra_dist;

	// for navigating around corners:
	// Mth::Vector			m_circle_center;
	// Mth::Vector			m_circle_arrival_pos; // do we really need this? no.  so fuck you!
	// use member m_turn_matrix from object structure...
//	Mth::Matrix			m_arc_matrix;	// Y axis is rotation axis, X is along radius
	// float				m_arc_ang;		// total sweep along the radius....
	// float				m_arc_dist;		// total distance along the arc...
	// float				m_circle_radius;

	uint32 pathob_state_checksum = 0;
	switch ( m_pathob_state )
	{
	case 0:
		pathob_state_checksum = CRCD( 0x16d6676f, "PATHOB_STATE_UNINITIALIZED" );
		break;
	case 1:
		pathob_state_checksum = CRCD( 0x55b5bee1, "PATHOB_STATE_FOLLOW_LINE" );
		break;
	case 2:
		pathob_state_checksum = CRCD( 0x6cc96f7, "PATHOB_STATE_FOLLOW_CURVE" );
		break;
	case 3:
		pathob_state_checksum = CRCD( 0xac01db8, "PATHOB_STATE_LINEAR_INTERP" );
		break;
	case 4:
		pathob_state_checksum = CRCD( 0x5bc8724c, "PATHOB_STATE_IDLE" );
		break;
	default:
		Dbg_MsgAssert( 0, ( "CPathOb::GetDebugInfo got bad m_pathob_state.  Was the enum changed?" ) );
		break;
	}
	p_info->AddChecksum( "m_pathob_state", pathob_state_checksum );
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj


