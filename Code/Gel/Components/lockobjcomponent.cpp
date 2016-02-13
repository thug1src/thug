//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       LockObjComponent.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/31/2002
//****************************************************************************

#include <gel/components/lockobjcomponent.h>

#include <core/singleton.h>

#include <gel/object/compositeobject.h>
#include <gel/objman.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/script.h>

#include <gfx/nxmodel.h>
#include <gfx/skeleton.h>
												   
// for kludgy skater stuff
#include <sk/modules/skate/skate.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/skeletoncomponent.h>


// For the manager ... messy
#include <gel/mainloop.h>


namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// This static function is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
CBaseComponent* CLockObjComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CLockObjComponent );	
}


// And this one is the optional "register" function that provides on-time initilization
// usually of a component manager
void	CLockObjComponent::s_register()
{
	// Create and initilize the manager
	LockOb::Manager::Create();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CLockObjComponent::CLockObjComponent() : CBaseComponent()
{
	SetType( CRC_LOCKOBJ );

	m_lock_enabled = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CLockObjComponent::~CLockObjComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CLockObjComponent::InitFromStructure( Script::CStruct* pParams )
{

	m_locked_to_object_name=0;
	m_locked_to_object_id=Obj::CBaseManager::vNO_OBJECT_ID;
	m_locked_to_object_bone_name=0;
	m_lock_offset.Set();
	
	if (pParams->ContainsFlag("Off"))
	{
		EnableLock( false );
		return;
	}

	EnableLock( true );

	#if 0
	// SHOULD ALSO CLEAR OUT SOME OF THE OTHER
	// FLAGS THAT CANNOT CO-EXIST!
	GetObject()->m_movingobj_status &= ~MOVINGOBJ_STATUS_FOLLOWING_LEADER;
	GetObject()->m_movingobj_status &= ~MOVINGOBJ_STATUS_MOVETO;
	#endif

	pParams->GetChecksum("ObjectName",&m_locked_to_object_name);
	pParams->GetVector(NONAME,&m_lock_offset);
	pParams->GetChecksum("id",&m_locked_to_object_id);
	pParams->GetChecksum("bone",&m_locked_to_object_bone_name);

	printf( "locking to object %s\n", Script::FindChecksumName(m_locked_to_object_id) );

	m_no_rotation=pParams->ContainsFlag("NoRotation");
	
	m_lag=false;
	
	m_lag_factor_range_a=1.0f;
	m_lag_factor_range_b=1.0f;
	if (pParams->GetPair("LagRange",&m_lag_factor_range_a,&m_lag_factor_range_b))
	{
		m_lag=true;
	}	
	
	m_lag_freq_a=0.0f;
	m_lag_freq_b=0.0f;
	if (pParams->GetPair("LagWobble",&m_lag_freq_a,&m_lag_freq_b))
	{
		m_lag=true;
	}	

	// The random lag phase is to ensure that two objects using the same script won't
	// wobble in phase.
	m_lag_phase=Mth::Rnd(1024);	
	uint32 t=Tmr::ElapsedTime(0)+m_lag_phase;
	t&=1023;
	float x=sinf(m_lag_freq_a*t*2.0f*3.1415926f/1024.0f) *
			sinf(m_lag_freq_b*t*2.0f*3.1415926f/1024.0f);
	
	m_lag_factor=m_lag_factor_range_a+(x+1)*(m_lag_factor_range_b-m_lag_factor_range_a)/2.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CLockObjComponent::Update()
{
	// Doing nothing, so tell code to do nothing next time around
	Suspend(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent::EMemberFunctionResult CLockObjComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{

		
	switch ( Checksum )
	{
        // @script | Obj_LockToObject | Makes the object be locked a fixed offset from some other
		// object, and use the same orientation.
        // @uparmopt tev_TramA01 | Name of the object to be locked to
        // @uparmopt (0,0,0) | Offset from the host object.
		// @flag Off | Switch off lock so that object can be moved independently again.
		// @flag NoRotation | Makes the objects rotation be left unaffected.
		// @parmopt pair | LagRange | (1,1) | The range of the lag value, which determines how quickly
		// the object will catch up to the object being followed. The lower the value the further
		// the object will lag behind. 1 means no lag at all.
		// Example which works quite well: (0.05,0.07)
		// @parmopt pair | LagWobble | (0,0) | The frequencies used to wobble the lag value between
		// the two ranges. Two frequencies are specified so that the lag will wobble in an
		// irregular manner. Example which works quite well: (0.779,0.65)
		case 0x785e57c7: // Obj_LockToObject
		{
			#if 0
			#ifdef	__NOPT_ASSERT__
			uint32 foo=0;
			if (!pParams->GetChecksum("ObjectName",&foo))
			{
				Dbg_MsgAssert(0,("\n%s\nMissing ObjectName parameter in Obj_LockToObject",pScript->GetScriptInfo()));
			}
			#endif				
			#endif				
			InitFromStructure( pParams );							
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

void CLockObjComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CLockObjComponent::GetDebugInfo"));
	// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
	
	p_info->AddChecksum("m_locked_to_object_name",m_locked_to_object_name);
	p_info->AddChecksum("m_locked_to_object_id",m_locked_to_object_id);
	p_info->AddChecksum("m_locked_to_object_bone_name",m_locked_to_object_bone_name);
	p_info->AddVector("m_lock_offset",m_lock_offset.GetX(),m_lock_offset.GetY(),m_lock_offset.GetZ());
	p_info->AddFloat("m_lag_factor",m_lag_factor);
	p_info->AddFloat("m_lag_factor_range_a",m_lag_factor_range_a);
	p_info->AddFloat("m_lag_factor_range_b",m_lag_factor_range_b);
	p_info->AddFloat("m_lag_freq_a",m_lag_freq_a);
	p_info->AddFloat("m_lag_freq_b",m_lag_freq_b);
	p_info->AddFloat("m_lag_phase",m_lag_phase);
	p_info->AddChecksum("m_lag",m_lag ? 0x203b372/*True*/:0xd43297cf/*False*/);
	p_info->AddChecksum("m_no_rotation",m_no_rotation ? 0x203b372/*True*/:0xd43297cf/*False*/);
	p_info->AddChecksum("m_lock_enabled",m_lock_enabled ? 0x203b372/*True*/:0xd43297cf/*False*/);
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCompositeObject* CLockObjComponent::get_locked_to_object()
{
	CCompositeObject* p_obj=NULL;
	if (m_locked_to_object_id!=CBaseManager::vNO_OBJECT_ID)
	{
		p_obj=(CCompositeObject*)Obj::ResolveToObject(m_locked_to_object_id);
	}
	else if (m_locked_to_object_name==0x5b8ab877) // Skater
	{
		// GJ:  phase this usage out...  we shouldn't have anything
		// skater-specific in here!
		Dbg_MsgAssert(Mdl::Skate::Instance()->GetNumSkaters()==1,("Called get_locked_to_object with skater as parent when there is more than one skater"));
		p_obj=(CCompositeObject*)Mdl::Skate::Instance()->GetLocalSkater();
	}
	else
	{
		//p_obj=CMovingObject::m_hash_table.GetItem(m_locked_to_object_name);
		p_obj = (Obj::CCompositeObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID( m_locked_to_object_name );
	
	}
	
	return p_obj;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLockObjComponent::LockToObject( void )
{
	Mth::Matrix old_matrix=GetObject()->m_matrix;
	
	CCompositeObject* p_obj=get_locked_to_object();
	if (!p_obj)
	{
		// Can't find the object, oh well.
		return;
	}	

	Mth::Vector target_pos = GetObject()->m_pos;
	
	if ( m_locked_to_object_bone_name )
	{
		Gfx::CSkeleton* pSkeleton = NULL;
		if ( p_obj->GetComponent(CRC_SKELETON) )
		{
			pSkeleton = GetSkeletonComponentFromObject(p_obj)->GetSkeleton();
		}
		Dbg_MsgAssert( pSkeleton, ( "Locking object to bone on non-skeletal object" ) );
		if ( pSkeleton && pSkeleton->GetBoneMatrix( m_locked_to_object_bone_name, &GetObject()->m_matrix ) )
		{
			Mth::Vector boneOffset = GetObject()->m_matrix[Mth::POS];
			GetObject()->m_matrix[Mth::POS] = Mth::Vector( 0.0f, 0.0f, 0.0f, 1.0f );

		    GetObject()->m_matrix = GetObject()->GetMatrix() * p_obj->GetMatrix();

			target_pos = p_obj->GetDisplayMatrix().Transform( boneOffset );
			target_pos += p_obj->m_pos+m_lock_offset*GetObject()->GetMatrix();
		}
	}
	else
	{
		// Using the display matrix otherwise the object will jitter if following the skater
		// up a half pipe.
		
		// K: If the parent object is doing some extra rotations (by using the RotateDisplay command),
		// the calculation of target pos is slightly different. 
		// However, as a speed optimization only do this for the skater because currently that 
		// is the only object that does extra rotations.
		// Fixes bug where the pizza box special item was not being displayed correctly when used in
		// a create-a-trick with rotations.
		
		bool calculate_extra_rotation = false;

		CCompositeObject* p_composite_object = (CCompositeObject*)p_obj;
		Obj::CModelComponent* p_model_component = (Obj::CModelComponent*)p_composite_object->GetComponent( CRC_MODEL );
		Dbg_Assert( p_model_component );
		if ( p_model_component->HasRefObject() )
		{
			// GJ THPS5:  We're now starting to have non-skater objects needing the extra rotate 
			// display (i.e. the CAT preview skater)...  in theory, we could still do an optimization
			// where each lockobjcomponent is flagged to support the rotatedisplay, but i decided
			// to just kludge it for now by checking for a refObject.
			calculate_extra_rotation = true;
		}
		else if ( p_obj->GetID() < Mdl::Skate::vMAX_SKATERS )
		{
			// If it's the skater ...
			calculate_extra_rotation = true;
		}

		if ( calculate_extra_rotation )
		{
			Mth::Matrix display_matrix;
			p_model_component->GetDisplayMatrixWithExtraRotation( display_matrix );
			
			// Note that p_obj->GetPos() is not added because the offset is contained in the matrix.
			target_pos=m_lock_offset*display_matrix;
			
			// but now zero out the translation part of the matrix otherwise the object will be rendered wrong.
			Mth::Vector zero(0.0f,0.0f,0.0f,1.0f);
			display_matrix.SetPos(zero);
			GetObject()->m_matrix=display_matrix;
		}
		else
		{
			GetObject()->m_matrix=((CCompositeObject*)p_obj)->GetDisplayMatrix();
			target_pos=p_obj->GetPos()+m_lock_offset*GetObject()->m_matrix;
		}
	}

	if (m_lag)
	{
		GetObject()->m_pos=GetObject()->GetPos()+(target_pos-GetObject()->GetPos())*m_lag_factor;
		
		uint32 t=Tmr::ElapsedTime(0)+m_lag_phase;
		t&=1023;
		float x=sinf(m_lag_freq_a*t*2.0f*Mth::PI/1024.0f) *
				sinf(m_lag_freq_b*t*2.0f*Mth::PI/1024.0f);
		
		m_lag_factor=m_lag_factor_range_a+(x+1)*(m_lag_factor_range_b-m_lag_factor_range_a)/2.0f;
	}
	else
	{
		GetObject()->m_pos=target_pos;
	}
	
	// take the object's velocity as well
	GetObject()->SetVel(p_obj->GetVel());
	
	// If the objects rotation is required to be preserved, recover the old value.
	// Alan's ferris wheel cars use this.
	if (m_no_rotation)
	{
		GetObject()->m_matrix[Mth::UP]=old_matrix[Mth::UP];
		GetObject()->m_matrix[Mth::AT]=old_matrix[Mth::AT];
		GetObject()->m_matrix[Mth::RIGHT]=old_matrix[Mth::RIGHT];
		//printf("m_pos=(%f,%f,%f)\n",m_pos[X],m_pos[Y],m_pos[Z]);
	}	
	
	// GJ:  This function is called from a task that
	// happens after all the matrices have already
	// been sent to the rendering code for this frame...
	// so we'll need to re-apply any new m_pos/m_matrix
	// changes to the rendered model
	// (suggestion for THPS5...  have a render task
	// that sends all the matrices to the model after
	// the object update task has finished...)
	Nx::CModel* pModel = NULL;
	if ( GetObject()->GetComponent( CRC_MODEL ) )
	{
		pModel = ((Obj::CModelComponent*)GetObject()->GetComponent(CRC_MODEL))->GetModel();
	}
	
	if ( pModel )
	{
		Gfx::CSkeleton* pSkeleton = NULL;
		if ( GetObject()->GetComponent( CRC_SKELETON ) )
		{
			pSkeleton = ((Obj::CSkeletonComponent*)GetObject()->GetComponent(CRC_SKELETON))->GetSkeleton();
		}

		Mth::Matrix rootMatrix;
		rootMatrix = GetObject()->GetDisplayMatrix();
		rootMatrix[Mth::POS] = GetObject()->GetPos();
		rootMatrix[Mth::POS][W] = 1.0f;
		bool should_animate = false;
		pModel->Render( &rootMatrix, !should_animate, pSkeleton );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/




	
}



// The following is the LockOb manager, which iterates over the lockob components 
// to lock them to whatever they are locked to

namespace LockOb
{
// The purpose of this manager is just to update all the locked objects, by calling
// CMovingObject::LockToObject on all objects that have had Obj_LockToObject run on them.
// It is a manager so that it can be given a task with a priority such that it is called
// after all the objects are updated.
// The reason the lock logic cannot be done in the object update is because sometimes the parent's 
// update might happen after the child's, causing a frame lag. This was making the gorilla appear
// behind the tram seat in the zoo when the tram was being skitched. (TT2324)

DefineSingletonClass( Manager, "Locked object Manager" )

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
	
//	if (Mdl::FrontEnd::Instance()->GamePaused())
//	{
//		// Must not update if game paused, because the game could be running a replay,
//		// which would mean the objects' models will have been deleted, causing the 
//		// LockToObject function to assert.
//		return;
//	}

	// new fast way, just go directly to the components, if any
	Obj::CLockObjComponent *p_lock_obj_component = static_cast<Obj::CLockObjComponent*>( Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( CRC_LOCKOBJ ));
	while( p_lock_obj_component )
	{
		if ( p_lock_obj_component->IsLockEnabled() )
		{
			p_lock_obj_component->LockToObject();
		}	
		p_lock_obj_component = static_cast<Obj::CLockObjComponent*>( p_lock_obj_component->GetNextSameType());
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}

