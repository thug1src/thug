/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Object (OBJ)											**
**																			**
**	File name:		objects/manual.h										**
**																			**
**	Created: 		01/31/01	-	ksh										**
**																			**
*****************************************************************************/

#ifndef __OBJECTS_MANUAL_H
#define __OBJECTS_MANUAL_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/



#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <sys/timer.h>

#include <gfx/gfxman.h>

#include <gel/inpman.h>
#include <gel/object.h>
#include <gel/object/compositeobject.h>

#include <core/math/matrix.h>
#include <core/math/vector.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/
namespace Obj
{
	class CSkaterBalanceTrickComponent;
	class CSkaterScoreComponent;
	class CInputComponent;
	
/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

class  CManual  : public Spt::Class
{
	
public:								// for debugging

	CCompositeObject *mpSkater;
	CSkaterBalanceTrickComponent* mpSkaterBalanceTrickComponent;
	CSkaterScoreComponent* mpSkaterScoreComponent;
	CInputComponent* mpInputComponent;
	
	Mth::Vector mManualPos;
	float mActualManualTime;
	float mManualTime;
	float mMaxTime;
	
	float mManualLean;
	float mManualLeanDir;
	float mManualCheese;
	bool mManualReleased;
	
	uint32 mStartTime;
	uint32 mButtonAChecksum;
	uint32 mButtonBChecksum;
	
	int mTweak;
	
	int mOldRailNode;
	
	bool mWobbleEnabled;
	bool mNeverShowMeters;

	// If true, then the animation will be moved in the opposite direction
	// when the skater is flipped.
	// This is required when the animation moves from side to side as in
	// the case of most grinds, but not for anims that move forward and
	// back, such as manuals.
	bool mDoFlipCheck;
	
	// If set, the range anim will be played backwards. Can be used in conjunction with
	// mDoFlipCheck.
	bool mPlayRangeAnimBackwards;
		
public:
	CManual();
	virtual ~CManual();
	
	void Init ( CCompositeObject *pSkater );

	void ResetCheese() {mManualCheese=0.0f;}	
	void Reset();
	void ClearMaxTime();
	void SwitchOffMeters();
	void SwitchOnMeters();
	void Stop();
	void UpdateRecord();

	float	GetMaxTime() {return mMaxTime;}
	
	void SetUp(uint32 ButtonAChecksum, uint32 ButtonBChecksum, int Tweak, bool DoFlipCheck, bool PlayRangeAnimBackwards);
	void EnableWobble();
	void DoManualPhysics();
	
private:
	inline float scale_with_frame_length ( float frame_length, float f );
	
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

#endif	// __OBJECTS_MANUAL_H
