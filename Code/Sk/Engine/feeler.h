
#ifndef	__ENGINE_FEELER_H__
#define	__ENGINE_FEELER_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/math.h>

#include <core/math.h>
#include <gfx/debuggfx.h>
#include <core/math/geometry.h>
#include <gel/collision/collision.h>
//#include <sk/objects/movingobject.h>		// needed for smart pointer destructor in feeler, dammit
#include <gel/object/compositeobject.h>
#include <gel/objptr.h>

namespace Nx
{
	class CCollCache;
}

class CFeeler;

typedef CFeeler CLineFeeler;

typedef	void (*FEELER_CB)(CFeeler*);		// feeler callback

class	CFeeler : public Mth::Line
{
public:
	CFeeler();
	CFeeler(const Mth::Vector &start, const Mth::Vector &end);

	void 							SetIgnore(uint16 ignore_1, uint16 ignore_0);
	void 							SetLine(const Mth::Vector &start, const Mth::Vector &end);
	void 							SetStart(const Mth::Vector &start);	
	void 							SetEnd(const Mth::Vector &end);

	void							DebugLine(int r=255, int g=255, int b=255, int num_frames = 0);
	
	bool							GetCollision(const Mth::Vector &start, const Mth::Vector &end, bool movables = true);
	bool							GetCollision(bool movables = true, bool far = false);
	inline bool						GetFarCollision(bool movables = true) {return GetCollision(movables, true);};
	bool							GetMovableCollision(const Mth::Vector &start, const Mth::Vector &end);
	bool							GetMovableCollision(bool far = false);
	inline bool						GetFarMovableCollision() {return GetMovableCollision(true);};
	bool							IsBrightnessAvailable();
	float							GetBrightness();
	
	float							GetDist() const;
	const Mth::Vector			&	GetPoint() const;
	const Mth::Vector			&	GetNormal() const;
	int								GetFaceIndex() const;
	
	void 							SetCallback(FEELER_CB p_callback);
	void 							SetCallbackData(void *p_callback_data);
	void * 							GetCallbackData() const;

	void							SetCache(Nx::CCollCache *p_cache);
	void							ClearCache();

	uint16 							GetFlags() const;
	ETerrainType 					GetTerrain() const;
	bool							IsMovableCollision(); 
	Nx::CCollObj *	 				GetSector() const;		// aieee!!!!!!
	Obj::CCompositeObject *			GetMovingObject();
	Obj::CCompositeObject *			GetCallbackObject() const;
	bool 							GetTrigger() const;
	uint32 							GetNodeChecksum() const;
	uint32 							GetScript() const;
	void 							SetScript(uint32 script);
	void 							SetTrigger(bool trigger);

	static void						sSetDefaultCache(Nx::CCollCache *p_cache);
	static void						sClearDefaultCache();

	// feeler callback probably should not be public						   
	FEELER_CB			mp_callback;	   // a callback called every frame
						   
private:
	void							init (   );
	
// for now, our implmentation will just copy over the relevent things from rw		
// as needed
// keep m_col_data private, and eventually we can get rid of it	
	Nx::CollData 		m_col_data;	 			// 
	float				m_dist;
	Mth::Vector			m_point;
	Mth::Vector			m_normal;

	Obj::CSmtPtr<Obj::CCompositeObject> mp_movable_collision_obj;	//  set if last collision was with a movable
	uint32 				m_movable_collision_id;		// id of this object, so we can check if it is dead.
	
	void 		*		mp_callback_data;		// 	pointer to some data the callback can use

	Nx::CCollCache *	mp_cache;

	static Nx::CCollCache *	sp_default_cache;
};

inline float	CFeeler::GetDist() const
{
	return m_dist;
}

inline const Mth::Vector &CFeeler::GetPoint() const
{
	return m_point;
}

inline const Mth::Vector &CFeeler::GetNormal() const
{
	return m_normal;
}

inline int		CFeeler::GetFaceIndex() const
{
	return m_col_data.surface.index;
}

inline uint16	CFeeler::GetFlags() const
{
	return m_col_data.flags;
}

inline ETerrainType CFeeler::GetTerrain() const
{
	return m_col_data.terrain;
}

inline Nx::CCollObj* CFeeler::GetSector() const // aieee!!!!!!
{
	return m_col_data.p_coll_object;
}

inline bool		CFeeler::GetTrigger() const
{
	return m_col_data.trigger;
}

inline uint32	CFeeler::GetNodeChecksum() const
{
	return m_col_data.node_name;
}

inline uint32	CFeeler::GetScript() const
{
	return m_col_data.script;	
}

inline void		CFeeler::SetScript(uint32 script)
{
	m_col_data.script = script; 
}

inline void		CFeeler::SetTrigger(bool trigger)
{
	m_col_data.trigger = trigger; 
}

inline void		CFeeler::SetCallback(FEELER_CB p_callback)
{
	mp_callback = p_callback;
}

inline void		CFeeler::SetCallbackData(void *p_callback_data)
{
	mp_callback_data = p_callback_data;
}

inline void *		CFeeler::GetCallbackData() const
{
	return mp_callback_data;
}

inline void		CFeeler::SetCache(Nx::CCollCache *p_cache)
{
	mp_cache = p_cache;
}

inline void		CFeeler::ClearCache()
{
	mp_cache = NULL;
}

inline void		CFeeler::sSetDefaultCache(Nx::CCollCache *p_cache)
{
	sp_default_cache = p_cache;
}

inline void		CFeeler::sClearDefaultCache()
{
	sp_default_cache = NULL;
}

#endif
