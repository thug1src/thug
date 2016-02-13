// SkaterButton.h

#ifndef	__SK_OBJECTS_SKATERBUTTON_H__
#define	__SK_OBJECTS_SKATERBUTTON_H__

#include <core/defines.h>
#include <core/support.h>
#include <sys\timer.h>


// A SkaterButton contains the state of a single pad button
// in a way that is specific to control issues
// initially this is similar to the old SPad structure in THPS-PSX
// but will probably be extended to handle analog buttons
// and storing, inspecting and flagging the history of button events 

class CSkaterButton : public Spt::Class
{


public:
#ifdef __NOPT_ASSERT__
	void		Update(int current_pressure, bool debug = false); 		// Update the button based on the current pressure
#else
	void		Update(int current_pressure); 		// Update the button based on the current pressure
#endif
	bool		GetPressed() {return m_pressed;}	// Get current "Pressed" state
	bool		GetTriggered();						// Get current "Triggered" state
	void		SetPressed( bool pressed );			// Set the button's "Pressed" state
	int			GetPressure();						// Get current pressure
	void		ClearTrigger();						// Clear the trigger
	void 		SetName(const char *p_name);		// Set name of button (mostly for dubugging)

	void		SetDebounce(float time);			// set time to debounce
	Tmr::Time	GetDebounceTime() { return static_cast< Tmr::Time >(m_debounce_time); }

	bool		GetReleased();
	void		ClearRelease();				
	
	uint32		GetPressedTime();  					// Get m_pressed_time
	uint32		GetReleasedTime();					// Get m_released_time

private:		
	
	
	bool		m_pressed;  						// true if pressed now, indicates the current state of the button
	uint32		m_pressed_time;						// time when button was last pressed
	uint32		m_released_time;					// time when button was last released
	int			m_pressure;							// current button presure;
	bool		m_triggered;	  					// true if was triggered, can be cleared
	int 		m_released;							// true if we released, can be cleared (for debouncing)
	uint32		m_name_checksum;					// Checksum of button name
	float		m_debounce_time;					// time until we we remain debounced
	//bool		m_debounce_pressed;					// state when "debounce" called
	//const char	*	mp_name;			  	 	// Pointer to name, used for debugging;
	
public:
	uint32 GetName() {return m_name_checksum;}
};

inline bool		CSkaterButton::GetTriggered()		// Get current "Triggered" state
{
	return m_triggered;
}

inline bool		CSkaterButton::GetReleased()		
{
	return m_released;
}

inline int			CSkaterButton::GetPressure()	// Get current pressure
{
	return	m_pressure;
}



#endif	__SK_OBJECTS_SKATERBUTTON_H__



