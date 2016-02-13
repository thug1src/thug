// skaterbutton.cpp

#include	<sk\objects\skaterbutton.h>

#include	<gel\scripting\checksum.h>

const	int	pressure_threshold = 64;

#ifdef __NOPT_ASSERT__
void		CSkaterButton::Update(int current_pressure, bool debug) 		// Update the pad based on teh current pressure
#else
void		CSkaterButton::Update(int current_pressure) 		// Update the pad based on teh current pressure
#endif
{
	// if were are debouncing the button then update the debounce state
	// based on time and release, and ignore the button if still debouncing
	if (m_debounce_time)
	{
		// if not pressed, or debounce time expired
		if (current_pressure <= pressure_threshold)
		{
//			dodgy_test(); printf("Ending Debouncing as not pressed\n");
			m_debounce_time = 0;
		}
		else if ((float)Tmr::GetTime() > m_debounce_time)
		{
//			dodgy_test(); printf("Ending Debouncing as time expired\n");
			m_debounce_time = 0;
		}
		else
		{
//			dodgy_test(); printf("Debouncing button\n");
			return;
		}
	}

	m_pressure = current_pressure;
	if (m_pressure > pressure_threshold)
	{
		// button is currently pressed
		if (!m_pressed)
		{
			m_pressed_time = Tmr::ElapsedTime(0);
			#ifdef __NOPT_ASSERT__
			if (debug)			
			{
				printf("%d: +++ Triggered %s +++\n",(int)Tmr::GetRenderFrame(),Script::FindChecksumName(m_name_checksum));
			}
			#endif
			// but not previously pressed, so set the "triggered" flag
			m_triggered = true;
		}
		m_pressed = true;
	}
	else
	{
		// button is currently NOT pressed
		if (m_pressed)
		{
			// but WAS previously pressed, so set the "released" flag
			m_released_time = Tmr::ElapsedTime(0);
			#ifdef __NOPT_ASSERT__
			if (debug)			
			{
				printf("%d: --- Released  %s ----\n",(int)Tmr::GetRenderFrame(),Script::FindChecksumName(m_name_checksum));
			}
			#endif
			m_released = true;
		}
		m_pressed = false;
	}

}

void		CSkaterButton::SetPressed( bool pressed )		// Set the button's "Pressed" state
{
	if( pressed )
	{
		// button is currently pressed
		if( !m_pressed )
		{
			m_pressed_time = Tmr::ElapsedTime(0);
			// but not previously pressed, so set the "triggered" flag
			m_triggered = true;
		}
		m_pressed = true;
	}
	else
	{
		// button is currently NOT pressed
		if (m_pressed)
		{
			// but WAS previously pressed, so set the "released" flag
			m_released = true;
		}
		m_pressed = false;
	}
	m_pressed = pressed;
}

uint32		CSkaterButton::GetReleasedTime()					// Get time elapsed since m_released_time
{
	return	Tmr::ElapsedTime(m_released_time);
}

uint32		CSkaterButton::GetPressedTime()					// Get time elapsed since m_presssed_time
{
	return	Tmr::ElapsedTime(m_pressed_time);
}

void		CSkaterButton::ClearTrigger()						// Clear the trigger
{
	m_triggered = false;
}

void		CSkaterButton::ClearRelease()						// Clear the trigger
{
	m_released = false;
}


void 		CSkaterButton::SetName(const char *p_name)
{
	m_name_checksum = Crc::GenerateCRCFromString(p_name);
}


void		CSkaterButton::SetDebounce(float time)
{
	// Mick:  When debouncing, we ignore furthur input until the time has elapsed
	// or the button "pressed" state changes from what it was at the start of the debounce 

//	printf ("SetDebounce #########################################################\n");
//	
//	printf ("GetTime = %d, m_pressed = %d, m_pressed_time = %d, m_triggered = %d\n",
//			Tmr::ElapsedTime(0), m_pressed  , m_pressed_time , m_triggered );
	
	
	m_debounce_time = time;
}


