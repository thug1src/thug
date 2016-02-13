///////////////////////////////////////////////////////
// Emitter.cpp

		  
#include <core/defines.h>
#include <core/singleton.h>
#include <core/math.h>
#include <gfx/camera.h>
#include <gfx/nx.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <sk/objects/Emitter.h>
#include <sk/scripting/nodearray.h>
#include <sk/modules/skate/skate.h>
#include <sk/gamenet/gamenet.h>
#include <sk/objects/skater.h>		   // used to get position of the skater's camera

//#define	DEBUG_EMITTER

namespace Obj
{

Lst::HashTable<CEmitterObject> *	CEmitterManager::sp_emitter_lookup = NULL;

void CEmitterManager::sInit()
{
	if (sp_emitter_lookup)
	{
		sCleanup();
	}
	else
	{
		sp_emitter_lookup = new Lst::HashTable<CEmitterObject>(4);	
	}
}

void CEmitterManager::sCleanup()
{
	if (sp_emitter_lookup)
	{
		sp_emitter_lookup->IterateStart();
		CEmitterObject *pNode = sp_emitter_lookup->IterateNext();
		while (pNode)
		{
			CEmitterObject *pNext = sp_emitter_lookup->IterateNext();
			uint32 checksum = pNode->m_name;   
			delete pNode;
			sp_emitter_lookup->FlushItem(checksum);			
			pNode = pNext;
	   	}

//		delete sp_emitter_lookup;
//		sp_emitter_lookup = NULL;
	}
}

void CEmitterManager::sAddEmitter(int node,bool active)
{
							
						
//	printf ("It's a Emitter (%d)\n",node);
	
	// create a new EmitterObject
	CEmitterObject *pEmitterObject = new CEmitterObject();

	pEmitterObject->m_node = node;

	pEmitterObject->m_active = active;  // created at start or not?
	
	pEmitterObject->mp_sector = NULL;
	pEmitterObject->m_shape_type = CEmitterObject::vSHAPE_SPHERE;  

	// Get pointer to the Emitter data	
	Script::CScriptStructure *pNode = SkateScript::GetNode(node);
	
	// Get name
	if (pNode->GetChecksum("name",&pEmitterObject->m_name))
	{
#ifdef	DEBUG_EMITTER
		printf ("Emitter: Got Name %s\n",Script::FindChecksumName(pEmitterObject->m_name));
#endif	
	}
	else
	{
		Dbg_MsgAssert(0,("No name in proxim node %d\n",node));
	}

	// Add to hash table
	sp_emitter_lookup->PutItem(pEmitterObject->m_name, pEmitterObject);

	// Get shape type
	uint32 shape_checksum;
	if (pNode->GetChecksum("type", &shape_checksum))
	{
		pEmitterObject->m_shape_type = CEmitterObject::sGetShapeFromChecksum(shape_checksum);
	}

	// Try to find geometry
	pEmitterObject->InitGeometry(Nx::CEngine::sGetMainScene()->GetSector(pEmitterObject->m_name));

	SkateScript::GetPosition(pNode,&pEmitterObject->m_pos);
#ifdef	DEBUG_EMITTER				
		Dbg_Message("Emitter: Pos = (%f, %f, %f)", pEmitterObject->m_pos[X], pEmitterObject->m_pos[Y], pEmitterObject->m_pos[Z]);
#endif	

	pNode->GetFloat (0xc48391a5 /*radius*/, &pEmitterObject->m_radius);  
	Dbg_MsgAssert(pEmitterObject->mp_sector || pEmitterObject->m_radius > 0.0f,( "Radius %f too small in proxim node %d\n",pEmitterObject->m_radius,node));	
#ifdef	DEBUG_EMITTER
	printf ("Emitter:  Radius = %f\n",pEmitterObject->m_radius);
#endif	
	pEmitterObject->m_radius_squared = pEmitterObject->m_radius * pEmitterObject->m_radius;	

	if (pNode->GetChecksum(0x2ca8a299,&pEmitterObject->m_script))		// Checksum of TriggerScript
	{
#ifdef	DEBUG_EMITTER
		printf ("Emitter: Got Script %s\n",Script::FindChecksumName(pEmitterObject->m_script));
#endif	
	}
	else
	{
		//Dbg_MsgAssert(0,("No triggerscript in proxim node %d\n",node));
	}

	
	//pEmitterObject->m_outside_flags = 0;			// say all inside for now....
}

void CEmitterManager::sAddedAll()
{
	
//	printf("Added all Emitters\n");		 
		 
	Dbg_Assert(sp_emitter_lookup);

	sp_emitter_lookup->IterateStart();
	CEmitterObject *pEmitterObject = sp_emitter_lookup->IterateNext();

	while (pEmitterObject)
	{
		// do any final per-node processing here
		pEmitterObject = sp_emitter_lookup->IterateNext();
	}				 
				 	
}			   

CEmitterObject * CEmitterManager::sGetEmitter(uint32 checksum)
{
	Dbg_Assert(sp_emitter_lookup);

	CEmitterObject *pEmitter = sp_emitter_lookup->GetItem(checksum);

	if (!pEmitter)
	{
		Dbg_Message("Warning: Can't find emitter %s", Script::FindChecksumName(checksum));
	}

	return pEmitter;
}

void CEmitterManager::sSetEmitterActive( int node, int active )
{
	Dbg_Assert(sp_emitter_lookup);

	sp_emitter_lookup->IterateStart();
	CEmitterObject *pEmitterObject = sp_emitter_lookup->IterateNext();
	while (pEmitterObject)
	{
		if (pEmitterObject->m_node == node)
		{
			pEmitterObject->m_active = active;
			return;
		}
		pEmitterObject = sp_emitter_lookup->IterateNext();
	}				 
}

#if 0
// Update all the proximity nodes				
// for a certain skater
// checking for proximity either with the skater or the camera					
void CEmitterManager::sUpdateEmitters(CSkater *pSkater, Gfx::Camera* camera)
{
	Dbg_Assert(sp_emitter_lookup);

	sp_emitter_lookup->IterateStart();
	CEmitterObject *pEmitterObject = sp_emitter_lookup->IterateNext();

    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	if( skate_mod->mpProximManager == NULL )
	{
		return;
	}
	
	int	skater_num = 0;						// 0 for now, should be viewport index

	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	if (!gamenet_man->InNetGame())
	{
		if (skate_mod->GetSkater(1) == pSkater)
		{
			skater_num = 1;
		}
	}

	int	skater_mask = 1 << skater_num;		// the bitmask for this skater

//	Mth::Vector	skater_pos = pSkater->m_pos;	
//	Mth::Vector	skater_old_pos = pSkater->m_old_pos;	

	// Aieeeeee!!!
	// to get the camera's position, we have to dig down to a low level
	// the CSkater::GetSkaterCam returns a *CSkaterCam
	// CSkaterCam::GetCamera returns a 	reference to Gfx::Camera
	// Gfx::Camera::GetCamera returns a *NsCamera (pointer)
	// so we pass that to RwCameraGetFrame, so we can get the position of the camera...
	// Matt added m_pos to CSkaterCam and removed the code below to find the camera:
	
//	Gfx::Camera* 	p_camera = pSkater->GetSkaterCam()->mpCamera ;
	Mth::Vector	camera_pos;
	// Local clients can get the camera's position directly from the skater cam
	// but observers need to get it from the camera that was passed in since
	// the skater they're following doesn't have a skatercam

// Mick - camera passed in is the active camera
//	if( pSkater->IsLocalClient())
//	{
//		camera_pos = pSkater->GetSkaterCam()->GetPos();
//	}
//	else
//	{
		camera_pos = camera->GetPos();
//	}
	
	while (pEmitterObject)
	{
	
	//Mth::Vector to_skater = skater_pos - pEmitterObject->m_pos;
	// Bitfield per viewport....
			
//		printf ("Proxim: Update,  Script %s\n",Script::FindChecksumName(pEmitterObject->m_script));	

		// find the squared dist from this node:
		
		// This code would crash with observers since the skater they were following had no cam
		//Mth::Vector		node_to_camera = pSkater->GetSkaterCam( )->m_pos - pEmitterObject->m_pos;
        
		bool now_inside = pEmitterObject->CheckInside(camera_pos);

		if ( skater_mask & pEmitterObject->m_outside_flags)
		{
			// skater is currently outside, check if moved to inside
			if (now_inside)
			{
			
				pEmitterObject->m_outside_flags &= ~skater_mask;
				pSkater->m_inside = true;
				
#ifdef	DEBUG_EMITTER				
				printf ("+++++ NOW INSIDE, running script %s \n",Script::FindChecksumName(pEmitterObject->m_script));
#endif				
				pSkater->SpawnAndRunScript(pEmitterObject->m_script,pEmitterObject->m_node );
			}			
		}
		else
		{
			// skater is currently inside, check if moved outside
			if (!now_inside)
			{
				pEmitterObject->m_outside_flags |= skater_mask;				
				pSkater->m_inside = false;
	#ifdef	DEBUG_EMITTER				
				printf ("----- NOW OUTSIDE, running script %s \n",Script::FindChecksumName(pEmitterObject->m_script));
	#endif				
					
				pSkater->SpawnAndRunScript(pEmitterObject->m_script,pEmitterObject->m_node );
			}			
		}		
		pEmitterObject = sp_emitter_lookup->IterateNext();
	}				 
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CEmitterObject::EShape CEmitterObject::sGetShapeFromChecksum(uint32 checksum)
{
	switch (checksum)
	{
	case 0xaa069978: // Sphere
		return vSHAPE_SPHERE;

	case 0xe460abde: // BoundingBox
		return vSHAPE_BBOX;

	case 0x3f1e5a7: // NonAxisAlignedBoundingBox
		return vSHAPE_NON_AA_BBOX;

	default:
		Dbg_MsgAssert(0, ("Unknown shape checksum %x\n", checksum));
		return vSHAPE_SPHERE;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CEmitterObject::InitGeometry(Nx::CSector *p_sector)
{
	mp_sector = p_sector;

	if (!mp_sector)
	{
		// No Geometry
		Dbg_MsgAssert(m_shape_type == CEmitterObject::vSHAPE_SPHERE, ("EmitterObject with shape %d has no geometry.", m_shape_type));
		return;
	}

#ifdef	DEBUG_EMITTER				
	Dbg_Message("Found Geometry for EmitterObject %s at address %x", Script::FindChecksumName(m_name), mp_sector);
#endif

	switch (m_shape_type)
	{
	case CEmitterObject::vSHAPE_SPHERE:
		Dbg_MsgAssert(0, ("EmitterObject with sphere shape has geometry."));
		break;

	case CEmitterObject::vSHAPE_BBOX:
		m_bbox = mp_sector->GetBoundingBox();
#ifdef	DEBUG_EMITTER				
		Dbg_Message("Bounding box of geometry (%f, %f, %f) - (%f, %f, %f)",
					m_bbox.GetMin()[X], m_bbox.GetMin()[Y], m_bbox.GetMin()[Z],
					m_bbox.GetMax()[X], m_bbox.GetMax()[Y], m_bbox.GetMax()[Z]);
#endif	
		break;

	case CEmitterObject::vSHAPE_NON_AA_BBOX:
		Dbg_MsgAssert(0, ("EmitterObject with non axis aligned bounding box shape not supported yet."));
		break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CEmitterObject::CheckInside(const Mth::Vector &object_pos) const
{
	Mth::Vector		node_to_object = object_pos - m_pos;
	float			dist_squared = node_to_object.LengthSqr();	   

	bool inside = false;

	switch (m_shape_type)
	{
	case vSHAPE_SPHERE:
		inside = (dist_squared <= m_radius_squared);
		break;

	case vSHAPE_BBOX:
		inside = m_bbox.Within(object_pos);
#ifdef	DEBUG_EMITTER
		m_bbox.DebugRender(0xff0000ff);
#endif //	DEBUG_EMITTER
		break;

	default:
		Dbg_MsgAssert(0, ("Can't handle EmitterObject type %d", m_shape_type));
		break;
	}

	return inside;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector 		CEmitterObject::GetClosestPoint(const Mth::Vector &object_pos) const
{
	// Return original position if within the object
	if (CheckInside(object_pos))
	{
		return object_pos;
	}

	Mth::Vector intersect_point;

	switch (m_shape_type)
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
		Dbg_MsgAssert(0, ("Can't handle EmitterObject type %d", m_shape_type));
		break;
	}

#ifdef	DEBUG_EMITTER
	Gfx::AddDebugStar(intersect_point);
#endif //	DEBUG_EMITTER

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
			   

