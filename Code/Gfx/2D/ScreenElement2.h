#ifndef __GFX_2D_SCREENELEMENT2_H__
#define __GFX_2D_SCREENELEMENT2_H__

#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif
#include <gel/event.h>

#include <gfx/image/imagebasic.h>
#include <gfx/NxViewport.h>


namespace Front
{

class CScreenElement;
class CWindowElement;
typedef Obj::CSmtPtr<CScreenElement> CScreenElementPtr;




/*
	The base class for all screen elements. Screen elements include things like onscreen text, HUD sprites,
	items in menus, etc. Every screen element has 2D coordinates and (rectangular) dimensions. Screen elements
	can be "children" of other screen elements, whereby the child uses coordinates relative the upper left corner
	of the parent. If the parent is moved, all its children will be moved along with it.
	
	A screen element also has a pair of scale values, which are independent of its base width and height. A
	screen element will generally have a fixed base width and height, but the scale can be varied for animation
	purposes, making the element appear to grow or shrink in size. Scale changes to an element are also passed to
	children.
	
	The same idea applies to alpha and RGBA. RGBA is a static setting for the screen element, but alpha is a 
	separate morpheable multiplier that can be used for animation.  
*/
class CScreenElement : public Obj::CObject
{
	friend class CScreenElementManager;

public:

	enum EScreenElementType
	{
		TYPE_CONTAINER_ELEMENT						= 0x5b9da842,
		TYPE_TEXT_ELEMENT							= 0x5200dfb6,
		TYPE_VMENU									= 0x130ef802,
		TYPE_HMENU									= 0xccded1e1,
		TYPE_TEXT_BLOCK_ELEMENT						= 0x40d92263,
		TYPE_SPRITE_ELEMENT							= 0xb12b510a,
		TYPE_VSCROLLING_MENU						= 0x84298a3c,
		TYPE_HSCROLLING_MENU						= 0x2900e3d0,
		TYPE_ELEMENT_3D								= 0x82c59d78,
		TYPE_WINDOW_ELEMENT							= 0xa13adbb7,
	};

	enum EPosRecalc
	{
		vDONT_RECALC_POS	= 0,
		vRECALC_POS			= 1,
	};
	
	// flags 8-23 belong to CScreenElement, 24-31 can be used by subclasses
	enum 
	{
		// One of these values will fill bits 8-9
		// in m_object_flags
		vANIM_LINEAR		= 0,
		vANIM_FAST_OUT		= 1,
		vANIM_FAST_IN 		= 2,
		vANIM_SLOW_INOUT	= 3,
		vANIM_FIRST_BIT		= 8,
		vANIM_BIT_MASK		= 3,
		
		/* 
			Should be set whenever one of the following changes:
				-a morpheable property (pos, alpha, scale, etc...)
				-active morph time
				-justification
				-base width, height
				
			UpdateProperties() will respond by calculating current morph frame,
			Once element has finished morph, flag will be turned off.
		*/ 
		vNEEDS_LOCAL_POS_CALC 	= (1<<10),
		/*
			Set by UpdatePropertie
			s() after fresh absolute morph attributes (pos, scale, alpha,
			etc.) are computed. This happens a) after new morph frame calculated for element, or
			b) after new absolute morph attributes computed for parent.
		
			Cleared upon next call to UpdateProperties (if not set again). Can be used by update()
			to decide whether or not to do expensive calculations that are only necessary when
			element moves.
		*/
		vDID_SUMMED_POS_CALC 	= (1<<11),
		/* 
			Should be set when change made to a static property (i.e. a non-morpheable one).
			Virtual update() function can use to decide whether or not to do expensive
			calculation.
			
			Cleared by UpdateProperties() after call to virtual update() function
		*/
		vCHANGED_STATIC_PROPS 	= (1<<12),
		// if set dimensions have been set from outside, don't allow to resize self
		vFORCED_DIMS			= (1<<13),
		// can't add children if this flags is set
		vCHILD_LOCK				= (1<<14),
		// if set, this element (and all its children) will
		vCUSTOM_Z_PRIORITY		= (1<<15),
		vIS_SCREEN_ELEMENT		= (1<<16),
		vMORPHING_PAUSED		= (1<<17),
		vMORPHING_PAUSED2		= (1<<18), // a second stage is necessary to allow one update
		v3D_POS					= (1<<19), // Element is in 3D space
		v3D_CULLED				= (1<<20), // Element in 3D space is culled
		vEVENTS_BLOCKED			= (1<<21), // Element is not accepting any events
		vHIDDEN					= (1<<22), // Element is hiddden
	};
	
	/* 
		These are properties which can be concatenated down the parent-child tree.
		
		There are local properties (relative to parent), and absolute properties
		(concatenation of own local properties with all those of ancestors).
	*/
	struct ConcatProps
	{
	public:
		void				SetScreenPos(float x, float y);
		void				SetScreenPos(float x, float y, Nx::ZBufferValue z);
		void				SetScreenUpperLeft(float x, float y);
		void				SetWorldPos(const Mth::Vector & pos);
		void				SetScale(float scale_x, float scale_y);
		void				SetAbsoluteScale(float scale_x, float scale_y);

		void				SetRotate( float rot );
		void				SetRGBA( Image::RGBA rgba );

		float				GetScreenPosX() const;
		float				GetScreenPosY() const;
		Nx::ZBufferValue	GetScreenPosZ() const;
		float				GetScreenUpperLeftX() const;
		float				GetScreenUpperLeftY() const;
		const Mth::Vector &	GetWorldPos() const;
		float				GetScaleX() const;
		float				GetScaleY() const;
		float				GetAbsoluteScaleX() const;
		float				GetAbsoluteScaleY() const;
		
		float				GetRotate() const;
		Image::RGBA			GetRGBA() const;

		// These values do not need to be protected, since they aren't treated differently in 3D

		// Alpha value
		float				alpha;

	private:
		// 3D position of anchor point
		Mth::Vector			world_pos;

		// 2D position of anchor point
		float				screen_x, screen_y;
		Nx::ZBufferValue	screen_z;				// If using z-buffer sorting

		// position of upper-left hand corner (for quick lookup)
		float				ulx, uly;

		// final render scale
		float				scalex, scaley;
		
		// absolute scale used for calling DoMorph with a relative scale and now for 3D positions
		float				absoluteScalex;
		float				absoluteScaley;

		float				m_rot_angle;
		Image::RGBA			m_rgba;
	};

	static const float vJUST_LEFT;
	static const float vJUST_TOP;
	static const float vJUST_CENTER;
	static const float vJUST_RIGHT;
	static const float vJUST_BOTTOM;

							CScreenElement();
	virtual					~CScreenElement();

	// "absolute" means concatenated
	float					GetAbsX();
	float					GetAbsY();
	float					GetAbsW();
	float					GetAbsH();
	float 					GetBaseW() {return m_base_w;}
	float 					GetBaseH() {return m_base_h;}
	float					GetScaleX() {return m_local_props.GetScaleX();}
	float					GetScaleY() {return m_local_props.GetScaleY();}
	void					GetLocalPos(float *pX, float *pY);
	void					GetLocalULPos(float *pX, float *pY);
	void					GetJust(float *pJustX, float *pJustY);
	
	enum EForceInstant
	{
		DONT_FORCE_INSTANT	= 0,
		FORCE_INSTANT,
	};
	void					SetPos(float x, float y, EForceInstant forceInstant = DONT_FORCE_INSTANT);
	void					SetPos3D(const Mth::Vector &pos3D, EForceInstant forceInstant = DONT_FORCE_INSTANT);
	enum EForceDims
	{
		DONT_FORCE_DIMS		= 0,
		FORCE_DIMS,
	};
	void					SetDims(float w, float h, EForceDims force = DONT_FORCE_DIMS);
	void					SetJust(float justX, float justY);
	void					SetScale(float scaleX, float scaleY, bool relative = false, EForceInstant forceInstant = DONT_FORCE_INSTANT);
	void					SetAlpha(float alpha, EForceInstant forceInstant = DONT_FORCE_INSTANT);
	void					SetAnimTime(Tmr::Time animTime);
	void					SetRGBA( Image::RGBA rgba, EForceInstant forceInstant = FORCE_INSTANT );
	void					SetZPriority(float zPriority);
	void					SetMorphPausedState(bool pause);

	enum ELockState
	{
		UNLOCK,
		LOCK,
	};
	void					SetChildLockState(ELockState lockState);

	// These don't need to return smart pointers	
	CScreenElement*		GetFirstChild() {return mp_child_list;}
	CScreenElement*		GetLastChild();
	CScreenElement*		GetPrevSibling() {return mp_prev_sibling;}
	CScreenElement*		GetNextSibling() {return mp_next_sibling;}
	CScreenElement*		GetChildById(uint32 id, int *pIndex = NULL);
	CScreenElement*		GetChildByIndex(int index);
	
	int						CountChildren();
	
	// See notes before function definition
	void					UpdateProperties();
  	//void					Draw();

	/* 
		Versions of SetProperties(), SetMorph(), CallMemberFunction() in subclasses 
		should call versions in this class.
	*/
	virtual void			SetProperties(Script::CStruct *pProps);
	virtual void			WritePropertiesToStruct( Script::CStruct *pStruct );
	virtual void			SetMorph(Script::CStruct *pProps);
	virtual bool 			CallMemberFunction( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript );

	bool					EventsBlocked();
	bool					IsHidden();

protected:

	CScreenElementPtr		get_child_by_id_or_index(uint32 id, int *pIndex);
	CScreenElementPtr		get_parent();
	CWindowElement *		get_window();
	void					set_parent(const CScreenElementPtr &pParent, EPosRecalc recalculatePosition = vRECALC_POS);
	void					compute_ul_pos(ConcatProps &props);
	void					auto_set_z_priorities_recursive(float topPriority);
	
	bool					resolve_pos(Script::CStruct *pProps, float *pX, float *pY);
	bool					resolve_just(Script::CStruct *pProps, const uint32 RefName, float *pHJust, float *pVJust);
	bool					resolve_rgba(Script::CStruct *pProps, const uint32 RefName, Image::RGBA *pRGBA);
	
public:	
	#ifdef __NOPT_ASSERT__
	// a debugging function
	void					debug_verify_integrity();
	#endif
protected:
	
	/* 
		See notes on UpdateProperties() about what this function can do.
		
		What it *CAN'T* do:
		-use the summed pos values from children (these will be a frame out of date)
		-changed the properties of the parent (the changes won't be converted to
			local and summed positions until the next frame)
	*/
	virtual void			update() {;}
	
	CScreenElementPtr		mp_parent;
	CScreenElementPtr		mp_child_list;
	CScreenElementPtr		mp_next_sibling;
	CScreenElementPtr		mp_prev_sibling;
	
	ConcatProps				m_local_props;
	ConcatProps				m_target_local_props;

	Tmr::Time				m_base_time;
	// how much time must elapse since m_base_time for animation to end
	// set to 0 when animation complete
	Tmr::Time				m_key_time;
	float					m_last_motion_grad;
	float					m_last_pct_changed;
	
	// the concatenation of local props in ancestors and this
	ConcatProps				m_summed_props;

	// determines offset of upper-left corner of screen element
	// relative	to anchor point
	// -1.0	= left/top
	//	0	= center
	//  1.0	= bottom/right
	float					m_just_x, m_just_y;
	
	// the dimensions of this screen element before scaling is applied
	// (in pixels) (local)
	float					m_base_w, m_base_h;

	Image::RGBA				m_rgba;

	static const float		AUTO_Z_SPACE;
	
	float					m_z_priority;

	// TODO: temp hack to get preserve_original_alpha and restore_original_alpha working
	float					m_originalAlpha;
};




class CContainerElement : public CScreenElement
{
	friend class CScreenElementManager;

public:
							CContainerElement();
	virtual					~CContainerElement();

	void					SetProperties(Script::CStruct *pProps);
	virtual bool			PassTargetedEvent(Obj::CEvent *pEvent, bool broadcast = false);
	
protected:

	uint32					m_focusable_child;
};


inline void					CScreenElement::ConcatProps::SetScreenPos(float x, float y)
{
	screen_x = x;
	screen_y = y;
}

inline void					CScreenElement::ConcatProps::SetScreenPos(float x, float y, Nx::ZBufferValue z)
{
	SetScreenPos(x, y);
	screen_z = z;
}

inline void					CScreenElement::ConcatProps::SetScreenUpperLeft(float x, float y)
{
	ulx = x;
	uly = y;
}

inline void					CScreenElement::ConcatProps::SetWorldPos(const Mth::Vector & pos)
{
	world_pos = pos;
}

inline void					CScreenElement::ConcatProps::SetScale(float scale_x, float scale_y)
{
	scalex = scale_x;
	scaley = scale_y;
}

inline void					CScreenElement::ConcatProps::SetAbsoluteScale(float scale_x, float scale_y)
{
	absoluteScalex = scale_x;
	absoluteScaley = scale_y;
}

inline void					CScreenElement::ConcatProps::SetRotate( float rot_angle )
{
	m_rot_angle = rot_angle;
}

inline void					CScreenElement::ConcatProps::SetRGBA( Image::RGBA rgba )
{
	m_rgba.r = rgba.r;
	m_rgba.g = rgba.g;
	m_rgba.b = rgba.b;
	m_rgba.a = rgba.a;
}

inline float				CScreenElement::ConcatProps::GetScreenPosX() const
{
	return screen_x;
}

inline float				CScreenElement::ConcatProps::GetScreenPosY() const
{
	return screen_y;
}

inline Nx::ZBufferValue		CScreenElement::ConcatProps::GetScreenPosZ() const
{
	return screen_z;
}

inline float				CScreenElement::ConcatProps::GetScreenUpperLeftX() const
{
	return ulx;
}

inline float				CScreenElement::ConcatProps::GetScreenUpperLeftY() const
{
	return uly;
}

inline const Mth::Vector &	CScreenElement::ConcatProps::GetWorldPos() const
{
	return world_pos;
}

inline float				CScreenElement::ConcatProps::GetScaleX() const
{
	return scalex;
}

inline float				CScreenElement::ConcatProps::GetScaleY() const
{
	return scaley;
}

inline float				CScreenElement::ConcatProps::GetAbsoluteScaleX() const
{
	return absoluteScalex;
}

inline float				CScreenElement::ConcatProps::GetAbsoluteScaleY() const
{
	return absoluteScaley;
}

inline float				CScreenElement::ConcatProps::GetRotate() const
{
	return m_rot_angle;
}

inline Image::RGBA			CScreenElement::ConcatProps::GetRGBA() const
{
	return m_rgba;
}



} // end namespace

#endif
