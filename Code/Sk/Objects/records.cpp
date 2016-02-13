/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Objects	(Record)										**
**																			**
**	File name:		objects/Record.cpp										**
**																			**
**	Created: 		7/11/01	- Mick											**
**																			**
**	Description:	Record Handling, high score tables and suchlike			**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <objects/records.h>
#include <gel/scripting/script.h> 
#include <gel/scripting/struct.h> 
#include <gel/scripting/array.h> 

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/



namespace Records
{
	

/*****************************************************************************
**								  Externals									**
*****************************************************************************/


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

		enum {
					vNumHighScores = 5,
					vNumBestCombos = 5,
			 };

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/


/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CInitials::Set(const char *initials) // K: Had to add a const so I could pass it a const got from GetText
{
	

	m_initials[0] = initials[0];
	m_initials[1] = initials[1];
	m_initials[2] = initials[2];
	m_initials[3] = 0;

}

char * CInitials::Get(void)
{
	

	return m_initials;			// note, returning a pointer

}



CRecord::CRecord()
{
	
	
	mp_initials = new CInitials();
	
	Set("XXX",1000,0); 			// set a default.  Will not hurt anything
	
}

CRecord::~CRecord()
{
	
	
	delete mp_initials;
}


// SetInitials will copy in the first three digits of the string   
void	CRecord::SetInitials(char *initials)
{
	
	mp_initials->Set(initials);

}

void	CRecord::SetValue(int value)
{
	
	m_value = value;

}

void	CRecord::SetNumber(int number)
{
	
	m_number = number;

}

void	CRecord::SetNewRecord(bool new_record)
{
	
	m_new_record = new_record;

}


void	CRecord::Set(char *initials, int value, int number)
{
	
	
	SetInitials(initials);
	SetValue(value);
	SetNumber(number);
}

void	CRecord::Set(CRecord *pRecord)
{
	
	
	SetInitials(pRecord->GetInitials());
	SetValue(pRecord->GetValue());
	SetNumber(pRecord->GetNumber());
}


char *	CRecord::GetInitials(void)	// note, returns a pointer to the initials, not the initials itself
{
	
	return mp_initials->Get();
}

int	   CRecord::GetValue(void)
{
	
	
	return m_value;
}

int	   CRecord::GetNumber(void)
{
	
	
	return m_number;

}

bool	   CRecord::GetNewRecord(void)
{
	
	
	return m_new_record;

}


bool  CRecord::BeatRecord(int value)		
{
	
	
	return value > GetValue();
}

bool  CRecord::MaybeNewRecord(int value, int number)		
{
	
	
	if (BeatRecord(value))
	{
		SetValue(value);
		SetNumber(number);
		SetNewRecord(true);
		return true;
	}
	else
	{
		SetNewRecord(false);
		return false;
	}
}


// if this was flagged as a new record, then set it to these initials
void 	CRecord::UpdateInitials(CInitials* pInitials)
{
	if (GetNewRecord())
	{
//		SetNewRecord(false);
		mp_initials->Set(pInitials->Get());
	}
}



void CRecord::WriteIntoStructure(Script::CStruct *pIn)
{
	

	Dbg_MsgAssert(pIn,("NULL pIn sent to CRecord::WriteIntoStructure"));

	pIn->AddComponent(Script::GenerateCRC("Value"),ESYMBOLTYPE_INTEGER,m_value);
	pIn->AddComponent(Script::GenerateCRC("Number"),ESYMBOLTYPE_INTEGER,m_number);
	Dbg_MsgAssert(mp_initials,("NULL mp_initials"));
	pIn->AddComponent(Script::GenerateCRC("Initials"),ESYMBOLTYPE_STRING,mp_initials->Get());
}

void CRecord::ReadFromStructure(Script::CStruct *pIn)
{
	
	Dbg_MsgAssert(pIn,("NULL pIn sent to CRecord::ReadFromStructure"));
	
	if (!pIn->GetInteger("Value",&m_value))
	{
		Dbg_MsgAssert(0,("Structure is missing Value parameter"));
	}	
	if (!pIn->GetInteger("Number",&m_number))
	{
		Dbg_MsgAssert(0,("Structure is missing Number parameter"));
	}	
	
	const char *pInitials="XXX";
	if (!pIn->GetText("Initials",&pInitials))
	{
		Dbg_MsgAssert(0,("Structure is missing Initials parameter"));
	}
	Dbg_MsgAssert(mp_initials,("NULL mp_initials"));
	mp_initials->Set(pInitials);
}



///////////////////////////////////////////////////////////////////////////////
// CRecordTable, a table of records, like a high score table
CRecordTable::CRecordTable(int num_records)
{
	
	
	m_numRecords = num_records;
	mp_records = new CRecord[m_numRecords];
}
			
CRecordTable::~CRecordTable()
{
	

	delete [] mp_records;
	
}

CRecord*   CRecordTable::GetRecord(int line)
{
	
	
	Dbg_MsgAssert(line < m_numRecords,("Bad Line %d in recordtable",line));
	return &mp_records[line];
}

int			CRecordTable::GetSize(void)
{
	
	
	return m_numRecords;	
}


void 	CRecordTable::UpdateInitials(CInitials* pInitials)
{
	for (int line= 0; line < m_numRecords; line++)
	{
		mp_records[line].UpdateInitials(pInitials);
	}	
}

// given a new "value", then insert it into the table in the position
// that matches this value, and move other entries down (losing the final entry)
// returns the entery number (fisr tline is 0)
// or -1 if it does not make the cut.
int			CRecordTable::MaybeNewEntry(int value, int number)
{
	

	int line;
	
	for (line= 0; line < m_numRecords; line++)
	{
		mp_records[line].SetNewRecord(false);
	}

	for (line= 0; line < m_numRecords; line++)
	{
		if (value > mp_records[line].GetValue())
		{
			// alright! this value belongs at "line"
			// so maove down any that are below this line
			// and insert this one
			for (int dest = m_numRecords - 1; dest > line; dest--)
			{
				mp_records[dest].Set(&mp_records[dest-1]);
			}
			mp_records[line].SetValue(value);	
			mp_records[line].SetNumber(number);	
			mp_records[line].SetInitials("XXX");	
			mp_records[line].SetNewRecord(true);
			return line;		
		}
	}

	return -1;
}


			
void 		CRecordTable::ReadFromStructure(Script::CStruct *pIn)
{
	

	Dbg_MsgAssert(pIn, ("NULL pIn sent to CRecordTable::ReadFromStructure"));
	Script::CArray *pArray=NULL;
	pIn->GetArray("RecordTable",&pArray);
	Dbg_MsgAssert(pArray,("Structure is missing RecordTable parameter"));
	
	if (m_numRecords!=(int)pArray->GetSize())
	{
		Dbg_Warning("m_numRecords mismatch: m_numRecords=%d, but num records in structure=%d",m_numRecords,pArray->GetSize());
		return;
	}
	Dbg_MsgAssert(mp_records,("NULL mp_records"));
		
	for (int i=0; i<m_numRecords; ++i)
	{
		mp_records[i].ReadFromStructure(pArray->GetStructure(i));
	}	
}
			
void 		CRecordTable::WriteIntoStructure(Script::CStruct *pIn)
{
	

	Dbg_MsgAssert(pIn, ("NULL pIn sent to CRecordTable::WriteIntoStructure"));
	
	Script::CArray *pArray=new Script::CArray;
	pArray->SetArrayType(m_numRecords,ESYMBOLTYPE_STRUCTURE);

	Dbg_MsgAssert(mp_records,("NULL mp_records"));
	for (int i=0; i<m_numRecords; ++i)
	{
		Script::CStruct *pStruct=new Script::CStruct;
		mp_records[i].WriteIntoStructure(pStruct);
		pArray->SetStructure(i,pStruct);
	}

	pIn->AddComponent(Script::GenerateCRC("RecordTable"),ESYMBOLTYPE_ARRAY,(int)pArray);
}
			


///////////////////////////////////////////////////////////////////////////////
// CLevelRecords - Records for a particualr level (Skate3 specific)
CLevelRecords::CLevelRecords(/*int numHighScores, int numBestCombos*/)
{
	
	
	
   	mp_highScores = new CRecordTable(vNumHighScores);
	
   	mp_bestCombos = new CRecordTable(vNumBestCombos);
   	mp_longestCombo = new CRecord;
	mp_longestCombo->SetValue(0);
   	mp_longestGrind = new CRecord;
	mp_longestGrind->SetValue(0);
   	mp_longestLipTrick = new CRecord;
	mp_longestLipTrick->SetValue(0);
   	mp_longestManual = new CRecord;
	mp_longestManual->SetValue(0);
	
	
}
			
CLevelRecords::~CLevelRecords()
{
	
	delete mp_longestManual;
	delete mp_longestLipTrick;
	delete mp_longestGrind;
	delete mp_longestCombo;
	delete mp_bestCombos;
	delete mp_highScores;
}


void		CLevelRecords::UpdateInitials(CInitials* pInitials)
{
	mp_longestManual->UpdateInitials(pInitials);
	mp_longestLipTrick->UpdateInitials(pInitials);
	mp_longestGrind->UpdateInitials(pInitials);
	mp_longestCombo->UpdateInitials(pInitials);
	mp_bestCombos->UpdateInitials(pInitials);
	mp_highScores->UpdateInitials(pInitials);
	
}
 


			
void		CLevelRecords::ReadFromStructure(Script::CStruct *pIn)
{
	

	Dbg_MsgAssert(pIn,("NULL pIn sent to CLevelRecords::ReadFromStructure"));

	Script::CStruct *pStruct=NULL;
	
	pIn->GetStructure("HighScores",&pStruct);
	Dbg_MsgAssert(pStruct,("Structure is missing HighScores"));
   	Dbg_MsgAssert(mp_highScores,("NULL mp_highScores"));
	mp_highScores->ReadFromStructure(pStruct);

	pIn->GetStructure("BestCombos",&pStruct);
	Dbg_MsgAssert(pStruct,("Structure is missing BestCombos"));
   	Dbg_MsgAssert(mp_bestCombos,("NULL mp_bestCombos"));
	mp_bestCombos->ReadFromStructure(pStruct);

	pIn->GetStructure("LongestCombo",&pStruct);
	Dbg_MsgAssert(pStruct,("Structure is missing LongestCombo"));
   	Dbg_MsgAssert(mp_longestCombo,("NULL mp_longestCombo"));
	mp_longestCombo->ReadFromStructure(pStruct);

	pIn->GetStructure("LongestGrind",&pStruct);
	Dbg_MsgAssert(pStruct,("Structure is missing LongestGrind"));
   	Dbg_MsgAssert(mp_longestGrind,("NULL mp_longestGrind"));
	mp_longestGrind->ReadFromStructure(pStruct);

	pIn->GetStructure("LongestLipTrick",&pStruct);
	Dbg_MsgAssert(pStruct,("Structure is missing LongestLipTrick"));
   	Dbg_MsgAssert(mp_longestLipTrick,("NULL mp_longestLipTrick"));
	mp_longestLipTrick->ReadFromStructure(pStruct);

	pIn->GetStructure("LongestManual",&pStruct);
	Dbg_MsgAssert(pStruct,("Structure is missing LongestManual"));
   	Dbg_MsgAssert(mp_longestManual,("NULL mp_longestManual"));
	mp_longestManual->ReadFromStructure(pStruct);
}
			
void		CLevelRecords::WriteIntoStructure(Script::CStruct *pIn)
{
	
	
	Dbg_MsgAssert(pIn,("NULL pIn sent to CLevelRecords::WriteIntoStructure"));
	

	Script::CStruct *pStruct=new Script::CStruct;
   	Dbg_MsgAssert(mp_highScores,("NULL mp_highScores"));
	mp_highScores->WriteIntoStructure(pStruct);
	pIn->AddComponent(Script::GenerateCRC("HighScores"),ESYMBOLTYPE_STRUCTUREPOINTER,(int)pStruct);
	
	pStruct=new Script::CStruct;
   	Dbg_MsgAssert(mp_bestCombos,("NULL mp_bestCombos"));
	mp_bestCombos->WriteIntoStructure(pStruct);
	pIn->AddComponent(Script::GenerateCRC("BestCombos"),ESYMBOLTYPE_STRUCTUREPOINTER,(int)pStruct);
	
	pStruct=new Script::CStruct;
   	Dbg_MsgAssert(mp_longestCombo,("NULL mp_longestCombo"));
	mp_longestCombo->WriteIntoStructure(pStruct);
	pIn->AddComponent(Script::GenerateCRC("LongestCombo"),ESYMBOLTYPE_STRUCTUREPOINTER,(int)pStruct);
	
	pStruct=new Script::CStruct;
   	Dbg_MsgAssert(mp_longestGrind,("NULL mp_longestGrind"));
	mp_longestGrind->WriteIntoStructure(pStruct);
	pIn->AddComponent(Script::GenerateCRC("LongestGrind"),ESYMBOLTYPE_STRUCTUREPOINTER,(int)pStruct);
	
	pStruct=new Script::CStruct;
   	Dbg_MsgAssert(mp_longestLipTrick,("NULL mp_longestLipTrick"));
	mp_longestLipTrick->WriteIntoStructure(pStruct);
	pIn->AddComponent(Script::GenerateCRC("LongestLipTrick"),ESYMBOLTYPE_STRUCTUREPOINTER,(int)pStruct);
   	
	pStruct=new Script::CStruct;
	Dbg_MsgAssert(mp_longestManual,("NULL mp_longestManual"));
	mp_longestManual->WriteIntoStructure(pStruct);
	pIn->AddComponent(Script::GenerateCRC("LongestManual"),ESYMBOLTYPE_STRUCTUREPOINTER,(int)pStruct);
}
			


///////////////////////////////////////////////////////////////////////////////
// CGameRecords - Records for the whole game

CGameRecords::CGameRecords(int numLevels /*, int numHighScores, int numBestCombos*/)
{
	
	m_numLevels = numLevels;

	// VC++ complains about constructing an array with parameters
	// so I (Mick) changed it to use and enum for those values,
	// as they are not likely to change anyway  
	mp_levelRecords = new CLevelRecords[numLevels]; /*(numHighScores,numBestCombos)*/
	mp_defaultInitials = new CInitials;
	mp_defaultInitials->Set("ABC");
}
			
CGameRecords::~CGameRecords()
{
	
	delete [] mp_levelRecords;
	delete mp_defaultInitials;
	
}


CInitials * CGameRecords::GetDefaultInitials(void)
{
	

	return mp_defaultInitials;
}

void CGameRecords::SetDefaultInitials(const char *p_initials)
{
	

	mp_defaultInitials->Set( p_initials );
}



CLevelRecords * CGameRecords::GetLevelRecords(int level)
{
	
	Dbg_MsgAssert(level>=0 && level <= m_numLevels,("bad level number %d (max %d)\n",level,m_numLevels));
	
	return &mp_levelRecords[level];
}

			
void 		CGameRecords::ReadFromStructure(Script::CStruct *pIn)
{
	

	Dbg_MsgAssert(pIn, ("NULL pIn sent to CGameRecords::ReadFromStructure"));	

	Script::CArray *pArray=NULL;
	pIn->GetArray("GameRecords",&pArray);
	Dbg_MsgAssert(pArray,("Structure is missing GameRecords parameter"));
	
	if (m_numLevels!=(int)pArray->GetSize())
	{
		Dbg_Warning("m_numLevels mismatch: m_numLevels=%d, but num levels in structure=%d",m_numLevels,pArray->GetSize());
		return;
	}
	Dbg_MsgAssert(mp_levelRecords,("NULL mp_levelRecords"));
		
	for (int i=0; i<m_numLevels; ++i)
	{
		mp_levelRecords[i].ReadFromStructure(pArray->GetStructure(i));
	}	
	
	const char *pDefaultInitials="ABC";
	pIn->GetText("DefaultInitials",&pDefaultInitials);
	Dbg_MsgAssert(mp_defaultInitials,("NULL mp_defaultInitials"));
	mp_defaultInitials->Set(pDefaultInitials);
	
}
			
void 		CGameRecords::WriteIntoStructure(Script::CStruct *pIn)
{
	
	Dbg_MsgAssert(pIn, ("NULL pIn sent to CGameRecords::WriteIntoStructure"));	
	
	Script::CArray *pArray=new Script::CArray;
	pArray->SetArrayType(m_numLevels,ESYMBOLTYPE_STRUCTURE);

	Dbg_MsgAssert(mp_levelRecords,("NULL mp_levelRecords"));
	for (int i=0; i<m_numLevels; ++i)
	{
		Script::CStruct *pStruct=new Script::CStruct;
		mp_levelRecords[i].WriteIntoStructure(pStruct);
		pArray->SetStructure(i,pStruct);
	}

	pIn->AddComponent(Script::GenerateCRC("GameRecords"),ESYMBOLTYPE_ARRAY,(int)pArray);
	pIn->AddComponent(Script::GenerateCRC("DefaultInitials"),ESYMBOLTYPE_STRING,mp_defaultInitials->Get());
}
			






} // namespace Records


