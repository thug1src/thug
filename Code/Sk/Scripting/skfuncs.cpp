//****************************************************************************
//* MODULE:         Sk/Scripting
//* FILENAME:       SkFuncs.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  6/10/2002
//****************************************************************************

// start autoduck documentation
// @DOC skfuncs
// @module skfuncs | None
// @subindex Scripting Database
// @index script | skfuncs

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/scripting/skfuncs.h>

#include <gel/assman/assman.h>
							 
#include <gel/objman.h>
#include <gel/objtrack.h>

#include <gel/components/modelcomponent.h>
#include <gel/components/skatercameracomponent.h>
#include <gel/components/trickcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>

#include <gel/scripting/script.h> 
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h> 
#include <gel/scripting/checksum.h> 
#include <gel/scripting/component.h>
#include <gel/scripting/utils.h>

#include <gel/music/music.h>
#include <gel/net/client/netclnt.h>
 
#include <gfx/casutils.h>
#include <gfx/facetexture.h>
#include <gfx/gfxutils.h>
#include <gfx/modelbuilder.h>
#include <gfx/nx.h>
#include <gfx/nxmodel.h>

#include <sk/gamenet/gamenet.h>
#include <sk/scripting/nodearray.h>

#include <sk/modules/skate/competition.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/horse.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/frontend/frontend.h>

#include <sk/objects/gameobj.h>
#include <sk/objects/moviecam.h>
#include <sk/objects/records.h>
#include <sk/objects/skater.h>
#include <sk/objects/skaterprofile.h>
#include <sk/objects/playerprofilemanager.h>
#include <sk/objects/skatercareer.h>

#include <sk/components/skaterstatecomponent.h>
#include <sk/parkeditor2/parked.h>

#include <sys/config/config.h>
#include <Gfx/NGPS/NX/chars.h>

#include <sys/SIO/keyboard.h>

#include <sys/file/filesys.h>
#include <sys/file/pre.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Front
{
	extern void SetScoreTHPS4(char* score_text, int skater_num);
}

namespace CFuncs
{

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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void s_preload_ped_part( uint32 partChecksum )
{
	Script::CArray* pArray = Script::GetArray( partChecksum );
	Dbg_Assert( pArray );

	for ( uint32 i = 0; i < pArray->GetSize(); i++ )
	{
		Script::CStruct* pStruct = pArray->GetStructure( i );

		int allow_random = 0;
		pStruct->GetInteger( CRCD(0xf1e3cd22,"allow_random"), &allow_random, Script::NO_ASSERT );
			
		if ( allow_random )
		{
			const char* pMeshName;
			if ( pStruct->GetText( CRCD(0x1e90c5a9,"mesh"), &pMeshName, false ) )
			{
				Nx::CModel* pDummy = Nx::CEngine::sInitModel();
//				Dbg_Message("Preloading random ped model %s", pMeshName );
				pDummy->AddGeom( pMeshName, 0, true );
				Nx::CEngine::sUninitModel( pDummy );
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Score queries:

static int s_get_score_from_params( Script::CStruct *pParams, Script::CScript *pScript )
{  
	int score;
	if ( !pParams->GetInteger( NONAME, &score ) )
	{
		Dbg_MsgAssert( 0,( "\n%s\nMust provide an integer for s_get_score_from_params.", pScript->GetScriptInfo( ) ));
		return ( 0 );
	}
	return ( score );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static Mdl::Score* s_get_score_struct( void )
{
	// find the skater, get the score landed:
	Obj::CSkater *pSkater = Mdl::Skate::Instance()->GetLocalSkater();						   
	if ( pSkater )
	{
		return ( pSkater->GetScoreObject() );
	}
	return ( NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Local function to return a career, given a flag number
static Obj::CSkaterCareer* s_get_career( int flag, Script::CStruct *pParams )
{
// Now this always just returns the global career, just kept for convenience	
	return Mdl::Skate::Instance()->GetCareer();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CurrentSkaterIsPro | Checks if the current skater is pro
bool ScriptCurrentSkaterIsPro(Script::CStruct *pParams, Script::CScript *pScript)
{   
	Obj::CSkaterProfile *pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile(pParams);
	return ( pSkaterProfile && pSkaterProfile->IsPro() );
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetGoalsCompleted | Returns goals (competitions) completed
bool ScriptGetGoalsCompleted(Script::CStruct *pParams, Script::CScript *pScript)
{   
	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();
	Dbg_MsgAssert(pCareer,("NULL pCareer"));
	
	int LevelNumber=0;
	pParams->GetInteger(NONAME,&LevelNumber);
	char pBuf[100];

	sprintf(pBuf,"%d/9",pCareer->CountGoalsCompleted(LevelNumber));
	pScript->GetParams()->AddComponent(CRCD(0xc661fe79,"GoalsCompleted"),ESYMBOLTYPE_STRING,pBuf);
	pScript->GetParams()->AddComponent(CRCD(0xa43dc969,"BestMedal"),ESYMBOLTYPE_STRING,"---");
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetNextLevelRequirements | Gets number of goals needed for next level
bool ScriptGetNextLevelRequirements(Script::CStruct *pParams, Script::CScript *pScript)
{   
	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();
	Dbg_MsgAssert(pCareer,("NULL pCareer"));
	
	char pBuf[100];
	sprintf(pBuf,"---");
	
	int TotalGoals=pCareer->CountTotalGoalsCompleted();
	int TotalMedals=pCareer->CountMedals();
	
	Script::CArray *pUnlockRequirementsArray=Script::GetArray(CRCD(0x1d8f1a7e,"UnlockRequirements"));
	for (uint32 i=0; i<pUnlockRequirementsArray->GetSize(); ++i)
	{
		Script::CStruct *pUnlockRequirements=pUnlockRequirementsArray->GetStructure(i);
		
		int NumGoalsRequired=0;
		int NumMedalsRequired=0;
		
		if (pUnlockRequirements->ContainsFlag(CRCD(0x38dbe1d0,"Goals")) || pUnlockRequirements->ContainsFlag(CRCD(0x032314d1,"Goal")))
		{
			// Need a certain number of goals.
			pUnlockRequirements->GetInteger(NONAME,&NumGoalsRequired);
		}	
				
		if (pUnlockRequirements->ContainsFlag(CRCD(0x56907f8b,"Medals")) || pUnlockRequirements->ContainsFlag(CRCD(0x23bba846,"Medal")))
		{
			// Need a certain number of medals.
			pUnlockRequirements->GetInteger(NONAME,&NumMedalsRequired);
		}
		
		#if ENGLISH
		if (TotalGoals<NumGoalsRequired)
		{
			sprintf(pBuf,"%d goals for next Level",NumGoalsRequired-TotalGoals);
			break;
		}
		if (TotalMedals<NumMedalsRequired)
		{
			sprintf(pBuf,"%d medals for next level",NumMedalsRequired-TotalMedals);
			break;
		}
		#else
		if (TotalGoals<NumGoalsRequired)
		{
			sprintf(pBuf,Script::GetLocalString(CRCD(0x407bfd24,"cfuncs_str_goalsfornextlevel")),NumGoalsRequired-TotalGoals);
			break;
		}
		if (TotalMedals<NumMedalsRequired)
		{
			sprintf(pBuf,Script::GetLocalString(CRCD(0x4f3e243e,"cfuncs_str_medalsfornextlevel")),NumMedalsRequired-TotalMedals);
			break;
		}
		#endif		
	}		
	
	pScript->GetParams()->AddComponent(CRCD(0xaf2305a6,"NextLevelRequirements"),ESYMBOLTYPE_STRING,pBuf);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetCurrentSkaterProfile | 
// @uparmopt 0 | Skater profile index - must be 0 or 1
bool ScriptSetCurrentSkaterProfile(Script::CStruct *pParams, Script::CScript *pScript)
{	
	int Profile=0;
	pParams->GetInteger(NONAME,&Profile);
	Dbg_MsgAssert(Profile==0 || Profile==1,("\n%s\nBad index sent to SetCurrentSkaterProfile, must be 0 or 1",pScript->GetScriptInfo()));
	
	
	Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
	pPlayerProfileManager->SetCurrentProfileIndex(Profile);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CurrentSkaterProfileIs | Checks if current skater profile is 
// equal to index value passed in
// @uparmopt 0 | Index value - must be 0 or 1
bool ScriptCurrentSkaterProfileIs(Script::CStruct *pParams, Script::CScript *pScript)
{   
	int Profile=0;
	pParams->GetInteger(NONAME,&Profile);
	Dbg_MsgAssert(Profile==0 || Profile==1,("\n%s\nBad index sent to CurrentSkaterProfileIs, must be 0 or 1",pScript->GetScriptInfo()));
	
	
	Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
	
	return pPlayerProfileManager->GetCurrentProfileIndex()==Profile;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AddSkaterProfile | Adds skater profile 
// @parm name | name | The name of the profile
bool ScriptAddSkaterProfile(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
	return pPlayerProfileManager->AddNewProfile( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AddTemporaryProfile | Adds temporary skater profile
// (used for credits, to temporarily hijack the skater profile
bool ScriptAddTemporaryProfile(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 profileName;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &profileName, Script::ASSERT );
	
	Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
	return pPlayerProfileManager->AddTemporaryProfile( profileName );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RememberTemporaryAppearance | 
bool ScriptRememberTemporaryAppearance(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 profileName;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &profileName, Script::ASSERT );
	
	Gfx::CModelAppearance theAppearance;
	Gfx::CModelAppearance* pAppearance = NULL;
	
	Obj::CPlayerProfileManager* pProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();

	Script::CStruct* pAppearanceStruct;
	if ( pParams->GetStructure( CRCD(0xefc8944c,"appearance_structure"), &pAppearanceStruct, Script::NO_ASSERT ) )
	{
		theAppearance.Load( pAppearanceStruct );
		pAppearance = &theAppearance;
	}
	else
	{
		int playerNum;
		pParams->GetInteger( CRCD(0x67e6859a,"player"), &playerNum, Script::ASSERT );

		Obj::CSkaterProfile* pProfile = pProfileManager->GetProfile( playerNum );
		Dbg_Assert( pProfile );
		pAppearance = pProfile->GetAppearance();
	}
	Dbg_Assert( pAppearance );

	Obj::CSkaterProfile* pTempProfile = pProfileManager->GetTemporaryProfile( profileName );
	Dbg_Assert( pTempProfile );
	Gfx::CModelAppearance* pTempAppearance = pTempProfile->GetAppearance();
	Dbg_Assert( pTempAppearance );

	if ( pParams->ContainsFlag( CRCD(0x3af36e6d,"NoFaceTexture") ) )
	{
		Script::CStruct* pOriginalStructure = pAppearance->GetStructure();
		Script::CStruct* pTempStructure = pTempAppearance->GetStructure();
		pTempStructure->Clear();
		pTempStructure->AppendStructure( pOriginalStructure );
	}
	else
	{
		*pTempAppearance = *pAppearance;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RestoreTemporaryAppearance | 
bool ScriptRestoreTemporaryAppearance(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 profileName;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &profileName, Script::ASSERT );
	
	int playerNum;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &playerNum, Script::ASSERT );

	Obj::CPlayerProfileManager* pProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
	Obj::CSkaterProfile* pProfile = pProfileManager->GetProfile( playerNum );
	Dbg_Assert( pProfile );
	Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();
	Dbg_Assert( pAppearance );

	Obj::CSkaterProfile* pTempProfile = pProfileManager->GetTemporaryProfile( profileName );
	Dbg_Assert( pTempProfile );
	Gfx::CModelAppearance* pTempAppearance = pTempProfile->GetAppearance();
	Dbg_Assert( pTempAppearance );

	if ( pParams->ContainsFlag( CRCD(0x3af36e6d,"NoFaceTexture") ) )
	{
		Script::CStruct* pOriginalStructure = pAppearance->GetStructure();
		Script::CStruct* pTempStructure = pTempAppearance->GetStructure();
		pOriginalStructure->Clear();
		pOriginalStructure->AppendStructure( pTempStructure );
	}
	else
	{
		*pAppearance = *pTempAppearance;
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SyncPlayer2Profile | 
bool ScriptSyncPlayer2Profile(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
	pPlayerProfileManager->SyncPlayer2();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PreloadModels | 
bool ScriptPreloadModels( Script::CStruct *pParams, Script::CScript *pScript )
{
	// Load all the files to get them into the asset manager, 
	// while the PRE file is still in memory

	Mem::PushMemProfile("PreLoadModels");
  
	// now loop through the NodeArray, looking for single-skinned models
	Script::CArray *pNodeArray = Script::GetArray( CRCD(0xc472ecc5,"NodeArray") );
	Dbg_MsgAssert( pNodeArray, ( "No NodeArray found in ParseNodeArray" ) );

	for ( uint32 i = 0; i < pNodeArray->GetSize(); i++ )
	{
		Script::CStruct* pNode = pNodeArray->GetStructure( i );
		Dbg_MsgAssert( pNode, ( "NULL pNode" ) );

		// The following nodes are capable of being ignored in net games		
				
		if ( Mdl::Skate::Instance()->ShouldBeAbsentNode( pNode ) )
		{
			// Don't load, as it's a net game
		}	
		else 
		{
			uint32 ClassChecksum = 0;
			pNode->GetChecksum( CRCD(0x12b4e660,"Class"), &ClassChecksum );

			uint32 TypeChecksum = 0;
			pNode->GetChecksum(CRCD(0x7321a8d6,"Type"), &TypeChecksum );
			
			// first eliminate the flag models, if this is non-net game 
			// On load model IF a multiplayer game OR NOT a flag model
			if (Mdl::Skate::Instance()->IsMultiplayerGame() ||
				TypeChecksum != CRCD(0xbebb41f0,"Flag_Red")         && 
				TypeChecksum != CRCD(0x836284a5,"Flag_Red_Base")    &&
				TypeChecksum != CRCD(0xc3ebe05e,"Flag_Green") 		 && 
				TypeChecksum != CRCD(0x4f8ff239,"Flag_Green_base")	 &&
				TypeChecksum != CRCD(0x8cca938a,"Flag_Blue")		 &&
				TypeChecksum != CRCD(0x41e5bcf,"Flag_Blue_base")	 &&
				TypeChecksum != CRCD(0xc2af1eb1,"Flag_Yellow")		 &&
				TypeChecksum != CRCD(0xed3ad7fe,"Flag_Yellow_base") )
			{
	
				// Handle pre-loading of models for non-net game nodes
				switch (ClassChecksum)
				{
					case 0xa0dfac98: // Pedestrian
					case 0xa71394a2: // AnimatingObject
					case 0x19b1e241: // ParticleEmitter				
						break;
					case 0xe47f1b79: // Vehicle
					case 0xef59c100: // GameObject
						{
							const char *pModelName = NULL;
	
							if ( pNode->GetText( 0x286a8d26, &pModelName ) ) // checksum 'model'
							{
								const char* p_model_name;
								pNode->GetText( CRCD(0x286a8d26,"model"), &p_model_name, true );
								if (stricmp("none",p_model_name) != 0)
								{		   
									Str::String fullModelName;
									fullModelName = Gfx::GetModelFileName(p_model_name, ".mdl");
									
									Nx::CModel* p_dummy = Nx::CEngine::sInitModel();
									//Dbg_Message( "Preloading model %s", fullModelName.getString() );
	
									//int texDictOffset = 0;
									//pNode->GetInteger( "texDictOffset", &texDictOffset, false );
	
									bool forceTexDictLookup = pNode->ContainsFlag( CRCD(0x6c37fdc7,"AllowReplaceTex") );
									
									p_dummy->AddGeom( fullModelName.getString(), 0, true, 0, forceTexDictLookup );
									Nx::CEngine::sUninitModel( p_dummy );
								}
							}
						}
						break;
					default:
						break;
				}
			}
		}				
	}

	Mem::PopMemProfile(/*"PreLoadModels"*/);
	
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PreloadPedestrians | 
bool ScriptPreloadPedestrians( Script::CStruct *pParams, Script::CScript *pScript )
{
//	Tmr::Time baseTime = Tmr::ElapsedTime(0);

	// should be skipped for net game?
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	if ( gamenet_man->InNetGame()/* || Mdl::Skate::Instance()->IsMultiplayerGame()*/)
	{
		return false;
	}

	if ( Script::GetInt( "NoPreloadRandomPeds", false ) )
	{
		// some create-a-skater artists don't want to
		// preload their random peds
		return false;
	}

	Mem::PushMemProfile("PreLoadPedestrians");

	if (pParams->ContainsFlag(CRCD(0xf6f8e158,"no_random")))
	{
		// This feature used when pre-loading peds for created goals in the park editor,
		// where we don't want loads of parts cos there isn't much memory
	}
	else
	{
		// loop through the master_editable_list, looking for
		// randomizable parts (based on the "allow_random" flag
		// which should have been set when preselecting ped parts.
		s_preload_ped_part( Script::GenerateCRC("ped_m_head") );
		s_preload_ped_part( Script::GenerateCRC("ped_m_torso") );
		s_preload_ped_part( Script::GenerateCRC("ped_m_legs") );
		s_preload_ped_part( Script::GenerateCRC("ped_f_head") );
		s_preload_ped_part( Script::GenerateCRC("ped_f_torso") );
		s_preload_ped_part( Script::GenerateCRC("ped_f_legs") );
	}
	
	// should really do an examination of what parts the thing actually uses
	// and then the levelassetlister should list those...

	// loop through the NodeArray, looking for single-skinned models
	Script::CArray *pNodeArray = Script::GetArray( CRCD(0xc472ecc5,"NodeArray") );
	Dbg_MsgAssert( pNodeArray, ( "No NodeArray found in ParseNodeArray" ) );

	for ( uint32 i = 0; i < pNodeArray->GetSize(); i++ )
	{
		// we need to give it a texDictOffset so that it
		// doesn't conflict with the secret ped skaters...
		// (make sure it's less than the cutscene tex dict range, 
		// though, or else it would conflict with the cutscene objects)
		int texDictOffset = Mdl::Skate::vMAX_SKATERS;

        Script::CStruct* pNode = pNodeArray->GetStructure( i );
		Dbg_MsgAssert( pNode, ( "NULL pNode" ) );

		uint32 ClassChecksum = 0;
		pNode->GetChecksum( Script::GenerateCRC("Class"), &ClassChecksum );

		if ( ClassChecksum == Script::GenerateCRC("pedestrian") )
		{
			const char* p_model_name;
			if ( pNode->GetText( "model", &p_model_name, false ) )
			{
				Str::String fullModelName;
				fullModelName = Gfx::GetModelFileName(p_model_name, ".skin");

				// preload the model!
				Nx::CModel* pDummy = Nx::CEngine::sInitModel();
//				Dbg_Message("Preloading single-skinned ped model %s", fullModelName.getString() );
				pDummy->AddGeom( fullModelName.getString(), 0, true, texDictOffset );
				
				Nx::CEngine::sUninitModel( pDummy );
			}

			// preload the shadow if necessary
			const char* p_shadow_model_name;
			if ( pNode->GetText( "shadowmodel", &p_shadow_model_name, false ) )
			{
				Str::String fullModelName;
				fullModelName = Gfx::GetModelFileName(p_shadow_model_name, ".mdl");

				// preload the model!
				Nx::CModel* pDummy = Nx::CEngine::sInitModel();
//				Dbg_Message("Preloading single-skinned ped model %s", fullModelName.getString() );
				pDummy->AddGeom( fullModelName.getString(), 0, true, texDictOffset );

				Nx::CEngine::sUninitModel( pDummy );
			}


			// now find the non-randoms
			uint32 profileName;
			if ( pNode->GetChecksum( CRCD(0x7ea855f0,"profile"), &profileName, false ) )
			{
				Script::CStruct* pStruct = Script::GetStructure( profileName );
				if ( pStruct )				   
				{
					Nx::CModel* pDummy = Nx::CEngine::sInitModel();
					Dbg_Assert( pDummy );
  //  				pDummy->LoadSkeleton( Script::GenerateCRC("human") );
					Gfx::CModelAppearance theTempAppearance;
					theTempAppearance.Load( pStruct, false );
					Gfx::CModelBuilder theBuilder( true, texDictOffset );
					theBuilder.BuildModel( &theTempAppearance, pDummy, NULL, Script::GenerateCRC("preload_model_from_appearance") );
					Nx::CEngine::sUninitModel( pDummy );
    			}
			}
		}
	}

//	Dbg_Message( "Preloading peds took %d ms", Tmr::ElapsedTime( baseTime ) );
			
	Mem::PopMemProfile(/*"PreLoadModels"*/);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PreselectRandomPedestrians | 
// Example: PreselectRandomPedestrians part=ped_f_legs list=sch_ped_f_legs num=4
bool ScriptPreselectRandomPedestrians( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 fullPartChecksum;
	pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &fullPartChecksum, Script::ASSERT );

	Script::CArray* pFullPartArray = NULL;
	pFullPartArray = Script::GetArray( fullPartChecksum, Script::ASSERT );

	uint32 levelName = Mdl::Skate::Instance()->m_requested_level;

	Dbg_Message( "Pre-selecting %s in %s", Script::FindChecksumName(fullPartChecksum), Script::FindChecksumName(levelName) );

	// go through the specified part array,
	// and remove any "allow_random" components...
	uint32 fullPartArraySize = pFullPartArray->GetSize();
	for ( uint32 i = 0; i < fullPartArraySize; i++ )
	{
		Script::CStruct* pStruct = pFullPartArray->GetStructure( i );
		
		// allowed_to_pick is a temp variable used only by this function...
		// clear it already exists...
		pStruct->RemoveComponent( CRCD(0x355f9467,"allowed_to_pick") );

		// they all default to not be able to be randomized
		pStruct->AddInteger( CRCD(0xf1e3cd22,"allow_random"), 0 );

		pStruct->AddInteger( CRCD(0x92bddfd9,"already_selected"), 0 );
		
		pStruct->AddInteger( CRCD(0x4b833e64,"random_index"), -1 );
	}
	
	int allowableCount = 0;
	for ( uint32 i = 0; i < fullPartArraySize; i++ )
	{
		Script::CStruct* pStruct = pFullPartArray->GetStructure( i );
		
		bool allowed_to_pick = true;

		// if it's level-specific...
		Script::CStruct* pLevelSpecificStruct;
		if ( pStruct->ContainsComponentNamed( CRCD(0xf6f8e158,"no_random") ) )
		{
			allowed_to_pick = false;
		
			if ( pStruct->GetStructure( CRCD(0x3598bf7d,"level_specific"), &pLevelSpecificStruct, false ) )
			{
				// sanity check:  level_specific and no_random are mutually exclusive
				Script::PrintContents( pStruct );
				Dbg_MsgAssert( 0, ( "no_random should not be used with level_specific" ) );
			}
		}
		else if ( pStruct->GetStructure( CRCD(0x3598bf7d,"level_specific"), &pLevelSpecificStruct, false ) )
		{
			Dbg_MsgAssert( !pStruct->ContainsComponentNamed(CRCD(0x94d12f97,"exclude_from_levels")), ( "level_specific and exclude_from_levels are mutually exclusive" ) )

			// check to see if we're in this level...
			allowed_to_pick = pLevelSpecificStruct->ContainsComponentNamed( levelName );
		}
		else if ( pStruct->GetStructure( CRCD(0x94d12f97,"exclude_from_levels"), &pLevelSpecificStruct, false ) )
		{
			Dbg_MsgAssert( !pStruct->ContainsComponentNamed(CRCD(0x3598bf7d,"level_specific")), ( "level_specific and exclude_from_levels are mutually exclusive" ) )

			// check to see if we're in this level...
			allowed_to_pick = !pLevelSpecificStruct->ContainsComponentNamed( levelName );
		}
		 
		if ( allowed_to_pick )
		{
			allowableCount++;
			pStruct->AddInteger( CRCD(0x355f9467,"allowed_to_pick"), 1 );
		}
		else
		{
			// clear the already picked flag
			pStruct->AddInteger( CRCD(0x355f9467,"allowed_to_pick"), 0 );
			
			if ( Script::GetInt( "cas_artist", false ) )
			{
//				Dbg_Message("Disallowing...");
//				Script::PrintContents(pStruct);
			}
		}
	}

	// Nolan's Fisherman Example:
	// If a non-randomized profile explicitly loads up a random ped part anyway,
	// then add that random ped asset to the pool of allowable randomized ped parts
	Script::CArray *pNodeArray=Script::GetArray( CRCD(0xc472ecc5,"NodeArray") );
	Dbg_MsgAssert( pNodeArray, ( "No NodeArray found in PreselectRandomPedestrians" ) );
	for ( uint32 i = 0; i < pNodeArray->GetSize(); i++ )
	{
		Script::CStruct* pNode = pNodeArray->GetStructure( i );
		Dbg_MsgAssert( pNode, ( "NULL pNode" ) );

		uint32 ClassChecksum = 0;
		pNode->GetChecksum( Script::GenerateCRC("Class"), &ClassChecksum );

		if ( ClassChecksum == Script::GenerateCRC("pedestrian") )
		{
			// now find the non-randoms
			uint32 profileName;
			if ( pNode->GetChecksum( CRCD(0x7ea855f0,"profile"), &profileName, false ) )
			{
				Script::CStruct* pProfileStruct = Script::GetStructure( profileName );
				if ( pProfileStruct )
				{
					Script::CStruct* pVirtualStruct;
					if ( pProfileStruct->GetStructure( fullPartChecksum, &pVirtualStruct, false ) )
					{
						uint32 descID;
						if ( pVirtualStruct->GetChecksum( CRCD(0x4bb2084e,"desc_id"), &descID, false ) )
						{
							Script::CStruct* pActualStruct = Cas::GetOptionStructure( fullPartChecksum, descID, false );
							if ( pActualStruct && !pActualStruct->ContainsComponentNamed( CRCD(0xf6f8e158,"no_random") ) )
							{
								if ( Script::GetInt( "cas_artist", false ) )
								{
//									Dbg_Message( "Allowing to pick %s", Script::FindChecksumName( descID ) );
								}
								pActualStruct->AddInteger( CRCD(0xf1e3cd22,"allow_random"), 1 );
								pActualStruct->AddInteger( CRCD(0x355f9467,"allowed_to_pick"), 1 );

								// these get lowest-priority when randomizing...
								pActualStruct->AddInteger( CRCD(0x92bddfd9,"already_selected"), 1 );
							}
						}
					}
				}
			}
		}
	}
	
	// sanity check that the number specified is 
	// greater than the actual number of random parts
	int numToSelect = allowableCount;
	pParams->GetInteger( CRCD(0x23bc5091,"num"), &numToSelect, Script::NO_ASSERT );
	if ( numToSelect > allowableCount )
	{
		Dbg_Message( "Not enough parts in list %s to select from (need %d, have %d)!", Script::FindChecksumName(fullPartChecksum), numToSelect, allowableCount );
		numToSelect = allowableCount;
	}

	//-------------------------------------------------------------
	// look for a item with a universal skin tone first...  if it
	// exists, automatically preselect it.  otherwise, make sure
	// that at least one of each skintone is chosen, or else the
	// skintones may not match up after part randomization.
	Script::CStruct* pSkinTonePicks[64];
	int skinToneCount = 0;

	Cas::BuildRandomSetList( fullPartChecksum, 0, pSkinTonePicks, &skinToneCount );
	if ( skinToneCount )
	{
		Script::CStruct* pStruct = pSkinTonePicks[Mth::Rnd( skinToneCount )];
		
		pStruct->AddInteger( CRCD(0xf1e3cd22,"allow_random"), 1 );
		pStruct->AddInteger( CRCD(0x355f9467,"allowed_to_pick"), 0 );
		pStruct->AddInteger( CRCD(0x4b833e64,"random_index"), numToSelect-1 );
		numToSelect--;
	}
	else
	{
		// this code will only kick in if all of the options have a skin-tone
		// (random_set) associated with it 
		Dbg_MsgAssert( numToSelect >= 3, ( "Num parts to select (%d) must be >= 3, so that ped skin tones will work (either need to increase '%s' in levels.q, or add at least one universal skintone option)", numToSelect, Script::FindChecksumName(fullPartChecksum) ) );
		
		Script::CStruct* pStruct;

		Cas::BuildRandomSetList( fullPartChecksum, CRCD(0x94e5a308,"light"), pSkinTonePicks, &skinToneCount );
		Dbg_MsgAssert( skinToneCount, ( "Couldn't find light-skinned version of this part %s", Script::FindChecksumName(fullPartChecksum) ) );
		pStruct = pSkinTonePicks[Mth::Rnd( skinToneCount )];
		pStruct->AddInteger( CRCD(0xf1e3cd22,"allow_random"), 1 );
		pStruct->AddInteger( CRCD(0x355f9467,"allowed_to_pick"), 0 );
		pStruct->AddInteger( CRCD(0x4b833e64,"random_index"), numToSelect-1 );
		numToSelect--;
		
		Cas::BuildRandomSetList( fullPartChecksum, CRCC(0x85aaf0d8,"tan"), pSkinTonePicks, &skinToneCount );
		Dbg_MsgAssert( skinToneCount, ( "Couldn't find tan-skinned version of this part %s", Script::FindChecksumName(fullPartChecksum) ) );
		pStruct = pSkinTonePicks[Mth::Rnd( skinToneCount )];
		pStruct->AddInteger( CRCD(0xf1e3cd22,"allow_random"), 1 );
		pStruct->AddInteger( CRCD(0x355f9467,"allowed_to_pick"), 0 );
		pStruct->AddInteger( CRCD(0x4b833e64,"random_index"), numToSelect-1 );
		numToSelect--;
		
		Cas::BuildRandomSetList( fullPartChecksum, CRCC(0xe4834204,"dark"), pSkinTonePicks, &skinToneCount );
		Dbg_MsgAssert( skinToneCount, ( "Couldn't find dark-skinned version of this part %s", Script::FindChecksumName(fullPartChecksum) ) );
		pStruct = pSkinTonePicks[Mth::Rnd( skinToneCount )];
		pStruct->AddInteger( CRCD(0xf1e3cd22,"allow_random"), 1 );
		pStruct->AddInteger( CRCD(0x355f9467,"allowed_to_pick"), 0 );
		pStruct->AddInteger( CRCD(0x4b833e64,"random_index"), numToSelect-1 );
		numToSelect--;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	// now, add the "allow_random" flag to <numToSelect> of the items in the list...
	for ( int i = 0; i < numToSelect; i++ )
	{
		int allowed_to_pick = 0;
		while ( !allowed_to_pick )
		{
			int randomPick = Mth::Rnd( fullPartArraySize );
			Dbg_Assert( randomPick >= 0 && randomPick < (int)fullPartArraySize );

			Script::CStruct* pStruct = pFullPartArray->GetStructure( randomPick );
			pStruct->GetInteger( CRCD(0x355f9467,"allowed_to_pick"), &allowed_to_pick, Script::ASSERT );
			if ( allowed_to_pick )
			{
				pStruct->AddInteger( CRCD(0xf1e3cd22,"allow_random"), 1 );
				pStruct->AddInteger( CRCD(0x355f9467,"allowed_to_pick"), 0 );
				pStruct->AddInteger( CRCD(0x4b833e64,"random_index"), i );
			}
		}
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	// debugging code to show which items are allowed to be randomized
#ifdef __NOPT_ASSERT__
	int randomCount = 0;
	
	for ( uint32 i = 0; i < fullPartArraySize; i++ )
	{
		Script::CStruct* pStruct = pFullPartArray->GetStructure( i );
		
		int allow_random = 0;
		pStruct->GetInteger( CRCD(0xf1e3cd22,"allow_random"), &allow_random, Script::ASSERT );

		uint32 descID;
		pStruct->GetChecksum( CRCD(0x4bb2084e,"desc_id"), &descID, Script::ASSERT );

		if ( allow_random )
		{
			Dbg_Message( "Setting random part #%d: %s (%d)", randomCount++, Script::FindChecksumName(descID), numToSelect );
		}
		
		// get rid of temp variable...
		pStruct->RemoveComponent( CRCD(0x355f9467,"allowed_to_pick") );
	}
#endif // __NOPT_ASSERT__
	//-------------------------------------------------------------

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptReplaceCarTextures( Script::CStruct *pParams, Script::CScript *pScript )
{
	// this is a highly-specific function used for quickly changing
	// a car's headlight textures for time-of-day stuff
	// (the previous implementation was too slow, 
	// involving looping through each model component,
	// getting the model, and attempting to replaceTexture
	// on it...  this one targets car objects, and
	// skips over texture dictionaries that have already
	// been swapped)

//	Tmr::Time baseTime = Tmr::ElapsedTime(0);
	
	int num_to_rebuild = 0;

	const int vMAX_TEX_DICTS = 32;
	Nx::CTexDict* tex_dicts[vMAX_TEX_DICTS];
	int num_tex_dicts = 0;

	// new fast way, just go directly to the components, if any
	// (this is still kind of slow, so you wouldn't want to call it that often,
	// maybe only when any hiccups can be masked out with a screen fade or something)
	Obj::CModelComponent *p_component = (Obj::CModelComponent*)Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( CRC_MODEL );
	while( p_component )
	{
		// GJ TODO:  somehow loop through all the
		// skin assets in the asset manager
		if ( p_component->GetObject()->GetType() == SKATE_TYPE_CAR )
		{
			Nx::CModel* pModel = p_component->GetModel();

			bool must_rebuild = false;

			for ( int i = 0; i < pModel->GetNumGeoms(); i++ )
			{
				Nx::CTexDict* pTexDict = pModel->GetTexDictByIndex( i );

				bool found = false;

				// if any tex dict in the model's list
				// has not been processed yet, then
				// replace it
				for ( int j = 0; j < num_tex_dicts; j++ )
				{
					if ( pTexDict == tex_dicts[j] )
					{
						found = true;
					}
				}

				if ( !found )
				{
					// mark that texture dictionary as
					// processed, so that we don't have
					// to reprocess future objects
                    Dbg_MsgAssert( num_tex_dicts < vMAX_TEX_DICTS, ( "Too many texture dictionaries to search through" ) );
					tex_dicts[num_tex_dicts] = pTexDict;
					num_tex_dicts++;
					
					must_rebuild = true;
				}
			}

			if ( must_rebuild )
			{                
				num_to_rebuild++;
				p_component->CallMemberFunction( CRCD(0x83f9be15,"Obj_ReplaceTexture"), pParams, pScript );
			}
		}

		p_component = (Obj::CModelComponent*)p_component->GetNextSameType();		
	}

//	Dbg_Message( "ReplaceCarTextures on %d items took %d ms", num_to_rebuild, Tmr::ElapsedTime( baseTime ) );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetUIFromPreferences | 
// @parmopt name | field | | field name
// @parmopt string | field | | field string
// @parmopt name | select_if | | check if this name equals the value of
// checksum in the field
// @parmopt name | menu_id | | the menu id to act on if the select_if is true
// @parmopt name | id | | the control id
// @flag mask_password | if set, all letters are replaced with stars 
bool ScriptSetUIFromPreferences(Script::CStruct *pParams, Script::CScript *pScript)
{
	// gets either network or splitscreen prefs, as appropriate
	Prefs::Preferences* pPreferences = Mdl::GetPreferences( pParams );
	
	uint32 field_id;

	// look for either a field checksum or string
	if ( !pParams->GetChecksum("field", &field_id ) )
	{
		const char* pFieldName;
		pParams->GetText("field", &pFieldName, true);
		field_id = Script::GenerateCRC( pFieldName );
	}
	
	uint32 checksum_value1;
	if ( pParams->GetChecksum( "select_if", &checksum_value1 ) )
	{	
		Script::CStruct* pStructure = pPreferences->GetPreference( field_id );
		Dbg_Assert(pStructure);
		uint32 checksum_value2;
		pStructure->GetChecksum( "checksum", &checksum_value2, true );

		if ( checksum_value1 == checksum_value2 )
		{
			/*
			uint32 menu_id;
			pParams->GetChecksum( "menu_id", &menu_id, true );
			Front::MenuFactory* menu_factory = Front::MenuFactory::Instance();
			Front::MenuElement *pMenuElement=menu_factory->GetMenuElement(menu_id);
			Front::VerticalMenu *pMenu=static_cast<Front::VerticalMenu*>(pMenuElement);
			Dbg_MsgAssert(pMenu,("\n%s\nNo menu with id '%s' found",pScript->GetScriptInfo(),Script::FindChecksumName(menu_id)));

			Script::PrintContents(pParams);

			uint32 control_id;
			pParams->GetChecksum( "id", &control_id, true );
//			pParams->GetChecksum( "control_id", &control_id, true );
			pMenu->AttemptSelect( control_id );
			*/
		}
	}
	else
	{
		uint32 control_name;
		pParams->GetChecksum( "id", &control_name, true );

		int mask_password = pParams->ContainsFlag( "mask_password" );
	
		// update all the elements
		pPreferences->UpdateUIElement( control_name, field_id, mask_password );
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetUIFromSkaterProfile | 
// @parm name | field_id | the field id
// @parmopt name | select_if | | 
// @parmopt name | menu_id | | the menu id
// @parmopt name | control_id | | 
// @parmopt name | slider_id | |
bool ScriptSetUIFromSkaterProfile(Script::CStruct *pParams, Script::CScript *pScript)
{   
	/*
	uint32 field_id;
	pParams->GetChecksum( "field_id", &field_id, true );


	Front::MenuFactory* menu_factory = Front::MenuFactory::Instance();
	Obj::CSkaterProfile* pSkaterProfile;
	pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile(pParams);

	uint32 slider_id;
	uint32 control_id;
	uint32 checksum_value;
	if ( pParams->GetChecksum( "select_if", &checksum_value ) )
	{
		if ( checksum_value == pSkaterProfile->GetChecksumValue(field_id) )
		{
			uint32 menu_id;
			pParams->GetChecksum( "menu_id", &menu_id, true );
			Front::MenuElement *pMenuElement=menu_factory->GetMenuElement(menu_id);
			Front::VerticalMenu *pMenu=static_cast<Front::VerticalMenu*>(pMenuElement);
			Dbg_MsgAssert(pMenu,("\n%s\nNo menu with id '%s' found",pScript->GetScriptInfo(),Script::FindChecksumName(menu_id)));
			pParams->GetChecksum( "control_id", &control_id, true );
			pMenu->AttemptSelect( control_id );
		}
	}
	else if ( pParams->GetChecksum( "slider_id", &slider_id ) )
	{
		// slider element
		Front::SliderMenuElement* pSliderElement = static_cast<Front::SliderMenuElement*>( menu_factory->GetMenuElement( slider_id, true ) );
		pSliderElement->SetValue( pSkaterProfile->GetUIValue(field_id) );
	}
	else if ( pParams->GetChecksum( "control_id", &control_id ) )
	{		
		Front::MenuEvent event;
		event.SetTypeAndTarget(Front::MenuEvent::vSETCONTENTS, control_id );
		event.GetData()->AddComponent(Script::GenerateCRC("string"), ESYMBOLTYPE_STRING, pSkaterProfile->GetUIString(field_id).getString() );
		menu_factory->LaunchEvent(&event);
	}
	else
	{
		Dbg_Assert( 0 );
	}
	*/

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetPreferencesFromUI | changes field defined by string to value defined by field
// @parm string | string | 
// @parm string | field | 
// @parmopt name | level_checksum | | 
bool ScriptSetPreferencesFromUI(Script::CStruct *pParams, Script::CScript *pScript)
{
	// gets either network or splitscreen prefs, as appropriate
	Prefs::Preferences* pPreferences = Mdl::GetPreferences( pParams );

	const char* p_string;
	pParams->GetText( "string", &p_string, true );

	const char* pFieldName;
	pParams->GetText("field", &pFieldName, true);

	Dbg_Message( "Changing field %s to %s", pFieldName, p_string );
	
	// transfer data back into the preferences
	Script::CStruct* pTempStructure = new Script::CStruct;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, p_string );
	
	uint32 checksum;
	checksum = 0;
	pParams->GetChecksum( "checksum", &checksum );
	pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, (int) checksum );

	if( ( stricmp( pFieldName, "time_limit" ) == 0 ) ||
		( stricmp( pFieldName, "horse_time_limit" ) == 0 ))
	{
		int time;

		pParams->GetInteger( "time", &time );
		Dbg_Message( "Changing time to %d", time );
		pTempStructure->AddComponent( Script::GenerateCRC("time"), ESYMBOLTYPE_INTEGER, time );
	}

	if( stricmp( pFieldName, "target_score" ) == 0 )
	{
		int score;

		if( pParams->GetInteger( "score", &score ) == false )
		{
			pParams->GetInteger( "time", &score, Script::ASSERT );
			score = Tmr::Seconds( score );
		}
		
		Dbg_Message( "Changing score to %d", score );
		pTempStructure->AddComponent( Script::GenerateCRC("score"), ESYMBOLTYPE_INTEGER, score );
	}



	pPreferences->SetPreference( Script::GenerateCRC(pFieldName), pTempStructure );
	delete pTempStructure;
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetAllToDefaultStats | 
bool ScriptResetAllToDefaultStats( Script::CStruct *pParams, Script::CScript *pScript )
{
//	Tmr::Time baseTime = Tmr::ElapsedTime(0);

	Script::CArray* pMasterSkaterArray = Script::GetArray( "master_skater_list", Script::ASSERT );

	for ( uint32 i = 0; i < pMasterSkaterArray->GetSize(); i++ )
	{
		Script::CStruct* pTempStructure = new Script::CStruct;
		
		Script::CStruct* pOriginalInfo = pMasterSkaterArray->GetStructure( i );

		uint32 skaterName;
		pOriginalInfo->GetChecksum( CRCD(0xa1dc81f9,"name"), &skaterName, Script::ASSERT );

		Script::CArray* pStatNameArray = Script::GetArray( "stat_names" );
		for ( uint32 j = 0; j < pStatNameArray->GetSize(); j++ )
		{
			Script::CStruct* pStatInfo = pStatNameArray->GetStructure(j);
			
			uint32 statName;
			pStatInfo->GetChecksum( CRCD(0xa1dc81f9,"name"), &statName, Script::ASSERT );
			
			int statVal;
			pOriginalInfo->GetInteger( statName, &statVal, Script::ASSERT );
			pTempStructure->AddInteger( statName, statVal );
		}

		int pointsAvailable;
		pOriginalInfo->GetInteger( "points_available", &pointsAvailable, Script::ASSERT );
		pTempStructure->AddInteger( "points_available", pointsAvailable );

		// now add it to the corresponding skater profile
		Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
		uint32 numProfiles = pPlayerProfileManager->GetNumProfileTemplates();
		for ( uint32 j = 0; j < numProfiles; j++ )
		{
			Obj::CSkaterProfile* pProfile = pPlayerProfileManager->GetProfileTemplateByIndex(j);
			Dbg_Assert( pProfile );
			Script::CStruct* pInfo = pProfile->GetInfo();

			uint32 test;
			pInfo->GetChecksum( CRCD(0xa1dc81f9,"name"), &test, Script::ASSERT );
			if ( test == skaterName )
			{
				pInfo->AppendStructure( pTempStructure );
				break;
			}
		}
		
		delete pTempStructure;
	}

//	Dbg_Message( "ResetAllToDefaultStats took %d ms", Tmr::ElapsedTime( baseTime ) );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetToDefaultProfile | 
bool ScriptResetToDefaultProfile( Script::CStruct *pParams, Script::CScript *pScript )
{
//	Tmr::Time baseTime = Tmr::ElapsedTime(0);

	uint32 resetThisSkaterName;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &resetThisSkaterName, Script::ASSERT );

	Script::CArray* pMasterSkaterArray = Script::GetArray( "master_skater_list", Script::ASSERT );

	for ( uint32 i = 0; i < pMasterSkaterArray->GetSize(); i++ )
	{
		Script::CStruct* pOriginalInfo = pMasterSkaterArray->GetStructure( i );
		Dbg_Assert( pOriginalInfo );

		uint32 skaterName;
		pOriginalInfo->GetChecksum( CRCD(0xa1dc81f9,"name"), &skaterName, Script::ASSERT );

		if ( resetThisSkaterName == skaterName )
		{
			Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
			Obj::CSkaterProfile* pProfile = pPlayerProfileManager->GetProfileTemplate( skaterName );
			Dbg_Assert( pProfile );

			uint32 partial;
            if ( pParams->GetChecksum( CRCD(0x55f82f0b,"partial"), &partial, Script::NO_ASSERT ) )
            {
                // resets appearance, tricks, NOT STATS
                pProfile->PartialReset( pOriginalInfo );
            }
            else
            {
                // resets appearance, stats, tricks
    			pProfile->Reset( pOriginalInfo );
            }
		}
	}

//	Dbg_Message( "ResetAllToDefaultProfile took %d ms", Tmr::ElapsedTime( baseTime ) );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetAllToDefaultProfile | 
bool ScriptResetAllToDefaultProfile( Script::CStruct *pParams, Script::CScript *pScript )
{
//	Tmr::Time baseTime = Tmr::ElapsedTime(0);

	Script::CArray* pMasterSkaterArray = Script::GetArray( "master_skater_list", Script::ASSERT );

	for ( uint32 i = 0; i < pMasterSkaterArray->GetSize(); i++ )
	{
		Script::CStruct* pOriginalInfo = pMasterSkaterArray->GetStructure( i );
		Dbg_Assert( pOriginalInfo );

		uint32 skaterName;
		pOriginalInfo->GetChecksum( CRCD(0xa1dc81f9,"name"), &skaterName, Script::ASSERT );

		Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
		Obj::CSkaterProfile* pProfile = pPlayerProfileManager->GetProfileTemplate( skaterName );
		Dbg_Assert( pProfile );
			
		// resets appearance, stats, tricks
		pProfile->Reset( pOriginalInfo );
	}

//	Dbg_Message( "ResetAllToDefaultProfile took %d ms", Tmr::ElapsedTime( baseTime ) );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ForEachSkaterName | 
bool ScriptForEachSkaterName( Script::CStruct *pParams, Script::CScript *pScript )
{
//	Tmr::Time baseTime = Tmr::ElapsedTime(0);

	uint32 scriptToRun;
	pParams->GetChecksum( CRCD(0x62ba3f6a,"do"), &scriptToRun, Script::ASSERT );

	Script::CStruct* pSubParams = NULL;
	pParams->GetStructure( CRCD(0x7031f10c,"params"), &pSubParams, Script::NO_ASSERT );

	Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
	int numProfiles = pPlayerProfileManager->GetNumProfileTemplates();
	for ( int i = 0; i < numProfiles; i++ )
	{
		Obj::CSkaterProfile* pProfile = pPlayerProfileManager->GetProfileTemplateByIndex(i);
		Dbg_Assert( pProfile );
		Script::CStruct* pInfo = pProfile->GetInfo();

		uint32 skaterName;
		pInfo->GetChecksum( CRCD(0xa1dc81f9,"name"), &skaterName, Script::ASSERT );

		Script::CStruct* pTempStructure = new Script::CStruct;
		pTempStructure->AppendStructure( pSubParams );
		pTempStructure->AddChecksum( CRCD(0xa1dc81f9,"name"), skaterName );
		Script::RunScript( scriptToRun, pTempStructure );
		delete pTempStructure;
	}

//	Dbg_Message( "ScriptForEachSkaterName took %d ms", Tmr::ElapsedTime( baseTime ) );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ForEachSkaterProfile | 
bool ScriptForEachSkaterProfile( Script::CStruct *pParams, Script::CScript *pScript )
{
//	Tmr::Time baseTime = Tmr::ElapsedTime(0);

	uint32 scriptToRun;
	pParams->GetChecksum( CRCD(0x62ba3f6a,"Do"), &scriptToRun, Script::ASSERT );

	Script::CStruct* pSubParams = NULL;
	pParams->GetStructure( CRCD(0x7031f10c,"params"), &pSubParams, Script::NO_ASSERT );

	Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
	int numProfiles = pPlayerProfileManager->GetNumProfileTemplates();
	for ( int i = 0; i < numProfiles; i++ )
	{
		Obj::CSkaterProfile* pProfile = pPlayerProfileManager->GetProfileTemplateByIndex(i);
		Dbg_Assert( pProfile );
		Script::CStruct* pInfo = pProfile->GetInfo();

		Script::CStruct* pTempStructure = new Script::CStruct;
		pTempStructure->AppendStructure( pSubParams );
		pTempStructure->AppendStructure( pInfo );
		Script::RunScript( scriptToRun, pTempStructure );
		delete pTempStructure;
	}

//	Dbg_Message( "ScriptForEachSkaterProfile took %d ms", Tmr::ElapsedTime( baseTime ) );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetSkaterProfileInfoByName | appends the skater profile info to the
// calling script's params. returns false if the specified profile can't be found
// @parm name | name | the name to search for
bool ScriptGetSkaterProfileInfoByName( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 name;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &name, Script::ASSERT );
	
	Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
	int numProfiles = pPlayerProfileManager->GetNumProfileTemplates();
	for ( int i = 0; i < numProfiles; i++ )
	{
		Obj::CSkaterProfile* pProfile = pPlayerProfileManager->GetProfileTemplateByIndex(i);
		Dbg_Assert( pProfile );
		Script::CStruct* pInfo = pProfile->GetInfo();

		uint32 test;
		pInfo->GetChecksum( CRCD(0xa1dc81f9,"name"), &test, Script::ASSERT );
		if ( test == name )
		{
			pScript->GetParams()->AppendStructure( pInfo );
			return true;
		}
	}
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSkaterProfileInfoByName | appends the skater profile info to the
// calling script's params. returns false if the specified profile can't be found
// @parm name | name | the name to search for
// @parm structure | params | the params structure to append to the skater profile
bool ScriptSetSkaterProfileInfoByName( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 name;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &name, Script::ASSERT );

	Script::CStruct* pSubParams = NULL;
	pParams->GetStructure( "params", &pSubParams, Script::ASSERT );

	Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
	int numProfiles = pPlayerProfileManager->GetNumProfileTemplates();
	for ( int i = 0; i < numProfiles; i++ )
	{
		Obj::CSkaterProfile* pProfile = pPlayerProfileManager->GetProfileTemplateByIndex(i);
		Dbg_Assert( pProfile );
		Script::CStruct* pInfo = pProfile->GetInfo();

		uint32 test;
		pInfo->GetChecksum( CRCD(0xa1dc81f9,"name"), &test, Script::ASSERT );
		if ( test == name )
		{
			pInfo->AppendStructure( pSubParams );
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetDefaultAppearance | 
// @parm int | skater | the skater to reset
bool ScriptResetDefaultAppearance( Script::CStruct *pParams, Script::CScript *pScript )
{   

	Obj::CSkaterProfile* pSkaterProfile;
	pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile(pParams);
	pSkaterProfile->ResetDefaultAppearance();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetDefaultTricks | 
// @parm int | skater | the skater to reset
bool ScriptResetDefaultTricks( Script::CStruct *pParams, Script::CScript *pScript )
{	

	Obj::CSkaterProfile* pSkaterProfile;
	pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile(pParams);
	pSkaterProfile->ResetDefaultTricks();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetDefaultStats | 
// @parm int | skater | the skater to reset
bool ScriptResetDefaultStats( Script::CStruct *pParams, Script::CScript *pScript )
{   

	Obj::CSkaterProfile* pSkaterProfile;
	pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile(pParams);
	pSkaterProfile->ResetDefaultStats();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RandomizeAppearance | 
// @parm int | skater | the skater to randomize
bool ScriptRandomizeAppearance( Script::CStruct *pParams, Script::CScript *pScript )
{  
	Dbg_Message( "STUB:  RandomizeAppearance" );

	/*

	Obj::CSkaterProfile* pSkaterProfile;
	pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile(pParams);
	Gfx::CModelAppearance* pSkaterAppearance = pSkaterProfile->GetAppearance();
	Dbg_Assert( pSkaterAppearance );
	pSkaterAppearance->Randomize();
	 */

	// Randomize: Randomizes parts, height, weight
	// Possibly to be moved into script...
	// Needs to take disqualifications into account

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PrintCurrentAppearance | 
// @parm int | skater | the skater num
bool ScriptPrintCurrentAppearance(Script::CStruct *pParams, Script::CScript *pScript)
{   

	
	Obj::CSkaterProfile* pSkaterProfile;
	pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile(pParams);
	Gfx::CModelAppearance* pAppearance;
	pAppearance = pSkaterProfile->GetAppearance();
	pAppearance->PrintContents(pAppearance->GetStructure());
	
	/*const Gfx::CFaceTexture* p_face = pAppearance->GetFaceTexture();
	if( p_face && p_face->IsValid())
		pAppearance->GetFaceTexture()->PrintContents();*/
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetNeversoftSkater | 
// @parm int | skater | the skater num
// @parm structure | appearance | current appearance structure 
// @parm structure | info | current info structure
bool ScriptSetNeversoftSkater(Script::CStruct *pParams, Script::CScript *pScript)
{


	// debug info:
//	printf( "Set Neversoft Skater" );
//	pParams->PrintContents();

	// make sure it's a custom skater
	Obj::CSkaterProfile* pSkaterProfile;
	pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile(pParams);
	Dbg_MsgAssert( !pSkaterProfile->IsPro(), ( "This function only works on a custom skater." ) );

	// get the current appearance structure
	Script::CStruct* pAppearanceStructure;
	pParams->GetStructure( "appearance", &pAppearanceStructure, true );

	// apply the desired appearance structure
	Gfx::CModelAppearance* pAppearance = pSkaterProfile->GetAppearance();
	pAppearance->Load( pAppearanceStructure );

	// get the current info structure
	Script::CStruct* pInfoStructure;
	pParams->GetStructure( "info", &pInfoStructure, true );

	// append the new info structure
	Script::CStruct* pInfo = pSkaterProfile->GetInfo();
	pInfo->AppendStructure( pInfoStructure );

#ifdef __NOPT_ASSERT__
	Script::PrintContents(pInfo);
#endif
	// TODO:  since the career is being reset,
	// we should clear the info too (like stats and trick configs)
	
	pSkaterProfile->SetHeadIsLocked( true );

	// reset the career as well
//	Obj::CSkaterCareer* pCareer = pSkaterProfile->GetCareer();
//	pCareer->Reset();
//
//	// reset the career flags
//	for ( int i = 0; i < Script::GetInt( "FIRST_SHARED_GLOBAL_FLAG" ); i++ )
//	{
//		pCareer->UnSetGlobalFlag( i );
//	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CurrentProfileIsLocked | returns true if this skater profile is locked
// @parm int | skater | the skater num
bool ScriptCurrentProfileIsLocked(Script::CStruct *pParams, Script::CScript *pScript)
{

	Obj::CSkaterProfile* pSkaterProfile;
	pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile(pParams);
	return pSkaterProfile->IsLocked();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetSkaters | skip skaters to their restart points
// can specify either a node number or a node name
// the node number is ued by the "goto restart" menu
// @parmopt int | node_number | -1 | number of a node to skip the skater to 
// @parmopt name | node_name |  | name of a node to skip the skater to 
bool ScriptResetSkaters(Script::CStruct *pParams, Script::CScript *pScript)
{
	int node = -1;
	if (!pParams->GetInteger("node_number",&node))
	{
		uint32	node_name;
		if (pParams->GetChecksum("node_name",&node_name))
		{
			node = SkateScript::FindNamedNode(node_name);
		}	
	}
						 
	Mdl::Skate::Instance()->ResetSkaters(node, pParams->ContainsFlag(CRCD(0xd6f06bf6, "RestartWalking")));
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSkaterProfileInfo | Appends the structure to the specified skater
// profile info.  (Should only append the item if the item is of the same
// type)
// @parm int | player | the player slot to set
// @parm structure | params | the stuff to set
bool ScriptSetSkaterProfileInfo(Script::CStruct *pParams, Script::CScript *pScript)
{
	// now append appropriate data
	int 	playerNum;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &playerNum, Script::ASSERT );

	Script::CStruct* pAppendStruct;
	pParams->GetStructure( "params", &pAppendStruct, Script::ASSERT );

	Obj::CPlayerProfileManager* pProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
	Obj::CSkaterProfile* pProfile = pProfileManager->GetProfile( playerNum );
	Script::CStruct* pStruct = pProfile->GetInfo();

	pStruct->AppendStructure( pAppendStruct );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetSkaterProfileInfo | Returns the skater profile info in the script params
// @parm int | player | the player slot to get
// @parm float | value | value 
bool ScriptGetSkaterProfileInfo(Script::CStruct *pParams, Script::CScript *pScript)
{
	int 	playerNum;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &playerNum, Script::ASSERT );	
	
	Obj::CPlayerProfileManager* pProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
	Obj::CSkaterProfile* pProfile = pProfileManager->GetProfile( playerNum );
	
	Dbg_MsgAssert( pScript, ( "NULL pScript" ) );
	Dbg_MsgAssert( pScript->GetParams(), ( "NULL pScript params" ) );

	pScript->GetParams()->AppendStructure( pProfile->GetInfo() );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSkaterProfileProperty | Debugging function to set individual stats
// @parm int | stat | the stat to set
// @parm float | value | value 
bool ScriptSetSkaterProfileProperty(Script::CStruct *pParams, Script::CScript *pScript)
{   
	int 	playerNum;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &playerNum, Script::ASSERT );

	uint32 	stat;
	int		value;
	pParams->GetChecksum( NONAME, &stat );
	pParams->GetInteger( NONAME, &value );

	
	Obj::CPlayerProfileManager* pProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
	Obj::CSkaterProfile* pProfile = pProfileManager->GetProfile( playerNum );

	return pProfile->SetPropertyValue( stat, value );

/*	// If no skater was specified, change the stats of all local skaters
	if( pScript->mpObject == NULL )
	{
		for( i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
		{
			pSkater = Mdl::Skate::Instance()->GetSkater(i);
			if (pSkater && pSkater->IsLocalClient())			// Skater might not exist
			{
				pSkater->SetStat((Obj::CSkater::EStat)stat,value);
			}
		}
	}
	else
	{
		pSkater = static_cast <Obj::CSkater*>( pScript->mpObject );
		Dbg_Assert( pSkater );

		pSkater->SetStat((Obj::CSkater::EStat)stat,value);
	}
	
	return true;
*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleAlwaysSpecial | ToggleAlwaysSpecial (on/off) fixes the 
// special meter to be always full. <nl>
// ToggleAlwaysSpecial         ; Toggle it (useful for puttin on a button) <nl>
// ToggleAlwaysSpecial on      ; Sets it on <nl>
// ToggleAlwaysSpecial off     ; Sets it off <nl>
// Note that when you set it on, the special meter is immediately full.
// However, when you turn it off, then the special meter will just decay 
// normally, so it might not be immediately obvious that you switched it off. 
bool ScriptToggleAlwaysSpecial(Script::CStruct *pParams, Script::CScript *pScript)
{
	bool on = pParams->ContainsFlag("on");
	bool off = pParams->ContainsFlag("off");
	

	for (int i=0;i<8;i++)
	{
		Obj::CSkater *pSkater = Mdl::Skate::Instance()->GetSkater(i);						   
		if (pSkater)			// Skater might not exist
		{
			if (on)
			{
				pSkater->SetAlwaysSpecial(true);
			}
			else if (off)
			{
				pSkater->SetAlwaysSpecial(false);
			}
			else 
			{
				pSkater->ToggleAlwaysSpecial();
			}
		}	
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | SkaterSpeedGreaterThan | Check the skater speed, as if to have a ped
// not fall when the skater is stationary
// @parm float | speed | Speed to check against
bool ScriptSkaterSpeedGreaterThan( Script::CStruct *pParams, Script::CScript *pScript )
{
	float speed;
	if ( pParams->GetFloat( NONAME, &speed ) )
	{
		
		Obj::CSkater *pSkater = Mdl::Skate::Instance()->GetLocalSkater();						   
		if ( pSkater )
		{
			float skaterVel = pSkater->GetVel( ).Length( );
			if ( skaterVel > speed)
			{
				return true;
			}
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | SkaterSpeedLessThan | Check the skater speed, as if to have a ped
// not fall when the skater is stationary
// @parm float | speed | Speed to check against
bool ScriptSkaterSpeedLessThan( Script::CStruct *pParams, Script::CScript *pScript )
{   
	float speed;
	if ( pParams->GetFloat( NONAME, &speed ) )
	{
		
		Obj::CSkater *pSkater = Mdl::Skate::Instance()->GetLocalSkater();						   
		if ( pSkater )
		{
			float skaterVel = pSkater->GetVel( ).Length( );
			if ( skaterVel < speed )
			{
				return true;
			}
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | SkaterLastScoreLandedGreaterThan | Detects last score to trigger if script.
// Example: if SkaterLastScoreLandedGreaterThan 2000 --do cool stuff-- endif
// @uparm 1 | Score (int)
bool ScriptLastScoreLandedGreaterThan( Script::CStruct *pParams, Script::CScript *pScript )
{   
	int score;
	score = s_get_score_from_params( pParams, pScript );
	Mdl::Score *pScore;
	pScore = s_get_score_struct( );
	if ( pScore )
	{
		int scorePot = pScore->GetLastScoreLanded();
		if ( scorePot > score )
			return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | SkaterLastScoreLandedLessThan | Detects last score to trigger if script.
// Example: if SkaterLastScoreLandedLessThan 2000 --do cool stuff-- endif
// @uparm 1 | Score (int)
bool ScriptLastScoreLandedLessThan( Script::CStruct *pParams, Script::CScript *pScript )
{   
	int score;
	score = s_get_score_from_params( pParams, pScript );
	
	Mdl::Score *pScore;
	
	pScore = s_get_score_struct( );
	if ( pScore )
	{
		int scorePot = pScore->GetLastScoreLanded();
		if ( scorePot < score )
			return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | AnySkaterTotalScoreGreaterThan | Checks if any skaters' score is greater than the score paramter
// Example: if AnySkaterTotalScoreGreaterThan 2000 --do cool stuff-- endif
// @uparm 1 | Score (int)
bool ScriptAnyTotalScoreAtLeast( Script::CStruct *pParams, Script::CScript *pScript )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	
	GameNet::PlayerInfo* player;
	Lst::Search< GameNet::PlayerInfo > sh;
	int score;
	score = s_get_score_from_params( pParams, pScript );

	if( Mdl::Skate::Instance()->GetGameMode()->IsTeamGame())
	{
		int i;
		int total_score[GameNet::vMAX_TEAMS] = {0};

		for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
		{
			Mdl::Score* pScore = player->m_Skater->GetScoreObject();

			total_score[player->m_Team] += pScore->GetTotalScore();
		}

		for( i = 0; i < GameNet::vMAX_TEAMS; i++ )
		{
			if( total_score[i] >= score )
			{
				return true;
			}
		}
	}
	else
	{
		for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
		{
			if( player->m_Skater )
			{
				Mdl::Score *pScore;
	
				pScore = player->m_Skater->GetScoreObject();
				int totalScore = pScore->GetTotalScore();
				if( totalScore >= score )
				{
					return true;
				}
			}
		}
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptOnlyOneSkaterLeft( Script::CStruct *pParams, Script::CScript *pScript )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	
	GameNet::PlayerInfo* player;
	Lst::Search< GameNet::PlayerInfo > sh;
	Lst::Search< GameNet::NewPlayerInfo > new_sh;
	int num_participants_left;
	
	num_participants_left = 0;
	if( Mdl::Skate::Instance()->GetGameMode()->IsTeamGame())
	{
		int i;
		int total_score[GameNet::vMAX_TEAMS] = {0};

		for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
		{
			Mdl::Score* pScore = player->m_Skater->GetScoreObject();

			total_score[player->m_Team] += pScore->GetTotalScore();
		}

		for( i = 0; i < GameNet::vMAX_TEAMS; i++ )
		{
			if( total_score[i] > 0 )
			{
				num_participants_left++;
			}
		}

		// If we're still waiting to load players fully, this test is invalid
		if( gamenet_man->FirstNewPlayerInfo( new_sh ))
		{
			return false;
		}
	}
	else
	{
		int num_players;

		num_players = 0;
		for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
		{
			if( player->m_Skater )
			{
				Mdl::Score *pScore;
	
				num_players++;
				pScore = player->m_Skater->GetScoreObject();
				int totalScore = pScore->GetTotalScore();
				if( totalScore > 0 )
				{
					num_participants_left++;
				}
			}
		}

		// If we're still waiting to load players fully, this test is invalid
		if( gamenet_man->FirstNewPlayerInfo( new_sh ))
		{
			return false;
		}

		if( num_players == 1 )
		{
			return false;
		}
	}
	
	if( gamenet_man->OnServer())
	{
		if(	( gamenet_man->GetHostMode() == GameNet::vHOST_MODE_AUTO_SERVE ) ||
			( gamenet_man->GetHostMode() == GameNet::vHOST_MODE_FCFS ))
		{
			if( num_participants_left == 0 )
			{
				return true;
			}
		}
	}
	
	return num_participants_left == 1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | SkaterTotalScoreGreaterThan | Detects last score to trigger if script.
// Example: if SkaterTotalScoreGreaterThan 2000 --do cool stuff-- endif
// @uparm 1 | Score (int)
bool ScriptTotalScoreGreaterThan( Script::CStruct *pParams, Script::CScript *pScript )
{
	
	int score;
	score = s_get_score_from_params( pParams, pScript );
	Mdl::Score *pScore;
	pScore = s_get_score_struct( );
	if ( pScore )
	{
		int totalScore = pScore->GetTotalScore();
		if ( totalScore > score )
			return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | SkaterTotalScoreLessThan | Detects last score to trigger if script.
// Example: if SkaterTotalScoreLessThan 2000 --do cool stuff-- endif
// @uparm 1 | Score (int)
bool ScriptTotalScoreLessThan( Script::CStruct *pParams, Script::CScript *pScript )
{   
	int score;
	score = s_get_score_from_params( pParams, pScript );
	Mdl::Score *pScore;
	pScore = s_get_score_struct( );
	if ( pScore )
	{
		int totalScore = pScore->GetTotalScore();
		if ( totalScore < score )
			return ( true );
	}
	return ( false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | SkaterCurrentScorePotGreaterThan | Detects last score to trigger if script.
// Example: if SkaterCurrentScorePotGreaterThan 2000 --do cool stuff-- endif
// @uparm 1 | Score (int)
bool ScriptCurrentScorePotGreaterThan( Script::CStruct *pParams, Script::CScript *pScript )
{   
	int score;
	int scale;
	
	scale = 1;
	score = s_get_score_from_params( pParams, pScript );
	pParams->GetInteger( CRCD(0xb08c5ae8,"point_scale"), &scale );
	score *= scale;

	Mdl::Score *pScore;
	pScore = s_get_score_struct( );
	if ( pScore )
	{
		int curScore = pScore->GetScorePotValue();
		if ( curScore > score )
			return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script bool | SkaterCurrentScorePotLessThan | Detects last score to trigger if script.
// Example: if SkaterCurrentScorePotLessThan 2000 --do cool stuff-- endif
// @uparm 1 | Score (int)
bool ScriptCurrentScorePotLessThan( Script::CStruct *pParams, Script::CScript *pScript )
{   
	int score;
	score = s_get_score_from_params( pParams, pScript );
	Mdl::Score *pScore;
	pScore = s_get_score_struct( );
	if ( pScore )
	{
		int curScore = pScore->GetScorePotValue();
		if ( curScore < score )
			return true;
	}
	return false;
}


// @script | SkaterGetScoreInfo | Adds the parameters ScorePot, TotalScore and LastScoreLanded to the script's parameters.
bool ScriptSkaterGetScoreInfo( Script::CStruct *pParams, Script::CScript *pScript )
{   
	Mdl::Score *p_score=s_get_score_struct( );
	
	pScript->GetParams()->AddInteger(CRCD(0x95e84391,"ScorePot"),p_score->GetScorePotValue());
	pScript->GetParams()->AddInteger(CRCD(0xee5b2b48,"TotalScore"),p_score->GetTotalScore());
	pScript->GetParams()->AddInteger(CRCD(0xea60ac69,"LastScoreLanded"),p_score->GetLastScoreLanded());
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalGreaterThan | Counts the number of goals you have got.
// @uparm 1.0 | value to check
bool ScriptGoalsGreaterThan( Script::CStruct *pParams, Script::CScript *pScript )
{														     
	float num;
	if ( pParams->GetFloat( NONAME, &num ) )
	{
	
		Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();	
		if (pCareer->CountTotalGoalsCompleted() > num)
		{
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalsEqualTo | Counts the number of goals you have got. 
// @uparm 1.0 | number of goals
bool ScriptGoalsEqualTo( Script::CStruct *pParams, Script::CScript *pScript )
{  
	float num;
	if ( pParams->GetFloat( NONAME, &num ) )
	{
	
		Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();	
		if (pCareer->CountTotalGoalsCompleted() == num)
		{
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MedalsGreaterThan |  Counts the number of medals you have got.  This 
// is just the number of levels you have gotten a medal in, so it will be 0 - 3.
// @uparm 1.0 | number of medals
bool ScriptMedalsGreaterThan( Script::CStruct *pParams, Script::CScript *pScript )
{														     
	float num;
	if ( pParams->GetFloat( NONAME, &num ) )
	{
	
		Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();	
		if (pCareer->CountMedals() > num)
		{
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | MedalsEqualTo |  Counts the number of medals you have got.  This 
// is just the number of levels you have gotten a medal in, so it will be 0 - 3.
// @uparm 1.0 | number of medals
bool ScriptMedalsEqualTo( Script::CStruct *pParams, Script::CScript *pScript )
{														  
	float num;
	if ( pParams->GetFloat( NONAME, &num ) )
	{
	
		Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();	
		if (pCareer->CountMedals() == num)
		{
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ToggleSkaterCamMode | Toggles the camera mode for a given skater
// @parm int | skater | 
bool ScriptToggleSkaterCamMode(Script::CStruct *pParams, Script::CScript *pScript)
{
	int skater;
	pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skater );

	
	if (!Mdl::Skate::Instance()->GetGameMode()->IsFrontEnd())
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		
		Obj::CSkater* p_skater = NULL;
		
		if (skater == 0)
		{
			GameNet::PlayerInfo* player;
			player = gamenet_man->GetLocalPlayer();
			if( player )
			{
				p_skater = player->m_Skater;
			}
		}
		else
		{											 
			p_skater = Mdl::Skate::Instance()->GetSkater( skater );
		}
		
		if( p_skater )
		{
			/*
			if ( p_skater->GetSkaterCam()->mMode < Obj::CSkaterCam::SKATERCAM_NUM_NORMAL_MODES )
			{
				p_skater->ToggleCamMode();
			}
			*/
			GetSkaterCameraComponentFromObject(p_skater->GetCamera())->ToggleMode();
			
		}
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetSkaterID | returns the id of the skater, so that you can run member functions on it from a global script
// @parm int | skater | Which skater's ID you want to grab
// The skater's id is returned in objID, check the script for examples

bool ScriptGetSkaterID( Script::CStruct* pParams, Script::CScript* pScript )
{
	// in this context, skater really means "viewport"
	int skaterId;
	Obj::CSkater* pSkater = NULL;
	if ( pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skaterId ) )
		 pSkater = Mdl::Skate::Instance()->GetSkater( skaterId );
	else	
		pSkater = Mdl::Skate::Instance()->GetLocalSkater();
	
	if ( pSkater && pSkater->IsLocalClient() )
	{
		uint32 id = pSkater->GetID();
		Dbg_Assert( pScript && pScript->GetParams() );
		pScript->GetParams()->AddChecksum( "objID", id );
		// Dbg_Printf( "************** SKATER ID %d\n", id );
		return true;
	}
	Dbg_MsgAssert( 0, ( "Couldn't find specified skater" ) );
	return false;
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

bool ScriptGetCurrentSkaterID( Script::CStruct* pParams, Script::CScript* pScript )
{
	Dbg_Assert( pScript->mpObject );

	Obj::CSkater* pSkater = static_cast <Obj::CSkater*>( pScript->mpObject.Convert() );
	if( pSkater && pSkater->IsLocalClient() )
	{
		uint32 id = pSkater->GetID();
		Dbg_Assert( pScript && pScript->GetParams() );
		pScript->GetParams()->AddChecksum( "objID", id );
		//Dbg_Printf( "************** SKATER ID %d\n", id );
		return true;
	}
	Dbg_MsgAssert( 0, ( "Couldn't find specified skater" ) );
	return false;
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

// @script | SetSkaterCamLerpReductionTimer | 
// @parmopt float | time | 0.0 | time value
bool ScriptSetSkaterCamLerpReductionTimer( Script::CStruct *pParams, Script::CScript *pScript )
{  
/*	
	Obj::CSkater* p_skater = static_cast <Obj::CSkater*>( pScript->mpObject.Convert() );
	Dbg_Assert( p_skater );

	Obj::CSkaterCam* p_cam = p_skater->GetSkaterCam();
	if( p_cam == NULL )
	{
		return true;
	}

	float time = 0.0f;
	pParams->GetFloat( 0x906b67ba, &time, true );  // time

	p_cam->SetLerpReductionTimer( time );
*/
//	printf ("skfuncs %d: SUTUBBBEDDDDDDDDDDDDDDDDDDDDDDDD\n",__LINE__);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PlaySkaterCamAnim | Play a camera animation for a given skater <nl>
// Example usage:
// script play_photoguy_cam
//		TRG_G_TS_PhotoGuy:Obj_GetID
//		PlaySkaterCamAnim skater=0 name=G_TS_CameraStart targetID=objID targetOffset=(0,50,0) positionOffset=(-100, 50, -100)
//		; camera looks at a point 50 units above the photoguy, camera is fixed to a position 50 units above, 100 units behind, and 100 units to the left
// endscript
// @parmopt int | skater | 0 |
// @parmopt name | skaterId | | find the skater by id, rather than index
// @flag stop | 
// @parm name | name | the name of the animation
// @flag loop | loop the animation
// @flag focus_skater | 
// @flag play_hold | 
// @parmopt int | skippable | | whether the movie is skippable
// @parmopt name | exitScript | | name of script to run when the movie finishes/is skipped through
// @parmopt structure | exitParams | | additional parameters to be passed to the exitScript
// @parmopt name | targetID | | m_id of the object to follow (not the node's name!  see example below) 
// @parmopt vector | targetOffset | | offset from the specified targetID to look at (in local space, not world space)
// @parmopt vector | positionOffset | | overrides the camera position with the specified offset from the specified targetID
bool ScriptPlaySkaterCamAnim( Script::CStruct *pParams, Script::CScript *pScript )
{
	if ( pParams->ContainsFlag( CRCD(0x0c567fa2,"use_last_camera") ) )
	{
		// used so that we don't go back to the skatercam
		// when the "want to save" dialog comes up
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		skate_mod->GetMovieManager()->ApplyLastCamera();
		return true;
	}

 	// in this context, skater really means "viewport"
	int skaterIndex = 0;
	uint32 skaterId;
	Obj::CSkater* pSkater = NULL;
	if ( pParams->GetChecksum( "skaterId", &skaterId, Script::NO_ASSERT ) )
	{
		Obj::CObject* pObj = Obj::ResolveToObject( skaterId);
		Dbg_MsgAssert( pObj && pObj->GetType() == SKATE_TYPE_SKATER, ( "%x is not a skater.", skaterId ) );
		pSkater = (Obj::CSkater*)pObj;
	}
	else
	{
		pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skaterIndex );
		pSkater = Mdl::Skate::Instance()->GetSkater( skaterIndex );
	}
	
	if ( pSkater )
	{
		if ( pSkater->IsLocalClient() )
		{
			// The stop flag is read with highest priority.
			if ( pParams->ContainsFlag( "stop" ) )
			{
				Mdl::Skate * skate_mod = Mdl::Skate::Instance();
				skate_mod->GetMovieManager()->ClearMovieQueue();
				return true;
			}
			else
			{
				uint32 movieName = 0;
				pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName );

				// GJ:  The camera may not be framed properly if the
				// skater was scaled up, so the following script will
				// shut off the scaling on the skater...  restore_skater_camera
				// will re-enable it when the movie is done
				Script::RunScript( CRCD(0xdf00bdff,"disable_skater_scaling") );

				// start the movie with the default params...
				Mdl::Skate * skate_mod = Mdl::Skate::Instance();
				skate_mod->GetMovieManager()->AddMovieToQueue( movieName, pParams, CRCD(0x66b7dd11,"skatercamanim") );
			}
		}
		else
		{
			if ( !pSkater->IsLocalClient() )
			{
				printf("this isn't a local skater!\n");
			}
		}
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSkaterCamAnimSkippable | set the current anim for a given
// skater as skippable
// @parmopt int | skater | 0 | the skater
// @parmopt name | skaterId | | the id of the skater
// @parmopt int | name | | the name of the movie to set skippable (otherwise, it will use the currently-playing movie)
// @parmopt int | skippable | 0 | set to anything greater than 0 to set anim to skippable
bool ScriptSetSkaterCamAnimSkippable( Script::CStruct *pParams, Script::CScript *pScript )
{
	// in this context, skater really means "viewport"
	uint32 skaterId;
	int skaterIndex = 0;

	Obj::CSkater* pSkater = NULL;
	if ( pParams->GetChecksum( "skaterId", &skaterId, Script::NO_ASSERT ) )
	{
		Obj::CObject* pObj = Obj::ResolveToObject( skaterId);
		Dbg_MsgAssert( pObj && pObj->GetType() == SKATE_TYPE_SKATER, ( "%x is not a CSkater", skaterId ) );
		pSkater = (Obj::CSkater*)pObj;
	}
	else
	{
		pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skaterIndex );
		pSkater = Mdl::Skate::Instance()->GetSkater( skaterIndex );
	}
		
	if( pSkater && pSkater->IsLocalClient() )
	{
		uint32 movieName = 0;
		pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName );

		int skippable = 0;
		pParams->GetInteger( "skippable", &skippable );

		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		skate_mod->GetMovieManager()->SetMovieSkippable( movieName, skippable );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSkaterCamAnimShouldPause | changes the pause mode of a skater cam.
// If the pause mode is set to false, the cam will play even if the game is paused
// @parmopt name | skaterId | | the id of the skater
// @parmopt int | skater | 0 | the skater index
bool ScriptSetSkaterCamAnimShouldPause( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 skaterId;
	int skaterIndex = 0;
	Obj::CSkater* pSkater = NULL;
	if ( pParams->GetChecksum( "skaterId", &skaterId, Script::NO_ASSERT ) )
	{
		Obj::CObject* pObj = Obj::ResolveToObject( skaterId);
		Dbg_MsgAssert( pObj && pObj->GetType() == SKATE_TYPE_SKATER, ( "%x is not a CSkater", skaterId ) );
		pSkater = (Obj::CSkater*)pObj;
	}
	else
	{
		pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skaterIndex );
		pSkater = Mdl::Skate::Instance()->GetSkater( skaterIndex );
	}

	if ( pSkater && pSkater->IsLocalClient() )
	{
		uint32 movieName = 0;
		pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName );

		int should_pause = 0;
		pParams->GetInteger( "should_pause", &should_pause );

		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		skate_mod->GetMovieManager()->SetMoviePauseMode( movieName, should_pause );
	}
	return true;
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

// @script | GetCurrentSkaterCamAnimName | returns the name of the current skater cam (if any)
bool ScriptGetCurrentSkaterCamAnimName( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	uint32 movieName = skate_mod->GetMovieManager()->GetCurrentMovieName();
	if ( movieName != 0 )
	{
		pScript->GetParams()->AddChecksum( CRCD(0xa1dc81f9,"name"), movieName );
		return true;
	}

	pScript->GetParams()->AddString( CRCD(0xa1dc81f9,"name"), "none" );
	return false;
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

// @script | GetSkaterCamAnimParams | returns the params of the current skater cam 
bool ScriptGetSkaterCamAnimParams( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 name = 0;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &name, Script::NO_ASSERT );
	
	uint32 skaterId;
	int skaterIndex = 0;
	Obj::CSkater* pSkater = NULL;
	if ( pParams->GetChecksum( "skaterId", &skaterId, Script::NO_ASSERT ) )
	{
		Obj::CObject* pObj = Obj::ResolveToObject( skaterId);
		Dbg_MsgAssert( pObj && pObj->GetType() == SKATE_TYPE_SKATER, ( "%x is not a CSkater", skaterId ) );
		pSkater = (Obj::CSkater*)pObj;
	}
	else
	{
		pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skaterIndex );
		pSkater = Mdl::Skate::Instance()->GetSkater( skaterIndex );
	}

	if ( pSkater && pSkater->IsLocalClient() )
	{
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		Script::CStruct* pMovieParams = skate_mod->GetMovieManager()->GetMovieParams( name );
		if ( pMovieParams )
		{
			pScript->GetParams()->AppendStructure( pMovieParams );
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

// @script | SkaterCamAnimFinished | returns true if the anim for the
// given skater is finished
// @parmopt int | skater | 0 | the skater
// @parmopt name | skaterId | | the id of the skater
// @parmopt int | cam | 1 | the camera number 
bool ScriptSkaterCamAnimFinished( Script::CStruct *pParams, Script::CScript *pScript )
{
	// in this context, skater really means "viewport"
	uint32 skaterId;
	int skaterIndex = 0;
	Obj::CSkater* pSkater = NULL;
	if ( pParams->GetChecksum( "skaterId", &skaterId, Script::NO_ASSERT ) )
	{
		Obj::CObject* pObj = Obj::ResolveToObject( skaterId);
		Dbg_MsgAssert( pObj && pObj->GetType() == SKATE_TYPE_SKATER, ( "%x is not a CSkater", skaterId ) );
		pSkater = (Obj::CSkater*)pObj;
	}
	else
	{
		pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skaterIndex );
		pSkater = Mdl::Skate::Instance()->GetSkater( skaterIndex );
	}
		
	if( pSkater && pSkater->IsLocalClient() )
	{
		uint32 movieName = 0;
		pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName );

		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		return skate_mod->GetMovieManager()->IsMovieComplete( movieName );
	}

	return true;
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

// @script | SkaterCamAnimHeld | returns true if anim is held
// @parmopt int | skater | 0 | the skater
// @parmopt name | skater | | the id of the skater
bool ScriptSkaterCamAnimHeld( Script::CStruct *pParams, Script::CScript *pScript )
{	
	// in this context, skater really means "viewport"
	uint32 skaterId;
	int skaterIndex = 0;
	Obj::CSkater* pSkater = NULL;
	if ( pParams->GetChecksum( "skaterId", &skaterId, Script::NO_ASSERT ) )
	{
		Obj::CObject* pObj = Obj::ResolveToObject( skaterId);
		Dbg_MsgAssert( pObj && pObj->GetType() == SKATE_TYPE_SKATER, ( "%x is not a CSkater", skaterId ) );
		pSkater = (Obj::CSkater*)pObj;
	}
	else
	{
		pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skaterIndex );
		pSkater = Mdl::Skate::Instance()->GetSkater( skaterIndex );
	}
		
	if( pSkater && pSkater->IsLocalClient() )
	{
		uint32 movieName = 0;
		pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName );

		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		return skate_mod->GetMovieManager()->IsMovieHeld( movieName );
	}

	return true;
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

// @script | KillSkaterCamAnim | 
// @parmopt int | skater | 0 | the skater
// @parmopt name | skaterId | | the id of the skater
// @flag current | kill the current cam anim
// @flag all | kill all cam anims
bool ScriptKillSkaterCamAnim( Script::CStruct *pParams, Script::CScript *pScript )
{
	bool success = true;

	// in this context, skater really means "viewport"
	uint32 skaterId;
	int skaterIndex = 0;
	Obj::CSkater* pSkater = NULL;
	if ( pParams->GetChecksum( "skaterId", &skaterId, Script::NO_ASSERT ) )
	{
		Obj::CObject* pObj = Obj::ResolveToObject( skaterId);
		Dbg_MsgAssert( pObj && pObj->GetType() == SKATE_TYPE_SKATER, ( "%x is not a CSkater", skaterId ) );
		pSkater = (Obj::CSkater*)pObj;
	}
	else if( pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skaterIndex ))
	{
		pSkater = Mdl::Skate::Instance()->GetSkater( skaterIndex );
	}
	else
	{
		pSkater = Mdl::Skate::Instance()->GetLocalSkater();
	}
		
	if( pSkater && pSkater->IsLocalClient() )
	{
		if ( pParams->ContainsFlag( "current" ) )
		{
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			success = skate_mod->GetMovieManager()->AbortCurrentMovie( false );
		}
		else if ( pParams->ContainsFlag( "all" ) )
		{
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			skate_mod->GetMovieManager()->ClearMovieQueue();
			success = true;
		}
		else
		{
			uint32 movieName = 0;
			pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName, true );
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			skate_mod->GetMovieManager()->RemoveMovieFromQueue( movieName );
			success = true;
		}
	}

	// GJ:  if the movie queue is empty, it's time to restore the skater
	// camera (this fixes SK5:TT13239 - "View goals - camera stays on
	// view goal position instead of returning to skater when unpausing"
	// or one of the park editor cameras
	if ( !Mdl::Skate::Instance()->GetMovieManager()->IsRolling() )
	{
		if ( Ed::CParkEditor::Instance()->EditingCustomPark() )
		{
			Ed::CParkEditor::Instance()->SetAppropriateCamera();	
		}
		else
		{
			Script::RunScript( CRCD(0x15674315,"restore_skater_camera") );
		}
	}

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ReloadSkaterCamAnim | undocumented
bool ScriptReloadSkaterCamAnim( Script::CStruct* pParams, Script::CScript* pScript )
{
	if ( !Config::GotExtraMemory() )
	{
		Dbg_Message( "ReloadSkaterCamAnim:  This function only works with the extra debug heap.  (Otherwise, it would fragment the bottom-up heap.)" );
		return false;
	}

	uint32 animName;
	const char* pFileName;

	if ( !pParams->GetChecksum( NONAME, &animName, false ) )
	{
		Dbg_Message( "ReloadSkaterCamAnim:  Missing cam anim name.\n" );
		return false;
	}

	if ( !pParams->GetText( NONAME, &pFileName, false )	)
	{
		Dbg_Message( "ReloadSkaterCamAnim:  Missing cam anim filename.\n" );
		return false;
	}

	Dbg_Message( "Reloading movie %08x with file %s...", animName, pFileName );

	// clear any existing movies in the queue, just in case we were in the middle of playing the anim to be replaced
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	skate_mod->GetMovieManager()->ClearMovieQueue();

	// reload the appropriate asset
	Ass::CAssMan * ass_man = Ass::CAssMan::Instance();
	
	// on the debug heap
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
	
	bool success =  ass_man->ReloadAsset( animName, pFileName, false );
	
	Mem::Manager::sHandle().PopContext();

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PlayCutscene | Play a cutscene
bool ScriptPlayCutscene( Script::CStruct *pParams, Script::CScript *pScript )
{
	if( pParams->ContainsFlag( "stop" ))
	{
		Dbg_MsgAssert( 0, ( "stop parameter doesn't work with PlayCutscene" ) );
	}
	
//	uint32 movieName = 0;
//	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName );

	// all cutscenes are named as "cutscene"...  this will
	// help me enforce that there is only one cutscene playing
	// at any given time...
	uint32 movieName = CRCD(0xf5012f52,"cutscene");

	// We don't want any messages showing through the cutscene
	Script::RunScript("destroy_goal_panel_messages");

	// start the movie with the default params...
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	skate_mod->GetMovieManager()->AddMovieToQueue( movieName, pParams, CRCD(0xf5012f52,"cutscene") );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | HasMovieStarted | return true if the video of a movie has started 
// (after loading data)
bool ScriptHasMovieStarted( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	return skate_mod->GetMovieManager()->HasMovieStarted();
}
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsMovieQueued | return true if there are any movies in the queue,
// regardless of whether the video has started
bool ScriptIsMovieQueued( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	return skate_mod->GetMovieManager()->IsMovieQueued();
}
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PlayMovingObjectAnim | Play an scene-based animation (earthquake, for instance)
bool ScriptPlayMovingObjectAnim( Script::CStruct *pParams, Script::CScript *pScript )
{
	if( pParams->ContainsFlag( "stop" ))
	{
		Dbg_MsgAssert( 0, ( "stop parameter doesn't work with PlayMovingObjectAnim" ) );
	}
	
	uint32 movieName = 0;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName );

	Dbg_Message( "playmovingobjectanim %s", Script::FindChecksumName(movieName) );

	// start the movie with the default params...
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	skate_mod->GetObjectAnimManager()->AddMovieToQueue( movieName, pParams, CRCD(0x12743edb,"objectanim") );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetMovingObjectAnimSkippable | set the current movie as skippable
// @parmopt int | name | | the name of the movie to set skippable (otherwise, it will use the currently-playing movie)
// @parmopt int | skippable | 0 | set to anything greater than 0 to set anim to skippable
bool ScriptSetMovingObjectAnimSkippable( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 movieName = 0;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName );

	int skippable = 0;
	pParams->GetInteger( "skippable", &skippable );

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	skate_mod->GetObjectAnimManager()->SetMovieSkippable( movieName, skippable );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetMovingObjectAnimShouldPause | changes the pause mode of a skater cam.
// If the pause mode is set to false, the cam will play even if the game is paused
bool ScriptSetMovingObjectAnimShouldPause( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 movieName = 0;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName );

	int should_pause = 0;
	pParams->GetInteger( "should_pause", &should_pause );

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	skate_mod->GetObjectAnimManager()->SetMoviePauseMode( movieName, should_pause );
	
	return true;
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

// @script | MovingObjectAnimFinished | returns true if the anim for the
// given skater is finished
// @parm int | skater | the skater
// @parmopt int | cam | 1 | the camera number 
bool ScriptMovingObjectAnimFinished( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 movieName = 0;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName );

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	return skate_mod->GetObjectAnimManager()->IsMovieComplete( movieName );
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

// @script | MovingObjectAnimHeld | returns true if anim is held
// @parm int | skater | the skater
bool ScriptMovingObjectAnimHeld( Script::CStruct *pParams, Script::CScript *pScript )
{	
	uint32 movieName = 0;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName );
		
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	return skate_mod->GetObjectAnimManager()->IsMovieHeld( movieName );
}

/******************************************************************/
/*																  */
/*                                                                */
/******************************************************************/

// @script | KillMovingObjectAnim | 
// @parmopt int | skater | 0 | the skater
// @flag current | kill the current cam anim
// @flag all | kill all cam anims
bool ScriptKillMovingObjectAnim( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CMovieManager* pObjectAnimManager = skate_mod->GetObjectAnimManager(); 

	if ( pParams->ContainsFlag( "current" ) )
	{
		return pObjectAnimManager->AbortCurrentMovie( false );
	}
	else if ( pParams->ContainsFlag( "all" ) )
	{
		pObjectAnimManager->ClearMovieQueue();
		return true;
	}
	else
	{
		uint32 movieName = 0;
		pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &movieName, true );
		pObjectAnimManager->RemoveMovieFromQueue( movieName );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ReloadMovingObjectAnim | undocumented
bool ScriptReloadMovingObjectAnim( Script::CStruct* pParams, Script::CScript* pScript )
{
	if ( !Config::GotExtraMemory() )
	{
		Dbg_Message( "ReloadMovingObjectAnim:  This function only works with the extra debug heap.  (Otherwise, it would fragment the bottom-up heap.)" );
		return false;
	}

	uint32 animName;
	const char* pFileName;

	if ( !pParams->GetChecksum( NONAME, &animName, false ) )
	{
		Dbg_Message( "ReloadMovingObjectAnim:  Missing cam anim name.\n" );
		return false;
	}

	if ( !pParams->GetText( NONAME, &pFileName, false )	)
	{
		Dbg_Message( "ReloadMovingObjectAnim:  Missing cam anim filename.\n" );
		return false;
	}

	Dbg_Message( "Reloading movie %08x with file %s...", animName, pFileName );

	// clear any existing movies in the queue, just in case we were in the middle of playing the anim to be replaced
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	skate_mod->GetObjectAnimManager()->ClearMovieQueue();

	// reload the appropriate asset
	Ass::CAssMan * ass_man = Ass::CAssMan::Instance();
	
	// on the debug heap
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
	
	bool success = ass_man->ReloadAsset( animName, pFileName, false );
	
	Mem::Manager::sHandle().PopContext();

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// @script | SkaterDebugOn | sets debug to on
bool ScriptSkaterDebugOn(Script::CStruct *pParams, Script::CScript *pScript)
{   
	Obj::DebugSkaterScripts=true;
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SkaterDebugOff | sets debug to off
bool ScriptSkaterDebugOff(Script::CStruct *pParams, Script::CScript *pScript)
{   
	Obj::DebugSkaterScripts=false;
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | VibrationIsOn | returns true if vibration for the specified
// player is on
// @uparm 0 | the player 
bool ScriptVibrationIsOn(Script::CStruct *pParams, Script::CScript *pScript)
{
	int i=0;
	pParams->GetInteger(NONAME,&i);
	Dbg_MsgAssert(i>=0 && i<Mdl::Skate::vMAX_SKATERS, ("Bad index of %d sent to VibrationIsOn",i));

	return Mdl::Skate::Instance()->mp_controller_preferences[i].VibrationOn;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | VibrationOn | turns on vibration for the specified player
// @uparm 0 | player
bool ScriptVibrationOn(Script::CStruct *pParams, Script::CScript *pScript)
{
	int i=0;
	pParams->GetInteger(NONAME,&i);
	Dbg_MsgAssert(i>=0 && i<Mdl::Skate::vMAX_SKATERS, ("Bad index of %d sent to VibrationOn",i));
	
	Mdl::Skate::Instance()->SetVibration(i,true);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | VibrationOff | turns off vibration for the specified player
// @uparm 0 | the player 
bool ScriptVibrationOff(Script::CStruct *pParams, Script::CScript *pScript)
{
	int i=0;
	pParams->GetInteger(NONAME,&i);
	Dbg_MsgAssert(i>=0 && i<Mdl::Skate::vMAX_SKATERS, ("Bad index of %d sent to VibrationOn",i));
	
	Mdl::Skate::Instance()->SetVibration(i,false);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AutoKickIsOn | returns true if autokick is on for the specified skater
// @uparmopt 0 | skater
bool ScriptAutoKickIsOn(Script::CStruct *pParams, Script::CScript *pScript)
{   
	int i=0;
	pParams->GetInteger(NONAME,&i);
	Dbg_MsgAssert(i>=0 && i<Mdl::Skate::vMAX_SKATERS, ("Bad index of %d sent to AutoKickIsOn",i));
	
	return Mdl::Skate::Instance()->mp_controller_preferences[i].AutoKickOn;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AutoKickOn | turns on autokick for the specified player
// @uparmopt 0 | the skater
bool ScriptAutoKickOn(Script::CStruct *pParams, Script::CScript *pScript)
{   
	int i=0;
	pParams->GetInteger(NONAME,&i);
	Dbg_MsgAssert(i>=0 && i<Mdl::Skate::vMAX_SKATERS, ("Bad index of %d sent to AutoKickOn",i));
	
	Mdl::Skate::Instance()->SetAutoKick(i,true);
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AutoKickOff | turns off autokick for the specified player
// @uparmopt 0 | skater
bool ScriptAutoKickOff(Script::CStruct *pParams, Script::CScript *pScript)
{   
	int i=0;
	pParams->GetInteger(NONAME,&i);
	Dbg_MsgAssert(i>=0 && i<Mdl::Skate::vMAX_SKATERS, ("Bad index of %d sent to AutoKickOff",i));
	
	Mdl::Skate::Instance()->SetAutoKick(i,false);
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SpinTapsAreOn | returns true if spin taps are on
// @uparmopt 0 | skater
bool ScriptSpinTapsAreOn(Script::CStruct *pParams, Script::CScript *pScript)
{   
	int i=0;
	pParams->GetInteger(NONAME,&i);
	Dbg_MsgAssert(i>=0 && i<Mdl::Skate::vMAX_SKATERS, ("Bad index of %d sent to SpinTapsAreOn",i));
	
	
	return Mdl::Skate::Instance()->mp_controller_preferences[i].SpinTapsOn;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SpinTapsOn | turns on spin taps for the specified skater
// @uparmopt 0 | skater
bool ScriptSpinTapsOn(Script::CStruct *pParams, Script::CScript *pScript)
{
	int i=0;
	pParams->GetInteger(NONAME,&i);
	Dbg_MsgAssert(i>=0 && i<Mdl::Skate::vMAX_SKATERS, ("Bad index of %d sent to SpinTapsOn",i));
	
	Mdl::Skate::Instance()->SetSpinTaps(i,true);
	return true;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SpinTapsOff | turns off spin taps for the specified skater
// @uparmopt 0 | skater
bool ScriptSpinTapsOff(Script::CStruct *pParams, Script::CScript *pScript)
{   
	int i=0;
	pParams->GetInteger(NONAME,&i);
	Dbg_MsgAssert(i>=0 && i<Mdl::Skate::vMAX_SKATERS, ("Bad index of %d sent to SpinTapsOff",i));
	
	Mdl::Skate::Instance()->SetSpinTaps(i,false);
	return true;
}	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetCurrentProDisplayInfo | 
bool ScriptGetCurrentProDisplayInfo(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile();

	pScript->GetParams()->AddComponent( Script::GenerateCRC( "string" ), ESYMBOLTYPE_STRING, pSkaterProfile->GetDisplayName());
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetPlayerAppearance | sets the player appearance 
// @parmopt int | player | 0 | index of player whose appearance you want to edit
// @parmopt name | appearance_structure | | the name of the profile
bool ScriptSetPlayerAppearance(Script::CStruct *pParams, Script::CScript *pScript)
{	
	int player;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &player, Script::ASSERT );
	
	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( player );
	Gfx::CModelAppearance* pAppearance = pSkaterProfile->GetAppearance();
	
	Script::CStruct* pAppearanceStruct;
	pParams->GetStructure( "appearance_structure", &pAppearanceStruct, Script::ASSERT );
	pAppearance->Load( pAppearanceStruct );
	
	return true;
}
					
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetPlayerFacePoints | Gets the player face points and puts it in the structure "current_face_points"
// @parm int | player | 0 | index of player
bool ScriptGetPlayerFacePoints(Script::CStruct *pParams, Script::CScript *pScript)
{
	int player;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &player, Script::ASSERT );
	
	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( player );
	Gfx::CModelAppearance* pAppearance = pSkaterProfile->GetAppearance();
	
	Gfx::CFaceTexture* pFaceTexture = pAppearance->GetFaceTexture();
	Dbg_MsgAssert( pFaceTexture, ( "Appearance doesn't have a face texture1" ) );
	Dbg_MsgAssert( pFaceTexture->IsValid(), ( "Face texture has not been initialized with valid texture data" ) );

	Script::CStruct *pFacePointsStruct = NULL;

	bool make_new_structure = false;
	if (!pScript->GetParams()->GetStructure( CRCD(0x12e87395,"current_face_points"), &pFacePointsStruct ))
	{
		pFacePointsStruct = new Script::CStruct;
		make_new_structure = true;
	}

	Nx::SFacePoints theFacePoints;
	theFacePoints = pFaceTexture->GetFacePoints( );
	if ( !Nx::SetFacePointsStruct( theFacePoints, pFacePointsStruct ) )
	{
		Script::PrintContents( pParams );
		Dbg_MsgAssert( 0, ("Couldn't set face points to structure") );
	}

	if (make_new_structure)
	{
		pScript->GetParams()->AddStructure( CRCD(0x12e87395,"current_face_points"), pFacePointsStruct );
		delete pFacePointsStruct;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetPlayerFacePoints |
// @parmopt struct | face_points | face points structure
bool ScriptSetPlayerFacePoints(Script::CStruct *pParams, Script::CScript *pScript)
{
	int player;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &player, Script::ASSERT );
	
	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( player );
	Gfx::CModelAppearance* pAppearance = pSkaterProfile->GetAppearance();
	
	Gfx::CFaceTexture* pFaceTexture = pAppearance->GetFaceTexture();
	Dbg_MsgAssert( pFaceTexture, ( "Appearance doesn't have a face texture1" ) );
	Dbg_MsgAssert( pFaceTexture->IsValid(), ( "Face texture has not been initialized with valid texture data" ) );

	Nx::SFacePoints theFacePoints;
	Script::CStruct *pFacePointsStruct;
	if (!pParams->GetStructure("face_points", &pFacePointsStruct))
	{
		pFacePointsStruct = pParams;
	}
	if ( !Nx::GetFacePointsStruct( theFacePoints, pFacePointsStruct ) )
	{
		Script::PrintContents( pParams );
		Dbg_MsgAssert( 0, ("Couldn't read face points from structure") );
	}
	pFaceTexture->SetFacePoints( theFacePoints );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetPlayerFaceTexture |
bool ScriptSetPlayerFaceTexture(Script::CStruct *pParams, Script::CScript *pScript)
{
	int player;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &player, Script::ASSERT );
	
	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( player );
	Gfx::CModelAppearance* pAppearance = pSkaterProfile->GetAppearance();
	
	const char* pFaceName;
	pParams->GetText( CRCD(0x7d99f28d,"texture"), &pFaceName, Script::ASSERT );

	// by default, LoadFace will prepend the filename with "images\\"
	// however, the neversoft skater images are located in "textures\\"
	// so the following flag will tell LoadFace not to prepend the
	// filename
	bool fullPathIncluded = pParams->ContainsFlag( CRCD(0x54c2e57c,"FullPathIncluded") );

	Gfx::CFaceTexture* pFaceTexture = pAppearance->GetFaceTexture();
	Dbg_MsgAssert( pFaceTexture, ( "Appearance doesn't have a face texture2" ) );
	pFaceTexture->LoadFace( pFaceName, fullPathIncluded );

	Dbg_MsgAssert( pFaceTexture->IsValid(), ( "Face texture should now be considered valid" ) );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetPlayerFaceOverlayTexture |
bool ScriptSetPlayerFaceOverlayTexture(Script::CStruct *pParams, Script::CScript *pScript)
{
	int player;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &player, Script::ASSERT );
	
	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( player );
	Gfx::CModelAppearance* pAppearance = pSkaterProfile->GetAppearance();
	
	const char* pOverlayTextureName;
	pParams->GetText( CRCD(0x7d99f28d,"texture"), &pOverlayTextureName, Script::ASSERT );

	Gfx::CFaceTexture* pFaceTexture = pAppearance->GetFaceTexture();
	Dbg_MsgAssert( pFaceTexture, ( "Appearance doesn't have a face texture3" ) );
	Dbg_MsgAssert( pFaceTexture->IsValid(), ( "Face texture is not valid" ) );

	pFaceTexture->SetOverlayTextureName( pOverlayTextureName );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ClearPlayerFaceTexture |
bool ScriptClearPlayerFaceTexture(Script::CStruct *pParams, Script::CScript *pScript)
{
#ifdef __PLAT_NGPS__
	int player;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &player, Script::ASSERT );
	
	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( player );
	Gfx::CModelAppearance* pAppearance = pSkaterProfile->GetAppearance();
	Dbg_Assert( pAppearance );

	Gfx::CFaceTexture* pFaceTexture = pAppearance->GetFaceTexture();
	Dbg_MsgAssert( pFaceTexture, ( "Appearance doesn't have a face texture4" ) );
	pFaceTexture->SetValid( false );
#endif
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PlayerFaceIsValid | returns whether the player has already loaded or downloaded a face
bool ScriptPlayerFaceIsValid(Script::CStruct *pParams, Script::CScript *pScript)
{
	int player;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &player, Script::ASSERT );
	
	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( player );
	Gfx::CModelAppearance* pAppearance = pSkaterProfile->GetAppearance();
	Dbg_Assert( pAppearance );

	Gfx::CFaceTexture* pFaceTexture = pAppearance->GetFaceTexture();
	//Dbg_MsgAssert( pFaceTexture, ( "Appearance doesn't have a face texture5" ) );
    if ( pFaceTexture )
    {
        return pFaceTexture->IsValid();
    }
    else
    {
        //Appearance doesn't have a face texture
        return false;
    }
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SelectCurrentSkater | sets the current profile to the specified skater
// @parmopt name | name | | the name of the profile
// @uparmopt name | name of the profile
bool ScriptSelectCurrentSkater(Script::CStruct *pParams, Script::CScript *pScript)
{	
	uint32 profileName;
	if ( !pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &profileName, false ) )
	{
		pParams->GetChecksum( NONAME, &profileName, true );
	}

	
	Obj::CPlayerProfileManager*	pPlayerProfileManager=Mdl::Skate::Instance()->GetPlayerProfileManager();
	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile();

	// remember old checksum to see if the data has changed
	if ( pSkaterProfile->GetSkaterNameChecksum() == profileName )
	{
		// no change
		return false;
	}
	
	pPlayerProfileManager->ApplyTemplateToCurrentProfile( profileName );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CareerStartLevel | Called at the start of the level, and whenever
// the level is restarted (by a retry, for example).  This will set the level
// number (unless you pass -1, in which case it uses the "current level"). This
// will store the values of the career goals, and the level flags, at the start
// of the level, so at the end of the level, we will be able to see if we have
// just got a particular goal, and play appropriate rewards. Currently you
// should not have to mess with this.  I've added appropriate CareerStartLevel 
// commands in the level loading script in levels.q.  I've also added (in the
// C++ code) what amounts to "CareerStartLevel level = -1" to the "RestartLevel"
// function in skate.cpp.  This gets called whenever you restart a level (via
// a retry). 
// @parm int | level | the level to start
bool ScriptCareerStartLevel(Script::CStruct *pParams, Script::CScript *pScript)
{
	int level; 		
	pParams->GetInteger("level", &level, true);  		// Note we assert if "level" is missing
	Mdl::Skate::Instance()->GetCareer()->StartLevel(level);
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CareerLevelIs | This is true if the current level number, as specified
// in the last call to CareerStartLevel, matches the value passed, for example: <nl>
// if CareerLevelIs 1 <nl>
//   printf "Foundry!" <nl>
// endif
// @uparm 1 | level to check
bool ScriptCareerLevelIs(Script::CStruct *pParams, Script::CScript *pScript)
{
	int level; 		
	pParams->GetInteger(NONAME, &level, true);	// Note we assert if the level value is missing
	

	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();

	return pCareer->GetLevel()==level;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetRecordText | 
// @parmopt int | level | | level num (default is current level)
bool ScriptGetRecordText(Script::CStruct *pParams, Script::CScript *pScript)
{
	int level; 	  
	  

	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();

	if (!pParams->GetInteger("level", &level, false))	
	{
		level = pCareer->GetLevel();
	}
			  
	Mdl::Skate::Instance()->GetRecordsText(level);	
	return true;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UpdateRecords | 
bool ScriptUpdateRecords(Script::CStruct *pParams, Script::CScript *pScript)
{

	Mdl::Skate::Instance()->UpdateRecords();	
	return true;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CareerReset | Resets all the career info back to as if you've
// just started a new career.  Note that this also sets the current level
// to 0 (as you'd not have unlocked any levels).  So if you want to use
// this in the middle of playing a  level, then you'd better add a "StartLevel
// level = ???" to set the correct level. See ResetLevelGoals for more
bool ScriptCareerReset(Script::CStruct *pParams, Script::CScript *pScript)
{

	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();

	pCareer->Reset();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetGoal | Sets the goal to the "got" state.  All this does
// is set a flag, anything else, like playing sounds, should be done in script
// @parm int | goal | the goal num
bool ScriptSetGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	int goal; 		
	pParams->GetInteger("goal", &goal, true);  		// Note we assert if "goal" is missing
	

	
	// if we are not in career mode, then exit  		   
	if (!Mdl::Skate::Instance()->GetGameMode()->IsTrue( CRCD(0x1ded1ea4,"is_career") ) && !Mdl::Skate::Instance()->GetGameMode()->IsFrontEnd() )
	{
		return true;
	}
	
	
	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();

	pCareer->SetGoal(goal);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnSetGoal | Clears a particular goal flag
// @parm int | goal | goal flag to clear
bool ScriptUnSetGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	int goal; 		
	pParams->GetInteger("goal", &goal, true);  		// Note we assert if "goal" is missing
	

	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();

	pCareer->UnSetGoal(goal);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetGoal | Get the state of a goal.  Will be "true" if you have
// got the goal, either in this session, or in some previous session. For
// example: <nl> 
// if GetGoal goal = GOAL_SKATE  ; if got SKATE, then do nothing here <nl>
// else <nl>
//   trigger skater letters <nl>
// endif <nl>
// The primary purpose of this function is the modify the functionality
// of the startup script, to stop things from being created, and to modify
// the intro camera. 
// @parm int | goal | the goal to check
bool ScriptGetGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	int goal; 		
	pParams->GetInteger("goal", &goal, true);  		// Note we assert if "goal" is missing
	

	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();

	return pCareer->GetGoal(goal);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | JustGotGoal | Is "true" if you got this goal in the current
// session (still valid if the session has just ended). For example: <nl>
// if JustGotoGoal goal = GOAL_SKATE <nl>
//  LaunchPanelMessage "Yay! You got the skate letters" <nl>
// endif 
// The primary purpose if to play one-time congratulatory sequences at
// the end of the level, in the end-level script.  This flag will only
// be true during the session when you actually get the goal
// @parm int | goal | goal th check
bool ScriptJustGotGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	int goal; 		
	pParams->GetInteger("goal", &goal, true);  		// Note we assert if "goal" is missing
	

	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();

	return pCareer->JustGotGoal(goal);   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetFlag | similar to setgoal.  there are 32 flags per level,
// numbered 0 - 31.
// @parm int | flag | the flag to set
bool ScriptSetFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
	int flag; 		
	pParams->GetInteger("flag", &flag, true);  		// Note we assert if "flag" is missing
	

	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();

	pCareer->SetFlag(flag);
	
	if (flag==Script::GetInt("GOAL_DECK"))
	{
		//Front::GetNewDeck();
	}	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnSetFlag | similar to unsetgoal.  there are 32 flags per level,
// numbered 0 - 31.
// @parm int | flag | the flag to unset
// @parmopt int | level | | level number
bool ScriptUnSetFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
	int flag; 		
	pParams->GetInteger("flag", &flag, true);  		// Note we assert if "flag" is missing
	

	int level = -1;
	pParams->GetInteger( "level", &level, Script::NO_ASSERT );

	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();

	pCareer->UnSetFlag(flag, level);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetFlag | similar to GetGoal.  gets the state of the flag
// @parm int | flag | the flag to get
// @parmopt int | level | | the level number (for level specific flags).  if
// no level number is given, it will use the current level number
bool ScriptGetFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
	int flag;
	pParams->GetInteger("flag", &flag, true);  		// Note we assert if "flag" is missing
	
	int level = 0;
	pParams->GetInteger( "level", &level, Script::NO_ASSERT );

	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();
	if ( level )
		return pCareer->GetFlag( flag, level );

	return pCareer->GetFlag( flag );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | JustGotFlag | similar to the JustGotGoal command.  returns true
// if we just got this flag
// @parm int | flag | the flag to check
bool ScriptJustGotFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
	int flag; 		
	pParams->GetInteger("flag", &flag, true);  		// Note we assert if "flag" is missing
	

	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();
	return pCareer->JustGotFlag(flag);	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetGlobalFlag | sets a global flag
// @parm int | flag | the global flag to set
bool ScriptSetGlobalFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
#ifdef __NOPT_ASSERT__
	if (!pParams->ContainsComponentNamed("flag"))
	{
		printf("Call to SetGlobalFlag has no flag (or that flag is undefined.\n%s", pScript->GetScriptInfo());
		Dbg_Assert(false);
	}
#endif

	int flag; 		
	pParams->GetInteger(CRCD(0x2e0b1465, "flag"), &flag, true);  		// Note we assert if "flag" is missing
	
	s_get_career(flag,pParams)->SetGlobalFlag(flag);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnSetGlobalFlag | unsets a global flag
// @parm int | flag | the flag to unset
bool ScriptUnSetGlobalFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
#ifdef __NOPT_ASSERT__
	if (!pParams->ContainsComponentNamed("flag"))
	{
		printf("Call to UnSetGlobalFlag has no flag (or that flag is undefined.\n%s", pScript->GetScriptInfo());
		Dbg_Assert(false);
	}
#endif

	int flag; 		
	pParams->GetInteger(CRCD(0x2e0b1465, "flag"), &flag, true);  		// Note we assert if "flag" is missing
	

	
	s_get_career(flag,pParams)->UnSetGlobalFlag(flag);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetGlobalFlag | gets a global flag
// @parm int | flag | the flag to get
bool ScriptGetGlobalFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
#ifdef __NOPT_ASSERT__
	if (!pParams->ContainsComponentNamed("flag"))
	{
		printf("Call to GetGlobalFlag has no flag (or that flag is undefined.\n%s", pScript->GetScriptInfo());
		Dbg_Assert(false);
	}
#endif
	
	int flag; 		
	pParams->GetInteger(CRCD(0x2e0b1465, "flag"), &flag, true);  		// Note we assert if "flag" is missing
	
	return Mdl::Skate::Instance()->GetCareer()->GetGlobalFlag(flag);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ProfileEquals | tells us whether the skater matches the given
// criteria.  example: <nl>
// if ProfileEquals is_pro
// @flag is_pro | check if this is a pro skater
// @flag is_custom | check if this is a custom skater
// @parmopt name | is_named | | check if the skater has the specified name
// @parmopt name | stance | | check for this stance
bool ScriptProfileEquals(Script::CStruct *pParams, Script::CScript *pScript)
{
	// look for the specified skater (0 by default)
	Obj::CSkater* pSkater;
	
	// if this function is being called through
	// a skater script, then use the specified skater...
	if( pScript && pScript->mpObject && pScript->mpObject->GetType() == SKATE_TYPE_SKATER )	
	{
		pSkater = (Obj::CSkater *) pScript->mpObject.Convert();
		return pSkater->SkaterEquals( pParams );
	}

	if ( !Mdl::Skate::Instance()->IsMultiplayerGame() || Mdl::Skate::Instance()->GetGameMode()->IsFrontEnd() )
	{
		Dbg_MsgAssert( 0, ( "ProfileEquals should only be called from a skater script in a multiplayer game." ) );
		return false;
	}

	// otherwise, get the first skater and check him
	pSkater = Mdl::Skate::Instance()->GetSkater( 0 );
	if( pSkater == NULL )
	{
		return false;
	}

	return pSkater->SkaterEquals( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsCareerMode | returns true if the current game mode is
// career mode
bool ScriptIsCareerMode(Script::CStruct *pParams, Script::CScript *pScript)
{


	return Mdl::Skate::Instance()->GetGameMode()->IsTrue( CRCD(0x1ded1ea4,"is_career") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ClearScoreGoals | clears all the score goals
bool ScriptClearScoreGoals(Script::CStruct *pParams, Script::CScript *pScript)
{

	Mdl::Skate::Instance()->ClearScoreGoals();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetScoreGoal | used to add a new score goal.  examples: <nl>
// SetScoreGoal  score = 1200000 goalscript = Got_SickScore goal=GOAL_SICKSCORE <nl>
// SetScoreGoal    score = 30000 goalscript = Got_HighScore goal=GOAL_HIGHSCORE
// @parm int | score | the score value for this goal
// @parm name | goalscript | the script to run 
// @parm int | goal | goal number
bool ScriptSetScoreGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	int score;
	pParams->GetInteger("score", &score, true);
	uint32	script;
	pParams->GetChecksum("goalscript", &script, true);
	int	goal;
	pParams->GetInteger("goal", &goal, true);

	
	// Note, for now we are just using the goal number as an index into the table
	// so long as we have enough entries, we will be fine
	Mdl::Skate::Instance()->SetScoreGoalScore(goal,score);
	Mdl::Skate::Instance()->SetScoreGoalScript(goal,script);
	Mdl::Skate::Instance()->SetScoreGoalGoal(goal,goal);
	
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EndRun | This command will end the current run (same as
// used by the "end run" selection in the pause menu)
bool ScriptEndRun(Script::CStruct *pParams, Script::CScript *pScript)
{

	Mdl::Skate::Instance()->EndRun();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ShouldEndRun | if "end run" was selected from the menu
// of the end conditions for this game mode have been met
bool ScriptShouldEndRun(Script::CStruct *pParams, Script::CScript *pScript)
{

	// if "end run" was selected from the menu or the
	// end conditions for this game mode have been met
	return ( Mdl::Skate::Instance()->EndRunSelected() || Mdl::Skate::Instance()->GetGameMode()->EndConditionsMet() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | InitializeSkaters | 
bool ScriptInitializeSkaters(Script::CStruct *pParams, Script::CScript *pScript)
{	

	for ( unsigned int i = 0; i < Mdl::Skate::Instance()->GetNumSkaters(); i++)
	{
		Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetSkater(i);
		Dbg_Assert( pSkater );
		if( pSkater->IsLocalClient())
		{
			pSkater->Reset();
		}
		Mdl::Skate::Instance()->HideSkater( pSkater, false );
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EndRunSelected | true if "end run" was selected from the menu
bool ScriptEndRunSelected(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	// if "end run" was selected from the menu
	return Mdl::Skate::Instance()->EndRunSelected();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AllSkatersAreIdle | true if all skaters are idle
bool ScriptAllSkatersAreIdle(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	return Mdl::Skate::Instance()->SkatersAreIdle();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | FirstTrickStarted | should only be called in horse mode.  
bool ScriptFirstTrickStarted(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	return Mdl::Skate::Instance()->FirstTrickStarted();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | FirstTrickCompleted | should only be called in horse mode
bool ScriptFirstTrickCompleted(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	return Mdl::Skate::Instance()->FirstTrickCompleted();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CalculateFinalScores | final scores for all skaters on server
bool ScriptCalculateFinalScores(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	

	if( gamenet_man->OnServer())
	{
		Mdl::Skate::Instance()->SendScoreUpdates( true );
		return true;
	}

	return gamenet_man->HaveReceivedFinalScores();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ReinsertSkaters | adds all skaters to the current world
bool ScriptReinsertSkaters(Script::CStruct *pParams, Script::CScript *pScript)
{
    // For now, this is commented out, until I can get the
    // skin model functionality over into the new CModel class.
	
	// synchronous, no need to pass through net msgs
	// add the skaters back to the world
	
	for ( uint32 i = 0; i < Mdl::Skate::Instance()->GetNumSkaters(); i++ )
	{   
		Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetSkater( i );
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterHeap(pSkater->GetHeapIndex()));
		pSkater->AddToCurrentWorld();
		Mem::Manager::sHandle().PopContext();
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnhookSkaters | Unhook the skaters from the world, so it can be unloaded
bool ScriptUnhookSkaters(Script::CStruct *pParams, Script::CScript *pScript)
{
	for ( uint32 i = 0; i < Mdl::Skate::Instance()->GetNumSkaters(); i++ )
	{   
		Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetSkater( i );
		pSkater->RemoveFromCurrentWorld();
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ApplySplitScreenOptions | applies various split screen prefs - 
// game type, time limit, etc.
bool ScriptApplySplitScreenOptions(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Prefs::Preferences* pPreferences;

	pPreferences = Mdl::Skate::Instance()->GetSplitScreenPreferences();
	Dbg_Assert(pPreferences);

	Script::CStruct* pStructure;
	uint32 checksum;

	// change the game type
	pStructure = pPreferences->GetPreference( Script::GenerateCRC("game_type") );
	Dbg_Assert(pStructure);
	pStructure->GetChecksum( "checksum", &checksum, true );
	Mdl::Skate::Instance()->GetGameMode()->LoadGameType( checksum );

	int time_limit = pPreferences->GetPreferenceValue( Script::GenerateCRC("time_limit"), Script::GenerateCRC("time") );

	if ( Mdl::Skate::Instance()->GetGameMode()->IsTrue( "is_horse" ) )
	{
		time_limit = pPreferences->GetPreferenceValue( Script::GenerateCRC("horse_time_limit"), Script::GenerateCRC("time") );
		Mdl::Skate::Instance()->GetHorse()->SetWord( pPreferences->GetPreferenceString(  Script::GenerateCRC("horse_word"), Script::GenerateCRC("ui_string") ) );

		Script::CStruct* pTempStructure = new Script::CStruct;
		pTempStructure->AddComponent( Script::GenerateCRC("default_time_limit"), ESYMBOLTYPE_INTEGER, time_limit );
		Mdl::Skate::Instance()->GetGameMode()->OverrideOptions( pTempStructure );		
		Mdl::Skate::Instance()->SetTimeLimit( time_limit );
		delete pTempStructure;

		//Mdl::Skate::Instance()->SetShadowMode( Gfx::vDETAILED_SHADOW );
	}
	else
	{
		//Mdl::Skate::Instance()->SetShadowMode( Gfx::vSIMPLE_SHADOW );
	}
	
	/*if( Mdl::Skate::Instance()->GetGameMode()->IsTrue( "is_king" ))
	{
		Script::CStruct* pTempStructure = new Script::CStruct;
		Script::CArray* pArray = new Script::CArray;
		Script::CopyArray(pArray,Script::GetArray("targetScoreArray"));
		Script::CStruct* pSubStruct = pArray->GetStructure(0);
		Dbg_Assert(pSubStruct);
		pSubStruct->AddComponent(Script::GenerateCRC("score"),ESYMBOLTYPE_INTEGER, (int) Tmr::Seconds( time_limit ));
		pTempStructure->AddComponent( Script::GenerateCRC("default_time_limit"), ESYMBOLTYPE_INTEGER, (int) 0 );
		pTempStructure->AddComponent( Script::GenerateCRC("victory_conditions"), pArray );
		Mdl::Skate::Instance()->GetGameMode()->OverrideOptions( pTempStructure );
		Mdl::Skate::Instance()->SetTimeLimit( 0 );
		delete pTempStructure;
	}
	else
	{
		// if the time limit is fixed, then don't override it
		if ( !Mdl::Skate::Instance()->GetGameMode()->IsTimeLimitConfigurable() )
		{
			time_limit = Mdl::Skate::Instance()->GetGameMode()->GetTimeLimit();
		}

		Script::CStruct* pTempStructure = new Script::CStruct;
		pTempStructure->AddComponent( Script::GenerateCRC("default_time_limit"), ESYMBOLTYPE_INTEGER, time_limit );
		Mdl::Skate::Instance()->GetGameMode()->OverrideOptions( pTempStructure );		
		Mdl::Skate::Instance()->SetTimeLimit( time_limit );
		delete pTempStructure;
	}
	
//	printf("Applying time limit\n");
//	pTempStructure->PrintContents();

	// update all the skaters' stats
	for ( unsigned int i = 0; i < Mdl::Skate::Instance()->GetNumSkaters(); i++)
	{
		Obj::CSkater *pSkater = Mdl::Skate::Instance()->GetSkater(i);
		Dbg_Assert( pSkater );

		Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( pSkater->GetID() );
		pSkater->UpdateStats( pSkaterProfile );
	}*/

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StartCompetition | 
// @parm float | gold | 
// @parm float | silver | 
// @parm float | bronze | 
// @parm float | gold_score |
// @parm float | silver_score | 
// @parm float | bronze_score | 
// @parm float | bail | 
bool ScriptStartCompetition(Script::CStruct *pParams, Script::CScript *pScript)
{   
	
//	float bronze, float silver, float gold, float bronze_score, float silver_score, float gold_score, float bail)
	float	gold,silver,bronze;
	float	gold_score,silver_score,bronze_score;
	float 	bail;
	
	if (!pParams->GetFloat("gold",&gold))
	{
		Dbg_MsgAssert(0,("\n%s\nStartCompetition missing 'gold' parameter",pScript->GetScriptInfo()));		
	}
	
	if (!pParams->GetFloat("silver",&silver))
	{
		Dbg_MsgAssert(0,("\n%s\nStartCompetition missing 'silver' parameter",pScript->GetScriptInfo()));		
	}
		
	if (!pParams->GetFloat("bronze",&bronze))
	{
		Dbg_MsgAssert(0,("\n%s\nStartCompetition missing 'bronze' parameter",pScript->GetScriptInfo()));		
	}
	if (!pParams->GetFloat("gold_score",&gold_score))
	{
		Dbg_MsgAssert(0,("\n%s\nStartCompetition missing 'gold_score' parameter",pScript->GetScriptInfo()));		
	}
	
	if (!pParams->GetFloat("silver_score",&silver_score))
	{
		Dbg_MsgAssert(0,("\n%s\nStartCompetition missing 'silver_score' parameter",pScript->GetScriptInfo()));		
	}
		
	if (!pParams->GetFloat("bronze_score",&bronze_score))
	{
		Dbg_MsgAssert(0,("\n%s\nStartCompetition missing 'bronze_score' parameter",pScript->GetScriptInfo()));		
	}
	if (!pParams->GetFloat("bail",&bail))
	{
		Dbg_MsgAssert(0,("\n%s\nStartCompetition missing 'bail' parameter",pScript->GetScriptInfo()));		
	}
	Script::CStruct* pExtraParams = NULL;
	pParams->GetStructure( CRCD(0xa61dc9a4,"extra_params"), &pExtraParams, Script::NO_ASSERT );

	// Need to use SkaterInfoHeap, as new strings can get created, which can fragment memory 									   
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());	
	if ( pExtraParams )
		Mdl::Skate::Instance()->GetCompetition()->EditParams( pExtraParams );
	Mdl::Skate::Instance()->GetCompetition()->StartCompetition(bronze, silver, gold, bronze_score, silver_score, gold_score, bail);
	Mem::Manager::sHandle().PopContext();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StartCompetitionRun | 
bool ScriptStartCompetitionRun(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Mdl::Skate::Instance()->GetCompetition()->StartRun();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EndCompetitionRun | 
bool ScriptEndCompetitionRun(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Obj::CSkater *pSkater = Mdl::Skate::Instance()->GetLocalSkater();						   
	Mdl::Score *pScore=pSkater->GetScoreObject();
	int score = pScore->GetTotalScore();


	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());
	Mdl::Skate::Instance()->GetCompetition()->EndRun(score);
	Mdl::Skate::Instance()->GetCompetition()->SetupJudgeText();
	Mem::Manager::sHandle().PopContext();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsTopJudge | true if the specified judge is the top judge
// @uparm 1 | judge num
bool ScriptIsTopJudge(Script::CStruct *pParams, Script::CScript *pScript)
{

	int judge;
	pParams->GetInteger(NONAME,&judge);

	
	return Mdl::Skate::Instance()->GetCompetition()->IsTopJudge(judge);
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PlaceIs | true if the comp place is equal to the specified place
// @uparm 1 | place
bool ScriptPlaceIs(Script::CStruct *pParams, Script::CScript *pScript)
{
	int place;
	pParams->GetInteger(NONAME,&place);

	
	return Mdl::Skate::Instance()->GetCompetition()->PlaceIs(place);
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RoundIs | true if the round equals the specified round
// @uparm 1 | round
bool ScriptRoundIs(Script::CStruct *pParams, Script::CScript *pScript)
{
	int round;
	pParams->GetInteger(NONAME,&round);

	
	return Mdl::Skate::Instance()->GetCompetition()->RoundIs(round);
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EndCompetition | 
bool ScriptEndCompetition(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());
	Mdl::Skate::Instance()->GetCompetition()->EndCompetition();
	Mem::Manager::sHandle().PopContext();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CompetitionEnded | true if the comp has ended
bool ScriptCompetitionEnded(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	return Mdl::Skate::Instance()->GetCompetition()->CompetitionEnded();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StartHorse |
bool ScriptStartHorse(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Mdl::Skate::Instance()->GetHorse()->Init();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EndHorse | 
bool ScriptEndHorse(Script::CStruct *pParams, Script::CScript *pScript)
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | HorseEnded | true if horse has ended
bool ScriptHorseEnded(Script::CStruct *pParams, Script::CScript *pScript)
{   
	
	return Mdl::Skate::Instance()->GetHorse()->Ended();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | HorseStatusEquals | pass in one or more of the flags to check status
// @flag GotLetter |
// @flag TieScore |
// @flag BeatScore |
// @flag Ended |
// @flag Idle |
// @flag NoScoreSet |
// @flag Terminator |
bool ScriptHorseStatusEquals(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	return Mdl::Skate::Instance()->GetHorse()->StatusEquals( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StartHorseRun | 
bool ScriptStartHorseRun(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Mdl::Skate::Instance()->GetHorse()->StartRun();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EndHorseRun | 
bool ScriptEndHorseRun(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Mdl::Skate::Instance()->GetHorse()->EndRun();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SwitchHorsePlayers | switches to the next valid player
bool ScriptSwitchHorsePlayers(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Mdl::Skate::Instance()->GetHorse()->SwitchPlayers();
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ApplyToHorsePanelString | 
// @parm name | whichString | the string to apply to
bool ScriptApplyToHorsePanelString(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 whichString, id;
	pParams->GetChecksum( "whichString", &whichString, true );
	pParams->GetChecksum( "id", &id, true );
	
	Str::String horseString = Mdl::Skate::Instance()->GetHorse()->GetString( whichString );

	Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetSkaterById( Mdl::Skate::Instance()->GetHorse()->GetCurrentSkaterId() );
	Dbg_Assert( pSkater );
	
	Script::CStruct* pTempStructure = new Script::CStruct;
	*pTempStructure+=*pParams;
	pTempStructure->AddString( "text", horseString.getString() );
	pTempStructure->AddChecksum( "id", id );
	Script::RunScript( "create_horse_panel_message", pTempStructure, pSkater );
	delete pTempStructure;
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// @parm name | whichString | the string to apply to
// @parmopt int | skater | 0 | the skater we're applying to (default is 0)
bool ScriptGetHorseString(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 whichString;
	Script::CStruct* p_return_parms;
	pParams->GetChecksum( "whichString", &whichString, true );

	Str::String horseString = Mdl::Skate::Instance()->GetHorse()->GetString( whichString );

	p_return_parms = pScript->GetParams();
	p_return_parms->AddString( "text", horseString.getString());

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | IsCurrentHorseSkater | true if the current horse skater
// is the same as the specified skater id
// @uparm 1 | skater id
bool ScriptIsCurrentHorseSkater(Script::CStruct *pParams, Script::CScript *pScript)
{
	int skaterId;
	if( pParams->GetInteger( NONAME, &skaterId, false ) == false )
	{
		pParams->GetChecksum( NONAME, (uint32*) &skaterId, true );
	}
	
	
	return ( Mdl::Skate::Instance()->GetHorse()->GetCurrentSkaterId() == skaterId );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ApplyToSkaterProfile | 
// @parmopt int | skater | 0 | the skater we're applying to (default is 0)
// @parmopt name | field_id | | the field to change
// @parmopt name | slider_id | | if the value is specified in a slider
// @parmopt name | keyboard_id | | value specified in a string
// @parmopt name | value | | specific value (in the absence of slider or keyboard)
bool ScriptApplyToSkaterProfile(Script::CStruct *pParams, Script::CScript *pScript)
{   
	/*
	Obj::CSkaterProfile* pProfile = NULL;

	// look for the specified skater (0 by default)
	int skater;


	if ( pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skater ) )
	{
		// get the specified skater profile
		pProfile = Mdl::Skate::Instance()->GetProfile( skater );
	}
	else
	{
		// look up the current skater profile
		pProfile = Mdl::Skate::Instance()->GetCurrentProfile( pParams );
	}

	Dbg_MsgAssert( pProfile,( "No profile found" ));

	uint32 field_id;
	uint32 checksumValue;
	uint32 slider_id;
	uint32 keyboard_id;
	pParams->GetChecksum( "field_id", &field_id, true );

	// the value will be in one of two places...
	if ( pParams->GetChecksum( "slider_id", &slider_id ) )
	{
		// de-reference the specified slider_id
		Front::MenuFactory* menu_factory = Front::MenuFactory::Instance();
		Front::SliderMenuElement* pSliderElement = static_cast<Front::SliderMenuElement*>( menu_factory->GetMenuElement( slider_id, true ) );
		uint32 value = pSliderElement->GetValue();
		pProfile->SetSkaterProperty( field_id, value );
	}
	else if ( pParams->GetChecksum( "keyboard_id", &keyboard_id ) )
	{
		// de-reference the specified slider_id
		Front::MenuFactory* menu_factory = Front::MenuFactory::Instance();
		Front::KeyboardControl* pKeyboardControl = static_cast<Front::KeyboardControl*>( menu_factory->GetMenuElement( keyboard_id, true ) );
		Str::String myString = pKeyboardControl->GetString();

		if ( strlen( myString.getString() ) )
		{
			printf("Setting property %s to %s\n", Script::FindChecksumName(field_id), myString.getString() );
			pProfile->SetSkaterProperty( field_id, myString.getString() );
		}
		else
		{
			printf("No string supplied.  Ignoring change to %s\n", Script::FindChecksumName(field_id) );
		}
	}
	else
	{
		// look for checksum value
		pParams->GetChecksum( "value", &checksumValue, true );
		printf("Setting %s\n", Script::FindChecksumName( checksumValue ));
		pProfile->SetSkaterProperty( field_id, checksumValue );
	}
	*/
		
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RefreshSkaterColors | this just synchs up the on-screen 
// skater with the color in the skater profile, in case it's changed
bool ScriptRefreshSkaterColors(Script::CStruct *pParams, Script::CScript *pScript)
{
	int profile = 0;
	int skater = 0;
	
	pParams->GetInteger( CRCD(0x7ea855f0,"profile"), &profile, Script::ASSERT );
	pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skater, Script::ASSERT );
//	Dbg_Printf( "Skater %d  Profile %d\n", skater, profile );
	Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetSkater(skater);
	Dbg_Assert( pSkater );

	Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile( profile );
	Dbg_Assert( pProfile );
    Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();
	
	Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pSkater );
	pModelComponent->RefreshModel( pAppearance, Gfx::vCHECKSUM_COLOR_MODEL_FROM_APPEARANCE );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RefreshSkaterScale | this just synchs up the on-screen 
// skater with the scale in the skater profile, in case it's changed
bool ScriptRefreshSkaterScale(Script::CStruct *pParams, Script::CScript *pScript)
{
	int profile = 0;
	int skater = 0;
	
	pParams->GetInteger( CRCD(0x7ea855f0,"profile"), &profile, Script::ASSERT );
	pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skater, Script::ASSERT );
//	Dbg_Printf( "Skater %d  Profile %d\n", skater, profile );
	Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetSkater(skater);
	Dbg_Assert( pSkater );

	Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile( profile );
	Dbg_Assert( pProfile );
    Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();
	
	Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pSkater );
	pModelComponent->RefreshModel( pAppearance, Gfx::vCHECKSUM_SCALE_MODEL_FROM_APPEARANCE );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RefreshSkaterVisibility | this just synchs up the on-screen 
// skater with the hidegeoms in the skater profile, in case it's changed
// (such as in the invisible man cheat
bool ScriptRefreshSkaterVisibility(Script::CStruct *pParams, Script::CScript *pScript)
{
	int profile = 0;
	int skater = 0;
	
	pParams->GetInteger( CRCD(0x7ea855f0,"profile"), &profile, Script::ASSERT );
	pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skater, Script::ASSERT );
//	Dbg_Printf( "Skater %d  Profile %d\n", skater, profile );
	Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetSkater(skater);
	Dbg_Assert( pSkater );

	Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile( profile );
	Dbg_Assert( pProfile );
    Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();
	
	Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pSkater );
	pModelComponent->RefreshModel( pAppearance, Gfx::vCHECKSUM_HIDE_MODEL_FROM_APPEARANCE );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RefreshSkaterUV | this just synchs up the on-screen 
// skater with the UVs in the skater profile, in case it's changed
bool ScriptRefreshSkaterUV(Script::CStruct *pParams, Script::CScript *pScript)
{
	int profile = 0;
	int skater = 0;
	
	pParams->GetInteger( CRCD(0x7ea855f0,"profile"), &profile, Script::ASSERT );
	pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skater, Script::ASSERT );
//	Dbg_Printf( "Skater %d  Profile %d\n", skater, profile );
	Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetSkater(skater);
	Dbg_Assert( pSkater );

	Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile( profile );
	Dbg_Assert( pProfile );
    Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();
	
	Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pSkater );
	pModelComponent->RefreshModel( pAppearance, CRCD(0x91d6261,"set_uv_from_appearance") );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AwardStatPoint | awards a stat point to the current skater
// @uparmopt 1 | increment value (can be negative)
bool ScriptAwardStatPoint(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile();

	int inc_val = 1;
	pParams->GetInteger( NONAME, &inc_val, Script::NO_ASSERT );

	pSkaterProfile->AwardStatPoint( inc_val );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AwardSpecialTrickSlot | awards a new special trick slot to the 
// current skater profile
// @flag all | award a trickslot to all profiles
bool ScriptAwardSpecialTrickSlot(Script::CStruct *pParams, Script::CScript *pScript)
{
	Obj::CPlayerProfileManager* pPlayerProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
	if ( pParams->ContainsFlag( "all" ) )
	{
		int num_profiles = pPlayerProfileManager->GetNumProfileTemplates();
		for ( int i = 0; i < num_profiles; i++ )
		{
			Obj::CSkaterProfile* pTempProfile = pPlayerProfileManager->GetProfileTemplateByIndex( i );
			pTempProfile->AwardSpecialTrickSlot();
		}
	}
	else
	{
		Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetCurrentProfile();
		pSkaterProfile->AwardSpecialTrickSlot();
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UpdateSkaterStats | 
bool ScriptUpdateSkaterStats(Script::CStruct *pParams, Script::CScript *pScript)
{
	int player;
	pParams->GetInteger(CRCD(0x67e6859a,"player"), &player, Script::ASSERT);	
		
	Obj::CSkater *pSkater = Mdl::Skate::Instance()->GetSkater(player);
	Dbg_Assert(pSkater);

	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile(player);
	pSkater->UpdateStats(pSkaterProfile);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UpdateInitials | 
bool ScriptUpdateInitials( Script::CStruct *pParams, Script::CScript *pScript )
{   
	
	int level = Mdl::Skate::Instance()->GetCareer()->GetLevel();
	Records::CGameRecords *pGameRecords=Mdl::Skate::Instance()->GetGameRecords();	
	pGameRecords->GetLevelRecords(level)->UpdateInitials(pGameRecords->GetDefaultInitials());
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | NewRecord | return true if the last call to UpdateRecords made
// there be a new record
bool ScriptNewRecord(  Script::CStruct *pParams, Script::CScript *pScript )
{      
	return Mdl::Skate::Instance()->m_new_record;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | TrickOffAllObjects | 
// @uparmopt 0 | skater id (0 is default)
bool ScriptTrickOffAllObjects( Script::CStruct *pParams, Script::CScript *pScript )
{
	// a cheat for getting all the graffiti objects

	if (!Config::CD())
	{	
		int skater_id = 0;
		pParams->GetInteger( NONAME, &skater_id );
		Mdl::Skate::Instance()->GetTrickObjectManager()->TrickOffAllObjects( skater_id );
	}	
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetScoreDegredation | 
// @uparm 1 | should degrade amount
bool ScriptGameModeSetScoreDegradation( Script::CStruct* pParams, Script::CScript* pScript )
{
    
	
	int should_degrade = 1;

	pParams->GetInteger( NONAME, &should_degrade );

	Mdl::Skate::Instance()->GetGameMode()->SetScoreDegradation( should_degrade );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetScoreAccumulation | 
// @uparm 1 | should accumulate amount
bool ScriptGameModeSetScoreAccumulation( Script::CStruct* pParams, Script::CScript* pScript )
{
    
	
	int should_accumulate = 1;

	pParams->GetInteger( NONAME, &should_accumulate );

	Mdl::Skate::Instance()->GetGameMode()->SetScoreAccumulation( should_accumulate );

	if ( should_accumulate == 0 && pParams->ContainsFlag( "freeze_score" ) )
	{
		Mdl::Skate::Instance()->GetGameMode()->SetScoreFrozen( true );
	}
	else 
	{
		Mdl::Skate::Instance()->GetGameMode()->SetScoreFrozen( false );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetScore |
bool ScriptResetScore(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	
	Dbg_Printf( "******** RESET SCORE\n" );

    for (uint32 i=0;i<skate_mod->GetNumSkaters();i++)
	{
		Obj::CSkater* p_Skater = skate_mod->GetSkater( i );
		if ( p_Skater->IsLocalClient() )
		{
			Mdl::Score * p_Score = ( p_Skater->GetScoreObject() );
			Dbg_Assert( p_Score );

			p_Score->Reset();
			if( ( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netking" )) ||
				( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "king" )))
			{
				Front::SetScoreTHPS4( "0:00", p_Skater->GetHeapIndex());
			}
			else
			{
				Front::SetScoreTHPS4( "0", p_Skater->GetHeapIndex());
			}
			
		}
	}
	
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UpdateScore |
bool ScriptUpdateScore(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	
    for (uint32 i=0;i<skate_mod->GetNumSkaters();i++)
	{
		Obj::CSkater* p_Skater = skate_mod->GetSkater( i );
		if ( p_Skater->IsLocalClient() )
		{
			Mdl::Score * p_Score = ( p_Skater->GetScoreObject() );
			Dbg_Assert( p_Score );

			p_Score->SetTotalScore( p_Score->GetTotalScore());
		}
	}
	
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetScoreDegradation | doesn't seem to do anything...
bool ScriptResetScoreDegradation( Script::CStruct* pParams, Script::CScript* pScript )
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SkaterIsBraking | returns true if the local skater is braking
bool ScriptSkaterIsBraking( Script::CStruct* pParams, Script::CScript* pScript )
{
    
    Obj::CCompositeObject *pSkater = Mdl::Skate::Instance()->GetLocalSkater();						   
    Dbg_Assert( pSkater );
    return pSkater->CallMemberFunction(CRCD(0x1f8bbd05, "Braking"), pParams, pScript);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LocalSkaterExists | returns true if the local skater exists
bool ScriptLocalSkaterExists( Script::CStruct* pParams, Script::CScript* pScript )
{
	
    Obj::CSkater *pSkater = Mdl::Skate::Instance()->GetLocalSkater();						   
    
    return ( pSkater != NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | InitSkaterModel | This will destroy the specified
// skater's model and replace it with the specified model or
// profile
bool ScriptInitSkaterModel(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	int player_num = 0;
	pParams->GetInteger( NONAME, &player_num );
	Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetSkater(player_num);
	Dbg_Assert( pSkater );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterGeomHeap(pSkater->GetHeapIndex()));
   	
	uint32 appearance_structure_name;
	pParams->GetChecksum( "default_appearance", &appearance_structure_name, true );
//	Script::PrintContents( pParams );

	// create the model
	Gfx::CModelAppearance theAppearance;
	theAppearance.Load( appearance_structure_name );

	Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pSkater );
	pModelComponent->InitModelFromProfile( &theAppearance, false, pSkater->GetHeapIndex() );
	
	Mem::Manager::sHandle().PopContext();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | RefreshSkaterModel | This will destroy the specified
// skater's model and replace it with the specified model or
// profile
// If there is a composite object called "bailboard" that will be destroyed
bool ScriptRefreshSkaterModel(Script::CStruct *pParams, Script::CScript *pScript)
{	
	int profile = 0;
	int skater = 0;
	int no_name = 0;
	
	
	
	pParams->GetInteger( NONAME, &no_name );
	profile = no_name;
	skater = no_name;
	pParams->GetInteger( CRCD(0x7ea855f0,"profile"), &profile );
	pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skater );
	Dbg_Printf( "Skater %d  Profile %d\n", skater, profile );
	Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetSkater(skater);
	Dbg_MsgAssert( pSkater, ( "Couldn't find specified skater" ) );


	// (Mick) Now check for bailboard, and kill it
	// Bit of a patch really, but a fairly safe high level one.
	// (need a more general solution for killing model that has instances...)
	if (pSkater->GetSkaterNumber() == 0)	// Only need it for single player
	{
		Obj::CCompositeObject*p_bailboard = (Obj::CCompositeObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0x884ef81b,"bailboard"));
		if (p_bailboard)
		{
			p_bailboard->MarkAsDead();
			Obj::CCompositeObjectManager::Instance()->FlushDeadObjects();
		}
	}

	Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile( profile );
	Dbg_Assert( pProfile );
    Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();
	
	// GJ:  rebuilding the face and texture takes
	// a while, causes Pcm audio "panic" assert...
	// just to be safe, i'll pause the music while
	// the model is being rebuilt.
	/*bool should_pause_music = !Pcm::MusicIsPaused();
	
	if ( should_pause_music )
	{
		Pcm::PauseMusic( true );
	}*/

	// create the model
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterGeomHeap(pSkater->GetHeapIndex()));

	Nx::CEngine::sFinishRendering();
   
#ifdef __PLAT_NGC__
	File::PreMgr* pre_mgr = File::PreMgr::Instance();
	bool loaded = pre_mgr->InPre( "skaterparts.pre");
	if ( !loaded )
	{
		pre_mgr->LoadPre( "skaterparts.pre", false);
	}
#endif
	Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pSkater );
	pModelComponent->InitModelFromProfile( pAppearance, false, pSkater->GetHeapIndex() );
#ifdef __PLAT_NGC__
	if ( !loaded )
	{
		pre_mgr->UnloadPre( "skaterparts.pre");
	}
#endif
	
	Mem::Manager::sHandle().PopContext();

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterHeap(pSkater->GetHeapIndex()));
	
	pSkater->UpdateStats( pProfile );

	Obj::CTrickComponent* pTrickComponent = GetTrickComponentFromObject(pSkater);
	Dbg_Assert(pTrickComponent);
	pTrickComponent->UpdateTrickMappings( pProfile );

	pSkater->UpdateSkaterInfo( pProfile );

	Mem::Manager::sHandle().PopContext();

    uint32 board;
    if ( pParams->GetChecksum( CRCD(0x41eec86b,"no_board"), &board ) )
    {
        // if Refreshing the skater in the CAS level,
        // Hide the board
        //Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
        //if ( skate_mod->m_cur_level == CRCD(0xa368b4f1,"load_cas") )
        //{
            // turn off board
            Obj::CModelComponent* pModelComponent = GetModelComponentFromObject(pSkater);
            pModelComponent->HideGeom(CRCD(0xa7a9d4b8,"board"), true, true);

			// GJ THPS5: Kludge to fix TT12405: CAS - board 
			// appears when adjusting leg tattoos
			// generally, this function will be called from a spawned script,
			// which gets called after the composite objects are updated
			// for the frame...  because of this, it will take 1 frame
			// for the hide flag to take effect, unless we explicitly update
			// the model component here
            pModelComponent->Update();
        //}
    }
    
	/*if ( should_pause_music )
	{
		// unpause the music
		Pcm::PauseMusic( false );
	}*/
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | EditPlayerAppearance | This runs an object script on the specified
// appearance structure
// @parm name | target | name of script that gets run on the player's appearance
// @parmopt structure | targetParams | | extra parameters sent to the target script
bool ScriptEditPlayerAppearance(Script::CStruct *pParams, Script::CScript *pScript)
{	
	int profile_num = 0;
	int no_name = 0;
	
	pParams->GetInteger( NONAME, &no_name );
	profile_num = no_name;
	pParams->GetInteger( CRCD(0x7ea855f0,"profile"), &profile_num );
	Dbg_Printf( "Profile %d\n", profile_num );

	Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile( profile_num );
	Dbg_Assert( pProfile );
    Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();
	   	
	uint32 target;
	pParams->GetChecksum(CRCD(0xb990d003,"target"), &target, Script::ASSERT);

	Script::CStruct* pTargetParams = NULL;
	pParams->GetStructure(CRCD(0x46a2869c,"targetParams"), &pTargetParams, Script::NO_ASSERT);

	// runs an object script on the specified appearance structure
	Script::RunScript(target, pTargetParams, pAppearance);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetCurrentSkaterProfileIndex | This returns the currently selected
// profile (either player 0 or player 1), presumably for use by other script functions
// @parm int | currentSkaterProfileIndex | index of currently selected profile (return value)
bool ScriptGetCurrentSkaterProfileIndex(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Obj::CPlayerProfileManager*	pPlayerProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
	int index = pPlayerProfileManager->GetCurrentProfileIndex();
	pScript->GetParams()->AddInteger(CRCD(0x3c64476b,"currentSkaterProfileIndex"), index);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetCustomSkaterName | 
bool ScriptGetCustomSkaterName(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Obj::CPlayerProfileManager*	pPlayerProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
	const char* name = pPlayerProfileManager->GetProfileTemplate(CRCD(0xa7be964,"custom"))->GetDisplayName();
	pScript->GetParams()->AddString(CRCD(0x2ab66cb8,"display_name"), name);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetScorePot | this will reset the local skater's current score pot.
// This currently just calls the Bail member func of the score object...
bool ScriptResetScorePot(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	
	bool useBailStyle = pParams->ContainsFlag(CRCD(0x929eb4d5, "UseBailStyle"));
	
    for (uint32 i=0;i<skate_mod->GetNumSkaters();i++)
	{
		Obj::CSkater* p_Skater = skate_mod->GetSkater( i );
		if ( p_Skater->IsLocalClient() )
		{   
			Mdl::Score *pScore=p_Skater->GetScoreObject();

			Dbg_Assert( pScore );
			pScore->Bail(!useBailStyle);
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptPrintSkaterStats | prints the specified skater's stats
bool ScriptPrintSkaterStats( Script::CStruct* pParams, Script::CScript* pScript )
{
#ifdef __NOPT_ASSERT__
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
    for (uint32 i=0;i<skate_mod->GetNumSkaters();i++)
	{
		printf("Skater %d stats:\n", i);

		Obj::CSkater* pSkater = skate_mod->GetSkater( i );
		
		printf("Air    %2.2f\n",pSkater->GetStat(Obj::CSkater::STATS_AIR));		
		printf("Run    %2.2f\n",pSkater->GetStat(Obj::CSkater::STATS_RUN));		
		printf("Ollie  %2.2f\n",pSkater->GetStat(Obj::CSkater::STATS_OLLIE));		
		printf("Speed  %2.2f\n",pSkater->GetStat(Obj::CSkater::STATS_SPEED));		
		printf("Spin   %2.2f\n",pSkater->GetStat(Obj::CSkater::STATS_SPIN));		
		printf("Flip   %2.2f\n",pSkater->GetStat(Obj::CSkater::STATS_FLIPSPEED));		
		printf("Switch %2.2f\n",pSkater->GetStat(Obj::CSkater::STATS_SWITCH));		
		printf("Rail   %2.2f\n",pSkater->GetStat(Obj::CSkater::STATS_RAILBALANCE));		
		printf("Lip    %2.2f\n",pSkater->GetStat(Obj::CSkater::STATS_LIPBALANCE));		
		printf("Manual %2.2f\n",pSkater->GetStat(Obj::CSkater::STATS_MANUAL));		
	}
#endif		// __NOPT_ASSERT__

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ScriptPrintSkaterStats | prints the specified skater's stats
bool ScriptPrintSkaterStats2( Script::CStruct* pParams, Script::CScript* pScript )
{
#ifdef __NOPT_ASSERT__
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
    for (uint32 i=0;i<skate_mod->GetNumSkaters();i++)
	{
		printf("Skater %d stats:\n", i);

        Obj::CPlayerProfileManager* pProfileManager = skate_mod->GetPlayerProfileManager();
        Obj::CSkaterProfile* pProfile = pProfileManager->GetProfile( i );
		
		printf("Air    %i\n",pProfile->GetStatValue(CRCD(0x439f4704,"AIR")));		
		printf("Run    %i\n",pProfile->GetStatValue(CRCD(0xaf895b3f,"RUN")));		
		printf("Ollie  %i\n",pProfile->GetStatValue(CRCD(0x9b65d7b8,"OLLIE")));		
		printf("Speed  %i\n",pProfile->GetStatValue(CRCD(0xf0d90109,"SPEED")));		
		printf("Spin   %i\n",pProfile->GetStatValue(CRCD(0xedf5db70,"SPIN")));		
		printf("Flip   %i\n",pProfile->GetStatValue(CRCD(0x6dcb497c,"FLIP_SPEED")));		
		printf("Switch %i\n",pProfile->GetStatValue(CRCD(0x9016b4e7,"SWITCH")));		
		printf("Rail   %i\n",pProfile->GetStatValue(CRCD(0xf73a13e3,"RAIL_BALANCE")));		
		printf("Lip    %i\n",pProfile->GetStatValue(CRCD(0xae798769,"LIP_BALANCE")));		
		printf("Manual %i\n",pProfile->GetStatValue(CRCD(0xb1fc0722,"MANUAL_BALANCE")));		
	}
#endif		// __NOPT_ASSERT__

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | PrintSkaterPosition | prints the local skater's current position
bool ScriptPrintSkaterPosition( Script::CStruct* pParams, Script::CScript* pScript )
{
#ifdef __NOPT_ASSERT__
	Obj::CSkater *pSkater = Mdl::Skate::Instance()->GetLocalSkater();
	
	Mth::Vector pos = pSkater->GetPos();
	float x = pos.GetX();
	float y = pos.GetY();
	float z = pos.GetZ();

	Dbg_Message("(%f, %f, %f)", x, y, z);
#endif		// __NOPT_ASSERT__

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetSkaterPosition | prints the local skater's current position
bool ScriptGetSkaterPosition( Script::CStruct* pParams, Script::CScript* pScript )
{

    Obj::CSkater* pSkater = static_cast< Obj::CSkater* >(pScript->mpObject.Convert());
	
    if (pSkater) 
    {
    	Mth::Vector pos = pSkater->GetPos();
    	float x = pos.GetX();
    	float y = pos.GetY();
    	float z = pos.GetZ();
    
        pScript->GetParams()->AddInteger("x", x);
        pScript->GetParams()->AddInteger("y", y);
        pScript->GetParams()->AddInteger("z", z);
    
        return true;
    }
    
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetSkaterPosition | prints the local skater's current position
bool ScriptGetSkaterVelocity( Script::CStruct* pParams, Script::CScript* pScript )
{

    Obj::CSkater* pSkater = static_cast< Obj::CSkater* >(pScript->mpObject.Convert());
	
    if (pSkater) 
    {
    	Mth::Vector vel = pSkater->GetVel();
    	float x = vel.GetX();
    	float y = vel.GetY();
    	float z = vel.GetZ();
		float scale = 1.0f;
		float skew_angle = 0.0f;
    
		
		pParams->GetFloat( CRCD(0x5582fe27,"skew_angle"), &skew_angle );

        pScript->GetParams()->AddInteger(CRCD(0x6187906d,"vel_x"), x);
        pScript->GetParams()->AddInteger(CRCD(0x1680a0fb,"vel_y"), y);
        pScript->GetParams()->AddInteger(CRCD(0x8f89f141,"vel_z"), z);

		pParams->GetFloat( CRCD(0x13b9da7b,"scale"), &scale );

		vel[Y] = 0.0f;

		if( vel.Length() > 100.0f )
		{
			vel.Normalize();

			if( skew_angle != 0 )
			{
				Mth::Matrix rot_mat;
	
				rot_mat.Ident();
				rot_mat.RotateY( Mth::DegToRad( skew_angle ));
	
				vel = rot_mat.Transform( vel );
			}

			pScript->GetParams()->AddInteger("scaled_x", vel[X] * scale );
			pScript->GetParams()->AddInteger("scaled_y", vel[Y] * scale );
			pScript->GetParams()->AddInteger("scaled_z", vel[Z] * scale );
		}
		else
		{
			Mth::Vector facing;

			facing = pSkater->GetMatrix()[Mth::AT];
	
			if( skew_angle != 0 )
			{
				Mth::Matrix rot_mat;
	
				rot_mat = pSkater->GetMatrix();
				rot_mat.RotateYLocal( Mth::DegToRad( skew_angle ));
	
				facing = rot_mat[Mth::AT];
			}

			pScript->GetParams()->AddInteger(CRCD(0xbded7a76,"scaled_x"), facing[X] * scale );
			pScript->GetParams()->AddInteger(CRCD(0xcaea4ae0,"scaled_y"), facing[Y] * scale );
			pScript->GetParams()->AddInteger(CRCD(0x53e31b5a,"scaled_z"), facing[Z] * scale );
		}
    
        return true;
    }
    
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetStatValue | this returns the specified stat value in
// the script's params as stat_value
// @uparm STATS_AIR | global int value of the stat (defined in physics.q)
// @parmopt name | skater | | the name of the profile you wish to get.  If you don't
// provide a name, the current skater's profile will be returned
bool ScriptGetStatValue( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 stat;
	pParams->GetChecksum( NONAME, &stat, Script::ASSERT );

	uint32 name;
	
	Obj::CPlayerProfileManager* pPlayerProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
	Obj::CSkaterProfile* pSkaterProfile = NULL;
	if ( pParams->GetChecksum( CRCD(0x5b8ab877,"skater"), &name, Script::NO_ASSERT ) )
		pSkaterProfile = pPlayerProfileManager->GetProfileTemplate( name );
	else
		pSkaterProfile = pPlayerProfileManager->GetCurrentProfile();
	
	Dbg_MsgAssert( pSkaterProfile, ( "Unable to get skater profile") );
	int value = pSkaterProfile->GetStatValue( stat );
	pScript->GetParams()->AddInteger( "stat_value", value );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetNumStatPointsAvailable | returns the number in the scripts
// params as points_available
// @parm int | player | the player number to check 
bool ScriptGetNumStatPointsAvailable( Script::CStruct* pParams, Script::CScript* pScript )
{
	int player;
	pParams->GetInteger( CRCD(0x67e6859a,"player"), &player, Script::ASSERT );

	
	Obj::CPlayerProfileManager* pProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
	Obj::CSkaterProfile* pProfile = pProfileManager->GetProfile( player );
	int num = pProfile->GetNumStatPointsAvailable();
	pScript->GetParams()->AddInteger( "points_available", num );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UnlockSkater | this makes locked characters accessible
// to the player
// @parm name | skater | the name of the locked character to unlock
bool ScriptUnlockSkater( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 name;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &name, Script::ASSERT );

	Obj::CPlayerProfileManager* pPlayerProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
	Obj::CSkaterProfile* pSkaterProfile = pPlayerProfileManager->GetProfileTemplate( name );
	Dbg_MsgAssert( pSkaterProfile, ( "Unable to get skater profile") );

	int isLocked = 1;
	pSkaterProfile->GetInfo()->GetInteger( "is_hidden", &isLocked, Script::ASSERT );
	if ( isLocked )
	{
#ifdef __NOPT_ASSERT__
		// don't need to sync it up to the current skater profiles,
		// because, theoretically, the player won't be able to
		// choose him through the front end...
		Obj::CSkaterProfile* pCurrentSkaterProfile = pPlayerProfileManager->GetCurrentProfile();
		Dbg_MsgAssert( pCurrentSkaterProfile->GetSkaterNameChecksum() != name, 
					   ( "Locked skaters aren't supposed to be currently selected!", 
						 Script::FindChecksumName(name) ) );
#endif
		Dbg_Message( "%s was locked\n", Script::FindChecksumName(name) );
		pSkaterProfile->GetInfo()->AddInteger( "is_hidden", 0 );
	}
	else
	{
		Dbg_Message( "%s was not locked\n", Script::FindChecksumName(name) );
	}
		
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetActualCASOptionStruct( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 partChecksum;
	pParams->GetChecksum(CRCD(0xb6f08f39,"part"), &partChecksum, Script::ASSERT);
	
	uint32 descChecksum;
	pParams->GetChecksum(CRCD(0x4bb2084e,"desc_id"), &descChecksum, Script::ASSERT);

	Script::CStruct* pActualStruct = Cas::GetOptionStructure( partChecksum, descChecksum );

	pScript->GetParams()->AppendStructure( pActualStruct );

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetActualPlayerAppearancePart( Script::CStruct* pParams, Script::CScript* pScript )
{
	int player_num = 0;
	pParams->GetInteger(CRCD(0x67e6859a,"player"), &player_num);
	
	Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile(player_num);
	Dbg_Assert(pProfile);
    Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();

	uint32 partChecksum;
	if ( pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &partChecksum, Script::NO_ASSERT ) )
	{
		Script::CStruct* pVirtualStruct;
		pVirtualStruct = pAppearance->GetActualDescStructure( partChecksum );
		if ( pVirtualStruct )
		{
			pScript->GetParams()->AppendStructure( pVirtualStruct );
			return true;
		}
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetPlayerAppearancePart( Script::CStruct* pParams, Script::CScript* pScript )
{	
	int player_num = 0;
	pParams->GetInteger(CRCD(0x67e6859a,"player"), &player_num);
	
	Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile(player_num);
	Dbg_Assert(pProfile);
    Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();

	uint32 partChecksum;
	if ( pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &partChecksum, Script::NO_ASSERT ) )
	{
		Script::CStruct* pVirtualStruct;
		pVirtualStruct = pAppearance->GetVirtualDescStructure( partChecksum );
		if ( pVirtualStruct )
		{
			pScript->GetParams()->AppendStructure( pVirtualStruct );
			return true;
		}
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptSetPlayerAppearanceColor( Script::CStruct* pParams, Script::CScript* pScript )
{
	int player_num = 0;
	pParams->GetInteger(CRCD(0x67e6859a,"player"), &player_num);
	
	Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile(player_num);
	Dbg_Assert(pProfile);
    Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();
	Dbg_Assert( pAppearance );

	uint32 partChecksum;
	pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &partChecksum, Script::ASSERT );

	Script::CStruct* pVirtualStruct;
	pVirtualStruct = pAppearance->GetVirtualDescStructure( partChecksum );

	if ( pVirtualStruct )
	{
		float h, s, v;
		int useDefaultHSV;
		pParams->GetFloat( CRCD(0x6e94f918,"h"), &h, Script::ASSERT );
		pParams->GetFloat( CRCD(0xe4f130f4,"s"), &s, Script::ASSERT );
		pParams->GetFloat( CRCD(0x949bc47b,"v"), &v, Script::ASSERT );
		pParams->GetInteger( CRCD(0x97dbdde6,"use_default_hsv"), &useDefaultHSV, Script::ASSERT );

		pVirtualStruct->AddInteger( CRCD(0x6e94f918,"h"), (int)h );
		pVirtualStruct->AddInteger( CRCD(0xe4f130f4,"s"), (int)s );
		pVirtualStruct->AddInteger( CRCD(0x949bc47b,"v"), (int)v );
		pVirtualStruct->AddInteger( CRCD(0x97dbdde6,"use_default_hsv"), useDefaultHSV );
	}
	else
	{
		Dbg_Message( "Nothing to color...  (appearance doesn't contain %s)", Script::FindChecksumName(partChecksum) );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptSetPlayerAppearanceScale( Script::CStruct* pParams, Script::CScript* pScript )
{
	int player_num = 0;
	pParams->GetInteger(CRCD(0x67e6859a,"player"), &player_num);
	
	Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile(player_num);
	Dbg_Assert(pProfile);
    Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();
	Dbg_Assert( pAppearance );

	uint32 partChecksum;
	pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &partChecksum, Script::ASSERT );

	Script::CStruct* pVirtualStruct;
	pVirtualStruct = pAppearance->GetVirtualDescStructure( partChecksum );

	if ( !pVirtualStruct )
	{
		pVirtualStruct = new Script::CStruct;
		Script::CStruct* pAppearanceStruct = pAppearance->GetStructure();
		pAppearanceStruct->AddStructurePointer( partChecksum, pVirtualStruct );
	}

	float x;
	if ( pParams->GetFloat( CRCD(0x7323e97c,"x"), &x, Script::NO_ASSERT ) )
	{
		pVirtualStruct->AddInteger( CRCD(0x7323e97c,"x"), (int)x );
	}
	
	float y;
	if ( pParams->GetFloat( CRCD(0x424d9ea,"y"), &y, Script::NO_ASSERT ) )
	{
		pVirtualStruct->AddInteger( CRCD(0x424d9ea,"y"), (int)y );
	}
	
	float z;
	if ( pParams->GetFloat( CRCD(0x9d2d8850,"z"), &z, Script::NO_ASSERT ) )
	{
		pVirtualStruct->AddInteger( CRCD(0x9d2d8850,"z"), (int)z );
	}
	
	int useDefaultScale;
	if ( pParams->GetInteger( CRCD(0x5a96985d,"use_default_scale"), &useDefaultScale, Script::NO_ASSERT ) )
	{
		pVirtualStruct->AddInteger( CRCD(0x5a96985d,"use_default_scale"), useDefaultScale );
	}	

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptSetPlayerAppearanceUV( Script::CStruct* pParams, Script::CScript* pScript )
{
	int player_num = 0;
	pParams->GetInteger(CRCD(0x67e6859a,"player"), &player_num);
	
	Obj::CSkaterProfile* pProfile = Mdl::Skate::Instance()->GetProfile(player_num);
	Dbg_Assert(pProfile);
    Gfx::CModelAppearance* pAppearance = pProfile->GetAppearance();
	Dbg_Assert( pAppearance );

	uint32 partChecksum;
	pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &partChecksum, Script::ASSERT );

	Script::CStruct* pVirtualStruct;
	pVirtualStruct = pAppearance->GetVirtualDescStructure( partChecksum );

	if ( pVirtualStruct )
	{
		float u, v, uv_scale, uv_rot;
		int useDefaultUV;
		pParams->GetFloat( CRCD(0xcf6aa087,"uv_u"), &u, Script::ASSERT );
		pParams->GetFloat( CRCD(0x5663f13d,"uv_v"), &v, Script::ASSERT );
		pParams->GetFloat( CRCD(0x266932c8,"uv_scale"), &uv_scale, Script::ASSERT );
		pParams->GetFloat( CRCD(0x1256b6c6,"uv_rot"), &uv_rot, Script::ASSERT );
		pParams->GetInteger( CRCD(0x8602f6ee,"use_default_uv"), &useDefaultUV, Script::ASSERT );
				   
		pVirtualStruct->AddFloat( CRCD(0xcf6aa087,"uv_u"), u );
		pVirtualStruct->AddFloat( CRCD(0x5663f13d,"uv_v"), v );
		pVirtualStruct->AddFloat( CRCD(0x266932c8,"uv_scale"), uv_scale );
		pVirtualStruct->AddFloat( CRCD(0x1256b6c6,"uv_rot"), uv_rot );
		pVirtualStruct->AddInteger( CRCD(0x8602f6ee,"use_default_uv"), useDefaultUV );
	}
	else
	{
		Dbg_Message( "Nothing to wibble...  (appearance doesn't contain %s yet?)", Script::FindChecksumName(partChecksum) );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* GetNextUndisqualified( Script::CArray* pPartArray, int startIndex )
{
	Dbg_Assert( pPartArray );

	startIndex++;
	if ( startIndex >= (int)pPartArray->GetSize() )
	{
		startIndex -= pPartArray->GetSize();
	}

	for ( int i = 0; i < (int)pPartArray->GetSize(); i++ )
	{
		int currIndex = (i + startIndex) % pPartArray->GetSize();

		Script::CStruct* pCurrStruct = pPartArray->GetStructure( currIndex );
		
		int disqualified;
		pCurrStruct->GetInteger( "disqualified", &disqualified, true );
		if ( !disqualified )
		{
			return pCurrStruct;
		}
	}

	// none found...
	return pPartArray->GetStructure( startIndex );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* GetPrevUndisqualified( Script::CArray* pPartArray, int startIndex )
{
	Dbg_Assert( pPartArray );

	startIndex--;
	if ( startIndex < 0 )
	{
		startIndex += pPartArray->GetSize();
	}

	for ( int i = (int)pPartArray->GetSize(); i >= 0; i-- )
	{
		int currIndex = (i + startIndex) % pPartArray->GetSize();

		Script::CStruct* pCurrStruct = pPartArray->GetStructure( currIndex );
		
		int disqualified;
		pCurrStruct->GetInteger( "disqualified", &disqualified, true );
		if ( !disqualified )
		{
			return pCurrStruct;
		}
	}

	// none found...
	return pPartArray->GetStructure( startIndex );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | FlushDeadObjects | 
// flush any objects that are marked as dead
// used so we can be sure they have gone, so we can re-use the memory
// and also avoid fragmentation
bool ScriptFlushDeadObjects( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::Skate::Instance()->GetObjectManager()->FlushDeadObjects();
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | BindTrickToKeyCombo | binds the given trick to the key combo
// @parm name | key_combo |
// @parm name | trick_checksum |
// @parmopt int | update_mappings | 1 | automatically update the trick mappings.
// This should only be 0 in a split screen game
bool ScriptBindTrickToKeyCombo( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 key_combo;
	pParams->GetChecksum( "key_combo", &key_combo, Script::ASSERT );

	uint32 trick_checksum=0;
	pParams->GetChecksum( "trick", &trick_checksum );

	// For create-a-tricks, the trick parameter is an integer index.
	int create_a_trick=0;
	bool got_create_a_trick=false;
	if (pParams->GetInteger( "trick", &create_a_trick ))
	{
		got_create_a_trick=true;
	}	

	Dbg_MsgAssert(got_create_a_trick || trick_checksum,("\n%s\nMissing trick parameter",pScript->GetScriptInfo()));
	
	bool update_mappings = true;
	int update_param = 1;
	pParams->GetInteger( "update_mappings", &update_param, Script::NO_ASSERT );
	if ( update_param == 0 )
		update_mappings = false;

	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();

	// are we binding a special trick?
	if ( pParams->ContainsFlag( "special" ) )
	{
		int index;
		if ( !pParams->GetInteger( "index", &index, Script::NO_ASSERT ) )
        {
            return false;
        }
        
		// add a new slot and update info
		Obj::SSpecialTrickInfo trickInfo;
		trickInfo.m_TrickSlot = key_combo;
		if (got_create_a_trick)
		{
            trickInfo.m_TrickName = create_a_trick;
        }
        else
        {
            trickInfo.m_TrickName = trick_checksum;
        }
		if ( pParams->ContainsFlag( CRCD(0x61a1bc57,"cat") ) )
		{
			trickInfo.m_isCat = true;
		}
        else
        {
			trickInfo.m_isCat = false;
		}

        pSkaterProfile->SetSpecialTrickInfo( index, trickInfo, update_mappings );
	}
	else
	{
		Script::CStruct* pTricks = pSkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );
		
		if (got_create_a_trick)
		{
			// Make sure any existing checksum parameter is removed.
			pTricks->RemoveComponent(key_combo);
			pTricks->AddInteger( key_combo, create_a_trick );
		}
		else
		{
			pTricks->RemoveComponent(key_combo);
            pTricks->AddChecksum( key_combo, trick_checksum );
		}	
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// this does breadth first recursion on the structure and returns trick
// highest in the tree (parent overwrites child).
static uint32 s_find_trick_in_mapping( Script::CStruct* pMapping, uint32 trick_checksum, int recurse_level )
{
	if ( recurse_level > 10 )
	{
		Dbg_MsgAssert( 0, ( "GetKeyComboArrayFromTrickArray recursed too much." ) );
		return 0;
	}

	Script::CComponent* p_comp = pMapping->GetNextComponent( NULL );
	uint32 rv = 0;
	while ( p_comp )
	{
		Script::CStruct* pTestStruct = Script::GetStructure( p_comp->mChecksum, Script::NO_ASSERT );
		Script::CComponent* p_next = pMapping->GetNextComponent( p_comp );
		if ( !rv && p_comp->mType == ESYMBOLTYPE_STRUCTURE )
		{
			// printf("recursing to level %i\n", recurse_level);
			rv = s_find_trick_in_mapping( p_comp->mpStructure, trick_checksum, recurse_level + 1 );
		}
		else if ( !rv && pTestStruct )
		{
			// printf("recursing to level %i\n", recurse_level);
			rv = s_find_trick_in_mapping( pTestStruct, trick_checksum, recurse_level + 1 );
		}
		if ( p_comp->mType == ESYMBOLTYPE_NAME && p_comp->mChecksum == trick_checksum )
			rv = p_comp->mNameChecksum;

		p_comp = p_next;
	}
	return rv;
}

static uint32 s_find_cat_in_mapping( Script::CStruct* pMapping, int cat_num )
{
	Script::CComponent* p_comp = pMapping->GetNextComponent( NULL );
	uint32 rv = 0;

    while ( p_comp )
	{
        Script::CComponent* p_next = pMapping->GetNextComponent( p_comp );
        if ( p_comp->mType == ESYMBOLTYPE_INTEGER && p_comp->mIntegerValue == cat_num )
        {
            printf("s_find_cat_in_mapping found %i\n", p_comp->mIntegerValue );
            rv = p_comp->mNameChecksum;
        }
        p_comp = p_next;
	}
    return rv;
}

// @script | GetKeyComboBoundToTrick | looks for any key combos that are currently
// associated with the specified trick.  The result is stored in current_key_combo
// @parm name | trick | trick to look for
bool ScriptGetKeyComboBoundToTrick( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 trick_checksum = NULL;
    int cat_num;
    if (!pParams->GetChecksum( CRCD(0x270f56e1,"trick"), &trick_checksum, Script::NO_ASSERT) )
    {
        pParams->GetInteger( CRCD(0xa75b8581,"cat_num"), &cat_num, Script::ASSERT );
    }
	
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();

	uint32 key_combo = 0;
	bool found_special = false;
	int special_index = 0;
	
    if (trick_checksum)
    {
        if ( !pParams->ContainsFlag( CRCD(0xb394c01c,"special") ) )
    	{
    		Script::CStruct* pTricks = pSkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );
    	
    		uint32 test_key_combo = s_find_trick_in_mapping( pTricks, trick_checksum, 0 );
    		
    		// check that the mapping holds true the other way (ie, when we grab the trick
    		// based on this key_combo, do we get the same trick?).
    		uint32 test_trick_checksum = 0;
    		if ( pTricks->GetChecksum( test_key_combo, &test_trick_checksum, Script::NO_ASSERT ) && test_trick_checksum == trick_checksum )
    				key_combo = test_key_combo;
    	}
    	else
    	{
    		if ( !key_combo && pParams->ContainsFlag( CRCD(0xb394c01c,"special") ) )
    		{
    			// printf("searching for special trick\n");
    			int numTricks = pSkaterProfile->GetNumSpecialTrickSlots();
    			// printf("%i tricks to check\n", numTricks);
    	
    			// search through current special tricks
    			for ( int i = 0; i < numTricks; i++ )
    			{
    				Obj::SSpecialTrickInfo trick_info = pSkaterProfile->GetSpecialTrickInfo( i );
    				if ( trick_info.m_TrickName == trick_checksum )
    				{
    					printf("GetKeyComboBoundToTrick found a special trick\n");
    					found_special = true;
    					special_index = i;
    					key_combo = trick_info.m_TrickSlot;
    					break;
    				}
    			}
    		}
    	}
    }
    else
    {
        Script::CStruct* pTricks = pSkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );
    	
        key_combo = s_find_cat_in_mapping( pTricks, cat_num );
    }
    
	
	if ( key_combo )
	{
		pScript->GetParams()->AddChecksum( "current_key_combo", key_combo );
		if ( found_special )
		{
			printf("current_index: %i\n", special_index);
			pScript->GetParams()->AddInteger( "current_index", special_index );
		}
		return true;
	}	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UpdateTrickMappings | forces the trick mappings to update
// @parmopt int | skater | | the skater num
// @parmopt name | skaterId | | the id of the skater
bool ScriptUpdateTrickMappings( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	Obj::CPlayerProfileManager* pProfileMan = pSkate->GetPlayerProfileManager();
	
	// TODO: if we ever do split screen network games, this will not work!
	// It assumes there is only one local skater when in a net game
	if ( gamenet_man->InNetGame() )
	{
		Obj::CSkater* pSkater = pSkate->GetLocalSkater();
		Dbg_MsgAssert( pSkater, ( "UpdateTrickMappings couldn't find a local skater" ) );
		
		Obj::CSkaterProfile* pProfile = pProfileMan->GetProfile( 0 );
		
		Obj::CTrickComponent* pTrickComponent = GetTrickComponentFromObject(pSkater);
		Dbg_Assert(pTrickComponent);
		pTrickComponent->UpdateTrickMappings( pProfile );
	}
	else
	{
		int skater_num;
		pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skater_num, Script::ASSERT );
		Obj::CSkater* pSkater = pSkate->GetSkater( skater_num );
		Dbg_Assert( pSkater );
		Obj::CSkaterProfile* pProfile = pProfileMan->GetProfile( skater_num );
		Dbg_Assert( pProfile );
	
		if ( pSkater && pProfile )
		{
			Obj::CTrickComponent* pTrickComponent = GetTrickComponentFromObject(pSkater);
			Dbg_Assert(pTrickComponent);
			pTrickComponent->UpdateTrickMappings( pProfile );

		}
	}
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetConfigurableTricksFromType | returns an array of trick checksums
// which have the type specified.  The array is stored as ConfigurableTricks
// @parm name | type | the type to search for
// @flag special | search for special tricks of this type
bool ScriptGetConfigurableTricksFromType( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 type = 0;
	pParams->GetChecksum( "type", &type, Script::NO_ASSERT );

	bool find_special = pParams->ContainsFlag( "special" );

	Dbg_MsgAssert( type || find_special, ( "You must specify a type or provide the special flag" ) );

	uint32 trick_list[128];
	int num_found = 0;
    int cat_list[128];
    int num_cats_found = 0;
    
    // grab some checksums
    //uint32 createdtrick = Script::GenerateCRC( "CreatedTrick" );
    uint32 grabtrick = Script::GenerateCRC( "GrabTrick" );
    uint32 fliptrick = Script::GenerateCRC( "FlipTrick" );

	// grab the list of all tricks
	Script::CArray* p_all_tricks = Script::GetArray( "ConfigurableTricks", Script::ASSERT );
	Dbg_MsgAssert( p_all_tricks->GetType() == ESYMBOLTYPE_NAME, ( "ConfigurableTricks array had wrong type\n" ) );

	int size = p_all_tricks->GetSize();
	for ( int i = 0; i < size; i++ )
	{
		Dbg_MsgAssert( num_found < 128, ( "Found too many tricks" ) );
		
		uint32 trick = p_all_tricks->GetChecksum( i );
		Script::CStruct* p_trick = Script::GetStructure( trick, Script::NO_ASSERT );

        if ( p_trick )
		{
			// get the type
			uint32 current_trick_type;

			// first check for optional TrickType var
			if ( !p_trick->GetChecksum( "TrickType", &current_trick_type, Script::NO_ASSERT ) )
			{
				if ( !p_trick->GetChecksum( "Scr", &current_trick_type, Script::NO_ASSERT ) )
				{
					// assume it's a grind
					current_trick_type = Script::GenerateCRC( "GrindTrick" );
				}
			}
			
			if ( ( find_special && !type ) || current_trick_type == type )
			{
				// get the params array and look for the special flag
				Script::CStruct* p_trick_params;
				p_trick->GetStructure( "Params", &p_trick_params, Script::ASSERT );

				if ( find_special && p_trick_params->ContainsFlag( "IsSpecial" ) )
				{
					trick_list[num_found] = trick;
					num_found++;
				}
				else if ( !find_special && !p_trick_params->ContainsFlag( "IsSpecial" ) )
				{
					trick_list[num_found] = trick;
					num_found++;
				}
			}
		}
	}

    // if type is grab or flip then list created tricks too!
    if ( type == grabtrick || type == fliptrick )
    {
        Mdl::Skate * pSkate = Mdl::Skate::Instance();
        Obj::CSkater* pSkater = pSkate->GetLocalSkater();
	    if ( pSkater )
        {
            int size = Game::vMAX_CREATED_TRICKS;
            for ( int i = 1; i < size; i++ )    // start at one because 0 is just a clipboard!
        	{
                int full = 0;
                if ( pSkater->m_created_trick[i]->mp_other_params->GetInteger( CRCD(0x1f802b5f,"full"), &full, Script::NO_ASSERT ) )
                {
                    if ( full )
                    {
                        cat_list[num_cats_found] = i; //trick;
                        num_cats_found++;
                    }
                }
            }
        }
    }

	if ( num_found == 0 )
		return false;

	// create an array to return
	Script::CArray* p_configurable_tricks = new Script::CArray();
	p_configurable_tricks->SetSizeAndType( num_found, ESYMBOLTYPE_NAME );
	for ( int i = 0; i < num_found; i++ )
		p_configurable_tricks->SetChecksum( i, trick_list[i] );

	// add the array
	pScript->GetParams()->AddArrayPointer( "ConfigurableTricks", p_configurable_tricks );

    if ( num_cats_found > 0 )
    {
        // create an array to return
    	Script::CArray* p_configurable_cats = new Script::CArray();
    	p_configurable_cats->SetSizeAndType(num_cats_found, ESYMBOLTYPE_INTEGER );
    	for ( int i = 0; i < num_cats_found; i++ )
    		p_configurable_cats->SetInteger( i, cat_list[i] );
    
    	// add the array
    	pScript->GetParams()->AddArrayPointer( "ConfigurableCats", p_configurable_cats );
    }

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | TrickIsLocked | true if the trick is associated with a skater
// that is locked
// @parm name | trick | the trick name
bool ScriptTrickIsLocked( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 trick;
	pParams->GetChecksum( "trick", &trick, Script::ASSERT );

	Script::CStruct* p_trick = Script::GetStructure( trick, Script::ASSERT );

	if ( p_trick )
	{
		Script::CStruct* p_trick_params;
		p_trick->GetStructure( "Params", &p_trick_params, Script::ASSERT );

		uint32 skater;
		if ( p_trick_params && p_trick_params->GetChecksum( CRCD(0x5b8ab877,"skater"), &skater, Script::NO_ASSERT ) )
		{
			Obj::CPlayerProfileManager* pPlayerProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
			
			if ( p_trick_params->ContainsFlag( "OnlyWith" ) )
			{
				Obj::CSkaterProfile* pCurrentProfile = pPlayerProfileManager->GetCurrentProfile();
				return ( pCurrentProfile->GetSkaterNameChecksum() != skater );
			}
			else
			{
				int num_profiles = pPlayerProfileManager->GetNumProfileTemplates();
				for ( int i = 0; i < num_profiles; i++ )
				{
					Obj::CSkaterProfile* pProfile = pPlayerProfileManager->GetProfileTemplateByIndex( i );
					if ( pProfile->GetSkaterNameChecksum() == skater )
					{
						int isLocked = 0;
						pProfile->GetInfo()->GetInteger( "is_hidden", &isLocked, Script::NO_ASSERT );
						return ( isLocked != 0 );
					}
				}
			}
			Dbg_MsgAssert( 0, ( "TrickIsLocked confused by skater %s in trick %s\n", Script::FindChecksumName( skater ), Script::FindChecksumName( trick ) ) );
		}
	}
	else
		Dbg_MsgAssert( 0, ( "TrickIsLocked couldn't find trick %s\n", Script::FindChecksumName( trick ) ) );

	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetTrickDisplayText | gets the text corresponding to this trick
// @parm name | trick | the trick name
bool ScriptGetTrickDisplayText( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 trick;
	pParams->GetChecksum( "trick", &trick, Script::ASSERT );

	Script::CStruct* p_trick = Script::GetStructure( trick, Script::ASSERT );

	if ( p_trick )
	{
		Script::CStruct* p_trick_params;
		p_trick->GetStructure( "Params", &p_trick_params, Script::ASSERT );

		const char* p_trick_name;
		p_trick_params->GetLocalString( CRCD(0xa1dc81f9,"name"), &p_trick_name, Script::ASSERT );

		pScript->GetParams()->AddString( "trick_display_text", p_trick_name );

		// look for any double tap tricks
		uint32 extra_trick = 0;
		if ( !p_trick_params->GetChecksum( "ExtraTricks", &extra_trick, Script::NO_ASSERT ) )
		{
			// look for an array of extra tricks and grab the first one
			Script::CArray* p_extra_tricks;
			if ( p_trick_params->GetArray( "ExtraTricks", &p_extra_tricks, Script::NO_ASSERT ) )
				extra_trick = p_extra_tricks->GetChecksum( 0 );
		}

		// add any extra trick info
		if ( extra_trick )
		{
			// grab this extra trick
			Script::CArray* p_temp = Script::GetArray( extra_trick, Script::ASSERT );
			Script::CStruct* p_extra_trick = p_temp->GetStructure( 0 );
			Dbg_Assert( p_extra_trick );
			Script::CStruct* p_extra_trick_params;
			p_extra_trick->GetStructure( "Params", &p_extra_trick_params, Script::ASSERT );
			const char* p_extra_trick_string;
			p_extra_trick_params->GetString( CRCD(0xa1dc81f9,"name"), &p_extra_trick_string, Script::ASSERT );
			pScript->GetParams()->AddString( "extra_trick_string", p_extra_trick_string );
			pScript->GetParams()->AddChecksum( "extra_trick_checksum", extra_trick );
		}

		return true;
	}
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetSpecialTrickInfo | gets the special trick info for the slot
// specified
// @parm int | index | the special trick slot
bool ScriptGetSpecialTrickInfo( Script::CStruct* pParams, Script::CScript* pScript )
{
	int index;
	pParams->GetInteger( "index", &index, Script::ASSERT );

	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	Obj::CSkaterProfile* p_SkaterProfile = skate_mod->GetCurrentProfile();
	Obj::SSpecialTrickInfo trick_info = p_SkaterProfile->GetSpecialTrickInfo( index );

	pScript->GetParams()->AddChecksum( "special_trickslot", trick_info.m_TrickSlot );
	
    
    if ( trick_info.m_isCat )
    {
        pScript->GetParams()->AddInteger( "special_trickname", trick_info.m_TrickName );
    }
    else
    {
        pScript->GetParams()->AddChecksum( "special_trickname", trick_info.m_TrickName );
    }
    
    pScript->GetParams()->AddInteger( "isCat", trick_info.m_isCat );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetTrickType | gets the type of the specified trick and returns
// the result in the trick_type param
// @parm name | trick | the name of the trick
bool ScriptGetTrickType( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 trick_checksum;
	pParams->GetChecksum( "trick", &trick_checksum, Script::ASSERT );

	Script::CStruct* p_trick = Script::GetStructure( trick_checksum, Script::ASSERT );

	uint32 trick_type;
	// first check for optional TrickType var
	if ( !p_trick->GetChecksum( "TrickType", &trick_type, Script::NO_ASSERT ) )
	{
		if ( !p_trick->GetChecksum( "Scr", &trick_type, Script::NO_ASSERT ) )
		{
			// assume it's a grind
			trick_type = Script::GenerateCRC( "GrindTrick" );
		}
	}
	pScript->GetParams()->AddChecksum( "trick_type", trick_type );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetIndexOfItemContaining | this will search through the given
// array (must be an array of structures) for the structure containing a param
// the index will be placed in the calling script's params (index)
// @parm array | array | the array to search
// @parmopt int | index | 0 | index to start at
// @parm name | name | name of the param to search for
// @parm name | value | value param should have for a match
bool ScriptGetIndexOfItemContaining( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 array_name;
	pParams->GetChecksum( "array", &array_name, Script::ASSERT );
	Script::CArray* p_array = Script::GetArray( array_name, Script::ASSERT );
	Dbg_MsgAssert( p_array->GetType() == ESYMBOLTYPE_STRUCTURE, ( "This function only works on arrays of structures." ) );

	uint32 item;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &item, Script::ASSERT );

	int index = 0;
	pParams->GetInteger( "index", &index, Script::NO_ASSERT );

	uint32 value;
	pParams->GetChecksum( "value", &value, Script::ASSERT );

	int array_size = p_array->GetSize();
	for ( ; index < array_size; index++ )
	{
		Script::CStruct* p_struct = p_array->GetStructure( index );
		if ( p_struct->ContainsComponentNamed( item ) )
		{
			uint32 item_value;
			p_struct->GetChecksum( item, &item_value, Script::ASSERT );
			if ( item_value == value )
			{
				pScript->GetParams()->AddInteger( "index", index );
				return true;
			}
		}
	}
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetLevelRecords | appends the records to the calling script's params
bool ScriptGetLevelRecords( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	
	int levelNum;
	if ( !pParams->GetInteger( "level", &levelNum, Script::NO_ASSERT ) )
		levelNum = pSkate->GetCareer()->GetLevel();	
	
	Records::CGameRecords* pGameRecords = pSkate->GetGameRecords();
	Records::CLevelRecords* pLevelRecords = pGameRecords->GetLevelRecords( levelNum );
	pLevelRecords->WriteIntoStructure( pScript->GetParams() );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ResetComboRecords | resets the score object's combo records
bool ScriptResetComboRecords( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkater* pSkater = pSkate->GetLocalSkater();
	if( pSkater )
	{
		Mdl::Score* pScore = pSkater->GetScoreObject();
	
		pScore->ResetComboRecords();
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetNumberOfTrickOccurrences | returns the number of times
// the given trick appears in the skater's current score pot
// @parm string | TrickText | trick string to look for
bool ScriptGetNumberOfTrickOccurrences( Script::CStruct *pParams, Script::CScript *pScript )
{
	const char* p_trick;
	pParams->GetString( "TrickText", &p_trick, Script::ASSERT );
	uint32 trick = Script::GenerateCRC( p_trick );

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* pSkater = skate_mod->GetLocalSkater();
	Mdl::Score* pScore = pSkater->GetScoreObject();

	int num = pScore->GetCurrentNumberOfOccurrencesByName( trick );
	pScript->GetParams()->AddInteger( "number_of_occurrences", num );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetNumSoundtracks | returns the number of soundtracks
// in the numSoundtracks param
bool ScriptGetNumSoundtracks( Script::CStruct *pParams, Script::CScript *pScript )
{
	Dbg_MsgAssert( Config::GetHardware() == Config::HARDWARE_XBOX, ( "GetNumSoundtracks can only be called on XBox" ) );
	int numSoundtracks = Nx::CEngine::sGetNumSoundtracks();
	pScript->GetParams()->AddInteger( "numSoundtracks", numSoundtracks );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetSoundtrackName( Script::CStruct *pParams, Script::CScript *pScript )
{
	Dbg_MsgAssert( Config::GetHardware() == Config::HARDWARE_XBOX, ( "GetNumSoundtracks can only be called on XBox" ) );
	int soundtrack_number;
	pParams->GetInteger( NONAME, &soundtrack_number, Script::ASSERT );
	const char* pSoundtrackName = Nx::CEngine::sGetSoundtrackName( soundtrack_number );
	pScript->GetParams()->AddString( "soundtrackName", pSoundtrackName );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | BindControllerToSkater | remaps the given controller to the
// skater
// @parm int | controller | the controller number
// @parm int | skater_heap_index | the skater num
bool ScriptBindControllerToSkater( Script::CStruct *pParams, Script::CScript *pScript )
{
	int controller;
	pParams->GetInteger( "controller", &controller, Script::ASSERT );

	int skater;
	pParams->GetInteger( "skater_heap_index", &skater, Script::ASSERT );

	printf("attempting to bind skater %i to controller %i\n", skater, controller);

	Mdl::Skate *pSkate = Mdl::Skate::Instance();
	pSkate->m_device_server_map[skater] = controller;

	pSkate->UpdateSkaterInputHandlers();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptBindFrontEndToController( Script::CStruct *pParams, Script::CScript *pScript )
{
	int controller;
	pParams->GetInteger( "controller", &controller, Script::ASSERT );

	int front_end_pad;
	pParams->GetInteger( "front_end_pad", &front_end_pad, Script::ASSERT );

	printf("attempting to bind front end %i to controller %i\n", front_end_pad, controller);

	Mdl::FrontEnd *pFrontEnd = Mdl::FrontEnd::Instance();
	
	// find any controllers using this front end mapping and switch
	int current = pFrontEnd->m_device_server_map[front_end_pad];
	for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		if ( pFrontEnd->m_device_server_map[i] == controller )
			pFrontEnd->m_device_server_map[i] = current;
	}
	pFrontEnd->m_device_server_map[front_end_pad] = controller;

	pFrontEnd->UpdateInputHandlerMappings();
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | ControllerBoundToDifferentSkater | 
// @parm int | controller |
// @parm int | skater | 
bool ScriptControllerBoundToDifferentSkater( Script::CStruct *pParams, Script::CScript *pScript )
{
	int skater;
	pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skater, Script::ASSERT );

	int controller;
	pParams->GetInteger( "controller", &controller, Script::ASSERT );

	Mdl::Skate* pSkate = Mdl::Skate::Instance();
	
	for ( int i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
	{
		// don't check the skater we're trying to bind
		if ( i == skater )
			continue;

		Obj::CSkater* pSkater = pSkate->GetSkater( i );
		if ( pSkater && pSkater->IsLocalClient() )
		{
			if ( pSkate->m_device_server_map[i] == controller )
				return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptControllerBoundToSkater( Script::CStruct* pParams, Script::CScript* pScript )
{
	int controller;
	pParams->GetInteger( "controller", &controller, Script::ASSERT );

	int skater;
	pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skater, Script::ASSERT );

	Dbg_MsgAssert( skater >= 0 && skater < Mdl::Skate::vMAX_SKATERS, ( "Bad skater index %i passed to ControllerBoundToSkater\n", skater ) );
	return ( Mdl::Skate::Instance()->m_device_server_map[skater] == controller );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetKeyComboArrayFromTrickArray | returns an array of key combos (KeyCombos)
// that correspond to the passed trick array
// @parm array | tricks | array of trick names, eg [Trick_KickFlip Trick_HeelFlip]
bool ScriptGetKeyComboArrayFromTrickArray( Script::CStruct *pParams, Script::CScript *pScript )
{
	Script::CArray* pTricks;
	pParams->GetArray( "tricks", &pTricks, Script::ASSERT );

	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();
	Script::CStruct* pTrickMapping = pSkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );

	int size = pTricks->GetSize();
	// make a return array
	Script::CArray* pKeyCombos = new Script::CArray();
	pKeyCombos->SetSizeAndType( size, ESYMBOLTYPE_NAME );

	for ( int i = 0; i < size; i++ )
	{
		uint32 trick_checksum = pTricks->GetChecksum( i );
		
		// search for this trick in the current mapping
		uint32 key_combo = s_find_trick_in_mapping( pTrickMapping, trick_checksum, 0 );

		if ( key_combo == 0 )
		{
			// look for a special trick
			int num_specials = pSkaterProfile->GetNumSpecialTrickSlots();
			for ( int j = 0; j < num_specials; j++ )
			{
				Obj::SSpecialTrickInfo trick_info = pSkaterProfile->GetSpecialTrickInfo( j );
				if ( trick_info.m_TrickName == trick_checksum )
				{
					key_combo = trick_info.m_TrickSlot;
					break;
				}
			}			
		}

		Dbg_MsgAssert( key_combo != 0, ( "GetKeyComboFromTrickArray found an unmpped trick - %s", Script::FindChecksumName( trick_checksum ) ) );
		pKeyCombos->SetChecksum( i, key_combo );
	}

	pScript->GetParams()->AddArray( "KeyCombos", pKeyCombos );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | FirstInputReceived | the game does not start checking for
// disconnected controllers until this is called
bool ScriptFirstInputReceived( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mdl::Skate::Instance()->FirstInputReceived();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | VibrateController | 
// @parm int | port | 
// @parm int | actuator |
// @parm int | percent |
bool ScriptVibrateController( Script::CStruct *pParams, Script::CScript *pScript )
{
	int port;
	pParams->GetInteger( "port", &port, Script::ASSERT );

	int actuator;
	pParams->GetInteger( "actuator", &actuator, Script::ASSERT );

	int percent;
	pParams->GetInteger( "percent", &percent, Script::ASSERT );
	
	SIO::Manager* sio_manager = SIO::Manager::Instance();

	// TODO: this won't work if we support multitap (assumes slot 0)
	SIO::Device* pDevice = sio_manager->GetDevice( port, 0 );
	if ( pDevice )
	{
		pDevice->ActivateActuator( actuator, percent );
		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | LockCurrentSkaterProfileIndex | 
bool ScriptLockCurrentSkaterProfileIndex( Script::CStruct *pParams, Script::CScript *pScript )
{
	int locked = true;
	pParams->GetInteger( NONAME, &locked, Script::ASSERT );

	Mdl::Skate * pSkate = Mdl::Skate::Instance();

	Obj::CPlayerProfileManager* pProfileMan = pSkate->GetPlayerProfileManager();
	pProfileMan->LockCurrentSkaterProfileIndex( locked );

	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetSpecialTrickInfo | 
// @parm int | slot | 
// @parm name | trick_name | 
// @parm name | key_combo |
// @flag update_mappings | 
bool ScriptSetSpecialTrickInfo( Script::CStruct *pParams, Script::CScript *pScript )
{
	int slot;
	pParams->GetInteger( "slot", &slot, Script::ASSERT );

	uint32 trick_name;
	pParams->GetChecksum( "trick_name", &trick_name, Script::ASSERT );
	
	uint32 key_combo;
	pParams->GetChecksum( "key_combo", &key_combo, Script::ASSERT );

	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	Obj::CPlayerProfileManager* pPlayerProfileManager = skate_mod->GetPlayerProfileManager();
	Obj::CSkaterProfile* pProfile = NULL;
	pProfile = pPlayerProfileManager->GetCurrentProfile();

	Dbg_MsgAssert( pProfile, ( "SetSpecialTrickInfo couldn't get a profile" ) );
	if ( pProfile )
	{
		// printf("setting slot %i to %s, %s\n", slot, Script::FindChecksumName( trick_name ), Script::FindChecksumName( key_combo ) );
		Obj::SSpecialTrickInfo trickInfo;
		trickInfo.m_TrickName = trick_name;
		trickInfo.m_TrickSlot = key_combo;
		pProfile->SetSpecialTrickInfo( slot, trickInfo, false );
		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static bool s_is_angle(Script::CArray *p_angles_array, uint32 component_name)
{
	if (!p_angles_array)
	{
		return false;
	}

	uint32 size=p_angles_array->GetSize();
	for (uint32 i=0; i<size; ++i)
	{
		if (component_name==p_angles_array->GetChecksum(i))
		{
			return true;
		}
	}
	
	return false;
}
	
static float make_angle_in_range(float a)
{
	while (a < 0.0f)
	{
		a+=360.0f;
	}
	while (a > 360.0f)
	{
		a-=360.0f;
	}
	return a;
}
		
// @script | InterpolateParameters | This will scan through the structure A, and any parameters in it
// which have type int or float and such that the same named parameter exists in B and also has type int
// or float will have their value interpolated between the two values, and the parameter put into a
// returned structure named Interpolated.
// Note that the returned structure will only contain the parameters that were interpolated, so if
// structure A contained a string, that parameter will not get into Interpolated
// @parm structure | a | The first structure
// @parm structure | b | The second structure
// @parmopt float | Proportion | 0.0 | A value between 0 and 1. 0 means all of structure A and none of structure B,
// 1 means all of B and none of A. 0.5 will therefore be half way between the two.
// @parmopt array | Ignore | | An optional array of parameter names to ignore. Parameters whose type cannot
// be interpolated, such as strings, will automatically be ignored, but the Ignore array allows certain
// integer or float params to be ignored too.
// @parmopt array | Angles | | An optional array of parameters whose values are to be treated as angles.
// So when interpolating between 10 and 350 degrees for example, instead of simply interpolating from
// the number 10 to 350, it will go from 10 backwards through 0 and 359 to 350.
bool ScriptInterpolateParameters( Script::CStruct *pParams, Script::CScript *pScript )
{
	Script::CStruct *p_new_structure=new Script::CStruct;
	
	Script::CStruct *p_a=NULL;
	pParams->GetStructure(CRCD(0x174841bc,"a"),&p_a,Script::ASSERT);
	
	Script::CStruct *p_b=NULL;
	pParams->GetStructure(CRCD(0x8e411006,"b"),&p_b,Script::ASSERT);
	
	float proportion=0.0f;
	pParams->GetFloat(CRCD(0x404e690d,"Proportion"),&proportion);
	
	Script::CArray *p_ignore=NULL;
	pParams->GetArray(CRCD(0xf277291d,"Ignore"),&p_ignore);
	
	Script::CArray *p_angles_array=NULL;
	pParams->GetArray(CRCD(0x9d2d0915,"Angles"),&p_angles_array);
	
	Script::CComponent *p_comp = p_a->GetNextComponent();
	while (p_comp)
	{
		bool ignore=false;
		
		if (p_ignore)
		{
			for (uint32 i=0; i<p_ignore->GetSize(); ++i)
			{
				if (p_comp->mNameChecksum==p_ignore->GetChecksum(i))
				{
					ignore=true;
					break;
				}
			}
		}

		if (!ignore)
		{
			uint32 component_name=0;
			float value_a=0.0f;
			bool write_integer=true;
			
			// See if the component in structure A is of a type that needs to be interpolated.
			if (p_comp->mType==ESYMBOLTYPE_INTEGER)
			{
				component_name=p_comp->mNameChecksum;
				value_a=(float)p_comp->mIntegerValue;
			}
			else if (p_comp->mType==ESYMBOLTYPE_FLOAT)
			{
				component_name=p_comp->mNameChecksum;
				value_a=p_comp->mFloatValue;
				write_integer=false;
			}
			
			// If found a candidate for interpolation, see if there is a similarly named
			// component in structure B, and if so, interpolate between the two and stick
			// a new component of that name into the new structure.
			if (component_name)
			{
				bool got_value_b=false;
				
				float value_b=0.0f;
				int integer_value_b=0;
				if (p_b->GetInteger(component_name,&integer_value_b))
				{
					got_value_b=true;
					value_b=(float)integer_value_b;
				}
				else if (p_b->GetFloat(component_name,&value_b))
				{
					got_value_b=true;
					write_integer=false;
				}
				
				if (got_value_b)
				{
					float new_value=0.0f;
					
					
					if (s_is_angle(p_angles_array,component_name))
					{
						float d=value_b-value_a;
						d=make_angle_in_range(d);
						if (d > 180.0f)
						{
							d=-(360.0f-d);
						}
						new_value=value_a+proportion*d;
						new_value=make_angle_in_range(new_value);
					}
					else
					{
						new_value=value_a+proportion*(value_b-value_a);
					}	
						
					if (write_integer)
					{
						p_new_structure->AddInteger(component_name,(int)new_value);
					}
					else
					{
						p_new_structure->AddFloat(component_name,new_value);
					}	
				}
			}
		}
				
		p_comp = pParams->GetNextComponent(p_comp);
	}	
	
	pScript->GetParams()->AddStructurePointer(CRCD(0xff6f3872,"Interpolated"),p_new_structure);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				
// @script | PlaySkaterStream | skater member func.
// @parm string | type | the stream type.
// @parmopt int | num_possible | 10 | the maximum number of streams to search for
bool ScriptPlaySkaterStream ( Script::CStruct *pParams, Script::CScript *pScript )
{
	Dbg_Assert(pScript->mpObject);
	Dbg_MsgAssert(pScript->mpObject->GetType() == SKATE_TYPE_SKATER, ("PlaySkaterStream may only be called on a skater object"));
	
	Obj::CSkater* p_skater = static_cast< Obj::CSkater* >(pScript->mpObject.Convert());
		
	const char* p_type;
	pParams->GetString( CRCD(0x7321a8d6,"type"), &p_type, Script::ASSERT );
	
	int max_num = 10;
	pParams->GetInteger( CRCD(0x58492707,"num_possible"), &max_num, Script::NO_ASSERT );

	Script::CArray* p_stream_indices = new Script::CArray();
	p_stream_indices->SetSizeAndType( max_num, ESYMBOLTYPE_INTEGER );
	// generate list of indices (start at 1)
	for ( int i = 0; i < max_num; i++ )
		p_stream_indices->SetInteger( i, i + 1 );

	// randomize list
	for ( int i = 0; i < max_num; i++ )
	{
		// grab a random index and switch with the current index
		int random_index = Mth::Rnd( max_num );
		int random_value = p_stream_indices->GetInteger( random_index );
		p_stream_indices->SetInteger( random_index, p_stream_indices->GetInteger( i ) );
		p_stream_indices->SetInteger( i, random_value );
	}

	// get the skater's first name
	char p_first_name[128];
	strcpy( p_first_name, p_skater->m_firstName );

	// resolve to last name
	char p_last_name[128];

	if ( p_skater->m_isPro )
	{
		// grab the last name string from global array
		Script::CStruct* pLastNames = Script::GetStructure( CRCD(0x5775194e,"goal_pro_last_names"), Script::ASSERT );
		uint32 first_name_checksum = Script::GenerateCRC( p_first_name );
		const char* p_temp_last_name;
		pLastNames->GetString( first_name_checksum, &p_temp_last_name, Script::ASSERT );
		Dbg_MsgAssert( strlen( p_temp_last_name ) < 128, ( "buffer overflow" ) );
		strcpy( p_last_name, p_temp_last_name );
	}
	else
	{
		strcpy( p_last_name, "custom" );
		if ( p_skater->m_isMale )
		{
			strcat( p_last_name, "m" );
		}
		else
		{
			strcat( p_last_name, "f" );
		}
	}
		
	Dbg_MsgAssert( strlen( p_last_name ) < 128, ( "buffer overflow in PlaySkaterStream: %s", p_last_name ) );
	// find the first valid stream and play
	char stream_name[128];
		
	for ( int i = 0; i < max_num; i++ )
	{
		Dbg_MsgAssert( strlen( p_last_name ) + strlen( p_type ) + 2 < 128, ( "buffer overflow in PlaySkaterStream: %s", stream_name ) );
		sprintf( stream_name, "%s_%s%02i", p_last_name, p_type, p_stream_indices->GetInteger( i ) );
		
		// printf("figured a stream name of %s\n", stream_name);
		if ( Pcm::StreamExists( Script::GenerateCRC( stream_name ) ) )
		{
			Script::CStruct* pTemp = new Script::CStruct;
			pTemp->AddChecksum( "stream_checksum", Script::GenerateCRC( stream_name ) );
			Script::RunScript( "skater_play_bail_stream", pTemp, pScript->mpObject );
			delete pTemp;
			break;
		}
	}
	Script::CleanUpArray( p_stream_indices );
	delete p_stream_indices;
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetTextureFromPath | returns the file name for a texture
// from the end of the give path
// @parm string | path | path of file including file name
bool ScriptGetTextureFromPath( Script::CStruct *pParams, Script::CScript *pScript )
{
	const char* p_path;
    char new_string[32];
    int i, t=0;

	pParams->GetString( CRCD(0xf4ab74f0,"path"), &p_path, Script::ASSERT );

    int length = strlen( p_path );

    // find last '/' in path
    for (i=(length); i>=0; i--)	 	// Mick: was previously "length+1", which would start at the character AFTER the string
    {								// which on rare occasions was a '/' char, which caused obscure crashes.
        if ( ( p_path[i] == '/' ) || ( p_path[i] == '\\' ) )
        {
            break;
        }
    }

    // copy everything after '/' into texture
    for (int p=(i+1); p<=length; p++)
    {
        new_string[t] = p_path[p];
        t++;
    }
	

	pScript->GetParams()->AddString( CRCD(0x7d99f28d,"texture"), new_string );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GetVramUsage | 
bool ScriptGetVramUsage( Script::CStruct *pParams, Script::CScript *pScript )
{
#	ifdef __PLAT_NGPS__
	//printf("FontVramStart = %uK \n", (NxPs2::FontVramStart/4) );
    //printf("FontVramBase = %uK \n", (NxPs2::FontVramBase/4) );
    printf("FontVramSize = %uK \n", (NxPs2::FontVramSize/4) );
    printf("Font Usage = %uK \n", ( (NxPs2::FontVramBase - NxPs2::FontVramStart )/4) );
#	endif
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CompositeObjectExists | 
bool ScriptCompositeObjectExists ( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 name;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &name, Script::ASSERT );
	Obj::CObject* pObj = Obj::ResolveToObject( name );
	return ( pObj != NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptClearPowerups( Script::CStruct *pParams, Script::CScript *pScript )
{
	int i;
	Mdl::Skate * pSkate = Mdl::Skate::Instance();

	for( i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
	{
		Obj::CSkater* pSkater = pSkate->GetSkater( i );
		if( pSkater )
		{
			Obj::CSkaterStateComponent *p_component = (Obj::CSkaterStateComponent*)Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( CRC_SKATERSTATE );
			p_component->ClearPowerups();
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptBroadcastProjectile( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 type;
	Mth::Vector pos, vel;
	int radius;
	float scale;
	uint32 id;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Net::Client* client;
	
	pParams->GetChecksum( CRCD(0x830ecaf,"objID"), &id );
	pParams->GetChecksum( CRCD(0x7321a8d6,"type"), &type );
	pParams->GetVector( CRCD(0x7f261953,"pos"), &pos );
	pParams->GetVector( CRCD(0xc4c809e,"vel"), &vel );
	pParams->GetFloat( CRCD(0x13b9da7b,"scale"), &scale );
	pParams->GetInteger( CRCD(0xc48391a5,"radius"), &radius );

	client = gamenet_man->GetClient( 0 );
	if( client )
	{
		Net::MsgDesc msg_desc;
		GameNet::MsgProjectile msg;
		
		msg.m_Id = id;
		msg.m_Pos = pos;
		msg.m_Vel = vel;
		msg.m_Radius = radius;
		msg.m_Scale = scale;
		msg.m_Latency = 0;
		msg.m_Type = type;

		msg_desc.m_Id = GameNet::MSG_ID_SPAWN_PROJECTILE;
		msg_desc.m_Data = &msg;
		msg_desc.m_Length = sizeof( GameNet::MsgProjectile );
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
	
		client->EnqueueMessageToServer( &msg_desc );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptBroadcastEnterVehicle ( Script::CStruct *pParams, Script::CScript *pScript )
{
	uint32 id;
	uint32 control_type;
	GameNet::Manager* gamenet_man = GameNet::Manager::Instance();
	Net::Client* client;
	
	pParams->GetChecksum( CRCD(0x5b24faaa, "SkaterId"), &id, Script::ASSERT );
	pParams->GetChecksum( CRCD(0x81cff663, "control_type"), &control_type, Script::ASSERT );
	
	client = gamenet_man->GetClient( 0 );
	if( client )
	{
		Net::MsgDesc msg_desc;
		GameNet::MsgEnterVehicle msg;
		
		msg.m_Id = id;
		msg.m_ControlType = control_type;

		msg_desc.m_Id = GameNet::MSG_ID_ENTER_VEHICLE;
		msg_desc.m_Data = &msg;
		msg_desc.m_Length = sizeof( GameNet::MsgEnterVehicle );
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
	
		client->EnqueueMessageToServer( &msg_desc );
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGetCollidingPlayerAndTeam( Script::CStruct *pParams, Script::CScript *pScript )
{
	int team = -1;
	int player_id = -1;
    Script::CStruct* p_pass_back_params = pScript->GetParams();
	float nearest_distance = 999999.9999f;
	Obj::CCompositeObject* flag_obj;

	flag_obj = static_cast <Obj::CCompositeObject*>( pScript->mpObject.Convert() );
	Dbg_Assert( flag_obj );
	
	int exclude_team;
	pParams->GetInteger(CRCD(0x6c2a140f, "exclude_team"), &exclude_team, Script::ASSERT);
	
	float radius_squared;
	pParams->GetFloat(CRCD(0xc48391a5, "radius"), &radius_squared, Script::ASSERT);
	radius_squared = FEET_TO_INCHES(radius_squared) * FEET_TO_INCHES(radius_squared);
	
	GameNet::PlayerInfo* player;
	for (uint32 i = 0; i < Mdl::Skate::Instance()->GetNumSkaters(); i++)
	{
		Obj::CCompositeObject* p_skater = Mdl::Skate::Instance()->GetSkater(i);
		
		if( p_skater )
		{
			player = GameNet::Manager::Instance()->GetPlayerByObjectID(p_skater->GetID());
			if (!player) continue;

			// If a player on the flag's team collides with the flag and doesn't have an enemy flag, just ignore it
			if ((player->m_Team == exclude_team) && !player->HasCTFFlag()) continue;

			// If a player already has a flag, just ignore it. They're being greedy.
			if ((player->m_Team != exclude_team ) && player->HasCTFFlag()) continue;
			
			float this_dist = Mth::DistanceSqr( p_skater->m_pos, flag_obj->GetPos());
			
			if (this_dist < radius_squared && this_dist < nearest_distance)
			{
				nearest_distance = this_dist;
				player_id = p_skater->GetID();
			}
		}
	}

	if (player_id > -1)
	{
		player = GameNet::Manager::Instance()->GetPlayerByObjectID(player_id);
		team = player->m_Team;
	}

	p_pass_back_params->AddInteger(CRCD(0x3b1f59e0, "team"), team);
	p_pass_back_params->AddInteger(CRCD(0x67e6859a, "player"), player_id);
	return true;
}

bool ScriptLobbyCheckKeyboard( Script::CStruct *pParams, Script::CScript *pScript )
{
	int num_chars;
	char makes[256];
    
	num_chars = SIO::KeyboardRead( makes );

    if( num_chars > 0 )
	{
		// Space brings up the chat interface
		if( makes[0] == 32 )
		{
			Script::CStruct* pParams;
			
			// Enter and space act as "choose" only if you're not currently using the on-screen keyboard
            pParams = new Script::CStruct;
			pParams->AddChecksum( Script::GenerateCRC( "id" ), Script::GenerateCRC( "keyboard_anchor" ));
            if( Obj::ScriptObjectExists( pParams, NULL ) == false )
			{
                Script::RunScript( "lobby_enter_kb_chat" );
                SIO::KeyboardClear();
			}
            delete pParams;
		}
    }
    return true;
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*
bool ScriptMarkRestarts ( Script::CStruct *pParams, Script::CScript *pScript )
{
	// Green:	Player1
	// Purple:	Multiplayer
	// Cyan:	Horse
	// Red:		CTF
	// Blue:	Team
	// Yellow:	Crown
	
	Script::CArray *pNodeArray = Script::GetArray(CRCD(0xc472ecc5, "NodeArray"));
	for (int i = pNodeArray->GetSize(); i--; )
	{
		uint32 color = 0;
		Mth::Vector offset;
		
		uint32 node_class;
		pNodeArray->GetStructure(i)->GetChecksum(CRCD(0x12b4e660, "Class"), &node_class);
		if (node_class == CRCD(0x1806ddf8, "Restart"))
		{
			Script::CArray *pRestartTypes;
			pNodeArray->GetStructure(i)->GetArray(CRCD(0xdd304987, "restart_types"), &pRestartTypes);
			for (int j = pRestartTypes->GetSize(); j--; )
			{
				switch (pRestartTypes->GetChecksum(j))
				{
					case CRCC(0x41639ce5, "Player1"):
						color = MAKE_RGB(0, 255, 0);
						offset.Set(0.0f, 0.0f, 0.0f);
						break;
					case CRCC(0xbae0e4da, "Multiplayer"):
						color = MAKE_RGB(255, 0, 255);
						offset.Set(1.0f, 0.0f, 0.0f);
						break;
					case CRCC(0x9d65d0e7, "Horse"):
						color = MAKE_RGB(0, 255, 255);
						offset.Set(0.0f, 1.0f, 0.0f);
						break;
					case CRCC(0xa5ad2b0b, "CTF"):
						color = MAKE_RGB(255, 0, 0);
						offset.Set(0.0f, 0.0f, 1.0f);
						break;
					case CRCC(0x3b1f59e0, "Team"):
						color = MAKE_RGB(0, 0, 255);
						offset.Set(1.0f, 1.0f, 0.0f);
						break;
					default:
						continue;
				}
			}
		}
		else if (node_class == CRCD(0xf8565321, "GenericNode"))
		{
			uint32 type;
			pNodeArray->GetStructure(i)->GetChecksum(CRCD(0x7321a8d6, "Type"), &type);
			if (type != CRCD(0xaf86421b, "Crown")) continue;
			
			color = MAKE_RGB(255, 255, 0);
			offset.Set(0.0f, 1.0f, 1.0f);
		}
		else
		{
			continue;
		}
		
		Mth::Vector pos;
		SkateScript::GetPosition(i, &pos);
		
		Gfx::AddDebugLine(pos, pos + 12.0f * offset + Mth::Vector(0.0f, 140.0f, 0.0f), color, 0, 0);
	}
	
	return true;
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace CFuncs
