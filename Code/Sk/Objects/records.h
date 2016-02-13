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
**	File name:		objects/records.h									   	**
**																			**
**	Created: 		11/7/01 - Mick 											**
**																			**
*****************************************************************************/
// Notes:
// as a bit of an experiment, I'm writing the whole structure using 
// dynamic containers,
// so each data type is more independent of the others
// but the constructor (and destructor) of the higher level types
// will have to create (and destroy) the dynamic instances of the others
//
// this gives us benefits, namely:
//   No cross includes needed between header files if we split into seperate classes
//   Number of elements in arrays (for high score tables) can be decided at run time.
//
// But has drawback:
//   Uses more memory, as each element is allocated individually
//   might be slower?
//   more code?  Dunno, that's why I'm experimenting....


#ifndef __OBJECTS_RECORDS_H
#define __OBJECTS_RECORDS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif


namespace Script
{
	class CStruct;
};

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Records
{

/*****************************************************************************
**							Forward Declarations						**
*****************************************************************************/

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

class  CInitials : public Spt::Class
{
	

	public:
		char *	Get(void);
		void	Set(const char *initials);				
	
	private:
		char 	m_initials[4];
		
};
				
class  CRecord  : public Spt::Class
{
	


	public:
								CRecord();
								~CRecord();
		
		// for memory card loading/saving
		void 						ReadFromStructure(Script::CStruct *pIn);
		void 						WriteIntoStructure(Script::CStruct *pIn);

		void						Set(char *initials, int value, int number);
		void						Set(CRecord *pRecord);
		
		void						SetInitials(char *initials);
		void						SetValue(int value);
		void						SetNumber(int number);
		void						SetNewRecord(bool new_record);
		
		char 	*					GetInitials(void);	// note, returns a pointer to the initials, not the initials itself
		int							GetValue(void);
		int							GetNumber(void);
		bool						GetNewRecord(void);	

		// Return true if we beat the record		
		bool						BeatRecord(int value);		

		// set the record and return true if we beat it
		bool						MaybeNewRecord(int value, int number);
		void 						UpdateInitials(CInitials* pInitials);
		


	private:
		CInitials		*			mp_initials;		// Initials
		int							m_value;		 	// 
		int							m_number;			// pro skater number (or could be used for anything)
		bool						m_new_record;		// set if it's a new record
};


class  CRecordTable  : public Spt::Class
{
	public:
									CRecordTable(int num_records);
									~CRecordTable();
		CRecord*   					GetRecord(int line);
		int		   					GetSize(void);
		int							MaybeNewEntry(int value, int number);
		// for memory card loading/saving
		void 						ReadFromStructure(Script::CStruct *pIn);
		void 						WriteIntoStructure(Script::CStruct *pIn);
		void 						UpdateInitials(CInitials* pInitials);

	private:		
		int							m_numRecords;		// Number of records
		CRecord		*				mp_records;			// pointer to records					
};


// CLevelRecords - Records for a particualr level
// Include table of high scores, and combo scores
// and individual records for other things like longest grind 
class  CLevelRecords  : public Spt::Class
{
	public:
									CLevelRecords(/*int numHighScores, int numBestCombos*/);
									~CLevelRecords();
		
		void 						UpdateInitials(CInitials* pInitials);
		
		// for memory card loading/saving
		void 						ReadFromStructure(Script::CStruct *pIn);
		void 						WriteIntoStructure(Script::CStruct *pIn);
	
	private:
		CRecordTable		*		mp_highScores;
		CRecordTable		*		mp_bestCombos;
		CRecord				*		mp_longestGrind;
		CRecord				*		mp_longestManual;
		CRecord				*		mp_longestLipTrick;
		CRecord				*		mp_longestCombo;	
	public:
		CRecordTable		*		GetHighScores()      {return mp_highScores;}        
		CRecordTable		*		GetBestCombos()      {return mp_bestCombos;}        
		CRecord				*		GetLongestGrind()    {return mp_longestGrind;}      
		CRecord				*		GetLongestManual()   {return mp_longestManual;}     
		CRecord				*		GetLongestLipTrick() {return mp_longestLipTrick;}   
		CRecord				*		GetLongestCombo()	 {return mp_longestCombo;}      
				
			
};

// Records for the whole game
// stores a single CLevelRecords for each level, in an array
// also stores the default initials used by the game records
								  
class CGameRecords : public Spt::Class
{
	public:
									CGameRecords(int numLevels /*, int numHighScores, int numBestCombos*/);
									~CGameRecords();
									
		CInitials 			* 		GetDefaultInitials(void);
		void				 		SetDefaultInitials(const char *p_initials);
		
		CLevelRecords 		*	 	GetLevelRecords(int level);
									
		// for memory card loading/saving
		void 						ReadFromStructure(Script::CStruct *pIn);
		void 						WriteIntoStructure(Script::CStruct *pIn);
	
	private:
		int							m_numLevels;
		CInitials			*		mp_defaultInitials;
		CLevelRecords		*		mp_levelRecords;
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

} // namespace Records

#endif	// __OBJECTS_RECORDS_H

