// skatetris goal
#include <sk/modules/skate/GoalManager.h>
#include <sk/modules/skate/SkatetrisGoal.h>
#include <sk/modules/skate/skate.h>
#include <sk/gamenet/gamenet.h>

#include <gel/scripting/script.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/symboltype.h>

#include <gfx/2D/ScreenElemMan.h>       // for tetris tricks
#include <gfx/2D/ScreenElement2.h>
#include <gfx/2D/TextElement.h>
#include <gfx/2D/SpriteElement.h>

#include <sk/objects/skater.h>
#include <sk/objects/skaterprofile.h>

namespace Game
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkatetrisGoal::CSkatetrisGoal( Script::CStruct* pParams ) : CGoal( pParams )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkatetrisGoal::~CSkatetrisGoal()
{
//	delete[] m_tetrisTricks;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkatetrisGoal::Activate()
{
	if ( !IsActive() )
	{
		StartTetrisGoal();
		return CGoal::Activate();
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkatetrisGoal::Deactivate( bool force, bool affect_tree )
{
	// printf("CSkatetrisGoal::Deactivate\n");
	EndTetrisGoal();
	return CGoal::Deactivate( force, affect_tree );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkatetrisGoal::Win()
{
	// the win function is called after time runs out, 
	// so deactivate must always be called

	// K: TT12058, Deactivate may remove the testing_goal flag, so store it and put it back in
	// afterwards if necessary. (The flag is required by the call to CGoal::Win below)
	bool testing_goal=mp_params->ContainsFlag(CRCD(0x54d3cac1,"testing_goal"));
	
	mp_params->AddChecksum( NONAME, CRCD( 0xc309cad1, "just_won_goal" ) );
	Deactivate();
	mp_params->RemoveFlag( CRCD( 0xc309cad1, "just_won_goal" ) );
	
	if (testing_goal)
	{
		mp_params->AddChecksum(NONAME,CRCD(0x54d3cac1,"testing_goal"));
	}
		
	/*if( gamenet_man->InNetGame())
	{
		CGoalManager* pGoalManager = GetGoalManager();
        Dbg_Assert( pGoalManager );
        return pGoalManager->WinGoal( GetGoalId());
	}
	else*/
	{
		bool ret_val=CGoal::Win();
		// Make sure the testing_goal flag is definitely gone.
		mp_params->RemoveFlag(CRCD(0x54d3cac1,"testing_goal"));
		return ret_val;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkatetrisGoal::Update()
{
	int wait_to_add_tricks = 0;
	mp_params->GetInteger( CRCD(0x7824963c,"wait_to_add_tricks"), &wait_to_add_tricks, Script::NO_ASSERT );
	if ( wait_to_add_tricks )
	{
		// bail out...
		return CGoal::Update();
	}

	int minTimeLeft = 1;
	if ( !IsSingleComboSkatetris() )
	{
		if ( mp_params->GetInteger( CRCD(0xa6d96603,"time_to_stop_adding_tricks"), &minTimeLeft, Script::NO_ASSERT ) )
			minTimeLeft *= 1000;
	}
	
	// check for win conditions
	if ( IsTricktris() )
	{
		if ( IsActive() && !IsPaused() )
		{
			// tricktris is won when you've completed the pre-determined number
			// of tricks
			int tricks_completed = 0;
			mp_params->GetInteger( CRCD(0x8d958f01,"tricks_completed"), &tricks_completed, Script::NO_ASSERT );
			int total_to_win;
			mp_params->GetInteger( CRCD(0xbfd3a831,"tricktris_total_to_win"), &total_to_win, Script::ASSERT );
			// printf("tricks_completed = %i, total_to_win = %i\n", tricks_completed, total_to_win);
			if ( tricks_completed >= total_to_win )
			{
				CGoalManager* pGoalManager = GetGoalManager();
				pGoalManager->WinGoal( GetGoalId() );
				return true;			
			}
		}
	}
	else if ( IsActive() && (int)m_timeLeft < minTimeLeft && !IsPaused() && AllTricksCleared() )
	{
		// win the goal if they've cleared all tricks and there's no time to add more
		CGoalManager* pGoalManager = GetGoalManager();
		pGoalManager->WinGoal( GetGoalId() );
		return true;
	}
	
	// update faded tricks if we're active - even if the time has run out
	if ( IsActive() && !IsPaused() )
	{
		UpdateFadedTricks();
	}
	
	if ( IsTricktris() )
	{
		if ( ( m_unlimitedTime || (int)m_timeLeft > 0.0f ) && !IsPaused() )
		{
			int tricks_completed = 0;
			mp_params->GetInteger( CRCD(0x8d958f01,"tricks_completed"), &tricks_completed, Script::NO_ASSERT );
			int total_to_win;
			mp_params->GetInteger( CRCD(0xbfd3a831,"tricktris_total_to_win"), &total_to_win, Script::ASSERT );
	
			// count number of valid entries and add if necessary
			int total_in_stack = 0;
			for ( int i = 0; i < Game::vMAXTETRISTRICKS; i++ )
			{
				if ( m_tetrisTricks[i].invalid == false )
					total_in_stack++;
			}
			int desired_stack_size;
			mp_params->GetInteger( CRCD(0x45a375c9,"tricktris_block_size"), &desired_stack_size, Script::ASSERT );
			if ( total_in_stack < desired_stack_size && total_in_stack + tricks_completed < total_to_win )
			{
				int num_to_add = desired_stack_size - total_in_stack;
				if ( total_to_win - tricks_completed - total_in_stack < num_to_add )
					num_to_add = total_to_win - tricks_completed - total_in_stack;
				AddTetrisTrick( num_to_add );
			}
		}
	}
	else if ( IsActive() && ( m_unlimitedTime || (int)m_timeLeft >= minTimeLeft ) && !IsPaused() )
	{
		m_tetrisTime += (int)( Tmr::FrameLength() * 1000 );

		if ( IsSingleComboSkatetris() )
		{
			// throw in a delay so the single combo doesn't appear right away
			// during the goal intro movie
			if ( !mp_params->ContainsFlag( CRCD(0xc93da683,"single_combo_added") ) && m_tetrisTime > 50 )
			{
				int combo_size;
				mp_params->GetInteger( CRCD(0x67dd90ca,"combo_size"), &combo_size, Script::ASSERT );
				AddTetrisTrick( combo_size );
				mp_params->AddChecksum( NONAME, CRCD(0xc93da683,"single_combo_added") );
			}
		}
		else
		{
			int trickTime;
			mp_params->GetInteger( CRCD(0x648d8d2a,"adjusted_trick_time"), &trickTime, Script::ASSERT );
			if ( m_tetrisTime > trickTime )
			{
				m_tetrisTime = 0;
				if ( IsComboSkatetris() )
				{
					int combo_size;
					mp_params->GetInteger( CRCD(0x67dd90ca,"combo_size"), &combo_size, Script::ASSERT );
					AddTetrisTrick( combo_size );
				}
				else
					AddTetrisTrick();
			}
		}
	}
	return CGoal::Update();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkatetrisGoal::StartTetrisGoal()
{
    m_numTetrisTricks = 0;
    m_tetrisTime = 0;
    int original_time;
    mp_params->GetInteger( CRCD( 0xded8540a, "trick_time" ), &original_time, Script::ASSERT );
    mp_params->AddInteger( CRCD( 0x648d8d2a, "adjusted_trick_time" ), original_time );
	mp_params->AddInteger( CRCD( 0x8d958f01, "tricks_completed" ), 0 );

	// start almost out of time so a trick gets added right away
	if ( IsSingleComboSkatetris() )
		m_tetrisTime = 0;
	else
		m_tetrisTime = original_time - 200;
    
    for ( int i = 0; i < Game::vMAXTETRISTRICKS; i++ )
    {
        m_tetrisTricks[i].invalid = true;
        m_tetrisTricks[i].trickNameChecksum = 0;
		m_tetrisTricks[i].altTrickNameChecksum = 0;
		m_tetrisTricks[i].keyCombo = 0;
		m_tetrisTricks[i].spin_mult = 0;
		m_tetrisTricks[i].num_taps = 1;
		m_tetrisTricks[i].require_perfect = false;

		m_validGroups[i] = false;
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkatetrisGoal::EndTetrisGoal()
{
    m_numTetrisTricks = 0;
    m_tetrisTime = 0;
    
    for ( int i = 0; i < Game::vMAXTETRISTRICKS; i++ )
    {
        m_tetrisTricks[i].invalid = false;
        m_tetrisTricks[i].trickNameChecksum = 0;
		m_tetrisTricks[i].altTrickNameChecksum = 0;
		m_tetrisTricks[i].keyCombo = 0;
		m_tetrisTricks[i].spin_mult = 0;
		m_tetrisTricks[i].num_taps = 1;
		m_tetrisTricks[i].require_perfect = false;

		m_validGroups[i] = false;
    }

	mp_params->RemoveFlag( CRCD(0xc93da683,"single_combo_added") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkatetrisGoal::AddTetrisTrick( int num_to_add )
{
    int max_tricks;
	mp_params->GetInteger( CRCD(0x89473db7,"max_tricks"), &max_tricks, Script::ASSERT );
	
	int tricks_in_group = num_to_add;
	int possible_num_tricks = m_numTetrisTricks + tricks_in_group;
    
	bool add_combo = false;
	if ( IsComboSkatetris() || IsSingleComboSkatetris() )
		add_combo = true;
	
/*	if ( add_combo )
	{
		mp_params->GetInteger( CRCD(0x67dd90ca,"combo_size"), &tricks_in_group, Script::ASSERT );
		possible_num_tricks += tricks_in_group;
	}
	else
	{
		possible_num_tricks++;
	}
*/
	
	// check that they haven't lost
	if ( possible_num_tricks > max_tricks )
    {
        uint32 goalId = GetGoalId();
        Dbg_Assert( goalId );
        CGoalManager* pGoalManager = GetGoalManager();
        Dbg_Assert( pGoalManager );
        pGoalManager->LoseGoal( goalId );
    }
    else 
    {
		// set the group id and increment number of trick groups
		int group_id = 0;
		if ( add_combo )
		{
			for ( int i = 0; i < Game::vMAXTETRISTRICKS; i++ )
			{
				if ( !m_validGroups[i] )
				{
					group_id = i;
					m_validGroups[group_id] = true;
					break;
				}
			}
			Dbg_Assert( !( group_id == Game::vMAXTETRISTRICKS ) );
		}		
		
		// add as many tricks as we need (1 by default)
		while ( tricks_in_group > 0 )
		{
			Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();
			
			// grab list of valid key combos
			Script::CArray* p_keyCombos = NULL;
			mp_params->GetArray( CRCD(0x7be1e689,"goal_tetris_key_combos"), &p_keyCombos, Script::NO_ASSERT );

			uint32 key_combo = 0;
			uint32 trick_name_checksum = 0;
			char trick_text[Game::NEW_STRING_LENGTH];
			
			// find the first empty trick
			int index = 0;

			for ( int i = 0; i < Game::vMAXTETRISTRICKS; i++ )
			{
				if ( m_tetrisTricks[i].invalid == true )
				{
					index = i;
					m_tetrisTricks[index].invalid = false;
					m_tetrisTricks[index].group_id = group_id;
					m_tetrisTricks[index].keyCombo = 0;
					m_tetrisTricks[index].trickNameChecksum = 0;
					m_tetrisTricks[index].altTrickNameChecksum = 0;
					m_tetrisTricks[index].spin_mult = 0;
					m_tetrisTricks[index].num_taps = 1;
					m_tetrisTricks[index].require_perfect = false;
					break;
				}
			}
			Dbg_Assert( !( index == Game::vMAXTETRISTRICKS ) );
			
			// check the type of the structure...this will tell us if this is a standard
			// skatetris goal (array of key combos), or a special one (array of structures
			// containing the name and text of the trick).
			const char* p_key_combo_text = NULL;
			int num_taps = 1;
			if ( !p_keyCombos )
			{
				// check for a trick array
				Script::CArray* p_tetris_tricks = NULL;
				mp_params->GetArray( CRCD(0xeb79fb49,"goal_tetris_tricks"), &p_tetris_tricks, Script::ASSERT );
				
				// grab a random trick
				// no safety checks, since we don't care if the trick is mapped
				Script::CStruct* p_trick = p_tetris_tricks->GetStructure( Mth::Rnd( p_tetris_tricks->GetSize() ) );
				const char* p_trick_display_text;
				p_trick->GetString( CRCD(0x270f56e1,"trick"), &p_trick_display_text, Script::ASSERT );
				
				if ( p_trick->ContainsFlag( CRCD( 0x255ed86f, "grind" ) ) )
				{
					// store FS and BS versions of the trick
					char grind_string[128];
					strcpy( grind_string, "FS " );
					strcat( grind_string, p_trick_display_text );
					m_tetrisTricks[index].trickNameChecksum = Script::GenerateCRC( grind_string );

					strcpy( grind_string, "BS " );
					strcat( grind_string, p_trick_display_text );
					m_tetrisTricks[index].altTrickNameChecksum = Script::GenerateCRC( grind_string );
				}
				else
				{
					m_tetrisTricks[index].trickNameChecksum = Script::GenerateCRC( p_trick_display_text );
				}

				const char* p_text;
				p_trick->GetString( CRCD(0xc4745838,"text"), &p_text, Script::ASSERT );
				strcpy( trick_text, p_text );

				if ( !p_trick->GetChecksum( CRCD(0xacfdb27a,"key_combo"), &key_combo, Script::NO_ASSERT ) )
					p_trick->GetString( CRCD(0xacfdb27a,"key_combo"), &p_key_combo_text, Script::NO_ASSERT );

				// printf("setting trick checksum: %s\n", Script::FindChecksumName( trick_name_checksum ) );
			}
			else
			{
				// see if this is a valid key_combo
				int key_combo_index = GetRandomIndexFromKeyCombos( p_keyCombos );
				if ( key_combo_index == -1 )
				{
					// couldn't find one - the goal is going to be decativated.
					return;
				}
				if ( p_keyCombos->GetType() == ESYMBOLTYPE_NAME )
				{
					key_combo = p_keyCombos->GetChecksum( key_combo_index );
				}
				else
				{
					Dbg_MsgAssert( p_keyCombos->GetType() == ESYMBOLTYPE_STRUCTURE, ( "goal_tetris_key_combos array has bad type" ) );
					Script::CStruct* p_subStruct = p_keyCombos->GetStructure( key_combo_index );
					p_subStruct->GetChecksum( CRCD(0xacfdb27a,"key_combo"), &key_combo, Script::ASSERT );
					
					int spin_mult = 0;
					if ( p_subStruct->GetInteger( CRCD(0x8c8abd19,"random_spin"), &spin_mult, Script::NO_ASSERT ) )
					{
						// make sure it's a multiple of 180
						Dbg_MsgAssert( spin_mult % 180 == 0, ( "Skatetris goal %s has bad spin_mult value %i", Script::FindChecksumName( GetGoalId() ), spin_mult ) );
						if (spin_mult < 360)
						{
							spin_mult=360;
						}
						
						// K: Make it so that spins of 180 are excluded, since they are too hard to do on vert.
						// Ie:
						// spin_mult=360 gives spins of 0, or 360
						// spin_mult=540 gives spins of 0, 360 or 540
						// spin_mult=720 gives spins of 0, 360, 540 or 720
						// etc ...
						m_tetrisTricks[index].spin_mult = Mth::Rnd(spin_mult / 180) + 1;
						if (m_tetrisTricks[index].spin_mult == 1)
						{
							m_tetrisTricks[index].spin_mult=0;
						}	
					}
					if ( p_subStruct->GetInteger( CRCD(0xedf5db70,"spin"), &spin_mult, Script::NO_ASSERT ) )
					{
						// make sure it's a multiple of 180
						Dbg_MsgAssert( spin_mult % 180 == 0, ( "Skatetris goal %s has bad spin_mult value %i", Script::FindChecksumName( GetGoalId() ), spin_mult ) );
						m_tetrisTricks[index].spin_mult = spin_mult / 180;
					}

					if ( p_subStruct->ContainsFlag( CRCD(0x1c39f1b9,"perfect") ) )
						m_tetrisTricks[index].require_perfect = true;

					if ( p_subStruct->GetInteger( CRCD(0xa4bee6a1,"num_taps"), &num_taps, Script::NO_ASSERT ) )
						m_tetrisTricks[index].num_taps = num_taps;
				}
				
				// get the global trick mapping
				Mdl::Skate* skate_mod = Mdl::Skate::Instance();
				Obj::CSkaterProfile* p_SkaterProfile = skate_mod->GetCurrentProfile();
				Script::CStruct* p_trickMappings = p_SkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );
		
				const char* p_text = NULL;
				
				// if it's not a regular assignment, grab cat info
				if ( !p_trickMappings->GetChecksum( key_combo, &trick_name_checksum, Script::NO_ASSERT ) )
				{
					int cat_index;
					p_trickMappings->GetInteger( key_combo, &cat_index, Script::ASSERT );
					Dbg_Assert( cat_index >= 0 && cat_index < vMAX_CREATED_TRICKS );
					Obj::CSkater* pSkater = skate_mod->GetLocalSkater();
					if ( pSkater )
					{
						Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[cat_index];
						pCreatedTrick->mp_other_params->GetString( CRCD( 0xa1dc81f9, "name" ), &p_text, Script::ASSERT );
						
						// mark it
						// no cat tricks have double versions!
						m_tetrisTricks[index].num_taps = 1;
					}					
				}
	
				if ( !p_text )
				{
					Script::CStruct* p_trick_structure = NULL;
					p_trick_structure = Script::GetStructure( trick_name_checksum, Script::NO_ASSERT );
					if ( !p_trick_structure )
					{
						Script::CArray* p_trick_array = Script::GetArray( trick_name_checksum, Script::ASSERT );
						p_trick_structure = p_trick_array->GetStructure( 0 );
		
						Dbg_MsgAssert( p_trick_structure, ( "Could not find trick structure" ) );
					}
					Script::CStruct* p_trick_params;
					p_trick_structure->GetStructure( CRCD(0x7031f10c,"Params"), &p_trick_params, Script::ASSERT );
					
					
					// get any multiple tap info
					while ( num_taps > 1 )
					{
						num_taps--;
	
						uint32 extra_trick;
						if ( p_trick_params->GetChecksum( CRCD(0x6e855102,"ExtraTricks"), &extra_trick, Script::NO_ASSERT ) )
						{
							Script::CArray* p_extra_trick_array = Script::GetArray( extra_trick, Script::ASSERT );
							Dbg_MsgAssert( p_extra_trick_array->GetType() == ESYMBOLTYPE_STRUCTURE, ( "ExtraTricks array %s has wrong type", Script::FindChecksumName( extra_trick ) ) );
							Script::CStruct* p_extra_trick_struct = p_extra_trick_array->GetStructure( 0 );
							p_extra_trick_struct->GetStructure( CRCD(0x7031f10c,"Params"), &p_trick_params, Script::ASSERT );
						}
						else
						{
							num_taps = 1;
							m_tetrisTricks[index].num_taps = 1;
						}
					}
					
					p_trick_params->GetLocalText( CRCD(0xa1dc81f9,"Name"), &p_text, Script::ASSERT );
				}

				// build the text string
				strcpy( trick_text, "" );

				// spin value
				if ( m_tetrisTricks[index].spin_mult != 0 )
				{
					// perfect modifier
					if ( m_tetrisTricks[index].require_perfect )
						sprintf( trick_text, "Perfect " );

					sprintf( trick_text, "\\c5%i\\c0 ", ( m_tetrisTricks[index].spin_mult * 180 ) );
				}
				// trick text
				strcat( trick_text, p_text );
				
				// store key combo
				m_tetrisTricks[index].keyCombo = key_combo;
			}
			
			// create the container for this element
			Script::CStruct *p_container_params = new Script::CStruct();
			Front::CContainerElement *p_container = new Front::CContainerElement();
			uint32 container_id = Obj::CBaseManager::vNO_OBJECT_ID;
			p_container->SetID( container_id );
			p_screen_elem_man->RegisterObject( *p_container );

			// figure the dims
			int y_dim = 20;
			if ( add_combo && tricks_in_group == 1 )
			{
				y_dim *= 2;
			}
			p_container_params->AddPair( CRCD(0x34a68574,"dims"), 100, y_dim );

			p_container->SetProperties( p_container_params );
			
			// set the parent
			Front::CScreenElement* p_vmenu = p_screen_elem_man->GetElement(CRCD(0xe35ec715,"tetris_tricks_menu"), Front::CScreenElementManager::ASSERT );
			p_vmenu->SetChildLockState( Front::CScreenElement::UNLOCK );
			p_screen_elem_man->SetParent( p_vmenu, p_container );

			delete p_container_params;						
			
			// create text name element
			Script::CStruct *p_elem_params = new Script::CStruct();
			p_elem_params->AddString( CRCD(0xc4745838,"text"), trick_text );
			p_elem_params->AddChecksum( CRCD(0x2f6bf72d,"font"), CRCD( 0xbc19c379, "newtrickfont" ) );
			p_elem_params->AddChecksum( NONAME, CRCD( 0x1d944426, "not_focusable" ) );
			uint32 id = Obj::CBaseManager::vNO_OBJECT_ID;
	
			Front::CTextElement* p_new_element = new Front::CTextElement();
			p_new_element->SetID( id );
			p_screen_elem_man->RegisterObject( *p_new_element );
			
			p_new_element->SetProperties( p_elem_params );
	
			// set the parent
			p_screen_elem_man->SetParent( p_container, p_new_element );
			
			delete p_elem_params;
			
			
			// create button combo element
			if ( p_key_combo_text || key_combo )
			{
				Script::CStruct* p_button_combos;
				if ( m_tetrisTricks[index].num_taps == 2 )
					p_button_combos = Script::GetStructure( CRCD(0xe7abfb95,"goal_tetris_trick_text_double_tap"), Script::ASSERT );
				else
					p_button_combos = Script::GetStructure( CRCD(0x8856c817,"goal_tetris_trick_text"), Script::ASSERT );

				if ( !p_key_combo_text && !p_button_combos->GetString( key_combo, &p_key_combo_text, Script::NO_ASSERT ) )
				{
					Dbg_MsgAssert( 0, ( "Could not find button text for skatetris trick" ) );
				}

				Script::CStruct *p_button_params = new Script::CStruct();
				p_button_params->AddString( CRCD(0xc4745838,"text"), p_key_combo_text );
				p_button_params->AddChecksum( CRCD(0x2f6bf72d,"font"), CRCD( 0x8aba15ec, "small" ) );
				p_button_params->AddChecksum( NONAME, CRCD( 0x1d944426, "not_focusable" ) );
				uint32 button_id = Obj::CBaseManager::vNO_OBJECT_ID;
		
				Front::CTextElement* p_button_element = new Front::CTextElement();
				p_button_element->SetID( button_id );
				p_screen_elem_man->RegisterObject( *p_button_element );
				
				p_button_element->SetProperties( p_button_params );
				
				// the button is a child of the container
				p_screen_elem_man->SetParent( p_container, p_button_element );
		
				delete p_button_params;
			}
	
			// store the id
			m_tetrisTricks[index].screenElementId = p_container->GetID();
			m_numTetrisTricks++;
	
			// run entrance animation
			Script::CStruct* p_script_params = new Script::CStruct();
			if ( !p_key_combo_text )
				p_script_params->AddChecksum( NONAME, CRCD( 0x7b716096, "no_key_combo" ) );
			uint32 script = CRCD(0x668cc9e3,"goal_tetris_add_trick");

			if ( !IsTricktris() && !IsSingleComboSkatetris() && m_numTetrisTricks > ( .75 * (float)max_tricks ) )
				script = CRCD(0x8e68afc2,"goal_tetris_add_red_trick");
	
			Script::CScript *p_new_script = Script::SpawnScript( script, p_script_params );
			#ifdef __NOPT_ASSERT__
			p_new_script->SetCommentString("Spawned from CSkatetrisGoal::AddTetrisTrick()");
			#endif
			p_new_script->mpObject = p_container;
			// normally, script won't be updated until next frame -- we want it NOW, motherfucker
			p_new_script->Update();
			delete p_script_params;
	
			// turn the elements red if you're over 75% of the way to the top,
			// but not if we're doing a single combo
			if ( !IsTricktris() && !IsSingleComboSkatetris() && m_numTetrisTricks > ( .75 * (float)max_tricks ) )
			{
				mp_params->AddInteger( CRCD(0xd02ac196,"list_is_red"), 1 );
				
				for ( int j = 0; j < Game::vMAXTETRISTRICKS; j++ )
				{
					if ( !m_tetrisTricks[j].invalid )
					{
						Front::CScreenElement* p_trick = p_screen_elem_man->GetElement( m_tetrisTricks[j].screenElementId , Front::CScreenElementManager::ASSERT );
						script = CRCD(0xeb016d1e,"goal_tetris_turn_trick_red");
						Script::CScript* p_trick_script = Script::SpawnScript( script, NULL );
						#ifdef __NOPT_ASSERT__
						p_trick_script->SetCommentString("Spawned from CSkatetrisGoal::AddTetrisTrick()");
						#endif
						p_trick_script->mpObject = p_trick;
						p_trick_script->Update();
					}
				}
			}
			tricks_in_group--;
		}
    }
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkatetrisGoal::CheckTetrisTricks()
{
	// special case trick flag...used for an additional condition that must be
	// met before any skatetris tricks can be cleared.
	// The flag is cleared each time the tricks are checked
	int trick_flag;
	if ( mp_params->GetInteger( CRCD(0x60b827d5,"trick_flag"), &trick_flag, Script::NO_ASSERT ) )
	{
		if ( trick_flag == 0 )
			return;
		else 
			mp_params->AddInteger( CRCD(0x60b827d5,"trick_flag"), 0 );
	}
	
	bool check_combo = false;
	if ( IsSingleComboSkatetris() || IsComboSkatetris() )
		check_combo = true;

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
    Obj::CSkater *pSkater = skate_mod->GetLocalSkater();
    Dbg_Assert( pSkater );
  
    Mdl::Score * p_score = ( pSkater->GetScoreObject() );
    Dbg_Assert( p_score );
    
	// check for combos!
	if ( check_combo )
	{
		CheckTetrisCombos();
	}
	else
	{
		// check for single tricks
		bool trick_removed = false;
		for ( int i = 0; i < Game::vMAXTETRISTRICKS; i++ )
		{
			if ( !m_tetrisTricks[i].invalid )
			{
				if ( m_tetrisTricks[i].trickNameChecksum != 0 )
				{
					if ( p_score->GetCurrentNumberOfOccurrencesByName( m_tetrisTricks[i].trickNameChecksum, m_tetrisTricks[i].spin_mult, m_tetrisTricks[i].require_perfect ) )
					{
						if ( !trick_removed )
						{
							Script::RunScript( "goal_tetris_play_trick_removed_sound" );
						}
						trick_removed = true;
						RemoveTrick( i );
					}
					else if ( m_tetrisTricks[i].altTrickNameChecksum != 0 )
					{
						// check alternate trick name - usually BS version of grind trick
						if ( p_score->GetCurrentNumberOfOccurrencesByName( m_tetrisTricks[i].altTrickNameChecksum, m_tetrisTricks[i].spin_mult, m_tetrisTricks[i].require_perfect ) )
						{
							if ( !trick_removed )
							{
								Script::RunScript( "goal_tetris_play_trick_removed_sound" );
							}
							trick_removed = true;
							RemoveTrick( i );							
						}
					}
				}
				else if ( m_tetrisTricks[i].keyCombo != 0 )
				{
					// printf( "checking for a key combo %s\n", Script::FindChecksumName( m_tetrisTricks[i].keyCombo ));
					if ( p_score->GetCurrentNumberOfOccurrences( m_tetrisTricks[i].keyCombo, m_tetrisTricks[i].spin_mult, m_tetrisTricks[i].require_perfect, m_tetrisTricks[i].num_taps ) )
					{
						if ( !trick_removed )
							Script::RunScript( "goal_tetris_play_trick_removed_sound" );
						trick_removed = true;
						RemoveTrick( i );
					}
				}
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkatetrisGoal::CheckTetrisCombos()
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
    Obj::CSkater *p_Skater = skate_mod->GetLocalSkater();
    Dbg_Assert( p_Skater );
  
    Mdl::Score * p_Score = ( p_Skater->GetScoreObject() );
    Dbg_Assert( p_Score );
    
	int group_size;
	mp_params->GetInteger( CRCD(0x67dd90ca,"combo_size"), &group_size, Script::ASSERT );
	
	for ( int group_id = 0; group_id < Game::vMAXTETRISTRICKS; group_id++ )
	{
		bool trick_removed = false;

		// skip invalid groups
		if ( !m_validGroups[group_id] )
			continue;
		
		// see how many tricks they got out of this group
		int group_total = 0;
		for ( int i = 0; i < Game::vMAXTETRISTRICKS; i++ )
		{
			if ( m_tetrisTricks[i].keyCombo != 0 )
			{
				if ( !m_tetrisTricks[i].invalid &&
					 m_tetrisTricks[i].group_id == group_id &&
					 p_Score->GetCurrentNumberOfOccurrences( m_tetrisTricks[i].keyCombo, m_tetrisTricks[i].spin_mult, m_tetrisTricks[i].require_perfect, m_tetrisTricks[i].num_taps ) > 0
				   )
					group_total++;
			}
			else if ( m_tetrisTricks[i].trickNameChecksum != 0 )
			{
				if ( !m_tetrisTricks[i].invalid &&
					 m_tetrisTricks[i].group_id == group_id &&
					 p_Score->GetCurrentNumberOfOccurrencesByName( m_tetrisTricks[i].trickNameChecksum, m_tetrisTricks[i].spin_mult, m_tetrisTricks[i].require_perfect, m_tetrisTricks[i].num_taps ) > 0
				   )
				{
					group_total++;
				}
				else if ( m_tetrisTricks[i].altTrickNameChecksum != 0 )
				{
					// check alternate version of trick - usually BS version of grind
					if ( !m_tetrisTricks[i].invalid &&
						 m_tetrisTricks[i].group_id == group_id &&
						 p_Score->GetCurrentNumberOfOccurrencesByName( m_tetrisTricks[i].altTrickNameChecksum, m_tetrisTricks[i].spin_mult, m_tetrisTricks[i].require_perfect, m_tetrisTricks[i].num_taps ) > 0
					   )
						group_total++;					
				}
			}
		}
	
		// remove this group?
		if ( group_total >= group_size )
		{
			// kill all tricks in this group
			for ( int j = 0; j < Game::vMAXTETRISTRICKS; j++ )
			{
				if ( !m_tetrisTricks[j].invalid && m_tetrisTricks[j].group_id == group_id )
				{
					if ( !trick_removed )
						Script::RunScript( CRCD(0xe4ea743a,"goal_tetris_play_trick_removed_sound") );
					trick_removed = true;

					RemoveTrick( j );
				}
			}

			// this group id is dead
			m_validGroups[group_id] = false;

			// was this a single combo goal?
			if ( IsSingleComboSkatetris() )
			{
				Game::CGoalManager* pGoalManager = Game::GetGoalManager();
				pGoalManager->WinGoal( GetGoalId() );
			}
		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkatetrisGoal::RemoveTrick( int index )
{	
	int max_tricks;
	mp_params->GetInteger( CRCD(0x89473db7,"max_tricks"), &max_tricks, Script::ASSERT );

	// printf("found a completed trick at %i\n", i);
	Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();
	uint32 id = m_tetrisTricks[index].screenElementId;
	Dbg_Assert( id );
	Front::CScreenElementPtr p_trick = p_screen_elem_man->GetElement( id, Front::CScreenElementManager::ASSERT );

	// kill any scripts running on this element so the animations don't conflict
	Script::CStruct* p_temp_params = new Script::CStruct();
	p_temp_params->AddChecksum( CRCD(0x40c698af,"id"), id );
	Script::RunScript( CRCD(0x235bde21,"goal_tetris_reset_trick_container"), p_temp_params );
	delete p_temp_params;
	
	uint32 script;
	
	// turn the elements back to white if you fell to under 75% full
	int list_is_red = 0;
	mp_params->GetInteger( CRCD(0xd02ac196,"list_is_red"), &list_is_red, Script::NO_ASSERT );
	if ( ( list_is_red == 1 ) && ( ( m_numTetrisTricks - 1 ) < ( .75 * (float)max_tricks ) ) )
	{
		mp_params->AddInteger( CRCD(0xd02ac196,"list_is_red"), 0 );
		for ( int j = 0; j < Game::vMAXTETRISTRICKS; j++ )
		{
			if ( !m_tetrisTricks[j].invalid )
			{
				Front::CScreenElement* p_trick = p_screen_elem_man->GetElement( m_tetrisTricks[j].screenElementId , Front::CScreenElementManager::ASSERT );
				script = CRCD(0xa39b24e0,"goal_tetris_turn_trick_white");
				Script::CScript* p_trick_script = Script::SpawnScript( script, NULL );
				#ifdef __NOPT_ASSERT__
				p_trick_script->SetCommentString("Spawned from CSkatetrisGoal::RemoveTrick()");
				#endif

				p_trick_script->mpObject = p_trick;
				p_trick_script->Update();
			}
		}
	}
	
	// remove the trick
	script = CRCD(0x23a6a456,"goal_tetris_remove_trick");
	Script::CStruct* pScriptParams = new Script::CStruct();
	
	// flag for removing tricktris tricks immediately
	if ( IsTricktris() )
		pScriptParams->AddChecksum( NONAME, CRCD(0xea7ba666,"tricktris") );
	
	Script::CScript *p_new_script = Script::SpawnScript( script, pScriptParams );

	#ifdef __NOPT_ASSERT__
	p_new_script->SetCommentString("Spawned from CSkatetrisGoal::RemoveTrick()");
	#endif
	p_new_script->mpObject = p_trick;
	p_new_script->Update();
	delete pScriptParams;
	
	// grab the number of tricks completed
	int tricks_completed = 0;
	mp_params->GetInteger( CRCD(0x8d958f01,"tricks_completed"), &tricks_completed, Script::NO_ASSERT );
	tricks_completed++;
	
	// accelerate if needed
	int acceleration_interval = 0;
	mp_params->GetInteger( CRCD(0x1181b29,"acceleration_interval"), &acceleration_interval, Script::NO_ASSERT );
	// printf("acceleration interval = %i, tricks completed = %i\n", acceleration_interval, tricks_completed );
	if ( acceleration_interval && ( tricks_completed % acceleration_interval == 0 ) )
	{
		// adjust the time between new tricks
		float acceleration_percent = 0;
		mp_params->GetFloat( CRCD(0x76f65c8,"acceleration_percent"), &acceleration_percent, Script::NO_ASSERT );
		int trick_time;
		mp_params->GetInteger( CRCD(0x648d8d2a,"adjusted_trick_time"), &trick_time, Script::ASSERT );
		trick_time = (int)( ( 1 - acceleration_percent ) * trick_time );
		mp_params->AddInteger( CRCD(0x648d8d2a,"adjusted_trick_time"), trick_time );
		// printf("setting trick time to %i\n", trick_time);
	}
	// store the number of tricks completed
	// printf("%i tricks completed\n", tricks_completed );
	mp_params->AddInteger( CRCD(0x8d958f01,"tricks_completed"), tricks_completed );
	
	// update number of tricks in menu
	m_numTetrisTricks--;
	
	// printf("setting %s trick to invalid at index %i\n", m_tetrisTricks[i].trickText, i );
	m_tetrisTricks[index].invalid = true;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkatetrisGoal::UpdateFadedTricks()
{
	// special case trick flag...used for an additional condition that must be
	// met before any skatetris tricks can be cleared.
	// The flag is cleared each time the tricks are checked
	bool found_trick_flag = false;
	int trick_flag;
	if ( mp_params->GetInteger( CRCD(0x60b827d5,"trick_flag"), &trick_flag, Script::NO_ASSERT ) )
	{
		found_trick_flag = true;
	}
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
    Obj::CSkater *pSkater = skate_mod->GetLocalSkater();
    Dbg_Assert( pSkater );
  
    Mdl::Score * pScore = ( pSkater->GetScoreObject() );
    Dbg_Assert( pScore );

	Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();
	Dbg_Assert( p_screen_elem_man );

	float faded_trick_alpha = Script::GetFloat( CRCD( 0x855e6da4, "goal_tetris_faded_trick_alpha" ), Script::ASSERT );
	float unfaded_trick_alpha = Script::GetFloat( CRCD( 0xdbe5b19e, "goal_tetris_unfaded_trick_alpha" ), Script::ASSERT );

	for ( int i = 0; i < vMAXTETRISTRICKS; i++ )
	{
		if ( !m_tetrisTricks[i].invalid )
		{
			Front::CScreenElement* p_trick = p_screen_elem_man->GetElement( m_tetrisTricks[i].screenElementId , Front::CScreenElementManager::ASSERT );
			bool should_be_faded = false;
			if ( found_trick_flag && !trick_flag )
			{
				should_be_faded = false;
			}
			else if ( m_tetrisTricks[i].keyCombo != 0 )
			{
				int n = pScore->GetCurrentNumberOfOccurrences( m_tetrisTricks[i].keyCombo, m_tetrisTricks[i].spin_mult, m_tetrisTricks[i].require_perfect, m_tetrisTricks[i].num_taps );
				if ( n > 0 )
				{
					should_be_faded = true;
				}
			}
			else if ( m_tetrisTricks[i].trickNameChecksum != 0 )
			{
				int n = pScore->GetCurrentNumberOfOccurrencesByName( m_tetrisTricks[i].trickNameChecksum, m_tetrisTricks[i].spin_mult, m_tetrisTricks[i].require_perfect );
				if ( n > 0 )
				{
					should_be_faded = true;
				}
				else if ( m_tetrisTricks[i].altTrickNameChecksum != 0 )
				{
					// check alternate version of trick - usually BS version of grind
					int n = pScore->GetCurrentNumberOfOccurrencesByName( m_tetrisTricks[i].altTrickNameChecksum, m_tetrisTricks[i].spin_mult, m_tetrisTricks[i].require_perfect );
					if ( n > 0 )
					{
						should_be_faded = true;
					}
				}
			}

			if ( should_be_faded )
				p_trick->SetAlpha( faded_trick_alpha, Front::CScreenElement::FORCE_INSTANT );
			else
				p_trick->SetAlpha( unfaded_trick_alpha, Front::CScreenElement::FORCE_INSTANT );
		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkatetrisGoal::Expire()
{
	// expire during normal skatetris is winning...during single combo it's losing
	if ( IsSingleComboSkatetris() || IsTricktris() )
		CGoal::Lose();
	else
	{
		if ( AllTricksCleared() )
		{
			CGoalManager* pGoalManager = GetGoalManager();
			pGoalManager->WinGoal( GetGoalId() );
		}
		else
		{
			RunCallbackScript( vEXPIRED );
	
			// TODO: fix after e3
			// this was done to make the horse and combo letter goals work right
			if ( m_endRunType != vNONE )
				Deactivate();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkatetrisGoal::AllTricksCleared()
{
    for ( int i = 0; i < Game::vMAXTETRISTRICKS; i++ )
    {
        if ( m_tetrisTricks[i].invalid == false )
			return false;
    }
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkatetrisGoal::ClearAllTricks()
{
    m_numTetrisTricks = 0;
    m_tetrisTime = 0;
    
    for ( int i = 0; i < Game::vMAXTETRISTRICKS; i++ )
    {
        m_tetrisTricks[i].invalid = true;
        m_tetrisTricks[i].trickNameChecksum = 0;
		m_tetrisTricks[i].altTrickNameChecksum = 0;
		m_tetrisTricks[i].keyCombo = 0;

		m_validGroups[i] = false;
    }

	mp_params->RemoveFlag( CRCD(0xc93da683,"single_combo_added") );

	// clear all the screen elements
	Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();
	Script::CScript* pScript = NULL;
	p_screen_elem_man->DestroyElement( CRCD(0xe35ec715,"tetris_tricks_menu"), Front::CScreenElementManager::RECURSE, Front::CScreenElementManager::PRESERVE_PARENT, pScript);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkatetrisGoal::IsComboSkatetris()
{
	int combo_test = 0;
	mp_params->GetInteger( CRCD(0x4ec3cfb5,"combo"), &combo_test, Script::NO_ASSERT );
	return ( combo_test || mp_params->ContainsFlag( CRCD(0x4ec3cfb5,"combo") ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkatetrisGoal::IsSingleComboSkatetris()
{
	int single_combo_test = 0;
	mp_params->GetInteger( CRCD(0xdf47b144,"single_combo"), &single_combo_test, Script::NO_ASSERT );
	return ( single_combo_test || mp_params->ContainsFlag( CRCD(0xdf47b144,"single_combo") ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkatetrisGoal::IsTricktris()
{
	int tricktris_test = 0;
	mp_params->GetInteger( CRCD(0xea7ba666,"tricktris"), &tricktris_test, Script::NO_ASSERT );
	return ( tricktris_test || mp_params->ContainsFlag( CRCD(0xea7ba666,"tricktris") ) );
}


}		// namespace Game
