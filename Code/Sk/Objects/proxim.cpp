///////////////////////////////////////////////////////
// Proxim.cpp

		  
#include <core/defines.h>
#include <core/singleton.h>
#include <core/math.h>
#include <gfx/camera.h>
#include <gfx/nx.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <modules/FrontEnd/FrontEnd.h>
#include <sk/objects/Proxim.h>
#include <sk/scripting/nodearray.h>
#include <sk/modules/skate/skate.h>
#include <sk/gamenet/gamenet.h>
#include <sk/objects/skater.h>		   // used to get position of the skater's camera
#include <sk/objects/moviecam.h>

//#define	DEBUG_PROXIM

namespace Obj
{

void Proxim_Init()
{
    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	
	if (skate_mod->mpProximManager)
	{
		Proxim_Cleanup();
	}
	skate_mod->mpProximManager = new CProximManager();
	skate_mod->mpProximManager->m_pFirstNode = NULL;
}

void Proxim_Cleanup()
{
    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	if (skate_mod->mpProximManager)
	{
		CProximNode *pNode = skate_mod->mpProximManager->m_pFirstNode;	   
		while (pNode)
		{
			CProximNode *pNext = pNode->m_pNext;
			delete pNode;
			pNode = pNext;			
		}

		delete skate_mod->mpProximManager;
		skate_mod->mpProximManager = NULL;
	}
}

#define CHECKSUM_TERRAIN_TYPE	0x54cf8532  // "terrainType"

void Proxim_AddNode(int node,bool active)
{
//	printf ("It's a Proxim (%d)\n",node);
    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Dbg_Assert(skate_mod->mpProximManager);
	
	// create a new Proxim node										   
	CProximNode *pProximNode = new CProximNode();

	// Link into the head of the Proxim manager									   
	pProximNode->m_pNext = skate_mod->mpProximManager->m_pFirstNode; 							  
	skate_mod->mpProximManager->m_pFirstNode = pProximNode;

	pProximNode->m_node = node;

	pProximNode->m_active = active;  // created at start or not?
	
	pProximNode->mp_sector = NULL;
	pProximNode->m_shape = CProximNode::vSHAPE_SPHERE;  

	// Get pointer to the Proxim data	
	Script::CScriptStructure *pNode = SkateScript::GetNode(node);
	
	// Get name
	if (pNode->GetChecksum("name",&pProximNode->m_name))
	{
#ifdef	DEBUG_PROXIM
		printf ("Proxim: Got Name %s\n",Script::FindChecksumName(pProximNode->m_name));
#endif	
	}
	else
	{
		Dbg_MsgAssert(0,("No name in proxim node %d\n",node));
	}

	// Get shape
	uint32 shape_checksum;
	if (pNode->GetChecksum("shape", &shape_checksum))
	{
		pProximNode->m_shape = CProximNode::sGetShapeFromChecksum(shape_checksum);
	}

	// Try to find geometry
	pProximNode->InitGeometry(Nx::CEngine::sGetMainScene()->GetSector(pProximNode->m_name));

	SkateScript::GetPosition(pNode,&pProximNode->m_pos);
#ifdef	DEBUG_PROXIM				
		Dbg_Message("Proxim: Pos = (%f, %f, %f)", pProximNode->m_pos[X], pProximNode->m_pos[Y], pProximNode->m_pos[Z]);
#endif	

	pProximNode->m_type = 0;  
	pNode->GetInteger( 0x7321a8d6 /* type */, &pProximNode->m_type );
#ifdef	DEBUG_PROXIM
	printf ("Proxim:  Type = %d\n",pProximNode->m_type);
#endif
	pNode->GetFloat (0xc48391a5 /*radius*/, &pProximNode->m_radius);  
	Dbg_MsgAssert(pProximNode->mp_sector || pProximNode->m_radius > 0.0f,( "Radius %f too small in proxim node %d\n",pProximNode->m_radius,node));	
#ifdef	DEBUG_PROXIM
	printf ("Proxim:  Radius = %f\n",pProximNode->m_radius);
#endif	
	pProximNode->m_radius_squared = pProximNode->m_radius * pProximNode->m_radius;	

	if (pNode->GetChecksum(0x2ca8a299,&pProximNode->m_script))		// Checksum of TriggerScript
	{
#ifdef	DEBUG_PROXIM
		printf ("Proxim: Got Script %s\n",Script::FindChecksumName(pProximNode->m_script));
#endif	
	}
	else
	{
		Dbg_MsgAssert(0,("No triggerscript in proxim node %d\n",node));
	}

	
	pProximNode->m_proxim_object = 0;	
	pProximNode->m_active_objects = 0;	
	pProximNode->m_active_scripts = 0;	
	if (pNode->ContainsFlag(CRCD(0x5113f19c,"ProximObject")))
	{
		// for proxim objects, it's best if we start "outside" the objects
		// so if we are actually inside, then the event will trigger
		pProximNode->m_outside_flags = 0xffffffff;			// say all outside
		pNode->GetChecksum("Name",&pProximNode->m_proxim_object);	
	}
	else
	{
		// for non-proxim objects, it's best if we start "inside" the node
		// in case something is depending on the outside script being called first
		// to keep things shut off.  We'll do this by starting with it active
		// and then setting it to inactive.
		pProximNode->m_outside_flags = 0;			// say all inside for now....
		pProximNode->m_active = true;

		// Set the script active flag for each skater that exists
		Obj::CObject *p_obj;
		int object_id = 0;
		while ((p_obj  = Obj::CCompositeObjectManager::Instance()->GetObjectByID(object_id)) && (object_id < 32))
		{
			Obj::CSkater *p_skater = (Obj::CSkater *) p_obj;
			if (p_skater->IsLocalClient())
			{
				pProximNode->m_active_scripts |= 1 << object_id;
			}
			object_id++;
		}

		pProximNode->SetActive(false);
		pProximNode->SetActive(active);
	}
				
}

void Proxim_AddedAll()
{
	
//	printf("Added all Proxims\n");		 
		 
    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Dbg_Assert(skate_mod->mpProximManager);
	CProximNode *pProximNode = skate_mod->mpProximManager->m_pFirstNode;
	while (pProximNode)
	{
		// do any final per-node processing here
		pProximNode = pProximNode->m_pNext;
	}				 
				 	
}			   

void Proxim_SetActive( int node, int active )
{
    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	
	if (!skate_mod->mpProximManager) return;
	
	CProximNode *pProximNode = skate_mod->mpProximManager->m_pFirstNode;
	while (pProximNode)
	{
		if (pProximNode->m_node == node)
		{
			pProximNode->SetActive(active);
			return;
		}
		pProximNode = pProximNode->m_pNext;
	}				 
}

CProximNode * CProximManager::sGetNode(uint32 checksum)
{
    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Dbg_Assert(skate_mod->mpProximManager);
	CProximNode *pProximNode = skate_mod->mpProximManager->m_pFirstNode;
	while (pProximNode)
	{
		if (pProximNode->m_name == checksum)
		{
			return pProximNode;
		}
		pProximNode = pProximNode->m_pNext;
	}				 

	return NULL;
}

// Update all the proximity nodes				
// for a certain skater
// checking for proximity either with the skater or the camera					
void Proxim_Update ( uint32 trigger_mask, CObject* p_script_target, const Mth::Vector& pos )
{
#if 0		// TT9254: Too many things were having problems with this.  The proxim node scripts themselves
			// will have to do this.
	// Don't update if the game is paused
	if( Mdl::FrontEnd::Instance()->GamePaused() && !(Script::GetInteger(CRCD(0x45e46afd,"goal_editor_placement_mode"))) )
	{
		return;
	}

#if 0
	// Don't update if the game is playing a CamAnim
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	if ( skate_mod->GetMovieManager()->IsRolling() )
	{
		return;
	}
#endif
#endif

	CProximManager* proxim_manager = Mdl::Skate::Instance()->mpProximManager;
	
	if (!proxim_manager) return;
	
	CProximNode *pProximNode = proxim_manager->m_pFirstNode;

	while (pProximNode)
	{
		if (!pProximNode->m_active)
		{
			pProximNode = pProximNode->m_pNext;
			continue;
		}
			
		bool now_inside = pProximNode->CheckInside(pos);

		if ( trigger_mask & pProximNode->m_outside_flags)
		{
			// trigger is currently outside, check if moved to inside
			if (now_inside)
			{
			
				pProximNode->m_outside_flags &= ~trigger_mask;
				
				if ( ! pProximNode->m_proxim_object )
				{
#ifdef	DEBUG_PROXIM				
				printf ("+++++ NOW INSIDE, mask = %d running script %s \n",trigger_mask, Script::FindChecksumName(pProximNode->m_script));
#endif				
					proxim_manager->m_inside_flag = true;
					proxim_manager->m_inside_flag_valid = true;
					
					if (p_script_target)
					{
						p_script_target->SpawnAndRunScript(pProximNode->m_script,pProximNode->m_node );
						Dbg_MsgAssert(p_script_target->GetID() < 32, ("Object ID %d is greater than 31", pProximNode->m_proxim_object));
						pProximNode->m_active_scripts |= 1 << p_script_target->GetID();
					}
					else
					{
						//Script::SpawnScript(pProximNode->m_script, NULL, NO_NAME, NULL, pProximNode->m_node );
						Script::RunScript(pProximNode->m_script);
					}

					proxim_manager->m_inside_flag_valid = false;
				}
				else if (p_script_target)
				{
					// Need to create an object with the id pProximNode->m_proxim_object

#ifdef	DEBUG_PROXIM				
				printf ("+++++ NOW INSIDE, creating object %s running script %s \n",Script::FindChecksumName(pProximNode->m_proxim_object),Script::FindChecksumName(pProximNode->m_script));
#endif				
					
					Obj::CCompositeObject* pObj = Obj::CCompositeObjectManager::Instance()->CreateCompositeObject();
					pObj->SetID(pProximNode->m_proxim_object + p_script_target->GetID());  // Mick mangle the proxim object ID with the player's ID
					pObj->m_pos = pProximNode->m_pos  ;

					Dbg_MsgAssert(p_script_target->GetID() < 32, ("Object ID %d is greater than 31", pProximNode->m_proxim_object));
					pProximNode->m_active_objects |= 1 << p_script_target->GetID();

					Script::CStruct component_params;	// ProximNode params
					if (pProximNode->HasGeometry())
					{
						// Only add if we want the geometry used
						component_params.AddChecksum("ProximNode", pProximNode->m_name);
					}
					pObj->CreateComponentsFromArray(Script::GetArray("proximobj_composite_structure"), &component_params);   
				 	pObj->Finalize();
   
					Script::CStruct params;	// dummy empty params
					pObj->SwitchScript(pProximNode->m_script,&params);
				}
			}			
		}
		else
		{
			// skater is currently inside, check if moved outside
			if (!now_inside)
			{
				pProximNode->m_outside_flags |= trigger_mask;				
				if ( ! pProximNode->m_proxim_object )
				{
	#ifdef	DEBUG_PROXIM				
					printf ("----- NOW OUTSIDE, mask = %d running script %s \n",trigger_mask,Script::FindChecksumName(pProximNode->m_script));
	#endif				
					proxim_manager->m_inside_flag = false;
					proxim_manager->m_inside_flag_valid = true;

					if (p_script_target)
					{
						p_script_target->SpawnAndRunScript(pProximNode->m_script,pProximNode->m_node );
						pProximNode->m_active_scripts &= ~(1 << p_script_target->GetID());
					}
					else
					{
						//Script::SpawnScript(pProximNode->m_script, NULL, NO_NAME, NULL, pProximNode->m_node );
						Script::RunScript(pProximNode->m_script);
					}

					proxim_manager->m_inside_flag_valid = false;
				}									
				else if (p_script_target)
				{
					// Need to create an object with the id pProximNode->m_proxim_object

#ifdef	DEBUG_PROXIM				
					printf ("+++++ NOW OUTSIDE, killing %s running script %s \n",Script::FindChecksumName(pProximNode->m_proxim_object),Script::FindChecksumName(pProximNode->m_script));
#endif				
					Obj::CObject *p_obj  = Obj::CCompositeObjectManager::Instance()->GetObjectByID(pProximNode->m_proxim_object  + p_script_target->GetID());
					if (p_obj)
					{
						p_obj->MarkAsDead();					
					}
					pProximNode->m_active_objects &= ~(1 << p_script_target->GetID());
				}
			}			
		}		
		pProximNode = pProximNode->m_pNext;
	}				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CProximNode::EShape CProximNode::sGetShapeFromChecksum(uint32 checksum)
{
	switch (checksum)
	{
	case 0xaa069978: // Sphere
		return vSHAPE_SPHERE;

	case 0xe460abde: // BoundingBox
		return vSHAPE_BBOX;

	case 0x3f1e5a7: // NonAxisAlignedBoundingBox
		return vSHAPE_NON_AA_BBOX;

	case 0x64fba415: // Cylinder
		return vSHAPE_CYLINDER;

	case 0x6aadf154: // Geometry
		return vSHAPE_GEOMETRY;

	default:
		Dbg_MsgAssert(0, ("Unknown shape checksum %x\n", checksum));
		return vSHAPE_SPHERE;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CProximNode::SetActive(bool active)
{
	// Check for changes
	if (m_active != active)
	{
		if (m_active)
		{
			// Turning off node, so treat like were going outside
			if ( m_proxim_object )
			{
				// Delete all created objects
#ifdef	DEBUG_PROXIM			    
				printf ("----- SETTING ProximNode INACTIVE, killing %s running script %s \n",Script::FindChecksumName(m_proxim_object),Script::FindChecksumName(m_script));
#endif				
				int object_id = 0;
				while (m_active_objects)
				{
					if (m_active_objects & 0x1)	// Check if this ID is turned on
					{
						Obj::CObject *p_obj  = Obj::CCompositeObjectManager::Instance()->GetObjectByID(m_proxim_object + object_id);
						if (p_obj)
						{
							p_obj->MarkAsDead();					
						}
					}

					// Advance ID
					m_active_objects = m_active_objects >> 1;
					object_id++;
				}
			}									
			else
			{
#ifdef	DEBUG_PROXIM				
				printf ("----- SETTING ProximNode INACTIVE, running script %s \n",Script::FindChecksumName(m_script));
#endif				
				CProximManager* proxim_manager = Mdl::Skate::Instance()->mpProximManager;

				if (proxim_manager)
				{
					int object_id = 0;
					while (m_active_scripts)
					{
						if (m_active_scripts & 0x1)	// Check if this ID is turned on
						{
							Obj::CObject *p_obj  = Obj::CCompositeObjectManager::Instance()->GetObjectByID(object_id);
							Dbg_MsgAssert(p_obj, ("Can't find object to play script on"));
							proxim_manager->m_inside_flag = false;

							proxim_manager->m_inside_flag_valid = true;
							p_obj->SpawnAndRunScript( m_script, m_node );
							proxim_manager->m_inside_flag_valid = false;
						}

						// Advance ID
						m_active_scripts = m_active_scripts >> 1;
						object_id++;
					}
				}
			}
		}

		// Make everything outside
		m_outside_flags = 0xffffffff;

		m_active = active;
	}

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CProximNode::InitGeometry(Nx::CSector *p_sector)
{
	mp_sector = p_sector;

	if (!mp_sector)
	{
		// No Geometry
		Dbg_MsgAssert(m_shape == CProximNode::vSHAPE_SPHERE, ("ProximNode %s with shape %d has no geometry.", Script::FindChecksumName(m_name), m_shape));
		return;
	}

#ifdef	DEBUG_PROXIM				
	Dbg_Message("Found Geometry for Proxim node %s at address %x", Script::FindChecksumName(m_name), mp_sector);
#endif

	switch (m_shape)
	{
	case CProximNode::vSHAPE_SPHERE:
		Dbg_MsgAssert(0, ("ProximNode with sphere shape has geometry."));
		break;

	case CProximNode::vSHAPE_BBOX:
		m_bbox = mp_sector->GetBoundingBox();
#ifdef	DEBUG_PROXIM				
		Dbg_Message("Bounding box of geometry (%f, %f, %f) - (%f, %f, %f)",
					m_bbox.GetMin()[X], m_bbox.GetMin()[Y], m_bbox.GetMin()[Z],
					m_bbox.GetMax()[X], m_bbox.GetMax()[Y], m_bbox.GetMax()[Z]);
#endif	
		break;

	case CProximNode::vSHAPE_NON_AA_BBOX:
		Dbg_MsgAssert(0, ("ProximNode with non axis aligned bounding box shape not supported yet."));
		break;

	case CProximNode::vSHAPE_CYLINDER:
		Dbg_MsgAssert(0, ("ProximNode with cylinder shape not supported yet."));
		break;
	case CProximNode::vSHAPE_GEOMETRY:
		Dbg_MsgAssert(0, ("ProximNode with geometry shape not supported yet."));
		break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool				CProximNode::HasGeometry() const
{
	return mp_sector != NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CProximNode::CheckInside(const Mth::Vector &object_pos) const
{
	Mth::Vector		node_to_object = object_pos - m_pos;
	float			dist_squared = node_to_object.LengthSqr();	   

	bool inside = false;

	switch (m_shape)
	{
	case vSHAPE_SPHERE:
		inside = (dist_squared <= m_radius_squared);
		break;

	case vSHAPE_BBOX:
		inside = m_bbox.Within(object_pos);
#ifdef	DEBUG_PROXIM
		m_bbox.DebugRender(0xff0000ff);
		{
			Mth::Vector closest_point;
			m_bbox.GetClosestIntersectPoint(object_pos, closest_point);
//			Gfx::AddDebugStar(closest_point);
		}
#endif //	DEBUG_PROXIM
		break;

	default:
		Dbg_MsgAssert(0, ("Can't handle ProximNode type %d", m_shape));
	}

	return inside;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector 		CProximNode::GetClosestIntersectionPoint(const Mth::Vector &object_pos) const
{
	Mth::Vector intersect_point;

	switch (m_shape)
	{
	case vSHAPE_SPHERE:
		{
			// Same for either within or outside sphere
			Mth::Vector direction = object_pos - m_pos;
			direction.Normalize();
			direction *= m_radius;

			intersect_point = m_pos + direction;
		}
		break;

	case vSHAPE_BBOX:
		m_bbox.GetClosestIntersectPoint(object_pos, intersect_point);
		break;

	default:
		Dbg_MsgAssert(0, ("Can't handle ProximNode type %d", m_shape));
		break;
	}

#ifdef	DEBUG_PROXIM
//	Gfx::AddDebugStar(intersect_point);
#endif //	DEBUG_PROXIM

	return intersect_point;
}

/*
{
	// Node 572 (Mick Test)
	Position = (-4060.666992,131.999329,-4604.379395)
	Angles = (0.000000,-3.141593,0.000000)
	Name = TRG_Micks_test
	Class = ProximNode
	Type = skater
    radius = 1200
	TriggerScript = xxx_test
}
*/


}
			   

