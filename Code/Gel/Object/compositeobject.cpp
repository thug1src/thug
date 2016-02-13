//****************************************************************************
//* MODULE:         Gel/Object
//* FILENAME:       compositeobject.cpp
//* OWNER:          Mick West
//* CREATION DATE:  10/17/2002
//****************************************************************************

/***************************************************************************
 A composite object is a generic object which basically consists of:
 - a position and orientation
 - some flags
 - a script
 - a list of components
 
 The purpose of the Composite Object is to represent a thing that exists 
 in the game world.  For example:
 
  - Cars and other vechicals
  - Pedestrians, animals, etc
  - Pickup Icons
  - 3D sound emitters
  - Particle systems
  - Skaters or other player type objects
  - Bouncy Objects like trash cans

 CCompositeObject is derived from CObject, so all CCompositeObjects have
 a mp_script (which might be NULL)
 
 A composite object is normally comprised of several components.  These are 
 generally independent things such as:
  - A Model	componet
  - A physics component (e.g., drive around, or skating)
  - A suspend component
  - and various other components as needed
  
 There is one global list of all composite objects.  This is managed by the 
 CCompositeObjectManager class, whcih is also responsible for creating
 the composite objects.
 
 A composite object is typically create from an array of structs
 and a structe that contains the initialization parameters.
 
 Here's an example of the array of structs.  Note that some components
 have default parameters.  These can be overridden by the struct of parameters
 
 gameobj_composite_structure = [
    { component = suspend }
    { component = model  }
    { component = exception }
    { component = collision }
 ]

****************************************************************************/

#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectManager.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/checksum.h>
#include <modules/skate/skate.h>
#include <sk/scripting/nodearray.h> // Needed by GetDebugInfo
#include <sk/engine/feeler.h>


namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCompositeObject::CCompositeObject()
{
    // set position and orientation to a known state (at the origin, identity matrix)	
	m_vel.Set( 0.0f, 0.0f, 0.0f );
	m_pos.Set( 0.0f, 0.0f, 0.0f );
	m_matrix.Ident();
	m_display_matrix.Ident();

	mp_component_list = NULL;
	m_composite_object_flags.ClearAll();

	SetFlags( GetFlags() | vCOMPOSITE);   // Kind of a temp solution for now

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCompositeObject::~CCompositeObject()
{   
	while ( mp_component_list )
    {
		// Get the component at the head of the list		
		CBaseComponent* pComponent = mp_component_list;
		
		// advance the list past this component, effectivly isolating it
		// (unless someone else is storing a pointer to it)
		mp_component_list =  pComponent->mp_next;
		
		// remove the component from the by-type list of components maintained by the CompositeObjectManager
		Obj::CCompositeObjectManager::Instance()->RemoveComponentByType( pComponent );

		// delete the isolated component
		delete pComponent;   
		     
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
#ifdef __NOPT_ASSERT__
// Runs through all the components and zeroes their update times.
void CCompositeObject::zero_component_update_times()
{
	CBaseComponent* pComponent = mp_component_list;
    while ( pComponent )
    {
		pComponent->m_update_time=0;
        pComponent = pComponent->mp_next;
    }
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompositeObject::Update()
{
#ifdef __USE_PROFILER__
	Sys::CPUProfiler->PushContext( m_profile_color );
#endif // __USE_PROFILER__

	Dbg_MsgAssert(IsFinalized(),("Update called on UnFinalized Composite object %s",Script::FindChecksumName(GetID())));

#ifdef	__NOPT_ASSERT__
	// GJ:  don't do this if the composite object is paused,
	// because some lock obj component might be trying
	// to lock to this object while the game is paused
	if (!m_composite_object_flags.Test(CO_SUSPENDED)
		&& !m_composite_object_flags.Test(CO_PAUSED))
	{
		// clear out the display matrix, to make 
		// sure that no one's relying on the
		// last frame's matrix                                                                                               
		m_display_matrix.Ident();
	}
#endif
	
#ifdef __NOPT_ASSERT__
	// Make sure the component update times are initialized to zero so that they do not
	// show incorrect old values in the debugger if this function returns before
	// getting to the component update loop.
	zero_component_update_times();
	
	m_total_script_update_time=0;
	m_do_game_logic_time=0;
#endif

	if (m_composite_object_flags.Test(CO_PAUSED))
	{
#ifdef __USE_PROFILER__
	Sys::CPUProfiler->PopContext();
#endif // __USE_PROFILER__
		return;
	}


	if (!m_composite_object_flags.Test(CO_SUSPENDED))
	{
#ifdef __NOPT_ASSERT__
		Tmr::CPUCycles time_before=Tmr::GetTimeInCPUCycles();
#endif
		if ( mp_script )
		{
			if ( mp_script->Update() == Script::ESCRIPTRETURNVAL_FINISHED )
			{
				// if we have script based events then we only want to kill the script if
				// it has an empty event handler table
				// as a script can finish, but still have event handlers
				// this kind of logic will be duplicated in various places in the code
				// so we could do with a bit of refactoring
				// like a mp_script->CanBeDeleted() function
				#ifdef	__SCRIPT_EVENT_TABLE__	 
					// in practice it seems okay to just leave the script
					// probably not a big issue   
				#else
					delete mp_script;
					mp_script = NULL;
				#endif
			}
		}
		
		// if a script has called "Die",
		// then don't update the components
		if ( IsDead() )
		{
			#	ifdef __USE_PROFILER__
				Sys::CPUProfiler->PopContext();
			#	endif // __USE_PROFILER__
			return;
		}
#ifdef __NOPT_ASSERT__
		// Convert to microseconds by dividing by 150
		m_total_script_update_time=(Tmr::GetTimeInCPUCycles()-time_before)/150;
#endif

		// transition-only function call,
		// call each specific object's
		// DoGameLogic() function (previously,
		// this was called by each object's task)
#ifdef __NOPT_ASSERT__
		time_before=Tmr::GetTimeInCPUCycles();
#endif
		// Mick:  DoGameLogic is Deprecated, and only exists for a few misc objects
		// it should eventually be removed
		DoGameLogic();
#ifdef __NOPT_ASSERT__
		// Convert to microseconds by dividing by 150
		m_do_game_logic_time=(Tmr::GetTimeInCPUCycles()-time_before)/150;
#endif

	}

	CBaseComponent* pComponent = mp_component_list;
    while ( pComponent )
    {
		if ( ! (pComponent->m_flags.Test(CBaseComponent::BC_NO_UPDATE)))
		{
			#ifdef __NOPT_ASSERT__
			Tmr::CPUCycles time_before_components=Tmr::GetTimeInCPUCycles();
			#endif
			
			pComponent->Update();
			
			#ifdef __NOPT_ASSERT__
			// Convert to microseconds by dividing by 150
			pComponent->m_update_time=(Tmr::GetTimeInCPUCycles()-time_before_components)/150;
			#endif

			// If a component update has killed the object
			// then we don't process any more components
			// as they might attempt to fire an event, or reference this object in some way
			// and it won't be in the tracking system any more
			if (IsDead())
			{
				break;
			}
			
			
		}
        pComponent = pComponent->mp_next;
    }
	#	ifdef __USE_PROFILER__
		Sys::CPUProfiler->PopContext();
	#	endif // __USE_PROFILER__
	
	m_composite_object_flags.Clear(CO_TELEPORTED);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCompositeObject::Hide( bool should_hide )
{
	if ( should_hide == m_composite_object_flags.Test(CO_HIDDEN) )
	{
		return;
	}

	m_composite_object_flags.Set(CO_HIDDEN, should_hide);
		
	// loop through all the components
	// and call their individual Hide functions...
	CBaseComponent* pComponent = mp_component_list;
	while ( pComponent )
	{
		pComponent->Hide( should_hide );
		pComponent = pComponent->mp_next;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Suspending a composite object tells it not to update the script
// but the individual components take care of suspending themselves
// (Generally handled by the BC_NO_UPDATE flag being set)	
// "suspending" is generally soemthing done when the object goes a
// certain distance from the camera
void CCompositeObject::Suspend(bool suspended)
{
    if (suspended == m_composite_object_flags.Test(CO_SUSPENDED)) return;
	
	m_composite_object_flags.Set(CO_SUSPENDED, suspended);
	
	// suspend the individual components
	CBaseComponent* pComponent = mp_component_list;
	while ( pComponent )
	{
		pComponent->Suspend(suspended);
		pComponent = pComponent->mp_next;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompositeObject::Teleport()
{
	// calls each component's Teleport() function so
	// that things like the model, the collision, and the
	// shadow can be updated immediately when the
	// object has radically changed position
	CBaseComponent* pComponent = mp_component_list;
	while ( pComponent )
	{
		pComponent->Teleport();
		pComponent = pComponent->mp_next;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompositeObject::AddComponent( CBaseComponent* pComponent )
{
#ifdef __NOPT_ASSERT__
	if ( GetComponent( pComponent->GetType() ) )
	{
		Dbg_MsgAssert( 0, ( "Object already has a component of type '%s'", Script::FindChecksumName(pComponent->GetType()) ) );
	}
#endif

	Dbg_MsgAssert(!IsFinalized(),("Adding Component %s to Finalized Composite object %s",Script::FindChecksumName(pComponent->GetType()),Script::FindChecksumName(GetID())));

	if (!mp_component_list)
	{
		mp_component_list = pComponent;
	}
	else
	{
		CBaseComponent* p_tail = mp_component_list;
		while(p_tail->mp_next)
		{
			p_tail = p_tail->mp_next;
		}
		p_tail->mp_next = pComponent;
	}

    // now that the component is "officially" associated with
    // this object, we can set the component's object ptr
    pComponent->SetObject( this );

	// add the component to the by-type list of components maintained by the CompositeObjectManager
	Obj::CCompositeObjectManager::Instance()->AddComponentByType( pComponent );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given a single component definition in the form:
// {
// 		component = ...
//      .... other params
// }
//
// Then create a component of the correct type
// add it to this object
// and initialize it with this structure
//
// Optionally supply a seperate additional p_params structure from which to initialize it

void CCompositeObject::CreateComponentFromStructure(Script::CStruct *p_struct, Script::CStruct *p_params)
{
	uint32	component_name;
	p_struct->GetChecksum("component",&component_name,Script::ASSERT);
	Obj::CBaseComponent* p_component = Obj::CCompositeObjectManager::Instance()->CreateComponent(component_name);
	AddComponent(p_component);	 // Add it first, as InitFromStructure might need to query the object
	if (p_params)
	{
		// If we have additional parameters, then add then in to the parameters in the array
		// this will override the array paramaeters, which are assumed to be defaults
		Script::CScriptStructure *p_combinedParams=new Script::CScriptStructure;
		*p_combinedParams += *p_struct;
		*p_combinedParams += *p_params;
		p_component->InitFromStructure(p_combinedParams);
		delete p_combinedParams;
	}
	else
	{
		// No additional parameters, so just initialze from the params in the structure in the array
		p_component->InitFromStructure(p_struct);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given an array of component definitions in the form:
// [
//   { component = ......
//     ... other params .....
//   }
//   {
//    ... other components
//   }
// ]
// then iterate over the array, and add the components to the composite object
//
// Optionally supply a "p_params" structure that contains all the initialization info
// instead of interleaving it in the array 
													  
void CCompositeObject::CreateComponentsFromArray(Script::CArray *p_array, Script::CStruct* p_params)
{
	int num_components = p_array->GetSize();
	for (int i=0;i<num_components;i++)
	{
		Script::CStruct* p_struct = p_array->GetStructure(i);
        CreateComponentFromStructure(p_struct, p_params);
	}
}


// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently isfrequently the contents of a node
// but you can pass in anything you like.	
void CCompositeObject::InitFromStructure( Script::CStruct* pParams )
{
	uint32 NameChecksum;
	if ( pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &NameChecksum ))
	{
		SetID(NameChecksum);
	}
	
	SkateScript::GetPosition(pParams,&m_pos);
	
	SkateScript::GetOrientation(pParams,&m_matrix);
	
#	ifdef __USE_PROFILER__
	int ProfileColor = 0x800000;			// default to blue profile color		
	pParams->GetInteger( CRCD(0x72444899,"ProfileColor"), &ProfileColor );
	SetProfileColor(ProfileColor);
#	endif // __USE_PROFILER__

	if (pParams->ContainsFlag(CRCD(0x23627fd7,"permanent")))
	{
		SetFlags( GetFlags() | vLOCKED);
	}
}


// Finalize is called after all components have been added
// and will call the virtual Finalize() function on each component
// This is intended for any components that are depended on other components
// but where the initialization order can't be guaranteed
void CCompositeObject::Finalize()
{
	Dbg_MsgAssert(!IsFinalized(),("Finalizing Composite object %s twice",Script::FindChecksumName(GetID())));
	CBaseComponent *p_component = mp_component_list;
	while (p_component)
	{
		p_component->Finalize();
		p_component = p_component->GetNext();
	}
	m_composite_object_flags.Set(CO_FINALIZED);

	// now that the component is finalized,
	// update the components that depend
	// on the position of the object
	Teleport();
}


// RefreshFromStructure is passed the same parameters as the above
// but will use them to update 
void CCompositeObject::RefreshFromStructure( Script::CStruct* pParams )
{
	// Make sure they are not trying to change the id to something else
	#ifdef	__NOPT_ASSERT__
	uint32 NameChecksum;
	if ( pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &NameChecksum ))
	{
		Dbg_MsgAssert(GetID() == NameChecksum,("Attempting to refresh id from %s to %s\n", Script::FindChecksumName(GetID()),NameChecksum));
	}
	#endif

	// Update the position	
	// Note this is just cut and paste from above
	// so if we're going to be doing more initting of the composite object
	// from a script struct, then we might want to factor it out
	SkateScript::GetPosition(pParams,&m_pos);
	
	// Now iterate over the components, and Refresh them
	CBaseComponent* pComponent = mp_component_list;
    while ( pComponent )
    {
		pComponent->RefreshFromStructure(pParams);
        pComponent = pComponent->mp_next;
    }


}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CCompositeObject::GetComponent( uint32 id ) const
{
	CBaseComponent *p_component = mp_component_list;
	while (p_component)
	{
		if (p_component->GetType() == id)
		{	
			return p_component;
		}
		p_component = p_component->GetNext();
	}
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool 	CCompositeObject::CallMemberFunction( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript )
{


	// This should probably go in some debug component?
	switch ( Checksum )
	{
		// @script | Obj_PrintDetails | This is a debug function.  Add this
        // command to an object's script
		// and it will print the details.  Add anything you want to the Dbg_Message( )
		// below.
        // @uparmopt "string" | text to print
		case 0xc2404947: // Obj_PrintDetails
		{
			const char* pText;
			if ( pParams->GetText( NONAME, &pText ) )
			{
				Dbg_Message( "%s", pText );
			}
			Dbg_Message( "Obj details:\n  pos %f %f %f\n  time %d\n  type %d",
				m_pos[ X ], m_pos[ Y ], m_pos[ Z ], Tmr::GetTime(), m_type );
			return true;
		}
		break;

		// @script | CreateComponentFromStructure |
		case 0x406998a5: // CreateComponentFromStructure
		{
			CreateComponentFromStructure( pParams, NULL );
			return true;
		}
		break;

		// @script | Obj_GetPosition |
		case 0xe90aad2d: // Obj_GetPosition
		{									 
			pScript->GetParams()->AddVector( CRCD(0x7f261953,"pos"), m_pos[X], m_pos[Y], m_pos[Z] );
			return true;
		}
		break;

		// @script | Obj_SetPosition | Sets the world position of the object.
		// @parmopt vector | Position | | Position to give to the object.
		case 0xf7251a64: // Obj_SetPosition
		{									 
			pParams->GetVector(CRCD(0xb9d31b0a,"Position"),&m_pos);
			return true;
		}
		break;
		
		// @script | Obj_SetOrientation |
		// @parmopt float | x | 0.0 | The x angle, in degrees
		// @parmopt float | y | 0.0 | The y angle, in degrees
		// @parmopt float | z | 0.0 | The z angle, in degrees
		// @parmopt vector | dir | (0,0,1) | Direction vector (an alternative to specifying angles)
		// The direction vector does not need to be normalized.
		case 0xc6f3baa5: // Obj_SetOrientation
		{
			Mth::Vector dir(0.0f,0.0f,1.0f);
			if (pParams->GetVector(CRCD(0x455485ef,"dir"),&dir))
			{
				Mth::Matrix mat;
				mat.Ident();
				
				mat[Z]=dir;
				mat[Z].Normalize();
				mat[X] = Mth::CrossProduct(mat[Y], mat[Z]);
				mat[X].Normalize();
				mat[Y] = Mth::CrossProduct(mat[Z], mat[X]);
				mat[Y].Normalize();
				
				SetMatrix(mat);
				SetDisplayMatrix(mat);
			}
			else
			{
				float p=0.0f;
				float h=0.0f;
				float r=0.0f;
				pParams->GetFloat(CRCD(0x7323e97c,"x"),&p);
				pParams->GetFloat(CRCD(0x424d9ea,"y"),&h);
				pParams->GetFloat(CRCD(0x9d2d8850,"z"),&r);
				p*=3.141592654f/180.0f;
				h*=3.141592654f/180.0f;
				r*=3.141592654f/180.0f;
				Mth::Matrix mat(p,h,r);
				SetMatrix(mat);
				SetDisplayMatrix(mat);
			}	
			return true;
		}	
		break;

        // @script | Obj_ForceUpdate | Does a single call to the object's Update function
		case 0xc1bff0f3: // Obj_ForceUpdate
		{
			Update();
			return true;
		}	
		break;
		
		// @script | Obj_GetVelocity |
		case 0x11fe9f71: // Obj_GetVelocity
		{								   
			pScript->GetParams()->AddVector( CRCD(0x0c4c809e,"vel"), m_vel[X], m_vel[Y], m_vel[Z] );
			return true;
		}
		break;
		
		// @script | GetSpeed |
		case 0xc0caac4a: // GetSpeed
		{								   
			pScript->GetParams()->AddFloat( CRCD(0xf0d90109,"speed"), m_vel.Length() );
			return true;
		}
		break;
		
		// @script | Hide | hides object
		case 0x5b6634d4: // Hide
		{
			Hide( true );
			return true;
		}
		break;

		// @script | Unhide | unhides object
		case 0xb60d1f35: // Unhide
		{
			Hide( false );
			return true;
		}
		break;

		// @script | IsHidden | checks the hide flag of object
		case 0xb16619ae: // IsHidden
		{
			return m_composite_object_flags.Test(CO_HIDDEN);
		}
		break;
		
        // @script | Move |
        // @parmopt float | x | 0.0 | x component
        // @parmopt float | y | 0.0 | y component
        // @parmopt float | z | 0.0 | z component
		case 0x10c1c887: // Move	
		{
			float distance = 0.0f;
			pParams->GetFloat(CRCD(0x7323e97c, "x"), &distance);
			m_pos += distance * m_matrix[X];
			distance = 0.0f;
			pParams->GetFloat(CRCD(0x424d9ea, "y"), &distance);
			m_pos += distance * m_matrix[Y];
			distance = 0.0f;
			pParams->GetFloat(CRCD(0x9d2d8850, "z"), &distance);
			m_pos += distance * m_matrix[Z];
			return true;
		}
		
		case 0xce0ca665: // Suspend
			Suspend(true);
			return true;
			break;

		case 0xca3c59a6: // Unsuspend
			Suspend(false);
			return true;
			break;
			
		case 0x28656d12: // Pause
			Pause(true);
			return true;
			break;

		case 0xff85d4d7: // Unpause
			Pause(false);
			return true;
			break;

// MOVED FROM CMOVINGOBJECT.CPP		
		
		// @script | Obj_GetCollision | Checks to see if there is a collision along a line in front
		// of the object.
        // @parmopt float | Height | 3.0 | Height above origin of start point in feet
        // @parmopt float | Lnegth | 3.0 | length of ocllision line in feet
        // @flag side | Check side collision instead of forward collision
        // @flag debug | display green debug lines at each collision test, white if there is a collision
		case 0x168b09c:	// Obj_GetCollision
			{
				CFeeler	feeler;
				//if (pParams->ContainsFlag(0xc4e78e22/*All*/))
				
				float	length = 3.0f;
				pParams->GetFloat(0xfe82614d /*length*/,&length);
				length *= 12.0f;
				float	height = 3.0f;
				pParams->GetFloat(0xab21af0 /*height*/,&height);
				height *= 12.0f;
				
				feeler.m_start = m_pos + m_matrix[Y] * height;;
				feeler.m_end = feeler.m_start;
				if (pParams->ContainsFlag(0xdc7ee44a/*side*/))
				{
					feeler.m_end += length * m_matrix[X];
				}
				else
				{
					feeler.m_end += length * m_matrix[Z];
				}
				
				if (feeler.GetCollision())
				{
					#ifdef	__NOPT_ASSERT__
					if (pParams->ContainsFlag(0x935ab858/*debug*/))
					{
						feeler.DebugLine(255,255,255);	 // White line = collision
					}
					#endif
					return true;
				}
				else
				{
					#ifdef	__NOPT_ASSERT__
					if (pParams->ContainsFlag(0x935ab858/*debug*/))
					{
						feeler.DebugLine(0,255,0);	 // green line = no collision
					}
					#endif
					return false;
				}
				
			}
		
        // @script | Obj_GetOrientation | Gets the X,Y,Z vector representing orientation 
		case 0xb9ed09d2: // Obj_GetOrientation
		{
			Mth::Vector atVector;
			atVector = m_display_matrix[Mth::AT];
			atVector.Normalize();

			pScript->GetParams()->AddFloat( "x", atVector[X] );
			pScript->GetParams()->AddFloat( "y", atVector[Y] );
			pScript->GetParams()->AddFloat( "z", atVector[Z] );
			return true;
		}
		break;
		
		case 0xd96c0a20: // Obj_GetDistToNode
		{
			uint32 node_name=0;
			pParams->GetChecksum(NONAME,&node_name);
			int node = SkateScript::FindNamedNode(node_name);
			Mth::Vector node_pos;
			SkateScript::GetPosition(node, &node_pos);
			pScript->GetParams()->AddFloat("Dist",(m_pos-node_pos).Length());
			return true;
		}
		
        // @script | Obj_GetDistanceToObject | Calculates the distance to the specified object,
		// and puts the result, in units of feet, into ObjectDistance
        // @uparmopt name | Name of the object. May be Skater.
		case 0x4af57bbb: // Obj_GetDistanceToObject	
		{
			uint32 object_name=0;
			pParams->GetChecksum(NONAME,&object_name);
			CCompositeObject* p_obj=(CCompositeObject*)Obj::ResolveToObject(object_name);
			
			float dist=0.0f;
			if (p_obj)
			{
				Mth::Vector d = m_pos - p_obj->GetPos();
				dist=d.Length()/12.0f;
			}
			pScript->GetParams()->AddFloat("ObjectDistance",dist);
			return true;
		}

		// @script | Obj_GetOrientationToObject |
        // @uparm name | Name of the object. May be Skater.
		case 0x2a26ffa2: // Obj_GetOrientationToObject	
		{
			uint32 object_name=0;
			pParams->GetChecksum(NONAME,&object_name,Script::ASSERT);
			CCompositeObject* p_obj=(CCompositeObject*)Obj::ResolveToObject(object_name);
			Dbg_MsgAssert( p_obj, ( "Couldn't find object %s to get orientation", Script::FindChecksumName(object_name) ) );
			
    		float dotProd=0.0f;
			float orientation=0.0f;

			if ( p_obj )
			{
				// first get the object's current right vector
				Mth::Vector atVector;
				atVector = m_matrix[Mth::RIGHT];
				atVector[W] = 0.0f;
				atVector.Normalize();

				// now take the vector to the object
				Mth::Vector	toObjVector;
				toObjVector = p_obj->m_pos - m_pos;
				toObjVector[W] = 0.0f;
				toObjVector.Normalize();
                
				// dot product tells us whether the right
				// vector is on the left or the right hand
				// side of the toObject vector
				dotProd = Mth::DotProduct( atVector, toObjVector );

				// subtracting 90 gets us whether the at vector is on
				// the left or the right...
				orientation = Mth::RadToDeg( Mth::Kenacosf( dotProd ) ) - 90.0f;	// add 90 to get the at vector
			}

			pScript->GetParams()->AddFloat(CRCD(0x7c6a7d7c,"DotProd"),dotProd);
			pScript->GetParams()->AddFloat(CRCD(0xc97f3aa9,"Orientation"),orientation);
			return true;
		}

        // @script | Backwards | Returns true if the skater is
        // facing backwards from his direction of travel.
		case CRCC(0xf8cfd515, "Backwards"):
			return Mth::DotProduct(m_vel, m_matrix[Z]) < 0.0f ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | SpeedEquals | true if skater speed is within 0.1 
        // of the specified speed
        // @uparm 1.0 | speed
		case CRCC(0x7dcc5fb9, "SpeedEquals"):	 
		{
			float TestSpeed = 0;
			pParams->GetFloat(NO_NAME, &TestSpeed);
			return Mth::Abs(m_vel.Length() - TestSpeed) < 0.1f ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE; 
		}
			
        // @script | SpeedGreaterThan | true if the skater speed is 
        // greater than the specified speed
        // @uparm 1.0 | speed
		case CRCC(0xe5df66d7, "SpeedGreaterThan"):
		{
			float TestSpeed = 0;
			pParams->GetFloat(NO_NAME, &TestSpeed);
			return m_vel.Length() > TestSpeed ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE; 
		}

        // @script | SpeedLessThan | true if the skater speed is
        // less than the specified speed
        // @uparm 1.0 | speed
		case CRCC(0xdd468509, "SpeedLessThan"):
		{
			float TestSpeed = 0;
			pParams->GetFloat(NO_NAME, &TestSpeed);
			return m_vel.Length() < TestSpeed ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE; 
		}
	}


	Dbg_MsgAssert(IsFinalized(),("CallMemberFunction %s to UnFinalized Composite object %s",Script::FindChecksumName(Checksum),Script::FindChecksumName(GetID())));
	CBaseComponent *p_component = mp_component_list;
	while (p_component)
	{
		switch (p_component->CallMemberFunction(Checksum, pParams, pScript))
		{
			case CBaseComponent::MF_TRUE:
				return true;
			case CBaseComponent::MF_FALSE:
				return false;
			default:
				break;
		}
		p_component = p_component->GetNext();
	}
	
	return CObject::CallMemberFunction( Checksum, pParams, pScript );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Used by the script debugger code (gel\scripting\debugger.cpp) to fill in a structure
// for transmitting to the monitor.exe utility running on the PC.

// This adds basic info about the CCompositeObject such as the id, position etc.
// It also adds the debug info for each of the components in the list.
// If classes derived from CCompositeObject override this function, then they should call
// CCompositeObject::GetDebugInfo at the end.
// It would not matter too much if they called it at the start instead, that would just mean
// that derived classes info would appear at the end of the structure (after the list of components)
// rather than at the start.
void CCompositeObject::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CCompositeObject::GetDebugInfo"));
	
	int node_index=SkateScript::FindNamedNode(m_id,false); // false means don't assert if not found.
	if (node_index >= 0 && m_id) // The && m_id is cos FindNamedNode erroneously returns 0 in that case.
	{
		Script::CStruct *p_node=SkateScript::GetNode(node_index);
		p_info->AddInteger("NodeIndex",node_index);
		p_info->AddStructure("NodeInfo",p_node);
	}
	else
	{
		p_info->AddChecksum(NONAME,CRCD(0xc39ae7b3,"NoNode"));
	}	
	
	#ifdef __NOPT_ASSERT__
	p_info->AddInteger("CPUTime",m_update_time);
	p_info->AddInteger("m_total_script_update_time",m_total_script_update_time);
	p_info->AddInteger("m_do_game_logic_time",m_do_game_logic_time);
	#endif

	CObject::GetDebugInfo(p_info);
	
	uint32 type_name=0;

	switch (m_type)
	{
		case SKATE_TYPE_UNDEFINED:
			type_name=CRCD(0x4cb79b3b,"SKATE_TYPE_UNDEFINED");
			break;
		case SKATE_TYPE_SKATER: 			
			type_name=CRCD(0xdbb4aeb6,"SKATE_TYPE_SKATER");
			break;
		case SKATE_TYPE_PED:
			type_name=CRCD(0xa88987b7,"SKATE_TYPE_PED");
			break;
		case SKATE_TYPE_CAR:
			type_name=CRCD(0x2651eacb,"SKATE_TYPE_CAR");
			break;
		case SKATE_TYPE_GAME_OBJ:			
			type_name=CRCD(0x84f6eb36,"SKATE_TYPE_GAME_OBJ");
			break;
		case SKATE_TYPE_BOUNCY_OBJ:		
			type_name=CRCD(0x5f4886c1,"SKATE_TYPE_BOUNCY_OBJ");
			break;
		case SKATE_TYPE_CASSETTE:			
			type_name=CRCD(0xd5021817,"SKATE_TYPE_CASSETTE");
			break;
		case SKATE_TYPE_ANIMATING_OBJECT:	
			type_name=CRCD(0x42018493,"SKATE_TYPE_ANIMATING_OBJECT");
			break;
		case SKATE_TYPE_CROWN:	
			type_name=CRCD(0xf4f7488e,"SKATE_TYPE_CROWN");
			break;
		case SKATE_TYPE_PARTICLE:			
			type_name=CRCD(0x8feeb37e,"SKATE_TYPE_PARTICLE");
			break;
		case SKATE_TYPE_REPLAY_DUMMY:		
			type_name=CRCD(0xf4ac5a55,"SKATE_TYPE_REPLAY_DUMMY");
			break;
		case SKATE_TYPE_COMPOSITE:	
			type_name=CRCD(0x8a61a62c,"SKATE_TYPE_COMPOSITE");
			break;
		default:
			type_name=0;
			break;
		
	}
	if (type_name)
	{
		p_info->AddChecksum("m_type",type_name);
	}
	else
	{
		// If the value is missing from the above switch statement then just add it as an integer.
		p_info->AddInteger("m_type",m_type);
	}
	
	p_info->AddChecksum("m_object_flags",m_object_flags);

	// CCompositeObject stuff
	if (m_composite_object_flags.Test(CO_PAUSED))
	{
		p_info->AddChecksum(NONAME,Script::GenerateCRC("Paused"));
	}
	
	p_info->AddVector("m_pos",m_pos.GetX(),m_pos.GetY(),m_pos.GetZ());
	p_info->AddVector("m_vel",m_vel.GetX(),m_vel.GetY(),m_vel.GetZ());
	
	Script::CArray *p_mat=new Script::CArray;
	p_mat->SetSizeAndType(3,ESYMBOLTYPE_VECTOR);
	Script::CVector *p_vec=new Script::CVector;
	p_vec->mX=m_matrix[X][X]; p_vec->mY=m_matrix[X][Y]; p_vec->mZ=m_matrix[X][Z];
	p_mat->SetVector(0,p_vec);
	p_vec=new Script::CVector;
	p_vec->mX=m_matrix[Y][X]; p_vec->mY=m_matrix[Y][Y]; p_vec->mZ=m_matrix[Y][Z];
	p_mat->SetVector(1,p_vec);
	p_vec=new Script::CVector;
	p_vec->mX=m_matrix[Z][X]; p_vec->mY=m_matrix[Z][Y]; p_vec->mZ=m_matrix[Z][Z];
	p_mat->SetVector(2,p_vec);
	p_info->AddArrayPointer("m_matrix",p_mat);

	p_mat=new Script::CArray;
	p_mat->SetSizeAndType(3,ESYMBOLTYPE_VECTOR);
	p_vec=new Script::CVector;
	p_vec->mX=m_display_matrix[X][X]; p_vec->mY=m_display_matrix[X][Y]; p_vec->mZ=m_display_matrix[X][Z];
	p_mat->SetVector(0,p_vec);
	p_vec=new Script::CVector;
	p_vec->mX=m_display_matrix[Y][X]; p_vec->mY=m_display_matrix[Y][Y]; p_vec->mZ=m_display_matrix[Y][Z];
	p_mat->SetVector(1,p_vec);
	p_vec=new Script::CVector;
	p_vec->mX=m_display_matrix[Z][X]; p_vec->mY=m_display_matrix[Z][Y]; p_vec->mZ=m_display_matrix[Z][Z];
	p_mat->SetVector(2,p_vec);
	p_info->AddArrayPointer("m_display_matrix",p_mat);

	Script::CStruct *p_components_structure=new Script::CStruct;
	CBaseComponent *p_comp=mp_component_list;
	while (p_comp)
	{
		Script::CStruct *p_component_structure=new Script::CStruct;
		p_comp->GetDebugInfo(p_component_structure);
		
		// Using the component type as the name given to the structure, since the
		// type is the checksum of the type name.
		p_components_structure->AddStructurePointer(p_comp->GetType(),p_component_structure);
		p_comp=p_comp->GetNext();
	}
		
	p_info->AddStructurePointer("Components",p_components_structure);
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompositeObject::SetTeleported( bool update_components )
{
	m_composite_object_flags.Set(CO_TELEPORTED);

	if ( update_components )
	{
		Teleport();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}

