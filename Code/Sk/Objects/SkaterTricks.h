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
**	File name:		objects/SkaterTricks.h									**
**																			**
**	Created: 		10/29/01	-	gj										**
**																			**
*****************************************************************************/

#ifndef __OBJECTS_SKATERTRICKS_H
#define __OBJECTS_SKATERTRICKS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/string/cstring.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Obj
{

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

class  SSpecialTrickInfo : public Spt::Class
{
	public:
		uint32	m_TrickName;
		uint32	m_TrickSlot;
		bool	m_isCat;

	public:
		SSpecialTrickInfo();
		bool			IsUnassigned() const;
		Str::String		GetTrickNameString() const;
		Str::String		GetTrickSlotString() const;
		Str::String		GetSlotMenuProperty() const;
};

class CTrickChecksumTable : public Spt::Class
{
	enum
	{
		// must be 256 or less, otherwise
		// the trick mapping database might
		// not compress to an acceptable size
		vMAX_TRICK_CHECKSUMS = 256
	};

	public:
		CTrickChecksumTable();

	public:
		void			Init( void );
		uint32			GetChecksumFromIndex( int index );
		int				GetIndexFromChecksum( uint32 checksum );

	protected:
		uint32			m_Checksum[vMAX_TRICK_CHECKSUMS];
		int				m_NumChecksums;
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

#endif	// __OBJECTS_SKATERTRICKS_H
