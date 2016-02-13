//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SetDisplayMatrixComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  8/6/3
//****************************************************************************

#include <gel/components/setdisplaymatrixcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSetDisplayMatrixComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSetDisplayMatrixComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSetDisplayMatrixComponent::CSetDisplayMatrixComponent() : CBaseComponent()
{
	SetType( CRC_SETDISPLAYMATRIX );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSetDisplayMatrixComponent::~CSetDisplayMatrixComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSetDisplayMatrixComponent::InitFromStructure( Script::CStruct* pParams )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSetDisplayMatrixComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSetDisplayMatrixComponent::Update()
{
	GetObject()->SetDisplayMatrix(GetObject()->GetMatrix());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSetDisplayMatrixComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSetDisplayMatrixComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSetDisplayMatrixComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
}
	
}
