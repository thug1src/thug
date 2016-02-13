//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       CameraLookAroundComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  4/14/3
//****************************************************************************

#ifdef TESTING_GUNSLINGER

#include <gel/components/gunslingercameralookaroundcomponent.cpp>

#else

#include <gel/components/cameralookaroundcomponent.h>
#include <gel/components/inputcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <gfx/debuggfx.h>
						   
namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CCameraLookAroundComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CCameraLookAroundComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCameraLookAroundComponent::CCameraLookAroundComponent() : CBaseComponent()
{
	SetType( CRC_CAMERALOOKAROUND );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCameraLookAroundComponent::~CCameraLookAroundComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCameraLookAroundComponent::InitFromStructure( Script::CStruct* pParams )
{
	mLookaroundTilt					= 0.0f;
	mLookaroundHeading				= 0.0f;
	mLookaroundZoom					= 1.0f;
	
	mLookaroundLock					= false;
	mLookaroundOverride				= false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCameraLookAroundComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCameraLookAroundComponent::Finalize (   )
{
	mp_input_component = GetInputComponentFromObject(GetObject());
	
	Dbg_Assert(mp_input_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCameraLookAroundComponent::Update()
{
	CControlPad * p_control_pad;
	
	if (Script::GetInt(CRCD(0x702247a5,"Viewer_controller"), false))
	{
		p_control_pad = &mp_input_component->GetControlPad2();
	}
	else
	{
		p_control_pad = &mp_input_component->GetControlPad();
	}
	

	// Handle pressing of "Select" button
	// 	
	if (!Script::GetInt(CRCD(0xf3e055e1,"select_shift")))
	{
		if (p_control_pad->m_select.GetTriggered())
		{
			printf ("Camera Detected Select Pressed\n");
			p_control_pad->m_select.ClearTrigger();
			// Depending on which skatercam we are, run one of the UserSelectSelect scripts
			if (GetObject()->GetID() == CRCD(0x967c138c,"SkaterCam0"))
			{
				Script::RunScript(CRCD(0x60871393,"UserSelectSelect"));
			}
			else
			{
				Script::RunScript(CRCD(0xa1b1146d,"UserSelectSelect2"));
			}
		}
	}
	
	if (p_control_pad->m_R3.GetTriggered())
	{
		mLookaroundLock = !mLookaroundLock;
		p_control_pad->m_R3.ClearTrigger();
	}
	
	float frame_length = Tmr::FrameLength();
	
	if (!mLookaroundLock && !mLookaroundOverride)
	{
		float target = p_control_pad->m_scaled_rightY;
		target = -1.4f * (target < 0.0f ? (0.7f * target) : (0.5f * target));
		if (Mth::Abs(target - mLookaroundTilt) > 0.001f)
		{
			mLookaroundTilt += (target - mLookaroundTilt) * 3.75f * frame_length;
		}
		else
		{
			mLookaroundTilt = target;
		}
		
		target = 3.0f * p_control_pad->m_scaled_rightX;
		if (Mth::Abs(target - mLookaroundHeading) > 0.001f)
		{
			mLookaroundHeading += (target - mLookaroundHeading) * 3.75f * frame_length;
		}
		else
		{
			mLookaroundHeading = target;
		}
	}
	
	float delta_time = Tmr::FrameLength();

	if (mLookaroundOverride)
	{
		if (mLookaroundDeltaTimer > 0.0f)
		{
			if (mLookaroundDeltaTimer > delta_time)
			{
				mLookaroundHeading		+= mLookaroundHeadingDelta * delta_time;
				mLookaroundTilt			+= mLookaroundTiltDelta * delta_time;
				mLookaroundDeltaTimer	-= delta_time;
			}
			else
			{
				mLookaroundHeading		= mLookaroundHeadingTarget;
				mLookaroundTilt			= mLookaroundTiltTarget;
				mLookaroundDeltaTimer	= 0.0f;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CCameraLookAroundComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | SC_SetSkaterCamOverride | 
        // @parm float | heading |
        // @parm float | tilt | 
        // @parm float | time | 
        // @parm float | zoom | 
		case 0xb8f23899:	// SC_SetSkaterCamOverride
		{
			float heading, tilt, time;
			float zoom = 1.0f;
			pParams->GetFloat( CRCD(0xfd4bc03e,"heading"), &heading, Script::ASSERT );
			pParams->GetFloat( CRCD(0xe3c07609,"tilt"), &tilt, Script::ASSERT );
			pParams->GetFloat( CRCD(0x906b67ba,"time"), &time, Script::ASSERT );
			pParams->GetFloat( CRCD(0x48d4868b,"zoom"), &zoom );

			SetLookaround( heading, tilt, time, zoom );
			
			break;
		}

        // @script | SC_ClearSkaterCamOverride |
		case 0xe3b327c0:	// SC_ClearSkaterCamOverride
			ClearLookaround();
			break;

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCameraLookAroundComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CCameraLookAroundComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCameraLookAroundComponent::SetLookaround( float heading, float tilt, float time, float zoom )
{
	mLookaroundOverride				= true;

	mLookaroundHeadingTarget		= heading;
	mLookaroundTiltTarget			= tilt;

	// Calculate delta - the amount to move in 1 second.
	if (time > 0.0f)
	{
		mLookaroundHeadingDelta		= (heading - mLookaroundHeading) / time;
		mLookaroundTiltDelta		= (tilt - mLookaroundTilt) / time;
	}

	mLookaroundDeltaTimer			= time;
	mLookaroundZoom					= zoom;
}

#if 0 // old control input checking code

char touched[256 * 256 / 8];
bool initialized = false;

void MapInput (   )
{
	if (!initialized)
	{
		for (int n = 255 * 255; n--; )
		{
			touched[n / 8] = 0;
		}
		initialized = true;
	}
	
	CControlPad& pad = mp_input_component->GetControlPad();
	
	int x = static_cast< int >(pad.m_rightX) + 128;
	int y = static_cast< int >(pad.m_rightY) + 128;
	
	// printf("x = %i; y = %i\n", x, y);
	
	int index = (x * 256) + y;
	
	// printf("index / 8 = %i\n", index / 8);
	// printf("MAX = %i\n", 256 * 256 / 8);
	
	touched[index / 8] |= (1 << (index % 8));
}


void DrawMap (   )
{
	Mth::Vector pos = GetObject()->GetPos();
	Mth::Matrix matrix = GetObject()->GetMatrix();
	
	Mth::Vector center = pos - 300.0f * matrix[Z];
	Mth::Vector up = matrix[Y];
	Mth::Vector left = -matrix[X];
	
	for (int n = 255 * 255; n--; )
	{
		if (!(touched[n / 8] & (1 << (n % 8)))) continue;
		
		int x = n / 256;
		int y = n % 256;
		
		Mth::Vector point = center + up * (y - 128) + left * (x - 128);
		
		Gfx::AddDebugLine(point + up + left, point - up - left, MAKE_RGB(255, 255, 255), 0, 1);
		Gfx::AddDebugLine(point + up - left, point - up + left, MAKE_RGB(255, 255, 255), 0, 1);
	}
}

#endif

}

#endif // TESTING_GUNSLINGER
