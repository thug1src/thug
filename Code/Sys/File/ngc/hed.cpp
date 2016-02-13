/*	Header file functionality...
	.Hed files that describe the contents of .Wad files
	Written by Ken, stolen by Matt*/
//#include <eetypes.h>
//#include <eekernel.h>
//#include <stdio.h>
//#include <sifdev.h>
//#include <libsdr.h>
//#include <sdrcmd.h>
//#include <sdmacro.h>
//#include <string.h>
//#include <ctype.h>
//#include <sif.h>
//#include <sifrpc.h>
//
//#include <libcdvd.h>

#include <core/macros.h>
#include <core/defines.h>

#include <gel/scripting/script.h> 

#include <sys/file/filesys.h>
#include <sys/file/ngc/hed.h>
#include <sys/ngc/p_file.h>

bool sLoadingHed = false;

namespace File
{

static void get_checksums( const char * p_name, uint32 * p_dir, uint32 * p_file )
{
	char name[128];
	char dir[128];
	char file[128];
	uint lp;

	// Convert / to \\.
	for ( lp = 0; lp <= strlen( p_name ); lp++ )
	{
		name[lp] = p_name[lp];
		if ( name[lp] == '/' ) name[lp] = '\\';
	}

	// Find the directory name and the filename.
	for ( lp = strlen( name ) - 1; lp >= 0; lp-- )
	{
		if ( name[lp] == '\\' )
		{
			// Found the directory end.
			strcpy( file, &name[lp+1] );
			name[lp] = '\0';
			strcpy( dir, name );
			break;
		}
	}

	// Chop off the extension (if it exists). 
	if ( file[strlen(file)-4] == '.' )
	{
		file[strlen(file)-4] = '\0';
	}

	// Create checksums.
	if ( p_dir ) *p_dir = Script::GenerateCRC( dir );
	if ( p_file ) *p_file = Script::GenerateCRC( file );
}




// Searches the hed file for the filename. If not found, returns NULL.
SHedFile *FindFileInHed(const char *pFilename, SHed *pHed)
{
	if ( sLoadingHed ) return NULL;

	Dbg_MsgAssert(pFilename,("NULL pFilename"));
	Dbg_MsgAssert(pHed,("NULL pHed"));

	uint32 check_dir;
	uint32 check_file;

	get_checksums( pFilename, &check_dir, &check_file );

	SHed *pHd=pHed;
	while ( pHd->numFiles != 0xffffffff )
	{
		if ( pHd->Checksum == check_dir )
		{
			SHedFile * pF = pHd->p_fileList;
			for ( uint lp2 = 0; lp2 < pHd->numFiles; lp2++ )
			{
				if ( pF->Checksum == check_file )
				{
					return pF;
				}
				pF++;
			}
		}
		pHd++;
	}

	return NULL;
}

// Searches the hed file for the filename with the same checksum. If not found, returns NULL.
SHedFile *FindFileInHedUsingChecksum( uint32 checksum, SHed *pHed )
{
	if ( sLoadingHed ) return NULL;

	Dbg_MsgAssert(pHed,("NULL pHed"));

	SHed *pHd=pHed;
	while ( pHd->numFiles != 0xffffffff )
	{
		SHedFile * pF = pHd->p_fileList;
		for ( uint lp2 = 0; lp2 < pHd->numFiles; lp2++ )
		{
			if ( pF->Checksum == checksum )
			{
				return pF;
			}
			pF++;
		}
		pHd++;
	}

	return NULL;
}

//#ifdef __NOPT_CDROM__
//
//extern void StopStreaming( void );
//
//SHed *LoadHed( const char *filename )
//{
//	NsFile file;
//	SHed *pHed = NULL;
////	int HedId=-1;
//	char hedFileName[255];
//	char uppercaseFilename[ 255 ];
//	int sL = strlen( filename );
//	Dbg_MsgAssert( sL < 255,( "Filename %s too long", filename ));
//	int i;
//	for ( i = 0; i < sL; i++ )
//	{
//		uppercaseFilename[ i ] = toupper( filename[ i ] );
//	}
//	uppercaseFilename[ i ] = '\0';
//	StopStreaming( );
//	sprintf( hedFileName, "cdrom0:\\%s.HED;1", uppercaseFilename );
//	while (HedId<0)
//	{
//		file.open( hedFileName );
////		HedId=sceOpen( hedFileName, SCE_RDONLY );
//		if (HedId<0)
//		{
//			printf("Retrying opening %s\n", hedFileName );
//		}	
//	}
//	printf("%s opened successfully\n", uppercaseFilename );
////	long HedSize=sceLseek(HedId, 0, SCE_SEEK_END);
//	long HedSize=file.size();
//	
//	Dbg_MsgAssert(pHed==NULL,("pHed not NULL ?"));
//	pHed=(SHed*)Mem::Malloc((HedSize+2047)&~2047);
//
///*
//	char *p = (char*) pHed;  
//	// Hmmm.....
//	for (int i=0 ;i < (HedSize+2047)&~2047;i++)
//	{
//		p[i] = 0x55;
//	}
//*/						
//					
////	sceLseek(HedId, 0, SCE_SEEK_SET);
////	sceRead(HedId,pHed,HedSize);
////	sceClose(HedId);
//	file.read( pHed,, HedSize );
//	file.close();
//	return ( pHed );
//	return ( NULL );
//}
//#else
//
SHed *LoadHed( const char *filename )
{
	sLoadingHed = true;

	int lp;
	char tempFileName[255];
	sprintf( tempFileName, "%sNGC.HED", filename );

	void *fp = File::Open( tempFileName, "rb" );

	int fileSize = File::GetFileSize( fp );
	
	uint32 *pHed = (uint32 *)new (Mem::Manager::sHandle().TopDownHeap()) uint8[fileSize];
	Dbg_MsgAssert( pHed,( "Failed to allocate memory for hed %s", tempFileName ));

	File::Read( pHed, 1, fileSize, fp );
	File::Close( fp );

	// Convert to more compact list.
	int files = 0;
	int dirs = 0;

	// First, count how many files we have.
	uint32 * p32 = pHed;
	while ( p32[0] != 0xffffffff )
	{
		files++;

		// Byte order is reversed on NGC.
		p32[0] = ( ( p32[0] >> 24 ) & 0xff ) | ( ( p32[0] >> 8 ) & 0xff00 ) | ( ( p32[0] << 8 ) & 0xff0000 ) | ( ( p32[0] << 24 ) & 0xff000000 );
		p32[1] = ( ( p32[1] >> 24 ) & 0xff ) | ( ( p32[1] >> 8 ) & 0xff00 ) | ( ( p32[1] << 8 ) & 0xff0000 ) | ( ( p32[1] << 24 ) & 0xff000000 );

		int len = strlen( (char *)&p32[2] ) + 1;
		len = ( len + 3 ) & ~3;
		p32 = (uint32*)(((int)&p32[2]) + len);
	}

	// Build list of checksums.
	uint32 *p_offset = new (Mem::Manager::sHandle().TopDownHeap()) uint32[files];
	uint32 *p_size = new (Mem::Manager::sHandle().TopDownHeap()) uint32[files];
	uint32 *dir_check = new (Mem::Manager::sHandle().TopDownHeap()) uint32[files];
	uint32 *file_check = new (Mem::Manager::sHandle().TopDownHeap()) uint32[files];
	int count = 0;
	p32 = pHed;
	while ( p32[0] != 0xffffffff )
	{
		p_offset[count] = p32[0];
		p_size[count] = p32[1];
		get_checksums( (char *)&p32[2], &dir_check[count], &file_check[count] );
		count++;

		int len = strlen( (char *)&p32[2] ) + 1;
		len = ( len + 3 ) & ~3;
		p32 = (uint32*)(((int)&p32[2]) + len);
	}

	// Second, count how many directories we have.
	for ( lp = 0; lp < files; lp++ )
	{
		bool unique = true;
		for ( int lp2 = 0; lp2 < lp; lp2++ )
		{
			if ( dir_check[lp] == dir_check[lp2] )
			{
				unique = false;
				break;
			}
		}
		if ( unique ) dirs++;
	}

	// Now, allocate final piece of memory.
	int size = ( sizeof( SHed ) * ( dirs + 1 ) ) + ( sizeof( SHedFile ) * files );
	SHed * p_hed = (SHed *)new uint8[size];

	// Fill in directory entries.
	int dirs_added = 0;
	for ( lp = 0; lp < files; lp++ )
	{
		bool unique = true;
		for ( int lp2 = 0; lp2 < dirs_added; lp2++ )
		{
			if ( p_hed[lp2].Checksum == dir_check[lp] )
			{
				unique = false;
				break;
			}
		}
		if ( unique )
		{
			p_hed[dirs_added].Checksum = dir_check[lp];
			dirs_added++;
		}
	}

	// Fill in file entries.
	SHedFile * p_hed_file = (SHedFile *)&p_hed[(dirs+1)];
	for ( lp = 0; lp < dirs; lp++ )
	{
		p_hed[lp].p_fileList = p_hed_file;
		int files_this_dir = 0;
		for ( int lp2 = 0; lp2 < files; lp2++ )
		{
			if ( dir_check[lp2] == p_hed[lp].Checksum )
			{
				p_hed_file->Offset = p_offset[lp2];
				p_hed_file->FileSize = p_size[lp2];
				p_hed_file->Checksum = file_check[lp2];
				files_this_dir++;
				p_hed_file++;
			}
		}
		p_hed[lp].numFiles = files_this_dir;
	}
	p_hed[dirs].numFiles = 0xffffffff;

	// Get rid of temp data.
	delete file_check;
	delete dir_check;
	delete p_size;
	delete p_offset;
	delete pHed;

	sLoadingHed = false;

	return p_hed;
}
//#endif

} // namespace File


