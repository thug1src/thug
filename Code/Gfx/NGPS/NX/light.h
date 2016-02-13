#ifndef __LIGHT_H
#define __LIGHT_H

#include	<core/defines.h>
#include	<core/math.h>

namespace NxPs2
{

// Forward declarations
class CLightGroup;

#if 0
class CBaseLightGroup
{
public:
								CBaseLightGroup();
	virtual						~CBaseLightGroup();

	virtual const Mth::Vector &	GetDirection(int idx) const;

	virtual const Mth::Vector &	GetAmbientColor() const;
	virtual const Mth::Vector &	GetDiffuseColor(int idx) const;

protected:
	// Directional light vectors
	Mth::Vector					m_light_vector[MAX_DIFFUSE_LIGHTS];

	// Light base colors
	Mth::Vector 				m_diffuse_base_color[MAX_DIFFUSE_LIGHTS];
	Mth::Vector 				m_ambient_base_color;

	friend CLightGroup;
};
#endif

class CLightGroup/* : public CBaseLightGroup*/
{

	friend class CVu1Context;
public:
	// Constants
	enum
	{
		MAX_DIFFUSE_LIGHTS = 3,
	};

								CLightGroup();
								~CLightGroup();
	//virtual						~CLightGroup();

	const Mth::Vector &			GetDirection(int idx) const;
	//virtual const Mth::Vector &	GetDirection(int idx) const;

	const Mth::Vector &			GetBaseAmbientColor() const;
	const Mth::Vector &			GetBaseDiffuseColor(int idx) const;

	const Mth::Vector &			GetAmbientColor() const;
	const Mth::Vector &			GetDiffuseColor(int idx) const;
	//virtual const Mth::Vector &	GetAmbientColor() const;
	//virtual const Mth::Vector &	GetDiffuseColor(int idx) const;

	void						SetDirection(int idx, const Mth::Vector & direction);
	void						SetBaseAmbientColor(const Mth::Vector & color);
	void						SetBaseDiffuseColor(int idx, const Mth::Vector & color);

	// Enable lights
	void						EnableAmbientLight(bool enable);
	void						EnableDiffuseLight(int idx, bool enable);
	bool						IsAmbientLightEnabled() const;
	bool						IsDiffuseLightEnabled(int idx) const;

	// Set brightness of lights
	bool						SetAmbientBrightness(float brightness);
	bool						SetDiffuseBrightness(int idx, float brightness);

	// Default lights
	static const Mth::Vector &	sGetDefaultDirection(int idx);
	static const Mth::Vector &	sGetDefaultAmbientColor();
	static const Mth::Vector &	sGetDefaultDiffuseColor(int idx);

	static void					sSetDefaultDirection(int idx, const Mth::Vector & direction);
	static void					sSetDefaultAmbientColor(const Mth::Vector & color);
	static void					sSetDefaultDiffuseColor(int idx, const Mth::Vector & color);

protected:
	// Internal flags.
	enum {
		mUSE_AMBIENT				= 0x0001,					// Use ambient light
		mUSE_DIFFUSE_0				= 0x0002,					// Use diffuse light 0
		mUSE_DIFFUSE_1				= 0x0004,					// rest of diffuse must follow 0
		mUSE_DIFFUSE_2				= 0x0008,					// rest of diffuse must follow 0
		mMODULATING_AMBIENT			= 0x0010,					// setting ambient brightness
		mMODULATING_DIFFUSE_0		= 0x0020,					// setting diffuse brightness 0
		mMODULATING_DIFFUSE_1		= 0x0040,					// rest of diffuse must follow 0
		mMODULATING_DIFFUSE_2		= 0x0040,					// rest of diffuse must follow 0
	};

	uint32					m_flags;

	// Directional light vectors
	Mth::Vector				m_light_vector[MAX_DIFFUSE_LIGHTS];

	// Light base colors
	Mth::Vector 			m_diffuse_base_color[MAX_DIFFUSE_LIGHTS];
	Mth::Vector 			m_ambient_base_color;

	// Light modulated colors (modified base colors)
	Mth::Vector 			m_diffuse_mod_color[MAX_DIFFUSE_LIGHTS];
	Mth::Vector 			m_ambient_mod_color;

	// Default lights
//	static Mth::Vector		s_default_directions[MAX_DIFFUSE_LIGHTS];
//	static CBaseLightGroup	s_default_colors;
	static CLightGroup		s_default_lights;

}; // class CLightGroup

// Inline functions


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::Vector &		CLightGroup::GetDirection(int idx) const
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	if (m_flags & (mUSE_DIFFUSE_0 << idx))
	{
		return m_light_vector[idx];
	} else {
		return s_default_lights.m_light_vector[idx];
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::Vector &		CLightGroup::GetBaseAmbientColor() const
{
	if (m_flags & mUSE_AMBIENT)
	{
		return m_ambient_base_color;
	} else {
		return s_default_lights.m_ambient_base_color;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::Vector &		CLightGroup::GetBaseDiffuseColor(int idx) const
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	if (m_flags & (mUSE_DIFFUSE_0 << idx))
	{
		return m_diffuse_base_color[idx];
	} else {
		return s_default_lights.m_diffuse_base_color[idx];
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::Vector &		CLightGroup::GetAmbientColor() const
{
	if (m_flags & (mUSE_AMBIENT | mMODULATING_AMBIENT))
	{
		return m_ambient_mod_color;
	} else {
		return s_default_lights.m_ambient_base_color;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::Vector &		CLightGroup::GetDiffuseColor(int idx) const
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	if (m_flags & ((mUSE_DIFFUSE_0 | mMODULATING_DIFFUSE_0) << idx))
	{
		return m_diffuse_mod_color[idx];
	} else {
		return s_default_lights.m_diffuse_base_color[idx];
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CLightGroup::SetDirection(int idx, const Mth::Vector & direction)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);
	//Dbg_Assert(m_flags & (mUSE_DIFFUSE_0 << idx));
	m_flags |= (mUSE_DIFFUSE_0 << idx);  // turn on light if it isn't already on

	m_light_vector[idx] = direction;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CLightGroup::SetBaseAmbientColor(const Mth::Vector & color)
{
	//Dbg_Assert(m_flags & mUSE_AMBIENT);
// Mick:  turning on and off lights should be done ABOVE the p_line
// Mick:	m_flags |= mUSE_AMBIENT;		// turn on light if it isn't already on

	m_ambient_base_color = color;

	if (!(m_flags & mMODULATING_AMBIENT))
	{
		m_ambient_mod_color = color;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CLightGroup::SetBaseDiffuseColor(int idx, const Mth::Vector & color)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);
	//Dbg_Assert(m_flags & (mUSE_DIFFUSE_0 << idx));
// Mick:  turning on and off lights should be done ABOVE the p_line
// Mick:	m_flags |= (mUSE_DIFFUSE_0 << idx);  // turn on light if it isn't already on

	m_diffuse_base_color[idx] = color;

	if (!(m_flags & (mMODULATING_DIFFUSE_0 << idx)))
	{
		m_diffuse_mod_color[idx] = color;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CLightGroup::EnableAmbientLight(bool enable)
{
	if (enable)
	{
		m_flags |= mUSE_AMBIENT;
	} else {
		m_flags &= ~mUSE_AMBIENT;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CLightGroup::EnableDiffuseLight(int idx, bool enable)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	if (enable)
	{
		m_flags |= (mUSE_DIFFUSE_0 << idx);
	} else {
		m_flags &= ~(mUSE_DIFFUSE_0 << idx);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool					CLightGroup::IsAmbientLightEnabled() const
{
	return (m_flags & mUSE_AMBIENT);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool					CLightGroup::IsDiffuseLightEnabled(int idx) const
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	return (m_flags & (mUSE_DIFFUSE_0 << idx));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool					CLightGroup::SetAmbientBrightness(float brightness)
{
	m_ambient_mod_color = GetBaseAmbientColor() * brightness;

	m_flags |= mMODULATING_AMBIENT;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool					CLightGroup::SetDiffuseBrightness(int idx, float brightness)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	m_diffuse_mod_color[idx] = GetBaseDiffuseColor(idx) * brightness;

	m_flags |= (mMODULATING_DIFFUSE_0 << idx);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::Vector &		CLightGroup::sGetDefaultDirection(int idx)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	return s_default_lights.m_light_vector[idx];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::Vector &		CLightGroup::sGetDefaultAmbientColor()
{
	return s_default_lights.m_ambient_base_color;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::Vector &		CLightGroup::sGetDefaultDiffuseColor(int idx)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	return s_default_lights.m_diffuse_base_color[idx];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CLightGroup::sSetDefaultDirection(int idx, const Mth::Vector & direction)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	s_default_lights.m_light_vector[idx] = direction;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CLightGroup::sSetDefaultAmbientColor(const Mth::Vector & color)
{
	s_default_lights.m_ambient_base_color = color;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void					CLightGroup::sSetDefaultDiffuseColor(int idx, const Mth::Vector & color)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	s_default_lights.m_diffuse_base_color[idx] = color;
}





} // namespace NxPs2



#endif // __LIGHT_H

