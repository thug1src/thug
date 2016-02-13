//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       bonedanim.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  11/14/2001
//****************************************************************************

#ifndef __GFX_BONEDANIM_H
#define __GFX_BONEDANIM_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>
#include <core/math.h>

#include <core/string/cstring.h>

#include <sys/file/AsyncFilesys.h>

#include <gfx/customanimkey.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

namespace Nx
{
	class CQuickAnim;
}

namespace Obj
{
	class CObject;
}

namespace File
{
	class CAsyncFileHandle;
}
			 
namespace Gfx
{
	class CAnimQKey;
	class CAnimTKey;
	struct SAnimCustomKey;
	struct SQuickAnimPointers;

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

// Raw Animation Data
class CBonedAnimFrameData
{
public:
    CBonedAnimFrameData();
    ~CBonedAnimFrameData();

public:
    bool    			   	Load(uint32* pData, int file_size, bool assertOnFail);
    bool    			   	Load(const char* p_fileName, bool assertOnFail, bool async, bool use_pip = false);
    bool    			   	PostLoad(bool assertOnFail, int file_size, bool delete_buffer = true);
	bool					LoadFinished();
	bool					IsValidTime(float time);
	float					GetDuration();
	int						GetNumBones();
	uint32					GetBoneName( int index );
    bool				    GetInterpolatedFrames(Mth::Quat* pRotations, Mth::Vector* pTranslations, float time, Nx::CQuickAnim* = NULL);
    bool				    GetCompressedInterpolatedFrames(Mth::Quat* pRotations, Mth::Vector* pTranslations, float time, Nx::CQuickAnim* = NULL);
    bool				    GetCompressedInterpolatedPartialFrames(Mth::Quat* pRotations, Mth::Vector* pTranslations, float time, Nx::CQuickAnim* = NULL);
    bool				    GetInterpolatedCameraFrames(Mth::Quat* pRotations, Mth::Vector* pTranslations, float time, Nx::CQuickAnim* = NULL);
	bool					ResetCustomKeys( void );
	bool					ProcessCustomKeys( float startTimeInclusive, float endTimeExclusive, Obj::CObject* pObject, bool endInclusive = false );

	CAnimQKey*				GetQFrames( void ) { return (CAnimQKey*)mp_qFrames; }
	CAnimTKey*				GetTFrames( void ) { return (CAnimTKey*)mp_tFrames; }

	void					SetQFrames( CAnimQKey * p_q ) { mp_qFrames = (char*)p_q; }
	void					SetTFrames( CAnimTKey * p_t ) { mp_tFrames = (char*)p_t; }

protected:	
	int						get_num_customkeys();
	CCustomAnimKey*			get_custom_key( int index );
	int						get_num_qkeys( void * p_frame_data, int boneIndex ) const;
	int						get_num_tkeys( void * p_frame_data, int boneIndex ) const;
	void					set_num_qkeys( int boneIndex, int numKeys );
	void					set_num_tkeys( int boneIndex, int numKeys );
	bool					is_hires() const;

	static void 			async_callback(File::CAsyncFileHandle *, File::EAsyncFunctionType function,
										   int result, unsigned int arg0, unsigned int arg1);

protected:
//	bool				   	intermediate_read_stream(void* pStream);
    bool                    plat_read_stream(uint8* pData, bool delete_buffer = true);
    bool                    plat_read_compressed_stream(uint8* pData, bool delete_buffer = true);
	bool					plat_dma_to_aram( int qbytes = 0, int tbytes = 0, uint32 flags = 0 );
	
protected:
	float				   	m_duration;		  // could be a short
	uint32					m_flags;
	int						m_numBones;

	// file buffer (the only malloc'ed pointer)
	void*					mp_fileBuffer;
	File::CAsyncFileHandle* mp_fileHandle;
	int						m_fileSize;

	// massive block of all Q-frames and T-frames
	char*		   			mp_qFrames;
	char*		   			mp_tFrames;

	// per-bone pointers into the massive block of Q-frames and T-frames
	void*					mp_perBoneFrames;

	// list of bone names (for object anims only)
	uint32*					mp_boneNames;
		
	// original number of bones + list of bone masks (for partial anims only)
	uint32*					mp_partialAnimData;

	uint32					m_fileNameCRC;

	// custom keys (for cameras, changing parent bones, etc.)
	CCustomAnimKey** 		mpp_customAnimKeyList;

	uint16*					mp_perBoneQFrameSize;
	uint16*					mp_perBoneTFrameSize;
	
	short					m_num_qFrames;
	short					m_num_tFrames;
	short					m_num_customKeys;
	
	short					m_printDebugInfo:1;
	short					m_dataLoaded:1;
	short					m_pipped:1;
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

bool InitQ48Table( const char* pFileName, bool assertOnFail = true );
bool InitT48Table( const char* pFileName, bool assertOnFail = true );

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline float CBonedAnimFrameData::GetDuration()
{
	return m_duration;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline int CBonedAnimFrameData::GetNumBones()
{
	return m_numBones;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline bool CBonedAnimFrameData::LoadFinished()
{
	return m_dataLoaded;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // namespace Gfx

#endif	// __GFX_BONEDANIM_H
