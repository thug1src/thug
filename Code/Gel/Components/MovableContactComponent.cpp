//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       MovableContactComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  5/19/3
//****************************************************************************

#include <gel/components/movablecontactcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

#include <sk/engine/feeler.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CMovableContactComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CMovableContactComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMovableContactComponent::CMovableContactComponent() : CBaseComponent()
{
	SetType( CRC_MOVABLECONTACT );
	
	mp_contact = NULL;
	
	m_lost_contact_timestamp = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CMovableContactComponent::~CMovableContactComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMovableContactComponent::InitFromStructure( Script::CStruct* pParams )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMovableContactComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CMovableContactComponent::Update()
{
	Suspend(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CMovableContactComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
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

void CMovableContactComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CMovableContactComponent::GetDebugInfo"));
	
	if (mp_contact && mp_contact->GetObject())
	{
		p_info->AddChecksum(CRCD(0xb39d19c7, "contact"), mp_contact->GetObject()->GetID());
	}

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CMovableContactComponent::CheckForMovableContact ( CFeeler& feeler )
{
	// we might now be in contact with something new
	if (feeler.IsMovableCollision())
	{
		if (HaveContact())
		{
			if (mp_contact->GetObject() != feeler.GetMovingObject())
			{
				LoseAnyContact();
			}
		}
		
		if (!HaveContact())
		{
			// only create a new contact point if there was not one before, or if we just deleted one
			mp_contact = new CContact(feeler.GetMovingObject());
			return true;
		}
	}
	else
	{
		// when we land on the ground, then we can't be in contact with anything, so delete any existing contact
		if (HaveContact())
		{
			LoseAnyContact();
		}
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCompositeObject* CMovableContactComponent::CheckInsideObjectsToTop ( Mth::Vector& pos )
{
	Mth::Vector top;
	CCompositeObject *p_object = find_inside_object(pos, top);
	if (!p_object) return NULL;
	
	pos = top;
	pos[Y] += 1.0f;
	return p_object;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCompositeObject* CMovableContactComponent::CheckInsideObjects ( Mth::Vector& pos, const Mth::Vector& safe_pos )
{
	// not movable contact functionality per say, but useful a service related to movable collision nonetheless
	
	// Check to see if the skater is inside any moving object and move him outside
	// Algorithm
	// start at a point 100 feet above the skater
	// check collision down to the skater
	// while there is a collision if skater has a moving object collision in the 100 feet above him
	// and no collisions (with that object) going up to that point, then we are inside the object

	Mth::Vector top;
	CCompositeObject *p_object = find_inside_object(pos, top);
	if (!p_object) return NULL;
	
	// check if the last position is now fine...
	Mth::Vector dummy;
	if (!find_inside_object(safe_pos, dummy))
	{
		// first choice: move back to old pos
		pos = safe_pos;
	}
	else
	{
		// recheck last position here, like adding in the contact movement
	
		Mth::Vector test = pos + p_object->GetVel() * Tmr::FrameLength() * 2.0f;  
		
		if (!find_inside_object(test, dummy))
		{
			// first choice: move back to old pos
			pos = test;
		}
		else
		{
			// last resort, snap up 
			pos = top;
			
			// when snapping up, put him an inch above the object; otherwise, you will not hit it next frame, as the point will be in the plane
			pos[Y] += 1.0f;
		}
	}
	
	return p_object;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCompositeObject* CMovableContactComponent::find_inside_object ( const Mth::Vector& pos, Mth::Vector& p_return_pos )
{
	// 	Given a position (pos), then find out if this position is inside this moving object
	//  Assumes anything within 6 inches below the top of the object is actually outside as the code assumes 

	// create a line that goes from 400 inches above this point to six inches above
	CFeeler	down_feeler;
	down_feeler.m_start = pos;
	down_feeler.m_start[Y] += 400.0f;
	down_feeler.m_end = pos;
	down_feeler.m_end[Y] += 6.0f;
	
	CFeeler	up_feeler;
	int loop_detect = 0;
	while (true)
	{
		// return on final builds to handle highly obscure cases I've not forseen....
		if (++loop_detect == 10) return NULL;
		
		// nothing above
		if (!down_feeler.GetCollision()) return NULL;
		
		// something above, but not movable
		if (!down_feeler.IsMovableCollision()) return NULL;
		
		// something movable above
		
		// Now we get the collision point, and check UP to that point from m_pos to a point just above that		
		up_feeler.m_start = pos;
		up_feeler.m_end = down_feeler.GetPoint();
		up_feeler.m_end[Y] += 0.1f;
		
		if (!up_feeler.GetCollision())
		{
			p_return_pos = down_feeler.GetPoint();
			return down_feeler.GetMovingObject(); 
		}
		else
		{
			down_feeler.m_start = down_feeler.GetPoint();
			down_feeler.m_start[Y] -= 0.1f;
		}
	}
}	

}
