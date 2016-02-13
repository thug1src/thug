////////////////////////////////////////////////////////////////////////////
//
// Contact.cpp	  - Mick
//
// a contact is a point "on" a moving object
// generally on the  surface of the object
// and more generally at a position realative to it
// it is responsible for maintaining changes in
// position, due to changes on the object it is on


#include <engine/contact.h>
#include <sk/objects/movingobject.h>
#include <gfx/debuggfx.h>
#include <gfx/nx.h>

						
					
CContact::CContact()
{
}

CContact::CContact(Obj::CCompositeObject *p_moving_object )
{

	mp_moving_object = p_moving_object;
//	m_object_id = mp_moving_object->GetID();
	
	m_object_last_matrix = mp_moving_object->GetMatrix();
	m_object_last_pos = mp_moving_object->GetPos();
	
	m_movement.Set();
	m_rotated = false;
}


// check to see if the object exists.									
bool		CContact::ObjectExists()
{
	// With smart pointers, GetObject will just return NULL if the object has been deleted 
	return (GetObject() != NULL);
}

// call this once per frame, to calculate the movement of the contact point
bool		CContact::Update(const Mth::Vector &pos)
{											
	if (!mp_moving_object.Convert())
	{
		return false;
	}
	
	m_movement = mp_moving_object->GetPos() - m_object_last_pos;
	
	// so far we have only accounted for the movement of the object's origin
	// (mp_moving_object->GetPos()), and we need to see if the object has rotated
	// which will give us an additional movement vector

	if (m_object_last_matrix != mp_moving_object->GetMatrix())
	{
		m_rotated = true;
//		printf ("Object has rotated\n");
		// Get the position relative to the origin on the object
		Mth::Vector old_offset = pos - m_object_last_pos;
		Mth::Vector offset = old_offset;

//		printf ("Offset (%.2f,%.2f,%.2f)\n",offset[X],offset[Y],offset[Z]);
		
		// transform by the inverse of the original matrix
		Mth::Matrix back = m_object_last_matrix;		  
		// just to be safe
		back[W] = Mth::Vector(0,0,0,1);
		back[X][W] = 0.0f;
		back[Y][W] = 0.0f;
		back[Z][W] = 0.0f;
		back.InvertUniform();

		// transform to the new matrix
		m_rotation =  mp_moving_object->GetMatrix();
		// just to be safe
		m_rotation[W] = Mth::Vector(0,0,0,1);
		m_rotation[X][W] = 0.0f;
		m_rotation[Y][W] = 0.0f;
		m_rotation[Z][W] = 0.0f;
		
		m_rotation *= back;		
		
		Mth::Vector new_offset = m_rotation.Transform(offset);

//		printf ("new Offset (%.2f,%.2f,%.2f)\n",new_offset[X],new_offset[Y],new_offset[Z]);
		
		// Calculate the movment due to rotation
		Mth::Vector	rotation_movement = (new_offset - old_offset);
	
		// add it to the movement, so it is included in the skater's movement
		m_movement += rotation_movement;

		m_object_last_matrix = mp_moving_object->GetMatrix();
	
	}
	else
	{
		m_rotated = false;
	}
	
	m_object_last_pos = mp_moving_object->GetPos();

	return true;
}

												   


