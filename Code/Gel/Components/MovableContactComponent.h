//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       MovableContactComponent.h
//* OWNER:          Dan
//* CREATION DATE:  5/19/3
//****************************************************************************

#ifndef __COMPONENTS_MOVABLECONTACTCOMPONENT_H__
#define __COMPONENTS_MOVABLECONTACTCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <sk/engine/contact.h>

#define		CRC_MOVABLECONTACT CRCD(0x408d2aa9, "MovableContact")

#define		GetMovableContactComponent() ((Obj::CMovableContactComponent*)GetComponent(CRC_MOVABLECONTACT))
#define		GetMovableContactComponentFromObject(pObj) ((Obj::CMovableContactComponent*)(pObj)->GetComponent(CRC_MOVABLECONTACT))

class CFeeler;
 
namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CMovableContactComponent : public CBaseComponent
{
public:
    CMovableContactComponent();
    virtual ~CMovableContactComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	bool							CheckForMovableContact ( CFeeler& feeler );
	void							ObtainContact ( CCompositeObject* p_composite_object );
	bool							HaveContact (   );
	void							LoseAnyContact (   );
	bool							UpdateContact ( const Mth::Vector& pos);
	CContact*						GetContact (   ) { return mp_contact; }
	CCompositeObject*				CheckInsideObjectsToTop ( Mth::Vector& pos );
	CCompositeObject*				CheckInsideObjects ( Mth::Vector& pos, const Mth::Vector& safe_pos );
	
	Tmr::Time						GetTimeSinceLastLostContact (   ) { return Tmr::ElapsedTime(m_lost_contact_timestamp); }
	
private:		
	CCompositeObject*				find_inside_object ( const Mth::Vector& pos, Mth::Vector& p_return_pos );

	CContact*						mp_contact;						// contact point (NULL if no current contact)
	
	Tmr::Time						m_lost_contact_timestamp;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CMovableContactComponent::LoseAnyContact (   )
{
	if (mp_contact)
	{
		delete mp_contact;
		mp_contact = NULL;
		
		m_lost_contact_timestamp = Tmr::GetTime();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CMovableContactComponent::HaveContact (   )
{
	if (!mp_contact) return false;
	
	if (mp_contact->GetObject()) return true;
	
	LoseAnyContact();
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CMovableContactComponent::ObtainContact ( CCompositeObject* p_composite_object )
{
	LoseAnyContact();
	mp_contact = new CContact(p_composite_object);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CMovableContactComponent::UpdateContact ( const Mth::Vector& pos)
{
	if (!HaveContact()) return false;
	
	if (mp_contact->Update(pos)) return true;
	
	LoseAnyContact();
	return false;
}

}

#endif
