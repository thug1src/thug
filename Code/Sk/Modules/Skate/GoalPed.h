// goal ped!

#ifndef __MODULES_SKATE_GOALPED_H
#define __MODULES_SKATE_GOALPED_H

#include <sk/modules/skate/goalmanager.h>
#include <sk/modules/skate/goal.h>

#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>

namespace Game
{

const uint32	MAX_STREAM_NAME_LENGTH = 256;
	
class CGoalPed
{
public:
	CGoalPed( Script::CStruct* pParams );
	~CGoalPed();
	
	void				DestroyGoalPed();
	
	void		 		GetPlayerFirstName( char* p_first_name, int buffer_size );
	const char*			GetProFirstName();
	bool				GetStreamChecksum( uint32* p_streamChecksum, int cam_anim_index = -1, bool success = false, const char* p_speaker_name = NULL, bool last_anim = false);
	void				LoadFam( uint32 stream_checksum, uint32 reference_checksum );
	void				UnloadLastFam();
	void				StopLastStream();
	void				PlayGoalStream( uint32 stream_checksum, bool use_pos_info = true, uint32 speaker_obj_id = 0, bool should_load_fam = false );
	void				PlayGoalStream( const char* type, Script::CStruct* pParams );
	void				PlayGoalStartStream( Script::CStruct* pParams );
	void				StopCurrentStream();
	void				PlayGoalProStream( const char* p_stream_type, int max_number_streams, bool use_player_name, bool use_pos_info = true );
	void				PlayGoalWinStream( Script::CStruct* pParams );
	void				PlayGoalWaitStream();
	void				PlayGoalMidStream();

	uint32				GetGoalAnimations( const char* type );

	bool				ProIsCurrentSkater();

	void				SuspendGoalLogic( bool suspend = true );

	Script::CStruct*	GetGoalParams();

	void				Hide( bool hide );
protected:
	uint32				m_goalId;
	bool				m_goalLogicSuspended;
	uint32				m_lastFam;
	uint32				m_lastFamObj;
	uint32				m_lastStreamId;
};


} // namespace Game

#endif // __MODULES_SKATE_GOALPED_H
