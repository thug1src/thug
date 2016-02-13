//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       CustomAnimKey.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  02/08/2002
//****************************************************************************

#ifndef __GFX_CUSTOMANIMKEY_H
#define __GFX_CUSTOMANIMKEY_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

#include <core/list/node.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Obj
{
	class CObject;
}

namespace Script
{
	class CStruct;
}

namespace Gfx
{
	enum RangeFlags
	{
		mSTART_INCLUSIVE = ( 1 << 0 ),
		mSTART_EXCLUSIVE = ( 1 << 1 ),
		mEND_INCLUSIVE = ( 1 << 2 ),
		mEND_EXCLUSIVE = ( 1 << 3 ),
	};

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

// interface to all the custom keys
// (the custom keys are defined in the CPP file)
// there should be no allocation going on in these keys

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// class CCustomAnimKey
class CCustomAnimKey : public Lst::Node<CCustomAnimKey>
{
public:
	CCustomAnimKey( int time );
	virtual bool	WithinRange( float startFrame, float endFrame, bool inclusive = false );
	void			SetActive( bool active );

public:
	bool			ProcessKey( Obj::CObject* pObject );

protected:
	virtual bool	process_key( Obj::CObject* pObject ) = 0;

protected:
	int				m_frame;
	bool			m_active;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// class CEventKey
class CEventKey : public CCustomAnimKey
{
public:
	CEventKey( int time, uint32 eventType, Script::CStruct* pParams );
	virtual ~CEventKey();

public:
	virtual	bool		process_key( Obj::CObject* pObject );

protected:
	uint32				m_eventType;
	Script::CStruct*	mp_eventParams;
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

CCustomAnimKey* ReadCustomAnimKey( uint8** pData );

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // namespace Gfx

#endif	// __GFX_CUSTOMANIMKEY_H



