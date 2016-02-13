//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       SkaterProfile.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  11/29/2000
//****************************************************************************

#ifndef __OBJECTS_SKATERPROFILE_H
#define __OBJECTS_SKATERPROFILE_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <gfx/modelappearance.h>
#include <sk/objects/skatertricks.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Obj
{

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

// TODO:  Eventually, rename this CPlayerProfile,
// and subclass to get the CSkaterProfile


class CSkaterProfile : public Spt::Class
{
public:
	enum
	{
		vVERSION_NUMBER = 2,
		vMAXSPECIALTRICKSLOTS = 12,
	};

public:
	// slowly phase this first constructor out
	CSkaterProfile();							// set up default profile
	CSkaterProfile( Script::CStruct* pParams );
	CSkaterProfile( const CSkaterProfile& rhs );
	CSkaterProfile&	operator=( const CSkaterProfile& rhs );

public:
	uint32						WriteToBuffer(uint8 *pBuffer, uint32 BufferSize, 
											bool ignoreFaceData = false );
	uint8*						ReadFromBuffer(uint8 *pBuffer);
	void 						WriteIntoStructure( Script::CStruct* pStuff );
	void 						ReadFromStructure( uint32 SkaterName, Script::CStruct* pStuff );

	bool						Reset( Script::CStruct* pParams = NULL );						// reset to initial conditions
    bool						PartialReset( Script::CStruct* pParams = NULL );				// reset non story stuff
	
	// for querying whether the profile meets certain criteria (i.e. is vert skater?  is rodney mullen?)
	bool						ProfileEquals( Script::CStruct* pStructure );
	
	void						ResetDefaultAppearance();										   	
	void						ResetDefaultTricks();
	void						ResetDefaultStats();
	
public:
	// general accessors
	Gfx::CModelAppearance*		GetAppearance() {return &m_Appearance;}
	Script::CStruct*			GetInfo() {return &m_Info;}
	
	// query functions
	uint32						GetSkaterNameChecksum();
	Script::CStruct*			GetTrickMapping( uint32 TrickMappingName );
	Script::CStruct*			GetSpecialTricksStructure();
	void						SetHeadIsLocked( bool is_locked );
	bool						IsPro();
	bool						IsSecret();
	bool						HeadIsLocked();
	bool						IsLocked();
	const char*					GetCASFileName();
	void						SetCASFileName(const char *pFileName);
	
	// UI
	void						SetSkaterProperty( uint32 fieldId, const char* pPropertyName );
	const char*					GetDisplayName();
	uint32						GetChecksumValue( uint32 field_id );
	Str::String					GetUIString( uint32 field_id );
	Str::String					GetUIString( const char* field_name );
	
	// stats
	int							GetStatValue( uint32 property );
	bool						SetPropertyValue( uint32 property, int value );
	bool						AwardStatPoint( int inc_val = 1 );
	int							GetNumStatPointsAvailable();
	
	// tricks
	bool						AwardSpecialTrickSlot( int inc_val = 1 );
	uint32						GetNumSpecialTrickSlots();
	SSpecialTrickInfo			GetSpecialTrickInfo( int slot_num );
	bool						SetSpecialTrickInfo( int slot_num, const SSpecialTrickInfo& theInfo, bool update_mappings = true );

protected:
	void						compress_trick_mappings( const char* pTrickMappingName );
	void						decompress_trick_mappings( const char* pTrickMappingName );
	
protected:
	// net packet compression
	uint32						write_extra_info_to_buffer(uint8* pBuffer, uint32 BufferSize);
	uint8*						read_extra_info_from_buffer(uint8* pBuffer);

private:
	Gfx::CModelAppearance		m_Appearance;
	Script::CStruct				m_Info;
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

#endif	// __OBJECTS_SKATERPROFILE_H

#if 0
	// UI
	void						SetSkaterProperty( uint32 fieldId, int value );
	int							GetUIValue( uint32 checksum );
	Str::String					GetUIString( uint32 checksum );
#endif

#if 0
	// temp functions
	bool						part_matches( uint32 list_name, uint32 desc_name, Script::CStruct* pIfStructure );
	bool						sponsor_is_disqualified( uint32 partChecksum, uint32 descChecksum, Script::CArray* pSponsorArray );
	bool						part_is_disqualified( uint32 list_name, uint32 desc_id );
	bool						appearance_currently_contains( Script::CStruct* pIfStructure );
#endif

#if 0
	// for setting up the skater model
	bool						PartIsDisqualified( uint32 list_name, uint32 desc_id );
	bool						ColorModulationAllowed( uint32 list_name );
	
	bool						SetCASOption(uint32 setName, const char* pDesiredName);
	bool						SetOption(uint32 setName, uint32 descId);

	void						SetHue( uint32 setName, int value );
	void						SetSaturation( uint32 setName, int value );
	void						SetValue( uint32 setName, int value );
	
	int							GetNumOptions( uint32 list_name );
	Str::String					OptionName( uint32 list_name, int i );
	uint32						OptionId( uint32 list_name, int i );
	bool						OptionSelected( uint32 list_name, int i );
	bool						OptionDisqualified( uint32 list_name, int i );
	uint32						CurrentOption( uint32 list_name );
	Str::String					GetOptionNameFromDescId( uint32 list_name, uint32 desc_id );
	void						RandomizeAppearance();
#endif

