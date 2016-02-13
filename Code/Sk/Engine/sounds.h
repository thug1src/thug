/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Skate Module (SKATE) 									**
**																			**
**	File name:		sk/engine/sounds.h										**
**																			**
**	Created by:		Matt Jan. 2001											**
**																			**
**	Description:	Lists of soundFX for terrains, etc...					**
**																			**
*****************************************************************************/

#ifndef __GEL_SK_SOUNDS_H
#define __GEL_SK_SOUNDS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/list.h>
#include <core/macros.h>

namespace Sk3Sfx
{




struct SkaterSoundInfo
{
	int		surfaceFlag;
	uint32	soundChecksum;
	float		maxPitch;
	float		minPitch;
	float		maxVol;
	float		minVol;
};

/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

class  CSk3SfxManager  : public Spt::Class
{
	
	DeclareSingletonClass( CSk3SfxManager );

public:
	// these sound trigger values are also used to
	// communicate with the sounds.cpp module and scripted
	// soundfx for jumping/landing... if changed please
	// change skater_sfx.q also:
	enum
	{
		vTABLE_WHEELROLL,
		vTABLE_GRIND, // on a rail...
		vTABLE_JUMP,
		vTABLE_LAND,
		vTABLE_BONK,
		vTABLE_GRINDJUMP,
		vTABLE_GRINDLAND,
		vTABLE_SLIDE, // on a rail
		vTABLE_SLIDEJUMP,
		vTABLE_SLIDELAND,
		vTABLE_CESS,
		vNUM_SOUND_TYPES,
	};
	
	CSk3SfxManager::CSk3SfxManager( void );
	CSk3SfxManager::~CSk3SfxManager( void );

	void			Reset( void );
	void			SetSkaterSoundInfo( int surfaceFlag, uint32 whichSound,
									int whichArray, float maxPitch, float minPitch, float maxVol, float minVol );
	SkaterSoundInfo	*GetSkaterSoundInfo( int surfaceFlag, int whichArray );

	float			GetVolPercent( SkaterSoundInfo *pInfo, float volPercent, bool clipToMaxVol = false );

	void 			PlaySound( int whichArray, int surfaceFlag, const Mth::Vector &pos, float volPercent,
							   bool propogate = true );

private:
	
	enum
	{
		vMAX_NUM_ENTRIES  = 64,
	};

	int mNumEntries[ vNUM_SOUND_TYPES ];
	SkaterSoundInfo mSoundArray[ vNUM_SOUND_TYPES ][ vMAX_NUM_ENTRIES ];
};

}  // namespace Sfx

#endif
