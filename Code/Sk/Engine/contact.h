
#ifndef	__ENGINE_CONTACT_H__
#define	__ENGINE_CONTACT_H__

#include <core/math.h>
#include <gel/objptr.h>
#include <gel/object/compositeobject.h>

class	CContact
{
public:
								CContact();
								CContact(Obj::CCompositeObject *p_moving_object);

	bool						ObjectExists();		// returns false if object has been deleted, or is "dead" or "inactive"
	bool						Update(const Mth::Vector &pos);		// Update the position, return false if object no longer exists
	const Mth::Vector		&	GetMovement() const;		// get movement vector of the contact point for this frame 	
	const Mth::Matrix		&	GetRotation() const;		// get rotation matrix for this frame 	
	bool						IsRotated() const;
						   
private:
	Obj::CSmtPtr<Obj::CCompositeObject>		mp_moving_object;
	Mth::Matrix					m_object_last_matrix; 	// The object's last orientation
	Mth::Vector					m_object_last_pos;		// The object's last position

	Mth::Vector					m_movement;
	Mth::Matrix					m_rotation;
	
	bool 						m_rotated;				// set if it was rotated

public:
	Obj::CCompositeObject*		GetObject() {return mp_moving_object.Convert();}		// return the object we are in contact with


};

inline const Mth::Vector		&	CContact::GetMovement() const
{
	return	m_movement;	
}

inline bool							CContact::IsRotated() const
{
	return	m_rotated;
}

inline const Mth::Matrix		&	CContact::GetRotation() const
{
	return	m_rotation;	

}

#endif

