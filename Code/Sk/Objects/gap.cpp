//////////////////////////////////////////////////////////
// Gap.cpp
//
// Functions for CGap and CGapMan
//
// A "Gap" in the game is a pair of events that cause something to happen
// Typically this is jumping across something, such as a gap between two buildings
// but might also be grinding a rail for a certain distance
// or doing something specific, like doing a kickflip over a barrel.
//
// The aim of the gap system is to provide lots of flexibility in setting up
// these pairs of events, and also to allow the user to set gaps quickly and simply
//
// Gaps are set up via script commands
// so they can easily be inserted in trigger scripts
// the simplest might look like:
/*

script TRG_Obj010
	StartGap GapId = JumpOverLog Cancel = [CANCEL_GROUND CANCEL_RAIL CANCEL_WALLRIDE] Expire = 20
endscript


script TRG_Obj012
	EndGap GapId = JumpOverLog Score = 100 Text = "You jumped over a log" Script = ScreenFlash01
endscript

Using the "StartGap" command, you can set up as many gaps as you like, starting from
a particular event.

For that gap, you specify what cancels the gap, if nothing is specified, then this should default
to CANCEL_GROUND.  These conditions are checked every frame

All gaps are cancelled by a bail (perhaps unless we have CANCEL_BAIL_NOT set????)

We can also have a specific "CancelGap"  command, for fuller control.

The optional "Expire" parameter says when the gap expires.  By default gaps do not expire				   

The "EndGap" command will check to see if the gap with the specified GapID had been started
(and not cancelled, or expired), and will then Execute the gap, having an optional Score,
text (defaults to "Gap") and an optional script, which is executed.

If there is no score, then it will not count as a trick, and you just get the script executing.





*/


#include <core/defines.h>
#include <core/singleton.h>
#include <core/math.h>
#include <sk/objects/gap.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <sk/scripting/nodearray.h>
#include <gfx/debuggfx.h>
#include <sk/modules/skate/skate.h>
#include <gel/modman.h>
#include <core/string/stringutils.h>

#include <sk/objects/skatercareer.h>
#include <sk/modules/skate/goalmanager.h>

namespace Obj
{

/*
	a CGapCheck is a single entry representing a gap with a unique name
	these are stored in a list: Mdl::Skate::Instance()->m_gapcheck_list
	which is re-created individually for each level whan the level is loaded
	
	This list is used to create the "View Gaps" menu (from gamemenu.q, script launch_gap_menu)
	
	The gap list is updated in game with CGapCheck::GetGap() whena  gap is got
	
	TODO -> add LandPendingGaps to "solidlfy" gaps.  The gap should be flagged
	as "pending" when you "get" it (with an EndGap).  But it should only
	be really flagged when you sucessfully land the combo
	and it should be cleared if you bail while doing the gap 


*/	

// Constructors are needed for types derived from the list node class
// so the "this" pointer can be passed down, so it ends up
// in the data member

CGapCheck::CGapCheck(): Lst::Node< CGapCheck > ( this )
{	
	m_got = 0;
	m_got_pending = 0;
	m_valid = true;			// newly created gaps are always valid
}

int				CGapCheck::Got()
{
	return m_got;
}

void				CGapCheck::GetGap()
{
	m_got+= m_got_pending;
	m_got_pending = 0;
}

void				CGapCheck::GetGapPending()
{
	m_got_pending += 1;
}

void				CGapCheck::UnGetGapPending()
{
	m_got_pending = 0;
}

int				CGapCheck::GotGapPending()
{
	 return m_got_pending;
}


void				CGapCheck::UnGetGap()
{
	m_got = false;
}

const char	*			CGapCheck::GetText()
{
	return m_name.getString();
}

void				CGapCheck::SetText(const char *p)
{
	m_name = p;
}

void CGapCheck::SetInfoForCamera(Mth::Vector pos, Mth::Vector dir)
{
	m_skater_start_pos=pos;
	m_skater_start_dir=dir;
}

void				CGapCheck::SetCount(int count)
{
	m_got = count;
}

void 				CGapCheck::SetValid(bool valid)
{
	m_valid = valid;
}

void				CGapCheck::Reset()
{
	m_got = 0;
	m_got_pending = 0;
	m_valid = true;			// newly created gaps are always valid
}


///////////////////////////////////////////////////////////////////////

CGap::CGap(): Lst::Node< CGap > ( this )
{
    m_trickChecksum = 0;
    m_numTrickOccurrences = 0;
	m_spinMult = 0;
	m_requirePerfect = false;
	mp_tricks = new Script::CArray();
	m_startGapTrickCount = 0;
	m_numTaps = 0;
}

CGap::~CGap()
{
	if ( mp_tricks )
		Script::CleanUpArray( mp_tricks );
	delete mp_tricks;
}

CGapTrick::CGapTrick(): Lst::Node< CGapTrick > ( this )
{
}

void CGapTrick::GetGapTrick()
{
    m_got = true;
}


// delete all gapchecks
// used when we load a level						  
// K: Also used when choosing test play in the park editor, just before adding all the park editor
// gaps.
void	CGapChecklist::FlushGapChecks()
{
	CGapCheck *pOther = (CGapCheck*)m_gapcheck_list.FirstItem();
	while (pOther != NULL)
	{
		delete pOther;
		pOther = (CGapCheck*)m_gapcheck_list.FirstItem();
	}
	m_valid = false;
}

// Given a name and a count, then add a GapCheck to the list
// if it is already in the list, then just set it valid (leaving the count)						   
// (that is so when we re-scan the level's gaps after loading a level, we can remove any
// that do not exist in the level any more)
void	CGapChecklist::AddGapCheck(const char *p_name, int count, int score)
{
	
	m_valid = true;	 							// the list becomes valid when at least one gapcheck is in it
	
	CGapCheck *pOther = (CGapCheck*)m_gapcheck_list.FirstItem();
	CGapCheck *pAppend = NULL;				// last item we find that is lower than this
	while (pOther)						 		
	{
		if (!strcmp(pOther->GetText(),p_name) && pOther->GetScore() == score)   // if it matches an existing gap in both text and score
		{
			pOther->SetValid(true);				// then set that one valid (as we might be pruning later)
			return;								// then break out
		}		  
		
		if (pOther->GetScore() < score)
		{
			pAppend = pOther;
		}
		else
		{
			if (pOther->GetScore() == score && Str::Compare(pOther->GetText(),p_name)<0)
			{
				pAppend = pOther;
			}
		}
		  		
		pOther = (CGapCheck*)pOther->GetNext();
	}

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap()); 
	
	CGapCheck* pGapCheck = new CGapCheck;		// create new gap		
	pGapCheck->UnGetGap();						// say not got it yet

	pGapCheck->SetText(p_name);			   		// set the text for it
	pGapCheck->SetCount(count);			   		// and the count
	pGapCheck->SetScore(score);			   		// and the count
	if (pAppend)
	{
		// Append after the last node that is smaller or equal to than this
		pAppend->Append( pGapCheck );
	}
	else
	{
		// add to the ehad of the list if it's smaller than everything
		m_gapcheck_list.AddToHead( pGapCheck );	 
	}
	
	Mem::Manager::sHandle().PopContext();
}

																					  
void CGapChecklist::got_gap(Script::CStruct *pScriptStruct, const uint8 *pPC)
{
	int score=0;
	pScriptStruct->GetInteger(CRCD(0xcd66c8ae,"score"),&score);
	
	const char *p_name="";
	pScriptStruct->GetText(CRCD(0xc4745838,"text"),&p_name);

	// only add gaps with some text, and a score
	if (score && p_name[0])
	{
		AddGapCheck(p_name,0,score);
	}	
	else
	{
		if (Script::GetInteger("PrintGaps"))
		{
			printf ("(Name '%s' Score %d - not in checklist)\n",p_name,score); // we don't add gaps with no name or score
		}
	}
}


static CGapChecklist * p_current;

// slightly dodgy callback needed to call memmber function
static void Gap_GotGap(Script::CStruct *pScriptStruct, const uint8 *pPC)
{
	p_current->got_gap(pScriptStruct,pPC);		   
}


CGapChecklist::CGapChecklist()
{
	m_valid = false;
}


//  Find all the gaps in the level
// note, we scan regardless, so during development, we 
// can add gaps to an already saved game
// (might want to also remove gaps if they have been deleted)
void CGapChecklist::FindGaps()
{	
	Dbg_Message("Looking for gaps ...\n");
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap()); 
	
	p_current = this;	  			// store curent 'this' pointer for the callback
	m_valid = true;					// say that we have looked for gaps in this level
	
	// Maybe - scan all gaps, and flag them as not found
	
	
	if (Script::GetInteger("PrintGaps"))
	{
		printf("\nGAP LIST\n--------------------------------------\n");
	}

	SkateScript::ScanNodeScripts(CRCD(0x2ca8a299,"TriggerScript"), CRCD(0xe5399fb2,"EndGap"), Gap_GotGap);	
	
	// Maybe - and then re-scan them here, and delete them if not flagged by got_gap
	
	Mem::Manager::sHandle().PopContext(); 
	Dbg_Message("Finished looking for gaps.\n");
}

// K: Given a gap name, this will return its index withing the gap checklist, or -1 if not found.
// Used by the end_gap function in skatergapcomponent when a created gap goal is active.
int CGapChecklist::GetGapChecklistIndex(const char *p_name)
{
	int index=0;
	CGapCheck *p_search =  (CGapCheck*) m_gapcheck_list.FirstItem();
	while (p_search)						 		
	{
		if (!strcmp(p_search->GetText(),p_name))
		{
			return index;
		}	
		p_search = (CGapCheck*)p_search->GetNext();
		++index;
	}
	return -1;
}

void CGapChecklist::SetInfoForCamera(const char *p_name, Mth::Vector pos, Mth::Vector dir)
{
	CGapCheck *p_search =  (CGapCheck*) m_gapcheck_list.FirstItem();
	while (p_search)						 		
	{
		if (!strcmp(p_search->GetText(),p_name))
		{
			p_search->SetInfoForCamera(pos,dir);
			break;
		}	
		p_search = (CGapCheck*)p_search->GetNext();
	}
}

void CGapChecklist::GetGapByText(const char *p_text)
{
	CGapCheck *pOther =  (CGapCheck*) m_gapcheck_list.FirstItem();
	while (pOther)						 		
	{
		if (!strcmp(pOther->GetText(),p_text))   // if it matches and existing gap 
		{
			pOther->GetGapPending();	// flag it as pending
		
			break;								// then break out
		}	
		else
		{
		}
		pOther = (CGapCheck*)pOther->GetNext();
	}
}

void CGapChecklist::AwardPendingGaps()
{
	int gap_index=0;
	CGapCheck *pOther =  (CGapCheck*) m_gapcheck_list.FirstItem();
	char ran_got_gap_for_first_time = 0;

	while (pOther)						 		
	{
		if(pOther->GotGapPending())
		{
			// K: The gap is being awarded, so let the goal manager know that the gap has
			// just been got in case a create-gap-goal is active.
			Game::CGoalManager* p_goal_manager = Game::GetGoalManager();
			Dbg_MsgAssert(p_goal_manager,("NULL p_goal_manager ?"));
			if (p_goal_manager->CreatedGapGoalIsActive())
			{
				// Let the goal manager know that this gap has just been gone through so that
				// it can tick it off it's list of required gaps, if it is one of them.
				p_goal_manager->GetCreatedGoalGap(gap_index);
			}
			
			// The code below is commented out so multiple new gap messages can be displayed
			if ( pOther->Got() == 0 )//&& !ran_got_gap_for_first_time )
			{
				++ran_got_gap_for_first_time;
				
				// this is the first time we've gotten the gap, so give them a message
				Script::CStruct* pScriptParams = new Script::CStruct();

				if( ran_got_gap_for_first_time > 1)
					pScriptParams->AddInteger( Script::GenerateCRC("multiple_new_gaps"), ( int ) ran_got_gap_for_first_time  );

				pScriptParams->AddString( CRCD(0xc69ce365,"gap_text"), pOther->GetText() );
				Script::RunScript( CRCD(0x318eaf64,"got_gap_for_first_time"), pScriptParams );
				delete pScriptParams;
			}
			
			pOther->GetGap();
		}
		pOther->UnGetGapPending();			
		pOther = (CGapCheck*)pOther->GetNext();
		++gap_index;
	}
}

void CGapChecklist::AwardAllGaps()
{

	CGapCheck *pOther =  (CGapCheck*) m_gapcheck_list.FirstItem();

	while (pOther)						 		
	{  	
		pOther->SetCount( pOther->Got() + 1 );			
		pOther = (CGapCheck*)pOther->GetNext();
	}
}

void CGapChecklist::ClearPendingGaps()
{
	CGapCheck *pOther =  (CGapCheck*) m_gapcheck_list.FirstItem();
	while (pOther)						 		
	{
		pOther->UnGetGapPending();			
		pOther = (CGapCheck*)pOther->GetNext();
	}
}

int	CGapChecklist::NumGaps()
{
	return m_gapcheck_list.CountItems();
}


bool CGapChecklist::GotAllGaps()
{
	CGapCheck *pOther =  (CGapCheck*) m_gapcheck_list.FirstItem();
	while (pOther)						 		
	{
		if ( pOther->Got() == 0 )
			return false;
		pOther = (CGapCheck*)pOther->GetNext();
	}
	return true;
}

void CGapChecklist::Reset()
{
	CGapCheck *pOther =  (CGapCheck*) m_gapcheck_list.FirstItem();
	while (pOther)						 		
	{
		pOther->Reset();
		pOther = (CGapCheck*)pOther->GetNext();
	}
}

bool CGapChecklist::ScriptGetLevelGapTotals( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGapCheck *pGap =  (CGapCheck*) m_gapcheck_list.FirstItem();
	
	int total_gaps = 0;
	int total_got = 0;
	while (pGap)						 		
	{
		CGapCheck* pNext = (CGapCheck*)pGap->GetNext();
		total_gaps++;
	
		if ( pGap->Got() )
			total_got++;

		pGap = (CGapCheck*)pNext;
	}
	pScript->GetParams()->AddInteger( "num_gaps", total_gaps );
	pScript->GetParams()->AddInteger( "num_collected_gaps", total_got );
	return true;
}


bool CGapChecklist::ScriptAddGapsToMenu( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 choose_script=CRCD(0xf7e902c,"nullscript");
	pParams->GetChecksum(CRCD(0xa7d7c610,"choose_script"),&choose_script);


	CGapCheck *pGap =  (CGapCheck*) m_gapcheck_list.FirstItem();
	
	Script::CStruct* p_gap_params;
	int entry = 0;
	while (pGap)						 		
	{
		p_gap_params = new Script::CStruct;

		// check if this is the first or last gap
		CGapCheck* pNext = (CGapCheck*)pGap->GetNext();
		if ( entry == 0 )
			p_gap_params->AddChecksum( NONAME, CRCD( 0x171665d5, "first_item" ) );
		else if ( !pNext )
			p_gap_params->AddChecksum( NONAME, CRCD( 0x76cf1ebd, "last_item" ) );

		p_gap_params->AddInteger( "gap_score", pGap->GetScore() );
		p_gap_params->AddString( "gap_name", pGap->GetText() );
		p_gap_params->AddInteger( "times", pGap->Got() );

		p_gap_params->AddInteger( "gap_number", entry );
		
		Mth::Vector v=pGap->GetSkaterStartPos();
		p_gap_params->AddVector(CRCD(0x26a4e85a,"skater_start_pos"),v[X],v[Y],v[Z]);
		v=pGap->GetSkaterStartDir();
		p_gap_params->AddVector(CRCD(0x1cd674e6,"skater_start_dir"),v[X],v[Y],v[Z]);
		
		p_gap_params->AddChecksum(CRCD(0xa7d7c610,"choose_script"),choose_script);
		
		Script::RunScript( "gap_menu_add_item", p_gap_params );
		delete p_gap_params;

		pGap = (CGapCheck*)pNext;
		entry++;
	}

	if (entry == 0)
	{
		return false;
	}
	return true;
}

bool CGapChecklist::ScriptCreateGapList( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGapCheck *p_gap =  (CGapCheck*) m_gapcheck_list.FirstItem();

	int num_gaps=0;
	while (p_gap)						 		
	{
		p_gap = (CGapCheck*)p_gap->GetNext();
		++num_gaps;
	}	
	pScript->GetParams()->AddInteger(CRCD(0x7f5b3d4d,"array_size"),num_gaps);
	
	if (!num_gaps)
	{
		pScript->GetParams()->AddInteger(CRCD(0x6a8a0061,"num_gaps_got"),0);
		return false;
	}
		
	Script::CArray *p_gap_list=new Script::CArray;
	p_gap_list->SetSizeAndType(num_gaps,ESYMBOLTYPE_STRUCTURE);
	
	p_gap =  (CGapCheck*) m_gapcheck_list.FirstItem();
	
	int gap_number = 0;
	int num_gaps_got=0;
	while (p_gap)						 		
	{
		if (p_gap->Got())
		{
			++num_gaps_got;
		}
			
		Script::CStruct* p_gap_params = new Script::CStruct;

		p_gap_params->AddInteger( "gap_score", p_gap->GetScore() );
		p_gap_params->AddString( "gap_name", p_gap->GetText() );
		p_gap_params->AddInteger( "times", p_gap->Got() );

		p_gap_params->AddInteger( "gap_number", gap_number );
		
		Mth::Vector v=p_gap->GetSkaterStartPos();
		p_gap_params->AddVector(CRCD(0x26a4e85a,"skater_start_pos"),v[X],v[Y],v[Z]);
		v=p_gap->GetSkaterStartDir();
		p_gap_params->AddVector(CRCD(0x1cd674e6,"skater_start_dir"),v[X],v[Y],v[Z]);
		
		p_gap_list->SetStructure(gap_number,p_gap_params);
		p_gap = (CGapCheck*)p_gap->GetNext();
		++gap_number;
	}

	pScript->GetParams()->AddArrayPointer(CRCD(0x737449de,"GapList"),p_gap_list);
	pScript->GetParams()->AddInteger(CRCD(0x6a8a0061,"num_gaps_got"),num_gaps_got);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | AddgapsToMenu | adds all gaps to the menu
bool ScriptAddGapsToMenu( Script::CStruct* pParams, Script::CScript* pScript )
{
	
	bool result = 	Mdl::Skate::Instance()->GetGapChecklist()->ScriptAddGapsToMenu(pParams,pScript); 
	
	
	return result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | CreateGapList | Similar to AddGapsToMenu, but instead creates an array of structures called GapList
// and adds it to the calling script's parameters.
bool ScriptCreateGapList( Script::CStruct* pParams, Script::CScript* pScript )
{
	return Mdl::Skate::Instance()->GetGapChecklist()->ScriptCreateGapList(pParams,pScript); 
}

// @script | GetLevelGapTotals | returns the total number of gaps and the number
// of gaps that have been completed in the level (num_gaps, num_completed_gaps)
// @parm int | level | the level number
bool ScriptGetLevelGapTotals( Script::CStruct* pParams, Script::CScript* pScript )
{
	int level;
	pParams->GetInteger( "level", &level, Script::ASSERT );
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkaterCareer* pCareer = skate_mod->GetCareer();
	
	bool result = pCareer->GetGapChecklist( level )->ScriptGetLevelGapTotals( pParams, pScript );
	return result;
}

// @script | GiveAllGaps | "Gets" all the gaps in level or current level if none is specified
// @parm int | level | the level number | optional
bool ScriptGiveAllGaps( Script::CStruct* pParams, Script::CScript* pScript )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkaterCareer* pCareer = skate_mod->GetCareer();

	int level;
	if(!pParams->GetInteger( "level", &level, Script::NO_ASSERT ))
		level = pCareer->GetLevel();
	
	pCareer->GetGapChecklist( level )->AwardAllGaps();
	return true;
}


CGapCheck *CGapChecklist::get_indexed_gapcheck(int gap_index)
{

	CGapCheck *pGap =  (CGapCheck*) m_gapcheck_list.FirstItem();
	int 	entry = 0;
	while (pGap)						 		
	{
		if (entry == gap_index)
		{
			return pGap;		
		}
		pGap = (CGapCheck*)pGap->GetNext();
		entry++;
	}
	Dbg_MsgAssert(0,("Gap index %ds invalid\n",gap_index));	
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGapChecklist::ScriptGotGap( Script::CStruct* pParams, Script::CScript* pScript )
{
	int	gap_index = 0;
	pParams->GetInteger(NONAME,&gap_index,true);		 
	return get_indexed_gapcheck(gap_index)->Got();
}

const char *	CGapChecklist::GetGapText(int gap)
{
	return get_indexed_gapcheck(gap)->GetText();
}

int	CGapChecklist::GetGapCount(int gap)
{
	return get_indexed_gapcheck(gap)->Got();
}

int	CGapChecklist::GetGapScore(int gap)
{
	return get_indexed_gapcheck(gap)->GetScore();
}

void CGapChecklist::GetSkaterStartPosAndDir(int gap, Mth::Vector *p_skaterStartPos, Mth::Vector *p_skaterStartDir)
{
	CGapCheck *p_gap_check=get_indexed_gapcheck(gap);
	
	*p_skaterStartPos=p_gap_check->GetSkaterStartPos();
	*p_skaterStartDir=p_gap_check->GetSkaterStartDir();
}

// @script | GotGap | returns true if we have got the indicated gap
// this really only makes sense in the context of the gap menu (see AddGapsToMenu)
bool ScriptGotGap( Script::CStruct* pParams, Script::CScript* pScript )
{
	return Mdl::Skate::Instance()->GetGapChecklist()->ScriptGotGap(pParams,pScript); 
}


 
}	// end of Ojb namespace


