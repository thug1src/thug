/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		skate3													**
**																			**
**	Module:									 								**
**																			**
**	File name:																**
**																			**
**	Created by:		rjm														**
**																			**
**	Description:						 									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/modules/skate/score.h>

#include <core/defines.h>
#include <core/string/stringutils.h>

#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>

#include <gel/scripting/symboltable.h>
#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/checksum.h>
#include <gel/components/trickcomponent.h>
#include <gel/components/statsmanagercomponent.h>
#include <gel/components/animationcomponent.h>

#include <gfx/nxviewman.h>
#include <gfx/2d/screenelemman.h>
#include <gfx/2d/textelement.h>
#include <gfx/2d/spriteelement.h>

#include <sk/gamenet/gamenet.h>
#include <sk/gamenet/gamemsg.h>

#include <sk/objects/skater.h>		   // for control and trick stuff
#include <sk/objects/skaterprofile.h>
#include <sk/objects/skatercareer.h>

#include <sk/components/skaterruntimercomponent.h>
#include <sk/components/skateradjustphysicscomponent.h>

#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/goalmanager.h>

#include <sk/scripting/cfuncs.h>

#include <sys/replay/replay.h>

namespace Front
{
	extern void SetScoreTHPS4(char* score_text, int skater_num);
}

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Mdl
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define NO_SCORE_DURING_UBER_FRIG
#define MAX_SCORE_DUE_TO_TWEAK (20000)
	
/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static const int MAX_SPIN_VALUES					= 6;
static const int SPIN_MULT_VALUES[MAX_SPIN_VALUES]	= {2, 3, 4, 5, 6, 7};
static const int MAX_DEPREC							= 5;
static const int DEPREC_VALUES[MAX_DEPREC]			= {100, 75, 50, 25, 10};


/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

// given a score increment, return a value by which it should be decremented
// basically return the closest multiple of 10 below score
int	score_increment(int score)
{
	if (score >1000000) return 1000000;
	if (score >100000) return 100000;
	if (score >10000) return 10000;
	if (score >10000) return 1000;
	if (score >1000) return 100;
	if (score >100) return 100;
	if (score >10) return 10;
	return 1;	
}




Score::Score() :
	m_historyTab(16),
	m_infoTab(8)
{
	m_skaterId = 0;
	m_totalScore = 0;
	m_scorePot = 0;
	m_recentScorePot = 0;
	m_recentSpecialScorePot = 0;
	m_currentTrick = 0;
	m_currentMult = 0;
	m_currentBlockingTrick = -1;
	m_currentSpinTrick = -1;

	m_specialScore = 0;
	set_special_is_active(false);

	m_special_interpolator = 0.0f;

	m_longestCombo = 0;
	m_bestCombo = 0;
	m_bestGameCombo = 0;
	
	//mp_trickWindow = NULL;

	m_debug = (bool) Script::GetInteger("print_trick_info");

	setup_balance_meter_stuff();
}




Score::~Score()
{
	Reset();
}

int Score::get_packed_score( int start, int end )
{
	int packed_score = 0;
	int current_spin_mult = 2;		// n over 2
	for (int i = start; i < end; i++)
	{
		// if a blocking trick, use default spin multiplier
		if (m_infoTab[i].flags & vBLOCKING)
		{
			current_spin_mult = 2;
		}
		// if not a blocking trick, but the last trick was, then use the spin multiplier here
		// to apply to subsequent tricks up to next blocking one
		else if (i == 0 || (m_infoTab[i-1].flags & vBLOCKING))
		{
			current_spin_mult = spinMult(m_infoTab[i].spin_mult_index);
		}

		int deprec_mult = deprecMult(m_infoTab[i].mult_index);
		
//		if (m_debug)
//			printf("   base score %d, reduction mult %.2f, spin mult %.1f\n", 
//				   m_infoTab[i].score, (float) deprec_mult / 100, (float) current_spin_mult / 2);
		
		int score = (m_infoTab[i].score * deprec_mult * current_spin_mult) / 200;
		if (m_infoTab[i].switch_mode)
		{
			score	= score * 120 / 100;
		}
		
		packed_score += score;
	}
	return packed_score;
}

void Score::pack_trick_info_table()
{
	TrickInfo theTrickInfo;
	strcpy( theTrickInfo.trickNameText, "... + " );
	strcpy( theTrickInfo.trickTextFormatted, "...\\_+\\_" );
	theTrickInfo.score = 0;
	theTrickInfo.id = 0;
	theTrickInfo.switch_mode = 0;
	theTrickInfo.flags = 0;
	theTrickInfo.mult_index = 0;
	theTrickInfo.spin_mult_index = 0;
	theTrickInfo.spin_degrees = 0;

	int firstBlockingTrick = m_infoTab.GetSize();

	for ( int i = 0; i < m_infoTab.GetSize(); i++ )
	{
//		if ( ( i != 0 ) && ( m_infoTab[i].flags & vBLOCKING ) )
		if ( ( i != 0 ) && ( m_infoTab[i].trickNameText[0] == 0 ) )
		{
			firstBlockingTrick = i;
			break;
		}
	}
	
	// GJ:  see note below
	if ( ( firstBlockingTrick > m_currentSpinTrick )
		 || ( firstBlockingTrick > m_currentBlockingTrick ) )
	{
		return;
	}

	theTrickInfo.score = get_packed_score( 0, firstBlockingTrick );

	// shift them all...
	for ( int i = 0; i < firstBlockingTrick; i++ )
	{
		m_infoTab.Remove( 1 );

		m_currentTrick--;
		m_currentSpinTrick--;
		m_currentBlockingTrick--;

		// GJ:  these asserts will fire off if the trick list is filled
		// with more non-blocking tricks than spin tricks...	 it rarely
		// happens, except for those glitches that lets the player do
		// infinite tricks.  The if-test above will abort early,
		// allowing the calling function ( Trigger() ) to gracefully
		// handle it
		Dbg_MsgAssert( m_currentTrick > -1, ( "m_currentTrick is negative (%d)", m_currentTrick ) );
		Dbg_MsgAssert( m_currentSpinTrick > -1, ( "m_currentSpinTrick is negative (%d)", m_currentSpinTrick ) );
		Dbg_MsgAssert( m_currentBlockingTrick > -1, ( "m_currentBlockingTrick is negative (%d)", m_currentBlockingTrick ) );
	}
	
	m_infoTab[0] = theTrickInfo;
	
	// so that the packed score will be correct next time
	m_infoTab[0].flags |= vBLOCKING;

#ifdef __USER_GARY__
	Dbg_Message( "Info table has %d elements.", m_infoTab.GetSize() );
#endif
}

void Score::print_trick_info_table()
{
	for ( int i = 0; i < m_infoTab.GetSize(); i++ )
	{
		printf( "info table %d\n", i );
		printf( "\ttrickname = %s\n", m_infoTab[i].trickNameText); 
		printf( "\ttrickformatted = %s\n", m_infoTab[i].trickTextFormatted); 
		printf( "\tscore = %d\n", m_infoTab[i].score); 
		printf( "\tmult_index = %d\n", m_infoTab[i].mult_index); 
		printf( "\tspin_mult_index = %d\n", m_infoTab[i].spin_mult_index); 
		printf( "\tswitch_mode = %d\n", m_infoTab[i].switch_mode);
/*
		m_infoTab[i].id,
		m_infoTab[i].switch_mode,
		m_infoTab[i].flags,
*/
		printf( "-------------------------------------\n" );
	}
}

void Score::Update()
{
	if (m_specialScore)
	{
		// shrink special bar
		if (m_specialIsActive)
			m_specialScore = (int) ((float) m_specialScore - (float) Tmr::FrameLength() * 200.0f / Tmr::vRESOLUTION);			
		else
			m_specialScore = (int) ((float) m_specialScore - (float) Tmr::FrameLength() * 50.0f / Tmr::vRESOLUTION);			
		if (m_specialScore <= 0)
		{
			set_special_is_active(false);
			m_specialScore = 0;
		}
	}

	if (m_scorePotState == WAITING_TO_COUNT_SCORE_POT)
	{
		m_scorePotCountdown--;
		if (!m_scorePotCountdown)
		{
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			Obj::CSkater* pSkater = skate_mod->GetSkaterById( m_skaterId );
			int index;

			index = 0;
			if( pSkater )
			{
				index = pSkater->GetHeapIndex();
			}
			TrickTextCountdown(index);
			Replay::WriteTrickTextCountdown();

			m_scorePotState = SHOW_COUNTED_SCORE_POT;
		}
	}
	else if (m_scorePotState == SHOW_COUNTED_SCORE_POT)
	{
		dispatch_score_pot_value_to_screen(m_countedScorePot, 0);
		m_countedScorePot -= score_increment(m_countedScorePot);
		if (m_countedScorePot < 0) 
		{
			m_countedScorePot = 0;
			m_scorePotState = SHOW_ACTUAL_SCORE_POT;
		}
	}
	
	#if 0
	Image::RGBA						m_special_rgba[3];
	float							m_special_interpolator;
	float							m_special_interpolator_rate;
	#endif
	
	Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Front::CScreenElementPtr p_special_bar;
	
	Obj::CSkater* pSkater = skate_mod->GetSkaterById( m_skaterId );
	if( pSkater )
	{
		p_special_bar = p_manager->GetElement(CRCD(0xfe1dc544,"the_special_bar_sprite") + pSkater->GetHeapIndex());
	}
	else
	{
		p_special_bar = p_manager->GetElement(CRCD(0xfe1dc544,"the_special_bar_sprite"));
	}
	// clamp scale of bar, so that it doesn't get all crazy
	float special_mult = m_specialScore / 3000.0f;
	if (special_mult > 1.0f)
		special_mult = 1.0f;
	else if (special_mult < 0.0f)
		special_mult = 0.0f;
	p_special_bar->SetScale(special_mult, p_special_bar->GetScaleY());
	Image::RGBA special_rgba = m_special_rgba[REGULAR_SPECIAL];
	if (m_specialIsActive)
	{
		float interpolate_mult = (cosf(m_special_interpolator) + 1.0f) / 2.0f;
		m_special_interpolator += m_special_interpolator_rate;
		
		special_rgba.r = (uint8) ((float) m_special_rgba[1].r + ((float) (m_special_rgba[2].r - m_special_rgba[1].r)) * interpolate_mult);
		special_rgba.g = (uint8) ((float) m_special_rgba[1].g + ((float) (m_special_rgba[2].g - m_special_rgba[1].g)) * interpolate_mult);
		special_rgba.b = (uint8) ((float) m_special_rgba[1].b + ((float) (m_special_rgba[2].b - m_special_rgba[1].b)) * interpolate_mult);
		special_rgba.a = (uint8) ((float) m_special_rgba[1].a + ((float) (m_special_rgba[2].a - m_special_rgba[1].a)) * interpolate_mult);
	}
	p_special_bar->SetRGBA(special_rgba);

	/*
	HUD::PanelMgr* panel_mgr = HUD::PanelMgr::Instance();
	HUD::Panel* panel;

	if(( panel = panel_mgr->GetPanelBySkaterId(m_skaterId, false)))
	{
		panel->SetSpecialPercent((float) m_specialScore / 3000.0f, m_specialIsActive);
	}
	*/
}




/*
	Called from CSkater constructor
*/
void Score::SetSkaterId(short skater_id) 
{
	m_skaterId = skater_id;
	//HUD::PanelMgr* panel_mgr = HUD::PanelMgr::Instance();
	//mp_trickWindow = panel_mgr->GetTrickWindow(m_skaterId);
}




void Score::copy_trick_name_with_special_spaces(char *pOut, const char *pIn)
{
	int count = 0;
	while(*pIn != '\0')
	{
		Dbg_Assert(count < TrickInfo::TEXT_BUFFER_SIZE);
		if (*pIn == ' ')
		{
			*pOut++ = '\\';
			*pOut++ = '_';
			count += 2;
		}
		else
		{
			*pOut++ = *pIn;
			count++;
		}
		pIn++;
	}
	*pOut = '\0';
}

/*
	Added by Ken. Returns the Id of the last trick added, or 0 if there are none.
	Added for use by the skater Display commmand when the AddSpin flag is specified.
	In that case, if the skater's current trick is the same as the last one in the score object,
	then instead of adding the same trick again it will add spin to the last one instead.
	Used by some flatland tricks.
	(See skater.cpp for the above code, search for | Display |)
*/
uint32 Score::GetLastTrickId()
{
	if (m_infoTab.GetSize())
	{
		return m_infoTab.Last().id;
	}
	return 0;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 Score::GetTrickId( int trickIndex )
{
	Dbg_MsgAssert( trickIndex >= 0 && trickIndex < m_currentTrick, ( "trickIndex out of range" ) );
	return m_infoTab[trickIndex].id;
}

void Score::TrickTextPulse(int index)
{
	Script::CStruct* pParams=new Script::CStruct;
	pParams->AddChecksum( CRCD(0x33124d2e,"trick_text_container_id"), CRCD(0x1e55886e,"trick_text_container")  + index);
	pParams->AddChecksum( CRCD(0x6e7c7ba7,"the_trick_text_id"), CRCD(0x44727dae,"the_trick_text")  + index);
	pParams->AddChecksum( CRCD(0x6d02989c,"the_score_pot_text_id"), CRCD(0xf4d3a70e,"the_score_pot_text") + index);
	Script::RunScript(CRCD(0x3bd51fe5,"trick_text_pulse"), pParams);
	delete pParams;
}

void Score::TrickTextCountdown(int index)
{
	Script::CStruct* pParams=new Script::CStruct;
	pParams->AddChecksum( CRCD(0x33124d2e,"trick_text_container_id"), CRCD(0x1e55886e,"trick_text_container")  + index );
	pParams->AddChecksum( CRCD(0x6e7c7ba7,"the_trick_text_id"), ( CRCD(0x44727dae,"the_trick_text") ) + index );
	pParams->AddChecksum( CRCD(0x6d02989c,"the_score_pot_text_id"), ( CRCD(0xf4d3a70e,"the_score_pot_text") ) + index );
	Script::RunScript(CRCD(0xee9ce723,"trick_text_countdown"), pParams );
	delete pParams;
}

void Score::TrickTextLanded(int index)
{
	Script::CStruct* pParams=new Script::CStruct;
	pParams->AddChecksum( CRCD(0x33124d2e,"trick_text_container_id"), ( CRCD(0x1e55886e,"trick_text_container") ) + index );
	pParams->AddChecksum( CRCD(0x6e7c7ba7,"the_trick_text_id"), ( CRCD(0x44727dae,"the_trick_text") ) + index );
	pParams->AddChecksum( CRCD(0x6d02989c,"the_score_pot_text_id"), ( CRCD(0xf4d3a70e,"the_score_pot_text") ) + index );
	Script::RunScript(CRCD(0xc890e875,"trick_text_landed"), pParams);
	delete pParams;
}

void Score::TrickTextBail(int index)
{
	Script::CStruct* pParams=new Script::CStruct;
	pParams->AddChecksum( CRCD(0x33124d2e,"trick_text_container_id"), ( CRCD(0x1e55886e,"trick_text_container") ) + index );
	pParams->AddChecksum( CRCD(0x6e7c7ba7,"the_trick_text_id"), ( CRCD(0x44727dae,"the_trick_text") ) + index );
	pParams->AddChecksum( CRCD(0x6d02989c,"the_score_pot_text_id"), ( CRCD(0xf4d3a70e,"the_score_pot_text") ) + index );
	Script::RunScript(CRCD(0xaa7f404d,"trick_text_bail"), pParams );
	delete pParams;
}

/*
	Called when a trick is done, adding it to the trick sequence.
*/
const int TRICK_LIMIT = 250;
void Score::Trigger(char *trick_name, int base_score, Flags flags)
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	
	bool null_trick=false;
	if (!trick_name[0])
	{
		null_trick=true;
	}
		
	// once some trick/score has appeared on-screen
	// we know that the trick has started
	Obj::CSkater* pSkater = skate_mod->GetSkaterById( m_skaterId );
	Dbg_Assert( pSkater );
	Obj::CTrickComponent* pTrickComponent = GetTrickComponentFromObject(pSkater);
	Dbg_Assert( pTrickComponent );
    Obj::CStatsManagerComponent* pStatsManagerComponent = GetStatsManagerComponentFromObject(pSkater);
	Dbg_Assert( pStatsManagerComponent );
	
	// Only flag a trick as having started if a real trick is being triggered
	if( trick_name && trick_name[0] )
	{
		pTrickComponent->SetFirstTrickStarted( true );
	}
	
    bool is_switch = false; // need to make this grab the switch flag eventually!!!
    pStatsManagerComponent->SetTrick( trick_name, base_score, is_switch );

	// We place a high limit on tricks, so if they 
	// cheat with perfect baclance, and keep tricking
	// then they will not run out of memory and crash
	if (m_currentTrick > TRICK_LIMIT-1)
	{
		pack_trick_info_table();

		if (m_currentTrick > TRICK_LIMIT-1)
		{
			// GJ:  You should never be able to hit the trick limit any more

			if ( !null_trick )
			{
				printf ("Trick Limit (%d) Reached\n",TRICK_LIMIT);
				//tweak_last_valid_trick(base_score);
				m_infoTab.Last().score 	+= base_score;  // give them the score, just stop recording tricks
				m_currentMult++;						// but still keep up the multipler 
				captureScore();
			}
			return;
		}
	}
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
	
	// trick type is based on id + switch mode
	uint32 id = Script::GenerateCRC(trick_name);
	uint32 switch_mode = (flags & (vSWITCH | vNOLLIE)) >> 1;
		
	if (m_infoTab.GetSize() < TRICK_LIMIT)
	{
		m_infoTab.Add(new TrickInfo);
		m_infoTab.Last().score 				= 0;
	}
	// if we have filled up the trick limit, then we just use the last one we added
	
	m_infoTab.Last().score 				+= base_score;
	//printf("triggering trick with id %s\n",trick_name);
	m_infoTab.Last().id 				= id;
	m_infoTab.Last().switch_mode		= switch_mode;
	m_infoTab.Last().flags 				= flags;
	m_infoTab.Last().spin_mult_index 	= 0;
	m_infoTab.Last().trickNameText[0]	= 0;
	m_infoTab.Last().trickTextFormatted[0]	= 0;
	m_infoTab.Last().spin_degrees		= 0;
	
	if (!null_trick)
	{
		copy_trick_name_with_special_spaces(m_infoTab.Last().trickNameText, trick_name);
		
		if ( flags & Score::vCAT )
		{
			sprintf(m_infoTab.Last().trickTextFormatted, "\\c3%s\\c0 ", m_infoTab.Last().trickNameText);
		}
		else if ( flags & Score::vSPECIAL )
		{
			// K: If the 'special' flag is set, use color 2
			sprintf(m_infoTab.Last().trickTextFormatted, "\\c2%s\\c0 ", m_infoTab.Last().trickNameText);
		}
		else
		{
			sprintf(m_infoTab.Last().trickTextFormatted, "%s ", m_infoTab.Last().trickNameText);
		}
		
		// toss the "+" onto the end of the previous trick
		
		// K: This bit of code used to do a sprintf, but I changed it to use a strcat instead
		// to preserve any color formatting added previously, such as the special-trick color
		// added above.
		
		// This is a loop so that null tricks (tricks with no name) get skipped over.
		int index=m_infoTab.GetSize() - 2;
		while (true)
		{
			if (index<0)
			{
				break;
			}
				
			char *p_trick_text_formatted=m_infoTab[index].trickTextFormatted;
			Dbg_MsgAssert(p_trick_text_formatted,("NULL p_trick_text_formatted"));
			
			int len=strlen(p_trick_text_formatted);
			if (*p_trick_text_formatted == '.')
			{
				// skip items that begin with "..."
				// (or else you get things like
				// "... +  + blah blah")
			}
			else if (len)
			{
				// Remove any trailing space character		
				if (p_trick_text_formatted[len-1]==' ')
				{
					p_trick_text_formatted[len-1]=0;
				}	
				
				// Wack on the +
				strcat(p_trick_text_formatted,"\\_+ ");
				break;
			}
			// Keep searching backwards until a named trick is found.
			--index;
		}
	}
			
	// if the last trick was a blocking trick, make this trick the spin trick
	// (or if this is the first trick)
	if (m_currentBlockingTrick == m_currentTrick - 1)
		m_currentSpinTrick = m_currentTrick;
	if (flags & vBLOCKING)
	{
		m_currentBlockingTrick = m_currentTrick;
	}

	// set depreciation stuff
	//Game::CGameMode* pGameMode = skate_mod->GetGameMode();
	if (!(flags & vGAP) && !(flags & vNODEGRADE) /*&& pGameMode->ShouldDegradeScore()*/)
	{
		// trick is not a gap, so figure out what its depreciation index will be
		TrickHistory *pHistory = m_historyTab.GetItem(id);
		if (!pHistory)
		{
			// if no history for this trick, create one
			pHistory = new TrickHistory;
			m_historyTab.PutItem(id, pHistory);
			for (int s = 0; s < 4; s++)
			{
				pHistory->total_count[s] = 0;
				pHistory->combo_count[s] = 0;
			}
		}
		m_infoTab.Last().mult_index = pHistory->total_count[switch_mode] + (pHistory->combo_count[switch_mode]++);
	}
	else
	{
		// gaps don't depreciate
		// no depreciation used in free skate
		m_infoTab.Last().mult_index = 0;
	}
	
	m_currentTrick++;
	if (!null_trick)
	{
		#ifdef NO_SCORE_DURING_UBER_FRIG
		int previousMult = m_currentMult;
		#endif
		
		m_currentMult++;
		if (m_currentMult == 1)
		{
			Obj::CSkaterRunTimerComponent* pSkaterRunTimerComponent = GetSkaterRunTimerComponentFromObject(pSkater);
			Dbg_Assert(pSkaterRunTimerComponent);
			pSkaterRunTimerComponent->ComboStarted();
			
			pSkater->BroadcastEvent(CRCD(0x670fda8c, "SkaterEnterCombo"));
		}

		#if 1	
		// (Mick) check if this is the last of a long line of non-blocking tricks 
		// and if so, then decrement the multiplier (leaving it unchanged)
		int non_block_count = 0;   
		for (int i = m_currentTrick-1; i >0; i--)
		{
			// if a blocking trick, use default spin multiplier
			if (m_infoTab[i].flags & vBLOCKING)
			{
				break;
			}
			non_block_count++;
		}	
//		printf ("%3d: ",non_block_count);
		if (non_block_count > 10)
		{
			#ifdef	__NOPT_ASSERT__
			printf ("CHEAT PREVENTION:  Limiting non blocking combo to 10\n");
			#endif
			m_infoTab.Last().score = 0;
			m_currentMult--;
		}
		#endif
		
		#ifdef NO_SCORE_DURING_UBER_FRIG
		Obj::CSkaterAdjustPhysicsComponent* pSkaterAdjustPhysicsComponent = GetSkaterAdjustPhysicsComponentFromObject(pSkater);
		if (pSkaterAdjustPhysicsComponent && pSkaterAdjustPhysicsComponent->UberFriggedThisFrame())
		{
			m_infoTab.Last().score = 0;
			m_currentMult = previousMult;
		}
		#endif
	}
	
	if (m_debug)
	{
		printf("Adding trick %s\n", trick_name);
	}
	
	captureScore();

	dispatch_trick_sequence_to_screen();

	TrickTextPulse(pSkater->GetHeapIndex());
	Replay::WriteTrickTextPulse();	
		
	m_scorePotState = SHOW_ACTUAL_SCORE_POT;
	dispatch_score_pot_value_to_screen(m_scorePot, m_currentMult);
	
	/*
	if (mp_trickWindow)
	{
		Dbg_MsgAssert(mp_trickWindow,( "no trick window defined"));
	
		HUD::TrickWindow::TrickType trick_type = HUD::TrickWindow::vREGULAR;
		if (flags & vGAP) trick_type = HUD::TrickWindow::vGAP;	
		if (flags & vSPECIAL) trick_type = HUD::TrickWindow::vSPECIAL;	
		
		mp_trickWindow->AddTrick(trick_name, m_currentTrick, trick_type);
	}
	*/
	Mem::Manager::sHandle().PopContext();
}




/* 
	Called when spin changes. (From CSkater::HandleAirRotation()). 
*/
void Score::UpdateSpin(int spin_degrees)
{   
	// if score pot was reset, m_currentSpinTrick is no longer valid
	if (m_currentTrick == 0 || m_currentSpinTrick < 0)
		return;
	
	// If a trick is blocking, then we don't want to add any
	// more spin to it after the initial spin		
	if (m_infoTab[m_currentSpinTrick].flags & vBLOCKING)
	{
		// printf ("Trick %d is blocing, %d degs\n",m_currentSpinTrick,spin_degrees);
		return;
	}
	
	SetSpin(spin_degrees);
}

/* 
	Called right after call to Trigger(). Current spin trick might be trick just added. Also 
	called by Update() (above).
	
	Updates the spin multiplier on the current spin trick. If the skater has increased his
	spin over the highest spin last recorded (increments of 180), then save the new spin
	multiplier.
*/
void Score::SetSpin(int spin_degrees)
{
	// if score pot was reset, m_currentSpinTrick is no longer valid
	if (m_currentTrick == 0 || m_currentSpinTrick < 0)
		return;

	int spin_position = spin_degrees;
    const char *p_direction = "";

    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
    Obj::CSkater* pSkater = skate_mod->GetSkaterById( m_skaterId );
    Dbg_Assert( pSkater );

    Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( pSkater );
	bool is_flipped = pAnimationComponent->IsFlipped();

    // frontside or backside?
    if (spin_position < 0)
    { 
        spin_position = -spin_position;
        if ( is_flipped )
        {
            p_direction = "BS";
        }
        else
        {
            p_direction = "FS";
        }
    }
    else
    {
        if ( is_flipped )
        {
            p_direction = "FS";
        }
        else
        {
            p_direction = "BS";
        }
    }

    // add a little fudge factor: doesn't need to fully rotate 180 to earn 180
	spin_position += (int)Obj::GetPhysicsFloat(CRCD(0x50c5cc2f, "spin_count_slop"));	
	int spin_index = spin_position / 180;

	// TT#3426 - Clamp spins if too high, to prevent cheaters getting stuck and doing a lot of tricks	
	// Not needed as spins only give you extra multiplier up to 900 anyway
	#if 0
	if (spin_index > 11)
	{
		#ifdef	__NOPT_ASSERT__
		printf ("CHEAT WARNING, clamping spins\n");
		#endif
		spin_index = 11;
	}
	#endif
	
	// update the rotation amount of this trick
	m_infoTab[m_currentSpinTrick].spin_degrees = spin_degrees;

	// don't bother updating text if spin index isn't big enough
	if (m_infoTab[m_currentSpinTrick].spin_mult_index < spin_index)
	{
		if (m_debug)
		{
			printf("New spin value %d:\n", spin_index * 180);
		}
		
        // Not sure why this is necessary, but Ollies w/o another trick
        // have the wrong spin direction on half rotations. i.e. 180, 540, 900, etc.
        if ( m_infoTab[m_currentSpinTrick].id == CRCD(0x9b65d7b8,"Ollie") )
        {
            if ( (spin_index%2) == 1 )
            {
                if ( p_direction == "FS")
                {
                    p_direction = "BS";
                }
                else
                {
                    p_direction = "FS";
                }
            }
        }
        
		int last_trick = m_infoTab.GetSize() - 1;
		Dbg_Assert(last_trick >= 0);
		int shown_spin = spin_index * 180;
		
		const char *p_format="%d\\_%s";
		if (last_trick > m_currentSpinTrick)
		{
			if (m_infoTab[m_currentSpinTrick].flags & Score::vCAT)
			{
				p_format="\\c3%s\\_%d\\_%s\\c0\\_+ ";
			}	
			else
			{
				if (m_infoTab[m_currentSpinTrick].flags & Score::vSPECIAL)
    			{
    				p_format="\\c2%s\\_%d\\_%s\\c0\\_+ ";
    			}
                else
                {
                    p_format="%s\\_%d\\_%s\\_+ ";
                }
			}	
		}			
		else
		{
			if (m_infoTab[m_currentSpinTrick].flags & Score::vCAT)
			{
				p_format="\\c3%s\\_%d\\_%s\\c0";
			}	
			else
			{
				if (m_infoTab[m_currentSpinTrick].flags & Score::vSPECIAL)
    			{
    				p_format="\\c2%s\\_%d\\_%s\\c0";
    			}
                else
                {
                    p_format="%s\\_%d\\_%s";
                }
			}	
		}		
		
        sprintf(m_infoTab[m_currentSpinTrick].trickTextFormatted, p_format, p_direction, shown_spin, m_infoTab[m_currentSpinTrick].trickNameText);
		dispatch_trick_sequence_to_screen();
		
		/*
		if (mp_trickWindow)
		{
			mp_trickWindow->ChangeSpin(spin_index * 180, m_currentSpinTrick);
		}
		*/
		m_infoTab[m_currentSpinTrick].spin_mult_index = spin_index;
		captureScore();

        Obj::CStatsManagerComponent* pStatsManagerComponent = GetStatsManagerComponentFromObject( pSkater );
        Dbg_Assert( pStatsManagerComponent );
        pStatsManagerComponent->SetSpin( (spin_index*180) );
		
		m_scorePotState = SHOW_ACTUAL_SCORE_POT;
		dispatch_score_pot_value_to_screen(m_scorePot, m_currentMult);
		
		pSkater->BroadcastEvent(CRCD(0x68b887bb, "SkaterSpinDisplayed"));
	}
}

void Score::TweakTrick(int tweak_value )
{	
	if (m_currentTrick <= 0) return;
	// K: If the last trick was a 'Null' trick, then do not add the score.
	// This is to fix TT5718, where it was possible to do a manual, then jump out of it
	// really quickly whilst still getting some score appearing on screen, even though you
	// did not really get those points because the non-null trick name had not been displayed yet.
	// (At the start of the manual, a 'null' trick is done using SetTrickName "", simply in order
	// to do a BlockSpin, but we don't want that null trick to enable the score counter)
	if (m_infoTab[m_currentTrick-1].trickNameText[0]==0) return;
	
	#if 1	
	// (Mick) check if this is the last of a long line of non-blocking tricks 
	// and if so, then decrement the multiplier (leaving it unchanged)
	int non_block_count = 0;   
	for (int i = m_currentTrick-1; i >0; i--)
	{
		// if a blocking trick, use default spin multiplier
		if (m_infoTab[i].flags & vBLOCKING)
		{
			break;
		}
		non_block_count++;
	}	
//		printf ("%3d: ",non_block_count);
	if (non_block_count > 10)
	{
		#ifdef	__NOPT_ASSERT__
		printf ("CHEAT PREVENTION:  Limiting tweak after 10 nonblocking trick\n");
		#endif
		return;
	}
	
	if (!(m_infoTab[m_currentTrick-1].flags & vBLOCKING) && m_infoTab[m_currentTrick-1].score > MAX_SCORE_DUE_TO_TWEAK) return;
	#endif
	
	#ifdef NO_SCORE_DURING_UBER_FRIG
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* pSkater = skate_mod->GetSkaterById(m_skaterId);
	Obj::CSkaterAdjustPhysicsComponent* pSkaterAdjustPhysicsComponent = GetSkaterAdjustPhysicsComponentFromObject(pSkater);
	if (pSkaterAdjustPhysicsComponent && pSkaterAdjustPhysicsComponent->UberFriggedThisFrame()) return;
	#endif
	
	m_infoTab[m_currentTrick-1].score += tweak_value;
	captureScore();
	
	dispatch_trick_sequence_to_screen();
	m_scorePotState = SHOW_ACTUAL_SCORE_POT;
	dispatch_score_pot_value_to_screen(m_scorePot, m_currentMult);
}

void Score::Land( void )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Net::Client* client;
    
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Game::CGameMode* pGameMode = skate_mod->GetGameMode();
	
	captureScore();
	
	/*
	if (mp_trickWindow)
	{
		Dbg_MsgAssert(mp_trickWindow,( "no trick window defined"));
		mp_trickWindow->Count(true);
    }
	*/
	
	// GLOOBY
	int trick_score = m_scorePot * m_currentMult;

	Obj::CSkater* pSkater = skate_mod->GetSkaterById( m_skaterId );
	Dbg_Assert( pSkater );
	Dbg_Assert( pSkater->IsLocalClient());
	// if playing graffiti, then update the colors as appropriate
	bool debug_graffiti = Script::GetInteger( "debug_graffiti" );
    Game::CGoalManager* pGoalManager = Game::GetGoalManager();
	pGoalManager->Land();
	if ( debug_graffiti || ( pGameMode->IsTrue("should_modulate_color") ) && ( trick_score > 0 ) )
	{
		// update the server		
		LogTrickObjectRequest( trick_score );
	}

	// flushes the graffiti trick buffer
	GetTrickComponentFromObject(pSkater)->SetGraffitiTrickStarted( false );
    
    if (!Script::GetInteger("NewSpecial"))	
	{
		// update special meter (when we land)
		m_specialScore += m_scorePot * m_currentMult;
		if (m_specialScore >= 3000)
		{
			set_special_is_active(true);
			m_specialScore = 3000;
		}
		if (m_specialScore < 0)
		{
			m_specialScore = 0;
		}
	}
	else
	{
		m_recentSpecialScorePot = 0;		// ensure next trick is not infuenced by this trick
	}
	
	// update combo records
	if ( m_currentMult > m_longestCombo )
	{
		// printf("length: old record - %i, new record - %i\n", m_longestCombo, m_currentMult);
		m_longestCombo = m_currentMult;
	}
	if ( GetLastScoreLanded() > m_bestCombo )
	{
		m_bestCombo = GetLastScoreLanded();
		// printf("points: old record - %i, new record - %i\n", m_bestCombo, GetLastScoreLanded());
	}

	if ( GetLastScoreLanded() > m_bestGameCombo )
	{
		if( gamenet_man->InNetGame() && ( pGameMode->GetNameChecksum() != CRCD(0x1c471c60,"netlobby")))
		{
			Net::MsgDesc msg_desc;
			Net::Client* client;
			GameNet::MsgScoreLanded msg;
	
			msg.m_Score = GetLastScoreLanded();
			msg.m_GameId = gamenet_man->GetNetworkGameId();
			
			msg_desc.m_Id = GameNet::MSG_ID_COMBO_REPORT;
			msg_desc.m_Data = &msg;
			msg_desc.m_Length = sizeof( GameNet::MsgScoreLanded );
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
	
			client = gamenet_man->GetClient( pSkater->GetSkaterNumber());
			Dbg_Assert( client );
					
			client->EnqueueMessageToServer( &msg_desc );
		}
		m_bestGameCombo = GetLastScoreLanded();
	}


	// if not degrading score, we need to clear our history
	if (!pGameMode->ShouldDegradeScore())  
	{
		// clear combo history, and total history as well, althought that really should never ahve been set.
		for (int i = 0; i < m_historyTab.getSize(); i++)
		{
			TrickHistory *pHistory = m_historyTab.GetItemByIndex(i);
			for (int s = 0; s < 4; s++)
			{
				pHistory->total_count[s] = 0;
				pHistory->combo_count[s] = 0;
			}
		}
		// and reset the rail counters between combos  
		reset_robot_detection();
	}
	else
	{
		// update trick history -- all counts from trick sequence are added to counts from session
		for (int i = 0; i < m_historyTab.getSize(); i++)
		{
			TrickHistory *pHistory = m_historyTab.GetItemByIndex(i);
			for (int s = 0; s < 4; s++)
			{
				pHistory->total_count[s] += pHistory->combo_count[s];
				pHistory->combo_count[s] = 0;
			}
		}
	}
	
	if (m_currentTrick)
	{
		m_scorePotState = WAITING_TO_COUNT_SCORE_POT;
		m_countedScorePot = m_scorePot * m_currentMult;
		m_scorePotCountdown = 150; // about 3 seconds		
		dispatch_score_pot_value_to_screen(m_scorePot * m_currentMult, 0);

		int index;
		index = pSkater->GetHeapIndex();
		TrickTextLanded(index);
		Replay::WriteTrickTextLanded();
	}
	
	resetScorePot();

	int old_score = GetTotalScore(); 

	// if we're not in graffiti mode, then update the total score
	// and panel (in graffiti mode, the client doesn't have the
	// authority to set its own scores)
	if ( !pGameMode->IsTrue(CRCD(0x11941568, "should_modulate_color")) )
	{
		if( pGameMode->ShouldTrackTrickScore())
		{
			if( ( pGameMode->ShouldAccumulateScore()) || 
				( ( pGameMode->ShouldTrackBestCombo()) &&
				  ( GetTotalScore() < trick_score )))
			{
				if( !gamenet_man->OnServer())
				{
					GameNet::MsgScoreLanded msg;
					Net::MsgDesc msg_desc;

					msg.m_GameId = gamenet_man->GetNetworkGameId();
					msg.m_Score = trick_score;
					
					msg_desc.m_Data = &msg;
					msg_desc.m_Length = sizeof( GameNet::MsgScoreLanded );
					msg_desc.m_Id = GameNet::MSG_ID_LANDED_TRICK;
					msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
					msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;

					client = gamenet_man->GetClient( pSkater->GetSkaterNumber());
					Dbg_Assert( client );
					
					client->EnqueueMessageToServer( &msg_desc );
				}

				if( pGameMode->ShouldTrackBestCombo())
				{
					SetTotalScore( trick_score );
				}
				else
				{
					SetTotalScore( m_totalScore + trick_score );
				}
			}
			else if( !pGameMode->ShouldTrackBestCombo())
			{
				// don't reset if the score is frozen
				if ( !pGameMode->ScoreFrozen() )
					SetTotalScore( trick_score );
			}
		}
	}

	int new_score = GetTotalScore();
	check_high_score(old_score, new_score);

	
	Obj::CSkaterRunTimerComponent* pSkaterRunTimerComponent = GetSkaterRunTimerComponentFromObject(pSkater);
	Dbg_Assert(pSkaterRunTimerComponent);
	pSkaterRunTimerComponent->ComboEnded();

	// XXX
//	printf("TRICK SCORE = %d\n", new_score);	
}


void Score::ForceSpecial( void )
{
	set_special_is_active(true);
	m_specialScore = 3000;
}




/*
	Called whenever skater bails. Clears combo history, resets score pot, stops special
*/
void Score::Bail( bool clearScoreText )
{    
	// update trick history -- clear counts for this combo
	for (int i = 0; i < m_historyTab.getSize(); i++)
	{
		TrickHistory *pHistory = m_historyTab.GetItemByIndex(i);
		for (int s = 0; s < 4; s++)
			pHistory->combo_count[s] = 0;
	}

	if (m_currentTrick)
	{
		m_scorePotState = SHOW_ACTUAL_SCORE_POT;
		
		if (!clearScoreText)
		{
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			Obj::CSkater* pSkater = skate_mod->GetSkaterById( m_skaterId );
			Dbg_Assert( pSkater );
	
			dispatch_score_pot_value_to_screen(m_scorePot * m_currentMult, 0);
			
			int index;
			index = pSkater->GetHeapIndex();
			TrickTextBail(index);
		}
		else
		{
			dispatch_trick_sequence_to_screen(true);
			dispatch_score_pot_value_to_screen( 0, 0 );
		}
		
		// Replay::WriteTrickTextBail();
	}
	
	/*
	if (mp_trickWindow)
	{
		Dbg_MsgAssert(mp_trickWindow,( "no trick window defined"));
		mp_trickWindow->Collapse();
	}
	*/
	
	resetScorePot();

	m_specialScore = 0;
	set_special_is_active(false);
	
	Obj::CSkaterRunTimerComponent* pSkaterRunTimerComponent
		= GetSkaterRunTimerComponentFromObject(Mdl::Skate::Instance()->GetSkaterById( m_skaterId ));
	Dbg_Assert(pSkaterRunTimerComponent);
	pSkaterRunTimerComponent->ComboEnded();

    Game::CGoalManager* pGoalManager = Game::GetGoalManager();
	pGoalManager->Bail();
}



// completely resets score object
void Score::Reset()
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* pSkater = skate_mod->GetSkaterById( m_skaterId );
	//GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	//Net::Server* server; 

	printf ("Score::RESET ...............\n");

	reset_robot_detection();

	resetScorePot();

	Lst::LookupTableDestroyer<TrickHistory> destroyer(&m_historyTab);	
	destroyer.DeleteTableContents();
	
	/*
	if (mp_trickWindow)
	{
		mp_trickWindow->ResetWindow();
		mp_trickWindow->ResetRecords();
	}
	*/

	if( pSkater && pSkater->IsLocalClient())
	{
		dispatch_trick_sequence_to_screen();
		dispatch_score_pot_value_to_screen( 0, 0 );
	}
	
	if( ( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0xbff33600,"netfirefight")) ||
		( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0x3d6d444f,"firefight")))
	{
		m_totalScore = 100;
	}
	else
	{
		m_totalScore = 0;
	}
	
	m_scorePot = 0;
	m_recentScorePot = 0;
	m_recentSpecialScorePot = 0;
	m_specialScore = 0;
	m_bestGameCombo = 0;
	
	set_special_is_active(false);

	// Mick:  The next two lines clear any pending "countdown" score that might show up again after
	// we switch levels (or return to the skateshop)
	m_countedScorePot = 0;
	m_scorePotState = SHOW_ACTUAL_SCORE_POT;

	setSpecialBarColors(); // Split into seperate function so that special bar colors could be updated for themes
	
	Obj::CSkater* p_skater = Mdl::Skate::Instance()->GetSkaterById( m_skaterId );
	if (p_skater && p_skater->IsLocalClient())
	{
		Obj::CSkaterRunTimerComponent* pSkaterRunTimerComponent = GetSkaterRunTimerComponentFromObject(p_skater);
		Dbg_Assert(pSkaterRunTimerComponent);
		pSkaterRunTimerComponent->ComboEnded();
	}

	/*
	HUD::PanelMgr* panel_mgr = HUD::PanelMgr::Instance();
	HUD::Panel* panel;

	if(( panel = panel_mgr->GetPanelBySkaterId(m_skaterId, false)))
	{
		panel->SetScore(0, true);
	}
	*/
}

void Score::setSpecialBarColors()
{
    Script::CArray *p_special_bar_colors = Script::GetArray("special_bar_colors", Script::ASSERT);
	for (int i = 0; i < 3; i++)
	{
		Script::CArray *p_rgba = p_special_bar_colors->GetArray(i);

		m_special_rgba[i].r = p_rgba->GetInteger(0);
		m_special_rgba[i].g = p_rgba->GetInteger(1);
		m_special_rgba[i].b = p_rgba->GetInteger(2);
		m_special_rgba[i].a = p_rgba->GetInteger(3);
	}
	m_special_interpolator_rate = Script::GetFloat("special_bar_iterpolator_rate", Script::ASSERT);

}


/*
	Totals up score pot of present trick sequence, sends number to trick text window
*/
void Score::captureScore()
{
	// if no tricks in list, then no score to capture 
	if (m_currentTrick == 0) 
	{
		m_recentSpecialScorePot = 0;
		m_recentScorePot = 0;
		return;
	}

	if (m_debug)
		printf("Score pot:\n");
	
	// compute score pot
	m_scorePot = 0;
	int current_spin_mult = 2;		// n over 2
	for (int i = 0; i < m_currentTrick; i++)
	{
		// if a blocking trick, use default spin multiplier
		if (m_infoTab[i].flags & vBLOCKING)
		{
			current_spin_mult = 2;
		}
		// if not a blocking trick, but the last trick was, then use the spin multiplier here
		// to apply to subsequent tricks up to next blocking one
		else
		{
			if (i == 0 || (m_infoTab[i-1].flags & vBLOCKING))
			{
				current_spin_mult = spinMult(m_infoTab[i].spin_mult_index);
			}
		}

		int deprec_mult = deprecMult(m_infoTab[i].mult_index);
		
		if (m_debug)
			printf("   base score %d, reduction mult %.2f, spin mult %.1f\n", 
				   m_infoTab[i].score, (float) deprec_mult / 100, (float) current_spin_mult / 2);
		
		int score = (m_infoTab[i].score * deprec_mult * current_spin_mult) / 200;
		if (m_infoTab[i].switch_mode)
		{
			score	= score * 120 / 100;
		}
		
		m_scorePot += score;
	}
	if (m_debug)
		printf("   total %d\n", m_scorePot);
	
	/*
	if( mp_trickWindow )
	{
		mp_trickWindow->SetScore(m_scorePot);
	}
	*/

	if (Script::GetInteger("NewSpecial"))	
	{
//		printf ("Score = %d, Recent = %d\n",m_scorePot * m_currentTrick , m_recentScorePot);
		if (m_scorePot * m_currentMult > m_recentSpecialScorePot)
		{
			// update special meter (constantly, as we score)
			m_specialScore += m_scorePot * m_currentMult - m_recentSpecialScorePot;
			if (m_specialScore <0)
			{
				set_special_is_active(false);
				m_specialScore = 0;
			}
			if (m_specialScore >= 3000)
			{
				set_special_is_active(true);
				m_specialScore = 3000;
			}	
		}
	}
	m_recentScorePot = m_scorePot * m_currentMult;
	m_recentSpecialScorePot = m_scorePot * m_currentMult;
}



void Score::resetScorePot()
{
	m_scorePot = 0;
	Lst::DynamicTableDestroyer<TrickInfo> destroyer(&m_infoTab);	
	destroyer.DeleteTableContents();
	m_currentTrick = 0;
	m_currentMult = 0;
	m_currentBlockingTrick = -1;
	m_currentSpinTrick = -1;
	reset_robot_detection_combo();
	//m_recentScorePot = 0;
}




void Score::dispatch_trick_sequence_to_screen ( bool clear_text )
{
	Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* pSkater = skate_mod->GetSkaterById( m_skaterId );
	int index = 0;

	if( pSkater )
	{
		index = pSkater->GetHeapIndex();
	}
	Front::CTextBlockElement *p_text_block = (Front::CTextBlockElement *) p_manager->GetElement(Script::GenerateCRC("the_trick_text") + index ).Convert();
	if (!p_text_block) return;

	const char *pp_string_tab[TRICK_LIMIT];

	if (!clear_text)
	{
		Dbg_Assert(m_infoTab.GetSize() <= TRICK_LIMIT);
		for (int i = 0; i < m_infoTab.GetSize(); i++)
		{
			pp_string_tab[i] = m_infoTab[i].trickTextFormatted;
		}

		p_text_block->SetText(pp_string_tab, m_infoTab.GetSize());
		// Replay::WriteTrickText(pp_string_tab, m_infoTab.GetSize());
	}
	else
	{
		p_text_block->SetText(pp_string_tab, 0);
	}

	Script::RunScript("UpdateTrickText");
}




int Score::spinMult(int x)
{
	// defines how the multiplier for spins ramps up.  .5 and 1 for 180 and 360, then 2,3,4,5,... for 540,720,900,1080,... etc.
	//return (((x)<MAX_SPIN_VALUES) ? (SPIN_MULT_VALUES[x]) : SPIN_MULT_VALUES[MAX_SPIN_VALUES-1]+4+4*((x)-MAX_SPIN_VALUES));
	// Ken: Made spin multiplier have a max value so that it doesn't get huge for flatland tricks.
	return (((x)<MAX_SPIN_VALUES) ? (SPIN_MULT_VALUES[x]) : SPIN_MULT_VALUES[MAX_SPIN_VALUES-1]);
}




int Score::deprecMult(int x)
{
	return ((x < MAX_DEPREC) ? DEPREC_VALUES[x] : DEPREC_VALUES[MAX_DEPREC-1]);
}




void Score::setup_balance_meter_stuff()
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Script::CStruct *p_balance_meter_struct = Script::GetStructure("balance_meter_info");
	Dbg_Assert(p_balance_meter_struct);
	
	// fetch arrow position table
	Script::CArray *p_arrow_array = NULL;
	p_balance_meter_struct->GetArray("arrow_positions", &p_arrow_array);
	Dbg_Assert(p_arrow_array);
	m_numArrowPositions = p_arrow_array->GetSize();
	for (int i = 0; i < m_numArrowPositions; i++)
	{
		m_arrowPosTab[i] = *p_arrow_array->GetPair(i);
	}
	// for good measure
	m_arrowPosTab[m_numArrowPositions] = m_arrowPosTab[m_numArrowPositions-1];
	m_arrowInterval = 1.0f / ((float) m_numArrowPositions);

	Script::CArray *p_bar_pos_array = NULL;
	if( ( CFuncs::ScriptInSplitScreenGame( NULL, NULL )) && 
		( skate_mod->GetGameMode()->GetNameChecksum() != Script::GenerateCRC( "horse" )) &&
		( skate_mod->GetGameMode()->GetNameChecksum() != Script::GenerateCRC( "nethorse" )))
	{
		if( Nx::CViewportManager::sGetScreenMode() == Nx::vSPLIT_V )
		{
			p_balance_meter_struct->GetArray("bar_positions_mp_v", &p_bar_pos_array);
		}
		else
		{
			p_balance_meter_struct->GetArray("bar_positions_mp_h", &p_bar_pos_array);
		}
	}
	else
	{
		p_balance_meter_struct->GetArray("bar_positions", &p_bar_pos_array);
	}

	Dbg_Assert(p_bar_pos_array);
	for (int i = 0; i < 2; i++)
	{
		m_meterPos[i] = *p_bar_pos_array->GetPair(i);
	}
}




void Score::position_balance_meter(bool state, float value, bool isVertical)
{
	Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* pSkater = skate_mod->GetSkaterById( m_skaterId );

	Front::CSpriteElement *p_balance_meter = (Front::CSpriteElement *) p_manager->GetElement(0xa4db8a4b + pSkater->GetHeapIndex()).Convert(); // "the_balance_meter"
	Dbg_Assert(p_balance_meter);
	
	int is_shown = 0;
	p_balance_meter->GetIntegerTag(0xc22eb9a0, &is_shown); // "tag_turned_on"
	if (state != (bool) is_shown)
	{
		Script::CStruct* pParams;
		pParams = new Script::CStruct;
		pParams->AddChecksum( "id", 0xa4db8a4b + pSkater->GetHeapIndex());
		if (state)
		{   
			Script::RunScript(CRCD(0xba95da16, "show_balance_meter"), pParams );
		}
		else
		{
			Script::RunScript(CRCD(0x58783b40, "hide_balance_meter"), pParams );
		}

		delete pParams;
	}
	
	if (isVertical)
		p_balance_meter->SetChecksumTag(0x863a07e2, 0xef24413b); // "tag_mode", "manual"
	else
		p_balance_meter->SetChecksumTag(0x863a07e2, 0x530be001); // "tag_mode", "balance"
	
	// figure out position of arrow
	float abs_value = Mth::Abs(value);
	int index = (int) ( abs_value / m_arrowInterval);
	float mix =  abs_value - ((float) index) * m_arrowInterval;
	m_arrowPos.mX = m_arrowPosTab[index].mX + (m_arrowPosTab[index+1].mX - m_arrowPosTab[index].mX) * mix / m_arrowInterval;
	m_arrowPos.mY = m_arrowPosTab[index].mY + (m_arrowPosTab[index+1].mY - m_arrowPosTab[index].mY) * mix / m_arrowInterval;
	//printf("arrow at %f, %f, index %d, mix %f\n", m_arrowPos.mX, m_arrowPos.mY, index, mix);
	m_arrowRot = -value * 45.0f;
	
	Front::CSpriteElement *p_balance_arrow = (Front::CSpriteElement *) p_balance_meter->GetFirstChild();//.Convert();
	Dbg_Assert(p_balance_arrow);
	
	if (isVertical)
	{
		p_balance_meter->SetPos(m_meterPos[1].mX, m_meterPos[1].mY, Front::CScreenElement::FORCE_INSTANT);
		p_balance_meter->SetRotate(-90.0f);
		if (value >= 0.0f)
			p_balance_arrow->SetPos(p_balance_meter->GetBaseW() / 2.0f + m_arrowPos.mY, p_balance_meter->GetBaseH() / 2.0f - m_arrowPos.mX, Front::CScreenElement::FORCE_INSTANT);
		else
			p_balance_arrow->SetPos(p_balance_meter->GetBaseW() / 2.0f + m_arrowPos.mY, p_balance_meter->GetBaseH() / 2.0f + m_arrowPos.mX, Front::CScreenElement::FORCE_INSTANT);
		p_balance_arrow->SetRotate(-m_arrowRot - 90.0f);
	}
	else
	{
		p_balance_meter->SetPos(m_meterPos[0].mX, m_meterPos[0].mY, Front::CScreenElement::FORCE_INSTANT);
		p_balance_meter->SetRotate(0);
		if (value >= 0.0f)
			p_balance_arrow->SetPos(p_balance_meter->GetBaseW() / 2.0f + m_arrowPos.mX, p_balance_meter->GetBaseH() / 2.0f + m_arrowPos.mY, Front::CScreenElement::FORCE_INSTANT);
		else
			p_balance_arrow->SetPos(p_balance_meter->GetBaseW() / 2.0f - m_arrowPos.mX, p_balance_meter->GetBaseH() / 2.0f + m_arrowPos.mY, Front::CScreenElement::FORCE_INSTANT);
		p_balance_arrow->SetRotate(-m_arrowRot);
	}	
}




void Score::dispatch_score_pot_value_to_screen(int score, int multiplier)
{
	Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	
	Obj::CSkater* pSkater = skate_mod->GetSkaterById( m_skaterId );
	int index = 0;

	if( pSkater )
	{
		index = pSkater->GetHeapIndex();
	}
	Front::CTextElement *p_score_pot_text = (Front::CTextElement *) p_manager->GetElement(0xf4d3a70e + index ).Convert(); // "the_score_pot_text"
	Dbg_Assert(p_score_pot_text);

	char p_string[128];
	if (!score)
		strcpy(p_string, " ");
	else if (multiplier)
		sprintf(p_string, "%s X %d", Str::PrintThousands(score), multiplier);
	else
		sprintf(p_string, "%s", Str::PrintThousands(score));

	
	p_score_pot_text->SetText(p_string);
	
	Replay::WriteScorePotText(p_string);
	Script::RunScript("UpdateScorepot");
}

// Check for a high score goals being achieved	   
// for each goal that is actieved, then run the script
// on the skater that got the score
void Score::check_high_score(int old_score, int new_score)
{
	#ifdef	DEBUG_HIGH_SCORE_GOALS
	printf("Checking High Scores, score changed from %d to %d\n",old_score, new_score); 
	#endif
	
	Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

	// if we are not in career mode, then do not check for high score			   
	if (!skate_mod->GetGameMode()->IsTrue( CRCD(0x1ded1ea4, "is_career") ))
	{
		return;
	}

	for (int i=0;i < Mdl::Skate::vMAX_SCORE_GOALS;i++)
	{
		int score = skate_mod->GetScoreGoalScore(i);
		int goal = skate_mod->GetScoreGoalGoal(i);
		uint32 script = skate_mod->GetScoreGoalScript(i);
		if (score)		
		{
			#ifdef	DEBUG_HIGH_SCORE_GOALS
			printf("For goal %2d, score %7d, script %x\n",goal,score,script); 
			#endif
			if (old_score < score && new_score >= score)
			{
				#ifdef	DEBUG_HIGH_SCORE_GOALS
				dodgy_test(); printf("Score change works\n"); 
				#endif
				if( !skate_mod->GetCareer()->GetGoal(goal))
				{
					#ifdef	DEBUG_HIGH_SCORE_GOALS
					printf("awarding this goal, and running script"); 
					#endif
					// not got this goal yet, so give it
					skate_mod->GetCareer()->SetGoal(goal);
					
					// and run the script
					skate_mod->GetSkaterById( m_skaterId )->SpawnAndRunScript(script);					
				}
				else
				{
					#ifdef	DEBUG_HIGH_SCORE_GOALS
					printf("Already got this goal\n"); 
					#endif
				}
			}  
			else
			{
				#ifdef	DEBUG_HIGH_SCORE_GOALS
				printf("Score change did not straddel this score\n"); 
				#endif
			}
		}
	}
}

void Score::SetBalanceMeter(bool state, float value)
{
	Replay::WriteBalanceMeter(state,value);
	//Ryan("balance: %f\n", value);
	position_balance_meter(state, value, false);
}




void Score::SetManualMeter(bool state, float value)
{
	Replay::WriteManualMeter(state,value);
	
	//Ryan("balance: %f\n", value);
	position_balance_meter(state, value, true);
}




void Score::SetTotalScore( int score )
{
	m_totalScore = score;

	// update the panel
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	/*Game::CGameMode* pGameMode = */skate_mod->GetGameMode();
//	bool can_zero_score = pGameMode->IsTrue("should_modulate_color");

	//	printf("Setting total score for %d: %d %d %s\n", m_skaterId, score, GetTotalScore(), Script::FindChecksumName(pGameMode->GetNameChecksum()) );
	
	Obj::CSkater* pSkater = skate_mod->GetSkaterById( m_skaterId );
	Dbg_Assert( pSkater );
	if ( pSkater->IsLocalClient() )
	{
		//HUD::PanelMgr* panel_mgr = HUD::PanelMgr::Instance();
		//HUD::Panel* panel;
		char score_text[64];
		
		if( ( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netking" )) ||
			( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "king" )))
		{
			sprintf( score_text, "%d:%.2d", Tmr::InSeconds( m_totalScore ) / 60, Tmr::InSeconds( m_totalScore ) % 60 );
		}
		else
		{
			sprintf( score_text, "%d", m_totalScore );
		}

		Front::SetScoreTHPS4( score_text, pSkater->GetHeapIndex());

		/*
		if(( panel = panel_mgr->GetPanelBySkaterId(m_skaterId, false)))
		{
			panel->SetScore( m_totalScore, can_zero_score );
		}
		*/
	}
}




int Score::GetTotalScore()
{	
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	if( gamenet_man->OnServer())
	{
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		Game::CGameMode* pGameMode = skate_mod->GetGameMode();

		// server is also responsible for calculating graffiti scores
		bool debug_graffiti = Script::GetInteger( "debug_graffiti" );
		if ( debug_graffiti || pGameMode->IsTrue("should_modulate_color") )
		{
			return skate_mod->GetTrickObjectManager()->GetScore( m_skaterId );
		}
		else
		{
			return m_totalScore;
		}
	}
	else
	{
		// clients already knows their total scores
		return m_totalScore;
	}

}




/*
	Returns the network connection.
*/
Net::Conn* Score::GetAssociatedNetworkConnection( void )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Obj::CSkater* skater;
	GameNet::PlayerInfo* player;

	

	skater = skate_mod->GetSkaterById( m_skaterId );
	if( skater )
	{
		player = gamenet_man->GetPlayerByObjectID( skater->GetID() );
		if( player )
		{
			return player->m_Conn;
		}
	}

	return NULL;
}




void Score::LogTrickObjectRequest( int score )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	GameNet::PlayerInfo* player;
	GameNet::MsgScoreLogTrickObject msg;
	Net::MsgDesc msg_desc;
	Net::Client* client;   

	player = gamenet_man->GetPlayerByObjectID( m_skaterId );
	Dbg_Assert( player );
    
	client = gamenet_man->GetClient( player->m_Skater->GetSkaterNumber());
	Dbg_MsgAssert( client, ( "Couldn't find client for skater %d", player->m_Skater->GetSkaterNumber() ) );

	msg.m_SubMsgId = GameNet::SCORE_MSG_ID_LOG_TRICK_OBJECT;
	msg.m_OwnerId = m_skaterId;
	msg.m_Score = score;
	msg.m_GameId = gamenet_man->GetNetworkGameId();

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* skater = skate_mod->GetSkaterById( m_skaterId );
	Dbg_Assert( skater );
	
	Obj::CTrickComponent* p_trick_component = GetTrickComponentFromObject(skater);
	Dbg_Assert(p_trick_component);

	// send out variable length data representing the trick chain
	uint32 max_pending_trick_buffer_size = GameNet::MsgScoreLogTrickObject::vMAX_PENDING_TRICKS * sizeof(uint32);
	uint32 actual_pending_trick_buffer_size = p_trick_component->WritePendingTricks( msg.m_PendingTrickBuffer, max_pending_trick_buffer_size );
	msg.m_NumPendingTricks = actual_pending_trick_buffer_size / sizeof( uint32 );

	printf( "Client -> server %d tricks\n", msg.m_NumPendingTricks );

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( GameNet::MsgScoreLogTrickObject ) - max_pending_trick_buffer_size + actual_pending_trick_buffer_size;
	msg_desc.m_Id = GameNet::MSG_ID_SCORE;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
	//client->EnqueueMessageToServer( &msg_desc );
	
	client->StreamMessageToServer( msg_desc.m_Id, msg_desc.m_Length, 
						   msg_desc.m_Data, "TrickObj Buffer", GameNet::vSEQ_GROUP_PLAYER_MSGS );

}




void Score::LogTrickObject( int skater_id, int score, uint32 num_pending_tricks, uint32* p_pending_tricks, bool propagate )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	GameNet::MsgScoreLogTrickObject msg;
	GameNet::MsgObsScoreLogTrickObject obs_msg;
	Net::Server* server;
	Net::Conn* conn;
	GameNet::PlayerInfo* player;
	Lst::Search< GameNet::PlayerInfo > sh;

	if( propagate )
	{
		bool send_steal_message;

		server = gamenet_man->GetServer();
		Dbg_Assert( server );

		conn = GetAssociatedNetworkConnection();

		// keep track of whom we need to send the steal message to
		int i;
		char previous_owner_flags[GameNet::vMAX_PLAYERS];
		for ( i = 0; i < GameNet::vMAX_PLAYERS; i++ )
		{
			previous_owner_flags[i] = 0;
		}
		
		num_pending_tricks = skate_mod->GetTrickObjectManager()->RequestLogTrick( num_pending_tricks, p_pending_tricks, previous_owner_flags, skater_id, score );
		
		if ( !num_pending_tricks )
		{
			return;
		}

		for ( i = 0; i < GameNet::vMAX_PLAYERS; i++ )
		{
			if ( previous_owner_flags[i] )
			{
				// GJ:  This only sends the stolen message to the two parties involved

				GameNet::PlayerInfo* new_owner, *old_owner;
				GameNet::MsgStealMessage steal_msg;

				steal_msg.m_NewOwner = skater_id;
				steal_msg.m_OldOwner = i;
				steal_msg.m_GameId = gamenet_man->GetNetworkGameId();

				new_owner = gamenet_man->GetPlayerByObjectID(skater_id);
				old_owner = gamenet_man->GetPlayerByObjectID( i );
				Dbg_Assert( new_owner );
				
				send_steal_message = true;
				if( skate_mod->GetGameMode()->IsTeamGame())
				{
					if( old_owner && new_owner && ( old_owner->m_Team == new_owner->m_Team ))
					{
						send_steal_message = false;
					}
				}

				if( send_steal_message )
				{
					Net::MsgDesc msg_desc;

					msg_desc.m_Data = &steal_msg;
					msg_desc.m_Length = sizeof( GameNet::MsgStealMessage );
					msg_desc.m_Id = GameNet::MSG_ID_STEAL_MESSAGE;
					server->EnqueueMessage( new_owner->GetConnHandle(), &msg_desc );
	
					steal_msg.m_NewOwner = skater_id;
					steal_msg.m_OldOwner = i;
					steal_msg.m_GameId = gamenet_man->GetNetworkGameId();
	
                    //Dbg_Assert( pPlayerInfo );
	
					// For now, don't assert if the player doesn't exist anymore. Just don't do anything.
					// Eventually, Gary will fix this so that pieces of exiting players are reset
					if( old_owner )
					{
						server->EnqueueMessage( old_owner->GetConnHandle(), &msg_desc );
					}
				}
			}
		}
		
#ifdef __USER_GARY__
		printf( "Broadcasting %d tricks\n", num_pending_tricks );
#endif
		
		msg.m_SubMsgId = GameNet::SCORE_MSG_ID_LOG_TRICK_OBJECT;
		msg.m_OwnerId = skater_id;
		msg.m_Score = score;
		msg.m_NumPendingTricks = num_pending_tricks;
		msg.m_GameId = gamenet_man->GetNetworkGameId();

		uint32 max_pending_trick_buffer_size = GameNet::MsgScoreLogTrickObject::vMAX_PENDING_TRICKS * sizeof(uint32);
		uint32 actual_pending_trick_buffer_size = msg.m_NumPendingTricks * sizeof(uint32);
		memcpy( msg.m_PendingTrickBuffer, p_pending_tricks, actual_pending_trick_buffer_size );

		Net::MsgDesc score_msg_desc;

		score_msg_desc.m_Data = &msg;
		score_msg_desc.m_Length = sizeof( GameNet::MsgScoreLogTrickObject ) - max_pending_trick_buffer_size + actual_pending_trick_buffer_size;
		score_msg_desc.m_Id = GameNet::MSG_ID_SCORE;
		score_msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		score_msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
		// tell players to change their colors
		for( player = gamenet_man->FirstPlayerInfo( sh ); player; 
				player = gamenet_man->NextPlayerInfo( sh ))
		{
			server->StreamMessage( player->GetConnHandle(), score_msg_desc.m_Id, score_msg_desc.m_Length, 
						   score_msg_desc.m_Data, "TrickObj Buffer", GameNet::vSEQ_GROUP_PLAYER_MSGS );
			//server->EnqueueMessage( player->GetConnHandle(), &score_msg_desc );
		}

		obs_msg.m_OwnerId = skater_id;
		obs_msg.m_NumPendingTricks = num_pending_tricks;
		obs_msg.m_GameId = gamenet_man->GetNetworkGameId();
		max_pending_trick_buffer_size = GameNet::MsgScoreLogTrickObject::vMAX_PENDING_TRICKS * sizeof(uint32);
		actual_pending_trick_buffer_size = obs_msg.m_NumPendingTricks * sizeof(uint32);
		memcpy( obs_msg.m_PendingTrickBuffer, p_pending_tricks, actual_pending_trick_buffer_size );

		score_msg_desc.m_Data = &obs_msg;
		score_msg_desc.m_Length = sizeof( GameNet::MsgObsScoreLogTrickObject ) - max_pending_trick_buffer_size + actual_pending_trick_buffer_size;
		score_msg_desc.m_Id = GameNet::MSG_ID_OBSERVER_LOG_TRICK_OBJ;
		// tell observers to change their colors
		for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; 
				player = gamenet_man->NextPlayerInfo( sh, true ))
		{
			if( player->IsObserving())
			{
				server->StreamMessage( player->GetConnHandle(), score_msg_desc.m_Id, score_msg_desc.m_Length, 
						   score_msg_desc.m_Data, "TrickObj Buffer", GameNet::vSEQ_GROUP_PLAYER_MSGS );
				//server->EnqueueMessage( player->GetConnHandle(), &score_msg_desc );
			}
		}

		// send score updates, as something has changed
		skate_mod->SendScoreUpdates( false );
		
		// Let the server's client do the rest of the work
		if( conn->IsLocal())
		{
			return;
		}
	}

#ifdef __USER_GARY__
	printf( "Client is receiving %d tricks\n", num_pending_tricks );
#endif
	
	int seq;

	if( skate_mod->GetGameMode()->IsTeamGame())
	{
		player = gamenet_man->GetPlayerByObjectID( skater_id );
		seq = player->m_Team;
	}
	else
	{
		seq = skater_id;
	}

	// modulate the color here
	for ( uint32 i = 0; i < num_pending_tricks; i++ )
	{
		
		skate_mod->GetTrickObjectManager()->ModulateTrickObjectColor( p_pending_tricks[i], seq );
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// utilities used by the trick counting functions to resolve a key_combo
// or trick to its trick text checksum after taking into account the number
// of taps.
inline uint32 get_trickname_checksum_from_trick_and_taps( uint32 trick_checksum, int num_taps = 1 )
{
	// get the trick text and associated checksum
	Script::CStruct* p_trick = Script::GetStructure( trick_checksum, Script::ASSERT );
	Script::CStruct* p_trick_params;
	p_trick->GetStructure( CRCD( 0x7031f10c, "params" ), &p_trick_params, Script::ASSERT );

	const char* p_trick_name;

	// get any double or triple tap info
	while ( num_taps > 1 )
	{			
		num_taps--;

		Script::CArray* p_extra_trick_array;
		uint32 extra_trick;

		if ( !p_trick_params->GetChecksum( CRCD( 0x6e855102, "ExtraTricks" ), &extra_trick, Script::NO_ASSERT ) )
			Dbg_MsgAssert( 0, ( "Couldn't find ExtraTricks to get multiple tap version of trick" ) );
		
		// printf("got extra trick %s\n", Script::FindChecksumName( extra_trick ) );
		
		p_extra_trick_array = Script::GetArray( extra_trick, Script::ASSERT );
		Dbg_MsgAssert( p_extra_trick_array->GetType() == ESYMBOLTYPE_STRUCTURE, ( "Extra trick array %s had wrong type", Script::FindChecksumName( extra_trick ) ) );
		Script::CStruct* p_sub_struct = p_extra_trick_array->GetStructure( 0 );
		p_sub_struct->GetStructure( CRCD( 0x7031f10c, "params" ), &p_trick_params, Script::ASSERT );
	}
	
	p_trick_params->GetLocalString( CRCD( 0xa1dc81f9, "name" ), &p_trick_name, Script::ASSERT );
	// printf("found name %s\n", p_trick_name);
	return Script::GenerateCRC( p_trick_name );
}

inline uint32 get_trickname_checksum_from_key_combo( uint32 key_combo, int num_taps = 1 )
{
	// printf("\tget_trickname_checksum_from_key_combo( %s, %i )\n", Script::FindChecksumName( key_combo ), num_taps );
	
	// get the key mapping and trick name
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();
	Script::CStruct* pTricks = pSkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );	
	uint32 trick_checksum = 0;
	
	// see if this is a normal or special trick
	if ( !pTricks->GetChecksum( key_combo, &trick_checksum, Script::NO_ASSERT ) )
	{
		Script::CStruct* pSpecialTricks = pSkaterProfile->GetSpecialTricksStructure();
		Script::CArray* pSpecialTricksArray;
		pSpecialTricks->GetArray( NONAME, &pSpecialTricksArray, Script::ASSERT );
		int numSpecials = pSpecialTricksArray->GetSize();
		for ( int i = 0; i < numSpecials; i++ )
		{
			Script::CStruct* pSpecial = pSpecialTricksArray->GetStructure( i );
			uint32 trickSlot;
			pSpecial->GetChecksum( CRCD( 0xa92a2280, "TrickSlot" ), &trickSlot, Script::ASSERT );
			if ( trickSlot == key_combo )
			{
				pSpecial->GetChecksum( CRCD( 0x5b077ce1, "TrickName" ), &trick_checksum, Script::ASSERT );
				break;
			}
		}
	}

	if ( trick_checksum == 0 )
		return 0;

	return get_trickname_checksum_from_trick_and_taps( trick_checksum, num_taps );	
}


inline uint32 get_cat_index_from_key_combo( uint32 key_combo )
{
	// printf("\tget_trickname_checksum_from_key_combo( %s, %i )\n", Script::FindChecksumName( key_combo ), num_taps );
	
	// get the key mapping and trick name
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();
	Script::CStruct* pTricks = pSkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );	
	int cat_trick = -1;
	
	// see if this is a normal or special trick

	if ( !pTricks->GetInteger( key_combo, &cat_trick, Script::NO_ASSERT ) )
	{
		Script::CStruct* pSpecialTricks = pSkaterProfile->GetSpecialTricksStructure();
		Script::CArray* pSpecialTricksArray;
		pSpecialTricks->GetArray( NONAME, &pSpecialTricksArray, Script::ASSERT );
		int numSpecials = pSpecialTricksArray->GetSize();
		for ( int i = 0; i < numSpecials; i++ )
		{
			Script::CStruct* pSpecial = pSpecialTricksArray->GetStructure( i );
			int is_cat = 0;
			pSpecial->GetInteger( CRCD(0xb56a8816,"isCat"), &is_cat, Script::NO_ASSERT );
			if ( is_cat != 0 )
			{
				uint32 trickslot;
				pSpecial->GetChecksum( CRCD(0xa92a2280,"trickSlot"), &trickslot, Script::ASSERT );
				if ( trickslot == key_combo )
				{
					uint32 cat_trick_checksum;
					pSpecial->GetChecksum( CRCD(0x5b077ce1,"trickName"), &cat_trick_checksum, Script::ASSERT );
					cat_trick = (int)cat_trick_checksum;
					break;
				}
			}
		}
	}

	return cat_trick;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Score::VerifyTrickMatch( int info_tab_index, uint32 trick_checksum, int spin_mult, bool require_perfect )
{
	if ( info_tab_index >= m_currentTrick )
		return false;
	
	// printf("\tVerifyTrickMatch( %i, %s, %i, %i )\n", info_tab_index, Script::FindChecksumName( trick_checksum ), spin_mult, require_perfect );
	// printf("m_infoTab[%i].id = %s\n", info_tab_index, Script::FindChecksumName( m_infoTab[info_tab_index].id ) );
	if ( m_infoTab[info_tab_index].id == trick_checksum )
	{
		if ( spin_mult == 0 )
			return true;
		else 
		{
			// printf("spin_degrees for this trick = %i\n", m_infoTab[i].spin_degrees);
			if ( spin_mult == m_infoTab[info_tab_index].spin_mult_index )
			{
				if ( !require_perfect )
					return true;
				else if ( Mth::Abs( Mth::Abs(m_infoTab[info_tab_index].spin_degrees) - ( spin_mult * 180 ) ) < Script::GetInteger( CRCD( 0xfcfacab8, "perfect_landing_slop" ) ) )
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

int Score::CountTrickMatches( uint32 trick_checksum, int spin_mult, bool require_perfect )
{
	// printf("CountTrickMatches( %s, %i, %i )\n", Script::FindChecksumName( trick_checksum ), spin_mult, require_perfect );
	int current_spin = 1;
	
	int total = 0;

	for ( int i = 0; i < m_currentTrick; i++ )
	{
		if ( m_infoTab[i].flags & vBLOCKING )
		{
			current_spin = 1;
		}
		else if ( i == 0 || ( i >= 1 && m_infoTab[i-1].flags & vBLOCKING ) )
		{
			current_spin = m_infoTab[i].spin_mult_index;
		}
		// printf("current_spin = %i\n", current_spin );
		
		if ( m_infoTab[i].id == trick_checksum )
		{
			if ( spin_mult == 0 )
				total++;
			else 
			{
				// printf("spin_degrees for this trick = %i\n", m_infoTab[i].spin_degrees);
				if ( spin_mult == current_spin )
				{
					if ( !require_perfect )
						total++;
					else if ( Mth::Abs( Mth::Abs(m_infoTab[i].spin_degrees) - ( spin_mult * 180 ) ) < Script::GetInteger( CRCD( 0xfcfacab8, "perfect_landing_slop" ) ) )
						total++;
				}
			}		
		}
	}
	return total;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Score::CountStringMatches( const char* string )
{
	int total = 0;
    
    for ( int i = 0; i < m_currentTrick; i++ )
	{
        if ( Str::StrStr( m_infoTab[i].trickNameText, string ) )
		{
            total++;		
		}
	}
	return total;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Score::IsLatestTrickByName( uint32 trick_text_checksum, int spin_mult, bool require_perfect, int num_taps )
{
	int index = m_currentTrick;
	do
	{
		if (index <= 0) return false;
		index--;
	}
	while (m_infoTab[index].trickNameText[0] == 0);
	
	// resolve multiple tap tricks
	if (num_taps > 1)
	{
		trick_text_checksum = get_trickname_checksum_from_trick_and_taps(trick_text_checksum, num_taps);
	}
	
	return VerifyTrickMatch(index, trick_text_checksum, spin_mult, require_perfect);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Score::IsLatestTrick( uint32 key_combo, int spin_mult, bool require_perfect, int num_taps )
{
	int index = m_currentTrick;
	do
	{
		if (index <= 0) return false;
		index--;
	}
	while (m_infoTab[index].trickNameText[0] == 0);
	
	// get the key mapping and trick name
	uint32 trick_name_checksum = get_trickname_checksum_from_key_combo( key_combo, num_taps );
	if ( trick_name_checksum == 0 )
	{
		// check for cat trick		
		int cat_trick = get_cat_index_from_key_combo( key_combo );
		if ( cat_trick != -1 )
		{
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			Dbg_Assert( skate_mod );
			Obj::CSkater* pSkater = skate_mod->GetLocalSkater();
			if ( pSkater )
			{
				Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[cat_trick];
				Dbg_Assert( pCreatedTrick );
				const char* p_trick_name;
				pCreatedTrick->mp_other_params->GetString( CRCD( 0xa1dc81f9, "name" ), &p_trick_name, Script::ASSERT );
				trick_name_checksum = Script::GenerateCRC( p_trick_name );
			}
		}
		else
		{
			return false;
		}
	}
	
	return VerifyTrickMatch(index, trick_name_checksum, spin_mult, require_perfect);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Score::GetNumberOfNonGapTricks()
{
	int non_gap_trick_count = 0;
	
	for ( int i = 0; i < m_currentTrick; i++ )
	{
		if (!TrickIsGap(i))
		{
			non_gap_trick_count++;
		}
	}
	
	return non_gap_trick_count;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// find a trick by name, rather than by key combo
int Score::GetCurrentNumberOfOccurrencesByName( uint32 trickTextChecksum, int spin_mult, bool require_perfect, int num_taps )
{
	// resolve multiple tap tricks
	if ( num_taps > 1 )
	{
		trickTextChecksum = get_trickname_checksum_from_trick_and_taps( trickTextChecksum, num_taps );
	}
	
	// uint32 trick_name_checksum = Script::GenerateCRC( trick_string );
	int num = CountTrickMatches( trickTextChecksum, spin_mult, require_perfect );
/*	for ( int i = 0; i < m_currentTrick; i++ )
	{
		if ( VerifyTrickMatch( i, trickTextChecksum, spin_mult, require_perfect ) )
			num++;
	}*/
	return num;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// searches by key combo
int Score::GetCurrentNumberOfOccurrences( uint32 key_combo, int spin_mult, bool require_perfect, int num_taps )
{
	// get the key mapping and trick name
	uint32 trick_name_checksum = get_trickname_checksum_from_key_combo( key_combo, num_taps );
	if ( trick_name_checksum == 0 )
	{
		// check for cat trick		
		int cat_trick = get_cat_index_from_key_combo( key_combo );
		if ( cat_trick != -1 )
		{
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			Dbg_Assert( skate_mod );
			Obj::CSkater* pSkater = skate_mod->GetLocalSkater();
			if ( pSkater )
			{
				Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[cat_trick];
				Dbg_Assert( pCreatedTrick );
				const char* p_trick_name;
				pCreatedTrick->mp_other_params->GetString( CRCD( 0xa1dc81f9, "name" ), &p_trick_name, Script::ASSERT );
				trick_name_checksum = Script::GenerateCRC( p_trick_name );
			}
		}
		else
		{
			return 0;
		}
	}

	int num = this->CountTrickMatches( trick_name_checksum, spin_mult, require_perfect );
/*    for ( int i = 0; i < m_currentTrick; i++ )
    {
        if ( this->VerifyTrickMatch( i, trick_name_checksum, spin_mult, require_perfect) )
			num++;
    }
*/
	return num;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Score::GetPreviousNumberOfOccurrencesByName( uint32 trickTextChecksum, int spin_mult, int num_taps )
{
	// resolve multiple tap tricks
	if ( num_taps > 1 )
	{
		trickTextChecksum = get_trickname_checksum_from_trick_and_taps( trickTextChecksum, num_taps );
	}
	
	int num = 0;
    bool justSawTrick = false;
    for ( int i = 0; i < m_currentTrick; i++ )
    {
        // printf("checking %s\n", m_infoTab[i].trickNameText );
        if ( m_infoTab[i].id == trickTextChecksum )
        {
            // printf("found trick\n");
            if ( spin_mult == 0 )
			{
				num++;
				justSawTrick = true;
			}
			else if ( spin_mult == m_infoTab[i].spin_mult_index )
			{
				num++;
				justSawTrick = true;
			}
            
        }
        else 
        {
            if ( ( m_infoTab[i].flags & vGAP ) && justSawTrick )
            {
                // printf("found gap after trick\n");
            }
            else
            {
                // if ( justSawTrick ) printf("found a non-gap trick after the trick\n");
                justSawTrick = false;
            }
        }
    }
    if ( justSawTrick )
    {
        // printf("followed by gaps\n");
        num--;
    }
    // printf("found %i previous occurences\n", num );
    return num;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Score::GetPreviousNumberOfOccurrences( uint32 key_combo, int spin_mult, int num_taps )
{
	// printf ( "GetPreviousNumberOfOccurrences( %s, %i, %i )\n", Script::FindChecksumName( key_combo ), spin_mult, num_taps );
	uint32 trick_name_checksum = get_trickname_checksum_from_key_combo( key_combo, num_taps );
	
	if ( trick_name_checksum == 0 )
	{
		// check for cat trick		
		int cat_trick = get_cat_index_from_key_combo( key_combo );
		if ( cat_trick != -1 )
		{
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			Dbg_Assert( skate_mod );
			Obj::CSkater* pSkater = skate_mod->GetLocalSkater();
			if ( pSkater )
			{
				Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[cat_trick];
				Dbg_Assert( pCreatedTrick );
				const char* p_trick_name;
				pCreatedTrick->mp_other_params->GetString( CRCD( 0xa1dc81f9, "name" ), &p_trick_name, Script::ASSERT );
				trick_name_checksum = Script::GenerateCRC( p_trick_name );
			}
		}
		else
		{
			return 0;
		}
	}
	
	int num = 0;
    bool justSawTrick = false;
    for ( int i = 0; i < m_currentTrick; i++ )
    {
        // printf("checking %s\n", m_infoTab[i].id );
        if ( m_infoTab[i].id == trick_name_checksum )
        {
            // printf("found trick\n");
            if ( spin_mult == 0 )
			{
				num++;
				justSawTrick = true;
			}
			else if ( spin_mult == m_infoTab[i].spin_mult_index )
			{
				num++;
				justSawTrick = true;
			}            
        }
        else 
        {
            if ( ( m_infoTab[i].flags & vGAP ) && justSawTrick )
            {
                // printf("found gap after trick\n");
            }
            else
            {
                // if ( justSawTrick ) printf("found a non-gap trick after the trick\n");
                justSawTrick = false;
            }
        }
    }
    if ( justSawTrick )
    {
        // printf("followed by gaps\n");
        num--;
    }
    // printf("found %i previous occurences\n", num );
    return num;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// utility used by GetCurrentNumberOfOccurrences( Script::CArray* )
inline void GetTrickInfoFromTrickArray( Script::CArray* pTricks, int trick_array_index, uint32 *p_trick_name_checksum, int *p_spin, bool *p_require_perfect, int *p_num_taps )
{
	// reset spin and perfect first
	*p_spin = 0;
	*p_require_perfect = false;

	int num_taps = 1;
	uint32 key_combo;

	if ( pTricks->GetType() == ESYMBOLTYPE_STRUCTURE )
	{
		// grab the key combo and any spin or perfect modifiers from the struct
		Script::CStruct* pSubStruct = pTricks->GetStructure( trick_array_index );
		pSubStruct->GetChecksum( CRCD( 0x95e16467, "KeyCombo" ), &key_combo, Script::ASSERT );
		pSubStruct->GetInteger( CRCD( 0xa4bee6a1, "num_taps" ), &num_taps, Script::NO_ASSERT );
		pSubStruct->GetInteger( CRCD(0xa4bee6a1,"num_taps"), &*p_num_taps, Script::NO_ASSERT );
		*p_trick_name_checksum = get_trickname_checksum_from_key_combo( key_combo, num_taps );
		if ( pSubStruct->GetInteger( CRCD( 0xedf5db70, "spin" ), &*p_spin, Script::NO_ASSERT ) )
		{
			Dbg_MsgAssert( *p_spin % 180 == 0, ( "Bad spin value %i in gap tricks structure", *p_spin ) );
			*p_spin = *p_spin / 180;
			*p_require_perfect = pSubStruct->ContainsFlag( CRCD( 0x1c39f1b9, "perfect" ) );
		}
		// printf("\tlooking for key_combo %s\n", Script::FindChecksumName( key_combo ) );
	}
	else
	{
		Dbg_MsgAssert( pTricks->GetType() == ESYMBOLTYPE_NAME, ( "Tricks array has wrong type" ) );
		*p_trick_name_checksum = get_trickname_checksum_from_key_combo( pTricks->GetChecksum( trick_array_index ) );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Score::GetCurrentNumberOfOccurrences( Script::CArray* pTricks, int start_trick_index )
{
	Dbg_Message("DEPRECATED! Tell Brad if you need this.\n");
	return 0;


/*	Dbg_MsgAssert( start_trick_index <= m_currentTrick, ( "GetCurrentNumberOfOccurrences got bad start_trick_index of %i - m_currentTrick = %i", start_trick_index, m_currentTrick ) );
	
	int trick_array_size = pTricks->GetSize();
	int total = 0;
	
	// parse through the current list of tricks
	for ( int trick_array_index = 0; trick_array_index )
	{
		
	}
	for ( int i = start_trick_index; i < m_currentTrick; i++ )
	{
		// printf("\tcurrent trick = %s\n", Script::FindChecksumName( m_infoTab[i].id ) );
		int trick_array_index = 0;
		uint32 trick_name_checksum;
		int spin = 0;
		bool require_perfect = false;
		int num_taps = 0;
		GetTrickInfoFromTrickArray( pTricks, trick_array_index, &trick_name_checksum, &spin, &require_perfect, &num_taps );

		while ( VerifyTrickMatch( i + trick_array_index, trick_name_checksum, spin, require_perfect ) )
		{
			// printf("\tfound a match\n");
			// if we've matched as many tricks as are in the list, we know we've
			// found a complete match
			if ( trick_array_index == trick_array_size - 1 )
			{
				total++;
				break;
			}
			trick_array_index++;
			GetTrickInfoFromTrickArray( pTricks, trick_array_index, &trick_name_checksum, &spin, &require_perfect, &num_taps );
		}
	}
*/
	// printf("found %i occurrence(s)\n", total);
	// return total;
}

void	Score::RepositionMeters( void )
{
	setup_balance_meter_stuff();
}

void Score::ResetComboRecords()
{
	m_longestCombo = 0;
	m_bestCombo = 0;
	m_bestGameCombo = 0;
}

/******************************************************************/
/* Handle network scoring messages                                */
/*                                                                */
/******************************************************************/

int	Score::s_handle_score_message ( Net::MsgHandlerContext* context )
{
	GameNet::MsgEmbedded* p_msg = (GameNet::MsgEmbedded*) ( context->m_Msg );
	Mdl::Score* pScore = ((Obj::CSkater*) ( context->m_Data ))->GetScoreObject();
	
	switch ( p_msg->m_SubMsgId )
	{
		case GameNet::SCORE_MSG_ID_LOG_TRICK_OBJECT:
		{
			GameNet::MsgScoreLogTrickObject* log_msg = (GameNet::MsgScoreLogTrickObject*) ( context->m_Msg );
			GameNet::Manager* gamenet_man =  GameNet::Manager::Instance();

			if ( log_msg->m_GameId == gamenet_man->GetNetworkGameId())
			{
				pScore->LogTrickObject( log_msg->m_OwnerId, log_msg->m_Score, log_msg->m_NumPendingTricks, log_msg->m_PendingTrickBuffer, false );
			}
			break;
		}
	}
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Score::TrickIsGap( int trickIndex )
{
	Dbg_MsgAssert( trickIndex >= 0 && trickIndex < m_currentTrick, ( "trickIndex out of range" ) );
	return ( m_infoTab[trickIndex].flags & vGAP );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Score::set_special_is_active ( bool is_active )
{
	if (is_active != m_specialIsActive)
	{
		m_specialIsActive = is_active;
		
		Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetSkaterById(m_skaterId);
		if( pSkater )
		{
			pSkater->BroadcastEvent(m_specialIsActive ? CRCD(0xd0cd8081, "SkaterEnterSpecial") : CRCD(0x7ccf5ee2, "SkaterExitSpecial"));
		}
	}
}


void Score::reset_robot_detection()
{
	m_num_robot_landed = 0;
	m_num_robot_unique = 0;
	for (int i=0;i<MAX_ROBOT_NODES;i++)
	{
		m_robot_detection_count[i] = 0;
	}
	reset_robot_detection_combo();

}

void Score::reset_robot_detection_combo()
{
	m_num_robot_landed_combo = 0;
	m_num_robot_unique_combo = 0;
	m_num_robot_first_combo = 0;
	for (int i=0;i<MAX_ROBOT_NODES;i++)
	{
		m_robot_detection_count_combo[i] = 0;
	}
	m_robot_rail_mult = 1.0f;

}
	
void Score::UpdateRobotDetection(int node)
{

// Mick:  Most levels fit inside the curretn 2500 limit
// so,rather than mess with memory, I'm just going to ignore those nodes that do not fit
// this gives us a very small chance of there being some area that is slightly more
// exploitable than others.   However, since the robot line detection does not have a very noticable effect
// there will basically be no impact on the overall game experience.
//	Dbg_MsgAssert(node < MAX_ROBOT_NODES,("robot node %d too big, tell Mick",node));
	if (node >= MAX_ROBOT_NODES)
	{
		m_robot_rail_mult = 1.0f;
		return;
	}



	m_num_robot_landed++;
	m_num_robot_landed_combo++;
	if (m_robot_detection_count[node] == 0)
	{	
		m_num_robot_unique++;
		m_num_robot_first_combo++;
	}
	if (m_robot_detection_count[node] < 255)
	{
		m_robot_detection_count[node]++;
	}
	
	if (m_robot_detection_count_combo[node] == 0)
	{	
		m_num_robot_unique_combo++;
	}
	if (m_robot_detection_count_combo[node] < 255)
	{
		m_robot_detection_count_combo[node]++;
	}
	
//	GetRobotMult();

								  
	int 	kick_in = Script::GetInteger(CRCD(0x6ee127b7,"robot_kick_in_count"));							  

	if (m_robot_detection_count_combo[node] > kick_in)
	{
		int ramp = 20;
		// The first time you go over robot_kick_count, we want 1/2, everything before that will be 1.0f
		m_robot_rail_mult = 1.0f/(m_robot_detection_count_combo[node]-(kick_in)+1) * ( (float)(ramp + 1 - m_robot_detection_count[node])/(float)ramp);
		// Might go small or negative after a really long time, so clamp it
		m_robot_rail_mult = Mth::Max(0.1f, m_robot_rail_mult);	
	}	
	else
	{
		m_robot_rail_mult = 1.0f;
	}
	

}


float	Score::GetRobotMult()
{
	
	printf ("landed = %3d, unique = %3d .. ",m_num_robot_landed,m_num_robot_unique);
	printf ("combo landed = %3d, unique = %3d, first = %d\n",m_num_robot_landed_combo,m_num_robot_unique_combo,m_num_robot_first_combo);
	
//	float unique_ratio = (float)m_num_robot_unique_combo/(float)m_num_robot_landed_combo;
//	float first_ratio = (float)m_num_robot_first_combo/(float)m_num_robot_landed_combo;
//	float punish = 1.0f - 0.5f * logf(robot);
//	float reward = logf(m_num_robot_unique_combo) - logf(robot);
//	printf ("robot = %2.3f, punish = x%2.3f, reward = x%2.3f\n",robot, punish, reward);	

	int	min_robot_effect = 3;  // number of repetitions at which "robot" adjustment begins

	float sub = 0.0f;
	// Scale any effect by the size of the combo, beyond the minimun needed to take effect	
//	float robot_scale = 0.0f;
	if ((m_num_robot_landed_combo - m_num_robot_unique_combo) > min_robot_effect && m_num_robot_unique_combo < m_num_robot_landed_combo)
	{
#	if !defined( __PLAT_NGC__ ) || ( defined( __PLAT_NGC__ ) && !defined( __NOPT_FINAL__ ) )
		float landed = m_num_robot_landed_combo - min_robot_effect;
		float dupes = m_num_robot_landed_combo - m_num_robot_unique_combo - min_robot_effect;
//		float first =  m_num_robot_first_combo;
		
//		robot_scale = 0.1f * logf(landed);   // with min of 10, 10=0, 50 =1.6, 100 = 1.95 
		// find an amount to subtract from a 1.0f multiplier, based on how repeditive this line is
//		float dupe_sub = (dupes/landed); 
//		float first_sub = 0.0f;	//0.05f * logf(landed - first); 
//		float sub = robot_scale * (dupe_sub + first_sub);
		float sub = dupes/landed;
//		printf ("(dupe = %2.3f + first = %2.3f) * scale %2.3f == sub %2.3f\n", dupe_sub, first_sub, robot_scale,  sub);
		printf ("sub = %2.3f\n",sub);
#endif		// __NOPT_FINAL__
	}
	
	float mult = 1.0f - sub;
	if (mult > 1.0f)
	{
		mult = 1.0f;
	}
	if (mult < 0.2f)
	{
		mult = 0.2f;
	}
	printf ("mult = %2.3f\n",mult);
	return mult;
}	



} // namespace HUD




