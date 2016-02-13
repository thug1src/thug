////////////////////////////////////////////////////////////
// gap.h

#ifndef	__GAP_H__
#define	__GAP_H__

#define	__USE_MJB_LIST__
	  
#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __CORE_MATH_H
#include <core/math.h>
#endif

#include <core/list.h>
#include <core/string/cstring.h>

#include <gel/scripting/array.h>

namespace Script
{
	class CStruct;
	class CScript;
}

namespace Obj
{

#define	CANCEL_GROUND				0x00000001
#define	CANCEL_AIR              	0x00000002
#define	CANCEL_RAIL             	0x00000004
#define	CANCEL_WALL             	0x00000008
#define	CANCEL_LIP              	0x00000010
#define	CANCEL_WALLPLANT			0x00000020
#define	CANCEL_MANUAL				0x00000040
#define CANCEL_HANG     	    	0x00000080
#define CANCEL_LADDER	         	0x00000100
#define CANCEL_SKATE				0x00000200
#define CANCEL_WALK					0x00000400
#define CANCEL_DRIVE				0x00000800

#define	CANCEL_MASK					0x0000FFFF

#define	REQUIRE_GROUND				0x00010000
#define	REQUIRE_AIR             	0x00020000
#define	REQUIRE_RAIL            	0x00040000
#define	REQUIRE_WALL            	0x00080000
#define	REQUIRE_LIP             	0x00100000
#define	REQUIRE_WALLPLANT			0x00200000
#define	REQUIRE_MANUAL				0x00400000
#define REQUIRE_HANG	        	0x00800000
#define REQUIRE_LADDER	        	0x01000000
#define REQUIRE_SKATE				0x02000000
#define REQUIRE_WALK				0x04000000
#define REQUIRE_DRIVE				0x08000000
                                
#define	REQUIRE_MASK				0xFFFF0000


class CGapCheck : public Lst::Node< CGapCheck >
{
public:
						CGapCheck();
	int					Got();
	void				GetGap();
	
	void				GetGapPending();
	void				UnGetGapPending();
	int					GotGapPending();
	
	void				SetInfoForCamera(Mth::Vector pos, Mth::Vector dir);
	Mth::Vector&		GetSkaterStartPos() {return m_skater_start_pos;}
	Mth::Vector&		GetSkaterStartDir() {return m_skater_start_dir;}
	
	
	void				UnGetGap();
	const char	*		GetText();
	void				SetText(const char *p);
	void				SetCount(int count);
	void				SetScore(int score) {m_score = score;}
	int					GetScore() {return m_score;};
	void 				SetValid(bool valid);

	void				Reset();
private:
	Mth::Vector			m_skater_start_pos;
	Mth::Vector			m_skater_start_dir;
	
	int					m_got_pending;			// number of times we have got this gap, but the landing of the combo is still pending
	int					m_got;					// number of times we have got this gap
	int					m_score;				// score for this gap
	bool				m_valid;				// flag used when parsing gaps to remove any that no longer exist (from an old save)
	Str::String			m_name;
};



// CGapChecklist contains a list of gaps for the level
// these gaps (CGap) count how many times they have been got,
// and so this list is used to display the in-game gap checklist
// there is one of these for each level in an array in CSkaterCareer
// 
class CGapChecklist 
{

public:
										CGapChecklist();
	void 								FindGaps();
	void 								GetGapByText(const char *p_text);
	int									GetGapChecklistIndex(const char *p_name);
	void								SetInfoForCamera(const char *p_name, Mth::Vector pos, Mth::Vector dir);
	void 								AwardPendingGaps(); 
	void 								AwardAllGaps();
	void 								ClearPendingGaps();
	bool 								ScriptAddGapsToMenu( Script::CStruct* pParams, Script::CScript* pScript );
	bool								ScriptCreateGapList( Script::CStruct* pParams, Script::CScript* pScript );
	
	bool								ScriptGotGap( Script::CStruct* pParams, Script::CScript* pScript );
	bool								ScriptGetLevelGapTotals( Script::CStruct* pParams, Script::CScript* pScript );
	
	bool								IsValid() {return m_valid;}
	int									NumGaps();

	const char *						GetGapText(int gap);
	int									GetGapCount(int gap);
	int									GetGapScore(int gap);
	void								GetSkaterStartPosAndDir(int gap, Mth::Vector *p_skaterStartPos, Mth::Vector *p_skaterStartDir);

	void								AddGapCheck(const char *p_name, int count, int score);
	void								FlushGapChecks();

	bool								GotAllGaps();
	void								Reset();

// semi-private, dammit...	
	void 								got_gap(Script::CStruct *pScriptStruct, const uint8 *pPC);
private:

	Obj::CGapCheck *					get_indexed_gapcheck(int gap_index);
	bool								m_valid;			// true if we'd parsed the scripts for this level
	Lst::Head< Obj::CGapCheck >			m_gapcheck_list;	// list of unique gaps in level, or NULL if none
};

// CGap is a gap we are in the middle of
class CGap : public Lst::Node< CGap >
{
	public:	
		CGap();
		~CGap();
	public:
		uint32			m_id;					// checksum of the gap's ID
		uint32			m_flags;				// bit fields saying what cancles it
		uint32			m_node;					// source node
        uint32          m_trickChecksum;
        int             m_numTrickOccurrences;
        uint32          m_trickscript;
		uint32			m_trickTextChecksum;
		int				m_spinMult;
		bool			m_requirePerfect;
		int				m_startGapTrickCount;
		int				m_numTaps;

		// K: The position & direction the skater is facing when the StartGap command was executed.
		// Used to determine a good camera position when showing the gap in the view-gaps menu.
		Mth::Vector		m_skater_start_pos;
		Mth::Vector		m_skater_start_dir;
				
		// these arrays are used for arrays of tricks that must
		// be completed in order to get the gap.
		Script::CArray* mp_tricks;
};

// CGaptrick contains list if gaps we are in the middle of, that contain tricks
class CGapTrick : public Lst::Node< CGapTrick >
{
public:
	CGapTrick();
    void                GetGapTrick();
public:
    Str::String         m_trickString;
    uint32  			m_id;                   // matches the gap's ID
	uint32	 			m_node;					// node this came from
	uint32	 			m_script;				// script to run when sucessful
    bool                m_got;
	uint32				m_frame;				// Logic frame number on which this was triggered
};

// Some gap utility functions
bool ScriptAddGapsToMenu( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptCreateGapList( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGetLevelGapTotals( Script::CStruct* pParams, Script::CScript* pScript );
bool ScriptGiveAllGaps( Script::CStruct* pParams, Script::CScript* pScript );

}  // end namespace obj














#endif
