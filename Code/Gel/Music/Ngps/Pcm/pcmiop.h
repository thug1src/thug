#if 0
#define PRINTF(x) printf x
#else
#define PRINTF(x) 
#endif
#define ERROR(x) printf x
#define xPRINTF(x) 

#define BASE_priority  32

#define OLDLIB 0
#define TRANS_CH  0

typedef char				int8;
typedef short				int16;
typedef int					int32;

typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;


typedef enum
{
	SEND_SOUND_RESULT_FALSE = 0,
	SEND_SOUND_RESULT_TRUE,
	SEND_SOUND_RESULT_PANIC,
} ESendSoundResult;

enum{
	MUSIC_LOAD_STATE_IDLE = 0, // Idle state must be zero!
	MUSIC_LOAD_STATE_PRELOADING_L,
	MUSIC_LOAD_STATE_PRELOADING_R,
	MUSIC_LOAD_STATE_WAITING_FOR_REFRESH,
	MUSIC_LOAD_STATE_WAITING_FOR_LAST_LOAD,
	MUSIC_LOAD_STATE_LOADING_L,
	MUSIC_LOAD_STATE_LOADING_R,
};

enum{
	STREAM_LOAD_STATE_IDLE = 0, // Idle state must be zero!
	STREAM_LOAD_STATE_PRELOADING,
	STREAM_LOAD_STATE_WAITING_FOR_REFRESH,
	STREAM_LOAD_STATE_WAITING_FOR_LAST_LOAD,
	STREAM_LOAD_STATE_LOADING,
};

// Holds all the data necessary for an audio or music stream
struct StreamInfo{
	// State variables
	volatile uint32	loadState;
	volatile uint32	status;
	volatile uint16	m_preloadMode;

	volatile uint16	m_iopBufLoaded[2];

	// Stream parameters
	volatile uint16	volumeSet;
	volatile uint32	volume;
	volatile uint16	paused;
	volatile uint16	stop;
	int				pitch;

	// Stream size info
	volatile uint32	size;
	int 			remaining;

	// Current buffer and address info
	uint16			m_iopBufIndex;
	uint32			m_iopOffset;
	uint32			m_spuBufSide;			// buffer we are LOADING to
	uint32			m_spuTransSize;
	volatile uint16 m_spuTransDone;
	uint32			m_spuCurAddr;
	uint32			m_spuEndAddr;
};
