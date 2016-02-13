//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SkaterProximityComponent.cpp
//* OWNER:          Mick West
//* CREATION DATE:  3/20/03
//****************************************************************************

/*

	Component for raising exceptions/events when the skater gets within a particular radius
	this used to be part of the ExceptionComponent, but as that is slated for destruction,
	I've had to seperate it out.  					

*/
					
// The CSkaterProximityComponent class is an skeletal version of a component
// It is intended that you use this as the basis for creating new
// components.  
// To create a new component called "Watch", (CWatchComponent):
//  - copy SkaterProximitycomponent.cpp/.h to watchcomponent.cpp/.h
//  - in both files, search and replace "SkaterProximity" with "Watch", preserving the case
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

#include <sk/components/SkaterProximitycomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/objtrack.h>


#include <sk/modules/skate/skate.h>
#include <sk/objects/pathob.h>
#include <sk/objects/skater.h>


#define	FLAGEXCEPTION(X) GetObject()->SelfEvent(X)


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

CBaseComponent* CSkaterProximityComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterProximityComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// All components set their type, which is a unique 32-bit number
// (the CRC of their name), which is used to identify the component	
CSkaterProximityComponent::CSkaterProximityComponent() : CBaseComponent()
{
	SetType( CRC_SKATERPROXIMITY );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterProximityComponent::~CSkaterProximityComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CSkaterProximityComponent::InitFromStructure( Script::CStruct* pParams )
{
	// ** Add code to parse the structure, and initialize the component

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// RefreshFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CSkaterProximityComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Default to just calline InitFromStructure()
	// but if that does not handle it, then will need to write a specific 
	// function here. 
	// The user might only want to update a single field in the structure
	// and we don't want to be asserting becasue everything is missing 
	
	InitFromStructure(pParams);


}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// The component's Update() function is called from the CCompositeObject's 
// Update() function.  That is called every game frame by the CCompositeObjectManager
// from the s_logic_code function that the CCompositeObjectManager registers
// with the task manger.
void CSkaterProximityComponent::Update()
{
	// GJ TODO:  maybe this should be made more generic
	// so that it takes a CCompositeObject*	(it would
	// make replacing the skater w/ a different player
	// object easier)

	// check skater dist exceptions...
	float distToSkaterSqr = 0.0f;
	
	if ( mInnerRadiusSqr )
	{
		distToSkaterSqr = GetDistToLocalSkaterSquared();
		//distToSkaterSqr = GetDistToNearestSkaterSquared( );
		if ( distToSkaterSqr <= mInnerRadiusSqr )
		{
			// If the local skater is in radius, then flag "SkaterInRadius"
			FLAGEXCEPTION( CRCD(0xdb7413fb,"SkaterInRadius") );
			if (!GetObject()->IsDead())
			{
				FLAGEXCEPTION( CRCD(0x5e8eb123,"AnySkaterInRadius") );
			}
		}
		else
		{
			distToSkaterSqr = GetDistToNearestSkaterSquared();	// MIGHT BE A NETWORK SKATER
			if ( distToSkaterSqr <= mInnerRadiusSqr )
			{
				FLAGEXCEPTION( CRCD(0x5e8eb123,"AnySkaterInRadius") );
			}
			distToSkaterSqr = 0.0f;		// Mick:  need to clear it, otherwise this value will be used for the
										// AvoidRadius settings, which only apply to local skaters
		}
	}
	
	if ( mOuterRadiusSqr )
	{
		if ( distToSkaterSqr == 0.0f )
		{
			distToSkaterSqr = GetDistToLocalSkaterSquared();
		}
		if ( distToSkaterSqr >= mOuterRadiusSqr )
		{
			if (!GetObject()->IsDead())
			{
				FLAGEXCEPTION( CRCD(0xa41f5336,"SkaterOutOfRadius") ); 
			}
		}
	}
	
	if ( mInnerAvoidRadiusSqr )
	{
		if ( distToSkaterSqr == 0.0f )
		{
			distToSkaterSqr = GetDistToLocalSkaterSquared();
		}
		if ( distToSkaterSqr <= mInnerAvoidRadiusSqr )
		{
			if (!GetObject()->IsDead())
			{
				FLAGEXCEPTION( CRCD(0xfaeec40f,"SkaterInAvoidRadius") );
			}
		}
	}
	
	if ( mOuterAvoidRadiusSqr )
	{
		if ( distToSkaterSqr == 0.0f )
		{
			distToSkaterSqr = GetDistToLocalSkaterSquared();
		}
		if ( distToSkaterSqr >= mOuterAvoidRadiusSqr )
		{
			if (!GetObject()->IsDead())
			{
				FLAGEXCEPTION( CRCD(0x9c5af1a0,"SkaterOutOfAvoidRadius") );
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the "Checksum" of a script command, then possibly handle it
// if it's a command that this component will handle	
CBaseComponent::EMemberFunctionResult CSkaterProximityComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | Obj_SetInnerRadius | 
        // @uparm 1.0 | inner radius
		case ( 0x82a0a873 ):	// Obj_SetInnerRadius
			if ( pParams->GetFloat( NONAME, &mInnerRadiusSqr ) )
			{
				mInnerRadiusSqr *= FEET_TO_INCHES( 1.0f );
				mInnerRadiusSqr *= mInnerRadiusSqr;
			}
			break;

        // @script | Obj_SetOuterRadius | 
        // @uparm 1.0 | outer radius
		case ( 0x8cb5d0c0 ):	// Obj_SetOuterRadius
			if ( pParams->GetFloat( NONAME, &mOuterRadiusSqr ) )
			{
				mOuterRadiusSqr *= FEET_TO_INCHES( 1.0f );
				mOuterRadiusSqr *= mOuterRadiusSqr;
			}
			break;

		// @script | Obj_SetInnerAvoidRadius | 
		// @uparm 1.0 | inner radius
		case 0x50f28d36: // Obj_SetInnerAvoidRadius
			if ( pParams->GetFloat( NONAME, &mInnerAvoidRadiusSqr ) )
			{
				mInnerAvoidRadiusSqr *= FEET_TO_INCHES( 1.0f );
				mInnerAvoidRadiusSqr *= mInnerAvoidRadiusSqr;
			}
			break;

		// @script | Obj_SetOuterAvoidRadius | 
		// @uparm 1.0 | outer radius
		case 0x2d703b94: // Obj_SetOuterAvoidRadius
			if ( pParams->GetFloat( NONAME, &mOuterAvoidRadiusSqr ) )
			{
				mOuterAvoidRadiusSqr *= FEET_TO_INCHES( 1.0f );
				mOuterAvoidRadiusSqr *= mOuterAvoidRadiusSqr;
			}
			break;



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

void CSkaterProximityComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterProximityComponent::GetDebugInfo"));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums

	/*	Example:
	p_info->AddInteger("m_never_suspend",m_never_suspend);
	p_info->AddFloat("m_suspend_distance",m_suspend_distance);
	*/
	p_info->AddFloat("mInnerRadiusSqr",mInnerRadiusSqr);
	p_info->AddFloat("mOuterRadiusSqr",mOuterRadiusSqr);
	p_info->AddFloat("mInnerAvoidRadiusSqr",mInnerAvoidRadiusSqr);
	p_info->AddFloat("mOuterAvoidRadiusSqr",mOuterAvoidRadiusSqr);

	
// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#define HUGE_DISTANCE_SQUARED ( HUGE_DISTANCE * HUGE_DISTANCE )

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CSkaterProximityComponent::GetDistToLocalSkaterSquared()
{
	CCompositeObject* pObj = Mdl::Skate::Instance()->GetLocalSkater();
	
	if ( pObj && ( pObj != GetObject() ))
	{
		return Mth::DistanceSqr( pObj->GetPos(), GetObject()->GetPos() );
	}
	
	return ( HUGE_DISTANCE_SQUARED );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CSkaterProximityComponent::GetDistToNearestSkaterSquared()
{	
	float nearest_distance = HUGE_DISTANCE_SQUARED;
	
	int num_skaters = Mdl::Skate::Instance()->GetNumSkaters();
	
	for( int i = 0; i < num_skaters; i++ )
	{
		CCompositeObject* pObj = Mdl::Skate::Instance()->GetSkater( i );
		if ( pObj && ( pObj != GetObject() ))
		{
			float this_dist = Mth::DistanceSqr( pObj->GetPos(), GetObject()->GetPos() );
			if( this_dist < nearest_distance )
			{
				nearest_distance = this_dist;
			}
		}
	}
	
	return nearest_distance;
}



	
}
