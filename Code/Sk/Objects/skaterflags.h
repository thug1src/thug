// Skaterflags.h

#ifndef	__SK_OBJECTS_SKATERFLAGS_H__	
#define	__SK_OBJECTS_SKATERFLAGS_H__	

namespace Obj
{
	// Flags that can be toggled on and off, and also store how long since last toggle
	// This is used as an index into CSkater::m_flags
	enum	ESkaterFlag
	{
		TENSE = 0, 					// tensing for a jump, usually X pressed											 0
		// SWITCH,					// skating switch																	 1		NOTE: unused?
		// LEAN,					// leaning L/R (might prohibit us doing other things								 2		NOTE: unused?
		// TENSE_ON_GROUND,			// we were on the ground, and the tense flag was set								 3		NOTE: unused?
		FLIPPED,					// animation is flipped																 4
		VERT_AIR,					// True if we are going to vert air													 5
		TRACKING_VERT,				// True if we are tracking a vert surface below us 									 6
		LAST_POLY_WAS_VERT,			// True if the last polygon we skated on was vert									 7
		CAN_BREAK_VERT,				// True if we can break the vert poly we are on										 8
		CAN_RERAIL,					// can get back on the rail again, ahead of rerail time								 9
		// STARTED_END_OF_RUN,		// true if the skater has started the "end of run" script							 10
		// FINISHED_END_OF_RUN,		// True if the skater has returned to his idle state after the "end of run" script	 11
		// STARTED_GOAL_END_OF_RUN,	// True if the goal_endofrun script has started
		// FINISHED_GOAL_END_OF_RUN,
		RAIL_SLIDING,				// True if skater is railsliding													 12
		CAN_HIT_CAR,				// True is we can hit a car.  Set when on ground, or have negative y vel			 13
		AUTOTURN,					// True if we can autoturn															 14
		IS_BAILING,					// True if skater is bailing...														 15
		// IS_ENDING_RUN,			// True is skater is in the process of ending his run								 16
		// IS_ENDING_GOAL,			// True if we're ending a goal
		SPINE_PHYSICS,				// True is skater is going over a spine												 17
		IN_RECOVERY,				// True if recovering from going off vert											 18
		SKITCHING,					// True if we are being towed by a car												 19
		OVERRIDE_CANCEL_GROUND,		// True if we want to ignore the "CANCEL_GROUND" flag for gaps						 20
		SNAPPED_OVER_CURB,			// True if we snapped up or down a curb this frame (for use by camera)				 21
		SNAPPED,					// True if we snapped slightly this frame (for use by camera)
		IN_ACID_DROP,				// True if we are in an acid drop
		AIR_ACID_DROP_DISALLOWED,	// True if in-air acid drops are not currently allowed
		CANCEL_WALL_PUSH,			// True if the current wallpush event is canceled via script
		NO_ORIENTATION_CONTROL,		// True if spins and leans are turned off due to switching from walking
		NEW_RAIL,					// True if the rail we land on is a "new" rail
		OLLIED_FROM_RAIL,			// True if entered current air via an ollie out of a rail
		// NOTE: If anyone adds any more flags so that NUM_ESKATERFLAGS > 32, let Steve G know

		NUM_ESKATERFLAGS
	};
	
	enum EStateType
	{
		GROUND,
		AIR,
		WALL,
		LIP,
		RAIL,
		WALLPLANT
	};
}
	
#endif

