/*
	MovingObject.cpp
	
	Skaters, Peds, Cars are derived from CMovingObject.

	These objects are controlled through scripts.  Script commands are
	sent through CallMemberFunction( ).  See all available commands by
	looking at CMovingObject::CallMemberFunction().
*/

// start autoduck documentation
// @DOC movingobject
// @module movingobject | None
// @subindex Scripting Database
// @index script | movingobject

#include <sk/objects/movingobject.h>

#include <gel/mainloop.h>
#include <gel/objman.h>
#include <gel/objsearch.h>
#include <gel/objtrack.h>

#include <gel/components/animationcomponent.h>
#include <gel/components/lockobjcomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/motioncomponent.h>
#include <gel/components/skeletoncomponent.h>
#include <gel/components/suspendcomponent.h>
#include <gel/components/soundcomponent.h>
#include <gel/components/specialitemcomponent.h>
#include <gel/components/streamcomponent.h>

#include <sk/components/skaterproximitycomponent.h>

#include <gel/object/compositeobjectmanager.h>

#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>

#include <gfx/debuggfx.h> // for AddDebugLine( )
#include <gfx/bbox.h>
#include <gfx/gfxutils.h>
#include <gfx/nx.h>
#include <gfx/nxmodel.h>
#include <gfx/skeleton.h>

#include <sk/modules/skate/skate.h>
#include <sk/objects/skater.h>
#include <sk/scripting/nodearray.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMovingObject::DrawBoundingBox( SBBox *pBox, Mth::Vector *pOffset, int numFrames, Mth::Vector *pRot )
{
    Gfx::AddDebugBox( m_matrix, m_pos, pBox, pOffset, numFrames, pRot);
} 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMovingObject::CMovingObject()
{
	// set up default bounding box
    m_bbox.m_max.Set(10.0f, 10.0f, 10.0f);
    m_bbox.m_min.Set(-10.0f, -10.0f, -10.0f);    
    m_bbox.centerOfGravity.Set();			// zero center of gravity......    

	// suspend component should come first, because it
	// controls whether the rest of the components
	// should be updated
	CSuspendComponent* p_suspendComponent = new CSuspendComponent;
	AddComponent(p_suspendComponent);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMovingObject::~CMovingObject( void )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMovingObject::MovingObjectCreateComponents()
{
	CMotionComponent* p_motionComponent = new CMotionComponent;
	AddComponent(p_motionComponent);

	CSpecialItemComponent* p_specialItemComponent = new CSpecialItemComponent;
	AddComponent(p_specialItemComponent);

	CSkaterProximityComponent* p_skaterProximityComponent = new CSkaterProximityComponent;
	AddComponent( p_skaterProximityComponent );

	CSoundComponent* p_soundComponent = new CSoundComponent;
	AddComponent(p_soundComponent);
	
	CStreamComponent* p_streamComponent = new CStreamComponent;
	AddComponent(p_streamComponent);
	
    CLockObjComponent* p_lockObjComponent = new CLockObjComponent;
	AddComponent(p_lockObjComponent);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMovingObject::MovingObjectInit( Script::CStruct* pNodeData, CGeneralManager* p_obj_man )
{
	// Dan: a hack for now to detect whether anyone has called CreateComponents() yet or not
	// once components are created in a consistent manner across object types, we can remove this sort of foolishness
	if (!GetMotionComponent())
	{
		MovingObjectCreateComponents();
	}
	
	Dbg_MsgAssert( GetMotionComponent(), ( "Motion component doesn't exist yet" ) );
	
	GetMotionComponent()->OrientToNode( pNodeData );
	
	//----------------------------------------
	// MOVED FROM POSOBJECT.H
	Dbg_MsgAssert( pNodeData, ( "Couldn't find node structure." ));
	
	// Get the checksum ...
	uint32	NodeNameChecksum = 0;
	pNodeData->GetChecksum(CRCD(0xa1dc81f9,"Name"),&NodeNameChecksum);
	if (NodeNameChecksum)
	{
		SetID(NodeNameChecksum);
	}	
	
	uint32 AIScriptChecksum=0;
	if ( pNodeData->GetChecksum(CRCD(0x2ca8a299,"TriggerScript"),&AIScriptChecksum) )
	{
		Script::CScriptStructure *pScriptParams=NULL;
		pNodeData->GetStructure(CRCD(0x7031f10c,"Params"),&pScriptParams);
		SwitchScript( AIScriptChecksum, pScriptParams );
	}
	//----------------------------------------
	
	GetMotionComponent()->InitFromStructure( pNodeData );
	GetSuspendComponent()->InitFromStructure( pNodeData );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Gfx::CSkeleton* CMovingObject::GetSkeleton()
{
	Obj::CSkeletonComponent* p_skeleton_component = GetSkeletonComponent();
	if ( p_skeleton_component )
	{
		return p_skeleton_component->GetSkeleton();
	}

    return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::CModel* CMovingObject::GetModel()
{
	Obj::CModelComponent* p_model_component = GetModelComponent();
    if ( p_model_component )
    {
        return p_model_component->GetModel();
    }
    
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovingObject::LookAtObject_Init( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 typeChecksum;
	if ( pParams->GetChecksum( 0x7321a8d6, &typeChecksum ) ) // "type"
	{
		int type;
		switch ( typeChecksum )
		{
			case 0x88c21962: // car
				type = SKATE_TYPE_CAR;
				break;
			case 0x61a741e: // ped
				type = SKATE_TYPE_PED;
				break;
			case 0x5b8ab877: // skater
				type = SKATE_TYPE_SKATER;
				break;
			case 0x3b5737a6:  // gameobj
				type = SKATE_TYPE_GAME_OBJ;
				break;
			default:
				Dbg_MsgAssert( 0,( "\n%s\nUnknown type %s", pScript->GetScriptInfo( ), Script::FindChecksumName( typeChecksum ) ));
				return false;
				break;
		}
		CCompositeObject* pClosestObject = GetClosestObjectOfType( type );
		if ( !pClosestObject )
		{
			Dbg_Message( "\n%s\nWarning:  Looking at object that doesn't exist.", pScript->GetScriptInfo( ) );
			return false;
		}
		return GetMotionComponent()->LookAt_Init( pParams, pClosestObject->m_pos );
	}
	
	// find a named object:
	uint32 nameChecksum;
	if ( pParams->GetChecksum( 0xa1dc81f9, &nameChecksum ) ) // "name"
	{
		CCompositeObject* pNext;
		CCompositeObject* pObj;
		Lst::Search< CObject >	sh;
		Dbg_MsgAssert(mp_manager,("NULL mp_Manager in MovingObject"));
		pNext = (Obj::CCompositeObject*) sh.FirstItem( mp_manager->GetRefObjectList() );
		while ( pNext )
		{
			Dbg_AssertType( pNext, CCompositeObject );
			pObj = pNext;
			pNext = (Obj::CCompositeObject*) sh.NextItem();
			// find objects spawned from this node, return true if out of this radius...
			if ( ( pObj != this ) && ( pObj->GetID() == nameChecksum ) )
			{
				return GetMotionComponent()->LookAt_Init( pParams, pObj->m_pos );
			}
		}
		Dbg_Message( "\n%s\nWarning:  Looking at object that doesn't exist.", pScript->GetScriptInfo( ) );
		return false;
	}
	Dbg_Message( "\n%s\nWarning: No object specified for Obj_LookAtObject", pScript->GetScriptInfo( ) );
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovingObject::ObjectFromNodeWithinRange( int nodeIndex, int radiusSqr, SiteBox* pBox )
{
	uint32	nodeNameChecksum = SkateScript::GetNodeNameChecksum(nodeIndex);

// TODO:  Since there can only be one object from a node, then should be able to get it directly
	
	bool retVal = false;
	CMovingObject* pNext;
	CMovingObject* pObj;
	Lst::Search< CObject >	sh;
//	Dbg_MsgAssert(mp_manager,("NULL mp_Manager in MovingObject. Node Checksum = 0x%x (%s)",GetID(),Script::FindChecksumName(GetID())));
	Dbg_MsgAssert(mp_manager,("NULL mp_Manager in MovingObject. Node Checksum = 0x%x (%s)",GetID(),Script::FindChecksumName(GetID())));
	pNext = (Obj::CMovingObject *) sh.FirstItem( mp_manager->GetRefObjectList() );
	while ( pNext )
	{
		Dbg_AssertType( pNext, CMovingObject );
		pObj = (Obj::CMovingObject *) pNext;
		pNext = (Obj::CMovingObject *) sh.NextItem();
		
		if ( ( pObj->GetID() == nodeNameChecksum ) && ( pObj != this ) )
		{
			if ( pBox )
			{
				if ( ObjInSiteBox( pObj, pBox ) )
				{
					retVal = true;
				}
			}
			else if ( Mth::DistanceSqr( pObj->m_pos, m_pos ) < radiusSqr )
			{
				retVal = true;
			}
		}
	}
	return ( retVal );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CMovingObject::GetSiteBoxMaxRadiusSquared( SiteBox* pBox )
{
	float maxBoxDist;
	if ( pBox->maxDistInitialized )
	{
		return ( pBox->maxDistSquared );
	}
	maxBoxDist = fabsf( pBox->dist );
	if ( fabsf( pBox->height ) > maxBoxDist )
	{
		maxBoxDist = fabsf( pBox->height );
	}
	if ( fabsf( pBox->width ) > maxBoxDist )
	{
		maxBoxDist = fabsf( pBox->width );
	}

	float maxOffset;
	Mth::Vector* pOffset;
	pOffset = &pBox->bbox.centerOfGravity;
	maxOffset = fabsf( pOffset->GetZ( ) );
	if ( fabsf( pOffset->GetY( ) ) > maxOffset )
	{
		maxOffset = fabsf( pOffset->GetY( ) );
	}
	if ( fabsf( pOffset->GetX( ) ) > maxOffset )
	{
		maxOffset = fabsf( pOffset->GetX( ) );
	}

	pBox->maxDistSquared = maxOffset + maxBoxDist;
	pBox->maxDistSquared *= pBox->maxDistSquared;
	pBox->maxDistInitialized = true;
	return ( pBox->maxDistSquared );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMovingObject::InitializeSiteBox( SiteBox* pBox )
{
	if ( pBox->initialized )
	{
		// already initialized!
		return;
	}
	
	pBox->matrix = m_matrix;
	if ( pBox->angle )
	{
		pBox->matrix = pBox->matrix.RotateYLocal( pBox->angle );
	}
	pBox->matrix.Invert( );
	
	SBBox* pBBox = &pBox->bbox;
	// start by setting min/max to half the dimentions:
	pBBox->m_max.Set( pBox->width / 2.0f, pBox->height / 2.0f, pBox->dist );
	pBBox->m_min = -pBBox->m_max;
	pBBox->m_min[ Z ] = 0.0f;

	// using the vector in pBox->bbox.centerOfGravity to store offset
	// sent by the player! MISNOMER:
	pBBox->m_max += pBBox->centerOfGravity;
	pBBox->m_min += pBBox->centerOfGravity;
		
	// gots ta move the site box up so that it comes from the object's
	// center of gravity (as the object's position isn't usually the center
	// of gravity):
/*	if ( mp_model )
	{
		//Mth::Vector temp3;
		//temp3 = m_pos + centerOfGravity;		
		//Gfx::AddDebugLine( m_pos, temp3 );
		pBBox->m_max += m_bbox.centerOfGravity;
		pBBox->m_min += m_bbox.centerOfGravity;
	}*/

#ifndef __PLAT_NGC__	
#ifdef __NOPT_ASSERT__	
	if ( pBox->debug )
	{
		pBBox->centerOfGravity.Set( ); // zero that out for the render...
		Mth::Vector drawOffset;
//		drawOffset.Set( 0, m_ground_offset, 0 );
		drawOffset.Set( 0, 0, 0 );
		if ( pBox->angle )
		{
			Mth::Vector angles;
			angles.Set( 0, pBox->angle, 0 );
			DrawBoundingBox( pBBox, &drawOffset, 10, &angles );
		}
		else
		{
			DrawBoundingBox( pBBox, &drawOffset, 10 );
		}
	}
#endif
#endif
	
	// Phew... now we can quickly check objects by rotating their position
	// into our world, and seeing if they're in the motherfuckin' box:
	pBox->initialized = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovingObject::ObjInSiteBox( CMovingObject* pObj, SiteBox* pBox )
{
	// K: There was a bug whereby the initial distance check below was not taking
	// into account the offset of the box.
	// The offset is stored in centerOfGravity which may get zeroed in InitializeSiteBox,
	// so store it here.
	Mth::Vector off=pBox->bbox.centerOfGravity;
	
	// initialize the site box and the BBox contained therein:
	InitializeSiteBox( pBox );

	// sorta trivial radius check and return...
	float distSquared;
	distSquared = Mth::DistanceSqr( pObj->m_pos+off, m_pos );
	float boxRadiusSquared = GetSiteBoxMaxRadiusSquared( pBox );
	if ( distSquared > boxRadiusSquared )
	{
		return false;
	}

	Mth::Vector objPos;
	objPos = pObj->m_pos;
	
	// center of the object...
	objPos[ Y ] += ( pObj->m_bbox.m_max[ Y ] - pObj->m_bbox.m_min[ Y ] ) / 2.0f;

#ifndef __PLAT_NGC__	
#ifdef __NOPT_ASSERT__
	Mth::Vector preTransform = objPos;
#endif	
#endif	

	objPos -= m_pos;
	// now we have the head and toes at the right distance from us and from each other...
	// rotate them into our world:
	objPos = pBox->matrix.Transform( objPos );
	
#ifndef __PLAT_NGC__	
#ifdef __NOPT_ASSERT__
	if ( pBox->debug )
	{
		if ( Gfx::PointInsideBox( objPos, pBox->bbox.m_max, pBox->bbox.m_min ) )
		{
			Gfx::AddDebugLine( m_pos, preTransform, MAKE_RGB( 150, 50, 50 ), MAKE_RGB( 150, 50, 50 ), 10 );
			return true;
		}
		else
		{
			Gfx::AddDebugLine( m_pos, preTransform, MAKE_RGB( 50, 50, 100 ), MAKE_RGB( 50, 50, 100 ), 10 );
			return false;
		}
	}
#endif
#endif
	return ( Gfx::PointInsideBox( objPos, pBox->bbox.m_max, pBox->bbox.m_min ) );
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMovingObject::FillInSiteBox(SiteBox* p_siteBox, Script::CStruct* pParams)
{
	Dbg_MsgAssert(p_siteBox,("NULL p_siteBox"));
	Dbg_MsgAssert(pParams,("NULL pParams"));
	
	p_siteBox->initialized = false;
	p_siteBox->angle = 0;
	p_siteBox->maxDistInitialized = false;
	
	// Misnomer: used for offsetting the site box...
	// But we will want it to default to the center of gravity:
	p_siteBox->bbox.centerOfGravity = m_bbox.centerOfGravity;
	
    p_siteBox->height = p_siteBox->width = p_siteBox->dist = 0.0f;
	
	Mth::Vector offset;
	offset[W]=0.0f; //
	if ( pParams->GetVector( 0xa6f5352f, &offset ) ) // offset
	{
		offset.FeetToInches( );
		p_siteBox->bbox.centerOfGravity += offset;
	}

	if ( pParams->GetFloat( 0xff7ebaf6, &p_siteBox->angle ) ) // angle
	{
		p_siteBox->angle = DEGREES_TO_RADIANS( p_siteBox->angle );
	}

	if ( pParams->GetFloat( 0xab21af0, &p_siteBox->height ) ) // height
	{
		p_siteBox->height = FEET_TO_INCHES( p_siteBox->height );
	}
	if ( pParams->GetFloat( 0x7e832f08, &p_siteBox->dist ) ) // dist
	{
		p_siteBox->dist = FEET_TO_INCHES( p_siteBox->dist  );
	}
	if ( pParams->GetFloat( 0x73e5bad0, &p_siteBox->width ) ) // width
	{
		p_siteBox->width = FEET_TO_INCHES( p_siteBox->width );
	}

	#ifdef	__NOPT_ASSERT__	
	p_siteBox->debug = 0;

	p_siteBox->debug = pParams->ContainsFlag( 0x935ab858 ); // debug
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovingObject::ObjectWithinRect( Script::CStruct* pParams, Script::CScript* pScript )
{
	SiteBox siteBox;
	FillInSiteBox(&siteBox,pParams);
	
	return ( ObjectWithinRange( pParams, pScript, &siteBox ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovingObject::SkaterInRange( SiteBox* pBox, float radiusSqr )
{
	int numSkaters = Mdl::Skate::Instance()->GetNumSkaters( );
	
	for ( int i = 0; i < numSkaters; i++ )
	{
		CMovingObject* pObj = Mdl::Skate::Instance()->GetSkater( i );
		if ( pObj != this )
		{
			if ( pBox )
			{
				if ( ObjInSiteBox( pObj, pBox ) )
				{
					return true;
				}
			}
			else if ( Mth::DistanceSqr( pObj->m_pos, m_pos ) < radiusSqr )
			{
				return true;
			}
		}
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovingObject::ObjTypeInRange( Script::CStruct* pParams, Script::CScript* pScript, float radiusSqr, SiteBox* pBox, uint32 typeChecksum )
{
	
	int type;
	Search objSearch;		
	CMovingObject* pObj;
	switch ( typeChecksum )
	{
		case 0x88c21962: // car
			type = SKATE_TYPE_CAR;
			break;
		case 0x61a741e: // ped
			type = SKATE_TYPE_PED;
			break;
		case 0x5b8ab877: // skater
			return ( SkaterInRange( pBox, radiusSqr ) );
			break;
		case 0x3b5737a6: // gameobj
			type = SKATE_TYPE_GAME_OBJ;
			break;
		default:
			Dbg_MsgAssert( "\n%s\nUnknown type %s",((char *)( pScript->GetScriptInfo( ), Script::FindChecksumName( typeChecksum ) )));
			return false;
			break;
	}
	Dbg_MsgAssert(mp_manager,("NULL mp_Manager in MovingObject"));
	pObj = (Obj::CMovingObject *) objSearch.FindFirstObjectOfType( mp_manager->GetRefObjectList(), type );
	while ( pObj )
	{
		if ( pObj != this )
		{
			if ( pBox )
			{
				if ( ObjInSiteBox( pObj, pBox ) )
				{
					return true;
				}
			}
			else if ( Mth::DistanceSqr( pObj->m_pos, m_pos ) < radiusSqr )
			{
				return true;
			}
		}
		pObj = (Obj::CMovingObject *) objSearch.FindNextObjectOfType( );
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovingObject::ObjectWithinRange( Script::CStruct* pParams, Script::CScript* pScript, SiteBox* pBox )
{
	float radiusSqr = 0.0f;
	
	if ( !pBox )
	{
		if( !( pParams->GetFloat( 0xc48391a5, &radiusSqr ) ) )//"radius"
		{
			Dbg_MsgAssert( 0,( "\n%s\nNo radius specified in Obj_*InRadius.", pScript->GetScriptInfo( ) ));
			return false;
		}
		radiusSqr = FEET_TO_INCHES( radiusSqr );
		radiusSqr *= radiusSqr;
	}

	Script::CArray* pArray=NULL;
	uint32 typeChecksum;
	if ( pParams->GetChecksum( 0x7321a8d6, &typeChecksum ) ) // "type"
	{
		return ( ObjTypeInRange( pParams, pScript, radiusSqr, pBox, typeChecksum ) );
	}
	else if ( pParams->GetArray( 0x7321a8d6, &pArray, Script::ASSERT ) ) // "type"
	{
		Dbg_MsgAssert( pArray->GetType( ) == ESYMBOLTYPE_NAME,( "\n%s\nObjectWithinRange: Array must be of names",pScript->GetScriptInfo()));
		uint32 i;
		for ( i = 0; i < pArray->GetSize( ); ++i )
		{
			if ( ObjTypeInRange( pParams, pScript, radiusSqr, pBox, pArray->GetChecksum( i ) ) )
			{
				return true;
			}
		}
		return false;		
	}

	// find a named object:
	uint32 nameChecksum;
	if ( pParams->GetChecksum( 0xa1dc81f9, &nameChecksum ) ) // "name"
	{
		int nodeNum = SkateScript::FindNamedNode( nameChecksum );
		return ( ObjectFromNodeWithinRange( nodeNum, radiusSqr, pBox ) );
	}
    const char* pPrefix;
	if ( pParams->GetText( 0x6c4e7971, &pPrefix ) ) // checksum 'prefix'
	{
		// Create with a prefix specified:
		uint16 numNodes = 0;
		const uint16* pMatchingNodes = SkateScript::GetPrefixedNodes( pPrefix, &numNodes );
		int i;
		for ( i = 0; i < numNodes; i++ )
		{
			if ( ObjectFromNodeWithinRange( pMatchingNodes[ i ], radiusSqr, pBox ) )
			{
				//printf("Oof old\n");
				return true;
			}
		}
		return false;
	}
	
//	Dbg_Message( "\n%s\nWarning: Unprocessed radius ( or site box ) check... unrecognized syntax. needs 'type', 'name' or 'prefix'", pScript->GetScriptInfo( ) );
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovingObject::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{   
	switch (Checksum)
	{				
		// @script | Obj_LookAtObject | 
		// Returns true if the object will rotate itself to point in the required direction. Ie, if
		// the object is already pointing the right way, the command will return false.
        // @parmopt name | lockAxis | | can lock any two axes (LOCK_X, LOCK_Y, LOCK_Z)
        // @parmopt float | time | vel/acceleration | in seconds
        // @parmopt float | speed | 360  | angular velocity in degrees per second
        // @parmopt float | acceleration | 0 | angular acceleration in degrees per second
        // per second
        // @parmopt name | name | | name of object
        // @parmopt name | type | | type of object to look at - will look at closest if 
        // there are several in the world
        case 0x33e04032:  // Obj_LookAtObject
			return LookAtObject_Init( pParams, pScript );
			break;

		case 0x37185d8a:  // Obj_AngleToNearestSkaterGreaterThan
		{
			int type = SKATE_TYPE_SKATER;
			Obj::CCompositeObject* pClosestObject = GetClosestObjectOfType( type );
			if ( !pClosestObject )
			{
				Dbg_Message( "\n%s\nWarning:  Checking angle to skater that doesn't exist.", pScript->GetScriptInfo() );
				return false;
				break;
			}

			float threshold;
			pParams->GetFloat( NONAME, &threshold, Script::ASSERT );
			Dbg_MsgAssert( threshold >= 0, ( "Negative angle passed to Obj_AngleToNearestSkaterGreaterThan" ) );
			threshold = DEGREES_TO_RADIANS( threshold );

			Mth::Vector pathHeading = pClosestObject->m_pos - m_pos;
			float test = Mth::GetAngle( m_matrix, pathHeading, 2, 1 );
			if ( fabs( test ) > threshold )
			{
				return true;
				break;
			}
			return false;
			break;
		}

        // @script | Obj_ObjectInRadius | 
        // @parmopt name | name | | use one of: name, prefix, or type
        // @parmopt name | prefix | | use one of: name, prefix, or type
        // @parmopt name | type | | use one of: name, prefix, or type
        // (type values are skater, ped, car)
        // @parm float | radius | radius in feet
		case 0x67660fe5:  // Obj_ObjectInRadius
			return ( ObjectWithinRange( pParams, pScript ) );
			break;

        // @script | Obj_ObjectInRect | 
        // @parmopt float | angle | 0 | in degrees - default is  
        // along the direction the object is facing
        // @parmopt float | height | (height of object) | 
        // @parmopt float | distance | (length of object) | in feet - 
        // offset by half of the object in the direction determined by the angle
        // @parmopt float | width | (width of object) | in feet
        // @parmopt vector | offset | | in feet - the rectangle by default will be located
        // sticking out of the XZ plane (or XY plane in Max coords ),
        // rotated by number of degrees in angle parameter
		// @flag debug | just a parameter to turn on debugging info, 
        // to quickly set up rectangles on an object.
        // @parmopt name | name | | use one of: name, prefix, or type
        // @parmopt name | prefix | | use one of: name, prefix, or type
        // @parmopt name | type | | use one of: name, prefix, or type
        // (type values are skater, ped, car)
        case 0x6520b902: // Obj_ObjectInRect
		{
			return ( ObjectWithinRect( pParams, pScript ) );
			break;
		}

        // @script | Obj_ObjectNotInRect | 
        // @parmopt float | angle | 0 | in degrees - default is  
        // along the direction the object is facing
        // @parmopt float | height | (height of object) | 
        // @parmopt float | distance | (length of object) | in feet - 
        // offset by half of the object in the direction determined by the angle
        // @parmopt float | width | (width of object) | in feet
        // @parmopt vector | offset | | in feet - the rectangle by default will be located
        // sticking out of the XZ plane (or XY plane in Max coords ),
        // rotated by number of degrees in angle parameter
        // @flag debug | just a parameter to turn on debugging info, 
        // to quickly set up rectangles on an object.
        // @parmopt name | name | | use one of: name, prefix, or type
        // @parmopt name | prefix | | use one of: name, prefix, or type
        // @parmopt name | type | | use one of: name, prefix, or type
        // (type values are skater, ped, car)
		case 0x3b6b66c3: // Obj_ObjectNotInRect
			return ( !ObjectWithinRect( pParams, pScript ) );
			break;


		// @script | Obj_SetBodyShape | Sets the body shape for this object
        // @parm structure | body_shape | a list of per-bone scales (see scaling.q for format)
		case 0x802a7600: // Obj_SetBodyShape
		{
			Script::CStruct* pBodyShapeStructure;
			pParams->GetStructure( CRCD(0x812684ef,"body_shape"), &pBodyShapeStructure, true );

			if ( GetModel() )
			{
				Gfx::CSkeleton* pSkeleton = GetSkeleton();
				Dbg_Assert( pSkeleton );
				pSkeleton->ApplyBoneScale( pBodyShapeStructure );
			
				Mth::Vector theScale( 1.0f, 1.0f, 1.0f );
				if ( Gfx::GetScaleFromParams( &theScale, pBodyShapeStructure ) )
				{
					GetModel()->SetScale( theScale );
				}
			}			
		}
		break;

		default:
			return ( CCompositeObject::CallMemberFunction( Checksum, pParams, pScript ) );
			break;
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCompositeObject* CMovingObject::GetClosestObjectOfType( int type )
{
	Search objSearch;
	CCompositeObject *pObj;		
	pObj = (CCompositeObject *) objSearch.FindFirstObjectOfType( mp_manager->GetRefObjectList(), type );
	if ( pObj == this )
	{
		if ( !( pObj = (CCompositeObject*) objSearch.FindNextObjectOfType( ) ) )
		{
			return ( NULL );
		}
	}
	if ( !( pObj ) )
	{
		return ( NULL );
	}
	float closestDist = FEET_TO_INCHES( 666.0f * 666.0f );
	float dist;
	CCompositeObject* pClosestObj = pObj;
	while ( pObj )
	{
		dist = Mth::Distance( pObj->m_pos, m_pos );
		if ( ( pObj != this ) && ( dist < closestDist ) )
		{
			closestDist = dist;
			pClosestObj = pObj;
		}
		pObj = (CCompositeObject*) objSearch.FindNextObjectOfType( );
	}
	return ( pClosestObj );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj
