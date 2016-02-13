/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		            											**
**																			**
**	Module:			              			 								**
**																			**
**	File name:		                    									**
**																			**
**	Created by:     rjm        				    			                **
**																			**
**	Description:					                                        **
**																			**
*****************************************************************************/

#ifndef __SK_PARKEDITOR_EDRAIL_H
#define __SK_PARKEDITOR_EDRAIL_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/math.h>
#include <sys/mem/CompactPool.h>
#include <sys/mem/Poolable.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace Ed
{

						


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class  RailPoint  : public Mem::CPoolable<RailPoint>
{
	

public:
//	enum RailType
//	{
//		vMETAL,
//		vWOOD,
//	};

	typedef		uint32		RailType;
	
	RailPoint();
	~RailPoint();

	RailPoint *								GetNext() {return mp_next;}

	Mth::Vector								m_pos;
	RailType								m_type;
	// id of associated Piece (and therefore trick object)
	uint32									m_objectId;
	RailPoint *								mp_next;
};




class RailSet;
class  RailString  : public Mem::CPoolable<RailString>
{
	

public:
	RailString();
	~RailString();

	void									AddPoint(RailPoint *pPoint);
	void									Destroy();
	RailPoint *								GetList() {return mp_pointList;}
	RailString *							GetNext() {return mp_next;}
	int										Count() {return m_numPoints;}
	int										CountLinkedPoints();

	void									CopyOffsetAndRot(RailString *pString, Mth::Vector &pos, int rot, uint32 piece_id);
	
	uint8									m_isLoop;
	uint32									m_id; // of associated piece
	
	RailPoint *								mp_pointList;
	RailPoint *								mp_lastPoint;
	uint16									m_numPoints;
	RailString *							mp_next;

	RailSet *								mp_parentSet;
};




class  RailSet  : public Spt::Class
{
	

public:
	RailSet();
	~RailSet();

	void									AddString(RailString *pString);
	void									Destroy();
	RailString *							GetList() {return mp_stringList;}
	int										Count() {return m_numStrings;}
	int										CountPoints();

	RailString *							GetString(uint32 id, int num);
	
	void									SetupAllocators(int num_points, int num_strings, bool in_set);
	void									FreeAllocators();
	RailPoint *								CreateRailPoint();
	RailString *							CreateRailString();
	void									DestroyRailPoint(RailPoint *pPoint);
	void									DestroyRailString(RailString *pString);

protected:	
	RailString *							mp_stringList;
	RailString *							mp_lastString;
	uint16									m_numStrings;

	Mem::CCompactPool *						mp_pointAllocator;
	Mem::CCompactPool *						mp_stringAllocator;

	char									m_setName[32];
};




/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


} // namespace Ed

#endif	// __SK_PARKEDITOR_EDRAIL_H


