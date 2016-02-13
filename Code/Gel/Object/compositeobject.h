//****************************************************************************
//* MODULE:         Gel/Object
//* FILENAME:       CompositeObject.h
//* OWNER:          Mick West
//* CREATION DATE:  10/17/2002
//****************************************************************************

#ifndef __OBJECT_COMPOSITEOBJECT_H__
#define __OBJECT_COMPOSITEOBJECT_H__

#include <core/defines.h>
#include <core/list.h>
#include <core/math.h>
#include <sys/profiler.h>
#include <gel/object.h>
#include <gel/object/basecomponent.h>

namespace Obj
{

// CComposite object contains a list of simple object components
// which can be combined to make up complex stuff like cars and stuff

class	CCompositeObject : public CObject
{

public:
									CCompositeObject();
    virtual 						~CCompositeObject();

	enum ECompositeObjectFlags
	{
		CO_PAUSED,
		CO_SUSPENDED,
		CO_HIDDEN,
		
		// components should not consider the object's change in position since the beginning of this frame to have been smooth
		CO_TELEPORTED,
		
		// Used by the script debugger.
		CO_SENDING_DEBUG_INFO_EVERY_FRAME,
		
		// Used to indicate the object has been finalized
		// so we can't add any more components to it
		CO_FINALIZED,
	};

    void 							Update();
	void 							Pause( bool paused )				{ m_composite_object_flags.Set(CO_PAUSED, paused); }
	bool 							IsPaused( void )  const  			{ return m_composite_object_flags.Test(CO_PAUSED); }
	void 							Suspend( bool suspended );
	bool							IsSuspended( void )	const			{ return m_composite_object_flags.Test(CO_SUSPENDED); }
	void							Hide( bool should_hide );
	bool 							IsHidden( void ) const				{ return m_composite_object_flags.Test(CO_HIDDEN); }
	bool 							IsFinalized( void )	const			{ return m_composite_object_flags.Test(CO_FINALIZED); }

	void							SetTeleported( bool update_components = true );
	void							ClearTeleported( void )				{ m_composite_object_flags.Clear(CO_TELEPORTED); }
	bool							HasTeleported( void )	const		{ return m_composite_object_flags.Test(CO_TELEPORTED); }

	// called when a model has radically changed position
	void							Teleport();

    void 							AddComponent( CBaseComponent* pComponent );
	
	void            				InitFromStructure( Script::CStruct* pParams );
	void							Finalize();
	void            				RefreshFromStructure( Script::CStruct* pParams );
	void 							CreateComponentFromStructure(Script::CStruct *p_struct, Script::CStruct* p_params = NULL);
	void 							CreateComponentsFromArray(Script::CArray *p_array, Script::CStruct * p_params = NULL);
	CBaseComponent* 				GetComponent( uint32 id ) const;
	virtual bool 					CallMemberFunction (uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript);

	
	void							LoadComponent ( CBaseComponent** pComp, uint32 id );
	
	// Used by the script debugger code (gel\scripting\debugger.cpp) to fill in a structure
	// for transmitting to the monitor.exe utility running on the PC.
	virtual void					GetDebugInfo(Script::CStruct *p_info);
	void							SetDebuggerEveryFrameFlag(bool state) { m_composite_object_flags.Set(CO_SENDING_DEBUG_INFO_EVERY_FRAME,state); }
	bool							GetDebuggerEveryFrameFlag()			{ return m_composite_object_flags.Test(CO_SENDING_DEBUG_INFO_EVERY_FRAME); }
	
		
	// when a composite object is paused, spawned (non-main) scripts running on it should be paused, if the script has its mPauseWithObject flag set
	virtual bool					ShouldUpdatePauseWithObjectScripts (   ) { return !IsPaused(); }

	
public:
	const Mth::Vector&					GetPos( void )	  const   { return m_pos; }
	void 							SetPos(const Mth::Vector& pos) 		{ m_pos = pos; }
	Mth::Matrix&					GetMatrix( void )			{ return m_matrix; }
	void							SetMatrix( const Mth::Matrix& matrix )	{ m_matrix = matrix; }
	void							SetDisplayMatrix( const Mth::Matrix& matrix )	{ m_display_matrix = matrix; }
	Mth::Matrix&					GetDisplayMatrix( void )	{ return m_display_matrix; }

//////////////////////////////
// These are public only while transitioning from CMovingObject to CComposititeobject
public:
	Mth::Vector						m_pos;
	Mth::Vector						m_old_pos;
	Mth::Matrix						m_matrix;
///////////////////////////////	
	
	// velocity data doesn't belong here,
	// but it's needed during the transition
	// so that it doesn't break the collision code
	Mth::Vector&					GetVel( void )  { return m_vel; }
	void							SetVel( const Mth::Vector& vel )			{ m_vel = vel; }
	Mth::Vector						m_vel;

protected:
	Mth::Matrix						m_display_matrix;		// final matrix used for display.  You need to rebuild this every frame
	
	// this is needed during the transition, only
	// until we move all of the specific objects'
	// logic into components.  this allows us
	// to get rid of the tasks
	virtual void					DoGameLogic() {}

private:	
	CBaseComponent*					mp_component_list;
	
	Flags<ECompositeObjectFlags>	m_composite_object_flags;
	
	#ifdef __NOPT_ASSERT__
	// The time (in microseconds) spent executing ::Update(), for displaying in the script debugger.
	int								m_update_time;		
	
	int								m_total_script_update_time;
	int								m_do_game_logic_time;
	
	// Runs through all the components and zeroes their update times.
	void							zero_component_update_times();
public:
	void							SetUpdateTime(int t) {m_update_time=t;}
	int								GetUpdateTime() const {return m_update_time;}
private:	
	#endif	

#ifdef __USE_PROFILER__
public:	
	void 							SetProfileColor(uint32 ProfileColor) {m_profile_color = ProfileColor;} 
	uint32 							GetProfileColor() const {return m_profile_color;} 
private:
	uint32							m_profile_color;
#else
public:	
	void 							SetProfileColor(uint32 ProfileColor) {;} 
#endif	
	
	// Composite objects have some common components hard wired, as it
	// much simpler and more efficient to do it that way	
};

inline void CCompositeObject::LoadComponent ( CBaseComponent** pComp, uint32 id )
{
	if (!*pComp)
	{
		*pComp = GetComponent(id);
	}
	Dbg_Assert(*pComp);
}

}

#endif


