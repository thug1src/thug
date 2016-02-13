#include "p_assert.h"
#include "p_file.h"
#include "p_dvd.h"

// Default to the DVD file access stuff.
NsFileAccessFunction NsFile::m_file_access_function = DVDFileAccess;

NsFile::NsFile()
{
	m_FileOpen = 0;
	m_sizeCached = 0;
	m_numCacheBytes = 0;
	m_seekOffset = 0;
	m_preHandle = NULL;
	m_unique_tag = 0x1234ABCD;
}


NsFileAccessFunction NsFile::setFileAccessFunction( NsFileAccessFunction file_access_function )
{
	NsFileAccessFunction old	= m_file_access_function; 
	m_file_access_function		= file_access_function;
	return old;
}



void NsFile::GetCacheBytes ( void * pDest, int numBytes )
{
	memcpy ( pDest, &m_cachedBytes[0], numBytes );
	DCFlushRange(pDest, numBytes );
	// Update the cache.
	m_numCacheBytes -= numBytes;
	if ( m_numCacheBytes ) memcpy ( &m_cachedBytes[0], &m_cachedBytes[numBytes], m_numCacheBytes );
}


bool NsFile::open ( const char * pFilename )
{
	assert( m_file_access_function );


	return (bool)m_file_access_function( this, FOPEN, pFilename, "rb" );
}

int NsFile::size ( void )
{
	assert( m_file_access_function );

	return m_file_access_function( this, FSIZE );
}

int NsFile::read ( void *pDest, int numBytes )
{
	assert( m_file_access_function );

	return m_file_access_function( this, FREAD, pDest, numBytes );
}

void NsFile::seek ( NsFileSeek type, int offset )
{
	assert( m_file_access_function );

	// Horrible hack to fix seek bugs.
	m_file_access_function( this, FSEEK, type, offset );
}

void NsFile::close ( void )
{
	assert( m_file_access_function );

	m_file_access_function( this, FCLOSE );
}

int NsFile::exist ( const char * pFilename )
{
	assert( m_file_access_function );

	return m_file_access_function( this, FEXIST, pFilename );
}

char NsFile::getcharacter ( void )
{
	assert( m_file_access_function );

	return (char)m_file_access_function( this, FGETC );
}

char * NsFile::gets ( char * s, int n )
{
	assert( m_file_access_function );

	return (char*)m_file_access_function( this, FGETS, s, n );
}

int NsFile::eof ( void )
{
	assert( m_file_access_function );

	return m_file_access_function( this, FEOF );
}

int NsFile::tell ( void )
{
	assert( m_file_access_function );

	return (int)m_file_access_function( this, FTELL );
}

void * NsFile::load ( const char * pFilename )
{
	bool open_result = open ( pFilename );

	if( open_result ) {
		void * pDest = new unsigned char[size()];
		read ( pDest, size() );

		close();

		return pDest;
	}
	else {
		return NULL;
	}
}
