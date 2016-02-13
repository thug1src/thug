//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SkeletonComponent.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/22/2002
//****************************************************************************

#include <gel/components/skeletoncomponent.h>

#include <gel/assman/assman.h>
#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <gfx/skeleton.h>

// TODO:  These won't be needed after the initial
// matrix data gets moved to the SKE file
#include <core/crc.h>
#include <core/math.h>
#include <gfx/nxanimcache.h>
#include <gfx/bonedanim.h>
#include <gfx/nxmiscfx.h>

// GJ:  CSkeletonComponent is supposed to be a wrapper
// around the Gfx::CSkeleton, which contains a skeletal
// object's final, post-blend matrix buffer.  It should
// be pretty dumb;  neither the CSkeletonComponent nor
// the Gfx::CSkeleton should contain any blending or 
// procedural anim data or functionality.  Furthermore, 
// it shouldn't know anything about animation flipping
// or skateboard rotation.   	    

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// This static function is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
CBaseComponent* CSkeletonComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkeletonComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkeletonComponent::init_skeleton( uint32 skeleton_name )
{
	Dbg_MsgAssert( mp_skeleton == NULL, ( "Component already has skeleton." ) );
    
	Gfx::CSkeletonData* pSkeletonData = (Gfx::CSkeletonData*)Ass::CAssMan::Instance()->GetAsset( skeleton_name, false );

	if ( !pSkeletonData )
	{
		Dbg_MsgAssert( 0, ("Unrecognized skeleton %s. (Is skeleton.q up to date?)", Script::FindChecksumName(skeleton_name)) );
	}
    
	mp_skeleton = new Gfx::CSkeleton( pSkeletonData );   
    Dbg_MsgAssert( mp_skeleton, ( "Couldn't create skeleton" ) );

    Dbg_MsgAssert( mp_skeleton->GetNumBones() > 0, ( "Skeleton needs at least one bone" ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkeletonComponent::CSkeletonComponent() : CBaseComponent()
{
	SetType( CRC_SKELETON );

    mp_skeleton = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkeletonComponent::~CSkeletonComponent()
{
    Dbg_MsgAssert( mp_skeleton, ( "No skeleton had been initialized" ) );
    
    if ( mp_skeleton )
    {
        delete mp_skeleton;
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkeletonComponent::InitFromStructure( Script::CStruct* pParams )
{
    uint32 skeletonName;

    if ( !pParams->GetChecksum( CRCD(0x222756d5,"skeleton"), &skeletonName, Script::NO_ASSERT ) )
	{
		pParams->GetChecksum( CRCD(0x9794932,"skeletonName"), &skeletonName, Script::ASSERT );
	}

    init_skeleton( skeletonName );

	int maxBoneSkipLOD;
	if(( pParams->GetInteger( CRCD(0xd3982061,"max_bone_skip_lod"), &maxBoneSkipLOD )) && mp_skeleton )
	{
		mp_skeleton->SetMaxBoneSkipLOD( maxBoneSkipLOD );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkeletonComponent::Update()
{
	// Doing nothing, so tell code to do nothing next time around
	Suspend(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Gfx::CSkeleton* CSkeletonComponent::GetSkeleton()
{
    return mp_skeleton;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkeletonComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch (Checksum)
    {
		// @script | Obj_SetBoneActive | changes whether the bone should be updated once per frame
		case 0x20f7b992: // Obj_SetBoneActive
		{
			Dbg_MsgAssert( mp_skeleton, ( "Obj_SetBoneActive on object without a skeleton" ) );
			
			uint32 boneName;
			pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, Script::ASSERT );
			
			int enabled;
			pParams->GetInteger( CRCD(0xb4e103fd,"active"), &enabled, Script::ASSERT );

			Dbg_Assert( mp_skeleton );
            mp_skeleton->SetBoneActive( boneName, enabled );

			break;
		}

		case 0xbe1c58ca:	// Obj_GetBonePosition
		{
			Mth::Vector bonePos(0.0f,0.0f,0.0f,1.0f);

			uint32 boneName;
			pParams->GetChecksum( CRCD(0xcab94088, "bone"), &boneName, Script::ASSERT );
			bool success = GetBoneWorldPosition( boneName, &bonePos );
			
			// make sure we have somewhere to return the data
			Dbg_Assert( pScript && pScript->GetParams() );
			pScript->GetParams()->AddFloat( CRCD(0x7323e97c,"x"), bonePos[X] );
			pScript->GetParams()->AddFloat( CRCD(0x424d9ea,"y"), bonePos[Y] );
			pScript->GetParams()->AddFloat( CRCD(0x9d2d8850,"z"), bonePos[Z] );
		
			return success ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		
		// @script | TextureSplat | create a texture splat
        // @parm float | size | 
        // @parmopt float | lifetime | 5.0 | lifetime in seconds
        // @parmopt float | radius | 0.0 | radius
        // @parmopt float | dropdown_length | 3.0 | dropdown length in feet
        // @parmopt float | forward | | distance forward of the origin that this spat goes, assuming no bone specififed		
        // @parmopt string | bone || bone name
        // @parmopt string | name || texture name
        // @parmopt string | trail || indicates that this splat should start or join a trail
		case CRCC(0xb244c9cf,"TextureSplat"):
			
		// @script | Skeleton_SpawnTextureSplat | create a texture splat
		// @parm float | size | 
		// @parmopt float | lifetime | 5.0 | lifetime in seconds
		// @parmopt float | radius | 0.0 | radius
		// @parmopt float | dropdown_length | 3.0 | dropdown length in feet
		// @parmopt float | forward | | distance forward of the origin that this spat goes, assuming no bone specififed		
		// @parmopt string | bone || bone name
		// @parmopt string | name || texture name
		// @parmopt string | trail || indicates that this splat should start or join a trail
		case CRCC(0xc51ce359, "Skeleton_SpawnTextureSplat"):
		{	
			float		size;
			float		radius				= 0.0f;
			float		lifetime			= 5.0f;
			float		dropdown_length		= 3.0f;
			float 		forward 			= 0.0f;
			bool		trail				= pParams->ContainsFlag( 0x4d977a70 /*"trail"*/ );
			bool		dropdown_vertical	= pParams->ContainsFlag( 0x7ee6c892 /*"dropdown_vertical"*/ );
			uint32		bone_name_checksum  = 0;
			const char*	p_texture_name;
			Mth::Vector	pos;

			pParams->GetFloat( CRCD(0x83fdb95,"size"), &size );
			pParams->GetFloat( CRCD(0xc48391a5,"radius"), &radius );
			pParams->GetFloat( CRCD(0xc218cf77,"lifetime"), &lifetime );
			pParams->GetFloat( CRCD(0x12c62572,"dropdown_length"), &dropdown_length );
			
			if( !( pParams->GetText( "name", &p_texture_name )))
			{
				Dbg_MsgAssert( 0, ("%s\nTextureSplat requires a texture name"));
			}

			if( !(pParams->GetChecksumOrStringChecksum( "bone", &bone_name_checksum )))
			{
				if (!pParams->GetFloat( CRCD(0x186ed079,"forward"), &forward ))
				{
					Dbg_MsgAssert( 0, ("%s\nTextureSplat requires a bone name"));
				}
				else
				{
					pos = GetObject()->GetPos();
					pos += forward * GetObject()->GetMatrix()[Z];
				}
			}

			SpawnTextureSplat( pos, bone_name_checksum, size, radius, lifetime, dropdown_length, p_texture_name, trail, dropdown_vertical );
            break;
		}
		
		// @script | Skeleton_SpawnCompositeObject | creates a rigidbody at the specified bone's position and orientation
		// finish autoducking
		case CRCC(0xf99f9faf, "Skeleton_SpawnCompositeObject"):
		{
			uint32 bone_name;
			pParams->GetChecksum(CRCD(0xcab94088, "bone"), &bone_name, Script::ASSERT);
			
			Mth::Vector offset;
			offset.Set();
			pParams->GetVector(CRCD(0xa6f5352f, "offset"), &offset);
			
			Script::CArray* p_component_array;
			pParams->GetArray(CRCD(0x11b70a02, "components"), &p_component_array, Script::ASSERT);
			
			Script::CStruct* p_params;
			pParams->GetStructure(CRCD(0x7031f10c, "params"), &p_params, Script::ASSERT);
			
			Mth::Vector relative_vel;
			pParams->GetVector(CRCD(0xc4c809e, "vel"), &relative_vel, Script::ASSERT);
			
			Mth::Vector relative_rotvel;
			pParams->GetVector(CRCD(0xfb1a83b2, "rotvel"), &relative_rotvel, Script::ASSERT);
			
			float object_vel_factor = 1.0f;
			pParams->GetFloat(CRCD(0x8c6a6e08, "object_vel_factor"), &object_vel_factor);
			
			SpawnCompositeObject(bone_name, p_component_array, p_params, object_vel_factor, relative_vel, relative_rotvel, offset);
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

bool CSkeletonComponent::GetBonePosition( uint32 boneName, Mth::Vector* pBonePos )
{
	Dbg_Assert(mp_skeleton);
	return mp_skeleton->GetBonePosition(boneName, pBonePos);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkeletonComponent::GetBoneWorldPosition( uint32 boneName, Mth::Vector* pBonePos )
{
	Dbg_Assert(mp_skeleton);
	
	if ( !mp_skeleton->GetBonePosition( boneName, pBonePos ) )
	{
		pBonePos->Set( 0.0f, 0.0f, 0.0f );
	}

	// now tranform it by the position of the object
    Mth::Matrix rootMatrix;
    rootMatrix = GetObject()->GetMatrix();
    rootMatrix[Mth::POS] = GetObject()->GetPos();
	rootMatrix[Mth::POS][W] = 1.0f;
	Mth::Vector vec = rootMatrix.Transform(* pBonePos );
	(*pBonePos) = vec;

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkeletonComponent::SpawnTextureSplat( Mth::Vector& pos, uint32 bone_name_checksum, float size, float radius, float lifetime, float dropdown_length, const char *p_texture_name, bool trail, bool dropdown_vertical )
{
	Mth::Vector bone_pos;
	if (!bone_name_checksum)
	{
		bone_pos = pos;
	}
	else
	{
		GetBoneWorldPosition( bone_name_checksum, &bone_pos );
	}

	// Convert dropdown_length from feet to inches.
	dropdown_length *= 12.0f;
	
	// Calculate the end position by dropping down 3 feet along the skater's up vector, or 'world up',
	// depending on the dropdown_vertical flag.
	Mth::Vector end;
	if( dropdown_vertical )
	{
		end = bone_pos - ( Mth::Vector( 0.0f, 1.0f, 0.0f ) * dropdown_length );
	}
	else
	{
		end = bone_pos - ( GetObject()->GetMatrix().GetUp() * dropdown_length );
	}
	
	// In some cases, the bone can be beneath geometry (trucks during a revert), so move the start position up slightly.
	bone_pos += ( GetObject()->GetMatrix().GetUp() * 12.0f );
	
	// Calculate radial offset for end position.
	if( radius > 0.0f )
	{
		float x_offset = -radius + (( radius * 2.0f ) * (float)rand() / RAND_MAX );
		float z_offset = -radius + (( radius * 2.0f ) * (float)rand() / RAND_MAX );

		end += ( GetObject()->GetMatrix().GetRight() * x_offset );
		end += ( GetObject()->GetMatrix().GetAt() * z_offset );
	}
	
	Nx::TextureSplat( bone_pos, end, size, lifetime, p_texture_name, trail ? bone_name_checksum : 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkeletonComponent::SpawnCompositeObject ( uint32 bone_name, Script::CArray* pArray, Script::CStruct* pParams,
	float object_vel_factor, Mth::Vector& relative_vel, Mth::Vector& relative_rotvel, Mth::Vector& offset )
{
	Dbg_Assert(pArray);
	Dbg_Assert(pParams);
	
	CSkeletonComponent* p_skeleton_component = GetSkeletonComponentFromObject(GetObject());
	Dbg_Assert(p_skeleton_component);

	// get the bone's matrix
	Mth::Matrix bone_matrix;
	p_skeleton_component->GetSkeleton()->GetBoneMatrix(bone_name, &bone_matrix);
	
	// build the object's matrix
	Mth::Matrix object_matrix = GetObject()->GetMatrix();
	object_matrix[W] = GetObject()->GetPos();
	object_matrix[X][W] = 0.0f;
	object_matrix[Y][W] = 0.0f;
	object_matrix[Z][W] = 0.0f;
	
	// NOTE: gludge to fix skater board model's coordinate system to match board bone's system
	Mth::Vector temp;
	temp = bone_matrix[Y];
	bone_matrix[Y] = bone_matrix[Z];
	bone_matrix[Z] = -temp;
	offset[W] = 0.0f;
	bone_matrix[W] += offset;
	
	// map the bone to world space
	bone_matrix *= object_matrix;
	
	// write the bone state into the composite object parameters
	
	pParams->AddVector(CRCD(0x7f261953, "pos"), bone_matrix[W]);
	
	Mth::Quat orientation(bone_matrix);
	if (orientation.GetScalar() > 0.0f)
	{
		pParams->AddVector(CRCD(0xc97f3aa9, "orientation"), orientation.GetVector());
	}
	else
	{
		pParams->AddVector(CRCD(0xc97f3aa9, "orientation"), -orientation.GetVector());
	}
	
	// setup rigidbody's initial velocity
	Mth::Vector vel = object_vel_factor * GetObject()->GetVel() + GetObject()->GetMatrix().Rotate(relative_vel);
	pParams->AddVector(CRCD(0xc4c809e, "vel"), vel);
	
	// setup rigidbody's initial rotational velocity
	Mth::Vector rotvel = GetObject()->GetMatrix().Rotate(relative_rotvel);
	pParams->AddVector(CRCD(0xfb1a83b2, "rotvel"), rotvel);
	
	CCompositeObjectManager::Instance()->CreateCompositeObjectFromNode(pArray, pParams);
}

}

