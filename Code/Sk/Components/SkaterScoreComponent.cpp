//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterScoreComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/12/3
//****************************************************************************

#include <sk/components/skaterscorecomponent.h>
#include <sk/objects/skatercareer.h>
#include <sk/gamenet/gamemsg.h>
#include <sk/modules/skate/skate.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

extern bool g_CheatsEnabled;

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent* CSkaterScoreComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterScoreComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterScoreComponent::CSkaterScoreComponent() : CBaseComponent()
{
	SetType( CRC_SKATERSCORE );
	mp_score = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterScoreComponent::~CSkaterScoreComponent()
{
	if (mp_score)
	{
		delete mp_score;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterScoreComponent::InitFromStructure( Script::CStruct* pParams )
{
	Dbg_MsgAssert(GetObject()->GetType() == SKATE_TYPE_SKATER, ("CSkaterScoreComponent added to non-skater composite object"));
	
	if (!mp_score)
	{
		mp_score = new Mdl::Score();
		mp_score->SetSkaterId(GetObject()->GetID());
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterScoreComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterScoreComponent::Update()
{
	// NOTE: in fact, non-local skaters may not need a score component at all
	if (!GetSkater()->IsLocalClient())
	{
		Suspend(true);
		return;
	}
	
	mp_score->Update();
	
	if (GetSkater()->m_always_special || Mdl::Skate::Instance()->GetCareer()->GetCheat(CRCD(0xeefecf1b, "CHEAT_ALWAYS_SPECIAL")))
	{
		// Here, set the flag. It may seem redundant, but the above line is very likely
		// to be hacked by gameshark. They probably won't notice this one, which will
		// set the flags as if they had actually enabled the cheat -- which enables us
		// to detect that it has been turned on more easily.
		Mdl::Skate::Instance()->GetCareer()->SetGlobalFlag( Script::GetInteger(CRCD(0xeefecf1b, "CHEAT_ALWAYS_SPECIAL")));
		mp_score->ForceSpecial();
		g_CheatsEnabled = true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterScoreComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | LastScoreLandedGreaterThan | Multiplayer-safe version 
		// of SkaterLastScoreLandedGreaterThan
		// Example: if LastScoreLandedGreaterThan 2000 --do cool stuff-- endif
		// @uparm 1 | Score (int)
		case CRCC(0xe0112aab, "LastScoreLandedGreaterThan"):
		{
			int score;
			pParams->GetInteger(NO_NAME, &score, Script::ASSERT);
			return mp_score->GetLastScoreLanded() > score ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		
		// @script | LastScoreLandedLessThan | Multiplayer-safe version 
		// of SkaterLastScoreLandedLessThan
		// Example: if LastScoreLandedLessThan 2000 --do cool stuff-- endif
		// @uparm 1 | Score (int)
		case CRCC(0x3c445a82, "LastScoreLandedLessThan"):
		{
			int score;
			pParams->GetInteger(NO_NAME, &score, Script::ASSERT);
			return mp_score->GetLastScoreLanded() < score ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		
		// @script | TotalScoreGreaterThan | Multiplayer-safe version 
		// of SkaterTotalScoreGreaterThan
		// Example: if TotalScoreGreaterThan 2000 --do cool stuff-- endif
		// @uparm 1 | Score (int)
		case CRCC(0x8bf43831, "TotalScoreGreaterThan"):
		{
			int score;
			pParams->GetInteger(NO_NAME, &score, Script::ASSERT);
			return mp_score->GetTotalScore() > score ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
			
		// @script | TotalScoreLessThan | Multiplayer-safe version 
		// of SkaterTotalScoreLessThan
		// Example: if TotalScoreLessThan 2000 --do cool stuff-- endif
		// @uparm 1 | Score (int)
		case CRCC(0xdf14bd98, "TotalScoreLessThan"):
		{
			int score;
			pParams->GetInteger(NO_NAME, &score, Script::ASSERT);
			return mp_score->GetTotalScore() < score ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
			
		// @script | CurrentScorePotGreaterThan | Multiplayer-safe version 
		// of SkaterCurrentScorePotGreaterThan
		// Example: if CurrentScorePotGreaterThan 2000 --do cool stuff-- endif
		// @uparm 1 | Score (int)
		case CRCC(0xd19bdb50, "CurrentScorePotGreaterThan"):
		{
			int score;
			pParams->GetInteger(NO_NAME, &score, Script::ASSERT);
			return mp_score->GetScorePotValue() > score ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}

		// @script | GetTotalScore | returns the total score of the skater
		// in TotalScore
		case CRCC( 0x8f2fe3d3, "GetTotalScore" ):
		{
			pScript->GetParams()->AddInteger( CRCD( 0xee5b2b48, "TotalScore" ), mp_score->GetTotalScore() );
			return CBaseComponent::MF_TRUE;
		}
			
		// @script | CurrentScorePotLessThan | Multiplayer-safe version 
		// of SkaterCurrentScorePotLessThan
		// Example: if CurrentScorePotLessThan 2000 --do cool stuff-- endif
		// @uparm 1 | Score (int)
		case CRCC(0x56645fc6, "CurrentScorePotLessThan"):
		{
			int score;
			pParams->GetInteger(NO_NAME, &score, Script::ASSERT);
			return mp_score->GetScorePotValue() < score ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		
		// @script | GetNumberOfNonGapTricks | returns number of non-gap tricks currently in the combo
		case CRCC(0xc068ea59, "GetNumberOfNonGapTricks"):
			pScript->GetParams()->AddInteger(CRCD(0xffe7e02c, "NumberOfNonGapTricks"), mp_score->GetNumberOfNonGapTricks());
			break;
		
        // @script | GotSpecial | true if special is active
		case CRCC(0x5589b902, "GotSpecial"):	
			return mp_score->GetSpecialState() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
        
		// @script | TweakTrick | 
        // @uparm 0 | tweak
		case CRCC(0xbe3e5463, "TweakTrick"):
		{
			int Tweak = 0;
			pParams->GetInteger(NO_NAME, &Tweak);
			
			mp_score->TweakTrick(Tweak);
			break;
		}
			
		// @script | IsLatestTrick | 
		case CRCC(0x2877b61a, "IsLatestTrick"):
		{
			uint32 key_combo;
			if (pParams->GetChecksum(CRCD(0x95e16467, "KeyCombo"), &key_combo))
			{
				int spin = 0;
				if (pParams->GetInteger(CRCD(0xedf5db70, "Spin"), &spin))
				{
					Dbg_MsgAssert(spin % 180 == 0, ("IsLatestTrick called with a spin value of %i which is not a multiple of 180", spin));
					spin /= 180;
				}
				
				int num_taps = 1;
				pParams->GetInteger(CRCD(0xc82cf71b, "NumTaps"), &num_taps);
				
				return mp_score->IsLatestTrick(key_combo, spin, false, num_taps) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			}
			else
			{
				const char* trick_text;
				if (pParams->GetString(CRCD(0x3eafa520, "TrickText"), &trick_text))
				{
					int spin = 0;
					if (pParams->GetInteger(CRCD(0xedf5db70, "Spin"), &spin))
					{
						Dbg_MsgAssert(spin % 180 == 0, ("IsLatestTrick called with a spin value of %i which is not a multiple of 180", spin));
						spin /= 180;
					}
					
					int num_taps = 1;
					pParams->GetInteger(CRCD(0xc82cf71b, "NumTaps"), &num_taps);
					
					return mp_score->IsLatestTrickByName(Script::GenerateCRC(trick_text), spin, false, num_taps) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
				}
				else
				{
					Dbg_MsgAssert(false, ("IsLatestTrick must have either a KeyCombo or a TrickText parameter"));
				}
			}
		}

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterScoreComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterScoreComponent::GetDebugInfo"));
	
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterScoreComponent::Reset (   )
{
	mp_score->Reset();

	mp_score->SetBalanceMeter(false);
	mp_score->SetManualMeter(false);
}

}
