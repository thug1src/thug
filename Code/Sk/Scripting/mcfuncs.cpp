//****************************************************************************
//* MODULE:         Sk/Scripting
//* FILENAME:       McFuncs.cpp
//* OWNER:          Kendall Harrison
//* CREATION DATE:  6/10/2002
//****************************************************************************

// Contains Mem card stuff.

// TODO: Make all local variables conform to the naming convention! Currently they are a mix

// start autoduck documentation
// @DOC cfuncs
// @module cfuncs | None
// @subindex Scripting Database
// @index script | cfuncs

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/scripting/mcfuncs.h>
#include <sys/file/filesys.h>
#include <sys/mcman.h>
#include <sys/config/config.h>
#include <gel/music/music.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/symboltable.h> 
#include <gel/scripting/utils.h>
#include <gel/soundfx/soundfx.h>
#include <sk/objects/records.h>
#include <sk/objects/playerprofilemanager.h>
#include <sk/objects/skatercareer.h>
#include <sk/modules/skate/GoalManager.h>
#include <sk/parkeditor2/parked.h>
#include <gel/scripting/component.h>
#include <sys/replay/replay.h>
#include <gel/objtrack.h>
#include <sk/components/goaleditorcomponent.h>
#include <sk/components/raileditorcomponent.h>

#ifdef __PLAT_NGPS__
#include <libmc.h>
#endif // __PLAT_NGPS__

#ifdef __PLAT_NGC__
#include "sys/ngc/p_file.h"
#include "sys/ngc/p_buffer.h"
#include "sys/ngc/p_dma.h"
#include "sys/ngc/p_aram.h"
bool g_mc_hack = false;
uint32 g_hack_address = 0;
extern char * g_p_buffer;
#endif		// __PLAT_NGC__

namespace CFuncs
{

using namespace Script;
	
/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

// Whenever the format of one of these file types has changed such that
// an old save will not load, increment its version number.
/*
// GJ:  I moved this to memcard.q, because the artists/designers sometimes
// do things that will require a version change.

#define VERSION_OPTIONSANDPROS 10
#define VERSION_NETWORKSETTINGS 3
#define VERSION_CAS 4
#define VERSION_CAT 1
#define VERSION_PARK 2
#define VERSION_REPLAY 6
*/

// The max number of files that can appear in the file list.
// A max is specified both to ensure we don't run out of memory in the files menu, and
// also to limit the pause before the menu comes up because it has to open each file first
// to extract the summary info.
#define MAX_THPS4_FILES_ALLOWED 75

// Limit the space that the summary info structure is allowed to take up when written to a buffer.
// This is because we want to be able to quickly read in the summary info from the start of the
// mem card file without having to read in the whole file. (Units are bytes)
#define MAX_SUMMARY_INFO_SIZE 100

// Max chars allowed in the low level card filename.
#define MAX_CARD_FILE_NAME_CHARS 100

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

// This value gets added to the character 'a' in the last char of the 
// low-level card filename to indicate what the type of the file is.
enum EMemCardFileType
{
	MEMCARD_FILETYPE_CAREER,
	MEMCARD_FILETYPE_CAS,
	MEMCARD_FILETYPE_PARK,
	MEMCARD_FILETYPE_REPLAY,
	MEMCARD_FILETYPE_NETWORKSETTINGS,
    MEMCARD_FILETYPE_CAT,
    MEMCARD_FILETYPE_CREATEDGOALS,
};

// The old OptionsAndPros save file now contains the custom skater too, and is divided into 4 parts.
// The LoadFromMemoryCard command provides the option of only applying certain parts to the game state.
// For example, when loading a custom skater, only the CUSTOM_SKATER part is applied, so the career
// remains unaffected.
enum EUnifiedSaveFileSubTypes
{
	APPLY_GLOBAL_INFO=0,
	APPLY_STORY=1,
	APPLY_STORY_SKATER=2,
	APPLY_CUSTOM_SKATER=3,
};
	
#ifdef __PLAT_NGC__
// Icons plus banner.
#define	NGC_MEMCARD_ICON_DATA_SIZE		( 64 + ( 32 * 32 * 7 ) + ( 256 * 2 ) + ( 96 * 32 ) + ( 256 * 2 ) )
#endif

// The mem card file starts with one of these, followed by two CStruct's as written
// out using WriteToBuffer.
// The first CStruct is the summary info for the file and occupies no more than
// MAX_SUMMARY_INFO_SIZE bytes.
// The second CStruct holds all the save data.
struct SMcFileHeader
{
	#ifdef __PLAT_NGC__
	// On the GameCube the icon data is stored within the file rather than as a
	// seperate file. So lets wack it into this structure.
	uint8 mpIconData[NGC_MEMCARD_ICON_DATA_SIZE];
	#endif
	
	#ifdef __PLAT_XBOX__
	// I think it is a TRC requirement to use the XBox's special way of calculating
	// a checksum. They probably have ways of telling whether we're using it or not.
	XCALCSIG_SIGNATURE mSignature;
	#else
	uint32 mChecksum;
	#endif // #ifdef __PLAT_XBOX__
	
	// This is so that the files menu can see if a file might be bad 
	// given only the summary info.
	uint32 mSummaryInfoChecksum;
	int mSummaryInfoSize;

	// This is the size of this header, the summary info structure, and
	// the main structure, rounded up to the platforms block size.
	// So for all file types except replays, this should match the total file size.
	// In the case of replays, the replay data comes after that lot.
	// mDataSize is needed to indicate the start of the replay data.
	int mDataSize;
	
	int mVersion;
};
	
/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

#ifndef __PLAT_NGC__
static unsigned short s_ascii_special[33][2] = {
	{0x8140, 32},		/*   */
	{0x8149, 33},		/* ! */
	{0x8168, 34},		/* " */
	{0x8194, 35},		/* # */
	{0x8190, 36},		/* $ */
	{0x8193, 37},		/* % */
	{0x8195, 38},		/* & */
	{0x8166, 39},		/* ' */
	{0x8169, 40},		/* ( */
	{0x816a, 41},		/* ) */
	{0x8196, 42},		/* * */
	{0x817b, 43},		/* + */
	{0x8143, 44},		/* , */
	{0x817c, 45},		/* - */
	{0x8144, 46},		/* . */
	{0x815e, 47},		/* / */
	{0x8146, 58},		/* : */
	{0x8147, 59},		/* ; */
	{0x8171, 60},		/* < */
	{0x8181, 61},		/* = */
	{0x8172, 62},		/* > */
	{0x8148, 63},		/* ? */
	{0x8197, 64},		/* @ */
	{0x816d, 91},		/* [ */
	{0x818f, 92},		/* \ */
	{0x816e, 93},		/* ] */
	{0x814f, 94},		/* ^ */
	{0x8151, 95},		/* _ */
	{0x8165, 96},		/* ` */
	{0x816f, 123},		/* { */
	{0x8162, 124},		/* | */
	{0x8170, 125},		/* } */
	{0x8150, 126},		/* ~ */
};

static unsigned short s_ascii_table[3][2] = {
	{0x824f, 0x30},	/* 0-9  */
	{0x8260, 0x41},	/* A-Z  */
	{0x8281, 0x61},	/* a-z  */
};
#endif // #ifndef __PLAT_NGC__

// Used by the SaveFailedDueToInsufficientSpace command.
static bool s_insufficient_space=false;

#if __USE_REPLAYS__
static char spReplayCardFileName[MAX_CARD_FILE_NAME_CHARS+1];
static bool sNeedToLoadReplayBuffer=false;
#endif
/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/
static const char *s_generate_ascii_checksum(char *p_dest, const char *p_string, uint32 fileType=0);
static uint32 s_determine_file_type(char c);
#ifndef __PLAT_NGC__
static unsigned short s_ascii_to_sjis(unsigned char ascii_code);
#endif
static void s_insert_global_info(CStruct *p_struct);
static void s_insert_story_info(CStruct *p_struct);
static void s_insert_story_skater_info(CStruct *p_struct);
static void s_insert_custom_skater_info(CStruct *p_struct);
static void s_insert_game_save_info(uint32 fileType, CStruct *p_struct);
static void s_generate_summary_info(CStruct *p_summaryInfo, uint32 fileType, CStruct *p_mainData);

static void s_read_global_info(CStruct *p_globalInfo, CScript *p_script);
static void s_read_story_info(CStruct *p_storyInfo);
static void s_read_story_skater_info(CStruct *p_storySkaterInfo, CStruct *p_customSkater);
static void s_read_custom_skater_info(CStruct *p_customSkater);
static void s_read_game_save_info(uint32 fileType, CStruct *p_struct, CScript *p_script);

static int s_get_icon_k_required(uint32 fileType);
static int s_calculate_total_space_used_on_card(uint32 fileType, int fileSize);
static int s_get_platforms_block_size();
static int s_round_up_to_platforms_block_size(int fileSize);
static int s_get_version_number(uint32 fileType);
static const char *s_generate_xbox_directory_name(uint32 fileType, const char *p_name);
static void s_generate_card_directory_name(uint32 fileType, const char *p_name, char *p_card_directory_name);
static bool s_make_xbox_dir_and_icons(	Mc::Card *p_card,
										uint32 fileType, const char *p_name, 
										char *p_card_file_name,
										bool *p_insufficientSpace);
static bool s_make_ps2_dir_and_icons(	Mc::Card *p_card,
										uint32 fileType, const char *p_name, 
										char *p_card_file_name,
										bool *p_insufficientSpace);
static bool s_insert_ngc_icon(	SMcFileHeader *p_fileHeader, 
								Mc::Card *p_card,
								Mc::File *p_file,
								uint32 fileType,
								const char *p_name);
static uint32 sGetFixedFileSize(uint32 fileType);



// When transferring data between the online vault and the mem card, this structure temporarily holds
// the data. The LoadFromMemoryCard and SaveToMemoryCard commands can be made to load to/from this structure
// instead, so that data can be transferred without loading it into the game.
static Script::CStruct *spVaultData=NULL;
static uint32 sVaultDataType=0;

// Steve's code calls this after downloading the binary data from the vault.
void SetVaultData(uint8 *p_data, uint32 type)
{
	if (spVaultData)
	{
		delete spVaultData;
		spVaultData=NULL;
	}
	
	spVaultData=new Script::CStruct;
	Script::ReadFromBuffer(spVaultData,p_data);
	sVaultDataType=type;
}

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This calculates an 8 character ascii checksum of the passed string.
// Used for generating the filename of the memory card file from the name the player has chosen.

// It works by calculating a 32 bit checksum the usual way, then
// converting this number to base 26 and using 'a' to represent 0, 'b' for 1, etc.
// Log to base 26 of 2^32 is 6.8, which means at most 7 ascii characters will be needed, so the 
// 8th letter in the string will actually always be a. So the last letter is used to indicate the file type.
static const char *s_generate_ascii_checksum(char *p_dest, const char *p_string, uint32 fileType)
{
	Dbg_MsgAssert(p_dest,("NULL p_dest"));
	
	uint32 Checksum=Script::GenerateCRC(p_string);
	for (int i=0; i<8; ++i)
	{
		int Rem=Checksum%26;
		p_dest[i]='a'+Rem;
		Checksum=(Checksum-Rem)/26;
	}	
	// Check that mathematics is still working ok
	Dbg_MsgAssert(Checksum==0,("Checksum not zero ???"));
	Dbg_MsgAssert(p_dest[7]=='a',("Last letter of ascii checksum not 'a' ???"));

	switch (fileType)
	{
		case 0xb010f357: // OptionsAndPros
			p_dest[7]='a'+MEMCARD_FILETYPE_CAREER;
			break;
		case 0xffc529f4: // Cas
			Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));
			//p_dest[7]='a'+MEMCARD_FILETYPE_CAS;
			break;
        case 0x61a1bc57: // CAT
			p_dest[7]='a'+MEMCARD_FILETYPE_CAT;
			break;
		case 0x3bf882cc: // Park
			p_dest[7]='a'+MEMCARD_FILETYPE_PARK;
			break;
		case 0x26c80b0d: // Replay
			p_dest[7]='a'+MEMCARD_FILETYPE_REPLAY;
			break;
		case 0xca41692d: // NetworkSettings
			p_dest[7]='a'+MEMCARD_FILETYPE_NETWORKSETTINGS;
			break;
		case 0x62896edf: // CreatedGoals
			p_dest[7]='a'+MEMCARD_FILETYPE_CREATEDGOALS;
			break;
		default:
			break;
	}
	
	p_dest[8]=0;
	return p_dest;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static uint32 s_determine_file_type(char c)
{
	switch (c-'a')
	{
	case MEMCARD_FILETYPE_CAREER:
		return 0xb010f357; // OptionsAndPros
		break;
	case MEMCARD_FILETYPE_CAS:
		return 0xffc529f4; // Cas
		break;
    case MEMCARD_FILETYPE_CAT:
		return 0x61a1bc57; // CAT
		break;
	case MEMCARD_FILETYPE_PARK:
		return 0x3bf882cc; // Park
		break;
	case MEMCARD_FILETYPE_REPLAY:	
		return 0x26c80b0d; // Replay
		break;
	case MEMCARD_FILETYPE_NETWORKSETTINGS:	
		return 0xca41692d; // NetworkSettings
		break;
	case MEMCARD_FILETYPE_CREATEDGOALS:	
		return 0x62896edf; // CreatedGoals
		break;
	default:
		return 0;
		break;
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


#ifndef __PLAT_NGC__
static unsigned short s_ascii_to_sjis(unsigned char ascii_code)
{
	if (Config::PAL())
	{
		// The PS2 shell will not display underscores (it shows them as spaces)
		// so convert them to hyphens.
		if (ascii_code=='_')
		{
			ascii_code='-';
		}	
	}	

	switch ((char)ascii_code)
	{
		case 'ß': ascii_code='B'; break;
		case 'Ä': ascii_code='A'; break;
		case 'Ü': ascii_code='U'; break;
		case 'Ö': ascii_code='O'; break;
		case 'à': ascii_code='a'; break;
		case 'â': ascii_code='a'; break;
		case 'ä': ascii_code='a'; break;
		case 'ê': ascii_code='e'; break;
		case 'è': ascii_code='e'; break;
		case 'é': ascii_code='e'; break;
		case 'ë': ascii_code='e'; break;
		case 'ì': ascii_code='i'; break;
		case 'î': ascii_code='i'; break;
		case 'ï': ascii_code='i'; break;
		case 'ô': ascii_code='o'; break;
		case 'ò': ascii_code='o'; break;
		case 'ö': ascii_code='o'; break;
		case 'ù': ascii_code='u'; break;
		case 'û': ascii_code='u'; break;
		case 'ü': ascii_code='u'; break;
		case 'ç': ascii_code='c'; break;
		case 'œ': ascii_code='o'; break;
		default: break;
	}
	
	unsigned short sjis_code = 0;
	unsigned char stmp=0;
	unsigned char stmp2 = 0;

	if((ascii_code >= 0x20) && (ascii_code <= 0x2f))
		stmp2 = 1;
	
	else if((ascii_code >= 0x30) && (ascii_code <= 0x39))
		stmp = 0;
	
	else if((ascii_code >= 0x3a) && (ascii_code <= 0x40))
		stmp2 = 11;
	
	else if((ascii_code >= 0x41) && (ascii_code <= 0x5a))
		stmp = 1;
	
	else if((ascii_code >= 0x5b) && (ascii_code <= 0x60))
		stmp2 = 37;
	
	else if((ascii_code >= 0x61) && (ascii_code <= 0x7a))
		stmp = 2;
	
	else if((ascii_code >= 0x7b) && (ascii_code <= 0x7e))
		stmp2 = 63;
	
	else 
	{
		printf("bad ASCII code 0x%x\n", ascii_code);
		return 0;
	}

	if (stmp2)
	   	sjis_code = s_ascii_special[ascii_code - 0x20 - (stmp2 - 1)][0];
	else
		sjis_code = s_ascii_table[stmp][0] + ascii_code - s_ascii_table[stmp][1];

	return sjis_code;
}
#endif		// __PLAT_NGC__

static void s_insert_global_info(CStruct *p_struct)
{
	// Build the global options structure and insert it into p_struct.
	
	Script::CStruct *pOptions=new Script::CStruct;
	
	// Attach the split screen preferences.
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Prefs::Preferences *pPreferences = pSkate->GetSplitScreenPreferences();
	Dbg_MsgAssert(pPreferences,("NULL split screen pPreferences"));
	
	// Create a new structure, append the data contained in the preferences, then insert the pointer to the
	// new structure into pOptions.
	Script::CStruct *pTemp=new Script::CStruct;
	pTemp->AppendStructure(pPreferences->GetRoot());
	pOptions->AddStructurePointer(CRCD(0xf7720c3f,"SplitScreenPreferences"),pTemp);

	// Get the sound options and stick them in a structure and add that.
	pTemp=new Script::CStruct;
	
	Sfx::CSfxManager * pSfxManager = Sfx::CSfxManager::Instance();
	float MainVolume=pSfxManager->GetMainVolume();
	pTemp->AddFloat(CRCD(0x6f016dfb,"MainVolume"),MainVolume);

	float MusicVolume=Pcm::GetVolume();
	pTemp->AddFloat(CRCD(0xabd4a575,"MusicVolume"),MusicVolume);
	
	//uint64 PlayListForbiddenTrackFlags=Pcm::GetPlaylist();
    uint64 list1,list2;
    Pcm::GetPlaylist(&list1, &list2);

	uint32 PlayListForbiddenTrackFlags1=(uint32)(list1>>32);
	uint32 PlayListForbiddenTrackFlags2=(uint32)list1;

    uint32 PlayListForbiddenTrackFlags3=(uint32)(list2>>32);
	uint32 PlayListForbiddenTrackFlags4=(uint32)list2;
	
	pTemp->AddInteger(CRCD(0x595d2c95,"PlayListForbiddenTrackFlags1"),PlayListForbiddenTrackFlags1);
	pTemp->AddInteger(CRCD(0xc0547d2f,"PlayListForbiddenTrackFlags2"),PlayListForbiddenTrackFlags2);
    pTemp->AddInteger(CRCD(0xb7534db9,"PlayListForbiddenTrackFlags3"),PlayListForbiddenTrackFlags3);
	pTemp->AddInteger(CRCD(0x2937d81a,"PlayListForbiddenTrackFlags4"),PlayListForbiddenTrackFlags4);

	// current_soundtrack only applies to XBox
	if (Config::GetHardware()==Config::HARDWARE_XBOX)
	{
		pTemp->AddChecksum(CRCD(0xe1f7c4ae,"current_soundtrack"),Script::GetChecksum("current_soundtrack"));
	}	
	
	if (Pcm::GetRandomMode())
	{
		pTemp->AddChecksum(NONAME,CRCD(0x31c71b70,"RandomMode"));
	}	
	
	pOptions->AddStructurePointer(CRCD(0x89eb9738,"SoundOptions"),pTemp);

	// Add the controller preferences.
	Script::CArray *pControllerPrefs=new Script::CArray;
	pControllerPrefs->SetSizeAndType(Mdl::Skate::vMAX_SKATERS, ESYMBOLTYPE_STRUCTURE);
	for (int i=0; i<Mdl::Skate::vMAX_SKATERS; ++i)
	{
		pTemp=new Script::CStruct;
		if (pSkate->mp_controller_preferences[i].AutoKickOn)
		{
			pTemp->AddChecksum(NONAME,CRCD(0x1eef7085,"AutoKickOn"));
		}	
		if (pSkate->mp_controller_preferences[i].SpinTapsOn)
		{
			pTemp->AddChecksum(NONAME,CRCD(0xa483ba67,"SpinTapsOn"));
		}	
		if (pSkate->mp_controller_preferences[i].VibrationOn)
		{
			pTemp->AddChecksum(NONAME,CRCD(0x73cee124,"VibrationOn"));
		}	
		pControllerPrefs->SetStructure(i,pTemp);
	}			
	
	pOptions->AddArrayPointer(CRCD(0x37bdb853,"ControllerPreferences"),pControllerPrefs);

	// Insert the taunt stuff
	GameNet::Manager * pGamenet = GameNet::Manager::Instance();
	pPreferences=pGamenet->GetTauntPreferences();
	Dbg_MsgAssert(pPreferences,("NULL taunt pPreferences"));
	pOptions->AddStructure(CRCD(0xe62b6586,"Taunts"),pPreferences->GetRoot());

	// Now insert the pointer to the newly constucted pOptions into the mem card structure.
	p_struct->AddStructurePointer(CRCD(0x2fca0578,"Options"),pOptions);

	// save the career (Game progress, flags and gap checklist)			
	Mdl::Skate::Instance()->GetCareer()->WriteIntoStructure(p_struct);

	// Add the game records.		  
	Records::CGameRecords *pGameRecords=pSkate->GetGameRecords();
	Dbg_MsgAssert(pGameRecords,("NULL pGameRecords"));
	pGameRecords->WriteIntoStructure(p_struct);
	
	// Wack in the pro skater profile info
	Script::CStruct *p_pros=new Script::CStruct;
	Obj::CPlayerProfileManager*	pPlayerProfileManager=pSkate->GetPlayerProfileManager();
	Dbg_MsgAssert(pPlayerProfileManager,("NULL pPlayerProfileManager"));
	pPlayerProfileManager->AddAllProProfileInfo(p_pros);
	p_struct->AddStructurePointer(CRCD(0xc9986baf,"Pros"),p_pros);
	
	// Note: Mustn't delete pOptions or the other structures created above
	// since pointers to these have been given to p_struct.
	// p_struct will clean them up when it gets deleted.
}

static void s_insert_story_info(CStruct *p_struct)
{
	// Save the goal manager parameters.
	Game::CGoalManager* p_goal_manager=Game::GetGoalManager();
	Dbg_MsgAssert(p_goal_manager,("NULL p_goal_manager"));
	
	CStruct *p_goal_manager_params=p_goal_manager->GetGoalManagerParams();
	Dbg_MsgAssert(p_goal_manager_params,("NULL p_goal_manager_params"));
	p_struct->AddStructure(CRCD(0xac7c2b62,"GoalManagerParams"),p_goal_manager_params);
}

static void s_insert_story_skater_info(CStruct *p_struct)
{
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkater* pSkater = pSkate->GetSkater(0);
	Dbg_Assert( pSkater );
	pSkater->AddCATInfo(p_struct);
	
    // stat goal status
    pSkater->AddStatGoalInfo(p_struct);
    
    // chapter status
    pSkater->AddChapterStatusInfo(p_struct);
}

static void s_insert_custom_skater_info(CStruct *p_struct)
{
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CPlayerProfileManager*	pPlayerProfileManager=pSkate->GetPlayerProfileManager();
	Dbg_MsgAssert(pPlayerProfileManager,("NULL pPlayerProfileManager"));
	
	pPlayerProfileManager->AddCASProfileInfo(p_struct);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void s_insert_game_save_info(uint32 fileType, CStruct *p_struct)
{
	// WARNING !		WARNING !		WARNING !		WARNING !		WARNING !
	// Make sure that no function in here stores any pointers to new CStructs.
	// This is because all new CStructs (and CComponents) allocated here will be coming
	// off a special pool, to avoid overflowing the regular pool.
	
	Dbg_MsgAssert(p_struct,("NULL p_struct"));

	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	#ifdef	__NOPT_ASSERT__
	#if 0 		// Actually remove it fully, as the code that uses it had been temp stubbed out,a nd we don't want to confuse the non final build into thinking it's there
	Obj::CPlayerProfileManager*	pPlayerProfileManager=pSkate->GetPlayerProfileManager();
	Dbg_MsgAssert(pPlayerProfileManager,("NULL pPlayerProfileManager"));
	#endif
	#endif
	
	switch (fileType)
	{
		case 0xb010f357: // OptionsAndPros
		{
			Script::CStruct *p_global_info=new Script::CStruct;
			s_insert_global_info(p_global_info);
			p_struct->AddStructurePointer(CRCD(0xf55cbd13,"GlobalInfo"),p_global_info);
			
			Script::CStruct *p_story=new Script::CStruct;
			s_insert_story_info(p_story);
			p_struct->AddStructurePointer(CRCD(0x14a9fbc7,"Story"),p_story);
			
			Script::CStruct *p_story_skater=new Script::CStruct;
			s_insert_story_skater_info(p_story_skater);
			p_struct->AddStructurePointer(CRCD(0xdf2f448,"StorySkater"),p_story_skater);
			
			Script::CStruct *p_custom_skater=new Script::CStruct;
			s_insert_custom_skater_info(p_custom_skater);
			p_struct->AddStructurePointer(CRCD(0x12bfac82,"CustomSkater"),p_custom_skater);
			break;
		}

		case 0xca41692d: // NetworkSettings		
		{
			// Add the network preferences
			GameNet::Manager * pGamenet = GameNet::Manager::Instance();
			Prefs::Preferences *pPreferences=pGamenet->GetNetworkPreferences();
			p_struct->AppendStructure(pPreferences->GetRoot());
			break;
		}	
		
		case 0xffc529f4: // Cas
		{
			Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));
			/*		
			pPlayerProfileManager->AddCASProfileInfo(p_struct);
            
            Obj::CSkater* pSkater = pSkate->GetSkater(0);
            Dbg_Assert( pSkater );
            pSkater->AddCATInfo(p_struct);
			*/
			break;
		}

        case 0x61a1bc57: // Cat
		{
			// Can only edit CAT on skater zero!
            Obj::CSkater* pSkater = pSkate->GetSkater(0);
            Dbg_Assert( pSkater );
            
            // Index is always 0 since that is the only one that can be edited directly.
            Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[0];
            Dbg_Assert( pCreatedTrick );

            //Other params
            p_struct->AddStructure( "other_params", pCreatedTrick->mp_other_params );
            
            //Rotation params
            p_struct->AddArray( "rotation_info", pCreatedTrick->mp_rotations );
            
            //Animation params
            p_struct->AddArray( "animation_info", pCreatedTrick->mp_animations );
        
            break;
		}
		
		case 0x3bf882cc: // Park	 (Save)
		{
			Ed::CParkManager::Instance()->WriteIntoStructure(p_struct);
			break;
		}
		
		case 0x26c80b0d: // Replay
		{
#if __USE_REPLAYS__
			Replay::AddReplayMemCardInfo(p_struct);
#endif			
			break;
		}

		case 0x62896edf: // CreatedGoals
		{
			Obj::CCompositeObject *p_obj=(Obj::CCompositeObject*)Obj::CTracker::Instance()->GetObject(CRCD(0x81f01058,"GoalEditor"));
			Dbg_MsgAssert(p_obj,("No GoalEditor object"));
			Obj::CGoalEditorComponent *p_goal_editor=GetGoalEditorComponentFromObject(p_obj);
			Dbg_MsgAssert(p_goal_editor,("No goal editor component ???"));
			p_goal_editor->WriteIntoStructure(p_struct);
			break;
		}
		
		default:
		{
			Dbg_MsgAssert(0,("Bad type of '%s' sent to s_insert_game_save_info",FindChecksumName(fileType)));
			break;
		}		
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Inserts summary info for the specified file type into p_struct.
// Summary info is a small amount of info that is written at the start of the mem card file
// so that it can be quickly read out without having to read the whole file.
// The info gets printed at the bottom of the files menu when the highlight is on a file.
static void s_generate_summary_info(CStruct *p_summaryInfo, uint32 fileType, CStruct *p_mainData)
{
	// WARNING !		WARNING !		WARNING !		WARNING !		WARNING !
	// Make sure that no function in here stores any pointers to new CStructs.
	// This is because all new CStructs (and CComponents) allocated here will be coming
	// off a special pool, to avoid overflowing the regular pool.
	
	Dbg_MsgAssert(p_summaryInfo,("NULL p_summaryInfo"));

	if (p_mainData)
	{
		// Extract the summary info from the passed p_mainData.
		// This is for when the save process is being done on data that got downloaded from the vault.
		
		switch (fileType)
		{
			case 0xb010f357: // OptionsAndPros
			{
				Script::CStruct *p_story=NULL;
				p_mainData->GetStructure(CRCD(0x14a9fbc7,"Story"),&p_story,Script::ASSERT);
				
				Script::CStruct *p_goal_manager_params=NULL;
				p_story->GetStructure(CRCD(0xac7c2b62,"GoalManagerParams"),&p_goal_manager_params,Script::ASSERT);

				CStruct *p_more_goal_manager_params=NULL;
				p_goal_manager_params->GetStructure(CRCD(0x23d4170a,"GoalManager_Params"),&p_more_goal_manager_params,Script::ASSERT);
				
				int chapter=0;
				p_more_goal_manager_params->GetInteger(CRCD(0xf884773c,"CurrentChapter"),&chapter);
				p_summaryInfo->AddInteger(CRCD(0xf884773c,"CurrentChapter"),chapter);
				
				// is_male is used by the script upload_content in net.q
				Script::CStruct *p_custom_skater=NULL;
				p_mainData->GetStructure(CRCD(0x12bfac82,"CustomSkater"),&p_custom_skater,Script::ASSERT);
				Script::CStruct *p_custom=NULL;
				p_custom_skater->GetStructure(CRCD(0xa7be964,"Custom"),&p_custom,Script::ASSERT);
				Script::CStruct *p_info=NULL;
				p_custom->GetStructure(CRCD(0x3476cea8,"Info"),&p_info,Script::ASSERT);
                int is_male=0;
				p_info->GetInteger(CRCD(0x3f813177,"is_male"),&is_male);
				p_summaryInfo->AddInteger(CRCD(0x3f813177,"is_male"),is_male);
                const char *p_name="";
                p_info->GetString(CRCD(0x2ab66cb8,"display_name"),&p_name);
				p_summaryInfo->AddString(CRCD(0xa1dc81f9,"name"),p_name);
				break;
			}	
			
			case 0xca41692d: // NetworkSettings		
			{
				p_summaryInfo->AddString("network_id","");
				break;
			}			
			
			case 0xffc529f4: // Cas
			{
				Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));
				break;
			}
			
            case 0x61a1bc57: // Cat
            {
                Script::CStruct *p_data=NULL;
				p_mainData->GetStructure(CRCD(0x4137cb14,"other_params"),&p_data,Script::ASSERT);
                
                const char *p_name="";
				p_data->GetString(CRCD(0xa1dc81f9,"name"),&p_name,Script::ASSERT);

                p_summaryInfo->AddString(CRCD(0xa1dc81f9,"name"),p_name);
                break;
            }

			case 0x26c80b0d: // Replay
			{
				break;
			}
	
			case 0x3bf882cc: // Park
			{
				int num_edited_goals=0;
				
				Script::CArray *p_goals=NULL;
				Script::CStruct *p_goals_struct=NULL;
				p_mainData->GetStructure(CRCD(0xd8eb825e,"Park_editor_goals"),&p_goals_struct);
				if (p_goals_struct)
				{
					p_goals_struct->GetArray(CRCD(0x38dbe1d0,"Goals"),&p_goals);
					if (p_goals)
					{
						num_edited_goals=p_goals->GetSize();
					}
				}		
				p_summaryInfo->AddInteger(CRCD(0xe1ec606f,"num_edited_goals"),num_edited_goals);
				
				int max_players=1;
				p_mainData->GetInteger(CRCD(0xb7e39b53,"MaxPlayers"),&max_players);
				p_summaryInfo->AddInteger(CRCD(0xb7e39b53,"MaxPlayers"),max_players);
				
				Script::CArray *p_map=NULL;
				p_mainData->GetArray(CRCD(0x337c5289,"Park_editor_map"),&p_map,Script::ASSERT);
				
				// Used by the script upload_content in net.q
				int num_gaps=0;
				int num_metas=0;
				uint16 theme=0;
				uint32 tod_script=0;
				int width=0;
				int length=0;
				Ed::CParkManager::Instance()->GetSummaryInfoFromBuffer((uint8*)p_map->GetArrayPointer(),&num_gaps,&num_metas,&theme,&tod_script,&width,&length);
				p_summaryInfo->AddInteger(CRCD(0xe6121ed0,"num_gaps"),num_gaps);
				p_summaryInfo->AddInteger(CRCD(0xfff3dc35,"num_pieces"),num_metas);
				p_summaryInfo->AddInteger(CRCD(0x688a18f7,"theme"),theme);
				p_summaryInfo->AddChecksum(CRCD(0x4c72ed98,"tod_script"),tod_script);
				p_summaryInfo->AddInteger(CRCD(0x73e5bad0,"width"),width);
				p_summaryInfo->AddInteger(CRCD(0xfe82614d,"length"),length);
				break;
			}
				
			case 0x62896edf: // CreatedGoals
			{
				int num_edited_goals=0;
				
				Script::CArray *p_goals=NULL;
				p_mainData->GetArray(CRCD(0x38dbe1d0,"Goals"),&p_goals);
				if (p_goals)
				{
					num_edited_goals=p_goals->GetSize();
				}	
				p_summaryInfo->AddInteger(CRCD(0xe1ec606f,"num_edited_goals"),num_edited_goals);
				break;
			}
				
			default:
			{
				Dbg_MsgAssert(0,("Bad type of '%s' sent to s_generate_summary_info",FindChecksumName(fileType)));
				break;
			}		
		}		
	}
	else
	{
		// Get the summary info from the game.
		
		switch (fileType)
		{
			case 0xb010f357: // OptionsAndPros
			{
				Game::CGoalManager* p_goal_manager=Game::GetGoalManager();
				Dbg_MsgAssert(p_goal_manager,("NULL p_goal_manager"));
				
				CStruct *p_goal_manager_params=p_goal_manager->GetGoalManagerParams();
				Dbg_MsgAssert(p_goal_manager_params,("NULL p_goal_manager_params"));
	
				// Grab the summary info.
				CStruct *p_more_goal_manager_params=NULL;
				p_goal_manager_params->GetStructure(CRCD(0x23d4170a,"GoalManager_Params"),&p_more_goal_manager_params);
				Dbg_MsgAssert(p_more_goal_manager_params,("No GoalManager_Params structure ??"));
				
				int chapter=0;
				p_more_goal_manager_params->GetInteger(CRCD(0xf884773c,"CurrentChapter"),&chapter);
				p_summaryInfo->AddInteger(CRCD(0xf884773c,"CurrentChapter"),chapter);
				break;
			}	
			
			case 0xca41692d: // NetworkSettings		
			{
				GameNet::Manager * pGamenet = GameNet::Manager::Instance();
				Prefs::Preferences *pPreferences=pGamenet->GetNetworkPreferences();
				CStruct *p_net_stuff=pPreferences->GetRoot();
				
				// Grab the summary info.
				CStruct *p_foo=NULL;
				p_net_stuff->GetStructure("network_id",&p_foo);
				Dbg_MsgAssert(p_foo,("No network_id structure ?"));
				
				const char *p_name="";
				p_foo->GetString("ui_string",&p_name);
				
				p_summaryInfo->AddString("network_id",p_name);
				break;
			}			
			
			case 0xffc529f4: // Cas
			{
				Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));			
				break;
			}
	
			case 0x61a1bc57: // Cat
			{
				// Can only edit CAT on skater zero!
                Mdl::Skate * pSkate = Mdl::Skate::Instance();
                Obj::CSkater* pSkater = pSkate->GetSkater(0);
                Dbg_Assert( pSkater );
                
                // Index is always 0 since that is the only one that can be edited directly.
                Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[0];
                Dbg_Assert( pCreatedTrick );
    
                const char *p_name="";
				pCreatedTrick->mp_other_params->GetString(CRCD(0xa1dc81f9,"name"),&p_name,Script::ASSERT);

                p_summaryInfo->AddString(CRCD(0xa1dc81f9,"name"),p_name);
                break;
			}
			
			case 0x26c80b0d: // Replay
			{
	#if __USE_REPLAYS__
				Replay::AddReplayMemCardSummaryInfo(p_summaryInfo);
	#endif			
				break;
			}
			
			case 0x3bf882cc: // Park
			case 0x62896edf: // CreatedGoals
			{
				Obj::CCompositeObject *p_obj=(Obj::CCompositeObject*)Obj::CTracker::Instance()->GetObject(CRCD(0x81f01058,"GoalEditor"));
				Dbg_MsgAssert(p_obj,("No GoalEditor object"));
				Obj::CGoalEditorComponent *p_goal_editor=GetGoalEditorComponentFromObject(p_obj);
				Dbg_MsgAssert(p_goal_editor,("No goal editor component ???"));
				
				p_summaryInfo->AddInteger(CRCD(0xe1ec606f,"num_edited_goals"),p_goal_editor->GetNumGoals());
				
				if (fileType==CRCD(0x3bf882cc,"Park"))
				{
					p_summaryInfo->AddInteger(CRCD(0xb7e39b53,"MaxPlayers"),Ed::CParkManager::Instance()->GetGenerator()->GetMaxPlayers());
					
					int num_gaps=0;
					int num_metas=0;
					uint16 theme=0;
					uint32 tod_script=0;
					int width=0;
					int length=0;
					Ed::CParkManager::Instance()->WriteCompressedMapBuffer();		// Ensure map buffer is correct
					Ed::CParkManager::Instance()->GetSummaryInfoFromBuffer(Ed::CParkManager::Instance()->GetCompressedMapBuffer(),&num_gaps,&num_metas,&theme,&tod_script,&width,&length);
					p_summaryInfo->AddInteger(CRCD(0xe6121ed0,"num_gaps"),num_gaps);
					p_summaryInfo->AddInteger(CRCD(0xfff3dc35,"num_pieces"),num_metas);
					p_summaryInfo->AddInteger(CRCD(0x688a18f7,"theme"),theme);
					p_summaryInfo->AddChecksum(CRCD(0x4c72ed98,"tod_script"),tod_script);
					p_summaryInfo->AddInteger(CRCD(0x73e5bad0,"width"),width);
					p_summaryInfo->AddInteger(CRCD(0xfe82614d,"length"),length);
				}	
				break;
			}
				
			default:
			{
				Dbg_MsgAssert(0,("Bad type of '%s' sent to s_generate_summary_info",FindChecksumName(fileType)));
				break;
			}		
		}		
	}	
}

static void s_read_global_info(CStruct *p_globalInfo, CScript *p_script)
{
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	
	Script::CStruct *pOptions=NULL;
	p_globalInfo->GetStructure(CRCD(0x2fca0578,"Options"),&pOptions);
	Dbg_MsgAssert(pOptions,("p_globalInfo is missing Options structure"));
	
	// Extract the split screen preferences
	Script::CStruct *pTemp=NULL;
	pOptions->GetStructure(CRCD(0xf7720c3f,"SplitScreenPreferences"),&pTemp);
	
	Prefs::Preferences *pPreferences = pSkate->GetSplitScreenPreferences();
	Dbg_MsgAssert(pPreferences,("NULL split screen pPreferences"));
	pPreferences->SetRoot(pTemp);

	// Get the taunt preferences
	pTemp=NULL;
	pOptions->GetStructure(CRCD(0xe62b6586,"Taunts"),&pTemp);
	
	GameNet::Manager * pGamenet = GameNet::Manager::Instance();
	pPreferences=pGamenet->GetTauntPreferences();
	Dbg_MsgAssert(pPreferences,("NULL taunt pPreferences"));
	pPreferences->SetRoot(pTemp);

	
	// Get the sound options.
	pOptions->GetStructure(CRCD(0x89eb9738,"SoundOptions"),&pTemp);
	Sfx::CSfxManager * pSfxManager = Sfx::CSfxManager::Instance();
	float MainVolume=0;
	if (pTemp->GetFloat(CRCD(0x6f016dfb,"MainVolume"),&MainVolume))
	{
		pSfxManager->SetMainVolume(MainVolume);
	}	

	float MusicVolume=0;
	if (pTemp->GetFloat(CRCD(0xabd4a575,"MusicVolume"),&MusicVolume))
	{
		Pcm::SetVolume(MusicVolume);
	}	

	uint64 PlayListForbiddenTrackFlagsA=0;
    uint64 PlayListForbiddenTrackFlagsB=0;
	uint32 PlayListForbiddenTrackFlags1=0;
	uint32 PlayListForbiddenTrackFlags2=0;
    uint32 PlayListForbiddenTrackFlags3=0;
	uint32 PlayListForbiddenTrackFlags4=0;

	if (pTemp->GetInteger(CRCD(0x595d2c95,"PlayListForbiddenTrackFlags1"),(int*)&PlayListForbiddenTrackFlags1))
	{
		pTemp->GetInteger(CRCD(0xc0547d2f,"PlayListForbiddenTrackFlags2"),(int*)&PlayListForbiddenTrackFlags2);
        pTemp->GetInteger(CRCD(0xb7534db9,"PlayListForbiddenTrackFlags3"),(int*)&PlayListForbiddenTrackFlags3);
        pTemp->GetInteger(CRCD(0x2937d81a,"PlayListForbiddenTrackFlags4"),(int*)&PlayListForbiddenTrackFlags4);
		
		PlayListForbiddenTrackFlagsA=(((uint64)PlayListForbiddenTrackFlags1)<<32)+PlayListForbiddenTrackFlags2;
        PlayListForbiddenTrackFlagsB=(((uint64)PlayListForbiddenTrackFlags3)<<32)+PlayListForbiddenTrackFlags4;
		Pcm::SetPlaylist(PlayListForbiddenTrackFlagsA, PlayListForbiddenTrackFlagsB);
	}	

	if (pTemp->ContainsFlag(CRCD(0x31c71b70,"RandomMode")))
	{
		Pcm::SetRandomMode(true);
	}
	else
	{
		Pcm::SetRandomMode(false);
	}

	if (Config::GetHardware()==Config::HARDWARE_XBOX)
	{
		uint32 current_soundtrack=0xffffffff;
		pTemp->GetChecksum(CRCD(0xe1f7c4ae,"current_soundtrack"),&current_soundtrack);
		
		Script::CSymbolTableEntry *p_sym=Script::LookUpSymbol(0xe1f7c4ae/*current_soundtrack*/);
		if (p_sym)
		{
			Dbg_MsgAssert(p_sym->mType==ESYMBOLTYPE_NAME,("Expected current_soundtrack to have type checksum"));
			p_sym->mChecksum=current_soundtrack;
		}	
		Script::CStruct* pTemp2 = new Script::CStruct;
        pTemp2->AddChecksum(CRCD(0x8c465905,"trackchecksum"),current_soundtrack);
        Script::RunScript(CRCD(0xd44400c2,"set_loaded_soundtrack"), pTemp2);
	}	

	// Extract the controller preferences
	Script::CArray *pControllerPrefs=NULL;
	if (pOptions->GetArray(CRCD(0x37bdb853,"ControllerPreferences"),&pControllerPrefs))
	{
		for (int i=0; i<Mdl::Skate::vMAX_SKATERS; ++i)
		{
			Script::CStruct *pPrefs=pControllerPrefs->GetStructure(i);
			Dbg_MsgAssert(pPrefs,("NULL pPrefs?"));

			pSkate->SetVibration(i,pPrefs->ContainsFlag(CRCD(0x73cee124,"VibrationOn")));
			pSkate->SetAutoKick(i,pPrefs->ContainsFlag(CRCD(0x1eef7085,"AutoKickOn")));
			pSkate->SetSpinTaps(i,pPrefs->ContainsFlag(CRCD(0xa483ba67,"SpinTapsOn")));
		}	
	}
	
	// Extract the game records.		  
	Records::CGameRecords *pGameRecords=pSkate->GetGameRecords();
	Dbg_MsgAssert(pGameRecords,("NULL pGameRecords"));
	pGameRecords->ReadFromStructure(p_globalInfo);
	
	// Extract the pro & cas skater profile info
	Script::CStruct *p_pros=NULL;
	p_globalInfo->GetStructure(CRCD(0xc9986baf,"Pros"),&p_pros,Script::ASSERT);
	Obj::CPlayerProfileManager*	pPlayerProfileManager=pSkate->GetPlayerProfileManager();
	pPlayerProfileManager->LoadAllProProfileInfo(p_pros);

	// Extract the career info (Game progress, flags and gap checklist)			
	Mdl::Skate::Instance()->GetCareer()->ReadFromStructure(p_globalInfo);
	
	// Return the LastLevelLoadScript and LastGameMode to the calling script.
	// Needed by Zac so that after autoloading the game can automatically load up the
	// last level that was being played when saved.
	uint32 last_level_load_script=0;
	uint32 last_game_mode=0;
	CStruct *p_career=NULL;
	int current_theme=0;
	p_globalInfo->GetStructure(CRCD(0x4da4937b,"Career"),&p_career);
	if (p_career)
	{
		p_career->GetChecksum(CRCD(0xe3335d2f,"LastLevelLoadScript"),&last_level_load_script);
		p_career->GetChecksum(CRCD(0x2cc06f5e,"LastGameMode"),&last_game_mode);
		// Extract current theme
		p_career->GetInteger(CRCD(0xcc946ff3,"current_theme"),&current_theme);
	}
	if (last_level_load_script)
	{
		p_script->GetParams()->AddChecksum(CRCD(0xe3335d2f,"LastLevelLoadScript"),last_level_load_script);
	}
	if (last_game_mode)
	{
		p_script->GetParams()->AddChecksum(CRCD(0x2cc06f5e,"LastGameMode"),last_game_mode);
	}
	//if (current_theme) TT7448: current_theme can be saved as zero
	//{
		Script::CStruct* pTemp2 = new Script::CStruct;
		pTemp2->AddInteger(CRCD(0x3bed2cf5,"theme_num"),current_theme);
		pTemp2->AddChecksum(CRCD(0x476cdadd,"loading_career"),0);
		Script::RunScript( "set_current_theme", pTemp2 );
		delete pTemp2;
   // }
	
	// Now, if the skater is custom, set his cas file name.
	Obj::CSkaterProfile* pSkaterProfile=pSkate->GetCurrentProfile();
	if (!pSkaterProfile->IsPro())
	{
		const char *pCASFileName="Unimplemented";
		p_globalInfo->GetString(CRCD(0xf36c1878,"CASFileName"),&pCASFileName);
		pSkaterProfile->SetCASFileName(pCASFileName);
	}
}
	
static void s_read_story_info(CStruct *p_storyInfo)
{
	// Load in the goal manager parameters.
	CStruct *p_loaded_goal_manager_params=NULL;
	p_storyInfo->GetStructure(CRCD(0xac7c2b62,"GoalManagerParams"),&p_loaded_goal_manager_params);
	Dbg_MsgAssert(p_loaded_goal_manager_params,("No goal manager params on mem card"));
	
	Game::CGoalManager* p_goal_manager=Game::GetGoalManager();
	Dbg_MsgAssert(p_goal_manager,("NULL p_goal_manager"));
	
	p_goal_manager->ResetCareer();
	
	CStruct *p_goal_manager_params=p_goal_manager->GetGoalManagerParams();
	Dbg_MsgAssert(p_goal_manager_params,("NULL p_goal_manager_params"));
	
	p_goal_manager_params->Clear();
	p_goal_manager_params->AppendStructure(p_loaded_goal_manager_params);

	// Refresh the goal manager with the new params.
	p_goal_manager->LevelLoad();
}

static void s_read_story_skater_info(CStruct *p_storySkaterInfo, CStruct *p_customSkater)
{
	// Which skater are we loading?
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	
	Obj::CPlayerProfileManager*	pPlayerProfileManager=pSkate->GetPlayerProfileManager();
	int index = pPlayerProfileManager->GetCurrentProfileIndex();
	Dbg_MsgAssert(index==0,("Should not be loading story skater info into any skater other than skater 0, tried to load it into skater %d",index));

	Obj::CSkater* pSkater = pSkate->GetSkater(index);
	Dbg_Assert( pSkater );
	pSkater->LoadCATInfo(p_storySkaterInfo);
    pSkater->LoadStatGoalInfo(p_storySkaterInfo);
    pSkater->LoadChapterStatusInfo(p_storySkaterInfo);
    
    pPlayerProfileManager->LoadCASProfileInfo(p_customSkater, true);
}

static void s_read_custom_skater_info(CStruct *p_customSkater)
{
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	
	Obj::CPlayerProfileManager*	pPlayerProfileManager=pSkate->GetPlayerProfileManager();
    int index = pPlayerProfileManager->GetCurrentProfileIndex();
    
    if ( index == 0 )
    {
        // if this is player one... don't apply the stats
        pPlayerProfileManager->LoadCASProfileInfo(p_customSkater, false);
    }
    else
    {
        // if this is any other skater, then apply the stats
        pPlayerProfileManager->LoadCASProfileInfo(p_customSkater, true);
    }
    
}

uint32 s_apply_flags=0;
bool s_did_apply_custom_skater_info=false;
static void s_read_game_save_info(uint32 fileType, CStruct *p_struct, CScript *p_script)
{
	Dbg_MsgAssert(p_struct,("NULL p_struct"));
	Dbg_MsgAssert(p_script,("NULL p_script"));
	
	s_did_apply_custom_skater_info=false;
	
	switch (fileType)
	{
		case 0xb010f357: // OptionsAndPros
		{
			Dbg_MsgAssert(s_apply_flags,("s_apply_flags is zero, need to call SetSectionsToApplyWhenLoading"));

            if (s_apply_flags & (1<<APPLY_GLOBAL_INFO))
			{
                printf("APPLY_GLOBAL_INFO\n");
                Script::CStruct *p_global_info=NULL;
				p_struct->GetStructure(CRCD(0xf55cbd13,"GlobalInfo"),&p_global_info,Script::ASSERT);
				s_read_global_info(p_global_info, p_script);
			}
			
			if (s_apply_flags & (1<<APPLY_STORY))
			{
				// Must do this before the story skater is loaded
                // so that the proper difficulty is set
                // when the stat goals get loaded
                printf("APPLY_STORY\n");
                Script::CStruct *p_story_info=NULL;
				p_struct->GetStructure(CRCD(0x14a9fbc7,"Story"),&p_story_info,Script::ASSERT);
				s_read_story_info(p_story_info);
			}
			
            if (s_apply_flags & (1<<APPLY_CUSTOM_SKATER))
			{
				printf("APPLY_CUSTOM_SKATER\n");
                Script::CStruct *p_custom_skater_info=NULL;
				p_struct->GetStructure(CRCD(0x12bfac82,"CustomSkater"),&p_custom_skater_info,Script::ASSERT);
				s_read_custom_skater_info(p_custom_skater_info);
				s_did_apply_custom_skater_info=true;
			}

			if (s_apply_flags & (1<<APPLY_STORY_SKATER))
			{
				printf("APPLY_STORY_SKATER\n");
                Script::CStruct *p_story_skater_info=NULL;
				p_struct->GetStructure(CRCD(0xdf2f448,"StorySkater"),&p_story_skater_info,Script::ASSERT);
                Script::CStruct *p_custom_skater_info=NULL;
				p_struct->GetStructure(CRCD(0x12bfac82,"CustomSkater"),&p_custom_skater_info,Script::ASSERT);
				s_read_story_skater_info(p_story_skater_info,p_custom_skater_info);
			}
			
            // Reset the flags to that the above assert will catch cases where the scripts 
			// have not called SetSectionsToApplyWhenLoading	
			s_apply_flags=0;
			break;
		}
		
		case 0xca41692d: // NetworkSettings		
		{
			// Extract the network preferences
			GameNet::Manager * pGamenet = GameNet::Manager::Instance();
			Prefs::Preferences *pPreferences=pGamenet->GetNetworkPreferences();
			Dbg_MsgAssert(pPreferences,("NULL network pPreferences"));
			pPreferences->SetRoot(p_struct);
			break;
		}
		
		case 0xffc529f4: // Cas
		{
			Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));
			/*
			Mdl::Skate * pSkate = Mdl::Skate::Instance();
			Obj::CPlayerProfileManager*	pPlayerProfileManager=pSkate->GetPlayerProfileManager();
			//Script::PrintContents( p_struct );
			pPlayerProfileManager->LoadCASProfileInfo(p_struct);

            // Which skater are we loading?
            int index = pPlayerProfileManager->GetCurrentProfileIndex();

            Obj::CSkater* pSkater = pSkate->GetSkater(index);
            Dbg_Assert( pSkater );
            pSkater->LoadCATInfo(p_struct);
			*/
			break;
		}

        case 0x61a1bc57: // Cat
		{
			Mdl::Skate * pSkate = Mdl::Skate::Instance();

            Script::CStruct *p_other_params;
            Script::CArray *p_rotation_info;
            Script::CArray *p_animation_info;
            
            Obj::CSkater* pSkater = pSkate->GetLocalSkater();
            if ( pSkater )
            {
                //Params
                p_struct->GetStructure( "other_params", &p_other_params, Script::ASSERT );
                //pSkater->m_created_trick[0]->mp_other_params = p_other_params;
                pSkater->m_created_trick[0]->mp_other_params->AppendStructure( p_other_params );
                
                //Rotations
                p_struct->GetArray( "rotation_info", &p_rotation_info, Script::ASSERT );
                Script::CopyArray( pSkater->m_created_trick[0]->mp_rotations, p_rotation_info );
                
                //Animations
                p_struct->GetArray( "animation_info", &p_animation_info, Script::ASSERT );
                Script::CopyArray( pSkater->m_created_trick[0]->mp_animations, p_animation_info );
            }
			break;
		}
		
		case 0x3bf882cc: // Park  (read save game info)
		{
			Ed::CParkManager::Instance()->ReadFromStructure(p_struct);
			break;
		}
		
		case 0x26c80b0d: // Replay
		{
#if __USE_REPLAYS__
			uint32 level_structure_name=0;
			p_struct->GetChecksum("level_structure_name",&level_structure_name);
			Replay::SetLevelStructureName(level_structure_name);
			
			// Look up the load_script in the level structure, and return it to the calling
			// script so that after calling LoadFromMemoryCard the script can call the load_script,
			// which will load the appropriate level.
			CStruct *p_level_structure=GetStructure(level_structure_name);
			Dbg_MsgAssert(p_level_structure,("Could not find level structure '%s'",FindChecksumName(level_structure_name)));
			
			uint32 load_script=0;
			p_level_structure->GetChecksum("load_script",&load_script);
			p_script->GetParams()->AddChecksum("load_script",load_script);
#endif			
			break;
		}

		case 0x62896edf: // CreatedGoals
		{
			Obj::CCompositeObject *p_obj=(Obj::CCompositeObject*)Obj::CTracker::Instance()->GetObject(CRCD(0x81f01058,"GoalEditor"));
			Dbg_MsgAssert(p_obj,("No GoalEditor object"));
			Obj::CGoalEditorComponent *p_goal_editor=GetGoalEditorComponentFromObject(p_obj);
			Dbg_MsgAssert(p_goal_editor,("No goal editor component ???"));
			
			p_goal_editor->ReadFromStructure(p_struct);
			break;
		}
		
		default:
		{
			Dbg_MsgAssert(0,("Bad type of '%s' sent to s_read_game_save_info",FindChecksumName(fileType)));
			break;
		}		
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static int s_get_icon_k_required(uint32 fileType)
{
	switch (fileType)
	{
		case 0xb010f357: // OptionsAndPros
			return Script::GetInteger(CRCD(0x4925ed9e,"OptionsProsIconSpaceRequired"));
			break;
		case 0xca41692d: // NetworkSettings
			return Script::GetInteger(CRCD(0xf3431f63,"NetworkSettingsIconSpaceRequired"));
			break;
		case 0xffc529f4: // Cas
			Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));		
			//return Script::GetInteger("CASIconSpaceRequired");
			break;
        case 0x61a1bc57: // Cat
			return Script::GetInteger(CRCD(0x46b0ef40,"CATIconSpaceRequired"));
			break;
		case 0x3bf882cc: // Park
			return Script::GetInteger(CRCD(0x82aaf8dc,"ParkIconSpaceRequired"));
			break;
		case 0x26c80b0d: // Replay
			return Script::GetInteger(CRCD(0x7eaf934f,"ReplayIconSpaceRequired"));
			break;
		case 0x62896edf: // CreatedGoals
			return Script::GetInteger(CRCD(0x3205bc07,"CreatedGoalsIconSpaceRequired"));
			break;
		default:
			Dbg_MsgAssert(0,("Bad fileType of '%s' sent to s_get_icon_k_required",FindChecksumName(fileType)));
			break;
	}
	
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static int s_calculate_total_space_used_on_card(uint32 fileType, int fileSize)
{
	int blocks=s_round_up_to_platforms_block_size(fileSize)/s_get_platforms_block_size();

	switch (Config::GetHardware())
	{
	case Config::HARDWARE_PS2:
	case Config::HARDWARE_PS2_PROVIEW:
	case Config::HARDWARE_PS2_DEVSYSTEM:
	{
		// Add in the icon space required
		blocks+=s_get_icon_k_required(fileType);
		
		// Then add in the space used up by the mem card file system.
		blocks+=4;
		break;
	}
			
	case Config::HARDWARE_XBOX:
	{
		// Add in the icon space required
		blocks+=3;
		break;
	}
			
	case Config::HARDWARE_NGC:
		// All done, I think.
		break;
				
	default:
		break;
	}		
	
	return blocks;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This returns the block size for mem card files.
// On some platforms, using a library function to get the file size will not
// return the original file size, but will return the size rounded up to the next
// multiple of the block size.
// In order that the calculated checksum be the same on loading, the files are always
// padded with zeros to be a multiple of the block size before the checksum is calculated.
static int s_get_platforms_block_size()
{
	switch (Config::GetHardware())
	{
		case Config::HARDWARE_PS2:
		case Config::HARDWARE_PS2_PROVIEW:
		case Config::HARDWARE_PS2_DEVSYSTEM:
			return 1024;
			break;
					
		case Config::HARDWARE_NGC:
			return 8192;
			break;
			
		case Config::HARDWARE_XBOX:
			return 16384;
			break;
		
		default:
			break;
	}
	
	return 1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int s_round_up_to_platforms_block_size(int fileSize)
{
	int block_size=s_get_platforms_block_size();
	if (fileSize%block_size)
	{
		return (1+fileSize/block_size)*block_size;
	}
	else
	{
		return fileSize;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int s_get_version_number(uint32 fileType)
{
	switch (fileType)
	{
		case 0xb010f357: // OptionsAndPros
			return Script::GetInt( "VERSION_OPTIONSANDPROS", Script::ASSERT );
			break;
		case 0xffc529f4: // Cas
			Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));
			//return Script::GetInt( "VERSION_CAS", Script::ASSERT );
			break;
        case 0x61a1bc57: // Cat
			return Script::GetInt( "VERSION_CAT", Script::ASSERT );
			break;
		case 0x3bf882cc: // Park
			return Script::GetInt( "VERSION_PARK", Script::ASSERT );
			break;
		case 0x26c80b0d: // Replay
			return Script::GetInt( "VERSION_REPLAY", Script::ASSERT );
			break;
		case 0xca41692d: // NetworkSettings
			return Script::GetInt( "VERSION_NETWORKSETTINGS", Script::ASSERT );
			break;
		case 0x62896edf: // CreatedGoals
			return Script::GetInt( "VERSION_CREATEDGOALS", Script::ASSERT );
			break;
		default:
			Dbg_MsgAssert(0,("Bad fileType of '%s' sent to s_get_version_number",FindChecksumName(fileType)));
			break;
	}
	return -1;
}

static const char *s_generate_xbox_directory_name(uint32 fileType, const char *p_name)
{
	static char p_directory_name[100];
	
	switch( fileType )
	{
		case 0xca41692d: // NetworkSettings
		{
			sprintf( p_directory_name,	"%s-NetworkSettings", p_name);
			break;
		}
		case 0xb010f357: // OptionsAndPros
		{
			sprintf( p_directory_name,	"%s-Story/Skater", p_name );
			break;
		}
		case 0xffc529f4: // Cas
		{
			Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));
			//sprintf( p_directory_name,	"%s-Skater", p_name );
			break;
		}
        case 0x61a1bc57: // Cat
		{
			sprintf( p_directory_name,	"%s-Trick", p_name );
			break;
		}
		case 0x3bf882cc: // Park
		{
			sprintf( p_directory_name,	"%s-Park", p_name );
			break;
		}
		case 0x26c80b0d: // Replay
		{
			sprintf( p_directory_name,	"%s-Replay", p_name );
			break;
		}
		case 0x62896edf: // CreatedGoals
		{
			sprintf( p_directory_name,	"%s-Goals", p_name );
			break;
		}
		default:
		{
			Dbg_MsgAssert( 0, ( "Bad file type of '%s' for file '%s' sent to s_generate_xbox_directory_name", FindChecksumName(fileType),p_name));
			break;
		}		
	}
	Dbg_MsgAssert( strlen( p_directory_name ) < 100, ( "Oops" ));
	
	return p_directory_name;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// Returns false if error.
static bool s_make_xbox_dir_and_icons(	Mc::Card *p_card,
										uint32 fileType, const char *p_name, 
										char *p_card_file_name,
										bool *p_insufficientSpace)
{
	Dbg_MsgAssert(p_card_file_name,("NULL p_card_file_name"));
	p_card_file_name[0]=0;
	
	// Generate the directory name
	const char *p_directory_name=s_generate_xbox_directory_name(fileType,p_name);
	
	// Create the directory
	if (!p_card->MakeDirectory(p_directory_name))
	{
		if (p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE)
		{
			*p_insufficientSpace=true;
		}	
		return false;
	}	

	// Calculate the low-level file name.
	const char *p_low_level_directory_name=p_card->ConvertDirectory(p_directory_name);
	if (!p_low_level_directory_name)
	{
		return false;
	}	
	
	sprintf( p_card_file_name, "/%s/%s", p_low_level_directory_name, p_low_level_directory_name );
	Dbg_MsgAssert(strlen(p_card_file_name)<MAX_CARD_FILE_NAME_CHARS,("Oops"));

	
	// Write out the icon file.	
	char	pTitle[100];
	char*	pSourceIconFile		= "";
	char*	pIcoName			= "";

	switch (fileType)
	{
		case 0xb010f357: // OptionsAndPros
		{
			const char *p_title=Script::GetString(CRCD(0x130ab28a,"cfuncs_str_thugstoryskater"));
			strcpy(pTitle,p_title);
			strcat(pTitle,p_name);
			pIcoName		= "saveimage.xbx";
			pSourceIconFile	= "images\\miscellaneous\\xbox_icon_career.xbx";
			break;
		}

		case 0xca41692d: // NetworkSettings
		{
			const char *p_title=Script::GetString(CRCD(0x92c497ae,"cfuncs_str_thugnetsettings"));
			strcpy(pTitle,p_title);
			strcat(pTitle,p_name);
			pIcoName		= "saveimage.xbx";
			// Need new icon ...
			pSourceIconFile	= "images\\miscellaneous\\xbox_icon_career.xbx";
			break;
		}

		case 0x62896edf: // CreatedGoals
		{
			const char *p_title=Script::GetString(CRCD(0x257678cb,"cfuncs_str_thuggoals"));
			strcpy(pTitle,p_title);
			strcat(pTitle,p_name);
			pIcoName		= "saveimage.xbx";
			// Need new icon ...
			pSourceIconFile	= "images\\miscellaneous\\xbox_icon_goal.xbx";
			break;
		}
		
		case 0xffc529f4: // Cas
		{
			Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));
			/*
			sprintf(pTitle,"THUG Skater:%s",p_name);
			pIcoName		= "saveimage.xbx";
			pSourceIconFile	= "images\\miscellaneous\\xbox_icon_skater.xbx";
			*/
			break;
		}

        case 0x61a1bc57: // CAT
		{
			const char *p_title=Script::GetString(CRCD(0x3aa2cffa,"cfuncs_str_thugtrick"));
			strcpy(pTitle,p_title);
			strcat(pTitle,p_name);
			pIcoName		= "saveimage.xbx";
			pSourceIconFile	= "images\\miscellaneous\\xbox_icon_tricks.xbx";
			break;
		}
		
		case 0x3bf882cc: // Park
		{
			const char *p_title=Script::GetString(CRCD(0x2171fddc,"cfuncs_str_thugpark"));
			strcpy(pTitle,p_title);
			strcat(pTitle,p_name);
			pIcoName		= "saveimage.xbx";
			pSourceIconFile	= "images\\miscellaneous\\xbox_icon_park.xbx";
			break;
		}
		
		case 0x26c80b0d: // Replay
		{
			//sprintf(pTitle,"THUG Replay:%s",p_name);
			//pIcoName		= "saveimage.xbx";
			//pSourceIconFile	= "images\\miscellaneous\\xbox_icon_replay.xbx";
			break;
		}
		
		default:
		{
			Dbg_MsgAssert( 0, ( "Bad file type sent to s_make_xbox_dir_and_icons" ));
			break;
		}		
	}

	// Now load the .ico file into memory
	uint32	FileSize	= 0;
	char*	pIco		= NULL;

	File::StopStreaming();
		
	// Get the file interface.
    void *pFP = File::Open(pSourceIconFile, "rb");
    Dbg_MsgAssert(pFP,("No %s file found.",pSourceIconFile));
	
	FileSize = File::GetFileSize( pFP );

	// Allocate a buffer to hold the file.
	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap());

	// Allocating the size rounded up to a multiple of 2048 in case file reading only reads a whole number of sectors.
	pIco = (char*)Mem::Malloc(( FileSize + 2047 ) & ~2047 );
	Dbg_MsgAssert( pIco, ( "Could not allocate memory for %s", pSourceIconFile ));
	Mem::Manager::sHandle().PopContext();

	// Read the file into the buffer and close the file.
#ifdef __NOPT_ASSERT__
	long BytesRead=
#endif	
	File::Read( pIco, 1, FileSize, pFP );
	Dbg_MsgAssert( (uint32)BytesRead <= FileSize, ( "Bad rwfread when loading %s", pSourceIconFile ));
	File::Close( pFP );

	bool SavedOK = false;
	char pFullIcoName[100];
	sprintf( pFullIcoName, "\\%s\\%s", p_low_level_directory_name, pIcoName );
	Dbg_MsgAssert( strlen( pFullIcoName ) < 100,( "Ooops" ));
	
	Mc::File* pFile = p_card->Open( pFullIcoName, Mc::File::mMODE_WRITE | Mc::File::mMODE_CREATE );
	if (p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE)
	{
		*p_insufficientSpace=true;
	}	
	
	if( pFile )
	{   
		uint32 CardBytesWritten = pFile->Write( pIco, FileSize );
		if (p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE)
		{
			*p_insufficientSpace=true;
		}	
		
		if( CardBytesWritten == FileSize )
		{
			SavedOK = true;
		}
		if( !pFile->Flush())
		{
			SavedOK = false;
		}	
		if( !pFile->Close())
		{
			SavedOK = false;
		}	
		delete pFile;
	}
	Mem::Free( pIco );
	return SavedOK;	
}

static void s_generate_card_directory_name(uint32 fileType, const char *p_name, char *p_card_directory_name)
{
	Dbg_MsgAssert(p_card_directory_name,("NULL p_card_directory_name"));
	
	char p_ascii_checksum[20];
	s_generate_ascii_checksum(p_ascii_checksum,p_name,fileType);

	const char *p_header=Config::GetMemCardHeader();
	sprintf(p_card_directory_name,"/%s%s",p_header,p_ascii_checksum);
											
	Dbg_MsgAssert(strlen(p_card_directory_name)<MAX_CARD_FILE_NAME_CHARS,("Oops"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#ifdef __PLAT_NGPS__ // Only compile on NGPS cos sceMcIconSys won't compile on other platforms.
static bool s_make_ps2_dir_and_icons(	Mc::Card *p_card,
										uint32 fileType, const char *p_name, 
										char *p_card_file_name,
										bool *p_insufficientSpace)
{
	char p_directory_name[MAX_CARD_FILE_NAME_CHARS+1];
	s_generate_card_directory_name(fileType,p_name,p_directory_name);
	
	Dbg_MsgAssert(p_card_file_name,("NULL p_card_file_name"));
	sprintf(p_card_file_name,"%s%s",p_directory_name,p_directory_name);
	Dbg_MsgAssert(strlen(p_card_file_name)<MAX_CARD_FILE_NAME_CHARS+1,("Oops"));
	
	// Check whether the icon file exists
	char p_icon_sys[MAX_CARD_FILE_NAME_CHARS+1];
	
	sprintf(p_icon_sys,"%s/icon.sys",p_directory_name);
	Dbg_MsgAssert(strlen(p_icon_sys)<MAX_CARD_FILE_NAME_CHARS+1,("Oops"));
	
	Dbg_MsgAssert(p_card,("NULL p_card"));
	Mc::File* pFile=p_card->Open( p_icon_sys, Mc::File::mMODE_READ );
	if (pFile)
	{
		// Icon exists already, hence so must the directory, so no need to create it.
		pFile->Flush();
		pFile->Close();
		delete pFile;
		pFile=NULL;
		return true;
	}
	
	// Icon does not exist hence the directory cannot either, 
	// so create it and save out the icon.
	if (!p_card->MakeDirectory(p_directory_name))
	{
		if (p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE)
		{
			*p_insufficientSpace=true;
		}	
		return false;
	}	
	
	
	sceMcIconSys IconSys;

	sceVu0IVECTOR bgcolor[4] = 
	{
		{ 0x80,    0,    0, 0 },
		{    0, 0x80,    0, 0 },
		{    0,    0, 0x80, 0 },
		{ 0x80, 0x80, 0x80, 0 },
	};
	
	sceVu0FVECTOR lightdir[3] = 
	{
		{ 0.5, 0.0, 0.0, 0.0 },
		{ 0.0, 0.5, 0.0, 0.0 },
		{ 0.0, 0.0, 0.5, 0.0 },
	};

	sceVu0FVECTOR lightcol[3] = 
	{
		{ 0.50, 0.50, 0.50, 0.00 },
		{ 0.25, 0.25, 0.40, 0.00 },
		{ 0.80, 0.80, 0.80, 0.00 },		
	};
	
	sceVu0FVECTOR ambient = { 0.50, 0.50, 0.50, 0.00 };
	
	memset(&IconSys, 0, sizeof(IconSys));
	strcpy((char*)IconSys.Head, "PS2D");
	
	// Convert the title from ascii to shift-jis & write it in.
	//static char pTitle[100];
	char pTitle[100];
	
	char *pSourceIconFile="";
	char *pIcoName="";
	#ifdef __NOPT_ASSERT__
	uint32 ExpectedIcoSize=0;
	#endif

	int LineBreak=16;
	
	switch (fileType)
	{
		case 0xb010f357: // OptionsAndPros
		{
			strcpy(pTitle,Script::GetString(CRCD(0x130ab28a,"cfuncs_str_thugstoryskater")));
			LineBreak=strlen(pTitle);
			Dbg_MsgAssert(LineBreak<=16,("Mem card title '%s' exceeds 16 chars",pTitle));
			Dbg_MsgAssert(pTitle[LineBreak-1]==':',("Expected mem card file name '%s' to end in ':'",pTitle));
			strcat(pTitle,p_name);
			
			pIcoName="thps5.ico";
			pSourceIconFile="memcard\\thps3.icn";
			#ifdef __NOPT_ASSERT__
			ExpectedIcoSize=Script::GetInt(CRCD(0x4925ed9e,"OptionsProsIconSpaceRequired"));
			#endif
			break;
		}

		case 0xca41692d: // NetworkSettings
		{
			strcpy(pTitle,Script::GetString(CRCD(0x92c497ae,"cfuncs_str_thugnetsettings")));
			LineBreak=strlen(pTitle);
			Dbg_MsgAssert(LineBreak<=16,("Mem card title '%s' exceeds 16 chars",pTitle));
			Dbg_MsgAssert(pTitle[LineBreak-1]==':',("Expected mem card file name '%s' to end in ':'",pTitle));
			strcat(pTitle,p_name);
			
			// For the moment use the options/pros icon, until a new icon is made.
			// TODO: Once got a new icon file, remember to update ScriptDeleteMemCardFile too.
			pIcoName="network.ico";
			pSourceIconFile="memcard\\network.icn";
			#ifdef __NOPT_ASSERT__
			ExpectedIcoSize=Script::GetInt(CRCD(0xf3431f63,"NetworkSettingsIconSpaceRequired"));
			#endif
			break;
		}

		case 0xffc529f4: // Cas
		{
			Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));
			/*
			#if ENGLISH
			sprintf(pTitle,"THUG Skater:%s",p_name);
			#else
			sprintf(pTitle,Script::GetLocalString(CRCD(0x7c2cfe2c,"cfuncs_str_thps3skater")),p_name);
			#endif
			LineBreak=12; // After 'THUG Skater:'
			
			pIcoName="cas.ico";
			pSourceIconFile="memcard\\cas.icn";
			#ifdef __NOPT_ASSERT__
			ExpectedIcoSize=Script::GetInt(CRCD(0xa79ee524,"CASIconSpaceRequired"));
			#endif
			*/
			break;
		}

        case 0x61a1bc57: // Cat
		{
			strcpy(pTitle,Script::GetString(CRCD(0x3aa2cffa,"cfuncs_str_thugtrick")));
			LineBreak=strlen(pTitle);
			Dbg_MsgAssert(LineBreak<=16,("Mem card title '%s' exceeds 16 chars",pTitle));
			Dbg_MsgAssert(pTitle[LineBreak-1]==':',("Expected mem card file name '%s' to end in ':'",pTitle));
			strcat(pTitle,p_name);
			
			pIcoName="tricks.ico";
			pSourceIconFile="memcard\\tricks.icn";
			#ifdef __NOPT_ASSERT__
			ExpectedIcoSize=Script::GetInt(CRCD(0x46b0ef40,"CATIconSpaceRequired"));
			#endif
			break;
		}
		
		case 0x62896edf: // CreatedGoals
		{
			strcpy(pTitle,Script::GetString(CRCD(0x257678cb,"cfuncs_str_thuggoals")));
			LineBreak=strlen(pTitle);
			Dbg_MsgAssert(LineBreak<=16,("Mem card title '%s' exceeds 16 chars",pTitle));
			Dbg_MsgAssert(pTitle[LineBreak-1]==':',("Expected mem card file name '%s' to end in ':'",pTitle));
			strcat(pTitle,p_name);
			
			pIcoName="goals.ico";
			pSourceIconFile="memcard\\goals.icn";
			#ifdef __NOPT_ASSERT__
			ExpectedIcoSize=Script::GetInt(CRCD(0x3205bc07,"CreatedGoalsIconSpaceRequired"));
			#endif
			break;
		}
		
		case 0x3bf882cc: // Park
		{
			strcpy(pTitle,Script::GetString(CRCD(0x2171fddc,"cfuncs_str_thugpark")));
			LineBreak=strlen(pTitle);
			Dbg_MsgAssert(LineBreak<=16,("Mem card title '%s' exceeds 16 chars",pTitle));
			Dbg_MsgAssert(pTitle[LineBreak-1]==':',("Expected mem card file name '%s' to end in ':'",pTitle));
			strcat(pTitle,p_name);
			
			pIcoName="parked.ico";
			pSourceIconFile="memcard\\parked.icn";
			#ifdef __NOPT_ASSERT__
			ExpectedIcoSize=Script::GetInt(CRCD(0x82aaf8dc,"ParkIconSpaceRequired"));
			#endif
			break;
		}
		
		default:
		{
			Dbg_MsgAssert(0,("Bad file type sent to s_create_mem_card_icon_file"));
			break;
		}		
	}

	// Convert the title to shift-jis and put it in IconSys.TitleName
	int Len=strlen(pTitle);
	Dbg_MsgAssert(Len<=32,("Title too long !!!"));
	pTitle[32]=0; // Just to be absolutely sure on non debug builds.
	
	char *pTit=(char*)IconSys.TitleName;
	for (int i=0; i<Len; ++i)
	{
		unsigned short Jis=s_ascii_to_sjis(pTitle[i]);
		*pTit++=(Jis>>8)&0xff;
		*pTit++=Jis&0xff;
	}
	*pTit++=0;
	*pTit=0;

	
	IconSys.OffsLF = 2*LineBreak;
	IconSys.TransRate = 0x60;
	memcpy(IconSys.BgColor, bgcolor, sizeof(bgcolor));
	memcpy(IconSys.LightDir, lightdir, sizeof(lightdir));
	memcpy(IconSys.LightColor, lightcol, sizeof(lightcol));
	memcpy(IconSys.Ambient, ambient, sizeof(ambient));


	strcpy((char*)IconSys.FnameView, pIcoName);
	strcpy((char*)IconSys.FnameCopy, pIcoName);
	strcpy((char*)IconSys.FnameDel, pIcoName);

	
	// Now load the .ico file into memory
	uint32 FileSize=0;
	char *pIco=NULL;

	File::StopStreaming();
		
    void *pFP = File::Open(pSourceIconFile, "rb");
    Dbg_MsgAssert(pFP,("No %s file found.",pSourceIconFile));
	
    FileSize=File::GetFileSize(pFP);
	
	// Make sure the ico file size matches that specified in memcard.q.
	// The +1 is because the icon.sys takes up 1K.
	Dbg_MsgAssert(1+(((FileSize+0x3ff)&(~0x3ff)) >> 10)==ExpectedIcoSize,("%s icon size mismatch",Script::FindChecksumName(fileType)));
	
	// Allocate a buffer to hold the file.
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	// Allocating the size rounded up to a multiple of 2048 in case file reading only
	// reads a whole number of sectors.
    pIco=(char*)Mem::Malloc((FileSize+2047)&~2047);
    Dbg_MsgAssert(pIco,("Could not allocate memory for %s",pSourceIconFile));
	Mem::Manager::sHandle().PopContext();

	// Read the file into the buffer and close the file.
#ifdef __NOPT_ASSERT__
	long BytesRead=
#endif	
	File::Read(pIco,1,FileSize,pFP);
	Dbg_MsgAssert(BytesRead<=FileSize,("Bad rwfread when loading %s",pSourceIconFile));
    File::Close(pFP);
	
	
	bool SavedOK=false;

	pFile=p_card->Open( p_icon_sys, Mc::File::mMODE_WRITE | Mc::File::mMODE_CREATE );
	if (p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE)
	{
		*p_insufficientSpace=true;
	}	
	
	if (pFile)
	{   
		//printf("Writing %s, %d bytes\n",pIconSys,sizeof(IconSys));
		uint32 CardBytesWritten=pFile->Write( &IconSys, sizeof(IconSys) );
		if (p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE)
		{
			*p_insufficientSpace=true;
		}	
		
		if (CardBytesWritten==sizeof(IconSys))
		{
			SavedOK=true;
		}

		if (!pFile->Flush())
		{
			SavedOK=false;
		}	
		if (!pFile->Close())
		{
			SavedOK=false;
		}	
				
		delete pFile;
	}

	// Bail out straight away if the previous save failed so that the error code as
	// returned by Card::GetLastError is correct.
	if (!SavedOK)
	{
		Mem::Free(pIco);
		return false;
	}	

	SavedOK=false;
	char pFullIcoName[MAX_CARD_FILE_NAME_CHARS+1];
	sprintf(pFullIcoName,"%s/%s",p_directory_name,pIcoName);
	Dbg_MsgAssert(strlen(pFullIcoName)<MAX_CARD_FILE_NAME_CHARS,("Ooops"));
	
	pFile=p_card->Open( pFullIcoName, Mc::File::mMODE_WRITE | Mc::File::mMODE_CREATE );
	if (p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE)
	{
		*p_insufficientSpace=true;
	}	
	
	if (pFile)
	{   
		//printf("Writing %s, %d bytes\n",pFullIcoName,FileSize);
		uint32 CardBytesWritten=pFile->Write( pIco, FileSize );
		if (p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE)
		{
			*p_insufficientSpace=true;
		}	
		
		if (CardBytesWritten==FileSize)
		{
			SavedOK=true;
		}

		if (!pFile->Flush())
		{
			SavedOK=false;
		}	
		if (!pFile->Close())
		{
			SavedOK=false;
		}	
				
		delete pFile;
	}
	
	Mem::Free(pIco);
	return SavedOK;
}
#else
static bool s_make_ps2_dir_and_icons(	Mc::Card *p_card,
										uint32 fileType, const char *p_name, 
										char *p_card_file_name,
										bool *p_insufficientSpace)
{
	return false;
}
#endif // #ifdef __PLAT_NGPS__

#ifdef __PLAT_NGC__
static bool s_insert_ngc_icon(	SMcFileHeader *p_fileHeader, 
								Mc::Card *p_card,
								Mc::File *p_file,
								uint32 fileType,
								const char *p_name)
{

	Dbg_MsgAssert(p_fileHeader,("NULL p_fileHeader"));
	Dbg_MsgAssert(p_card,("NULL p_card"));
	Dbg_MsgAssert(p_file,("NULL p_file"));
	Dbg_MsgAssert(p_name,("NULL p_name"));
	
	uint8 *p_buffer=p_fileHeader->mpIconData;
	
	char* p_icon_name="icons\\CreateSk8r_Icon_01.img.ngc";

	switch( fileType )
	{
		case 0xb010f357: // OptionsAndPros
		case 0xca41692d: // NetworkSettings	
		{
			break;
		}	
		
		case 0xffc529f4: // Cas
		{
			Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));
			break;
		}

        case 0x61a1bc57: // CAT
		{
			p_icon_name = "icons\\Trick_Icon_01.img.ngc";
			break;
		}

		case 0x62896edf: // CreatedGoals
		{
			p_icon_name = "icons\\Goal_Icon_01.img.ngc";
			break;
		}
		
		case 0x3bf882cc: // Park
		{
			p_icon_name = "icons\\ParkEd_Icon_01.img.ngc";
			break;
		}
		
		default:
		{
			Dbg_MsgAssert( 0, ( "Bad file type" ));
			break;
		}		
	}
	
	// We used to have each of the save types in the switch statement above choose a different
	// name, one which contained the save type, ie "THUG: Skater"
	// We then had to change them to all have the same name, namely "Tony Hawk's Underground"
	// This is because Nintendo required that we prefix all our save names with Tony Hawk's Underground, but
	// when we tried to append the save type as well the text was too long for the IPL screen.
	strcpy((char*)p_buffer, Script::GetString(CRCD(0x6a6214bc,"cfuncs_str_ngc_generic_save_name")));
	Dbg_MsgAssert(strlen((const char*)p_buffer) < 32,("p_buffer '%s' too long",p_buffer));
	strcpy((char*)p_buffer + 32, p_name );

	// Find the index of the number character that precedes the .
	// This used to be set a hard wired value for each of the cases above, but changed to
	// calculate it instead cos the icon file names could quite likely change.
	int name_index=0;
	while (p_icon_name[name_index] && p_icon_name[name_index] != '.')
	{
		++name_index;
	}
	Dbg_MsgAssert(p_icon_name[name_index],("No '.' found in icon name '%s'",p_icon_name));
	--name_index;
		

	CARDStat stat;
	CARDGetStatus( p_card->GetSlot(), p_file->m_file_number, &stat );

	// Set up to write the banner and icon data.
	char* next_pixel = (char*)p_buffer + 64;

	// Load the banner file.
	void *p_FH = File::Open( "icons\\thps3_bannericon_01.img.ngc", "rb" );
	Dbg_MsgAssert( p_FH, ( "Couldn't open texture file %s\n", "icons\\thps3_bannericon_01.img.ngc" ) );

	int dummy;
	int width;
	int height;
	// Read header info.
	File::Read( &dummy, sizeof( int ), 1, p_FH );		// version
	File::Read( &dummy, sizeof( int ), 1, p_FH );		// checksum
	File::Read( &width, sizeof( int ), 1, p_FH );
	File::Read( &height, sizeof( int ), 1, p_FH );
	File::Read( &dummy, sizeof( int ), 1, p_FH );		// depth
	File::Read( &dummy, sizeof( int ), 1, p_FH );		// levels
	File::Read( &dummy, sizeof( int ), 1, p_FH );		// rwidth
	File::Read( &dummy, sizeof( int ), 1, p_FH );		// rheight
	File::Read( &dummy, sizeof( int ), 1, p_FH );		// has_holes/alphamap

	Dbg_MsgAssert( width == 96, ( "Bad mem card banner width" ));
	Dbg_MsgAssert( height == 32, ( "Bad mem card banner height" ));

	// Read image & palette data.
	File::Read( next_pixel, 96 * 32, 1, p_FH );
	next_pixel += ( 96 * 32 );
	File::Read( next_pixel, 256 * 2, 1, p_FH );
	next_pixel += ( 256 * 2 );

	File::Close( p_FH );

#	if 0
	// Write out a dummy array representing the banner image.
	OSReport( "unsigned short image[96*32] = {\n" );
	for( int i = 0; i < 32; ++i )
	{
		for( int j = 0; j < 96; ++j )
		{
			OSReport( "0x%x,", ((unsigned short*)p_texture->m_pPalette )[((unsigned char*)p_texture->m_pImage )[( i * 32 ) + j]] );
		}
		OSReport( "\n" );
	}
	OSReport( "};\n" );
#	endif

	// Load the icon files.
	for( int i = 0; i < 7; ++i )
	{
		p_icon_name[name_index] = '1' + i;

		// Load the icon file.
		void *p_FH = File::Open( p_icon_name, "rb" );
		Dbg_MsgAssert( p_FH, ( "Couldn't open icon file %s\n", p_icon_name ) );

		int dummy;
		int width;
		int height;
		// Read header info.
		File::Read( &dummy, sizeof( int ), 1, p_FH );		// version
		File::Read( &dummy, sizeof( int ), 1, p_FH );		// checksum
		File::Read( &width, sizeof( int ), 1, p_FH );
		File::Read( &height, sizeof( int ), 1, p_FH );
		File::Read( &dummy, sizeof( int ), 1, p_FH );		// depth
		File::Read( &dummy, sizeof( int ), 1, p_FH );		// levels
		File::Read( &dummy, sizeof( int ), 1, p_FH );		// rwidth
		File::Read( &dummy, sizeof( int ), 1, p_FH );		// rheight
		File::Read( &dummy, sizeof( int ), 1, p_FH );		// has_holes/alphamap

		Dbg_MsgAssert( width == 32, ( "Bad mem card banner width" ));
		Dbg_MsgAssert( height == 32, ( "Bad mem card banner height" ));

		// Read image & palette data.
		File::Read( next_pixel, 32 * 32, 1, p_FH );
		next_pixel += ( 32 * 32 );
		if( i == 6 )
		{
			File::Read( next_pixel, 256 * 2, 1, p_FH );
			next_pixel += 256*2;
		}

		File::Close( p_FH );

		// Icon formats.
		CARDSetIconFormat( &stat, i, CARD_STAT_ICON_C8 ); 

		// Anim speeds.
		CARDSetIconSpeed( &stat, i, CARD_STAT_SPEED_MIDDLE ); 
	}

	Dbg_MsgAssert(next_pixel==(char*)p_buffer+NGC_MEMCARD_ICON_DATA_SIZE,("Incorrect NGC_MEMCARD_ICON_DATA_SIZE, too big by %d bytes",NGC_MEMCARD_ICON_DATA_SIZE-(next_pixel-(char*)p_buffer)));
	stat.commentAddr	= (uint32)p_fileHeader->mpIconData-(uint32)p_fileHeader;
	stat.iconAddr		= stat.commentAddr + 64;	// End of comment strings.

	// Banner format.
	CARDSetBannerFormat( &stat, CARD_STAT_BANNER_C8 ); 
	CARDSetIconSpeed( &stat, 7, CARD_STAT_SPEED_END ); 
	CARDSetIconAnim( &stat, CARD_STAT_ANIM_LOOP );
	CARDSetStatus( p_card->GetSlot(), p_file->m_file_number, &stat );
	return true;
}
#else
static bool s_insert_ngc_icon(	SMcFileHeader *p_fileHeader, 
								Mc::Card *p_card,
								Mc::File *p_file,
								uint32 fileType,
								const char *p_name)
{
	return false;
}
#endif // #ifdef __PLAT_NGC__

static bool s_first_date_is_more_recent(Script::CStruct *p_a, Script::CStruct *p_b)
{
	int year_a=0;
	int month_a=0;
	int day_a=0;
	int hour_a=0;
	int minutes_a=0;
	int seconds_a=0;
	
	Dbg_MsgAssert(p_a,("NULL p_a"));
	p_a->GetInteger(CRCD(0x447d8cc8,"Year"),&year_a);
	p_a->GetInteger(CRCD(0x7149eff9,"Month"),&month_a);
	p_a->GetInteger(CRCD(0x1a5fd66f,"Day"),&day_a);
	p_a->GetInteger(CRCD(0x8fe1eeb1,"Hour"),&hour_a);
	p_a->GetInteger(CRCD(0x5f94b55c,"Minutes"),&minutes_a);
	p_a->GetInteger(CRCD(0xd029f619,"Seconds"),&seconds_a);

	int year_b=0;
	int month_b=0;
	int day_b=0;
	int hour_b=0;
	int minutes_b=0;
	int seconds_b=0;
	
	Dbg_MsgAssert(p_b,("NULL p_b"));
	p_b->GetInteger(CRCD(0x447d8cc8,"Year"),&year_b);
	p_b->GetInteger(CRCD(0x7149eff9,"Month"),&month_b);
	p_b->GetInteger(CRCD(0x1a5fd66f,"Day"),&day_b);
	p_b->GetInteger(CRCD(0x8fe1eeb1,"Hour"),&hour_b);
	p_b->GetInteger(CRCD(0x5f94b55c,"Minutes"),&minutes_b);
	p_b->GetInteger(CRCD(0xd029f619,"Seconds"),&seconds_b);

	bool more_recent=false;
	
	if (year_a < year_b)
	{
	}
	else if (year_a > year_b)
	{
		more_recent=true;
	}
	else if (month_a < month_b)
	{
	}
	else if (month_a > month_b)
	{
		more_recent=true;
	}
	else if (day_a < day_b)
	{
	}
	else if (day_a > day_b)
	{
		more_recent=true;
	}
	else if (hour_a < hour_b)
	{
	}
	else if (hour_a > hour_b)
	{
		more_recent=true;
	}
	else if (minutes_a < minutes_b)
	{
	}
	else if (minutes_a > minutes_b)
	{
		more_recent=true;
	}
	else if (seconds_a < seconds_b)
	{
	}
	else if (seconds_a > seconds_b)
	{
		more_recent=true;
	}
	else
	{
		// Exactly the same date/time, so call it more recent.
		more_recent=true;
	}
	
	return more_recent;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				
// @script | GetMostRecentSave | Finds the most recent save of the given type in the passed
// directory listing. Puts the result into a structure called MostRecentSave. Any existing
// parameter called MostRecentSave will be removed first. If there are no files, no MostRecentSave
// parameter will be created.
// @uparmopt [] | The directory listing. Must be an array of structures.
// @uparmopt name | The save type. If omitted it will return the most recent save of any type.
bool ScriptGetMostRecentSave(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->RemoveComponent("MostRecentSave");
	
	CArray *p_array=NULL;
	pParams->GetArray(NONAME,&p_array);
	
	if (!p_array)
	{
		return false;
	}	
	
	uint32 file_type=0;
	pParams->GetChecksum(NONAME,&file_type);

	CStruct *p_most_recent=NULL;
	for (uint32 i=0; i<p_array->GetSize(); ++i)
	{
		CStruct *p_struct=p_array->GetStructure(i);
		
		uint32 this_file_type=0;
		p_struct->GetChecksum("file_type",&this_file_type);
		
		if (file_type && file_type!=this_file_type)
		{
			continue;
		}
		
		if (p_struct->ContainsFlag("BadVersion") ||
			p_struct->ContainsFlag("Corrupt"))
		{
			if (Config::GetHardware() == Config::HARDWARE_XBOX ||
				Config::GetHardware() == Config::HARDWARE_NGC)
			{
				// For the XBox, do not ignore bad files, so that an error message will appear on bootup
				// if the most recent save is bad. (TT6236)
				// Same for NGC (TT13519)
			}
			else
			{
				// For other platforms, find the most recent good save.
				continue;
			}	
		}	
			
		if (p_most_recent)
		{
			if (s_first_date_is_more_recent(p_struct, p_most_recent))
			{
				p_most_recent=p_struct;
			}	
		}	
		else
		{
			p_most_recent=p_struct;
		}	
	}
	
	if (p_most_recent)
	{
		pScript->GetParams()->AddStructure("MostRecentSave",p_most_recent);
        return true;
	}
	
	return false;	
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				
// @script | GetMemCardSpaceAvailable | Puts the amount of space left on the mem card
// into the parameter SpaceAvailable. Units are K for the PS2
bool ScriptGetMemCardSpaceAvailable(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->AddInteger(CRCD(0xc37c363,"SpaceAvailable"),0);
	pScript->GetParams()->AddInteger(CRCD(0x855b2fc,"FilesLeft"),1000000);

	Mc::Manager * mc_man = Mc::Manager::Instance();
	Mc::Card* p_card=mc_man->GetCard(0,0);
	if (p_card)
	{
		pScript->GetParams()->AddInteger(CRCD(0xc37c363,"SpaceAvailable"),p_card->GetNumFreeClusters());
		#ifdef __PLAT_NGC__
		pScript->GetParams()->AddInteger(CRCD(0x855b2fc,"FilesLeft"),p_card->CountFilesLeft());
		#endif
			
		return true;
	}	
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetMemCardSpaceRequired | Calculates the amount of space required to save the specified
// file type to the memory card, and puts it into the parameter SpaceRequired.
// Units are K for the PS2.
// @uparm name | The save type.
bool ScriptGetMemCardSpaceRequired(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 file_type=0;
	pParams->GetChecksum(NONAME,&file_type);
	
	// s_calculate_total_space_used_on_card adds in the space used by the icons.
	int space_required=s_calculate_total_space_used_on_card(file_type,sGetFixedFileSize(file_type));

	pScript->GetParams()->AddInteger("SpaceRequired",space_required);
	return true;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				
// @script | MemCardFileExists | returns true if the file already exists
// on the card
// @parm string | Name | the name of the file
// @parm name | type | the type of the file
bool ScriptMemCardFileExists(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mc::Manager * mc_man = Mc::Manager::Instance();
	Mc::Card* p_card=mc_man->GetCard(0,0);
	if (!p_card)
	{
		return false;
	}
		
	const char *p_name=NULL;
	pParams->GetText("Name",&p_name);
	
	uint32 file_type=0;
	pParams->GetChecksum("Type",&file_type);

	if (!p_name || !file_type)
	{
		return false;
	}
		
	char p_card_file_name[MAX_CARD_FILE_NAME_CHARS+1];
	p_card_file_name[0]=0;

	const char *p_xbox_directory_name=NULL;
	const char *p_xbox_converted_directory_name=NULL;
	
	switch (Config::GetHardware())
	{
		case Config::HARDWARE_XBOX:
		{
			p_xbox_directory_name=s_generate_xbox_directory_name(file_type,p_name);
			p_xbox_converted_directory_name=p_card->ConvertDirectory(p_xbox_directory_name);
			if (!p_xbox_converted_directory_name)
			{
				return false;
			}	
			sprintf(p_card_file_name,"/%s/%s",p_xbox_converted_directory_name,p_xbox_converted_directory_name);
			Dbg_MsgAssert(strlen(p_card_file_name)<MAX_CARD_FILE_NAME_CHARS,("Oops"));
			break;
		}
		
		case Config::HARDWARE_PS2:
		case Config::HARDWARE_PS2_PROVIEW:
		case Config::HARDWARE_PS2_DEVSYSTEM:
		case Config::HARDWARE_NGC:
		{
			const char *p_header=Config::GetMemCardHeader();
			char p_ascii_checksum[20];
			s_generate_ascii_checksum(p_ascii_checksum,p_name,file_type);
			
			sprintf(p_card_file_name,"/%s%s/%s%s",	p_header,p_ascii_checksum,
													p_header,p_ascii_checksum);
			Dbg_MsgAssert(strlen(p_card_file_name)<MAX_CARD_FILE_NAME_CHARS,("Oops"));
			break;
		}
		
		default:
			break;
	}		

	Mc::File* pFile = p_card->Open( p_card_file_name, Mc::File::mMODE_READ );
	if (pFile)
	{
		int file_size=pFile->Seek(0,Mc::File::BASE_END);
		int total_size_on_card=s_calculate_total_space_used_on_card(file_type,file_size);
		pScript->GetParams()->AddInteger("total_file_size",total_size_on_card);
	
		pFile->Flush();
		pFile->Close();
		delete pFile;
		return true;
	}
	
	if (Config::GetHardware()==Config::HARDWARE_XBOX)
	{
		// Better delete the directory we just created ...
		Dbg_MsgAssert(p_xbox_directory_name,("NULL p_xbox_directory_name ?"));
		p_card->DeleteDirectory(p_xbox_directory_name);
	}
	return false;		
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				
// @script | DeleteMemCardFile | Deletes the specified file from the memory card
// @parmopt string | CardFileName | | The name of the file as stored on the memory card. If this is
// not specified then it is derived from UserFileName and Type
// @parmopt string | UserFileName | | The name of the file as it appears in the files menu
// @parm name | Type | The file type (NetworkSettings, OptionsAndPros, Cas, Park, Replay)

bool ScriptDeleteMemCardFile(Script::CStruct *pParams, Script::CScript *pScript)
{
	if ( Config::GetHardware() != Config::HARDWARE_XBOX)
	{
		Pcm::PauseMusic(true);
		Pcm::PauseStream(true);
	}
	
	Mc::Manager * mc_man = Mc::Manager::Instance();
	Mc::Card* p_card=mc_man->GetCard(0,0);
	if (!p_card)
	{
		if ( Config::GetHardware() != Config::HARDWARE_XBOX)
		{
			Pcm::PauseMusic( -1 );
			Pcm::PauseStream( -1 );
		}	
		return false;
	}
		
	bool delete_succeeded=true;
	
	const char *p_xbox_directory_name=NULL;
	pParams->GetString("XBoxDirectoryName",&p_xbox_directory_name);
	const char *p_full_file_name=NULL;
	pParams->GetString("CardFileName",&p_full_file_name);
	uint32 file_type=0;
	pParams->GetChecksum("Type",&file_type);
	const char *p_name="";
	pParams->GetString("UserFileName",&p_name);

	
	switch (Config::GetHardware())
	{
		case Config::HARDWARE_NGC:
		{
			if (p_full_file_name)
			{
				if (!p_card->Delete(p_full_file_name))
				{
					delete_succeeded=false;
				}	
			}
			else
			{
				char p_temp[MAX_CARD_FILE_NAME_CHARS+1];
				
				// If p_full_file_name is not specified then derive it from Type and UserFileName
				
				const char *p_header=Config::GetMemCardHeader();
				char p_ascii_checksum[20];
				s_generate_ascii_checksum(p_ascii_checksum,p_name,file_type);
		
				sprintf(p_temp,"%s%s%s%s",p_header,p_ascii_checksum,
									      p_header,p_ascii_checksum);
				Dbg_MsgAssert(strlen(p_temp)<MAX_CARD_FILE_NAME_CHARS,("Oops"));
				
				if (!p_card->Delete(p_temp))
				{
					delete_succeeded=false;
				}	
			}
			break;
		}
		
		case Config::HARDWARE_XBOX:
		{
			if (!p_xbox_directory_name)
			{
				// Derive p_xbox_directory_name from the file type and user file name
				p_xbox_directory_name=s_generate_xbox_directory_name(file_type,p_name);
				
			}
			if (!p_card->DeleteDirectory( p_xbox_directory_name ))
			{
				delete_succeeded=false;
			}
			break;
		}
		
		case Config::HARDWARE_PS2:
		case Config::HARDWARE_PS2_PROVIEW:
		case Config::HARDWARE_PS2_DEVSYSTEM:
		{
			char p_temp[MAX_CARD_FILE_NAME_CHARS+1];
			
			if (!p_full_file_name)
			{
				// If p_full_file_name is not specified then derive it from Type and UserFileName
				
				const char *p_header=Config::GetMemCardHeader();
				char p_ascii_checksum[20];
				s_generate_ascii_checksum(p_ascii_checksum,p_name,file_type);
		
				sprintf(p_temp,"/%s%s/%s%s",p_header,p_ascii_checksum,
											p_header,p_ascii_checksum);
				Dbg_MsgAssert(strlen(p_temp)<MAX_CARD_FILE_NAME_CHARS,("Oops"));
				
				p_full_file_name=p_temp;
			}
										   
			int len=strlen(p_full_file_name);
			uint32 file_type=s_determine_file_type(p_full_file_name[len-1]);
		
			// Generate the directory name.
			Dbg_MsgAssert(len<100,("File name too long"));
			char p_directory_name[100];
			char *p_dest=p_directory_name;
			const char *p_source=p_full_file_name;
			// Copy the first '/'
			*p_dest++=*p_source++;
			while (*p_source && *p_source!='/')
			{
				*p_dest++=*p_source++;
			}
			*p_dest=0;
		
			// Delete the file that has the same name as the directory
			if (!p_card->Delete(p_full_file_name))
			{
				delete_succeeded=false;
			}	
			
			// Delete the icon.sys file
			char p_buf[100];
			sprintf(p_buf,"%s/icon.sys",p_directory_name);
			Dbg_MsgAssert(strlen(p_buf)<100,("Oops"));
			if (!p_card->Delete(p_buf))
			{
				delete_succeeded=false;
			}	
			
			// Delete the .ico file.
			switch (file_type)
			{
				case 0xffc529f4: // Cas
				{
					Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));
					//sprintf(p_buf,"%s/cas.ico",p_directory_name);
					break;
				}

                case 0x61a1bc57: // CAT
				{
					sprintf(p_buf,"%s/tricks.ico",p_directory_name);
					break;
				}
				
				case 0x3bf882cc: // Park
				{
					sprintf(p_buf,"%s/parked.ico",p_directory_name);
					break;
				}
				
				case 0x26c80b0d: // Replay
				{
					sprintf(p_buf,"%s/replay.ico",p_directory_name);
					break;
				}
				
				case 0xb010f357: // OptionsAndPros
				{
					sprintf(p_buf,"%s/thps5.ico",p_directory_name);
					break;
				}
				
				case 0x62896edf: // CreatedGoals
				{
					sprintf(p_buf,"%s/goals.ico",p_directory_name);
					break;
				}

				case 0xca41692d: // NetworkSettings
				{
					// TODO: Change this once got a new icon file for the net settings
					sprintf(p_buf,"%s/network.ico",p_directory_name);
					break;
				}
				
				default:
				{
					Dbg_MsgAssert(0,("Bad file type sent to DeleteMemCardFile"));
					break;
				}		
			}
			Dbg_MsgAssert(strlen(p_buf)<100,("Oops"));
			if (!p_card->Delete(p_buf))
			{
				delete_succeeded=false;
			}	
	
			// Delete the directory
			if (!p_card->DeleteDirectory(p_directory_name))
			{
				delete_succeeded=false;
			}	
			break;
		}
		default:
		{
			delete_succeeded=false;
			break;
		}	
	}
		
	if ( Config::GetHardware() != Config::HARDWARE_XBOX)
	{
		Pcm::PauseMusic( -1 );
		Pcm::PauseStream( -1 );
	}
	
	return delete_succeeded;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | FormatCard | Formats the memory card
bool ScriptFormatCard(Script::CStruct *pParams, Script::CScript *pScript)
{
	Pcm::PauseMusic(true);
	Pcm::PauseStream(true);
	
	Mc::Manager * mc_man = Mc::Manager::Instance();
	Mc::Card* p_card=mc_man->GetCard(0,0);
	bool Worked=false;
	if (p_card)
	{
		Worked=p_card->Format();
	}

	Pcm::PauseMusic( -1 );
	Pcm::PauseStream( -1 );
	return Worked;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CardIsInSlot | returns true if the memory card is in the slot
bool ScriptCardIsInSlot(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Mc::Manager * mc_man = Mc::Manager::Instance();
	Mc::Card* p_card=mc_man->GetCard(0,0);
	if (p_card)
	{
		return true;
	}
	else
	{
		#ifdef __PLAT_NGC__
		// On the NGC, p_card will also be NULL if the card is in the slot, but is broken.
		if (mc_man->GotFatalError())
		{
			return true;
		}	
		
		// On the NGC, p_card will also be NULL if something is in the slot,
		// but gives a 'wrong device' error
		if (mc_man->GotWrongDevice())
		{
			return true;
		}	
		#endif
		return false;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CardIsFormatted | returns true if the memory card is formatted
bool ScriptCardIsFormatted(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mc::Manager * mc_man = Mc::Manager::Instance();
	Mc::Card* p_card=mc_man->GetCard(0,0);
	if (p_card)
	{
		return p_card->IsFormatted();
	}
	else
	{
		// If there is no card, then I guess it isn't formatted.
		return false;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SaveFailedDueToInsufficientSpace | 
bool ScriptSaveFailedDueToInsufficientSpace(Script::CStruct *pParams, Script::CScript *pScript)
{
	return s_insufficient_space;
}
		
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetSummaryInfo	| 
bool ScriptGetSummaryInfo(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 file_type=0;
	pParams->GetChecksum(NONAME,&file_type);
	
	int use_vault_data=0;
	pParams->GetInteger(CRCD(0xc78dabf6,"VaultData"),&use_vault_data);
	
	if (use_vault_data)
	{
		file_type=sVaultDataType;
		Dbg_MsgAssert(spVaultData,("Called GetSummaryInfo on the vault data when there was no vault data"));
		s_generate_summary_info(pScript->GetParams(), file_type, spVaultData);
	}
	else
	{
		if (file_type==0xb010f357) // OptionsAndPros
		{
			// Force the goal manager to refresh the goal manager params structure.
			// Note that this cannot be done inside s_generate_summary_info, because LevelUnload()
			// will create a structure and store a pointer to it.
			// In other places SSwitchToNextPool() will be called before calling s_generate_summary_info,
			// and we don't want the structure created by LevelUnload() to come off the special memcard pool.
			Game::CGoalManager* p_goal_manager=Game::GetGoalManager();
			Dbg_MsgAssert(p_goal_manager,("NULL p_goal_manager"));
			p_goal_manager->LevelUnload();
		}
		
		s_generate_summary_info(pScript->GetParams(), file_type, NULL);
	}
		
	return true;
}

#if __USE_REPLAYS__
static int sGetReplayDataSize(int num_dummies)
{
	return 	sizeof(Replay::SReplayDataHeader)+
			num_dummies*sizeof(Replay::SSavedDummy)+
			Replay::GetBufferSize()+
			4; // The checksum of that lot
}			
#endif

static uint32 sGetFixedFileSize(uint32 fileType)
{
	uint32 fixed_size=0;
	
	switch (fileType)
	{
		case 0xb010f357: // OptionsAndPros
			fixed_size=Script::GetInteger(CRCD(0xe7f51ffe,"SaveSize_OptionsAndPros"),Script::ASSERT);
			break;
		case 0xffc529f4: // Cas
			Dbg_MsgAssert(0,("CAS mem card files are no longer supported! Use OptionsAndPros file type instead"));
			//fixed_size=Script::GetInteger("SaveSize_Cas",Script::ASSERT);
			break;
        case 0x61a1bc57: // Cat
			fixed_size=Script::GetInteger(CRCD(0x9eecdec9,"SaveSize_Cat"),Script::ASSERT);
			break;
		case 0x62896edf: // CreatedGoals
			fixed_size=Script::GetInteger(CRCD(0x3308efc0,"SaveSize_CreatedGoals"),Script::ASSERT);
			break;
		case 0x3bf882cc: // Park
			fixed_size=Script::GetInteger(CRCD(0x2cb071ed,"SaveSize_Park"),Script::ASSERT);
			break;
		case 0x26c80b0d: // Replay
			fixed_size=Script::GetInteger(CRCD(0x8ee3aa00,"SaveSize_Replay"),Script::ASSERT);
			break;
		case 0xca41692d: // NetworkSettings
			fixed_size=Script::GetInteger(CRCD(0x651c978d,"SaveSize_NetworkSettings"),Script::ASSERT);
			break;
		default:
			Dbg_MsgAssert(0,("Bad file_type of %s",Script::FindChecksumName(fileType)));
			break;
	}
	fixed_size+=sizeof(SMcFileHeader);
	fixed_size=s_round_up_to_platforms_block_size(fixed_size);
	return fixed_size;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SaveToMemoryCard | 
bool ScriptSaveToMemoryCard(Script::CStruct *pParams, Script::CScript *pScript)
{
	if (Config::GetHardware() == Config::HARDWARE_NGC)
	{
		// On the GameCube, the temp mem card pools only exist for the duration of this function,
		// because they use space normally used for rendering, hence cannot exist during the mem card scripts
		// which may execute over many frames.
		ScriptCreateTemporaryMemCardPools(NULL,NULL);
	}
		
	if ( Config::GetHardware() != Config::HARDWARE_XBOX)
	{
		Pcm::PauseMusic(true);
		Pcm::PauseStream(true);
	}
	
	s_insufficient_space=false;
	
	const char *p_name="";
	pParams->GetText("Name",&p_name);

	uint32 file_type=0;
	pParams->GetChecksum("Type",&file_type);

	// If the SaveVaultData flag is set, it will save the stored spVaultData, which
	// will have been loaded from the online vault.
	int save_vault_data=0;
	pParams->GetInteger(CRCD(0xbb609e73,"SaveVaultData"),&save_vault_data);
	if (save_vault_data)
	{
		Dbg_MsgAssert(spVaultData,("Called SaveToMemoryCard with no vault data"));
		file_type=sVaultDataType;
	}	


	if (!save_vault_data && file_type==0xb010f357) // OptionsAndPros
	{
		// Force the goal manager to refresh the goal manager params structure.
		// This has to be done before calling SSwitchToNextPool(), because LevelUnload()
		// will create a structure and store a pointer to it.
		// It is best for the special pools to be kept empty outside of GetMemCardSpaceRequired
		// or SaveToMemoryCard, otherwise saving the game could run out of pool space.
		Game::CGoalManager* p_goal_manager=Game::GetGoalManager();
		Dbg_MsgAssert(p_goal_manager,("NULL p_goal_manager"));
		p_goal_manager->LevelUnload();
	}


	// If the special pools exist, switch to them so that the components & structs etc get allocated off them.
	// The special pools use the space freed by unloading the skater anims, and are for preventing memory
	// overflows when the save size gets large.
	Dbg_MsgAssert(CComponent::SGetCurrentPoolIndex()==0,("Bad current CComponent pool"));
	Dbg_MsgAssert(CStruct::SGetCurrentPoolIndex()==0,("Bad current CStruct pool"));
	Dbg_MsgAssert(CVector::SGetCurrentPoolIndex()==0,("Bad current CVector pool"));
	bool got_special_pools=false;
	CComponent::SSwitchToNextPool();
	if (CComponent::SPoolExists())
	{
		CStruct::SSwitchToNextPool();
		Dbg_MsgAssert(CStruct::SPoolExists(),("No special CStruct pool ?"));
		CVector::SSwitchToNextPool();
		Dbg_MsgAssert(CVector::SPoolExists(),("No special CVector pool ?"));
		
		got_special_pools=true;
	}
	else
	{
		CComponent::SSwitchToPreviousPool();
	}	

	
	#ifdef	__NOPT_ASSERT__
	int initial_ccomponent_num_used_items=CComponent::SGetNumUsedItems();
	int initial_cstruct_num_used_items=CStruct::SGetNumUsedItems();
	int initial_cvector_num_used_items=CVector::SGetNumUsedItems();
	#endif

	// WARNING !		WARNING !		WARNING !		WARNING !		WARNING !
	// Between here and SSwitchToPreviousPool(), all CStruct's and CComponent's will be allocated
	// off a special pool.
	// This is to avoid overflowing the regular pools with these temporary big structures.
	// Make sure that no function, or function that it calls, stores any pointers to new CStructs
	// between here and SSwitchToPreviousPool().
	// This is just to keep the special pool clean. Things will get confusing if other stuff
	// gets allocated off it and uses up space there, cos then saving to mem card could start
	// running out of pool again.

	Script::CStruct *pMemCardStuff=new Script::CStruct;
	if (save_vault_data)
	{
		pMemCardStuff->AppendStructure(spVaultData);
	}
	else
	{
		s_insert_game_save_info(file_type,pMemCardStuff);	
	}	
	//Script::PrintContents(pMemCardStuff);
	
	Script::CStruct *pSummaryInfo=new Script::CStruct;
	s_generate_summary_info(pSummaryInfo,file_type,pMemCardStuff);	
	
	// Put the user filename into the summary info too.
	pSummaryInfo->AddString("filename",p_name);

	#ifdef	__NOPT_ASSERT__
	printf("Save type = '%s'\n",Script::FindChecksumName(file_type));
	printf("Num CComponents used by save = %d\n",CComponent::SGetNumUsedItems()-initial_ccomponent_num_used_items);
	printf("Num CStructs used by game save = %d\n",CStruct::SGetNumUsedItems()-initial_cstruct_num_used_items);
	printf("Num CVectors used by game save = %d\n",CVector::SGetNumUsedItems()-initial_cvector_num_used_items);
	#endif


	Mc::File* pFile=NULL;
	uint8 *pTemp=NULL;
	uint8 *p_pad=NULL;
	uint32 pad_size=0;
	bool SavedOK=false;

	uint32 CardBytesWritten=0;
	uint8 *p_dest=NULL;
	SMcFileHeader *p_file_header=NULL;
	uint32 BytesWritten=0;
	
	Mc::Manager * mc_man=NULL;
	Mc::Card* p_card=NULL;
	
	// fixed_size is the hard-wired file size as defined in memcard.q, rounded up to
	// the platform's block size. A file of exactly this size will be opened for writing.
	// The code will assert if the required space is greater than this.
	// The file sizes are required to be fixed as a TRC thing for GameCube.
	uint32 fixed_size=sGetFixedFileSize(file_type);
	
	// header_and_structures_size is the exact space used by the header, summary info, and 
	// game save info structures. It is not rounded up to the platform's block size.
	// For all save types except replays, this comprises all the info in the file.
	// Replay's have the replay data tagged on the end of the file instead, because there
	// is not enough memory to keep it in the structures.
	uint32 header_and_structures_size=0;

	
	// This is the size of the replay data, if any. Not rounded up to the platform's block size.
	uint32 replay_data_size=0;

#if __USE_REPLAYS__
	// The big replay data has to be tagged on the end rather than being put inside pMemCardStuff
	// since there is not enough memory to make a copy of the replay data there.
	Replay::SReplayDataHeader replay_data_header;
	if (file_type==0x26c80b0d/* Replay */)
	{
		Replay::WriteReplayDataHeader(&replay_data_header);
		replay_data_size=sGetReplayDataSize(replay_data_header.mNumStartStateDummies);
	}	
#endif	
	
	/////////////////////////////////////////////////////////////////////////
	//pMemCardStuff->PrintContents();
	//printf("Space required = %d\n",pMemCardStuff->CalculateBufferSize());
	/////////////////////////////////////////////////////////////////////////
	
	// Get how much space is needed to store the structures.
	// Calculating this before opening the file so that if CalculateBufferSize fails,
	// the file won't be left with zero size, erasing the previous save game.
	// Passing fixed_size as the size of the temp buffer used when calculating the actual size needed.
	uint32 structure_size=CalculateBufferSize(pMemCardStuff, fixed_size);
	uint32 summary_info_size=CalculateBufferSize(pSummaryInfo,MAX_SUMMARY_INFO_SIZE);
	Dbg_MsgAssert(summary_info_size<=MAX_SUMMARY_INFO_SIZE,("summary_info_size too big !!"));
	header_and_structures_size=sizeof(SMcFileHeader)+summary_info_size+structure_size;

	uint32 required_size=header_and_structures_size+replay_data_size;
	
	#ifdef __NOPT_ASSERT__
	Dbg_MsgAssert(required_size <= fixed_size,("Need to update fixed file size for type '%s'\nMin required size = %d",Script::FindChecksumName(file_type),required_size));
	printf("required_size=%d, fixed_size=%d, %d percent\n",required_size,fixed_size,(100*required_size)/fixed_size);
	if (required_size > fixed_size)
	{
		goto ERROR;
	}	
	#endif

	pad_size=fixed_size-required_size;
	if (pad_size && Config::GetHardware() != Config::HARDWARE_NGC)
	{
		p_pad=(uint8*)Mem::Malloc(pad_size);
		for (uint32 i=0; i<pad_size; ++i)
		{
			p_pad[i]=0x69;
		}
	}
	
	mc_man = Mc::Manager::Instance();
	p_card=mc_man->GetCard(0,0);
	if (!p_card)
	{
		goto ERROR;
	}
	// GameCube often crashes if try to do card operations on a bad card, so do this check first.
	if (!p_card->IsFormatted())
	{
		goto ERROR;
	}
	
	char p_card_file_name[MAX_CARD_FILE_NAME_CHARS+1];
	switch (Config::GetHardware())
	{
		case Config::HARDWARE_PS2:
		case Config::HARDWARE_PS2_PROVIEW:
		case Config::HARDWARE_PS2_DEVSYSTEM:
			if (!s_make_ps2_dir_and_icons(p_card,
										  file_type,p_name,
										  p_card_file_name,
										  &s_insufficient_space))
			{
				goto ERROR;
			}
			break;
			
		case Config::HARDWARE_NGC:
		{
			// On the NGC, just generate the card file name.
			// The icon cannot be created at this point because on the NGC the
			// icon images have to be stored in the file data itself, so we have to
			// wait until after the file is opened.
			char p_directory_name[MAX_CARD_FILE_NAME_CHARS+1];
			s_generate_card_directory_name(file_type,p_name,p_directory_name);
			
			sprintf(p_card_file_name,"%s%s",p_directory_name,p_directory_name);
			Dbg_MsgAssert(strlen(p_card_file_name)<MAX_CARD_FILE_NAME_CHARS+1,("Oops"));
			break;
		}	
		
		case Config::HARDWARE_XBOX:
			if (!s_make_xbox_dir_and_icons(p_card,
										   file_type,p_name,
										   p_card_file_name,
										   &s_insufficient_space))
			{
				goto ERROR;
			}
			break;
		default:
			{
				goto ERROR;
			}
			break;
	}
			

	// Open a file big enough to hold all the data.
	//printf("Opening file %s of size %d for writing\n",p_card_file_name,fixed_size);
	pFile=p_card->Open( p_card_file_name, Mc::File::mMODE_WRITE | Mc::File::mMODE_CREATE, fixed_size );
	s_insufficient_space=p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE;

	if (!pFile)
	{
		goto ERROR;
	}
		
	pTemp=(uint8*)Mem::Malloc(header_and_structures_size);
	
	p_file_header=(SMcFileHeader*)pTemp;
	// Zero the whole structure to ensure that the XBox mSignature member is zeroed since
	// it will be part of the data who's signature is calculated.
	p_dest=(uint8*)p_file_header;
	for (uint32 i=0; i<sizeof(SMcFileHeader); ++i)
	{
		*p_dest++=0;
	}

	if (Config::GetHardware()==Config::HARDWARE_NGC)
	{
		if (!s_insert_ngc_icon(p_file_header,p_card,pFile,file_type,p_name))
		{
			goto ERROR;
		}
	}
	
	p_file_header->mVersion=s_get_version_number(file_type);
	p_file_header->mSummaryInfoSize=summary_info_size;
	p_file_header->mDataSize=header_and_structures_size;

	p_dest=(uint8*)(p_file_header+1);

	// Write in the summary info and calculate its checksum.
	BytesWritten=WriteToBuffer(pSummaryInfo, p_dest, summary_info_size, Script::NO_ASSERT);
	if (BytesWritten!=summary_info_size)
	{
		// Note: This assert is surrounded by an if to prevent a warning on non debug builds
		// due to the BytesWritten variable being unused.
		Dbg_MsgAssert(0,("BytesWritten does not equal summary_info_size ?"));
	}	
	p_file_header->mSummaryInfoChecksum=Crc::GenerateCRCCaseSensitive((const char *)p_dest,summary_info_size);
	p_dest+=summary_info_size;

	
	// Write in the game save info
	BytesWritten=WriteToBuffer(pMemCardStuff, p_dest, structure_size, Script::NO_ASSERT);
	
	// K: Last minute fix, 'encrypt' the network settings so that the AOL info cannot be 
	// read. Not very secure, but it should be enough to make the strings non obvious.
	if (file_type==0xca41692d) // NetworkSettings
	{
		//printf("Encrypting %d bytes ...\n",structure_size);
		for (uint32 e=0; e<structure_size; ++e)
		{
			p_dest[e]+=0x69;
		}
	}
	
	if (BytesWritten!=structure_size)
	{
		// Note: This assert is surrounded by an if to prevent a warning on non debug builds
		// due to the BytesWritten variable being unused.
		Dbg_MsgAssert(0,("BytesWritten does not equal structure_size ?"));
	}	
	p_dest+=structure_size;

	Dbg_MsgAssert(p_dest==pTemp+header_and_structures_size,("What ?"));
					
	#ifdef __PLAT_XBOX__
	HANDLE h_signature=XCalculateSignatureBegin(0);
	if (XCalculateSignatureUpdate(h_signature,(const BYTE *)(pTemp),header_and_structures_size) != ERROR_SUCCESS)
	{
		Dbg_MsgAssert(0,("XCalculateSignatureUpdate failed!"));
	}
	if (pad_size)
	{
		Dbg_MsgAssert(p_pad,("NULL p_pad ?"));
		if (XCalculateSignatureUpdate(h_signature,(const BYTE *)(p_pad),pad_size) != ERROR_SUCCESS)
		{
			Dbg_MsgAssert(0,("XCalculateSignatureUpdate failed!"));
		}
	}	
	if (XCalculateSignatureEnd(h_signature,&p_file_header->mSignature)!=ERROR_SUCCESS)
	{
		Dbg_MsgAssert(0,("XCalculateSignatureEnd failed!"));
	}
	#else
	// Now calculate a checksum of all that data.
	// It will include the contents of p_file_header in the data too, 
	// including mChecksum itself which will be zero at this point.
	// By zeroing mChecksum and including it in the data that is checksum'd it
	// means that the order of the members of SMcFileHeader does not matter.
	// Just seems kind of ugly having to have mChecksum be the first member, cos
	// it would also mean having to take the address of the second member to get
	// the start address of the data to checksum, bleurgh.
	
	p_file_header->mChecksum=Crc::GenerateCRCCaseSensitive((const char*)pTemp,header_and_structures_size);
	#endif // #ifdef __PLAT_XBOX__
	
	CardBytesWritten=pFile->Write( pTemp, header_and_structures_size );
	//printf("Wrote %d bytes\n",CardBytesWritten);
	s_insufficient_space=p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE;
	if (CardBytesWritten!=header_and_structures_size)
	{
		goto ERROR;
	}


#if __USE_REPLAYS__
	// Write out the big replay buffer.
	// It cannot be written out in one go by getting its base pointer, because on
	// the GameCube it is stored in ARAM.
	// The only way to access it is via the ReadFromBuffer function.
	// Also, there is not enough memory to copy it into a big temporary buffer, 
	// so write it out in small chunks.
	if (file_type==0x26c80b0d/* Replay */)
	{
		// Initialise the checksum, which is calculated cumulatively.
		uint32 replay_data_checksum=0xffffffff;
		
		// Write out the replay data header.
		CardBytesWritten=pFile->Write( (uint8*)&replay_data_header, sizeof(Replay::SReplayDataHeader) );
		s_insufficient_space=p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE;
		if (CardBytesWritten!=sizeof(Replay::SReplayDataHeader))
		{
			goto ERROR;
		}
	
		replay_data_checksum=Crc::UpdateCRC((const char *)&replay_data_header,sizeof(Replay::SReplayDataHeader),replay_data_checksum);


		// Write out the start-state dummies.	
		Replay::SSavedDummy saved_dummy;
		Replay::CDummy *p_dummy=Replay::GetFirstStartStateDummy();
		int count=0;
		while (p_dummy)
		{
			p_dummy->Save(&saved_dummy);
			
			CardBytesWritten=pFile->Write( (uint8*)&saved_dummy, sizeof(Replay::SSavedDummy) );
			s_insufficient_space=p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE;
			if (CardBytesWritten!=sizeof(Replay::SSavedDummy))
			{
				goto ERROR;
			}
			replay_data_checksum=Crc::UpdateCRC((const char *)&saved_dummy,sizeof(Replay::SSavedDummy),replay_data_checksum);
			
			p_dummy=p_dummy->mpNext;
			++count;
		}
		if (count!=replay_data_header.mNumStartStateDummies)
		{
			Dbg_MsgAssert(0,("Dummy count mismatch"));
		}	
		
		int buf_size=Replay::GetBufferSize();
		Dbg_MsgAssert((buf_size%REPLAY_BUFFER_CHUNK_SIZE)==0,("Replay buffer size not a multiple of REPLAY_BUFFER_CHUNK_SIZE"));
		
		uint8 *p_chunk=Replay::GetTempBuffer();
		int num_chunks=buf_size/REPLAY_BUFFER_CHUNK_SIZE;
		
		uint32 offset=0;
		for (int i=0; i<num_chunks; ++i)
		{
			Replay::ReadFromBuffer(p_chunk,offset,REPLAY_BUFFER_CHUNK_SIZE);
			replay_data_checksum=Crc::UpdateCRC((const char *)p_chunk,REPLAY_BUFFER_CHUNK_SIZE,replay_data_checksum);
			
			offset+=REPLAY_BUFFER_CHUNK_SIZE;
			
			CardBytesWritten=pFile->Write( p_chunk, REPLAY_BUFFER_CHUNK_SIZE );
			s_insufficient_space=p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE;
			if (CardBytesWritten!=REPLAY_BUFFER_CHUNK_SIZE)
			{
				goto ERROR;
			}
		}
		
		// Write out the replay data checksum
		CardBytesWritten=pFile->Write( (uint8*)&replay_data_checksum, 4 );
		s_insufficient_space=p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE;
		if (CardBytesWritten!=4)
		{
			goto ERROR;
		}
	}
#endif
	
	// Now write out padding so as to make sure the file has size fixed_size, otherwise
	// there will be a discrepancy between the space that the game claims is required and
	// the size that appears in the file list, and I'll get a ton of TT bugs.
	if ( Config::GetHardware() != Config::HARDWARE_NGC)
	{
		// TODO: Fix bug in the Write function for the NGC. It always writes from offset 0,
		// meaning that writing this padding overwrites the start of the file data.
		// We don't actually have to write this padding on the NGC, because the resultant file size
		// will be exactly that specified in the file Open command.
		// But the bug will manifest itself when saving the replay data, so need to fix it ...
		if (pad_size)
		{
			Dbg_MsgAssert(p_pad,("NULL p_pad ?"));			
			CardBytesWritten=pFile->Write( p_pad, pad_size );
			//printf("Wrote %d bytes\n",CardBytesWritten);
			
			s_insufficient_space=p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE;
			if (CardBytesWritten != pad_size)
			{
				goto ERROR;
			}
		}	
	}	
	
	SavedOK=true;
	
ERROR:	
	if (p_pad)
	{
		Mem::Free(p_pad);
	}
	if (pTemp)
	{
		Mem::Free(pTemp);
	}
	if (pFile)
	{
		if (!pFile->Flush())
		{
			SavedOK=false;
		}	
		if (!pFile->Close())
		{
			SavedOK=false;
		}	
		delete pFile;
	}	
	
	delete pSummaryInfo;
	delete pMemCardStuff;

	
	if (got_special_pools)
	{
		Dbg_MsgAssert(CComponent::SGetNumUsedItems()==0,("Expected the special mem card CComponent pool to be empty at this point, but got %d items",CComponent::SGetNumUsedItems()));
		Dbg_MsgAssert(CStruct::SGetNumUsedItems()==0,("Expected the special mem card CStruct pool to be empty at this point, but got %d items",CStruct::SGetNumUsedItems()));
		Dbg_MsgAssert(CVector::SGetNumUsedItems()==0,("Expected the special mem card CVector pool to be empty at this point, but got %d items",CVector::SGetNumUsedItems()));
		CComponent::SSwitchToPreviousPool();
		CStruct::SSwitchToPreviousPool();
		CVector::SSwitchToPreviousPool();
	}
	
	if ( Config::GetHardware() != Config::HARDWARE_XBOX)
	{
		Pcm::PauseMusic( -1 );
		Pcm::PauseStream( -1 );
	}

	if (Config::GetHardware() == Config::HARDWARE_NGC)
	{
		ScriptRemoveTemporaryMemCardPools(NULL,NULL);
	}
		
	return SavedOK;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#if __USE_REPLAYS__
	#ifdef __PLAT_NGC__
	static SMcFileHeader sFileHeader __attribute__((aligned( 32 )));
	#else
	static SMcFileHeader sFileHeader;
	#endif
#endif

bool ScriptSetSectionsToApplyWhenLoading(Script::CStruct *pParams, Script::CScript *pScript)
{
    printf("ScriptSetSectionsToApplyWhenLoading was called.........................\n");
    // These flags only apply when loading type OptionsAndPros.
	// They allow for only certain sections of the file being applied to the game state.
	// For example, when loading a custom skater, only the CUSTOM_SKATER part must be applied.
	s_apply_flags=0;
	
	if (pParams->ContainsFlag(CRCD(0x16b506c0,"ApplyCustomSkater")))
	{
		s_apply_flags |= 1<<APPLY_CUSTOM_SKATER;
	}	
	if (pParams->ContainsFlag(CRCD(0xdc7ea3ce,"ApplyStorySkater")))
	{
		s_apply_flags |= 1<<APPLY_STORY_SKATER;
	}	
	if (pParams->ContainsFlag(CRCD(0x33ec233f,"ApplyStory")))
	{
		s_apply_flags |= 1<<APPLY_STORY;
	}	
	if (pParams->ContainsFlag(CRCD(0xb39d7cc4,"ApplyGeneralOptions")))
	{
		s_apply_flags |= 1<<APPLY_GLOBAL_INFO;
	}	
	
	if (pParams->ContainsFlag(CRCD(0xc4e78e22,"all")))
	{
		s_apply_flags=0xffffffff;
	}	
	
	return true;
}

// @script | LoadFromMemoryCard | Load the specified file from the memory card
// @parm string | Name | The name of the file to load
// @parm name | Type | The type of the file (cas, network, park, etc.)
bool ScriptLoadFromMemoryCard(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_MsgAssert(!Config::Bootstrap(),("Can't use memory card from bootstrap demo"));

	if ( Config::GetHardware() != Config::HARDWARE_XBOX)
	{
		Pcm::PauseMusic(true);
		Pcm::PauseStream(true);
	}
		
	// Create structures for holding the summary info & game save info
	CStruct *pSummaryInfo=new CStruct;
	CStruct *pMemCardStuff=new CStruct;

	// A bunch of variable declarations, which need to be up here cos there are goto's later
	const char *p_name="";
	uint32 file_type=0;
	
	Mc::File* p_file=NULL;
	int file_size=0;
	uint8 *p_temp=NULL;
	uint8 *p_stuff=NULL;
	SMcFileHeader *p_file_header;
	bool loaded_ok=false;
	uint32 calculated_checksum=0;
	uint32 stored_checksum=0;
	bool checksum_matches=false;
	
	// The LoadForUpload flag is passed as an integer so that that calls to LoadFromMemoryCard
	// in script can set LoadForUpload equal to the value of some script global for convenience.
	int load_for_upload=0;
	pParams->GetInteger(CRCD(0x8c6808d4,"LoadForUpload"),&load_for_upload);

	#ifdef __PLAT_XBOX__
	XCALCSIG_SIGNATURE stored_signature;
	XCALCSIG_SIGNATURE calculated_signature;
	HANDLE h_signature;
	BYTE *p_a=NULL;
	BYTE *p_b=NULL;
	#endif	
	
	// Get the card.
	Mc::Manager * mc_man = Mc::Manager::Instance();
	Mc::Card* p_card=mc_man->GetCard(0,0);
	if (!p_card)
	{
		goto ERROR;
	}
	// GameCube often crashes if try to do card operations on a bad card, so do this check first.
	if (!p_card->IsFormatted())
	{
		goto ERROR;
	}
		
		
	pParams->GetText("Name",&p_name);
	pParams->GetChecksum("Type",&file_type);

	// Calculate the low-level filename
	char p_card_file_name[MAX_CARD_FILE_NAME_CHARS+1];
	p_card_file_name[0]=0;
	
	switch (Config::GetHardware())
	{
		case Config::HARDWARE_PS2:
		case Config::HARDWARE_PS2_PROVIEW:
		case Config::HARDWARE_PS2_DEVSYSTEM:
		{
			const char *p_header=Config::GetMemCardHeader();
			char p_ascii_checksum[20];
			s_generate_ascii_checksum(p_ascii_checksum,p_name,file_type);
			
			sprintf(p_card_file_name,"/%s%s/%s%s",	p_header,p_ascii_checksum,
													p_header,p_ascii_checksum);
			break;
		}	
		case Config::HARDWARE_NGC:
		{
			char p_directory_name[MAX_CARD_FILE_NAME_CHARS+1];
			s_generate_card_directory_name(file_type,p_name,p_directory_name);
			
			sprintf(p_card_file_name,"%s%s",p_directory_name,p_directory_name);
			break;
		}	
		case Config::HARDWARE_XBOX:
		{
			// Generate the directory name
			const char *p_directory_name=s_generate_xbox_directory_name(file_type,p_name);
			const char *p_low_level_directory_name=p_card->ConvertDirectory(p_directory_name);
		
			if (!p_low_level_directory_name)
			{
				goto ERROR;
			}
						
			// Calculate the low-level file name.
			sprintf( p_card_file_name, "/%s/%s", p_low_level_directory_name, p_low_level_directory_name );
			break;
		}	
		default:
		{
			goto ERROR;
			break;
		}	
	}
	
	Dbg_MsgAssert(strlen(p_card_file_name)<MAX_CARD_FILE_NAME_CHARS+1,("Oops"));
	
#if __USE_REPLAYS__
	if (file_type==0x26c80b0d/* Replay */)
	{
		strcpy(spReplayCardFileName,p_card_file_name);
		sNeedToLoadReplayBuffer=true;
	}
#endif
		
	// Open the file.
	p_file=p_card->Open( p_card_file_name, Mc::File::mMODE_READ );
	if (!p_file)
	{
		// File could not be opened
		goto ERROR;
	}	
	
	// File opened OK
	
	// Check the file size.
	file_size=p_file->Seek( 0 ,Mc::File::BASE_END);
	// Removed the file size check, because the fixed size needed to be updated, and keeping the
	// check would prevent existing parks from being able to be loaded.
	//if (file_size != (int)sGetFixedFileSize(file_type))
	//{
	//	// Size mismatch, so consider the file corrupted.
	//	pScript->GetParams()->AddChecksum(NONAME,GenerateCRC("CorruptedData"));
	//	goto ERROR;
	//}	
	
	// Read the file into memory
	// Allocate a temporary buffer for reading the file into
	p_temp=(uint8*)Mem::Malloc(file_size);
	Dbg_MsgAssert(p_temp,("Could not allocate %d bytes for file buffer for file %s",file_size,p_card_file_name));

	p_file->Seek(0,Mc::File::BASE_START);
	if (p_file->Read(p_temp, file_size) != file_size)
	{
		// Some sort of read error.
		goto ERROR;
	}		
	
	// Seemed to read into memory OK
	// Check if the data is corrupt by recalculating the checksum.
	p_stuff=p_temp;
	p_file_header=(SMcFileHeader*)p_stuff;
	
	#ifdef __PLAT_XBOX__
	// Load p_file_header->mSignature into stored_signature
	// then zero out p_file_header->mSignature
	p_a=p_file_header->mSignature.Signature;
	p_b=stored_signature.Signature;
	for (int i=0; i<XCALCSIG_SIGNATURE_SIZE; ++i)
	{
		*p_b=*p_a;
		*p_a=0;
		++p_a;
		++p_b;
	}
							
	// Calculate the signature of the data.						
	h_signature=XCalculateSignatureBegin(0);
	if (XCalculateSignatureUpdate(h_signature,(const BYTE *)p_stuff,file_size) != ERROR_SUCCESS)
	{
		Dbg_MsgAssert(0,("XCalculateSignatureUpdate failed!"));
	}
	if (XCalculateSignatureEnd(h_signature,&calculated_signature)!=ERROR_SUCCESS)
	{
		Dbg_MsgAssert(0,("XCalculateSignatureEnd failed!"));
	}
	
	// Compare the calculated signature with the stored one.
	p_a=stored_signature.Signature;
	p_b=calculated_signature.Signature;
	checksum_matches=true;
	for (int i=0; i<XCALCSIG_SIGNATURE_SIZE; ++i)
	{
		if (*p_a != *p_b)
		{
			checksum_matches=false;
			break;
		}
		++p_a;
		++p_b;
	}		

	#else
	checksum_matches=false;
	stored_checksum=p_file_header->mChecksum;
	
	// Set the checksum to zero because that was what it was when
	// the checksum was calculated.
	p_file_header->mChecksum=0;
	// Using p_file_header->mDataSize instead of file_size, because file_size includes the
	// padding, which is not included in the checksum calculation on PS2 and NGC
	if (p_file_header->mDataSize > 500000)
	{
		// If mDataSize itself is corrupted, then don't go trying to calculate the
		// checksum of megabytes of data, in case that hangs the game by hitting some sort
		// of restricted memory location.
	}
	else
	{
		calculated_checksum=Crc::GenerateCRCCaseSensitive((const char*)p_stuff,p_file_header->mDataSize);
	
		if (calculated_checksum==stored_checksum &&
			p_file_header->mVersion==s_get_version_number(file_type))
		{
			checksum_matches=true;
		}
	}	
	#endif

		
	if (checksum_matches)
	{
		// The data is OK, so it is safe to parse it.
		
		// Skip over the header.
		p_stuff=(uint8*)(p_file_header+1);

		// Skip over the summary info
		p_stuff=ReadFromBuffer(pSummaryInfo, p_stuff);

		// K: Last minute fix, 'decrypt' the network settings.
		if (file_type==0xca41692d) // NetworkSettings
		{
			int num_bytes=p_file_header->mDataSize-(p_stuff-((uint8*)p_file_header));
			//printf("Decrypting %d bytes ...\n",num_bytes);
			for (int e=0; e<num_bytes; ++e)
			{
				p_stuff[e]-=0x69;
			}
		}	

		// Read in the game save info						
		ReadFromBuffer(pMemCardStuff, p_stuff);

		// The data is now safely in pMemCardStuff ...
		
		if (load_for_upload)
		{
			// If the file was loaded for upload to the net, then do not process the data
			// but just store it away so that the GetMemCardDataForUpload command can retrieve
			// it a bit later.
			Dbg_MsgAssert(spVaultData==NULL,("\n%s\nLoadFromMemoryCard expected the vault data to be cleared at this point ?",pScript->GetScriptInfo()));
			spVaultData=new Script::CStruct;
			spVaultData->AppendStructure(pMemCardStuff);
			sVaultDataType=file_type;

            if ( file_type == CRCD(0x61a1bc57,"cat") )
            {
                // load Created trick in edit slot so we can get some info out of it for uploading
                s_read_game_save_info(file_type, pMemCardStuff, pScript);
            }
		}
		else
		{
			// process the data according to the type of file loaded.
			s_read_game_save_info(file_type, pMemCardStuff, pScript);
		}
				
		loaded_ok=true;
	}	
	else
	{
		// Corrupted data, or bad version number
		pScript->GetParams()->AddChecksum(NONAME,GenerateCRC("CorruptedData"));
	}	

	// if the loading was successful,
	// then run some post-load functions
	if ( loaded_ok )
	{
		if (load_for_upload)
		{
			// If the file was loaded for upload to the net, then do not do any post processing,
			// since the data has just been stored away for retrieval later.
		}
		else
		{
			Script::RunScript( "post_load_from_memory_card", pParams );
		}	
	}

ERROR:	
	// Cleanup and unpause the music.
	if (p_file)
	{
		p_file->Close();
		delete p_file;
	}

	if (p_temp)
	{
		Mem::Free(p_temp);
	}

	delete pSummaryInfo;
	delete pMemCardStuff;
	if ( Config::GetHardware() != Config::HARDWARE_XBOX)
	{
		Pcm::PauseMusic( -1 );
		Pcm::PauseStream( -1 );
	}
		
	return loaded_ok;
}

bool ScriptLoadedCustomSkater(Script::CStruct *pParams, Script::CScript *pScript)
{
	return s_did_apply_custom_skater_info;
}	

// Functions required for when loading a file of mem card for uploading to the net.
bool ScriptGetMemCardDataForUpload(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_MsgAssert(spVaultData,("\n%s\nNo mem card data present",pScript->GetScriptInfo()));
	
	pScript->GetParams()->AddStructure(CRCD(0x6d2ab6a,"DataForUpload"),spVaultData);
	pScript->GetParams()->AddChecksum(CRCD(0x7321a8d6,"Type"),sVaultDataType);
	return true;
}	

bool ScriptClearMemCardDataForUpload(Script::CStruct *pParams, Script::CScript *pScript)
{
	if (spVaultData)
	{
		delete spVaultData;
		spVaultData=NULL;
	}	
	sVaultDataType=0;
	return true;
}	

bool ScriptNeedToLoadReplayBuffer(Script::CStruct *pParams, Script::CScript *pScript)
{
#if __USE_REPLAYS__
	return sNeedToLoadReplayBuffer;
#else
	return false;
#endif
}

bool ScriptLoadReplayData(Script::CStruct *pParams, Script::CScript *pScript)
{
#if __USE_REPLAYS__
	Dbg_MsgAssert(!Config::Bootstrap(),("Can't use memory card from bootstrap demo"));

	Pcm::PauseMusic(true);
	Pcm::PauseStream(true);
	
	Dbg_MsgAssert(sNeedToLoadReplayBuffer,("Called LoadReplayBuffer when sNeedToLoadReplayBuffer is false"));

	int replay_buffer_size=0;
	Replay::SReplayDataHeader header;
	Replay::SSavedDummy saved_dummy;
	uint32 replay_data_checksum=0xffffffff; // Initialise the checksum, which is calculated accumulatively.
	uint32 ch=0;	
	bool loaded_ok=false;
	int bytes_read=0;
	Mc::File* p_file=NULL;
	uint8 *p_chunk=NULL;
	int num_chunks=0;
	int offset=0;
	
	// Get the card.
	Mc::Manager * mc_man = Mc::Manager::Instance();
	Mc::Card* p_card=mc_man->GetCard(0,0);
	if (!p_card)
	{
		goto ERROR;
	}
	
	// GameCube often crashes if try to do card operations on a bad card, so do this check first.
	if (!p_card->IsFormatted())
	{
		goto ERROR;
	}

	// Open the file.
	p_file=p_card->Open( spReplayCardFileName, Mc::File::mMODE_READ );
	if (!p_file)
	{
		// File could not be opened
		goto ERROR;
	}	
	
	// File opened OK

	// Seek to the start of the replay data by reading in the header, reading out the start data
	// size from the header, then seeking there.	
	p_file->Seek( 0 ,Mc::File::BASE_START);
	if (p_file->Read((uint8*)&sFileHeader, sizeof(SMcFileHeader)) != sizeof(SMcFileHeader))
	{
		// Some sort of read error.
		goto ERROR;
	}		
	p_file->Seek(sFileHeader.mDataSize,Mc::File::BASE_START);

	// Make sure the buffer does exist
	Replay::AllocateBuffer();
	// and that it is cleared to start with.
	Replay::ClearBuffer();
			
	// Read in the header			
	bytes_read=p_file->Read( (uint8*)&header, sizeof(Replay::SReplayDataHeader) );
	if (bytes_read!=sizeof(Replay::SReplayDataHeader))
	{
		// Some sort of read error.
		goto ERROR;
	}
	Replay::ReadReplayDataHeader(&header);
	replay_data_checksum=Crc::UpdateCRC((const char *)&header,sizeof(Replay::SReplayDataHeader),replay_data_checksum);
	
	// Read in the dummies
	for (int i=0; i<header.mNumStartStateDummies; ++i)
	{
		bytes_read=p_file->Read( (uint8*)&saved_dummy, sizeof(Replay::SSavedDummy) );
		if (bytes_read!=sizeof(Replay::SSavedDummy))
		{
			// Some sort of read error.
			goto ERROR;
		}
		replay_data_checksum=Crc::UpdateCRC((const char *)&saved_dummy,sizeof(Replay::SSavedDummy),replay_data_checksum);
		
		Replay::CreateDummyFromSaveData(&saved_dummy);
	}
			
	// Read the data in a chunk at a time.
	replay_buffer_size=Replay::GetBufferSize();
	Dbg_MsgAssert((replay_buffer_size%REPLAY_BUFFER_CHUNK_SIZE)==0,("Replay buffer size not a multiple of REPLAY_BUFFER_CHUNK_SIZE"));
	
	p_chunk=Replay::GetTempBuffer();
	num_chunks=replay_buffer_size/REPLAY_BUFFER_CHUNK_SIZE;
	
	offset=0;
	for (int i=0; i<num_chunks; ++i)
	{
		bytes_read=p_file->Read( p_chunk, REPLAY_BUFFER_CHUNK_SIZE );
		if (bytes_read!=REPLAY_BUFFER_CHUNK_SIZE)
		{
			// Some sort of read error.
			goto ERROR;
		}
		Replay::WriteIntoBuffer(p_chunk,offset,REPLAY_BUFFER_CHUNK_SIZE);
		replay_data_checksum=Crc::UpdateCRC((const char *)p_chunk,REPLAY_BUFFER_CHUNK_SIZE,replay_data_checksum);
		
		offset+=REPLAY_BUFFER_CHUNK_SIZE;
	}

	// Read in and check the checksum
	bytes_read=p_file->Read( (uint8*)&ch, 4 );
	if (bytes_read!=4)
	{
		// Some sort of read error.
		goto ERROR;
	}
	if (ch != replay_data_checksum)
	{
		goto ERROR;
	}	
		
	// Hooray!	
	loaded_ok=true;

ERROR:	
	sNeedToLoadReplayBuffer=false;
	
	if (p_file)
	{
		p_file->Close();
		delete p_file;
	}

	Pcm::PauseMusic( -1 );
	Pcm::PauseStream( -1 );
	return loaded_ok;
#else
	return false;
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetMaxTHPS4FilesAllowed(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->AddInteger("MaxTHPS4FilesAllowed",MAX_THPS4_FILES_ALLOWED);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static uint8 spSummaryInfoBuffer[sizeof(SMcFileHeader)+MAX_SUMMARY_INFO_SIZE];
bool ScriptGetMemCardDirectoryListing(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->RemoveComponent("DirectoryListing");
	pScript->GetParams()->RemoveComponent("FilesLimitReached");
	pScript->GetParams()->RemoveComponent("TotalTHPS4FilesOnCard");

	uint32 file_type_to_list=0;
	pParams->GetChecksum("FileType",&file_type_to_list);

	// Get a directory listing of the whole card.
	Mc::Manager * p_mc_man = Mc::Manager::Instance();
	Mc::Card* p_card=p_mc_man->GetCard(0,0);
	if (!p_card)
	{
		return true;
	}	

	Lst::Head< Mc::File > file_list;
	p_card->GetFileList( "*", file_list );

	// If the special pools exist, switch to them so that the components & structs etc get allocated off them.
	// The special pools use the space freed by unloading the skater anims, and are for preventing memory
	// overflows when the save size gets large.
	Dbg_MsgAssert(CComponent::SGetCurrentPoolIndex()==0,("Bad current CComponent pool"));
	Dbg_MsgAssert(CStruct::SGetCurrentPoolIndex()==0,("Bad current CStruct pool"));
	bool got_special_pools=false;
	
	CComponent::SSwitchToNextPool();
	if (CComponent::SPoolExists())
	{
		CStruct::SSwitchToNextPool();
		Dbg_MsgAssert(CStruct::SPoolExists(),("No special CStruct pool ?"));
		got_special_pools=true;
	}
	else
	{
		CComponent::SSwitchToPreviousPool();
	}	

	
	int num_files_added=0;
	CStruct *pp_structs[MAX_THPS4_FILES_ALLOWED];
	
	// Run through all the files (well, directory names actually) and compare their
	// first 12 characters with the THPS4 header.	
	const char *p_header=Config::GetMemCardHeader();
	int header_len=strlen(p_header);
	
	int num_files=file_list.CountItems();
	int total_thps4_files=0;
	for (int i=0; i<num_files; ++i)
	{
		Mc::File *p_file=file_list.GetItem(i)->GetData();
		
		bool is_THPS4_file=true;
		
		// No need to check the file name if it's the XBox because all the files
		// will be in our filespace.
		if ( Config::GetHardware() != Config::HARDWARE_XBOX )
		{
			for (int j=0; j<header_len; ++j)
			{
				if (p_file->m_Filename[j]!=p_header[j])
				{
					is_THPS4_file=false;
					break;
				}
			}
		}	
		
		if (!is_THPS4_file)
		{
			continue;
		}
		
		++total_thps4_files;
		
		// It is a THPS4 file, so compare the file type with that requested.
		uint32 file_type=0;
		
		
		switch (Config::GetHardware())
		{
		case Config::HARDWARE_PS2:
		case Config::HARDWARE_PS2_PROVIEW:
		case Config::HARDWARE_PS2_DEVSYSTEM:
		case Config::HARDWARE_NGC:
			// The filetype is indicated by the last letter of the 8 characters
			// that follow the header. The other 7 contain the ascii checksum of
			// the full file name as entered by the user.
			file_type=s_determine_file_type(p_file->m_Filename[header_len+8-1]);
			break;
		case Config::HARDWARE_XBOX:
		{
			// TODO: Remove this ifdef somehow, needed currently because m_DisplayFilename
			// is only defined for XBox. Shouldn't really have any platform ifdefs in this file.
			#ifdef __PLAT_XBOX__
			int length = strlen( p_file->m_DisplayFilename );
			if( stricmp( &p_file->m_DisplayFilename[length - 15], "NetworkSettings" ) == 0 )
			{
				file_type = 0xca41692d; // NetworkSettings
			}
			else if( stricmp( &p_file->m_DisplayFilename[length - 6], "Skater" ) == 0 )
			{
				file_type = 0xb010f357; // OptionsAndPros
			}
			else if( stricmp( &p_file->m_DisplayFilename[length - 4], "Park" ) == 0 )
			{
				file_type = 0x3bf882cc; // Park
			}
			else if( stricmp( &p_file->m_DisplayFilename[length - 6], "Skater" ) == 0 )
			{
				file_type = 0xffc529f4; // Cas
			}
            else if( stricmp( &p_file->m_DisplayFilename[length - 5], "Trick" ) == 0 )
			{
				file_type = 0x61a1bc57; // CAT
			}
			else if( stricmp( &p_file->m_DisplayFilename[length - 6], "Replay" ) == 0 )
			{
				file_type = 0x26c80b0d; // Replay
			}
			else if( stricmp( &p_file->m_DisplayFilename[length - 5], "Goals" ) == 0 )
			{
				file_type = 0x62896edf; // CreatedGoals
			}
			#endif // #ifdef __PLAT_XBOX__
			break;
		}	
		case Config::HARDWARE_WIN32:
			break;
		default:
			break;
		}
		
		
		
		if ( (file_type_to_list==0 && file_type!=0) || 
			 file_type==file_type_to_list)
		{
			if (num_files_added < MAX_THPS4_FILES_ALLOWED)
			{
				// It's safe to add a new entry to the array.
				CStruct *p_struct=new CStruct;
				p_struct->AddChecksum("file_type",file_type);
				p_struct->AddInteger("Index",num_files_added);


				p_struct->AddInteger("Year",p_file->m_Modified.m_Year);
				p_struct->AddInteger("Month",p_file->m_Modified.m_Month);
				p_struct->AddInteger("Day",p_file->m_Modified.m_Day);
				p_struct->AddInteger("Hour",p_file->m_Modified.m_Hour);
				p_struct->AddInteger("Minutes",p_file->m_Modified.m_Minutes);
				p_struct->AddInteger("Seconds",p_file->m_Modified.m_Seconds);

				
				char p_card_file_name[100];
				if (Config::GetHardware()==Config::HARDWARE_NGC)
				{
					sprintf(p_card_file_name,"%s",p_file->m_Filename);
				}	
				else
				{
					sprintf(p_card_file_name,"/%s/%s",p_file->m_Filename,p_file->m_Filename);
				}	
				Dbg_MsgAssert(strlen(p_card_file_name)<100,("Oops"));
				
				// Store the low level file name in the structure too so that the scripts
				// can pass it on to the file delete function. This is necessary because
				// even though the low level file name can be derived from the file name as
				// stored in the file itself, if the file is corrupted the name might be too,
				// so in that case it would not be possible to delete it.
				p_struct->AddString("actual_file_name",p_card_file_name);
				
				#ifdef __PLAT_XBOX__
				p_struct->AddString("xbox_directory_name",p_file->m_DisplayFilename);
				#endif

				// Need to open the file to get the summary info out of it ...
				Mc::File* p_mc_file=p_card->Open( p_card_file_name, Mc::File::mMODE_READ );
				if (p_mc_file)
				{   
					// File opened OK, so grab the first hundred or so bytes.
					p_mc_file->Seek( 0 ,Mc::File::BASE_START);
					p_mc_file->Read( spSummaryInfoBuffer, sizeof(SMcFileHeader)+MAX_SUMMARY_INFO_SIZE );
					
					SMcFileHeader *p_file_header=(SMcFileHeader*)spSummaryInfoBuffer;
					uint8 *p_summary_info=(uint8*)(p_file_header+1);
					
					// Determine whether the summary info is corrupted.
					bool corrupt_summary_info=false;
					if (p_file_header->mSummaryInfoSize > MAX_SUMMARY_INFO_SIZE)
					{
						// The mSummaryInfoSize itself is corrupted!
						corrupt_summary_info=true;
					}
					else
					{
						uint32 calculated_summary_info_checksum=Crc::GenerateCRCCaseSensitive((const char *)p_summary_info,p_file_header->mSummaryInfoSize);
						if (calculated_summary_info_checksum != p_file_header->mSummaryInfoChecksum)
						{
							corrupt_summary_info=true;
						}
					}		

					// Calculate the total file size
					int file_size=p_mc_file->Seek(0,Mc::File::BASE_END);
					//printf("Directory listing: %s, size=%d\n",p_card_file_name,file_size);
					int total_size_on_card=s_calculate_total_space_used_on_card(file_type,file_size);
					p_struct->AddInteger("total_file_size",total_size_on_card);

					
					// Removed the file size check, because the fixed size needed to be updated, and keeping the
					// check would prevent existing parks from being able to be loaded.
					//if (corrupt_summary_info || file_size != (int)sGetFixedFileSize(file_type))
					if (corrupt_summary_info)
					{
						p_struct->AddChecksum(NONAME,Script::GenerateCRC("Corrupt"));
						if (Config::GetHardware()==Config::HARDWARE_NGC)
						{
							p_struct->AddString("filename",GetString("NGCDamagedFile"));
						}
						else
						{
							p_struct->AddString("filename",GetString("DamagedFile"));
						}	
					}
					else if (p_file_header->mVersion!=s_get_version_number(file_type))
					{
						#ifdef __NOPT_ASSERT__
						p_struct->AddChecksum(NONAME,Script::GenerateCRC("BadVersion"));
						p_struct->AddString("filename",GetString("BadVersionNumber"));
						#else
						p_struct->AddChecksum(NONAME,Script::GenerateCRC("Corrupt"));
						if (Config::GetHardware()==Config::HARDWARE_NGC)
						{
							p_struct->AddString("filename",GetString("NGCDamagedFile"));
						}
						else
						{
							p_struct->AddString("filename",GetString("DamagedFile"));
						}	
						#endif
					}
					else
					{
						// Extract the summary info and stick it in the structure.
						// This summary info gets printed at the bottom of the files menu when
						// the highlight is on the file.
						// The summary info is actually in the main structure stored in the file,
						// but that would require loading in the whole thing which would take a
						// long time.
						
						// Have to use a temporary structure because ReadFromBuffer clears the
						// passed structure first.
						CStruct *p_temp=new CStruct;
						ReadFromBuffer(p_temp,p_summary_info);
						p_struct->AppendStructure(p_temp);
						delete p_temp;
					}
					
					p_mc_file->Close();
					delete p_mc_file;
				}
				else
				{
					// If the file did not open at all, treat it as corrupted.
					p_struct->AddChecksum(NONAME,Script::GenerateCRC("Corrupt"));
					p_struct->AddString("filename",GetString("DamagedFile"));
				}	
					
				pp_structs[num_files_added++]=p_struct;
			}	
		}
	}	

	if (got_special_pools)
	{
		// Note: The pools will contain stuff at this point. The script will delete them
		// when it does its cleanup.
		CComponent::SSwitchToPreviousPool();
		CStruct::SSwitchToPreviousPool();
	}
	
	// The file_list will probably get cleaned up when it goes out of scope, but just to be sure ...
	file_list.DestroyAllNodes();

	pScript->GetParams()->AddInteger("TotalTHPS4FilesOnCard",total_thps4_files);
	
	// Set the FilesLimitReached flag so that the script can determine whether to add
	// the 'Create New' entry to the files menu.
	if (total_thps4_files >= MAX_THPS4_FILES_ALLOWED)
	{
		pScript->GetParams()->AddChecksum(NONAME,0x4eec27b5/*FilesLimitReached*/);
	}
	
	// If files were found, add the array to the script's params.
	if (num_files_added)
	{
		// First, sort the array so that the files are definitely in date order (TT6112)
		while (true)
		{
			bool did_a_swap=false;
			
			for (int i=0; i<num_files_added-1; ++i)
			{
				if (s_first_date_is_more_recent(pp_structs[i+1],pp_structs[i]))
				{
					CStruct *p_temp=pp_structs[i];
					pp_structs[i]=pp_structs[i+1];
					pp_structs[i+1]=p_temp;
					did_a_swap=true;
				}	
			}
			
			if (!did_a_swap)
			{
				break;
			}	
		}	
	
	
		CArray *p_directory_listing_array=new CArray;
		p_directory_listing_array->SetSizeAndType(num_files_added,ESYMBOLTYPE_STRUCTURE);
		
		for (int f=0; f<num_files_added; ++f)
		{
			p_directory_listing_array->SetStructure(f,pp_structs[f]);
		}
		
		pScript->GetParams()->AddArrayPointer("DirectoryListing",p_directory_listing_array);
		
		// Note: Not deleting the pp_structs[], cos they've been given to the array.
		// Not deleting p_directory_listing_array, because it's been given to the scripts params.
	}
		
	return true;
}

// Returns true if the sector size is 8K, false otherwise.
// Needed as part of the GameCube card check procedure to check that the card is not some weird type.
// If it's a PS2 build, it will just return true.
bool ScriptSectorSizeOK(Script::CStruct *pParams, Script::CScript *pScript)
{
	#ifdef __PLAT_NGC__
	Spt::SingletonPtr< Mc::Manager > mc_man;
	Mc::Card* pCard=mc_man->GetCard(0,0);
	if (pCard)
	{
		if (pCard->m_sector_size==8192)
		{
			return true;
		}
		else
		{
			return false;
		}
	}		
	return false;
	#else
	return true;
	#endif
}

bool ScriptCardIsDamaged(Script::CStruct *pParams, Script::CScript *pScript)
{
	#ifdef __PLAT_NGC__
	Spt::SingletonPtr< Mc::Manager > mc_man;
	Mc::Card* pCard=mc_man->GetCard(0,0);
	if (pCard)
	{
		return pCard->m_broken;
	}
	else
	{
		if (mc_man->GotFatalError())
		{
			return true;
		}
	}	
	return false;
	#else
	return false;
	#endif
}

bool ScriptCardIsForeign(Script::CStruct *pParams, Script::CScript *pScript)
{
	#ifdef __PLAT_NGC__
	Spt::SingletonPtr< Mc::Manager > mc_man;
	Mc::Card* pCard=mc_man->GetCard(0,0);
	if (pCard)
	{
		return pCard->IsForeign();
	}
	return false;
	#else
	return false;
	#endif
}

bool ScriptBadDevice(Script::CStruct *pParams, Script::CScript *pScript)
{
	#ifdef __PLAT_NGC__
	Spt::SingletonPtr< Mc::Manager > mc_man;
	Mc::Card* pCard=mc_man->GetCard(0,0);
	if (pCard)
	{
		return false;
	}
	else
	{
		if (mc_man->GotWrongDevice())
		{
			return true;
		}
	}	
	return false;
	#else
	return false;
	#endif
}

// For debugging, so that we can use the script debugger to view the contents of the save structure that would
// get saved out to the mem card.
bool ScriptGetSaveInfo(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 file_type=0;
	pParams->GetChecksum(CRCD(0x7321a8d6,"type"),&file_type);

	Script::CStruct *p_main_structure=new Script::CStruct;
	s_insert_game_save_info(file_type, p_main_structure);
	uint32 structure_size=CalculateBufferSize(p_main_structure);
	
	Script::CStruct *p_summary_info=new Script::CStruct;
	s_generate_summary_info(p_summary_info, file_type, p_main_structure);
	uint32 summary_info_size=CalculateBufferSize(p_summary_info);

	
	pScript->GetParams()->AddInteger(CRCD(0x172c1344,"MainStructureSize"),structure_size);
	pScript->GetParams()->AddInteger(CRCD(0x96635475,"SummaryInfoSize"),summary_info_size);
	pScript->GetParams()->AddInteger(CRCD(0x6afd2f7f,"MaxSummaryInfoSize"),MAX_SUMMARY_INFO_SIZE);
	pScript->GetParams()->AddInteger(CRCD(0xecbb8262,"TotalSize"),sizeof(SMcFileHeader)+summary_info_size+structure_size);
	pScript->GetParams()->AddInteger(CRCD(0xb35eb1d1,"MaxTotalSize"),sGetFixedFileSize(file_type));
	
	pScript->GetParams()->AddStructurePointer(CRCD(0x703e60ca,"SummaryInfo"),p_summary_info);
	pScript->GetParams()->AddStructurePointer(CRCD(0x671c9c00,"MainStructure"),p_main_structure);
	
	return true;
}

// It is safe to call this multiple times.
bool ScriptCreateTemporaryMemCardPools(Script::CStruct *pParams, Script::CScript *pScript)
{
	// If the pools exist already, do nothing.
	CComponent::SSwitchToNextPool();
	if (CComponent::SPoolExists())
	{
		CComponent::SSwitchToPreviousPool();
		
		CStruct::SSwitchToNextPool();								 
		Dbg_MsgAssert(CStruct::SPoolExists(),("Expected CStruct pool to exist"));
		CStruct::SSwitchToPreviousPool();

		CVector::SSwitchToNextPool();								 
		Dbg_MsgAssert(CVector::SPoolExists(),("Expected CVector pool to exist"));
		CVector::SSwitchToPreviousPool();
		
		return true;
	}	
	CComponent::SSwitchToPreviousPool();
	
#ifdef __PLAT_NGC__
#define NUM_COM 16000
#define NUM_STR 8000
#define NUM_VEC 3000

#define BUFFER_SIZE ( ( NUM_COM * 16 ) + ( NUM_STR * 8 ) + ( NUM_VEC * 16 ) )

	// Time for a hack...
	g_mc_hack = true;

	g_hack_address = NsARAM::alloc( BUFFER_SIZE );
	if ( g_hack_address )
	{
		NsDMA::toARAM( g_hack_address, g_p_buffer, BUFFER_SIZE );
	}

	memset( g_p_buffer, 0, BUFFER_SIZE );

	NsBuffer::reset( false );
#else
#define NUM_COM 50000
#define NUM_STR 50000
#define NUM_VEC 10000
#endif		// __PLAT_NGC__

		
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

	Mem::PushMemProfile("Mem card CComponent");
	Dbg_MsgAssert(CComponent::SGetCurrentPoolIndex()==0,("Bad current CComponent pool"));
	CComponent::SSwitchToNextPool();
	CComponent::SCreatePool(NUM_COM, "Reserve CComponent");
	CComponent::SSwitchToPreviousPool();
	Mem::PopMemProfile();
										 
	Mem::PushMemProfile("Mem card CStruct");
	Dbg_MsgAssert(CStruct::SGetCurrentPoolIndex()==0,("Bad current CStruct pool"));
	CStruct::SSwitchToNextPool();
	CStruct::SCreatePool(NUM_STR, "Reserve CStruct");
	CStruct::SSwitchToPreviousPool();
	Mem::PopMemProfile();
	
	// 12 bytes each  (Actually 16)
	Mem::PushMemProfile("Mem card CVector");
	Dbg_MsgAssert(CVector::SGetCurrentPoolIndex()==0,("Bad current CVector pool"));
	CVector::SSwitchToNextPool();
	CVector::SCreatePool(NUM_VEC, "Reserve CVector");
	CVector::SSwitchToPreviousPool();
	Mem::PopMemProfile();

	Mem::Manager::sHandle().PopContext();

	return true;
}

// It is safe to call this multiple times.
bool ScriptRemoveTemporaryMemCardPools(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_MsgAssert(CComponent::SGetCurrentPoolIndex()==0,("Bad current CComponent pool"));
	CComponent::SSwitchToNextPool();
	CComponent::SRemovePool(); // Does nothing if the pool does not exist.
	CComponent::SSwitchToPreviousPool();
	
	Dbg_MsgAssert(CStruct::SGetCurrentPoolIndex()==0,("Bad current CStruct pool"));
	CStruct::SSwitchToNextPool();
	CStruct::SRemovePool();
	CStruct::SSwitchToPreviousPool();
		
	Dbg_MsgAssert(CVector::SGetCurrentPoolIndex()==0,("Bad current CVector pool"));
	CVector::SSwitchToNextPool();
	CVector::SRemovePool();
	CVector::SSwitchToPreviousPool();
	
#ifdef __PLAT_NGC__
	if ( g_hack_address )
	{
		NsDMA::toMRAM( g_p_buffer, g_hack_address, BUFFER_SIZE );
	}
	NsARAM::free( g_hack_address );

	g_mc_hack = false;
	NsBuffer::reset( true );
#endif		// __PLAT_NGC__

	return true;
}

bool ScriptSwitchToTempPoolsIfTheyExist(Script::CStruct *pParams, Script::CScript *pScript)
{
	Dbg_MsgAssert(CComponent::SGetCurrentPoolIndex()==0 && CStruct::SGetCurrentPoolIndex()==0 && CVector::SGetCurrentPoolIndex()==0, ("Expected current pools to be 0"));
	CComponent::SSwitchToNextPool();
	CStruct::SSwitchToNextPool();
	CVector::SSwitchToNextPool();
	if (CComponent::SPoolExists() && CStruct::SPoolExists() && CVector::SPoolExists())
	{
		return true;
	}
	CComponent::SSwitchToPreviousPool();
	CStruct::SSwitchToPreviousPool();
	CVector::SSwitchToPreviousPool();

	return false;
}

bool ScriptSwitchToRegularPools(Script::CStruct *pParams, Script::CScript *pScript)
{
	if (CComponent::SGetCurrentPoolIndex()==0 && 
		CStruct::SGetCurrentPoolIndex()==0 &&
		CVector::SGetCurrentPoolIndex()==0)
	{
		return true;
	}
		
	CComponent::SSwitchToPreviousPool();
	CStruct::SSwitchToPreviousPool();
	CVector::SSwitchToPreviousPool();
	return true;
}

// Saves any old data file to mem card
bool SaveDataFile(const char *p_name, uint8 *p_data, uint32 size)
{
	Dbg_MsgAssert(p_name,("NULL p_name"));
	Dbg_MsgAssert(p_data,("NULL p_data"));

	switch (Config::GetHardware())
	{
		case Config::HARDWARE_PS2:
		case Config::HARDWARE_PS2_PROVIEW:
		case Config::HARDWARE_PS2_DEVSYSTEM:
			break;
		default:
			return false;
			break;
	}
				
	if ( Config::GetHardware() != Config::HARDWARE_XBOX)
	{
		Pcm::PauseMusic(true);
		Pcm::PauseStream(true);
	}
	
	Mc::File* pFile=NULL;
	char p_card_file_name[MAX_CARD_FILE_NAME_CHARS+1];
	const char *p_header=Config::GetMemCardHeader();
	char p_ascii_checksum[20];
	int suffix=0;
	#define FULL_NAME_BUF_SIZE 100
	char p_full_name[FULL_NAME_BUF_SIZE];
	bool SavedOK=false;
	uint32 CardBytesWritten=0;


	Mc::Manager * mc_man = Mc::Manager::Instance();
	Mc::Card* p_card=mc_man->GetCard(0,0);
	if (!p_card)
	{
		goto ERROR;
	}

	while (true)
	{
		Dbg_MsgAssert(strlen(p_name)<FULL_NAME_BUF_SIZE-3,("p_name too long"));
		sprintf(p_full_name,"%s%03d",p_name,suffix);
		s_generate_ascii_checksum(p_ascii_checksum,p_full_name,0xb010f357/*OptionsAndPros*/);
		sprintf(p_card_file_name,"/%s%s/%s%s",	p_header,p_ascii_checksum,
												p_header,p_ascii_checksum);
		Dbg_MsgAssert(strlen(p_card_file_name)<MAX_CARD_FILE_NAME_CHARS,("Oops"));

		pFile = p_card->Open( p_card_file_name, Mc::File::mMODE_READ );
		if (pFile)
		{
			pFile->Flush();
			pFile->Close();
			delete pFile;
			pFile=NULL;
		}
		else
		{
			break;
		}
		++suffix;
		Dbg_MsgAssert(suffix<1000,("Too many files!!"));
	}		

	printf ("Creating mem card file '%s'\n",p_full_name);
	
	if (!s_make_ps2_dir_and_icons(p_card,
								  0xb010f357, // OptionsAndPros tee hee
								  p_full_name,
								  p_card_file_name,
								  &s_insufficient_space))
	{
		goto ERROR;
	}

	// Open a file big enough to hold all data
	pFile=p_card->Open( p_card_file_name, Mc::File::mMODE_WRITE | Mc::File::mMODE_CREATE, s_round_up_to_platforms_block_size(size) );
	s_insufficient_space=p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE;

	if (!pFile)
	{
		goto ERROR;
	}

	CardBytesWritten=pFile->Write( p_data, size );
	s_insufficient_space=p_card->GetLastError()==Mc::Card::vINSUFFICIENT_SPACE;
	if (CardBytesWritten!=size)
	{
		goto ERROR;
	}

	SavedOK=true;
	
ERROR:	
	if (pFile)
	{
		if (!pFile->Flush())
		{
			SavedOK=false;
		}	
		if (!pFile->Close())
		{
			SavedOK=false;
		}	
		delete pFile;
	}	

	if ( Config::GetHardware() != Config::HARDWARE_XBOX)
	{
		Pcm::PauseMusic( -1 );
		Pcm::PauseStream( -1 );
	}
		
	return SavedOK;
}



// Load len bytes of any old data file to mem card to p_data
bool LoadDataFile(const char *p_name, uint8 *p_data, uint32 size)
{
	Dbg_MsgAssert(p_name,("NULL p_name"));
	Dbg_MsgAssert(p_data,("NULL p_data"));

	switch (Config::GetHardware())
	{
		case Config::HARDWARE_PS2:
		case Config::HARDWARE_PS2_PROVIEW:
		case Config::HARDWARE_PS2_DEVSYSTEM:
			break;
		default:
			return false;
			break;
	}
				
	if ( Config::GetHardware() != Config::HARDWARE_XBOX)
	{
		Pcm::PauseMusic(true);
		Pcm::PauseStream(true);
	}
	
	Mc::File* pFile=NULL;
	char p_card_file_name[MAX_CARD_FILE_NAME_CHARS+1];
	const char *p_header=Config::GetMemCardHeader();
	char p_ascii_checksum[20];
	#define FULL_NAME_BUF_SIZE 100
	char p_full_name[FULL_NAME_BUF_SIZE];

	Mc::Manager * mc_man = Mc::Manager::Instance();
	Mc::Card* p_card=mc_man->GetCard(0,0);
	if (!p_card)
	{
		return false;;
	}

	Dbg_MsgAssert(strlen(p_name)<FULL_NAME_BUF_SIZE-3,("p_name too long"));
	sprintf(p_full_name,"%s",p_name);
	s_generate_ascii_checksum(p_ascii_checksum,p_full_name,0xb010f357/*OptionsAndPros*/);
	sprintf(p_card_file_name,"/%s%s/%s%s",	p_header,p_ascii_checksum,
											p_header,p_ascii_checksum);
	Dbg_MsgAssert(strlen(p_card_file_name)<MAX_CARD_FILE_NAME_CHARS,("Oops"));

	pFile = p_card->Open( p_card_file_name, Mc::File::mMODE_READ );
	if (pFile)
	{
		printf ("Loading From Memory Card\n");
		pFile->Flush();
		pFile->Seek( 0 ,Mc::File::BASE_START);
		pFile->Read(p_data,size);
		pFile->Flush();
		pFile->Close();
		delete pFile;
		return true;
	}
	return false;
}



} // namespace CFuncs

