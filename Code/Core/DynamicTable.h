/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core library											**
**																			**
**	Module:			DynamicTable      			 							**
**																			**
**	File name:		Core\DynamicTable.h										**
**																			**
**	Created by:		12/14/2000 - rjm						                **
**																			**
**	Description:	A handy DynamicTable class				                **
**																			**
*****************************************************************************/

#ifndef __CORE_LIST_DYNAMICTABLE_H
#define __CORE_LIST_DYNAMICTABLE_H


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace Lst
{

						


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

nTemplateBaseClass(_V, DynamicTable)
{
    Dbg_TemplateBaseClass(_V, DynamicTable);

public:
	DynamicTable(int size, int grow_increment = 8);
	~DynamicTable();

	// it goes on the end of the table
	void Add(_V *pItem);
	int GetSize();
	void Reset();

	_V &Last();

	void Replace(int index, _V *pNewItem);
	void Remove(int index);

/*	
	bool IsEmpty(int index);
*/

	_V &operator[](int index);
	
private:
	_V **			mpp_table;
	int 			m_size;
	int				m_entriesFilled;
	int 			m_growIncrement;
};



template<class _V>
class DynamicTableDestroyer
{
public:
	DynamicTableDestroyer(DynamicTable<_V> *pTable);

	void DeleteTableContents();

private:
	DynamicTable<_V> *			mp_table;
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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template<class _V> 
DynamicTable<_V>::DynamicTable(int size, int grow_increment)
{
    m_size = size;
	m_growIncrement = grow_increment;
	mpp_table = (_V**) Mem::Malloc(sizeof(_V *) * m_size);
	//mpp_table = new _V*[m_size];
	for (int i = 0; i < m_size; i++)
		mpp_table[i] = NULL;
	
	m_entriesFilled = 0;
}



template<class _V> 
DynamicTable<_V>::~DynamicTable()
{
    Mem::Free(mpp_table);
	//delete [] mpp_table;
}



template<class _V> 
void DynamicTable<_V>::Add(_V* pItem)
{
	
	Dbg_MsgAssert(pItem,( "can't add NULL item to table"));

	// do we need to create a new table?
	if (m_size <= m_entriesFilled)
	{
		// create new table
		int new_size = m_size + m_growIncrement;
		_V **ppTemp = (_V**) Mem::Malloc(sizeof(_V *) * new_size);
		//_V **ppTemp = new _V*[new_size];

		// copy contents of old one
		int i;
		for (i = 0; i < m_size; i++)
			ppTemp[i] = mpp_table[i];
		for (i = m_size; i < new_size; i++)
			ppTemp[i] = NULL;

		Mem::Free(mpp_table);
		//delete [] mpp_table;
		mpp_table = ppTemp;
		m_size = new_size;
	}
	
	mpp_table[m_entriesFilled++] = pItem;
}



template<class _V> 
int DynamicTable<_V>::GetSize()
{
	return m_entriesFilled;
}



template<class _V> 
void DynamicTable<_V>::Reset()
{
	m_entriesFilled = 0;
}



template<class _V> 
void DynamicTable<_V>::Replace(int index, _V *pNewItem)
{
	
	Dbg_MsgAssert(index >= 0 && index < m_entriesFilled,( "index out of bounds in DynamicTable"));
	Dbg_AssertPtr(pNewItem);
	mpp_table[index] = pNewItem;
}


template<class _V> 
void DynamicTable<_V>::Remove(int index)
{
	Dbg_MsgAssert(index >= 0 && index < m_entriesFilled,( "index out of bounds in DynamicTable"));
	
	delete mpp_table[index];
	
	for ( int i = index; i < m_entriesFilled-1; i++ )
	{
		mpp_table[i] = mpp_table[i+1];
	}
	mpp_table[m_entriesFilled-1] = NULL;
	m_entriesFilled--;
}

/*
template<class _V> 
bool DynamicTable<_V>::IsEmpty(int index)
{
	
	Dbg_MsgAssert(index >= 0 && index < m_entriesFilled,( "index out of bounds in DynamicTable"));
	if (mpp_table[index]) return false;
	return true;
}
*/



template<class _V> 
_V &DynamicTable<_V>::operator[](int index)
{
	
	Dbg_MsgAssert(index >= 0 && index < m_entriesFilled,( "index %d out of bounds in DynamicTable", index));
	Dbg_MsgAssert(mpp_table[index],( "table entry is NULL"));
	Dbg_AssertPtr(mpp_table[index]);

	return *mpp_table[index];
}



template<class _V> 
_V &DynamicTable<_V>::Last()
{
	
	Dbg_MsgAssert(m_entriesFilled > 0,( "empty list"));

	return *mpp_table[m_entriesFilled-1];
}



template<class _V> 
DynamicTableDestroyer<_V>::DynamicTableDestroyer(DynamicTable<_V> *pTable)
{
	mp_table = pTable;
}



template<class _V> 
void DynamicTableDestroyer<_V>::DeleteTableContents()
{
	int size = mp_table->GetSize();
	for (int i = 0; i < size; i++)
	{
		//if (!mp_table->IsEmpty())
			delete &(*mp_table)[i];
	}
	mp_table->Reset();
}



} // namespace Lst

#endif	// __CORE_LIST_DYNAMICTABLE_H


