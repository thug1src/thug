#ifndef _FILE_H_
#define _FILE_H_

#include <stdarg.h>
#include <dolphin.h>

typedef enum {
	NsFileSeek_Start = 0,
	NsFileSeek_End,
	NsFileSeek_Current,

	NsFileSeek_Pos = NsFileSeek_Current,

	NsFileSeek_Max
} NsFileSeek;

typedef enum
{
	FSIZE,
	FOPEN,
	FREAD,
	FSEEK,
	FGETC,
	FGETS,
	FCLOSE,
	FEOF,
	FTELL,
	FEXIST
} NsFileAccessType;



class NsFile;
typedef int (*NsFileAccessFunction)( NsFile* p_file, NsFileAccessType access_type, ... );


class NsFile
{
public:
	unsigned int				m_unique_tag;

	char						m_cachedBytes[32];
	void					  *	m_preHandle;

	DVDFileInfo					m_FileInfo;

	int							m_FileOpen;
	int							m_sizeCached;
	int							m_cachedSize;

#ifdef DVDETH
	char					  * mp_data;
	int							m_size;
	int							m_offset;
#endif		// DVDETH

	int							m_numCacheBytes;
	int							m_seekOffset;

	void						GetCacheBytes			( void * pDest, int numBytes );
	char						getcharacter			( void );

								NsFile					();

	bool						open					( const char * pFilename );
	int							size					( void );
	int							read					( void *pDest, int numBytes );
//	char						getc					( void );
	char					  * gets					( char * s, int n );
	void						seek					( NsFileSeek type, int offset );
	void						close					( void );
	int							exist					( const char * pFilename );
	int							eof						( void );
	int							tell					( void );
	void					  * load					( const char * pFilename );
	static NsFileAccessFunction	setFileAccessFunction	( NsFileAccessFunction file_access_function );

	bool						verify					( void )		{ return ( m_unique_tag == 0x1234ABCD ) ? true : false; }

private:

	static NsFileAccessFunction	m_file_access_function;
};




#endif		// _FILE_H_
