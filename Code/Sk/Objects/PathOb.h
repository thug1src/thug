//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       PathOb.h
//* OWNER:          Matt Duncan
//* CREATION DATE:  11/2/2000
//****************************************************************************

#ifndef __OBJECTS_PATHOB_H
#define __OBJECTS_PATHOB_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>
#include <core/math.h>

#include <gel/scripting/struct.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

// debugging options:
#define FINDING_GLITCHES_IN_PATH_TRAVERSAL	0
#define DRAW_COLOR_CODED_NAV_PATH			0

#define HUGE_DISTANCE		FEET_TO_INCHES( 666 * 666 )

namespace Obj
{

class CCompositeObject;											 
											 
/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

class Manager;

class CPathOb : public Spt::Class
{
	
public:
	CPathOb( CCompositeObject* pObj );
	virtual ~CPathOb();

	virtual	void		GetDebugInfo( Script::CStruct* p_info );

	Mth::Vector			m_nav_pos;		//car pos is set by finding the ground under the nav pos
	CCompositeObject* 	mp_object;		// owner

	// for the curve from one path to the next...
	Mth::Matrix			m_turn_matrix;	// Y axis is rotation axis, X is along radius
	
	// waypoint indices:
	int					m_node_from;
	int					m_node_to;
	int					m_node_next;
	
	// waypoint positions:
	Mth::Vector			m_wp_pos_from;
	Mth::Vector			m_wp_pos_to;
	Mth::Vector			m_wp_pos_next;

    // DESIGNER SETTABLE, through script file:
    float				m_enter_turn_dist;				// in inches...
	float				m_min_turn_circle_radius;	// in inches
    // end DESIGNER SETTABLE variables...
	
	// varies depending on the distance between waypoints:
	float				m_current_turn_dist;
	
	// navigational info (save runtime calculations...)
	float				m_dist_to_wp;
	float				m_dist_traveled;
	
	// could be private:
	int					m_pathob_flags;

	float				m_extra_dist;

	// for navigating around corners:
	Mth::Vector			m_circle_center;
	Mth::Vector			m_circle_arrival_pos; // do we really need this? no.  so fuck you!
	// use member m_turn_matrix from object structure...
//	Mth::Matrix			m_arc_matrix;	// Y axis is rotation axis, X is along radius
	float				m_arc_ang;		// total sweep along the radius....
	float				m_arc_dist;		// total distance along the arc...
	float				m_circle_radius;

	int					m_pathob_state;
 
	bool				Busy( void );
	float				GetDistToWaypoint( void );
			   
	// Returns 'true' when path has been traversed (end of path!)
    bool				TraversePath( const float deltaDist, Mth::Vector *pHeading = NULL );
	
	void				SetHeading( Mth::Vector *p_heading );

	bool				GetNextWaypoint( void );

	// call when beginning a new path:
	void 				InitPath( int nodeNum );
	void				NewPath( int nodeNum, const Mth::Vector &currentPos );

	float				m_y_rot;
	void				DrawPath( void );

private:

	// Set up the path according to m_node_from and m_node_to...
	// Do this after getting the next waypoint in a path, or when
	// beginning a new path.  m_node_from should indicate where we
	// are now, m_node_to should be the node index of where we're
	// going!
	void				SetUpPath( void );

	// set heading vector to correspond to new path...
	void				AlignToPath( void );

	bool 				SetUpNavPointLineMotion( );
	int 				SetUpTurningCircle( void );
	void				PrepareTurn( void );
	bool				SetUpTurnDist( void );

	void				UpdateNavPosAlongPath( void );
	void				UpdateNavPosAlongCurve( float distTraveled );

#if FINDING_GLITCHES_IN_PATH_TRAVERSAL
	// can remove if no longer debugging:
	int					m_prev_pathob_state;
	// nav pos moves smoothly between the waypoints or along the curve connecting paths...
	Mth::Vector 		m_old_nav_pos;	//for debuggery...
	// debugging function:
	void				FindGlitch( void );
#endif

};

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Obj

#endif	// __OBJECTS_PATHOB_H

