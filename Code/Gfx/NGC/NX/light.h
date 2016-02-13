#ifndef __LIGHT_H
#define __LIGHT_H

#include	<core/defines.h>
#include	<core/math.h>

namespace NxNgc
{

// Forward declarations
class CLightGroup;

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
	Mth::Vector					m_light_vector[2];

	// Light base colors
	Mth::Vector 				m_diffuse_base_color[2];
	Mth::Vector 				m_ambient_base_color;

	friend CLightGroup;
};

class CLightGroup/* : public CBaseLightGroup*/
{

public:
	// Constants
	enum
	{
		MAX_DIFFUSE_LIGHTS = 2,
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
		mMODULATING_AMBIENT			= 0x0008,					// setting ambient brightness
		mMODULATING_DIFFUSE_0		= 0x0010,					// setting diffuse brightness 0
		mMODULATING_DIFFUSE_1		= 0x0020,					// rest of diffuse must follow 0
	};

	uint32					m_flags;

	// Directional light vectors
	Mth::Vector				m_light_vector[2];

	// Light base colors
	Mth::Vector 			m_diffuse_base_color[2];
	Mth::Vector 			m_ambient_base_color;

	// Light modulated colors (modified base colors)
	Mth::Vector 			m_diffuse_mod_color[2];
	Mth::Vector 			m_ambient_mod_color;

	// Default lights
//	static Mth::Vector		s_default_directions[2];
//	static CBaseLightGroup	s_default_colors;
	static CLightGroup		s_default_lights;

}; // class CLightGroup



} // namespace NxNgc



#endif // __LIGHT_H


