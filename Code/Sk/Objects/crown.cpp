/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		skate4													**
**																			**
**	Module:			CCrown			 										**
**																			**
**	File name:		Crown.cpp												**
**																			**
**	Created by:		08/06/01	-	SPG										**
**																			**
**	Description:	King of the Hill Crown Object							**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/objects/crown.h>

#include <core/defines.h>

#include <gel/objman.h>
#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>

#include <gel/components/modelcomponent.h>
#include <gel/components/motioncomponent.h>
#include <gel/components/suspendcomponent.h>
					 
#include <gfx/gfxutils.h>
#include <gfx/debuggfx.h>
#include <gfx/nxmodel.h>
#include <gfx/skeleton.h>

#include <sk/objects/skater.h>
#include <sk/modules/skate/skate.h>
#include <sk/scripting/nodearray.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Obj
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

const float CCrown::vCROWN_RADIUS = FEET( 8.0f );

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCrown::DoGameLogic()
{
	if ( m_on_king )
	{
		Gfx::CSkeleton* pSkeleton;
		pSkeleton = mp_king->GetSkeleton();
		Dbg_Assert( pSkeleton );

		pSkeleton->GetBoneMatrix( CRCD(0xddec28af,"Bone_Head"), &m_matrix );

		m_matrix.RotateXLocal( 90.0f );

		// raise the crown by 1 foot
		//m_matrix.TranslateLocal( Mth::Vector( 0.0f, FEET(1.0f), 0.0f, 0.0f ) );
		m_matrix.TranslateLocal( Mth::Vector( 0.0f, 0.0f, -FEET(1.5f), 0.0f ) );

		Mth::Vector crownOffset = m_matrix[Mth::POS];
		m_matrix[Mth::POS] = Mth::Vector( 0.0f, 0.0f, 0.0f, 1.0f );
		
// for some reason, the below doesn't work as expected.
// so instead, use the skater's root matrix for the orientation

		m_matrix = m_matrix * mp_king->GetMatrix();

//		m_matrix = mp_king->GetMatrix();

		m_pos = mp_king->GetMatrix().Transform( crownOffset );
		m_pos += mp_king->m_pos;
		
//		Gfx::AddDebugStar( m_pos, 10, 0x0000ffff, 1 );
	}

}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

CCrown::CCrown( void )
{
	SetID( Script::GenerateCRC( "crown_object" ));

#if 0	
	// GJ:  Why is the crown not registered here, instead of in InitCrown?
	p_obj_man->RegisterObject( *this );
#endif
	
    m_on_king = false;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCrown::~CCrown( void )
{	
	RemoveFromWorld();

	m_on_king = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCrown::PlaceOnKing( CSkater* king )
{   
	mp_king = king;
    
    m_on_king = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCrown::RemoveFromWorld( void )
{
	m_on_king = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCrown::RemoveFromKing( void )
{
	m_on_king = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCrown::OnKing( void )
{
	return m_on_king;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCrown::InitCrown( CGeneralManager* p_obj_man, Script::CStruct* pNodeData )
{   
	p_obj_man->RegisterObject( *this );
	
	MovingObjectCreateComponents();

	// Create an empty model 
	Dbg_MsgAssert( GetModelComponent() == NULL, ( "Model component already exists" ) );
	Script::CStruct* pModelStruct = new Script::CStruct;
	pModelStruct->AddChecksum( CRCD(0xb6015ea8,"component"), CRC_MODEL );
	this->CreateComponentFromStructure(pModelStruct, NULL);
	delete pModelStruct;
	
	MovingObjectInit( pNodeData, p_obj_man );
	SetID( Script::GenerateCRC( "crown_object" ));
	
	Obj::CSuspendComponent* pSuspendComponent = GetSuspendComponent();
	Dbg_MsgAssert( pSuspendComponent, ( "No suspend component?" ) );
	pSuspendComponent->m_lod_dist[0] = 1000000;
	pSuspendComponent->m_lod_dist[1] = 1000001;
	pSuspendComponent->m_lod_dist[2] = 1000002;  // Note, this is used as a magic number to turn off the high level culling
	pSuspendComponent->m_lod_dist[3] = 1000003;

	Str::String fullModelName;
//	fullModelName = Gfx::GetModelFileName("gameobjects/skate/letter_a/letter_a.mdl", ".mdl");
	fullModelName = Gfx::GetModelFileName("crown", ".mdl");
	// Model file name should look like this:  "models/testcar/testcar.mdl"
	Dbg_Printf( "Initializing Crown using file %s\n", fullModelName.getString() );
	GetModel()->AddGeom( fullModelName.getString(), 0, true );
	
	GetModel()->SetActive( true );
	SkateScript::GetPosition( pNodeData, &m_pos );
	Dbg_Printf( "Placing crown at %f %f %f\n", m_pos[X], m_pos[Y], m_pos[Z] );

	Finalize();

	GetModelComponent()->FinalizeModelInitialization();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // namespace Obj

