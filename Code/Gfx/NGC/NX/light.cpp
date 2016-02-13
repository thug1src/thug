#include <core/defines.h>
#include "light.h"

namespace NxNgc
{


// Statics
CLightGroup		CLightGroup::s_default_lights;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CLightGroup::CLightGroup()
{
	// Don't use new lights unless someone sets them
	m_flags = 0;

	// Init to default lights
	m_light_vector = s_default_lights.m_light_vector;

	m_diffuse_base_color = s_default_lights.m_diffuse_base_color;
	m_diffuse_mod_color = s_default_lights.m_diffuse_base_color;

	m_ambient_base_color = s_default_lights.m_ambient_base_color;
	m_ambient_mod_color = s_default_lights.m_ambient_base_color;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CLightGroup::~CLightGroup()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &		CLightGroup::GetDirection(int idx) const
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

const Mth::Vector &		CLightGroup::GetBaseAmbientColor() const
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

const Mth::Vector &		CLightGroup::GetBaseDiffuseColor(int idx) const
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

const Mth::Vector &		CLightGroup::GetAmbientColor() const
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

const Mth::Vector &		CLightGroup::GetDiffuseColor(int idx) const
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

void					CLightGroup::SetDirection(int idx, const Mth::Vector & direction)
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

void					CLightGroup::SetBaseAmbientColor(const Mth::Vector & color)
{
	//Dbg_Assert(m_flags & mUSE_AMBIENT);
	m_flags |= mUSE_AMBIENT;		// turn on light if it isn't already on

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

void					CLightGroup::SetBaseDiffuseColor(int idx, const Mth::Vector & color)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);
	//Dbg_Assert(m_flags & (mUSE_DIFFUSE_0 << idx));
	m_flags |= (mUSE_DIFFUSE_0 << idx);  // turn on light if it isn't already on

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

void					CLightGroup::EnableAmbientLight(bool enable)
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

void					CLightGroup::EnableDiffuseLight(int idx, bool enable)
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

bool					CLightGroup::IsAmbientLightEnabled() const
{
	return (m_flags & mUSE_AMBIENT);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool					CLightGroup::IsDiffuseLightEnabled(int idx) const
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	return (m_flags & (mUSE_DIFFUSE_0 << idx));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool					CLightGroup::SetAmbientBrightness(float brightness)
{
	m_ambient_mod_color = GetBaseAmbientColor() * brightness;

	m_flags |= mMODULATING_AMBIENT;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool					CLightGroup::SetDiffuseBrightness(int idx, float brightness)
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

const Mth::Vector &		CLightGroup::sGetDefaultDirection(int idx)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	return s_default_lights.m_light_vector[idx];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &		CLightGroup::sGetDefaultAmbientColor()
{
	return s_default_lights.m_ambient_base_color;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &		CLightGroup::sGetDefaultDiffuseColor(int idx)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	return s_default_lights.m_diffuse_base_color[idx];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void					CLightGroup::sSetDefaultDirection(int idx, const Mth::Vector & direction)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	s_default_lights.m_light_vector[idx] = direction;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void					CLightGroup::sSetDefaultAmbientColor(const Mth::Vector & color)
{
	s_default_lights.m_ambient_base_color = color;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void					CLightGroup::sSetDefaultDiffuseColor(int idx, const Mth::Vector & color)
{
	Dbg_Assert(idx < MAX_DIFFUSE_LIGHTS);

	s_default_lights.m_diffuse_base_color[idx] = color;
}

} // namespace NxNgc


