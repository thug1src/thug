/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate3													**
**																			**
**	Module:			GameNet					 								**
**																			**
**	File name:		GameNet.cpp												**
**																			**
**	Created:		03/12/2001	-	gj										**
**																			**
**	Description:	Network Preferences 									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>


#include <gel/prefs/prefs.h>

#include <gel/scripting/utils.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/



namespace Prefs
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
**							  Private Functions								**
*****************************************************************************/

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Preferences::Preferences()
{
}
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Preferences::~Preferences()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 Preferences::WriteToBuffer(uint8 *pBuffer, uint32 BufferSize)
{
	return Script::WriteToBuffer(&m_root, pBuffer, BufferSize);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Preferences::ReadFromBuffer(uint8 *pBuffer)
{
	Script::ReadFromBuffer(&m_root,pBuffer);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* Preferences::GetPreference( uint32 field_id )
{
	Script::CStruct* p_structure = NULL;
	m_root.GetStructure( field_id, &p_structure );
	return p_structure;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CArray* Preferences::GetPreferenceArray( uint32 field_id )
{
	Script::CArray* p_array = NULL;
	m_root.GetArray( field_id, &p_array );

	return p_array;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Preferences::RemoveComponent( uint32 field_id, uint32 sub_field_id )
{
	Script::CStruct* p_structure = NULL;
	m_root.GetStructure( field_id, &p_structure );

	p_structure->RemoveComponent( sub_field_id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Preferences::SetRoot(Script::CStruct* pStuff)
{
	
	// OK if pStuff is NULL, so no need to assert
	// Probably still an error if it is NULL, but AppendStructure won't crash if it is.
	
	// Don't clear out the options structure. New stuff should just override old options
	//m_root.Clear();
	m_root.AppendStructure(pStuff); 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char* Preferences::GetPreferenceString( uint32 field_id, uint32 sub_field_id )
{
	Script::CStruct* p_structure = NULL;
	m_root.GetStructure( field_id, &p_structure );

	const char* p_string;
	p_structure->GetText( sub_field_id, &p_string, true );
	return p_string;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Preferences::GetPreferenceValue( uint32 field_id, uint32 sub_field_id )
{
	Script::CStruct* p_structure = NULL;
	m_root.GetStructure( field_id, &p_structure );

	int returnVal;
	p_structure->GetInteger( sub_field_id, &returnVal, true );
	return returnVal;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 Preferences::GetPreferenceChecksum( uint32 field_id, uint32 sub_field_id )
{
	Script::CStruct* p_structure = NULL;
	m_root.GetStructure( field_id, &p_structure, true );

	uint32 returnVal;
	p_structure->GetChecksum( sub_field_id, &returnVal, true );
	return returnVal;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Preferences::SetPreference( uint32 field_id, Script::CStruct* p_to_append )
{
	
	
	Script::CStruct* p_structure = NULL;

	// TODO:  Decide if we can add preferences at runtime,
	// or whether all the categories already exist at load-time
	if ( !m_root.GetStructure( field_id, &p_structure ) )
	{
		// structure doesn't already exist, so it's not a valid preference
		Dbg_MsgAssert( 0,( "Trying to add an invalid preference" ));
		return false;
	}
	
	p_structure->AppendStructure( p_to_append );

//	p_structure->PrintContents();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Preferences::SetPreference( uint32 field_id, Script::CArray* p_to_append )
{
	Script::CArray* p_array = NULL;

	// TODO:  Decide if we can add preferences at runtime,
	// or whether all the categories already exist at load-time
	if ( !m_root.GetArray( field_id, &p_array ) )
	{
		// structure doesn't already exist, so it's not a valid preference
		Dbg_MsgAssert( 0,( "Trying to add an invalid preference" ));
		return false;
	}
	
	m_root.AddArray( field_id, p_to_append );

	//PrintContents();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Preferences::Load( uint32 structure_checksum )
{
	

	// Don't clear out the options structure. New stuff should just override old options
	//Reset();
	
	Script::CStruct* p_structure = Script::GetStructure( structure_checksum, Script::ASSERT );

	m_root.AppendStructure( p_structure );
	
#if 0
	uint8 testbuffer[2048];
	m_root.WriteToBuffer(testbuffer, 2048);
	Script::CStruct* p_new_structure = new Script::CStruct;
	p_new_structure->Clear();
	p_new_structure->ReadFromBuffer(testbuffer);
	p_new_structure->PrintContents();
	delete p_new_structure;
#endif
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Preferences::Reset()
{
	m_root.Clear();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Preferences::UpdateUIElement( uint32 control_id, uint32 field_id, bool mask_password )
{
	
	
	Script::CStruct* pStructure = NULL;
	m_root.GetStructure( field_id, &pStructure, true );

	char buf[256];
	const char* pText = NULL;
	int value = 0;
	if ( pStructure->GetText( "ui_string", &pText ) )
	{
		Dbg_Assert( strlen( pText ) < 256 );
		strcpy( buf, pText );
	}
	else if ( pStructure->GetInteger( "value", &value ) )
	{
		sprintf( buf, "%d", value );
	}
	else if ( pStructure->GetText( NONAME, &pText, true ) )
	{
		Dbg_Assert( strlen( pText ) < 256 );
		strcpy( buf, pText );
	}
	else
	{
		Dbg_MsgAssert( "Couldn't find valid parameters in %s",( Script::FindChecksumName( control_id ) ));
	}

	if ( mask_password )
	{
		// replace all the letters with stars
		for ( uint32 i = 0; i < strlen(buf); i++ )
		{
			buf[i] = '*';
		}
	}
	
	//Front::SendString( control_id, buf );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Preferences::PrintContents()
{
#ifdef __NOPT_ASSERT__
	Script::PrintContents(&m_root);
#endif
}

} // namespace Prefs




