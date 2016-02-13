//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       CasUtils.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  ?/??/????
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/casutils.h>

#include <core/math/math.h>

#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h> 
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Cas
{

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

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

uint32 get_desc_id_from_structure( Script::CStruct* pStructure )
{
	Dbg_Assert( pStructure );

	uint32 descId;
	pStructure->GetChecksum( CRCD(0x4bb2084e,"desc_id"), &descId, true );
	return descId;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool IsValidOption( uint32 partChecksum, uint32 lookFor )
{
	return ( GetOptionStructure( partChecksum, lookFor ) != NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* GetOptionStructure( uint32 partChecksum, uint32 lookFor, bool assertOnFail )
{
	Script::CArray* pArray;
	pArray = Script::GetArray( partChecksum, Script::ASSERT );

	Script::CStruct* pReturnStructure = NULL;

	if ( lookFor == 0 )
	{
		// special checksum, meaning to return NULL
		// (handy when preprocessing random descs)
		return NULL;
	}

	for ( uint32 i = 0; i < pArray->GetSize(); i++ )
	{
		Script::CStruct* pCurrDesc = pArray->GetStructure( i );
		uint32 desc_id = get_desc_id_from_structure( pCurrDesc );
		if ( desc_id == lookFor )
		{
			pReturnStructure = pCurrDesc;
			break;
		}
	}

	if ( assertOnFail )
	{
		Dbg_MsgAssert( pReturnStructure, ( "Couldn't find %s %s", Script::FindChecksumName(partChecksum), Script::FindChecksumName(lookFor) ) );
	}

	return pReturnStructure;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* GetFirstOptionStructure( uint32 partChecksum )
{
	Script::CArray* pArray;
	pArray = Script::GetArray( partChecksum, Script::ASSERT );

	return pArray->GetStructure( 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void BuildRandomSetList( uint32 partChecksum, uint32 random_set, Script::CStruct** ppReturnStructs, int* pReturnNumStructs )
{
	Script::CArray* pArray;
	pArray = Script::GetArray( partChecksum, Script::ASSERT );

	// reset the return value
	*pReturnNumStructs = 0;

	int part_array_size = pArray->GetSize();
	for ( int i = 0; i < part_array_size; i++ )
	{
		Script::CStruct* pStruct = pArray->GetStructure( i );
		int allowed_to_pick = 0;
		pStruct->GetInteger( CRCD(0x355f9467,"allowed_to_pick"), &allowed_to_pick, Script::ASSERT );
		
		if ( allowed_to_pick )
		{
			// if a random_set was specified, then
			// make sure that this item works with it
			// (used for matching skin tones)
			uint32 this_random_set = 0;
			pStruct->GetChecksum( CRCD(0x0d7260fd,"random_set"), &this_random_set, Script::NO_ASSERT );

			if ( this_random_set == random_set )
			{
				*ppReturnStructs = pStruct;
				ppReturnStructs++;

				(*pReturnNumStructs)++;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* GetRandomOptionStructure( uint32 partChecksum, uint32 random_set )
{
	Script::CArray* pArray;
	pArray = Script::GetArray( partChecksum, Script::ASSERT );

	const int vMAX_CAS_ITEMS = 32;
	Script::CStruct* pAllowedStructs[vMAX_CAS_ITEMS];
	bool alreadySelected[vMAX_CAS_ITEMS];
	int allowRandomCount = 0;
	
	int part_array_size = pArray->GetSize();
	for ( int i = 0; i < part_array_size; i++ )
	{
		Script::CStruct* pStruct = pArray->GetStructure( i );
		int allow_random = !pStruct->ContainsFlag( CRCD(0xf6f8e158,"no_random") );
		pStruct->GetInteger( CRCD(0xf1e3cd22,"allow_random"), &allow_random, Script::ASSERT );
		
		if ( random_set )
		{
			// if a random_set was specified, then
			// make sure that this item works with it
			// (used for matching skin tones)
			uint32 this_random_set = 0;
			if ( pStruct->GetChecksum( CRCD(0x0d7260fd,"random_set"), &this_random_set, Script::NO_ASSERT ) )
			{
				if ( this_random_set != random_set )
				{
					allow_random = false;
				}
			}
		}
		
		if ( allow_random )
		{
			Dbg_Assert( allowRandomCount < vMAX_CAS_ITEMS );
			pAllowedStructs[allowRandomCount] = pStruct;
			
			int already_selected;
			pStruct->GetInteger( CRCD(0x92bddfd9,"already_selected"), &already_selected, true );
			alreadySelected[allowRandomCount] = already_selected;
			
			allowRandomCount++;
		}
	}

	// make sure there's at least one allow_random,
	// or else we'll get into an infinite loop
	Dbg_MsgAssert( allowRandomCount > 0, ( "No allow_random was found in this array %s (Tell Gary!)", Script::FindChecksumName(partChecksum) ) );

	// count how many of them are already unselected
	int unselected_count = 0;
	for ( int i = 0; i < allowRandomCount; i++ )
	{
		if ( !alreadySelected[i] )
		{
			unselected_count++;
		}
	}

	// the following makes sure that all the "allow_random" parts
	// are used at least once, before duplicates begin.
	if ( unselected_count == 0 )
	{
		for ( int i = 0; i < allowRandomCount; i++ )
		{
			alreadySelected[i] = false;
		}
	}

	// now get rid of all the already selected items
	int currentCount = 0;
	for ( int i = 0; i < allowRandomCount; i++ )
	{
		if ( !alreadySelected[i] )
		{
			pAllowedStructs[currentCount] = pAllowedStructs[i];
			alreadySelected[currentCount] =	alreadySelected[i];
			currentCount++;
		}
	}
	allowRandomCount = currentCount;
	
	Dbg_MsgAssert( allowRandomCount >= 1, ( "No part to pick" ) );
	int index = Mth::Rnd( allowRandomCount );
	Dbg_Assert( index >= 0 && index < allowRandomCount );
	Script::CStruct* pReturnStruct = pAllowedStructs[index];

	Dbg_Assert( pReturnStruct );
	pReturnStruct->AddInteger( CRCD(0x92bddfd9,"already_selected"), 1 );

//	uint32 checksum;
//	pStruct->GetChecksum( "desc_id", &checksum, true );
//	Dbg_Message( "Choosing random part %s", Script::FindChecksumName( checksum ) );

	return pReturnStruct;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* GetRandomOptionStructureByIndex( uint32 partChecksum, int desired_index, uint32 random_set )
{
	Script::CArray* pArray;
	pArray = Script::GetArray( partChecksum, Script::ASSERT );

	int part_array_size = pArray->GetSize();

	if ( !random_set )
	{
		for ( int i = 0; i < part_array_size; i++ )
		{
			Script::CStruct* pStruct = pArray->GetStructure( i );

			int random_index = -1;
			if ( pStruct->GetInteger( CRCD(0x4b833e64,"random_index"), &random_index, Script::ASSERT ) )
			{
				if ( random_index == desired_index )
				{
					return pStruct;
				}
			}
		}
	}
	else
	{
		// GJ FIX FOR SK5:TT12579:  "NJ - Goal peds being created with mismached skin colors"
		// if a random_set (skintone) was specified, then find the n-th item that actually works with this
		// random set
		while ( 1 )
		{
			#ifdef	__NOPT_ASSERT__
			int current_random_index = desired_index;
			#endif

			for ( int i = 0; i < part_array_size; i++ )
			{
				Script::CStruct* pStruct = pArray->GetStructure( i );

				int allow_random = !pStruct->ContainsFlag( CRCD(0xf6f8e158,"no_random") );
				pStruct->GetInteger( CRCD(0xf1e3cd22,"allow_random"), &allow_random, Script::ASSERT );

				uint32 this_random_set = 0;
				pStruct->GetChecksum( CRCD(0x0d7260fd,"random_set"), &this_random_set, Script::NO_ASSERT );
				bool is_universal_item = ( this_random_set == 0 );

				if ( allow_random && ( ( this_random_set == random_set ) || is_universal_item ) )
				{
					if ( desired_index <= 0 )
					{
						return pStruct;
					}

					desired_index--;
				}
			}

			// once we get to the end of the array, we need to start over
			// (basically, we're doing a modulo of the desired_index)...
			// the following checks for an infinite loop in which there
			// were no items found that works with this random set
			Dbg_MsgAssert( current_random_index != desired_index, ( "Couldn't find 'random_index' %d for ped part %s (are there any that fit this skintone %s?)", desired_index, Script::FindChecksumName(partChecksum), Script::FindChecksumName(random_set) ) );
		}
	}

	Dbg_MsgAssert( 0, ( "Couldn't find 'random_index' %d for ped part %s", desired_index, Script::FindChecksumName(partChecksum) ) );
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // end namespace


