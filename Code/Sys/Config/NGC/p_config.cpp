// Config Manager stuff. KSH 20 Mar 2002
#include <sys/config/config.h>
#include <dolphin.h>
#include <sys/file/filesys.h>

extern void * get_font_file_address( const char * p_filename );

namespace Config
{

void Plat_Init(sint argc, char** argv)
{
	gHardware=HARDWARE_NGC;
	
	// I don't know yet how to autodetect the language for this platform, so
	// just set it to English for the moment.
	
	switch ( OSGetLanguage() )
	{
		case OS_LANG_GERMAN:
			gLanguage=LANGUAGE_GERMAN;
			break;
		case OS_LANG_FRENCH:
			gLanguage=LANGUAGE_FRENCH;
			{
				// Load French font file.
				void * p = get_font_file_address( "small" );
				void *p_FH = File::Open( "fonts\\small_fr.fnt.ngc", "rb" );
				if( p_FH )
				{
					int size = File::GetFileSize( p_FH );
					char * p_new = new char[size];
					File::Read( p_new, size, 1, p_FH );
					memcpy( p, p_new, size );
					delete p_new;
				}
			}
			break;
		case OS_LANG_SPANISH:
		case OS_LANG_ITALIAN:
		case OS_LANG_DUTCH:
		case OS_LANG_ENGLISH:
		default:
			gLanguage=LANGUAGE_ENGLISH;
			break;
	}

	// Autodetect these somehow ...
	if ( VIGetTvFormat() == VI_PAL )
	{
		gDisplayType=DISPLAY_TYPE_PAL;
		gFPS=50;
	}
	else
	{
		gDisplayType=DISPLAY_TYPE_NTSC;
		gFPS=60;
	}
	
	// See if we're on a devkit or not by checking the amount of memory
	int megs = (uint32)OSGetArenaHi() - (uint32)OSGetArenaLo();

	if ( megs >= ( 1024 * 1024 * 24 ) )
	{
		// We're on a devkit
		gCD=false;
	}
	else
	{
		// We're on an NR-Reader
		gCD=true;
	}
	
	gpMemCardHeader="SK5";
}

} // namespace Config

