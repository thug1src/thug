//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       AnimChannel.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/13/01
//****************************************************************************

// Current state of an animation.  There should be no
// references to skeletons or geometry;  this is purely 
// a time-keeping class).

#ifndef __GFX_ANIMCHANNEL_H
#define __GFX_ANIMCHANNEL_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

#include <core/list.h>

#include <gfx/bonedanimtypes.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Script
{
	class CStruct;
}
						
namespace Gfx
{

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

class CAnimChannel : public Lst::Node<CAnimChannel>
{
public:
    CAnimChannel();
    virtual ~CAnimChannel();

    virtual void        Reset();
    virtual void        Update();
	uint32				GetCurrentAnim();
	
    float               GetCurrentAnimTime( void );
    void                GetAnimTimes( float*, float*, float* );
    bool                IsAnimComplete( void );
    bool                IsLoopingAnim( void );
    void                SetAnimSpeed( float );
    float               GetAnimSpeed( void );
	void				ReverseDirection( void );
    void                SetLoopingType( EAnimLoopingType );
	EAnimLoopingType	GetLoopingType() const {
		return m_loopingType;
	}
    virtual void        PlaySequence( uint32 animName, float start_time, float end_time, EAnimLoopingType loop_type, float blend_period, float speed );
	void				AddTime( float incVal );

	virtual void		GetDebugInfo( Script::CStruct* p_info );

	// made this public while i'm changing over to new anim controllers

//protected:
public:
    float				get_new_anim_time( void );
    
    float				m_startTime;
    float				m_currentTime;
    float				m_endTime;
    float				m_animSpeed;
    EAnimDirection		m_direction;
    EAnimLoopingType	m_loopingType;
    bool				m_animComplete;
	uint32				m_animName;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // namespace Gfx

#endif	// __GFX_ANIMCHANNEL_H
