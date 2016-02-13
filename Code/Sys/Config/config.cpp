// Config Manager stuff. KSH 20 Mar 2002
#include <sys/config/config.h>

namespace Config
{
ELanguage gLanguage=LANGUAGE_UNDEFINED;
ETerritory gTerritory=TERRITORY_UNDEFINED;
EHardware gHardware=HARDWARE_UNDEFINED;
bool gGotExtraMemory=false;
bool gCD=false;
bool gBootstrap=false;
bool gSonyBootstrap=false;
float	gMasterVolume = 100.0f; 		// precentage at which we play the overall volume
char	gReturnTo[128];					// full path of executable we return to from a demo (second argument if first is "bootstrap")
char	gReturnString[128];				// string that we return for demo

EDisplayType gDisplayType=DISPLAY_TYPE_NTSC;
int gFPS=60;

const char *gpMemCardHeader="";

// Returns true if p_string starts with p_prefix. Non-case-sensitive.
// (don't want to use strstr cos it is case sensitive)
static bool sIsPrefixedWith(const char *p_string, const char *p_prefix)
{
	if (!p_string || !p_prefix) return false;
	
	int string_len=strlen(p_string);
	int prefix_len=strlen(p_prefix);
	if (prefix_len>string_len)
	{
		// Can't be prefixed if shorter than the prefix.
		return false;
	}
	
	char p_buf[100];
	Dbg_MsgAssert(string_len<100,("String too long for buffer"));
	strcpy(p_buf,p_string);
	p_buf[prefix_len]=0;
	if (stricmp(p_buf,p_prefix)==0)
	{
		return true;
	}
	return false;
}

// If p_name is found to be followed by an equals in the command line, then this function
// will return the word following the equals.
// If p_name is not found, or equals blank, it will return NULL. 
const char *GetCommandLineParam(const char *p_name, sint argc, char** argv)
{
	const char *p_found=NULL;
	
	for (int i=0; i<argc; ++i)
	{
		if (sIsPrefixedWith(argv[i],p_name))
		{
			// p_name matches the start of argv[i]
			// Now see if it is followed by an equals.
			int len=strlen(p_name);
			char *p_next_char=argv[i]+len;
			if (*p_next_char=='=')
			{
				// It is followed by an equals.
				++p_next_char;
				if (*p_next_char)
				{
					// There is something after the equals, so use that.
					p_found=argv[i]+len+1;
				}
				else
				{
					// Use the next word, if there is one.
					if (i<argc-1)
					{
						p_found=argv[i+1];
					}	
				}
			}
			else if (*p_next_char==0)
			{
				// p_name matches argv[i] exactly.
				// So see if argv[i+1] starts with an equals.
				if (i<argc-1 && argv[i+1][0]=='=')
				{
					if (argv[i+1][1]==0)
					{
						// argv[i+1] is just '=', so use the next string.
						if (i<argc-2)
						{
							p_found=argv[i+2];
						}
					}
					else
					{
						// There is a word following the initial '=' in argv[i+1],
						// so use that word.
						p_found=argv[i+1]+1;
					}
				}		
			}
		}
	}
	return p_found;
}

// Returns true if the command line contains the word p_name anywhere. Not case sensitive.
bool CommandLineContainsFlag(const char *p_name, sint argc, char** argv)
{
	if (!p_name) return false;
	
	for (int i=0; i<argc; ++i)
	{
		if (stricmp(argv[i],p_name)==0)
		{
			return true;
		}	
	}
	return false;
}

// This function is the first thing called from main()
// It fills in all the globals in the Config namespace.
// It calls the platform specific Plat_Init() first.
// The idea is that the Plat_Init() function will do its best to fill in as many of the globals
// as it can.
// Then, the rest of the Init() function will fill in any that are missing, or override some
// based on the command line parameters.

// For example, the PS2 Plat_Init will look at the executable name to derive the territory.
// However, if the executable name is just skate5.elf it contains no territory info.
// So the Init() function, on seeing that the territory is undefined, will derive the territory
// from the language and whether it is PAL.
void Init(sint argc, char** argv)
{
	Plat_Init(argc,argv);
	
	// Override the gCD flag if CD or NotCD are present on the command line.
	if (CommandLineContainsFlag("CD",argc,argv))		
	{
		gCD=true;
	}
	if (CommandLineContainsFlag("NotCD",argc,argv))		
	{
		gCD=false;
	}

	if (CommandLineContainsFlag("Bootstrap",argc,argv))		
	{
		gBootstrap=true;
		if (argc > 2)
		{
			strcpy(gReturnTo,argv[2]);
			if (argc > 3)
			{
				strcpy(gReturnString,argv[3]);
			}
			else
			{
				sprintf(gReturnString,"THPS4");
			}			
			printf ("This bootstrap demo will return to %s passing back: %s\n",gReturnTo,gReturnString);
		}
	}
	if (CommandLineContainsFlag("NotBootstrap",argc,argv))		
	{
		gBootstrap=false;
	}
	
	if (CommandLineContainsFlag("Pal",argc,argv))		
	{
		gDisplayType=DISPLAY_TYPE_PAL;
	}
	if (CommandLineContainsFlag("NTSC",argc,argv))		
	{
		gDisplayType=DISPLAY_TYPE_NTSC;
	}
	
	const char *p_frame_rate=GetCommandLineParam("FrameRate",argc,argv);
	if (p_frame_rate)
	{
		if (strcmp(p_frame_rate,"50")==0)
		{
			gFPS=50;
		}
		if (strcmp(p_frame_rate,"60")==0)
		{
			gFPS=60;
		}
	}	
}

const char* GetMemCardHeader()
{
	return gpMemCardHeader;	
}

// Return the string for the directory we are running in
// generally this will be the root
// but when making a demo disk, then it will be the "THPS4" directory
const char* GetDirectory()
{
	if (Bootstrap())
	{
		return	"THPS4\\";	 // generally a subdirectory off the root
	}
	else
	{
		return "";			 // nothing needed it it is the root
	}
}

const char* GetLanguageName()
{
	switch (gLanguage)
	{
	case LANGUAGE_ENGLISH: 		return "English"; break;
	case LANGUAGE_FRENCH: 		return "French"; break;
	case LANGUAGE_GERMAN: 		return "German"; break;
	case LANGUAGE_JAPANESE:		return "Japanese"; break;
	case LANGUAGE_SPANISH:		return "Spanish"; break;
	case LANGUAGE_ITALIAN:		return "Italian"; break;
	case LANGUAGE_DUTCH:		return "Dutch"; break;
	case LANGUAGE_PORTUGUESE:	return "Portuguese"; break;
	default: 				return "Unknown language"; break;
	}
}

const char* GetTerritoryName()
{
	switch (gTerritory)
	{
	case TERRITORY_EUROPE: 	return "Europe"; break;
	case TERRITORY_US: 		return "US"; break;
	case TERRITORY_ASIA: 	return "Asia"; break;
	default: 				return "Unknown territory"; break;
	}
}

const char* GetHardwareName()
{
	switch (gHardware)
	{
	case HARDWARE_PS2: 				return "PS2"; break;
	case HARDWARE_PS2_PROVIEW:		return "PS2 ProView"; break;
	case HARDWARE_PS2_DEVSYSTEM:	return "PS2 Dev System"; break;
	case HARDWARE_XBOX:				return "XBox"; break;
	case HARDWARE_NGC:				return "GameCube"; break;
	case HARDWARE_WIN32:			return "Win32"; break;
	default:						return "Unknown platform"; break;
	}	
}

const char* GetDisplayTypeName()
{
	switch (gDisplayType)
	{
	case DISPLAY_TYPE_PAL:		return "PAL"; break;
	case DISPLAY_TYPE_NTSC:		return "NTSC"; break;
	default:					return "Unknown display type"; break;
	}
}

} // namespace Config

