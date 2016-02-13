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
**	Module:			Gel						 								**
**																			**
**	File name:		Prefs.h													**
**																			**
**	Created:		03/12/2001	-	gj										**
**																			**
**	Description:	Generic Preferences Class								**
**																			**
*****************************************************************************/

#ifndef __GEL_PREFS_H
#define __GEL_PREFS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

//#include <gel/scripting/script.h>

#ifndef	__SCRIPTING_STRUCT_H
#include <gel/scripting/struct.h>
#endif


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Prefs
{

						


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  Preferences  : public Spt::Class
{
	

public:
	Preferences();
	virtual						~Preferences();

public:
	uint32						WriteToBuffer(uint8 *pBuffer, uint32 BufferSize);
	void						ReadFromBuffer(uint8 *pBuffer);

public:
	bool						Reset();
	bool						Load( uint32 structure_checksum );
	void						PrintContents();
	bool						UpdateUIElement( uint32 control_id, uint32 field_id, bool mask_password = false );

public:
	Script::CScriptStructure*	GetPreference( uint32 field_id );
	bool						SetPreference( uint32 field_id, Script::CScriptStructure* p_to_append );
	bool						SetPreference( uint32 field_id, Script::CArray* p_to_append );
	Script::CArray*				GetPreferenceArray( uint32 field_id );
	const char*					GetPreferenceString( uint32 field_id, uint32 sub_field_id );
	int							GetPreferenceValue( uint32 field_id, uint32 sub_field_id );
	uint32						GetPreferenceChecksum( uint32 field_id, uint32 sub_field_id );
	void						RemoveComponent( uint32 field_id, uint32 sub_field_id );

	Script::CScriptStructure*	GetRoot() {return &m_root;}
	void						SetRoot(Script::CScriptStructure* pStuff);
	
private:
	Script::CScriptStructure	m_root;
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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Prefs

#endif	// __GEL_PREFS_H


