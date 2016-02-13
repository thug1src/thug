//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       CasUtils.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  ?/??/????
//****************************************************************************

#ifndef __GFX_CASUTILS_H
#define __GFX_CASUTILS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Script
{
	class CStruct;
}

namespace Cas
{

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

// for searching/traversing an actual optionset
bool				IsValidOption( uint32 partChecksum, uint32 descId );
Script::CStruct*	GetOptionStructure( uint32 partChecksum, uint32 descId, bool assertOnFail = true );
Script::CStruct*	GetFirstOptionStructure( uint32 partChecksum );
Script::CStruct*	GetNullOptionStructure( uint32 partChecksum );
void				BuildRandomSetList( uint32 partChecksum, uint32 random_set, Script::CStruct** ppReturnStructs, int* pReturnCount );
Script::CStruct*	GetRandomOptionStructure( uint32 partChecksum, uint32 random_set = 0 );
Script::CStruct*	GetRandomOptionStructureByIndex( uint32 partChecksum, int index, uint32 random_set = 0 );

} // end namespace

#endif
