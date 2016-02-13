/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Objects (OBJ) 											**
**																			**
**	File name:		objects/SkaterCareer.cpp								**
**																			**
**	Created: 		21/5/01 - Mick (Based on SkaterApperance.cpp by gj)		**
**																			**
**	Description:	Skater career code 										**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <objects/skatercareer.h>
#include <objects/gap.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/string.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/gamenet/gamenet.h>


bool g_CheatsEnabled = false;

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Obj
{


static bool						s_is_competition[CSkaterCareer::vMAX_CAREER_LEVELS];
	
	
	

int CountOnes(uint32 v)
{
	

	int NumOnes=0;
	for (int i=0; i<32; ++i)
	{
		if (v&1)
		{
			++NumOnes;
		}
		v>>=1;
	}		
	return NumOnes;
}

int CountOnes(uint64 v)
{
	

	int NumOnes=0;
	for (int i=0; i<64; ++i)
	{
		if (v&1)
		{
			++NumOnes;
		}
		v>>=1;
	}		
	return NumOnes;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterCareer::CSkaterCareer()
{
   
	for (unsigned int i = 0; i<vMAX_CAREER_LEVELS; i++)
	{
		mp_gap_checklist[i] = new Obj::CGapChecklist;
		m_level_visited[i] = false;
	} 

	Init();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterCareer::~CSkaterCareer()
{
	for (unsigned int i = 0; i<vMAX_CAREER_LEVELS; i++)
	{
		delete mp_gap_checklist[i];
	} 	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCareer::Init( bool init_gaps /*= true*/ )
{	
	// Reset everything to "Not Done Yet"
	// Note: we do not currently reset the gap checklist here
	// I'm not sure at what point we would want to do that 
	
	for (unsigned int i = 0; i<vMAX_CAREER_LEVELS; i++)
	{
		m_goal_flags[i][0] = 0;
		m_level_flags[i][0] = 0;
		m_goal_flags[i][1] = 0;
		m_level_flags[i][1] = 0;
		s_is_competition[i] = false;
	}
	
	m_start_goal_flags[0] = 0;
	m_start_level_flags[0] = 0;
	m_start_goal_flags[1] = 0;
	m_start_level_flags[1] = 0;

	m_current_level = 0;
	
	if ( init_gaps )
	{
		for ( uint32 i = 0; i < vMAX_CAREER_LEVELS; i++ )
		{
			Obj::CGapChecklist* pGapChecklist = GetGapChecklist( i );
			pGapChecklist->Reset();
		}
	}
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCareer::Reset()
{
	
	
	Init(false);
	
	return true;
}


/******************************************************************/
// Return the gap checklist, either for the current level
// of for the level specified
					 
					 
Obj::CGapChecklist	*		CSkaterCareer::GetGapChecklist(int level)
{
	if (level == -1)
	{
		level = m_current_level;
	}

	Dbg_MsgAssert(level >= 0 && level < (int)vMAX_CAREER_LEVELS,("bad level number (%d) in  GetGapChecklist",level));
	return mp_gap_checklist[level];
}

bool CSkaterCareer::GotAllGaps()
{
	//Rulon: Made this function more robust to handle the changing level arrangement.
	Script::CArray *p_array = Script::GetArray(CRCD(0xeb1a4bc9,"levels_with_gaps"),Script::ASSERT);
	uint32 array_size = p_array->GetSize();

	for ( uint32 i = 0; i < vMAX_CAREER_LEVELS; ++i )
	{
		// Skip up levels we dont care about
		uint32 j = 0;
		for ( j = 0; j < array_size; ++j) 
		{
			uint32 check = p_array->GetChecksum(j);
			uint32 level_num = Script::GetInt(check ,Script::ASSERT);
			if( i == level_num )
			{
				//printf("level %i\n", i);
				break;
			}
		}
		
		if (j == array_size)
			continue;

		Obj::CGapChecklist* pGapChecklist = GetGapChecklist( i );
		
		// If this level should be taken into account but is unvisited
		if ( !this->HasVisitedLevel( i ) )
		{
			 //printf("haven't been to level %i\n", i);
			return false;
		}

		if ( !pGapChecklist->GotAllGaps() )
		{
			//printf("didn't get all the gaps in level %i\n", i);
			return false;
		}
		 //printf("got all gaps in level %i\n", i);
	}
	  return true;
	
}
					 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSkaterCareer::StartLevel(int level)
{
	

	if (level == -1)
	{
		level = m_current_level;
	}
	else
	{	
		m_current_level = level;
	}

	this->MarkLevelVisited( m_current_level );

	Dbg_MsgAssert(m_current_level < (int)vMAX_CAREER_LEVELS,( "level number %d too big",m_current_level)); 
	m_start_goal_flags[0]	= m_goal_flags[level][0];
	m_start_goal_flags[1]	= m_goal_flags[level][1];
	m_start_level_flags[0]	= m_level_flags[level][0];
	m_start_level_flags[1]	= m_level_flags[level][1];
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CSkaterCareer::GetLevel()
{
	
	return m_current_level;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCareer::MarkLevelVisited( int level_num )
{
	Dbg_MsgAssert( level_num >= 0 && level_num < (int)vMAX_CAREER_LEVELS, ( "MarkLevelVisited called with bad level num %i", level_num ) );
	m_level_visited[level_num] = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCareer::HasVisitedLevel( int level_num )
{
	Dbg_MsgAssert( level_num >= 0 && level_num < (int)vMAX_CAREER_LEVELS, ( "MarkLevelVisited called with bad level num %i", level_num ) );
	return m_level_visited[level_num];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSkaterCareer::SetGoal(int goal, int level)
{
	

	Dbg_MsgAssert(goal >= 0 && goal <256, ("goal %d out of range"));
	
	if (level == -1)
	{
		level = m_current_level;
	}
	
	m_goal_flags[level][goal>>5] |= (1<<(goal&31));
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSkaterCareer::UnSetGoal(int goal, int level)
{
	
	
	Dbg_MsgAssert(goal >= 0 && goal <256, ("goal %d out of range"));
	
	
	if (level == -1)
	{
		level = m_current_level;
	}
	
	m_goal_flags[level][goal>>5] &= ~(1<<(goal&31));
	m_start_goal_flags[goal>>5]  &= ~(1<<(goal&31));
	
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSkaterCareer::GetGoal(int goal, int level)
{
	
	Dbg_MsgAssert(goal >= 0 && goal <256, ("goal %d out of range"));
	if (level == -1)
	{
		level = m_current_level;
	}
	
	if (m_goal_flags[level][goal>>5] & (1<<(goal&31)))
	{
		return true;
	}
	
	return false;
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSkaterCareer::JustGotGoal(int goal)
{
	
	Dbg_MsgAssert(goal >= 0 && goal <256, ("goal %d out of range"));

	// if start goal flag is different from the current one, then we must
	// have just got the goal	
	if ((m_goal_flags[m_current_level][goal>>5] ^ m_start_goal_flags[goal>>5]) & (1<<(goal&31)) )
	{
		return true;
	}
	
	return false;

}

int	CSkaterCareer::CountGoalsCompleted(int level)
{
	
	if (level == -1)
	{
		level = m_current_level;
	}
	return CountOnes(m_goal_flags[level][0]) +  CountOnes(m_goal_flags[level][1]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	CSkaterCareer::CountTotalGoalsCompleted()
{
	
	int Goals=0;
	for (uint i=0; i<vMAX_CAREER_LEVELS; ++i)
	{
		if (!s_is_competition[i])
		{
			Goals+=CountGoalsCompleted(i);
		}
	}
	return Goals;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	CSkaterCareer::CountMedals()
{
	

	int Medals=0;
	for (uint i=0; i<vMAX_CAREER_LEVELS; ++i)
	{
		if (s_is_competition[i])
		{
			Medals += (CountGoalsCompleted(i) > 0) ? 1:0;		// count one per level
		}
	}
	return Medals;
}

// Returns 0 if no medal was won.
// If a medal was won, it returns 1+the best-medal number, where the medal number is whatever it is defined to be
// in goal_scripts.q
int CSkaterCareer::GetBestMedal(int Level)
{
	
	
	Dbg_MsgAssert(Level>=0 && Level<(int)vMAX_CAREER_LEVELS,("Bad Level of %d sent to GetBestMedal",Level));
	
	int Gold=Script::GetInteger("GOAL_GOLD");
	int Silver=Script::GetInteger("GOAL_SILVER");
	int Bronze=Script::GetInteger("GOAL_BRONZE");
	
	Dbg_MsgAssert(s_is_competition[Level],("Level %d is not a competition level",Level));
	
	if (m_goal_flags[Level][Gold>>5] & (1<<Gold))
	{
		return Gold+1;
	}
	else if (m_goal_flags[Level][Silver>>5] & (1<<Silver))
	{
		return Silver+1;
	}
	else if (m_goal_flags[Level][Bronze>>5] & (1<<Bronze))
	{
		return Bronze+1;
	}
	else
	{
		return 0;
	}	
	
	return 0;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSkaterCareer::GetCheat(uint32 cheat_checksum)
{
	g_CheatsEnabled = GetGlobalFlag(Script::GetInteger(cheat_checksum));
	// This should be the ONLY way we access the cheat flags, so we can override them in network games
	//if (GameNet::Manager::Instance()->InNetGame()) return false;
	return g_CheatsEnabled;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSkaterCareer::SetGlobalFlag(int flag)
{
	
	Dbg_MsgAssert(flag<512,("Flag %d is over 255",flag));
	m_global_flags[flag>>5] |= (1<<(flag&31));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSkaterCareer::UnSetGlobalFlag(int flag)
{
	
	Dbg_MsgAssert(flag<512,("Flag %d is over 255",flag));
	m_global_flags[flag>>5] &= ~(1<<(flag&31));
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSkaterCareer::GetGlobalFlag(int flag)
{
	
	Dbg_MsgAssert(flag<512,("Flag %d is over 255",flag));
	if (m_global_flags[flag>>5] & (1<<(flag&31)))
	{
		return true;
	}
	
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSkaterCareer::SetFlag(int flag, int level)
{
	
	Dbg_MsgAssert(flag<256,("Flag %d is over 255",flag));
	if (level == -1)
	{
		level = m_current_level;
	}
	m_level_flags[level][flag>>5] |= (1<<(flag&31));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CSkaterCareer::UnSetFlag(int flag, int level)
{
	
	Dbg_MsgAssert(flag<256,("Flag %d is over 255",flag));
	if (level == -1)
	{
		level = m_current_level;
	}
	m_level_flags[level][flag>>5] &= ~(1<<(flag&31));
	m_start_level_flags[flag>>5]  &= ~(1<<(flag&31));
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSkaterCareer::GetFlag(int flag, int level)
{
	
	Dbg_MsgAssert(flag<256,("Flag %d is over 255",flag));
	if (level == -1)
	{
		level = m_current_level;
	}
	
	if (m_level_flags[level][flag>>5] & (1<<(flag&31)))
	{
		return true;
	}
	
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CSkaterCareer::JustGotFlag(int flag)
{
	
	Dbg_MsgAssert(flag<256,("Flag %d is over 255",flag));
	// if start goal flag is different from the current one, then we must
	// have just got the goal	
	if ((m_level_flags[m_current_level][flag>>5] ^ m_start_level_flags[flag>>5]) & (1<<(flag&31)) )
	{
		return true;
	}
	
	return false;
}


void CSkaterCareer::WriteIntoStructure(Script::CScriptStructure *pIn)
{
	
	// Create an array for holding the goal flags
	Script::CArray *pGoalFlags=new Script::CArray;
	pGoalFlags->SetSizeAndType(8*vMAX_CAREER_LEVELS, ESYMBOLTYPE_INTEGER);

	// Wack 'em all in
	for (unsigned int i = 0; i<vMAX_CAREER_LEVELS; ++i)
	{
		uint32 j;
		for (j = 0;j<8;j++)
		{
			pGoalFlags->SetInteger(i*8+j,m_goal_flags[i][j]);
		}
	}	
	
	// Create an array for holding the level flags
	Script::CArray *pLevelFlags=new Script::CArray;
	pLevelFlags->SetSizeAndType(8*vMAX_CAREER_LEVELS, ESYMBOLTYPE_INTEGER);
	
	// Wack 'em all in
	for (unsigned int i = 0; i<vMAX_CAREER_LEVELS; ++i)
	{
		uint32 j;
		for (j = 0;j<8;j++)
		{
			pLevelFlags->SetInteger(i*8+j,m_level_flags[i][j]);
		}
	}	

	// Create an array for holding the global flags
	Script::CArray *pGlobalFlags=new Script::CArray;
	pGlobalFlags->SetSizeAndType(vMAX_GLOBAL_FLAGS, ESYMBOLTYPE_INTEGER);
	
	// Wack 'em all in
	for (unsigned int i = 0; i<vMAX_GLOBAL_FLAGS; ++i)
	{
		pGlobalFlags->SetInteger(i,m_global_flags[i]);
	}

#if 1
	// Create an array for holding the gap checklists
	
	Script::CArray *pGapChecklists=new Script::CArray;
	pGapChecklists->SetSizeAndType(vMAX_CAREER_LEVELS, ESYMBOLTYPE_STRUCTURE);
	
	for (unsigned int i = 0; i<vMAX_CAREER_LEVELS; ++i)
	{
		CGapChecklist * p_checklist = GetGapChecklist(i);
		Script::CStruct *p_gap_struct = new Script::CScriptStructure; 
		p_gap_struct->AddInteger(CRCD(0x651533ec,"level"),i);
		if (!p_checklist->IsValid())
		{
			p_gap_struct->AddInteger(CRCD(0x73f8ca00,"valid"),0);
		}
		else
		{
			p_gap_struct->AddInteger(CRCD(0x73f8ca00,"valid"),1);
			int num_gaps = p_checklist->NumGaps();
			
			Script::CArray *p_gaps_array=new Script::CArray;
			p_gaps_array->SetSizeAndType(num_gaps, ESYMBOLTYPE_STRUCTURE);

			for (int gap=0; gap<num_gaps; ++gap)
			{
				Script::CStruct *p_struct = new Script::CStruct; 
				
				p_struct->AddString(CRCD(0x699fcfc0,"GapName"),p_checklist->GetGapText(gap));
				p_struct->AddInteger(CRCD(0x25eb70db,"GapCount"),p_checklist->GetGapCount(gap));
				p_struct->AddInteger(CRCD(0x92ab03e8,"GapScore"),p_checklist->GetGapScore(gap));
				
				Mth::Vector skater_start_pos;
				Mth::Vector skater_start_dir;
				skater_start_pos.Set();
				skater_start_dir.Set();
				if (p_checklist->GetGapCount(gap))
				{
					p_checklist->GetSkaterStartPosAndDir(gap,&skater_start_pos,&skater_start_dir);
				}				
				// The vectors are always added to keep the save structure size as constant as possible.
				p_struct->AddVector(CRCD(0xa4605544,"SkaterStartPos"),skater_start_pos[X],skater_start_pos[Y],skater_start_pos[Z]);
				p_struct->AddVector(CRCD(0x9e12c9f8,"SkaterStartDir"),skater_start_dir[X],skater_start_dir[Y],skater_start_dir[Z]);
				
				p_gaps_array->SetStructure(gap,p_struct);
			}
						
			p_gap_struct->AddArrayPointer(CRCD(0xd76c173e,"Gaps"),p_gaps_array);
		}
		pGapChecklists->SetStructure(i,p_gap_struct);
	}
#endif

	Script::CArray* pVisitedLevels = new Script::CArray();
	pVisitedLevels->SetSizeAndType( vMAX_CAREER_LEVELS, ESYMBOLTYPE_INTEGER );
	for ( unsigned int i = 0; i < vMAX_CAREER_LEVELS; ++i )
		pVisitedLevels->SetInteger( i, m_level_visited[i] );

	// Insert the above arrays into a new structure.
	Script::CScriptStructure *pTemp=new Script::CScriptStructure;
	pTemp->AddComponent(Script::GenerateCRC("GoalFlags"),ESYMBOLTYPE_ARRAY,(int)pGoalFlags);
	pTemp->AddComponent(Script::GenerateCRC("LevelFlags"),ESYMBOLTYPE_ARRAY,(int)pLevelFlags);
	pTemp->AddComponent(Script::GenerateCRC("GlobalFlags"),ESYMBOLTYPE_ARRAY,(int)pGlobalFlags);
	
#if 1	
	pTemp->AddComponent(Script::GenerateCRC("GapChecklists"),ESYMBOLTYPE_ARRAY,(int)pGapChecklists);
#endif
	
	pTemp->AddArrayPointer( "VisitedLevels", pVisitedLevels );

	// K: Added these for Zac so that when autoloading on startup it can automatically go
	// straight to the last level played
	Mdl::Skate * p_skate_mod =  Mdl::Skate::Instance();
	pTemp->AddChecksum(CRCD(0xe3335d2f,"LastLevelLoadScript"),p_skate_mod->m_cur_level);
	pTemp->AddChecksum(CRCD(0x2cc06f5e,"LastGameMode"),p_skate_mod->GetGameMode()->GetNameChecksum());
    pTemp->AddInteger("current_theme",Script::GetInteger("stored_theme_prefix"));
					 

	// Thrust the new structure into the passed structure.
	Dbg_MsgAssert(pIn,("NULL pIn"));
	pIn->AddComponent(Script::GenerateCRC("Career"),ESYMBOLTYPE_STRUCTUREPOINTER,(int)pTemp);
	
	// Note: The above arrays and structure are not deleted here, because pointers to them have
	// been given to the passed structure, so it will delete them when it gets deleted.

//	Script::PrintContents(pTemp);
}

void CSkaterCareer::ReadFromStructure(Script::CScriptStructure *pIn)
{
	
	Dbg_MsgAssert(pIn,("NULL pIn"));

	Script::CScriptStructure *pCareer=NULL;
	pIn->GetStructure("Career",&pCareer);
	Dbg_MsgAssert(pCareer,("Missing Career structure in structure passed to CSkaterCareer::ReadFromStructure"));
	
	Script::CArray *pGoalFlags=NULL;
	pCareer->GetArray("GoalFlags",&pGoalFlags);
	Dbg_MsgAssert(pGoalFlags,("Missing GoalFlags array in Career structure"));
	Dbg_MsgAssert(pGoalFlags->GetSize()==8*vMAX_CAREER_LEVELS,("Bad size of %d for goal flags array, expected %d",pGoalFlags->GetSize(),vMAX_CAREER_LEVELS));

	Script::CArray *pLevelFlags=NULL;
	pCareer->GetArray("LevelFlags",&pLevelFlags);
	Dbg_MsgAssert(pLevelFlags,("Missing LevelFlags array in Career structure"));
	Dbg_MsgAssert(pLevelFlags->GetSize()==8*vMAX_CAREER_LEVELS,("Bad size of %d for level flags array, expected %d",pGoalFlags->GetSize(),vMAX_CAREER_LEVELS));

	for (unsigned int i = 0; i<vMAX_CAREER_LEVELS; ++i)
	{
		uint32 j;
		for (j = 0;j<8;j++)
		{
			m_goal_flags[i][j]=pGoalFlags->GetInteger(i*8+j);
		}
		
		for (j = 0;j<8;j++)
		{
			m_level_flags[i][j]=pLevelFlags->GetInteger(i*8+j);
		}
	}	
	
	Script::CArray *pGlobalFlags=NULL;
	pCareer->GetArray("GlobalFlags",&pGlobalFlags);
	Dbg_MsgAssert(pGlobalFlags,("Missing GlobalFlags array in Career structure"));
	Dbg_MsgAssert(pGlobalFlags->GetSize()==vMAX_GLOBAL_FLAGS,("Bad size of %d for global flags array, expected %d",pGlobalFlags->GetSize(),vMAX_GLOBAL_FLAGS));
	for (unsigned int i = 0; i<vMAX_GLOBAL_FLAGS; ++i)
	{
		m_global_flags[i]=pGlobalFlags->GetInteger(i);
	}
	
	Script::CArray *pGapChecklists = NULL;
	pCareer->GetArray("GapChecklists",&pGapChecklists);	
	//Dbg_MsgAssert(pGapChecklists,("Missing Gap checklist in Career structure"));
	Dbg_MsgAssert(pGapChecklists->GetSize()==vMAX_CAREER_LEVELS,("Bad size of %d for gap checklist array, expected %d",pGapChecklists->GetSize()==vMAX_CAREER_LEVELS));
	if (pGapChecklists)
	{
		for (unsigned int i = 0; i<vMAX_CAREER_LEVELS; ++i)
		{
			CGapChecklist * p_checklist = GetGapChecklist(i);
			p_checklist->FlushGapChecks();
			Script::CStruct *p_gap_struct = pGapChecklists->GetStructure(i);
			int level = 0;
			int valid = 0;
			p_gap_struct->GetInteger("level",&level,true);
			p_gap_struct->GetInteger("valid",&valid,true);
			if (valid)
			{
				Script::CArray *p_gaps=NULL;
				p_gap_struct->GetArray(CRCD(0xd76c173e,"Gaps"),&p_gaps);
				Dbg_MsgAssert(p_gaps,("Missing gaps array"));
				
				int num_gaps = p_gaps->GetSize();
				for (int gap = 0; gap<num_gaps; gap++)
				{
					Script::CStruct *p_struct=p_gaps->GetStructure(gap);
					
					const char *p_name = NULL;
					int count = 0;
					int score = 0;
					p_struct->GetString(CRCD(0x699fcfc0,"GapName"),&p_name);
					p_struct->GetInteger(CRCD(0x25eb70db,"GapCount"),&count);
					p_struct->GetInteger(CRCD(0x92ab03e8,"GapScore"),&score);
					p_checklist->AddGapCheck(p_name,count,score);  
					
					if (count)
					{
						Mth::Vector skater_start_pos;
						Mth::Vector skater_start_dir;
						p_struct->GetVector(CRCD(0xa4605544,"SkaterStartPos"),&skater_start_pos);
						p_struct->GetVector(CRCD(0x9e12c9f8,"SkaterStartDir"),&skater_start_dir);
						
						p_checklist->SetInfoForCamera(p_name,skater_start_pos,skater_start_dir);
					}
					
					//printf ("%d: %s (%d)\n",level,p_name,count);				
				}
			}
		}
	}

	Script::CArray* pVisitedLevels = NULL;
	pCareer->GetArray( "VisitedLevels", &pVisitedLevels, Script::NO_ASSERT );
	if ( pVisitedLevels )
	{
#ifdef __NOPT_ASSERT__
		int size = pVisitedLevels->GetSize();
		Dbg_MsgAssert( size <= (int)vMAX_CAREER_LEVELS, ( "wrong array size" ) );
#endif		// __NOPT_ASSERT__
		for ( int i = 0; i < (int)vMAX_CAREER_LEVELS; i++ )
			m_level_visited[i] = pVisitedLevels->GetInteger( i );
	}
}

} // namespace Obj

