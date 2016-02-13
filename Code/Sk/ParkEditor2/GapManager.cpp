#include <core/defines.h>
#include <sk/ParkEditor2/EdMap.h>
#include <sk/ParkEditor2/GapManager.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <sk/modules/skate/skate.h>
#include <sk/objects/skatercareer.h>
#include <sk/objects/gap.h>
#include <sk/components/GoalEditorComponent.h>


namespace Ed
{




CGapManager *CGapManager::sp_instance = NULL;


GapInfo::GapInfo()
{
	mp_gapPiece[0] 		= NULL;
	mp_gapPiece[1] 		= NULL;
	mp_regularMeta[0] 	= NULL;
	mp_regularMeta[1] 	= NULL;
	
	m_descriptor.mCancelFlags=0;
}




CGapManager::CGapManager(CParkManager *pManager)
{
	m_currentId = 2000;

	for (int i = 0; i < vMAX_GAPS; i++)
		mp_gapInfoTab[i] = NULL;

	mp_manager = pManager;
	SetInstance();
}




CGapManager::~CGapManager()
{
	// XXX
	Ryan("in ~CGapManager()\n");
	RemoveAllGaps();
	// XXX
	Ryan("leaving ~CGapManager()\n");
	sp_instance = NULL;
}

CGapManager& CGapManager::operator=( const CGapManager& rhs )
{
	m_currentId = rhs.m_currentId;
	
	for (int i = 0; i < vMAX_GAPS; i++)
	{
		if (rhs.mp_gapInfoTab[i])
		{
			GapInfo *p_new = new GapInfo;
			
			p_new->m_id[0] = rhs.mp_gapInfoTab[i]->m_id[0];
			p_new->m_id[1] = rhs.mp_gapInfoTab[i]->m_id[1];
			p_new->mp_gapPiece[0] = NULL;
			p_new->mp_gapPiece[1] = NULL;
			p_new->mp_regularMeta[0] = NULL;
			p_new->mp_regularMeta[1] = NULL;
			Dbg_MsgAssert(strlen(rhs.mp_gapInfoTab[i]->mp_text) < 128,("Bad source gap text"));
			strcpy(p_new->mp_text, rhs.mp_gapInfoTab[i]->mp_text);
			p_new->m_descriptor = rhs.mp_gapInfoTab[i]->m_descriptor;
			
			mp_gapInfoTab[i] = p_new;
		}
		else
		{
			mp_gapInfoTab[i]=NULL;
		}	
		
		m_startOfGap[i] = rhs.m_startOfGap[i];
	}
	
	mp_manager=NULL;
	
	return *this;
}



int CGapManager::GetFreeGapIndex()
{
	for (int i = 0; i < vMAX_GAPS; i++)
		if (!mp_gapInfoTab[i])
			return i;
	return -1;
}




void CGapManager::StartGap(CClonedPiece *pGapPiece, CConcreteMetaPiece *pRegularMeta, int tab_index)
{
	
	Dbg_Assert(pGapPiece);
	Dbg_Assert(pRegularMeta);

	//Ryan("Starting gap 0x%x\n", m_currentId);
	
	// find empty slot
	// BAD MEM
	mp_gapInfoTab[tab_index] = new GapInfo();
	mp_gapInfoTab[tab_index]->m_id[0] = m_currentId++;
	mp_gapInfoTab[tab_index]->mp_gapPiece[0] = pGapPiece;
	//mp_gapInfoTab[tab_index]->mp_gapPiece[0]->MarkAsGapInProgress();
	mp_gapInfoTab[tab_index]->mp_regularMeta[0] = pRegularMeta;
	mp_gapInfoTab[tab_index]->m_descriptor.tabIndex = tab_index;
	m_startOfGap[tab_index] = true;
}




void CGapManager::EndGap(CClonedPiece *pGapPiece, CConcreteMetaPiece *pRegularMeta, int tab_index)
{
	
	Dbg_Assert(pGapPiece);
	Dbg_Assert(pRegularMeta);
	Dbg_MsgAssert(tab_index >= 0 && tab_index < vMAX_GAPS, ("invalid tab_index"));

	GapInfo *pStartInfo = mp_gapInfoTab[tab_index];
	Dbg_Assert(pStartInfo);
	
	//Ryan("Ending gap 0x%x\n", m_currentId);
	
	// find empty slot
	for (int i = 0; i < vMAX_GAPS; i++)
		if (!mp_gapInfoTab[i])
		{
			mp_gapInfoTab[i] = pStartInfo;
			mp_gapInfoTab[i]->m_id[1] = m_currentId++;
			mp_gapInfoTab[i]->mp_gapPiece[1] = pGapPiece;
			//mp_gapInfoTab[i]->mp_gapPiece[0]->UnmarkAsGapInProgress();
			//mp_gapInfoTab[i]->mp_gapPiece[1]->UnmarkAsGapInProgress();
			mp_gapInfoTab[i]->mp_regularMeta[1] = pRegularMeta;
			mp_gapInfoTab[i]->m_descriptor.tabIndex = tab_index;
			m_startOfGap[i] = false;
			return;
		}

	Dbg_MsgAssert(0, ("out of gap slots"));
	return;
}

// K: Called when CParkEditor::SetState switches to test play or regular play.
// This is necessary to make the gaps appear in the view-gaps menu, which lists those
// in the skater career.
void CGapManager::RegisterGapsWithSkaterCareer()
{
	Obj::CGapChecklist	*p_gap_checklist=Mdl::Skate::Instance()->GetCareer()->GetGapChecklist();
	if (!p_gap_checklist)
	{
		return;
	}

	p_gap_checklist->FlushGapChecks();

	for (int i = 0; i < vMAX_GAPS; i++)
	{
		if (mp_gapInfoTab[i])
		{
			p_gap_checklist->AddGapCheck(mp_gapInfoTab[i]->m_descriptor.text, 0,
										 mp_gapInfoTab[i]->m_descriptor.score);
		}
	}	
}

void CGapManager::SetGapInfo(int tab_index, GapDescriptor &descriptor)
{
	
	Dbg_MsgAssert(tab_index >= 0 && tab_index < vMAX_GAPS, ("invalid tab_index"));
	Dbg_Assert(mp_gapInfoTab[tab_index]);
	Dbg_Assert(descriptor.tabIndex >= 0 && descriptor.tabIndex < vMAX_GAPS);

	sprintf(mp_gapInfoTab[tab_index]->mp_text, "%s", descriptor.text);
	mp_gapInfoTab[tab_index]->m_descriptor = descriptor;

	RegisterGapsWithSkaterCareer();
	//mp_gapInfoTab[tab_index]->mp_gapPiece[0]->MakeTriggerable(true);
	//mp_gapInfoTab[tab_index]->mp_gapPiece[1]->MakeTriggerable(true);
}




bool CGapManager::IsGapAttached(const CConcreteMetaPiece *pRegularMeta, int *pTabIndex)
{
	for (int i = 0; i < vMAX_GAPS; i++)
		if (mp_gapInfoTab[i])
		{
			int half_num = (m_startOfGap[i]) ? 0 : 1;
			#if 0
			if (i == 16 || i == 17)
				Ryan("           testing gap index %d, pMeta[0]=%x, pMeta[1]=%x, half=%d, test_meta=%p\n", 
					 i,
					 mp_gapInfoTab[i]->mp_regularMeta[0],
					 mp_gapInfoTab[i]->mp_regularMeta[1], 
					 half_num, pRegularMeta);  
			#endif
			if (mp_gapInfoTab[i]->mp_regularMeta[half_num] == pRegularMeta)
			{
				#if 0
				Ryan("     found gap at table index %d, position=(%d,%d,%d), half=%d, pMeta=%p\n", 
					 i, 
					 mp_gapInfoTab[i]->m_descriptor.loc[half_num].GetX(), 
					 mp_gapInfoTab[i]->m_descriptor.loc[half_num].GetY(),
					 mp_gapInfoTab[i]->m_descriptor.loc[half_num].GetZ(),
					 half_num,
					 pRegularMeta);
				#endif
				if (pTabIndex)
					*pTabIndex = i;
				return true;
			}
		}
	return false;
}




void CGapManager::RemoveGap(int tab_index, bool unregisterGapFromGoalEditor)
{
	
	Dbg_MsgAssert(tab_index >= 0 && tab_index < vMAX_GAPS, ("invalid tab_index"));

	GapInfo *pRemoveInfo = mp_gapInfoTab[tab_index];
	Dbg_Assert(pRemoveInfo);
	
	
	for (int gp = 0; gp < 2; gp++)
	{
		if (pRemoveInfo->mp_gapPiece[gp])
			mp_manager->GetGenerator()->DestroyClonedPiece(pRemoveInfo->mp_gapPiece[gp]);
	}

	// Note: Need to check mp_manager cos it could be NULL in the case of this being
	// the CParkEditor's mp_play_mode_gap_manager. (Fixes TT12087)
	if (unregisterGapFromGoalEditor)
	{
		// Determine the gap number of the gap as stored in the career gap check list	
		int gap_number=Mdl::Skate::Instance()->GetGapChecklist()->GetGapChecklistIndex(pRemoveInfo->m_descriptor.text);
		if (gap_number >= 0)
		{
			Obj::GetGoalEditor()->RefreshGapGoalsAfterGapRemovedFromPark(gap_number);
		}	
	}
		
	for (int i = 0; i < vMAX_GAPS; i++)
	{
		if (mp_gapInfoTab[i] == pRemoveInfo)
		{
			mp_gapInfoTab[i] = NULL;
		}
	}
	delete pRemoveInfo;
}




void CGapManager::RemoveAllGaps()
{
	for (int i = 0; i < vMAX_GAPS; i++)
	{
		if (mp_gapInfoTab[i])
		{
			// Normally, when a gap piece is deleted such as when editing a park, it gets unregistered
			// from the goal editor.
			// However, when cleaning up the park, we don't want to do this.
			// The false means do not unregister the gap from the goal editor.
			RemoveGap(i, false);
		}
	}	
}




uint32 CGapManager::GetGapTriggerScript(CClonedPiece *pGapPiece)
{
	for (int i = 0; i < vMAX_GAPS; i++)
		if (mp_gapInfoTab[i])
		{
			int half_num = (m_startOfGap[i]) ? 0 : 1;
			if (mp_gapInfoTab[i]->mp_gapPiece[half_num] == pGapPiece)
			{
				if (i&1  && Script::GetInt("PrintGaps",false))
				{
					printf ("Editor Gap %d points: %s\n",mp_gapInfoTab[i]->m_descriptor.score, mp_gapInfoTab[i]->m_descriptor.text);  
				}
				
				char gap_trigger_script[48];
				sprintf(gap_trigger_script, "EditorTrgGap%d", i);
				return Script::GenerateCRC(gap_trigger_script);
			}
		}

	Dbg_MsgAssert(0, ("no gap trigger script"));
	return 0;
}



	
CGapManager::GapDescriptor *CGapManager::GetGapDescriptor(int tab_index, int *pHalfNum)
{
	
	if (!mp_gapInfoTab[tab_index])
		return NULL;
	
	*pHalfNum = (m_startOfGap[tab_index]) ? 0 : 1;
	strcpy(mp_gapInfoTab[tab_index]->m_descriptor.text, mp_gapInfoTab[tab_index]->mp_text);
	return &mp_gapInfoTab[tab_index]->m_descriptor;
}




void CGapManager::MakeGapWireframe(int tab_index)
{
	
	Dbg_MsgAssert(tab_index >= 0 && tab_index < vMAX_GAPS, ("invalid tab_index %d", tab_index));

	if (!mp_gapInfoTab[tab_index])
		return;

	if (mp_gapInfoTab[tab_index]->mp_gapPiece[0])
		mp_gapInfoTab[tab_index]->mp_gapPiece[0]->SetVisibility(true);
	if (mp_gapInfoTab[tab_index]->mp_gapPiece[1])
		mp_gapInfoTab[tab_index]->mp_gapPiece[1]->SetVisibility(true);
}




void CGapManager::LaunchGap(int tab_index, Script::CScript *pScript)
{
#if 1	
	Dbg_MsgAssert(tab_index >= 0 && tab_index < vMAX_GAPS, ("invalid tab_index"));
	Dbg_Assert(mp_gapInfoTab[tab_index]);
	Dbg_Assert(pScript);
	Dbg_Assert(pScript->mpObject);
		
	for (int i = 0; i < 2; i++)
	{
		int half_num = (m_startOfGap[tab_index]) ? i : 1 - i;

		Script::CStruct *pGapParms = new Script::CStruct();
		pGapParms->AddChecksum(CRCD(0x3b442e26,"GapID"), mp_gapInfoTab[tab_index]->m_id[half_num]);
		pGapParms->AddInteger(CRCD(0xcd66c8ae,"score"), mp_gapInfoTab[tab_index]->m_descriptor.score);
		pGapParms->AddString(CRCD(0xc4745838,"text"), mp_gapInfoTab[tab_index]->mp_text);
		//pGapParms->AddComponent(Script::GenerateCRC("text"), ESYMBOLTYPE_STRING, "boogah");
		
		if (i == 0)
		{
			uint32 cancel_flags=mp_gapInfoTab[tab_index]->m_descriptor.mCancelFlags;
			if (!cancel_flags)
			{
				// If no flags are set for some reason, use the default of just CANCEL_GROUND
				cancel_flags=Script::GetInteger(CRCD(0x9a27e74d,"CANCEL_GROUND"));
			}
			pGapParms->AddChecksum(CRCD(0x2760de9e,"combined_flags"),cancel_flags);
		
			pScript->mpObject->CallMemberFunction(Script::GenerateCRC("StartGap"), pGapParms, pScript);
		}	
		else
		{
			pScript->mpObject->CallMemberFunction(Script::GenerateCRC("EndGap"), pGapParms, pScript);
		}
		
		delete pGapParms;	
	}
#endif
}




CGapManager *CGapManager::sInstance()
{
	Dbg_Assert(sp_instance);
	return sp_instance;
}




}





