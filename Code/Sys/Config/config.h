// Config Manager stuff. KSH 20 Mar 2002
//
// A bunch of inline functions for quering what type of platform the executable is running on,
// what language is set, whether it's PAL or NTSC, etc blaa blaa.
//
// The first thing main() does is call the Config::Init function, which parses the command
// line and does various things to work out what the config is. Then the inline functions just
// return the values calculated in Init, so they're nice and fast.


#ifndef	__CONFIG_CONFIG_H
#define	__CONFIG_CONFIG_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

namespace Config
{

enum ELanguage
{
	LANGUAGE_UNDEFINED,
	
	// Prefixed with LANGUAGE_ so that they don't clash with the old defines that are still
	// in the code.
	LANGUAGE_JAPANESE,
	LANGUAGE_ENGLISH,
	LANGUAGE_FRENCH,
	LANGUAGE_SPANISH,
	LANGUAGE_GERMAN,
	LANGUAGE_ITALIAN,
	LANGUAGE_DUTCH,
	LANGUAGE_PORTUGUESE,
};

enum ETerritory
{
	TERRITORY_UNDEFINED,
	TERRITORY_EUROPE,
	TERRITORY_US,
	TERRITORY_ASIA,
};

enum EHardware
{
	// All prefixed with HARDWARE_ to be consistent with the language enum above ...
	HARDWARE_UNDEFINED,
	
	HARDWARE_PS2,			// A normal PS2
	HARDWARE_PS2_PROVIEW,	// A PS2 running ProView
	HARDWARE_PS2_DEVSYSTEM,	// A dev system with lots of memory
	
	HARDWARE_XBOX,		// An XBox
	HARDWARE_NGC,		// A GameCube
	HARDWARE_WIN32,		// A PC running Win32
};

enum EDisplayType
{
	DISPLAY_TYPE_PAL,
	DISPLAY_TYPE_NTSC,
};

extern EDisplayType gDisplayType;
inline EDisplayType GetDisplayType() {return gDisplayType;}
// Perhaps change this to just return a bool if speed is a problem ...
inline bool PAL() {return gDisplayType==DISPLAY_TYPE_PAL;}
inline bool NTSC() {return gDisplayType==DISPLAY_TYPE_NTSC;}

extern int gFPS;
extern inline int FPS() {return gFPS;}

extern EHardware gHardware;
inline EHardware GetHardware() {return gHardware;}

extern bool gGotExtraMemory;
inline bool GotExtraMemory() {return gGotExtraMemory;}

extern ELanguage gLanguage;
inline ELanguage GetLanguage() {return gLanguage;}

extern ETerritory gTerritory;
inline ETerritory GetTerritory() {return gTerritory;}

extern bool gCD;
inline bool CD() {return gCD;}

extern	float	gMasterVolume;
inline	float	GetMasterVolume() {return gMasterVolume;}
inline	void 	SetMasterVolume(float vol) {gMasterVolume=vol;}

extern bool gBootstrap;
extern bool gSonyBootstrap;
inline bool Bootstrap() {return gBootstrap;}				// true if some kind of bootstrap demo
inline bool SonyBootstrap() {return gSonyBootstrap;}		// true if it's a Sony specific bootstrap demo (with timeout, etc)
extern char	gReturnTo[];  
extern char	gReturnString[];

extern const char *gpMemCardHeader;

const char* GetDirectory();

const char* GetMemCardHeader();

const char *GetCommandLineParam(const char *p_name, sint argc, char** argv);
bool CommandLineContainsFlag(const char *p_name, sint argc, char** argv);

void Init(sint argc, char** argv);
void Plat_Init(sint argc, char** argv);

// Utility functions.
const char* GetLanguageName();
const char* GetTerritoryName();
const char* GetHardwareName();
const char* GetDisplayTypeName();

// Sony Specific, not needed on the others
const char *GetElfName();


} // namespace Config

#endif	// __CONFIG_CONFIG_H

