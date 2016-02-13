#include <core/defines.h>
#include <gfx/2D/BlurEffect.h>

#include <gel/scripting/struct.h>
#include <gel/scripting/vecpair.h>

namespace Front
{



CBlurEffect::CBlurEffect()
{
}




CBlurEffect::~CBlurEffect()
{
}




void CBlurEffect::Update()
{
	Tmr::Time current_time = Tmr::GetTime();
	float motion_grad = (float) (current_time - m_last_time) / (float) m_key_time;
	if (current_time - m_last_time < m_key_time)
		m_key_time -= current_time - m_last_time;
	else
		m_key_time = 0;
	m_last_time = current_time;
	
	if (m_key_time)
	{
		m_current.topAlpha = m_current.topAlpha + (m_target.topAlpha - m_current.topAlpha) * motion_grad;
		m_current.bottomAlpha = m_current.bottomAlpha + (m_target.bottomAlpha - m_current.bottomAlpha) * motion_grad;
		m_current.bottomScaleX = m_current.bottomScaleX + (m_target.bottomScaleX - m_current.bottomScaleX) * motion_grad;
		m_current.bottomScaleY = m_current.bottomScaleY + (m_target.bottomScaleY - m_current.bottomScaleY) * motion_grad;
		m_current.maxDisplacementX = m_current.maxDisplacementX + (m_target.maxDisplacementX - m_current.maxDisplacementX) * motion_grad;
		m_current.maxDisplacementY = m_current.maxDisplacementY + (m_target.maxDisplacementY - m_current.maxDisplacementY) * motion_grad;
	}
	else
	{
		m_current = m_target;
	}
	
	for (int i = 0; i < 16; i++)
	{
		float depth_mult = (float) i / 16.0f;
		m_tab[i].SetScreenPos(m_current.maxDisplacementX * depth_mult,
							  m_current.maxDisplacementY * depth_mult);
		m_tab[i].SetScale(1.0f + (m_current.bottomScaleX - 1.0f) * depth_mult,
						  1.0f + (m_current.bottomScaleY - 1.0f) * depth_mult);
		m_tab[i].alpha = (m_current.topAlpha + (m_current.bottomAlpha - m_current.topAlpha) * depth_mult) / 16.0f;
	}
}




CScreenElement::ConcatProps &CBlurEffect::GetInfo(int index)
{
	return m_tab[index];
}




void CBlurEffect::SetAllTargetProps(const Props &newProps, Tmr::Time time) 
{
	m_target = newProps;
	SetAnimTime(time);
}




void CBlurEffect::SetAnimTime(Tmr::Time time)
{
	if (time == 0)
		m_current = m_target;
	
	m_key_time = time;
	m_last_time = Tmr::GetTime();
}




const CBlurEffect::Props &CBlurEffect::SetMorph(Script::CStruct *pProps, Tmr::Time *pRetTime)
{	
	Script::CPair alpha_pair;
	if (pProps->GetPair(CRCD(0x21de240c,"blur_alpha_pair"), &alpha_pair))
	{
		SetAlphas(alpha_pair.mX, alpha_pair.mY);
	}

	Script::CPair bottom_scales;
	if (pProps->GetPair(CRCD(0x3e10436d,"blur_bottom_scales"), &bottom_scales))
	{
		SetBottomScales(bottom_scales.mX, bottom_scales.mY);
	}

	Script::CPair max_displacement;
	if (pProps->GetPair(CRCD(0x4c2a3cde,"blur_max_displacement"), &max_displacement))
	{
		SetMaxDisplacements(max_displacement.mX, max_displacement.mY);
	}
	
	float desired_time;
	Tmr::Time anim_time;
	if (pProps->GetFloat(CRCD(0x906b67ba,"time"), &desired_time))
	{
		anim_time = (Tmr::Time) (desired_time * (float) Tmr::vRESOLUTION);
	}
	else
		anim_time = 0;
	SetAnimTime(anim_time);
	if (pRetTime)
		*pRetTime = anim_time;

	return m_target;
}




}
