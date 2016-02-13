#include <core/defines.h>
#include <core/HashTable.h>
#include <gfx/NxFontMan.h>
#include <gfx/NxViewMan.h>
#include <gfx/2D/ScreenElement2.h>
#include <gfx/2D/ScreenElemMan.h>
#include <gfx/2D/Window.h>
#include <gel/Event.h>
#include <gel/objtrack.h>
#include <gel/Scripting/array.h>
#include <gel/Scripting/scriptdefs.h>
#include <gel/Scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/array.h>
#include <gel/scripting/utils.h>
#ifdef	__PLAT_NGPS__	
#include <gfx/ngps/nx/line.h>
#endif

/*
	=========================================================
	RTFM!!!!
	
	Be sure to check out the user's guide for the menu system
	to get an idea of what all this code's for. Heurghh!!
	=========================================================
*/

namespace Front
{


const float CScreenElement::vJUST_LEFT		= -1.0f;
const float CScreenElement::vJUST_TOP		= -1.0f;
const float CScreenElement::vJUST_CENTER	= 0.0f;
const float CScreenElement::vJUST_RIGHT		= 1.0f;
const float CScreenElement::vJUST_BOTTOM	= 1.0f;

const float CScreenElement::AUTO_Z_SPACE	= 1.0f;

CScreenElement::CScreenElement()
	: CObject()
{
	m_key_time = 0;
	mp_parent = NULL;
	mp_child_list = NULL;
	mp_prev_sibling = NULL;
	mp_next_sibling = NULL;

	m_local_props.SetScale(1.0f, 1.0f);
	m_local_props.SetAbsoluteScale(1.0f, 1.0f);
	m_local_props.SetScreenPos(0.0f, 0.0f);
	m_local_props.SetRotate( 0.0f );
	m_local_props.SetWorldPos( Mth::Vector(0.0f, 0.0f, 0.0f) );
	
	Image::RGBA default_rgba;
	default_rgba.r = 128;
	default_rgba.g = 128;
	default_rgba.b = 128;
	default_rgba.a = 128;
	m_local_props.SetRGBA( default_rgba );

	m_local_props.alpha = 1.0f;
	m_target_local_props = m_local_props;
	m_summed_props = m_local_props;

	m_base_w = 0.0f;
	m_base_h = 0.0f;

	m_rgba.r = 128;
	m_rgba.g = 128;
	m_rgba.b = 128;
	m_rgba.a = 128;
	
	m_z_priority = 0.0f;

	m_originalAlpha = 1.0f;
	
	m_object_flags |= vNEEDS_LOCAL_POS_CALC;
	m_object_flags |= vIS_SCREEN_ELEMENT;
}




CScreenElement::~CScreenElement()
{
	set_parent(NULL, vDONT_RECALC_POS);
	m_id = 0xDEADBEEF;
}




// absolute upper-left, last frame's
float CScreenElement::GetAbsX()
{
	return m_summed_props.GetScreenUpperLeftX();
}




// absolute upper-left, last frame's
float CScreenElement::GetAbsY()
{
	return m_summed_props.GetScreenUpperLeftY();
}




// absolute width, includes scale, last frame's
float CScreenElement::GetAbsW()
{
	return m_base_w * m_summed_props.GetAbsoluteScaleX();
}




// absolute height, includes scale, last frame's
float CScreenElement::GetAbsH()
{
	return m_base_h * m_summed_props.GetAbsoluteScaleY();
}




/*
	Position of anchor point, in parent's space. 'Target' means where it's going,
	not necessarily where it is.
*/
void CScreenElement::GetLocalPos(float *pX, float *pY)
{
	Dbg_Assert(pX);
	Dbg_Assert(pY);
	
	*pX = m_target_local_props.GetScreenPosX();
	*pY = m_target_local_props.GetScreenPosY();
}




/*
	Position of upper left corner of element, in parent's space. 'Target' means where 
	it's going, not necessarily where it is.
*/
void CScreenElement::GetLocalULPos(float *pX, float *pY)
{
	Dbg_Assert(pX);
	Dbg_Assert(pY);
	
	*pX = m_target_local_props.GetScreenUpperLeftX();
	*pY = m_target_local_props.GetScreenUpperLeftY();
}




/*
	Justification is a fixed attribute (doesn't morph)
*/
void CScreenElement::GetJust(float *pJustX, float *pJustY)
{
	Dbg_Assert(pJustX);
	Dbg_Assert(pJustY);
	
	*pJustX = m_just_x;
	*pJustY = m_just_y;
}




/* 
	Sets desired local position, element will morph from current position to
	that position over set morph time (unless force_instant set).
*/
void CScreenElement::SetPos(float x, float y, EForceInstant forceInstant)
{
	Dbg_MsgAssert(!(m_object_flags & CScreenElement::v3D_POS), ("Illegal to set the 2D position of a 3D screen element"));

	if (m_target_local_props.GetScreenPosX() != x || m_target_local_props.GetScreenPosY() != y)
	{
		m_target_local_props.SetScreenPos(x, y);
		m_object_flags |= vNEEDS_LOCAL_POS_CALC;
		compute_ul_pos(m_target_local_props);
	}
	if (forceInstant) 
	{
		if (m_local_props.GetScreenPosX() != x || m_local_props.GetScreenPosY() != y)
		{
			// on the next update, this element will arrive at the target position
			m_local_props.SetScreenPos(x, y);
			m_object_flags |= vNEEDS_LOCAL_POS_CALC;
		}
	}
}


/* 
	Sets desired world position, element will morph from current position to
	that position over set morph time (unless force_instant set).
*/
void CScreenElement::SetPos3D(const Mth::Vector & pos3D, EForceInstant forceInstant)
{
	if (m_target_local_props.GetWorldPos() != pos3D )
	{
		m_target_local_props.SetWorldPos(pos3D);
		m_object_flags |= vNEEDS_LOCAL_POS_CALC;
		compute_ul_pos(m_target_local_props);
	}
	if (forceInstant) 
	{
		if (m_local_props.GetWorldPos() != pos3D )
		{
			// on the next update, this element will arrive at the target position
			m_local_props.SetWorldPos(pos3D);
			m_object_flags |= vNEEDS_LOCAL_POS_CALC;
		}
	}

	m_object_flags |= CScreenElement::v3D_POS;
}


/*
	Set 'force' if dims being set from outside element. When that happens, element cannot
	change own dims.
	
	Well, actually it can (VMenu), but 'force' is taken is a suggestion.
*/
void CScreenElement::SetDims(float w, float h, EForceDims force)
{
	if (m_base_w != w || m_base_h != h)
	{
		m_object_flags |= vNEEDS_LOCAL_POS_CALC;
		m_base_w = w;
		m_base_h = h;
		compute_ul_pos(m_target_local_props);
	}
	
	if (force) 
		m_object_flags |= vFORCED_DIMS;
	else
		m_object_flags &= ~vFORCED_DIMS;
}




/*
	Justification effects where the upper-left corner of an element is relative to 
	the element's assigned position (anchor point). 
	
	(-1.0, -1.0):	anchor is at upper-left of element
	(0.0, 0.0):		anchor is in center of element
	(1.0, 1.0):		anchor is at lower-right of element
	
	You can figure out the meaning of other numbers.
*/
void CScreenElement::SetJust(float justX, float justY)
{
	if (m_just_x != justX || m_just_y != justY)
	{
		// calculate justification differences and adjust new position so the item still appears to
		// have the same location as before
		float x_diff = m_local_props.GetScaleX() * m_base_w * (justX - m_just_x) / 2.0f;
		float y_diff = m_local_props.GetScaleY() * m_base_h * (justY - m_just_y) / 2.0f;
		m_local_props.SetScreenPos(m_local_props.GetScreenPosX() + x_diff, m_local_props.GetScreenPosY() + y_diff);
		m_target_local_props.SetScreenPos(m_target_local_props.GetScreenPosX() + x_diff, m_target_local_props.GetScreenPosY() + y_diff);
		//m_local_props.ulx += x_diff;
		//m_local_props.uly += y_diff;

		m_just_x = justX;
		m_just_y = justY;
		m_object_flags |= vNEEDS_LOCAL_POS_CALC;
		
		compute_ul_pos(m_target_local_props);
	}
}




/* 
	Sets desired local scale, element will morph from current scale to
	that scale over set morph time (unless force_instant set).
*/
void CScreenElement::SetScale(float scaleX, float scaleY, bool relative, EForceInstant forceInstant )
{
	if (m_target_local_props.GetScaleX() != scaleX || m_target_local_props.GetScaleY() != scaleY || relative)
	{
		if ( !relative )
		{
			m_target_local_props.SetAbsoluteScale(scaleX, scaleY);
		}
		else
		{
			scaleX *= m_target_local_props.GetAbsoluteScaleX();
			scaleY *= m_target_local_props.GetAbsoluteScaleY();
		}
		m_target_local_props.SetScale(scaleX, scaleY);
		compute_ul_pos(m_target_local_props);
		m_object_flags |= vNEEDS_LOCAL_POS_CALC;
	}
	if (forceInstant) 
	{
		if (m_local_props.GetScaleX() != scaleX || m_local_props.GetScaleY() != scaleY)
		{
			// on the next update, this element will arrive at the target scale
			m_local_props.SetScale(scaleX, scaleY);
			m_object_flags |= vNEEDS_LOCAL_POS_CALC;
		}
	}
}




/* 
	Sets desired local alpha, element will morph from current alpha to
	that alpha over set morph time (unless force_instant set).
*/
void CScreenElement::SetAlpha(float alpha, EForceInstant forceInstant)
{
	if (m_target_local_props.alpha != alpha)
	{
		m_target_local_props.alpha = alpha;
		m_object_flags |= vNEEDS_LOCAL_POS_CALC;
	}
	if (forceInstant) 
	{
		if (m_local_props.alpha != alpha)
		{
			// on the next update, this element will arrive at the target alpha
			m_local_props.alpha = alpha;
			m_object_flags |= vNEEDS_LOCAL_POS_CALC;
		}
	}
}




/*
	Specifies the amount of time over which this element will changes from its
	current morphable properties (pos, scale, alpha) to the target ones. If
	an animation already in progress, override previously set time.
*/
void CScreenElement::SetAnimTime(Tmr::Time animTime)
{
	m_base_time = Tmr::GetTime();
	m_key_time = animTime;
	m_last_motion_grad = 0.0f;
	m_last_pct_changed = 0.0f;
	
	if (!animTime)
	{
		m_local_props = m_target_local_props;
	}
	m_object_flags |= vNEEDS_LOCAL_POS_CALC;
	m_object_flags &= ~vMORPHING_PAUSED;
	m_object_flags &= ~vMORPHING_PAUSED2;
}




/*
	Note: The alpha component here is multiplied by the alpha from SetAlpha() to
	determine final alpha.
*/
void CScreenElement::SetRGBA( Image::RGBA rgba, EForceInstant forceInstant )
{
	Image::RGBA target_rgba = m_target_local_props.GetRGBA();
	if ( target_rgba.r != rgba.r || target_rgba.g != rgba.g || target_rgba.b != rgba.b || target_rgba.a != rgba.a )
	{
		m_target_local_props.SetRGBA( rgba );
		m_object_flags |= vNEEDS_LOCAL_POS_CALC;
	}
	if ( forceInstant )
	{
		m_local_props.SetRGBA( rgba );
		m_object_flags |= vNEEDS_LOCAL_POS_CALC;
	}
}


/*
	Used to force drawing order entry (instead of it being automatically assigned). Client code should be 
	written using knowledge of how these values work.
*/
void CScreenElement::SetZPriority(float zPriority)
{
	m_z_priority = zPriority;
	m_object_flags |= vCHANGED_STATIC_PROPS;
	m_object_flags |= vCUSTOM_Z_PRIORITY;
}




void CScreenElement::SetMorphPausedState(bool pause)
{
	if (pause)
		m_object_flags |= vMORPHING_PAUSED;
	else
	{
		m_object_flags &= ~vMORPHING_PAUSED;
		m_object_flags &= ~vMORPHING_PAUSED2;
	}
}




/*
	When element is unlocked, children can be added or removed, otherwise they can't. Really just an 
	optimization so that lots of children can be added en masse without slowdown.
	
	Note: The children you add to an element don't officially exist until you lock the element again,
	so don't forget to do this.
*/
void CScreenElement::SetChildLockState(CScreenElement::ELockState lockState)
{
	ELockState current_lock_state = UNLOCK;
	if (m_object_flags & CScreenElement::vCHILD_LOCK) current_lock_state = LOCK;
	if (lockState == current_lock_state)
		return;

	if (lockState == LOCK)
		m_object_flags |= CScreenElement::vCHILD_LOCK;
	else
		m_object_flags &= ~CScreenElement::vCHILD_LOCK;

	Obj::CTracker* p_tracker = Obj::CTracker::Instance();
	
	if (lockState == LOCK) 
	{	
		//Ryan("Setting child lock on, item %s\n", Script::FindChecksumName(m_id));
		
		// send out notice that child lock has been turned on
		p_tracker->LaunchEvent(Obj::CEvent::TYPE_NOTIFY_CHILD_LOCK, m_id);
	}
	else
	{
		//Ryan("Setting child lock off, item %s\n", Script::FindChecksumName(m_id));
		
		// send out notice that child lock has been turned off
		p_tracker->LaunchEvent(Obj::CEvent::TYPE_NOTIFY_CHILD_UNLOCK, m_id);
	}
}




CScreenElement* CScreenElement::GetLastChild()
{
	CScreenElement* pChild = mp_child_list;
	while(pChild)
	{
		if (!pChild->GetNextSibling())
			break;
		
		pChild = pChild->GetNextSibling();
	}

	return pChild;
}




/*
	Returns a pointer to the child with that ID.
*/
CScreenElement* CScreenElement::GetChildById(uint32 id, int *pIndex)
{
	return get_child_by_id_or_index(id, pIndex);
}




/*
	Returns a pointer to the Nth child, where N = index.
	
	The children are indexed in the order they are added. If one is removed,
	then the indices of all subsequent children are shifted down a notch.
*/
CScreenElement* CScreenElement::GetChildByIndex(int index)
{
	return get_child_by_id_or_index(0, &index);
}




/*
	Returns number of children.
*/
int CScreenElement::CountChildren()
{
	CScreenElementPtr p_child = mp_child_list;
	int count = 0;
	while(p_child)
	{
		p_child = p_child->GetNextSibling();
		count++;
	}
	return count;
}




/*
	An important entry point to CScreenElement from script. Sets the elements static
	(as opposed to morphable) properties.
	
	Note: a virtual function, so can be extended by subclasses
	Note: SetProperties() in a subclass should call this function before doing its own logic
*/
void CScreenElement::SetProperties(Script::CStruct *pProps)
{
	CScreenElementManager *pManager = static_cast<CScreenElementManager *>(mp_manager);
	Dbg_Assert(pManager);

	Script::CPair pos;
	if (pProps->GetPair(CRCD(0x7f261953,"pos"), &pos))
	{
		// in this case, position will be forced instantly to the specified position
		SetPos(pos.mX, pos.mY, FORCE_INSTANT);
	}

	Mth::Vector pos3D;
	if (pProps->GetVector(CRCD(0x4b491900,"pos3D"), &pos3D))
	{
		pos3D[W] = 1.0f;		// force to be a point
		//Dbg_Message("************ position (%f, %f, %f)", pos3D[X], pos3D[Y], pos3D[Z]);
		SetPos3D(pos3D, FORCE_INSTANT);
	}

	Script::CPair dims;
	if (pProps->GetPair(CRCD(0x34a68574,"dims"), &dims))
	{
		SetDims(dims.mX, dims.mY, FORCE_DIMS);
	}

	uint32 parent_id = pManager->ResolveComplexID(pProps, CRCD(0xc2719fb0,"parent"));
	if (parent_id)
	{
		CScreenElementPtr pParent = pManager->GetElement(parent_id);
		#ifdef __NOPT_ASSERT__
		
		if (pParent->GetType() == CScreenElement::TYPE_TEXT_BLOCK_ELEMENT)
		{
			if (GetType() != CScreenElement::TYPE_TEXT_ELEMENT)
			{
				Script::PrintContents(pProps);
				Dbg_MsgAssert(0,("TextBlockElement %s parenting child %s that is NOT a CTextElement (it's %s)\nIn CScreenElement::SetProperties\n^^^ SEE STRUCTURE ABOVE MEM DUMP & TELL BRAD^^^\n",
					Script::FindChecksumName(pParent->GetID()), Script::FindChecksumName(GetID()), Script::FindChecksumName(GetType())));
			}
		}
		
		if (!pParent)
		{
			Script::PrintContents(pProps, 2);
			Dbg_MsgAssert(0, ("could not resolve parent, see struct above"));
		}
		#endif
		pManager->SetParent(pParent, this);
	}

	float just[2] = { 0.0f, 0.0f };
	if (resolve_just(pProps, CRCD(0x8b60022f,"just"), just, just+1))
		SetJust(just[0], just[1]);
	
	
	Image::RGBA rgba;
	if (resolve_rgba(pProps, CRCD(0x3f6bcdba,"rgba"), &rgba))
	{
		SetRGBA(rgba);
	}
		
	float z_priority;
	if (pProps->GetFloat(CRCD(0x57710f31,"z_priority"), &z_priority))
	{
		SetZPriority(z_priority);
	}
	
	if (pProps->ContainsFlag(CRCD(0x1d944426,"not_focusable")))
	{
		// sets a flag
		SetChecksumTag(NONAME, CRCD(0xf33a3321,"tag_not_focusable"));
	}
	else if (pProps->ContainsFlag(CRCD(0x5cdecf29,"focusable")))
	{
		RemoveFlagTag(CRCD(0xf33a3321,"tag_not_focusable"));
	}
	
	uint32 local_id;
	if (pProps->GetChecksum(CRCD(0xa2a5defe,"local_id"), &local_id))
	{
		SetChecksumTag(CRCD(0x9494a525,"tag_local_id"), local_id); 
	}

	if (pProps->ContainsFlag(CRCD(0xff85d4d7,"unpause")))
	{
		SetMorphPausedState(false);
	}

	if ( pProps->ContainsFlag( CRCD(0xe5d6fe3e,"block_events") ) )
	{
		m_object_flags |= vEVENTS_BLOCKED;
	}
	else if ( pProps->ContainsFlag( CRCD(0xd5614a50,"unblock_events") ) )
	{
		m_object_flags &= ~vEVENTS_BLOCKED;
	}

	if ( pProps->ContainsFlag( CRCD(0x5b6634d4,"hide") ) )
	{
		m_object_flags |= vHIDDEN;
	}
	else if ( pProps->ContainsFlag( CRCD(0xb60d1f35,"unhide") ) )
	{
		m_object_flags &= ~vHIDDEN;
		m_object_flags |= vCHANGED_STATIC_PROPS;
	}
	
	CObject::SetProperties(pProps);
		
	return;
}


void CScreenElement::WritePropertiesToStruct( Script::CStruct *pStruct )
{
	// alpha value
	pStruct->AddFloat( CRCD(0x2f1fc695,"alpha"), m_local_props.alpha );
	
	// rgba array
	Image::RGBA current_rgba = m_local_props.GetRGBA();
	Script::CArray* p_rgba = new Script::CArray();
	p_rgba->SetSizeAndType( 4, ESYMBOLTYPE_INTEGER );
	p_rgba->SetInteger( 0, current_rgba.r );
	p_rgba->SetInteger( 1, current_rgba.g );
	p_rgba->SetInteger( 2, current_rgba.b );
	p_rgba->SetInteger( 3, current_rgba.a );
	pStruct->AddArray( CRCD(0x3f6bcdba,"rgba"), p_rgba );
	Script::CleanUpArray( p_rgba );
	delete p_rgba;

	// scale value
	float scaleX = m_local_props.GetScaleX();
	float scaleY = m_local_props.GetScaleY();
	// only add a pair if they're different
	if ( scaleX == scaleY )
		pStruct->AddFloat( CRCD(0x13b9da7b,"scale"), scaleX );
	else
		pStruct->AddPair( CRCD(0x13b9da7b,"scale"), scaleX, scaleY );
		
	// position
	pStruct->AddPair( CRCD(0x7f261953,"pos"), m_local_props.GetScreenPosX(), m_local_props.GetScreenPosY() );

	// are the events blocked?
	if ( EventsBlocked() )
		pStruct->AddInteger( CRCD(0x73cb6d65,"events_blocked"), 1 );
	else
		pStruct->AddInteger( CRCD(0x73cb6d65,"events_blocked"), 0 );
}


/*
	Another important entry point to CScreenElement from script. Sets the elements morphable
	(as opposed to static) properties.
	
	Note: a virtual function, so can be extended by subclasses.
	Note: SetMorph() in a subclass should call this function before doing its own logic
*/
void CScreenElement::SetMorph(Script::CStruct *pProps)
{	
	float pos_x, pos_y;
	if (resolve_pos(pProps, &pos_x, &pos_y))
	{
		SetPos(pos_x, pos_y);
	}

	// TODO: temp hack for restoring alpha values
	if ( pProps->ContainsFlag( CRCD(0x5d8bb535,"restore_alpha") ) )
		SetAlpha( m_originalAlpha );
	
	float alpha;
	if ( pProps->GetFloat(CRCD(0x2f1fc695,"alpha"), &alpha) )
	{
		// TODO: temp hack for remembering alpha values		
		if ( pProps->ContainsFlag( CRCD(0xff8df26b,"remember_alpha") ) )
			m_originalAlpha = m_local_props.alpha;
		
		SetAlpha( alpha );
	}

	float scale;
	int relative_scale = 0;
	if ( pProps->ContainsFlag( CRCD(0xe1f4711f,"relative_scale") ) )
		relative_scale = 1;
	if (pProps->GetFloat(CRCD(0x13b9da7b,"scale"), &scale))
	{
		SetScale(scale, scale, relative_scale );
	}
	Script::CPair scale_pair;
	if (pProps->GetPair(CRCD(0x13b9da7b,"scale"), &scale_pair))
	{
		SetScale(scale_pair.mX, scale_pair.mY, relative_scale);
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

	uint32 anim_type = 0;
	// clear affected bits
	m_object_flags &= ~(vANIM_BIT_MASK << vANIM_FIRST_BIT);
	if (pProps->GetChecksum(CRCD(0x98549ba4,"anim"), &anim_type))
	{
		if (anim_type == CRCD(0xf5b95ce4, "fast_out"))
			m_object_flags |= vANIM_FAST_OUT << vANIM_FIRST_BIT;
		else if (anim_type == CRCD(0x6aa7dd49, "fast_in"))
			m_object_flags |= vANIM_FAST_IN << vANIM_FIRST_BIT;
		else if (anim_type == CRCD(0x768f6843, "gentle"))
			m_object_flags |= vANIM_SLOW_INOUT << vANIM_FIRST_BIT;
	}
	if (!anim_type)
	{
		m_object_flags |= (vANIM_LINEAR << vANIM_FIRST_BIT);
	}

	// rgba values
	Image::RGBA rgba;
	if ( resolve_rgba( pProps, CRCD(0x3f6bcdba,"rgba"), &rgba ) )
	{
		SetRGBA( rgba, DONT_FORCE_INSTANT );
	}

}




/*
	This function must be implemented for access to member commands from script.
*/
bool CScreenElement::CallMemberFunction( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript )
{
	if (Checksum == CRCD(0x6c63c7c5, "SetProps"))
	{
		SetProperties(pParams);
	}
    else if (Checksum == CRCD(0xdd0c8df3, "DoMorph"))
	{
		float time;
		if (pParams->GetFloat(CRCD(0x906b67ba,"time"), &time))
		{
			Tmr::Time anim_time = (Tmr::Time) (time * (float) Tmr::vRESOLUTION);
			pScript->WaitTime(anim_time);
		}

		SetMorph(pParams);
	}
	else if (Checksum == CRCD(0xc6870028, "Die"))
	{
		CScreenElementManager* p_manager = CScreenElementManager::Instance();		
		
		CScreenElementPtr p_parent = mp_parent;
		p_parent->SetChildLockState(UNLOCK);
		
		// will kill this screen element and all its children
		// THIS IS POTENTIALLY DANGEROUS, LEAVE THIS FUNCTION ASAP
		p_manager->DestroyElement(m_id, CScreenElementManager::RECURSE, CScreenElementManager::DONT_PRESERVE_PARENT, pScript);
		
		p_parent->SetChildLockState(LOCK);
		return true;
	}
	else if ( Checksum == CRCD(0x9492f814, "GetProps" ) )
	{
		WritePropertiesToStruct( pScript->GetParams() );
	}
	else
	{
		return CObject::CallMemberFunction(Checksum, pParams, pScript);
	}

	return true;
}




/*
	Update Procedure:
	
	(Recurse through all elements in tree, starting at root)
	
	For each element:
	1. Update local position, scale, alpha
	2. Update summed position, scale, alpha
	3. Update: (optional)
		-properties of children
		-underlying CSprite or CText object(s)
		-underlying 3D object(s)
		-own base_w, base_h dimensions
	4. Run these steps on all children

	Events may be launched at various points.
*/
void CScreenElement::UpdateProperties()
{
	#ifdef __NOPT_ASSERT__
	debug_verify_integrity();
	#endif
	//Dbg_MsgAssert(m_id && m_id != 0xDEADBEEF, ("this screen element seems to have been deleted"));
	
	Tmr::Time current_time = Tmr::GetTime();
	
	AddReference();
	
	// for convenience -- once element is being used, we can assume it's locked
	SetChildLockState(LOCK);
	
	// only do expensive calculation when change occurs
	bool needs_summed_pos_calc = false;

	// Update 3D position, if any
	if (m_object_flags & CScreenElement::v3D_POS)
	{
		Nx::CViewport *p_viewport = Nx::CViewportManager::sGetActiveViewport();
		Dbg_MsgAssert(p_viewport, ("Can't find an active viewport"));

		float screen_pos_x, screen_pos_y;
		Nx::ZBufferValue screen_pos_z;
		float scale_3d;
		scale_3d = p_viewport->TransformToScreenCoord(m_target_local_props.GetWorldPos(), screen_pos_x, screen_pos_y, screen_pos_z);
		if (scale_3d >= 0.0f)
		{
			m_target_local_props.SetScreenPos(screen_pos_x, screen_pos_y, screen_pos_z);
			SetScale(scale_3d, scale_3d, true);		// set to relative scale
			m_object_flags &= ~CScreenElement::v3D_CULLED;
		} else {
			m_target_local_props.SetScreenPos(-320.0f, -224.0f);
			m_object_flags |= CScreenElement::v3D_CULLED;
		}
		compute_ul_pos(m_target_local_props);

		if (mp_parent)
		{
			Dbg_MsgAssert(!(mp_parent->m_object_flags & CScreenElement::v3D_POS), ("Can't handle having 3D parent"));
		}

		m_object_flags |= vNEEDS_LOCAL_POS_CALC;
	}

	// continue animation of this element
	if ((m_object_flags & vNEEDS_LOCAL_POS_CALC) && !(m_object_flags & vMORPHING_PAUSED2))
	{
		if (m_key_time)
		{
			float pct_changed = (float) (current_time - m_base_time) / (float) m_key_time;
			// if element has reached its new destination, end animation
			if (current_time - m_base_time >= m_key_time)
			{
				pct_changed = 1.0f;
				m_key_time = 0;
			}
	
			float motion_grad;
			int anim_type = ((m_object_flags >> vANIM_FIRST_BIT) & vANIM_BIT_MASK);
			switch (anim_type)
			{
				case vANIM_FAST_OUT:
					motion_grad = 2.0f * pct_changed - pct_changed * pct_changed;
					break;
				case vANIM_FAST_IN:
					motion_grad = pct_changed * pct_changed;
					break;
				case vANIM_SLOW_INOUT:
					motion_grad = (3.0f - 2.0f * pct_changed) * pct_changed * pct_changed;
					break;
				default:
					motion_grad = pct_changed;
					break;
			}
			
			float motion_inc = (motion_grad - m_last_motion_grad) / (1.0f - m_last_motion_grad);
			float pct_changed_inc = (pct_changed - m_last_pct_changed) / (1.0f - m_last_pct_changed);
			m_local_props.SetScreenPos(m_local_props.GetScreenPosX() + (m_target_local_props.GetScreenPosX() - m_local_props.GetScreenPosX()) * motion_inc, 
									   m_local_props.GetScreenPosY() + (m_target_local_props.GetScreenPosY() - m_local_props.GetScreenPosY()) * motion_inc);
			m_local_props.SetScale(m_local_props.GetScaleX() + (m_target_local_props.GetScaleX() - m_local_props.GetScaleX()) * pct_changed_inc,
								   m_local_props.GetScaleY() + (m_target_local_props.GetScaleY() - m_local_props.GetScaleY()) * pct_changed_inc);
			
			m_local_props.SetRotate( m_local_props.GetRotate() + ( m_target_local_props.GetRotate() - m_local_props.GetRotate() ) * pct_changed_inc);
			
			m_local_props.alpha = m_local_props.alpha + (m_target_local_props.alpha - m_local_props.alpha) * pct_changed_inc;
			compute_ul_pos(m_local_props);

			// figure any new rgba values 
			Image::RGBA current_rgba = m_local_props.GetRGBA();
			Image::RGBA target_rgba = m_target_local_props.GetRGBA();
			Image::RGBA new_rgba;
			new_rgba.r = (int)( current_rgba.r + ( target_rgba.r - current_rgba.r ) * pct_changed_inc );
			new_rgba.g = (int)( current_rgba.g + ( target_rgba.g - current_rgba.g ) * pct_changed_inc );
			new_rgba.b = (int)( current_rgba.b + ( target_rgba.b - current_rgba.b ) * pct_changed_inc );
			new_rgba.a = (int)( current_rgba.a + ( target_rgba.a - current_rgba.a ) * pct_changed_inc );
			m_local_props.SetRGBA( new_rgba );
			
			m_last_motion_grad = motion_grad;
			m_last_pct_changed = pct_changed;
		
			/*
			if (m_key_time == 0)
			{
				//m_local_props = m_target_local_props;
				m_local_props.ulx = m_local_props.x - m_local_props.scalex * m_base_w * (m_just_x + 1.0f) / 2.0f;
				m_local_props.uly = m_local_props.y - m_local_props.scaley * m_base_h * (m_just_y + 1.0f) / 2.0f;
				printf("Hurk!! target UL=(%.2f,%.2f) local UL=(%.2f,%.2f)\n", 
					   m_target_local_props.ulx, m_target_local_props.uly,
					   m_local_props.ulx, m_local_props.uly);
			}
			*/
		}
		else
		{
			m_local_props = m_target_local_props;
			m_object_flags &= ~vNEEDS_LOCAL_POS_CALC;
		}
		needs_summed_pos_calc = true;
		
		/*
		if (GetID() == Script::GenerateCRC("test_h_menu"))
			printf("local_pos=(%.2f,%.2f), ul=(%.2f,%.2f), dims=(%.2f,%.2f)\n", 
				   m_local_props.x, m_local_props.y, 
				   m_local_props.ulx, m_local_props.uly, 
				   m_base_w, m_base_h);
		*/
	} // end if

	if (mp_parent && (mp_parent->m_object_flags & vDID_SUMMED_POS_CALC))
		needs_summed_pos_calc = true;
	m_object_flags &= ~vDID_SUMMED_POS_CALC;
	
	if (needs_summed_pos_calc)
	{
		if (mp_parent && !(m_object_flags & CScreenElement::v3D_POS))
		{
			m_summed_props.SetScale(m_local_props.GetScaleX() * mp_parent->m_summed_props.GetScaleX(),
									m_local_props.GetScaleY() * mp_parent->m_summed_props.GetScaleY());
			m_summed_props.SetScreenPos(mp_parent->m_summed_props.GetScreenUpperLeftX() + m_local_props.GetScreenPosX() * mp_parent->m_summed_props.GetScaleX(),
										mp_parent->m_summed_props.GetScreenUpperLeftY() + m_local_props.GetScreenPosY() * mp_parent->m_summed_props.GetScaleY());
			m_summed_props.alpha = m_local_props.alpha * mp_parent->m_summed_props.alpha;
			m_summed_props.SetScreenUpperLeft(mp_parent->m_summed_props.GetScreenUpperLeftX() + m_local_props.GetScreenUpperLeftX() * mp_parent->m_summed_props.GetScaleX(),
											  mp_parent->m_summed_props.GetScreenUpperLeftY() + m_local_props.GetScreenUpperLeftY() * mp_parent->m_summed_props.GetScaleY());
		}
		else
			m_summed_props = m_local_props;
		m_object_flags |= vDID_SUMMED_POS_CALC;
	} // end if

	update();
	
	m_object_flags &= ~vCHANGED_STATIC_PROPS;
	if (m_object_flags & vMORPHING_PAUSED)
		m_object_flags |= vMORPHING_PAUSED2;

	// update all children
	//CScreenElementPtr pChild = mp_child_list;
	CScreenElement * pChild = mp_child_list;
	while(pChild)
	{
		pChild->UpdateProperties();
		pChild = pChild->GetNextSibling();
	}

	RemoveReference();
}




CScreenElementPtr CScreenElement::get_child_by_id_or_index(uint32 id, int *pIndex)
{
	int count = 0;
	CScreenElementPtr pChild = mp_child_list;
	while(pChild)
	{
		// if we are getting by index
		if (id == 0) 
		{
			if (*pIndex == count)
				break;
		}
		// if we are getting by ID
		else if (pChild->GetID() == id) 
		{
			// save index
			if (pIndex)
				*pIndex = count;
			break;
		}

		pChild = pChild->GetNextSibling();
		count++;
	}

	return pChild;
}


CScreenElementPtr CScreenElement::get_parent()
{
	return mp_parent;
}


CWindowElement * CScreenElement::get_window()
{
	CScreenElement * p_parent = get_parent();
	while (p_parent)
	{
		if (p_parent->GetType() == (sint) TYPE_WINDOW_ELEMENT)
		{
			return static_cast<CWindowElement *>(p_parent);
		}

		p_parent = p_parent->get_parent();
	}

	Dbg_MsgAssert(0, ("Could not find window for CScreenElement."));
	return NULL;
}

/*
	Private function, should not be called by anything except
	CScreenElementManager.
*/
void CScreenElement::set_parent(const CScreenElementPtr &pParent, EPosRecalc recalculatePosition)
{
	if (pParent)	
	{
		Dbg_MsgAssert(!(pParent->m_object_flags & CScreenElement::vCHILD_LOCK), ("can't add child %s -- locked parent - %s", Script::FindChecksumName(m_id), Script::FindChecksumName( pParent->GetID())));
		//Ryan("parent of screen element %s is now %s\n", Script::FindChecksumName(GetID()), Script::FindChecksumName(pParent->GetID()));
		#ifdef	__NOPT_ASSERT__
		if (pParent->GetType() == CScreenElement::TYPE_TEXT_BLOCK_ELEMENT)
		{
			Dbg_MsgAssert(GetType() == CScreenElement::TYPE_TEXT_ELEMENT,("TextBlockElement %s parenting child %s that is NOT a CTextElement (it's %s)\n",
			Script::FindChecksumName(pParent->GetID()), Script::FindChecksumName(GetID()), Script::FindChecksumName(GetType())));
		}
		#endif
	}



	if (mp_parent)
	{
		// remove from existing parent's child list before setting new
		// (there are max. 5 references to remove)
		
		
		CScreenElementPtr p_elem = mp_parent->mp_child_list;
		while(p_elem)
		{
			if (p_elem == this)
			{
				if (GetPrevSibling())
				{
					GetPrevSibling()->mp_next_sibling = GetNextSibling();
				}
				else
				{
					// ref count won't change for next sibling, since was ref'd by this
					mp_parent->mp_child_list = GetNextSibling();
				}
				
				if (GetNextSibling())
				{
					// Either original parent or original prev sibling is now pointing to original next sibling
					// Would both add and remove a reference on next, but they cancel each other, so do nothing

					GetNextSibling()->mp_prev_sibling = GetPrevSibling();
				}

				break;
			}
			
			p_elem = p_elem->GetNextSibling();
		}
	}

	// add to new parent's child list
	// (at end of list)
	mp_parent = pParent;
	if (mp_parent) 
	{
		CScreenElementPtr p_prev_sib = mp_parent->GetFirstChild();
		if (p_prev_sib)
		{
			while(p_prev_sib->GetNextSibling())
				p_prev_sib = p_prev_sib->GetNextSibling();
			p_prev_sib->mp_next_sibling = this;
		}
		else
			mp_parent->mp_child_list = this;
		mp_prev_sibling = p_prev_sib;
	}
	else
		// no parent, therefore no siblings
		mp_prev_sibling = NULL;
	mp_next_sibling = NULL;

	if (pParent)
		auto_set_z_priorities_recursive(pParent->m_z_priority + AUTO_Z_SPACE);
	else
		auto_set_z_priorities_recursive(0.0f);
}




/*
	...given element's dimensions, justification, scale, and target position.
*/
void CScreenElement::compute_ul_pos(ConcatProps &props)
{
	props.SetScreenUpperLeft(props.GetScreenPosX() - props.GetScaleX() * m_base_w * (m_just_x + 1.0f) / 2.0f,
							 props.GetScreenPosY() - props.GetScaleY() * m_base_h * (m_just_y + 1.0f) / 2.0f);
}




// called by set_parent()
void CScreenElement::auto_set_z_priorities_recursive(float topPriority)
{
	if (!(m_object_flags & vCUSTOM_Z_PRIORITY))
	{
		m_object_flags |= vCHANGED_STATIC_PROPS;
		m_z_priority = topPriority;
	}
	
	CScreenElementPtr pChild = mp_child_list;
	while(pChild)
	{
		pChild->auto_set_z_priorities_recursive(topPriority + AUTO_Z_SPACE);
		pChild = pChild->GetNextSibling();
	}	
}




/* 
	Resolves position info stored in a script structure:
	
	pos=(x y)
	pos={relative (x y)}
	pos={proportional (x y)}
	
	Returns false if no position info found.
*/
bool CScreenElement::resolve_pos(Script::CStruct *pProps, float *pX, float *pY)
{
	Script::CPair pos_pair;
	Script::CStruct *p_pos_struct;
	if (pProps->GetStructure(CRCD(0x7f261953,"pos"), &p_pos_struct))
	{
		p_pos_struct->GetPair(NONAME, &pos_pair, true);
		if (p_pos_struct->ContainsFlag(0x91a4c826)) // "relative"
		{
			*pX = m_target_local_props.GetScreenPosX() + pos_pair.mX;
			*pY = m_target_local_props.GetScreenPosY() + pos_pair.mY;
		}
		else if (p_pos_struct->ContainsFlag(0xb922906a)) // "proportional"
		{
			Dbg_Assert(mp_parent);
			*pX = pos_pair.mX * mp_parent->GetBaseW();
			*pY = pos_pair.mY * mp_parent->GetBaseH();
		}
		else
			Dbg_MsgAssert(0, ("position needs 'relative' or 'proportional' flag"));
		return true;
	}	
	else if (pProps->GetPair(CRCD(0x7f261953,"pos"), &pos_pair))
	{
		*pX = pos_pair.mX;
		*pY = pos_pair.mY;
		return true;
	}
	
	return false;
}




/* 
	Resolves justification info stored in a script structure, e.g.:
	
	just=[left top]
	just=[center center]
	just=[right bottom]
	just=(-1.0 1.0)
	just=(.65 3.75)
	
	Returns false if no justification info found.
*/
bool CScreenElement::resolve_just(Script::CStruct *pProps, const uint32 RefName, float *pHJust, float *pVJust)
{
	Script::CArray *p_just;
	if (pProps->GetArray(RefName, &p_just))
	{
		Script::ESymbolType type = (Script::ESymbolType) p_just->GetType();
		switch(type)
		{
			case ESYMBOLTYPE_FLOAT:
				*pHJust = p_just->GetFloat(0);
				*pVJust = p_just->GetFloat(1);
				break;
			case ESYMBOLTYPE_NAME:
			{
				float just[2];
				for (int i = 0; i < 2; i++)
				{
					uint32 crc = p_just->GetNameChecksum(i); 
					if (crc == CRCD(0x85981897, "left") || crc == CRCD(0xe126e035, "top"))
						just[i] = -1.0f;
					else if (crc == CRCD(0x4b358aeb, "right") || crc == CRCD(0x76a08d5b, "bottom"))
						just[i] = 1.0f;
					else
						just[i] = 0;
				}
				*pHJust = just[0];
				*pVJust = just[1];
				break;
			}
			default:
				Dbg_MsgAssert(0, ("wrong type for justification"));
				break;
		}
		return true;
	}
	return false;
}




/*
	Retrieves RGBA information from a script struct.
*/
bool CScreenElement::resolve_rgba(Script::CStruct *pProps, const uint32 RefName, Image::RGBA *pRGBA)
{
	Script::CArray *p_color;
	if (pProps->GetArray(RefName, &p_color))
	{
		uint32 rgba = 0;
		int size = p_color->GetSize();
		Dbg_MsgAssert(size >= 3 && size <= 4, ("wrong size %d for color array", size));
		for (int i = 0; i < size; i++) 
		{
			rgba |= (p_color->GetInteger(i) & 255) << (i*8);
		}
#ifdef __PLAT_NGC__
		rgba = ( ( rgba & 0xff000000 ) >> 24 ) | ( ( rgba & 0xff0000 ) >> 8 ) | ( ( rgba & 0xff00 ) << 8 ) | ( ( rgba & 0xff ) << 24 );
#endif		// __PLAT_NGC__
		*pRGBA = *((Image::RGBA *) &rgba);
		return true;
	}
	return false;
}




#ifdef __NOPT_ASSERT__
/*
	Used to verify that screen element has not been corrupted.
*/
void CScreenElement::debug_verify_integrity()
{
	
// Mick - Removed for now, as it's taking up rather a lot of time....	
	return;
	
	Dbg_MsgAssert(Mem::Valid(this), ("not a valid block!"));
	Dbg_MsgAssert(m_id, ("screen element has id of 0, that shouldn't happen"));
	Dbg_MsgAssert(m_id != 0xDEADBEEF, ("screen element has been deleted"));

	CScreenElementManager* p_manager = CScreenElementManager::Instance();
	CScreenElementPtr p_in_tracker = p_manager->GetElement(m_id);
	Dbg_MsgAssert(p_in_tracker == this, ("this element seems to have been lost from tracking"));
	
	if (mp_parent)
	{
		Dbg_MsgAssert(!((uint) mp_parent.Convert() & 3), ("parent has goofy address"));
		Dbg_MsgAssert(Mem::Valid(mp_parent), ("parent not a valid block!"));
		CScreenElementPtr p_sib = mp_parent->mp_child_list;
		while(p_sib)
		{
			if (p_sib == this)
				break;
			p_sib = p_sib->GetNextSibling();
		}
		Dbg_MsgAssert(p_sib, ("this element not in its parent's child list"));
	}

	CScreenElementPtr p_child = mp_child_list;
	while(p_child)
	{
		Dbg_MsgAssert(!((uint) (p_child.Convert()) & 3), ("child has goofy address"));
		Dbg_MsgAssert(Mem::Valid(p_child), ("child not a valid block!"));
		Dbg_MsgAssert(p_child->mp_parent == this, ("this element not parent of child"));
		p_child = p_child->GetNextSibling();
	}
}
#endif




CContainerElement::CContainerElement()
{
	//Ryan("I am a new CContainerElement\n");
	m_focusable_child = 0;

	SetType(CScreenElement::TYPE_CONTAINER_ELEMENT);
}




CContainerElement::~CContainerElement()
{
	//Ryan("Destroying CContainerElement\n");
}




void CContainerElement::SetProperties(Script::CStruct *pProps)
{
	pProps->GetChecksum("focusable_child", &m_focusable_child);
	
	CScreenElement::SetProperties(pProps);
}




bool CContainerElement::PassTargetedEvent(Obj::CEvent *pEvent, bool broadcast)
{
	if (!Obj::CObject::PassTargetedEvent(pEvent))
	{
		return false;
	}
	
	if (pEvent->GetType() == Obj::CEvent::TYPE_FOCUS || pEvent->GetType() == Obj::CEvent::TYPE_UNFOCUS)
	{
		// forward events of these types to specified focusable child
		if (m_focusable_child) 
		{
			int controller = Obj::CEvent::sExtractControllerIndex(pEvent);
			Script::CStruct data;
			data.AddInteger(CRCD(0xb30d9965,"controller"), controller);

			Obj::CTracker* p_tracker = Obj::CTracker::Instance();
			p_tracker->LaunchEvent(pEvent->GetType(), m_focusable_child, Obj::CEvent::vSYSTEM_EVENT, &data);
		}
		pEvent->MarkHandled(m_id);
	}
	return true;
}

bool CScreenElement::EventsBlocked()
{
	// recurse back up through the parents to find any parent that is hidden
	if ( ( m_object_flags & vEVENTS_BLOCKED) || ( mp_parent && mp_parent->EventsBlocked() ) )
		 return true;
	return false;
}

bool CScreenElement::IsHidden()
{
	// recurse back up through the parents to find any parent that is hidden
	if ( ( m_object_flags & vHIDDEN ) || ( mp_parent && mp_parent->IsHidden() ) )
		 return true;
	return false;
}

} // end namespace
