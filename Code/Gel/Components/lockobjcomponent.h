//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       LockObjComponent.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  11/4/2002
//****************************************************************************

#ifndef __COMPONENTS_LOCKOBJCOMPONENT_H__
#define __COMPONENTS_LOCKOBJCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/singleton.h>
#include <core/task.h>

#include <gel/object/basecomponent.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_LOCKOBJ CRCD(0x5cb5cb7e,"LockObj")
#define		GetLockObjComponent() ((Obj::CLockObjComponent*)GetComponent(CRC_LOCKOBJ))
#define		GetLockObjComponentFromObject(pObj) ((Obj::CLockObjComponent*)(pObj)->GetComponent(CRC_LOCKOBJ))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CLockObjComponent : public CBaseComponent
{
public:
    CLockObjComponent();
    virtual ~CLockObjComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );

	virtual void					GetDebugInfo(Script::CStruct *p_info);
	
	static CBaseComponent*			s_create();
	static void						s_register();

protected:
	CCompositeObject*				get_locked_to_object();

public:
	void							LockToObject();
	bool							IsLockEnabled() const {return m_lock_enabled;}
	void							EnableLock( bool enabled ) {m_lock_enabled = enabled;}
	
protected:
	uint32							m_locked_to_object_name;
	uint32							m_locked_to_object_id;
	uint32							m_locked_to_object_bone_name;
	Mth::Vector						m_lock_offset;
	float							m_lag_factor;
	float							m_lag_factor_range_a;
	float							m_lag_factor_range_b;
	float 							m_lag_freq_a;
	float 							m_lag_freq_b;
	uint32							m_lag_phase;
	bool							m_lag;
	bool							m_no_rotation;

protected:
	bool							m_lock_enabled;
};

}

namespace LockOb
{
// The purpose of this manager is just to update all the locked objects, by calling
// CMovingObject::LockToObject on all objects that have had Obj_LockToObject run on them.
// It is a manager so that it can be given a task with a priority such that it is called
// after all the objects are updated.
// The reason the lock logic cannot be done in the object update is because sometimes the parent's 
// update might happen after the child's, causing a frame lag. This was making the gorilla appear
// behind the tram seat in the zoo when the tram was being skitched. (TT2324)

class Manager  : public Spt::Class
{
	Tsk::Task< Manager > *mp_task;
	static	Tsk::Task< Manager >::Code 	s_code;

public:
	Manager();
	~Manager();
	
private:	
	DeclareSingletonClass( Manager );
};
};



#endif
