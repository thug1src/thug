/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Objects (OBJ) 											**
**																			**
**	File name:		objects/SkaterTricks.h									**
**																			**
**	Created: 		10/29/01	-	gj										**
**																			**
**	Description:	Skater tricks code										**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/objects/skatertricks.h>

#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Obj
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

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

SSpecialTrickInfo::SSpecialTrickInfo()
{
	m_isCat = false;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool SSpecialTrickInfo::IsUnassigned ( void ) const
{
	return m_TrickName == Script::GenerateCRC( "Unassigned" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Str::String SSpecialTrickInfo::GetTrickNameString ( void ) const
{
	if ( IsUnassigned() )
	{
		return Script::GetLocalString( "sp_str_unassigned" );
	}

	Script::CScriptStructure* pTrickStructure;
	if ( ( Script::GetStructure( m_TrickName, Script::ASSERT ) )->GetStructure( "params", &pTrickStructure ) )
	{
		// get the trick name
		const char* pTrickName;
		pTrickStructure->GetLocalText( "name", &pTrickName, true );
		return pTrickName;
	}

	return "Unknown";
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Str::String SSpecialTrickInfo::GetTrickSlotString ( void ) const
{
	if ( IsUnassigned() )
	{
		return Script::GetLocalString( "sp_str_unassigned" );
	}

	Script::CArray* pButtonArray = Script::GetArray( "SpecialCombos" );
	Dbg_Assert( pButtonArray );

	for ( int i = 0; i < (int)pButtonArray->GetSize(); i++ )
	{
		Script::CScriptStructure* pStructure = pButtonArray->GetStructure( i );
		Dbg_Assert( pStructure );

		uint32 currTrickSlot;
		pStructure->GetChecksum( "trickslot", &currTrickSlot, true );

		if ( currTrickSlot == m_TrickSlot )
		{
			const char* pTrickSlotString;
			pStructure->GetText( "desc", &pTrickSlotString, true );
			return pTrickSlotString;
		}	
	}

	return "Unknown";
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Str::String SSpecialTrickInfo::GetSlotMenuProperty ( void ) const
{
	if ( IsUnassigned() )
	{
		return "trick_menu_property";
	}
	else
	{
		return "trickslot_menu_property";
	}
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickChecksumTable::CTrickChecksumTable( void )
{
	m_NumChecksums = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickChecksumTable::Init( void )
{
	m_NumChecksums = 0;

	Script::CArray* pArray = Script::GetArray( "ConfigurableTricks" );

	for ( int i = 0; i < (int)pArray->GetSize(); i++ )
	{
		uint32 trickChecksum = pArray->GetNameChecksum( i );

		// not found
		if ( GetIndexFromChecksum( trickChecksum ) == -1 )
		{
			Dbg_MsgAssert( m_NumChecksums < vMAX_TRICK_CHECKSUMS, ("Too many trick checksums (limit = %d)", vMAX_TRICK_CHECKSUMS) );
			m_Checksum[m_NumChecksums] = trickChecksum;
			m_NumChecksums++;
		}
	}
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CTrickChecksumTable::GetChecksumFromIndex( int index )
{
		
	Dbg_Assert( index >= 0 && index < m_NumChecksums );

	return m_Checksum[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CTrickChecksumTable::GetIndexFromChecksum( uint32 checksum )
{
	for ( int i = 0; i < m_NumChecksums; i++ )
	{
		if ( checksum == m_Checksum[i] )
		{
			// found index
			return i;
		}
	}

	// not found
	return -1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj
