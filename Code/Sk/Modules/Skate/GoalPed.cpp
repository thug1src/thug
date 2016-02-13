// goal ped!

#include <sk/modules/skate/goalmanager.h>
#include <sk/modules/skate/goal.h>

#include <sk/modules/skate/skate.h>

#include <sk/objects/skater.h>
#include <sk/objects/skaterprofile.h>
#include <sk/objects/playerprofilemanager.h>

#include <gel/assman/assman.h>
#include <gel/music/music.h>

#include <gel/object/compositeobjectmanager.h>

#include <gel/components/animationcomponent.h>

#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>

namespace Game
{

CGoalPed::CGoalPed( Script::CStruct* pParams )
{
	pParams->GetChecksum( CRCD(0x9982e501,"goal_id"), &m_goalId, Script::ASSERT );
	m_goalLogicSuspended = false;
	m_lastFam = 0;
	m_lastFamObj = 0;
	m_lastStreamId = 0;
}

CGoalPed::~CGoalPed()
{
	// nothing yet.
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/	

inline void parse_first_name( char* pFirstName, const char** ppDisplayName, int buffer_size )
{
	if ( !strstr( *ppDisplayName, " " ) )
	{		
		Dbg_MsgAssert( strlen( *ppDisplayName ) < (uint32)buffer_size, ( "buffer overflow" ) );
		strcpy( pFirstName, *ppDisplayName );
		// printf("first name has no spaces, using %s\n", pFirstName);
	}
	else
	{
		char *p_in = pFirstName;
		int length = 0;
		while ( **ppDisplayName != ' ' )
		{
			*p_in++ = **ppDisplayName;
			(*ppDisplayName)++;
			length++;
		}
		*p_in++ = '\0';
		length++;
		Dbg_MsgAssert( length < buffer_size, ( "buffer overflow" ) );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/	

const char* CGoalPed::GetProFirstName()
{	
	// get params
	Script::CStruct* pParams = GetGoalParams();
	
	// grab the pro
	const char* p_default_pro_name;
	const char* p_alternate_pro_name;
	if ( !pParams->GetText( CRCD(0x944b2900,"pro"), &p_default_pro_name ) )
	{
		// printf("no pro associated with this goal\n");
		return 0;
	}

	// check what pro the player is using
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();	
	const char* p_current_pro_name;
	p_current_pro_name = pSkaterProfile->GetDisplayName();
	// printf("the current display name is %s\n", p_current_pro_name);
	
	// only check the display name if it's a pro
	bool use_alternate = false;
	if ( pSkaterProfile->IsPro() )
	{	
		// chop off the first name
		char current_pro_first_name[128];
		parse_first_name( current_pro_first_name, &p_current_pro_name, 128 );

		Dbg_MsgAssert( strlen( current_pro_first_name ) > 0, ( "No first name found for the current pro" ) );
		// printf("the player's first name is %s\n", current_pro_first_name);
	
		// see if we should use the alternate pro
		if ( pSkaterProfile->IsPro() && ( Script::GenerateCRC( current_pro_first_name ) == Script::GenerateCRC( p_default_pro_name ) ) )
		{
			if ( pParams->GetText( CRCD(0x878ccc12,"alternate_pro"), &p_alternate_pro_name ) )
			{
				use_alternate = true;
				// printf("found an alternate name of %s\n", p_alternate_pro_name);
			}
			else
			{
				// printf("no alternate pro found for this goal\n");
				return 0;
			}
		}
	}
	
	// build the stream name
	if ( use_alternate )
		return p_alternate_pro_name;
	else
		return p_default_pro_name;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/	

inline void possibly_add_success_suffix( bool success, char* buffer, int buffer_length )
{
	if ( success )
	{
		Dbg_MsgAssert( (int)strlen( buffer ) + 8 < buffer_length, ( "buffer overflow" ) );
		sprintf( buffer, "%s_success", buffer );
	}
	// printf("done creating stream %s\n", buffer );
}

// returns true if it found a lip version of the stream, signifying
// that there exists a fam associated with the stream
bool CGoalPed::GetStreamChecksum( uint32* p_streamChecksum, int cam_anim_index, bool success, const char* p_speaker_name, bool last_anim )
{
	Script::CStruct* pGoalParams = GetGoalParams();

	const char* p_first_name = GetProFirstName();
	if ( !p_first_name )
		return 0;

	const char* p_goal_name;
	pGoalParams->GetText( CRCD(0xbfecc45b,"goal_name"), &p_goal_name, Script::ASSERT );
	
	// build stream name
	char p_stream_name[MAX_STREAM_NAME_LENGTH];
	Dbg_MsgAssert( strlen( p_first_name ) + strlen( p_goal_name ) + 1 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
	sprintf( p_stream_name, "%s_%s", p_first_name, p_goal_name );

	#ifdef	__NOPT_ASSERT__
	uint32 stream_name_length = (uint32)strlen( p_stream_name );
	#endif
	
	// get string representation of difficulty level
	const char* p_difficulty_level_string;
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	GOAL_MANAGER_DIFFICULTY_LEVEL difficulty_level = pGoalManager->GetDifficultyLevel();
	switch ( difficulty_level )
	{
		case ( GOAL_MANAGER_DIFFICULTY_LOW ):
			p_difficulty_level_string = "easy";
			break;
		case ( GOAL_MANAGER_DIFFICULTY_MEDIUM ):
			p_difficulty_level_string = "medium";
			break;
		case ( GOAL_MANAGER_DIFFICULTY_HIGH ):
			p_difficulty_level_string = "hard";
			break;
		default:
			Dbg_MsgAssert( 0, ( "Unknown difficulty level" ) );
			return 0;
			break;
	}

	// build different prefix possibilities
	// format is: <pro>_<goal_id>_<difficulty>_<camanimXX>_<speaker>_success_lip
	// where difficulty, camAnimXX, speaker, success, and lip are optional

	// figure out if we need to look for a female vo
	bool is_female_skater = false;
	if ( p_speaker_name && Script::GenerateCRC( p_speaker_name ) == CRCD(0x5b8ab877,"skater") )
	{
		Mdl::Skate * pSkate = Mdl::Skate::Instance();
		Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();	
		int male_test;
		pSkaterProfile->GetInfo()->GetInteger( CRCD(0x3f813177,"is_male"), &male_test, Script::ASSERT );
		if ( !male_test )
		{
			is_female_skater = true;
		}
	}
	
	// diff level
	char p_diff_stream_name[MAX_STREAM_NAME_LENGTH];
	Dbg_MsgAssert( stream_name_length + strlen( p_difficulty_level_string ) + 1 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
	sprintf( p_diff_stream_name, "%s_%s", p_stream_name, p_difficulty_level_string );
	#ifdef	__NOPT_ASSERT__
	uint32 diff_stream_name_length = (uint32)strlen( p_diff_stream_name );
	#endif
	
	// if we have a cam anim index...
	char p_diff_cam_stream_name[MAX_STREAM_NAME_LENGTH];
	strcpy( p_diff_cam_stream_name, "" );
	char p_cam_stream_name[MAX_STREAM_NAME_LENGTH];
	strcpy( p_cam_stream_name, "" );
	char p_cam_speaker_stream_name[MAX_STREAM_NAME_LENGTH];
	strcpy( p_cam_speaker_stream_name, "" );
	char p_diff_cam_speaker_stream_name[MAX_STREAM_NAME_LENGTH];
	strcpy( p_diff_cam_speaker_stream_name, "" );
	if ( cam_anim_index >= 0 && !last_anim )
	{	
		// diff level and cam anim
		Dbg_MsgAssert( diff_stream_name_length + 10 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
		sprintf( p_diff_cam_stream_name, "%s_camAnim%02i", p_diff_stream_name, cam_anim_index );
		possibly_add_success_suffix( success, p_diff_cam_stream_name, MAX_STREAM_NAME_LENGTH );
		
		// cam anim
		Dbg_MsgAssert( stream_name_length + 10 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
		sprintf( p_cam_stream_name, "%s_camAnim%02i", p_stream_name, cam_anim_index );
		possibly_add_success_suffix( success, p_cam_stream_name, MAX_STREAM_NAME_LENGTH );

		if ( p_speaker_name )
		{
			#ifdef	__NOPT_ASSERT__
			int speaker_name_length = strlen( p_speaker_name );
			// cam anim and speaker name
			Dbg_MsgAssert( stream_name_length + 10 + speaker_name_length + 1 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
			#endif
			
			sprintf( p_cam_speaker_stream_name, "%s_camAnim%02i_%s", p_stream_name, cam_anim_index, p_speaker_name );
			if ( is_female_skater )
			{
				Dbg_MsgAssert( strlen( p_cam_speaker_stream_name ) + 2 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
				sprintf( p_cam_speaker_stream_name, "%s_f", p_cam_speaker_stream_name );
			}
			possibly_add_success_suffix( success, p_cam_speaker_stream_name, MAX_STREAM_NAME_LENGTH );

			
			// diff level, cam anim, and speaker name
			Dbg_MsgAssert( diff_stream_name_length + 10 + speaker_name_length + 1 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
			sprintf( p_diff_cam_speaker_stream_name, "%s_camAnim%02i_%s", p_diff_stream_name, cam_anim_index, p_speaker_name );
			if ( is_female_skater )
			{
				Dbg_MsgAssert( strlen( p_diff_cam_speaker_stream_name ) + 2 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
				sprintf( p_diff_cam_speaker_stream_name, "%s_f", p_diff_cam_speaker_stream_name );
			}
			possibly_add_success_suffix( success, p_diff_cam_speaker_stream_name, MAX_STREAM_NAME_LENGTH );
		}		
	}

	char p_diff_speaker_stream_name[MAX_STREAM_NAME_LENGTH];
	strcpy( p_diff_speaker_stream_name, "" );
	char p_speaker_stream_name[MAX_STREAM_NAME_LENGTH];
	strcpy( p_speaker_stream_name, "" );
	if ( p_speaker_name )
	{
		#ifdef	__NOPT_ASSERT__
		int speaker_name_length = strlen( p_speaker_name );
		// diff level and speaker name
		Dbg_MsgAssert( diff_stream_name_length + speaker_name_length + 1 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
		#endif
		sprintf( p_diff_speaker_stream_name, "%s_%s", p_diff_stream_name, p_speaker_name );
		if ( is_female_skater )
		{
			Dbg_MsgAssert( strlen( p_diff_speaker_stream_name ) + 2 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
			sprintf( p_diff_speaker_stream_name, "%s_f", p_diff_speaker_stream_name );
		}
		possibly_add_success_suffix( success, p_diff_speaker_stream_name, MAX_STREAM_NAME_LENGTH );
	
		// speaker name
		Dbg_MsgAssert( stream_name_length + speaker_name_length + 1 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
		sprintf( p_speaker_stream_name, "%s_%s", p_stream_name, p_speaker_name );
		if ( is_female_skater )
		{
			Dbg_MsgAssert( strlen( p_speaker_stream_name ) + 2 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
			sprintf( p_speaker_stream_name, "%s_f", p_speaker_stream_name );
		}
		possibly_add_success_suffix( success, p_speaker_stream_name, MAX_STREAM_NAME_LENGTH );
	}
	
	// now that we're done using the diff stream  and stream name
	// as pieces, add the success suffix
	possibly_add_success_suffix( success, p_diff_stream_name, MAX_STREAM_NAME_LENGTH );
	possibly_add_success_suffix( success, p_stream_name, MAX_STREAM_NAME_LENGTH );
	
	// array of pointers to my streams to make the code more readable
	// 0 - p_stream_name
	// 1 - p_diff_stream_name
	// 2 - p_diff_cam_stream_name
	// 3 - p_cam_stream_name
	// 4 - p_cam_speaker_stream_name
	// 5 - p_diff_cam_speaker_stream_name
	// 6 - p_diff_speaker_stream_name
	// 7 - p_speaker_stream_name
	char* pp_stream_names[8];
	pp_stream_names[0] = p_stream_name;
	pp_stream_names[1] = p_diff_stream_name;
	pp_stream_names[2] = p_speaker_stream_name;
	pp_stream_names[3] = p_diff_speaker_stream_name;
	pp_stream_names[4] = p_diff_cam_stream_name;
	pp_stream_names[5] = p_cam_stream_name;
	pp_stream_names[6] = p_cam_speaker_stream_name;
	pp_stream_names[7] = p_diff_cam_speaker_stream_name;

	// search!
	// if there's a cam anim index defined, don't search for 
	// ones without cam anim index
	int start_index = 0;
	if ( cam_anim_index >= 0 )
	{
		start_index = 4;
	}

	// search for regular or lip version of stream
	uint32 existing_stream = 0;
	bool found_lip = false;
	for ( int i = start_index; i < 8; i++ )
	{
		// make sure there's something to check
		int stream_name_length = strlen( pp_stream_names[i] );
		if ( stream_name_length == 0 )
		{
			continue;
		}
		
		// test lip version first - it's more likely to happen
		char p_lip_stream[MAX_STREAM_NAME_LENGTH];
		Dbg_MsgAssert( (uint32)stream_name_length + 4 < MAX_STREAM_NAME_LENGTH, ( "buffer overflow" ) );
		sprintf( p_lip_stream, "%s_lip", pp_stream_names[i] );
		// printf("looking for %s\n", p_lip_stream );
		uint32 lip_stream_checksum = Script::GenerateCRC( p_lip_stream );
		if ( Pcm::StreamExists( lip_stream_checksum ) )
		{
			// printf("Existing stream: %s\n", p_lip_stream);
			existing_stream = lip_stream_checksum;
			found_lip = true;
			break;
		}
		else
		{
			uint32 stream_checksum = Script::GenerateCRC( pp_stream_names[i] );
			// printf("looking for %s\n", pp_stream_names[i]);
			if ( Pcm::StreamExists( stream_checksum ) )
			{
				// printf( "Existing stream: %s\n", pp_stream_names[i] );
				existing_stream = stream_checksum;
				break;				
			}
		}
	}
	*p_streamChecksum = existing_stream;
	return found_lip;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/	

void CGoalPed::GetPlayerFirstName( char* p_first_name, int buffer_size )
{
	// check what pro the player is using
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();	
	
	const char* p_current_pro_name;
	p_current_pro_name = pSkaterProfile->GetDisplayName();
	
	strcpy( p_first_name, "" );
	parse_first_name( p_first_name, &p_current_pro_name, buffer_size );
	Dbg_MsgAssert( strlen( p_first_name ) > 0, ( "No current player name found" ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoalPed::PlayGoalStream( uint32 stream_checksum, bool use_pos_info, uint32 speaker_obj_id, bool should_load_fam )
{
	if ( m_goalLogicSuspended )
		return;

	if ( !Pcm::StreamExists( stream_checksum ) )
		return;

	// get params
	Script::CStruct* pParams = GetGoalParams();

	// printf("PlayGoalStream: %x\n", stream_checksum );
	
	// run script and associate with trigger
	uint32 trigger_obj_id = speaker_obj_id;
	if ( !trigger_obj_id )
	{
		pParams->GetChecksum( CRCD(0x02d7e03d,"trigger_obj_id"), &trigger_obj_id, Script::ASSERT );
	}
	
	Obj::CCompositeObject* p_pos_obj = NULL;
	if ( trigger_obj_id == CRCD(0x5b8ab877,"skater") )
	{		
		Mdl::Skate* skate_mod = Mdl::Skate::Instance();
		p_pos_obj = skate_mod->GetLocalSkater();
	}
	else
	{
		//p_pos_obj = Obj::CMovingObject::m_hash_table.GetItem( trigger_obj_id );
		p_pos_obj = (Obj::CCompositeObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID( trigger_obj_id);
	}
	
	if ( p_pos_obj )
	{		
		Script::CStruct* p_tempParams = new Script::CStruct();
		p_tempParams->AddChecksum( CRCD(0x0572ea28,"stream_checksum"), stream_checksum );

		if ( !use_pos_info )
		{
			p_tempParams->AddInteger( CRCD(0xc9b0deb7,"use_pos_info"), 0 );
			
			// get the reference checksum, so that we can play anims on the correct skeleton
			// (either animload_thps5_human or animload_ped_female)
			Obj::CAnimationComponent* pAnimComponent = GetAnimationComponentFromObject( p_pos_obj );
			Dbg_Assert( pAnimComponent );
			uint32 reference_checksum = pAnimComponent->GetAnimScriptName();
	
			if ( should_load_fam )
			{
				LoadFam( stream_checksum, reference_checksum );
			}
		
			// preload the stream
			uint32 streamId = Pcm::PreLoadStream( stream_checksum, 100 );
			p_tempParams->AddChecksum( CRCD(0x8a68ab90,"streamId"), streamId );
			m_lastStreamId = streamId;
	
			// Script::KillSpawnedScriptsThatReferTo( CRCD(0x5376a011,"goal_play_stream") );
			
			// delete p_tempParams;
		
			// look for an FAM (frame amplitude) with that name
			// (needs an actual skeleton name, which we will
			// hardcode as "animload_thps5_human" for now)
			Ass::CAssMan* ass_man = Ass::CAssMan::Instance();
			if ( ass_man->GetAsset( stream_checksum + reference_checksum, false ) )
			{
				p_tempParams->AddChecksum( NONAME, CRCD(0x93516575,"play_anim") );
				m_lastFamObj = trigger_obj_id;
				
				/*Script::CStruct* pTempStruct = new Script::CStruct;
				pTempStruct->AddChecksum( CRCD(0x7321a8d6,"type"), CRCD(0x659bf355,"partialAnim") );
				pTempStruct->AddChecksum( CRCD(0x40c698af,"id"), CRCD(0xc0ba0665,"jawRotation") );
				
				// first we want to kill off any old jaw rotation anims that might be playing...
				// pAnimComponent->CallMemberFunction( CRCD(0x986d274e,"RemoveAnimController"), pTempStruct, NULL );
				// Script::RunScript( CRCD(0x986d274e,"RemoveAnimController"), pTempStruct, p_pos_obj );
																				  
				pTempStruct->AddChecksum( CRCD(0x6c2bfb7f,"animName"), stream_checksum );
				pTempStruct->AddChecksum( CRCD(0x46e55e8f,"from"), CRCD(0x6086aa70,"start") );
				pTempStruct->AddChecksum( CRCD(0x28782d3b,"to"), CRCD(0xff03cc4e,"end") );
				//pTempStruct->AddChecksum( NONAME, CRCD(0x4f792e6c,"cycle") );
				pTempStruct->AddFloat( CRCD(0xf0d90109,"speed"), 1.0f );
				// printf("adding animcontroller to %s\n", Script::FindChecksumName( trigger_obj_id ) );
				pAnimComponent->CallMemberFunction( CRCD(0x83654874,"AddAnimController"), pTempStruct, NULL );
				// Script::RunScript( CRCD(0x83654874,"AddAnimController"), pTempStruct, p_pos_obj );
				
				delete pTempStruct;*/
			}
			else
			{
				Dbg_Message( "Couldn't find FAM file '%s'...  is it preloaded?.\n", Script::FindChecksumName(stream_checksum) );
			}
		}
		Script::CScript* pSpawnedScript = Script::SpawnScript( CRCD(0x5376a011,"goal_play_stream"), p_tempParams );
		pSpawnedScript->mpObject = p_pos_obj;
		delete p_tempParams;
	}
	else
	{
		Dbg_Message( "WARNING: Could not find pro to associate with goal_play_stream" );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoalPed::PlayGoalStream( const char* type, Script::CStruct* pParams )
{
	// Garrett: This function can take over 1 ms to execute when searching for multiple streams.
	//uint64 start_time = Tmr::GetTimeInUSeconds();	

	if ( m_goalLogicSuspended )
		return;

	// check for any flags
	bool play_random = pParams->ContainsFlag( CRCD(0x2be2056a,"play_random") );
	bool call_player_by_name = pParams->ContainsFlag( CRCD(0x9f38b332,"call_player_by_name") );

	bool found_player_name_vo = false;

	if ( play_random && call_player_by_name )
	{
		Dbg_MsgAssert( 0, ("GoalManager_PlayGoalStream called with play_random and call_player_by_name flags") );
	}

	uint32 stream_checksum;
	if ( pParams->GetChecksum( CRCD(0x1a4e4fb9,"vo"), &stream_checksum, Script::NO_ASSERT ) )
	{
		// play it!
		PlayGoalStream( stream_checksum );
		//uint64 end_time = Tmr::GetTimeInUSeconds();	
		//if ((end_time - start_time) > 250)
		//	Dbg_Message("PlayGoalStream String 1 Time %d", (end_time - start_time));
		return;
	}
	// else if ( call_player_by_name )
	else if ( play_random || call_player_by_name )
	{
		// check what pro the player is using
		Mdl::Skate * pSkate = Mdl::Skate::Instance();
		Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();	

		// don't play the special vo unless they're using a pro.
		if ( pSkaterProfile->IsPro() )
		{
			//uint64 mid_time = Tmr::GetTimeInUSeconds();	

			const char* p_speaker_name = GetProFirstName();
			char p_player_name[128];
			GetPlayerFirstName( p_player_name, 128 );
			char stream_name[128];

			Script::CArray* p_stream_numbers = new Script::CArray();
			p_stream_numbers->SetSizeAndType( vMAXCALLSKATERBYNAMESTREAMS, ESYMBOLTYPE_INTEGER );
			for ( int num = 0; num < vMAXCALLSKATERBYNAMESTREAMS; num++ )
			{
				p_stream_numbers->SetInteger( num, num + 1 );   // stream numbers start at 1, not 0
			}
			for ( int i = 0; i < vMAXCALLSKATERBYNAMESTREAMS; i++ )
			{
				// grab a random index and switch with the current index
				int random_index = Mth::Rnd( vMAXCALLSKATERBYNAMESTREAMS );
				int random_value = p_stream_numbers->GetInteger( random_index );
				p_stream_numbers->SetInteger( random_index, p_stream_numbers->GetInteger( i ) );
				p_stream_numbers->SetInteger( i, random_value );
			}

			//uint64 mid2_time = Tmr::GetTimeInUSeconds();	
			// find one!
			for ( int i = 0; i < vMAXCALLSKATERBYNAMESTREAMS; i++ )
			{
				sprintf( stream_name, "%s_Far%s%02i", p_speaker_name, p_player_name, i + 1 );
				
				if ( Pcm::StreamExists( Script::GenerateCRC( stream_name ) ) )
				{
					// printf("playing targetpro vo directly: %s\n", stream_name);
					found_player_name_vo = true;
					PlayGoalStream( Script::GenerateCRC( stream_name ) );
					break;
				}				
			}
			//uint64 end_time = Tmr::GetTimeInUSeconds();	
			//if ((end_time - start_time) > 250)
			//	Dbg_Message("******** PlayGoalStream String 2 Time %d Mid %d Mid2 %d", (end_time - start_time), (mid2_time - mid_time), (end_time - mid2_time));
			
			// clean up the array
			Script::CleanUpArray( p_stream_numbers );
			delete p_stream_numbers;
		}
		// else
		if ( !found_player_name_vo )
		{
			// go ahead and play a random one
			call_player_by_name = false;
			play_random = true;
		}
	}
	
	
	if ( ( call_player_by_name || play_random ) && !found_player_name_vo )
	{
		switch ( Script::GenerateCRC( type ) )
		{
		case ( CRCC(0x82117c1a,"wait") ):
			PlayGoalWaitStream();
			break;
		// case ( CRCC(0x90ff204d,"Success") ):
			// PlayGoalWinStream();
			// break;
		case ( CRCC(0x1623b689,"Midgoal") ):
			PlayGoalMidStream();
			break;
		// case ( CRCC(0x6657a075,"Talking") ):
			// PlayGoalStartStream();
			// break;
		default:
			//Pcm::StopStreams();
			PlayGoalProStream( type, vMAXWAITSTREAMS, false );
			break;
		}
	}

	//uint64 end_time = Tmr::GetTimeInUSeconds();	
	//if ((end_time - start_time) > 250)
	//	Dbg_Message("PlayGoalStream String 3 Time %d", (end_time - start_time));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoalPed::PlayGoalStartStream( Script::CStruct* pParams )
{
	if ( m_goalLogicSuspended )
		return;

	// get goal params
	Script::CStruct* pGoalParams = GetGoalParams();
	
	// K: Added this to prevent an assert when created-goal peds try to play a non-existent stream.
	if ( pGoalParams->ContainsFlag( CRCD( 0x102a8591, "no_stream" ) ) )
	{
		return;
	}	
	
	int cam_anim_index = -1;
	pParams->GetInteger( CRCD(0x64eee2e6,"cam_anim_index"), &cam_anim_index, Script::NO_ASSERT );

	const char* p_speaker_name = NULL;
	pParams->GetString( CRCD(0x82cbe8d1,"speaker_name"), &p_speaker_name, Script::NO_ASSERT );

	bool last_anim = pParams->ContainsFlag( CRCD(0xe7f5ff8,"last_anim") );
	bool should_load_fam = false;
	uint32 stream_checksum;
	if ( GetStreamChecksum( &stream_checksum, cam_anim_index, false, p_speaker_name, last_anim ) )
	{
		// load the fam as well.
		should_load_fam = true;
	}
	
	if ( stream_checksum )
	{		
		// stop any current vo's
		StopCurrentStream();

		uint32 speaker_obj_id = 0;
		pParams->GetChecksum( CRCD(0xf0c598fa,"speaker_obj_id"), &speaker_obj_id, Script::NO_ASSERT );
		PlayGoalStream( stream_checksum, false, speaker_obj_id, should_load_fam );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalPed::LoadFam( uint32 stream_checksum, uint32 reference_checksum )
{
	UnloadLastFam();
	char p_famName[32];
	sprintf( p_famName, "fam\\%08x.fam", stream_checksum );
	// printf("loading %s\n", p_famName);
	Ass::CAssMan* ass_man =  Ass::CAssMan::Instance();
	
	uint32 old_ref_checksum = ass_man->GetReferenceChecksum();
	if ( ass_man->LoadAnim( p_famName, stream_checksum, reference_checksum, false, false ) )
	{
		// printf("loaded\n");
		m_lastFam = Script::GenerateCRC( p_famName );
	}
	ass_man->SetReferenceChecksum( old_ref_checksum );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalPed::UnloadLastFam()
{
	if ( m_lastFam )
	{		
		if ( m_lastFamObj )
		{			
			Obj::CCompositeObject* p_pos_obj = NULL;
			if ( m_lastFamObj == CRCD(0x5b8ab877,"skater") )
			{		
				// printf("getting skater\n");
				Mdl::Skate* skate_mod = Mdl::Skate::Instance();
				p_pos_obj = skate_mod->GetLocalSkater();
			}
			else
			{
				//p_pos_obj = Obj::CMovingObject::m_hash_table.GetItem( trigger_obj_id );
				// printf("getting ped\n");
				p_pos_obj = (Obj::CCompositeObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID( m_lastFamObj );
			}

			Script::StopScriptsUsingThisObject_Proper( p_pos_obj, CRCD(0x5376a011,"goal_play_stream") );
			Script::CStruct* pTempStruct = new Script::CStruct;
			pTempStruct->AddChecksum( CRCD(0x7321a8d6,"type"), CRCD(0x659bf355,"partialAnim") );
			pTempStruct->AddChecksum( CRCD(0x40c698af,"id"), CRCD(0xc0ba0665,"jawRotation") );

			// first we want to kill off any old jaw rotation anims that might be playing...		
			if ( p_pos_obj )
			{
				// printf("removing animcontroller\n");
				Obj::CAnimationComponent* pAnimComponent = GetAnimationComponentFromObject( p_pos_obj );
				Dbg_Assert( pAnimComponent );
				pAnimComponent->CallMemberFunction( CRCD(0x986d274e,"RemoveAnimController"), pTempStruct, NULL );				
			}
			delete pTempStruct;
			m_lastFamObj = 0;
		}

		// get the assman
		Ass::CAssMan * ass_man = Ass::CAssMan::Instance();

		Ass::CAsset* pAsset = ass_man->GetAssetNode( m_lastFam, false );

		if ( pAsset )
		{
			// printf("unloading fam\n");
			ass_man->DestroyReferences( pAsset );
			ass_man->UnloadAsset( pAsset );
			Script::RunScript( CRCD( 0xb3dce47f, "invalidate_anim_cache" ) );
		}
		m_lastFam = 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalPed::StopLastStream()
{
	if ( m_lastStreamId )
	{
		if ( m_lastFamObj )
		{			
			Obj::CCompositeObject* p_pos_obj = NULL;
			if ( m_lastFamObj == CRCD(0x5b8ab877,"skater") )
			{		
				Mdl::Skate* skate_mod = Mdl::Skate::Instance();
				p_pos_obj = skate_mod->GetLocalSkater();
			}
			else
			{
				p_pos_obj = (Obj::CCompositeObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID( m_lastFamObj );
			}
			Script::StopScriptsUsingThisObject_Proper( p_pos_obj, CRCD(0x5376a011,"goal_play_stream") );
		}
		Pcm::StopStreamFromID( m_lastStreamId );
		m_lastStreamId = 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalPed::PlayGoalWaitStream()
{
	if ( m_goalLogicSuspended )
		return;

	PlayGoalProStream( "Far", vMAXWAITSTREAMS, true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalPed::PlayGoalWinStream( Script::CStruct* pParams )
{
	if ( m_goalLogicSuspended )
		return;

	// get params
	const char* p_speaker_name = NULL;
	pParams->GetString( CRCD(0x82cbe8d1,"speaker_name"), &p_speaker_name, Script::NO_ASSERT );
	// int cam_anim_index = -1;
	// pParams->GetInteger( CRCD(0x64eee2e6,"cam_anim_index"), &cam_anim_index, Script:::NO_ASSERT );
	// bool last_anim = pParams->ContainsFlag( CRCD(0xe7f5ff8,"last_anim") );

	uint32 stream_checksum;
	bool should_load_fam = false;

	if ( GetStreamChecksum( &stream_checksum, -1, true, p_speaker_name, true ) )
	{
		should_load_fam = true;
	}

	uint32 speaker_obj_id = 0;
	pParams->GetChecksum( CRCD(0xf0c598fa,"speaker_obj_id"), &speaker_obj_id, Script::NO_ASSERT );
	
	if ( stream_checksum )
		PlayGoalStream( stream_checksum, false, speaker_obj_id, should_load_fam );
	// else
		// PlayGoalProStream( "Success", vMAXWINSTREAMS, false, false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalPed::PlayGoalMidStream()
{
	if ( m_goalLogicSuspended )
		return;
	PlayGoalProStream( "Midgoal", vMAXMIDGOALSTREAMS, false );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalPed::PlayGoalProStream( const char* p_stream_type, int max_number_streams, bool use_player_name, bool use_pos_info )
{
	Dbg_MsgAssert( p_stream_type, ("PlayGoalProStream called without stream type") );
	Dbg_MsgAssert( max_number_streams, ("PlayGoalProStream called without stream type") );
	
	if ( m_goalLogicSuspended )
		return;
	
	// stop any current vo's
	StopCurrentStream();
	
	// get the first name of the speaker and the player
	const char* p_speaker_name = GetProFirstName();
	if ( !p_speaker_name )
		return;
	
	// check what pro the player is using
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();	
	
	const char* p_current_pro_name;
	p_current_pro_name = pSkaterProfile->GetDisplayName();
	
	char p_player_name[128];
	parse_first_name( p_player_name, &p_current_pro_name, 128 );
	Dbg_MsgAssert( strlen( p_player_name ) > 0, ( "No player name found" ) );
	// printf("got a first name of %s\n", p_player_name);
	
	// create an array of all possible stream numbers
	int num_possible_streams = max_number_streams;

	if ( use_player_name )
		// there are x wait streams plus 20 possible streams directed specifically at the player ("Hey, tony!")
		num_possible_streams += 20;

	Script::CArray* p_stream_numbers = new Script::CArray();
	p_stream_numbers->SetSizeAndType( num_possible_streams, ESYMBOLTYPE_INTEGER );
	for ( int num = 0; num < num_possible_streams; num++ )
	{
		p_stream_numbers->SetInteger( num, num + 1 );   // stream numbers start at 1, not 0
	}
	for ( int i = 0; i < num_possible_streams; i++ )
	{
		// grab a random index and switch with the current index
		int random_index = Mth::Rnd( num_possible_streams );
		int random_value = p_stream_numbers->GetInteger( random_index );
		p_stream_numbers->SetInteger( random_index, p_stream_numbers->GetInteger( i ) );
		p_stream_numbers->SetInteger( i, random_value );
	}

	// go through until we find the first valid stream
	// uint32 current_vo_stream = 0;
	for ( int i = 0; i < num_possible_streams; i++ )
	{
		int stream_number = p_stream_numbers->GetInteger( i );
		// printf( "got a stream number of %i\n", stream_number );
		char stream_name[128];

		// check if this is the special "hey tony" type stream number
		if ( ( stream_number > max_number_streams ) )
		{
			// don't play the special vo unless they're using a pro.
			if ( pSkaterProfile->IsPro() )
			{
				// offset the stream number.
				stream_number -= max_number_streams;
				sprintf( stream_name, "%s_%s%s%02i", p_speaker_name, p_stream_type, p_player_name, stream_number );
				// printf("figured a special vo of %s\n", stream_name);
				if ( Pcm::StreamExists( Script::GenerateCRC( stream_name ) ) )
				{
					PlayGoalStream( Script::GenerateCRC( stream_name ), use_pos_info );
					// current_vo_stream = Pcm::PlayStream( Script::GenerateCRC( stream_name ), 100, 100 );
					break;
				}
			}
		}
		else
		{
			sprintf( stream_name, "%s_%s%02i", p_speaker_name, p_stream_type, stream_number );
			// printf("figured vo: %s\n", stream_name);
			if ( Pcm::StreamExists( Script::GenerateCRC( stream_name ) ) )
			{
				PlayGoalStream( Script::GenerateCRC( stream_name ), use_pos_info );
				// current_vo_stream = Pcm::PlayStream( Script::GenerateCRC( stream_name ), 100, 100 );
				break;
			}
		}
	}
	
	Script::CleanUpArray( p_stream_numbers );
	delete p_stream_numbers;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoalPed::StopCurrentStream()
{
	// get params
	Script::CStruct* pParams = GetGoalParams();

	uint32 current_vo_stream = 0;
	if ( pParams->GetChecksum( CRCD(0xdc14d454,"current_vo_stream"), &current_vo_stream, Script::NO_ASSERT ) )
	{
		pParams->RemoveComponent( CRCD(0xdc14d454,"current_vo_stream") );
		if ( current_vo_stream )
			Pcm::StopStreamFromID( current_vo_stream );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoalPed::ProIsCurrentSkater()
{
	// get params
	Script::CStruct* pParams = GetGoalParams();
	
	// are they using the right skater?
	Script::CStruct* p_last_names = Script::GetStructure( CRCD(0x621d1828,"goal_pro_last_name_checksums"), Script::ASSERT );
	const char* first_name_string;
	pParams->GetString( CRCD(0x4a91eceb,"pro_challenge_pro_name"), &first_name_string, Script::ASSERT );
	uint32 last_name;
	p_last_names->GetChecksum( Script::GenerateCRC( first_name_string ), &last_name, Script::ASSERT );
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CPlayerProfileManager* pProfileManager = skate_mod->GetPlayerProfileManager();
	Obj::CSkaterProfile* pSkaterProfile = pProfileManager->GetCurrentProfile();
	uint32 current_skater = pSkaterProfile->GetSkaterNameChecksum();

	return ( current_skater == last_name );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CGoalPed::GetGoalAnimations( const char* type )
{
	// get params
	Script::CStruct* pParams = GetGoalParams();
	
	uint32 array_name = 0;
	
	// check for specific goal animation
	char goal_specific_array_name[128];
	const char* p_goal_name;
	pParams->GetString( CRCD(0xbfecc45b,"goal_name"), &p_goal_name, Script::ASSERT );
	sprintf( goal_specific_array_name, "%s_%s", p_goal_name, type );
	if ( ( Script::GetArray( goal_specific_array_name, Script::NO_ASSERT ) ) )
	{
		// printf("got a specific array name of %s\n", p_goal_specific_array_name );
		array_name = Script::GenerateCRC( goal_specific_array_name );
	}
	else if ( type )
	{
		// build the name of the pro's generic anims array
		const char* p_pro_name = GetProFirstName();
		char pro_array_name[128];
		sprintf( pro_array_name, "%s_goal_%s", p_pro_name, type );

		if ( ( Script::GetArray( pro_array_name, Script::NO_ASSERT ) ) )
		{
			// pro's generic array
			// printf("got generic pro array: %s\n", pro_array_name );
			array_name = Script::GenerateCRC( pro_array_name );
		}
		else
		{
			// get the generic anim array
			char generic_array_name[128];
			sprintf( generic_array_name, "generic_pro_anims_%s", type );
			// printf("looking for array: %s\n", generic_array_name);
			if ( !Script::GetArray( generic_array_name, Script::NO_ASSERT ) )
			{
				 Dbg_MsgAssert( 0, ("Unable to find generic pro array '%s'",generic_array_name) );
			}
			array_name = Script::GenerateCRC( generic_array_name );
		}
	}
	return array_name;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalPed::DestroyGoalPed()
{
	// get params
	Script::CStruct* pParams = GetGoalParams();
	
	uint32 trigger_obj_id = 0;

	pParams->GetChecksum( CRCD(0x02d7e03d,"trigger_obj_id"), &trigger_obj_id );
	if( trigger_obj_id )
	{
		Obj::CCompositeObject* p_pos_obj = NULL;
		//p_pos_obj = Obj::CMovingObject::m_hash_table.GetItem( trigger_obj_id );
		p_pos_obj = (Obj::CCompositeObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID( trigger_obj_id);
		
		if( p_pos_obj )
		{
			delete p_pos_obj;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalPed::SuspendGoalLogic( bool suspend )
{
	m_goalLogicSuspended = suspend;

	if ( suspend )
	{
		Script::RunScript( CRCD(0xbf67a3cf,"goal_pro_stop_anim_scripts"), GetGoalParams() );
		StopCurrentStream();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalPed::Hide( bool hide )
{
	// get params
	Script::CStruct* pParams = GetGoalParams();
	
	if ( pParams->ContainsFlag( CRCD( 0x3d1cab0b, "null_goal" ) ) )
	{
		return;
	}
	
	uint32 trigger_obj_id = 0;

	pParams->GetChecksum( CRCD(0x02d7e03d,"trigger_obj_id"), &trigger_obj_id );
	if ( trigger_obj_id )
	{
		Obj::CCompositeObject* p_pos_obj = NULL;
		//p_pos_obj = Obj::CMovingObject::m_hash_table.GetItem( trigger_obj_id );
		p_pos_obj = (Obj::CCompositeObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID( trigger_obj_id );
		
		if ( p_pos_obj )
		{
			p_pos_obj->Hide( hide );
		}

		// kill/create arrow
		Script::CStruct* pTempParams = new Script::CStruct();
		pTempParams->AddChecksum( CRCD(0x9982e501,"goal_id"), m_goalId );
		if ( hide )
			Script::RunScript( CRCD(0x6d39f2f3,"goal_ped_kill_arrow"), pTempParams );
		else
		{
			// check if this goal has been beaten...hack for mike v special goal
			CGoalManager* pGoalManager = GetGoalManager();
			Dbg_Assert( pGoalManager );
			CGoal* pGoal = pGoalManager->GetGoal( m_goalId, false );
			bool should_add_arrow = true;
			if ( pGoal && pGoal->HasWonGoal() )
				should_add_arrow = false;
			
			if ( should_add_arrow )
				Script::RunScript( CRCD(0x6ea4d309,"goal_add_ped_arrow"), pTempParams );
		}
		delete pTempParams;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* CGoalPed::GetGoalParams()
{
	CGoalManager* pGoalManager = Game::GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->GetGoalParams( m_goalId );
}

}	// namespace Game
