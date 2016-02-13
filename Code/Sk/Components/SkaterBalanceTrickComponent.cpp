//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterBalanceTrickComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  328/3
//****************************************************************************

#include <sk/components/skaterbalancetrickcomponent.h>
#include <sk/components/skaterscorecomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/components/animationcomponent.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent* CSkaterBalanceTrickComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterBalanceTrickComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterBalanceTrickComponent::CSkaterBalanceTrickComponent() : CBaseComponent()
{
	SetType( CRC_SKATERBALANCETRICK );
	
	mp_animation_component = NULL;
	
	mpBalanceParams = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterBalanceTrickComponent::~CSkaterBalanceTrickComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterBalanceTrickComponent::InitFromStructure( Script::CStruct* pParams )
{
	mManual.Init(GetSkater());
	mGrind.Init(GetSkater());
	mLip.Init(GetSkater());
	mSkitch.Init(GetSkater());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterBalanceTrickComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterBalanceTrickComponent::Finalize (   )
{
	
	mp_animation_component = GetAnimationComponentFromObject(GetObject());
	
	Dbg_Assert(mp_animation_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterBalanceTrickComponent::Update()
{
	Suspend(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterBalanceTrickComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | SetWobbleDetails | gets called from the animation
		// component's PlayAnim handler (needs to come after the
		// animation has launched...)
		case CRCC(0xea6d0efd,"SetWobbleDetails"):
		{
			// Intercepts the animation component's PlayAnim member function and handle's skater specific logic
			
			// If this is a wobbling balance anim, then allow the manual or grind to programmatically wobble it.
			if (pParams->ContainsFlag(CRCD(0x6d941203, "Wobble")))
			{
				mManual.EnableWobble();
				mGrind.EnableWobble();
				mLip.EnableWobble();
				mSkitch.EnableWobble();
			}	
			
			Script::CStruct* p_wobble_params = Script::GetStructure(CRCD(0x5adb96d4, "DefaultWobbleParams"), Script::ASSERT);
			pParams->GetStructure(CRCD(0xd095379c, "WobbleParams"), &p_wobble_params);
			set_wobble_params(p_wobble_params);
			
			return CBaseComponent::MF_TRUE;
		}
		
        // @script | DoBalanceTrick | 
        // @parmopt int | Tweak | 0 | 
        // @parm name | ButtonA | 
        // @parm name | ButtonB |                       
        // @parmopt name | Type | 0 | balance trick type (NoseManual, 
        // Manual, Grind, Slide, Lip)
        // @flag DoFlipCheck | If set, the range anim will be played backwards if the skater is flipped.
        // @flag PlayRangeAnimBackwards | If set, the range anim will be played backwards. Can be
		// used in conjunction with DoFlipCheck. Ie, if both flags are set, then the anim will be
		// played forwards if the skater is flipped.
		// @flag ClearCheese | If set this will reset any previous cheese.
        // @parmopt structure | BalanceParams | | If not specified then the balance trick will use
		// the params defined by the script globals LipParams, or ManualParams etc depending on the
		// Type value. 
		case CRCC(0x7e92670c, "DoBalanceTrick"):	  
		{
			if (mpBalanceParams)
			{
				delete mpBalanceParams;
				mpBalanceParams = NULL;
			}
			Script::CStruct* p_balance_params = NULL;
			if (pParams->GetStructure(CRCD(0x647facc2, "BalanceParams"), &p_balance_params))
			{
				set_balance_trick_params(p_balance_params);
			}
			
			int Tweak = 0;
			pParams->GetInteger(CRCD(0x2bfd3d35, "Tweak"), &Tweak);
			
			uint32 ButtonA = 0;
			uint32 ButtonB = 0;
			pParams->GetChecksum(CRCD(0x4f1a0b01, "ButtonA"), &ButtonA, Script::ASSERT);
			pParams->GetChecksum(CRCD(0xd6135abb, "ButtonB"), &ButtonB, Script::ASSERT);
			
			uint32 NewBalanceTrickType = 0;
			pParams->GetChecksum(CRCD(0x7321a8d6, "Type"), &NewBalanceTrickType);

			// Do nothing if already doing the same type of balance trick.  This is so that the meter does not glitch when switching to
			// a balance trick from another balance trick where there is no StopBalanceTrick script command in between.
			if (mBalanceTrickType == NewBalanceTrickType) break;
			mBalanceTrickType = NewBalanceTrickType;
			
			bool DoFlipCheck = pParams->ContainsFlag(CRCD(0x1304e677, "DoFlipCheck"));
			bool PlayRangeAnimBackwards = pParams->ContainsFlag(CRCD(0x8fe31ed, "PlayRangeAnimBackwards"));
			
			// This needed because for the extra grind tricks (eg left triangle triangle) the
			// DoBalanceTrick command gets called twice in a row, once for the first time the grind is detected
			// by the first triangle, then again once the extra trick is detected.
			// In that case the cheese needs to be reset otherwise it thinks you've jumped and re-railed & you
			// fall off straight away.
			bool clear_cheese=pParams->ContainsFlag(CRCD(0xbbdb0f78,"ClearCheese"));
				
			switch (mBalanceTrickType)
			{
				case 0:
					Dbg_MsgAssert(false, ("\n%s\nMust specify a type in DoBalanceTrick, eg Type=NoseManual", pScript->GetScriptInfo()));
					break;
					
				case CRCC(0xac90769, "NoseManual"):
				case CRCC(0xef24413b, "Manual"):
					if (clear_cheese)
					{
						mManual.Reset();
					}	
					mManual.UpdateRecord();
					mManual.SetUp(ButtonA, ButtonB, Tweak, DoFlipCheck, PlayRangeAnimBackwards);
					break;
					
				case CRCC(0x255ed86f, "Grind"):
				case CRCC(0x8d10119d, "Slide"):
					if (clear_cheese)
					{
						mGrind.Reset();
					}	
					mGrind.UpdateRecord();
					mGrind.SetUp(ButtonA, ButtonB, Tweak, DoFlipCheck, PlayRangeAnimBackwards);
					break;
					
				case CRCC(0xa549b57b, "Lip"):
					if (clear_cheese)
					{
						mLip.Reset();
					}	
					mLip.UpdateRecord();
					mLip.SetUp(ButtonA, ButtonB, Tweak, DoFlipCheck, PlayRangeAnimBackwards);
					break;
					
				case CRCC(0x3506ce64, "Skitch"):
					if (clear_cheese)
					{
						mSkitch.Reset();
					}	
					mSkitch.UpdateRecord();
					mSkitch.SetUp(ButtonA, ButtonB, Tweak, DoFlipCheck, PlayRangeAnimBackwards);
					break;
					
				default:	
					Dbg_Assert(false);
					break;
			}		
			break;
		}
		
        // @script | AdjustBalance | Make minor adjustments, nudges, to the current balance trick, if any
        // @parmopt float | TimeAdd | 0.0 | 
        // @parmopt float | LeanAdd | 0.0 | 
        // @parmopt float | SpeedAdd | 0.0 | 
        // @parmopt float | TimeMult | 1.0 | 
        // @parmopt float | LeanMult | 1.0 | 
        // @parmopt float | SpeedMult | 1.0 | 
		case CRCC(0x7a0dcd4b, "AdjustBalance"):
		{
			if (mBalanceTrickType)
			{
				float TimeMult = 1.0f;
				float LeanMult = 1.0f;
				float SpeedMult = 1.0f;
				float TimeAdd = 0.0f;
				float LeanAdd = 0.0f;
				float SpeedAdd = 0.0f;
				pParams->GetFloat(CRCD(0x3a4e9413, "TimeAdd"), &TimeAdd);
				pParams->GetFloat(CRCD(0xb4cecc1, "LeanAdd"), &LeanAdd);
				pParams->GetFloat(CRCD(0xeb9ecabb, "SpeedAdd"), &SpeedAdd);
				pParams->GetFloat(CRCD(0x4c074698, "TimeMult"), &TimeMult);
				pParams->GetFloat(CRCD(0x24ebf718, "LeanMult"), &LeanMult);
				pParams->GetFloat(CRCD(0x94dbbd1c, "SpeedMult"), &SpeedMult);

				// Get whichever balance trick we are doing	
				CManual* pBalance;
				switch (mBalanceTrickType)
				{
					case CRCC(0x255ed86f, "Grind"):
					case CRCC(0x8d10119d, "Slide"):
						pBalance = &mGrind;
						break;
						
					case CRCC(0xa549b57b, "Lip"):
						pBalance = &mLip;
						break;
					
					case CRCC(0x3506ce64, "Skitch"):
						pBalance = &mSkitch;
						break;
						
					case CRCC(0xac90769, "NoseManual"):
					case CRCC(0xef24413b, "Manual"):
						pBalance = &mManual;
						break;
						
					default:
						pBalance = NULL;
						Dbg_Assert(false);
				}
	
				pBalance->mManualTime *= TimeMult;
				pBalance->mManualTime += TimeAdd;
				if (pBalance->mManualTime < 0.0f)
				{
					pBalance->mManualTime = 0.0f;
				}
				pBalance->mManualLean *= LeanMult;
				pBalance->mManualLean += LeanAdd * Mth::Sgn(pBalance->mManualLean);
				pBalance->mManualLeanDir *= SpeedMult;
				pBalance->mManualLeanDir += SpeedAdd  * Mth::Sgn(pBalance->mManualLeanDir);
			}
			break;
		}

        // @script | StopBalanceTrick | 
		case CRCC(0xe553a5b8, "StopBalanceTrick"):
			stop_balance_trick();
			break;
		
        // @script | StartBalanceTrick | 
		case CRCC(0x6cc475b7, "StartBalanceTrick"):
			mDoingBalanceTrick = true;
			break;
			
		case CRCC(0x78e669ab, "SwitchOffBalanceMeter"):
		{
			CSkaterScoreComponent* p_score_component = GetSkaterScoreComponentFromObject(GetObject());
			Dbg_Assert(p_score_component);
			
			p_score_component->GetScore()->SetBalanceMeter(false);
			p_score_component->GetScore()->SetManualMeter(false);
			
			mManual.SwitchOffMeters();
			mGrind.SwitchOffMeters();
			mLip.SwitchOffMeters();
			mSkitch.SwitchOffMeters();
			break;
		}
			
        // @script | SwitchOnBalanceMeter |
		case CRCC(0x73727701, "SwitchOnBalanceMeter"):
			mManual.SwitchOnMeters();
			mGrind.SwitchOnMeters();
			mLip.SwitchOnMeters();
			mSkitch.SwitchOnMeters();
			break;
			
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterBalanceTrickComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterBalanceTrickComponent::GetDebugInfo"));
	
	p_info->AddChecksum("mDoingBalanceTrick", mDoingBalanceTrick ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	p_info->AddChecksum("mBalanceTrickType", mBalanceTrickType);

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterBalanceTrickComponent::ClearMaxTimes (   )
{
   	mManual.ClearMaxTime();
	mGrind.ClearMaxTime();
	mLip.ClearMaxTime();
	mSkitch.ClearMaxTime();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterBalanceTrickComponent::UpdateRecord (   )
{
	mManual.UpdateRecord();		   
	mGrind.UpdateRecord();		   
	mLip.UpdateRecord();		   
	mSkitch.UpdateRecord();		   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterBalanceTrickComponent::Reset (   )
{
	mManual.Reset();
	mGrind.Reset();
	mSkitch.Reset();
	mLip.Reset();
	
	mBalanceTrickType = 0;
	mDoingBalanceTrick = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkaterBalanceTrickComponent::ExcludeBalanceButtons ( int& numButtonsToIgnore, uint32 pButtonsToIgnore[] )
{
	// If a balance trick is being done, then this will stop the buttons being used to control the balance from
	// generating the events used to trigger tricks.
	// Otherwise, ChrisR can't do kickflips by very quickly jumping out of a grind & kickflipping by rolling
	// from X to Square.
	
	// Note, in case of problems later:
	// Now, potentially a button could be pressed just before the balance trick,
	// then released during the balance trick, and the release would not get recorded as an event.
	// So if the event buffer were analysed, it would appear that the button was still pressed.
	// However, this does not seem to cause any problems, because generally if we want to know if a button
	// is pressed right now we just read the pad. Just noting this if any problems occur later.
	switch (mBalanceTrickType)
	{
		case CRCC(0xac90769, "NoseManual"):
		case CRCC(0xef24413b, "Manual"):
			numButtonsToIgnore = 2;
			pButtonsToIgnore[0] = mManual.mButtonAChecksum;
			pButtonsToIgnore[1] = mManual.mButtonBChecksum;
			break;
			
		case CRCC(0xa549b57b, "Lip"):
			numButtonsToIgnore = 2;
			pButtonsToIgnore[0] = mLip.mButtonAChecksum;
			pButtonsToIgnore[1] = mLip.mButtonBChecksum;
			break;
			
		case CRCC(0x255ed86f, "Grind"):
		case CRCC(0x8d10119d, "Slide"):
			numButtonsToIgnore = 2;
			pButtonsToIgnore[0] = mGrind.mButtonAChecksum;
			pButtonsToIgnore[1] = mGrind.mButtonBChecksum;
			break;			
			
		case CRCC(0x3506ce64, "Skitch"):
			numButtonsToIgnore = 2;
			pButtonsToIgnore[0] = mSkitch.mButtonAChecksum;
			pButtonsToIgnore[1] = mSkitch.mButtonBChecksum;
			break;
			
		default:	
			numButtonsToIgnore = 0;
			break;
	}		
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CSkaterBalanceTrickComponent::GetBalanceStat ( uint32 Checksum )
{
	// If some custom params have been set, use them.
	if (mpBalanceParams)
	{
		return GetSkater()->GetScriptedStat(Checksum, mpBalanceParams);
	}
		
	switch (mBalanceTrickType)
	{
		case CRCC(0xac90769, "NoseManual"):
		case CRCC(0xef24413b, "Manual"):
			return GetSkater()->GetScriptedStat(Checksum, Script::GetStructure(CRCD(0xb8cc8633, "ManualParams")));
			
		case CRCC(0x255ed86f, "Grind"):
		case CRCC(0x8d10119d, "Slide"):
			return GetSkater()->GetScriptedStat(Checksum, Script::GetStructure(CRCD(0x6b7fdadd, "GrindParams")));
			
		case CRCC(0xa549b57b, "Lip"):
			return GetSkater()->GetScriptedStat(Checksum, Script::GetStructure(CRCD(0xbe615d0a, "LipParams")));
		
		case CRCC(0x3506ce64, "Skitch"):
			return GetSkater()->GetScriptedStat(Checksum, Script::GetStructure(CRCD(0xfc87cc01, "SkitchParams")));
			
		default:
			Dbg_Assert(false);
			return 0.0f;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkaterBalanceTrickComponent::ClearBalanceParameters (   )
{
	if (mpBalanceParams)
	{
		delete mpBalanceParams;
		mpBalanceParams = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterBalanceTrickComponent::stop_balance_trick (   )
{
	// Stops doing any balance trick.
	// Called when going into the lip state, and also called by the StopBalanceTrick script command.
	mManual.Stop();	
	mGrind.Stop();	
	mLip.Stop();	
	mSkitch.Stop();
		
	mBalanceTrickType = 0;
	mDoingBalanceTrick = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterBalanceTrickComponent::set_wobble_params ( Script::CStruct *pParams )
{
	if (!GetObject()->GetScript())
	{
		GetObject()->SetScript(new Script::CScript);
	}

	Dbg_Assert(pParams);

	// Extract all the wobble details and stick them in WobbleDetails, then send that to SetWobbleDetails
	
	Script::CStruct* pStat = NULL;
	Gfx::SWobbleDetails theWobbleDetails;

	pParams->GetStructure(CRCD(0xfd266a26, "WobbleAmpA"), &pStat, Script::ASSERT);
	theWobbleDetails.wobbleAmpA = GetSkater()->GetScriptedStat(pStat);

	pParams->GetStructure(CRCD(0x642f3b9c, "WobbleAmpB"), &pStat, Script::ASSERT);
	theWobbleDetails.wobbleAmpB = GetSkater()->GetScriptedStat(pStat);

	pParams->GetStructure(CRCD(0xf43fd49, "WobbleK1"), &pStat, Script::ASSERT);
	theWobbleDetails.wobbleK1 = GetSkater()->GetScriptedStat(pStat);

	pParams->GetStructure(CRCD(0x964aacf3, "WobbleK2"), &pStat, Script::ASSERT);
	theWobbleDetails.wobbleK2 = GetSkater()->GetScriptedStat(pStat);

	pParams->GetStructure(CRCD(0xf90b0824, "SpazFactor"), &pStat, Script::ASSERT);
	theWobbleDetails.spazFactor = GetSkater()->GetScriptedStat(pStat);

	mp_animation_component->SetWobbleDetails(theWobbleDetails, true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterBalanceTrickComponent::set_balance_trick_params ( Script::CStruct *pParams )
{
	Dbg_Assert(pParams);

	if (!mpBalanceParams)
	{
		mpBalanceParams = new Script::CStruct;
	}
	mpBalanceParams->Clear();
	mpBalanceParams->AppendStructure(pParams);	
}

}
