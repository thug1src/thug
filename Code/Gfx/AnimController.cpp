//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       AnimChannel.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/13/01
//****************************************************************************

// Current state of an animation.  There should be no
// references to skeletons or geometry;  this is purely 
// a time-keeping class).

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/animcontroller.h>
                                  
#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>
								   
#include <sys/timer.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Gfx
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
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CAnimChannel::get_new_anim_time( void )
{
#ifdef __NOPT_ASSERT__	
	float old_time = m_currentTime;
#endif

	float new_time=0;
	float new_hold_time=0;
	float new_loop_time=0;

	m_animComplete = false;

	if ( m_loopingType == LOOPING_WOBBLE )
	{
		Dbg_MsgAssert( 0, ( "Was not expecting a wobble looping type in this controller" ) );
		return 0.0f;
	}
	
	if ( m_direction == ANIM_DIR_FORWARDS )
	{
		new_time = m_currentTime + ( m_animSpeed * Tmr::FrameRatio() );
		m_animComplete = new_time > m_endTime;
		
		if (m_startTime == m_endTime)
		{
			new_loop_time = m_endTime;
		}
		else
		{
			new_loop_time = m_startTime + ( new_time - m_endTime );
			while ( new_loop_time > m_endTime )
			{
				// when the FrameRatio() > 1.0f, it's possible that
				// we need to subtract more than once, esp. with short anims
				new_loop_time = m_startTime + ( new_loop_time - m_endTime );
			}
		}
		new_hold_time = m_endTime;
	}
	else if ( m_direction == ANIM_DIR_BACKWARDS )
	{
		new_time = m_currentTime - ( m_animSpeed * Tmr::FrameRatio() );
		m_animComplete = new_time < m_startTime;
		if (m_startTime == m_endTime)
		{
			new_loop_time = m_endTime;
		}
		else
		{
			new_loop_time = m_endTime + ( new_time - m_startTime );
			while ( new_loop_time < m_startTime )
			{
				// when the FrameRatio() > 1.0f, it's possible that
				// we need to subtract more than once, esp. with short anims
				new_loop_time = m_endTime + ( new_loop_time - m_startTime );
			}
		}
		new_hold_time = m_startTime;
	}
	else
	{
		Dbg_Assert( 0 );
	}

	if ( m_animComplete )
	{
		if ( m_loopingType == LOOPING_HOLD )
		{
			new_time = new_hold_time;
		}
		else if ( m_loopingType == LOOPING_PINGPONG )
		{
			m_direction = ( m_direction == ANIM_DIR_FORWARDS ) ? ANIM_DIR_BACKWARDS : ANIM_DIR_FORWARDS;
			new_time = new_hold_time;
		}
        else if ( m_loopingType == LOOPING_CYCLE )
        {
            new_time = new_loop_time;
		}
	}

	// Ken: This is temporary, to prevent the following assert going off when running a replay.
	// Eventually the replays will be fixed to always recreate the correct new_time
    if ( new_time < m_startTime )
	{
		new_time = m_startTime;
	}	
	if ( new_time > m_endTime )
	{
		new_time = m_endTime;
	}	

#ifdef __NOPT_ASSERT__
    if ( new_time < m_startTime || new_time > m_endTime )
    {
        Dbg_Message( "old_time = %f", old_time );
        Dbg_Message( "new_time = %f", new_time );
        Dbg_Message( "new_loop_time = %f", new_loop_time );
        Dbg_Message( "new_hold_time = %f", new_hold_time );
        Dbg_Message( "m_startTime = %f", m_startTime );
        Dbg_Message( "m_endTime = %f", m_endTime );
        Dbg_Message( "m_loopingType = %d", m_loopingType );
        Dbg_Message( "m_direction = %d", m_direction );
        Dbg_Message( "m_animComplete = %d", m_animComplete );
        Dbg_Message( "m_animName = %s", Script::FindChecksumName( m_animName ) );
		Dbg_Assert( 0 );
    }
#endif

	return new_time;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CAnimChannel::CAnimChannel() : Lst::Node<CAnimChannel>(this)
{
    Reset();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CAnimChannel::~CAnimChannel()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimChannel::Reset()
{
	m_startTime = 0.0f;
    m_currentTime = 0.0f;
    m_endTime = 0.0f;
    m_animSpeed = 0.0f;
    m_direction = ANIM_DIR_FORWARDS;
    m_loopingType = LOOPING_CYCLE;
    m_animComplete = false;

	// Ken: Changed this to be a hard wired checksum rather than calling Script::GenerateCRC
	// The reason for this is that CAnimChannel::Reset() gets called from 
	// CSkaterTrackingInfo::Reset() in replay.cpp, which gets called from the CSkaterTrackingInfo
	// constructor. There is a static instance of a CSkaterTrackingInfo in replay.cpp, so the
	// constructor will get called at some point before main(). Since Script::GenerateCRC accesses
	// an array of data, there is a chance that it might get called before the array is initialised,
	// since the order of execution is not well defined.
	// It probably would be OK, I just want to be safe.
	m_animName = 0xf60c9090;		// unassigned <-- just giving it a non-zero value, so that
									// we can be sure it's getting initialized correctly...
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CAnimChannel::GetCurrentAnim( void )
{
    return m_animName;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CAnimChannel::GetCurrentAnimTime( void )
{
    return m_currentTime;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimChannel::GetAnimTimes(float *pStart, float *pCurrent, float *pEnd)
{
  	Dbg_MsgAssert( pStart, ("NULL pStart") );
	Dbg_MsgAssert( pCurrent, ("NULL pCurrent") );
	Dbg_MsgAssert( pEnd, ("NULL pEnd") );

	*pCurrent = m_currentTime;
	
    if ( m_direction == ANIM_DIR_FORWARDS )
	{
		*pStart = m_startTime;
		*pEnd = m_endTime;
	}	
	else
	{
		*pStart = m_endTime;
		*pEnd = m_startTime;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimChannel::AddTime( float incVal )
{
	// GJ:  a way to fool the animation controller
	// into incrementing its time (frame rate
	// independent) for the viewer object

	float oldAnimSpeed = m_animSpeed;

	m_animSpeed = 0.0f;

	m_currentTime += incVal;

	CAnimChannel::Update();

	m_animSpeed = oldAnimSpeed;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimChannel::Update()
{
	m_currentTime = get_new_anim_time();
}

/******************************************************************/
/*                                                                */
/* Returns whether the animation has completed.  This is only     */
/* valid with "hold on last frame" animations.				  */
/*                                                                */
/******************************************************************/

bool CAnimChannel::IsAnimComplete( void )
{
	if ( m_loopingType == LOOPING_HOLD )
	{
		if ( m_direction == ANIM_DIR_FORWARDS )
		{
			return ( m_currentTime == m_endTime );
		}	
		else
		{
			return ( m_currentTime == m_startTime );
		}	
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

bool CAnimChannel::IsLoopingAnim( void )
{	
	// Returns whether the current animation will loop forever.
	// Used in skater.cpp, to determine whether or not it should
	// wait for an animation to finish.
	return ( m_loopingType == LOOPING_CYCLE || m_loopingType == LOOPING_PINGPONG );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimChannel::SetAnimSpeed( float speed )
{
	m_animSpeed = speed / 60.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CAnimChannel::GetAnimSpeed( void )
{
	return ( m_animSpeed * 60.0f );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimChannel::ReverseDirection( void )
{
	if ( m_direction == ANIM_DIR_FORWARDS )
	{
		m_direction = ANIM_DIR_BACKWARDS;
	}	
	else
	{
		m_direction = ANIM_DIR_FORWARDS;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimChannel::SetLoopingType( EAnimLoopingType type )
{
	m_loopingType = type;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimChannel::PlaySequence( uint32 anim_name, float start_time, float end_time, EAnimLoopingType loop_type, float blend_period, float speed )
{		   
// Don't reset any more, because this will call the CBlendChannel's virtual
// virtual function, which makes the animation status inactive...
//	Reset();

	m_startTime							    = (start_time < end_time) ? start_time : end_time;
	m_currentTime							= start_time;
	m_endTime								= (start_time < end_time) ? end_time : start_time;
	m_animSpeed							    = speed / 60.0f;
	m_direction								= (start_time < end_time) ? ANIM_DIR_FORWARDS : ANIM_DIR_BACKWARDS;
	m_loopingType							= loop_type;
	m_animComplete							= false;
	m_animName								= anim_name;

//	Dbg_MsgAssert(m_startTime != m_endTime,("m_startTime == m_endTime (%f) TELL MICK",m_startTime));
//	m_wobbleTargetTime					    = (start_time + end_time) / 2;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAnimChannel::GetDebugInfo( Script::CStruct* p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert( p_info, ( "NULL p_info sent to CAnimChannel::GetDebugInfo" ) );

	// put the name first
	p_info->AddChecksum( "m_animName", m_animName );
	
	uint32 looping_type_checksum = 0;
	switch ( m_loopingType )
	{
	case 0:
		looping_type_checksum = CRCD( 0x3f00df04, "LOOPING_HOLD" );
		break;
	case 1:
		looping_type_checksum = CRCD( 0xfbada576, "LOOPING_CYCLE" );
		break;
	case 2:
		looping_type_checksum = CRCD( 0x792b924b, "LOOPING_PINGPONG" );
		break;
	case 3:
		looping_type_checksum = CRCD( 0x90423ff2, "LOOPING_WOBBLE" );
		break;
	default:
		Dbg_MsgAssert( 0, ( "Unknown looping type found in CAnimChannel::GetDebugInfo.  Was the enum changed?" ) );
		break;
	}
	if ( looping_type_checksum )
		p_info->AddChecksum( CRCD(0x84b06b3,"m_loopingType"), looping_type_checksum );

	switch ( m_direction )
	{
	case 0:
		p_info->AddChecksum( CRCD(0x9f648df4,"m_direction"), CRCD( 0xdf8b2509, "ANIM_DIR_FORWARDS" ) );
		break;
	case -1:
		p_info->AddChecksum( CRCD(0x9f648df4,"m_direction"), CRCD( 0xfdf0c4a4, "ANIM_DIR_BACKWARDS" ) );
		break;
	default:
		Dbg_MsgAssert( 0, ( "Unknown direction found in CAnimChannel::GetDebugInfo.  Was the enum changed?" ) );
		break;
	}

	p_info->AddFloat( CRCD(0x8fbac25e,"m_startTime"), m_startTime );
	p_info->AddFloat( CRCD(0xb73f0945,"m_currentTime"), m_currentTime );
	p_info->AddFloat( CRCD(0xf1cef774,"m_endTime"), m_endTime );
	p_info->AddFloat( CRCD(0xaa1e73de,"m_animSpeed"), m_animSpeed );
	p_info->AddInteger( CRCD(0x2d574054,"m_animComplete"), m_animComplete );
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx
