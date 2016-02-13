/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:																**
**																			**
**	Module:					      			 								**
**																			**
**	File name:																**
**																			**
**	Created by:		rjm										                **
**																			**
**	Description:															**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <sk/ParkEditor/EdRail.h>


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

DefinePoolableClass(Ed::RailPoint)
DefinePoolableClass(Ed::RailString)

namespace Ed
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

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

RailPoint::RailPoint() :
	m_pos(0.0f, 0.0f, 0.0f, 0.0f)
{
	mp_next = NULL;
}




RailPoint::~RailPoint() 
{
}




RailString::RailString()
{
	mp_pointList = NULL;
	m_numPoints = 0;
	mp_next = NULL;

	m_isLoop = false;
}




RailString::~RailString()
{
}




void RailString::AddPoint(RailPoint *pPoint)
{
	if (!mp_pointList)
		mp_pointList = pPoint;
	else
		mp_lastPoint->mp_next = pPoint;
	mp_lastPoint = pPoint;
	m_numPoints++;
}




void RailString::Destroy()
{
	RailPoint *pPoint = mp_pointList;
	while(pPoint)
	{
		RailPoint *pNext = pPoint->mp_next;
		mp_parentSet->DestroyRailPoint(pPoint);
		pPoint = pNext;
	}
	mp_pointList = NULL;
	mp_lastPoint = NULL;
	m_numPoints = 0;
}




int	RailString::CountLinkedPoints()
{
	

	if (m_isLoop)
		return Count();

	int count = 0;
	for (RailPoint *pPoint = mp_pointList; pPoint != NULL; pPoint = pPoint->mp_next)
		count++;

	Dbg_MsgAssert(count > 1, ("invalid number of linked rail points in a rail (%d) need 2 or more",count));
	return count - 1;
}




void RailString::CopyOffsetAndRot(RailString *pSource, Mth::Vector &pos, int rot, uint32 piece_id)
{
	RailPoint *pSourcePt = pSource->GetList();

	while(pSourcePt)
	{
		RailPoint *pOut = mp_parentSet->CreateRailPoint();
		float x = 0.0f, z = 0.0f;
		switch(rot)
		{
			case 0:
				x =  pSourcePt->m_pos.GetX();
				z = pSourcePt->m_pos.GetZ();
// GJ:  No longer need to negate the Z component, now that we're using new-style "Pos" vectors!
//				z = -pSourcePt->m_pos.GetZ();
				break;
			case 1:
				x = pSourcePt->m_pos.GetZ();
				z = -pSourcePt->m_pos.GetX();
// GJ:  No longer need to negate the Z component, now that we're using new-style "Pos" vectors!
//				z = pSourcePt->m_pos.GetX();
				break;
			case 2:
				x = -pSourcePt->m_pos.GetX();
				z = -pSourcePt->m_pos.GetZ();
// GJ:  No longer need to negate the Z component, now that we're using new-style "Pos" vectors!
//				z = pSourcePt->m_pos.GetZ();
				break;
			case 3:
				x =  -pSourcePt->m_pos.GetZ();
				z =  pSourcePt->m_pos.GetX();
// GJ:  No longer need to negate the Z component, now that we're using new-style "Pos" vectors!
//				z =  -pSourcePt->m_pos.GetX();
				break;
		}
		float y = pSourcePt->m_pos.GetY() +	pos.GetY();
		x += pos.GetX();
		z += pos.GetZ();
// GJ:  No longer need to negate the Z component, now that we're using new-style "Pos" vectors!
		pOut->m_pos.Set(x, y, z);
//		pOut->m_pos.Set(x, y, -z);
		pOut->m_type = pSourcePt->m_type;
		pOut->m_objectId = piece_id;

		AddPoint(pOut);

		pSourcePt = pSourcePt->GetNext();
	}

	m_id = pSource->m_id;
	m_isLoop = pSource->m_isLoop;
}




RailSet::RailSet()
{
	mp_stringList = NULL;
	m_numStrings = 0;

	mp_pointAllocator = NULL;
	mp_stringAllocator = NULL;
}




RailSet::~RailSet()
{
}




void RailSet::AddString(RailString *pString)
{
	if (!mp_stringList)
		mp_stringList = pString;
	else
		mp_lastString->mp_next = pString;
	mp_lastString = pString;
	m_numStrings++;
}




void RailSet::Destroy()
{
	RailString *pString = mp_stringList;
	while(pString)
	{
		RailString *pNext = pString->mp_next;
		pString->Destroy();
		DestroyRailString(pString);
		pString = pNext;
	}
	mp_stringList = NULL;
	mp_lastString = NULL;
	m_numStrings = 0;
}




int RailSet::CountPoints()
{
	int num_points = 0;
	RailString *pString = mp_stringList;
	while (pString)
	{
		num_points += pString->Count();
		pString = pString->GetNext();
	}

	return num_points;
}




RailString *RailSet::GetString(uint32 id, int requested_num)
{
	RailString *pString = mp_stringList;
	int num = 0;
	while(pString)
	{
		if (pString->m_id == id)
		{
			if (num == requested_num)
				return pString;
			else
				num++;
		}
		pString = pString->GetNext();
	}
	return NULL;
}




void RailSet::SetupAllocators(int num_points, int num_strings, bool in_set)
{
	if (in_set)
		strcpy(m_setName, "In set");
	else
		strcpy(m_setName, "Out set");
	char point_name[64];
	sprintf(point_name, "%s RailPoint", m_setName);
	char string_name[64];
	sprintf(string_name, "%s RailString", m_setName);
	
	Dbg_MsgAssert(mp_pointAllocator==NULL,("mp_pointAllocator not NULL"));
	mp_pointAllocator = new Mem::CCompactPool(sizeof(RailPoint), num_points, point_name);
	Dbg_MsgAssert(mp_stringAllocator==NULL,("mp_stringAllocator not NULL"));
	mp_stringAllocator = new Mem::CCompactPool(sizeof(RailString), num_strings, string_name);
}




void RailSet::FreeAllocators()
{
	if (mp_stringAllocator)
	{
		delete mp_stringAllocator;
		mp_stringAllocator=NULL;
	}
	if (mp_pointAllocator)
	{
		delete mp_pointAllocator;
		mp_pointAllocator=NULL;
	}	
}




RailPoint *RailSet::CreateRailPoint()
{
	
	Dbg_Assert(mp_pointAllocator);

	RailPoint::SAttachPool(mp_pointAllocator);
	RailPoint *pNew = new RailPoint;
	RailPoint::SRemovePool();
	return pNew;
}




RailString *RailSet::CreateRailString()
{
	
	Dbg_Assert(mp_stringAllocator);

	RailString::SAttachPool(mp_stringAllocator);
	RailString *pString = new RailString;
	RailString::SRemovePool();
	pString->mp_parentSet = this;
	return pString;
}




void RailSet::DestroyRailPoint(RailPoint *pPoint)
{
	
	Dbg_Assert(mp_pointAllocator);

	RailPoint::SAttachPool(mp_pointAllocator);
	delete pPoint;
	RailPoint::SRemovePool();
}




void RailSet::DestroyRailString(RailString *pString)
{
	
	Dbg_Assert(mp_stringAllocator);

	RailString::SAttachPool(mp_stringAllocator);
	delete pString;
	RailString::SRemovePool();
}




} // namespace Ed




