//****************************************************************************
//* MODULE:			Gfx
//* FILENAME:		Skeleton.h
//* OWNER:			Gary Jesdanun
//* CREATION DATE:	11/15/2001
//****************************************************************************

#ifndef __GFX_SKELETON_H
#define __GFX_SKELETON_H

/*****************************************************************************
**								Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

#include <gfx/pose.h>

/*****************************************************************************
**								Defines									**
*****************************************************************************/

namespace Mth
{
	class Matrix;
	class Quat;
	class Vector;
}
									
namespace Script
{
	class CStruct;
};

namespace Gfx
{

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

	class CBone;

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/
    
class CSkeletonData	: public Spt::Class
{
public:

	static const int	BONE_SKIP_LOD_BITS = 32;

					CSkeletonData();
	virtual			~CSkeletonData();
	bool			Load( const char* p_fileName, bool assertOnFail );
	bool			Load( uint32* p_data, int data_size, bool assertOnFail );
	void			InitialiseBoneSkipList( const char* p_fileName );
//	void			SetAnimScriptName( uint32 animScriptName );

public:
	int				GetNumBones() const;
	uint32			GetBoneName( int index );
	uint32			GetParentName( int index );
	uint32			GetParentIndex( int index );
	uint32			GetFlipName( int index );
	uint32			GetIndex( uint32 boneName );
//	uint32			GetAnimScriptName() const;
	uint32*			GetBoneSkipList( void )				{ return m_skipLODTable; }
	float			GetBoneSkipDistance( uint32 lod )	{ return m_skipLODDistances[lod]; }

	Mth::Matrix*	GetInverseNeutralPoseMatrices();

protected:
	int				m_numBones;
	uint32			m_boneNameTable[vMAX_BONES];
	uint32			m_parentNameTable[vMAX_BONES];
	uint32			m_flipNameTable[vMAX_BONES];
	uint32			m_skipLODTable[vMAX_BONES];				// Each uint32 for a specific bone provides 32 levels of skip information.
	float			m_skipLODDistances[BONE_SKIP_LOD_BITS];
	uint32			m_animScriptName;
	Mth::Matrix* 	mp_inverseNeutralPoseMatrices;
	
public:
	int				m_flags;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class CSkeleton
{
public:
	CSkeleton( CSkeletonData* pSkeletonData );
	~CSkeleton();

public:
	int						GetNumBones( void ) const;
	bool					ApplyBoneScale( Script::CStruct* pBodyShapeStructure );
	void					ResetScale( void );
	
public:
	void 					Update( CPose* pPose );
	void					Update( Mth::Quat* pQuat, Mth::Vector* pTrans );
	void					Display( Mth::Matrix* pMatrix, float r, float g, float b );
	
public:
	Mth::Matrix*			GetMatrices();
	Mth::Matrix				GetNeutralMatrix( int boneIndex );
	bool					GetBoneMatrix( uint32 boneId, Mth::Matrix* pMatrix );
	bool					GetBonePosition( uint32 boneId, Mth::Vector* pVector );
	int						GetBoneIndexById( uint32 boneId );
	int						GetFlipIndex( int boneIndex );
//	void					GetSkipList( int* pSkipList );
	uint32*					GetBoneSkipList( void );
	uint32					GetBoneSkipIndex( void )		{ return m_skipIndex; }
	void					SetBoneSkipDistance( float dist );
	void					SetMaxBoneSkipLOD( uint32 max )	{ m_maxBoneSkipLOD = max; }

public:
	// The following should be moved to the CAnimationComponent class	
//	bool					GetBoneRotationByIndex( int boneIndex );

public:
	bool					SetBoneActive( uint32 boneId, bool active );
	bool					SetBoneScale( uint32 boneId, const Mth::Vector& theBoneScale, bool isLocalScale );
	bool					GetBoneScale( uint32 boneId, Mth::Vector* pBoneScaleVector );
	void					CopyBoneScale( Gfx::CSkeleton* pSourceSkeleton );
	void					SetNeutralPose( Mth::Quat* pQuat, Mth::Vector* pTrans );
	CSkeletonData*			GetSkeletonData() { return mp_skeletonData; }

protected:
	CBone*					get_bone_by_id( uint32 boneId );
	void					update_matrices();

protected:
	uint32					m_flags;
	Mth::Matrix*			mp_matrices;
    CSkeletonData*			mp_skeletonData;
	uint32					m_skipIndex;
	uint32					m_maxBoneSkipLOD;

	// for non-procedural bone anims
	int						m_numBones;
	CBone*					mp_bones;
	
protected:
	void					initialize_hierarchy( CSkeletonData* pSkeletonData );
	void					initialize_flip_matrices( CSkeletonData* pSkeletonData );
	void					initialize_bone_names( CSkeletonData* pSkeletonData );
};

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Gfx

#endif	// __GFX_SKELETON_H

