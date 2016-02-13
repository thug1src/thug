#ifndef	__SK_LANGUAGE_H
#define	__SK_LANGUAGE_H

#ifdef __PLAT_XBOX__
#include <xtl.h>
#endif // __PLAT_XBOX__

// These only apply if PAL
#define ENGLISH 1
#define FRENCH 0
#define GERMAN 0

inline bool IsEnglish( void )
{
#	ifdef __PLAT_XBOX__
	DWORD lang = XGetLanguage();
	if( lang == XC_LANGUAGE_ENGLISH )
		return true;
	else
	{
		// For NTSC, the only language allowed is English.
		if( XGetVideoStandard() != XC_VIDEO_STANDARD_PAL_I )
		{
			return true;
		}

		// Any languages other than French and German should also be considered English.
		if(( lang != XC_LANGUAGE_GERMAN ) && ( lang != XC_LANGUAGE_FRENCH ))
		{
			return true;
		}
	}
	return false;
//		return true;		// For now...
#	else
#	if ENGLISH
	return true;
#	else
	return false
#	endif // ENGLISH
#	endif // __PLAT_XBOX__
}



#endif

