//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SuspendComponent.cpp
//* OWNER:          ???
//* CREATION DATE:  ??/??/??
//****************************************************************************

#include <gel/components/suspendcomponent.h>

#include <gel/components/modelcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>

#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>
#include <gel/mainloop.h>

#include <gfx/nx.h>
#include <gfx/nxmodel.h>
#include <gfx/nxviewman.h>

#include <sk/modules/skate/skate.h>		// <<<<<<<<<<<   GET RID OF THIS, use the NX camera's directly
#include <sk/objects/skatercareer.h>	// <<<<<<<<<<<   GET RID OF THIS TOO!

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// This static function is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
CBaseComponent* CSuspendComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSuspendComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// And this one is the optional "register" function that provides on-time initilization
// usually of a component manager
void	CSuspendComponent::s_register()
{
	// Create and initilize the manager
	SuspendManager::Manager::Create();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSuspendComponent::CSuspendComponent() : CBaseComponent()
{
	SetType( CRC_SUSPEND );
	
	m_suspend_distance=Script::GetFloat(CRCD(0xad501995,"DefaultMovingObjectSuspendDistance")) * 12.0f;
	m_suspend_distance_squared=m_suspend_distance*m_suspend_distance;
	m_never_suspend=true;
	m_no_suspend_count = 3; // default to not allowing suspensions in the first 3 frames

	// Mick: We don't want to display things before they have done at least one frame of
	// animation, because they start out in their default position (the "Blair Witch" position)
	// so, we set a default number of animations that we MUST do before we are allowed to
	// not animate.
	// This also overrides the SkipLogic test, so we do a frame of logic
	// which fixes problems where something is moving to the first node of a path
	
	// Since the first couple of frames of the game are hidden behind the loading screen
	// we don't need to worry about framerate glitched due to the large 
	// number of objects being created at start.	
	
	// I Initially had this at 1, which seemed fine
	// however, when you change levels, it needs to be at 3
	// I'm not sure why - it might be good to look into this later.
	m_initial_animations = 3;	 	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSuspendComponent::~CSuspendComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSuspendComponent::InitFromStructure( Script::CStruct* pParams )
{
	// anything that has a suspend distance will allow suspension
	if (pParams->GetFloat(CRCD(0xe4279ba2,"SuspendDistance"),&m_suspend_distance))
	{
		if (m_suspend_distance == 0.0f)
		{
			// if zero suspend dist, then try to get it from lod_dist1
			if (!pParams->GetFloat(CRCD(0x4eedfae7,"Lod_dist1"),&m_suspend_distance))
			{
				// else get the default distance
				m_suspend_distance=Script::GetFloat(CRCD(0xad501995,"DefaultMovingObjectSuspendDistance"));
			}
		}
	
		// if we've got a suspend distance, then convert it into inches
		m_suspend_distance *= 12.0f;
		m_suspend_distance_squared=m_suspend_distance*m_suspend_distance;
		m_never_suspend=false;
	}
	
	// suspension can be overridden by the "NeverSuspend" flag
	if (pParams->ContainsFlag(CRCD(0xcb839dc1,"NeverSuspend")))
	{
		m_never_suspend=true;
	}

	// Get the LOD distances from the node
	if (Script::GetInt(CRCD(0x95edc6f6,"NoLOD"),false))					 
	{
		m_lod_dist[0] = 1000000;
		m_lod_dist[1] = 1000001;
		m_lod_dist[2] = 1000002;  // Note, this is used as a magic number to turn off the high level culling
		m_lod_dist[3] = 1000003;
	}
	else
	{
		Dbg_Assert( pParams );
		m_lod_dist[0] = 50;
		m_lod_dist[2] = 1000001;
		m_lod_dist[3] = 1000002;
		pParams->GetFloat(CRCD(0x4eedfae7,"lod_dist1"),&m_lod_dist[0]);
		m_lod_dist[1] = m_lod_dist[0] + 75;						// default for LOD dist 2 is 75 feet beyond 1
		pParams->GetFloat(CRCD(0xd7e4ab5d,"lod_dist2"),&m_lod_dist[1]);
		pParams->GetFloat(CRCD(0xa0e39bcb,"lod_dist3"),&m_lod_dist[2]);
		pParams->GetFloat(CRCD(0x3e870e68,"lod_dist4"),&m_lod_dist[3]);
		// LOD dist is in feet
		m_lod_dist[0] *= 12.0f;
		m_lod_dist[1] *= 12.0f;
		m_lod_dist[2] *= 12.0f;
		m_lod_dist[3] *= 12.0f;
		if ( m_lod_dist[0]>=m_lod_dist[1] )
		{
			Script::PrintContents( pParams );
			Dbg_MsgAssert(0,("lod_dist1 (%f) >= lod_dist2 (%f) in node",m_lod_dist[0],m_lod_dist[1]));
		}
		if ( m_lod_dist[1]>=m_lod_dist[2] )
		{
			Script::PrintContents( pParams );
			Dbg_MsgAssert(0,("lod_dist2 (%f) >= lod_dist3 (%f) in node",m_lod_dist[1],m_lod_dist[2]));
		}
		if ( m_lod_dist[2]>=m_lod_dist[3] )
		{
			Script::PrintContents( pParams );
			Dbg_MsgAssert(0,("lod_dist3 (%f) >= lod_dist4 (%f) in node",m_lod_dist[2],m_lod_dist[3]));
		}

#if defined( __PLAT_XBOX__ ) || defined( __PLAT_NGC__ )
		// Some platform specific changes to the LOD stuff
		m_lod_dist[0]		*= Script::GetFloat( 0xd3d072f9 /* LOD0DistanceMultiplier */ );
		m_lod_dist[1]		*= Script::GetFloat( 0x0432f2a1 /* LOD1DistanceMultiplier */ );
#endif

	}
}

void	CSuspendComponent::Finalize()
{
	// Might be NULL, if we don't have a model
	mp_model_component = GetModelComponentFromObject( GetObject() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSuspendComponent::Update()
{
	// this funtion does not need to be called, so flag it as such
	CBaseComponent::Suspend(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent::EMemberFunctionResult CSuspendComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
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
	
void CSuspendComponent::Suspend(bool suspend)
{
	// does nothing, as we need to keep checking for unsuspension
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// returns true if we skip logic on this frame
// this can be true for a number of reasons
// but mostly boils down to LOD
bool CSuspendComponent::SkipLogic()
{
	return m_skip_logic;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// returns true if we skip logic on this frame
// this can be true for a number of reasons
// but mostly boils down to LOD
bool CSuspendComponent::SkipRender()
{
		
	if ( m_camera_distance_squared > m_lod_dist[1]*m_lod_dist[1] )
	{
		return true;
	}	
	
	return false;

}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSuspendComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSuspendComponent::GetDebugInfo"));
	// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	

	p_info->AddInteger(CRCD(0x7cf2a233,"m_never_suspend"),m_never_suspend);
	p_info->AddFloat(CRCD(0x519ab8e0,"m_suspend_distance"),m_suspend_distance);
	p_info->AddFloat(CRCD(0x340a53d3,"distance_to_camera"),sqrtf(m_camera_distance_squared));
	p_info->AddFloat(CRCD(0x266306b9,"m_lod_dist_0"),m_lod_dist[0]);
	p_info->AddFloat(CRCD(0x5164362f,"m_lod_dist_1"),m_lod_dist[1]);
	p_info->AddFloat(CRCD(0xc86d6795,"m_lod_dist_2"),m_lod_dist[2]);
	p_info->AddFloat(CRCD(0xbf6a5703,"m_lod_dist_3"),m_lod_dist[3]);
	
	p_info->AddInteger(CRCD(0xd9872f4b,"SKIPLOGIC_RETURNS"),SkipLogic());
	p_info->AddInteger(CRCD(0xd55771a6,"SKIPRENDER_RETURNS"),SkipRender());
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSuspendComponent::should_animate( float *p_dist )
{
	bool should_animate = true;  

	// Mick:  don't update the skeleton if it's a long way from the camera
//	Gfx::Camera * p_camera = Nx::CViewportManager::sGetActiveCamera( 0 );
//	if (p_camera)
	{
//		Mth::Vector cam_pos = p_camera->GetPos();
//		cam_pos = cam_pos - GetObject()->GetPos();
//		float dist = cam_pos.LengthSqr();

		float dist = m_camera_distance_squared;

		// Save this dist if a valid pointer was passed.
		if( p_dist )
		{
			*p_dist = dist;
		}

		if (dist > m_lod_dist[0] * m_lod_dist[0])		// dist at what we stop animating			
		{
		    should_animate = false;
		}

		// update animation intermittently
		// more so as distance from camera increases

		else
		{
#ifndef TESTING_GUNSLINGER

#if defined( __PLAT_XBOX__ ) || defined( __PLAT_NGC__ )
			float interleave2 = Script::GetFloat( 0xdb120fb5 /* AnimLODInterleave2 */ );
			float interleave4 = Script::GetFloat( 0x3271aa80 /* AnimLODInterleave4 */ );
			interleave2 *= interleave2;
			interleave4 *= interleave4;
#else
			float interleave2 = 500.0f * 500.0f;
			float interleave4 = 800.0f * 800.0f;
#endif

			m_interleave++;
			should_animate = false;				
			if (dist > interleave4)						  
			{
				// 1 in 4 for distance 500-1000
				if (!(m_interleave&3))
				{
					should_animate = true;
				}
			}
			else if (dist > interleave2)
			{
				// 1 in 2 for 200 to 500
				if ((m_interleave&1))
				{
					should_animate = true;
				}
			 }
			 else
			 {
				// always animate if < 200
				should_animate = true;
			 }
#endif		
		}

	}
	
	if (m_initial_animations)
	{
		should_animate = true;
	}

	return should_animate;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Set Model's Active/Inactive flag based on other flags
// and if the model is actually visible or not (with culling and occlusion)
// and upon the load_dist2 distance  
void CSuspendComponent::CheckModelActive()
{
	if ( mp_model_component && mp_model_component->GetModel() )
	{
		bool active = false;
		if ( GetObject()->GetFlags() & CObject::vINVISIBLE )
		{
			// just leave it inactive
		}
		else
		{
			// quick check for if the NoLOD flag is set
			// Note use of magic number (but this is only for debugging/screenshots)
			if ( m_lod_dist[2] == 1000002 )
			{
				active =  true ;
			}
			else
			{
				// PATCH for multiple viewports
				// always return true, until we sort out the per-viewport visibility (which might not be needed)				   
				if ( Nx::CViewportManager::sGetNumActiveViewports() > 1 )
				{
					active = true;
				}
				else if ( mp_model_component->HasRefObject() )
				{
					// GJ:  a ref model doesn't actually have a valid bounding sphere
					// so don't let Nx::CEngine::sIsVisible() be called...
					// (alternatively, we could fake a large bounding sphere
					// but that seems kludgy)
					active = true;
				}
				else
				{
					float dist = m_camera_distance_squared;
					
					if ( dist < m_lod_dist[1] * m_lod_dist[1] )
					{		
						Mth::Vector sphere = mp_model_component->GetModel()->GetBoundingSphere();
						Mth::Vector position = sphere;
						position[3] = 1.0f;
						position *= GetObject()->GetMatrix();
						position += GetObject()->GetPos();
						active = ( Nx::CEngine::sIsVisible( position, sphere[3]) );
					}
					else
					{
						active = false;
					}
				}
			}
		}
		bool	was_active = mp_model_component->GetModel()->GetActive();
		mp_model_component->GetModel()->SetActive( active);		
		// If we've just turned the model component back on, then run the update on it
		// so we set it to the right place
		// otherwise it will be in the old place for a frame
		// TODO:  This all might be better IN the modelcomponent
		if (active && !was_active)
		{
			mp_model_component->Update();			
		}
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CSuspendComponent::GetDistanceSquaredToCamera()
{
	Gfx::Camera * p_camera = Nx::CViewportManager::sGetActiveCamera( 0 );
	if ( p_camera )
	{
		return ( p_camera->GetPos() - GetObject()->GetPos() ).LengthSqr();
	}

	printf( "No active camera!\n" );

	// no camera?!?
	return 0.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/



// The following is the SuspendOb manager, which iterates over the suspendob components 
// to suspend them to whatever they are suspended to

namespace SuspendManager
{
// The purpose of this manager is just to update all the suspended objects, by calling
// CMovingObject::SuspendToObject on all objects that have had Obj_SuspendToObject run on them.
// It is a manager so that it can be given a task with a priority such that it is called
// after all the objects are updated.
// The reason the suspend logic cannot be done in the object update is because sometimes the parent's 
// update might happen after the child's, causing a frame lag. This was making the gorilla appear
// behind the tram seat in the zoo when the tram was being skitched. (TT2324)

DefineSingletonClass( Manager, "Suspended object Manager" )

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::Manager()
{
	mp_task = new Tsk::Task< Manager > ( s_code, *this,
										Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_LOCKED_OBJECT_MANAGER_LOGIC );

	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
	mlp_manager->AddLogicTask( *mp_task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::~Manager()
{
	delete mp_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::s_code( const Tsk::Task< Manager >& task )
{

	Mdl::SPreCalculatedObjectUpdateInfo* p_info = Mdl::Skate::Instance()->GetPreCalculatedObjectUpdateInfo();
	
	Obj::CSuspendComponent *p_suspend_component = static_cast<Obj::CSuspendComponent*>( Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( CRC_SUSPEND ));
	while( p_suspend_component )
	{

		// Set camera dist to zero for the purposes of LOD
		p_suspend_component->m_camera_distance_squared = 0;
		p_suspend_component->m_skip_logic = false;
	
		if (p_suspend_component->m_no_suspend_count)
		{
			p_suspend_component->m_no_suspend_count--;
			continue;
		}

		if ( p_info->mDoNotSuspendAnything )
		{
		}	
		else if ( p_suspend_component->m_never_suspend )
		{
		}
		else if ( p_suspend_component->m_initial_animations )
		{
			p_suspend_component->m_initial_animations--;
		}
		else
		{
			p_suspend_component->m_camera_distance_squared = Mth::DistanceSqr(p_suspend_component->GetObject()->GetPos(),p_info->mActiveCameraPosition);
		
			if ( p_suspend_component->m_camera_distance_squared > p_suspend_component->m_suspend_distance_squared )
			{
				p_suspend_component->m_skip_logic = true;
			}	
			else
			{
				p_suspend_component->m_skip_logic = false;
			}
		}
		// suspend or unsuspend the object and its components
		// based on the new state of m_skip_logic
		if (p_suspend_component->m_skip_logic)
		{
			if (!p_suspend_component->GetObject()->IsSuspended())
			{
				p_suspend_component->GetObject()->Suspend(true);
			}
		}
		else
		{
			if (p_suspend_component->GetObject()->IsSuspended())
			{
				p_suspend_component->GetObject()->Suspend(false);
			}
		}
	
		// If it has a model, then check for LOD to nothing	
		if (p_suspend_component->mp_model_component)
		{
			if (p_suspend_component->m_initial_animations)
			{
				if (p_suspend_component->mp_model_component->GetModel())
				{
					p_suspend_component->mp_model_component->GetModel()->SetActive(true);
				}
			}
			else
			{
				p_suspend_component->CheckModelActive();	
			}
		}
		
		p_suspend_component = static_cast<Obj::CSuspendComponent*>( p_suspend_component->GetNextSameType());
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/



}
	
}
