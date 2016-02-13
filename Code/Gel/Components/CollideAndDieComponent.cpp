//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       CollideAndDieComponent.cpp
//* OWNER:          SPG
//* CREATION DATE:  7/10/03
//****************************************************************************

// The CEmptyComponent class is an skeletal version of a component
// It is intended that you use this as the basis for creating new
// components.  
// To create a new component called "Watch", (CWatchComponent):
//  - copy emptycomponent.cpp/.h to watchcomponent.cpp/.h
//  - in both files, search and replace "Empty" with "Watch", preserving the case
//  - in WatchComponent.h, update the CRCD value of CRC_WATCH
//  - in CompositeObjectManager.cpp, in the CCompositeObjectManager constructor, add:
//		  	RegisterComponent(CRC_WATCH,			CWatchComponent::s_create); 
//  - and add the include of the header
//			#include <gel/components/watchcomponent.h> 
//  - Add it to build\gel.mkf, like:
//          $(NGEL)/components/WatchComponent.cpp\
//  - Fill in the OWNER (yourself) and the CREATION DATE (today's date) in the .cpp and the .h files
//	- Insert code as needed and remove generic comments
//  - remove these comments
//  - add comments specfic to the component, explaining its usage

#include <gel/components/CollideAndDieComponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>

#include <sk/engine/feeler.h>
#include <sk/components/ProjectileCollisionComponent.h>
#include <sk/gamenet/gamenet.h>

#define vFIREBALL_LIFETIME 	Tmr::Seconds( 10 )

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// s_create is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
// s_create	returns a CBaseComponent*, as it is to be used
// by factor creation schemes that do not care what type of
// component is being created
// **  after you've finished creating this component, be sure to
// **  add it to the list of registered functions in the
// **  CCompositeObjectManager constructor  

CBaseComponent* CCollideAndDieComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CCollideAndDieComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// All components set their type, which is a unique 32-bit number
// (the CRC of their name), which is used to identify the component	
CCollideAndDieComponent::CCollideAndDieComponent() : CBaseComponent()
{
    SetType( CRC_COLLIDEANDDIE );
	m_radius = 0.0f;
	m_scale = 0;
	m_birth_time = Tmr::GetTime();
	m_death_script = 0;
	m_dying = false;
	m_first_frame = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCollideAndDieComponent::~CCollideAndDieComponent()
{   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCollideAndDieComponent::SetCollisionRadius( float radius )
{
	m_radius = radius;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CCollideAndDieComponent::InitFromStructure( Script::CStruct* pParams )
{
	pParams->GetFloat(CRCD(0xc48391a5,"radius"), &m_radius, Script::ASSERT);
	pParams->GetFloat(CRCD(0x13b9da7b,"scale"), &m_scale, Script::ASSERT);
	
	pParams->GetChecksum(CRCD(0x6647adc3,"death_script"), &m_death_script, Script::ASSERT);

    m_vel.Set( 0, 0, 1 );
	pParams->GetVector(CRCD(0xc4c809e, "vel"), &m_vel, Script::ASSERT);
	m_vel.Normalize();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// RefreshFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CCollideAndDieComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Default to just calline InitFromStructure()
	// but if that does not handle it, then will need to write a specific 
	// function here. 
	// The user might only want to update a single field in the structure
	// and we don't want to be asserting becasue everything is missing 
	
	//InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCollideAndDieComponent::Finalize()
{
	mp_suspend_component =  GetSuspendComponentFromObject( GetObject() );
}
	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollideAndDieComponent::Hide( bool should_hide )
{
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// The component's Update() function is called from the CCompositeObject's 
// Update() function.  That is called every game frame by the CCompositeObjectManager
// from the s_logic_code function that the CCompositeObjectManager registers
// with the task manger.
void CCollideAndDieComponent::Update()
{
	GameNet::Manager* gamenet_man = GameNet::Manager::Instance();

	if (gamenet_man->InNetGame() || !mp_suspend_component->SkipLogic())
	{
		if(( Tmr::GetTime() - m_birth_time ) > vFIREBALL_LIFETIME )
		{
			GetObject()->MarkAsDead();
		}
		else
		{
			if( m_dying )
			{
				if( Tmr::GetTime() > m_death_time )
				{
					GetObject()->MarkAsDead();
				}
			}
			else
			{
				Mth::Vector vel, start, end;
				CFeeler feeler;
		
				vel = GetObject()->GetVel();
				vel.Normalize();
				if( m_first_frame )
				{
					start = GetObject()->GetPos();
				}
				else
				{
					start = m_last_pos;
				}
				
				start[Y] += FEET( 2 );
				end = GetObject()->GetPos();
				end[Y] += FEET( 2 );
				end += ( vel * m_radius );
				feeler.SetStart( start );
				feeler.SetEnd( end );
		
				if( feeler.GetCollision())
				{
					if( m_dying == false )
					{
						CProjectileCollisionComponent* proj_comp;

						proj_comp = GetProjectileCollisionComponentFromObject( GetObject());
						if( proj_comp )
						{
							proj_comp->MarkAsDying();
						}

						m_dying = true;
						m_death_time = Tmr::GetTime() + Tmr::Seconds( 1 );
						if( m_death_script != 0 )
						{
							Script::CStruct* params;
							Mth::Vector pos;

							pos = feeler.GetPoint();
							params = new Script::CStruct;
							
							params->AddVector( CRCD(0x7f261953,"pos"), pos );
							params->AddVector( CRCD(0xc4c809e,"vel"), m_vel );
							params->AddFloat( CRCD(0x13b9da7b,"scale"), m_scale );

							Script::RunScript( m_death_script, params );

							delete params;
						}
					}
				}
			}
		}

		m_last_pos = GetObject()->GetPos();
		m_first_frame = false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the "Checksum" of a script command, then possibly handle it
// if it's a command that this component will handle	
CBaseComponent::EMemberFunctionResult CCollideAndDieComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
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

void CCollideAndDieComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to CCollideAndDieComponent::GetDebugInfo"));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums
	

// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}
