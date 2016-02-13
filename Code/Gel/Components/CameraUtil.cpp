//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       CameraUtil.cpp
//* OWNER:          Dan
//* CREATION DATE:  4/10/3
//****************************************************************************

/*
 * Utility functions for camera behavior components.
 */

#include <gel/components/camerautil.h>

#include <sk/engine/feeler.h>

namespace Obj
{

/*******************************************************************************************************
logic for getTimeAdjustedSlerp()

If the camera is lagging a distance D behind the target, and the target is moving with velocity V, and the lerp is set to L.
Then when the camera is stable, it will be moving at the same rate as the target (V), and since the distance it will move is
equal to the lerp multiplied by the distance from the target (which will be the old distance plus the new movement of the
target V)

Then:

L*(D+V) = V 
L*D + L*V = V
D = V * (1 - L) / L

Assuming in the above T = 1, then if we have a variable T, the speed of the skater will be T*V, yet we want the distance (Dt)
moved to remain unchanged (D), so for a time adjusted lerp Lt

Dt = T * V * (1-Lt)/Lt

Since D = Dt

V * (1 - L) / L = T * V * (1-Lt)/Lt

V cancels out, and we get

Lt = TL / (1 - L + TL)

Sanity check,  

if L is 0.25, and T is 1, then Lt = 1*0.25 / (1 - 0.25 + 1*0.25)  = 0.25
if L is 0.25, and T is 2, then Lt = 2*0.25 / (1 - 0.25 + 2*0.25)  = 0.5 / 1.25 = 0.40
if L is 0.25, and T is 5, then Lt = 5*0.25 / (1 - 0.25 + 5*0.25)  = 1.25 / 2 = 0.625

Sounds about right.
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float GetTimeAdjustedSlerp( float slerp, float delta )
{
	// Mick's method, tries to guarantee that the distance from the target point
	// will never alter at constant velocity, with changing frame rate
	// Lt = TL / (1 - L + TL)
	float t		= delta * 60.0f;
	float Lt	= t * slerp / ( 1.0f - slerp + t * slerp );
	return Lt;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#define vMIN_WALL_DISTANCE (2.0f)

void ApplyCameraCollisionDetection ( Mth::Vector& camera_pos, Mth::Matrix& camera_matrix, const Mth::Vector& target_pos, const Mth::Vector& focus_pos, bool side_feelers, bool refocus )
{
	// camera collision detection
	
	Mth::Vector target_to_camera_direction = camera_pos - target_pos;
	target_to_camera_direction.Normalize();
	
	CFeeler feeler;
	
	// Ignore faces based on face falgs
	feeler.SetIgnore(IGNORE_FACE_FLAGS_1, IGNORE_FACE_FLAGS_0);
	feeler.SetLine(target_pos, camera_pos + vMIN_WALL_DISTANCE * target_to_camera_direction);
	bool collision = feeler.GetCollision(true);
	if( collision )
	{
//			Gfx::AddDebugLine( at_pos, cam_pos, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 2000 );

		// Limits the camera to getting nearer than 11.9 inches to the focus point.
		float distance = feeler.GetDist();
		float length = feeler.Length();
		if( length * distance < 11.9f + vMIN_WALL_DISTANCE )
		{
			distance = (11.9f + vMIN_WALL_DISTANCE) / length;
		}
		camera_pos = target_pos + (distance * length - vMIN_WALL_DISTANCE) * target_to_camera_direction;
	}
	
	if (!side_feelers) return;

//#	if defined( __PLAT_NGPS__ ) || defined ( __PLAT_XBOX__ )
	// Now do two additional checks 1 foot to either side of the camera.
	Mth::Vector	left( camera_matrix[X][X], 0.0f, camera_matrix[X][Z], 0.0f );
	left.Normalize( 8.0f );
	left += camera_pos;

	collision = feeler.GetCollision(camera_pos, left, true);
	if( collision )
	{
		left		   -= feeler.GetPoint();
		camera_pos		   -= left;
	}
	else
	{
		Mth::Vector	right( -camera_matrix[X][X], 0.0f, -camera_matrix[X][Z], 0.0f );
		right.Normalize( 8.0f );
		right += camera_pos;

		collision = feeler.GetCollision(camera_pos, right, true);
		if( collision )
		{
			right		   -= feeler.GetPoint();
			camera_pos		   -= right;
		}
	}
	
	if (!refocus) return;

	if( collision )
	{
		// Re-orient the camera again.
		camera_matrix[Z].Set( focus_pos.GetX() - camera_pos.GetX(), focus_pos.GetY() - camera_pos.GetY(), focus_pos.GetZ() - camera_pos.GetZ());
		camera_matrix[Z].Normalize();

		// Read back the Y from the current matrix.
//			target[Y][0]	= p_frame_matrix->up.x;
//			target[Y][1]	= p_frame_matrix->up.y;
//			target[Y][2]	= p_frame_matrix->up.z;

		// Generate new orthonormal X and Y axes.
		camera_matrix[X]		= Mth::CrossProduct( camera_matrix[Y], camera_matrix[Z] );
		camera_matrix[X].Normalize();
		camera_matrix[Y]		= Mth::CrossProduct( camera_matrix[Z], camera_matrix[X] );
		camera_matrix[Y].Normalize();

		// Write back into camera matrix.
		camera_matrix[X]		= -camera_matrix[X];
		camera_matrix[Z]		= -camera_matrix[Z];
		
		// Fix the final column
		camera_matrix[X][W] = 0.0f;
		camera_matrix[Y][W] = 0.0f;
		camera_matrix[Z][W] = 0.0f;
		camera_matrix[W][W] = 1.0f;
		
	}
//#endif
}

}
