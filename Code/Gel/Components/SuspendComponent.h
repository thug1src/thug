//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SuspendComponent.h
//* OWNER:          ???
//* CREATION DATE:  ??/??/??
//****************************************************************************

#ifndef __COMPONENTS_SUSPENDCOMPONENT_H__
#define __COMPONENTS_SUSPENDCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/singleton.h>
#include <core/task.h>

#include <gel/object/basecomponent.h>

#define		CRC_SUSPEND CRCD(0xce0ca665,"Suspend")
#define		GetSuspendComponent() ((Obj::CSuspendComponent*)GetComponent(CRC_SUSPEND))
#define		GetSuspendComponentFromObject(pObj) ((Obj::CSuspendComponent*)(pObj)->GetComponent(CRC_SUSPEND))

namespace Script
{
    class CScript;
    class CStruct;
}


              
namespace Obj
{

	class	CModelComponent;


namespace SuspendManager
{

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

}



class CSuspendComponent : public CBaseComponent
{
	friend class SuspendManager::Manager;
	 
public:
    CSuspendComponent();
    virtual ~CSuspendComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
	virtual	void					Finalize();	
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual	void					Suspend(bool suspend);
	

	void 							GetDebugInfo(Script::CStruct *p_info);
	bool							SkipLogic();
	bool 							SkipRender();

	
	static CBaseComponent*			s_create();
	static void						s_register();

public:		
	void 							CheckModelActive();
	float							GetDistanceSquaredToCamera();

private:
	bool							m_never_suspend;
	float							m_suspend_distance;
	float							m_suspend_distance_squared; // K: For a fast distance-squared check
	float							m_camera_distance_squared;  // current distance from camera
	int								m_no_suspend_count;	// number of initial frames we cannot suspend in

	CModelComponent* 				mp_model_component;

	bool							m_skip_logic;
	bool							m_skip_render;
	
// public during the transition:
public:
//protected:
	float							m_lod_dist[4];		// Mick: LOD is in the moving object for now....	
	int								m_initial_animations;		// must do at least these initial animations

protected:
	int								m_interleave;	// Mick: counter for interleaving animation at a distance
public:  // just for now	
	bool							should_animate( float *p_dist = NULL );
};




}

#endif
