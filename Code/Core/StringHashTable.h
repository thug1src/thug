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
**	Module:			HashTable      			 								**
**																			**
**	File name:		Core\HashTable.h										**
**																			**
**	Created by:		9/22/2000 - rjm							                **
**																			**
**	Description:	A handy Hashtable class				                	**
**																			**
*****************************************************************************/

#ifndef __CORE_LIST_STRINGHASHTABLE_H
#define __CORE_LIST_STRINGHASHTABLE_H

#ifndef __PLAT_WN32__
	#include <sys/mem/PoolManager.h>
#endif

#include "HashTable.h"

namespace Crc
{
	uint32 GenerateCRCFromString( const char *pName );
}

namespace Script
{
	// So as not to require inclusion of checksum.h
#ifdef __NOPT_ASSERT__	
	const char *FindChecksumName(uint32 checksum);
#endif
}

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

template<class _V>
class StringHashTable : public HashTable<_V>
{

public:
									StringHashTable(int numBits);

    bool 							PutItem(const char *stringKey, _V *item);
    _V *							GetItem(const char *stringKey, bool assert_if_clash = true);
    void 							FlushItem(const char *stringKey);
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

template<class _V> //inline
StringHashTable<_V>::StringHashTable(int numBits) :
    HashTable<_V>(numBits)
{
}



template<class _V> //inline
bool StringHashTable<_V>::PutItem(const char *stringKey, _V *item)
{
    uint32 key = Crc::GenerateCRCFromString(stringKey);
    return HashTable<_V>::PutItem(key, item);
}



template<class _V> //inline
_V *StringHashTable<_V>::GetItem(const char *stringKey, bool assert_if_clash)
{
    uint32 key = Crc::GenerateCRCFromString(stringKey);
    return HashTable<_V>::GetItem(key, assert_if_clash);
}


template<class _V> //inline
void StringHashTable<_V>::FlushItem(const char *stringKey)
{
    uint32 key = Crc::GenerateCRCFromString(stringKey);
    HashTable<_V>::FlushItem(key);
}

} // namespace Lst

#endif	// __CORE_LIST_STRINGHASHTABLE_H


