//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       skeleton.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  11/15/2001
//****************************************************************************

// TODO:
// Assert that the anim sequence contains the same number of bones as the skeleton?

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/skeleton.h>

#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <gfx/debuggfx.h>

#include <sys/file/filesys.h>
#include <core/crc.h>

#ifdef __PLAT_NGC__
#include "dolphin.h"
extern bool g_in_cutscene;
#endif
/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Gfx
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

#define nxBONEFLAGS_ROTATE			(1<<31)
#define nxBONEFLAGS_NOANIM			(1<<30)
#define nxBONEFLAGS_SCALELOCAL		(1<<29)
#define nxBONEFLAGS_SCALENONLOCAL	(1<<28)

#define MAX_LOD_DISTANCE			3.4e+38f;

class CBone
{
public:
	CBone();

public:
	uint32					m_name;
	Mth::Matrix*			mp_parentMatrix;
	Mth::Matrix*			mp_flippedMatrix;

// GJ:  The neutral pose has been moved to the skeleton data,
// since it can be shared by all the instances of this skeleton
//	Mth::Matrix				m_invertedNeutralMatrix;
//	Mth::Matrix*			mp_invertedNeutralParentMatrix;
	
	uint32					m_flags;
// GJ:  The parent name doesn't seem to be needed, and
// can be accessed from the skeleton data if necessary...
//	uint32					m_parentName;
	int						m_flipIndex;
	Mth::Vector				m_scale;
};

// GJ:  Removed reference to skateboard, because
// there shouldn't be anything skate-specific in here!
//#define nxSKELETONFLAGS_ROTATESKATEBOARD	(1<<30)

// This flag has been moved to CAnimationComponent,
// and will eventually be removed...
#define nxSKELETONFLAGS_FLIPPED				(1<<29)

// This flag was used as a kludge on THPS4 to get
// female models to scale properly (because supposedly-
// equivalent vertices in the female model were weighted
// to different bones in the male model)
#define nxSKELETONFLAGS_FEMALESKELETON		(1<<28)

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBone* CSkeleton::get_bone_by_id( uint32 boneId )
{
	for ( int i = 0; i < m_numBones; i++ )
	{
		if ( boneId == mp_bones[i].m_name )
			return &mp_bones[i];
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBone::CBone()
{
	m_name = 0;
	mp_parentMatrix = NULL;
// GJ:  Parent name has been moved to skeleton data
//	m_parentName = 0;
	mp_flippedMatrix = NULL;
// GJ:  Neutral pose has been moved to skeleton data
//	m_invertedNeutralMatrix.Ident();
//	mp_invertedNeutralParentMatrix = NULL;
	m_flipIndex = -1;
	m_flags = 0;
	m_scale = Mth::Vector( 1.0f, 1.0f, 1.0f );
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkeleton::CSkeleton( CSkeletonData* pSkeletonData )
{
    Dbg_Assert( pSkeletonData );

	// clear out flags completely
	m_flags = 0;
	
	if ( pSkeletonData->m_flags & 0x1 )
	{
		m_flags |= nxSKELETONFLAGS_FEMALESKELETON;
	}

#ifdef __PLAT_NGC__
	int size = sizeof( CBone ) * pSkeletonData->GetNumBones();
	int mem_available;
	bool need_to_pop = false;
	if ( g_in_cutscene )
	{
		Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().FrontEndHeap() );
		mem_available = Mem::Manager::sHandle().Available();
		if ( size < ( mem_available - ( 40 * 1024 ) ) )
		{
			need_to_pop = true;
		}
		else
		{
			Mem::Manager::sHandle().PopContext();
			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ThemeHeap() );
			mem_available = Mem::Manager::sHandle().Available();
			if ( size < ( mem_available - ( 5 * 1024 ) ) )
			{
				need_to_pop = true;
			}
			else
			{
				Mem::Manager::sHandle().PopContext();
				Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ScriptHeap() );
				mem_available = Mem::Manager::sHandle().Available();
				if ( size < ( mem_available - ( 40 * 1024 ) ) )
				{
					need_to_pop = true;
				}
				else
				{
					Mem::Manager::sHandle().PopContext();
				}
			}
		}
	}
#endif	// __PLAT_NGC__

	mp_bones = new CBone[pSkeletonData->GetNumBones()];

#ifdef __PLAT_NGC__
	if ( need_to_pop )
	{
		Mem::Manager::sHandle().PopContext();
	}
#endif	// __PLAT_NGC__

#ifdef __PLAT_NGC__
	size = sizeof( Mth::Matrix ) * pSkeletonData->GetNumBones();
	need_to_pop = false;
	if ( g_in_cutscene )
	{
		Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().FrontEndHeap() );
		mem_available = Mem::Manager::sHandle().Available();
		if ( size < ( mem_available - ( 40 * 1024 ) ) )
		{
			need_to_pop = true;
		}
		else
		{
			Mem::Manager::sHandle().PopContext();
			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ThemeHeap() );
			mem_available = Mem::Manager::sHandle().Available();
			if ( size < ( mem_available - ( 5 * 1024 ) ) )
			{
				need_to_pop = true;
			}
			else
			{
				Mem::Manager::sHandle().PopContext();
				Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ScriptHeap() );
				mem_available = Mem::Manager::sHandle().Available();
				if ( size < ( mem_available - ( 40 * 1024 ) ) )
				{
					need_to_pop = true;
				}
				else
				{
					Mem::Manager::sHandle().PopContext();
				}
			}
		}
	}
#endif	// __PLAT_NGC__

	mp_matrices = new Mth::Matrix[pSkeletonData->GetNumBones()];
	
#ifdef __PLAT_NGC__
	if ( need_to_pop )
	{
		Mem::Manager::sHandle().PopContext();
	}
#endif	// __PLAT_NGC__

	// clear out all the matrices to the identity
	Mth::Matrix* p_currentMatrix = mp_matrices;
	for ( int i = 0; i < pSkeletonData->GetNumBones(); i++ )
	{
		p_currentMatrix->Ident();
		p_currentMatrix++;
	}

	m_numBones = pSkeletonData->GetNumBones();
	
    initialize_bone_names( pSkeletonData );
    initialize_hierarchy( pSkeletonData );
    initialize_flip_matrices( pSkeletonData );

	mp_skeletonData		= pSkeletonData;

	// Set the maxmimum possible bone skip LOD level.
	SetMaxBoneSkipLOD( CSkeletonData::BONE_SKIP_LOD_BITS - 1 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkeleton::~CSkeleton()
{
	delete[] mp_bones;
	delete[] mp_matrices;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CSkeleton::GetBoneIndexById( uint32 boneId )
{
	for ( int i = 0; i < m_numBones; i++ )
	{
		if ( boneId == mp_bones[i].m_name )
			return i;
	}

	return -1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CSkeleton::GetFlipIndex( int i )
{
	CBone* pBone = &mp_bones[i];
	return pBone->m_flipIndex;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkeleton::initialize_flip_matrices( CSkeletonData* pSkeletonData )
{
    Dbg_Assert( pSkeletonData );
    Dbg_Assert( pSkeletonData->GetNumBones() == m_numBones );
	
	for ( int i = 0; i < pSkeletonData->GetNumBones(); i++ )
	{
        // if it's got a flip
        if ( pSkeletonData->GetFlipName(i) )
        {
            int index = pSkeletonData->GetIndex( pSkeletonData->GetFlipName(i) );
            mp_bones[i].mp_flippedMatrix = mp_matrices + index;
			mp_bones[i].m_flipIndex = index;

			// and vice versa...
			mp_bones[index].m_flipIndex = i;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkeleton::initialize_hierarchy( CSkeletonData* pSkeletonData )
{
	// the parent of the root is itself.
    
    Dbg_Assert( pSkeletonData );
    Dbg_Assert( pSkeletonData->GetNumBones() == m_numBones );

    for ( int i = 0; i < pSkeletonData->GetNumBones(); i++ )
    {
        int parentIndex;
         
        if ( pSkeletonData->GetParentName(i) == 0 )
        {
            parentIndex = 0;
        }
        else
        {
            parentIndex = pSkeletonData->GetParentIndex( i );
        }

		CBone* p_currentBone = &mp_bones[i];

//		Dbg_Message( "Parent of %d is %d\n", i, parentIndex );

// GJ:  Parent name has been moved to skeleton data
//		p_currentBone->m_parentName = pSkeletonData->GetParentName(i);

		// Assign parentage here...
		p_currentBone->mp_parentMatrix = mp_matrices + parentIndex;

// GJ:  Neutral pose has been moved to skeleton data
//		CBone* p_parentBone = &mp_bones[parentIndex];
//		p_currentBone->mp_invertedNeutralParentMatrix = &p_parentBone->m_invertedNeutralMatrix;

		p_currentBone->m_flags = 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkeleton::initialize_bone_names( CSkeletonData* pSkeletonData )
{
    Dbg_Assert( pSkeletonData );
    Dbg_Assert( pSkeletonData->GetNumBones() == m_numBones );

    for ( int i = 0; i < pSkeletonData->GetNumBones(); i++ )
    {
		mp_bones[i].m_name = pSkeletonData->GetBoneName( i );
   	}
}

#if 0
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Mth::Matrix CSkeleton::GetNeutralMatrix( int boneIndex )
{
	Dbg_MsgAssert( 0, ( "This function has been deprecated." ) );

	Dbg_Assert( boneIndex >= 0 && boneIndex < m_numBones );

	Mth::Matrix theReturnMatrix;

	theReturnMatrix = mp_bones[boneIndex].m_invertedNeutralMatrix;
	theReturnMatrix.InvertUniform();

	return theReturnMatrix;
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Mth::Matrix* CSkeleton::GetMatrices()
{
	return mp_matrices;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CSkeleton::GetNumBones() const
{
	return m_numBones;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkeleton::Display( Mth::Matrix* pRootMatrix, float r, float g, float b )
{
	Mth::Matrix* p_inverseNeutralPoseMatrices = mp_skeletonData->GetInverseNeutralPoseMatrices();

	for ( int i = 0; i < m_numBones; i++ )
	{
		CBone* p_currentBone = &mp_bones[i];
    
		Mth::Matrix tempMatrix0, tempMatrix1;
					
		Mth::Matrix boneMatrix;
		boneMatrix = *( mp_matrices + i );
		tempMatrix0 = *( p_inverseNeutralPoseMatrices + i );
		tempMatrix0.InvertUniform();
		tempMatrix0 = tempMatrix0 * boneMatrix;
		tempMatrix0 *= (*pRootMatrix);
		
		boneMatrix = *p_currentBone->mp_parentMatrix;
		if ( i == 0 )
		{
			// the root has no parent
			tempMatrix1.Ident();
		}
		else
		{
			tempMatrix1 = *( p_inverseNeutralPoseMatrices + mp_skeletonData->GetParentIndex(i) );
		}
		tempMatrix1.InvertUniform();
		tempMatrix1 = tempMatrix1 * boneMatrix; 
		tempMatrix1 *= (*pRootMatrix);
		
		AddDebugBone( tempMatrix0.GetPos(), tempMatrix1.GetPos(), r, g, b );
   }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkeleton::SetBoneActive( uint32 boneId, bool active )
{
	CBone* pBone = get_bone_by_id( boneId );

	if ( pBone )
	{
		pBone->m_flags &= ~nxBONEFLAGS_NOANIM;
		pBone->m_flags |= ( active ? 0 : nxBONEFLAGS_NOANIM );
	}

	return pBone;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkeleton::ApplyBoneScale( Script::CStruct* pBodyShapeStructure )
{
	// TODO:  CSkeleton shouldn't know anything about
	// Script::CStruct*...  this should be moved
	// out to a higher level...

	if ( !pBodyShapeStructure ) 
	{
		return false;
	}

	int scaleFromOrigin = 0;
	pBodyShapeStructure->GetInteger( "scaleFromOrigin", &scaleFromOrigin, Script::NO_ASSERT );
	bool localScale = !scaleFromOrigin;

	Mth::Vector theBoneScaleVector;
	
	for ( int i = 0; i < m_numBones; i++ )
	{
		if ( pBodyShapeStructure->GetVector(mp_bones[i].m_name,&theBoneScaleVector,Script::NO_ASSERT) )
		{
			uint32 name = mp_bones[i].m_name;

			if ( m_flags & nxSKELETONFLAGS_FEMALESKELETON )
			{
				if ( name == CRCD(0x580c0963, "head") )
				{
					name = CRCD(0xdfee29ac,"neck");
				}		   
			}

			// GJ PATCH:  test for Nolan
			Script::CArray* pNolanScalingTest = Script::GetArray( "nonlocal_bones", Script::NO_ASSERT );
			if ( pNolanScalingTest )
			{
				for ( uint32 i = 0; i < pNolanScalingTest->GetSize(); i++ )
				{
					if ( pNolanScalingTest->GetChecksum(i) == name )
					{
						localScale = false;
					}
				}
			}

			Mth::Vector vec = Mth::Vector( 1.0f, 1.0f, 1.0f, 1.0f );
			if ( GetBoneScale( name, &vec ) )
			{
				vec[X] *= theBoneScaleVector[X];
				vec[Y] *= theBoneScaleVector[Y];
				vec[Z] *= theBoneScaleVector[Z];
				vec[W] = 1.0f;

				bool success = SetBoneScale(name, vec, localScale);
				if ( !success )
				{
					Dbg_MsgAssert( 0, ( "Couldn't apply bone scale %s", Script::FindChecksumName(mp_bones[i].m_name) ) );
				}
			}
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkeleton::ResetScale( void )
{
	for ( int i = 0; i < m_numBones; i++ )
	{
		CBone* pBone = &mp_bones[i];
		pBone->m_flags &= ~(nxBONEFLAGS_SCALELOCAL | nxBONEFLAGS_SCALENONLOCAL);
		pBone->m_scale = Mth::Vector( 1.0f, 1.0f, 1.0f, 1.0f );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkeleton::GetBoneScale( uint32 boneId, Mth::Vector* pBoneScaleVector )
{
	CBone* pBone = get_bone_by_id( boneId );

	if ( pBone )
	{
		Dbg_Assert( pBoneScaleVector );
		*pBoneScaleVector = pBone->m_scale;
	}
	else
	{
		Dbg_Message( "Couldn't find bone to scale %s", Script::FindChecksumName(boneId) );
	}

	return pBone;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkeleton::SetBoneScale( uint32 boneId, const Mth::Vector& theBoneScale, bool isLocalScale )
{
	CBone* pBone = get_bone_by_id( boneId );

	if ( pBone )
	{
		pBone->m_scale = theBoneScale;

		bool isIdentityScale = ( theBoneScale[X] == 1.0f && theBoneScale[Y] == 1.0f && theBoneScale[Z] == 1.0f );
		pBone->m_flags &= ~(nxBONEFLAGS_SCALELOCAL | nxBONEFLAGS_SCALENONLOCAL);
		if ( !isIdentityScale )
		{
			pBone->m_flags |= ( isLocalScale ? nxBONEFLAGS_SCALELOCAL : nxBONEFLAGS_SCALENONLOCAL );
		}
	}
	else
	{
		Dbg_Message( "Couldn't find bone to scale %s", Script::FindChecksumName(boneId) );
	}

	return pBone;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkeleton::CopyBoneScale( Gfx::CSkeleton* pSourceSkeleton )
{
	CSkeletonData* pSkeletonData = pSourceSkeleton->GetSkeletonData();
	for ( int i = 0; i < pSkeletonData->GetNumBones(); i++ )
	{
		uint32 boneName = pSkeletonData->GetBoneName( i );

		Mth::Vector sourceScale;

		// if the bone exists in the destination skeleton,
		// and the source skeleton, then copy the scale
		if ( get_bone_by_id( boneName )
			 && pSourceSkeleton->GetBoneScale( boneName, &sourceScale ) )
		{
			bool isLocalScale = true;

			#if 0
			// GJ:  This code handles the shoulder bones differently
			// so that the female skaters aren't so broad-shouldered.
			// However, it ends up screwing stuff up during cutscenes,
			// because the female's hands will not be aligned to the
			// items that she might be holding.  The only way I can
			// think of around this (right now) is to create female-
			// specific versions of the cutscenes, but it's too late 
			// for that now.
			Script::CArray* pShoulderScalingArray = Script::GetArray( CRCD(0x20d9ac2f,"nonlocal_bones"), Script::NO_ASSERT );
			if ( pShoulderScalingArray )
			{
				for ( uint32 i = 0; i < pShoulderScalingArray->GetSize(); i++ )
				{
					if ( pShoulderScalingArray->GetChecksum(i) == boneName )
					{
						isLocalScale = false;
					}
				}
			}
			#endif

			SetBoneScale( boneName, sourceScale, isLocalScale );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void sQuatVecToMatrix( Mth::Quat* pQ, Mth::Vector* pT, Mth::Matrix* pMatrix, bool rotate, bool flip )
{
	Dbg_Assert( pQ );
   	Dbg_Assert( pT );
	Dbg_Assert( pMatrix );
	
#ifdef __PLAT_NGPS__
	Dbg_MsgAssert( ( (uint32)&(pQ->GetVector()) - (uint32)pQ )==0, ("Quat::quat is no longer offset 0 bytes from the start of the Quat, need to modify the VU code in sQuatVecToMatrix"));
	
	asm __volatile__("
	
	lw	$8,(%3)	# Put 'rotate' flag in t0
	lw	$9,(%4)	# Put 'flip' flag in t1
	
	lqc2    vf4,0x00(%0)			# Load pQ into vf4
	lqc2    vf7,0x00(%1)			# Load pT into vf7
	
	#	square[X] = (*pQ)[X] * (*pQ)[X];
	#	square[Y] = (*pQ)[Y] * (*pQ)[Y];
	#	square[Z] = (*pQ)[Z] * (*pQ)[Z];
	vmul.xyz vf5,vf4,vf4			# vf5=square
									
	#	cross[X] = (*pQ)[Y] * (*pQ)[Z];
	#	cross[Y] = (*pQ)[Z] * (*pQ)[X];
	#	cross[Z] = (*pQ)[X] * (*pQ)[Y];
	vopmula.xyz     ACC,vf4,vf4		# ACC=cross

	#	wimag[X] = -(*pQ)[W] * (*pQ)[X];
	#	wimag[Y] = -(*pQ)[W] * (*pQ)[Y];
	#	wimag[Z] = -(*pQ)[W] * (*pQ)[Z];
	vmsubw.xyzw vf8,vf4,vf4w	# vf8=cross+wimag
	vmaddw.xyzw vf9,vf4,vf4w	# vf9=cross-wimag

	
	vmulax.w	ACC,vf0,vf5x	# ACCw=pQx squared
	vmadday.w	ACC,vf0,vf5y	# ACCw=pQx squared + pQy squared
	vmaddz.w	vf1,vf0,vf5z	# vf1w=pQx squared + pQy squared + pQz squared
	
	# 3 vnops here ... use 2 of them with the following.
	vadd.xyz	vf8,vf8,vf8		# vf8=2*(cross+wimag)
	vadd.xyz	vf9,vf9,vf9		# vf9=2*(cross-wimag)
	
	vsubw.xyz	vf1,vf5,vf1w	# vf1=(-y2-z2,-x2-z2,-x2-y2)
	# 3 vnops here that could be used for something
	vaddaw.xyz	ACC,vf1,vf0w	# ACC=(1-y2-z2,1-x2-z2,1-x2-y2)
	vmaddw.xyz	vf1,vf1,vf0w	# vf1=(1-2(y2+z2),1-2(x2+z2),1-2(x2+y2))


	beqz	$8,norotate		# if (rotate)
	nop

	beqz	$9,noflip		# if (flip)
	nop

	# (*pMatrix)[Mth::RIGHT][X] = -(1 - 2 * (square[Y] + square[Z]));
	# (*pMatrix)[Mth::RIGHT][Y] = 2 * (cross[Z] + wimag[Z]);
	# (*pMatrix)[Mth::RIGHT][Z] = 2 * (cross[Y] - wimag[Y]);
	# (*pMatrix)[Mth::RIGHT][W] = 0.0f;
	vsubax.x	ACC,vf0,vf1x
	vaddaz.y	ACC,vf0,vf8z
	vadday.z 	ACC,vf0,vf9y
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::RIGHT]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x00(%2)

	# (*pMatrix)[Mth::UP][X] = 2 * (cross[Z] - wimag[Z]);
	# (*pMatrix)[Mth::UP][Y] = -(1 - 2 * (square[X] + square[Z]));
	# (*pMatrix)[Mth::UP][Z] = -(2 * (cross[X] + wimag[X]));
	# (*pMatrix)[Mth::UP][W] = 0.0f;
	vaddaz.x	ACC,vf0,vf9z
	vsubay.y	ACC,vf0,vf1y
	vsubax.z	ACC,vf0,vf8x
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::UP]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x10(%2)

	# (*pMatrix)[Mth::AT][X] = -(2 * (cross[Y] + wimag[Y]));
	# (*pMatrix)[Mth::AT][Y] = 2 * (cross[X] - wimag[X]);
	# (*pMatrix)[Mth::AT][Z] = 1 - 2 * (square[X] + square[Y]);
	# (*pMatrix)[Mth::AT][W] = 0.0f;
	vsubay.x	ACC,vf0,vf8y
	vaddax.y	ACC,vf0,vf9x
	vaddaz.z	ACC,vf0,vf1z
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::AT]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x20(%2)

	# (*pMatrix)[Mth::POS][X] = -pT->GetX();
	# (*pMatrix)[Mth::POS][Y] = pT->GetY();
	# (*pMatrix)[Mth::POS][Z] = pT->GetZ();
	# (*pMatrix)[Mth::POS][W] = 1.0f;
	vsubax.x	ACC,vf0,vf7x
	vadday.y	ACC,vf0,vf7y
	vaddaz.z	ACC,vf0,vf7z
	vaddax.w	ACC,vf0,vf0x
	# Save out (*pMatrix)[Mth::POS]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x30(%2)
	
	j	overnorotate		
	nop
noflip:							# else  (if (flip))

	# (*pMatrix)[Mth::RIGHT][X] = -(1 - 2 * (square[Y] + square[Z]));
	# (*pMatrix)[Mth::RIGHT][Y] = -(2 * (cross[Z] + wimag[Z]));
	# (*pMatrix)[Mth::RIGHT][Z] = -(2 * (cross[Y] - wimag[Y]));
	# (*pMatrix)[Mth::RIGHT][W] = 0.0f;
	vsubax.x	ACC,vf0,vf1x
	vsubaz.y	ACC,vf0,vf8z
	vsubay.z 	ACC,vf0,vf9y
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::RIGHT]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x00(%2)

	# (*pMatrix)[Mth::UP][X] = -(2 * (cross[Z] - wimag[Z]));
	# (*pMatrix)[Mth::UP][Y] = -(1 - 2 * (square[X] + square[Z]));
	# (*pMatrix)[Mth::UP][Z] = -(2 * (cross[X] + wimag[X]));
	# (*pMatrix)[Mth::UP][W] = 0.0f;
	vsubaz.x	ACC,vf0,vf9z
	vsubay.y	ACC,vf0,vf1y
	vsubax.z	ACC,vf0,vf8x
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::UP]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x10(%2)

	# (*pMatrix)[Mth::AT][X] = 2 * (cross[Y] + wimag[Y]);
	# (*pMatrix)[Mth::AT][Y] = 2 * (cross[X] - wimag[X]);
	# (*pMatrix)[Mth::AT][Z] = 1 - 2 * (square[X] + square[Y]);
	# (*pMatrix)[Mth::AT][W] = 0.0f;
	vadday.x	ACC,vf0,vf8y
	vaddax.y	ACC,vf0,vf9x
	vaddaz.z	ACC,vf0,vf1z
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::AT]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x20(%2)

	# (*pMatrix)[Mth::POS][X] = pT->GetX();
	# (*pMatrix)[Mth::POS][Y] = pT->GetY();
	# (*pMatrix)[Mth::POS][Z] = pT->GetZ();
	# (*pMatrix)[Mth::POS][W] = 1.0f;
	vaddax.x	ACC,vf0,vf7x
	vadday.y	ACC,vf0,vf7y
	vaddaz.z	ACC,vf0,vf7z
	vaddax.w	ACC,vf0,vf0x
	# Save out (*pMatrix)[Mth::POS]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x30(%2)

	j	overnorotate		
	nop						# else (if (rotate))
norotate:

	beqz	$9,noflip2		# if (flip)
	nop

	# (*pMatrix)[Mth::RIGHT][X] = 1 - 2 * (square[Y] + square[Z]);
	# (*pMatrix)[Mth::RIGHT][Y] = -(2 * (cross[Z] + wimag[Z]));
	# (*pMatrix)[Mth::RIGHT][Z] = -(2 * (cross[Y] - wimag[Y]));
	# (*pMatrix)[Mth::RIGHT][W] = 0.0f;
	vaddax.x	ACC,vf0,vf1x
	vsubaz.y	ACC,vf0,vf8z
	vsubay.z 	ACC,vf0,vf9y
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::RIGHT]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x00(%2)

	# (*pMatrix)[Mth::UP][X] = -(2 * (cross[Z] - wimag[Z]));
	# (*pMatrix)[Mth::UP][Y] = 1 - 2 * (square[X] + square[Z]);
	# (*pMatrix)[Mth::UP][Z] = 2 * (cross[X] + wimag[X]);
	# (*pMatrix)[Mth::UP][W] = 0.0f;
	vsubaz.x	ACC,vf0,vf9z
	vadday.y	ACC,vf0,vf1y
	vaddax.z	ACC,vf0,vf8x
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::UP]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x10(%2)

	# (*pMatrix)[Mth::AT][X] = -(2 * (cross[Y] + wimag[Y]));
	# (*pMatrix)[Mth::AT][Y] = 2 * (cross[X] - wimag[X]);
	# (*pMatrix)[Mth::AT][Z] = 1 - 2 * (square[X] + square[Y]);
	# (*pMatrix)[Mth::AT][W] = 0.0f;
	vsubay.x	ACC,vf0,vf8y
	vaddax.y	ACC,vf0,vf9x
	vaddaz.z	ACC,vf0,vf1z
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::AT]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x20(%2)

	# (*pMatrix)[Mth::POS][X] = -pT->GetX();
	# (*pMatrix)[Mth::POS][Y] = pT->GetY();
	# (*pMatrix)[Mth::POS][Z] = pT->GetZ();
	# (*pMatrix)[Mth::POS][W] = 1.0f;
	vsubax.x	ACC,vf0,vf7x
	vadday.y	ACC,vf0,vf7y
	vaddaz.z	ACC,vf0,vf7z
	vaddax.w	ACC,vf0,vf0x
	# Save out (*pMatrix)[Mth::POS]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x30(%2)

	j	overnoflip2			# else  (if (flip))
	nop
noflip2:

	# (*pMatrix)[Mth::RIGHT][X] = 1 - 2 * (square[Y] + square[Z]);
	# (*pMatrix)[Mth::RIGHT][Y] = 2 * (cross[Z] + wimag[Z]);
	# (*pMatrix)[Mth::RIGHT][Z] = 2 * (cross[Y] - wimag[Y]);
	# (*pMatrix)[Mth::RIGHT][W] = 0.0f;
	vaddax.x	ACC,vf0,vf1x
	vaddaz.y	ACC,vf0,vf8z
	vadday.z 	ACC,vf0,vf9y
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::RIGHT]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x00(%2)

	# (*pMatrix)[Mth::UP][X] = 2 * (cross[Z] - wimag[Z]);
	# (*pMatrix)[Mth::UP][Y] = 1 - 2 * (square[X] + square[Z]);
	# (*pMatrix)[Mth::UP][Z] = 2 * (cross[X] + wimag[X]);
	# (*pMatrix)[Mth::UP][W] = 0.0f;
	vaddaz.x	ACC,vf0,vf9z
	vadday.y	ACC,vf0,vf1y
	vaddax.z	ACC,vf0,vf8x
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::UP]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x10(%2)

	# (*pMatrix)[Mth::AT][X] = 2 * (cross[Y] + wimag[Y]);
	# (*pMatrix)[Mth::AT][Y] = 2 * (cross[X] - wimag[X]);
	# (*pMatrix)[Mth::AT][Z] = 1 - 2 * (square[X] + square[Y]);
	# (*pMatrix)[Mth::AT][W] = 0.0f;
	vadday.x	ACC,vf0,vf8y
	vaddax.y	ACC,vf0,vf9x
	vaddaz.z	ACC,vf0,vf1z
	vsubaw.w	ACC,vf0,vf0w
	# Save out (*pMatrix)[Mth::AT]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x20(%2)

	# (*pMatrix)[Mth::POS][X] = pT->GetX();
	# (*pMatrix)[Mth::POS][Y] = pT->GetY();
	# (*pMatrix)[Mth::POS][Z] = pT->GetZ();
	# (*pMatrix)[Mth::POS][W] = 1.0f;
	vaddax.x	ACC,vf0,vf7x
	vadday.y	ACC,vf0,vf7y
	vaddaz.z	ACC,vf0,vf7z
	vaddax.w	ACC,vf0,vf0x
	# Save out (*pMatrix)[Mth::POS]
	vmaddx.xyzw	vf11,vf0,vf0x
	sqc2		vf11,0x30(%2)

overnoflip2:
	
overnorotate:

	": : "r" (pQ), "r" (pT), "r" (pMatrix), "r" (&rotate), "r" (&flip) : "$8","$9");

#else
#ifdef __PLAT_NGC__
	MTXQuat( (Mtx)pMatrix, (QuaternionPtr)pQ );

	(*pMatrix)[Mth::POS][X] = pT->GetX();
	(*pMatrix)[Mth::POS][Y] = pT->GetY();
	(*pMatrix)[Mth::POS][Z] = pT->GetZ();
	(*pMatrix)[Mth::POS][W] = 1.0f;
#else
	// Ye olde slowe code
	Mth::Vector		square;
	Mth::Vector		cross;
	Mth::Vector		wimag;

	square[X] = (*pQ)[X] * (*pQ)[X];
	square[Y] = (*pQ)[Y] * (*pQ)[Y];
	square[Z] = (*pQ)[Z] * (*pQ)[Z];

	cross[X] = (*pQ)[Y] * (*pQ)[Z];
	cross[Y] = (*pQ)[Z] * (*pQ)[X];
	cross[Z] = (*pQ)[X] * (*pQ)[Y];

	wimag[X] = -(*pQ)[W] * (*pQ)[X];
	wimag[Y] = -(*pQ)[W] * (*pQ)[Y];
	wimag[Z] = -(*pQ)[W] * (*pQ)[Z];

	(*pMatrix)[Mth::RIGHT][X] = 1 - 2 * (square[Y] + square[Z]);
	(*pMatrix)[Mth::RIGHT][Y] = 2 * (cross[Z] + wimag[Z]);
	(*pMatrix)[Mth::RIGHT][Z] = 2 * (cross[Y] - wimag[Y]);
	(*pMatrix)[Mth::RIGHT][W] = 0.0f;
	
	(*pMatrix)[Mth::UP][X] = 2 * (cross[Z] - wimag[Z]);
	(*pMatrix)[Mth::UP][Y] = 1 - 2 * (square[X] + square[Z]);
	(*pMatrix)[Mth::UP][Z] = 2 * (cross[X] + wimag[X]);
	(*pMatrix)[Mth::UP][W] = 0.0f;

	(*pMatrix)[Mth::AT][X] = 2 * (cross[Y] + wimag[Y]);
	(*pMatrix)[Mth::AT][Y] = 2 * (cross[X] - wimag[X]);
	(*pMatrix)[Mth::AT][Z] = 1 - 2 * (square[X] + square[Y]);
	(*pMatrix)[Mth::AT][W] = 0.0f;

	(*pMatrix)[Mth::POS][X] = pT->GetX();
	(*pMatrix)[Mth::POS][Y] = pT->GetY();
	(*pMatrix)[Mth::POS][Z] = pT->GetZ();
	(*pMatrix)[Mth::POS][W] = 1.0f;
#endif		// __PLAT_NGC__

	if (rotate)
	{
		// compensate for bone rotation (like for the skateboards)

		if (flip)
		{
			// Right: -x,y,z
			(*pMatrix)[Mth::RIGHT][X] = -(*pMatrix)[Mth::RIGHT][X];
			// Up: x,-y,-z
			(*pMatrix)[Mth::UP][Y]=-(*pMatrix)[Mth::UP][Y];
			(*pMatrix)[Mth::UP][Z]=-(*pMatrix)[Mth::UP][Z];
			// At: -x,y,z
			(*pMatrix)[Mth::AT][X]=-(*pMatrix)[Mth::AT][X];
			// Pos: -x,y,z
			(*pMatrix)[Mth::POS][X]=-(*pMatrix)[Mth::POS][X];
		}
		else
		{
			// Right: -x,-y,-z
			(*pMatrix)[Mth::RIGHT][X] = -(*pMatrix)[Mth::RIGHT][X];
			(*pMatrix)[Mth::RIGHT][Y] = -(*pMatrix)[Mth::RIGHT][Y];
			(*pMatrix)[Mth::RIGHT][Z] = -(*pMatrix)[Mth::RIGHT][Z];
			// Up: -x,-y,-z
			(*pMatrix)[Mth::UP][X]=-(*pMatrix)[Mth::UP][X];
			(*pMatrix)[Mth::UP][Y]=-(*pMatrix)[Mth::UP][Y];
			(*pMatrix)[Mth::UP][Z]=-(*pMatrix)[Mth::UP][Z];
			// At: x,y,z
			// Pos: x,y,z
		}	
	}
	else
	{
		if (flip)
		{
			// Right: x,-y,-z
			(*pMatrix)[Mth::RIGHT][Y] = -(*pMatrix)[Mth::RIGHT][Y];
			(*pMatrix)[Mth::RIGHT][Z] = -(*pMatrix)[Mth::RIGHT][Z];
			// Up: -x,y,z
			(*pMatrix)[Mth::UP][X]=-(*pMatrix)[Mth::UP][X];
			// At: -x,y,z
			(*pMatrix)[Mth::AT][X]=-(*pMatrix)[Mth::AT][X];
			// Pos: -x,y,z
			(*pMatrix)[Mth::POS][X]=-(*pMatrix)[Mth::POS][X];
		}
		else
		{
			// Right: x,y,z
			// Up: x,y,z
			// At: x,y,z
			// Pos: x,y,z
		}	
	}	
#endif	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkeleton::Update( CPose* pPose )
{
	this->Update( &pPose->m_rotations[0], &pPose->m_translations[0] );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkeleton::Update( Mth::Quat* pQuat, Mth::Vector* pTrans )
{
	CBone* p_currentBone = mp_bones;
	Mth::Matrix* p_currentMatrix=mp_matrices;
	Mth::Quat* p_current_quat=pQuat;
	Mth::Vector* p_current_trans=pTrans;

#ifdef __NOPT_ASSERT__
	bool flipped = m_flags & nxSKELETONFLAGS_FLIPPED;
	if ( flipped )
	{
		Dbg_MsgAssert( 0, ( "Flipping has been moved to CAnimationComponent" ) );
	}
#endif

	// Calculate the mask for testing the bone skip data.
	uint32 skip_index_mask	= 1 << GetBoneSkipIndex();
	uint32*	p_skip_list		= mp_skeletonData->GetBoneSkipList();

	for ( int i = 0; i < m_numBones; i++ )
	{
		if( !( p_currentBone->m_flags & nxBONEFLAGS_NOANIM ))
		{
			// Decide whether we can skip this bone. Skipping bones where a child bone is not skipped will cause
			// problems.
			bool skip_this_bone = (( p_skip_list[i] & skip_index_mask ) != 0 );
			if( !skip_this_bone )
			{
				// Skateboard rotation has been moved to CAnimationComponent.
				Dbg_MsgAssert( !( p_currentBone->m_flags & nxBONEFLAGS_ROTATE ), ( "Skateboard rotation has been moved to CAnimationComponent" ) );

				sQuatVecToMatrix( p_current_quat, p_current_trans, p_currentMatrix, false, false );
		
				if ( p_currentBone->m_flags & nxBONEFLAGS_SCALENONLOCAL )
				{
					p_currentMatrix->Scale( p_currentBone->m_scale );
				}

				if ( i != 0 )
				{
					// if it's not the root, then apply the parent's xform
					(*p_currentMatrix) *= *(p_currentBone->mp_parentMatrix);
				}
			}
			else
			{
				// Use the parent bone's matrix for now.
				(*p_currentMatrix) = *(p_currentBone->mp_parentMatrix);
			}
		}
		
		p_currentMatrix++;
		p_currentBone++;
		p_current_quat++;
		p_current_trans++;
	}

	Mth::Matrix* p_currentInverseNeutralPoseMatrix = mp_skeletonData->GetInverseNeutralPoseMatrices();
	Dbg_MsgAssert( p_currentInverseNeutralPoseMatrix, ( "Was expecting neutral pose matrices to be initialized.  SKE files not in version 2 format?" ) );

	// Loop through each bone and update its matrices
	p_currentBone=mp_bones;
	p_currentMatrix=mp_matrices;
	for ( int i = 0; i < m_numBones; i++ )
	{
		if ( !(p_currentBone->m_flags & nxBONEFLAGS_NOANIM) )
		{
			if ( p_currentBone->m_flags & nxBONEFLAGS_SCALELOCAL )
			{
				p_currentMatrix->ScaleLocal( p_currentBone->m_scale );
			}

			// switching over to the new system of having the neutral pose
//			Dbg_MsgAssert( p_currentInverseNeutralPoseMatrix, ( "default anims have been deprecated" ) );
//			(*p_currentMatrix) = p_currentBone->m_invertedNeutralMatrix * (*p_currentMatrix);		

			// data inside the SKE file, rather than the skeleton instance
			(*p_currentMatrix) = (*p_currentInverseNeutralPoseMatrix) * (*p_currentMatrix);		
				
			// clear out final component, just in case...
			// it's probably be worth investigating whether we need to do this...
			(*p_currentMatrix)[Mth::RIGHT][W] = 0.0f;
			(*p_currentMatrix)[Mth::UP][W] = 0.0f;
			(*p_currentMatrix)[Mth::AT][W] = 0.0f;
//			(*p_currentMatrix)[Mth::POS][W] = 1.0f;
		}

		p_currentInverseNeutralPoseMatrix++;
		p_currentMatrix++;
		p_currentBone++;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkeleton::GetBoneMatrix( uint32 boneId, Mth::Matrix* pBoneMatrix )
{
	Dbg_Assert( pBoneMatrix );

	CBone* pBone = get_bone_by_id( boneId );
	int i = GetBoneIndexById( boneId );

	if ( !pBone )
	{
		// for now, since we're changing our bone names around,
		// don't assert if we can't find the bone...
		return false;
	}

	Dbg_MsgAssert( pBone, ( "Couldn't find bone with id %s", Script::FindChecksumName(boneId) ) );

	// m_matrix represents the transform needed to go from the default (neutral)
	// pose to the actual pose with animation applied to it.  So, we need to
	// transform this value by the root-to-neutral matrix (m_neutralMatrix)
	// in order to get the actual animated-bone's position for this frame (relative
	// to the root of the object).

	Mth::Matrix boneMatrix;
	boneMatrix = *(mp_matrices + i);
	
	// GJ 2/21/03:  i'm not sure why we needed to deal with the
	// neutral matrix at all...   should look into this at some
	// point...
	Mth::Matrix theMat = *( mp_skeletonData->GetInverseNeutralPoseMatrices() + i );
//	Mth::Matrix theMat = pBone->m_invertedNeutralMatrix;
	theMat.InvertUniform();
	theMat = theMat * boneMatrix;
    
	// GJ kludge:  the root bone contains a 90 degree rotation
	// to compensate for the MAX->Game coordinate change
	// but we don't want that to affect the bone matrix, 
	// so transform it by the inverse of the root matrix
//	CBone* pDummyBone = get_bone_by_id( CRCD(0x9f173eeb,"dummy_scale_zz") );
//	if ( pDummyBone )
	
	int dummyIndex = GetBoneIndexById( CRCD(0x9f173eeb,"dummy_scale_zz") );
	if ( dummyIndex != -1 )
	{
		Mth::Vector temp = theMat[Mth::POS];
		theMat[Mth::POS] = Mth::Vector(0.0f,0.0f,0.0f,1.0f);
		Mth::Matrix dummyMatrix = *( mp_skeletonData->GetInverseNeutralPoseMatrices() + dummyIndex );
//		Mth::Matrix dummyMatrix = pDummyBone->m_invertedNeutralMatrix;
		theMat = dummyMatrix * theMat;
		theMat[Mth::POS] = temp;
	}

	(*pBoneMatrix) = theMat;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32* CSkeleton::GetBoneSkipList( void )
{
	if( mp_skeletonData )
	{
		return mp_skeletonData->GetBoneSkipList();
	}
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkeleton::SetBoneSkipDistance( float dist )
{
	if( mp_skeletonData )
	{
		for( uint32 lod = 0; lod <= m_maxBoneSkipLOD; ++lod )
		{
			if( dist < mp_skeletonData->GetBoneSkipDistance( lod ))
			{
				m_skipIndex = lod;
				return;
			}
		}
	}
	m_skipIndex = m_maxBoneSkipLOD;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkeleton::GetBonePosition( uint32 boneId, Mth::Vector* pBonePos )
{
	Dbg_Assert( pBonePos );

	CBone* pBone = get_bone_by_id( boneId );
	int i = GetBoneIndexById( boneId );

	if ( !pBone )
	{
		// for now, since we're changing our bone names around,
		// don't assert if we can't find the bone...
		return false;
	}

	Dbg_MsgAssert( pBone, ( "Couldn't find bone with id %s", Script::FindChecksumName(boneId) ) );

	// m_matrix represents the transform needed to go from the default (neutral)
	// pose to the actual pose with animation applied to it.  So, we need to
	// transform this value by the root-to-neutral matrix (m_neutralMatrix)
	// in order to get the actual animated-bone's position for this frame (relative
	// to the root of the object).

	// GJ 2/21/03:  i'm not sure why we needed to deal with the
	// neutral matrix at all...   should look into this at some
	// point...

	Mth::Matrix boneMatrix;
	boneMatrix = *(mp_matrices + i);
	Mth::Matrix theMat = *( mp_skeletonData->GetInverseNeutralPoseMatrices() + i );
//	Mth::Matrix theMat = pBone->m_invertedNeutralMatrix;
	theMat.InvertUniform();
	theMat = theMat * boneMatrix;
    
    (*pBonePos) = theMat[Mth::POS];

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkeletonData::CSkeletonData()
{
    m_numBones = 0;
	m_flags = 0;
    m_animScriptName = 0;
	mp_inverseNeutralPoseMatrices = NULL;

	// Default the Level0 LOD distance to a large number.
	m_skipLODDistances[0] = MAX_LOD_DISTANCE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkeletonData::~CSkeletonData()
{
	if ( mp_inverseNeutralPoseMatrices )
	{
		delete[] mp_inverseNeutralPoseMatrices;
		mp_inverseNeutralPoseMatrices = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkeletonData::Load( uint32* p_data32, int data_size, bool assertOnFail )
{
	uint8* p_data = (uint8*)p_data32;

	// new load of platform-specific SKE files...
	bool success = false;

#ifdef __NOPT_ASSERT__
	int versionNumber = *((int*)p_data);
	Dbg_MsgAssert( versionNumber >= 2, ( "No longer supporting old SKE format" ) );
#endif
	p_data += sizeof(int);
		
	m_flags = *((uint32*)p_data);
	p_data += sizeof(uint32);

	m_numBones = *((int*)p_data);
	p_data += sizeof(int);
    
    Dbg_MsgAssert( m_numBones < vMAX_BONES, ( "Too many bones in skeleton (raise vMAX_BONES from %d to %d)", vMAX_BONES, m_numBones ) );

	memcpy( m_boneNameTable, p_data, m_numBones * sizeof(uint32) );
	p_data += ( m_numBones * sizeof(uint32) );

	memcpy( m_parentNameTable, p_data, m_numBones * sizeof(uint32) );
	p_data += ( m_numBones * sizeof(uint32) );

	memcpy( m_flipNameTable, p_data, m_numBones * sizeof(uint32) );
	p_data += ( m_numBones * sizeof(uint32) );
	
	Dbg_MsgAssert( mp_inverseNeutralPoseMatrices == NULL, ( "Inverse neutral pose matrices not NULL?!?" ) );
	mp_inverseNeutralPoseMatrices = new Mth::Matrix[m_numBones];

	for ( int i = 0; i < m_numBones; i++ )
	{
// Mick: read a word at a time, as it's not aligned and I'm trying to ensure
// that all vector operations work on aligned boundries
// so we can optimize with that assumption	
//		Mth::Quat theQuat = *((Mth::Quat*)p_data);
		Mth::Quat theQuat;
		theQuat[X] = *((float*)p_data);
		theQuat[Y] = *((float*)(p_data+4));
		theQuat[Z] = *((float*)(p_data+8));
		theQuat[W] = *((float*)(p_data+12));
		p_data += sizeof(Mth::Quat);
		
//		Mth::Vector theVector = *((Mth::Vector*)p_data);
		Mth::Vector theVector;
		theVector[X] = *((float*)(p_data));
		theVector[Y] = *((float*)(p_data+4));
		theVector[Z] = *((float*)(p_data+8));
		theVector[W] = *((float*)(p_data+12));
		p_data += sizeof(Mth::Vector);

		Mth::Matrix theNeutralPoseMatrix;

		// gets the skeleton into the model's space
		Mth::QuatVecToMatrix( &theQuat, &theVector, &theNeutralPoseMatrix );

		if ( i != 0 )
		{
			Mth::Matrix neutral_parent_matrix = *(mp_inverseNeutralPoseMatrices + GetIndex(GetParentName(i)));

			// GJ:  shouldn't this already be inverted from
			// previous iterations of the for-loop?
			neutral_parent_matrix.InvertUniform();

			theNeutralPoseMatrix *= neutral_parent_matrix;

			theNeutralPoseMatrix[X][W] = 0.0f;
			theNeutralPoseMatrix[Y][W] = 0.0f;
			theNeutralPoseMatrix[Z][W] = 0.0f;
			theNeutralPoseMatrix[W][W] = 1.0f;
		}

		theNeutralPoseMatrix.InvertUniform();
		*(mp_inverseNeutralPoseMatrices + i) = theNeutralPoseMatrix;
	}

	// if we get here, then it's successful
	success = true;

//ERROR:
    return success;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkeletonData::InitialiseBoneSkipList( const char* p_fileName )
{
	// Stip leading path information and trailing extensions to obtain the name of the skeleton.
	char skeleton_name[128];

	char *p_copy = strrchr( p_fileName, '/' );
	if( p_copy == NULL )
		p_copy = strrchr( p_fileName, '\\' );

	if( p_copy )
	{
		++p_copy;
		strcpy( skeleton_name, p_copy );
	}
	else
	{
		strcpy( skeleton_name, p_fileName );
	}

	p_copy = strchr( skeleton_name, '.' );
	if( p_copy )
	{
		*p_copy = '\0';
	}

	// First, get a pointer to the script global SkipBoneLODInfo.
	Script::CStruct *p_all_lod_info = Script::GetStructure( CRCD( 0x7da53781, "BoneSkipLODInfo" ));
	if( p_all_lod_info )
	{
		Script::CArray *p_skeleton_lod_info = NULL;
		p_all_lod_info->GetArray( skeleton_name, &p_skeleton_lod_info );

		// No requirement that there *must* be SkipBoneLODInfo for a given skeleton.
		if( p_skeleton_lod_info )
		{
			// Now run through each element of the array. Each element is a structure.
			uint32 array_size = p_skeleton_lod_info->GetSize();
			for( uint32 lod_level = 0; lod_level < array_size; ++lod_level )
			{
				Script::CStruct *p_element = p_skeleton_lod_info->GetStructure( lod_level );
	
				// Obtain the distance, required, below which this LOD is active.
				float lod_distance = 0.0f;
				if( p_element->GetFloat( "LODDistance", &lod_distance ))
				{
					// Convert to inches.
					lod_distance = FEET_TO_INCHES( lod_distance );

					// Store the square of the distance (in inches).
					// Special case - if this is the last lod level, set the distance such that it will always be active.
					if( lod_level == ( array_size - 1 ))
					{
						m_skipLODDistances[lod_level] = MAX_LOD_DISTANCE;
					}
					else
					{
						m_skipLODDistances[lod_level] = lod_distance * lod_distance;
					}
				}
				else
				{
					Dbg_MsgAssert( 0, ( "Missing LODDistance field for LOD level %d in skeleton %s\n", lod_level, skeleton_name ));
				}
	
				Script::CArray *p_skip_bones_array = NULL;
				if( p_element->GetArray( "SkipBones", &p_skip_bones_array ))
				{
					// There are bones to be skipped for this entry.
					uint32 size = p_skip_bones_array->GetSize();
					for( uint32 bone = 0; bone < size; ++bone )
					{
						// Get the checksum of the bone name.
						uint32 bone_checksum = p_skip_bones_array->GetChecksum( bone );

						// Obtain the bone index from this bone checksum.
						uint32 bone_index			= GetIndex( bone_checksum );

						// Set the bitfield flag for this bone.
						m_skipLODTable[bone_index]	= 0xFFFFFFFF & ~(( 1 << lod_level ) - 1 );
					}
				}
			}
		}
	}	
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkeletonData::Load(const char* p_fileName, bool assertOnFail)
{
	// new load of platform-specific SKE files...
	bool success = false;
	int file_size;
	uint32* p_fileBuffer = NULL;

	// open the file as a stream
	void* pStream = File::Open( p_fileName, "rb" );
	
    // make sure the file is valid
	if ( !pStream )
	{
		Dbg_MsgAssert( assertOnFail, ("Load of %s failed - file not found?", p_fileName) );
		goto ERROR;
	}

	file_size = File::GetFileSize(pStream);
	Dbg_MsgAssert( file_size, ("Skeleton file %s size is 0", p_fileName) );
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	p_fileBuffer = (uint32*)Mem::Malloc( file_size );
	Mem::Manager::sHandle().PopContext();
	
	if ( !File::Read( p_fileBuffer, file_size, 1, pStream ) )
	{
		Dbg_MsgAssert( assertOnFail, ("Load of %s failed - read failed?", p_fileName) );
		goto ERROR;
	}

	if ( !Load( p_fileBuffer, file_size, assertOnFail ) )
	{
		Dbg_MsgAssert( assertOnFail, ("Load of %s failed - parse failed?", p_fileName) );
		goto ERROR;
	}

	// GJ:  THPS4 patch for weird head scaling on female peds...
	if ( strstr( p_fileName, "ped_f" ) )
	{
		m_flags |= 0x1;
	}

	// if we get here, then it's successful
	success = true;

ERROR:
	if ( p_fileBuffer )
	{
		Mem::Free( p_fileBuffer );
	}
	File::Close(pStream);    

	InitialiseBoneSkipList( p_fileName );

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CSkeletonData::GetNumBones() const
{
    return m_numBones;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CSkeletonData::GetBoneName( int index )
{
    Dbg_Assert( index >= 0 && index < m_numBones );
    
    return m_boneNameTable[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CSkeletonData::GetParentName( int index )
{
    Dbg_Assert( index >= 0 && index < m_numBones );
    
    return m_parentNameTable[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CSkeletonData::GetParentIndex( int index )
{
    Dbg_Assert( index >= 0 && index < m_numBones );
    
    return GetIndex( m_parentNameTable[index] );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CSkeletonData::GetFlipName( int index )
{
    Dbg_Assert( index >= 0 && index < m_numBones );
    
    return m_flipNameTable[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CSkeletonData::GetIndex( uint32 boneName )
{
    for ( int i = 0; i < m_numBones; i++ )
    {
        if ( boneName == m_boneNameTable[i] )
            return i;
    }

    // not found
    Dbg_Assert( 0 );
    return 0;
}



#if 0
// The following functions have been deprecated...
// they were formerly used to find the correct
// default SKA file in order to build the neutral
// pose matrices.  Now, however, the neutral pose
// is stored inside the SKE file
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkeletonData::SetAnimScriptName( uint32 anim_script_name )
{
	m_animScriptName = anim_script_name;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CSkeletonData::GetAnimScriptName() const
{
	return m_animScriptName;
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Matrix* CSkeletonData::GetInverseNeutralPoseMatrices()
{
	return mp_inverseNeutralPoseMatrices;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx


// DEBUG FUNCTIONS

#if 0

	#if 0
		// draw the bone between the origin of that bone, and its parent
		tempMatrix0 = (*p_currentBone->mp_parentMatrix) * (*pRootMatrix);
		tempMatrix1 = ( ( mp_matrices + i ) ) * (*pRootMatrix); 
		AddDebugBone( tempMatrix0.GetPos(), tempMatrix1.GetPos(), r, g, b );	
	#endif

	#if 0
		// * test neutral-in-skater-space pose *
		tempMatrix0 = *p_currentBone->mp_invertedNeutralParentMatrix;
		tempMatrix0.InvertUniform();
		tempMatrix0	= tempMatrix0 * (*pRootMatrix); 
			
		tempMatrix1 = p_currentBone->m_invertedNeutralMatrix;
		tempMatrix1.InvertUniform();
		tempMatrix1 = tempMatrix1 * (*pRootMatrix);
			
		AddDebugBone( tempMatrix0.GetPos(), tempMatrix1.GetPos(), r, g, b );	
	#endif
	
	#if 0
		// * test inverted-to-neutral *
		Mth::Matrix boneMatrix;
		boneMatrix = *( ( mp_matrices + i ) );
		tempMatrix0 = tempMatrix1;
		tempMatrix1 = p_currentBone->m_neutralMatrix * boneMatrix;
		tempMatrix1 *= (*pRootMatrix);
		AddDebugBone( tempMatrix0.GetPos(), tempMatrix1.GetPos(), r, g, b );
	#endif

#endif


#if 0
	// Programmatic jaw test
	if ( p_currentBone->m_name == Script::GenerateCRC("jaw") )
	{
		static int jaw_rotation = 0;
		jaw_rotation++;
		p_currentMatrix->RotateLocal(Mth::Vector( Mth::DegToRad((float)(jaw_rotation)/10.0f), 0.0f, 0.0f));
		if (jaw_rotation>60)
		{
			jaw_rotation=0;
		}
	}
#endif

#if 0
	// This scales the item, with respect to the model's
	// origin.  There are some neat looking effects
	// that can be done with this, such as a gorilla man
	// with a over-sized upper body, but makes the lip
	// tricks look incorrect.
	if ( Script::GetInt( "ScalingTest", false ) )
	{
		// override the scale if the GorillaMode is set...
		Script::CArray* pArray = Script::GetArray( "master_scaling_table" );
		if ( pArray && i < (int)pArray->GetSize() )
		{
			Script::CVector* pVector = pArray->GetVector(i);
			Mth::Vector theVector;
			const float weight_scale=Script::GetFloat("weight_scale");
			theVector[X] = 1.0f + (pVector->mX-1.0f) * weight_scale;
			theVector[Y] = 1.0f + (pVector->mY-1.0f) * weight_scale;
			theVector[Z] = 1.0f + (pVector->mZ-1.0f) * weight_scale;
			p_currentMatrix->Scale(theVector);
		}
	}
#endif
