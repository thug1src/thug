//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       TrickComponent.cpp
//* OWNER:          Mr Kendall Stuart Harrison Esq.
//* CREATION DATE:  3 Jan 2003
//****************************************************************************

#include <sys/config/config.h>

#include <gel/components/trickcomponent.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/statsmanagercomponent.h>

#include <gel/objtrack.h>
#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/component.h>

#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/score.h>
#include <sk/modules/skate/goalmanager.h>
#include <sk/objects/skater.h> // Needed for GetPhysicsInt(...)
#include <sk/objects/skaterprofile.h>
#include <sk/components/skaterbalancetrickcomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skatergapcomponent.h>
#include <sk/components/skaterflipandrotatecomponent.h>
#include <sk/components/skaterstatecomponent.h>
#include <sk/components/skaterruntimercomponent.h>

namespace Obj
{

static bool s_is_direction_button(uint32 buttonName);
#ifdef	__DEBUG_CODE__
static void s_add_array_of_names(Script::CStruct *p_info, int size, uint32 *p_source_array, const char *p_name);
#endif				 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Used by ExtraGapTrickLogic
static bool s_is_direction_button(uint32 buttonName)
{
	switch (buttonName)
	{
		case 0xbc6b118f: // Up
		case 0xe3006fc4: // Down
		case 0x85981897: // Left
		case 0x4b358aeb: // Right
		case 0xb7231a95: // UpLeft
		case 0xa50950c5: // UpRight
		case 0xd8847efa: // DownLeft
		case 0x786b8b68: // DownRight
			return true;
			break;
				
		default:
			break;
	}
	return false;	
}

#ifdef	__DEBUG_CODE__
// Converts an array of uint32's to a script array and inserts it into the passed structure, naming it p_name
static void s_add_array_of_names(Script::CStruct *p_info, int size, uint32 *p_source_array, const char *p_name)
{
	Script::CArray *p_array=new Script::CArray;
	if (size)
	{
		p_array->SetSizeAndType(size,ESYMBOLTYPE_NAME);
		
		for (int i=0; i<size; ++i)
		{
			p_array->SetChecksum(i,p_source_array[i]);
		}
	}
	p_info->AddArrayPointer(p_name,p_array);
}
#endif				 

// s_create is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
// s_create	returns a CBaseComponent*, as it is to be used
// by factor creation schemes that do not care what type of
// component is being created
// **  after you've finished creating this component, be sure to
// **  add it to the list of registered functions in the
// **  CCompositeObjectManager constructor  

CBaseComponent* CTrickComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CTrickComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// All components set their type, which is a unique 32-bit number
// (the CRC of their name), which is used to identify the component	
CTrickComponent::CTrickComponent() : CBaseComponent()
{
	SetType( CRC_TRICK );
	
	mpTrickMappings=NULL;
	mp_score=NULL;
	mp_input_component=NULL;
	mp_skater_balance_trick_component=NULL;
	mp_skater_core_physics_component=NULL;
	mp_skater_flip_and_rotate_component=NULL;
	mp_skater_state_component=NULL;
    mp_stats_manager_component=NULL;
				 
	Clear();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CTrickComponent::~CTrickComponent()
{
	if (mpTrickMappings)
	{
		delete mpTrickMappings;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickComponent::Finalize()
{
	mp_input_component = GetInputComponentFromObject(GetObject());
	mp_skater_balance_trick_component = GetSkaterBalanceTrickComponentFromObject(GetObject());
	mp_skater_core_physics_component = GetSkaterCorePhysicsComponentFromObject(GetObject());
	mp_skater_flip_and_rotate_component = GetSkaterFlipAndRotateComponentFromObject(GetObject());
	mp_skater_state_component = GetSkaterStateComponentFromObject(GetObject());
    mp_stats_manager_component = GetStatsManagerComponentFromObject(GetObject());
            
	Dbg_Assert(mp_input_component);
	Dbg_Assert(mp_skater_balance_trick_component);
	Dbg_Assert(mp_skater_core_physics_component);
	Dbg_Assert(mp_skater_flip_and_rotate_component);
	Dbg_Assert(mp_skater_state_component);
    Dbg_Assert(mp_stats_manager_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Gets called from the CTrickComponent constructor, and also from CSkater::ResetEverything()
void CTrickComponent::Clear()
{
	m_num_runtrick_recursions=0;
	mNumButtonsToIgnore=0;

	mUseSpecialTrickText=false;
	
	mFrontTrick = -1;
	mBackTrick = -1;
	
	ClearQueueTricksArrays();
	
	mSpecialTricksArrayChecksum = 0;
	mDoNotIgnoreMask=0;

	ClearManualTrick();
	mSpecialManualTricksArrayChecksum=0;
	
	ClearExtraGrindTrick();
	mSpecialExtraGrindTrickArrayChecksum=0;
	
	mGotExtraTricks = false;
	mExtraTricksInfiniteDuration = false;
	mExtraTricksDuration = 0;
	mExtraTricksStartTime = 0;
	
	mNumExtraTrickArrays = 0;
	mSpecialExtraTricksArrayChecksum = 0;
	mExcludedSpecialExtraTricks = 0;
	
	ClearEventBuffer();
	for (int i=0; i<MAX_TRICK_BUTTONS; ++i)
	{
		mp_trick_button[i]=0;
	}	
	
	for (int i=0; i<PAD_NUMBUTTONS; ++i)
	{
		mButtonState[i]=false;
		mButtonDebounceTime[i]=0;
	}
		
	mBashingEnabled = true;
}

Script::CStruct *CTrickComponent::get_trigger_structure(Script::CStruct *p_struct)
{
	Script::CStruct *p_trigger=NULL;
	if (Config::GetHardware()==Config::HARDWARE_XBOX)
	{
		if (!p_struct->GetStructure(CRCD(0xce42ee1a,"XBox_Trigger"),&p_trigger))
		{
			p_struct->GetStructure(CRCD(0xe594f0a2,"Trigger"),&p_trigger,Script::ASSERT);
		}
	}
	else if (Config::GetHardware()==Config::HARDWARE_NGC)
	{
		if (!p_struct->GetStructure(CRCD(0xcb0e600c,"NGC_Trigger"),&p_trigger))
		{
			p_struct->GetStructure(CRCD(0xe594f0a2,"Trigger"),&p_trigger,Script::ASSERT);
		}
	}
	else
	{
		p_struct->GetStructure(CRCD(0xe594f0a2,"Trigger"),&p_trigger,Script::ASSERT);
	}
	
	return p_trigger;
}

// K: 6/16/03 This is to implement a new feature, where as well as a Trigger structure, an Alt_Trigger
// structure can be specified. If either of Trigger or Alt_Trigger is satisfied, the trick triggers.
// The Alt_Trigger is the same for all three platforms.
Script::CStruct *CTrickComponent::get_alternate_trigger_structure(Script::CStruct *p_struct)
{
	Script::CStruct *p_trigger=NULL;
	// Note: Not passing Script::ASSERT as in get_trigger_structure above, because the Alt_Trigger is optional.
	p_struct->GetStructure(CRCD(0x68b74085,"Alt_Trigger"),&p_trigger);
	
	return p_trigger;
}
	
void CTrickComponent::ClearQueueTricksArrays()
{
	mNumQueueTricksArrays=0;
	for (int i=0; i<MAX_QUEUE_TRICK_ARRAYS; ++i)
	{
		mpQueueTricksArrays[i]=0;
	}	
}

void CTrickComponent::ClearEventBuffer()
{
	for (int i=0; i<MAX_EVENTS; ++i)
	{
		mpButtonEvents[i].EventType=EVENT_NONE;
		mpButtonEvents[i].MaybeUsed=false;
		mpButtonEvents[i].Used=false;
	}
	mNumEvents = 0;
	mLastEvent = 0;
}

// Called by the ClearManualTrick script command.
void CTrickComponent::ClearManualTrick()
{
	mGotManualTrick=false;
	mNumManualTrickArrays=0;
	
	// Just to be sure, zero the lot of 'em.
	for (int i=0; i<MAX_MANUAL_TRICK_ARRAYS; ++i)
	{
		mpManualTrickArrays[i]=0;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickComponent::UpdateTrickMappings( Obj::CSkaterProfile* pSkaterProfile )
{
	Dbg_Assert( pSkaterProfile );
	
	if (!mpTrickMappings )
	{
		mpTrickMappings = new Script::CStruct;
	}
	
	mpTrickMappings->Clear();

	// "max_specials" is not stored in the skater profile by
	// the time the skater gets here...  so just loop through
	// all 10 slots
	
	// get the special tricks from the skater profile
	// they're stored in a different format than the
	// regular tricks, so we can't just append the structure
	for ( int i = 0; i < Obj::CSkaterProfile::vMAXSPECIALTRICKSLOTS; i++ )
	{
		SSpecialTrickInfo theInfo = pSkaterProfile->GetSpecialTrickInfo( i );
		if ( !theInfo.IsUnassigned() )
		{
			if ( theInfo.m_isCat )
			{
				mpTrickMappings->AddComponent( theInfo.m_TrickSlot, ESYMBOLTYPE_INTEGER, (int)theInfo.m_TrickName );
			}
			else
				mpTrickMappings->AddComponent( theInfo.m_TrickSlot, ESYMBOLTYPE_NAME, (int)theInfo.m_TrickName );
		}
	}

	// get the regular tricks from the skater profile
	mpTrickMappings->AppendStructure( pSkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") ) );
}

/******************************************************************/
/*  Adds a single new event and returns a pointer to it.		  */
/*                                                                */
/******************************************************************/

SButtonEvent *CTrickComponent::AddEvent(EEventType EventType)
{
	Dbg_MsgAssert(mNumEvents<=MAX_EVENTS,("mNumEvents too big, = %d",mNumEvents));
	Dbg_MsgAssert(mLastEvent<MAX_EVENTS,("mLastEvent too big, = %d",mLastEvent));
	
	++mLastEvent;
	if (mLastEvent>=MAX_EVENTS)
	{
		mLastEvent=0;
	}
	if (mNumEvents<MAX_EVENTS)
	{
		++mNumEvents;
	}	
	mpButtonEvents[mLastEvent].EventType=EventType;
	mpButtonEvents[mLastEvent].Time=Tmr::ElapsedTime(0);
	mpButtonEvents[mLastEvent].MaybeUsed=false;
	mpButtonEvents[mLastEvent].Used=0;
	
	return &mpButtonEvents[mLastEvent];
}

void CTrickComponent::ResetMaybeUsedFlags()
{
	int Index=mLastEvent;
	for (int i=0; i<mNumEvents; ++i)
	{
		mpButtonEvents[Index].MaybeUsed=false;
		--Index;
		if (Index<0)
		{
			Index+=MAX_EVENTS;
		}
	}		
}

void CTrickComponent::ConvertMaybeUsedToUsed(uint32 UsedMask)
{
	int Index=mLastEvent;
	for (int i=0; i<mNumEvents; ++i)
	{
		if (mpButtonEvents[Index].MaybeUsed)
		{
			mpButtonEvents[Index].MaybeUsed=false;
			mpButtonEvents[Index].Used|=UsedMask;
		}
			
		--Index;
		if (Index<0)
		{
			Index+=MAX_EVENTS;
		}
	}		
}

// Flags as used all button events for the given button that are older than the passed time.
void CTrickComponent::RemoveOldButtonEvents(uint32 Button, uint32 OlderThan)
{
	int Index=mLastEvent;
	for (int i=0; i<mNumEvents; ++i)
	{
		if (mpButtonEvents[Index].ButtonNameChecksum==Button)
		{
			if (Tmr::ElapsedTime(mpButtonEvents[Index].Time) >= OlderThan)
			{
				mpButtonEvents[Index].Used=0xffffffff;
			}	
		}	
		--Index;
		if (Index<0)
		{
			Index+=MAX_EVENTS;
		}
	}
}

bool CTrickComponent::TriggeredInLastNMilliseconds(uint32 ButtonNameChecksum, uint32 Duration, uint32 IgnoreMask)
{
	
	
	int Index=mLastEvent;
	for (int i=0; i<mNumEvents; ++i)
	{
		if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
		{
			return false;
		}

		if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
			mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
			mpButtonEvents[Index].ButtonNameChecksum==ButtonNameChecksum)
		{
			mpButtonEvents[Index].MaybeUsed=true;
			return true;
		}	
		
		--Index;
		if (Index<0)
		{
			Index+=MAX_EVENTS;
		}	
	}
	return false;
}

bool CTrickComponent::BothTriggeredNothingInBetween(uint32 Button1, uint32 Button2, uint32 Duration, uint32 IgnoreMask)
{
	bool GotButton1=false;	
	bool GotButton2=false;	
	bool AnyOtherCancel=false;
	
	int Index=mLastEvent;
	for (int i=0; i<mNumEvents; ++i)
	{
		if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
		{
			return false;
		}

		if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
			mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED)
		{
			if (mpButtonEvents[Index].ButtonNameChecksum==Button1)
			{
				mpButtonEvents[Index].MaybeUsed=true;
				if (GotButton2)
				{
					return true;
				}
				GotButton1=true;
				AnyOtherCancel=true;
			}	
			else if (mpButtonEvents[Index].ButtonNameChecksum==Button2)
			{
				mpButtonEvents[Index].MaybeUsed=true;
				if (GotButton1)
				{
					return true;
				}
				GotButton2=true;
				AnyOtherCancel=true;
			}	
			else
			{
				if (AnyOtherCancel)
				{
					return false;
				}
			}		
		}	
		
		--Index;
		if (Index<0)
		{
			Index+=MAX_EVENTS;
		}	
	}
	return false;
}

// Reads the button names and time duration out of a 'Trigger' structure.
uint32 CTrickComponent::get_buttons_and_duration(Script::CComponent *p_comp, int num_buttons, uint32 *p_button_names)
{
	Dbg_MsgAssert(p_comp,("NULL p_comp"));
	Dbg_MsgAssert(p_button_names,("NULL p_button_names"));
	Dbg_MsgAssert(num_buttons<=MAX_TRICK_BUTTONS,("Bad num_buttons"));
	
	for (int i=0; i<num_buttons; ++i)
	{
		Dbg_MsgAssert(p_comp,("Missing button name"));
		Dbg_MsgAssert(p_comp->mType==ESYMBOLTYPE_NAME,("Bad component type, expected name"));
		
		switch (p_comp->mChecksum)
		{
			case 0x9a7dc229: // Parent1
				p_button_names[i]=mp_trick_button[0];
				break;
			case 0x03749393: // Parent2
				p_button_names[i]=mp_trick_button[1];
				break;
			case 0x7473a305: // Parent3
				p_button_names[i]=mp_trick_button[2];
				break;
			default:
				p_button_names[i]=p_comp->mChecksum;
				break;
		}
				
		p_comp=p_comp->mpNext;
	}	
	
	Dbg_MsgAssert(p_comp,("Missing duration"));
	if (p_comp->mType==ESYMBOLTYPE_INTEGER)
	{
		return p_comp->mIntegerValue;
	}	
	Dbg_MsgAssert(p_comp->mType==ESYMBOLTYPE_NAME,("Bad duration value in trick, must be either an integer or a named integer"));
	return GetPhysicsInt(p_comp->mChecksum);
}

void CTrickComponent::record_last_tricks_buttons(uint32 *p_button_names)	
{
	Dbg_MsgAssert(p_button_names,("NULL p_button_names"));
	
	// Remember what buttons were used by the last trick that got triggered.
	// They are stored so that the extra-trick trigger can use one of the parent trick's buttons,
	// rather than being hard wired. This is needed in case the player re-maps the parent trick to
	// a different button combination.
	uint32 *p_dest=mp_trick_button;
	for (int i=0; i<MAX_TRICK_BUTTONS; ++i)
	{
		*p_dest++=*p_button_names++;
	}			
}

void CTrickComponent::ButtonRecord(uint Button, bool Pressed)
{
	Dbg_MsgAssert(Button<PAD_NUMBUTTONS,("Bad Button"));
	
	if (mButtonDebounceTime[Button])
	{
		if ((!Pressed) || (Tmr::GetTime() > mButtonDebounceTime[Button]))
		{
			mButtonDebounceTime[Button] = 0;
		}
		else
		{
			return;
		}
	}
		 
	if (mButtonState[Button]==Pressed)
	{
		return;
	}	
	
	mButtonState[Button]=Pressed;


	uint32 ButtonChecksum=Inp::GetButtonChecksum( Button );
	
	// Perhaps ignore the button.
	// This feature is used by the skater to ignore the button events used to control the balance
	// when doing a manual, otherwise ChrisR can't do kickflips by very quickly jumping out of a 
	// grind & kickflipping by rolling from X to Square.
	for (int i=0; i<mNumButtonsToIgnore; ++i)
	{
		if (ButtonChecksum==mpButtonsToIgnore[i])
		{
			return;
		}
	}
			
	// Generate an event.
	if (Pressed)
	{
		SButtonEvent *pEvent=AddEvent(EVENT_BUTTON_PRESSED);
		Dbg_MsgAssert(pEvent,("NULL pEvent ?"));
		pEvent->ButtonNameChecksum=ButtonChecksum;
	}
	else
	{
		SButtonEvent *pEvent=AddEvent(EVENT_BUTTON_RELEASED);
		Dbg_MsgAssert(pEvent,("NULL pEvent ?"));
		pEvent->ButtonNameChecksum=ButtonChecksum;
	}
}

bool CTrickComponent::GetButtonState(uint32 Checksum)
{
	return mButtonState[Inp::GetButtonIndex(Checksum)];
}

// Note: This probably needs a lot of optimization, hmmm.
bool CTrickComponent::QueryEvents(Script::CStruct *pQuery, uint32 UsedMask, uint32 IgnoreMask)
{
	if (!pQuery)
	{
		return false;
	}	
	
	if (mp_input_component->IsInputDisabled())
	{
		return false;
	}
		
	//Script::PrintContents(pQuery);	
    Script::CComponent *pComp=pQuery->GetNextComponent(NULL);
	if (!pComp)
	{
		return false;
	}	
	
	Dbg_MsgAssert(pComp->mType==ESYMBOLTYPE_NAME,("Bad component type, expected name"));
	uint32 NameChecksum=pComp->mChecksum;
	pComp=pComp->mpNext;
	
	uint32 p_button_names[MAX_TRICK_BUTTONS];
	for (int b=0; b<MAX_TRICK_BUTTONS; ++b)
	{
		p_button_names[b]=0;
	}	
	
	switch (NameChecksum)
	{
		case 0x14fe9b7d: //	AirTrickLogic
		{
			// Get two button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,2,p_button_names);
			
			if (GetButtonState(p_button_names[1]))
			{
				// Search for the event saying that button 2 got pressed, and flag it as used.
				// If this was not done, we'd be able to get two kickflips by doing left-square,
				// then letting go of left and pressing square again.
				int Index=mLastEvent;
				for (int i=0; i<mNumEvents; ++i)
				{
					if (mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						break;
					}	
			
					--Index;
					if (Index<0)
					{
						Index+=MAX_EVENTS;
					}	
				}
			
				if (TriggeredInLastNMilliseconds(p_button_names[0],Duration,IgnoreMask))
				{
					ConvertMaybeUsedToUsed(UsedMask);
					record_last_tricks_buttons(p_button_names);
					return true;
				}
			}
			else
			{
				if (BothTriggeredNothingInBetween(p_button_names[0],p_button_names[1],Duration,IgnoreMask))
				{
					ConvertMaybeUsedToUsed(UsedMask);
					record_last_tricks_buttons(p_button_names);
					return true;
				}
			}
			break;
		}	
		
		case 0xffb718bd: // PressAndRelease
		{
			// Get two button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,2,p_button_names);

			int Index=mLastEvent;
			int Count=0;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}
				
				switch (Count)
				{
				case 0:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=1;
					}	
					break;
				case 1:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						ConvertMaybeUsedToUsed(UsedMask);
						record_last_tricks_buttons(p_button_names);
						return true;
					}	
					break;
				default:
					break;
				}
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}	
		case 0x2c434c90: // TapTwiceRelease
		{
			// Get two button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,2,p_button_names);

			int Index=mLastEvent;
			int Count=0;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}
			
				switch (Count)
				{
				case 0:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						(mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED && 
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1]))
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=1;
					}	
					break;
				case 1:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=2;
					}	
					break;
				case 2:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						ConvertMaybeUsedToUsed(UsedMask);
						record_last_tricks_buttons(p_button_names);
						return true;
					}	
					break;
				default:
					break;
				}
				
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}	
		case 0x696e5e66: // ReleaseAndTap
		{
			// Get two button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,2,p_button_names);

			int e[3]={-1,-1,-1};
				
			int index=mLastEvent;
			int count=0;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (!(mpButtonEvents[index].Used & IgnoreMask))
				{
					uint32 button=mpButtonEvents[index].ButtonNameChecksum;
					if (button != CRCD(0xbc6b118f,"up") &&
						button != CRCD(0xe3006fc4,"down") &&
						button != CRCD(0x85981897,"left") &&
						button != CRCD(0x4b358aeb,"right"))
					{
						e[count++]=index;
						if (count==3)
						{
							break;
						}	
					}
				}
					
				--index;
				if (index<0)
				{
					index+=MAX_EVENTS;
				}	
			}
			
			if (e[0]==-1 || e[1]==-1 || e[2]==-1)
			{
				return false;
			}
				
			if (Tmr::ElapsedTime(mpButtonEvents[e[0]].Time)>=Duration ||
				Tmr::ElapsedTime(mpButtonEvents[e[1]].Time)>=Duration ||
				Tmr::ElapsedTime(mpButtonEvents[e[2]].Time)>=Duration)
			{
				return false;
			}	

			if (mpButtonEvents[e[0]].EventType==EVENT_BUTTON_RELEASED && 
				mpButtonEvents[e[0]].ButtonNameChecksum==p_button_names[1] &&
                mpButtonEvents[e[1]].EventType==EVENT_BUTTON_PRESSED && 
				mpButtonEvents[e[1]].ButtonNameChecksum==p_button_names[1] &&
                mpButtonEvents[e[2]].EventType==EVENT_BUTTON_RELEASED && 
				mpButtonEvents[e[2]].ButtonNameChecksum==p_button_names[0])
			{
				mpButtonEvents[e[0]].MaybeUsed=true;
				mpButtonEvents[e[1]].MaybeUsed=true;
				mpButtonEvents[e[2]].MaybeUsed=true;
				ConvertMaybeUsedToUsed(UsedMask);
				record_last_tricks_buttons(p_button_names);
				return true;
			}
			break;
		}	
		case 0x3c3028f4: // PressTwo
		{
			// Get two button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,2,p_button_names);
			
			if (GetButtonState(p_button_names[0]) && GetButtonState(p_button_names[1]))
			{
				if (BothTriggeredNothingInBetween(p_button_names[0],p_button_names[1],Duration,IgnoreMask))
				{
					ConvertMaybeUsedToUsed(UsedMask);
					record_last_tricks_buttons(p_button_names);
					return true;
				}
			}		
			break;
		}	
		
		case 0x7d482318: // InOrder
		{
			// Get two button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,2,p_button_names);
			
			int Index=mLastEvent;
			int Count=0;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}
			
				switch (Count)
				{
				case 0:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=1;
					}	
					break;
				case 1:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						ConvertMaybeUsedToUsed(UsedMask);
						record_last_tricks_buttons(p_button_names);
						return true;
					}	
					break;
				default:
					break;

				}
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}	

		case 0x3f369070: // PressTwoAnyOrder
		{
			int i;
			int Index;
			
			// Get two button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,2,p_button_names);
			
			bool Found = false;
			
			bool FirstPressed = false;
			bool SecondPressed = false;
			Index=mLastEvent;
			for (i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}
			
				if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
					mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
				{
					if (mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED)
					{
						mpButtonEvents[Index].MaybeUsed=true;
						FirstPressed=false;
						SecondPressed=false;
					}
					else if (mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED)
					{
						mpButtonEvents[Index].MaybeUsed=true;
						if (SecondPressed)
						{
							Found=true;
							break;
						}
						FirstPressed=true;
					}
				}	
			
				if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
					mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
				{
					if (mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED)
					{
						mpButtonEvents[Index].MaybeUsed=true;
						FirstPressed=false;
						SecondPressed=false;
					}
					else if (mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED)
					{
						mpButtonEvents[Index].MaybeUsed=true;
						if (FirstPressed)
						{
							Found=true;
							break;
						}
						SecondPressed=true;
					}
				}	
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			
			if (Found)
			{
				mpButtonEvents[Index].MaybeUsed=true;
				ConvertMaybeUsedToUsed(UsedMask);
				record_last_tricks_buttons(p_button_names);
				return true;
			}
			else
			{
				return false;
			}
		}	
		
		// Various different TripleInOrder's, which became necessary when using it for different types of
		// tricks such as the boneless, and the special grinds.
		// It was found that the logic needs to be quite sloppy for the boneless so that they can get triggered
		// easily otherwise it feels wrong, whereas it needs to be stricter for special grinds otherwise 
		// they go off all the time.
		
		// Requires that Button1 be pressed, followed by Button2 pressed, followed by Button3 pressed.
		// Doesn't care when they get released.
		case 0xc1ad35c0: // TripleInOrderSloppy
		{
			// Get three button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,3,p_button_names);
			
			int Index=mLastEvent;
			int Count=0;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}
			
				switch (Count)
				{
				case 0:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[2])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=1;
					}	
					break;
				case 1:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=2;
					}	
					break;
				case 2:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						ConvertMaybeUsedToUsed(UsedMask);
						record_last_tricks_buttons(p_button_names);
						return true;
					}	
					break;
				default:
					break;

				}
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}	
		// Button1 must be pressed then released, then Button2 must be pressed then released,
		// then Button3 must be pressed. Doesn't require that Button3 be released, because it
		// feels better if the logic takes effect exactly on the 3rd press.
		case 0x3fe7c311: // TripleInOrderStrict
		{
			// Get three button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,3,p_button_names);
			
			int Index=mLastEvent;
			int Count=0;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}
			
				switch (Count)
				{
				case 0:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[2])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=1;
					}	
					break;
				case 1:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=2;
					}	
					break;
				case 2:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=3;
					}	
					break;
				case 3:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=4;
					}	
					break;
				case 4:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						ConvertMaybeUsedToUsed(UsedMask);
						record_last_tricks_buttons(p_button_names);
						return true;
					}	
					break;
				default:
					break;

				}
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}	
		// Button1 must be pressed then released, followed by either button2 pressed then button3 pressed,
		// or button3 pressed then button2 pressed.
		// Sort of strict & sloppy at the same time.
		case 0xa2e042d7: // TripleInOrder
		{
			// Get three button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,3,p_button_names);
			
			int Index=mLastEvent;
			int Count=0;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}
			
				switch (Count)
				{
				case 0:
					// Even though it's called TripleInOrder, buttons 2 and 3
					// can be pressed in either order, so we do a bit of convoluted
					// branching here to check for both cases.
					if (!(mpButtonEvents[Index].Used & IgnoreMask))
					{
						if (mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
							mpButtonEvents[Index].ButtonNameChecksum==p_button_names[2])
						{
							mpButtonEvents[Index].MaybeUsed=true;
							Count=1;
							break;
						}	
						if (mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
							mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
						{
							mpButtonEvents[Index].MaybeUsed=true;
							Count=2;
							break;
						}	
					}	
					break;
					
					
				case 1:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=3;
					}	
					break;
					
				case 2:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[2])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=3;
					}	
					break;
					
					
				// Button 1 has to be pressed and released before the other 2.	
				case 3:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=4;
					}	
					break;
				case 4:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						ConvertMaybeUsedToUsed(UsedMask);
						record_last_tricks_buttons(p_button_names);
						return true;
					}	
					break;
				default:
					break;

				}
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}
		case 0x823b8342: // Press
		{
			// Get one button name followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,1,p_button_names);
			
			int Index=mLastEvent;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}
			
				if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
					mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
					mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
				{
					mpButtonEvents[Index].Used|=UsedMask;
					record_last_tricks_buttons(p_button_names);
					return true;
				}	
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}	
		case 0x61b8fce2: // Release
		{
			// Get one button name followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,1,p_button_names);
			
			int Index=mLastEvent;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}
				
				if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
					mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED &&
					mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
				{
					mpButtonEvents[Index].Used|=UsedMask;
					record_last_tricks_buttons(p_button_names);
					return true;
				}	
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}	
		case 0x7fa5cdbb: // Tap
		{
			// Get one button name followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,1,p_button_names);
			
			int Count=0;
			int Index=mLastEvent;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}
				
				if (Count == 0)
				{
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count++;
					}
				}
				else
				{
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						ConvertMaybeUsedToUsed(UsedMask);
						return true;
					}
				}
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}	
		case 0xf630dae5: // HoldTwoAndPress
		{
			// Get three button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,3,p_button_names);
			
			int Index=mLastEvent;
			int Count=0;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}
			
				switch (Count)
				{
				case 0:
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[2])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						Count=1;
						break;
					}	
					break;
					
				case 1:
					if (!(mpButtonEvents[Index].Used & IgnoreMask))
					{
						if (mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
						{
							if (mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED)
							{
								mpButtonEvents[Index].MaybeUsed=true;
								Count=2;
							}
							else if (mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED)
							{
								return false;
							}	
						}
						else if (mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
						{
							if (mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED)
							{
								mpButtonEvents[Index].MaybeUsed=true;
								Count=3;
							}
							else if (mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED)
							{
								return false;
							}	
						}
					}		
					break;
					
				case 2:
					if (!(mpButtonEvents[Index].Used & IgnoreMask))
					{
						if (mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
						{
							if (mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED)
							{
								mpButtonEvents[Index].MaybeUsed=true;
								ConvertMaybeUsedToUsed(UsedMask);
								record_last_tricks_buttons(p_button_names);
								return true;
							}
							else if (mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED)
							{
								return false;
							}	
						}
					}		
					break;

				case 3:
					if (!(mpButtonEvents[Index].Used & IgnoreMask))
					{
						if (mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
						{
							if (mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED)
							{
								mpButtonEvents[Index].MaybeUsed=true;
								ConvertMaybeUsedToUsed(UsedMask);
								record_last_tricks_buttons(p_button_names);
								return true;
							}
							else if (mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED)
							{
								return false;
							}	
						}
					}		
					break;
					
				default:
					break;

				}
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}	

		case 0xac2b1445: // ReleaseTwoAndPress
		{
			// Get three button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,3,p_button_names);
			
			int Index=mLastEvent;
			bool got_a_released=false;
			bool got_b_released=false;
			bool got_c_pressed=false;
			
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}

				if (!got_a_released)
				{
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						got_a_released=true;
					}	
				}					
				if (!got_b_released)
				{
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						got_b_released=true;
					}	
				}					
				if (!got_c_pressed)
				{
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[2])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						got_c_pressed=true;
					}	
				}					


				if (got_a_released && !got_b_released)
				{
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						return false;
					}	
				}					
				if (got_b_released && !got_a_released)
				{
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
					{
						return false;
					}	
				}					
				
				if (got_a_released && got_b_released && got_c_pressed)
				{
					ConvertMaybeUsedToUsed(UsedMask);
					record_last_tricks_buttons(p_button_names);
					return true;
				}	
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}	

		case 0x395ec2d4: // ReleaseTwo
		{
			// Get two button names followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,2,p_button_names);
			
			int Index=mLastEvent;
			bool got_a_released=false;
			bool got_b_released=false;
			
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					return false;
				}

				if (!got_a_released)
				{
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						got_a_released=true;
					}	
				}					
				if (!got_b_released)
				{
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
					{
						mpButtonEvents[Index].MaybeUsed=true;
						got_b_released=true;
					}	
				}					

				if (got_a_released && !got_b_released)
				{
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						return false;
					}	
				}					
				if (got_b_released && !got_a_released)
				{
					if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
						mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED &&
						mpButtonEvents[Index].ButtonNameChecksum==p_button_names[1])
					{
						return false;
					}	
				}					
				
				if (got_a_released && got_b_released)
				{
					ConvertMaybeUsedToUsed(UsedMask);
					record_last_tricks_buttons(p_button_names);
					return true;
				}	
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}
			break;
		}	

		case 0xb6258557: // ExtraGrabTrickLogic
		{
			uint32 last_direction_button=0;
			for (int b=0; b<MAX_TRICK_BUTTONS; ++b)
			{
				if (s_is_direction_button(mp_trick_button[b]))
				{
					last_direction_button=mp_trick_button[b];
				}
			}		
			
			// Get one button name followed by a duration value.
			uint32 Duration=get_buttons_and_duration(pComp,1,p_button_names);
			
			bool do_trigger=false;
			int pressed_index=0;
			
			int Index=mLastEvent;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
				{
					break;
				}
			
				if (!(mpButtonEvents[Index].Used & IgnoreMask) &&
					mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED)
				{
					if (mpButtonEvents[Index].ButtonNameChecksum==p_button_names[0])
					{
						// OK, so the button has been pressed, but don't return true
						// just yet cos we want to keep looking back in time & kill any
						// direction button events that are the same direction button as
						// used by last trick.
						do_trigger=true;
						pressed_index=Index;
					}	
					else if (s_is_direction_button(mpButtonEvents[Index].ButtonNameChecksum))
					{
						// A direction button has gotten pushed during the same time interval ...
						
						if (mpButtonEvents[Index].ButtonNameChecksum != last_direction_button)
						{
							// It is not the same as the previous trick's direction button,
							// so do not trigger at all. Run away without killing any events.
							return false;
						}	
						else
						{
							// It is the same as the previous trick's direction button,
							// so flag it as maybe used.
							// If we survive the rest of this loop without the above 'return true'
							// firing then the MaybeUsed flags will get converted to used after
							// the loop has finished.
							mpButtonEvents[Index].MaybeUsed=true;
						}	
					}
				}	
				
				--Index;
				if (Index<0)
				{
					Index+=MAX_EVENTS;
				}	
			}

			if (do_trigger)
			{
				// Kill the events for sure.
				mpButtonEvents[pressed_index].Used|=UsedMask;
				ConvertMaybeUsedToUsed(UsedMask);
				record_last_tricks_buttons(p_button_names);
				return true;
			}
			break;
		}

		case 0xa8123ecf: // HoldThree			
		{
			Script::CComponent *p_comp=pComp;
			
			Dbg_MsgAssert(p_comp && p_comp->mType==ESYMBOLTYPE_NAME,("HoldThree logic requires 3 button names"));
			uint32 button1=p_comp->mChecksum;
			p_comp=p_comp->mpNext;
			Dbg_MsgAssert(p_comp && p_comp->mType==ESYMBOLTYPE_NAME,("HoldThree logic requires 3 button names"));
			uint32 button2=p_comp->mChecksum;
			p_comp=p_comp->mpNext;
			Dbg_MsgAssert(p_comp && p_comp->mType==ESYMBOLTYPE_NAME,("HoldThree logic requires 3 button names"));
			uint32 button3=p_comp->mChecksum;
			
			return GetButtonState(button1) && GetButtonState(button2) && GetButtonState(button3);
			break;
		}
			
		default:
			Dbg_MsgAssert(0,("Unexpected name '%s'",Script::FindChecksumName(NameChecksum)));
			break;
	}		

	ResetMaybeUsedFlags();
	return false;
}

/*****************************************************************************
**																			**
**							Trick queue stuff								**
**																			**
*****************************************************************************/

void CTrickComponent::ClearTrickQueue()
{
	
	mFrontTrick=mBackTrick=-1;
}

void CTrickComponent::AddTrick(uint32 ArrayChecksum, uint Index, bool UseSpecialTrickText)
{
	int i=0;
	if (mFrontTrick==-1 && mBackTrick==-1)
	{
		// The queue is empty, so initialise the front and back.
		mFrontTrick=i;
		mBackTrick=i;
	}
	else
	{
		Dbg_MsgAssert(mFrontTrick!=-1,("mFrontTrick = -1 ??"));
		Dbg_MsgAssert(mBackTrick!=-1,("mBackTrick = -1 ??"));
		
		// Go to the current back of the queue
		i=mBackTrick;
		// Advance to the next entry
		++i;
		if (i>=TRICK_QUEUE_SIZE)
		{
			i=0;
		}
		// If reached the front again then there is no room left,
		// so don't do anything.
		if (i==mFrontTrick)
		{
			return;
		}
		// Got a new back of the queue.
		mBackTrick=i;			
	}	
	
	// Write in the info.
	mpTricks[i].ArrayChecksum=ArrayChecksum;
	mpTricks[i].Index=Index;
	mpTricks[i].UseSpecialTrickText=UseSpecialTrickText;
}

void CTrickComponent::RemoveFrontTrick()
{
	
	// If the queue is empty then there is nothing to do.
	if (mFrontTrick==-1)
	{
		return;
	}	
	
	Dbg_MsgAssert(mBackTrick!=-1,("mBackTrick = -1 ??"));
	
	// If only one element in the queue, then clear the queue.
	if (mFrontTrick==mBackTrick)
	{
		ClearTrickQueue();
		return;
	}	
	
	// Advance the front trick to the next entry.
	++mFrontTrick;
	if (mFrontTrick>=TRICK_QUEUE_SIZE)
	{
		mFrontTrick=0;
	}	
}

// This will remove all tricks that came from a certain trick array.
// This is used by the ClearTricksFrom command, which is used by air-trick
// scripts to remove any boneless's as soon as the air trick is triggered, so that 
// the boneless does not trigger afterwards. It is possible to trigger a boneless for
// a short time after becoming airborne due to the ground-gone exception, this is
// the 'late-jump'.
void CTrickComponent::ClearTricksFrom(uint32 ArrayChecksum)
{
	
	
	int i;
	
	// Remove the array from the list of arrays being checked.
	for (i=0; i<mNumQueueTricksArrays; ++i)
	{
		if (mpQueueTricksArrays[i]==ArrayChecksum)
		{
			// Shift everything down
			for (int j=i; j<mNumQueueTricksArrays-1; ++j)
			{
				mpQueueTricksArrays[j]=mpQueueTricksArrays[j+1];
			}	
			// Oooh, changing the for-loop limit inside the for-loop!
			--mNumQueueTricksArrays;
		}
	}	
	
	// Remove any tricks already in the trick queue that came from the specified array.
	for (i=0; i<TRICK_QUEUE_SIZE; ++i)
	{
		if (mpTricks[i].ArrayChecksum==ArrayChecksum)
		{
			// Cancel the trick by zeroing its checksum.
			mpTricks[i].ArrayChecksum=0;
		}
	}
}

bool CTrickComponent::TrickIsDefined(Script::CStruct *pTrick)
{
	
	Dbg_MsgAssert(pTrick,("NULL pTrick"));
	
	uint32 ScriptChecksum=0;
	if (pTrick->GetChecksum(0xa6d2d890/* Scr */,&ScriptChecksum))
	{
		return true;
	}
	if (pTrick->GetChecksum(0x22e168c1/* Scripts */,&ScriptChecksum))
	{
		return true;
	}
	Script::CArray *p_template_array=NULL;
	if (pTrick->GetArray(0x689fe07c/* Template */,&p_template_array))
	{
		return true;
	}


	// Get the checksum of the slot ...
	uint32 TrickSlotChecksum=0;
	if (pTrick->GetChecksum(0xa92a2280/* TrickSlot */,&TrickSlotChecksum))
	{
		// Look up this slot in the trick mappings ...
		Dbg_MsgAssert(mpTrickMappings,("NULL mpTrickMappings ?"));
		uint32 TrickChecksum=0;
		mpTrickMappings->GetChecksum(TrickSlotChecksum,&TrickChecksum);
	
		if (TrickChecksum)
		{
			// Now, look up the global structure with this name.
			if (Script::GetStructure(TrickChecksum))
			{
				return true;
			}
		}
		else
		{
			// If the trick slot is defined to be an integer, then it is referring
			// to a create-a-trick.
			int create_a_trick=0;
			if (mpTrickMappings->GetInteger(TrickSlotChecksum,&create_a_trick))
			{
				return true;
			}	
		}
	}			
	return false;
}

bool CTrickComponent::RunTrick(Script::CStruct *pTrick, uint32 optionalFlag, Script::CStruct* pExtraParams)
{
	Dbg_MsgAssert(pTrick,("NULL pTrick"));
	
    uint32 ScriptChecksum=0;
    pTrick->GetChecksum(CRCD(0xa6d2d890,"Scr"),&ScriptChecksum);
	
	// Dan: HACK
	// final check to insure that we don't do a grind extra trick when we're not on a rail; if the scripts were perfect, this would never occur; however,
	// this should prevent an obscure net crash
	if (ScriptChecksum == CRCD(0x255ed86f, "Grind") && mp_skater_core_physics_component->GetRailNode() == -1)
	{
		return false;
	}
    
    // counting fliptricks and grabtricks for stats goals
    Script::CStruct* pParams;
    if(pTrick->GetStructure(CRCD(0x7031f10c,"Params"),&pParams))
    {
        if (!pParams->ContainsComponentNamed(CRCD(0x7a16aca0,"IsExtra")) )
        {
            mp_stats_manager_component->SetTrickType( ScriptChecksum );
        }
    }
	
#ifdef __NOPT_ASSERT__
	if (m_num_runtrick_recursions>5)
	{
		#ifdef	__NOPT_ASSERT__
		printf("WARNING! More than 5 RunTrick recursions\nScriptChecksum='%s'",Script::FindChecksumName(ScriptChecksum));
		#endif
		return false;
	}	
#endif		// __NOPT_ASSERT__
	
	if (ScriptChecksum)
	{
		// Increment this so that too many recursions can be detected.
		++m_num_runtrick_recursions;
		
		// A trick is triggered, so clear any manual or special grind trick that might have got triggered before.
		mGotManualTrick=false;
		mGotExtraGrindTrick=false;
	
		// Also clear the do-not-ignore mask, which may have got set by the last SetQueueTricks command.
		mDoNotIgnoreMask=0;
		
		// Change the skater script to be the trick script.
		Script::CStruct *pScriptParams=NULL;
		pTrick->GetStructure(CRCD(0x7031f10c,"Params"),&pScriptParams);
	
		// Run the script.
		mp_skater_flip_and_rotate_component->DoAnyFlipRotateOrBoardRotateAfters(); // <- See huge comment above definition of this function.
		
		CCompositeObject *p_object=GetObject();
		Dbg_MsgAssert(p_object,("Trick component has NULL object ?"));
		p_object->SwitchScript(ScriptChecksum,pScriptParams);

		// If a flag value was passed in, insert it into the script's parameters.
		if (optionalFlag)
		{
			// Note that the flag is being inserted into the script's params after calling
			// SetScript rather than putting it into pScriptParams.
			// This is because pScriptParams could be pointing into some global structure, and
			// that should be kept read-only.
			Script::CStruct *p_script_params=p_object->GetScript()->GetParams();
			Dbg_MsgAssert(p_script_params,("NULL p_script_params"));
			p_script_params->AddChecksum(NONAME,optionalFlag);
		}
		
		p_object->GetScript()->GetParams()->AppendStructure(pExtraParams);
			
		p_object->GetScript()->Update();
	
		// Set the mDoingTrick flag so that the camera can detect that a trick is being done.
		mp_skater_state_component->SetDoingTrick( true );
	
		// Increment the trick count, which may set pedestrian exceptions to
		// make them go "oooOOOOOooooh"
		IncrementNumTricksInCombo();
		
		// Decrement the recursion counter now that this chunk of code has completed.
		--m_num_runtrick_recursions;
		return true;
	}	
	else
	{
		#ifdef	__NOPT_ASSERT__
		uint32 trickslot=0;
		if (!pTrick->GetChecksum(CRCD(0xa92a2280,"TrickSlot"),&trickslot))
		{
			printf("WARNING! Scr not defined in trick structure!\n");
			Script::PrintContents(pTrick);
		}	
		#endif
	}			
	
	return false;
}

/******************************************************************/
/* Runs the next trick in the queue.							  */
/******************************************************************/
void CTrickComponent::TriggerNextQueuedTrick(uint32 scriptToRunFirst, Script::CStruct *p_scriptToRunFirstParams, Script::CStruct* pExtraParams)
{
	mp_skater_state_component->SetDoingTrick( false );
	mUseSpecialTrickText=false;

	while (true)
	{
		if (mFrontTrick==-1)
		{
			// No tricks left in the queue, so break.
			break;
		}
			
		// Grab the array checksum & index of the trick within the array,
		// then remove the trick from the queue.
		// Removing the trick straight away in case the new script also does
		// a DoNextTrick, which would cause infinite recursion if this trick
		// was still in the queue.
		uint32 ArrayChecksum=mpTricks[mFrontTrick].ArrayChecksum;
		uint Index=mpTricks[mFrontTrick].Index;
		// Set this flag so that any special trick scripts that do get executed during the RunTrick
		// will use the yellow text if they execute a Display command. 
		mUseSpecialTrickText=mpTricks[mFrontTrick].UseSpecialTrickText;
		
		RemoveFrontTrick();

		// The ArrayChecksum might be zero, this would indicate that 
		// the trick got cancelled by a call to ClearTricksFrom().
		if (ArrayChecksum)
		{
			// The DoNextTrick command can specify a script that must be run before the trick script.
			// So if one was specified, run it.
			if (scriptToRunFirst)
			{
				GetObject()->AllocateScriptIfNeeded();
				GetObject()->GetScript()->Interrupt(scriptToRunFirst, p_scriptToRunFirstParams);
			}
				
			// Get the trick array that the trick belongs to.
			Script::CArray *pArray=Script::GetArray(ArrayChecksum);
			Dbg_MsgAssert(pArray,("NULL pArray ?"));
			
			// Get the structure defining the trick.
			Script::CStruct *pTrickStruct=pArray->GetStructure(Index);
			Dbg_MsgAssert(pTrickStruct,("NULL pTrickStruct ???"));
			
			// Try running the trick the old way, which is where the trick script is
			// specified directly in the structure. 
			if (!RunTrick(pTrickStruct, 0, pExtraParams))
			{
				// That didn't work, so they must be wanting to specify a trick slot
				// instead, so get the trickslot ...
				
				// Get the checksum of the slot ...
				uint32 TrickSlotChecksum=0;
				pTrickStruct->GetChecksum(CRCD(0xa92a2280,"TrickSlot"),&TrickSlotChecksum);
			
				if (TrickSlotChecksum)
				{
					// Look up this slot in the trick mappings ...
					Dbg_MsgAssert(mpTrickMappings,("NULL mpTrickMappings ?"));
				
					// Maybe it is a create-a-trick, in which case it is referred to by an
					// integer index value.
					int create_a_trick=0;
					if (mpTrickMappings->GetInteger(TrickSlotChecksum,&create_a_trick))
					{
						// There is a create-a-trick defined!
						// Run the special CreateATrick script passing the index as a parameter.
						Script::CStruct *p_params=new Script::CStruct;
						p_params->AddInteger(CRCD(0xb4a39841,"trick_index"),create_a_trick);
						
						p_params->AppendStructure(pExtraParams);
						
						CCompositeObject *p_object=GetObject();
						Dbg_MsgAssert(p_object,("Trick component has NULL object ?"));
						p_object->SwitchScript(CRCD(0x2d90485d,"CreateATrick"),p_params);
						delete p_params;
							
						p_object->GetScript()->Update();
						
						// Set the mDoingTrick flag so that the camera can detect that a trick is being done.
						mp_skater_state_component->SetDoingTrick( true );
					
						// Increment the trick count, which may set pedestrian exceptions to
						// make them go "oooOOOOOooooh"
						IncrementNumTricksInCombo();
					}
					else
					{
						uint32 TrickChecksum=0;
						mpTrickMappings->GetChecksum(TrickSlotChecksum,&TrickChecksum);
					
						if (TrickChecksum)
						{
							// Now, look up the global structure with this name.
							
							Script::CStruct *pTrick=Script::GetStructure(TrickChecksum);
							if (pTrick)
							{
								RunTrick(pTrick, 0, pExtraParams);
							}
						}
					}
				}			
			}
			
			// The ArrayChecksum was not zero, so break.
			// We only keep looping if the ArrayChecksum is zero, so that all the zeros get skipped over.
			// If the ArrayChecksum is zero this indicates that the trick got cancelled by a call to
			// ClearTricksFrom().
			break;
		}
	}
}

/******************************************************************/
/* Scans through a trick array, and adds any tricks that got   	  */
/* triggered to the trick queue.                                  */
/******************************************************************/
void CTrickComponent::MaybeAddTrick(uint32 ArrayChecksum, bool UseSpecialTrickText, uint32 DoNotIgnoreMask)
{
	

	Dbg_MsgAssert(ArrayChecksum,("Zero ArrayChecksum sent to MaybeAddTrick"));
		
	// Resolve the checksum into a CArray pointer.
	// The array is stored by checksum in case it gets reloaded.
	Script::CArray *pArray=Script::GetArray(ArrayChecksum);
		
	// The above would have asserted if the array wasn't found, but check pArray isn't NULL anyway.
	if (pArray)
	{
		// Scan through the array checking each trick's trigger condition.
		int Size=pArray->GetSize();
		for (int i=0; i<Size; ++i)
		{
			Script::CStruct *pStruct=pArray->GetStructure(i);
			Dbg_MsgAssert(pStruct,("NULL pStruct ???"));
			// pStruct is the structure defining the trick.
			
			if (TrickIsDefined(pStruct))
			{
				// Get the trigger, which is a structure defining the button combination that triggers the trick.
				Script::CStruct *p_trigger=get_trigger_structure(pStruct);
				// An alternate trigger may also be specified. This will also trigger the trick.
				Script::CStruct *p_alternate_trigger=get_alternate_trigger_structure(pStruct);
			
				// p_trigger could be NULL, but OK cos QueryEvents will return false in that case.
				// The DoNotIgnoreMask specifies additional used-events that should not be ignored.
				uint32 ignore_mask=(~USED_BY_MANUAL_TRICK) & (~DoNotIgnoreMask);
				
				if (QueryEvents(p_trigger,USED_BY_REGULAR_TRICK,ignore_mask) ||
					QueryEvents(p_alternate_trigger,USED_BY_REGULAR_TRICK,ignore_mask))
				{
					// The conditions are met, so add the trick to the trick queue.
					// The trick is specified by the name of the array it is in, and its index within that array.
					AddTrick(ArrayChecksum,i,UseSpecialTrickText);
				}
			}	
		}
	}
}
	
void CTrickComponent::AddTricksToQueue()
{
	

	// If a special tricks array is specified, and we're in the special state, check them.
	if (mSpecialTricksArrayChecksum)
	{
		Mdl::Score *pScore=GetScoreObject();
		if (pScore->GetSpecialState())
		{
			MaybeAddTrick(mSpecialTricksArrayChecksum,USE_SPECIAL_TRICK_TEXT,mDoNotIgnoreMask);
		}
	}	

	// Check the usual arrays.
	for (int TrickArrayIndex=0; TrickArrayIndex<mNumQueueTricksArrays; ++TrickArrayIndex)
	{
		// Wouldn't expect any of the checksums to be zero.
		Dbg_MsgAssert(mpQueueTricksArrays[TrickArrayIndex],("Zero mpQueueTricksArrays[%d]",TrickArrayIndex));
		MaybeAddTrick(mpQueueTricksArrays[TrickArrayIndex],0,mDoNotIgnoreMask);
	}
}

void CTrickComponent::TriggerAnyManualTrick(Script::CStruct *pExtraParams)
{
	uint32 scriptToRunFirst=0;
	Script::CStruct *pScriptToRunFirstParams=NULL;
	pExtraParams->GetChecksum(CRCD(0x148fee96,"ScriptToRunFirst"),&scriptToRunFirst);
	pExtraParams->GetStructure(CRCD(0x7031f10c,"Params"),&pScriptToRunFirstParams);

	mUseSpecialTrickText=false;
	
	if (mGotManualTrick)
	{
		// Now that the trick is being done, remove it.
		// Removing it before changing the script rather than after to
		// prevent any possibility of infinite recursion.
		mGotManualTrick=false;
		
		
		// The DoNextManualTrick command can specify a script that must be run before the trick script.
		// So if one was specified, run it.
		if (scriptToRunFirst)
		{
			// we need to copy and restore the script parameters as they will be corrupted by the interrupt
			Script::CStruct extraParamsCopy(*pExtraParams);
			GetObject()->AllocateScriptIfNeeded();
			GetObject()->GetScript()->Interrupt(scriptToRunFirst, pScriptToRunFirstParams);
			*pExtraParams = extraParamsCopy;
		}
		
		// Get the trick array that the trick belongs to.
		Script::CArray *pArray=Script::GetArray(mManualTrick.ArrayChecksum);
		if (pArray)
		{
			// Get the structure defining the trick.
			Script::CStruct *pStruct=pArray->GetStructure(mManualTrick.Index);
			Dbg_MsgAssert(pStruct,("NULL pStruct ???"));
			

			// Get the checksum of the slot if there is one ...
			uint32 TrickSlotChecksum=0;
			pStruct->GetChecksum(CRCD(0xa92a2280,"TrickSlot"),&TrickSlotChecksum);
			
			if (TrickSlotChecksum)
			{
				// Look up this slot in the trick mappings ...
				Dbg_MsgAssert(mpTrickMappings,("NULL mpTrickMappings ?"));
				uint32 TrickChecksum=0;
				mpTrickMappings->GetChecksum(TrickSlotChecksum,&TrickChecksum);
				
				if (TrickChecksum)
				{
					// Now, look up the global structure with this name.
					
					Script::CStruct *pTrick=Script::GetStructure(TrickChecksum);
					if (pTrick)
					{
						// Now change pStruct to be pTrick, so that Scr and Params get read out of it instead.
						pStruct=pTrick;
					}
				}
			}
			// If there is no TrickSlot specified, then use pStruct as is, to maintain backwards compatibility.			

			
			// Read the script checksum of the trick.
			uint32 ScriptChecksum=0;
			pStruct->GetChecksum(CRCD(0xa6d2d890,"Scr"),&ScriptChecksum);
			if (ScriptChecksum)
			{
				mp_skater_flip_and_rotate_component->DoAnyFlipRotateOrBoardRotateAfters(); // <- See huge comment above definition of this function.
				mUseSpecialTrickText=mManualTrick.UseSpecialTrickText;
			
				// Change the skater script to be the trick script.
				Script::CStruct *pScriptParams=NULL;
				pStruct->GetStructure(CRCD(0x7031f10c,"Params"),&pScriptParams);
				
				CCompositeObject *p_object=GetObject();
				Dbg_MsgAssert(p_object,("Trick component has NULL object ?"));

				if (pExtraParams)
				{
					Dbg_MsgAssert(pExtraParams!=pScriptParams,("Eh ??"));
					
					// If extra params (params following the DoNextTrick command) were
					// specified, then merge the pScriptParams onto them and use them.
					pExtraParams->AppendStructure(pScriptParams);
					
					p_object->SwitchScript(ScriptChecksum,pExtraParams);
				}
				else
				{
					p_object->SwitchScript(ScriptChecksum,pScriptParams);
				}
				p_object->GetScript()->Update();
								
				// Set the mDoingTrick flag so that the camera can detect that a trick is being done.
				mp_skater_state_component->SetDoingTrick( true );
				
				// Increment the trick count, which may set pedestrian exceptions to
				// make them go "oooOOOOOooooh"
				IncrementNumTricksInCombo();
			}	
		}
	}
}
	
void CTrickComponent::CheckManualTrickArray(uint32 ArrayChecksum, uint32 IgnoreMask, bool UseSpecialTrickText)
{
	
	Dbg_MsgAssert(ArrayChecksum,("Zero ArrayChecksum sent to CheckManualTrickArray"));
	
	// Resolve the checksum into a CArray pointer.
	// The array is stored by checksum in case it gets reloaded.
	Script::CArray *pArray=Script::GetArray(ArrayChecksum);
		
	// The above would have asserted if the array wasn't found, but check pArray isn't NULL anyway.
	if (pArray)
	{
		// Scan through the array checking each trick.
		int Size=pArray->GetSize();
		for (int t=0; t<Size; ++t)
		{
			Script::CStruct *pStruct=pArray->GetStructure(t);
			Dbg_MsgAssert(pStruct,("NULL pStruct ???"));
			// pStruct is the structure defining the trick.
			
			if (TrickIsDefined(pStruct))
			{
				// Get the trigger, which is a structure defining the button combination that triggers the trick.
				Script::CStruct *p_trigger=get_trigger_structure(pStruct);
				// An alternate trigger may also be specified. This will also trigger the trick.
				Script::CStruct *p_alternate_trigger=get_alternate_trigger_structure(pStruct);
				
				
				// p_trigger could be NULL, but OK cos QueryEvents will return false in that case.
				if (QueryEvents(p_trigger,USED_BY_MANUAL_TRICK,IgnoreMask) ||
					QueryEvents(p_alternate_trigger,USED_BY_MANUAL_TRICK,IgnoreMask))
				{
					// The conditions are met, so add the manual trick.
					mManualTrick.ArrayChecksum=ArrayChecksum;
					mManualTrick.Index=t;
					mManualTrick.Time=Tmr::GetTime();
					mManualTrick.Duration=0xffffffff;
					mManualTrick.UseSpecialTrickText=UseSpecialTrickText;
					pStruct->GetInteger(0x79a07f3f/*Duration*/,(int*)&mManualTrick.Duration);
					mGotManualTrick=true;
				}
			}	
		}
	}
}
	
// Always called every frame.
void CTrickComponent::MaybeQueueManualTrick()
{
	

	// Check any special manual tricks.
	if (mSpecialManualTricksArrayChecksum)
	{
		// Got extra manual tricks ...
		Mdl::Score *pScore=GetScoreObject();
		if (pScore->GetSpecialState())
		{
			// And we're special, so check the tricks.
			// The ~USED_BY_MANUAL_TRICK means don't ignore button events that have already been
			// used by manuals. This is so that any normal manual that has just got queued won't
			// steal the events needed by a up-down-triangle special manual for example.
			CheckManualTrickArray(mSpecialManualTricksArrayChecksum,~USED_BY_MANUAL_TRICK,USE_SPECIAL_TRICK_TEXT);
			if (mGotManualTrick)
			{
				return;
			}	
		}
	}
	
	// For each of the trick arrays, check whether any of the tricks listed within are triggered.
	for (int i=0; i<mNumManualTrickArrays; ++i)
	{
		Dbg_MsgAssert(mpManualTrickArrays[i],("Zero mpManualTrickArrays[i]"));
		CheckManualTrickArray(mpManualTrickArrays[i]);
	}
}

void CTrickComponent::MaybeExpireManualTrick()
{
	
	if (mGotManualTrick && mManualTrick.Duration!=0xffffffff)
	{
		if (Tmr::ElapsedTime(mManualTrick.Time)>mManualTrick.Duration)
		{
			mGotManualTrick=false;
		}	
	}	
}

/*****************************************************************************
**																			**
**							Extra trick stuff								**
**																			**
*****************************************************************************/

bool CTrickComponent::TriggerAnyExtraTrick(uint32 ArrayChecksum, uint32 ExcludedTricks)
{
	

	bool TriggeredATrick=false;
	
	Dbg_MsgAssert(ArrayChecksum,("Zero ArrayChecksum sent to TriggerAnyExtraTrick"));
	Script::CArray *pArray=Script::GetArray(ArrayChecksum);
	
	// The above would have asserted if the array wasn't found, but check pArray isn't NULL anyway.
	if (pArray)
	{
		// Scan through the array checking each trick.
		int Size=pArray->GetSize();
		// Only 32 bits available in the mpExcludedExtraTricks entries.
		Dbg_MsgAssert(Size<=32,("Extra trick array '%s' has more than 32 entries",Script::FindChecksumName(ArrayChecksum)));
		
		for (int i=0; i<Size; ++i)
		{
			if (!(ExcludedTricks & (1<<i)))
			{
				Script::CStruct *pTrickStruct=pArray->GetStructure(i);
				Dbg_MsgAssert(pTrickStruct,("NULL pTrickStruct ???"));
				// pTrickStruct is the structure defining the trick.
				
				if (TrickIsDefined(pTrickStruct))
				{
					// Get the trigger, which is a structure defining the button combination that triggers the trick.
					Script::CStruct *p_trigger=get_trigger_structure(pTrickStruct);
					// An alternate trigger may also be specified. This will also trigger the trick.
					Script::CStruct *p_alternate_trigger=get_alternate_trigger_structure(pTrickStruct);
					
					// p_trigger could be NULL, but OK cos QueryEvents will return false in that case.
					
					// The USED_BY_EXTRA_TRICK flags value means flag any button events used as being
					// used by the extra tricks.
					if (QueryEvents(p_trigger,USED_BY_EXTRA_TRICK,~USED_BY_MANUAL_TRICK) ||
						QueryEvents(p_alternate_trigger,USED_BY_EXTRA_TRICK,~USED_BY_MANUAL_TRICK))
					{
						// Run the trick ...
					
						// Try running the trick the old way, which is where the trick script is
						// specified directly in the structure. 
						// Passing the flag IsExtra to the script, so that the script can tell
						// whether it was run as an extra trick or a regular trick.
						if (!RunTrick(pTrickStruct,0x7a16aca0/*IsExtra*/))
						{
							// That didn't work, so they must be wanting to specify a trick slot
							// instead, so get the trickslot ...
					
							// Get the checksum of the slot ...
							uint32 TrickSlotChecksum=0;
							pTrickStruct->GetChecksum(0xa92a2280/*TrickSlot*/,&TrickSlotChecksum);
					
							if (TrickSlotChecksum)
							{
								// Look up this slot in the trick mappings ...
								Dbg_MsgAssert(mpTrickMappings,("NULL mpTrickMappings ?"));
								uint32 TrickChecksum=0;
								mpTrickMappings->GetChecksum(TrickSlotChecksum,&TrickChecksum);
					
								if (TrickChecksum)
								{
									// Now, look up the global structure with this name.
					
									Script::CStruct *pTrick=Script::GetStructure(TrickChecksum);
									if (pTrick)
									{
										// Passing the flag IsExtra to the script, so that the script can tell
										// whether it was run as an extra trick or a regular trick.
										if (RunTrick(pTrick,0x7a16aca0/*IsExtra*/))
										{
											TriggeredATrick=true;
										}	
									}
								}
							}
						}
						else
						{
							TriggeredATrick=true;
						}	
					}
							
					// Check the status of mGotExtraTricks again, because it might have got
					// reset by a call to KillExtraTricks in any script run above.		
					if (!mGotExtraTricks)
					{
						return TriggeredATrick;
					}	
				}	
			}
		}
	}	
	
	return TriggeredATrick;
}

// Called every frame.
void CTrickComponent::TriggerAnyExtraTricks()
{
	

	// Check whether any special extra tricks need to get triggered.
	if (mGotExtraTricks)
	{
		if (!mExtraTricksInfiniteDuration && Tmr::ElapsedTime(mExtraTricksStartTime)>mExtraTricksDuration)
		{
			// If not infinite duration, and the time is up, then stop checking for extra tricks.
			mGotExtraTricks=false;
		}
		else
		{
			// Check any special extra tricks if there are some & we're special.
			if (mSpecialExtraTricksArrayChecksum)
			{
				Mdl::Score *pScore=GetScoreObject();
				if (pScore->GetSpecialState())
				{
					// Set this flag so that any trick scripts that do get executed during the TriggerAnyExtraTrick
					// will use the yellow text if they execute a Display command. 
					mUseSpecialTrickText=true;
					if (!TriggerAnyExtraTrick(mSpecialExtraTricksArrayChecksum,mExcludedSpecialExtraTricks))
					{
						// No trick was run, so reset this flag just to be sure that it doesn't cause a non-special
						// trick triggered later to get displayed in yellow.
						mUseSpecialTrickText=false;
					}	
					
					// Check the status of mGotExtraTricks again, because it might have got
					// reset by a call to KillExtraTricks in any script run above.		
					if (!mGotExtraTricks)
					{
						return;
					}	
				}	
				else
				{
					mUseSpecialTrickText=false;
				}	
			}
			
			// Check all the extra-trick arrays
			for (int i=0; i<mNumExtraTrickArrays; ++i)
			{
				if (TriggerAnyExtraTrick(mpExtraTrickArrays[i],mpExcludedExtraTricks[i]))
				{
					mUseSpecialTrickText=false;
				}	
									
				// Check the status of mGotExtraTricks again, because it might have got
				// reset by a call to KillExtraTricks in any script run above.		
				if (!mGotExtraTricks)
				{
					return;
				}	
			}
		}
	}
}
	
void CTrickComponent::SetExtraTricks(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	Dbg_MsgAssert(pParams,("NULL pParams"));
	Dbg_MsgAssert(pScript,("NULL pScript"));
	
	mGotExtraTricks=true;
	
	mExtraTricksInfiniteDuration=true;
	float Duration=0.0f;
	if (pParams->GetFloat(CRCD(0x79a07f3f,"Duration"),&Duration))
	{
		mExtraTricksInfiniteDuration=false;
		mExtraTricksStartTime=Tmr::ElapsedTime(0);
		// Convert Duration, which is assumed to be a number of 60ths, to milliseconds.
		mExtraTricksDuration=(uint32)(Duration*100/6.0f);
	}	

	mNumExtraTrickArrays=0;

	// If a single Tricks parameter is specified, then use that. 
	uint32 ExtraTrickArrayChecksum=0;
	if (pParams->GetChecksum(CRCD(0x1e26fd3e,"Tricks"),&ExtraTrickArrayChecksum))
	{
		mNumExtraTrickArrays=1;
		mpExtraTrickArrays[0]=ExtraTrickArrayChecksum;
	}
	else
	{
		// Otherwise, assume all the un-named names in the parameter list are the trick arrays required.
		Script::CComponent *pComp=NULL;
		while (true)
		{
			pComp=pParams->GetNextComponent(pComp);
			if (!pComp)
			{
				break;
			}
		
			if (pComp->mNameChecksum==0 && pComp->mType==ESYMBOLTYPE_NAME)
			{
				// Found a name, so add it to the array.
				Dbg_MsgAssert(mNumExtraTrickArrays<MAX_EXTRA_TRICK_ARRAYS,("\n%s\nToo many extra trick arrays, ask Ken to increase MAX_EXTRA_TRICK_ARRAYS",pScript->GetScriptInfo()));
				Dbg_MsgAssert(pComp->mChecksum,("Zero checksum ???"));
				mpExtraTrickArrays[mNumExtraTrickArrays++]=pComp->mChecksum;
			}	
		}
	}

	// Set any special extra tricks required.
	mSpecialExtraTricksArrayChecksum=0;
	pParams->GetChecksum(CRCD(0xb394c01c,"Special"),&mSpecialExtraTricksArrayChecksum);
	
	// Make sure no tricks are going to be excluded first of all ...
	for (int i=0; i<MAX_EXTRA_TRICK_ARRAYS; ++i)
	{
		mpExcludedExtraTricks[i]=0;
	}	
	mExcludedSpecialExtraTricks=0;
	
	// Exclude any 'Ignore' tricks specified. May be just one, may be an array of them.
	const char *pExtraTrickToIgnore="";
	Script::CArray *pArrayOfTricksToIgnore=NULL;
	if (pParams->GetLocalText(CRCD(0xf277291d,"Ignore"),&pExtraTrickToIgnore))
	{
		ExcludeExtraTricks(pExtraTrickToIgnore);
	}	
	else if (pParams->GetArray(CRCD(0xf277291d,"Ignore"),&pArrayOfTricksToIgnore))
	{
		Dbg_MsgAssert(pArrayOfTricksToIgnore,("Eh ? NULL pArrayOfTricksToIgnore ??"));
		int Size=pArrayOfTricksToIgnore->GetSize();
		for (int i=0; i<Size; ++i)
		{
			ExcludeExtraTricks(pArrayOfTricksToIgnore->GetLocalString(i));
		}
	}		
}
	
void CTrickComponent::KillExtraTricks()
{
	
	mGotExtraTricks=false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 									Extra grind trick stuff
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Called by the ClearExtraGrindTrick script command.
void CTrickComponent::ClearExtraGrindTrick()
{
	mGotExtraGrindTrick=false;
	mNumExtraGrindTrickArrays=0;
	
	// Just to be sure, zero the lot of 'em.
	for (int i=0; i<MAX_EXTRA_GRIND_TRICK_ARRAYS; ++i)
	{
		mpExtraGrindTrickArrays[i]=0;
	}	
}

// Called from CSkater::MaybeStickToRail()
// Returns true if it did trigger a trick, false otherwise.
bool CTrickComponent::TriggerAnyExtraGrindTrick(bool Right, bool Parallel, bool Backwards, bool Regular)
{
	if (mGotExtraGrindTrick)
	{
		// Now that the trick is being done, remove it.
		// Removing it before changing the script rather than after to
		// prevent any possibility of infinite recursion.
		mGotExtraGrindTrick=false;
		
		// Get the trick array that the trick belongs to.
		Script::CArray *pArray=Script::GetArray(mExtraGrindTrick.ArrayChecksum);
		if (pArray)
		{
			// Get the structure defining the trick.
			Script::CStruct *pStruct=pArray->GetStructure(mExtraGrindTrick.Index);
			Dbg_MsgAssert(pStruct,("NULL pStruct ???"));
			
			Script::CArray *pScriptArray=NULL;
			pStruct->GetArray(CRCD(0x22e168c1,"Scripts"),&pScriptArray);
			
			// The scripts array can alternatively be specified using a template array and a prefix.
			Script::CArray *p_template_array=NULL;
			pStruct->GetArray(CRCD(0x689fe07c,"Template"),&p_template_array);
			const char *p_prefix=NULL;
			pStruct->GetString(CRCD(0x6c4e7971,"Prefix"),&p_prefix);
			
			if (!(pScriptArray || p_template_array))
			{
				// No array parameters found, so look for a TrickSlot
				uint32 TrickSlotChecksum=0;
				pStruct->GetChecksum(CRCD(0xa92a2280,"TrickSlot"),&TrickSlotChecksum);
				if (TrickSlotChecksum)
				{
					// Look up this slot in the trick mappings ...
					Dbg_MsgAssert(mpTrickMappings,("NULL mpTrickMappings ?"));
					uint32 TrickChecksum=0;
					mpTrickMappings->GetChecksum(TrickSlotChecksum,&TrickChecksum);
				
					if (TrickChecksum)
					{
						// Now, look up the global structure with this name.
						Script::CStruct *pFoo=Script::GetStructure(TrickChecksum);
						if (pFoo)
						{
							pFoo->GetArray(CRCD(0x22e168c1,"Scripts"),&pScriptArray);
							pFoo->GetArray(CRCD(0x689fe07c,"Template"),&p_template_array);
							pFoo->GetString(CRCD(0x6c4e7971,"Prefix"),&p_prefix);
						}
					}		
				}
			}	

			// Calculate the index into the scripts array, which ahs size 16
			uint Index=0;
			if (Right) Index|=1;
			if (Parallel) Index|=2;
			if (Backwards) Index|=4;
			if (Regular) Index|=8;

				
			uint32 script_checksum=0;
			if (pScriptArray)
			{
				script_checksum=pScriptArray->GetNameChecksum(Index);
			}
			else
			{
				// The grind script arrays all tend to follow the same pattern, with all the
				// script names having a common prefix, with the suffix following a fixed pattern.
				// So to save Scott having to add a whole new array for each new grind trick, a
				// template array can be specified instead.
				// This template array is an array of 16 suffix strings. The full script name can 
				// then be calculated given the prefix.
				Dbg_MsgAssert(p_template_array,("No template array found for entry %d of array '%s'",mExtraGrindTrick.Index,Script::FindChecksumName(mExtraGrindTrick.ArrayChecksum)));
				Dbg_MsgAssert(p_prefix,("No script prefix specified for entry %d of array '%s'",mExtraGrindTrick.Index,Script::FindChecksumName(mExtraGrindTrick.ArrayChecksum)));
				
				// Look up the suffix to use.
				const char *p_suffix=p_template_array->GetString(Index);
				char p_temp[100];
				Dbg_MsgAssert(strlen(p_prefix)+strlen(p_suffix)<100,("Oops, grind script name '%s%s' too long",p_prefix,p_suffix));
				// Generate the full script name
				sprintf(p_temp,"%s%s",p_prefix,p_suffix);
				// and hence calculate the script name checksum.
				script_checksum=Script::GenerateCRC(p_temp);
			}
				
			Script::CStruct *pParams=NULL;
			pStruct->GetStructure(CRCD(0x7031f10c,"Params"),&pParams);
			

			// Initialise mGrindTweak, which should get set by a SetGrindTweak command
			// in the script that is about to be run.
			// TODO: Is there a neater way of doing this?
			CCompositeObject *p_object=GetObject();
			if (p_object->GetType()==SKATE_TYPE_SKATER)
			{
				mp_skater_core_physics_component->ResetGrindTweak();
			}

			// Set this flag so that any special trick scripts that do get executed during the mp_script->Update()
			// will use the yellow text if they execute a Display command. 
			mUseSpecialTrickText=mExtraGrindTrick.UseSpecialTrickText;
		
			mp_skater_flip_and_rotate_component->DoAnyFlipRotateOrBoardRotateAfters(); // <- See huge comment above definition of this function.
			
			p_object->SwitchScript(script_checksum,pParams);
			p_object->GetScript()->Update();
			
			// Set the mDoingTrick flag so that the camera can detect that a trick is being done.
			mp_skater_state_component->SetDoingTrick( true );

			// Increment the trick count, which may set pedestrian exceptions to
			// make them go "oooOOOOOooooh"
			IncrementNumTricksInCombo();
			
			return true;
		}
	}
	
	return false;
}

// Checks a single array of grind tricks.
void CTrickComponent::MaybeQueueExtraGrindTrick(uint32 ArrayChecksum, bool UseSpecialTrickText)
{
	
	Dbg_MsgAssert(ArrayChecksum,("Zero ArrayChecksum sent to MaybeQueueExtraGrindTrick"));

	// Resolve the checksum into a CArray pointer.
	// The array is stored by checksum in case it gets reloaded.
	Script::CArray *pArray=Script::GetArray(ArrayChecksum);
		
	// The above would have asserted if the array wasn't found, but check pArray isn't NULL anyway.
	if (pArray)
	{
		// Scan through the array checking each trick.
		int Size=pArray->GetSize();
		for (int t=0; t<Size; ++t)
		{
			Script::CStruct *pStruct=pArray->GetStructure(t);
			Dbg_MsgAssert(pStruct,("NULL pStruct ???"));
			// pStruct is the structure defining the trick.
			
			if (TrickIsDefined(pStruct))
			{
				// Get the trigger, which is a structure defining the button combination that triggers the trick.
				Script::CStruct *p_trigger=get_trigger_structure(pStruct);
				// An alternate trigger may also be specified. This will also trigger the trick.
				Script::CStruct *p_alternate_trigger=get_alternate_trigger_structure(pStruct);
				
				// p_trigger could be NULL, but OK cos QueryEvents will return false in that case.
				if (QueryEvents(p_trigger,USED_BY_EXTRA_GRIND_TRICK,~USED_BY_MANUAL_TRICK) ||
					QueryEvents(p_alternate_trigger,USED_BY_EXTRA_GRIND_TRICK,~USED_BY_MANUAL_TRICK))
				{
					// The conditions are met, so add the special grind trick.
					mExtraGrindTrick.ArrayChecksum=ArrayChecksum;
					mExtraGrindTrick.Index=t;
					mExtraGrindTrick.Time=Tmr::GetTime();
					mExtraGrindTrick.Duration=0xffffffff;
					mExtraGrindTrick.UseSpecialTrickText=UseSpecialTrickText;
					pStruct->GetInteger(0x79a07f3f/*Duration*/,(int*)&mExtraGrindTrick.Duration);
					mGotExtraGrindTrick=true;
				}
			}	
		}
	}
}
	
// Always called every frame.
void CTrickComponent::MaybeQueueExtraGrindTrick()
{
	
	
	// If got a special special grind trick array, and we're special, check it.
	if (mSpecialExtraGrindTrickArrayChecksum)
	{
		Mdl::Score *pScore=GetScoreObject();
		if (pScore->GetSpecialState())
		{
			MaybeQueueExtraGrindTrick(mSpecialExtraGrindTrickArrayChecksum,USE_SPECIAL_TRICK_TEXT);
		}	
	}
	
	// Note: Should this function carry on checking once mGotExtraGrindTrick becomes true?
	// Currently it does, so the first trick triggered will be overridden by any further special
	// grind. Check with Scott.
	
	// For each of the trick arrays, check whether any of the tricks listed within are triggered.
	for (int i=0; i<mNumExtraGrindTrickArrays; ++i)
	{
		Dbg_MsgAssert(mpExtraGrindTrickArrays[i],("Zero mpExtraGrindTrickArrays[%d]",i));
		MaybeQueueExtraGrindTrick(mpExtraGrindTrickArrays[i]);
	}
}

// Also always called every frame.
void CTrickComponent::MaybeExpireExtraGrindTrick()
{
	
	if (mGotExtraGrindTrick && mExtraGrindTrick.Duration!=0xffffffff)
	{
		if (Tmr::ElapsedTime(mExtraGrindTrick.Time)>mExtraGrindTrick.Duration)
		{
			// It's past its sell by date, so kill it.
			mGotExtraGrindTrick=false;
		}	
	}	
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Returns true if the passed trick has the name pIgnoreName.
bool CTrickComponent::IsExcluded(Script::CStruct *pTrick, const char *pIgnoreName)
{
	
	
	Dbg_MsgAssert(pTrick,("NULL pTrick"));
	
	// See if it has a trick slot.
	uint32 TrickSlotChecksum=0;
	if (pTrick->GetChecksum(0xa92a2280/*TrickSlot*/,&TrickSlotChecksum))
	{
		// Look up this slot in the trick mappings ...
		Dbg_MsgAssert(mpTrickMappings,("NULL mpTrickMappings ?"));
		uint32 TrickChecksum=0;
		mpTrickMappings->GetChecksum(TrickSlotChecksum,&TrickChecksum);
							
		if (TrickChecksum)
		{
			// Now, look up the global structure with this name.
			pTrick=Script::GetStructure(TrickChecksum);
			if (!pTrick)
			{
				return false;
			}
		}
	}

	Script::CStruct *pParams=NULL;
	Dbg_MsgAssert(pTrick,("Eh ?  NULL pTrick"));
	pTrick->GetStructure(CRCD(0x7031f10c,"Params"),&pParams);
	if (pParams)
	{
		const char *pName=NULL;
		pParams->GetLocalText(CRCD(0xa1dc81f9,"Name"),&pName);
		if (pName)
		{
			Dbg_MsgAssert(pIgnoreName,("NULL pIgnoreName"));
			// Compare pName and pIgnoreName.
			// If they match, return true so that the trick gets excluded.
			if (stricmp(pName,pIgnoreName)==0)
			{
				return true;
			}	
		}
	}		
		
	return false;
}				

// Returns a bitfield indicating which of the entries in the passed array are excluded because they have the passed Id.
uint32 CTrickComponent::CalculateIgnoreMask(uint32 ArrayChecksum, const char *pIgnoreName)
{
	
	Dbg_MsgAssert(ArrayChecksum,("Zero ArrayChecksum sent to CalculateIgnoreMask"));
	Script::CArray *pArray=Script::GetArray(ArrayChecksum);
	
	int Size=pArray->GetSize();
	// Only 32 bits available ...
	Dbg_MsgAssert(Size<=32,("Extra-trick array '%s' has more than 32 entries",Script::FindChecksumName(ArrayChecksum)));
			
	uint32 Mask=0;	
	for (int i=0; i<Size; ++i)
	{
		Script::CStruct *pTrick=pArray->GetStructure(i);
		if (IsExcluded(pTrick,pIgnoreName))
		{
			Mask |= (1<<i);
		}
	}
	
	return Mask;			
}

// Runs through all the extra-tricks arrays flagging as excluded all those with the name pIgnoreName
void CTrickComponent::ExcludeExtraTricks(const char *pIgnoreName)
{
	
	for (int i=0; i<mNumExtraTrickArrays; ++i)
	{
		// Note: Or-ing on top of what is already there rather than setting equal to the
		// passed mask, because ExcludeExtraTricks may be called on a set of IgnoreId's.
		mpExcludedExtraTricks[i] |= CalculateIgnoreMask(mpExtraTrickArrays[i],pIgnoreName);
	}
	
	// Also do the special extra tricks.
	if (mSpecialExtraTricksArrayChecksum)
	{
		mExcludedSpecialExtraTricks |= CalculateIgnoreMask(mSpecialExtraTricksArrayChecksum,pIgnoreName);
	}	
}

void CTrickComponent::RecordButtons()
{
	uint32 input_mask = mp_input_component->GetInputMask();
	
	uint32 Direction = CSkaterPad::sGetDirection(
		input_mask & Inp::Data::mA_UP,
		input_mask & Inp::Data::mA_DOWN,
		input_mask & Inp::Data::mA_LEFT,
		input_mask & Inp::Data::mA_RIGHT
	);
	
	ButtonRecord(PAD_U, Direction == PAD_U);
	ButtonRecord(PAD_D, Direction == PAD_D);
	ButtonRecord(PAD_L, Direction== PAD_L);
	ButtonRecord(PAD_R, Direction == PAD_R);
	ButtonRecord(PAD_UL, Direction == PAD_UL);
	ButtonRecord(PAD_UR, Direction == PAD_UR);
	ButtonRecord(PAD_DL, Direction == PAD_DL);
	ButtonRecord(PAD_DR, Direction == PAD_DR);
	
	ButtonRecord(PAD_CIRCLE, input_mask & Inp::Data::mA_CIRCLE);
	ButtonRecord(PAD_SQUARE, input_mask & Inp::Data::mA_SQUARE);
	ButtonRecord(PAD_TRIANGLE, input_mask & Inp::Data::mA_TRIANGLE);
	ButtonRecord(PAD_X, input_mask & Inp::Data::mA_X);
	ButtonRecord(PAD_L1, input_mask & Inp::Data::mA_L1);
	ButtonRecord(PAD_L2, input_mask & Inp::Data::mA_L2);
	ButtonRecord(PAD_L3, input_mask & Inp::Data::mA_L3);
	ButtonRecord(PAD_R1, input_mask & Inp::Data::mA_R1);
	ButtonRecord(PAD_R2, input_mask & Inp::Data::mA_R2);
	ButtonRecord(PAD_R3, input_mask & Inp::Data::mA_R3);
	ButtonRecord(PAD_BLACK, input_mask & Inp::Data::mA_BLACK);
	ButtonRecord(PAD_WHITE, input_mask & Inp::Data::mA_WHITE);
	ButtonRecord(PAD_Z, input_mask & Inp::Data::mA_Z);
}

void CTrickComponent::DumpEventBuffer()
{
	printf("Event buffer:\n");
	int Index=mLastEvent;
	int n=mNumEvents;
	if (n>10) n=10;
	for (int i=0; i<n; ++i)
	{
		printf("T=%d Used=0x%08x Button=",mpButtonEvents[Index].Time,mpButtonEvents[Index].Used);
		
		switch (mpButtonEvents[Index].ButtonNameChecksum)
		{
		case 0x0:  printf("Nothing"); break;
		case 0xbc6b118f:  printf("Up"); break;
		case 0xe3006fc4:  printf("Down"); break;
		case 0x85981897:  printf("Left"); break;
		case 0x4b358aeb:  printf("Right"); break;
		case 0xb7231a95:  printf("UpLeft"); break;
		case 0xa50950c5:  printf("UpRight"); break;
		case 0xd8847efa:  printf("DownLeft"); break;
		case 0x786b8b68:  printf("DownRight"); break;
		case 0x2b489a86:  printf("Circle"); break;
		case 0x321c9756:  printf("Square"); break;
		case 0x7323e97c:  printf("X"); break;
		case 0x20689278:  printf("Triangle"); break;
		case 0x26b0c991:  printf("L1"); break;
		case 0xbfb9982b:  printf("L2"); break;
		case 0xf2f1f64e:  printf("R1"); break;
		case 0x6bf8a7f4:  printf("R2"); break;
		case 0x767a45d7:  printf("Black"); break;
		case 0xbd30325b:  printf("White"); break;
		default: break;
		}
		printf(" Event=");
		
		switch (mpButtonEvents[Index].EventType)
		{
			case EVENT_NONE:  printf("None"); break;
			case EVENT_BUTTON_PRESSED:  printf("Pressed"); break;
			case EVENT_BUTTON_RELEASED:  printf("Released"); break;
			default: break;
		}
		printf("\n");
			
		--Index;
		if (Index<0)
		{
			Index+=MAX_EVENTS;
		}
	}		
}
	
void CTrickComponent::HandleBashing()
{
	if (!mBashingEnabled) return;
	
	CAnimationComponent* p_animation_component = GetAnimationComponentFromObject(GetObject());
	Dbg_Assert(p_animation_component);
	p_animation_component->SetAnimSpeed(GetBashFactor(), true);
}

// Calculates the amount by which the skater's animation needs to be sped up
// due to the player bashing buttons.
// The animation speed needs to be multiplied by the value that this returns.
float CTrickComponent::GetBashFactor()
{
	uint32 Duration=GetPhysicsInt(CRCD(0x6c83fdbf,"BashPeriod"));
	
	// Scan through all the events in the last BashPeriod milliseconds,
	// counting all the button press and release events.
	int Bashes=0;
	int Index=mLastEvent;
	for (int i=0; i<mNumEvents; ++i)
	{
		if (Tmr::ElapsedTime(mpButtonEvents[Index].Time)>=Duration)
		{
			break;
		}
	
		if (mpButtonEvents[Index].EventType==EVENT_BUTTON_PRESSED ||
			mpButtonEvents[Index].EventType==EVENT_BUTTON_RELEASED)
		{
			// Flag the event as used, so that it won't register once the bail has finished.
			mpButtonEvents[Index].Used=0xffffffff;
			++Bashes;
		}	
		
		--Index;
		if (Index<0)
		{
			Index+=MAX_EVENTS;
		}	
	}
	
	// Increase the anim speed based on the number of bashes.
	
	float bash_factor=Bashes*GetPhysicsFloat(CRCD(0xced14273, "BashSpeedupFactor"));
	float max=GetPhysicsFloat(CRCD(0xd24abfa3, "BashMaxPercentSpeedup"))/100.0f;
	if (bash_factor > max)
	{
		bash_factor=max;
	}	
	
	return 1.0f+bash_factor;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently isfrequently the contents of a node
// but you can pass in anything you like.	
void CTrickComponent::InitFromStructure( Script::CStruct* pParams )
{
	Dbg_MsgAssert(GetObject()->GetType() == SKATE_TYPE_SKATER, ("CTrickComponent added to non-skater composite object"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickComponent::Update()
{
	
	// nonlocal clients have a SkaterCorePhysicsComponent only to provide uber_frig functionality
	if (!GetSkater()->IsLocalClient())
	{
		Suspend(true);
		return;
	}
	
	RecordButtons();
	
	mp_skater_balance_trick_component->ExcludeBalanceButtons(mNumButtonsToIgnore, mpButtonsToIgnore);
	
	TriggerAnyExtraTricks();
	AddTricksToQueue();
	MaybeQueueManualTrick();
	MaybeExpireManualTrick();
	MaybeQueueExtraGrindTrick();
	MaybeExpireExtraGrindTrick();
	HandleBashing();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the "Checksum" of a script command, then possibly handle it
// if it's a command that this component will handle	
CBaseComponent::EMemberFunctionResult CTrickComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | DumpEventBuffer | dumps the last ten button events
		case 0x72c926ca: // DumpEventBuffer
			DumpEventBuffer();
			break;

        // @script | Held | true if the specified button is being held
        // @uparmopt name | Single button name
		// @parmopt array | Buttons | | Optional array of button names. If specified, then if
		// any of those buttons are held it will return true.
		case 0xeda2772e: // Held
		{
			Script::CArray *p_array=NULL;
			if (pParams->GetArray(CRCD(0xbca37a49,"Buttons"),&p_array))
			{
				// If an array of buttons is specified, check each of them.
				int n=p_array->GetSize();
				for (int i=0; i<n; ++i)
				{
					if (GetButtonState(p_array->GetChecksum(i)))
					{
						return CBaseComponent::MF_TRUE;
					}
				}
				return CBaseComponent::MF_FALSE;		
			}
			else
			{
			uint32 ButtonChecksum=0;
			pParams->GetChecksum(NONAME,&ButtonChecksum);
			if (!ButtonChecksum)
			{
				return CBaseComponent::MF_FALSE;
			}	
			return GetButtonState(ButtonChecksum) ? CBaseComponent::MF_TRUE:CBaseComponent::MF_FALSE;
			}	
			break;
		}
			
        // @script | Released | true if the specified button is released
        // @uparm X | button name
		case 0x4ba9ee9: // Released	
		{
			uint32 ButtonChecksum=0;
			pParams->GetChecksum(NONAME,&ButtonChecksum);
			if (!ButtonChecksum)
			{
				return CBaseComponent::MF_FALSE;
			}	
			return GetButtonState(ButtonChecksum) ? CBaseComponent::MF_FALSE:CBaseComponent::MF_TRUE;
			break;
		}

/*
        // @script | DoNextTrick | runs the next trick in the queue
		// @parmopt name | ScriptToRunFirst | | Script that will be run first before any trick script.
		// @parmopt structure | Params | | Parameters to pass to ScriptToRunFirst
		
		case 0x09d58f25: // DoNextTrick
		{
			uint32 scriptToRunFirst=0;
			Script::CStruct *pScriptToRunFirstParams=NULL;
			pParams->GetChecksum(CRCD(0x148fee96,"ScriptToRunFirst"),&scriptToRunFirst);
			pParams->GetStructure(CRCD(0x7031f10c,"Params"),&pScriptToRunFirstParams);
			TriggerNextQueuedTrick(scriptToRunFirst, pScriptToRunFirstParams);
			break;
		}
*/
		
         // @script | ClearTricksFrom | 
		case 0xcb30572d: // ClearTricksFrom
		{
			// Run through all the parameters.
			Script::CComponent *pComp=NULL;
			while (true)
			{
				pComp=pParams->GetNextComponent(pComp);
				if (!pComp)
				{
					break;
				}
			
				if (pComp->mNameChecksum==0 && pComp->mType==ESYMBOLTYPE_NAME)
				{
					// It's an unnamed name, so remove the tricks that came from the array with that name.
					ClearTricksFrom(pComp->mChecksum);
				}	
			}
			break;
		}

        // @script | SetQueueTricks | sets the tricks queue
        // @uparmopt name | trick array.  if no array specified
        // a default air tricks array will be used
        // @parmopt name | Special | | special tricks array
		case 0xd8c79bb0: // SetQueueTricks
		{
			mNumQueueTricksArrays=0;

			Script::CComponent *pComp=NULL;
			while (true)
			{
				pComp=pParams->GetNextComponent(pComp);
				if (!pComp)
				{
					break;
				}
			
				if (pComp->mNameChecksum==0 && pComp->mType==ESYMBOLTYPE_NAME)
				{
					// Found a name, so add it to the array.
					Dbg_MsgAssert(mNumQueueTricksArrays<MAX_QUEUE_TRICK_ARRAYS,("\n%s\nToo many trick arrays",pScript->GetScriptInfo()));
					Dbg_MsgAssert(pComp->mChecksum,("Zero checksum ???"));
					mpQueueTricksArrays[mNumQueueTricksArrays++]=pComp->mChecksum;
				}	
			}
			
			if (mNumQueueTricksArrays==1)
			{
				// Check if the first array exists. If not, use DefaultAirTricks instead. This array is required to exist, otherwise
				// AddTricksToQueue will assert.
				
				Script::CArray *pArray=Script::GetArray(mpQueueTricksArrays[0]);
				if (!pArray)
				{
					mpQueueTricksArrays[0]=CRCD(0x9e22d268,"DefaultAirTricks");
				}	
			}	
			
			// Set any special tricks array required.
			mSpecialTricksArrayChecksum=0;
			pParams->GetChecksum(CRCD(0xb394c01c,"Special"),&mSpecialTricksArrayChecksum);
			break;
		}

        // @script | UseGrindEvents | 
		case 0xd4099d35: // UseGrindEvents
			mDoNotIgnoreMask=USED_BY_EXTRA_GRIND_TRICK;		
			break;
			
        // @script | AddTricksToQueue | This will check the button events and add any triggered tricks 
		// to the trick queue. The c-code actually does this every frame anyway, but this function
		// allows it to be done immediately when necessary.
		case 0xde98de7a: // AddTricksToQueue
			AddTricksToQueue();
			break;
			
        // @script | ClearTrickQueue | clears the trick queue
		case 0x54c33b20: // ClearTrickQueue
			ClearTrickQueue();		
			break;
			
        // @script | DoNextManualTrick | does next trick while in a manual
		case 0x788f9a4e: //	DoNextManualTrick
			TriggerAnyManualTrick(pParams);
			break;
			
        // @script | SetManualTricks | 
        // @uparm name | trick array
		case 0x1fcf080a: // SetManualTricks
		{
			mNumManualTrickArrays=0;

			Script::CComponent *pComp=NULL;
			while (true)
			{
				pComp=pParams->GetNextComponent(pComp);
				if (!pComp)
				{
					break;
				}
			
				if (pComp->mNameChecksum==0 && pComp->mType==ESYMBOLTYPE_NAME)
				{
					// Found a name, so add it to the array.
					Dbg_MsgAssert(mNumManualTrickArrays<MAX_MANUAL_TRICK_ARRAYS,("\n%s\nToo many manual trick arrays",pScript->GetScriptInfo()));
					Dbg_MsgAssert(pComp->mChecksum,("Zero checksum ???"));
					mpManualTrickArrays[mNumManualTrickArrays++]=pComp->mChecksum;
				}	
			}
			
			// Set any special manual tricks array required.
			mSpecialManualTricksArrayChecksum=0;
			pParams->GetChecksum(CRCD(0xb394c01c,"Special"),&mSpecialManualTricksArrayChecksum);
			break;
		}
				
        // @script | ClearManualTrick | 
		case 0x8e3d3bec: // ClearManualTrick
			ClearManualTrick();
			break;
			
		/////////////////////////////////////////////////////////////////////////////////////////////
		//
		// Extra-Grind trick stuff, which has a very similar interface to the manual stuff above,
		// except that there is no script command corresponding to DoNextManualTrick because the
		// C-code will automatically run any queued extra grind trick when it snaps the player
		// to a rail in CSkater::MaybeStickToRail
		//
		/////////////////////////////////////////////////////////////////////////////////////////////

        // @script | SetExtraGrindTricks | 
        // @uparm name | trick array
		case 0x6ba35ab4: // SetExtraGrindTricks
		{
			mNumExtraGrindTrickArrays=0;

			Script::CComponent *pComp=NULL;
			while (true)
			{
				pComp=pParams->GetNextComponent(pComp);
				if (!pComp)
				{
					break;
				}
			
				if (pComp->mNameChecksum==0 && pComp->mType==ESYMBOLTYPE_NAME)
				{
					// Found a name, so add it to the array.
					Dbg_MsgAssert(mNumExtraGrindTrickArrays<MAX_EXTRA_GRIND_TRICK_ARRAYS,("\n%s\nToo many special grind trick arrays",pScript->GetScriptInfo()));
					Dbg_MsgAssert(pComp->mChecksum,("Zero checksum ???"));
					mpExtraGrindTrickArrays[mNumExtraGrindTrickArrays++]=pComp->mChecksum;
				}	
			}
			
			mSpecialExtraGrindTrickArrayChecksum=0;
			pParams->GetChecksum(CRCD(0xb394c01c,"Special"),&mSpecialExtraGrindTrickArrayChecksum);
			break;
		}
				
        // @script | ClearExtraGrindTrick | 
		case 0xab6b8dd9: // ClearExtraGrindTrick
			ClearExtraGrindTrick();
			break;

        // @script | ClearEventBuffer |
        // @parmopt array | Buttons | | buttons array
        // @parmopt int | OlderThan | 0 | used in conjuction with optional buttons array
		case 0x86928082: // ClearEventBuffer	
		{
			Script::CArray *pButtonArray=NULL;
			if (pParams->GetArray(CRCD(0xbca37a49,"Buttons"),&pButtonArray))
			{
				int OlderThan=0;
				pParams->GetInteger(CRCD(0xcdcfee8a,"OlderThan"),&OlderThan);
				int Size=pButtonArray->GetSize();
				for (int i=0; i<Size; ++i)
				{
					RemoveOldButtonEvents(pButtonArray->GetNameChecksum(i),OlderThan);
				}
			}
			else
			{
				ClearEventBuffer();
			}	
			break;
		}

        // @script | RemoveXEvents | Run through all the recorded button
        // events, and remove any involving button X.
		// This is used as an easy way of fixing the boneless bug.
	    // This function is called at the start of the land script, 
        // which prevents any previously
		// queued bonelesses from being detected.
		case 0x70e934d: // RemoveXEvents
		{
			for (int i=0; i<mNumEvents; ++i)
			{
				if (mpButtonEvents[i].ButtonNameChecksum==0x7323e97c/* X */)
				{
					// Flag the event as having been used, so that it won't be used again.
					mpButtonEvents[i].Used=true;
				}	
			}
			break;
		}

		// @script | RestoreEvents | Restores the events used by a particular trick type.
        // @parmopt name | UsedBy | | The trick type, one of Regular, Extra, Manual or ExtraGrind
		// @parmopt int | Duration | 200 | How far back in time to restore events, in milliseconds
		case 0x54d43789: // RestoreEvents
		{
			int duration=200;
			pParams->GetInteger(CRCD(0x79a07f3f,"Duration"),&duration);
			
			uint32 used_by=0;
			pParams->GetChecksum(CRCD(0x9694fc41,"UsedBy"),&used_by);
			
			uint32 mask=0;
			switch (used_by)
			{
				case 0xb58efc2b: // Regular
					mask=USED_BY_REGULAR_TRICK;
					break;
				case 0xb2c0f29a: // Extra
					mask=USED_BY_EXTRA_TRICK;
					break;
				case 0xef24413b: // Manual
					mask=USED_BY_MANUAL_TRICK;
					break;
				case 0xec066b6a: // ExtraGrind
					mask=USED_BY_EXTRA_GRIND_TRICK;
					break;
				default:
					break;
			}
				
			int index=mLastEvent;
			for (int i=0; i<mNumEvents; ++i)
			{
				if (Tmr::ElapsedTime(mpButtonEvents[index].Time) >= (uint32)duration)
				{
					break;
				}
		
				if (mpButtonEvents[index].Used & mask)
				{
					mpButtonEvents[index].Used&=~mask;
				}	
				
				--index;
				if (index<0)
				{
					index+=MAX_EVENTS;
				}	
			}
			
			break;
		}

        // @script | SetExtraTricks | 
        // @parmopt float | Duration | | 
        // @parmopt name | Tricks | | trick array
        // @parmopt name | Special | | special tricks array
        // @parmopt local string | Ignore | | trick to ignore
        // @parmopt array | Ignore | | array of tricks to ignore
		case 0x7627dc71: // SetExtraTricks
			SetExtraTricks(pParams,pScript);
			break;
			
        // @script | KillExtraTricks | 
		case 0xd5813251: // KillExtraTricks
			KillExtraTricks();
			break;

        // @script | SetSlotTrick | 
        // @parm name | Slot | slot name
        // @parm name | Trick | 
		case 0xb42a1230: // SetSlotTrick
		{
			uint32 Slot=0;
			pParams->GetChecksum(CRCD(0x53f1df98,"Slot"),&Slot);
			Dbg_MsgAssert(Slot,("\n%s\nSetSlotTrick requires a slot name",pScript->GetScriptInfo()));
			uint32 Trick=0;
			pParams->GetChecksum(CRCD(0x270f56e1,"Trick"),&Trick);
			
			Dbg_MsgAssert(mpTrickMappings,("NULL mpTrickMappings"));
			// This will just replace the slot if it exists already.
			mpTrickMappings->AddComponent(Slot,(uint8)ESYMBOLTYPE_NAME,Trick);
			break;
		}

        // @script | ChangeProTricks | 
		case 0xabff7889: // ChangeProTricks 
		{
			// Have to use GetNextComponent rather than GetChecksum, because GetChecksum won't work if
			// the checksum is unnamed, because by convention an unnamed checksum that resolves to a structure
			// effectively makes that structure part of the params, & the name of the structure is just considered
			// an intermediate thing which is not part of the params.
			Script::CComponent *pComp=pParams->GetNextComponent(NULL);
			if (pComp && pComp->mNameChecksum==0 && pComp->mType==ESYMBOLTYPE_NAME)
			{
				Script::CStruct *pNewTrickMappings=Script::GetStructure(pComp->mChecksum);
				if (pNewTrickMappings)
				{
					// Found the new structure, so load it in to mpTrickMappings.
					Dbg_MsgAssert(mpTrickMappings,("NULL mpTrickMappings"));
					mpTrickMappings->Clear();
					mpTrickMappings->AppendStructure(pNewTrickMappings);
				}
				else
				{
					#ifdef __NOPT_ASSERT__
					printf("\n%s\nWarning: Protrick mapping '%s' not found\n",pScript->GetScriptInfo(),Script::FindChecksumName(pComp->mChecksum));
					#endif
				}
			}
			else
			{
				#ifdef __NOPT_ASSERT__
				printf("\n%s\nWarning: ChangeProTricks command requires a name.\n",pScript->GetScriptInfo());
				#endif
			}			
			break;
		}
					
        // @script | BashOn | turns bash on (for speeding up bails?)
		case 0xd872002d: // BashOn
			mBashingEnabled=true;
			// Clear the event buffer so that button presses done before the bail will not
			// speed it up.
			ClearEventBuffer();
			break;
			
        // @script | BashOff | turns bash off
		case 0x290f6010: // BashOff
			mBashingEnabled=false;
			break;
		
        // @script | SetTrickName | 
        // @uparmopt 'local string' | trick name
        // @uparmopt "string" | trick name
		case 0x4b2ee2bc: // SetTrickName
		{
			const char *pName=NULL;
			pParams->GetString(NO_NAME,&pName);
			// Make it accept regular strings too, because trick names probably won't get translated.
			if (pName)
			{
				Dbg_MsgAssert(strlen(pName)<MAX_TRICK_NAME_CHARS,("\n%s\nToo many chars in trick name, max=%d",pScript->GetScriptInfo(),MAX_TRICK_NAME_CHARS));
				strcpy(mpTrickName, pName);
			}
			else
			{
				mpTrickName[0]=0;
			}
			break;
		}

        // @script | SetTrickScore | sets trick score flag
        // @uparm 1 | trick score
		case 0xcb3a8fd2: // SetTrickScore
			pParams->GetInteger(NO_NAME,&mTrickScore);	  
			break;
                          
        // @script | GetSpin | Gets the current spin value, and adds it as an integer parameter
		// called Spin. It will always be positive and a multiple of 180. It takes into account
		// the spin slop.
		case 0x4ba7719f: // GetSpin
		{
			int spin = static_cast< int >(Mth::Abs(mTallyAngles) + GetPhysicsFloat(CRCD(0x50c5cc2f, "spin_count_slop")));
			pScript->GetParams()->AddFloat(CRCD(0xedf5db70, "Spin"), (spin / 180) * 180.0f);
			break;
		}
		
        // @script | ResetSpin | 
		case 0xe50d471e: // ResetSpin
			mTallyAngles = 0.0f;
			break;
		
        // @script | Display | display trick
        // @flag Deferred | If deferred (currently only used for the ollie)
        // then store it away, because we may not want to add it to the
        // combo after all. Eg, a 180 ollie on its own is counted,
		// but if you do a 180 kickflip, you'll get the kickflip but not 
        // the ollie as well. 
        // @flag BlockSpin |
		// @parmopt int | AddSpin | 0 | If this is specified and the current trick is the
		// same as the last trick, then the spin will be added to the last trick rather than
		// adding it as a new trick.
		case 0xf32e8d5c: // Display				  
		{
			if (mpTrickName)
			{
				Mdl::Score *pScore=GetScoreObject();
				int spin=0;
				
				if (pParams->ContainsFlag(CRCD(0x5c9b1765,"Deferred")))
				{
					strcpy(mpDeferredTrickName,mpTrickName);
					mDeferredTrickScore=mTrickScore;
					if (mp_skater_core_physics_component->IsSwitched())
					{
						mDeferredTrickFlags|=Mdl::Score::vSWITCH;
					}
				}
				// If an AddSpin value is specified and the current trick is the same as
				// the last one in the score, then instead of adding the trick again just
				// give the last one some more spin. Used by the truckstand2 spin.
				// (See the ManualLink script in groundtricks.q)
				else if (pParams->GetInteger(CRCD(0x83cb0082,"AddSpin"),&spin) && 
						 pScore->GetLastTrickId()==Script::GenerateCRC(mpTrickName))
				{
					mTallyAngles+=spin;
					if (GetSkater()->IsLocalClient())
					{
						pScore->UpdateSpin(mTallyAngles);
					}	
				}
				else
				{
					Mdl::Score::Flags Flags=0;
					if (pParams->ContainsFlag(CRCD(0xc2150bb0,"BlockSpin")))
					{
						Flags|=Mdl::Score::vBLOCKING;
					}
					if (pParams->ContainsFlag(CRCD(0x3c24ac46,"NoDegrade")))
					{
						Flags|=Mdl::Score::vNODEGRADE;
					}	
					if (mp_skater_core_physics_component->IsSwitched())
					{
						Flags|=Mdl::Score::vSWITCH;
					}
					if (mUseSpecialTrickText)
					{
						Flags|=Mdl::Score::vSPECIAL;
					}
					if ( pParams->ContainsFlag( CRCD(0x61a1bc57,"cat") ) )
					{
						Flags |= Mdl::Score::vCAT;
					}
					
					// Need to also include nollie check later.
						
					if( GetSkater()->IsLocalClient())
					{
						// if on a rail, then might need to degrade the rail score
						if (mp_skater_core_physics_component->GetState()==RAIL && mTrickScore)
						{
							printf ("RAIL score %d * %2.3f = %d\n", mTrickScore,pScore->GetRobotRailMult(),(int) (pScore->GetRobotRailMult()*mTrickScore));
							mTrickScore = (int) (mTrickScore * pScore->GetRobotRailMult());
						}
					
						// add a trick to the series
						pScore->Trigger(mpTrickName, mTrickScore, Flags);
						
						if (mpTrickName[0])
						{
							// tell any observers to do the chicken dance:
							GetObject()->BroadcastEvent(CRCD(0x11d8bc9e, "SkaterTrickDisplayed"));
							
							if (mp_skater_core_physics_component->GetFlag(VERT_AIR) || mp_skater_core_physics_component->GetTrueLandedFromVert())
							{
								// If vert, only count it if the spin is at least 360
								if (Mth::Abs(mTallyAngles)>=360.0f-GetPhysicsFloat(CRCD(0x50c5cc2f,"spin_count_slop")) + 0.1f)
								{
									pScore->SetSpin(mTallyAngles);
								}
							}
							else
							{
								pScore->SetSpin(mTallyAngles);
							}
						}
						else
						{
							// K: If the trick was a 'Null' trick, then do not add the spin.
							// This is to fix TT6057
							// A null trick is a trick whose name is set using SetTrickName ""
							// and is often used to do a BlockSpin without displaying a trick name.
						}
					}
					
					if (pParams->ContainsFlag(CRCD(0xc2150bb0,"BlockSpin")))
					{
						mTallyAngles=0.0f;
					}
						
					// Clear the deferred trick, so that you won't get a 180 kickflip and a 180 ollie.
					//dodgy_test(); printf("Clearing deferred trick\n");
					mpDeferredTrickName[0]=0;
				}	
            }
			
			// Only clear the mUseSpecialTrickText flag if it is not a 'null' trick, which is
			// a trick whose name is just '' used to insert spin blocks.
			// Want to preserve the mUseSpecialTrickText for the next proper trick.
			if (mpTrickName[0])
			{
				mUseSpecialTrickText=false;
			}	
			break;
		}	
		
        // @script | ClearPanel_Landed | end of trick combo...scoring and such
		case 0x11ca5c42: // ClearPanel_Landed
		{
			Mdl::Score *pScore=GetScoreObject();

			if(GetSkater()->IsLocalClient())
			{
				// If there is a deferred trick stored up, then decide whether to add it
				// to the combo depending on how much spin there is.
				if (mpDeferredTrickName[0])
				{
					//dodgy_test(); printf("Got a deferred trick, mTallyAngles=%f\n",mTallyAngles);
					// Note: Using the new 'mp_skater_core_physics_component->m_true_landed_from_vert' rather than 'mp_skater_core_physics_component->mLandedFromVert', because
					// mp_skater_core_physics_component->mLandedFromVert can be cleared by a script command, and if landing from vert it
					// will be cleared at this point, due to some script logic for detecting reverts.
					if (mp_skater_core_physics_component->GetTrueLandedFromVert())
					{
						// If vert, only count it if the spin is at least 360
						if (Mth::Abs(mTallyAngles)>= 360.0f - GetPhysicsFloat(CRCD(0x50c5cc2f,"spin_count_slop")) + 0.1f)	// (Mick) adkisted for slop
						{
							// add the deferred trick to the series
							pScore->Trigger(mpDeferredTrickName, mDeferredTrickScore, mDeferredTrickFlags);
							pScore->SetSpin(mTallyAngles);
						}	
					}
					else
					{
						// Only count it if the spin is at least 180, cos just ollieing is easy.
						if (Mth::Abs(mTallyAngles)>=180.0f - GetPhysicsFloat(CRCD(0x50c5cc2f,"spin_count_slop")) + 0.1f)  // (Mick) adkisted for slop
						{
							// add the deferred trick to the series
							pScore->Trigger(mpDeferredTrickName, mDeferredTrickScore, mDeferredTrickFlags);
							pScore->SetSpin(mTallyAngles);
						}	
					}
						
					// Clear the deferred trick.
					mpDeferredTrickName[0]=0;
				}
			
                // check for special trick if we're in a special trick goal
                Game::CGoalManager* pGoalManager = Game::GetGoalManager();
                Dbg_MsgAssert( pGoalManager, ( "Unable to get goal manager\n" ) );
                pGoalManager->CheckTrickText();
				
				if (pScore->GetScorePotValue() != 0)
				{
					Script::CStruct* p_params = new Script::CStruct;
					p_params->AddChecksum(CRCD(0x5b24faaa, "SkaterId"), GetObject()->GetID());
					GetObject()->BroadcastEvent(CRCD(0x4b3ce1fe, "SkaterExitCombo"), p_params);
					delete p_params;
				}

				///////////////////////////////////////////////////////////////////////
				// K: Warning! I moved AwardPendingGaps() to before Land(), so that AwardPendingGaps()
				// can query the score to see if the required trick was got in the case of the
				// gap being part of a created goal.
				// Possible alternative solution in case this causes problems: Modify the c-code of
				// StartGap so that it detects if the gap is part of a created goal, and if so sets the
				// tricktext according to the trick required by the goal. 
				GetSkaterGapComponentFromObject(GetObject())->AwardPendingGaps();
                
				pScore->Land();
				mTallyAngles=0.0f;		// Mick: Cleared, so a manual will not get it
				///////////////////////////////////////////////////////////////////////
				
				
                mp_stats_manager_component->Land();
			}
			
			// tell any observers to cheer if they want:
			GetObject()->BroadcastEvent( CRCD(0xc98ba111,"skaterLanded"));

			// allows us to end the run if we're in horse mode
			if ( m_first_trick_started )
				m_first_trick_completed = true;

			mNumTricksInCombo=0;
			
			mp_skater_balance_trick_component->UpdateRecord();
			mp_skater_balance_trick_component->Reset();
			
			// Clear the special friction index (used by Reverts)
			mp_skater_core_physics_component->ResetSpecialFrictionIndex();

			break;
		}
		  		
        // @script | ClearPanel_Bailed | trick combo ended in bail...
		case 0xb8045433: // ClearPanel_Bailed
		{
			if( GetSkater()->IsLocalClient())
			{
				if (GetScoreObject()->GetScorePotValue() != 0)
				{
					Script::CStruct* p_params = new Script::CStruct;
					p_params->AddChecksum(CRCD(0x5b24faaa, "SkaterId"), GetObject()->GetID());
					GetObject()->BroadcastEvent(CRCD(0x4b3ce1fe, "SkaterExitCombo"), p_params);
					delete p_params;
				}
				
				GetScoreObject()->Bail();
			}

			// tell any observers to balk if they want:
			GetObject()->BroadcastEvent( CRCD(0x6045a960,"skaterBailed"));
		   
 
			m_first_trick_started = true;
			m_first_trick_completed = true;

			// clear out the graffiti tricks
			SetGraffitiTrickStarted( false );
			
			mNumTricksInCombo=0;
			
			mp_skater_balance_trick_component->Reset();
			
            mp_skater_core_physics_component->StopSkitch();
			
			// Mick: Cleared, so a manual will not get it
			mTallyAngles=0.0f;
			
			// Clear the special friction index (used by Reverts)
			mp_skater_core_physics_component->ResetSpecialFrictionIndex();
			m_pending_tricks.FlushTricks();
			
			GetSkaterGapComponentFromObject(GetObject())->ClearPendingGaps();

            mp_stats_manager_component->Bail();

			break;
		}		

        // @script | TrickOffObject | 
        // @uparm name | object name
		case 0x36b2be57: // TrickOffObject
		{
			uint32 clusterName;
			pParams->GetChecksum( NO_NAME, &clusterName, Script::ASSERT );
			TrickOffObject( clusterName );
			break;
		}

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	// the "default" case of the switch statement handles
	// unrecognized functions;  if we make it down here,
	// that means that the component already handled it
	// somehow
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CTrickComponent::GetDebugInfo"));
	// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums

	p_info->AddStructure("mpTrickMappings",mpTrickMappings);
 
	Script::CArray *p_events_array=new Script::CArray;
	if (mNumEvents)
	{
		p_events_array->SetSizeAndType(mNumEvents,ESYMBOLTYPE_STRUCTURE);
		
		int index=mLastEvent;
		for (int i=0; i<mNumEvents; ++i)
		{
			Script::CStruct *p_event=new Script::CStruct;
			
			p_event->AddChecksum(CRCD(0xc5f953c2,"Button"),mpButtonEvents[index].ButtonNameChecksum);
			
			uint32 event_type=0;
			switch (mpButtonEvents[index].EventType)
			{
				case EVENT_NONE:  event_type=CRCD(0x806fff30,"None"); break;
				case EVENT_BUTTON_PRESSED:  event_type=CRCD(0xe4ab4785,"Pressed"); break;
				case EVENT_BUTTON_RELEASED:  event_type=CRCD(0x4ba9ee9,"Released"); break;
				default: break;
			}
			p_event->AddChecksum(NONAME,event_type);

			p_event->AddInteger(CRCD(0x906b67ba,"Time"),mpButtonEvents[index].Time);
			p_event->AddInteger(CRCD(0x86b89ce7,"Used"),mpButtonEvents[index].Used);
				
			p_events_array->SetStructure(i,p_event);
				
			--index;
			if (index<0)
			{
				index+=MAX_EVENTS;
			}
		}		
	}
    p_info->AddArrayPointer(CRCD(0xac78a8b5,"Events"),p_events_array);

	s_add_array_of_names(p_info,mNumQueueTricksArrays,mpQueueTricksArrays,"QueueTrickArrays");
	
	p_info->AddInteger(CRCD(0x50cab1f1,"GotManualTrick"),mGotManualTrick);
	s_add_array_of_names(p_info,mNumManualTrickArrays,mpManualTrickArrays,"ManualTrickArrays");
	p_info->AddChecksum(CRCD(0x9c93be77,"SpecialManualTricksArray"),mSpecialManualTricksArrayChecksum);
	
	p_info->AddInteger(CRCD(0xdf2c1464,"GotExtraGrindTrick"),mGotExtraGrindTrick);
	s_add_array_of_names(p_info,mNumExtraGrindTrickArrays,mpExtraGrindTrickArrays,"ExtraGrindTrickArrays");
	p_info->AddChecksum(CRCD(0x4cb1bfe,"SpecialExtraGrindTrickArray"),mSpecialExtraGrindTrickArrayChecksum);

	p_info->AddInteger(CRCD(0x33660920,"GotExtraTricks"),mGotExtraTricks);
	p_info->AddInteger(CRCD(0x15d82e35,"ExtraTricksInfiniteDuration"),mExtraTricksInfiniteDuration);
	p_info->AddInteger(CRCD(0x4c85f690,"ExtraTricksDuration"),mExtraTricksDuration);
	p_info->AddInteger(CRCD(0x9737fa16,"ExtraTricksStartTime"),mExtraTricksStartTime);
	s_add_array_of_names(p_info,mNumExtraTrickArrays,mpExtraTrickArrays,"ExtraTrickArrays");
	p_info->AddChecksum(CRCD(0x42569716,"SpecialExtraTricksArray"),mSpecialExtraTricksArrayChecksum);
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickComponent::Debounce ( int button, float time )
{
	mButtonState[button] = false;
	mButtonDebounceTime[button] = time;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Increment the trick count, which may set pedestrian exceptions to
// make them go "oooOOOOOooooh"

// Note: This count won't be totally accurate, because if an 'extra' trick is triggered it
// will increment the count too, making the count get incremented more times than it should.
// But this shouldn't be a big problem, cos it'll just mean the croud will more likely to cheer if
// you do lots of extra tricks.
// NOTE: move to trick component
void CTrickComponent::IncrementNumTricksInCombo()
{
	++mNumTricksInCombo;
	if ( mNumTricksInCombo > 3 )
	{
		// tell any observers to start taking notice:
		GetObject()->BroadcastEvent(CRCD(0x94182a08,"skaterStartingRun"));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickComponent::SetGraffitiTrickStarted( bool started )
{
	m_graffiti_trick_started = started;
	if ( !started )
	{
		m_pending_tricks.FlushTricks();
	}
	else
	{
		TrickOffObject( mp_skater_core_physics_component->GetLastNodeChecksum() );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickComponent::TrickOffObject( uint32 node_name )
{
	if ( GraffitiTrickStarted() )
	{
		m_pending_tricks.TrickOffObject( node_name );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickComponent::ClearMiscellaneous()
{
	mNumTricksInCombo = 0;
	m_first_trick_completed = false;
	m_first_trick_started = false;
	SetGraffitiTrickStarted( false );
	m_pending_tricks.FlushTricks();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CTrickComponent::WritePendingTricks( uint32* p_buffer, uint32 max_size )
{
	uint32 size = m_pending_tricks.WriteToBuffer( p_buffer, max_size );

	// resets the graffiti trick list
	SetGraffitiTrickStarted( false );
	
	return size;
}


// Experimenting with a new way of binding script commands
// Doing it like this is twice as fast (11us vs 22 us) which can add up for
// things that are called every frame (like the skater physics scripts) 
// It's a bit cumbersome to do it like this though
// Perhaps there could be a more general database of script commands
// with registration info about which component it goes to.
// and a pointer to the memeber function
 
// @script | DoNextTrick | runs the next trick in the queue
// @parmopt name | ScriptToRunFirst | | Script that will be run first before any trick script.
// @parmopt structure | Params | | Parameters to pass to ScriptToRunFirst
bool ScriptDoNextTrick( Script::CStruct *pParams, Script::CScript *pScript )
{
	CCompositeObject *p_object = (CCompositeObject *) (pScript->mpObject.Convert());
	Dbg_MsgAssert(p_object, ("Can't run DoNextTrick with no object"));
	CTrickComponent* p_trick = GetTrickComponentFromObject(p_object);
	Dbg_MsgAssert(p_trick, ("Can't run DoNextTrick with no object"));
		
	uint32 scriptToRunFirst=0;
	Script::CStruct *pScriptToRunFirstParams=NULL;
	pParams->GetChecksum(CRCD(0x148fee96,"ScriptToRunFirst"),&scriptToRunFirst);
	pParams->GetStructure(CRCD(0x7031f10c,"Params"),&pScriptToRunFirstParams);
	
	Script::CStruct *pExtraTrickParams=NULL;
	pParams->GetStructure(CRCD(0x31261d2f, "TrickParams"),&pExtraTrickParams);
	if (pExtraTrickParams)
	{
		Script::CStruct *pCopyOfExtraTrickParams=pCopyOfExtraTrickParams = new Script::CStruct(*pExtraTrickParams);
		p_trick->TriggerNextQueuedTrick(scriptToRunFirst, pScriptToRunFirstParams, pCopyOfExtraTrickParams);
		delete pCopyOfExtraTrickParams;
	}
	else
	{
		p_trick->TriggerNextQueuedTrick(scriptToRunFirst, pScriptToRunFirstParams);
	}
	return true;
} 
 
}
