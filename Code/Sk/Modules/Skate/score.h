/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2001 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:																**
**																			**
**	Module:									 								**
**																			**
**	File name:																**
**																			**
**	Created by:		rjm, 1/23/2001											**
**																			**
**	Description:						 									**
**																			**
*****************************************************************************/

#ifndef __MODULES_SKATE_SCORE_H
#define __MODULES_SKATE_SCORE_H

#include <core/defines.h>

#include <core/lookuptable.h>
#include <core/dynamictable.h>

#include <gel/scripting/vecpair.h>
#include <gel/scripting/array.h>
#include <gfx/Image/ImageBasic.h>


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Net
{
	class	Conn;
	class	MsgHandlerContext;
}

namespace Mdl
{

						


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  Score  : public Spt::Class
{
	

public:
	
	typedef uint32 Flags;
	enum
	{
		vBLOCKING 		= nBit(1),
		vSWITCH			= nBit(2),
		vNOLLIE			= nBit(3),
		vGAP			= nBit(4),
		vSPECIAL		= nBit(5),
		vNODEGRADE		= nBit(6),
		vCAT			= nBit(7),
	};

	enum
	{
		MAX_ROBOT_NODES = 2500
	};
									Score();
									~Score();

	void 							Update();
	void 							SetSkaterId(short skater_id);

	void 							SetTotalScore( int score );
	int 							GetTotalScore();
	int 							GetLastScoreLanded() {return m_recentScorePot;}
	int 							GetScorePotValue() {return m_scorePot * m_currentMult;}  // current pot value
	bool 							GetSpecialState() {return m_specialIsActive;}
	void 							ForceSpecial( void );	

	uint32							GetLastTrickId(); // Added by Ken
	uint32							GetTrickId( int trickIndex );

	void							TrickTextPulse(int index); // Added by Ken so that replay can use it
	void							TrickTextCountdown(int index); // Added by Ken so that replay can use it
	void							TrickTextLanded(int index); // Added by Ken so that replay can use it
	void							TrickTextBail(int index); // Added by Ken so that replay can use it
	void 							Trigger(char *trick_name, int base_score, Flags flags = 0);
	void 							SetSpin(int spin_degrees );
	void 							UpdateSpin(int spin_degrees );
	void 							TweakTrick(int tweak_value );
	void 							Land( void );
	void 							Bail( bool clearScoreText = false );

	void 							Reset();

	Net::Conn* 						GetAssociatedNetworkConnection( void );
	void 							LogTrickObject( int skater_id, int score, uint32 num_pending_tricks, uint32* p_pending_tricks, bool propagate );
	void 							LogTrickObjectRequest( int score );

	void 							SetBalanceMeter(bool state, float value = 0.0f);
	void 							SetManualMeter(bool state, float value = 0.0f);
	void							RepositionMeters( void );

	int								GetNumberOfNonGapTricks();
	
	bool							IsLatestTrickByName( uint32 trick_text_checksum, int spin_mult, bool require_perfect, int num_taps );
	bool							IsLatestTrick( uint32 key_combo, int spin_mult, bool require_perfect, int num_taps );

    // counting occurrences of trick
    int                             GetPreviousNumberOfOccurrencesByName( uint32 trickTextChecksum, int spin_degrees = 0, int num_taps = 1 );
    int                             GetPreviousNumberOfOccurrences( uint32 trick_checksum, int spin_degrees = 0, int num_taps = 1 );
//	int								GetPreviousNumberOfOccurrences( Script::CArray* pTricks );
    
    int                             GetCurrentNumberOfOccurrencesByName( uint32 trickTextChecksum, int spin_degrees = 0, bool require_perfect = false, int num_taps = 1 );
    int                             GetCurrentNumberOfOccurrences( uint32 trick_checksum, int spin_degrees = 0, bool require_perfect = false, int num_taps = 1 );
	int                             GetCurrentNumberOfOccurrences( Script::CArray* pTricks, int start_trick_index = 0 );

	bool							VerifyTrickMatch( int info_tab_index, uint32 trick_checksum, int spin_degrees = 0, bool require_perfect = false );
	int								CountTrickMatches( uint32 trick_checksum, int spin_degrees = 0, bool require_perfect = false );
    int								CountStringMatches( const char* string );

	int								GetCurrentTrickCount() { return m_currentTrick; }
    int								GetCurrentMult() { return m_currentMult; }

	// track the best and longest combos
	int								GetLongestCombo() { return m_longestCombo; }
	int								GetBestCombo() { return m_bestCombo; }
	void							SetBestCombo( int best_combo ) { m_bestCombo = best_combo; }
	void							ResetComboRecords();
    void 							setSpecialBarColors();

	bool							TrickIsGap( int trickIndex );
	
	static int						s_handle_score_message ( Net::MsgHandlerContext* context );

	void							UpdateRobotDetection(int node);
	float							GetRobotMult();
	float							GetRobotRailMult() {return m_robot_rail_mult;}
	

private:

	void							reset_robot_detection();
	void							reset_robot_detection_combo();

	int								get_packed_score( int start, int end );
	void							pack_trick_info_table();
	void							print_trick_info_table();
	void 							copy_trick_name_with_special_spaces(char *pOut, const char *pIn);
	
    void 							captureScore();
	void 							resetScorePot();
	void							dispatch_trick_sequence_to_screen(bool clear_text=false);
	int 							spinMult(int x);
	int 							deprecMult(int x);

	void							setup_balance_meter_stuff();
	void							position_balance_meter(bool state, float value, bool isVertical);

	void							dispatch_score_pot_value_to_screen(int score, int multiplier);
	
	void							check_high_score(int old_score, int new_score);
	
	void							set_special_is_active ( bool is_active );
	
	short							m_skaterId;

	enum EScorePotState
	{
		SHOW_ACTUAL_SCORE_POT,
		WAITING_TO_COUNT_SCORE_POT,
		SHOW_COUNTED_SCORE_POT,
	};
	
	int								m_totalScore;
	EScorePotState					m_scorePotState;
	int 							m_scorePot;
	int 							m_countedScorePot;
	int								m_scorePotCountdown;
	int    							m_recentScorePot;
	int    							m_recentSpecialScorePot;

	int								m_longestCombo;
	int								m_bestCombo;
	int								m_bestGameCombo;

	//HUD::TrickWindow*				mp_trickWindow;

	struct TrickHistory
	{
		uint32						id;
		// there are 4 switch modes, so 4 entries
		int							total_count[4]; 	// done during session 
		int	 						combo_count[4];		// done	during combo
	};
	Lst::LookupTable<TrickHistory>	m_historyTab;
	
	struct TrickInfo
	{
		enum
		{
			TEXT_BUFFER_SIZE		= 64,
		};
		
		int							score;
		int 						mult_index;
		int 						spin_mult_index;
		int							spin_degrees;
		uint32						id;
		uint32						switch_mode;
		Flags						flags;
		char						trickNameText[TEXT_BUFFER_SIZE];
		char						trickTextFormatted[TEXT_BUFFER_SIZE];
	};
	// keeps track of tricks done in current sequence
	Lst::DynamicTable<TrickInfo>	m_infoTab;
	int								m_currentTrick;	 // Tricks cap out at 250
	int								m_currentMult;	 // but multiplier will keep going
	// Indicates most recent blocking trick
	int								m_currentBlockingTrick;
	// Most recent spin trick. Must come after a blocking trick.
	// Can be a blocking trick itself, but will be treated differently
	// if so.
	int								m_currentSpinTrick;

	int								m_specialScore;
	bool							m_specialIsActive;

	enum
	{
		REGULAR_SPECIAL	=			0,
		SPECIAL_SPECIAL
	};
	Image::RGBA						m_special_rgba[3];
	float							m_special_interpolator;
	float							m_special_interpolator_rate;

	bool 							m_debug;

	// Balance meter stuff -- here for now
	int								m_numArrowPositions;
	Script::CPair 					m_arrowPosTab[20];
	float							m_arrowInterval;
	Script::CPair					m_arrowPos;
	float							m_arrowRot;
	Script::CPair					m_meterPos[2];
	
	uint8							m_robot_detection_count[MAX_ROBOT_NODES];
	uint8							m_robot_detection_count_combo[MAX_ROBOT_NODES];
	int								m_num_robot_landed;
	int								m_num_robot_landed_combo;
	int								m_num_robot_unique;
	int								m_num_robot_first_combo;
	int								m_num_robot_unique_combo;
	float							m_robot_rail_mult;
	
};



} // namespace Mdl

#endif	// __MODULES_SKATE_SCORE_H


