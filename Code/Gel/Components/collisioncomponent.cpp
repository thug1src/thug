//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       CollisionComponent.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/31/2002
//****************************************************************************

#include <gel/components/Collisioncomponent.h>

#include <gel/assman/assman.h>
#include <gel/collision/collision.h>
#include <gel/collision/movcollman.h>

#include <gel/components/modelcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/checksum.h>
							
#include <gfx/nx.h>
#include <gfx/nxhierarchy.h>
#include <gfx/nxmesh.h>
#include <gfx/nxmodel.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// This static function is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
CBaseComponent* CCollisionComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CCollisionComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCollisionComponent::CCollisionComponent() : CBaseComponent()
{
	SetType( CRC_COLLISION );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCollisionComponent::~CCollisionComponent()
{
	if ( mp_collision )
	{
		Nx::CMovableCollMan::sRemoveCollision( mp_collision );
		// We don't want to do this after we start the PIP loading
		delete mp_collision;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCollisionComponent::InitFromStructure( Script::CStruct* pParams )
{	
	Dbg_MsgAssert( pParams, ( "No node data?" ) );
	Dbg_MsgAssert( !mp_collision, ( "Set up collision twice?" ) );

	uint32 col_type_checksum = Nx::vCOLL_TYPE_NONE;
	pParams->GetChecksum( CRCD(0x2d7e583b,"CollisionMode"), &col_type_checksum );
	
	Nx::CollType col_type = Nx::CCollObj::sConvertTypeChecksum( col_type_checksum );
	if ( col_type != Nx::vCOLL_TYPE_NONE )
	{
		uint32 class_checksum = 0;
		pParams->GetChecksum("class", &class_checksum);
		if ( class_checksum == CRCD(0xb7b3bd86,"LevelObject") )
		{
			uint32 name;
			pParams->GetChecksum(CRCD(0xa1dc81f9,"name"), &name, Script::ASSERT);
			Nx::CSector *p_sector = Nx::CEngine::sGetSector(name);
			Dbg_MsgAssert( p_sector, ( "WARNING: sGetSector(0x%x) returned NULL (%s)\n", name, Script::FindChecksumName(name) ) );
			Nx::CCollObjTriData *p_coll_tri_data = p_sector->GetCollSector()->GetGeometry();
			InitCollision(col_type, p_coll_tri_data);   
		}
		else
		{
			InitCollision(col_type);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCollisionComponent::InitCollision( Nx::CollType type, Nx::CCollObjTriData *p_coll_tri_data )
{		
	Dbg_Assert( !mp_collision );

	if ( p_coll_tri_data )
	{
		mp_collision = Nx::CCollObj::sCreateMovableCollision(type, p_coll_tri_data, 1, GetObject());
	} 
	else
	{
		Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( GetObject() );
		Dbg_MsgAssert( pModelComponent, ( "Initing collision on something with no model component" ) );

		Nx::CModel* pModel = pModelComponent->GetModel();
		Dbg_MsgAssert( pModel, ( "Initing collision on something with no model" ) );

		Ass::CAssMan * ass_man = Ass::CAssMan::Instance();
		Nx::CMesh* pMesh = (Nx::CMesh*)ass_man->GetAsset(pModel->GetFileName(), false);
		if (pMesh)
		{
			mp_collision = Nx::CCollObj::sCreateMovableCollision(type, pMesh->GetCollisionTriDataArray(), pMesh->GetCollisionTriDataArraySize(), GetObject());
		}
		else
		{
			mp_collision = Nx::CCollObj::sCreateMovableCollision(type, NULL, 0, GetObject());
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Nx::CCollObj* CCollisionComponent::GetCollision() const
{
	return mp_collision;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCollisionComponent::Teleport()
{
	// GJ:  the collision component's Update()
	// function doesn't really require finalization
	// right now, but having this assert in here
	// might help catch future errors
	Dbg_MsgAssert( GetObject()->IsFinalized(), ( "Has not been finalized!  Tell Gary!" ) );

	Update();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCollisionComponent::Update()
{
//	GJ:  On THPS4, we did the following test first before
//	updating the collision;  I think this will be handled
//	by the suspend component, so we shouldn't have to do
//  this here...
//	if ( (m_object_flags & vINVISIBLE) && !initialize )
//	{
//		return;
//	}

	// get the collision object
	Nx::CCollObj* p_coll = GetCollision();
	
	// Update collision
	// Might be problem with things being LODed off, but still needing collision?
	if ( p_coll )
	{
		// GJ TODO:  Somehow remove the collision object's dependency on the model!

		Nx::CModel*	p_model = NULL;
		Nx::CHierarchyObject* p_hierarchy = NULL;
		Obj::CModelComponent* p_model_component = GetModelComponentFromObject( GetObject() );
		if ( p_model_component )
		{
			p_model = p_model_component->GetModel();
			Dbg_MsgAssert( p_model, ( "No model?!?" ) );
			p_hierarchy = p_model->GetHierarchy();
		}

		if (p_hierarchy)
		{
			Mth::Vector translation(p_hierarchy->GetSetupMatrix()[W]);
			translation[W] = 0.0f;		// turn into vector
			p_hierarchy->GetSetupMatrix().Transform(translation);		// Re-orients the translation

			p_coll->SetWorldPosition( GetObject()->GetPos() + translation );
			p_coll->SetOrientation( p_hierarchy->GetSetupMatrix() * GetObject()->GetDisplayMatrix() );
		}
		else
		{
			p_coll->SetWorldPosition( GetObject()->GetPos() );
			p_coll->SetOrientation( GetObject()->GetDisplayMatrix() );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent::EMemberFunctionResult CCollisionComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
//    Dbg_Assert( 0 );

    return CBaseComponent::MF_NOT_EXECUTED;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}
