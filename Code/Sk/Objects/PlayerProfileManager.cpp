//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       PlayerProfileManager.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  4/29/2002
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/objects/playerprofilemanager.h>

#include <sk/objects/skaterprofile.h>

#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/array.h>
#include <gel/scripting/checksum.h>

#include <gfx/facetexture.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Obj
{

// Selects which one is the current player profile

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/
	
/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPlayerProfileManager::CPlayerProfileManager() : m_Profiles(vMAX_PROFILES)
{
	m_CurrentProfileIndex = 0;

	for ( int i = 0; i < vMAX_PLAYERS; i++ )
	{
		mp_CurrentProfile[i] = NULL;
	}

	m_LockCurrentSkaterProfileIndex = false;

	m_TemporaryProfileCount = 0;

#if 0

/*
appearance_custom_skater_worst_case_net_packet = 
{
    body = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    board = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    skater_m_head = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    skater_m_torso = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    skater_m_legs = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    skater_m_hair = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    skater_m_backpack = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    skater_m_jaw = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    skater_m_socks = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    sleeves = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    kneepads = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    elbowpads = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    shoes = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    sleeves = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    front_logo = { desc_id=#"Wrist Bands" uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
    back_logo = { desc_id=#"Wrist Bands" uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
    hat = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    helmet = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
    accessories = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
	hat_logo = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}
	helmet_logo = { desc_id=#"Wrist Bands" h=350 s=99 v=98 use_default_hsv = 1}

	// pos items...
	griptape = { desc_id=#"Wrist Bands" }
	deck_graphic = { desc_id=#"Wrist Bands" }
	cad_graphic = { desc_id=#"Wrist Bands" h=350 s=99 v=98 uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	deck_layer1 = { desc_id=#"Wrist Bands" h=350 s=99 v=98 uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	deck_layer2 = { desc_id=#"Wrist Bands" h=350 s=99 v=98 uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	deck_layer3 = { desc_id=#"Wrist Bands" h=350 s=99 v=98 uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	deck_layer4 = { desc_id=#"Wrist Bands" h=350 s=99 v=98 uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	deck_layer5 = { desc_id=#"Wrist Bands" h=350 s=99 v=98 uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
		
	head_tattoo = { desc_id=#"Wrist Bands" uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	left_bicep_tattoo = { desc_id=#"Wrist Bands" uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	left_forearm_tattoo = { desc_id=#"Wrist Bands" uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	right_bicep_tattoo = { desc_id=#"Wrist Bands" uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	right_forearm_tattoo = { desc_id=#"Wrist Bands" uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	chest_tattoo = { desc_id=#"Wrist Bands" uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	back_tattoo = { desc_id=#"Wrist Bands" uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	left_leg_tattoo = { desc_id=#"Wrist Bands" uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}
	right_leg_tattoo = { desc_id=#"Wrist Bands" uv_u=199.999999 uv_v=199.999999 uv_scale=199.999999 uv_rot=355.000000 use_default_uv = 1}

	headtop_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	Jaw_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	nose_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	head_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	torso_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	stomach_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	upper_arm_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	lower_arm_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	hands_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	upper_leg_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	lower_leg_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	feet_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	board_bone_group = { x=160 y=160 z=160 use_default_scale = 1}
	object_scaling = { x=160 y=160 z=160 use_default_scale = 1}
}
*/

	uint8 tBuf[vMAX_APPEARANCE_BUFFER_SIZE];
	Script::CStruct* pTempStructure = Script::GetStructure("appearance_custom_skater_worst_case_net_packet",Script::ASSERT);
	Obj::CSkaterProfile* pPlayerProfile = new Obj::CSkaterProfile;
	pPlayerProfile->GetAppearance()->Load( pTempStructure, false );	
	uint32 size = pPlayerProfile->WriteToBuffer(tBuf, vMAX_APPEARANCE_BUFFER_SIZE);
	Dbg_Assert( size < vMAX_PRACTICAL_APPEARANCE_DATA_SIZE );
	Dbg_Message("*********** Worst case model appearance data size = %d bytes\n", size);
	delete pTempStructure;
	delete pPlayerProfile;
	Dbg_Assert( 0 );
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPlayerProfileManager::Init()
{
	int dummy;
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());

	// TODO:  this is an optimization to go down from 4-byte
	// checksums to a 1- or 2-byte index into a checksum table.
	// This is probably not a very good system, because it's not
	// scalable to downloadable content
//	Cas::InitDescChecksumTable();
	
	// initialize the pro skater profiles here
	Script::RunScript( "init_pro_skaters" );

	// the number of profiles must exceed the number of players
	Dbg_MsgAssert( m_Profiles.getSize() >= vMAX_PLAYERS, ( "Not enough profiles (must be greater than maxplayers)" ) );

	// profile #0 points to an actual profile
	mp_CurrentProfile[0] = m_Profiles.GetItemByIndex( 0, &dummy );

	// the others point to temp copies...  this is so that player
	// #1 through N can edit their appearances without affecting
	// the source profile
	for ( int i = 1; i < vMAX_PLAYERS; i++ )
	{
		mp_CurrentProfile[i] = new CSkaterProfile;
		*mp_CurrentProfile[i] = *m_Profiles.GetItemByIndex( i, &dummy );
	}

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPlayerProfileManager::~CPlayerProfileManager()
{
	uint32 tableSize = m_Profiles.getSize( );
	for ( uint32 i = 0; i < tableSize; i++ )
	{
		int key;
		CSkaterProfile* pProfile = m_Profiles.GetItemByIndex( 0, &key );
		delete pProfile;
		m_Profiles.FlushItem( key );
	}

	for ( int i = 1; i < vMAX_PLAYERS; i++ )
	{
		Dbg_Assert( mp_CurrentProfile[i] );
		delete mp_CurrentProfile[i];
		mp_CurrentProfile[i] = NULL;
	}

	for ( int i = 0; i < m_TemporaryProfileCount; i++ )
	{
		Dbg_Assert( mp_TemporaryProfiles[i] );
		delete mp_TemporaryProfiles[i];
		mp_TemporaryProfiles[i] = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CPlayerProfileManager::GetNumProfileTemplates( void )
{
	return m_Profiles.getSize();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CPlayerProfileManager::GetProfileTemplateChecksum( int i )
{
	Dbg_MsgAssert( i >= 0 && i < m_Profiles.getSize(),( "Profile %d out of range (0-%d)", i, m_Profiles.getSize()-1 ));

	int key;
	m_Profiles.GetItemByIndex( i, &key );
	return (uint32)key;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterProfile*		CPlayerProfileManager::GetProfileTemplateByIndex( int i)
{
	Dbg_MsgAssert( i >= 0 && i < m_Profiles.getSize(),( "Profile %d out of range (0-%d)", i, m_Profiles.getSize()-1 ));

	return m_Profiles.GetItemByIndex( i );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPlayerProfileManager::SyncPlayer2()
{
	// GJ:  need to push the skaterinfoheap or else the custom skater's face
	// texture may end up on the bottom up heap (you'd only notice that
	// if you went into 2 player mode and both characters were already
	// custom skaters, because pressing LEFT to switch from a pro skater
	// to a custom skater would automatically push the skater info heap)
	// this whole process is probably worth looking at next time because
	// it's kind of kludgy, but it's too risky to fix in any other
	// way right now...
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());

	int profile_index = 1;

	// get the currently selected skater for player #1
	uint32 checksum = mp_CurrentProfile[profile_index]->GetSkaterNameChecksum();

	// sync up the profile from the root profile
	*mp_CurrentProfile[profile_index] = *m_Profiles.GetItem( checksum );
	
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPlayerProfileManager::SetCurrentProfileIndex(int i)
{
	if ( m_LockCurrentSkaterProfileIndex )
	{
		// we lock it so that we make sure no one accesses
		// this during network games or splitscreen games,
		// which aren't guaranteed to return the correct
		// value
		Dbg_MsgAssert( 0, ( "Can't access current profile index! Are you playing a network game? (See Gary)" ) );
	}

	Dbg_MsgAssert( i >= 0 && i < vMAX_PLAYERS,( "Profile %d out of range (0-%d)", i, vMAX_PLAYERS-1 ));
	
	m_CurrentProfileIndex = i;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CPlayerProfileManager::GetCurrentProfileIndex()
{
	if ( m_LockCurrentSkaterProfileIndex )
	{
		// we lock it so that we make sure no one accesses
		// this during network games or splitscreen games,
		// which aren't guaranteed to return the correct
		// value
		Dbg_MsgAssert( 0, ( "Can't access current profile index! Are you playing a network game? (See Gary)" ) );
	}

	return m_CurrentProfileIndex;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPlayerProfileManager::ApplyTemplateToCurrentProfile( uint32 checksum )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());
	if ( m_CurrentProfileIndex == 0 )
	{
		mp_CurrentProfile[m_CurrentProfileIndex] = GetProfileTemplate( checksum );
	}
	else
	{
		*mp_CurrentProfile[m_CurrentProfileIndex] = *GetProfileTemplate( checksum );
	}
	Mem::Manager::sHandle().PopContext();	//Mem::Manager::sHandle().SkaterInfoHeap());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterProfile* CPlayerProfileManager::GetCurrentProfile()
{
	return mp_CurrentProfile[m_CurrentProfileIndex];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterProfile* CPlayerProfileManager::GetProfile( int i )
{
	Dbg_MsgAssert( i >= 0 && i < vMAX_PLAYERS,( "Profile %d out of range (0-%d)", i, vMAX_PLAYERS-1 ));

	return mp_CurrentProfile[i];
}

/******************************************************************/
/*                                                                
	If any of the skater careers have this flag set, it returns true.
	Used by the cassette menu when used for free skate, session etc,
	to find out what levels need to be selectable.
																  */
/******************************************************************/

/*
bool CPlayerProfileManager::GetGlobalFlag(int flag)
{
	Dbg_MsgAssert(0,("CPlayerProfileManager has been deprecated - careers no longer in profiles\n"));
	uint32 tableSize = m_Profiles.getSize( );
	for ( uint32 i = 0; i < tableSize; i++ )
	{
		int key;
		CSkaterProfile* pProfile = m_Profiles.GetItemByIndex( i, &key );
		Dbg_MsgAssert(pProfile,("NULL pProfile"));

		CSkaterCareer *pCareer=pProfile->GetCareer();
		Dbg_MsgAssert(pCareer,("NULL pCareer"));

		if (pCareer->GetGlobalFlag(flag))
		{
			return true;
		}
	}

	return false;
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterProfile* CPlayerProfileManager::GetProfileTemplate( uint32 checksum )
{
	Dbg_MsgAssert( m_Profiles.GetItem( checksum ), ( "Couldn't find profile %s", Script::FindChecksumName(checksum) ) );

	return m_Profiles.GetItem( checksum );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPlayerProfileManager::Reset()
{
	uint32 tableSize = m_Profiles.getSize( );
	for ( uint32 i = 0; i < tableSize; i++ )
	{
		int key;
		CSkaterProfile* pProfile = m_Profiles.GetItemByIndex( i, &key );
		pProfile->Reset();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPlayerProfileManager::AddAllProProfileInfo(Script::CStruct *pStuff)
{
	Dbg_MsgAssert(pStuff,("NULL pStuff"));
	
	uint32 tableSize = m_Profiles.getSize( );
	for ( uint32 i = 0; i < tableSize; i++ )
	{
		int key;
		CSkaterProfile* pProfile = m_Profiles.GetItemByIndex( i, &key );

		// Only add the pro's, not the create-a-skater.
		if (pProfile->IsPro())
		{
			pProfile->WriteIntoStructure(pStuff);
		}
	}
	
	
	Dbg_MsgAssert(mp_CurrentProfile[m_CurrentProfileIndex],("NULL mp_CurrentProfile[%d]",m_CurrentProfileIndex));
	
	uint32 CurrentSkaterNameChecksum = mp_CurrentProfile[m_CurrentProfileIndex]->GetSkaterNameChecksum();
	pStuff->AddComponent(Script::GenerateCRC("CurrentSkater"),ESYMBOLTYPE_NAME,(int)CurrentSkaterNameChecksum);
	
	Str::String DisplayName=mp_CurrentProfile[m_CurrentProfileIndex]->GetUIString("display_name");
	const char *pDisplayName=DisplayName.getString();
	pStuff->AddComponent(Script::GenerateCRC("DisplayName"),ESYMBOLTYPE_STRING,pDisplayName);
	
	/* The CAS file name is no longer needed since the cas info is now in the unified save file.
	
	const char *pFileName=mp_CurrentProfile[m_CurrentProfileIndex]->GetCASFileName();
	pStuff->AddString("CASFileName",pFileName);
	
	// The s_insert_game_save_info function in mcfuncs.cpp is also used to determine ahead of time
	// how much space a particular type of save will use, so that we can check that there is enough
	// space on the memcard.
	// However, at that point the file name for the cas save is not known, because the user has not
	// chosen it yet.
	// So add a pad string, so that whatever the current filename happens to be, the amount of
	// space used will be the same once the user has chosen a filename.
	int pad=Script::GetInteger("MAX_MEMCARD_FILENAME_LENGTH",Script::ASSERT)-strlen(pFileName);
	Dbg_MsgAssert(pad>=0 && pad<100,("Oops! '%s'",pFileName));
	char p_foo[100];
	for (int i=0; i<pad; ++i)
	{
		p_foo[i]='*';
	}
	p_foo[pad]=0;	
	pStuff->AddString("FileNamePad",p_foo);
	*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPlayerProfileManager::LoadAllProProfileInfo(Script::CStruct *pStuff)
{
	Dbg_MsgAssert(pStuff,("NULL pStuff"));

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());
	
	uint32 tableSize = m_Profiles.getSize( );
	for ( uint32 i = 0; i < tableSize; i++ )
	{
		int key;
		CSkaterProfile* pProfile = m_Profiles.GetItemByIndex( i, &key );

		if (pProfile->IsPro())
		{
			uint32 Name=pProfile->GetSkaterNameChecksum();
			pProfile->ReadFromStructure(Name,pStuff);
		}	
	}
	
	uint32 CurrentSkaterNameChecksum=0;
	pStuff->GetChecksum("CurrentSkater",&CurrentSkaterNameChecksum);
	ApplyTemplateToCurrentProfile(CurrentSkaterNameChecksum);
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPlayerProfileManager::AddCASProfileInfo(Script::CStruct *pStuff)
{
	Dbg_MsgAssert(pStuff,("NULL pStuff"));
	
	uint32 tableSize = m_Profiles.getSize( );
	for ( uint32 i = 0; i < tableSize; i++ )
	{
		int key;
		CSkaterProfile* pProfile = m_Profiles.GetItemByIndex( i, &key );

		if (!pProfile->IsPro())
		{
			pProfile->WriteIntoStructure(pStuff);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPlayerProfileManager::LoadCASProfileInfo( Script::CStruct *pStuff, bool load_info )
{
	Dbg_MsgAssert(pStuff,("NULL pStuff"));

	ApplyTemplateToCurrentProfile(0xa7be964/*custom*/);
	CSkaterProfile* pProfile=GetCurrentProfile();

	uint32 Name = Script::GenerateCRC("custom");
	Script::CStruct *pSkaterInfo=NULL;
	pStuff->GetStructure(Name,&pSkaterInfo);
   
	Dbg_MsgAssert(pSkaterInfo,("Memory card data is missing info for skater '%s'",Script::FindChecksumName(Name)));
	
	// load only if there is a version number and it is current.
	int Version=0;
	if (pSkaterInfo->GetInteger("version_number",&Version))
	{
		if (Version==CSkaterProfile::vVERSION_NUMBER)
		{
			Script::CStruct *pTemp=NULL;
	
			pSkaterInfo->GetStructure("Appearance",&pTemp);
			Gfx::CModelAppearance* pAppearance=pProfile->GetAppearance();
			Dbg_MsgAssert(pAppearance,("NULL pAppearance"));
			pAppearance->Load(pTemp);
	
//			CSkaterCareer* pCareer=pProfile->GetCareer();
//			Dbg_MsgAssert(pCareer,("NULL pCareer"));
//			pCareer->ReadFromStructure(pSkaterInfo);
	
			// GJ:  maybe this function could be refactored to use
			// CSkaterProfile's ReadFromStructure() function?
			// i'm not sure why there's one set of code for the pros
			// and one for the custom skater
			Gfx::CFaceTexture* pFaceTexture = pAppearance->GetFaceTexture();
			Dbg_MsgAssert( (!pProfile->IsPro()) == (pFaceTexture!=NULL), ( "Only custom skaters should have face textures (Pro=%d, FaceTexture=%p)", pProfile->IsPro(), pFaceTexture ) )
			if ( pFaceTexture )
			{
				bool faceTextureFound = pSkaterInfo->GetStructure( CRCD(0xf0d3bc94,"FaceTexture"), &pTemp );
				if ( faceTextureFound )
				{
					// load the face texture, if it exists
					pFaceTexture->ReadFromStructure( pTemp );
					printf( "Face texture found\n" );
				}
				else
				{
					printf( "Face texture not found\n" );
				}
				pFaceTexture->SetValid( faceTextureFound );
			}
            
            // store current stats
            int air_value;
            int run_value;
            int ollie_value;
            int speed_value;
            int spin_value;
            int flip_speed_value;
            int switch_value;
            int rail_balance_value;
            int lip_balance_value;
            int manual_balance_value;
            air_value = pProfile->GetStatValue(CRCD(0x439f4704,"air"));
            run_value = pProfile->GetStatValue(CRCD(0xaf895b3f,"run"));
            ollie_value = pProfile->GetStatValue(CRCD(0x9b65d7b8,"ollie"));
            speed_value = pProfile->GetStatValue(CRCD(0xf0d90109,"speed"));
            spin_value = pProfile->GetStatValue(CRCD(0xedf5db70,"spin"));
            flip_speed_value = pProfile->GetStatValue(CRCD(0x6dcb497c,"flip_speed"));
            switch_value = pProfile->GetStatValue(CRCD(0x9016b4e7,"switch"));
            rail_balance_value = pProfile->GetStatValue(CRCD(0xf73a13e3,"rail_balance"));
            lip_balance_value = pProfile->GetStatValue(CRCD(0xae798769,"lip_balance"));
            manual_balance_value = pProfile->GetStatValue(CRCD(0xb1fc0722,"manual_balance"));

			int num_trickslots;
			num_trickslots = pProfile->GetNumSpecialTrickSlots();
			Script::CStruct* pSpecialTricks = new Script::CStruct();
			pSpecialTricks->AppendStructure( pProfile->GetSpecialTricksStructure() );
            
            
            pTemp=NULL;
            pSkaterInfo->GetStructure("Info",&pTemp);
            Script::CStruct* pInfo = pProfile->GetInfo();
            Dbg_Assert(pInfo);
            pInfo->Clear();
            pInfo->AppendStructure(pTemp);
            
            if ( !load_info )
            {
                // restore stats
                pProfile->SetPropertyValue(CRCD(0x439f4704,"air"), air_value);
                pProfile->SetPropertyValue(CRCD(0xaf895b3f,"run"), run_value);
                pProfile->SetPropertyValue(CRCD(0x9b65d7b8,"ollie"), ollie_value);
                pProfile->SetPropertyValue(CRCD(0xf0d90109,"speed"), speed_value);
                pProfile->SetPropertyValue(CRCD(0xedf5db70,"spin"), spin_value);
                pProfile->SetPropertyValue(CRCD(0x6dcb497c,"flip_speed"), flip_speed_value);
                pProfile->SetPropertyValue(CRCD(0x9016b4e7,"switch"), switch_value);
                pProfile->SetPropertyValue(CRCD(0xf73a13e3,"rail_balance"), rail_balance_value);
                pProfile->SetPropertyValue(CRCD(0xae798769,"lip_balance"), lip_balance_value);
                pProfile->SetPropertyValue(CRCD(0xb1fc0722,"manual_balance"), manual_balance_value);

				// restore trickslot number
				pProfile->SetPropertyValue( CRCD(0xac9b9eda,"max_specials"), num_trickslots );
				pProfile->GetSpecialTricksStructure()->Clear();
				pProfile->GetSpecialTricksStructure()->AppendStructure( pSpecialTricks );
            }
			delete pSpecialTricks;
            

            // First deck is always unlocked, so make sure this is the default just in case DeckFlags is not found.
			int DeckFlags=1;
			pSkaterInfo->GetInteger("DeckFlags",&DeckFlags);
			//Front::SetDeckFlags(Name,(uint32)DeckFlags);
			
			return;
		}
		Dbg_Warning("\n\nBad version number of %d in saved custom skater. Required version = %d\n\n",Version,CSkaterProfile::vVERSION_NUMBER);
		return;
	}		
	Dbg_Warning("\n\nNo version number in saved custom skater. Required version = %d\n\n",CSkaterProfile::vVERSION_NUMBER);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPlayerProfileManager::AddTemporaryProfile( uint32 profileName )
{
	Dbg_MsgAssert( m_TemporaryProfileCount < vMAX_TEMPORARY_PROFILES, ( "Too many temporary profiles (max=%d)", vMAX_TEMPORARY_PROFILES ) );

	for ( int i = 0; i < m_TemporaryProfileCount; i++ )
	{
		if ( m_TemporaryProfileChecksums[i] == profileName )
		{
			Dbg_MsgAssert( 0, ( "Temporary profile checksum %s was already used\n", Script::FindChecksumName(profileName) ) );
		}
	}

	mp_TemporaryProfiles[m_TemporaryProfileCount] = new Obj::CSkaterProfile;
	m_TemporaryProfileChecksums[m_TemporaryProfileCount] = profileName;

	m_TemporaryProfileCount++;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterProfile* CPlayerProfileManager::GetTemporaryProfile(uint32 profileName)
{
	for ( int i = 0; i < m_TemporaryProfileCount; i++ )
	{
		if ( m_TemporaryProfileChecksums[i] == profileName )
		{
			return mp_TemporaryProfiles[i];
		}
	}
	
	Dbg_MsgAssert( 0, ( "Couldn't find temporary profile with checksum %s\n", Script::FindChecksumName(profileName) ) );

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPlayerProfileManager::AddNewProfile( Script::CStruct* pParams )
{
	Dbg_Assert( pParams );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());

	// profileChecksum = hawk, thomas, custom, etc.
	uint32 profileChecksum;
	pParams->GetChecksum( "name", &profileChecksum, true );
	
	// add to the lookup table
	CSkaterProfile* pPlayerProfile = new CSkaterProfile;
	pPlayerProfile->Reset( pParams );
	m_Profiles.PutItem( profileChecksum, pPlayerProfile );

#ifdef __USER_GARY__
//	uint8 tBuf[vMAX_APPEARANCE_BUFFER_SIZE];
//	uint32 size = pPlayerProfile->WriteToBuffer(tBuf, vMAX_APPEARANCE_BUFFER_SIZE);
//	Dbg_Assert( size < vMAX_PRACTICAL_APPEARANCE_DATA_SIZE );
//	Dbg_Message("*********** AddNewProfile %s appearance data size = %d bytes\n", Script::FindChecksumName(profileChecksum), size);
#endif

	Mem::Manager::sHandle().PopContext();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPlayerProfileManager::LockCurrentSkaterProfileIndex( bool locked )
{
	// GJ:  This is purely for debugging purposes,
	// to catch any accesses to the current skater profile index.
	// When using the CModelBuilder class, we NEVER want to use
	// the current skater profile index, because it may or may
	// not be valid in a network or splitscreen game.
	// So, the CModelBuilder class will turn this lock on
	// at the beginning of the model creation process, and
	// turn it off at the end.  Any calls to GetCurrentSkaterProfileIndex
	// will assert while locked.

	m_LockCurrentSkaterProfileIndex = locked;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj

