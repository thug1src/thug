//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       FaceTexture.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  4/29/2003
//****************************************************************************

#ifndef	__GFX_FACE_TEXTURE_H__
#define	__GFX_FACE_TEXTURE_H__

#include <core/defines.h>
#include <core/support.h>

// for SFacePoints
#include <gfx/facemassage.h>

namespace Gfx
{

//////////////////////////////////////////////////////////////////////////////
// This encapsulates all the face texture-related data
// (raw texture data, and any tweaks needed for correct
// mapping).  The "WriteToBuffer/ReadFromBuffer" is
// used to store this data to a memory card, to a skater
// profile, or for use in network games.
class CFaceTexture : public Spt::Class
{
public:
	enum
	{
		// TODO:  Find out how big the data really needs to be...
		// (right now, it's 16K for the texture data, and 1K for the palette data)
		vRAW_TEXTURE_SIZE = ( 17440 ),
		vMAX_OVERLAY_NAME_SIZE = 64,
		vTOTAL_CFACETEXTURE_SIZE = vRAW_TEXTURE_SIZE + sizeof(Nx::SFacePoints) + vMAX_OVERLAY_NAME_SIZE,
	};

	CFaceTexture();

public:
	// general accessors
	bool				IsValid() const;
	void				SetValid( bool valid );
	uint8*				GetTextureData();
	int					GetTextureSize() const;
	Nx::SFacePoints		GetFacePoints() const;
	void				SetFacePoints( const Nx::SFacePoints& facePoints );
	void				SetOverlayTextureName(const char* pTexture);
	const char*			GetOverlayTextureName();

	// debug face
	void				LoadFace( const char* pFaceName, bool fullPathIncluded );
	void				PrintContents();

public:
	// for downloading faces
	// (handles raw IMG.PS2 data only)
	uint8*				ReadTextureDataFromBuffer(uint8* pBuffer, int bufferSize);

public:
	// for network message handling
	// (handles raw IMG.PS2 data, face points, overlay information)
	int					WriteToBuffer(uint8* pBuffer, int bufferSize );
	uint8*				ReadFromBuffer(uint8* pBuffer, int bufferSize );
	
public:
	// for memory card loading/saving
	// (handles raw IMG.PS2 data, face points, overlay information)
	void 				WriteIntoStructure( Script::CStruct* pStuff );
	void 				ReadFromStructure( Script::CStruct* pStuff );

protected:
	bool				m_isValid;
	Nx::SFacePoints		m_points;
	uint8				m_rawData[vRAW_TEXTURE_SIZE];
	char				m_overlayTextureName[vMAX_OVERLAY_NAME_SIZE];
};

}

#endif //	__GFX_FACE_TEXTURE_H__

