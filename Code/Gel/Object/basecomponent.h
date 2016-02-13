//****************************************************************************
//* MODULE:         Gel/Object
//* FILENAME:       BaseComponent.h
//* OWNER:          Mick West
//* CREATION DATE:  10/17/2002
//****************************************************************************

#ifndef __OBJECT_BASECOMPONENT_H__
#define __OBJECT_BASECOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object.h>

namespace Script
{
    class CStruct;
    class CScript;
}

namespace Obj
{

// CBaseComponent is a virtual base class for object components
class CBaseComponent : public Spt::Class
{
	friend	class	CCompositeObject;

	
public:
    CBaseComponent();
    virtual ~CBaseComponent();

	// Possible results from calling CallMemberFunction
	// (If the order of these change, then please
	// also change Gfx::EAnimFunctionResult)
	enum EMemberFunctionResult
	{
		MF_FALSE 		= 0,
		MF_TRUE  		= 1,
		MF_NOT_EXECUTED = 2
	};
	
	enum EBaseComponentFlags
	{
		BC_NO_UPDATE
	};


public:
	virtual	void 					Update() = 0;
    virtual void 					InitFromStructure( Script::CStruct* pParams ) = 0;
	virtual	void 					Finalize();
    virtual void 					RefreshFromStructure( Script::CStruct* pParams );
	virtual void					ProcessWait( Script::CScript * pScript );
	
	void							SetObject(CCompositeObject* p_object) {mp_object = p_object;}
	CCompositeObject*				GetObject() const {return mp_object;}
	CBaseComponent*					GetNext() const {return mp_next;}
	CBaseComponent*					GetNextSameType() const {return mp_next_same_type;}
	void							SetNextSameType( CBaseComponent *p_component ){ mp_next_same_type = p_component; }
	uint32							GetType() const {return m_type;}
	virtual	void					Suspend(bool suspend);
	bool							IsSuspended() const { return m_flags.Test(BC_NO_UPDATE); }
	virtual void					Hide(bool shouldHide);
	virtual void					Teleport();

	// Used by the script debugger code to fill in a structure
	// for transmitting to the monitor.exe utility running on the PC.
	virtual void					GetDebugInfo(Script::CStruct *p_info);

public:
	virtual EMemberFunctionResult 	CallMemberFunction( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript );
	
protected:
	void							SetType(uint32 type) {m_type = type;}

private:
	uint32							m_type;	   			// Unique ID of the component, stuck here during constuction
	Flags<EBaseComponentFlags>		m_flags;
	CCompositeObject	* 			mp_object; 			// Parent object that contains this component
	CBaseComponent * 				mp_next;			// next component in the list
	CBaseComponent * 				mp_next_same_type;	// next component in the list that is of the same type

	#ifdef __NOPT_ASSERT__
	// The time spent (in microseconds) executing ::Update(), for displaying in the script debugger.
	int								m_update_time;
	#endif	
};

}

#endif
