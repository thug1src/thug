//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       ModelAppearance.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  4/2/2000
//****************************************************************************

#ifndef __GFX_MODELAPPEARANCE_H
#define __GFX_MODELAPPEARANCE_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

#include <gel/object.h>

#include <gel/scripting/struct.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Gfx
{

/*****************************************************************************
**							Forward Declarations						**
*****************************************************************************/

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

	class CFaceTexture;

// A CModelAppearance contains all the data associated with
// generating a rendered model (NxModel).
			
class CModelAppearance : public Obj::CObject
{
public:
	CModelAppearance();
	CModelAppearance( const CModelAppearance& rhs );
	CModelAppearance&	operator=( const CModelAppearance& rhs );
	virtual 			~CModelAppearance();

public:
	virtual bool		CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript );

public:
	// Init: Clears it out to nothing
	bool				Init();

	// Load: Appends a structure
	bool				Load( Script::CStruct* pStructure, bool resolve_randoms = true );
	bool				Load( uint32 structure_name, bool resolve_randoms = true );

public:
	// for network message building
	// (note that the face texture is not included in this buffer
	// because the buffer is too small...  face textures need to
	// be sent through a separate pathway)
	uint32				WriteToBuffer(uint8* pBuffer, uint32 bufferSize, bool ignoreFaceData = false);
	uint8*				ReadFromBuffer(uint8* pBuffer);

public:
	// for debugging
	void				PrintContents(const Script::CStruct* p_structure = 0);

public:
	// used by builder, to apply the desired appearance to a particular SkinModel
	Script::CStruct*	GetActualDescStructure( uint32 partChecksum );
	Script::CStruct*	GetVirtualDescStructure( uint32 partChecksum );
	Script::CStruct*	GetStructure() { return &m_appearance; }
	Gfx::CFaceTexture*	GetFaceTexture();
	void				CreateFaceTexture();
	void				DestroyFaceTexture();
		
protected:
	void				resolve_randomized_desc_ids();
	void				set_part(uint32 partChecksum, uint32 descID, Script::CStruct* pParams);
	void				clear_part(uint32 partChecksum);
	void				set_checksum(uint32 fieldChecksum, uint32 valueChecksum);

protected:
	Script::CStruct		m_appearance;
	Gfx::CFaceTexture*	mp_faceTexture;
	
public:
	// the following is needed because skaters created in net games 
	// don't have a face texture during initial model building (the
	// face texture will eventually come in a later net packet),
	// but the model builder still needs to know if there will eventually
	// be a face texture so that it can load up the correct head geometry
	bool				WillEventuallyHaveFaceTexture();

protected:
	bool				m_willEventuallyHaveFaceTexture;
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

} // namespace Gfx

#endif	// __OBJECTS_MODELAPPEARANCE_H
