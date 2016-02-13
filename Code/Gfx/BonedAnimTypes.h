//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       BonedAnimTypes.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  02/04/2003
//****************************************************************************

#ifndef __GFX_BONEDANIMTYPES_H
#define __GFX_BONEDANIMTYPES_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Gfx
{
 
/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class CAnimQKey
{
public:
#ifdef __PLAT_NGC__
	short			signBit:1;	// 1 = negative
	short			timestamp:15;
#else
	short			timestamp:15;
	short			signBit:1;	// 1 = negative
#endif		// __PLAT_NGC__

protected:
	CAnimQKey() {}
};

class CStandardAnimQKey	: public CAnimQKey
{
public:
    short           qx;
    short           qy;
    short           qz;
};

class CHiResAnimQKey : public CAnimQKey
{
public:
    float           qx;
    float           qy;
    float           qz;
};

class CAnimTKey
{
public:
	short			timestamp;

protected:
	CAnimTKey() {}
};

class CStandardAnimTKey	: public CAnimTKey
{
public:
    short           tx;
    short           ty;
    short           tz;
};

class CHiResAnimTKey : public CAnimTKey
{
public:
    float           tx;
    float           ty;
    float           tz;
};

struct SQuickAnimPointers
{
	char* 				pQuickQKey[64];
	char*				pQuickTKey[64];
	CStandardAnimQKey	theStartQKey[64];
	CStandardAnimQKey	theEndQKey[64];
	CStandardAnimTKey	theStartTKey[64];
	CStandardAnimTKey	theEndTKey[64];
//	char				qSkip[64];
//	char				tSkip[64];
	bool				valid;
	uint32*				pSkipList;
	uint32				skipIndex;
};

// NOTE: if you change this enum, update the CAnimChannel::GetDebugInfo switch statement!	
enum EAnimLoopingType
{
	LOOPING_HOLD				= 0,	// holds on last frame
	LOOPING_CYCLE,						// cycles the animation forever
	LOOPING_PINGPONG,					// pingpongs the animation forever
	LOOPING_WOBBLE,						// Aims towards wobble_target_time whilst wobbling a bit. Used for manuals & grinds.

// these are samples of other possible looping types,
// although they have not yet been implemented
//	LOOPING_CYCLE_X_TIMES,				// cycles the same animation x times
//	LOOPING_PINGPONG_X_TIMES			// pingpongs the animation x times
};
	
// NOTE: if you change this enum, update the CAnimChannel::GetDebugInfo switch statement!	
enum EAnimDirection 
{
	ANIM_DIR_FORWARDS			= 0,
	ANIM_DIR_BACKWARDS			= -1
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
**						Inline Functions									**
*****************************************************************************/
					
} // namespace Gfx

#endif // __GFX_BONEDANIMTYPES_H