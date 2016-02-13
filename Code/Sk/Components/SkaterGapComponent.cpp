//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterGapComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/5/3
//****************************************************************************

#include <sk/components/skatergapcomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skaterbalancetrickcomponent.h>
#include <sk/components/skaterphysicscontrolcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/components/trickcomponent.h>
#include <gel/components/walkcomponent.h>
#include <gel/components/vehiclecomponent.h>
#include <gel/objtrack.h>							// for event broadcasting on sucess of a gap

#include <sk/objects/gap.h>
#include <sk/objects/skatercareer.h>
#include <sk/scripting/nodearray.h>
#include <sk/modules/skate/score.h>
#include <sk/modules/skate/skate.h>

#include <sk/parkeditor2/parked.h>

// #define DEBUG_GAPS

namespace Obj
{
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterGapComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterGapComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterGapComponent::CSkaterGapComponent() : CBaseComponent()
{
	SetType( CRC_SKATERGAP );
	
	mp_core_physics_component = NULL;
	mp_balance_trick_component = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterGapComponent::~CSkaterGapComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterGapComponent::InitFromStructure( Script::CStruct* pParams )
{
	Dbg_MsgAssert(GetObject()->GetType() == SKATE_TYPE_SKATER, ("CSkaterGapComponent added to non-skater composite object"));
	
	m_frame_count = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterGapComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterGapComponent::Finalize (   )
{
	mp_core_physics_component = GetSkaterCorePhysicsComponentFromObject(GetObject());
	mp_balance_trick_component = GetSkaterBalanceTrickComponentFromObject(GetObject());
	mp_physics_control_component = GetSkaterPhysicsControlComponentFromObject(GetObject());
	mp_walk_component = GetWalkComponentFromObject(GetObject());
	
	Dbg_Assert(mp_core_physics_component);
	Dbg_Assert(mp_balance_trick_component);
	Dbg_Assert(mp_physics_control_component);
	Dbg_Assert(mp_walk_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterGapComponent::Update()
{
	m_frame_count++;
	
	// setup flags based on skater state
	int cancel, require;
	get_state_flags(cancel, require);
	
	UpdateCancelRequire(cancel, require);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Split out the actual updating from Update(), so we co do specific updates
// of the require/cancel flags at arbitart times
// specifically we want to update the "Lip" requirements as soon as we land on them
// and do it in isolation, so nothing else is messed up

void CSkaterGapComponent::UpdateCancelRequire(int cancel, int require)
{
	CGap* pGap = static_cast< CGap* >(m_gap_list.FirstItem());
	
	// don't bother determining the state flags if there are no active gaps
	if (!pGap) return;
	
	// update the active gaps based on the flags
	while (pGap)
	{
		CGap* pNext = static_cast< CGap* >(pGap->GetNext());
		
		// Clear any require flags that match the require mask, so if we do all that is required, then the require part of the flags field will be cleared
		pGap->m_flags &= ~require;
	
		// if any cancel flags match the cancel mask, cancel the gap
		if (pGap->m_flags & cancel)
		{
//			printf ("Cancelled by %x\n",pGap->m_flags & cancel); 
			if (pGap->m_trickscript)
			{
				// delete the gap trick
				CGapTrick *pGapTrick = static_cast< CGapTrick* >(m_gaptrick_list.FirstItem());
				while (pGapTrick)
				{
					CGapTrick * pNext = static_cast< CGapTrick* >(pGapTrick->GetNext());
	
					// only delete gaptricks if you've not been awarded them, and they exactly match the gap we're cancelling
					if (!pGapTrick->m_got
						&& pGapTrick->m_id == pGap->m_id
						&& pGapTrick->m_script == pGap->m_trickscript
						&& pGapTrick->m_node == pGap->m_node)
					{
						delete pGapTrick;
						break;
					}
	
					pGapTrick = pNext;
				}
			}
			
			#ifdef DEBUG_GAPS
			MESSAGE("DELETING GAP");
			DUMPC(pGap->m_id);
			#endif
	
			// delete the gap
			delete pGap;
		}
		pGap = pNext;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterGapComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | StartGap | Start a gap...used in conjunction with EndGap. <nl>
        // See online doc for detailed description and examples
        // @parmopt name | GapID | Links | the name of the gap
        // @parmopt array | flags | | array of flags to use
		// @parmopt name | trickscript | | Script that is run when you land a combo that includes this gap
		// subject to the two optional conditions below
		// @parmopt checksum | KeyCombo | | Name of a key combo, like Air_SquareD, you have to do trick with this button combo
		// @parmopt string | TrickText | | This text substring, like 'kickflip' has to be in the combo
		// @nextparm | flags | flags | file | gapflags.lst | done
		// @nextparm | trickscript | script
		// @nextparm | KeyCombo | list | file | keycombo.lst | done
		// @parmopt checksum | combined_flags | | An optional way of specifying the cancel flags as a uint32 consisting
		// of the flag bit values or'd together. Currently only used when the Park Editor calls StartGap from within
		// the c-code.
		case CRCC(0x65b9ec39, "StartGap"):
			start_gap(pParams, pScript);
			break;
		
        // @script | StartGapTrick | This adds a gap trick to the skaters
        // current list
        // @parm string | TrickText | text to look for in the trick text 
        // @parm name | gapscript | script to run when you land this gap trick
		// @parmopt checksum | KeyCombo | | Name of a key combo, like Air_SquareD, you have to do trick with this button combo
		case CRCC(0xfc4f2009, "StartGapTrick"):
			start_gap_trick(pParams, pScript);
			break;
			
        // @script | EndGap | end a gap - used with StartGap <nl>
        // See online doc for detailed description and examples
        // @parm name | GapID | gap gap_id
        // @parmopt string | text | "Unnamed Gap" | text to display
        // @parmopt int | score | | score to award for this gap
        // @flag NetEnabled | 
        // @flag Permanent | 
        // @parmopt structure | continue | | for linking gaps <nl>
        // e.g.  continue = { GapID = SecondHalf Cancel = CANCEL_GROUND }
		// @nextparm | score | slidernum | 0 | 999999
		case CRCC(0xe5399fb2, "EndGap"):
			end_gap(pParams, pScript);
			break;
		
		// @script | CheckGapTricks | Check to see if there are any gap tricks,
		// then check to see if we did the trick yet
		// if so, then fire them off
		// note, this does not delete gaps that you did not do
		// so you would ahve to call ClearGapTricks
		// in that case
        case CRCC(0x42f80e5e, "CheckGapTricks"):
			check_gap_tricks(pParams);
			break;
		
        // @script | ClearGapTricks | clears all the gap tricks.
        // Currently, I call CheckGapTricks then ClearGapTricks
        // at any point in the skater's logic where he can land
        // a trick combo. So if you just stick a "StartGapTrick"
        // in your gap script, then it will automatically work
		case CRCC(0x196772b, "ClearGapTricks"):
			clear_gap_tricks(pParams);
			break;
		
		// @script | DuplicateTrigger | quick way of doing two way gaps <nl>
        // see online doc for detailed description and examples
		case CRCC(0x4562e4a6, "DuplicateTrigger"):
		{
			// used in a trigger script
			// find the first node linked to this one that has
			// a trigger script, and execute that instead
			{
				int node = pScript->mNode;
				Dbg_MsgAssert(node != -1,("DuplicateTrigger in script with no node,\n%s\n",pScript->GetScriptInfo()));
				if (node != -1)	 		// Failsafe
				{
					int links = SkateScript::GetNumLinks(node);
					//dodgy_test(); printf("node %d has %d links\n",node,links);
					
					Dbg_MsgAssert(links>0,("DuplicateTrigger in node (%d) with no links,\n%s\n",node,pScript->GetScriptInfo()));
					int i;
					for (i=0;i<links;i++)
					{
						int linked_node = SkateScript::GetLink(node,i); 
						Script::CStruct *pDest = SkateScript::GetNode(linked_node);
						uint32 script = 0;
						//dodgy_test(); printf("node %d linked to node %d\n",node,linked_node);
						if (pDest->GetChecksum(CRCD(0x2ca8a299, "TriggerScript"),&script))
						{
							// found another trigger script in the node we are linked to
							GetObject()->SpawnAndRunScript(script,pScript->mNode);
							break;		
						}						
					}
					Dbg_MsgAssert(i<links,("DuplicateTrigger in node (%d) not linked to trigger,\n%s\n",node,pScript->GetScriptInfo()));					
				}
			}
			break;
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

void CSkaterGapComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterGapComponent::GetDebugInfo"));
	
	static const uint32 p_gap_flag_checksums [   ]
	= {
		CRCD(0x9a27e74d, "CANCEL_GROUND"),				// 0x00000001
		CRCD(0xc6362354, "CANCEL_AIR"),              	// 0x00000002
		CRCD(0xcd4decee, "CANCEL_RAIL"),             	// 0x00000004
		CRCD(0x87e4e899, "CANCEL_WALL"),             	// 0x00000008
		CRCD(0x20e0d12b, "CANCEL_LIP"),              	// 0x00000010
		CRCD(0x78d5a029, "CANCEL_WALLPLANT"),			// 0x00000020
		CRCD(0x2d03dae1, "CANCEL_MANUAL"),				// 0x00000040
		CRCD(0x2a7a145a, "CANCEL_HANG"),     	    	// 0x00000080
		CRCD(0x0a65d800, "CANCEL_LADDER"),	         	// 0x00000100
		CRCD(0xa4721771, "CANCEL_SKATE"),				// 0x00000200
		CRCD(0x19807d3a, "CANCEL_WALK"),   				// 0x00000400
		CRCD(0x678677cc, "CANCEL_DRIVE"),				// 0x00000800
		CRCD(0xc6aa6589, "NO_SUCH_GAP_FLAG"),			// 0x00001000
		CRCD(0xc6aa6589, "NO_SUCH_GAP_FLAG"),			// 0x00002000
		CRCD(0xc6aa6589, "NO_SUCH_GAP_FLAG"),			// 0x00004000
		CRCD(0xc6aa6589, "NO_SUCH_GAP_FLAG"),			// 0x00008000
		CRCD(0xae0f7a14, "REQUIRE_GROUND"),				// 0x00010000
		CRCD(0xdfc901ab, "REQUIRE_AIR"),             	// 0x00020000
		CRCD(0xe056fc41, "REQUIRE_RAIL"),            	// 0x00040000
		CRCD(0xaafff836, "REQUIRE_WALL"),            	// 0x00080000
		CRCD(0x391ff3d4, "REQUIRE_LIP"),             	// 0x00100000
		CRCD(0xeae2e98f, "REQUIRE_WALLPLANT"),			// 0x00200000
		CRCD(0x192b47b8, "REQUIRE_MANUAL"),				// 0x00400000
		CRCD(0x076104f5, "REQUIRE_HANG"),	        	// 0x00800000
		CRCD(0x3e4d4559, "REQUIRE_LADDER"),	        	// 0x01000000
		CRCD(0xe236b218, "REQUIRE_SKATE"),				// 0x02000000
		CRCD(0x349b6d95, "REQUIRE_WALK"),				// 0x04000000
		CRCD(0x21c2d2a5, "REQUIRE_DRIVE"),				// 0x08000000
		CRCD(0xc6aa6589, "NO_SUCH_GAP_FLAG"),			// 0x10000000
		CRCD(0xc6aa6589, "NO_SUCH_GAP_FLAG"),			// 0x20000000
		CRCD(0xc6aa6589, "NO_SUCH_GAP_FLAG"),			// 0x40000000
		CRCD(0xc6aa6589, "NO_SUCH_GAP_FLAG")			// 0x80000000
	};
	
	int gap_count = 0;
	CGap* pGap = static_cast< CGap* >(m_gap_list.FirstItem());
	while (pGap)
	{
		gap_count++;
		pGap = static_cast< CGap* >(pGap->GetNext());
	}
	
	Script::CArray* p_gaps_array = new Script::CArray;
	p_gaps_array->SetSizeAndType(gap_count, ESYMBOLTYPE_STRUCTURE);
	
	int i = 0;
	pGap = static_cast< CGap* >(m_gap_list.FirstItem());
	while (pGap)
	{
		Script::CStruct* p_gap_struct = new Script::CStruct;
		p_gap_struct->AddChecksum(CRCD(0x40c698af, "Id"), pGap->m_id);
		
		Script::CStruct* p_gap_flags = new Script::CStruct;
		for (int n = 0; n < 32; n++)
		{
			uint32 mask = 1 << n;
			if (mask & pGap->m_flags)
			{
				p_gap_flags->AddChecksum(NO_NAME, p_gap_flag_checksums[n]);
			}
		}
		p_gap_struct->AddStructurePointer(CRCD(0xf4fabe45, "Flags"), p_gap_flags);
		
		p_gaps_array->SetStructure(i, p_gap_struct);
		i++;
		pGap = static_cast< CGap* >(pGap->GetNext());
	}
	p_info->AddArrayPointer(CRCD(0xd76c173e, "Gaps"), p_gaps_array);

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkaterGapComponent::ClearActiveGaps (   )
{
	m_gap_list.DestroyAllNodes();
	m_gaptrick_list.DestroyAllNodes();
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterGapComponent::ClearPendingGaps (   )
{
	Mdl::Skate::Instance()->GetGapChecklist()->ClearPendingGaps();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkaterGapComponent::AwardPendingGaps (   )
{
	Mdl::Skate::Instance()->GetGapChecklist()->AwardPendingGaps();
	
	Obj::CSkaterCareer* pCareer = Mdl::Skate::Instance()->GetCareer();
	if (!pCareer->GetGlobalFlag(405/*GOT_ALL_GAPS*/) && pCareer->GotAllGaps())
	{
		Script::SpawnScript(CRCD(0xcc74cc2e, "got_all_gaps_screen_create"));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkaterGapComponent::get_state_flags ( int& cancel, int& require )
{
	cancel = require = 0;
	
	if (!mp_physics_control_component->IsDriving())
	{
		if (mp_physics_control_component->IsSkating())
		{
			// skating
			
			cancel |= CANCEL_SKATE;
			require |= REQUIRE_SKATE;
			
			switch (mp_core_physics_component->GetState())
			{
				case AIR:
					cancel |= CANCEL_AIR;
					require |= REQUIRE_AIR;
					break;
					
				case GROUND:
					if (!mp_core_physics_component->GetFlag(OVERRIDE_CANCEL_GROUND)
						&& !mp_balance_trick_component->GetBalanceTrickType()
						&& !mp_balance_trick_component->DoingBalanceTrick())
						// && !mp_core_physics_component->HaveLandedThisFrame())
					{
						cancel |= CANCEL_GROUND;
						require |= REQUIRE_GROUND; 		
					}
					if (mp_balance_trick_component->GetBalanceTrickType() || mp_balance_trick_component->DoingBalanceTrick())
					{
						cancel |= CANCEL_MANUAL;
						require |= REQUIRE_MANUAL;
					}
					break;
		
				case RAIL:
					cancel |= CANCEL_RAIL;
					require |= REQUIRE_RAIL;
					break;
			
				case WALL:
					cancel |= CANCEL_WALL;
					require |= REQUIRE_WALL; 		
					break;
					
				case LIP:
					cancel |= CANCEL_LIP;
					require |= REQUIRE_LIP; 		
					break;
					
				case WALLPLANT:
					cancel |= CANCEL_WALLPLANT;
					require |= REQUIRE_WALLPLANT; 		
					break;
			}
		}
		else
		{
			// walking
			
			cancel |= CANCEL_WALK;
			require |= REQUIRE_WALK;
			
			switch (mp_walk_component->GetState())
			{
				case CWalkComponent::WALKING_AIR:
					cancel |= CANCEL_AIR;
					require |= REQUIRE_AIR;
					break;
					
				case CWalkComponent::WALKING_GROUND:
					cancel |= CANCEL_GROUND;
					require |= REQUIRE_GROUND;
					break;
					
				case CWalkComponent::WALKING_HANG:
					cancel |= CANCEL_HANG;
					require |= REQUIRE_HANG;
					break;
					
				case CWalkComponent::WALKING_LADDER:
					cancel |= CANCEL_LADDER;
					require |= REQUIRE_LADDER;
					break;
					
				default:
					break;
			}
		}
	}
	else
	{
		// driving
		
		cancel |= CANCEL_DRIVE;
		require |= REQUIRE_DRIVE;
		
		// driving gaps are a bit of a kludge right now
		
		CCompositeObject* p_vehicle = static_cast< CCompositeObject* >(CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0x824c6a24, "PlayerVehicle")));
		Dbg_Assert(p_vehicle);
		CVehicleComponent* p_vehicle_component = GetVehicleComponentFromObject(p_vehicle);
		Dbg_Assert(p_vehicle_component);
		
		if (p_vehicle_component->IsOnGround())
		{
			cancel |= CANCEL_GROUND;
			require |= REQUIRE_GROUND;
		}
		else
		{
			cancel |= CANCEL_AIR;
			require |= REQUIRE_AIR;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterGapComponent::start_gap ( Script::CStruct *pParams, Script::CScript* pScript )
{
	if (mp_core_physics_component->GetFlag(IS_BAILING)) return;
						  
	uint32 gap_id = 0;
	pParams->GetChecksum(CRCD(0x3b442e26, "GapID"), &gap_id);
	if (!gap_id)
	{
		// no gap_id is assumed to indicate a "Links" gap
		Dbg_MsgAssert(pScript->mNode != -1, ("\n%s\nLink gap with no node", pScript->GetScriptInfo()));
		Dbg_MsgAssert(SkateScript::GetNumLinks(pScript->mNode), ("\n%s\nLink gap with no links", pScript->GetScriptInfo()));
	}
	
	#ifdef DEBUG_GAPS
	MESSAGE("STARTING GAP");
	DUMPC(gap_id);
	#endif
	
	// check to see if there are any other gaps from this node with this ID and delete them
	// (note that this means you can only have one "Links" gap from a single node although there can be many targets)
	CGap* pGap = static_cast< CGap* >(m_gap_list.FirstItem());
	while (pGap)
	{
		CGap* pNext = static_cast< CGap* >(pGap->GetNext());

		if (pGap->m_id == gap_id && static_cast< int >(pGap->m_node) == pScript->mNode)
		{
			delete pGap;
		}
		pGap = pNext;
	}

	// create the gap and add the info to it
	pGap = new CGap;
	m_gap_list.AddToHead(pGap);
	pGap->m_id = gap_id;
	pGap->m_node = pScript->mNode;
	pGap->m_trickTextChecksum = 0;
	
	// Take a snapshot of the skater's position & orientation so that if the gap is successfully
	// got this info can be recorded into the CGapCheck instance and used to determine a good
	// camera position for viewing the gap in the view-gaps menu.
	pGap->m_skater_start_pos=((CSkater*)GetObject())->GetCamera()->GetPos();
	pGap->m_skater_start_dir=-((CSkater*)GetObject())->GetCamera()->GetMatrix()[Mth::AT];

	// if it has a trickscript, it's a gaptrick
	uint32 trickscript;
	if (pParams->GetChecksum(CRCD(0xa26994e6, "trickscript"), &trickscript))
	{
		uint32 key_combo;
		const char* tricktext;
		Script::CArray* p_gapTricks;
		
		pGap->m_trickscript = trickscript;
		
		Dbg_Assert(mp_score);

		if (pParams->GetChecksum(CRCD(0x95e16467,"KeyCombo"), &key_combo))
		{
			pGap->m_trickChecksum = key_combo;

			int spin;
			if ( pParams->GetInteger(CRCD(0xedf5db70, "spin"), &spin))
			{
				pGap->m_requirePerfect = pParams->ContainsFlag(CRCD(0x1c39f1b9, "perfect"));
				Dbg_MsgAssert(spin % 180 == 0, ("StartGap called with a spin value of %i which is not a multiple of 180", spin));
				pGap->m_spinMult = spin / 180;
			}
			pParams->GetInteger( CRCD(0xa4bee6a1,"num_taps"), &pGap->m_numTaps, Script::NO_ASSERT );

			pGap->m_numTrickOccurrences = mp_score->GetPreviousNumberOfOccurrences( key_combo, pGap->m_spinMult, pGap->m_numTaps );
		}
		else if (pParams->GetString(CRCD(0x3eafa520, "TrickText"), &tricktext))
		{
			pGap->m_trickTextChecksum = Script::GenerateCRC(tricktext);

			int spin;
			if (pParams->GetInteger(CRCD(0xedf5db70, "spin"), &spin))
			{
				pGap->m_requirePerfect = pParams->ContainsFlag(CRCD(0x1c39f1b9, "perfect"));
				Dbg_MsgAssert(spin % 180 == 0, ("StartGap called with a spin value of %i which is not a multiple of 180", spin));
				pGap->m_spinMult = spin / 180;
			}
			
			pParams->GetInteger( CRCD(0xa4bee6a1,"num_taps"), &pGap->m_numTaps, Script::NO_ASSERT );

			pGap->m_numTrickOccurrences = mp_score->GetPreviousNumberOfOccurrencesByName( pGap->m_trickTextChecksum, pGap->m_spinMult, pGap->m_numTaps );
		}
		else if (pParams->GetArray(CRCD(0x1e26fd3e, "tricks"), &p_gapTricks))
		{
			Script::CopyArray(pGap->mp_tricks, p_gapTricks);
			pGap->m_startGapTrickCount = mp_score->GetCurrentTrickCount();
		}
		
		// create the gap trick object
		start_gap_trick(pParams, pScript);
	}

	
	pGap->m_flags = 0;
	Script::CArray* pArray = NULL;			
	pParams->GetArray(CRCD(0xf4fabe45, "flags"), &pArray);
	if (pArray)
	{
		for (uint32 i = 0; i < pArray->GetSize(); i++)
		{
			int checksum = pArray->GetChecksum(i);
			pGap->m_flags |= Script::GetInteger(checksum);	
		}
	}
	else
	{
		uint32 checksum;
		if (pParams->GetChecksum(CRCD(0xf4fabe45, "flags"), &checksum, false))
		{
			pGap->m_flags |= Script::GetInteger(checksum);
		}
	}

	// K: Added this as part of implementing ability to set cancel type for gaps in the Park Editor. (TT1969)
	// When the park editor runs the StartGap command, it generates the passed parameters in the C-code.
	// At that point, the PE already has the or'd together cancel flags. It would be tricky to try to construct
	// the array of flag checksums at that point, since it would have to do lots of compares with script globals
	// and if any new ones were added we'd have to remember to update that bit of code.
	// Easier to just support passing of the combined flags here.
	uint32 combined_flags=0;
	if (pParams->GetChecksum(CRCD(0x2760de9e,"combined_flags"),&combined_flags))
	{
		pGap->m_flags |= combined_flags;
	}
	
	uint32 car_cancel_flags=CANCEL_SKATE | CANCEL_WALK;
	
	if (pParams->ContainsFlag(CRCD(0xc442e33d, "carGap")))
	{
		// car gaps cancel when skating or walking
		pGap->m_flags |= car_cancel_flags;
	}
	else if (pParams->ContainsFlag(CRCD(0xe3ed4ade, "complexGap")))
	{
		// complex gaps must be setup completely by their designer
	}
	else
	{
		// default to gaps which cancel driving
		if (pGap->m_flags & car_cancel_flags)
		{
			// Unless we decided we did want to drive earlier
		}
		else
		{
			pGap->m_flags |= CANCEL_DRIVE;
		}	
	}
	
	// Because the car can trigger multiple trigger scripts in the same frame, we must allow the gap to be canceled immediately.  Otherwise, if one wheel
	// triggers a start gap and another an end gap in the same frame, the cancel flags of the gap will never be checked.
	if (mp_physics_control_component->IsDriving() && (pGap->m_flags & CANCEL_DRIVE)
		// Created park gaps can be so close together that we might not have a frame with which to check our cancel flags
		|| Ed::CParkEditor::Instance()->UsingCustomPark())
	{
		int cancel, require;
		get_state_flags(cancel, require);
		UpdateCancelRequire(cancel, require);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterGapComponent::start_gap_trick ( Script::CStruct *pParams, Script::CScript* pScript )
{
	// create the gap trick and add the info to it
	CGapTrick* pGapTrick = new CGapTrick;
	m_gaptrick_list.AddToHead(pGapTrick);
	pGapTrick->m_node = pScript->mNode;
	pGapTrick->m_got = false;
	pGapTrick->m_frame = m_frame_count;

	uint32 gap_id = 0;
	pParams->GetChecksum(CRCD(0x3b442e26, "GapId"), &gap_id);
	pGapTrick->m_id = gap_id;

	const char *p_trickString = NULL;
	pParams->GetString(CRCD(0x3eafa520, "TrickText"), &p_trickString);
	pGapTrick->m_trickString = p_trickString;
	
	pGapTrick->m_script = 0;
	pParams->GetChecksum(CRCD(0xa26994e6, "trickscript"), &pGapTrick->m_script, Script::ASSERT);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterGapComponent::end_gap ( Script::CStruct *pParams, Script::CScript* pScript )
{
	if (mp_core_physics_component->GetFlag(IS_BAILING)) return;
	
	uint32 gap_id = 0;
	pParams->GetChecksum(CRCD(0x3b442e26, "GapID"), &gap_id);
	
	#ifdef DEBUG_GAPS
	MESSAGE("ENDING GAP");
	DUMPC(gap_id);
	#endif
	
	Dbg_Assert(mp_score);

	// find the ending gap in our gap list
	CGap* pNext;
	for (CGap* pGap = static_cast< CGap* >(m_gap_list.FirstItem()); pGap; pGap = pNext)
	{
		pNext = static_cast< CGap* >(pGap->GetNext());
		
		if (pGap->m_id != gap_id) continue;
		
		// check if its got a gap_id, or if the gaps node is linked to this node					
		if (gap_id == 0 && !SkateScript::IsLinkedTo(pGap->m_node, pScript->mNode)) continue;
				
		// found the gap
		
		// check requirements
		if (pGap->m_flags & REQUIRE_MASK)
		{							 
			// requirements failed, so just delete the gap
			delete pGap;
			continue;
		}
		
		// requirements met

		// Get the gap name, this is what is displayed as the trick name for this gap.
		// This is not required, and will default if not there.
		const char *p_name;
		if (!pParams->GetString(CRCD(0xc4745838, "text"), &p_name))
		{
			p_name = "Unnamed Gap";
		}
		char gap_name[256];
		sprintf(gap_name, "\\c1%s\\c0", p_name);


		// check for gap trick; this MUST occur before the gap is added to the trick string
		if (pGap->m_trickscript != 0)
		{
			bool gotGapTrick = false;
			
			if (pGap->m_trickChecksum != 0 || pGap->m_trickTextChecksum != 0 || pGap->mp_tricks->GetSize() > 0)
			{
				// see if the trick has been done since the gap started
				if ( pGap->m_trickChecksum && pGap->m_numTrickOccurrences < mp_score->GetCurrentNumberOfOccurrences( pGap->m_trickChecksum, pGap->m_spinMult, pGap->m_requirePerfect, pGap->m_numTaps ) )
				{
					gotGapTrick = true;
				}
				else if ( pGap->m_trickTextChecksum && pGap->m_numTrickOccurrences < mp_score->GetCurrentNumberOfOccurrencesByName( pGap->m_trickTextChecksum, pGap->m_spinMult, pGap->m_requirePerfect, pGap->m_numTaps ) )
				{
					gotGapTrick = true;
				}
				else if ( pGap->mp_tricks->GetSize() > 0 && mp_score->GetCurrentNumberOfOccurrences( pGap->mp_tricks, pGap->m_startGapTrickCount ) > 0 )
				{
					gotGapTrick = true;
				}

				// if they get the gap trick
				if ( gotGapTrick )
				{
					// get the gap trick
					CGapTrick* pGapTrick = static_cast< CGapTrick* >(m_gaptrick_list.FirstItem());
					while ( pGapTrick )
					{
						CGapTrick* pNext = static_cast< CGapTrick* >(pGapTrick->GetNext());
						
						if (pGapTrick->m_id == pGap->m_id)
						{
							pGapTrick->GetGapTrick();
							break;
						}

						pGapTrick = pNext;
					}
				} // END if they get the gap trick
			}
			else 
			{
				// if no trick script, look for a no requirement gap trick
				CGapTrick* pGapTrick = static_cast< CGapTrick* >(m_gaptrick_list.FirstItem());
				while (pGapTrick)
				{
					CGapTrick* pNext = static_cast< CGapTrick* >(pGapTrick->GetNext());

					if (pGapTrick->m_id == pGap->m_id)
					{
						pGapTrick->GetGapTrick();
						break;
					}
					pGapTrick = pNext;
				}
			}
		} // END if trickscript != 0


		Mdl::Skate::Instance()->GetGapChecklist()->SetInfoForCamera(p_name,pGap->m_skater_start_pos,pGap->m_skater_start_dir);
		
		// Get the score for this gap.
		// If there is no score defined, then we don't award a trick.
		int score = 0;
		if (pParams->GetInteger(CRCD(0xcd66c8ae, "score"), &score))
		{
			Mdl::Score::Flags flags = Mdl::Score::vGAP;
			if (GetSkater()->IsLocalClient())
			{
				mp_score->Trigger(gap_name, score, flags);
				
				CTrickComponent* p_trick_component = GetTrickComponentFromObject(GetObject());
				Dbg_Assert(p_trick_component);
				p_trick_component->SetFirstTrickStarted(true);

				Mdl::Skate::Instance()->GetGapChecklist()->GetGapByText(p_name);  
				  
				GetObject()->SpawnAndRunScript(CRCD(0x541a2485, "DefaultGapScript"));
			}
		}
		
		//  execute the gap_script
		uint32 gap_script;
		if (pParams->GetChecksum(CRCD(0x7c9e51ab, "gapscript"), &gap_script))
		{
			GetObject()->SpawnAndRunScript(
				gap_script,
				pScript->mNode,
				pParams->ContainsFlag(CRCD(0x20209c31, "NetEnabled")),
				pParams->ContainsFlag(CRCD(0x23627fd7, "Permanent"))
			);
		}	  			  
		
		// handle for continue script parameter
		Script::CStruct * p_continue = NULL;
		if (pParams->GetStructure(CRCD(0xec1cd520, "continue"), &p_continue))
		{
			start_gap(p_continue, pScript);
		}										  

		// Broadcast an event saying we got the gap
		// won't happen very often, and broadcasing is cheap)
		// we also pass in the parameters, just in case we want to do 
		uint32	id_got = Crc::ExtendCRCWithString(gap_id,"_Success");
		Obj::CTracker::Instance()->LaunchEvent( id_got, 0xffffffff, GetObject()->GetID(), pParams, true /*, radius */  );
						
		// Script functions we might have called could have deleted this gap and maybe the next gap.
		// The safest things is to check to see if this one is still there, delete it, then reset back to the start of the list.
		CGap* pKill = static_cast< CGap* >(m_gap_list.FirstItem());
		while (pKill)
		{
			if (pKill == pGap)
			{
				delete pGap;
				break;
			}
			pKill = static_cast< CGap* >(pKill->GetNext());
		}
		
		// restart our pass through the gaps
		pNext = static_cast< CGap* >(m_gap_list.FirstItem());
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterGapComponent::check_gap_tricks ( Script::CStruct *pParams )
{
	// look for completed gaps and run their scripts
	CGapTrick* pGapTrick = static_cast< CGapTrick* >(m_gaptrick_list.FirstItem());
	while (pGapTrick)
	{
		CGapTrick* pNext = static_cast< CGapTrick* >(pGapTrick->GetNext());
		if (pGapTrick->m_got)
		{
			GetObject()->SpawnAndRunScript(pGapTrick->m_script, pGapTrick->m_node);
		}
		pGapTrick = pNext;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterGapComponent::clear_gap_tricks ( Script::CStruct *pParams )
{
	if (pParams->ContainsFlag(CRCD(0x97975226, "NotInSameFrame")))
	{
		// delete all gap tricks leaving those created this frame
		CGapTrick* pGapTrick = static_cast< CGapTrick* >(m_gaptrick_list.FirstItem());
		while (pGapTrick)
		{
			CGapTrick* pNext = static_cast< CGapTrick* >(pGapTrick->GetNext());
			if (pGapTrick->m_frame != m_frame_count)
			{
				delete pGapTrick;
			}
			pGapTrick = pNext;
		}
	}
	else
	{
		// delete all gap tricks
		CGapTrick* pGapTrick = static_cast< CGapTrick* >(m_gaptrick_list.FirstItem());
		while (pGapTrick)
		{
			CGapTrick* pNext = static_cast< CGapTrick* >(pGapTrick->GetNext());
			delete pGapTrick;
			pGapTrick = pNext;
		}
	}
}

}
