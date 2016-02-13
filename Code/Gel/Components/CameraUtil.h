//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       CameraUtil.cpp
//* OWNER:          Dan
//* CREATION DATE:  4/10/3
//****************************************************************************

#ifndef __COMPONENTS_CAMERAUTIL_H__
#define __COMPONENTS_CAMERAUTIL_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/math.h>

#define SKATERCAMERACOMPONENT_PERFECT_ABOVE			(3.0f)
			
#if 0
// Ignore faces that are NOT flagged as camera collidable
#define IGNORE_FACE_FLAGS_1 	(0)
#define IGNORE_FACE_FLAGS_0 	(mFD_CAMERA_COLLIDABLE)
#endif

#if 0
// ignore faces that ARE flagged as camera collidable
#define IGNORE_FACE_FLAGS_1 	(0)
#define IGNORE_FACE_FLAGS_0 	(mFD_CAMERA_COLLIDABLE)
#endif

#if 1
// ignore faces that are non-collidable or invisible
//#define IGNORE_FACE_FLAGS_1 	(mFD_NON_COLLIDABLE | mFD_INVISIBLE)
#define IGNORE_FACE_FLAGS_1 	(mFD_NON_COLLIDABLE | mFD_CAMERA_COLLIDABLE)
#define IGNORE_FACE_FLAGS_0 	(0)
#endif

#	define CAMERA_SLERP_STOP 0.9999f
	   
namespace Obj
{
	float			GetTimeAdjustedSlerp ( float slerp, float delta );
	void			ApplyCameraCollisionDetection ( Mth::Vector& camera_pos,  Mth::Matrix& camera_matrix, const Mth::Vector& target_pos, const Mth::Vector& forcus_pos, bool side_feelers = true, bool refocus = true );
	
	struct SCameraState
	{
		Mth::Matrix lastActualMatrix;
		Mth::Vector lastTripodPos;
		float lastDot;
		float lastZoom;
	};
}

#endif
