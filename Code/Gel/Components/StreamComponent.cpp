//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       StreamComponent.cpp
//* OWNER:          ???
//* CREATION DATE:  ???
//****************************************************************************

// The CStreamComponent class is an skeletal version of a component
// It is intended that you use this as the basis for creating new
// components.  
// To create a new component called "Watch", (CWatchComponent):
//  - copy streamcomponent.cpp/.h to watchcomponent.cpp/.h
//  - in both files, search and replace "Stream" with "Watch", preserving the case
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

#include <gel/components/streamcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

#include <gel/music/music.h>

#include <gfx/camera.h>

// TODO:  Refactor this - 
#include <sk/objects/emitter.h>
#include <sk/objects/proxim.h>


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

CBaseComponent* CStreamComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CStreamComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// All components set their type, which is a unique 32-bit number
// (the CRC of their name), which is used to identify the component	
CStreamComponent::CStreamComponent() : CBaseComponent()
{
	SetType( CRC_STREAM );
	mp_emitter = NULL;
	mp_proxim_node = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CStreamComponent::~CStreamComponent()
{
	// If there is a streaming sound attached, remove it.
	int i;
	for( i = 0; i < MAX_STREAMS_PER_OBJECT; i++ )
	{
		if( mStreamingID[i] )
		{
			Pcm::StopStreamFromID( mStreamingID[i] );
		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CStreamComponent::InitFromStructure( Script::CStruct* pParams )
{
	uint32 proxim_node_checksum;

	if (pParams->GetChecksum("ProximNode", &proxim_node_checksum))
	{
		//Dbg_Message("StreamComponent: Looking for ProximNode %s", Script::FindChecksumName(proxim_node_checksum));
		mp_proxim_node = CProximManager::sGetNode(proxim_node_checksum);
		//if (mp_proxim_node)
		//{
		//	Dbg_Message("Found ProximNode %s", Script::FindChecksumName(mp_proxim_node->m_name));
		//}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// RefreshFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CStreamComponent::RefreshFromStructure( Script::CStruct* pParams )
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
void CStreamComponent::Update()
{
	// Doing nothing, so tell code to do nothing next time around
	Suspend(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the "Checksum" of a script command, then possibly handle it
// if it's a command that this component will handle	
CBaseComponent::EMemberFunctionResult CStreamComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{

		case 0xdb05a233:  // Obj_PlayStream
			PlayStream( pParams, pScript );
			break;
		
		case 0xa738aeb8:  // Obj_StopStream
		{
			// eventually want to be able to specify channel on here...
			int i;
			for ( i = 0; i < MAX_STREAMS_PER_OBJECT; i++ )
			{
				if ( mStreamingID[ i ] )
				{
					Pcm::StopStreamFromID( mStreamingID[ i ] );
					mStreamingID[ i ] = 0;
				}
			}
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

void CStreamComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to C......Component::GetDebugInfo"));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums

	/*	Example:
	p_info->AddInteger("m_never_suspend",m_never_suspend);
	p_info->AddFloat("m_suspend_distance",m_suspend_distance);
	*/
	
// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector	CStreamComponent::GetClosestEmitterPos( Gfx::Camera *pCamera )
{
	if (mp_emitter && pCamera)
	{
		return mp_emitter->GetClosestPoint(pCamera->GetPos());
	}
	else
	{
		return GetObject()->m_pos;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CStreamComponent::GetClosestDropoffPos( Gfx::Camera *pCamera, Mth::Vector & dropoff_pos )
{
	if (pCamera && mp_proxim_node)
	{
		dropoff_pos = mp_proxim_node->GetClosestIntersectionPoint(pCamera->GetPos());
		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


// Returns 0 if no stream slots empty, or the index + 1
int CStreamComponent::StreamSlotEmpty( void )
{
	int j;
	for ( j = 0; j < MAX_STREAMS_PER_OBJECT; j++ )
	{
		if ( !mStreamingID[ j ] )
		{
			return ( j + 1 );
		}
	}
	return ( 0 );
}


bool CStreamComponent::PlayStream( Script::CScriptStructure *pParams, Script::CScript *pScript )
{
	
	if ( !StreamSlotEmpty( ) )
	{
	
		#ifdef	__NOPT_ASSERT__
		printf ("Playing too many streams on object\n");
		#endif
										  
		return ( false ); // can only play 3 stream on each object max!!!
	}
	uint32 streamChecksum;
	uint32 emitterChecksum;
	uint32 dropoffFuncChecksum;
	uint32 id = 0;
	float dropoff = 0.0f;
	float volume = 100.0f;
	float pitch = 100.0f;
	int priority = STREAM_DEFAULT_PRIORITY;
	EDropoffFunc dropoffFunction = DROPOFF_FUNC_STANDARD;
	pParams->GetFloat( 0xf6a36814, &volume ); // "vol"
	pParams->GetFloat( 0xd8604126, &pitch ); // "pitch"
	pParams->GetInteger( 0x9d5923d8, &priority ); // priority
	pParams->GetChecksum( 0x40c698af, &id ); // id
	if ( pParams->GetFloat( 0xff2020ec, &dropoff ) ) // "dropoff"
	{
		dropoff = FEET_TO_INCHES( dropoff );
	}
	if ( pParams->GetChecksum( 0xc6ac50a, &dropoffFuncChecksum ) ) // "dropoff_function"
	{
		dropoffFunction = Sfx::GetDropoffFunctionFromChecksum( dropoffFuncChecksum );
	}
	if (pParams->GetChecksum( 0x8a7132ce, &emitterChecksum ) ) // "emitter"
	{
		mp_emitter = CEmitterManager::sGetEmitter(emitterChecksum);
	}
	else
	{
		mp_emitter = NULL;
	}
	if ( pParams->GetChecksum( NONAME, &streamChecksum ) )
	{
		int use_pos_info = 1;
		pParams->GetInteger( "use_pos_info", &use_pos_info, Script::NO_ASSERT );
		//printf ("Playing stream 0x%x on object %s\n",streamChecksum,Script::FindChecksumName(GetObject()->GetID()));
		return ( Pcm::PlayStreamFromObject( this, streamChecksum, dropoff, volume, pitch, priority, use_pos_info, dropoffFunction, id ) );
	}
	return ( false );
}





	
}
