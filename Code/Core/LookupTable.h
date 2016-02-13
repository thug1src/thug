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
**	Module:			LookupTable      			 							**
**																			**
**	File name:		Core\LookupTable.h										**
**																			**
**	Created by:		9/22/2000 - rjm							                **
**																			**
**	Description:	A handy Lookuptable class				                **
**																			**
*****************************************************************************/

#ifndef __CORE_LIST_LOOKUPTABLE_H
#define __CORE_LIST_LOOKUPTABLE_H

#ifndef __CORE_CRC_H
#include <core/crc.h>
#endif

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

/****************************************************************************
 *																			
 * Class:			LookupItem
 *
 * Description:		Used to represent an item in a Lookup table.
 *
 *                  Usage:  
 *                          
 *                          
 *
 ****************************************************************************/

// forward declaration
template< class _V > class LookupTable;

//nTemplateBaseClass2(_K, _V, LookupItem)
template< class _V > class LookupItem
{
//    Dbg_TemplateBaseClass2(_K, _V, LookupItem);

	friend class LookupTable<_V>;
	
private:
	LookupItem();

	int 				m_key;
	_V *				mp_value;
	LookupItem<_V> *	mp_next;
};
    


nTemplateBaseClass(_V, LookupTable)
{
    Dbg_TemplateBaseClass(_V, LookupTable);

public:
	LookupTable(int size=0);	   	// Mick, size is not used, so give it default until we get rid of it
	~LookupTable();

	// if any item exists with the same key, replace it
	bool PutItem(const int &key, _V *item);
	// gets a pointer to requested item, returns NULL if item not in table
	_V *GetItem(const int &key);
	// gets a pointer to requested item, returns NULL if item not in table
	_V *GetItemByIndex(const int &index, int *pKey = NULL);
	int getSize() {return m_size;}

    // removes item from table, calls its destructor if requested
    void FlushItem(const int &key);
	void flushAllItems();

private:
	int 				m_size;
	LookupItem<_V> *	mp_list;		  // first item in list
	LookupItem<_V> *	mp_last;		  // last item in list
	LookupItem<_V> *    mp_current;		  // Pointer to current item in this table, NULL if invalid
	int					m_currentIndex;	  // the index of this item. Only valid if mp_current is not NULL 
};



template<class _V>
class StringLookupTable : public LookupTable<_V>
{
    //Dbg_TemplateBaseClass2(_K, _V, LookupItem);

public:
    StringLookupTable(int size);

    bool PutItem(const char *stringKey, _V *item);
    _V *GetItem(const char *stringKey);
    void FlushItem(const char *stringKey);
};



template<class _V>
class LookupTableDestroyer
{
public:
	LookupTableDestroyer(LookupTable<_V> *pTable);

	void DeleteTableContents();

private:
	LookupTable<_V> *			mp_table;
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

template<class _V> //inline
LookupItem<_V>::LookupItem()
{
    mp_value = NULL;
	mp_next = NULL;
}



template<class _V> //inline
LookupTable<_V>::LookupTable(int size=0) 
{
    m_size = 0;
	mp_list = NULL;
	mp_current = NULL;	   			// initialized invalid, so we don't try to use it
}

	

template<class _V> //inline
LookupTable<_V>::~LookupTable()
{
	flushAllItems();
}



template<class _V> //inline
bool LookupTable<_V>::PutItem(const int &key, _V *item)
{
    
    Dbg_AssertPtr(item);
    //Ryan("putting item in lookup table\n");

#	ifdef __NOPT_DEBUG__
	if (GetItem(key)) return false;
#	endif
	
	LookupItem<_V> *pItem = new LookupItem<_V>;
	pItem->mp_value = item;
	pItem->m_key = key;

	if (!mp_list)
		mp_list = pItem;
	else
		mp_last->mp_next = pItem;
	mp_last = pItem;	
	m_size++;
	mp_current = NULL;						// no longer valid
	
	return true;
}



template<class _V> //inline
_V *LookupTable<_V>::GetItem(const int &key)
{
    
    
	LookupItem<_V> *pItem = mp_list;
	mp_current = NULL;				  			// set invalid now, so if not found, then it will be correctly invalid  	
	m_currentIndex = 0;							// index should be 0, for the first item
	while(pItem)
	{
		if (pItem->m_key == key)
		{
			mp_current = pItem;			 		// we have a valid current, and hence index
			return pItem->mp_value;
		}
		m_currentIndex++;					    // update index, in case we find the item 
		pItem = pItem->mp_next;
	}
    
	//printf("returning NULL from GetItem()\n");
	
	// warning should be given by calling function, if necessary
    //Dbg_Warning("Item not found in lookup table");
    return NULL;
}



template<class _V> //inline
_V *LookupTable<_V>::GetItemByIndex(const int &index, int *pKey)
{
    
	// size must be at least 1
	Dbg_MsgAssert(index >= 0 && index < m_size,( "bad index %d", index));

	// if we have a valid mp_current, then check if we can use that (for speed)
	if (mp_current)
	{
		// is it the same as last time?
		if (m_currentIndex == index)
		{
			// It's this one, so just leave it alone
		}
		else if (m_currentIndex < index)
		{
			// need to step forward until we find the correct one
			// this is the most likely scenario if
			// we are using GetItemByIndex in a for loop
			// really we need an interator access class
			// but this should be similar, in terms of code speed
			while (m_currentIndex < index)
			{
				m_currentIndex++;
				mp_current = mp_current->mp_next;
			}
		}
		else
		{
			// otherwise, we need to start again, so invalidate mp_current
			mp_current = NULL;
		}
	}

	// if we don't have a valid mp_current at this point, then we need
	// to start searching from the start again
	if (!mp_current)
	{
		LookupItem<_V> *pItem = mp_list;
		for (int i = 0; i < index; i++)
		{
			pItem = pItem->mp_next;
		}
		m_currentIndex = index;
		mp_current = pItem;
	}

	// if they want us to returnt he key, then poke it into the supplied destination		
	if ( pKey )
		*pKey = mp_current->m_key;

	// the current value is correct	
	return mp_current->mp_value;
}



template<class _V> //inline
void LookupTable<_V>::FlushItem(const int &key)
{
    

	mp_current = NULL;			   					// No longer valid
    
	LookupItem<_V> *pItem = mp_list;
	LookupItem<_V> *prev = NULL; 
	while(pItem)
	{
		if (pItem->m_key == key)
		{
			if (prev)
				prev->mp_next = pItem->mp_next;
			else
				mp_list = pItem->mp_next;
			
			if (mp_last == pItem)
				mp_last = prev;
			
			delete pItem;
			m_size--;
			
			return;
		}
		prev = pItem;
		pItem = pItem->mp_next;
	}
}



template<class _V> //inline
void LookupTable<_V>::flushAllItems()
{
    
    
	LookupItem<_V> *pItem = mp_list;
	while(pItem)
	{
		LookupItem<_V> *pNext = pItem->mp_next;
		delete pItem;
		pItem = pNext;
	}
	mp_list = NULL;
	mp_current = NULL;	
	m_size = 0;
}

/*
int m_size;
LookupItem<_K, _V> *m_array;

_K m_key;
_V *mp_value;
*/



template<class _V> //inline
StringLookupTable<_V>::StringLookupTable(int size) :
    LookupTable<_V>(size)
{
}



template<class _V> //inline
bool StringLookupTable<_V>::PutItem(const char *stringKey, _V *item)
{
    int key = Crc::GenerateCRCFromString(stringKey);
    return LookupTable<_V>::PutItem(key, item);
}



template<class _V> //inline
_V *StringLookupTable<_V>::GetItem(const char *stringKey)
{
    int key = Crc::GenerateCRCFromString(stringKey);
    return LookupTable<_V>::GetItem(key);
}


template<class _V> //inline
void StringLookupTable<_V>::FlushItem(const char *stringKey)
{
    int key = Crc::GenerateCRCFromString(stringKey);
    LookupTable<_V>::FlushItem(key);
}



template<class _V> //inline
LookupTableDestroyer<_V>::LookupTableDestroyer(LookupTable<_V> *pTable)
{
	mp_table = pTable;
}



template<class _V> inline
void LookupTableDestroyer<_V>::DeleteTableContents()
{
	int num_items = mp_table->getSize();
	
	for (int i = 0; i < num_items; i++)
	{
		int key;
		// since we are flushing items out of the table as we go, 
		// we should always grab the first one
		_V *pItem = mp_table->GetItemByIndex(0, &key);
		mp_table->FlushItem(key);
		if (pItem) delete pItem;
	}
}



} // namespace Lst

#endif	// __CORE_LIST_LOOKUPTABLE_H


