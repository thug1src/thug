//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       quickanim.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/4/2003
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/nxquickanim.h>

#include <gfx/bonedanim.h>
#include <gfx/nxanimcache.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Nx
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

inline float quatDown(short theSource)
{
    return (float)(theSource / 16384.0f);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float transDown(short theSource, float scaleFactor)
{
    return (float)(theSource / scaleFactor);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void get_rotation_from_key( Gfx::CAnimQKey* p_in, Mth::Quat* pQuat, bool isHiRes )
{
	if ( isHiRes )
	{
		(*pQuat)[X] = ((Gfx::CHiResAnimQKey*)p_in)->qx;
		(*pQuat)[Y] = ((Gfx::CHiResAnimQKey*)p_in)->qy;
		(*pQuat)[Z] = ((Gfx::CHiResAnimQKey*)p_in)->qz;
//  	(*pQuat)[W] = ((Gfx::CHiResAnimQKey*)p_in)->qw;
	}
	else
	{
		(*pQuat)[X] = quatDown( ((Gfx::CStandardAnimQKey*)p_in)->qx );
		(*pQuat)[Y] = quatDown( ((Gfx::CStandardAnimQKey*)p_in)->qy );
		(*pQuat)[Z] = quatDown( ((Gfx::CStandardAnimQKey*)p_in)->qz );
//		(*pQuat)[W] = quatDown( ((Gfx::CStandardAnimQKey*)p_in)->qw );
	}
	
	float qx = (*pQuat)[X];
	float qy = (*pQuat)[Y];
	float qz = (*pQuat)[Z];

	// Dave note: added 09/12/02 - a simple check to ensure we don't try to take the square root of a negative
	// number, which will hose Nan-sensitive platforms later on...
	float sum	= 1.0f - qx * qx - qy * qy - qz * qz;
	(*pQuat)[W] = sqrtf (( sum < 0.0f ) ? 0.0f : sum );
	
	if ( p_in->signBit )
	{
		(*pQuat)[W] = -(*pQuat)[W];
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void get_translation_from_key( Gfx::CAnimTKey* p_in, Mth::Vector* pVector, bool isHiRes )
{
	if ( isHiRes )
	{
		(*pVector)[X] = ((Gfx::CHiResAnimTKey*)p_in)->tx;
		(*pVector)[Y] = ((Gfx::CHiResAnimTKey*)p_in)->ty;
		(*pVector)[Z] = ((Gfx::CHiResAnimTKey*)p_in)->tz;
		(*pVector)[W] = 1.0f;
	}
	else
	{
		(*pVector)[X] = transDown( ((Gfx::CStandardAnimTKey*)p_in)->tx, 32.0f );
		(*pVector)[Y] = transDown( ((Gfx::CStandardAnimTKey*)p_in)->ty, 32.0f );
		(*pVector)[Z] = transDown( ((Gfx::CStandardAnimTKey*)p_in)->tz, 32.0f );
		(*pVector)[W] = 1.0f;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float get_alpha( float timeStamp1, float timeStamp2, float time )
{
	Dbg_MsgAssert(timeStamp1 <= time && timeStamp2 >= time, ( "%f should be within [%f %f]", time, timeStamp1, timeStamp2 ));
    
	return (( time - timeStamp1 ) / ( timeStamp2 - timeStamp1 ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
						  
inline void interpolate_q_frame(Mth::Quat* p_out, Gfx::CAnimQKey* p_in1, Gfx::CAnimQKey* p_in2, float alpha, bool isHiRes)
{
	Dbg_Assert(p_out);
	Dbg_Assert(p_in1);
	Dbg_Assert(p_in2);

	Mth::Quat	qIn1;
	get_rotation_from_key( p_in1, &qIn1, isHiRes );

	if ( alpha == 0.0f )
	{
		// don't need to slerp, because it's the start time
		*p_out = qIn1; 
		return;
	}

	Mth::Quat   qIn2;
	get_rotation_from_key( p_in2, &qIn2, isHiRes );
    
	if ( alpha == 1.0f )
	{
		// don't need to slerp, because it's the end time
		*p_out = qIn2; 
		return;
	}

	// fast slerp, stolen from game developer magazine
	*p_out = Mth::FastSlerp( qIn1, qIn2, alpha );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void interpolate_t_frame(Mth::Vector* p_out, Gfx::CAnimTKey* p_in1, Gfx::CAnimTKey* p_in2, float alpha, bool isHiRes)
{
	Dbg_Assert(p_out);
	Dbg_Assert(p_in1);
	Dbg_Assert(p_in2);
    
	// INTERPOLATE T-COMPONENT
    Mth::Vector   tIn1;					  
	get_translation_from_key( p_in1, &tIn1, isHiRes );

	if ( alpha == 0.0f )
	{
		// don't need to lerp, because it's the start time
		*p_out = tIn1; 
		return;
	}

	Mth::Vector   tIn2;
	get_translation_from_key( p_in2, &tIn2, isHiRes );

	if ( alpha == 1.0f )
	{
		// don't need to slerp, because it's the end time
		*p_out = tIn2; 
		return;
	}

    /* Linearly interpolate positions */
    *p_out = Mth::Lerp( tIn1, tIn2, alpha );
}

///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions are provided here:
// so engine implementors can leave certain functionality until later

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CQuickAnim::plat_get_interpolated_frames( Mth::Quat* pRotations, Mth::Vector* pTranslations, uint32* pSkipList, uint32 skipIndex, float time )
{
	Dbg_MsgAssert( mp_frameData, ( "No pointer to frame data" ) );

/*
	// this optimization doesn't really speed things up at all,
	// since the time-consuming part is walking through the compressed
	// key lists in CBonedAnimFrameData::GetCompressedInterpolatedFrames.
	// i've commented this out for now until I have more time
	// to figure out a better alternative...
	
	if ( m_quickAnimPointers.valid )
	{
		float timeStamp = time * 60.0f;
		int numBones = mp_frameData->GetNumBones();
		Mth::Quat* pCurrRotations = pRotations;
		Mth::Vector* pCurrTranslations = pTranslations;

		for ( int i = 0; i < numBones; i++ )
		{
			m_quickAnimPointers.qSkip[i] = false;

			Gfx::CAnimQKey* pStartQKey = &m_quickAnimPointers.theStartQKey[i];
			Gfx::CAnimQKey* pEndQKey = &m_quickAnimPointers.theEndQKey[i];

			// if the two adjacent keys haven't changed,
			// then we can use the optimization
			if ( timeStamp >= (int)pStartQKey->timestamp && timeStamp <= (int)pEndQKey->timestamp )
			{
				float qAlpha = get_alpha( (float)pStartQKey->timestamp, (float)pEndQKey->timestamp, timeStamp );

				interpolate_q_frame( pCurrRotations, 
								 pStartQKey, 
								 pEndQKey,
								 qAlpha,
								 false );

				m_quickAnimPointers.qSkip[i] = true;
			}

			pCurrRotations++;

			m_quickAnimPointers.tSkip[i] = false;

			Gfx::CAnimTKey* pStartTKey = &m_quickAnimPointers.theStartTKey[i];
			Gfx::CAnimTKey* pEndTKey = &m_quickAnimPointers.theEndTKey[i];

			// if the two adjacent keys haven't changed,
			// then we can use the optimization
			if ( timeStamp >= (int)pStartTKey->timestamp && timeStamp <= (int)pEndTKey->timestamp )
			{
				float tAlpha = get_alpha( (float)pStartTKey->timestamp, (float)pEndTKey->timestamp, timeStamp );

				interpolate_t_frame( pCurrTranslations, 
								 pStartTKey, 
								 pEndTKey,
								 tAlpha,
								 false );

				m_quickAnimPointers.tSkip[i] = true;
			}
			
			pCurrTranslations++;
		}
	}
*/



	m_quickAnimPointers.pSkipList	= pSkipList;
	m_quickAnimPointers.skipIndex	= skipIndex;
	
	Dbg_MsgAssert( mp_frameData, ( "No frame data" ) );
	mp_frameData->GetInterpolatedFrames(pRotations, pTranslations, time, this);
	
	m_quickAnimPointers.valid = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CQuickAnim::CQuickAnim()
{
	mp_frameData = NULL;
	m_quickAnimPointers.valid = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CQuickAnim::~CQuickAnim()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CQuickAnim::Enable( bool enabled )
{
	m_quickAnimPointers.valid = enabled;

	// if the cache is invalid, then regrab the pointer to the anim
	if ( !enabled )
	{
		mp_frameData = Nx::GetCachedAnim( m_animAssetName, true );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CQuickAnim::SetAnimAssetName( uint32 animAssetName )
{
	m_animAssetName = animAssetName;
	
	// set the pointer to the animation
	mp_frameData = Nx::GetCachedAnim( m_animAssetName, true );
	
	// invalidate the cache
	Enable(false);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// These functions are the platform independent part of the interface to 
// the platform specific code
// parameter checking can go here....
// although we might just want to have these functions inline, or not have them at all?

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CQuickAnim::GetInterpolatedFrames( Mth::Quat* pRotations, Mth::Vector* pTranslations, uint32* pSkipList, uint32 skipIndex, float time )
{
	plat_get_interpolated_frames( pRotations, pTranslations, pSkipList, skipIndex, time );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CQuickAnim::GetInterpolatedHiResFrames( Mth::Quat* pRotations, Mth::Vector* pTranslations, float time )
{
	Dbg_MsgAssert( mp_frameData, ( "No pointer to frame data" ) );

	mp_frameData->GetInterpolatedCameraFrames(pRotations, pTranslations, time, this);
	
	m_quickAnimPointers.valid = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CQuickAnim::GetNumBones()
{
	Dbg_MsgAssert( mp_frameData, ( "No pointer to frame data" ) );

	return mp_frameData->GetNumBones();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CQuickAnim::ResetCustomKeys()
{
	Dbg_MsgAssert( mp_frameData, ( "No pointer to frame data" ) );

	mp_frameData->ResetCustomKeys();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CQuickAnim::GetDuration()
{
	Dbg_MsgAssert( mp_frameData, ( "No pointer to frame data" ) );

	return mp_frameData->GetDuration();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
uint32 CQuickAnim::GetBoneName( int i )
{
	Dbg_MsgAssert( mp_frameData, ( "No pointer to frame data" ) );

	return mp_frameData->GetBoneName( i );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CQuickAnim::ProcessCustomKeys( float startTimeInclusive, float endTimeInclusive, Obj::CObject* pObject )
{
	Dbg_MsgAssert( mp_frameData, ( "No pointer to frame data" ) );

	bool inclusive = true;
	return mp_frameData->ProcessCustomKeys( startTimeInclusive, endTimeInclusive, pObject, inclusive );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}

