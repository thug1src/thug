// Config Manager stuff. KSH 20 Mar 2002
#include <sys/config/config.h>

namespace Config
{

void Plat_Init(sint argc, char** argv)
{
	gHardware	= HARDWARE_XBOX;
	DWORD lang	= XGetLanguage();
	if( lang == XC_LANGUAGE_ENGLISH )
		gLanguage = LANGUAGE_ENGLISH;
	else
	{
		if( XGetVideoStandard() != XC_VIDEO_STANDARD_PAL_I )
		{
			// For NTSC, the only language allowed is English.
			gLanguage = LANGUAGE_ENGLISH;
		}
		else if( lang == XC_LANGUAGE_FRENCH )
		{
			gLanguage = LANGUAGE_FRENCH;
		}
		else if( lang == XC_LANGUAGE_GERMAN )
		{
			gLanguage = LANGUAGE_GERMAN;
		}
		else
		{
			// Any languages other than French and German should also be considered English.
			gLanguage = LANGUAGE_ENGLISH;
		}
	}
	
	// Kind of meaningless, but default CD to true for Final builds, false otherwise.
#	ifdef __NOPT_ASSERT__
	gCD = false;
#	else
	gCD = true;
#	endif
	
	switch( XGetVideoStandard())
	{
		case XC_VIDEO_STANDARD_PAL_I:
		{
			gDisplayType	= DISPLAY_TYPE_PAL;
			gFPS			= 50;
			if( XGetVideoFlags() & XC_VIDEO_FLAGS_PAL_60Hz )
			{
				gFPS		=	60;
			}	
			break;
		}
		case XC_VIDEO_STANDARD_NTSC_M:
		case XC_VIDEO_STANDARD_NTSC_J:
		{
			gDisplayType	= DISPLAY_TYPE_NTSC;
			gFPS			= 60;
			break;
		}
		default:
		{
			Dbg_MsgAssert( 0, ("Unrecognized return value (%d) from XGetVideoStandard()", XGetVideoStandard()));
			break;
		}
	}	
}

} // namespace Config

