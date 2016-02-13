/*	Header file functionality...
	.Hed files that describe the contents of .Wad files
	Written by Ken, stolen by Matt*/
#include <eetypes.h>
#include <eekernel.h>
#include <stdio.h>
#include <sifdev.h>
#include <libsdr.h>
#include <sdrcmd.h>
#include <sdmacro.h>
#include <string.h>
#include <ctype.h>
#include <sif.h>
#include <sifrpc.h>

#include <libcdvd.h>


#include <core/macros.h>
#include <core/defines.h>

#include <gel/scripting/script.h> 

#include <sys/file/filesys.h>
#include <sys/file/ngps/hed.h>
#include <sys/config/config.h>

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
			if ( name[0]=='\\' )
			{
				strcpy( dir, &name[1] );
			}
			else
			{
				strcpy( dir, name );
			}
			break;
		}
	}

	// Create checksums.
	if ( p_dir ) *p_dir = Script::GenerateCRC( dir );
	if ( p_file ) *p_file = Script::GenerateCRC( file );
}


// Searches the hed file for the filename. If not found, returns NULL.
SHedFile *FindFileInHed(const char *pFilename, SHed *pHed)
{
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
	// Garrett: This function can take over .1 ms to execute when there are over 2000 entries.  We may want to make a better
	// search for this.
	//uint64 start_time = Tmr::GetTimeInUSeconds();	
	//int iterations = 0;

	Dbg_MsgAssert(pHed,("NULL pHed"));

	SHed *pHd=pHed;
	while ( pHd->numFiles != 0xffffffff )
	{
		SHedFile * pF = pHd->p_fileList;
		for ( uint lp2 = 0; lp2 < pHd->numFiles; lp2++ )
		{
			//iterations++;
			if ( pF->Checksum == checksum )
			{
				//uint64 end_time = Tmr::GetTimeInUSeconds();	
				//if ((end_time - start_time) > 100)
				//	Dbg_Message("FindNameFromChecksum Found Iterations %d Time %d", iterations, (end_time - start_time));
				return pF;
			}
			pF++;
		}
		pHd++;
	}

	//uint64 end_time = Tmr::GetTimeInUSeconds();	
	//if ((end_time - start_time) > 100)
	//	Dbg_Message("FindNameFromChecksum NotFound Iterations %d Time %d", iterations, (end_time - start_time));
	return NULL;
}

extern void StopStreaming( void );

#ifdef	__NOPT_ASSERT__
// Keep track of the hed buffers so the leak code doesn't think they are leaks
#define MAX_HED_BUFFERS 3
uint32 *sp_hed_buffers[MAX_HED_BUFFERS] = { (uint32 *) 16, (uint32 *) 16, (uint32 *) 16 };
int s_num_hed_buffers = 0;
#endif //	__NOPT_ASSERT__

SHed *LoadHed( const char *filename, bool force_cd, bool no_wad )
{
	int lp;
	uint32 *pHed = NULL;

	if (Config::CD() || force_cd)
	{
		int HedId=-1;
		char hedFileName[255];
		char uppercaseFilename[ 255 ];
		int sL = strlen( filename );
		Dbg_MsgAssert( sL < 255,( "Filename %s too long", filename ));
		int i;
		for ( i = 0; i < sL; i++ )
		{
			uppercaseFilename[ i ] = toupper( filename[ i ] );
		}
		uppercaseFilename[ i ] = '\0';
		StopStreaming( );
		sprintf( hedFileName, "cdrom0:\\%s%s.HED;1", Config::GetDirectory(),uppercaseFilename );
		int retry = 0;
		while (HedId<0)
		{
			HedId=sceOpen( hedFileName, SCE_RDONLY );
			if (HedId<0)
			{
				printf("Retrying opening %s\n", hedFileName );
				retry++;
				if (retry == 10)
				{
					return NULL;					
				}
			}	
		}
		printf("%s opened successfully\n", uppercaseFilename );
		long HedSize=sceLseek(HedId, 0, SCE_SEEK_END);
		
		Dbg_MsgAssert(pHed==NULL,("pHed not NULL ?"));
		if (no_wad)
		{
			// We'll want to keep this around
			pHed = new (Mem::Manager::sHandle().DebugHeap()) uint32[((HedSize+2047)&~2047) >> 2];
		}
		else
		{
			pHed=(uint32*)Mem::Malloc((HedSize+2047)&~2047);
		}
		Dbg_MsgAssert(pHed,("pHed NULL ?"));

	/*
		char *p = (char*) pHed;  
		// Hmmm.....
		for (int i=0 ;i < (HedSize+2047)&~2047;i++)
		{
			p[i] = 0x55;
		}
	*/						
		// 9/5/03 - Garrett: Burn fix.  Added retry code to see if HED file actually written.
		// If it cannot, it frees the buffer and returns NULL.
		int read_bytes = 0;
		retry = 0;
		while (read_bytes != HedSize)
		{
			sceLseek(HedId, 0, SCE_SEEK_SET);
			read_bytes = sceRead(HedId,pHed,HedSize);
			if (read_bytes != HedSize)
			{
				printf("READ ERROR: Retrying read of %s\n", hedFileName );
				retry++;
				if (retry == 4)
				{
					printf("READ ERROR: aborting read of %s\n", hedFileName );
					delete pHed;
					return NULL;					
				}
			}	
		}

		sceClose(HedId);
	}
	else
	{
		char tempFileName[ 255];

		// Make filename
		if (no_wad)
		{
			sprintf( tempFileName, "host:%shost.HED", filename );
		}
		else
		{
			sprintf( tempFileName, "host:%s.HED", filename );
		}
	
		void *fp = File::Open( tempFileName, "rb" );
		if ( !fp )
		{
// Mick:   Don't assert, as we want to be able to run without a .HED on the disk, for music
//			Dbg_MsgAssert( 0,( "Couldn't find hed file %s", tempFileName ));
			printf( "failed to find %s\n", tempFileName );
			return ( NULL );
		}
		int fileSize = File::GetFileSize( fp );
		File::Seek( fp, 0, SEEK_SET );
		
		if (no_wad)
		{
			// We'll want to keep this around
			pHed = new (Mem::Manager::sHandle().DebugHeap()) uint32[((fileSize+2047)&~2047) >> 2];

#ifdef	__NOPT_ASSERT__
			if (s_num_hed_buffers < MAX_HED_BUFFERS)
			{
				sp_hed_buffers[s_num_hed_buffers++] = pHed;
			}
#endif //	__NOPT_ASSERT__
		}
		else
		{
			pHed=(uint32*)Mem::Malloc((fileSize+2047)&~2047);
		}
		Dbg_MsgAssert( pHed,( "Failed to allocate memory for hed %s", tempFileName ));
		
		File::Read( pHed, 1, fileSize, fp );
		File::Close( fp );
	}

	// Convert to more compact list.
	int files = 0;
	int dirs = 0;

	// First, count how many files we have.
	uint32 * p32 = pHed;
	while ( p32[0] != 0xffffffff )
	{
		files++;

		int len = strlen( (char *)&p32[2] ) + 1;
		len = ( len + 3 ) & ~3;
		p32 = (uint32*)(((int)&p32[2]) + len);
	}

	// Build list of checksums.
	uint32 *p_offset = new (Mem::Manager::sHandle().TopDownHeap()) uint32[files];
	uint32 *p_size = new (Mem::Manager::sHandle().TopDownHeap()) uint32[files];
	uint32 *dir_check = new (Mem::Manager::sHandle().TopDownHeap()) uint32[files];
	uint32 *file_check = new (Mem::Manager::sHandle().TopDownHeap()) uint32[files];
	char **p_filename = new (Mem::Manager::sHandle().TopDownHeap()) (char *)[files];
	int count = 0;
	p32 = pHed;
	while ( p32[0] != 0xffffffff )
	{
		p_offset[count] = p32[0];
		p_size[count] = p32[1];
		p_filename[count] = (char *)&p32[2];
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
				p_hed_file->FileSize = p_size[lp2];
				if (no_wad)
				{
					p_hed_file->p_filename = p_filename[lp2];
					p_hed_file->FileSize |= SHedFile::mNO_WAD;		// This marks that we are using the no_wad version
				}
				else
				{
					p_hed_file->Offset = p_offset[lp2];
				}
				p_hed_file->Checksum = file_check[lp2];
				files_this_dir++;
				p_hed_file++;
			}
		}
		p_hed[lp].numFiles = files_this_dir;
	}
	p_hed[dirs].numFiles = 0xffffffff;

	// Get rid of temp data.
	delete p_filename;
	delete file_check;
	delete dir_check;
	delete p_size;
	delete p_offset;
	if (!no_wad)
	{
		delete pHed;
	}

	return p_hed;
}

} // namespace File
