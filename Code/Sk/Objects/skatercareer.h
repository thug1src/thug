/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Object (OBJ)											**
**																			**
**	File name:		objects/SkaterCareer.h								   	**
**																			**
**	Created: 		21/5/01 - Mick (Based on SkaterApperance.h by gj)		**
**																			**
*****************************************************************************/

#ifndef __OBJECTS_SKATERCAREER_H
#define __OBJECTS_SKATERCAREER_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

namespace Script
{
	class CStruct;
}	


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Obj
{

/*****************************************************************************
**							Forward Declarations						**
*****************************************************************************/

class	CGapChecklist;

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

class  CSkaterCareer  : public Spt::Class
{
	

	public:
		enum
		{
			vVERSION_NUMBER = 0x00000001,
			vMAX_CAREER_LEVELS = 21, 		// changes in this are handled automatically, so don't change the version number
			vMAX_GLOBAL_FLAGS = 16,
			vMAGIC_NUMBER = 0x84751129,		// magic terminator, for integrity check
			vNUM_FLAG_BITS = 64,			// used for assertions
		};
	
		CSkaterCareer();
		virtual						~CSkaterCareer();

	public:
		// for memory card loading/saving
		uint32						WriteToBuffer(uint8* pBuffer, uint32 bufferSize);
		uint8*						ReadFromBuffer(uint8* pBuffer);
		void 						ReadFromStructure(Script::CStruct *pIn);
		void 						WriteIntoStructure(Script::CStruct *pIn);

		// Initialize the career to "Nothing Done Yet" 
		bool						Init( bool init_gaps = true );

		// Reset's the career to "Nothing Done Yet"
		bool						Reset( void );


		// Start the level, setting the default level number
		// and remembering which goals and flags were set at the start of this level
		void						StartLevel(int level = -1);
		
		// Access function for Level. Added by Ken, used by CareerLevelIs script command.
		int							GetLevel();

		// marks a level visited
		void						MarkLevelVisited( int level_num );
		bool						HasVisitedLevel( int level_num );

		// Access functions for goals and flags
		void						SetGoal(int goal, int level = -1);
		void						UnSetGoal(int goal, int level = -1);
		bool						GetGoal(int goal, int level = -1);
		int							CountGoalsCompleted(int level = -1);
		bool						JustGotGoal(int goal);
		
		void						SetFlag(int flag, int level = -1);
		void						UnSetFlag(int flag, int level = -1);
		bool						GetFlag(int flag, int level = -1);
		
		bool						GetCheat(uint32 cheat_checksum);
		
		void						SetGlobalFlag(int flag);
		void						UnSetGlobalFlag(int flag);
		bool						GetGlobalFlag(int flag);

		bool						JustGotFlag(int flag);
		
		
		int							CountTotalGoalsCompleted();
		int							CountMedals();
		int 						GetBestMedal(int Level);
		
		Obj::CGapChecklist	*		GetGapChecklist(int level = -1);
		bool						GotAllGaps();


	public:
		
	protected:
		uint32						m_goal_flags[vMAX_CAREER_LEVELS][8];
		uint32						m_level_flags[vMAX_CAREER_LEVELS][8];
		uint32						m_start_goal_flags[8];
		uint32						m_start_level_flags[8];
		
		uint32						m_global_flags[vMAX_GLOBAL_FLAGS];		// 8x32 = 256 global flags
		
		int							m_current_level;
		
		Obj::CGapChecklist		*   mp_gap_checklist[vMAX_CAREER_LEVELS];
		bool						m_level_visited[vMAX_CAREER_LEVELS];
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

} // namespace Obj

#endif	// __OBJECTS_SKATERCAREER_H
