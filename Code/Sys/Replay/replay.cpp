/*	This is for storing information during a game that
	will be used to watch a replay of the game.

	So far, it is only used for single player games, in
	single session or career modes.

*/


#include <sys/replay/replay.h>

#if __USE_REPLAYS__

#include <sys/mem/poolable.h>

#include <sk/objects/movingobject.h>

#include <gfx/2D/ScreenElement2.h>
#include <gfx/2D/SpriteElement.h>
#include <gfx/skeleton.h>
#include <gfx/nxgeom.h>

#include <gel/scripting/checksum.h>
#include <gel/scripting/parse.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/script.h>
#include <gel/scripting/utils.h>
#include <gel/environment/terrain.h>
#include <gel/soundfx/soundfx.h>

#include <gel/mainloop.h>
#include <gel/music/music.h>
#include <gel/assman/assman.h>
#include <gel/objtrack.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/frontend/frontend.h>
#include <gfx/nx.h>
#include <gfx/nxmodel.h>
#include <gfx/nxmiscfx.h>
#include <gfx/modelbuilder.h>
#include <gfx/ModelAppearance.h>
#include <gfx/gfxutils.h>
#include <gfx/2d/textelement.h>
#include <gfx/2d/screenelemman.h>
#include <sk/objects/car.h>
#include <sk/objects/ped.h>
#include <sk/objects/skater.h>
#include <sk/objects/skatercareer.h>
#include <sk/objects/skaterflags.h>
#include <sk/objects/gameobj.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/gamenet/gamenet.h>
#include <gfx/nxviewman.h>
#include <sys/config/config.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/skeletoncomponent.h>
#include <gel/components/soundcomponent.h>
#include <gel/components/vibrationcomponent.h>
#include <gel/components/motioncomponent.h>

// disabling replays while we refactor
#define DISABLE_REPLAYS
//#define CHECK_TOKEN_USAGE

namespace CFuncs
{
	bool CheckButton(Inp::Handler< Mdl::FrontEnd >* pHandler, uint32 button);
}	

namespace Replay
{

DefineSingletonClass( Manager, "Replay Manager" )

// These are hard coded rather than wasting space in the replay data. It would waste quite
// a bit of space since there are lots of hovering cash icons. They all appear to use the same
// parameters anyway.
#define HOVER_ROTATE_SPEED 250.0f
#define HOVER_AMPLITUDE 10.0f
#define HOVER_PERIOD 1000
#define HOVER_PERIOD_RND 100

static uint32 sLevelStructureName=0;

static uint32 sNextPlaybackFrameOffset=0;
static bool sBufferFilling=true;
static bool sReachedEndOfReplay=false;
static bool sReplayPaused=false;
static bool sNeedToInitialiseVibration=false;
static bool sTrickTextGotCleared=false;

static uint32 sBigBufferStartOffset=0;
static uint32 sBigBufferEndOffset=0;

// This is a small buffer where the replay info for one frame is stored.
// It is then appended to the big cyclic buffer by sWriteFrameBufferToBigBuffer
#define FRAME_BUFFER_SIZE 20480
static uint8 spFrameBuffer[FRAME_BUFFER_SIZE];
static uint8 *spFrameBufferPos=NULL;

enum EReplayMode
{
	NONE,
	RECORD,
	PLAYBACK
};
static EReplayMode sMode=NONE;
	
enum EPointerType
{
	UNDEFINED,
	CMOVINGOBJECT,
	CSCREENELEMENT,
};

enum ESpecialIDs
{
	ID_SKATER=0,
	ID_CAMERA=1,
};
	
enum EAnimStartEndType
{
	START_TO_END,
	END_TO_START,
	SPECIFIC_START_END_TIMES
};

enum EReplayToken
{
	BLANK=0,
	FRAME_START,
	CREATE_OBJECT,
	KILL_OBJECT,
	OBJECT_ID,
	MODEL_NAME,
	SKELETON_NAME,
	PROFILE_NAME,
	SECTOR_NAME,
	ANIM_SCRIPT_NAME,
	SET_SCALE,
	SET_POSITION,
	SET_POSITION_ANGLES,	
	SET_VELOCITY,
	SET_ANGLE_X,
	SET_ANGLE_Y,
	SET_ANGLE_Z,
	SET_ANGULAR_VELOCITY_X,
	SET_ANGULAR_VELOCITY_Y,
	SET_ANGULAR_VELOCITY_Z,
	
	PRIMARY_ANIM_CONTROLLER_CHANGES,
	DEGENERATE_ANIM_CONTROLLER_CHANGES,
	
	PLAY_POSITIONAL_SOUND_EFFECT,
	PLAY_SKATER_SOUND_EFFECT,
	PLAY_SOUND,
	
	PLAY_LOOPING_SOUND,
	STOP_LOOPING_SOUND,
	PITCH_MIN,
	PITCH_MAX,
	
	SCREEN_BLUR,
	SCREEN_FLASH,
	
	SPARKS_ON,
	SPARKS_OFF,
	
	TRICK_TEXT,
	TRICK_TEXT_PULSE,
	TRICK_TEXT_COUNTDOWN,
	TRICK_TEXT_LANDED,
	TRICK_TEXT_BAIL,
	
	SCORE_POT_TEXT,
	
	PANEL_MESSAGE,
	
	SET_ATOMIC_STATES,
		
	MANUAL_METER_ON,
	MANUAL_METER_OFF,
	BALANCE_METER_ON,
	BALANCE_METER_OFF,
	
	FLIP,
	UNFLIP,
	
	MODEL_ACTIVE,
	MODEL_INACTIVE,
	
	PAD_VIBRATION,
	PLAY_POSITIONAL_STREAM,
	PLAY_STREAM,
	STOP_STREAM,
	
	SET_CAR_ROTATION_X,
	SET_CAR_ROTATION_X_VEL,
	SET_WHEEL_ROTATION_X,
	SET_WHEEL_ROTATION_X_VEL,
	SET_WHEEL_ROTATION_Y,
	SET_WHEEL_ROTATION_Y_VEL,
	
	HOVERING,
	
	SECTOR_ACTIVE,
	SECTOR_INACTIVE,

	SECTOR_VISIBLE,
	SECTOR_INVISIBLE,
	
	SHATTER_PARAMS,
	SHATTER_ON,
	SHATTER_OFF,

	TEXTURE_SPLAT,
	
	SCRIPT_CALL,
	SKATER_SCRIPT_CALL,
	
	PAUSE_SKATER,
	UNPAUSE_SKATER,
		
	// Keep the total number of tokens less than 256, cos they're stored in a byte.
	NUM_REPLAY_TOKEN_TYPES
};

class CPosTracker
{
	float m_calculated_last;
	float m_vel;
	float m_actual_last;
	
public:
	CPosTracker();
	~CPosTracker();
	
	void WriteChanges(float newVal, EReplayToken setToken, EReplayToken velToken, float tolerance=0.005f);
	float GetActualLast() {return m_actual_last;}
};

class CSkaterTrackingInfo
{
public:
	CSkaterTrackingInfo();
	~CSkaterTrackingInfo();

	void Reset();

    enum
    {
        NUM_DEGENERATE_ANIMS = 3
    };
    Gfx::CAnimChannel m_degenerateControllers[NUM_DEGENERATE_ANIMS];
	
	bool m_playing_looping_sound;
	uint32 m_looping_sound_checksum;
	float m_pitch_min;
	float m_pitch_max;
};

CSkaterTrackingInfo::CSkaterTrackingInfo()
{
	Reset();
}

CSkaterTrackingInfo::~CSkaterTrackingInfo()
{
}

void CSkaterTrackingInfo::Reset()
{
	for (int i=0; i<NUM_DEGENERATE_ANIMS; ++i)
	{
		m_degenerateControllers[i].Reset();
	}
	
	m_playing_looping_sound=false;
	m_looping_sound_checksum=0;
	m_pitch_min=-1.0f; // Choose strange defaults to ensure a change is detected at the start.
	m_pitch_max=-1.0f;
}

enum ETrackingInfoFlags
{
	TRACKING_INFO_FLAG_OBJECT_CREATED	= (1<<0),
	TRACKING_INFO_FLAG_FLIPPED			= (1<<1),
	TRACKING_INFO_FLAG_ACTIVE			= (1<<2),
	TRACKING_INFO_FLAG_HOVERING			= (1<<3),
};

class CTrackingInfo : public Mem::CPoolable<CTrackingInfo>
{
public:	
	CTrackingInfo();
	~CTrackingInfo();

	CTrackingInfo *mpNext;
	CTrackingInfo *mpPrevious;
	
	void SetMovingObject(Obj::CMovingObject *p_ob);
	void SetSkaterCamera();

	void RecordPositionAndAngleChanges(Mth::Vector &actual_pos, Mth::Vector &actual_angles, bool exact=false);

	uint32 m_id;
	EPointerType mPointerType;
	Obj::CSmtPtr<Obj::CMovingObject> mpMovingObject;
		
	uint32 mFlags;
	float mScale;
	
	Mth::Vector mReplayVel;
	Mth::Vector mReplayLastPos;
	Mth::Vector mActualLastPos;

	CPosTracker mTrackerAngleX;
	CPosTracker mTrackerAngleY;
	CPosTracker mTrackerAngleZ;

	Mth::Matrix mLastMatrix;
		
	CPosTracker mTrackerCarRotationX;
	CPosTracker mTrackerWheelRotationX;
	CPosTracker mTrackerWheelRotationY;

	Gfx::CAnimChannel m_primaryController;
	
	// Made this static because only the skater needs these members, and there is only one skater.
	// Need to keep the CTrackingInfo class as small as possible because a pool of 300 of them exists.
	static CSkaterTrackingInfo sSkaterTrackingInfo;
	
	static int sScreenBlurTracker;
	
};	
CSkaterTrackingInfo CTrackingInfo::sSkaterTrackingInfo;
int CTrackingInfo::sScreenBlurTracker=0;


static CTrackingInfo *spTrackingInfoHead=NULL;
static Lst::HashTable<CTrackingInfo> sTrackingInfoHashTable(8);

static CDummy *spStartStateDummies=NULL;
static CDummy *spReplayDummies=NULL;

static Lst::HashTable<CDummy> sObjectStartStateHashTable(8);
static Lst::HashTable<CDummy> sReplayDummysHashTable(8);

static SGlobalStates sStartState;
static SGlobalStates sCurrentState;
}

DefinePoolableClass(Replay::CTrackingInfo);

namespace Replay
{

#define MAX_PANEL_MESSAGE_SIZE 200

static void sDeleteObjectTrackingInfo();
static Lst::HashTable<CDummy> *sGetDummyHashTable(EDummyList whichList);
static CDummy *sGetDummyListHeadPointer(EDummyList whichList);
static void sSetDummyListHeadPointer(EDummyList whichList, CDummy *p_dummy);
static int sCountStartStateDummies();
static void sStopReplayDummyLoopingSounds();
static uint32 sReadAnimControllerChanges(uint32 offset, Gfx::CAnimChannel *p_anim_controller);
static uint32 sReadFrame(uint32 offset, bool *p_nothingFollows, EDummyList whichList);
static void sMakeEnoughSpace(uint32 frameSize);
static void sWriteFrameBufferToBigBuffer();
static void sWriteSingleToken(EReplayToken token);
static void sWriteUint8(uint8 v);
static void sWriteUint32(EReplayToken token, uint32 v);
static void sWriteUint32(uint32 v);
static void sWriteFloat(EReplayToken token, float f);
static void sWriteFloat(float f);
static void sWriteVector(EReplayToken token, Mth::Vector &v);
//static void sWriteString(EReplayToken token, const char *p_string);
//static uint8 sWriteCAnimChannelChanges(const Gfx::CAnimChannel *p_a, const Gfx::CAnimChannel *p_b);
static bool sIsFlipped(Obj::CMovingObject *p_movingObject);
static void sRecordSkaterCamera(CTrackingInfo *p_info);
static void sRecordCMovingObject(CTrackingInfo *p_info);
static void sRecord();
static void sPlayback(bool display=true);
static void sEnsureTrackerExists( Obj::CObject *p_ob, void *p_data );
static void sEnsureCameraTrackerExists();
static void sHideForReplayPlayback( Obj::CObject *p_ob, void *p_data );
static void sRestoreAfterReplayPlayback( Obj::CObject *p_ob, void *p_data );
static void sDeleteDummies(EDummyList whichList);
static void sUnpauseCertainScreenElements();
static void sSwitchOffVibrations();
static void sClearLastPanelMessage();
static void sClearTrickAndScoreText();
static Obj::CSkater *sGetSkater();
static Mdl::Score *sGetSkaterScoreObject();
#ifdef CHECK_TOKEN_USAGE
static const char *sGetTokenName(EReplayToken token);
#endif

SGlobalStates::SGlobalStates()
{
	Reset();
}

SGlobalStates::~SGlobalStates()
{
}

void SGlobalStates::Reset()
{
	mBalanceMeterStatus=false;
	mBalanceMeterValue=0.0f;

	mManualMeterStatus=false;
	mManualMeterValue=0.0f;
	
	int i;
	for (i=0; i<Obj::CVibrationComponent::vVB_NUM_ACTUATORS; ++i)
	{
		mActuatorStrength[i]=0;
	}	

	for (i=0; i<SECTOR_STATUS_BUFFER_SIZE; ++i)
	{
		mpSectorStatus[i]=0xffffffff; // All sectors active and visible
	}
	
	mSkaterPaused=false;
}

CPosTracker::CPosTracker()
{
	m_vel=0.0f;
	m_actual_last=0.0f;
	m_calculated_last=0.0f;
}

CPosTracker::~CPosTracker()
{
}
	
void CPosTracker::WriteChanges(float newVal, EReplayToken setToken, EReplayToken velToken, float tolerance)
{
	float predicted=m_calculated_last+m_vel;
	float d=newVal-predicted;
	if (fabs(d) > tolerance)
	{
		m_calculated_last=newVal;
		sWriteFloat(setToken,newVal);
		
		m_vel=newVal-m_actual_last;
		sWriteFloat(velToken,m_vel);
	}
	else
	{
		m_calculated_last=predicted;
	}	
    m_actual_last=newVal;
}

CTrackingInfo::CTrackingInfo()
{
	m_id=0xffffffff;		
	mPointerType=UNDEFINED;
	mpMovingObject=NULL;
	mFlags=0;
	mScale=1.0f;
			
	mReplayVel.Set();
	mReplayLastPos.Set();
	mActualLastPos.Set();

	// Making sure this won't match the object's matrix on the first frame so that
	// changes will get recorded for the first frame.
	mLastMatrix[0][0]=0.0f;	

	m_primaryController.Reset();
	
	mpNext=spTrackingInfoHead;
	mpPrevious=NULL;
	if (mpNext)
	{
		mpNext->mpPrevious=this;
	}
	spTrackingInfoHead=this;
}

CTrackingInfo::~CTrackingInfo()
{
	if (mpPrevious) mpPrevious->mpNext=mpNext;
	if (mpNext) mpNext->mpPrevious=mpPrevious;
	if (!mpPrevious) spTrackingInfoHead=mpNext;
	
	sTrackingInfoHashTable.FlushItem((uint32)m_id);
}

void CTrackingInfo::SetMovingObject(Obj::CMovingObject *p_ob)
{
	mPointerType=CMOVINGOBJECT;
	mpMovingObject=p_ob;
	m_id=p_ob->GetID();
	
	// Use p_ob as the hash key so that the CTrackingInfo for any given CMovingObject
	// can be quickly looked up.
	sTrackingInfoHashTable.PutItem((uint32)m_id,this);
}	

void CTrackingInfo::SetSkaterCamera()
{
	Dbg_MsgAssert(mPointerType==UNDEFINED,("Expected mPointerType to be UNDEFINED"));
	Dbg_MsgAssert(mpMovingObject==NULL,("Expected mpMovingObject to be NULL"));
	mpMovingObject=NULL;
	
	m_id=ID_CAMERA;
	sTrackingInfoHashTable.PutItem((uint32)m_id,this);
}

void CTrackingInfo::RecordPositionAndAngleChanges(Mth::Vector &actual_pos, Mth::Vector &actual_angles, bool exact)
{
	if (exact)
	{
		sWriteSingleToken(SET_POSITION_ANGLES);
		sWriteFloat(actual_pos[X]);
		sWriteFloat(actual_pos[Y]);
		sWriteFloat(actual_pos[Z]);
		sWriteFloat(actual_angles[X]);
		sWriteFloat(actual_angles[Y]);
		sWriteFloat(actual_angles[Z]);
		return;
	}
		
	// Compare the actual position with that predicted by the last
	// position and velocity
	Mth::Vector predicted_pos=mReplayLastPos+mReplayVel;
	Mth::Vector d=actual_pos-predicted_pos;
	
	if (fabs(d[X]) > 0.1f || fabs(d[Y]) > 0.1f || fabs(d[Z]) > 0.1f)
	{
		mReplayLastPos=actual_pos;
		sWriteVector(SET_POSITION,actual_pos);

		
		mReplayVel=actual_pos-mActualLastPos;
		sWriteVector(SET_VELOCITY,mReplayVel);
	}
	else
	{
		mReplayLastPos=predicted_pos;
	}	
    mActualLastPos=actual_pos;


	// Do the angles.
	mTrackerAngleX.WriteChanges(actual_angles[X],SET_ANGLE_X,SET_ANGULAR_VELOCITY_X);
	mTrackerAngleY.WriteChanges(actual_angles[Y],SET_ANGLE_Y,SET_ANGULAR_VELOCITY_Y);
	mTrackerAngleZ.WriteChanges(actual_angles[Z],SET_ANGLE_Z,SET_ANGULAR_VELOCITY_Z);
}

static void sDeleteObjectTrackingInfo()
{
	CTrackingInfo *p_info=spTrackingInfoHead;
	while (p_info)
	{
		CTrackingInfo *p_next=p_info->mpNext;
		delete p_info;
		p_info=p_next;
	}	
}

static Lst::HashTable<CDummy> *sGetDummyHashTable(EDummyList whichList)
{
	if (whichList==START_STATE_DUMMY_LIST)
	{
		return &sObjectStartStateHashTable;
	}
	else
	{
		return &sReplayDummysHashTable;
	}
}
		
CDummy *GetFirstStartStateDummy()
{
	return spStartStateDummies;
}
	
static CDummy *sGetDummyListHeadPointer(EDummyList whichList)
{
	if (whichList==START_STATE_DUMMY_LIST)
	{
		return spStartStateDummies;
	}
	else	
	{
		return spReplayDummies;
	}
}

static void sSetDummyListHeadPointer(EDummyList whichList, CDummy *p_dummy)
{
	if (whichList==START_STATE_DUMMY_LIST)
	{
		spStartStateDummies=p_dummy;
	}
	else
	{
		spReplayDummies=p_dummy;
	}
}
		
static int sCountStartStateDummies()
{
	int n=0;
	CDummy *p_dummy=spStartStateDummies;
	while (p_dummy)
	{
		++n;
		p_dummy=p_dummy->mpNext;
	}
	return n;	
}

#ifdef CHECK_TOKEN_USAGE
static const char *sGetTokenName(EReplayToken token)
{
	switch (token)
	{
	case BLANK                               :return "BLANK"; break;
	case FRAME_START                         :return "FRAME_START"; break;                       
	case CREATE_OBJECT                       :return "CREATE_OBJECT"; break;                     
	case KILL_OBJECT                         :return "KILL_OBJECT"; break;                       
	case OBJECT_ID                           :return "OBJECT_ID"; break;                         
	case MODEL_NAME                          :return "MODEL_NAME"; break;                        
	case SKELETON_NAME                       :return "SKELETON_NAME"; break;                     
	case PROFILE_NAME                        :return "PROFILE_NAME"; break;                      
	case SECTOR_NAME                         :return "SECTOR_NAME"; break;                       
	case ANIM_SCRIPT_NAME                    :return "ANIM_SCRIPT_NAME"; break;                  
	case SET_POSITION                        :return "SET_POSITION"; break;                      
	case SET_POSITION_ANGLES	             :return "SET_POSITION_ANGLES"; break;               
	case SET_VELOCITY                        :return "SET_VELOCITY"; break;                      
	case SET_ANGLE_X                         :return "SET_ANGLE_X"; break;                       
	case SET_ANGLE_Y                         :return "SET_ANGLE_Y"; break;                       
	case SET_ANGLE_Z                         :return "SET_ANGLE_Z"; break;                       
	case SET_ANGULAR_VELOCITY_X              :return "SET_ANGULAR_VELOCITY_X"; break;            
	case SET_ANGULAR_VELOCITY_Y              :return "SET_ANGULAR_VELOCITY_Y"; break;            
	case SET_ANGULAR_VELOCITY_Z              :return "SET_ANGULAR_VELOCITY_Z"; break;            
	case PRIMARY_ANIM_CONTROLLER_CHANGES     :return "PRIMARY_ANIM_CONTROLLER_CHANGES"; break;   
	case DEGENERATE_ANIM_CONTROLLER_CHANGES  :return "DEGENERATE_ANIM_CONTROLLER_CHANGES"; break;
	case PLAY_POSITIONAL_SOUND_EFFECT        :return "PLAY_POSITIONAL_SOUND_EFFECT"; break;      
	case PLAY_SKATER_SOUND_EFFECT            :return "PLAY_SKATER_SOUND_EFFECT"; break;          
	case PLAY_SOUND							 :return "PLAY_SOUND"; break;
	case PLAY_LOOPING_SOUND					 :return "PLAY_LOOPING_SOUND"; break;
	case STOP_LOOPING_SOUND					 :return "STOP_LOOPING_SOUND"; break;
	case PITCH_MIN							 :return "PITCH_MIN"; break;
	case PITCH_MAX							 :return "PITCH_MAX"; break;
	case SCREEN_BLUR                         :return "SCREEN_BLUR"; break;                       
	case SCREEN_FLASH						 :return "SCREEN_FLASH"; break;
	case SPARKS_ON                           :return "SPARKS_ON"; break;                         
	case SPARKS_OFF                          :return "SPARKS_OFF"; break;                        
	case TRICK_TEXT                          :return "TRICK_TEXT"; break;                        
	case TRICK_TEXT_PULSE                    :return "TRICK_TEXT_PULSE"; break;                  
	case TRICK_TEXT_COUNTDOWN                :return "TRICK_TEXT_COUNTDOWN"; break;              
	case TRICK_TEXT_LANDED                   :return "TRICK_TEXT_LANDED"; break;                 
	case TRICK_TEXT_BAIL                     :return "TRICK_TEXT_BAIL"; break;                   
	case SCORE_POT_TEXT                      :return "SCORE_POT_TEXT"; break;                    
	case PANEL_MESSAGE                       :return "PANEL_MESSAGE"; break;                     
	case SET_ATOMIC_STATES                   :return "SET_ATOMIC_STATES"; break;                  
	case MANUAL_METER_ON                     :return "MANUAL_METER_ON"; break;                   
	case MANUAL_METER_OFF                    :return "MANUAL_METER_OFF"; break;                  
	case BALANCE_METER_ON                    :return "BALANCE_METER_ON"; break;                  
	case BALANCE_METER_OFF                   :return "BALANCE_METER_OFF"; break;                 
	case FLIP                                :return "FLIP"; break;                              
	case UNFLIP                              :return "UNFLIP"; break;                            
	case MODEL_ACTIVE                        :return "MODEL_ACTIVE"; break;                              
	case MODEL_INACTIVE                      :return "MODEL_INACTIVE"; break;                              
	case PAD_VIBRATION                       :return "PAD_VIBRATION"; break;                     
	case PLAY_POSITIONAL_STREAM              :return "PLAY_POSITIONAL_STREAM"; break;            
	case PLAY_STREAM                         :return "PLAY_STREAM"; break;                       
	case STOP_STREAM                         :return "STOP_STREAM"; break;                       
	case SET_CAR_ROTATION_X                  :return "SET_CAR_ROTATION_X"; break;                
	case SET_CAR_ROTATION_X_VEL              :return "SET_CAR_ROTATION_X_VEL"; break;            
	case SET_WHEEL_ROTATION_X                :return "SET_WHEEL_ROTATION_X"; break;              
	case SET_WHEEL_ROTATION_X_VEL            :return "SET_WHEEL_ROTATION_X_VEL"; break;          
	case SET_WHEEL_ROTATION_Y                :return "SET_WHEEL_ROTATION_Y"; break;              
	case SET_WHEEL_ROTATION_Y_VEL            :return "SET_WHEEL_ROTATION_Y_VEL"; break;          
	case HOVERING							 :return "HOVERING"; break;          
	case SECTOR_ACTIVE						 :return "SECTOR_ACTIVE"; break;          
	case SECTOR_INACTIVE					 :return "SECTOR_INACTIVE"; break;          
	case SECTOR_VISIBLE						 :return "SECTOR_VISIBLE"; break;          
	case SECTOR_INVISIBLE					 :return "SECTOR_INVISIBLE"; break;          
	case SHATTER_PARAMS					 	 :return "SHATTER_PARAMS"; break;          
	case SHATTER_ON					 		 :return "SHATTER_ON"; break;          
	case SHATTER_OFF				 		 :return "SHATTER_OFF"; break;          
	case TEXTURE_SPLAT						 :return "TEXTURE_SPLAT"; break;
	case SCRIPT_CALL						 :return "SCRIPT_CALL"; break;
	case SKATER_SCRIPT_CALL					 :return "SKATER_SCRIPT_CALL"; break;
	case PAUSE_SKATER						 :return "PAUSE_SKATER"; break;
	case UNPAUSE_SKATER						 :return "UNPAUSE_SKATER"; break;
	default									 :return "UNKNOWN"; break;
	}
}
#endif

// When the game is in paused mode, which it will be when running a replay, certain screen
// elements such as the balance meters will not update properly.
// This function will unpause them so that they work during the replay.
static void sUnpauseCertainScreenElements()
{
	Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
	Front::CSpriteElement *p_balance_meter = (Front::CSpriteElement *) p_manager->GetElement(0xa4db8a4b + sGetSkater()->GetHeapIndex()).Convert(); // "the_balance_meter"
	Dbg_MsgAssert(p_balance_meter,("NULL p_balance_meter"));
	Front::CSpriteElement *p_balance_arrow = (Front::CSpriteElement *) p_balance_meter->GetFirstChild().Convert();
	
	p_balance_meter->SetMorphPausedState(false);
	p_balance_arrow->SetMorphPausedState(false);
}

static void sSwitchOffVibrations()
{
	Obj::CSkater *p_skater=sGetSkater();
	for (int i=0; i<Obj::CVibrationComponent::vVB_NUM_ACTUATORS; ++i)
	{
		p_skater->GetDevice()->ActivateActuator(i,0);
	}	
}

static Obj::CSkater *sGetSkater()
{
	Mdl::Skate * p_skate_mod = Mdl::Skate::Instance();
	Obj::CSkater *p_skater = p_skate_mod->GetSkater(0);
	Dbg_MsgAssert(p_skater,("NULL p_skater"));
	return p_skater;
}
	
static Mdl::Score *sGetSkaterScoreObject()
{
	Mdl::Score *p_score=sGetSkater()->GetScoreObject();
	Dbg_MsgAssert(p_score,("NULL p_score"));
	return p_score;
}

static void sDeleteDummies(EDummyList whichList)
{
	CDummy *p_dummy=sGetDummyListHeadPointer(whichList);
	while (p_dummy)
	{
		CDummy *p_next=p_dummy->mpNext;
		delete p_dummy;
		p_dummy=p_next;
	}	
	sGetDummyHashTable(whichList)->FlushAllItems();
	Dbg_MsgAssert(spReplayDummies==NULL,("Hey! spReplayDummies not NULL !!!"));
}

bool ScriptDeleteDummies( Script::CStruct *pParams, Script::CScript *pScript )
{
	sDeleteDummies(PLAYBACK_DUMMY_LIST);
	return true;
}
	
CDummy::CDummy(EDummyList whichList, uint32 id)
{
	m_list=whichList;
	SetID(id);

	mpNext=sGetDummyListHeadPointer(m_list);
	mpPrevious=NULL;
	if (mpNext)
	{
		mpNext->mpPrevious=this;
	}	
	sSetDummyListHeadPointer(m_list,this);
	sGetDummyHashTable(whichList)->PutItem(m_id,this);
	
	//m_type = SKATE_TYPE_REPLAY_DUMMY;

	mAtomicStates=0xffffffff;

	mFlags=0;
	
	// Add a bit of randomness so that groups of hovering things slowly
	// go out of phase with each other rather than hovering in unison.
	mHoverPeriod=HOVER_PERIOD+Mth::Rnd(2*HOVER_PERIOD_RND+1)-HOVER_PERIOD_RND;
	Dbg_MsgAssert(mHoverPeriod>0,("Bad mHoverPeriod"));
	
	m_car_rotation_x=0.0f;
	m_car_rotation_x_vel=0.0f;
	m_wheel_rotation_x=0.0f;
	m_wheel_rotation_x_vel=0.0f;
	m_wheel_rotation_y=0.0f;
	m_wheel_rotation_y_vel=0.0f;

	mAnimScriptName=0;
	mSectorName=0;
	mScale=1.0f;

	m_looping_sound_id=0;
	m_looping_sound_checksum=0;
	m_pitch_min=50.0f;
	m_pitch_max=120.0f;
			
	m_is_displayed=true;
				
	// Note: CDummy is derived from CMovingObject which is derived from Spt::Class, so we
	// get the autozeroing of the members.
}

CDummy::~CDummy()
{
	if (mp_rendered_model && m_id!=0)
	{
		// Must call sUninitModel rather than just delete it.
		Nx::CEngine::sUninitModel(mp_rendered_model);
    }

    if (mp_skeletonComponent)
    {
        delete mp_skeletonComponent;
		mp_skeletonComponent = NULL;
    }
	
	if (mpPrevious) mpPrevious->mpNext=mpNext;
	if (mpNext) mpNext->mpPrevious=mpPrevious;
	if (mpPrevious==NULL)
	{
		sSetDummyListHeadPointer(m_list,mpNext);
	}	
	
	sGetDummyHashTable(m_list)->FlushItem(m_id);
	
	if (mp_skater_camera)
	{
		delete mp_skater_camera;
//		Obj::CSkater *p_skater=sGetSkater();

		// Replay has finished, so we'd want to switch the camera back to the skater camera
		// can't do it the old way, need to use
		
		printf ("STUBBED:  replay.cpp, line %d  -------- not resetting skater camera\n",__LINE__);											
		//Nx::CViewportManager::sSetCamera( /*m_skater_number*/0, p_skater->GetSkaterCam() );			
	}	
}

CDummy& CDummy::operator=( const CDummy& rhs )
{
#if 0
	Dbg_MsgAssert(strlen(rhs.mpModelName)<=MAX_MODEL_NAME_CHARS,("rhs mpModelName too long ?"));
	strcpy(mpModelName,rhs.mpModelName);
	mSkeletonName=rhs.mSkeletonName;
	mProfileName=rhs.mProfileName;
	mAnimScriptName=rhs.mAnimScriptName;
	mSectorName=rhs.mSectorName;
	mScale=rhs.mScale;
	
	mFlags=rhs.mFlags;
	
	mHoverPeriod=rhs.mHoverPeriod;
	
	// Don't really need to copy m_id and m_pos here, because they are members of
	// CMovingObject so will get copied by the CMovingObject default assignement operator, I think.
	// I feel more comfortable doing it here too though.
	m_id=rhs.m_id;
	m_pos=rhs.m_pos;
	m_type=rhs.m_type;
	
	m_vel=rhs.m_vel;
	m_angles=rhs.m_angles;
	m_ang_vel=rhs.m_ang_vel;

	// This is because for the skater dummy the speed for calculating the looping sound
	// volume is calculated using m_old_pos. If it were left equal to 0,0,0 it would cause the
	// looping sound to be played at max volume for the first frame, because it would think
	// the skater was moving very fast.
	m_old_pos=m_pos;

	m_car_rotation_x		=rhs.m_car_rotation_x;
	m_car_rotation_x_vel    =rhs.m_car_rotation_x_vel;
	m_wheel_rotation_x      =rhs.m_wheel_rotation_x;
	m_wheel_rotation_x_vel  =rhs.m_wheel_rotation_x_vel;
	m_wheel_rotation_y      =rhs.m_wheel_rotation_y;
	m_wheel_rotation_y_vel  =rhs.m_wheel_rotation_y_vel;
	
	m_primaryController=rhs.m_primaryController;
	
	for (int i=0; i<NUM_DEGENERATE_ANIMS; ++i)
	{
		m_degenerateControllers[i]=rhs.m_degenerateControllers[i];
	}
		
	mp_rendered_model=NULL;
	mp_skater_camera=NULL;
	
	mAtomicStates=rhs.mAtomicStates;

	// Note: m_looping_sound_id is not copied, since this is a unique id for the instance of the sound.
	m_looping_sound_checksum=rhs.m_looping_sound_checksum;
	m_pitch_min=rhs.m_pitch_min;
	m_pitch_max=rhs.m_pitch_max;
#endif
	
	// CDummy::m_list is not copied, and similarly not mpNext or mpPrevious because the
	// new CDummy may want to be in a different list. Copying mpNext and mpPrevious would
	// tangle up the lists anyway.
	return *this;
}

void CDummy::Save(SSavedDummy *p_saved_dummy)
{
#if 0
	Dbg_MsgAssert(p_saved_dummy,("NULL p_saved_dummy"));

	Dbg_MsgAssert(strlen(mpModelName)<=MAX_MODEL_NAME_CHARS,("mpModelName too long ?"));
	strcpy(p_saved_dummy->mpModelName,mpModelName);
	
	p_saved_dummy->m_id				=	m_id;
	p_saved_dummy->m_type			=	m_type;
	p_saved_dummy->mSkeletonName	=	mSkeletonName;
	p_saved_dummy->mProfileName		=	mProfileName;
	p_saved_dummy->mAnimScriptName	=	mAnimScriptName;
	p_saved_dummy->mSectorName		=	mSectorName;
	p_saved_dummy->mScale			=	mScale;
	p_saved_dummy->mFlags			=	mFlags;
	// Note: The mHoverPeriod is not saved out. No need, it does not need to be restored exactly,
	// and the CDummy constructor generates a new random value for it.
	p_saved_dummy->m_pos			=	m_pos;
	p_saved_dummy->m_vel			=	m_vel;
	p_saved_dummy->m_angles			=	m_angles;
	p_saved_dummy->m_ang_vel		=	m_ang_vel;
	p_saved_dummy->m_car_rotation_x       = m_car_rotation_x;      
	p_saved_dummy->m_car_rotation_x_vel   = m_car_rotation_x_vel;  
	p_saved_dummy->m_wheel_rotation_x     = m_wheel_rotation_x;    
	p_saved_dummy->m_wheel_rotation_x_vel = m_wheel_rotation_x_vel;
	p_saved_dummy->m_wheel_rotation_y     = m_wheel_rotation_y;    
	p_saved_dummy->m_wheel_rotation_y_vel = m_wheel_rotation_y_vel;
	
	p_saved_dummy->m_primaryController		=	m_primaryController;
	
	for (int i=0; i<NUM_DEGENERATE_ANIMS; ++i)
	{
		p_saved_dummy->m_degenerateControllers[i]=m_degenerateControllers[i];
	}
	
	p_saved_dummy->mAtomicStates		= mAtomicStates;

	// Note: m_looping_sound_id is not saved, since this is a unique id for the instance of the sound.
	p_saved_dummy->m_looping_sound_checksum = m_looping_sound_checksum;
	p_saved_dummy->m_pitch_min = m_pitch_min;
	p_saved_dummy->m_pitch_max = m_pitch_max;
#endif
}

void CDummy::UpdateLoopingSound()
{
	if (!m_looping_sound_checksum)
	{
		return;
	}
		
	float percent_of_max_speed=100.0f;
	float maxSpeed = 1100.0f;
	Mth::Vector v=m_pos-m_old_pos;
	v[Y]=0.0f;
	float speed = v.Length( ) * 60.0f;
	if ( fabsf( speed ) < maxSpeed )
	{
		percent_of_max_speed=( 100.0f * speed ) / maxSpeed;
	}

	Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
	
	if (!m_looping_sound_id)
	{
		m_looping_sound_id = sfx_manager->PlaySound( m_looping_sound_checksum, 0, 0 );
	}
		
	Sfx::sVolume vol;
	sfx_manager->SetVolumeFromPos( &vol, m_pos, sfx_manager->GetDropoffDist( m_looping_sound_checksum ) );
	
	vol.PercentageAdjustment( percent_of_max_speed );

	float pitch = PERCENT( m_pitch_max - m_pitch_min, percent_of_max_speed );
	pitch += m_pitch_min;
	
	sfx_manager->UpdateLoopingSound(m_looping_sound_id,&vol,pitch);
}		

void CDummy::Update()
{
#if 0
	if (mFlags&DUMMY_FLAG_HOVERING)
	{
		m_angles[Y]+=DEGREES_TO_RADIANS(HOVER_ROTATE_SPEED)/60.0f;
	}
	else
	{
		m_angles+=m_ang_vel;
		m_pos+=m_vel;
	
		if (m_type==SKATE_TYPE_CAR)
		{
			m_car_rotation_x	+= m_car_rotation_x_vel; 
			m_wheel_rotation_x	+= m_wheel_rotation_x_vel;
			m_wheel_rotation_y	+= m_wheel_rotation_y_vel;
		}
	
		if (m_list==PLAYBACK_DUMMY_LIST)
		{
			Mth::Matrix mat(m_angles[X],m_angles[Y],m_angles[Z]);
			Obj::CSkater *p_skater=sGetSkater();
			
			if (m_id==ID_SKATER)
			{
				p_skater->UpdateShadow(m_pos,mat);
				
				// Update the skater's pos and display matrix so that the sparks follow the dummy.
				// The skater's RestoreAfterReplayPlayback function will restore the skater's
				// original position when the replay ends.
				p_skater->SetActualDisplayMatrix(mat);
				p_skater->m_pos=m_pos;
				
				UpdateLoopingSound();
			}	
			else if (m_id==ID_CAMERA)
			{
				if (!mp_skater_camera)
				{
					mp_skater_camera=new Gfx::Camera;
					
					//mp_skater_camera=new Obj::CSkaterCam(0);
					//mp_skater_camera->SetMode( Obj::CSkaterCam::SKATERCAM_MODE_NORMAL_MEDIUM, 0.0f );
					//mp_skater_camera->SetSkater(p_skater);
					
					Nx::CViewportManager::sSetCamera( /*m_skater_number*/0, mp_skater_camera );			
				}		
				
				//mp_skater_camera->Update();
				m_pos[W]=1.0f;
				mp_skater_camera->SetPos(m_pos);
				mp_skater_camera->SetMatrix(mat);
			}
		}
	}	
	
	m_old_pos=m_pos;
#endif
}

void CDummy::CreateModel()
{
#if 0
	Dbg_MsgAssert(mp_rendered_model==NULL,("Called CreateModel() when model already exists"));

	if (m_id==ID_SKATER)
	{
		Obj::CSkater *p_skater=sGetSkater();
		mp_rendered_model=p_skater->GetModel();
		// This switches off any bouncing boobs on Jenna initially.
		Dbg_Assert( p_skater->GetSkeleton() );
		p_skater->GetSkeleton()->SetProceduralBoneTransActive( 0x47c76c69/*breast_cloth_zz*/, 0 );
	}
	else
	{
		if (mpModelName[0]==0 && mProfileName==0 && mSectorName==0)
		{
			m_is_displayed=false;
			return;
		}
	
		mp_rendered_model = Nx::CEngine::sInitModel();
		Dbg_MsgAssert(mp_rendered_model,("sInitModel() returned NULL"));
		Mth::Vector scale(mScale,mScale,mScale);
		mp_rendered_model->SetScale(scale);

		switch (m_type)
		{
			case SKATE_TYPE_CAR:
				if (mSkeletonName)
				{
					this->LoadSkeleton(mSkeletonName);
				}
				if (mpModelName[0])
				{
					mp_rendered_model->AddGeom(mpModelName, 0, true );
				}		
				break;
			case SKATE_TYPE_PED:	
				if (mSkeletonName)
				{
					this->LoadSkeleton(mSkeletonName);
					
					if (mProfileName)
					{
						Gfx::CModelAppearance thePedAppearance;
						thePedAppearance.Load( mProfileName );
						
						Gfx::CModelBuilder theBuilder( true, 0 );
						theBuilder.BuildModel( &thePedAppearance, mp_rendered_model );
					}
					else
					{
						Str::String fullModelName;
						fullModelName = Gfx::GetModelFileName(mpModelName, ".skin");
					
						mp_rendered_model->AddGeom(fullModelName.getString(), 0, true );
					}
				}
				break;
			case SKATE_TYPE_GAME_OBJ:	
			case SKATE_TYPE_BOUNCY_OBJ:	
				if (mSectorName)
				{
					Nx::CSector *p_sector = Nx::CEngine::sGetSector(mSectorName);
					
					if ( p_sector )
					{
						// need to clone the source, not the instance?
						Nx::CGeom* pGeom = p_sector->GetGeom();
						if( pGeom )
						{
							Nx::CGeom* pClonedGeom = pGeom->Clone( true );
							pClonedGeom->SetActive(true);
							mp_rendered_model->AddGeom( pClonedGeom, 0 );
						}
					}	
				}
				else if (mSkeletonName)
				{
					this->LoadSkeleton(mSkeletonName);
					
					Ass::CAssMan*	ass_manager = Ass::CAssMan::Instance();
					ass_manager->SetReferenceChecksum( mAnimScriptName );
					Script::RunScript( mAnimScriptName );
					
					Str::String fullModelName;
					fullModelName = Gfx::GetModelFileName(mpModelName, ".skin");
				
					mp_rendered_model->AddGeom(fullModelName.getString(), 0, true );
				}
				else if (mpModelName[0])
				{
					Str::String fullModelName;
					fullModelName = Gfx::GetModelFileName(mpModelName, ".mdl");
					
					mp_rendered_model->AddGeom(fullModelName.getString(), 0, true );
				}				
				break;
			default:
				break;
		}		
	}	
	
	// Now initialise the atomic states according to the states stored in the dummy.
	Dbg_MsgAssert(mp_rendered_model,("NULL mp_rendered_model"));
	mp_rendered_model->SetGeomActiveMask(mAtomicStates);

	mp_rendered_model->SetActive(mFlags & DUMMY_FLAG_ACTIVE);
	
	// And initialise the flipped status
	Gfx::CSkeleton* p_skeleton=this->GetSkeleton();
	if (m_id==ID_SKATER)
	{
		Obj::CSkater *p_skater=sGetSkater();
		p_skeleton = p_skater->GetSkeleton();
	}
#endif

#if 0
	if (p_skeleton)
	{
		p_skeleton->FlipAnimation( 0, mFlags&DUMMY_FLAG_FLIPPED, 0, false );
	}
#endif
}

void CDummy::DisplayModel()
{
	if (mp_rendered_model && mp_rendered_model->GetActive())
	{
		Mth::Matrix display_matrix(m_angles[X],m_angles[Y],m_angles[Z]);
		display_matrix[Mth::POS] = m_pos;
		display_matrix[Mth::POS][W] = 1.0f;

		bool should_animate=true;
		if (!mSkeletonName)
		{
			should_animate=false;
		}	
		
		switch (m_type)
		{
			case SKATE_TYPE_CAR:
			{
				if (!mp_rendered_model->GetHierarchy())
				{
					break;
				}	
				// This updates the rotating wheels.
				Obj::CalculateCarHierarchyMatrices( GetSkeleton(),
                                                    mp_rendered_model,
													m_car_rotation_x,
													m_wheel_rotation_x,
													m_wheel_rotation_y);
				break;
			}	
			case SKATE_TYPE_PED:
			case SKATE_TYPE_SKATER:
			case SKATE_TYPE_GAME_OBJ:
				if (should_animate)
				{
//                    if ( mp_skeletonComponent )
//                    {
//                        mp_skeletonComponent->SetNeutralPose( mAnimScriptName+0x1ca1ff20/*default*/ );
//                    }
					
#if 0
					int degeneratingCount = 0;
					for ( int i = 0; i < NUM_DEGENERATE_ANIMS; i++ )
					{
						if ( m_degenerateControllers[i].GetStatus() != Gfx::ANIM_STATUS_INACTIVE )
						{
							degeneratingCount++;
						}
					}
					
					if ( degeneratingCount )
					{
						// animation has blending...
						
						uint32 degenerate_anim_name0,degenerate_anim_name1,degenerate_anim_name2;
						float degenerate_anim_time0,degenerate_anim_time1,degenerate_anim_time2;
						float degenerate_anim_blend_value0,degenerate_anim_blend_value1,degenerate_anim_blend_value2;
						
						m_degenerateControllers[0].GetNameTimeAndBlendValue(&degenerate_anim_name0,&degenerate_anim_time0,&degenerate_anim_blend_value0);
						m_degenerateControllers[1].GetNameTimeAndBlendValue(&degenerate_anim_name1,&degenerate_anim_time1,&degenerate_anim_blend_value1);
						m_degenerateControllers[2].GetNameTimeAndBlendValue(&degenerate_anim_name2,&degenerate_anim_time2,&degenerate_anim_blend_value2);
						
						if (degenerate_anim_name0) degenerate_anim_name0+=mAnimScriptName;
						if (degenerate_anim_name1) degenerate_anim_name1+=mAnimScriptName;
						if (degenerate_anim_name2) degenerate_anim_name2+=mAnimScriptName;
						
						#if 0
						// disabled replays of skeletons until the transition is complete
                        if ( mp_skeletonComponent )
                        {
                            mp_skeletonComponent->Update( 
                                mAnimScriptName+m_primaryController.GetAnimName(), m_primaryController.GetCurrentAnimTime(),
                                degenerate_anim_name0,degenerate_anim_time0,degenerate_anim_blend_value0,
                                degenerate_anim_name1,degenerate_anim_time1,degenerate_anim_blend_value1,
                                degenerate_anim_name2,degenerate_anim_time2,degenerate_anim_blend_value2 );                        }
						#endif
					}
					else
					{
						#if 0
						// disabled replays of skeletons until the transition is complete
						if ( mp_skeletonComponent )
                        {
                            mp_skeletonComponent->Update( mAnimScriptName+m_primaryController.GetAnimName(),
								m_primaryController.GetCurrentAnimTime() );
                        }
						#endif
					}
#endif
				
				}	
				break;
			default:
				break;
		}		


		if (mFlags&DUMMY_FLAG_HOVERING)
		{
			uint32 t=Tmr::ElapsedTime(0)%mHoverPeriod;
			display_matrix[Mth::POS][Y]+=HOVER_AMPLITUDE*sinf(t*2*3.141592653f/mHoverPeriod);
		}
		
		mp_rendered_model->Render(&display_matrix,!should_animate,GetSkeleton());
	}	
}

void CDummy::UpdateMutedSounds()
{
	GetSoundComponent()->UpdateMutedSounds();
}	

static void sStopReplayDummyLoopingSounds()
{
	Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
	
	CDummy *p_dummy=spReplayDummies;
	while (p_dummy)
	{
		if (p_dummy->m_looping_sound_id)
		{
			sfx_manager->StopSound( p_dummy->m_looping_sound_id );
			p_dummy->m_looping_sound_id=0;
		}	
		
		p_dummy=p_dummy->mpNext;
	}
}
		
void ClearBuffer()
{
	if (BufferAllocated())
	{
		FillBuffer(0,GetBufferSize(),BLANK);
	}	
	sBigBufferStartOffset=0;
	sBigBufferEndOffset=0;
	sBufferFilling=true;
}

static uint8 spBufferChunk[REPLAY_BUFFER_CHUNK_SIZE];
uint8 *GetTempBuffer()
{
	return spBufferChunk;
}
	
void WriteReplayDataHeader(SReplayDataHeader *p_header)
{
	Dbg_MsgAssert(p_header,("NULL p_header"));
	p_header->mBufferStartOffset=sBigBufferStartOffset;
	p_header->mBufferEndOffset=sBigBufferEndOffset;
	p_header->mNumStartStateDummies=sCountStartStateDummies();
	p_header->mStartState=sStartState;
}

void ReadReplayDataHeader(const SReplayDataHeader *p_header)
{
	Dbg_MsgAssert(p_header,("NULL p_header"));
	sBigBufferStartOffset=p_header->mBufferStartOffset;
	sBigBufferEndOffset=p_header->mBufferEndOffset;
	sStartState=p_header->mStartState;
	Nx::CEngine::sReadSectorStatusBitfield(sStartState.mpSectorStatus,SECTOR_STATUS_BUFFER_SIZE);
}
	
void CreateDummyFromSaveData(SSavedDummy *p_saved_dummy)
{
#if 0
	Dbg_MsgAssert(p_saved_dummy,("NULL p_saved_dummy"));
	
	CDummy *p_dummy=new CDummy(START_STATE_DUMMY_LIST,p_saved_dummy->m_id);
	p_dummy->SetType(p_saved_dummy->m_type);
	
	Dbg_MsgAssert(strlen(p_saved_dummy->mpModelName)<=MAX_MODEL_NAME_CHARS,("p_saved_dummy mpModelName too long ?"));
	strcpy(p_dummy->mpModelName,p_saved_dummy->mpModelName);
	
	p_dummy->mSkeletonName	=	p_saved_dummy->mSkeletonName;
	p_dummy->mProfileName	=	p_saved_dummy->mProfileName;
	p_dummy->mAnimScriptName=	p_saved_dummy->mAnimScriptName;
	p_dummy->mSectorName	=	p_saved_dummy->mSectorName;
	p_dummy->mScale			=	p_saved_dummy->mScale;
	p_dummy->mFlags			=	p_saved_dummy->mFlags;
	// Note: The mHoverPeriod is not restored. No need, it does not need to be restored exactly,
	// and the CDummy constructor will have generated a new random value for it.
	p_dummy->m_pos			=	p_saved_dummy->m_pos;
	p_dummy->m_vel			=	p_saved_dummy->m_vel;
	p_dummy->m_angles		=	p_saved_dummy->m_angles;
	p_dummy->m_ang_vel		=	p_saved_dummy->m_ang_vel;
	p_dummy->m_car_rotation_x       = p_saved_dummy->m_car_rotation_x;      
	p_dummy->m_car_rotation_x_vel   = p_saved_dummy->m_car_rotation_x_vel;  
	p_dummy->m_wheel_rotation_x     = p_saved_dummy->m_wheel_rotation_x;    
	p_dummy->m_wheel_rotation_x_vel = p_saved_dummy->m_wheel_rotation_x_vel;
	p_dummy->m_wheel_rotation_y     = p_saved_dummy->m_wheel_rotation_y;    
	p_dummy->m_wheel_rotation_y_vel = p_saved_dummy->m_wheel_rotation_y_vel;
	
	p_dummy->m_primaryController	=	p_saved_dummy->m_primaryController;
	
	for (int i=0; i<NUM_DEGENERATE_ANIMS; ++i)
	{
		p_dummy->m_degenerateControllers[i]=p_saved_dummy->m_degenerateControllers[i];
	}
	
	p_dummy->mAtomicStates			= p_saved_dummy->mAtomicStates;

	// The looping sound id is not copied in, since it is a unique id for the instance of the sound.
	p_dummy->m_looping_sound_id=0;
	p_dummy->m_looping_sound_checksum = p_saved_dummy->m_looping_sound_checksum;
	p_dummy->m_pitch_min = p_saved_dummy->m_pitch_min;
	p_dummy->m_pitch_max = p_saved_dummy->m_pitch_max;
	
	// This is because for the skater dummy the speed for calculating the looping sound
	// volume is calculated using m_old_pos. If it were left equal to 0,0,0 it would cause the
	// looping sound to be played at max volume for the first frame, because it would think
	// the skater was moving very fast.
	p_dummy->m_old_pos=p_dummy->m_pos;
#endif
}

static bool sPoolsCreated=false;
//char p_foo[sizeof(CTrackingInfo)/0];
void CreatePools()
{
	if (!sPoolsCreated)
	{
		// MEMOPT: Make this pool smaller
		CTrackingInfo::SCreatePool(300, "CTrackingInfo");
		sPoolsCreated=true;
	}	
}

void RemovePools()
{
	if (sPoolsCreated)
	{
		CTrackingInfo::SRemovePool();
		sPoolsCreated=false;
	}	
}

bool ScriptReplayRecordSimpleScriptCall(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Do nothing if not in record mode.
	if (sMode!=RECORD)
	{
		return true;
	}
	
	uint32 script_name=0;
	pParams->GetChecksum("scriptname",&script_name);
	
	if (pParams->ContainsFlag("skaterscript"))
	{
		sWriteUint32(SKATER_SCRIPT_CALL,script_name);
	}
	else
	{
		sWriteUint32(SCRIPT_CALL,script_name);
	}
	return true;
}

static uint32 sLastRecordedPanelMessageID=0;
bool ScriptRecordPanelMessage(Script::CStruct *pParams, Script::CScript *pScript)
{
	if (sMode!=RECORD)
	{
		return true;
	}

	// Remember what the last panel message id was so that it can be killed before
	// replaying the replay.
	//sLastRecordedPanelMessageID=0;
	//pParams->GetChecksum("id",&sLastRecordedPanelMessageID);
	
	Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
	sLastRecordedPanelMessageID = p_manager->ResolveComplexID(pParams, "id");
	
	
	
	uint8 p_buf[MAX_PANEL_MESSAGE_SIZE];
	int size=Script::WriteToBuffer(pParams,p_buf,MAX_PANEL_MESSAGE_SIZE,Script::NO_ASSERT);
	if (!size)
	{
		Script::PrintContents(pParams);
		Dbg_MsgAssert(0,("Panel message structure too big"));
		return true;
	}	
	
	sWriteUint32(PANEL_MESSAGE,size);
	
	Dbg_MsgAssert(spFrameBufferPos+size <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	memcpy(spFrameBufferPos,p_buf,size);
	spFrameBufferPos+=size;
	
	return true;
}
	
// Call this to allocate the resources needed to store a replay.
// May be called repeatedly.
bool ScriptAllocateReplayMemory(Script::CStruct *pParams, Script::CScript *pScript)
{
	return true;
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	AllocateBuffer();
	ClearBuffer();
	CreatePools();
	Mem::Manager::sHandle().PopContext();
	sMode=RECORD;	
	
	CTrackingInfo::sSkaterTrackingInfo.Reset();
	Nx::CEngine::sInitReplayStartState();
	return true;
}

void DeallocateReplayMemory()
{
	sDeleteDummies(START_STATE_DUMMY_LIST);
	sDeleteDummies(PLAYBACK_DUMMY_LIST);
	Dbg_MsgAssert(spReplayDummies==NULL,("Hey! spReplayDummies not NULL !!!"));
	sDeleteObjectTrackingInfo();
	ClearBuffer();
	DeallocateBuffer();
	RemovePools();
	sStartState.Reset();
	sCurrentState.Reset();
	sMode=NONE;
}

// Call this to free up the resources used to store a replay.
// May be called repeatedly.
bool ScriptDeallocateReplayMemory(Script::CStruct *pParams, Script::CScript *pScript)
{
	DeallocateReplayMemory();
	return true;
}	

// If in record mode, this will clear what has been recorded and start afresh.
void StartRecordingAfresh()
{
	Dbg_MsgAssert(!Mdl::Skate::Instance()->IsMultiplayerGame(),("Tried to start recording replay in multiplayer game"));
	sMode=RECORD;

	sDeleteDummies(START_STATE_DUMMY_LIST);
	sDeleteDummies(PLAYBACK_DUMMY_LIST);
	Dbg_MsgAssert(spReplayDummies==NULL,("Hey! spReplayDummies not NULL !!!"));
	sDeleteObjectTrackingInfo();
	ClearBuffer();
	sStartState.Reset();
	sCurrentState.Reset();
	CTrackingInfo::sSkaterTrackingInfo.Reset();
	Nx::CEngine::sInitReplayStartState();
}

bool ScriptStartRecordingAfresh(Script::CStruct *pParams, Script::CScript *pScript)
{
	if (Mdl::Skate::Instance()->IsMultiplayerGame())
	{
		return false;
	}	
	StartRecordingAfresh();
	return true;
}	

static uint32 sReadAnimControllerChanges(uint32 offset, Gfx::CAnimChannel *p_anim_controller)
{
	#define MAX_CHANGES 18
	uint8 p_buf[5*MAX_CHANGES];
	
	uint8 num_changes;
	ReadFromBuffer(&num_changes,offset,1);
	++offset;
	Dbg_MsgAssert(num_changes<=MAX_CHANGES,("Too many anim controller changes in one instruction"));
	
	ReadFromBuffer(p_buf,offset,5*num_changes);
	offset+=5*num_changes;
	
	uint32 *p_dest=(uint32*)p_anim_controller;
	uint8 *p_foo=p_buf;
	for (int i=0; i<num_changes; ++i)
	{
		uint8 off=*p_foo++;
		Dbg_MsgAssert(off<sizeof(Gfx::CAnimChannel)/4,("Bad offset of %d in anim controller change instruction",off));
		p_dest[off]=Script::Read4Bytes(p_foo).mUInt;
		p_foo+=4;
	}

	return offset;
}


#define TRICK_TEXT_BUF_SIZE 5000
static char spTrickText[TRICK_TEXT_BUF_SIZE];
// TODO: This is duplicated in score.cpp, move it to a header ...
const int TRICK_LIMIT = 250;

#ifdef CHECK_TOKEN_USAGE
static int sTokenCount[NUM_REPLAY_TOKEN_TYPES];
#endif

// If the offset points to a frame, ie data that starts with the FRAME_START token, it will
// skip over it and return the new offset.
// If nothing follows the frame, it will set the contents of p_nothingFollows to true, false otherwise.
// Nothing follows the frame if either it is followed by a BLANK token, or if the offset following
// it is the end of the big buffer.
//
// If offset does not point to a FRAME_START token, then the returned offset will be unchanged.
static uint32 sReadFrame(uint32 offset, bool *p_nothingFollows, EDummyList whichList)
{
	Dbg_MsgAssert(p_nothingFollows,("NULL p_nothingFollows"));
	Dbg_MsgAssert(offset<GetBufferSize(),("Bad offset sent to sReadFrame"));

	#ifdef CHECK_TOKEN_USAGE
	for (int i=0; i<NUM_REPLAY_TOKEN_TYPES; ++i)
	{
		sTokenCount[i]=0;
	}	
	#endif


	uint8 token;
	ReadFromBuffer(&token,offset,1);
	if (token!=FRAME_START)
	{
		*p_nothingFollows=true;
		return offset;
	}	
	offset+=5;


	Obj::CSkater *p_skater=sGetSkater();
	
	Lst::HashTable<CDummy> *p_dummy_table=sGetDummyHashTable(whichList);
	*p_nothingFollows=false;
	CDummy *p_dummy=NULL;
	bool end=false;
	while (!end)
	{
		if (offset==GetBufferSize())
		{
			*p_nothingFollows=true;
			break;
		}	
		Dbg_MsgAssert(offset<GetBufferSize(),("sReadFrame went past the end of the buffer"));
		
		ReadFromBuffer(&token,offset,1);
		
		#ifdef CHECK_TOKEN_USAGE
		Dbg_MsgAssert(token<NUM_REPLAY_TOKEN_TYPES,("Bad token"));
		++sTokenCount[token];
		#endif
		
		switch (token)
		{
			case BLANK:
				*p_nothingFollows=true;
				end=true;
				break;
				
			case FRAME_START:	
				end=true;
				break;

			case SCREEN_BLUR:
			{
				// Must not do blurs if recording
				if (sMode==RECORD)
				{
					offset+=1+4;
					break;
				}
				
				++offset;
				int amount;
				ReadFromBuffer((uint8*)&amount,offset,4);
				amount=Script::Read4Bytes((uint8*)&amount).mInt;
				offset+=4;
				
				Nx::CEngine::sSetScreenBlur(amount);
				break;
			}

			case SCREEN_FLASH:
			{
				// Must not do flashes if recording
				if (sMode==RECORD)
				{
					offset+=1+4*6;
					break;
				}
				
				++offset;
				
				int viewport;
				ReadFromBuffer((uint8*)&viewport,offset,4);
				viewport=Script::Read4Bytes((uint8*)&viewport).mInt;
				offset+=4;

				Image::RGBA from;
				ReadFromBuffer((uint8*)&from,offset,4);
				offset+=4;
				
				Image::RGBA to;
				ReadFromBuffer((uint8*)&to,offset,4);
				offset+=4;

				float duration;
				ReadFromBuffer((uint8*)&duration,offset,4);
				duration=Script::Read4Bytes((uint8*)&duration).mFloat;
				offset+=4;
				
				float z;
				ReadFromBuffer((uint8*)&z,offset,4);
				z=Script::Read4Bytes((uint8*)&z).mFloat;
				offset+=4;
				
				uint32 flags;
				ReadFromBuffer((uint8*)&flags,offset,4);
				flags=Script::Read4Bytes((uint8*)&flags).mUInt;
				offset+=4;
				
				flags|=Nx::SCREEN_FLASH_FLAG_IGNORE_PAUSE;
				
				Nx::AddScreenFlash(viewport,from,to,duration,z,flags,NULL);
				break;
			}

			case SHATTER_PARAMS:
			{
				// Must not do shatters if recording
				if (sMode==RECORD)
				{
					offset+=1+4*9;
					break;
				}
				
				++offset;
				
				Mth::Vector velocity;
				ReadFromBuffer((uint8*)&velocity[X],offset,4*3);
				velocity[X]=Script::Read4Bytes((uint8*)&velocity[X]).mFloat;
				velocity[Y]=Script::Read4Bytes((uint8*)&velocity[Y]).mFloat;
				velocity[Z]=Script::Read4Bytes((uint8*)&velocity[Z]).mFloat;
				offset+=4*3;
				
				float area_test, velocity_variance, spread_factor, lifetime;
				float bounce, bounce_amplitude;
				
				ReadFromBuffer((uint8*)&area_test,offset,4);
				area_test=Script::Read4Bytes((uint8*)&area_test).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&velocity_variance,offset,4);
				velocity_variance=Script::Read4Bytes((uint8*)&velocity_variance).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&spread_factor,offset,4);
				spread_factor=Script::Read4Bytes((uint8*)&spread_factor).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&lifetime,offset,4);
				lifetime=Script::Read4Bytes((uint8*)&lifetime).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&bounce,offset,4);
				bounce=Script::Read4Bytes((uint8*)&bounce).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&bounce_amplitude,offset,4);
				bounce_amplitude=Script::Read4Bytes((uint8*)&bounce_amplitude).mFloat;
				offset+=4;
				
				Nx::ShatterSetParams( velocity, area_test, velocity_variance, spread_factor, lifetime, bounce, bounce_amplitude );
				break;
			}

			case TEXTURE_SPLAT:
			{
				++offset;
				
				Mth::Vector splat_start;
				ReadFromBuffer((uint8*)&splat_start[X],offset,4*3);
				splat_start[X]=Script::Read4Bytes((uint8*)&splat_start[X]).mFloat;
				splat_start[Y]=Script::Read4Bytes((uint8*)&splat_start[Y]).mFloat;
				splat_start[Z]=Script::Read4Bytes((uint8*)&splat_start[Z]).mFloat;
				offset+=4*3;

				Mth::Vector splat_end;
				ReadFromBuffer((uint8*)&splat_end[X],offset,4*3);
				splat_end[X]=Script::Read4Bytes((uint8*)&splat_end[X]).mFloat;
				splat_end[Y]=Script::Read4Bytes((uint8*)&splat_end[Y]).mFloat;
				splat_end[Z]=Script::Read4Bytes((uint8*)&splat_end[Z]).mFloat;
				offset+=4*3;

				float size;
				ReadFromBuffer((uint8*)&size,offset,4);
				size=Script::Read4Bytes((uint8*)&size).mFloat;
				offset+=4;

				float lifetime;
				ReadFromBuffer((uint8*)&lifetime,offset,4);
				lifetime=Script::Read4Bytes((uint8*)&lifetime).mFloat;
				offset+=4;
				
				uint8 len;
				ReadFromBuffer(&len,offset,1);
				++offset;
				char p_text[100];
				Dbg_MsgAssert(len<100,("Texture splat file name string too long\n"));
				ReadFromBuffer((uint8*)p_text,offset,len);
				offset+=len;
				p_text[len]=0;

				uint32 trail;
				ReadFromBuffer((uint8*)&trail,offset,4);
				trail=Script::Read4Bytes((uint8*)&trail).mUInt;
				offset+=4;

				// Must not do splats if recording
				// Still need to do the above reads due to the string that needs to be
				// skipped over.
				if (sMode!=RECORD)
				{
					Nx::TextureSplat(splat_start,splat_end,size,lifetime,p_text,trail);
				}
				
				break;
			}

			case SHATTER_ON:
			case SHATTER_OFF:
			{
				// Must not do shatters if recording
				if (sMode==RECORD)
				{
					offset+=1+4;
					break;
				}
				
				++offset;

				uint32 sector_name;
				ReadFromBuffer((uint8*)&sector_name,offset,4);
				sector_name=Script::Read4Bytes((uint8*)&sector_name).mUInt;
				offset+=4;
			
				Nx::CSector *p_sector = Nx::CEngine::sGetSector(sector_name);
				if (p_sector)
				{
					p_sector->SetShatter(token==SHATTER_ON);
				}	
			
				break;
			}

			
			case SCRIPT_CALL:	
			case SKATER_SCRIPT_CALL: // Used to record BloodParticlesOn & off
			{
				// Must not call the script if recording.
				if (sMode==RECORD)
				{
					offset+=1+4;
					break;
				}
				
				++offset;

				uint32 script_name;
				ReadFromBuffer((uint8*)&script_name,offset,4);
				script_name=Script::Read4Bytes((uint8*)&script_name).mUInt;
				offset+=4;
				
				if (token==SCRIPT_CALL)
				{
					Script::RunScript(script_name);
				}
				else
				{
					Script::RunScript( script_name, NULL, p_skater );
				}
				break;
			}
			
			case PAD_VIBRATION:
			{
				++offset;
				uint8 actuator;
				ReadFromBuffer(&actuator,offset,1);
				++offset;
				Dbg_MsgAssert(actuator<Obj::CVibrationComponent::vVB_NUM_ACTUATORS,("Bad actuator value of %d",actuator));
				
				int percent;
				ReadFromBuffer((uint8*)&percent,offset,4);
				percent=Script::Read4Bytes((uint8*)&percent).mInt;
				offset+=4;
				
				if (whichList==START_STATE_DUMMY_LIST)
				{
					sStartState.mActuatorStrength[actuator]=percent;
				}
				else
				{
					sCurrentState.mActuatorStrength[actuator]=percent;
					p_skater->GetDevice()->ActivateActuator(actuator,percent);
				}	
				break;
			}
				
			// This command is needed because when the goal cutscenes play in mid goal
			// they do a PauseSkaters command, which in-game pauses the vibrations.
			// Need to do that in the replay too, otherwise in the 4 angry seals goal in sf
			// the pad vibrates during the camera anims, cos the skater is usually in a grind then.
			case PAUSE_SKATER:	
			case UNPAUSE_SKATER:	
			{
				++offset;
				
				if (whichList==START_STATE_DUMMY_LIST)
				{
					sStartState.mSkaterPaused=(token==PAUSE_SKATER);
				}
				else
				{
					sCurrentState.mSkaterPaused=(token==PAUSE_SKATER);
					if (sCurrentState.mSkaterPaused)
					{
						// Switch off the vibrations.
						for (int i=0; i<Obj::CVibrationComponent::vVB_NUM_ACTUATORS; ++i)
						{
							p_skater->GetDevice()->ActivateActuator(i,0);
						}	
					}
					else
					{
						// Restore the vibrations.
						for (int i=0; i<Obj::CVibrationComponent::vVB_NUM_ACTUATORS; ++i)
						{
							p_skater->GetDevice()->ActivateActuator(i,sCurrentState.mActuatorStrength[i]);
						}	
					}
				}
				break;
			}
				
			case MANUAL_METER_ON:
			{
				++offset;
				float value;
				ReadFromBuffer((uint8*)&value,offset,4);
				value=Script::Read4Bytes((uint8*)&value).mFloat;
				offset+=4;
				
				if (whichList==START_STATE_DUMMY_LIST)
				{
					sStartState.mManualMeterStatus=true;
					sStartState.mManualMeterValue=value;
				}
				else
				{
					sCurrentState.mManualMeterStatus=true;
					sCurrentState.mManualMeterValue=value;
					sGetSkaterScoreObject()->SetManualMeter(true,value);
				}
				break;
			}
			
			case MANUAL_METER_OFF:
			{
				++offset;
					
				if (whichList==START_STATE_DUMMY_LIST)
				{
					sStartState.mManualMeterStatus=false;
					sStartState.mManualMeterValue=0.0f;
				}
				else
				{
					sCurrentState.mManualMeterStatus=false;
					sCurrentState.mManualMeterValue=0.0f;
					sGetSkaterScoreObject()->SetManualMeter(false);
				}
				break;
			}

			case BALANCE_METER_ON:
			{
				++offset;
				float value;
				ReadFromBuffer((uint8*)&value,offset,4);
				value=Script::Read4Bytes((uint8*)&value).mFloat;
				offset+=4;
				
				if (whichList==START_STATE_DUMMY_LIST)
				{
					sStartState.mBalanceMeterStatus=true;
					sStartState.mBalanceMeterValue=value;
				}
				else
				{
					sCurrentState.mBalanceMeterStatus=true;
					sCurrentState.mBalanceMeterValue=value;
					sGetSkaterScoreObject()->SetBalanceMeter(true,value);
				}
				break;
			}
			
			case BALANCE_METER_OFF:
			{
				++offset;
					
				if (whichList==START_STATE_DUMMY_LIST)
				{
					sStartState.mBalanceMeterStatus=false;
					sStartState.mBalanceMeterValue=0.0f;
				}
				else
				{
					sCurrentState.mBalanceMeterStatus=false;
					sCurrentState.mBalanceMeterValue=0.0f;
					sGetSkaterScoreObject()->SetBalanceMeter(false);
				}
				break;
			}
			
#if 0
			case FLIP:	
			case UNFLIP:	
			{
				++offset;
				
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				if (token==FLIP)
				{
					p_dummy->mFlags|=DUMMY_FLAG_FLIPPED;
				}
				else
				{
					p_dummy->mFlags&=~DUMMY_FLAG_FLIPPED;
				}
				
				if (whichList==PLAYBACK_DUMMY_LIST && p_dummy->mp_rendered_model)
				{
					Gfx::CSkeleton* p_skeleton=p_dummy->GetSkeleton();
					if (p_skeleton)
					{
						p_skeleton->FlipAnimation( 0, p_dummy->mFlags&DUMMY_FLAG_FLIPPED, 0, false );
					}
				}		
				break;
			}
#endif

			case MODEL_ACTIVE:	
			case MODEL_INACTIVE:	
			{
				++offset;
				
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				if (token==MODEL_ACTIVE)
				{
					p_dummy->mFlags|=DUMMY_FLAG_ACTIVE;
				}
				else
				{
					p_dummy->mFlags&=~DUMMY_FLAG_ACTIVE;
				}
				
				if (whichList==PLAYBACK_DUMMY_LIST && p_dummy->mp_rendered_model)
				{
					p_dummy->mp_rendered_model->SetActive(p_dummy->mFlags&DUMMY_FLAG_ACTIVE);
				}		
				break;
			}
				
			case SPARKS_ON:
			{
				++offset;
				// Must not do sparks if recording
				if (sMode==RECORD)
				{
					break;
				}
				
				Script::RunScript( "sparks_on", NULL, p_skater );
				break;
			}

			case SPARKS_OFF:
			{
				++offset;
				if (sMode==RECORD)
				{
					break;
				}
				
				Script::RunScript( "sparks_off", NULL, p_skater );
				break;
			}
			
			case PANEL_MESSAGE:
			{
				++offset;
				int size;
				ReadFromBuffer((uint8*)&size,offset,4);
				size=Script::Read4Bytes((uint8*)&size).mInt;
				offset+=4;
				
				uint8 p_buf[MAX_PANEL_MESSAGE_SIZE];
				Dbg_MsgAssert(size<=MAX_PANEL_MESSAGE_SIZE,("Panel message size too big in replay"));
				ReadFromBuffer(p_buf,offset,size);
				offset+=size;

				if (sMode==RECORD)
				{
					break;
				}
				
				Script::CStruct *p_struct=new Script::CStruct;
				Script::ReadFromBuffer(p_struct,p_buf);
				Script::RunScript("Create_Panel_Message",p_struct);
				delete p_struct;
				break;
			}
					
			case SCORE_POT_TEXT:
			{
				++offset;
				uint8 len;
				ReadFromBuffer(&len,offset,1);
				++offset;
				char p_text[100];
				if (len<100)
				{
					ReadFromBuffer((uint8*)p_text,offset,len);
					p_text[len]=0;
				}
				else
				{
					p_text[0]=0;
				}
				offset+=len;

				if (sMode==RECORD)
				{
					break;
				}
				
				Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
				
				int index = p_skater->GetHeapIndex();

				Front::CTextElement *p_score_pot_text = (Front::CTextElement *) p_manager->GetElement(0xf4d3a70e + index ).Convert(); // "the_score_pot_text"
				if (p_score_pot_text)
				{
					p_score_pot_text->SetText(p_text);
				}
				break;
			}
			
			case TRICK_TEXT:
			{
				++offset;
				uint8 num_strings;
				ReadFromBuffer(&num_strings,offset,1);
				++offset;

				const char *pp_text[TRICK_LIMIT];
				int num_strings_loaded=0;
				char *p_dest=spTrickText;
				for (uint8 i=0; i<num_strings; ++i)
				{
					uint8 len;
					ReadFromBuffer(&len,offset,1);
					++offset;

					// Only copy the string into the buffer if there is enough room,
					// for safety. Not asserting, in case this happens on a release build.
					if (p_dest+len+1-spTrickText < TRICK_TEXT_BUF_SIZE && num_strings_loaded<TRICK_LIMIT)
					{
						pp_text[num_strings_loaded++]=p_dest;
						ReadFromBuffer((uint8*)p_dest,offset,len);
						p_dest+=len;
						*p_dest++=0;
					}
					
					offset+=len;
				}	

				if (sMode==RECORD)
				{
					break;
				}
				else
				{
					Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
					
					int index=p_skater->GetHeapIndex();
					
					Front::CTextBlockElement *p_text_block = (Front::CTextBlockElement *) p_manager->GetElement(0x44727dae/*the_trick_text*/ + index ).Convert();
					if (p_text_block)
					{
						p_text_block->SetText(pp_text, num_strings_loaded);
					}
					sTrickTextGotCleared=false;
					break;
				}
			}

			case TRICK_TEXT_PULSE:
			case TRICK_TEXT_COUNTDOWN:
			case TRICK_TEXT_LANDED:
			case TRICK_TEXT_BAIL:
			{
				++offset;
				if (sMode==RECORD)
				{
					break;
				}
				
				if (sTrickTextGotCleared)
				{
					break;
				}
					
				int index=p_skater->GetHeapIndex();
				
				Mdl::Score *p_score=p_skater->GetScoreObject();
				
				switch (token)
				{
					case TRICK_TEXT_PULSE:
						p_score->TrickTextPulse(index);
						break;
					case TRICK_TEXT_COUNTDOWN:
						p_score->TrickTextCountdown(index);
						break;
					case TRICK_TEXT_LANDED:
						p_score->TrickTextLanded(index);
						break;
					case TRICK_TEXT_BAIL:
						p_score->TrickTextBail(index);
						break;
					default:
						break;
				}				
				
				break;
			}
			
			case SET_ATOMIC_STATES:							
			{
				++offset;
				uint32 id;
				ReadFromBuffer((uint8*)&id,offset,4);
				id=Script::Read4Bytes((uint8*)&id).mUInt;
				offset+=4;
				
				uint32 atomic_states;
				ReadFromBuffer((uint8*)&atomic_states,offset,4);
				atomic_states=Script::Read4Bytes((uint8*)&atomic_states).mUInt;
				offset+=4;

				p_dummy=p_dummy_table->GetItem(id);
				if (p_dummy)
				{
					// Record the atomic state change in the dummy too, so that the initial states
					// can be set at the start of replay playback.
					p_dummy->mAtomicStates=atomic_states;
				
					if (p_dummy->mp_rendered_model)
					{
						p_dummy->mp_rendered_model->SetGeomActiveMask(atomic_states);
					}				
				}	
				break;
			}
			
			case PLAY_STREAM:
			{
				// Must not play sounds if recording
				if (sMode==RECORD)
				{
					offset+=1+4*5;
					break;
				}
				
				++offset;
				uint32 checksum;
//				float volume_l, volume_r;
				float vol;
				float pitch;

				int priority;
				ReadFromBuffer((uint8*)&checksum,offset,4);
				checksum=Script::Read4Bytes((uint8*)&checksum).mUInt;
				offset+=4;

				uint32 volume_type;
				ReadFromBuffer((uint8*)&volume_type,offset,4);
				volume_type=Script::Read4Bytes((uint8*)&volume_type).mInt;
				offset+=4;

				Sfx::sVolume vol_struct((Sfx::EVolumeType)volume_type );

				ReadFromBuffer((uint8*)&vol,offset,4);
				vol=Script::Read4Bytes((uint8*)&vol).mFloat;
				offset+=4;
				vol_struct.SetChannelVolume( 0, vol );

				ReadFromBuffer((uint8*)&vol,offset,4);
				vol=Script::Read4Bytes((uint8*)&vol).mFloat;
				offset+=4;
				vol_struct.SetChannelVolume( 1, vol );

				// Read channels 2, 3 and 4 if a 5 channel sound type.
				if( vol_struct.GetVolumeType() == Sfx::VOLUME_TYPE_5_CHANNEL_DOLBY5_1 )
				{
					ReadFromBuffer((uint8*)&vol,offset,4);
					vol=Script::Read4Bytes((uint8*)&vol).mFloat;
					offset+=4;
					vol_struct.SetChannelVolume( 2, vol );

					ReadFromBuffer((uint8*)&vol,offset,4);
					vol=Script::Read4Bytes((uint8*)&vol).mFloat;
					offset+=4;
					vol_struct.SetChannelVolume( 3, vol );

					ReadFromBuffer((uint8*)&vol,offset,4);
					vol=Script::Read4Bytes((uint8*)&vol).mFloat;
					offset+=4;
					vol_struct.SetChannelVolume( 4, vol );
				}

				ReadFromBuffer((uint8*)&pitch,offset,4);
				pitch=Script::Read4Bytes((uint8*)&pitch).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&priority,offset,4);
				priority=Script::Read4Bytes((uint8*)&priority).mInt;
				offset+=4;

				Pcm::PlayStream(checksum,&vol_struct,pitch,priority);
				break;
			}

			case STOP_STREAM:
			{
				// Must not stop streams if recording
				if (sMode==RECORD)
				{
					offset+=1+4;
					break;
				}
				
				++offset;
				int channel;
				ReadFromBuffer((uint8*)&channel,offset,4);
				channel=Script::Read4Bytes((uint8*)&channel).mInt;
				offset+=4;
				
				Pcm::StopStreams(channel);
				break;
			}
						
			case PLAY_POSITIONAL_STREAM:
			{
				// Must not play sounds if recording
				if (sMode==RECORD)
				{
					offset+=1+4*7;
					break;
				}
				
				++offset;
				uint32 id, stream_name;
				float volume, pitch, drop_off;
				int priority, use_pos_info;
				ReadFromBuffer((uint8*)&id,offset,4);
				id=Script::Read4Bytes((uint8*)&id).mUInt;
				offset+=4;
				ReadFromBuffer((uint8*)&stream_name,offset,4);
				stream_name=Script::Read4Bytes((uint8*)&stream_name).mUInt;
				offset+=4;
				ReadFromBuffer((uint8*)&drop_off,offset,4);
				drop_off=Script::Read4Bytes((uint8*)&drop_off).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&volume,offset,4);
				volume=Script::Read4Bytes((uint8*)&volume).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&pitch,offset,4);
				pitch=Script::Read4Bytes((uint8*)&pitch).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&priority,offset,4);
				priority=Script::Read4Bytes((uint8*)&priority).mInt;
				offset+=4;
				ReadFromBuffer((uint8*)&use_pos_info,offset,4);
				use_pos_info=Script::Read4Bytes((uint8*)&use_pos_info).mInt;
				offset+=4;

				p_dummy=p_dummy_table->GetItem(id);
				if (p_dummy)
				{
					printf ("STUBBED replay.cpp %d\n",__LINE__);
//					Pcm::PlayStreamFromObject(p_dummy,stream_name,drop_off,volume,pitch,channel,use_pos_info);
				}
				
				break;
			}
			
			case PLAY_POSITIONAL_SOUND_EFFECT:				
			{
				// Must not play sounds if recording
				if (sMode==RECORD)
				{
					offset+=1+4*5;
					break;
				}
				
				++offset;
				uint32 id, sound_name;
				float volume, pitch, drop_off_dist;
				ReadFromBuffer((uint8*)&id,offset,4);
				id=Script::Read4Bytes((uint8*)&id).mUInt;
				offset+=4;
				ReadFromBuffer((uint8*)&sound_name,offset,4);
				sound_name=Script::Read4Bytes((uint8*)&sound_name).mUInt;
				offset+=4;
				ReadFromBuffer((uint8*)&volume,offset,4);
				volume=Script::Read4Bytes((uint8*)&volume).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&pitch,offset,4);
				pitch=Script::Read4Bytes((uint8*)&pitch).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&drop_off_dist,offset,4);
				drop_off_dist=Script::Read4Bytes((uint8*)&drop_off_dist).mFloat;
				offset+=4;

				p_dummy=p_dummy_table->GetItem(id);
				if (p_dummy)
				{
					Sfx::CSfxManager * p_sfx_manager = Sfx::CSfxManager::Instance();
					Sfx::SoundUpdateInfo soundUpdateInfo;
					soundUpdateInfo.volume = volume;
					soundUpdateInfo.pitch = pitch;
					soundUpdateInfo.dropoffDist = drop_off_dist;

					// These next two parameters need to be added to the replay code
					soundUpdateInfo.dropoffFunction = DROPOFF_FUNC_STANDARD;
					bool noPosUpdate = false;
					p_sfx_manager->PlaySoundWithPos( sound_name, &soundUpdateInfo, GetSoundComponentFromObject( p_dummy ), noPosUpdate );
				}
				
				break;
			}
				
			case PLAY_SKATER_SOUND_EFFECT:				
			{
				// Must not play sounds if recording
				if (sMode==RECORD)
				{
					offset+=1+4*6;
					break;
				}
				
				++offset;
				
				int which_array, surface_flag;
				Mth::Vector pos;
				float vol_percent;
				
				ReadFromBuffer((uint8*)&which_array,offset,4);
				which_array=Script::Read4Bytes((uint8*)&which_array).mInt;
				offset+=4;
				ReadFromBuffer((uint8*)&surface_flag,offset,4);
				surface_flag=Script::Read4Bytes((uint8*)&surface_flag).mInt;
				offset+=4;
				ReadFromBuffer((uint8*)&pos[X],offset,4);
				pos[X]=Script::Read4Bytes((uint8*)&pos[X]).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&pos[Y],offset,4);
				pos[Y]=Script::Read4Bytes((uint8*)&pos[Y]).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&pos[Z],offset,4);
				pos[Z]=Script::Read4Bytes((uint8*)&pos[Z]).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&vol_percent,offset,4);
				vol_percent=Script::Read4Bytes((uint8*)&vol_percent).mFloat;
				offset+=4;

				// NOTE: need to record sound pitch and choice for use here
				Env::CTerrainManager::sPlaySound((Env::ETerrainActionType) which_array, (ETerrainType) surface_flag,pos,vol_percent,100.0f,0.0f,false);
				break;
			}
				
			case PLAY_SOUND:
			{
				// Must not play sounds if recording
				if (sMode==RECORD)
				{
					offset+=1+4*4;
					break;
				}
				
				++offset;
				
				uint32 checksum;
				ReadFromBuffer((uint8*)&checksum,offset,4);
				checksum=Script::Read4Bytes((uint8*)&checksum).mUInt;
				offset+=4;
				
				float vol_l,vol_r;
				ReadFromBuffer((uint8*)&vol_l,offset,4);
				vol_l=Script::Read4Bytes((uint8*)&vol_l).mFloat;
				offset+=4;
				ReadFromBuffer((uint8*)&vol_r,offset,4);
				vol_r=Script::Read4Bytes((uint8*)&vol_r).mFloat;
				offset+=4;

				float pitch;
				ReadFromBuffer((uint8*)&pitch,offset,4);
				pitch=Script::Read4Bytes((uint8*)&pitch).mFloat;
				offset+=4;
				
				Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();

				// Dave note 10/17/02, the replay code should store all 5 channels where appropriate.
				Sfx::sVolume vol;
				vol.SetSilent();
				vol.SetChannelVolume( 0, vol_l );
				vol.SetChannelVolume( 1, vol_r );
//				sfx_manager->PlaySound(checksum,vol_l,vol_r,pitch);
				sfx_manager->PlaySound(checksum,&vol,pitch);
				break;
			}
			
			case PLAY_LOOPING_SOUND:
			{
				++offset;
				
				uint32 checksum;
				ReadFromBuffer((uint8*)&checksum,offset,4);
				checksum=Script::Read4Bytes((uint8*)&checksum).mUInt;
				offset+=4;

				p_dummy->m_looping_sound_checksum=checksum;
					
				if (whichList==PLAYBACK_DUMMY_LIST)
				{
					Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
					
					Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
					if (p_dummy->m_looping_sound_id)
					{
						sfx_manager->StopSound( p_dummy->m_looping_sound_id );
					}	
					p_dummy->m_looping_sound_id = sfx_manager->PlaySound( checksum, 0, 0 );
				}
				else
				{
					// This bit will execute if the list is the start-state dummy list.
					// m_looping_sound_id isn't used in that case, but may as well set it to zero.
					p_dummy->m_looping_sound_id=0;
				}	
					
				break;
			}

			case STOP_LOOPING_SOUND:
			{
				++offset;
				
				if (whichList==PLAYBACK_DUMMY_LIST)
				{
					Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
					
					Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
					if (p_dummy->m_looping_sound_id)
					{
						sfx_manager->StopSound( p_dummy->m_looping_sound_id );
					}	
				}	
				
				p_dummy->m_looping_sound_id=0;
				p_dummy->m_looping_sound_checksum=0;
				break;
			}

			case PITCH_MIN:
			case PITCH_MAX:
			{
				++offset;
				
				float pitch;
				ReadFromBuffer((uint8*)&pitch,offset,4);
				pitch=Script::Read4Bytes((uint8*)&pitch).mFloat;
				offset+=4;

				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				if (token==PITCH_MIN)
				{
					p_dummy->m_pitch_min=pitch;
				}
				else
				{
					p_dummy->m_pitch_max=pitch;
				}
				break;
			}
				
			case OBJECT_ID:
			{
				++offset;
				uint32 id;
				ReadFromBuffer((uint8*)&id,offset,4);
				id=Script::Read4Bytes((uint8*)&id).mUInt;
				
				p_dummy=p_dummy_table->GetItem(id);
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?  id=%d",id));
				
				offset+=4;
				break;
			}

			case CREATE_OBJECT:
			{
				++offset;
				uint32 id;
				uint8 type;
				ReadFromBuffer((uint8*)&id,offset,4);
				id=Script::Read4Bytes((uint8*)&id).mUInt;
				offset+=4;
				ReadFromBuffer(&type,offset,1);
				++offset;
				
				p_dummy=p_dummy_table->GetItem(id);
				if (!p_dummy)
				{
					p_dummy=new CDummy(whichList,id);
					p_dummy->SetType(type);
				}
				else
				{
					Dbg_MsgAssert(0,("Got CREATE_OBJECT when dummy already exists ?"));
				}	
				
				break;
			}
			
			case KILL_OBJECT:
			{
				++offset;
				uint32 id;
				ReadFromBuffer((uint8*)&id,offset,4);
				id=Script::Read4Bytes((uint8*)&id).mUInt;
				
				p_dummy=p_dummy_table->GetItem(id);
				if (p_dummy)
				{
					delete p_dummy;
					p_dummy=NULL;
				}	
				
				offset+=4;
				break;
			}
			
#if 0
			case MODEL_NAME:
			{
				++offset;
				uint8 len;
				ReadFromBuffer(&len,offset,1);
				++offset;
				
				Dbg_MsgAssert(len<=MAX_MODEL_NAME_CHARS,("Too many chars in model name"));
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)p_dummy->mpModelName,offset,len);
				// The string has no terminating 0 in the big buffer, so wack one on.
				p_dummy->mpModelName[len]=0;
				offset+=len;
				break;
			}
#endif
			case SKELETON_NAME:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->mSkeletonName,offset,4);
				p_dummy->mSkeletonName=Script::Read4Bytes((uint8*)&p_dummy->mSkeletonName).mUInt;
				offset+=4;
				break;
			}
			case PROFILE_NAME:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->mProfileName,offset,4);
				p_dummy->mProfileName=Script::Read4Bytes((uint8*)&p_dummy->mProfileName).mUInt;
				offset+=4;
				break;
			}
			case SECTOR_NAME:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->mSectorName,offset,4);
				p_dummy->mSectorName=Script::Read4Bytes((uint8*)&p_dummy->mSectorName).mUInt;
				offset+=4;
				break;
			}
			case ANIM_SCRIPT_NAME:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->mAnimScriptName,offset,4);
				p_dummy->mAnimScriptName=Script::Read4Bytes((uint8*)&p_dummy->mAnimScriptName).mUInt;
				offset+=4;
				break;
			}
			case SET_SCALE:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->mScale,offset,4);
				p_dummy->mScale=Script::Read4Bytes((uint8*)&p_dummy->mScale).mFloat;
				if (p_dummy->mp_rendered_model)
				{
					Mth::Vector s(p_dummy->mScale,p_dummy->mScale,p_dummy->mScale);
					p_dummy->mp_rendered_model->SetScale(s);
				}	
				offset+=4;
				break;
			}	
			case SET_CAR_ROTATION_X:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_car_rotation_x,offset,4);
				p_dummy->m_car_rotation_x=Script::Read4Bytes((uint8*)&p_dummy->m_car_rotation_x).mFloat;
				p_dummy->m_car_rotation_x_vel=0.0f;
				offset+=4;
				break;
			}	
			case SET_WHEEL_ROTATION_X:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_wheel_rotation_x,offset,4);
				p_dummy->m_wheel_rotation_x=Script::Read4Bytes((uint8*)&p_dummy->m_wheel_rotation_x).mFloat;
				p_dummy->m_wheel_rotation_x_vel=0.0f;
				offset+=4;
				break;
			}	
			case SET_WHEEL_ROTATION_Y:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_wheel_rotation_y,offset,4);
				p_dummy->m_wheel_rotation_y=Script::Read4Bytes((uint8*)&p_dummy->m_wheel_rotation_y).mFloat;
				p_dummy->m_wheel_rotation_y_vel=0.0f;
				offset+=4;
				break;
			}	
			case SET_CAR_ROTATION_X_VEL:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_car_rotation_x_vel,offset,4);
				p_dummy->m_car_rotation_x_vel=Script::Read4Bytes((uint8*)&p_dummy->m_car_rotation_x_vel).mFloat;
				offset+=4;
				break;
			}	
			case SET_WHEEL_ROTATION_X_VEL:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_wheel_rotation_x_vel,offset,4);
				p_dummy->m_wheel_rotation_x_vel=Script::Read4Bytes((uint8*)&p_dummy->m_wheel_rotation_x_vel).mFloat;
				offset+=4;
				break;
			}	
			case SET_WHEEL_ROTATION_Y_VEL:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_wheel_rotation_y_vel,offset,4);
				p_dummy->m_wheel_rotation_y_vel=Script::Read4Bytes((uint8*)&p_dummy->m_wheel_rotation_y_vel).mFloat;
				offset+=4;
				break;
			}	
			
			case SET_ANGLE_X:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_angles[X],offset,4);
				p_dummy->m_angles[X]=Script::Read4Bytes((uint8*)&p_dummy->m_angles[X]).mFloat;
				p_dummy->m_ang_vel[X]=0.0f;
				offset+=4;
				break;
			}	
			case SET_ANGLE_Y:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_angles[Y],offset,4);
				p_dummy->m_angles[Y]=Script::Read4Bytes((uint8*)&p_dummy->m_angles[Y]).mFloat;
				p_dummy->m_ang_vel[Y]=0.0f;
				offset+=4;
				break;
			}	
			case SET_ANGLE_Z:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_angles[Z],offset,4);
				p_dummy->m_angles[Z]=Script::Read4Bytes((uint8*)&p_dummy->m_angles[Z]).mFloat;
				p_dummy->m_ang_vel[Z]=0.0f;
				offset+=4;
				break;
			}	
			case SET_ANGULAR_VELOCITY_X:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_ang_vel[X],offset,4);
				p_dummy->m_ang_vel[X]=Script::Read4Bytes((uint8*)&p_dummy->m_ang_vel[X]).mFloat;
				offset+=4;
				break;
			}	
			case SET_ANGULAR_VELOCITY_Y:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_ang_vel[Y],offset,4);
				p_dummy->m_ang_vel[Y]=Script::Read4Bytes((uint8*)&p_dummy->m_ang_vel[Y]).mFloat;
				offset+=4;
				break;
			}	
			case SET_ANGULAR_VELOCITY_Z:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_ang_vel[Z],offset,4);
				p_dummy->m_ang_vel[Z]=Script::Read4Bytes((uint8*)&p_dummy->m_ang_vel[Z]).mFloat;
				offset+=4;
				break;
			}	
				
			case SET_POSITION:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_pos[X],offset,4*3);
				p_dummy->m_pos[X]=Script::Read4Bytes((uint8*)&p_dummy->m_pos[X]).mFloat;
				p_dummy->m_pos[Y]=Script::Read4Bytes((uint8*)&p_dummy->m_pos[Y]).mFloat;
				p_dummy->m_pos[Z]=Script::Read4Bytes((uint8*)&p_dummy->m_pos[Z]).mFloat;
				p_dummy->m_vel.Set();
				
				offset+=4*3;
				break;
			}	
			
			case SET_POSITION_ANGLES:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				
				ReadFromBuffer((uint8*)&p_dummy->m_pos[X],offset,4*3);
				p_dummy->m_pos[X]=Script::Read4Bytes((uint8*)&p_dummy->m_pos[X]).mFloat;
				p_dummy->m_pos[Y]=Script::Read4Bytes((uint8*)&p_dummy->m_pos[Y]).mFloat;
				p_dummy->m_pos[Z]=Script::Read4Bytes((uint8*)&p_dummy->m_pos[Z]).mFloat;
				offset+=4*3;

				ReadFromBuffer((uint8*)&p_dummy->m_angles[X],offset,4*3);
				p_dummy->m_angles[X]=Script::Read4Bytes((uint8*)&p_dummy->m_angles[X]).mFloat;
				p_dummy->m_angles[Y]=Script::Read4Bytes((uint8*)&p_dummy->m_angles[Y]).mFloat;
				p_dummy->m_angles[Z]=Script::Read4Bytes((uint8*)&p_dummy->m_angles[Z]).mFloat;
				offset+=4*3;
				
				p_dummy->m_vel.Set();
				p_dummy->m_ang_vel.Set();
				break;
			}	
			
			case SET_VELOCITY:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				ReadFromBuffer((uint8*)&p_dummy->m_vel[X],offset,4*3);
				p_dummy->m_vel[X]=Script::Read4Bytes((uint8*)&p_dummy->m_vel[X]).mFloat;
				p_dummy->m_vel[Y]=Script::Read4Bytes((uint8*)&p_dummy->m_vel[Y]).mFloat;
				p_dummy->m_vel[Z]=Script::Read4Bytes((uint8*)&p_dummy->m_vel[Z]).mFloat;
				offset+=4*3;
				break;
			}	
			case PRIMARY_ANIM_CONTROLLER_CHANGES:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				offset=sReadAnimControllerChanges(offset,&p_dummy->m_primaryController);
				break;
			}	
			case DEGENERATE_ANIM_CONTROLLER_CHANGES:
			{
				++offset;
				uint8 index=0;
				ReadFromBuffer(&index,offset,1);
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				Dbg_MsgAssert(p_dummy->GetID()==ID_SKATER,("Got degenerate anim changes for a non-skater ?"));
				offset=sReadAnimControllerChanges(offset,&p_dummy->m_degenerateControllers[index]);
				break;
			}	
			case HOVERING:
			{
				++offset;
				Dbg_MsgAssert(p_dummy,("NULL p_dummy ?"));
				p_dummy->mFlags|=DUMMY_FLAG_HOVERING;
				
				ReadFromBuffer((uint8*)&p_dummy->m_pos[X],offset,4*3);
				p_dummy->m_pos[X]=Script::Read4Bytes((uint8*)&p_dummy->m_pos[X]).mFloat;
				p_dummy->m_pos[Y]=Script::Read4Bytes((uint8*)&p_dummy->m_pos[Y]).mFloat;
				p_dummy->m_pos[Z]=Script::Read4Bytes((uint8*)&p_dummy->m_pos[Z]).mFloat;
				offset+=4*3;
				
				break;
			}	
			
			case SECTOR_ACTIVE:
			case SECTOR_INACTIVE:
			{
				++offset;
				uint32 sector_name;
				ReadFromBuffer((uint8*)&sector_name,offset,4);
				sector_name=Script::Read4Bytes((uint8*)&sector_name).mUInt;
				offset+=4;
				
				Nx::CSector *p_sector = Nx::CEngine::sGetSector(sector_name);
				if (p_sector)
				{
					if (sMode==RECORD)
					{
						// If recording, apply to the start state.
						p_sector->SetActiveAtReplayStart(token==SECTOR_ACTIVE);
					}
					else
					{
						// Otherwise apply to the current world.
						p_sector->SetActive(token==SECTOR_ACTIVE);
					}
				}
				break;
			}	

			case SECTOR_VISIBLE:
			case SECTOR_INVISIBLE:
			{
				++offset;
				uint32 sector_name;
				ReadFromBuffer((uint8*)&sector_name,offset,4);
				sector_name=Script::Read4Bytes((uint8*)&sector_name).mUInt;
				offset+=4;
				
				Nx::CSector *p_sector = Nx::CEngine::sGetSector(sector_name);
				if (p_sector)
				{
					if (sMode==RECORD)
					{
						// If recording, apply to the start state.
						p_sector->SetVisibleAtReplayStart(token==SECTOR_VISIBLE);
					}
					else
					{
						// Otherwise apply to the current world.
						p_sector->SetVisibility(token==SECTOR_VISIBLE ?0xff:0x00);
					}
				}
				break;
			}	
			
			default:
				Dbg_MsgAssert(0,("Unsupported token type of %d in sReadFrame",token));
				break;
		}
	}	

	// Update all the dummy objects
	p_dummy=sGetDummyListHeadPointer(whichList);
	while (p_dummy)
	{
		if (whichList==PLAYBACK_DUMMY_LIST && !p_dummy->mp_rendered_model && p_dummy->m_is_displayed)
		{
			p_dummy->CreateModel();
		}
		
		p_dummy->Update();		
		p_dummy=p_dummy->mpNext;
	}	



	#ifdef CHECK_TOKEN_USAGE
	sTokenCount[OBJECT_ID]=0;
	int biggest=0;
	int worst_token=-1;
	for (int i=0; i<NUM_REPLAY_TOKEN_TYPES; ++i)
	{
		if (sTokenCount[i]>biggest)
		{
			biggest=sTokenCount[i];
			worst_token=i;
		}	
	}	
	printf("Worst token = %s\n",sGetTokenName((EReplayToken)worst_token));
	#endif




	return offset;
}

// After calling this, sBigBufferEndOffset will be guaranteed to point to enough contiguous space
// to hold frameSize bytes.
static void sMakeEnoughSpace(uint32 frameSize)
{
	// This loop makes one modification to the buffer each time around, and breaks out once
	// enough space has been freed.
	while (true)
	{
		if (sBigBufferStartOffset < sBigBufferEndOffset)
		{
			Dbg_MsgAssert(sBigBufferStartOffset==0,("sBigBufferStartOffset not zero ?"));
			
			// We know that all the space from sBigBufferEndOffset to the end of the buffer
			// is free to use, so check if that space is big enough.
			if (GetBufferSize()-sBigBufferEndOffset >= frameSize)
			{
				// It is!
				break;
			}
			else
			{
				// Wrap the end offset around to the start of the buffer, so that next time
				// around this loop frames at the start will start getting chomped up to make space.
				sBigBufferEndOffset=0;
				
				sBufferFilling=false;
			}	
		}
		else if (sBigBufferStartOffset > sBigBufferEndOffset)
		{
			// The space between end and start is free to use, so check if it is big enough. 
			if (sBigBufferStartOffset-sBigBufferEndOffset >= frameSize)
			{
				// It is!
				break;
			}
			else
			{
				// Skip the start offset over the frame it is pointing to.
				bool nothing_follows=false;
				uint32 old_start=sBigBufferStartOffset;
				sBigBufferStartOffset=sReadFrame(sBigBufferStartOffset,&nothing_follows,START_STATE_DUMMY_LIST);
				if (sBigBufferStartOffset==old_start)
				{
					// The start offset did not move, meaning it is pointing to empty space.
					// The only time this should happen is when the buffer is empty, in which
					// case the start and end offset should both be zero. So assert.
					Dbg_MsgAssert(0,("Start offset points to empty space, even though it is greater than end offset ?"));
					
					// Just in case this does ever happen on a release build, reset the buffer so that
					// it does not hang. All that will happen is that the replay will be lost.
					sDeleteDummies(START_STATE_DUMMY_LIST);
					sDeleteDummies(PLAYBACK_DUMMY_LIST);
					Dbg_MsgAssert(spReplayDummies==NULL,("Hey! spReplayDummies not NULL !!!"));
					sDeleteObjectTrackingInfo();
					ClearBuffer();
				}
				else
				{
					// Fill the frame just skipped over with BLANK's
					FillBuffer(old_start,sBigBufferStartOffset-old_start,BLANK);
					// Wrap around if necessary.
					if (nothing_follows)
					{
						sBigBufferStartOffset=0;
					}	
				}	
			}
		}
		else
		{
			// sBigBufferStartOffset equals sBigBufferEndOffset
			bool nothing_follows=false;
			uint32 old_start=sBigBufferStartOffset;
			sBigBufferStartOffset=sReadFrame(sBigBufferStartOffset,&nothing_follows,START_STATE_DUMMY_LIST);
			if (sBigBufferStartOffset==old_start)
			{
				// If the start offset did not move, it must be pointing to empty space.
				// The only time the start offset should point to empty space is when the 
				// buffer is totally empty, in which case the start offset would be expected to be zero.
				// So check that.
				Dbg_MsgAssert(sBigBufferStartOffset==0,("Expected sBigBufferStartOffset to be zero"));
				// Check that the frame size is not bigger than the entire buffer.
				Dbg_MsgAssert(GetBufferSize() >= frameSize,("Frame size %d too big for replay buffer",frameSize));
				// There is enough space, so break out.
				break;
			}
			else
			{
				// Fill the frame just skipped over with BLANK's
				FillBuffer(old_start,sBigBufferStartOffset-old_start,BLANK);
				// Wrap around if necessary.
				if (nothing_follows)
				{
					sBigBufferStartOffset=0;
				}	
			}	
		}
	}	
}

static void sWriteFrameBufferToBigBuffer()
{
	uint32 frame_size=spFrameBufferPos-spFrameBuffer;

	sMakeEnoughSpace(frame_size);

	// sBigBufferEndOffset is now guaranteed to point to enough contiguous space to
	// hold frame_size bytes, so write in the frame data.
	WriteIntoBuffer(spFrameBuffer,sBigBufferEndOffset,frame_size);
	sBigBufferEndOffset+=frame_size;
	// Wrap around if necessary.
	if (sBigBufferEndOffset==GetBufferSize())
	{
		sBigBufferEndOffset=0;
	}	
}

static void sWriteSingleToken(EReplayToken token)
{
	Dbg_MsgAssert(!Mdl::Skate::Instance()->IsMultiplayerGame(),("Replay Active in Multiplayer game!"));
	Dbg_MsgAssert(spFrameBufferPos+1 <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	
	*spFrameBufferPos++=token;
}

/*
static void sWriteUint8(EReplayToken token, uint8 v)
{
	Dbg_MsgAssert(spFrameBufferPos+2 <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	
	*spFrameBufferPos++=token;
	*spFrameBufferPos++=v;
}
*/

static void sWriteUint8(uint8 v)
{
	Dbg_MsgAssert(spFrameBufferPos+1 <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	*spFrameBufferPos++=v;
}

static void sWriteUint32(EReplayToken token, uint32 v)
{
	Dbg_MsgAssert(spFrameBufferPos+5 <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	
	*spFrameBufferPos++=token;
	spFrameBufferPos=Script::Write4Bytes(spFrameBufferPos, v);
}

static void sWriteUint32(uint32 v)
{
	Dbg_MsgAssert(spFrameBufferPos+4 <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	
	spFrameBufferPos=Script::Write4Bytes(spFrameBufferPos, v);
}

static void sWriteFloat(EReplayToken token, float f)
{
	Dbg_MsgAssert(spFrameBufferPos+5 <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	
	*spFrameBufferPos++=token;
	spFrameBufferPos=Script::Write4Bytes(spFrameBufferPos, f);
}

static void sWriteFloat(float f)
{
	Dbg_MsgAssert(spFrameBufferPos+4 <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	
	spFrameBufferPos=Script::Write4Bytes(spFrameBufferPos, f);
}

/*
static void sWrite2Floats(EReplayToken token, float a, float b)
{
	Dbg_MsgAssert(spFrameBufferPos+9 <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	
	*spFrameBufferPos++=token;
	spFrameBufferPos=Script::Write4Bytes(spFrameBufferPos, a);
	spFrameBufferPos=Script::Write4Bytes(spFrameBufferPos, b);
}
*/

static void sWriteVector(EReplayToken token, Mth::Vector &v)
{
	Dbg_MsgAssert(spFrameBufferPos+13 <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	
	*spFrameBufferPos++=token;
	spFrameBufferPos=Script::Write4Bytes(spFrameBufferPos, v[X]);
	spFrameBufferPos=Script::Write4Bytes(spFrameBufferPos, v[Y]);
	spFrameBufferPos=Script::Write4Bytes(spFrameBufferPos, v[Z]);
}

#if 0
static void sWriteString(EReplayToken token, const char *p_string)
{
	Dbg_MsgAssert(p_string,("NULL p_string"));
	int len=strlen(p_string);
	Dbg_MsgAssert(len<256,("String length too big, '%s'",p_string));
	Dbg_MsgAssert(spFrameBufferPos+2+len <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	
	*spFrameBufferPos++=token;
	*spFrameBufferPos++=len;
	for (int i=0; i<len; ++i)
	{
		*spFrameBufferPos++=*p_string++;
	}	
	// Note that the string is not terminated with a zero. Instead it is preceded with a byte
	// containing its length.
	// This is so that the string can be read out of the buffer in one go using ReadFromBuffer, rather
	// than one byte at a time.
	// I'm not sure how slow ReadFromBuffer is going to be on the GameCube since it will be in aram.
}

static void sWriteString(const char *p_string)
{
	Dbg_MsgAssert(p_string,("NULL p_string"));
	int len=strlen(p_string);
	Dbg_MsgAssert(len<256,("String length too big, '%s'",p_string));
	Dbg_MsgAssert(spFrameBufferPos+1+len <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
	
	*spFrameBufferPos++=len;
	for (int i=0; i<len; ++i)
	{
		*spFrameBufferPos++=*p_string++;
	}	
	// Note that the string is not terminated with a zero. Instead it is preceded with a byte
	// containing its length.
	// This is so that the string can be read out of the buffer in one go using ReadFromBuffer, rather
	// than one byte at a time.
	// I'm not sure how slow ReadFromBuffer is going to be on the GameCube since it will be in aram.
}
#endif

#if 0
static uint8 sWriteCAnimChannelChanges(const Gfx::CAnimChannel *p_a, const Gfx::CAnimChannel *p_b)
{
	uint8 num_changes_written=0;
	
	Dbg_MsgAssert((sizeof(Gfx::CAnimChannel)&3)==0,("sizeof(Gfx::CAnimChannel) not a multiple of 4 ??"));
	Dbg_MsgAssert(sizeof(Gfx::CAnimChannel)/4 < 256,("sizeof(Gfx::CAnimChannel) too big !"));
	
	uint32 *p_words_a=(uint32*)p_a;
	uint32 *p_words_b=(uint32*)p_b;
	for (uint8 i=0; i<sizeof(Gfx::CAnimChannel)/4; ++i)
	{
		if (1)//i!=Gfx::CAnimChannel::sGetCurrentTimeOffset())
		{
			if (*p_words_a != *p_words_b)
			{
				Dbg_MsgAssert(spFrameBufferPos+5 <= spFrameBuffer+FRAME_BUFFER_SIZE,("Replay frame buffer overflow"));
				*spFrameBufferPos++=i;
				spFrameBufferPos=Script::Write4Bytes(spFrameBufferPos, *p_words_b);
				++num_changes_written;
			}
		}	
		++p_words_a;
		++p_words_b;
	}

	return num_changes_written;
}
#endif

void WritePadVibration(int actuator, int percent)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	if (Config::GetHardware()==Config::HARDWARE_NGC)
	{
		// NGC must not replay vibrations, TRC requirement.
		return;
	}
		
	sWriteSingleToken(PAD_VIBRATION);
	sWriteUint8(actuator);
	sWriteUint32(percent);
}

void WritePauseSkater()
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(PAUSE_SKATER);
}

void WriteUnPauseSkater()
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(UNPAUSE_SKATER);
}

void WriteScreenFlash(int viewport, Image::RGBA from, Image::RGBA to, float duration, float z, uint32 flags)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	
	sWriteSingleToken(SCREEN_FLASH);
	sWriteUint32(viewport);
	Dbg_MsgAssert(sizeof(Image::RGBA)==4,("sizeof(Image::RGBA) not 4 ??"));
	sWriteUint32(*(uint32*)&from);
	sWriteUint32(*(uint32*)&to);
	sWriteFloat(duration);
	sWriteFloat(z);
	sWriteUint32(flags);
}

void WriteShatterParams(	Mth::Vector& velocity, float area_test, float velocity_variance, 
							float spread_factor, float lifetime, float bounce, float bounce_amplitude)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(SHATTER_PARAMS);
	sWriteFloat(velocity[X]);
	sWriteFloat(velocity[Y]);
	sWriteFloat(velocity[Z]);
	sWriteFloat(area_test);
	sWriteFloat(velocity_variance);
	sWriteFloat(spread_factor);
	sWriteFloat(lifetime);
	sWriteFloat(bounce);
	sWriteFloat(bounce_amplitude);
}

void Replay::WriteTextureSplat(Mth::Vector& splat_start, Mth::Vector& splat_end, float size, float lifetime, const char *p_texture_name, uint32 trail )
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(TEXTURE_SPLAT);
	sWriteFloat(splat_start[X]);
	sWriteFloat(splat_start[Y]);
	sWriteFloat(splat_start[Z]);
	sWriteFloat(splat_end[X]);
	sWriteFloat(splat_end[Y]);
	sWriteFloat(splat_end[Z]);
	sWriteFloat(size);
	sWriteFloat(lifetime);
//	sWriteString(p_texture_name);
	sWriteUint32(trail);
}

void WriteShatter(uint32 sectorName, bool on)
{
	if (sMode!=RECORD)
	{
		return;
	}
	sWriteSingleToken(on ? SHATTER_ON:SHATTER_OFF);
	sWriteUint32(sectorName);
}

void WriteSectorActiveStatus(uint32 sectorName, bool active)
{
	if (sMode!=RECORD)
	{
		return;
	}
	sWriteUint32(active ? SECTOR_ACTIVE:SECTOR_INACTIVE,sectorName);
}	

void WriteSectorVisibleStatus(uint32 sectorName, bool visible)
{
	if (sMode!=RECORD)
	{
		return;
	}
	sWriteUint32(visible ? SECTOR_VISIBLE:SECTOR_INVISIBLE,sectorName);
}	
	
void WriteManualMeter(bool state, float value)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	
	if (state)
	{
		sWriteSingleToken(MANUAL_METER_ON);
		sWriteFloat(value);
		sCurrentState.mManualMeterStatus=true;
		sCurrentState.mManualMeterValue=value;
	}
	else
	{
		if (sCurrentState.mManualMeterStatus)
		{
			sWriteSingleToken(MANUAL_METER_OFF);
			sCurrentState.mManualMeterStatus=false;
			sCurrentState.mManualMeterValue=0.0f;
		}
	}		
}

void WriteBalanceMeter(bool state, float value)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	
	if (state)
	{
		sWriteSingleToken(BALANCE_METER_ON);
		sWriteFloat(value);
		sCurrentState.mBalanceMeterStatus=true;
		sCurrentState.mBalanceMeterValue=value;
	}
	else
	{
		if (sCurrentState.mBalanceMeterStatus)
		{
			sWriteSingleToken(BALANCE_METER_OFF);
			sCurrentState.mBalanceMeterStatus=false;
			sCurrentState.mBalanceMeterValue=0.0f;
		}
	}		
}

void WriteSparksOn()
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(SPARKS_ON);
}	

void WriteSparksOff()
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(SPARKS_OFF);
}	

void WriteScorePotText(const char *p_text)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	Dbg_MsgAssert(p_text,("NULL p_text"));
	sWriteSingleToken(SCORE_POT_TEXT);
//	sWriteString(p_text);
}

static uint32 sLastTrickTextChecksum=0;
void WriteTrickText(const char **pp_text, int numStrings)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	uint32 ch=0;
	for (int i=0; i<numStrings; ++i)
	{
		ch=Crc::UpdateCRC(pp_text[i],strlen(pp_text[i]),ch);
	}	
	if (ch==sLastTrickTextChecksum)
	{
		return;
	}
	sLastTrickTextChecksum=ch;	
	
	sWriteSingleToken(TRICK_TEXT);
	Dbg_MsgAssert(numStrings<256,("numStrings too big"));
	sWriteUint8(numStrings);
	for (int string=0; string<numStrings; ++string)
	{
//		sWriteString(pp_text[string]);
	}	
}

void WriteTrickTextPulse()
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(TRICK_TEXT_PULSE);
}

void WriteTrickTextCountdown()
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(TRICK_TEXT_COUNTDOWN);
}

void WriteTrickTextLanded()
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(TRICK_TEXT_LANDED);
}

void WriteTrickTextBail()
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(TRICK_TEXT_BAIL);
}

void WriteSetAtomicStates(uint32 id, uint32 mask)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(SET_ATOMIC_STATES);
	sWriteUint32(id);
	sWriteUint32(mask);
}

void WritePlayStream(uint32 checksum, Sfx::sVolume *p_volume, float pitch, int priority)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(PLAY_STREAM);
	sWriteUint32(checksum);

	sWriteUint32(p_volume->GetVolumeType());
	sWriteFloat(p_volume->GetChannelVolume(0));
	sWriteFloat(p_volume->GetChannelVolume(1));
	if( p_volume->GetVolumeType() == Sfx::VOLUME_TYPE_5_CHANNEL_DOLBY5_1 )
	{
		sWriteFloat(p_volume->GetChannelVolume(2));
		sWriteFloat(p_volume->GetChannelVolume(3));
		sWriteFloat(p_volume->GetChannelVolume(4));
	}

	sWriteFloat(pitch);
	sWriteUint32(priority);
}

void WriteStopStream(int channel)
{
	if (sMode!=RECORD)
	{
		return;
	}
	sWriteSingleToken(STOP_STREAM);
	sWriteUint32(channel);
}

void WritePositionalStream(uint32 dummyId, uint32 streamNameChecksum, float dropoff, float volume, float pitch, int priority, int use_pos_info)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(PLAY_POSITIONAL_STREAM);
	sWriteUint32(dummyId);
	sWriteUint32(streamNameChecksum);
	sWriteFloat(dropoff);
	sWriteFloat(volume);
	sWriteFloat(pitch);
	sWriteUint32(priority);
	sWriteUint32(use_pos_info);
}	

void WritePositionalSoundEffect(uint32 dummyId, uint32 soundName, float volume, float pitch, float dropOffDist)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(PLAY_POSITIONAL_SOUND_EFFECT);
	sWriteUint32(dummyId);
	sWriteUint32(soundName);
	sWriteFloat(volume);
	sWriteFloat(pitch);
	sWriteFloat(dropOffDist);
}	

// Called from CSk3SfxManager::PlaySound, takes the same parameters, except for 'propogate' (sic)
// which I guess will always be false, since no replays in multiplayer.
void WriteSkaterSoundEffect(int whichArray, int surfaceFlag, const Mth::Vector &pos, 
							float volPercent)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(PLAY_SKATER_SOUND_EFFECT);
	sWriteUint32(whichArray);
	sWriteUint32(surfaceFlag);
	sWriteFloat(pos[X]);
	sWriteFloat(pos[Y]);
	sWriteFloat(pos[Z]);
	sWriteFloat(volPercent);
}

void WritePlaySound(uint32 checksum, float volL, float volR, float pitch)
{
	if (sMode!=RECORD)
	{
		return;
	}	
	sWriteSingleToken(PLAY_SOUND);
	sWriteUint32(checksum);
	sWriteFloat(volL);
	sWriteFloat(volR);
	sWriteFloat(pitch);
}

static void sRecordSkaterCamera(CTrackingInfo *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info"));
	
	if (p_info->mFlags & TRACKING_INFO_FLAG_OBJECT_CREATED)
	{
		sWriteUint32(CREATE_OBJECT,ID_CAMERA);
		sWriteUint8(SKATE_TYPE_UNDEFINED);
		p_info->mFlags &= ~TRACKING_INFO_FLAG_OBJECT_CREATED;
	}	
	else
	{
		// Write in the object Id
		sWriteUint32(OBJECT_ID,ID_CAMERA);
	}	
	
	Obj::CSkater *p_skater=sGetSkater();
	Gfx::Camera *p_skater_camera=p_skater->GetActiveCamera();
	Dbg_MsgAssert(p_skater_camera,("NULL p_skater_camera"));
	
	Mth::Vector actual_angles;
	p_skater_camera->GetMatrix().GetEulers(actual_angles);
	p_info->RecordPositionAndAngleChanges(p_skater_camera->GetPos(),actual_angles,true);
}

static bool sIsFlipped(Obj::CMovingObject *p_movingObject)
{
	Dbg_MsgAssert(p_movingObject,("NULL p_movingObject"));
	
	if (p_movingObject->GetType()==SKATE_TYPE_SKATER)
	{
		return GetSkaterCorePhysicsComponentFromObject(p_movingObject)->GetFlag(Obj::FLIPPED);
	}
	else
	{
		Dbg_MsgAssert( 0, ( "Flipped-ness has been moved to animation component" ) );
		return false;
//		return p_movingObject->IsFlipped();
	}
}

static void sRecordCMovingObject(CTrackingInfo *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info"));
	Dbg_MsgAssert(p_info->mPointerType==CMOVINGOBJECT,("Not a moving object?"));
	
	if (!p_info->mpMovingObject)
	{
		sWriteUint32(KILL_OBJECT,p_info->m_id);
		delete p_info;
		return;
	}	
	
	Obj::CMovingObject *p_moving_object=p_info->mpMovingObject;
	Nx::CModel* p_model=p_moving_object->GetModel();

	uint8 *p_start=spFrameBufferPos;
	
	if (p_info->mFlags & TRACKING_INFO_FLAG_OBJECT_CREATED)
	{
		sWriteUint32(CREATE_OBJECT,p_moving_object->GetID());
		sWriteUint8(p_moving_object->GetType());
		p_info->mFlags &= ~TRACKING_INFO_FLAG_OBJECT_CREATED;

		Obj::CAnimationComponent* pAnimComponent = GetAnimationComponentFromObject( p_moving_object );
        if ( pAnimComponent )
        {
            sWriteUint32(ANIM_SCRIPT_NAME,pAnimComponent->GetAnimScriptName());
        }
        else
        {
            sWriteUint32(ANIM_SCRIPT_NAME,0);
        }

		switch (p_moving_object->GetType())
		{
			case SKATE_TYPE_SKATER:
				sWriteUint32(SKELETON_NAME,0x5a9d2a0a/*Human*/);
				break;
			case SKATE_TYPE_CAR:
			{
				/*
				Obj::CCar *p_car=(Obj::CCar*)p_moving_object;
				Obj::CModelRestorationInfo *p_model_restoration_info=&p_car->m_model_restoration_info;
				
				sWriteUint32(SKELETON_NAME,p_model_restoration_info->mSkeletonName);
				sWriteString(MODEL_NAME,p_model_restoration_info->GetModelName());
				*/
				break;
			}	
			case SKATE_TYPE_PED:
			{
				/*
				Obj::CPed *p_ped=(Obj::CPed*)p_moving_object;
				Obj::CModelRestorationInfo *p_model_restoration_info=&p_ped->m_model_restoration_info;
				
				sWriteUint32(SKELETON_NAME,p_model_restoration_info->mSkeletonName);
				if (p_model_restoration_info->mProfileName)
				{
					sWriteUint32(PROFILE_NAME,p_model_restoration_info->mProfileName);
				}
				else
				{
					sWriteString(MODEL_NAME,p_model_restoration_info->GetModelName());
				}
				*/
				break;
			}	
			case SKATE_TYPE_GAME_OBJ:
			{
				/*
				Obj::CGameObj *p_game_ob=(Obj::CGameObj*)p_moving_object;
				Obj::CModelRestorationInfo *p_model_restoration_info=&p_game_ob->m_model_restoration_info;
			 
				sWriteUint32(SECTOR_NAME,p_model_restoration_info->mSectorName);
				sWriteUint32(SKELETON_NAME,p_model_restoration_info->mSkeletonName);
				sWriteString(MODEL_NAME,p_model_restoration_info->GetModelName());
				*/
				break;
			}
			/*
			// CBouncyObj has been replaced by CComposite object
			case SKATE_TYPE_BOUNCY_OBJ:
			{
				Obj::CBouncyObj *p_bouncy_ob=(Obj::CBouncyObj*)p_moving_object;
				Obj::CModelRestorationInfo *p_model_restoration_info=&p_bouncy_ob->m_model_restoration_info;
				
				sWriteUint32(SECTOR_NAME,p_model_restoration_info->mSectorName);
				break;
			}
			*/
			default:
				printf("Created object, type=%d\n",p_moving_object->GetType());
				break;
		}		

		p_info->mFlags &= ~TRACKING_INFO_FLAG_ACTIVE;
		p_info->mFlags &= ~TRACKING_INFO_FLAG_FLIPPED;
		p_info->mFlags &= ~TRACKING_INFO_FLAG_HOVERING;
		
		// Write out a SET_ATOMIC_STATES token to initialise the atomic states of the
		// dummy when it gets created later.
		if (p_model)
		{
			sWriteUint32(SET_ATOMIC_STATES,p_moving_object->GetID());
			sWriteUint32(p_model->GetGeomActiveMask());
		}	
	}	
	else
	{
		// Write in the object Id
		sWriteUint32(OBJECT_ID,p_moving_object->GetID());
	}	

	if (p_model && p_model->GetScale().GetX() != p_info->mScale)
	{
		// Some peds do use non 1 scale, eg the gorilla in the tram in the zoo
		// And the shark in sf2
		sWriteFloat(SET_SCALE,p_model->GetScale().GetX());
		p_info->mScale=p_model->GetScale().GetX();
	}	

	bool hovering=false;
	Obj::CMotionComponent *p_motion_component=GetMotionComponentFromObject(p_moving_object);
	if (p_motion_component)
	{
		hovering = p_motion_component->IsHovering();
	}

	
	
	if (hovering)
	{
		if (p_info->mFlags & TRACKING_INFO_FLAG_HOVERING)
		{
		}
		else
		{
			p_info->mFlags |= TRACKING_INFO_FLAG_HOVERING;
			Mth::Vector pos;
			p_motion_component->GetHoverOrgPos(&pos);
			
			sWriteSingleToken(HOVERING);
			sWriteFloat(pos[X]);
			sWriteFloat(pos[Y]);
			sWriteFloat(pos[Z]);
		}	
	}
	else
	{
		if (p_info->mFlags & TRACKING_INFO_FLAG_HOVERING)
		{
			// Need to support this? I don't think anything ever goes from hovering to not hovering?
		}
	}
						
	bool flipped=sIsFlipped(p_moving_object);
	if (flipped)
	{
		if (p_info->mFlags & TRACKING_INFO_FLAG_FLIPPED)
		{
		}
		else
		{
			p_info->mFlags |= TRACKING_INFO_FLAG_FLIPPED;
			sWriteSingleToken(FLIP);
		}	
	}
	else
	{
		if (p_info->mFlags & TRACKING_INFO_FLAG_FLIPPED)
		{
			p_info->mFlags &= ~TRACKING_INFO_FLAG_FLIPPED;
			sWriteSingleToken(UNFLIP);
		}
	}

	bool active=false;
	if (p_model)
	{
		active=p_model->GetActive();
	}
	if (active)
	{
		if (p_info->mFlags & TRACKING_INFO_FLAG_ACTIVE)
		{
		}
		else
		{
			p_info->mFlags |= TRACKING_INFO_FLAG_ACTIVE;
			sWriteSingleToken(MODEL_ACTIVE);
		}	
	}
	else
	{
		if (p_info->mFlags & TRACKING_INFO_FLAG_ACTIVE)
		{
			p_info->mFlags &= ~TRACKING_INFO_FLAG_ACTIVE;
			sWriteSingleToken(MODEL_INACTIVE);
		}
	}
	
	//Mth::Matrix objects_display_matrix=p_moving_object->GetDisplayMatrix();
	//objects_display_matrix[Mth::POS]=p_moving_object->GetPos();
	//Mth::Vector actual_pos=objects_display_matrix[Mth::POS];
	
	if (!hovering)
	{
		Mth::Vector actual_pos=p_moving_object->GetPos();
		Mth::Vector actual_angles;
		
		//if (p_moving_object->IsSpecialItem())
		//{
		//	printf("Recording exact pos of special item %x\n",p_moving_object->GetID());
		//}
			
		// GJ:  Need to reimplement special-case special item code
		// because the current implementation conflicts with our
		// new CCOmpositeObject model.
		if (p_info->m_id==ID_SKATER )
//		if (p_info->m_id==ID_SKATER || p_moving_object->IsSpecialItem())
		{
			p_moving_object->GetDisplayMatrix().GetEulers(actual_angles);
			p_info->RecordPositionAndAngleChanges(actual_pos,actual_angles,true);
		}
		else
		{
			if (p_moving_object->GetDisplayMatrix() != p_info->mLastMatrix)
			{
				p_moving_object->GetDisplayMatrix().GetEulers(actual_angles);
				p_info->mLastMatrix=p_moving_object->GetDisplayMatrix();
			}
			else
			{
				actual_angles[X]=p_info->mTrackerAngleX.GetActualLast();
				actual_angles[Y]=p_info->mTrackerAngleY.GetActualLast();
				actual_angles[Z]=p_info->mTrackerAngleZ.GetActualLast();
			}	
			p_info->RecordPositionAndAngleChanges(actual_pos,actual_angles);
		}	
	}
		
	// Do the animation stuff ...
#if 0
	uint8 *p_num_changes=NULL;
	uint8 num_changes=0;
	
	Obj::CAnimationComponent* pAnimComponent = GetAnimationComponentFromObject( p_moving_object );
	if ( pAnimComponent && pAnimComponent->HasAnims() )
	{
		sWriteSingleToken(PRIMARY_ANIM_CONTROLLER_CHANGES);
		p_num_changes=spFrameBufferPos;
		++spFrameBufferPos;
		num_changes=sWriteCAnimChannelChanges(&p_info->m_primaryController,pAnimComponent->GetPrimaryController());
		if (!num_changes)
		{
			spFrameBufferPos-=2;
		}
		else
		{
			p_info->m_primaryController=*pAnimComponent->GetPrimaryController();
			*p_num_changes=num_changes;
		}
	}	
	
	if (p_info->m_id==ID_SKATER)
	{
		for (int i=0; i<NUM_DEGENERATE_ANIMS; ++i)
		{
			sWriteSingleToken(DEGENERATE_ANIM_CONTROLLER_CHANGES);
			sWriteUint8(i);
			p_num_changes=spFrameBufferPos;
			++spFrameBufferPos;
            if (pAnimComponent)
            {
                num_changes=sWriteCAnimChannelChanges(&p_info->sSkaterTrackingInfo.m_degenerateControllers[i],pAnimComponent->GetDegenerateController(i));
            }
            else
            {
                num_changes=0;
            }
			if (!num_changes)
			{
				spFrameBufferPos-=3;
			}	
			else
			{
				p_info->sSkaterTrackingInfo.m_degenerateControllers[i]=*pAnimComponent->GetDegenerateController(i);
				*p_num_changes=num_changes;
			}
		}
		
		// Check for skater looping sound changes
		uint32 skater_looping_sound_id=0;
		uint32 skater_looping_sound_checksum=0;
		float skater_pitch_min;
		float skater_pitch_max;
		Obj::CSkater *p_skater=(Obj::CSkater*)p_moving_object;
		
		// NOTE: this information has moved to CSkaterLoopingSoundComponent
		p_skater->GetLoopingSoundInfo(	&skater_looping_sound_id,
										&skater_looping_sound_checksum,
										&skater_pitch_min,
										&skater_pitch_max);
		if (skater_looping_sound_id)
		{
			// Skater is playing a looping sound
			if (CTrackingInfo::sSkaterTrackingInfo.m_playing_looping_sound)
			{
				// Tracker already knows the skater was playing a looping sound, but
				// perhaps the name of the sound has changed.
				if (CTrackingInfo::sSkaterTrackingInfo.m_looping_sound_checksum != skater_looping_sound_checksum)
				{
					sWriteUint32(PLAY_LOOPING_SOUND,skater_looping_sound_checksum);
					CTrackingInfo::sSkaterTrackingInfo.m_looping_sound_checksum = skater_looping_sound_checksum;
				}
			}	
			else
			{
				sWriteUint32(PLAY_LOOPING_SOUND,skater_looping_sound_checksum);
				CTrackingInfo::sSkaterTrackingInfo.m_looping_sound_checksum = skater_looping_sound_checksum;
				CTrackingInfo::sSkaterTrackingInfo.m_playing_looping_sound=true;
			}	
			
			if (skater_pitch_min != CTrackingInfo::sSkaterTrackingInfo.m_pitch_min)
			{
				sWriteFloat(PITCH_MIN,skater_pitch_min);
				CTrackingInfo::sSkaterTrackingInfo.m_pitch_min = skater_pitch_min;
			}	
			if (skater_pitch_max != CTrackingInfo::sSkaterTrackingInfo.m_pitch_max)
			{
				sWriteFloat(PITCH_MAX,skater_pitch_max);
				CTrackingInfo::sSkaterTrackingInfo.m_pitch_max = skater_pitch_max;
			}	
		}
		else
		{
			if (CTrackingInfo::sSkaterTrackingInfo.m_playing_looping_sound)
			{
				sWriteSingleToken(STOP_LOOPING_SOUND);
				CTrackingInfo::sSkaterTrackingInfo.m_playing_looping_sound=false;
				CTrackingInfo::sSkaterTrackingInfo.m_looping_sound_checksum=0;
			}
		}		
	}	
#endif

	if (p_moving_object->GetType()==SKATE_TYPE_CAR)
	{
		float car_rotation_x, wheel_rotation_x, wheel_rotation_y;
		((Obj::CCar*)p_moving_object)->GetRotationValues(&car_rotation_x,&wheel_rotation_x,&wheel_rotation_y);
		
		p_info->mTrackerCarRotationX.WriteChanges(car_rotation_x,SET_CAR_ROTATION_X,SET_CAR_ROTATION_X_VEL);
		p_info->mTrackerWheelRotationX.WriteChanges(wheel_rotation_x,SET_WHEEL_ROTATION_X,SET_WHEEL_ROTATION_X_VEL);
		p_info->mTrackerWheelRotationY.WriteChanges(wheel_rotation_y,SET_WHEEL_ROTATION_Y,SET_WHEEL_ROTATION_Y_VEL);
	}
	
	// If no changes at all got written in for this object, then remove the object Id.
	// This is an important space optimization, because if this redundant info were not
	// removed then each frame would be several hundred bytes bigger.
	if (*p_start==OBJECT_ID && spFrameBufferPos-p_start==5)
	{
		spFrameBufferPos=p_start;
	}	
}

//static int record_frame=0;
static void sRecord()
{
	CTrackingInfo *p_info=spTrackingInfoHead;
	while (p_info)
	{
		CTrackingInfo *p_next=p_info->mpNext;

		if (p_info->m_id==ID_CAMERA)
		{
			sRecordSkaterCamera(p_info);
		}
		else
		{
			switch (p_info->mPointerType)
			{
				case CMOVINGOBJECT:
					sRecordCMovingObject(p_info);
					break;
				default:
					break;
			}
		}
									
		p_info=p_next;
	}	

	int current_blur=Nx::CEngine::sGetScreenBlur();
	if (current_blur != CTrackingInfo::sScreenBlurTracker)
	{
		sWriteSingleToken(SCREEN_BLUR);
		sWriteUint32(current_blur);
		CTrackingInfo::sScreenBlurTracker=current_blur;
	}	
	
	// REMOVE
	//printf("Size=%d\n",spFrameBufferPos-spFrameBuffer);
	
	sWriteFrameBufferToBigBuffer();
}

//static int playback_frame=0;
static void sPlayback(bool display)
{
	// If reached the end of the replay, do nothing.
	if (sReachedEndOfReplay)
	{
		return;
	}	
	if (sReplayPaused)
	{
		return;
	}	

	Obj::CSkater *p_skater=sGetSkater();
	// NGC must not replay vibrations, TRC requirement.
	if (Config::GetHardware() != Config::HARDWARE_NGC)
	{
		if (sNeedToInitialiseVibration)
		{
			if (!sCurrentState.mSkaterPaused)
			{
				for (int i=0; i<Obj::CVibrationComponent::vVB_NUM_ACTUATORS; ++i)
				{
					p_skater->GetDevice()->ActivateActuator(i,sCurrentState.mActuatorStrength[i]);
				}	
			}	
			sNeedToInitialiseVibration=false;
		}
	}
		
	// Read the next frame
	bool nothing_follows=false;
	sNextPlaybackFrameOffset=sReadFrame(sNextPlaybackFrameOffset,&nothing_follows,PLAYBACK_DUMMY_LIST);

	// The process of reading the frame will have updated all the dummy's.
	// Now display them & do other misc stuff like update sounds.
	CDummy *p_dummy=spReplayDummies;
	while (p_dummy)
	{
		p_dummy->UpdateMutedSounds();
		if (display)
		{
			p_dummy->DisplayModel();
		}	
		p_dummy=p_dummy->mpNext;
	}


	// Wrap around if reached the end of the replay buffer.
	if (sNextPlaybackFrameOffset==GetBufferSize())
	{
		sNextPlaybackFrameOffset=0;
	}
	
	// It is possible that the frame could be followed by some space filled with
	// BLANK, so skip over it until either a new frame is hit or we get back
	// to the start frame.
	while (true)
	{
		uint8 token;
		ReadFromBuffer(&token,sNextPlaybackFrameOffset,1);
		
		if (token != BLANK)
		{
			// Hit a new frame
			Dbg_MsgAssert(token==FRAME_START,("Expected token to be FRAME_START"));
			break;
		}
		
		// sBufferFilling is a flag used to indicate that the buffer is filling up for the
		// first time and has not cycled around yet.
		// Knowing this means we don't need to read each individual byte, since we know everything
		// will be blank till the end of the buffer, so just set sNextPlaybackFrameOffset to 0
		// straight away.
		// This is a speed optimization for the GameCube, since it has a 4meg replay buffer, and
		// it takes several seconds so read all those blanks.
		if (sBufferFilling)
		{
			Dbg_MsgAssert(sBigBufferStartOffset==0,("Expected sBigBufferStartOffset to be zero when the buffer is filling up?"));
			sNextPlaybackFrameOffset=0;
		}
		
		if (sNextPlaybackFrameOffset==sBigBufferStartOffset)
		{
			// Reached the start again.
			break;
		}
			
		++sNextPlaybackFrameOffset;
		if (sNextPlaybackFrameOffset==GetBufferSize())
		{
			sNextPlaybackFrameOffset=0;
		}
	}					
		
	// If we've got back to the start, then that is the end of the replay,
	// so kill all the dummys and restore everything the way it was.
	if (sNextPlaybackFrameOffset==sBigBufferStartOffset)
	{
		// Switch off any vibrations being done by the replay, so that they do not continue
		// while the end-replay menu is on screen.
		// Note that any vibrations that were being done in-game before the replay was run will
		// automatically get restored when the game is un-paused, so we do not need to restore them here.
		sSwitchOffVibrations();

		sStopReplayDummyLoopingSounds();
		
		// Now that the replay has ended, the player may want to save it to mem card.
		// So make sure that the start-state contains the correct active/visible status of all the sectors.
		Nx::CEngine::sWriteSectorStatusBitfield(sStartState.mpSectorStatus,SECTOR_STATUS_BUFFER_SIZE);
		
		Script::RunScript("EndOfReplay");
		sReachedEndOfReplay=true;
	}	
}

static void sEnsureTrackerExists( Obj::CObject *p_ob, void *p_data )
{
	Dbg_MsgAssert(p_ob,("NULL p_ob"));

	CTrackingInfo *p_info=sTrackingInfoHashTable.GetItem((uint32)p_ob->GetID());
	if (!p_info)
	{
		int type=p_ob->GetType();
		Dbg_MsgAssert(!(p_ob->GetID()==0 && type!=SKATE_TYPE_SKATER),("Non-skater object has an id of 0, type=%d\n",type));
		
		if (type==SKATE_TYPE_CAR ||
			type==SKATE_TYPE_PED ||
			type==SKATE_TYPE_BOUNCY_OBJ ||
			type==SKATE_TYPE_SKATER ||
			type==SKATE_TYPE_GAME_OBJ)
		{
			p_info=new CTrackingInfo;
			p_info->SetMovingObject((Obj::CMovingObject*)p_ob);
			p_info->mFlags |= TRACKING_INFO_FLAG_OBJECT_CREATED;
		}	
	}	
}

static void sEnsureCameraTrackerExists()
{
	CTrackingInfo *p_info=sTrackingInfoHashTable.GetItem((uint32)ID_CAMERA);
	if (!p_info)
	{
		p_info=new CTrackingInfo;
		p_info->SetSkaterCamera();
		p_info->mFlags |= TRACKING_INFO_FLAG_OBJECT_CREATED;
	}	
}

static void sHideForReplayPlayback( Obj::CObject *p_ob, void *p_data )
{
	Dbg_MsgAssert(p_ob,("NULL p_ob"));
	p_ob->HideForReplayPlayback();
}

static void sRestoreAfterReplayPlayback( Obj::CObject *p_ob, void *p_data )
{
	Dbg_MsgAssert(p_ob,("NULL p_ob"));
	p_ob->RestoreAfterReplayPlayback();
}

static void sClearLastPanelMessage()
{
	Script::CStruct *p_params=new Script::CStruct;
	p_params->AddChecksum("id",sLastRecordedPanelMessageID);
	
	Script::RunScript("kill_panel_message_if_it_exists",p_params);
	delete p_params;
}

static void sClearTrickAndScoreText()
{
	Front::CScreenElementManager* p_manager = Front::CScreenElementManager::Instance();
	int index=sGetSkater()->GetHeapIndex();
	Front::CTextBlockElement *p_text_block = (Front::CTextBlockElement *) p_manager->GetElement(0x44727dae/*the_trick_text*/ + index ).Convert();
	if (p_text_block)
	{
		p_text_block->SetText(NULL, 0);
	}
	Front::CTextElement *p_score_pot_text = (Front::CTextElement *) p_manager->GetElement(0xf4d3a70e + index ).Convert(); // "the_score_pot_text"
	if (p_score_pot_text)
	{
		p_score_pot_text->SetText("");
	}
	sTrickTextGotCleared=true;
}

bool ScriptClearTrickAndScoreText(Script::CStruct *pParams, Script::CScript *pScript)
{
	sClearTrickAndScoreText();
	return true;
}

bool ScriptPlaybackReplay(Script::CStruct *pParams, Script::CScript *pScript)
{
	sClearTrickAndScoreText();
	sClearLastPanelMessage();
	Nx::MiscFXCleanup();
	Nx::CEngine::sSetScreenBlur(0);
	
	// Check that there are no replay dummy's in existence at the moment.
	Dbg_MsgAssert(spReplayDummies==NULL,("Expected spReplayDummies to be NULL")); 
	sReplayDummysHashTable.IterateStart();
	Dbg_MsgAssert(sReplayDummysHashTable.IterateNext()==NULL,("Expected sReplayDummysHashTable to be empty")); 

	// Create the replay dummy's by making copies of the start-state dummy's.
	CDummy *p_source_dummy=spStartStateDummies;
	while (p_source_dummy)
	{
		CDummy *p_new_dummy=new CDummy(PLAYBACK_DUMMY_LIST,p_source_dummy->GetID());
		// Make a copy of the contents of the CDummy.
		// The assignement operator for CDummy is overloaded.
		*p_new_dummy=*p_source_dummy;
		
		p_source_dummy=p_source_dummy->mpNext;
	}	

	Nx::CEngine::sPrepareSectorsForReplayPlayback(pParams->ContainsFlag("store"));
	
	sCurrentState=sStartState;
	sNextPlaybackFrameOffset=sBigBufferStartOffset;
	sLastTrickTextChecksum=0;
	sMode=PLAYBACK;
	sReachedEndOfReplay=false;
	sReplayPaused=false;
	
	// NGC must not replay vibrations, TRC requirement.
	if (Config::GetHardware()==Config::HARDWARE_NGC)
	{
		sSwitchOffVibrations();
	}
		
	sNeedToInitialiseVibration=true;
	sUnpauseCertainScreenElements();
	
	return true;
}

bool ScriptHideGameObjects( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	pSkate->GetObjectManager()->ProcessAllObjects( sHideForReplayPlayback, NULL );
	return true;
}

bool ScriptShowGameObjects( Script::CStruct *pParams, Script::CScript *pScript )
{
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	pSkate->GetObjectManager()->ProcessAllObjects( sRestoreAfterReplayPlayback, NULL );
	
	// Make sure that the balance meters are not left on if the replay was quit during a balance.
	sGetSkaterScoreObject()->SetManualMeter(false);
	sGetSkaterScoreObject()->SetBalanceMeter(false);
	
	// Restore the active/visible state of the world sectors.
	Nx::CEngine::sRestoreSectorsAfterReplayPlayback();
	return true;
}

bool ScriptSwitchToReplayRecordMode( Script::CStruct *pParams, Script::CScript *pScript )
{
	if (Mdl::Skate::Instance()->IsMultiplayerGame())
	{
		return false;
	}	
	sMode=RECORD;
	return true;
}

bool ScriptSwitchToReplayIdleMode( Script::CStruct *pParams, Script::CScript *pScript )
{
	if (Mdl::Skate::Instance()->IsMultiplayerGame())
	{
		return false;
	}	
	sMode=NONE;
	return true;
}

bool RunningReplay()
{
	return sMode==PLAYBACK;
}

bool ScriptRunningReplay( Script::CStruct *pParams, Script::CScript *pScript )
{
	return RunningReplay();
}
	
bool ScriptPauseReplay( Script::CStruct *pParams, Script::CScript *pScript )
{
	sReplayPaused=true;	
	
	// Switch off any vibration while the replay is paused
	sSwitchOffVibrations();
	
	sStopReplayDummyLoopingSounds();
	
	// Then set this flag so that the sPlayback function will switch the vibration back
	// on if need be once the replay is unpaused.
	sNeedToInitialiseVibration=true;
	return true;
}

bool ScriptUnPauseReplay( Script::CStruct *pParams, Script::CScript *pScript )
{
	sReplayPaused=false;	
	sUnpauseCertainScreenElements();
	return true;
}
	
bool Paused()
{
	return sReplayPaused;
}
	
void AddReplayMemCardInfo(Script::CStruct *p_struct)
{
	Dbg_MsgAssert(p_struct,("NULL p_struct"));
	p_struct->AddChecksum("level_structure_name",sLevelStructureName);
}

void AddReplayMemCardSummaryInfo(Script::CStruct *p_struct)
{
	Dbg_MsgAssert(p_struct,("NULL p_struct"));
	
	Script::CStruct *p_level_def=Script::GetStructure(sLevelStructureName);
	Dbg_MsgAssert(p_level_def,("Could not find level def '%s'",Script::FindChecksumName(sLevelStructureName)));
	
	const char *p_level_name="";
	p_level_def->GetString("Name",&p_level_name);
	
	p_struct->AddString("LevelName",p_level_name);
}

bool ScriptRememberLevelStructureNameForReplays(Script::CStruct *pParams, Script::CScript *pScript)
{
	pParams->GetChecksum("level_structure_name",&sLevelStructureName,Script::ASSERT);
	return true;
}

void SetLevelStructureName(uint32 level_structure_name)
{
	sLevelStructureName=level_structure_name;
}

bool ScriptGetReplayLevelStructureName(Script::CStruct *pParams, Script::CScript *pScript)
{
	pScript->GetParams()->AddChecksum("level_structure_name",sLevelStructureName);
	return true;
}

Manager::Manager()
{
	Dbg_MsgAssert(NUM_REPLAY_TOKEN_TYPES<=256,("Too many token types !!"));
	
	mp_start_frame_task = new Tsk::Task< Manager > ( s_start_frame_code, *this,
										Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_REPLAY_START_FRAME );

	mp_end_frame_task = new Tsk::Task< Manager > ( s_end_frame_code, *this,
										Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_REPLAY_END_FRAME );
										
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
	mlp_manager->AddLogicTask( *mp_start_frame_task );
	mlp_manager->AddLogicTask( *mp_end_frame_task );
}

Manager::~Manager()
{
	delete mp_start_frame_task;
	delete mp_end_frame_task;
}

void Manager::s_start_frame_code( const Tsk::Task< Manager >& task )
{
	//Manager& man = task.GetData();
	spFrameBufferPos=spFrameBuffer;
	sWriteUint32(FRAME_START,Tmr::GetTime());
}

/*
static Tmr::CPUCycles sStartTime;
static int s_num_times=0;
static int s_time_index=0;
#define MAX_TIMES 60
static Tmr::CPUCycles spTimes[MAX_TIMES];

void NewTime(Tmr::CPUCycles t)
{
	spTimes[s_time_index++]=t;
	if (s_time_index>=MAX_TIMES)
	{
		s_time_index=0;
	}
	if (s_num_times<MAX_TIMES)
	{
		++s_num_times;
	}
	
	Tmr::CPUCycles total=0;
	for (int i=0; i<s_num_times; ++i)
	{
		total+=spTimes[i];
	}	
	
	printf("%d\n",total/s_num_times);
}	
*/

void Manager::s_end_frame_code( const Tsk::Task< Manager >& task )
{
	//Manager& man = task.GetData();
	#ifdef DISABLE_REPLAYS
	return;
	#endif

	if (Mdl::Skate::Instance()->IsMultiplayerGame())
	{
		return;
	}	
	
	switch (sMode)
	{
		case RECORD:
		{
			//sStartTime=Tmr::GetTimeInCPUCycles();

			sEnsureCameraTrackerExists();
					
			Mdl::FrontEnd* p_front = Mdl::FrontEnd::Instance();
			if (!p_front->GamePaused())
			{
				Mdl::Skate * p_skate = Mdl::Skate::Instance();
				p_skate->GetObjectManager()->ProcessAllObjects( sEnsureTrackerExists, NULL );
				
				sRecord();
			}	
			
			//Tmr::CPUCycles t_record_time=Tmr::GetTimeInCPUCycles()-sStartTime;
			//NewTime(t_record_time);
			break;
		}	
		case PLAYBACK:
		{
			bool left=false;
			bool right=false;
			Mdl::FrontEnd* pFront = Mdl::FrontEnd::Instance();
	
			for ( int i = 0; i < SIO::vMAX_DEVICES; i++ )
			{
				if ( CFuncs::CheckButton( pFront->GetInputHandler( i ), 0x85981897 ) ) // left
				{
					left=true;
				}	
				if ( CFuncs::CheckButton( pFront->GetInputHandler( i ), 0x4b358aeb ) ) // right
				{
					right=true;
				}	
			}
			
			if (left)
			{
				if (Tmr::GetVblanks()&1)
				{
					sPlayback();
				}	
			}
			else if (right)
			{
				sPlayback(false);
				sPlayback();
			}	
			else
			{
				sPlayback();
			}	
			break;
		}	
		default:
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Gfx::CSkeleton* CDummy::GetSkeleton()
{
	if ( mp_skeletonComponent )
	{
		return mp_skeletonComponent->GetSkeleton();
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CDummy::LoadSkeleton( uint32 skeletonName )
{
    // temporarily moved this function from CModel,
    // in order to make it easier to split off
    // skeleton into its own component.

	Gfx::CSkeletonData* pSkeletonData = (Gfx::CSkeletonData*) Ass::CAssMan::Instance()->GetAsset( skeletonName, false );

	if ( !pSkeletonData )
	{
		Dbg_MsgAssert( 0, ("Unrecognized skeleton %s. (Is skeleton.q up to date?)", Script::FindChecksumName(skeletonName)) );
	}
    
	Dbg_MsgAssert( mp_skeletonComponent == NULL, ( "Model already has skeleton component.  Possible memory leak?" ) );    
	mp_skeletonComponent = new Obj::CSkeletonComponent;

    Script::CStruct* pTempStructure = new Script::CStruct;
    pTempStructure->AddChecksum( CRCD(0x222756d5,"skeleton"), skeletonName );
    
    mp_skeletonComponent->InitFromStructure( pTempStructure );
    delete pTempStructure;

#ifdef __NOPT_ASSERT__
	Gfx::CSkeleton* pSkeleton = mp_skeletonComponent->GetSkeleton();
    
    Dbg_Assert( pSkeleton );
    Dbg_Assert( pSkeleton->GetNumBones() > 0 );
#endif		// __NOPT_ASSERT__
	
	return true;
//    return mp_rendered_model->SetSkeleton( mp_skeletonComponent->GetSkeleton() );
}

}  // namespace Replay

#endif
