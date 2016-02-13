// SkaterPad.h

#ifndef	__SK_OBJ_SKATERPAD_H__
#define	__SK_OBJ_SKATERPAD_H__


#include <core/defines.h>
#include <core/support.h>
#include <core/math.h>
#include <sk\objects\skaterbutton.h>

namespace Inp
{
	class Data;
}

// A CSkaterPad class contains the state of the pad controlling the skater 
class  CSkaterPad  : public Spt::Class
{
	
public:
	CSkaterPad();

	uint32			GetPressedMask( void );
	
	#ifdef __NOPT_ASSERT__
	void			Update ( Inp::Data* input, bool debug = false );
	#else
	void			Update ( Inp::Data* input );
	#endif
	
	void			Zero (   );
	void			Reset (   );
	
	float			GetScaledAnalogStickMagnitude ( float analog_x, float analog_y, float analog_angle  );
	float			GetScaledLeftAnalogStickMagnitude (   ) { return GetScaledAnalogStickMagnitude(m_leftX, m_leftY, m_leftAngle); }
	float			GetScaledRightAnalogStickMagnitude (   ) { return GetScaledAnalogStickMagnitude(m_rightX, m_rightY, m_rightAngle); }
	
	bool			IsLeftAnalogUpPressed();
	bool			IsLeftAnalogDownPressed();
	void			DebounceLeftAnalogUp ( float duration );
	void			DebounceLeftAnalogDown( float duration );

	CSkaterButton 	m_up;
	CSkaterButton 	m_down;
	CSkaterButton 	m_left;
	CSkaterButton 	m_right;
	CSkaterButton 	m_L1;
	CSkaterButton 	m_L2;
	CSkaterButton 	m_L3;
	CSkaterButton 	m_R1;
	CSkaterButton 	m_R2;
	CSkaterButton 	m_R3;
	CSkaterButton 	m_circle;
	CSkaterButton 	m_square;
	CSkaterButton 	m_triangle;
	CSkaterButton 	m_x;
	CSkaterButton 	m_start;
	CSkaterButton 	m_select;

	float			m_rightX;
	float			m_rightY;
	float			m_leftX;
	float			m_leftY;

	float			m_scaled_rightX;
	float			m_scaled_rightY;
	float			m_scaled_leftX;
	float			m_scaled_leftY;

	// angle and amount we are push the direction stick
	float			m_rightAngle;
	float			m_rightLength;
	float			m_leftAngle;
	float			m_leftLength;
	
	Tmr::Time		m_leftAnalogUpDebounceTime;
	Tmr::Time		m_leftAnalogDownDebounceTime;
	
	// Given the checksum of a button name, this will return that button.
	CSkaterButton*	GetButton(uint32 NameChecksum);

	// related utility functions
	
	static uint32	sGetDirection(bool Up, bool Down, bool Left, bool Right);
	static float	sGetAngleFromDPad(bool Up, bool Down, bool Left, bool Right);
};

inline bool CSkaterPad::IsLeftAnalogUpPressed (   )
{
	return m_leftAnalogUpDebounceTime == 0 && m_leftLength != 0.0f && Mth::Abs(m_leftAngle) < Mth::DegToRad(45.0f);
}

inline bool CSkaterPad::IsLeftAnalogDownPressed (   )
{
	return m_leftAnalogDownDebounceTime == 0 && m_leftLength != 0.0f && (Mth::PI - Mth::Abs(m_leftAngle)) < Mth::DegToRad(45.0f);
}

inline void CSkaterPad::DebounceLeftAnalogUp ( float duration )
{
	m_leftAnalogUpDebounceTime = Tmr::GetTime() + static_cast< Tmr::Time >(1000.0f * duration);
}

inline void CSkaterPad::DebounceLeftAnalogDown ( float duration )
{
	m_leftAnalogDownDebounceTime = Tmr::GetTime() + static_cast< Tmr::Time >(1000.0f * duration);
}

typedef CSkaterPad CControlPad;

#endif	//	__SK_OBJ_SKATERPAD_H__

