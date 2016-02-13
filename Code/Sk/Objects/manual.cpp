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
**	File name:		manual.cpp												**
**																			**
**	Created:		01/31/01	-	ksh										**
**																			**
**	Description:	Skater object code										**
**																			**
*****************************************************************************/

//#define		__DEBUG_CHEESE__

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/singleton.h>

#include <core/math.h>

#include <gel/inpman.h>
#include <gel/mainloop.h>
#include <gel/net/server/netserv.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/objtrack.h>
#include <gel/components/statsmanagercomponent.h>

#include <objects/manual.h>
#include <objects/rail.h>
#include <objects/skaterprofile.h>

#include <sys/timer.h>


#include <engine/sounds.h>

#include <gfx/camera.h>
#include <gfx/shadow.h>

#include <gfx/debuggfx.h>

#include <gel/soundfx/soundfx.h>

#include <sk/modules/skate/skate.h> 

#include <sk/gamenet/gamenet.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skaterbalancetrickcomponent.h>
#include <sk/components/skaterscorecomponent.h>

#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/

namespace Obj
{



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

CManual::CManual()
{
	Reset();
}

CManual::~CManual()
{
	
}

void CManual::Init ( CCompositeObject *pSkater )
{
	mpSkater = pSkater;
	Dbg_Assert(mpSkater);
	
	mpSkaterBalanceTrickComponent = GetSkaterBalanceTrickComponentFromObject(mpSkater);
	Dbg_Assert(mpSkaterBalanceTrickComponent);
	
	mpSkaterScoreComponent = GetSkaterScoreComponentFromObject(mpSkater);
	Dbg_Assert(mpSkaterScoreComponent);
	
	mpInputComponent = GetInputComponentFromObject(mpSkater);
	Dbg_Assert(mpInputComponent);
}

void CManual::Reset()
{

#ifdef		__DEBUG_CHEESE__
	printf ("%d:   Resetting cheese to 0 in CManual::Reset\n",__LINE__);
#endif	
	mManualCheese=0.0f;
	mActualManualTime=0.0f;
	mManualTime=0.0f;
	mManualLean=0.0f;
	mManualLeanDir=0.0f;
	mManualReleased=false;
	mOldRailNode=-1;
	mWobbleEnabled=false;
}

// a utility function to scale a linear value based on 
// the framerate
// m_time will be 1/60 when running at 60fps
// and 1/50 when running at 50fps (for pal)
// So, this 60.0f is correct for PAL
// as the adjustment is done to m_time					
float CManual::scale_with_frame_length ( float frame_length, float f )
{
	return f * frame_length * 60.0f;	  // Do NOT change this in PAL mode, leave it at 60
}

// clear the record time for this level, only call this at start of a session
void CManual::ClearMaxTime()
{
	
	mMaxTime=0.0f;
}


void CManual::SwitchOffMeters()
{
	
	mNeverShowMeters=true;
}

void CManual::SwitchOnMeters()
{
	
	mNeverShowMeters=false;
}

// Switched off the meters and sets the buttons to zero so that calling DoManualPhysics will
// have no effect.
void CManual::Stop()
{
	
	// Switch off the meters.
	if (mpSkaterScoreComponent->GetScore())
	{
		mpSkaterScoreComponent->GetScore()->SetBalanceMeter(false);
		mpSkaterScoreComponent->GetScore()->SetManualMeter(false);
	} 
	mButtonAChecksum=0;
	mButtonBChecksum=0;
}


// Check to see if this is a new record				 
void CManual::UpdateRecord()
{
    // Update the longest grind/manual/lip
	if (mActualManualTime>mMaxTime)
	{
		mMaxTime=mActualManualTime;
	}
	mActualManualTime = 0.0f;
}



void CManual::SetUp(uint32 ButtonAChecksum, uint32 ButtonBChecksum, int Tweak, bool DoFlipCheck, bool PlayRangeAnimBackwards)
{
	
	mActualManualTime = 0.0f;
	mManualReleased=false;
	mButtonAChecksum=ButtonAChecksum;
	mButtonBChecksum=ButtonBChecksum;
	mTweak=Tweak;
	mDoFlipCheck=DoFlipCheck;
	mPlayRangeAnimBackwards=PlayRangeAnimBackwards;

	Dbg_MsgAssert(mpSkater,("NULL mpSkater"));

	// Check for whether the skater has re-railed onto the same rail
	// or a different one.
	if (mpSkaterBalanceTrickComponent->mBalanceTrickType==0x255ed86f || // Grind
		mpSkaterBalanceTrickComponent->mBalanceTrickType==0x8d10119d) // Slide
	{
		const CRailNode* p_rail_node = GetSkaterCorePhysicsComponentFromObject(mpSkater)->mp_rail_node;
		
		Dbg_MsgAssert(p_rail_node,("What, NULL mp_rail_node ??"));
		
		if (mOldRailNode==-1)
		{
			// This is a fresh rail.
		}
		else
		{
		
			if (p_rail_node->ProbablyOnSameRailAs(mOldRailNode))
			{
				// Jumped and landed on the same rail without ending
				// the combo.
				// Add some time to make this more difficult.
				mManualTime+=mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0xfbec5b6c, "Same_Grind_Add_Time"));
				#ifdef		__DEBUG_CHEESE__
					printf ("%d:  %.2f:  leaving cheese, probably on same rail\n",__LINE__,mManualCheese);
				#endif					
			}
			else
			{
				// It's a different rail from the last one, ie jumped from a rail to
				// another rail without ending the combo.
				// Reward the player by subtracting a bit of time to make it easier.
				mManualTime-=mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0xc5e1c8eb,"New_Grind_Sub_Time"));
				if (mManualTime<0.0f)
				{
					mManualTime=0.0f;
				}	
				// And clear the cheese
				// as it's not cheesey to change rails
				// so we don't to penalize the player for doing so
				#ifdef		__DEBUG_CHEESE__
					printf ("%d:   Resetting cheese to 0 as it's a new grind\n",__LINE__);
				#endif	

				mManualCheese = 0.0f;
			}	
		}	
		mOldRailNode=p_rail_node->GetNode();
		
		Mdl::Score *pScore=mpSkaterScoreComponent->GetScore();
		float robot = pScore->GetRobotRailMult();
		if ( robot < 0.5f)	   // <0.5 means any time you are on the same rail twice in a combo
		{
//			printf ("SHOULD ADJUST BALANCE HERE, AS ON A ROBOT LINE\n");
			// Need to add something to the Manual Time
			mManualTime += Script::GetFloat(CRCD(0xf54ca12f,"robot_rail_add_time") )* 0.5f / robot;
			
			// And give him  percentage nudge away from 0
			// (actually, an absolute value would be better, more consistent
			// perhaps not even based on the amount, just trigger 20% whenever  
			// you land on something robotesque
			mManualLean += Script::GetFloat(CRCD(0x30990680,"robot_rail_nudge")) * Mth::Sgn(mManualLeanDir) * 0.5f / robot;
//			printf ("Time = %.2f,  Lean = %.2f, dir = %.2f\n",mManualTime, mManualLean, mManualLeanDir);
		}
		
		
	}

	float repeat_min = mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0x8f4db5d3,"Repeat_Min"));
	if ( Mth::Abs(mManualLeanDir) != 0.0f)
	{
		// first multiplied by the reat multipler
		mManualLeanDir = mManualLeanDir * mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0x93dd6a10,"Repeat_Multiplier"));		
				
		// use the sign of previous balance
		if ( Mth::Abs(mManualLeanDir) < repeat_min)
		{
			//  if too small, then set to minimum
			mManualLeanDir = repeat_min * Mth::Sgn(mManualLeanDir);
		}	
		else
		{
		}
	}
	else
	{
		// No previous balance
		// so just set to default minimum
		mManualLeanDir = repeat_min;
		// Adjust the sign of the lean to point in the direction of a bail.
		switch (mpSkaterBalanceTrickComponent->mBalanceTrickType)
		{
			case 0xef24413b: // Manual
				// Manuals keep positive sign.
				break;
			case 0x0ac90769: // NoseManual	
				// Nosemanuals fall the other way.
				mManualLeanDir=-mManualLeanDir;
				break;
			default:
				// If in a grind, choose a random side to fall towards.
				// Hmm, should choose differently for a lip?
				if (Mth::Rnd(2))
				{
					mManualLeanDir=-mManualLeanDir;
				}
				break;
		}		

	}
	
	
	// multiply this initial direction by some value based on the time
//	mManualLean = Mth::Abs(mManualLean) * Mth::Sgn(mManualLeanDir);		// possibly alter lean to face the bail direction

	mManualLean *= mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0x6ce10ea4,"Lean_Repeat_Multiplier"));

	// add cheese after lean multiply (otherwise, it can be ineffective)																				 
	#ifdef		__DEBUG_CHEESE__
		printf ("%d:  %.2f + %.2f = %.2f:  Adding cheese to lean \n",__LINE__,mManualLean, mManualCheese * Mth::Sgn(mManualLean),mManualLean += mManualCheese * Mth::Sgn(mManualLean));
	#endif					

	mManualLean += mManualCheese * Mth::Sgn(mManualLean);				// add in the cheese	to the lean
	
	float BailAngle=mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0xf81ab0e9,"Lean_Bail_Angle"));
	if (Mth::Abs(mManualLean) > BailAngle)
	{
		#ifdef		__DEBUG_CHEESE__
			printf ("%d:  %.2f:  Cheese caused an instant bail!!! \n",__LINE__,mManualCheese);
		#endif					
		// The cheese has caused an instant bail, so in order that the meter does become
		// visible, make the lean a wee bit less that the bail angle.
		mManualLean=BailAngle*0.9f;
		mManualLean = Mth::Abs(mManualLean) * Mth::Sgn(mManualLeanDir);
		mManualLeanDir = Mth::Sgn(mManualLeanDir) * BailAngle;  // ensure a bail next frame
	}

	
	// Reset the cheese.
	mManualCheese=mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0x1231ae3c,"Cheese"));

	#ifdef		__DEBUG_CHEESE__
		printf ("%d:  %.2f:  Cheese reset to initial scripted value \n",__LINE__,mManualCheese);
	#endif					

	
	// The programmatic wobble applied to the animation is off initially, so that it does not get applied
	// to the init anim.
	mWobbleEnabled=false;
	
	mStartTime=Tmr::ElapsedTime(0);
}

// Gets called by the EnableWobble skater script command.
void CManual::EnableWobble()
{
	
	mWobbleEnabled=true;
}

//static float MaxTime=0;
void CManual::DoManualPhysics()
{
	float frame_length = Tmr::FrameLength();
	
	// Do nothing if no buttons are specified.
	if (!mButtonAChecksum || !mButtonBChecksum)
	{
		return;
	}
		
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Net::Server* server;
	GameNet::PlayerInfo* player;
	Net::Conn* skater_conn;
	
	Dbg_MsgAssert(mpSkater,("NULL mpSkater"));
	
	skater_conn = NULL;
	if(( server = gamenet_man->GetServer()))
	{
		if(( player = gamenet_man->GetPlayerByObjectID( mpSkater->GetID() )))
		{
			skater_conn = player->m_Conn;
		}
	}
	
	// decrement the cheese value, which is added to the lean angle when we do the next manual
	// this is to prevent "cheesing" the manuals (by rapidly doing several manuals that only
	// last a frame or two, and don't give the balancing physics any time to take effect).

	if (mManualCheese>0.01f)
	{
		mManualCheese -= scale_with_frame_length(frame_length, mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0x1231ae3c,"Cheese"))/mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0xa4362662,"CheeseFrames")));
		if (mManualCheese < 0.0f)
			mManualCheese = 0.0f;
		#ifdef		__DEBUG_CHEESE__
			printf ("%d:  %.2f:  Cheese decremented\n",__LINE__,mManualCheese);
		#endif					

	}
//	printf(Cheese=%f\n",mManualCheese);

	// Add in the tweak to the score.
	Mdl::Score *pScore=mpSkaterScoreComponent->GetScore();
	//pScore->TweakTrickRequest(mTweak);
	pScore->TweakTrick(mTweak);

	mActualManualTime+=frame_length;
	mManualTime+=frame_length;

    Obj::CStatsManagerComponent* pStatsManagerComponent = GetStatsManagerComponentFromObject(mpSkater);
	Dbg_Assert( pStatsManagerComponent );
    
    pStatsManagerComponent->CheckTimedRecord( mpSkaterBalanceTrickComponent->mBalanceTrickType, mActualManualTime, false );
	
	float ManualLeanGravity=mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0x6851748,"Lean_Gravity_Stat"));
	float ManualInstableRate=mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0xa9fef5ab,"Instable_Rate"));
	float ManualBase = mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0xb6a634f3,"Instable_base")); 
	float ManualInstability = ManualBase + (mManualTime * ManualInstableRate);

	
	// the more you lean, the more you lean
//	mManualLean += scale_with_frame_length(mManualLean/ManualLeanGravity * (0.5f + mManualTime/2)/ManualInstableRate);
//	mManualLean += scale_with_frame_length(mManualLeanDir * (0.5f + mManualTime/2)/ManualInstableRate);	 // speed of turn
	
	mManualLean += scale_with_frame_length(frame_length, mManualLean * ManualLeanGravity * ManualInstability);
	mManualLean += scale_with_frame_length(frame_length, mManualLeanDir * ManualInstability);	 	


	CSkaterButton *pButt=mpInputComponent->GetControlPad().GetButton(mButtonAChecksum);
	bool APressed=pButt->GetPressed();
	
	pButt=mpInputComponent->GetControlPad().GetButton(mButtonBChecksum);
	bool BPressed=pButt->GetPressed();

	// The released flag stays false until they let go of the buttons. This is so that if the player
	// was holding one of the control buttons at the instant the balance trick started, as they probably
	// will be if doing a manual, then the button won't have any effect on the balance, so as not to
	// unbalance them straight away.
	if (!APressed && !BPressed)
	{
		mManualReleased = true;
	}
	
	
	uint32   safe_period = (uint32)Script::GetInt(CRCD(0x8cc8f176,"BalanceSafeButtonPeriod"));
	uint32	 elapsed_time = Tmr::ElapsedTime(mStartTime);

	// If the buttons have not technically been released, but have been held for longer than a certain
	// amount of time, then assume that the player does want the button to have an effect after all.
	if (!mManualReleased && elapsed_time>(uint32)Script::GetInt(CRCD(0x7da1621,"BalanceIgnoreButtonPeriod")))
	{
		mManualReleased = true;
	}
	
	
	if (APressed && mManualReleased)
	{
		// this ramping up of the accleration for a few hundred miliseconds
		// will prevent us from overcompensating
		// due to the button being held down
		// and will actually make it easier to "Stick" the mManualLeanDir
		// the ramping is only in effect if you press in the direction opposite 
		// from what you need to get more upright
		// so it prevents you from becoming more unbalanced
		float mult = 1.0f;
		if (mManualLean < 0.0f && elapsed_time < safe_period)
		{
			mult =  elapsed_time / safe_period; 
		}
		
		mManualLeanDir -= scale_with_frame_length(frame_length, mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0x61ad5596,"Lean_Acc"))) * mult;
	}
	else if (BPressed && mManualReleased)
	{
		float mult = 1.0f;
		if (mManualLean > 0.0f && elapsed_time < safe_period)
		{
			mult =  elapsed_time / safe_period; 
		}
		mManualLeanDir += scale_with_frame_length(frame_length, mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0x61ad5596,"Lean_Acc"))) * mult;
	}
	else
	{
		if (Mth::Abs(mManualLeanDir) < mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0xef27e8ff,"Lean_Min_Speed")))
		{
			int r=(int)mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0x32eb3308,"Lean_Rnd_Speed"));
			r|=1;
//			mManualLeanDir = Mth::Rnd(r)-r/2;
			mManualLeanDir = Mth::Rnd(r)*Mth::Sgn(mManualLeanDir);
		}
		else
		{
			// always add some randomness to it, to make sure we don't ever get stuck
			mManualLeanDir += (Mth::Rnd(50))/100.0f * Mth::Sgn(mManualLeanDir);
		}
	}

	
	/*
	HUD::PanelMgr* panelMgr = HUD::PanelMgr::Instance();
	HUD::Panel *pPanel=panelMgr->GetPanelBySkaterId(mpSkater->GetID());
	
	char pBuf[100];
	sprintf(pBuf,"Record %.2f",mMaxTime);
	panelMgr->SendConsoleMessage(pBuf, 2);
	sprintf(pBuf,"Time %.2f",mActualManualTime);
	panelMgr->SendConsoleMessage(pBuf, 3);
	*/

	if (!mpSkater->GetScript())
	{
		mpSkater->SetScript(new Script::CScript);
	}

	if (Mth::Abs(mManualLean)>mpSkaterBalanceTrickComponent->GetBalanceStat(CRCD(0xf81ab0e9,"Lean_Bail_Angle")))
	{
		if (mManualLean>0)
		{
			mpSkater->SelfEvent(CRCD(0xd77b6ff9,"OffMeterTop"));
		}
		else	
		{
			mpSkater->SelfEvent(CRCD(0x47d44b84,"OffMeterBottom"));
		}

		mManualLean=0;
		mpSkaterBalanceTrickComponent->mBalanceTrickType=0;
	}	

	if (mWobbleEnabled)
	{
		bool flip=mPlayRangeAnimBackwards;
		if (mDoFlipCheck && GetSkaterCorePhysicsComponentFromObject(mpSkater)->GetFlag(FLIPPED))
		{
			flip=!flip;
		}	
		// Update the anim frame.
		Obj::CAnimationComponent* pAnimComponent = GetAnimationComponentFromObject(mpSkater);
        if ( pAnimComponent )
        {
            if (flip)
            {
                pAnimComponent->SetWobbleTarget((-mManualLean+4096)/8192, true);
            }
            else
            {
                pAnimComponent->SetWobbleTarget((mManualLean+4096)/8192, true);
            }		
        }
	}	


	// Update the balance meter.
	GameNet::MsgByteInfo msg;
	float final_val = -mManualLean/4096;
	
	if( server && skater_conn )
	{
		msg.m_Data = (char) ( final_val * 127.0f );
		/*server->EnqueueMessage( skater_conn->GetHandle(), GameNet::MSG_ID_SET_MANUAL_METER, 
								sizeof(GameNet::MsgByteInfo), &msg );*/
	}
	
	// Need to also check we are still doing a balance trick, since exceptions
	// that we fired above (offmetertop/bottom) might have canceled it
	if (!mNeverShowMeters && mpSkaterBalanceTrickComponent->mBalanceTrickType)
	{
		if ((mButtonAChecksum==0xbc6b118f/*Up*/ || mButtonAChecksum==0xe3006fc4/*Down*/) &&
			(mButtonBChecksum==0xbc6b118f/*Up*/ || mButtonBChecksum==0xe3006fc4/*Down*/))
		{
			if (mpSkaterScoreComponent->GetScore())
			{
				mpSkaterScoreComponent->GetScore()->SetManualMeter( true, final_val );
			}
		}
		else
		{
			if (mpSkaterScoreComponent->GetScore())
			{
				mpSkaterScoreComponent->GetScore()->SetBalanceMeter( true, final_val );
			}
		}
	}	 
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj


