//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       FaceTexture.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  4/29/2003
//****************************************************************************

#include <gfx/facetexture.h>

#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

// for temporary texture
#include <sys/file/filesys.h>

namespace Gfx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CFaceTexture::CFaceTexture()
{
	m_isValid = false;

	SetDefaultFacePoints( &m_points );
	
	SetOverlayTextureName( "faces\\CS_NSN_Head_wht_alpha" );

/*
	// should already be cleared out
	memset( m_rawData, 0, vFACE_TEXTURE_SIZE );
*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFaceTexture::LoadFace( const char* pFaceName, bool fullPathIncluded )
{
	// As a test, set the freak image
	// (otherwise, we'd need to go through the face download interface)
	char extended_filename[512];
	int file_size;

	if ( fullPathIncluded )
	{
		sprintf( extended_filename, "%s.img.ps2", pFaceName );
	}
	else
	{
		sprintf( extended_filename, "images\\%s.img.ps2", pFaceName );	
	}

	char* pData = (char*)File::LoadAlloc( extended_filename, &file_size );
	memcpy( m_rawData, pData, file_size );
	Dbg_MsgAssert( file_size <= vRAW_TEXTURE_SIZE, ( "File too big (file is %d, buffer is %d)", file_size, vRAW_TEXTURE_SIZE ) );
	Mem::Free( pData );
	
	m_isValid = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8* CFaceTexture::ReadTextureDataFromBuffer(uint8* pBuffer, int bufferSize)
{
	// READ RAW IMG.PS2 DATA ONLY, NOT FACE POINT OR OVERLAY INFO

	// if a safety-check was specified...
	#ifdef	__NOPT_ASSERT__
	if ( bufferSize )
	{
		int totalSize = vRAW_TEXTURE_SIZE;
		Dbg_MsgAssert( bufferSize >= totalSize, ( "Buffer size (%d bytes) must <= %d bytes", bufferSize, totalSize ) );
	}
	#endif
	
	memcpy( &m_rawData[0], pBuffer, vRAW_TEXTURE_SIZE );

	pBuffer += vRAW_TEXTURE_SIZE;
	
	return pBuffer;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CFaceTexture::WriteToBuffer( uint8* pBuffer, int bufferSize )
{
	int totalSize = 0;
		
	totalSize += vRAW_TEXTURE_SIZE;
	Dbg_MsgAssert( bufferSize >= totalSize, ( "Buffer size (%d bytes) must >= %d bytes", bufferSize, totalSize ) );
	memcpy( pBuffer, &m_rawData[0], vRAW_TEXTURE_SIZE );
	pBuffer += vRAW_TEXTURE_SIZE;
	
	totalSize += sizeof(Nx::SFacePoints);
	Dbg_MsgAssert( bufferSize >= totalSize, ( "Buffer size (%d bytes) must >= %d bytes", bufferSize, totalSize ) );
	memcpy( pBuffer, &m_points, sizeof(Nx::SFacePoints) );
	pBuffer += sizeof(Nx::SFacePoints);

	totalSize += ( strlen( m_overlayTextureName ) + 1 );
	Dbg_MsgAssert( bufferSize >= totalSize, ( "Buffer size (%d bytes) must >= %d bytes", bufferSize, totalSize ) );
	memcpy( pBuffer, m_overlayTextureName, strlen( m_overlayTextureName ) + 1 );
	pBuffer += ( strlen( m_overlayTextureName ) + 1 );

	return totalSize;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8* CFaceTexture::ReadFromBuffer( uint8* pBuffer, int bufferSize )
{
	int totalSize = 0;
	
	totalSize += vRAW_TEXTURE_SIZE;
	if ( bufferSize )
	{
		// if a safety-check was specified...
		Dbg_MsgAssert( bufferSize >= totalSize, ( "Buffer size (%d bytes) must <= %d bytes", bufferSize, totalSize ) );
	}
	memcpy( &m_rawData[0], pBuffer, vRAW_TEXTURE_SIZE );
	pBuffer += vRAW_TEXTURE_SIZE;
	
	totalSize += sizeof(Nx::SFacePoints);
	if ( bufferSize )
	{
		// if a safety-check was specified...
		Dbg_MsgAssert( bufferSize >= totalSize, ( "Buffer size (%d bytes) must <= %d bytes", bufferSize, totalSize ) );
	}
	memcpy( &m_points, pBuffer, sizeof(Nx::SFacePoints) );
	pBuffer += sizeof(Nx::SFacePoints);

	int overlayNameLength = strlen( (char*)pBuffer );
	if ( overlayNameLength >= vMAX_OVERLAY_NAME_SIZE )
	{
		Dbg_MsgAssert( 0, ( "Unusual length for overlay name" ) );
	}
	
	totalSize += ( overlayNameLength + 1 );
	if ( bufferSize )
	{
		// if a safety-check was specified...
		Dbg_MsgAssert( bufferSize >= totalSize, ( "Buffer size (%d bytes) must <= %d bytes", bufferSize, totalSize ) );
	}
	memcpy( m_overlayTextureName, pBuffer, overlayNameLength + 1 );
	pBuffer += ( overlayNameLength + 1 );

	return pBuffer;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CFaceTexture::WriteIntoStructure( Script::CStruct* pSubStruct )
{
	Dbg_Assert( pSubStruct );        
	Dbg_MsgAssert( IsValid(), ( "Face texture is not valid!" ) );

	Script::CArray* pRawTextureArray = new Script::CArray;
	Dbg_MsgAssert( ( vRAW_TEXTURE_SIZE ) % 4 == 0, ( "Was expecting raw texture size to be a multiple of 4", vRAW_TEXTURE_SIZE ) ); 
	pRawTextureArray->SetSizeAndType( vRAW_TEXTURE_SIZE / 4, ESYMBOLTYPE_NAME );

	uint32* pFaceData = (uint32*)&m_rawData[0];

	for ( int i = 0; i < (vRAW_TEXTURE_SIZE / 4); i++ )
	{
		
		pRawTextureArray->SetChecksum( i, *pFaceData );
		pFaceData++;
	}

	pSubStruct->AddArrayPointer( CRCD(0x99790d96,"rawTextureData"), pRawTextureArray );
	pSubStruct->AddString( CRCD(0xa7e7a264,"overlayTextureName"), m_overlayTextureName );
	Nx::SetFacePointsStruct( m_points, pSubStruct );

	// readfromstructure should also set the valid flag
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CFaceTexture::ReadFromStructure( Script::CStruct* pSubStruct )
{
	Dbg_Assert( pSubStruct );

	Script::CArray* pRawTextureArray;
	pSubStruct->GetArray( CRCD(0x99790d96,"rawTextureData"), &pRawTextureArray, Script::ASSERT );
	
	uint32* pFaceData = (uint32*)&m_rawData[0];
	Dbg_Assert( pRawTextureArray->GetSize() * sizeof(uint32) == vRAW_TEXTURE_SIZE );
	for ( uint32 i = 0; i < pRawTextureArray->GetSize(); i++ )
	{
		*pFaceData = pRawTextureArray->GetChecksum( i );
		pFaceData++;
	}
	
	const char* pOverlayTextureName;
	pSubStruct->GetString( CRCD(0xa7e7a264,"overlayTextureName"), &pOverlayTextureName, Script::ASSERT );
	strcpy( m_overlayTextureName, pOverlayTextureName );

	Nx::GetFacePointsStruct( m_points, pSubStruct );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CFaceTexture::PrintContents()
{
#ifdef __NOPT_ASSERT__
	m_points.PrintData();
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CFaceTexture::IsValid() const
{
	return m_isValid;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFaceTexture::SetValid( bool valid )
{
	m_isValid = valid;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
uint8* CFaceTexture::GetTextureData()
{
	return m_rawData;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CFaceTexture::GetTextureSize() const
{
	return vRAW_TEXTURE_SIZE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::SFacePoints CFaceTexture::GetFacePoints() const
{
	return m_points;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFaceTexture::SetFacePoints( const Nx::SFacePoints& facePoints )
{
	m_points = facePoints;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFaceTexture::SetOverlayTextureName(const char* pTextureName)
{
	Dbg_MsgAssert( pTextureName, ( "No texture name specified" ) );
	Dbg_MsgAssert( strlen(pTextureName) < vMAX_OVERLAY_NAME_SIZE, ( "Overlay name is too long %s (max=%d)", pTextureName, vMAX_OVERLAY_NAME_SIZE ) );

	strcpy( m_overlayTextureName, pTextureName );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char* CFaceTexture::GetOverlayTextureName()
{
	return m_overlayTextureName;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx


