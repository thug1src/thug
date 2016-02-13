#ifndef __SYS_REPLAY_H
#define __SYS_REPLAY_H

#define	__USE_REPLAYS__ 0

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __CORE_TASK_H
#include <core/task.h>
#endif

#ifndef __CORE_MATH_VECTOR_H
#include <core/math/vector.h>
#endif

#ifndef __GFX_ANIMCONTROLLER_H
#include <gfx/animcontroller.h>
#endif

#ifndef	__GFX_NXMODEL_H__
#include <gfx/nxmodel.h>
#endif

#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif

#ifndef __GEL_SOUNDFX_H
#include <gel/soundfx/soundfx.h>
#endif

#ifndef __GEL_COMPONENTS_VIBRATIONCOMPONENT_H
#include <gel/components/vibrationcomponent.h>
#endif

#ifndef __OBJECTS_SKATER_H
#include <sk/objects/skater.h>
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Obj
{
	class CObject;
	class CSkeletonComponent;
}

namespace Script
{
	class CStruct;
	class CScript;
}
	
namespace Replay
{

class  Manager  : public Spt::Class
{
	Tsk::Task< Manager > *mp_start_frame_task;
	static	Tsk::Task< Manager >::Code 	s_start_frame_code;

	Tsk::Task< Manager > *mp_end_frame_task;
	static	Tsk::Task< Manager >::Code 	s_end_frame_code;

public:
	Manager();
	~Manager();
	
private:	
	DeclareSingletonClass( Manager );
};

// 128 allows for a max of 4096 sectors
#define SECTOR_STATUS_BUFFER_SIZE 128
struct SGlobalStates
{
public:
	SGlobalStates();
	~SGlobalStates();

	void Reset();
	// Warning! If any new members get added or removed, update VERSION_REPLAY in mcfuncs.cpp
	// Also update the Reset function.
	bool mManualMeterStatus;
	float mManualMeterValue;
	bool mBalanceMeterStatus;
	float mBalanceMeterValue;
	
	int mActuatorStrength[Obj::CVibrationComponent::vVB_NUM_ACTUATORS];
	uint32 mpSectorStatus[SECTOR_STATUS_BUFFER_SIZE];
	bool mSkaterPaused;
};

struct SReplayDataHeader
{
	uint32 mBufferStartOffset;
	uint32 mBufferEndOffset;
	int mNumStartStateDummies;
	SGlobalStates mStartState;
};

enum
{
	NUM_DEGENERATE_ANIMS=3
};	

enum
{
	DUMMY_FLAG_FLIPPED		= (1<<0),
	DUMMY_FLAG_HOVERING		= (1<<1),
	DUMMY_FLAG_ACTIVE		= (1<<2),
	// Keep the number of these <= 32
};
	
struct SSavedDummy
{
	uint32 m_id;
	int	m_type;
//	char mpModelName[MAX_MODEL_NAME_CHARS+1];	
	uint32 mSkeletonName;
	uint32 mProfileName;
	uint32 mAnimScriptName;
	uint32 mSectorName;
	uint32 mFlags;
	float  mScale;
	
	Mth::Vector m_pos;
	Mth::Vector m_vel;
	Mth::Vector m_angles;
	Mth::Vector m_ang_vel;
	float m_car_rotation_x;
	float m_car_rotation_x_vel;
	float m_wheel_rotation_x;
	float m_wheel_rotation_x_vel;
	float m_wheel_rotation_y;
	float m_wheel_rotation_y_vel;

	Gfx::CAnimChannel	m_primaryController;

    Gfx::CAnimChannel   m_degenerateControllers[NUM_DEGENERATE_ANIMS];
	uint32 mAtomicStates;
	// These only apply to the skater.
	uint32 m_looping_sound_checksum;
	float m_pitch_min;
	float m_pitch_max;
};

enum EDummyList
{
	START_STATE_DUMMY_LIST,
	PLAYBACK_DUMMY_LIST,
};

class CDummy : public Obj::CMovingObject
{
public:	
	CDummy(EDummyList whichList, uint32 id);
	virtual ~CDummy(void);

	// If any new members are added to CDummy, remember to update the following:
	// CDummy constructor, initialise new member
	// assignement operator, copy new member.
	// CDummy::Save function, save new member, and add to SSavedDummy, and update mem card version number
	// Replay::CreateDummyFromSaveData function, copy in new member.
	// Update VERSION_REPLAY in memcard.q
	CDummy& operator=( const CDummy& rhs );

	EDummyList m_list;
	CDummy *mpNext;
	CDummy *mpPrevious;
	
	void Update();
	void CreateModel();
	void DisplayModel();
	void UpdateMutedSounds();
	void UpdateLoopingSound();
	
	void Save(SSavedDummy *p_saved_dummy);

//	char mpModelName[MAX_MODEL_NAME_CHARS+1];	
	uint32 mSkeletonName;
	uint32 mProfileName;
	uint32 mAnimScriptName;
	uint32 mSectorName;

	bool m_is_displayed;
	
	float mScale;
	
	uint32 mFlags;

	int mHoverPeriod;
	
	Mth::Vector m_vel;
	Mth::Vector m_angles;
	Mth::Vector m_ang_vel;

	float m_car_rotation_x;
	float m_car_rotation_x_vel;
	float m_wheel_rotation_x;
	float m_wheel_rotation_x_vel;
	float m_wheel_rotation_y;
	float m_wheel_rotation_y_vel;
	
	Gfx::CAnimChannel 	m_primaryController;

    Gfx::CAnimChannel    m_degenerateControllers[NUM_DEGENERATE_ANIMS];
	
    Nx::CModel *mp_rendered_model;
	Gfx::Camera *mp_skater_camera;
	//Obj::CSkaterCam *mp_skater_camera;
	
	uint32 mAtomicStates;
	
	uint32 m_looping_sound_id;
	uint32 m_looping_sound_checksum;
	float m_pitch_min;
	float m_pitch_max;

    // TEMP
    bool LoadSkeleton( uint32 skeletonChecksum );
    Gfx::CSkeleton* GetSkeleton();
    Obj::CSkeletonComponent* mp_skeletonComponent;
};

// The bigger this is the better, since it will speed up saving of replays to mem card.
// A static buffer of this size exists though, so don't make it too big.
#define REPLAY_BUFFER_CHUNK_SIZE 2048


#if __USE_REPLAYS__  

// These functions have platform specific versions.
// On the GameCube for example, the buffer is in ARAM.
void AllocateBuffer();
void DeallocateBuffer();
bool BufferAllocated();
uint32 GetBufferSize();
void ReadFromBuffer(uint8 *p_dest, int bufferOffset, int numBytes);
void WriteIntoBuffer(uint8 *p_source, int bufferOffset, int numBytes);
void FillBuffer(int bufferOffset, int numBytes, uint8 value);
void WriteReplayDataHeader(SReplayDataHeader *p_header);
void ReadReplayDataHeader(const SReplayDataHeader *p_header);
void CreateDummyFromSaveData(SSavedDummy *p_saved_dummy);
CDummy *GetFirstStartStateDummy();

void ClearBuffer();
uint8 *GetTempBuffer();
void CreatePools();
void RemovePools();
void DeallocateReplayMemory();
bool ScriptDeleteDummies( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptHideGameObjects( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptShowGameObjects( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptRunningReplay( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptPauseReplay( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptUnPauseReplay( Script::CStruct *pParams, Script::CScript *pScript );

bool ScriptAllocateReplayMemory(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptDeallocateReplayMemory(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptStartRecordingAfresh(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptRememberLevelStructureNameForReplays(Script::CStruct *pParams, Script::CScript *pScript);
void SetLevelStructureName(uint32 level_structure_name);
bool ScriptGetReplayLevelStructureName(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptPlaybackReplay(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptReplayRecordSimpleScriptCall(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptRecordPanelMessage(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSwitchToReplayRecordMode( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptSwitchToReplayIdleMode( Script::CStruct *pParams, Script::CScript *pScript );
bool ScriptClearTrickAndScoreText(Script::CStruct *pParams, Script::CScript *pScript);

void WritePadVibration(int actuator, int percent);
void WritePauseSkater();
void WriteUnPauseSkater();
void WriteScreenFlash(int viewport, Image::RGBA from, Image::RGBA to, float duration, float z, uint32 flags);
void WriteShatter(uint32 sectorName, bool on);
void WriteShatterParams(Mth::Vector& velocity, float area_test, float velocity_variance, float spread_factor, float lifetime, float bounce, float bounce_amplitude);
void WriteTextureSplat(Mth::Vector& splat_start, Mth::Vector& splat_end, float size, float lifetime, const char *p_texture_name, uint32 trail );
void WriteSectorActiveStatus(uint32 sectorName, bool active);
void WriteSectorVisibleStatus(uint32 sectorName, bool visible);
void WriteManualMeter(bool state, float value);
void WriteBalanceMeter(bool state, float value);
void WriteSparksOn();
void WriteSparksOff();
void WriteScorePotText(const char *p_text);
void WriteTrickText(const char **pp_text, int numStrings);
void WriteTrickTextPulse();
void WriteTrickTextCountdown();
void WriteTrickTextLanded();
void WriteTrickTextBail();
void WriteSetAtomicStates(uint32 id, uint32 mask);
void WritePlayStream(uint32 checksum, Sfx::sVolume *p_volume, float pitch, int priority);
void WriteStopStream(int channel);
void WritePositionalStream(uint32 dummyId, uint32 streamNameChecksum, float dropoff, float volume, float pitch, int priority, int use_pos_info);
void WritePositionalSoundEffect(uint32 dummyId, uint32 soundName, float volume, float pitch, float dropOffDist);
void WriteSkaterSoundEffect(int whichArray, int surfaceFlag, const Mth::Vector &pos, 
							float volPercent);
void WritePlaySound(uint32 checksum, float volL, float volR, float pitch);
void PrepareForReplayPlayback(bool hideObjects);
bool RunningReplay();
bool Paused();
void AddReplayMemCardInfo(Script::CStruct *p_struct);
void AddReplayMemCardSummaryInfo(Script::CStruct *p_struct);
SGlobalStates *GetStartState();

#else

// These functions have platform specific versions.
// On the GameCube for example, the buffer is in ARAM.
inline void AllocateBuffer(){}
inline void DeallocateBuffer(){}
//bool BufferAllocated(){}
//uint32 GetBufferSize(){}
inline void ReadFromBuffer(uint8 *p_dest, int bufferOffset, int numBytes){}
inline void WriteIntoBuffer(uint8 *p_source, int bufferOffset, int numBytes){}
inline void FillBuffer(int bufferOffset, int numBytes, uint8 value){}
inline void WriteReplayDataHeader(SReplayDataHeader *p_header){}
inline void ReadReplayDataHeader(const SReplayDataHeader *p_header){}
inline void CreateDummyFromSaveData(SSavedDummy *p_saved_dummy){}
//CDummy *GetFirstStartStateDummy(){}

inline void ClearBuffer(){}
//uint8 *GetTempBuffer(){}
inline void CreatePools(){}
inline void RemovePools(){}
inline void DeallocateReplayMemory(){}
inline bool ScriptDeleteDummies( Script::CStruct *pParams, Script::CScript *pScript ){return true;}
inline bool ScriptHideGameObjects( Script::CStruct *pParams, Script::CScript *pScript ){return true;}
inline bool ScriptShowGameObjects( Script::CStruct *pParams, Script::CScript *pScript ){return true;}
inline bool ScriptRunningReplay( Script::CStruct *pParams, Script::CScript *pScript ){return false;}
inline bool ScriptPauseReplay( Script::CStruct *pParams, Script::CScript *pScript ){return true;}
inline bool ScriptUnPauseReplay( Script::CStruct *pParams, Script::CScript *pScript ){return true;}

inline bool ScriptAllocateReplayMemory(Script::CStruct *pParams, Script::CScript *pScript){return true;}
inline bool ScriptDeallocateReplayMemory(Script::CStruct *pParams, Script::CScript *pScript){return true;}
inline bool ScriptStartRecordingAfresh(Script::CStruct *pParams, Script::CScript *pScript){return true;}

inline bool ScriptRememberLevelStructureNameForReplays(Script::CStruct *pParams, Script::CScript *pScript){return true;}
inline void SetLevelStructureName(uint32 level_structure_name){}
inline bool ScriptGetReplayLevelStructureName(Script::CStruct *pParams, Script::CScript *pScript){return true;}

inline bool ScriptPlaybackReplay(Script::CStruct *pParams, Script::CScript *pScript){return true;}

inline bool ScriptReplayRecordSimpleScriptCall(Script::CStruct *pParams, Script::CScript *pScript){return true;}
inline bool ScriptRecordPanelMessage(Script::CStruct *pParams, Script::CScript *pScript){return true;}
inline bool ScriptSwitchToReplayRecordMode( Script::CStruct *pParams, Script::CScript *pScript ){return true;}
inline bool ScriptSwitchToReplayIdleMode( Script::CStruct *pParams, Script::CScript *pScript ){return true;}
inline bool ScriptClearTrickAndScoreText(Script::CStruct *pParams, Script::CScript *pScript){return true;}

inline void WritePadVibration(int actuator, int percent){}
inline void WritePauseSkater(){}
inline void WriteUnPauseSkater(){}
inline void WriteScreenFlash(int viewport, Image::RGBA from, Image::RGBA to, float duration, float z, uint32 flags){}
inline void WriteShatter(uint32 sectorName, bool on){}
inline void WriteShatterParams(Mth::Vector& velocity, float area_test, float velocity_variance, float spread_factor, float lifetime, float bounce, float bounce_amplitude){}
inline void WriteTextureSplat(Mth::Vector& splat_start, Mth::Vector& splat_end, float size, float lifetime, const char *p_texture_name, uint32 trail ){}
inline void WriteSectorActiveStatus(uint32 sectorName, bool active){}
inline void WriteSectorVisibleStatus(uint32 sectorName, bool visible){}
inline void WriteManualMeter(bool state, float value){}
inline void WriteBalanceMeter(bool state, float value){}
inline void WriteSparksOn(){}
inline void WriteSparksOff(){}
inline void WriteScorePotText(const char *p_text){}
inline void WriteTrickText(const char **pp_text, int numStrings){}
inline void WriteTrickTextPulse(){}
inline void WriteTrickTextCountdown(){}
inline void WriteTrickTextLanded(){}
inline void WriteTrickTextBail(){}
inline void WriteSetAtomicStates(uint32 id, uint32 mask){}
inline void WritePlayStream(uint32 checksum, Sfx::sVolume *p_volume, float pitch, int priority){}
inline void WriteStopStream(int channel){}
inline void WritePositionalStream(uint32 dummyId, uint32 streamNameChecksum, float dropoff, float volume, float pitch, int priority, int use_pos_info){}
inline void WritePositionalSoundEffect(uint32 dummyId, uint32 soundName, float volume, float pitch, float dropOffDist){}
inline void WriteSkaterSoundEffect(int whichArray, int surfaceFlag, const Mth::Vector &pos, 
							float volPercent){}
inline void WritePlaySound(uint32 checksum, float volL, float volR, float pitch){}
inline void PrepareForReplayPlayback(bool hideObjects){}
inline bool RunningReplay(){return false;}
inline bool Paused(){return false;}
inline void AddReplayMemCardInfo(Script::CStruct *p_struct){}
inline void AddReplayMemCardSummaryInfo(Script::CStruct *p_struct){}
//SGlobalStates *GetStartState(){}
#endif

} // namespace Replay

#endif	// __SYS_REPLAY_H




