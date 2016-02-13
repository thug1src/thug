//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterMatrixQueriesComponent.h
//* OWNER:          Dan
//* CREATION DATE:  3/12/3
//****************************************************************************

#include <sk/components/skatermatrixqueriescomponent.h>
#include <sk/components/skatercorephysicscomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

#include <sk/modules/skate/skate.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterMatrixQueriesComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterMatrixQueriesComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterMatrixQueriesComponent::CSkaterMatrixQueriesComponent() : CBaseComponent()
{
	SetType( CRC_SKATERMATRIXQUERIES );
	
	mp_core_physics_component = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterMatrixQueriesComponent::~CSkaterMatrixQueriesComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterMatrixQueriesComponent::InitFromStructure( Script::CStruct* pParams )
{
	Dbg_MsgAssert(GetObject()->GetType() == SKATE_TYPE_SKATER, ("CSkaterMatrixQueriesComponent added to non-skater composite object"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterMatrixQueriesComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterMatrixQueriesComponent::Finalize (   )
{
	mp_core_physics_component = GetSkaterCorePhysicsComponentFromObject(GetObject());
		
	Dbg_Assert(mp_core_physics_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterMatrixQueriesComponent::Update()
{
	// Store the matrix from the last frame. Used by script functions for measuring angles & stuff. Has to use the last frame's matrix
	// because the landing physics snaps the orientation to the ground, yet the land script (which executes just after) needs to measure 
	// the angles on impact to maybe trigger bails and such.
	m_latest_matrix = mp_core_physics_component->m_lerping_display_matrix;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterMatrixQueriesComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | YawBetween | yaw between the two specified angles
        // @uparm (45, 135) | angle range values
		case CRCC(0xb992f3cc, "YawBetween"):
		{
			// if (CHEAT_SNOWBOARD) 
			// {
				// return false;
			// }
			
			Script::CPair Pair;
			if (!pParams->GetPair(NO_NAME, &Pair, Script::ASSERT))
			Dbg_MsgAssert(Pair.mX < Pair.mY,("\n%s\n1st angle must be less than the 2nd angle", pScript->GetScriptInfo()));
			
			Mth::Vector a = GetSkater()->m_vel;
			a.RotateToPlane(m_latest_matrix[Y]);
			return (Mth::AngleBetweenGreaterThan(a, m_latest_matrix[Z], Pair.mX)
				&& !Mth::AngleBetweenGreaterThan(a, m_latest_matrix[Z], Pair.mY))
				? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}

        // @script | YawingLeft | true if currently yawing left
		case CRCC(0xa745c080, "YawingLeft"):
		{
			Mth::Vector a = GetSkater()->m_vel;
			a.RotateToPlane(m_latest_matrix[Y]);
			return Mth::CrossProduct(a, m_latest_matrix[Z])[Y] > 0.0f ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
			
        // @script | YawingRight | true if currently yawing right
		case CRCC(0xc8c4d2f4, "YawingRight"):
		{
			Mth::Vector a = GetSkater()->m_vel;
			a.RotateToPlane(m_latest_matrix[Y]);
			return Mth::CrossProduct(a, m_latest_matrix[Z])[Y] < 0.0f ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
			
        // @script | PitchGreaterThan | true if the pitch is greater
        // than the specified value
        // @uparm 0.0 | test angle
		case CRCC(0xa0551543, "PitchGreaterThan"):
		{
			float TestAngle = 0.0f;
			pParams->GetFloat(NO_NAME, &TestAngle, Script::ASSERT);
			
			return Mth::DotProduct(m_latest_matrix[Y], mp_core_physics_component->m_last_display_normal) < cosf(Mth::DegToRad(TestAngle))
				? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}

		case CRCC(0x5e269b2b, "AbsolutePitchGreaterThan"):
		{
			float TestAngle = 0.0f;
			pParams->GetFloat(NO_NAME, &TestAngle, Script::ASSERT);
			
			return m_latest_matrix[Y][Y] < cosf(Mth::DegToRad(TestAngle)) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
			
		/*
        // @script | PitchingForward | true if pitching forward
		case CRCC(0xdaeda59c, "PitchingForward"):
		{
			Mth::Vector b = Mth::CrossProduct(m_latest_matrix[Y], mp_core_physics_component->m_last_display_normal);
			return Mth::DotProduct(b, m_latest_matrix[X]) < 0.0f ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}

        // @script | PitchingBackward | true if pitching backward
		case CRCC(0x7dd9e92c, "PitchingBackward"):
		{
			Mth::Vector b = Mth::CrossProduct(m_latest_matrix[Y], mp_core_physics_component->m_last_display_normal);
			return Mth::DotProduct(b, m_latest_matrix[X]) > 0.0f ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		*/

        // @script | RollGreaterThan | true if the roll is greater than
        // the specified angle value
        // @uparm 0.0 | angle value
		case CRCC(0xd3313e92, "RollGreaterThan"):
		{
			float TestAngle = 0.0f;
			pParams->GetFloat(NO_NAME, &TestAngle, Script::ASSERT);
			
			Mth::Vector a = m_latest_matrix[X];
			a.RotateToPlane(mp_core_physics_component->m_last_display_normal);
			return Mth::AngleBetweenGreaterThan(a, m_latest_matrix[X], TestAngle) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}

		/*
		// @script | RollingLeft | true if rolling left
		case CRCC(0x7328c9ad, "RollingLeft"):
		{
			Mth::Vector b = Mth::CrossProduct(m_latest_matrix[Y], mp_core_physics_component->m_last_display_normal);
			return Mth::DotProduct(b, m_latest_matrix[Z]) > 0.0f ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}

        // @script | RollingRight | true if rolling right
		case CRCC(0x8dcfe388, "RollingRight"):
		{
			Mth::Vector b = Mth::CrossProduct(m_latest_matrix[Y], mp_core_physics_component->m_last_display_normal);
			return Mth::DotProduct(b, m_latest_matrix[Z]) < 0.0f ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		*/
			
		// @script | GetSlope | Puts the angle of the slope into a param called Slope.
		// Units are degrees. Zero is horizontal, positive is up, negative is down.
		// The change in slope since the last call to GetSlope is put in a parameter
		// called ChangeInSlope
		case CRCC(0x97201739, "GetSlope"):
		{
			Mth::Vector v = GetObject()->m_matrix[Z];
			v[Y] = 0.0f;
			float slope = Mth::GetAngle(GetObject()->m_matrix[Z], v);
			if (GetObject()->m_matrix[Z][Y] < 0.0f)
			{
				slope = -slope;
			}	
			pScript->GetParams()->AddFloat(CRCD(0xa733ba7a, "Slope"), slope);
			pScript->GetParams()->AddFloat(CRCD(0x21afff16, "ChangeInSlope"), slope - m_last_slope);
			m_last_slope = slope;
			break;
		}
		
		case CRCC(0x8e7833be, "GetHeading"):
		{
			float heading = Mth::RadToDeg(cosf(GetObject()->m_matrix[Z][X]));
			if (GetObject()->m_matrix[Z][Z] < 0.0f)
			{
				heading = 360.0f - heading;
			}	
			pScript->GetParams()->AddFloat(CRCD(0xfd4bc03e, "heading"), heading);
			pScript->GetParams()->AddFloat(CRCD(0x2315ef17, "cosine"), GetObject()->m_matrix[Z][X]);
			pScript->GetParams()->AddFloat(CRCD(0x26910cc0, "sine"), GetObject()->m_matrix[Z][Z]);
			break;
		}
		

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterMatrixQueriesComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterMatrixQueriesComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

}
