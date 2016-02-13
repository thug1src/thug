/* Vag streaming into SPU2 -- from sony sample -- matt may 2001 */

#include <kernel.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <sif.h>
#include <sifcmd.h>
#include <libsd.h>
#include <sdrcmd.h>
#include <thread.h>
#include "pcm.h"
#include "pcmiop.h"

#define PRINT_DROPPED_REQUESTS	0

// ================================================================

#define _1st 0
#define _2nd 1

#define TRUE  1
#define FALSE 0

#define TRANS_DMA_CH_MUSIC		1
#define TRANS_DMA_CH_STREAM		0

#define NUM_CORES				2

unsigned int gSndSem;

extern volatile int gUsingStreamDMA;
extern volatile int gSPUUsingStreamDMA;

#if PRINT_DROPPED_REQUESTS
static int s_received_requests = 0;
#endif

//	sceSdSetTransIntrHandler( TRANS_DMA_CH_MUSIC, ( sceSdTransIntrHandler ) _AdpcmDmaIntMusic, ( void * ) &gSem );
//  sceSdSetTransIntrHandler( TRANS_DMA_CH_STREAM, ( sceSdTransIntrHandler ) _AdpcmDmaIntStream, ( void * ) &gSem );

// Used to hold the queued up SPU switch registers
volatile static uint32			s_spu_reg_kon[NUM_CORES];
volatile static uint32			s_spu_reg_koff[NUM_CORES];
volatile static uint32			s_spu_kon_changed[NUM_CORES];
volatile static uint32			s_spu_koff_changed[NUM_CORES];
volatile static uint32			s_spu_reg_endx[NUM_CORES];
volatile static uint32			s_switch_alarm_set;
volatile static uint32			s_send_spu_status;

#define	SET_PARAM_ALARM_TIME	(100)			// Number of microseconds to wait before setting SPU switches (must be greater
												// than 1/24000 seconds)

uint32 switch_set_callback(void *p_nothing)
{
	int core;

	for (core = 0; core < NUM_CORES; core++)
	{
		// First, get the status
		s_spu_reg_endx[core] = sceSdGetSwitch( core | SD_S_ENDX );

		if (s_spu_kon_changed[core])
		{
			sceSdSetSwitch( core | SD_S_KON, s_spu_reg_kon[core] );
			s_spu_kon_changed[core] = FALSE;
			s_spu_reg_endx[core] &= ~s_spu_reg_kon[core];	// Update the status since SPU won't do it for another tick
			s_spu_reg_kon[core] = 0;
		}

		if (s_spu_koff_changed[core])
		{
			sceSdSetSwitch( core | SD_S_KOFF, s_spu_reg_koff[core] );
			s_spu_koff_changed[core] = FALSE;
			s_spu_reg_endx[core] |= s_spu_reg_koff[core];	// Update the status since SPU won't do it for another tick
			s_spu_reg_koff[core] = 0;
		}
	}

	s_switch_alarm_set = FALSE;
	s_send_spu_status = TRUE;

	// And wake up the dispatch thread
	iSignalSema(gSndSem);

	return 0;
}

// This function will set an alarm so that multiple KON and KOFF settings are combined.  If it isn't these type
// of switches, it will be passed through
void queue_set_switch(unsigned short entry, unsigned int value)
{
	int core = entry & 0x1;
	int switch_reg = entry & 0xFF00;
	int oldStat;

	// Make sure alarm callback doesn't run here
	CpuSuspendIntr(&oldStat);

	switch (switch_reg)
	{
	case SD_S_KON:
		if(s_spu_reg_koff[core] & value)
		{
			//Dbg_MsgAssert(0, ("Turning on a voice that was turned off this frame."));
			Kprintf("Turning on a voice that was turned off this SPU tick.\n");
			s_spu_reg_koff[core] &= ~(s_spu_reg_koff[core] & value);	// turn off those bits
		}

		s_spu_reg_kon[core] |= value;
		s_spu_kon_changed[core] = TRUE;

		break;

	case SD_S_KOFF:
		if(s_spu_reg_kon[core] & value)
		{
			//Dbg_MsgAssert(0, ("Turning off a voice that was turned on this frame."));
			Kprintf("Turning off a voice that was turned on this SPU tick.\n");
			s_spu_reg_kon[core] &= ~(s_spu_reg_kon[core] & value);		// turn off those bits
		}

		s_spu_reg_koff[core] |= value;
		s_spu_koff_changed[core] = TRUE;

		break;

	default:
		// Just set SPU and exit (so we don't set alarm)
		sceSdSetSwitch( entry, value );
		CpuResumeIntr(oldStat);
		return;
	}

	// Turn on alarm if it isn't already on
	if (!s_switch_alarm_set)
	{
		struct SysClock clock;
		USec2SysClock(SET_PARAM_ALARM_TIME, &clock);

		SetAlarm(&clock, switch_set_callback, NULL);

		s_switch_alarm_set = TRUE;
	}

	CpuResumeIntr(oldStat);
}

// This function belongs in pcm_sound.c, but because of some SN Sys link error, it has to be in here for now.
int execute_sound_request(unsigned short function, unsigned short entry, unsigned int value,
						  unsigned int arg0, unsigned int arg1, unsigned int arg2, ESendSoundResult *p_send_result)
{ 
	int ret = -1;
	*p_send_result = SEND_SOUND_RESULT_FALSE;

	switch (function)
	{
	case rSdInit:
		ret = sceSdInit( entry );
		*p_send_result = SEND_SOUND_RESULT_TRUE;
		break;

	case rSdSetParam:
		sceSdSetParam( entry, value );
		break;

	case rSdGetParam:
		ret = sceSdGetParam( entry );
		*p_send_result = SEND_SOUND_RESULT_TRUE;
		break;

	case rSdSetSwitch:
		queue_set_switch( entry, value );
		break;

	case rSdGetSwitch:
		ret = sceSdGetSwitch( entry );
		*p_send_result = SEND_SOUND_RESULT_TRUE;
		break;

	case rSdSetAddr:
		sceSdSetAddr( entry, value );
		//Kprintf("Setting addr %x to %x\n", entry, value);
		break;

	case rSdGetAddr:
		ret = sceSdGetAddr( entry );
		*p_send_result = SEND_SOUND_RESULT_TRUE;
		break;

	case rSdSetCoreAttr:
		sceSdSetCoreAttr( entry, value );
		break;

	case rSdGetCoreAttr:
		ret = sceSdGetCoreAttr( entry );
		*p_send_result = SEND_SOUND_RESULT_TRUE;
		break;

	case rSdNote2Pitch:
		ret = sceSdNote2Pitch ( entry /*center_note*/, value /*center_fine*/, arg0 /*note*/, arg1 /*fine*/);
		*p_send_result = SEND_SOUND_RESULT_TRUE;
		break;

	case rSdPitch2Note:
		ret = sceSdPitch2Note ( entry /*center_note*/, value /*center_fine*/, arg0 /*pitch*/);
		*p_send_result = SEND_SOUND_RESULT_TRUE;
		break;

	case rSdVoiceTrans:
		sceSdVoiceTrans( entry /*channel*/, value /*mode*/, (u_char *) arg0 /*m_addr*/, arg1 /*s_addr*/, arg2 /*size*/ );
		break;

	case rSdBlockTrans:
		sceSdBlockTrans( entry /*channel*/, value /*mode*/, (u_char *) arg0 /*m_addr*/, arg1 /*size*/, (u_char *) arg2 /*start_addr*/ );
		break;

	case rSdVoiceTransStatus:
		ret = sceSdVoiceTransStatus (entry /*channel*/, value /*flag*/);
		*p_send_result = SEND_SOUND_RESULT_TRUE;
		break;

	case rSdBlockTransStatus:
		ret = sceSdBlockTransStatus (entry /*channel*/, value /*flag*/);
		*p_send_result = SEND_SOUND_RESULT_TRUE;
		break;

	case rSdSetEffectAttr:
		{
			sceSdEffectAttr attr;

			attr.core		= entry;
			attr.mode		= value;
			attr.depth_L	= (short) (arg0 >> 16);
			attr.depth_R	= (short) (arg0 & 0xFFFF);
			attr.delay		= arg1;
			attr.feedback	= arg2;

			//Kprintf("About to set core %d reverb mode to %d\n", entry, value);
			ret = sceSdSetEffectAttr(entry /*core*/, &attr);

			// Wait for Stream DMA (0) to finish
			while (gUsingStreamDMA)
				;

			// Clear out reverb memory (and make sure it doesn't interfere with streams)
			gSPUUsingStreamDMA = 1;
            sceSdCleanEffectWorkArea(entry, 0, value);
            sceSdVoiceTransStatus(0, SD_TRANS_STATUS_WAIT);
			if ((value == SD_REV_MODE_OFF) && gSPUUsingStreamDMA)		// This mode doesn't seem to cause a DMA callback
			{
				gSPUUsingStreamDMA = 0;
				//Kprintf("Manually clearing DMA flag\n");
			}
			//Kprintf("Set core %d reverb mode to %d with return value of %d and %d\n", entry, value, ret);
		}
		*p_send_result = SEND_SOUND_RESULT_TRUE;
		break;

		// This is a meaningless call because we can't send the data back
//	case rSdGetEffectAttr:
//		sceSdGetEffectAttr(entry /*core*/, (sceSdEffectAttr *) value /*attr*/);
//		break;

//#define rSdClearEffectWorkArea   0x8150
//#define rSdStopTrans             0x8180
//#define rSdCleanEffectWorkArea   0x8190
//#define rSdSetEffectMode         0x81a0
//#define rSdSetEffectModeParams   0x81b0

	default:
		ret = -1;
		*p_send_result = SEND_SOUND_RESULT_PANIC;
		Kprintf("Unknown or unsupported SPU function %x\n", function);
		break;
	}

    return ret;
}

// SifCmd variables

// Note these can be changed in an interrupt
#define NUM_SOUND_REQUESTS	(128)

static volatile SSoundRequest SoundRequestArray[NUM_SOUND_REQUESTS];
static volatile int FirstSoundRequest;		// Interrupt only reads this value.
static volatile	int FreeSoundRequest;		// This is the main variable that changes in the interrupt

// Set to TRUE if we hit a problem
static volatile int global_sound_panic = FALSE;

static SSifCmdSoundResultPacket SoundResult;

void sound_request_callback(void *p, void *q);

int sce_sound_loop( void )
{
	int oldStat;
	int last_cmd_id = -1;

	// Create semaphore to signal command came in
	struct SemaParam sem;
	sem.initCount = 0;
	sem.maxCount = 1;
	sem.attr = AT_THFIFO;
	gSndSem = CreateSema (&sem);

	// Init the stream queue
	FirstSoundRequest = 0;
	FreeSoundRequest = 0;

	// set local buffer & functions
	CpuSuspendIntr(&oldStat);

	// SIFCMD
	// No longer need to call sceSifSetCmdBuffer() since we share it with fileio.irx
	sceSifAddCmdHandler(SOUND_REQUEST_COMMAND, (void *) sound_request_callback, NULL );

	CpuResumeIntr(oldStat);

	// The loop
	while (1) {
		//printf("waiting for sound command semaphore\n");
		WaitSema(gSndSem);		// Get signal from callback
		//printf("got sound command semaphore\n");

		// Note that FreeSoundRequest can change in the interrupt at any time
		// Also, FirstSoundRequest is examined in the interrupt, but just to make sure we don't overflow the buffer
		//Dbg_Assert(FreeSoundRequest != FirstSoundRequest);
		while (FreeSoundRequest != FirstSoundRequest)
		{
			int ret;
			ESendSoundResult send_result;

			volatile SSoundRequest *p_request = &(SoundRequestArray[FirstSoundRequest]);
			//Kprintf("Sound: %d executing command %x with entry %x and value %x\n", FirstSoundRequest, p_request->m_function, p_request->m_entry, p_request->m_value);
			ret = execute_sound_request(p_request->m_function, p_request->m_entry, p_request->m_value,
										p_request->m_arg_0, p_request->m_arg_1, p_request->m_arg_2, &send_result);

			if ((send_result != SEND_SOUND_RESULT_FALSE) || global_sound_panic)
			{
				//Kprintf("Sending sound result from command %x with entry %x and result %x\n", p_request->m_function, p_request->m_entry, ret);

				// Send the result back
				if (last_cmd_id >= 0)	// Wait for previous send to complete (it should already be done, though)
				{
					while(sceSifDmaStat(last_cmd_id) >= 0)
						;
				}

				// Gotta copy the function and entry
				SoundResult.m_function	= p_request->m_function;
				SoundResult.m_entry 	= p_request->m_entry;
				SoundResult.m_result	= ret;
				SoundResult.m_panic		= (send_result == SEND_SOUND_RESULT_PANIC) || global_sound_panic;
				last_cmd_id = sceSifSendCmd(SOUND_RESULT_COMMAND, &SoundResult, sizeof(SoundResult), 0, 0, 0);
			}

			// Increment request index; Note that interrupt can look at this
			FirstSoundRequest = (FirstSoundRequest + 1) % NUM_SOUND_REQUESTS;
		}

		// Send the status back, if it has been updated
		if (s_send_spu_status)
		{
			int core;

			//Kprintf("Sending sound status back\n");

			for (core = 0; core < NUM_CORES; core ++)
			{
				if (last_cmd_id >= 0)	// Wait for previous send to complete
				{
					while(sceSifDmaStat(last_cmd_id) >= 0)
						;
				}

				// Gotta copy the function and entry
				SoundResult.m_function	= rSdGetSwitch;
				SoundResult.m_entry 	= SD_S_ENDX | core;
				SoundResult.m_result	= s_spu_reg_endx[core];
				SoundResult.m_panic		= FALSE;
				last_cmd_id = sceSifSendCmd(SOUND_RESULT_COMMAND, &SoundResult, sizeof(SoundResult), 0, 0, 0);
			}
			s_send_spu_status = FALSE;
		}
	}

    return 0;
}

void sound_request_callback(void *p, void *q)
{
	SSifCmdSoundRequestPacket *h = (SSifCmdSoundRequestPacket *) p;

	// Check to make sure we can add
	int nextFreeReq = (FreeSoundRequest + 1) % NUM_SOUND_REQUESTS;
	if (nextFreeReq == FirstSoundRequest)
	{
		// We can't allow a request to be ignored.  We must abort
		Kprintf("Fuck, we've overrun the sound request buffer\n");
		global_sound_panic = TRUE;
	}

#if PRINT_DROPPED_REQUESTS
	if ((++s_received_requests) != h->m_request.m_pad[0])
	{
		Kprintf("ERROR: Missed sound request.  Expected %d but only got up to %d\n", h->m_request.m_pad[0], s_received_requests);
		s_received_requests = h->m_request.m_pad[0];
	}
#endif PRINT_DROPPED_REQUESTS

	// Copy the request into the reuest queue
	SoundRequestArray[FreeSoundRequest] = h->m_request;
	FreeSoundRequest = nextFreeReq;

	//Kprintf("Sound: %d got command %x with entry %x and value %x\n", FreeSoundRequest, h->m_request.m_function, h->m_request.m_entry, h->m_request.m_value);

	// And wake up the dispatch thread
	iSignalSema(gSndSem);

	return;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
