// Config Manager stuff. KSH 20 Mar 2002
#include <sys/config/config.h>
#include <sk/product_codes.h>
#include <libscf.h>

extern "C"
{
int snputs(const char* pszStr);
}

namespace Config
{

static char s_elf_name[64];  // will be 'local' or 'skate5'

const char * GetElfName()
{
	return (const char *)s_elf_name;
}

static const char *sGenerateElfName(const char *p_memCardHeader)
{
	Dbg_MsgAssert(strlen(p_memCardHeader)==12,("Bad length for mem card header '%s'\n",p_memCardHeader));
	static char sp_elf_name[50];

	// Note: p_memCardHeader will have the form "BASLUS-20731"
	// "BASLUS-20731" needs to map to an elf name of "cdrom:\SLUS_207.31;1"
	
	sprintf(sp_elf_name,"cdrom0:\\SL%cS_%c%c%c.%c%c;1",
			p_memCardHeader[4],
			p_memCardHeader[7],
			p_memCardHeader[8],
			p_memCardHeader[9],
			p_memCardHeader[10],
			p_memCardHeader[11]);
	return sp_elf_name;
}

void Plat_Init(sint argc, char** argv)
{
	gHardware=HARDWARE_UNDEFINED;
	gGotExtraMemory=false;
	gCD=false;


	
	// must check to see if the supplied filename starts "cdrom0:\THPS4\"    SLUS_207.31;1

	// if the first real argument exists and starts with a digit, then assume
	// that we are running from a bootstrap (as we will just have been passed the language in argv[1])
	// and preempt any other parameters...
	// so assume running on a regular PS2, from the CD
	if (argc > 1)
	{
		if (argv[1][0] >= '0' && argv[1][0] <= '9')
		{
			printf ("argv[1][0] is a digit, so assuming bootstrap format, using \\thps4 directory\n");
			gBootstrap=true;
			gSonyBootstrap = true;
			gHardware=HARDWARE_PS2;
			gGotExtraMemory=false;
			gCD=true;

			// the rest we could, in theory, set from the sceLibDemo calls......			
			gLanguage=LANGUAGE_ENGLISH;
			gTerritory=TERRITORY_UNDEFINED;
			gDisplayType=DISPLAY_TYPE_NTSC;
			gFPS=60;

			
			return;
		}
		else
		{
			printf ("argv[1][0] (%s) is not digit, so it's a regular boot, using root\n",argv[1]);
		}
	}
	else
	{
		printf ("no arguments, so it's a regular boot, using root\n");
	}
	

	
	// If the filename ends in .elf, then extract out the name
	// c:\skate5\build\NGPSgnu\local.elf
	if (stricmp(argv[0]+strlen(argv[0])-4,".elf")==0)
	{
		char *p = argv[0]+strlen(argv[0])-5;		// letter before the .elf
		while (*p != '\\') p--;
		p++; // first letter of the name
		char *q = &s_elf_name[0];
		while (*p != '.') *q++ = *p++;
		*q++ = 0;
	}
	

	if (!argv[0] || stricmp(argv[0]+strlen(argv[0])-4,".elf")==0)
	{
		gHardware=HARDWARE_PS2_DEVSYSTEM;
		gGotExtraMemory=true;
		gCD=false;
	}
	else
	{
		// It's just a normal PS2
		gHardware=HARDWARE_PS2;
		gGotExtraMemory=false;
		gCD=true;
	}
	
	// Check command line to see if they're using ProView
	if (snputs("Plat_Init detected ProView ...\n")!=-1)
	{
		gHardware=HARDWARE_PS2_PROVIEW;
		gGotExtraMemory=false;
		gCD=false;
	}
	
	// Check command line to see if they want to force gGotExtraMemory on or off
	if (CommandLineContainsFlag("GotExtraMemory",argc,argv))		
	{
		gGotExtraMemory=true;
	}
	if (CommandLineContainsFlag("NoExtraMemory",argc,argv))		
	{
		gGotExtraMemory=false;
	}
	
	// Detect the language from the product code.
	gLanguage=LANGUAGE_ENGLISH;
	gpMemCardHeader=NGPS_NTSC;
	if (argv[0])
	{
		// Doing a stricmp just in case it changes to be CDROM later or something.
		if (stricmp(argv[0],sGenerateElfName(NGPS_NTSC))==0)
		{
			gLanguage=LANGUAGE_ENGLISH;
			gpMemCardHeader=NGPS_NTSC;
		}
		else if (stricmp(argv[0],sGenerateElfName(NGPS_PAL_ENGLISH))==0)
		{
			gLanguage=LANGUAGE_ENGLISH;
			gpMemCardHeader=NGPS_PAL_ENGLISH;
		}
		else if (stricmp(argv[0],sGenerateElfName(NGPS_PAL_FRENCH))==0)
		{
			gLanguage=LANGUAGE_FRENCH;
			gpMemCardHeader=NGPS_PAL_FRENCH;
		}
		else if (stricmp(argv[0],sGenerateElfName(NGPS_PAL_GERMAN))==0)
		{
			gLanguage=LANGUAGE_GERMAN;
			gpMemCardHeader=NGPS_PAL_GERMAN;
		}
		else if (stricmp(argv[0],sGenerateElfName(NGPS_PAL_ITALIAN))==0)
		{
			gLanguage=LANGUAGE_ITALIAN;
			gpMemCardHeader=NGPS_PAL_ITALIAN;
		}
		else if (stricmp(argv[0],sGenerateElfName(NGPS_PAL_SPANISH))==0)
		{
			gLanguage=LANGUAGE_SPANISH;
			gpMemCardHeader=NGPS_PAL_SPANISH;
		}
	}	

	// They may want to force the language to be something else from the command line ...
	const char *p_language=GetCommandLineParam("Language",argc,argv);
	if (p_language)
	{
		if (stricmp(p_language,"English")==0)
		{
			gLanguage=LANGUAGE_ENGLISH;
			gpMemCardHeader=NGPS_NTSC;
		}
		else if (stricmp(p_language,"French")==0)
		{
			gLanguage=LANGUAGE_FRENCH;
			gpMemCardHeader=NGPS_PAL_FRENCH;
		}
		else if (stricmp(p_language,"German")==0)
		{
			gLanguage=LANGUAGE_GERMAN;
			gpMemCardHeader=NGPS_PAL_GERMAN;
		}
		else if (stricmp(p_language,"Italian")==0)
		{
			gLanguage=LANGUAGE_ITALIAN;
			gpMemCardHeader=NGPS_PAL_ITALIAN;
		}
		else if (stricmp(p_language,"Spanish")==0)
		{
			gLanguage=LANGUAGE_SPANISH;
			gpMemCardHeader=NGPS_PAL_SPANISH;
		}
		else
		{
			Dbg_MsgAssert(0,("Language '%s' not supported",p_language));
		}
	}


	
	gTerritory=TERRITORY_UNDEFINED;

	gDisplayType=DISPLAY_TYPE_NTSC;
	gFPS=60;
	
	// Figure out if it is PAL from the product code ...
	if (argv[0])
	{
		char p_temp[50];
		strncpy(p_temp,argv[0],12);
		p_temp[12]=0; // strncpy won't terminate
		// Doing a stricmp just in case it changes to be CDROM later or something.
		if (stricmp(p_temp,"cdrom0:\\SLES")==0)
		{
			gDisplayType=DISPLAY_TYPE_PAL;
			gFPS=50;
		}	
	}
}

} // namespace Config

