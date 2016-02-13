//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterStancePanelComponent.h
//* OWNER:          Dan
//* CREATION DATE:  2/25/3
//****************************************************************************

#ifndef __COMPONENTS_STANCEPANELCOMPONENT_H__
#define __COMPONENTS_STANCEPANELCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <sys/siodev.h>

#include <gel/object/basecomponent.h>

#include <sk/components/skatercorephysicscomponent.h>

#define		CRC_SKATERSTANCEPANEL CRCD(0x21fc3301, "SkaterStancePanel")

#define		GetSkaterStancePanelComponent() ((Obj::CSkaterStancePanelComponent*)GetComponent(CRC_SKATERSTANCEPANEL))
#define		GetSkaterStancePanelComponentFromObject(pObj) ((Obj::CSkaterStancePanelComponent*)(pObj)->GetComponent(CRC_SKATERSTANCEPANEL))
														 
namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CSkaterStancePanelComponent : public CBaseComponent
{
	
public:
    CSkaterStancePanelComponent();
    virtual ~CSkaterStancePanelComponent();

public:
    virtual void            		Update (   );
    virtual void            		InitFromStructure ( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure ( Script::CStruct* pParams );
	virtual void					Finalize (   );
    
    virtual EMemberFunctionResult   CallMemberFunction ( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo ( Script::CStruct* p_info );

	static CBaseComponent*			s_create (   );
	
	void							Reset (   );
	
private:
	int								determine_stance (   );
	
private:
	int								m_last_stance;
	
	bool							m_active;
	
	bool							m_in_nollie;					// Whether in Nollie stance or not.
    bool							m_in_pressure;					// Whether in pressure stance or not.

	// peer components
	CSkaterCorePhysicsComponent*	mp_core_physics_component;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterStancePanelComponent::Reset (   )
{
	m_last_stance = -1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline int CSkaterStancePanelComponent::determine_stance (   )
{
	// regular
    int stance = 5;
	
    // pressure
    if (m_in_pressure)
	{
        // switch pressure
		if (mp_core_physics_component->IsSwitched())
    	{
    		stance -= 2;
    	}
        else
        {
            stance -= 1;
        }
	}
	else
    {
        // switch
    	if (mp_core_physics_component->IsSwitched())
    	{
    		// fakie
            if (m_in_nollie)
        	{
                stance -= 4;
            }
            else
            {
                stance -= 3;
            }
    	}
        else
        {
            // nollie
            if (m_in_nollie)
        	{
        		stance -= 5;
        	}
        }
    }
    
    return stance;
}

}

#endif
