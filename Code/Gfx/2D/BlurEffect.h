#ifndef __GFX_2D_BLUREFFECT_H__
#define __GFX_2D_BLUREFFECT_H__

#include <gfx/2D/ScreenElement2.h>
#include <sys/timer.h>

namespace Script
{
	class CStruct;
}

namespace Front
{

class CBlurEffect
{
public:

	struct Props
	{
		float						topAlpha;
		float						bottomAlpha;
		float 						bottomScaleX;
		float						bottomScaleY;					
		float						maxDisplacementX;
		float						maxDisplacementY;
	};
		
									CBlurEffect();
									~CBlurEffect();

	void							Update();

	CScreenElement::ConcatProps &	GetInfo(int index);
	int								GetNumEntries() {return 16;}
	Image::RGBA						GetRGBA() {return m_rgba;}

	void							SetAlphas(float top, float bottom) {m_target.topAlpha = top; m_target.bottomAlpha = bottom;}
	void							SetBottomScales(float scaleX, float scaleY) {m_target.bottomScaleX = scaleX; m_target.bottomScaleY = scaleY;}
	void							SetMaxDisplacements(float dispX, float dispY) {m_target.maxDisplacementX = dispX; m_target.maxDisplacementY = dispY;}
	void							SetRGBA(Image::RGBA rgba) {m_rgba = rgba;}
	void							SetAllTargetProps(const Props &newProps, Tmr::Time time);
	void							SetAnimTime(Tmr::Time time);

	const Props &					SetMorph(Script::CStruct *pProps, Tmr::Time *pRetTime = NULL);

protected:

	CScreenElement::ConcatProps		m_tab[16];

	Props							m_target;
	Props							m_current;

	Tmr::Time						m_last_time;
	Tmr::Time						m_key_time;
	
	Image::RGBA						m_rgba;
};

}

#endif // __GFX_2D_BLUREFFECT_H__


