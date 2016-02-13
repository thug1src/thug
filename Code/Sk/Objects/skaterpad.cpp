// SkaterPad.cpp

#include <sk\objects\skaterpad.h>
#include <gel\inpman.h>
#include <core\math.h>

//#define		PS2_SIMULATING_XBOX

// The constructor of a skater pad just sets all the names
CSkaterPad::CSkaterPad()
{
	// Note: These names must match those used when defining event queries in .q files.
	// (Otherwise the queries won't work)
	m_up.SetName("UP");
	m_down.SetName("DOWN");
	m_left.SetName("LEFT");
	m_right.SetName("RIGHT");
	m_square.SetName("SQUARE");
	m_circle.SetName("CIRCLE");
	m_triangle.SetName("TRIANGLE");
	m_x.SetName("X");
	m_L1.SetName("L1");
	m_L2.SetName("L2");
	m_L3.SetName("L3");
	m_R1.SetName("R1");
	m_R2.SetName("R2");
	m_R3.SetName("R3");
	m_start.SetName("START");
	m_select.SetName("SELECT");
	
	m_leftAnalogUpDebounceTime = 0;
	m_leftAnalogDownDebounceTime = 0;
}

#ifdef __NOPT_ASSERT__
void CSkaterPad::Update ( Inp::Data* input, bool debug )
#else
void CSkaterPad::Update ( Inp::Data* input )
#endif
{
	#ifdef __NOPT_ASSERT__
	
	m_up.Update(input->m_Event[Inp::Data::vA_UP] ? 255 : 0, debug);
	m_down.Update(input->m_Event[Inp::Data::vA_DOWN] ? 255 : 0, debug);
	m_left.Update(input->m_Event[Inp::Data::vA_LEFT] ? 255 : 0, debug);
	m_right.Update(input->m_Event[Inp::Data::vA_RIGHT] ? 255 : 0, debug);

	m_square.Update(input->m_Event[Inp::Data::vA_SQUARE] ? 255 : 0, debug);
	m_circle.Update(input->m_Event[Inp::Data::vA_CIRCLE] ? 255 : 0, debug);
	m_x.Update(input->m_Event[Inp::Data::vA_X] ? 255 : 0, debug);
	m_triangle.Update(input->m_Event[Inp::Data::vA_TRIANGLE] ? 255 : 0, debug);

	m_L1.Update(input->m_Event[Inp::Data::vA_L1] ? 255 : 0, debug);
	m_R1.Update(input->m_Event[Inp::Data::vA_R1] ? 255 : 0, debug);

	#ifndef	PS2_SIMULATING_XBOX
	m_L2.Update(input->m_Event[Inp::Data::vA_L2] ? 255 : 0, debug);
	m_R2.Update(input->m_Event[Inp::Data::vA_R2] ? 255 : 0, debug);
	#else	// update both 1 and 2 from L1/R1, to simulate the X-Box
	m_L2.Update(input->m_Event[Inp::Data::vA_L1] ? 255 : 0, debug);
	m_R2.Update(input->m_Event[Inp::Data::vA_R1] ? 255 : 0, debug);
	#endif
	
	m_L3.Update(input->m_Buttons & Inp::Data::mD_L3 ? 255 : 0, debug);
	m_R3.Update(input->m_Buttons & Inp::Data::mD_R3 ? 255 : 0, debug);
	
	
	m_select.Update(input->m_Buttons & Inp::Data::mD_SELECT ? 255 : 0, debug);
	
	#else
	
	m_up.Update(input->m_Event[Inp::Data::vA_UP] ? 255 : 0);
	m_down.Update(input->m_Event[Inp::Data::vA_DOWN] ? 255 : 0);
	m_left.Update(input->m_Event[Inp::Data::vA_LEFT] ? 255 : 0);
	m_right.Update(input->m_Event[Inp::Data::vA_RIGHT] ? 255 : 0);

	m_square.Update(input->m_Event[Inp::Data::vA_SQUARE] ? 255 : 0);
	m_circle.Update(input->m_Event[Inp::Data::vA_CIRCLE] ? 255 : 0);
	m_x.Update(input->m_Event[Inp::Data::vA_X] ? 255 : 0);
	m_triangle.Update(input->m_Event[Inp::Data::vA_TRIANGLE] ? 255 : 0);

	m_L1.Update(input->m_Event[Inp::Data::vA_L1] ? 255 : 0);
	m_R1.Update(input->m_Event[Inp::Data::vA_R1] ? 255 : 0);

	#ifndef	PS2_SIMULATING_XBOX
	m_L2.Update(input->m_Event[Inp::Data::vA_L2] ? 255 : 0);
	m_R2.Update(input->m_Event[Inp::Data::vA_R2] ? 255 : 0);
	#else	// update both 1 and 2 from L1/R1, to simulate the X-Box
	m_L2.Update(input->m_Event[Inp::Data::vA_L1] ? 255 : 0);
	m_R2.Update(input->m_Event[Inp::Data::vA_R1] ? 255 : 0);
	#endif
	
	m_L3.Update(input->m_Buttons & Inp::Data::mD_L3 ? 255 : 0);
	m_R3.Update(input->m_Buttons & Inp::Data::mD_R3 ? 255 : 0);

	m_select.Update(input->m_Buttons & Inp::Data::mD_SELECT ? 255 : 0);
	
	#endif
	
	int in;
	
	in = input->m_Event[Inp::Data::vA_RIGHT_X] - 128;
	if (in == 0)
	{
		m_scaled_rightX = 0.0f;
	}
	else
	{
		if (in == -128)
		{
			in = -127;
		}
		m_scaled_rightX = (in + (in > 0 ? -Inp::vANALOGUE_TOL : Inp::vANALOGUE_TOL)) / (127.0f - Inp::vANALOGUE_TOL);
	}
	
	in = input->m_Event[Inp::Data::vA_RIGHT_Y] - 128;
	if (in == 0)
	{
		m_scaled_rightY = 0.0f;
	}
	else
	{
		if (in == -128)
		{
			in = -127;
		}
		m_scaled_rightY = (in + (in > 0 ? -Inp::vANALOGUE_TOL : Inp::vANALOGUE_TOL)) / (127.0f - Inp::vANALOGUE_TOL);
	}
	
	in = input->m_Event[Inp::Data::vA_LEFT_X] - 128;
	if (in == 0)
	{
		m_scaled_leftX = 0.0f;
	}
	else
	{
		if (in == -128)
		{
			in = -127;
		}
		m_scaled_leftX = (in + (in > 0 ? -Inp::vANALOGUE_TOL : Inp::vANALOGUE_TOL)) / (127.0f - Inp::vANALOGUE_TOL);
	}

	in = input->m_Event[Inp::Data::vA_LEFT_Y] - 128;
	if (in == 0)
	{
		m_scaled_leftY = 0.0f;
	}
	else
	{
		if (in == -128)
		{
			in = -127;
		}
		m_scaled_leftY = (in + (in > 0 ? -Inp::vANALOGUE_TOL : Inp::vANALOGUE_TOL)) / (127.0f - Inp::vANALOGUE_TOL);
	}
	
	m_rightX = input->m_Event[Inp::Data::vA_RIGHT_X_UNCLAMPED] - 128;
	m_rightY = input->m_Event[Inp::Data::vA_RIGHT_Y_UNCLAMPED] - 128;
	m_leftX = input->m_Event[Inp::Data::vA_LEFT_X_UNCLAMPED] - 128;
	m_leftY = input->m_Event[Inp::Data::vA_LEFT_Y_UNCLAMPED] - 128;
	
	// Calculate the direction, and the amount

	m_rightLength = sqrtf(m_rightX * m_rightX + m_rightY * m_rightY);
	m_rightAngle = atan2f(m_rightX, -m_rightY);

	m_leftLength = sqrtf(m_leftX * m_leftX + m_leftY * m_leftY);
	if (m_leftLength > 0.001f)
	{
		m_leftAngle = atan2f(m_leftX, -m_leftY);
	}
	else
	{
		// if left analog stick not pressed, then get it from the D-Pad
		bool Up;
		bool Down;
		bool Left;
		bool Right;
		Up = input->m_Event[Inp::Data::vA_UP];
		Down = input->m_Event[Inp::Data::vA_DOWN];
		Left = input->m_Event[Inp::Data::vA_LEFT];
		Right = input->m_Event[Inp::Data::vA_RIGHT];

		if (Up || Down || Left || Right)
		{
			m_leftAngle = sGetAngleFromDPad(Up, Down, Left, Right);
			m_leftLength = 127.0f;
		}
		else
		{
			m_leftAngle = 0.0f;
			m_leftLength = 0.0f;				
		}
	}
	
	if (m_leftAnalogUpDebounceTime)
	{
		// if not pressed, or debounce time expired
		if (m_leftLength == 0.0f || !(Mth::Abs(m_leftAngle) < Mth::DegToRad(45.0f)))
		{
			m_leftAnalogUpDebounceTime = 0;
		}
		else if ((float)Tmr::GetTime() > m_leftAnalogUpDebounceTime)
		{
			m_leftAnalogUpDebounceTime = 0;
		}
	}
	
	if (m_leftAnalogDownDebounceTime)
	{
		// if not pressed, or debounce time expired
		if (m_leftLength == 0.0f || !((Mth::PI - Mth::Abs(m_leftAngle)) < Mth::DegToRad(45.0f)))
		{
			m_leftAnalogDownDebounceTime = 0;
		}
		else if ((float)Tmr::GetTime() > m_leftAnalogDownDebounceTime)
		{
			m_leftAnalogDownDebounceTime = 0;
		}
	}
}

CSkaterButton *CSkaterPad::GetButton(uint32 NameChecksum)
{
	
	// Maybe optimize using a hash table later for speed.
	if (m_up.GetName()==NameChecksum) return &m_up;
	else if (m_down.GetName()==NameChecksum) return &m_down;
	else if (m_left.GetName()==NameChecksum) return &m_left;
	else if (m_right.GetName()==NameChecksum) return &m_right;
	else if (m_square.GetName()==NameChecksum) return &m_square;
	else if (m_circle.GetName()==NameChecksum) return &m_circle;
	else if (m_triangle.GetName()==NameChecksum) return &m_triangle;
	else if (m_x.GetName()==NameChecksum) return &m_x;
	else if (m_L1.GetName()==NameChecksum) return &m_L1;
	else if (m_L2.GetName()==NameChecksum) return &m_L2;
	else if (m_L3.GetName()==NameChecksum) return &m_L3;
	else if (m_R1.GetName()==NameChecksum) return &m_R1;
	else if (m_R2.GetName()==NameChecksum) return &m_R2;
	else if (m_R3.GetName()==NameChecksum) return &m_R3;
	else if (m_start.GetName()==NameChecksum) return &m_start;
	else if (m_select.GetName()==NameChecksum) return &m_select;
	else 
	{
		Dbg_MsgAssert(0,("Bad checksum '%x' sent to GetButton",NameChecksum));
	}
	return NULL;
}	
																			
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
																			
float	CSkaterPad::GetScaledAnalogStickMagnitude ( float analog_x, float analog_y, float analog_angle )
{
	float x = analog_x / (analog_x > 0.0f ? 127.0f : 128.0f);
	float y = analog_y / (analog_y > 0.0f ? 127.0f : 128.0f);
	
	#ifndef __PLAT_NGC__
	
	float angle;
	if (analog_angle > (-Mth::PI / 4.0f) && analog_angle < (Mth::PI / 4.0f))
	{
		// top quadrant
		angle = analog_angle;
	}
	else if (analog_angle > (Mth::PI / 4.0f) && analog_angle < (3.0f * Mth::PI / 4.0f))
	{
		// right quadrant
		angle = analog_angle - (Mth::PI / 2.0f);
	}
	else if (Mth::Abs(analog_angle) > (3.0f * Mth::PI / 4.0f))
	{
		// bottom quadrant
		angle = Mth::Abs(analog_angle) - Mth::PI;
	}
	else
	{
		// left quadrant
		angle = analog_angle + (Mth::PI / 2.0f);
	}
	
	return sqrtf(x * x + y * y) * Mth::Abs(cosf(angle));
	
	#else
	
	return sqrtf(x * x + y * y);
	
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	CSkaterPad::GetPressedMask( void )
{
	uint32 mask;

	mask = 0;

	if( m_up.GetPressed())
	{
		mask |= Inp::Data::mA_UP;
	}
	if( m_down.GetPressed())
	{
		mask |= Inp::Data::mA_DOWN;
	}
	if( m_left.GetPressed())
	{
		mask |= Inp::Data::mA_LEFT;
	}
	if( m_right.GetPressed())
	{
		mask |= Inp::Data::mA_RIGHT;
	}
	if( m_L1.GetPressed())
	{
		mask |= Inp::Data::mA_L1;
	}
	if( m_L2.GetPressed())
	{
		mask |= Inp::Data::mA_L2;
	}
	if( m_R1.GetPressed())
	{
		mask |= Inp::Data::mA_R1;
	}
	if( m_R2.GetPressed())
	{
		mask |= Inp::Data::mA_R2;
	}
	if( m_circle.GetPressed())
	{
		mask |= Inp::Data::mA_CIRCLE;
	}
	if( m_square.GetPressed())
	{
		mask |= Inp::Data::mA_SQUARE;
	}
	if( m_triangle.GetPressed())
	{
		mask |= Inp::Data::mA_TRIANGLE;
	}
	if( m_x.GetPressed())
	{
		mask |= Inp::Data::mA_X;
	}
		
	return mask;
}
																			
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the pressed state of the four cardinal directions
// then return a sensible value including diagonals
// given that we might be pressing opposing directions (Up+Down)
// and we still want to get some sensible response
// Here Up+Down is mapped to PAD_D
// all mapping where there is some conflict needing resolution are
// marked with an asterix
uint32 CSkaterPad::sGetDirection(bool Up, bool Down, bool Left, bool Right)
{
	int Dir = Up + (Down << 1) + (Left << 2) + (Right << 3);

	static uint DirectionMap[16] =
	{
		// RLDU
		0,				// 0000
		PAD_U,		// 0001
		PAD_D,		// 0010
		PAD_D,		// 0011	 *
		PAD_L,		// 0100
		PAD_UL,		// 0101
		PAD_DL,		// 0110
		PAD_DL,		// 0111	 *
		PAD_R,		// 1000
		PAD_UR,		// 1001
		PAD_DR,		// 1010
		PAD_DR,		// 1011	 *
		PAD_R,		// 1100	 *
		PAD_UR,		// 1101	 *
		PAD_DR,		// 1110	 *
		PAD_DR,		// 1111	 *
	};

	return DirectionMap[Dir];
}
																			
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#define		PAD_ANGLE_U		( 0.0f )
#define		PAD_ANGLE_UR	( Mth::PI / 4.0f)
#define		PAD_ANGLE_R     ( Mth::PI / 2.0f) 
#define		PAD_ANGLE_DR    ( Mth::PI * 3.0f / 4.0f)
#define		PAD_ANGLE_D		( Mth::PI )
#define		PAD_ANGLE_DL	( - Mth::PI * 3.0f / 4.0f) 
#define		PAD_ANGLE_L		( - Mth::PI / 2.0f ) 
#define		PAD_ANGLE_UL	( - Mth::PI / 4.0f) 

float CSkaterPad::sGetAngleFromDPad(bool Up, bool Down, bool Left, bool Right)
{
	int Dir = Up + (Down << 1) + (Left << 2) + (Right << 3);

	static float DirectionMap[16] =
	{
		// RLDU
		0.0f,				// 0000
		PAD_ANGLE_U,		// 0001
		PAD_ANGLE_D,		// 0010
		PAD_ANGLE_D,		// 0011	 *
		PAD_ANGLE_L,		// 0100
		PAD_ANGLE_UL,		// 0101
		PAD_ANGLE_DL,		// 0110
		PAD_ANGLE_DL,		// 0111	 *
		PAD_ANGLE_R,		// 1000
		PAD_ANGLE_UR,		// 1001
		PAD_ANGLE_DR,		// 1010
		PAD_ANGLE_DR,		// 1011	 *
		PAD_ANGLE_R,		// 1100	 *
		PAD_ANGLE_UR,		// 1101	 *
		PAD_ANGLE_DR,		// 1110	 *
		PAD_ANGLE_DR,		// 1111	 *
	};

	return DirectionMap[Dir];
}
																			
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
																			
void CSkaterPad::Zero (   )
{
	m_up.Update( 0 );
	m_down.Update( 0 );
	m_left.Update( 0 );
	m_right.Update( 0 );
	m_L1.Update( 0 );
	m_L2.Update( 0 );
	m_L3.Update( 0 );
	m_R1.Update( 0 );
	m_R2.Update( 0 );
	m_R3.Update( 0 );
	m_square.Update( 0 );
	m_circle.Update( 0 );
	m_x.Update( 0 );
	m_triangle.Update( 0 );
	m_start.Update( 0 );
	m_select.Update( 0 );
	
	m_rightX = 0.0f;
	m_rightY = 0.0f;
	m_leftX = 0.0f;
	m_leftY = 0.0f;

	m_scaled_rightX = 0.0f;
	m_scaled_rightY = 0.0f;
	m_scaled_leftX = 0.0f;
	m_scaled_leftY = 0.0f;

	m_rightAngle = 0.0f;
	m_rightLength = 0.0f;
	m_leftAngle = 0.0f;
	m_leftLength = 0.0f;
}
																			
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterPad::Reset (   )
{
	Zero();
	
	m_up.ClearTrigger( );
	m_up.ClearRelease( );
	m_down.ClearTrigger( );
	m_down.ClearRelease( );
	m_left.ClearTrigger( );
	m_left.ClearRelease( );
	m_right.ClearTrigger( );
	m_right.ClearRelease( );
	m_L1.ClearTrigger( );
	m_L1.ClearRelease( );
	m_L2.ClearTrigger( );
	m_L2.ClearRelease( );
	m_L3.ClearTrigger( );
	m_L3.ClearRelease( );
	m_R1.ClearTrigger( );
	m_R1.ClearRelease( );
	m_R2.ClearTrigger( );
	m_R2.ClearTrigger( );
	m_R3.ClearRelease( );
	m_R3.ClearRelease( );
	m_square.ClearTrigger( );
	m_square.ClearRelease( );
	m_circle.ClearTrigger( );
	m_circle.ClearRelease( );
	m_x.ClearTrigger( );
	m_x.ClearRelease( );
	m_triangle.ClearTrigger( );
	m_triangle.ClearRelease( );
	m_start.ClearTrigger( );
	m_start.ClearRelease( );
	m_select.ClearTrigger( );
	m_select.ClearRelease( );
}
