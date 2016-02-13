//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       PlayerProfileManager.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  4/29/2002
//****************************************************************************

#ifndef __OBJECTS_PLAYERPROFILEMANAGER_H
#define __OBJECTS_PLAYERPROFILEMANAGER_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/lookuptable.h>
                               
#include <sk/gamenet/gamenet.h>
                                              
#include <sk/objects/skaterprofile.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Obj
{

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

class CPlayerProfileManager : public Spt::Class
{
	enum
	{
		vMAX_PROFILES = 32,
		vMAX_PLAYERS = GameNet::vMAX_LOCAL_CLIENTS,

		// this is the biggest packet we can reasonably send
		// over a network packet (it's not really based on 
		// the max packet size, but rather the biggest packet
		// that we've seen work so far...  the max packet size
		// specified in net.h seems to be too aggressive)
		vMAX_PRACTICAL_APPEARANCE_DATA_SIZE = 1150,

		// this is just the size of a temp buffer
		vMAX_APPEARANCE_BUFFER_SIZE = 4096,

		// temporary profiles, used for remembering/restoring
		// skater profiles (like for credits)
		vMAX_TEMPORARY_PROFILES = 3,
	};
	
	public:
		CPlayerProfileManager();
		~CPlayerProfileManager();

	public:
		void					Init();

		// these deal with players (0 to numLocalClients)
		void					SetCurrentProfileIndex(int i);
		int						GetCurrentProfileIndex();
		void					ApplyTemplateToCurrentProfile(uint32 checksum);
		CSkaterProfile*			GetCurrentProfile();
		CSkaterProfile*			GetProfile(int i);
//		bool					GetGlobalFlag(int flag);
		void					SyncPlayer2();

		// these deal with templates (0 to numPros)
		CSkaterProfile*			GetProfileTemplate(uint32 checksum);
		CSkaterProfile*			GetProfileTemplateByIndex(int i);

		// for populating front end menus
		uint32					GetNumProfileTemplates();
		uint32					GetProfileTemplateChecksum(int i);
		
		void					AddAllProProfileInfo(Script::CStruct *pStuff);
		void					LoadAllProProfileInfo(Script::CStruct *pStuff);
		void					AddCASProfileInfo(Script::CStruct *pStuff);
		void					LoadCASProfileInfo(Script::CStruct *pStuff, bool load_info);
		
		void					Reset();
		bool					AddNewProfile(Script::CStruct* pParams);
		void					LockCurrentSkaterProfileIndex( bool locked );
		
		bool					AddTemporaryProfile(uint32 checksum);
		CSkaterProfile*			GetTemporaryProfile(uint32 checksum);

	protected:
		Lst::LookupTable<CSkaterProfile>	m_Profiles;
		CSkaterProfile*			mp_CurrentProfile[GameNet::vMAX_LOCAL_CLIENTS];
		int						m_CurrentProfileIndex;
		bool					m_LockCurrentSkaterProfileIndex;
		
		int						m_TemporaryProfileCount;
		uint32					m_TemporaryProfileChecksums[vMAX_TEMPORARY_PROFILES];
		CSkaterProfile*			mp_TemporaryProfiles[vMAX_TEMPORARY_PROFILES];
};

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/


} // namespace Obj

#endif	// __OBJECTS_PLAYERPROFILEMANAGER_H
