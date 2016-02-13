///////////////////////////////////////////////////////////////////////////////////////
//
// nodearray.cpp		KSH 7 Nov 2001
//
// Functions related to the NodeArray, such as the node name hash table,
// prefix info for rapidly finding all the nodes with a given prefix,
// and utility functions for getting the links and stuff.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <sk/scripting/nodearray.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/tokens.h>
#include <gel/scripting/parse.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/scriptcache.h>
#include <core/crc.h> // For Crc::GenerateCRCFromString
#include <core/math.h>
#include <sk/components/goaleditorcomponent.h>

namespace SkateScript
{
using namespace Script;

////////////////////////////////////////////////////////////////////
//
// Node name hash table stuff, for speeding up FindNamedNode
//
////////////////////////////////////////////////////////////////////

#if	1//def __PLAT_NGC__

// Gets called from SkateScript::Init in sk\scripting\init.cpp
void InitNodeNameHashTable()
{
}

// Resets the hash table, and deletes any extra entries created.
// This currently gets called from the rwviewer cleanup function.
void ClearNodeNameHashTable()
{
}

// Given that a new NodeArray symbol has got loaded due to the loading of a qb containing it,
// this will generate the node name hash table.

// More efficient hash table.
#define HASH_SIZE 16384
#define HASH_MASK (HASH_SIZE-1)

short node_hash[HASH_SIZE];

void CreateNodeNameHashTable()
{
	for ( int lp = 0; lp < HASH_SIZE; lp++ )
	{
		node_hash[lp] = -1;
	}
	// Get the NodeArray
	CArray *p_node_array=GetArray(0xc472ecc5/*NodeArray*/);
	Dbg_MsgAssert(p_node_array,("CreateNodeNameHashTable could not find NodeArray"));

	// Scan through each node, getting it's name, and if it has a name add it in to the hash table.	
	for (uint32 i=0; i<p_node_array->GetSize(); ++i)
	{
		CStruct *p_node=p_node_array->GetStructure(i);
		
		uint32 name_checksum=0;
		if (p_node->GetChecksum("Name",&name_checksum))
		{
			uint16 hash = (uint16)(name_checksum & HASH_MASK );

			while ( node_hash[hash] != -1 )
			{
				hash++;
				hash &= HASH_MASK;
			}
			node_hash[hash] = i;
		}
	}
}
	
// Searches the node array for a node whose Name is the passed checksum.
int FindNamedNode(uint32 checksum, bool assert)
{
	// Get the NodeArray
	CArray *p_node_array=GetArray(CRCD(0xc472ecc5,"NodeArray"));
	Dbg_MsgAssert(p_node_array,("CreateNodeNameHashTable could not find NodeArray"));

	// Added 9/18/03 for THUG submission. If the node array doesn't exist, interpret that
	// as just another condition of the node not existing
	if( p_node_array == NULL )
	{
		return -1;
	}

	uint16 hash = (uint16)(checksum & HASH_MASK );

	while ( node_hash[hash] != -1 )
	{
		CStruct *p_node=p_node_array->GetStructure(node_hash[hash]);
		if ( assert ) Dbg_MsgAssert(p_node,("NULL p_node"));
		
		uint32 name_checksum=0;
		if (p_node->GetChecksum(CRCD(0xa1dc81f9,"Name"),&name_checksum))
		{
			if ( name_checksum == checksum )
			{
				return node_hash[hash];
			}
		}

		hash++;
		hash &= HASH_MASK;
	}

//	// Scan through each node, getting it's name, and if it has a name add it in to the hash table.	
//	for (uint32 i=0; i<p_node_array->GetSize(); ++i)
//	{
//		CStruct *p_node=p_node_array->GetStructure(i);
//		if ( assert ) Dbg_MsgAssert(p_node,("NULL p_node"));
//		
//		uint32 name_checksum=0;
//		if (p_node->GetChecksum("Name",&name_checksum))
//		{
//			if ( name_checksum == checksum )
//			{
//				return i;
//			}
//		}
//	}
	return -1;
}

int FindNamedNode(const char *p_name)
{
	return FindNamedNode(Crc::GenerateCRCFromString(p_name));
}

// Added for use by the NodeExists script function
bool NodeExists(uint32 checksum)
{
	return ( FindNamedNode( checksum, true ) == -1 ) ? false : true;

//	// Get the NodeArray
//	CArray *p_node_array=GetArray(0xc472ecc5/*NodeArray*/);
//	Dbg_MsgAssert(p_node_array,("CreateNodeNameHashTable could not find NodeArray"));
//
//	// Scan through each node, getting it's name, and if it has a name add it in to the hash table.	
//	for (uint32 i=0; i<p_node_array->GetSize(); ++i)
//	{
//		CStruct *p_node=p_node_array->GetStructure(i);
//		Dbg_MsgAssert(p_node,("NULL p_node"));
//		
//		uint32 name_checksum=0;
//		if (p_node->GetChecksum("Name",&name_checksum))
//		{
//			if ( name_checksum == checksum )
//			{
//				return true;
//			}
//		}
//	}
//	return false;
}

#else

#define	__OLD_HASH_TABLE__  0
#define NODE_NAME_HASHBITS 13

#if __OLD_HASH_TABLE__
#define NODE_NAME_HASH_SIZE ((1<<NODE_NAME_HASHBITS))
#else
#define NODE_NAME_HASH_SIZE ((1<<NODE_NAME_HASHBITS) + 100)
#endif

struct SNodeNameHashEntry
{
	uint32 mNameChecksum;
	uint32 mNodeIndex;
	#if	__OLD_HASH_TABLE__
	SNodeNameHashEntry *mpNext;
	#endif	
};

static bool s_node_name_hash_table_initialised=false;
static SNodeNameHashEntry sp_node_name_hash_table[NODE_NAME_HASH_SIZE];

// Gets called from SkateScript::Init in sk\scripting\init.cpp
void InitNodeNameHashTable()
{
	printf("InitNodeNameHashTable ...\n");
	for (int i=0; i<(NODE_NAME_HASH_SIZE); ++i)
	{
		sp_node_name_hash_table[i].mNameChecksum=0;
		sp_node_name_hash_table[i].mNodeIndex=0;
	#if	__OLD_HASH_TABLE__
		sp_node_name_hash_table[i].mpNext=NULL;
	#endif
	}	
	s_node_name_hash_table_initialised=true;
}

// Resets the hash table, and deletes any extra entries created.
// This currently gets called from the rwviewer cleanup function.
void ClearNodeNameHashTable()
{
	Dbg_MsgAssert(s_node_name_hash_table_initialised,("Node name hash table not initialised"));
	for (int i=0; i<(NODE_NAME_HASH_SIZE); ++i)
	{
		#if	__OLD_HASH_TABLE__
			SNodeNameHashEntry *p_entry=sp_node_name_hash_table[i].mpNext;
			// Delete any extra entries.
			while (p_entry)
			{
				SNodeNameHashEntry *p_next=p_entry->mpNext;
				Mem::Free(p_entry);
				p_entry=p_next;
			}
			sp_node_name_hash_table[i].mpNext=NULL;	
		#endif					
		sp_node_name_hash_table[i].mNameChecksum=0;
		sp_node_name_hash_table[i].mNodeIndex=0;
	}	
}

// Given that a new NodeArray symbol has got loaded due to the loading of a qb containing it,
// this will generate the node name hash table.
void CreateNodeNameHashTable()
{
	Dbg_MsgAssert(s_node_name_hash_table_initialised,("Node name hash table not initialised"));
	
	// Make sure the old one is cleared.
	ClearNodeNameHashTable();

	// Get the NodeArray
	CArray *p_node_array=GetArray(0xc472ecc5/*NodeArray*/);
	Dbg_MsgAssert(p_node_array,("CreateNodeNameHashTable could not find NodeArray"));

	// Make sure we're using the script heap, cos the hash table is going to hang around
	// for the duration of the level.
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());

	// Scan through each node, getting it's name, and if it has a name add it in to the hash table.	
	for (uint32 i=0; i<p_node_array->GetSize(); ++i)
	{
		CStruct *p_node=p_node_array->GetStructure(i);
		Dbg_MsgAssert(p_node,("NULL p_node"));
		
		uint32 name_checksum=0;
		if (p_node->GetChecksum("Name",&name_checksum))
		{
			// Node i has name name_checksum, so add it in to the hash table.
			
			// A checksum of zero is used to indicate an empty hash table entry,
			// so check that the name checksum is not zero by chance.
			Dbg_MsgAssert(name_checksum,("Node has zero name checksum ?"));
			
			SNodeNameHashEntry *p_entry=&sp_node_name_hash_table[name_checksum&((1<<NODE_NAME_HASHBITS)-1)];
			// p_entry is now the entry in the hash table array indexed by the lower bits of the checksum.
			
			#if	__OLD_HASH_TABLE__
			if (p_entry->mNameChecksum)
			{
				// p_entry is already occupied, so skip to the end of the list that starts at p_entry.
				while (p_entry->mpNext)
				{
					p_entry=p_entry->mpNext;
				}
				
				// p_entry is now the last entry in the list.
				
				// Create a new entry and fill it in.
				SNodeNameHashEntry *p_new=(SNodeNameHashEntry*)Mem::Malloc(sizeof(SNodeNameHashEntry));
				p_new->mNameChecksum=name_checksum;
				p_new->mNodeIndex=i;
				p_new->mpNext=NULL;
				
				// Tag it onto the end of the list.
				p_entry->mpNext=p_new;
				
				
			}
			else
			{
				// p_entry is free, so stick the info in there.
				p_entry->mNameChecksum=name_checksum;
				p_entry->mNodeIndex=i;
				// Quick check for leaks.
				Dbg_MsgAssert(p_entry->mpNext==NULL,("p_entry->mpNext not NULL ??"));
				p_entry->mpNext=NULL;
			}
			#else
			// just skip forward until we find an empty entry
			while (p_entry->mNameChecksum)
			{
				p_entry++;
				// note in the following assertion we leave one entry to guarentee we have
				// a zero entry roadblock to stop searches
				// otherwise we have a slight possibility of very obscure bugs
				Dbg_MsgAssert(p_entry < &sp_node_name_hash_table[NODE_NAME_HASH_SIZE-1],("Hash table overflow"));
			}
			p_entry->mNameChecksum=name_checksum;
			p_entry->mNodeIndex=i;
			#endif
	
		}
	}
	Mem::Manager::sHandle().PopContext();
}
	
// Searches the node array for a node whose Name is the passed checksum.
int FindNamedNode(uint32 checksum, bool assert)
{
	// TODO: Fix bug where this returns 0 when a checksum of 0 is passed. It should
	// not find a node in that case.
	
	Dbg_MsgAssert(s_node_name_hash_table_initialised,("Node name hash table not initialised"));

	SNodeNameHashEntry *p_entry=&sp_node_name_hash_table[checksum&((1<<NODE_NAME_HASHBITS)-1)];
	
	#if	__OLD_HASH_TABLE__
	while (p_entry && p_entry->mNameChecksum!=checksum)
	{
		p_entry=p_entry->mpNext;
	}
	
	if (assert)
	{
		Dbg_MsgAssert(p_entry,("No node with name %s found.",FindChecksumName(checksum)));
	}
	if (!p_entry)
	{
		return -1;
	}
	#else
	while (p_entry->mNameChecksum && p_entry->mNameChecksum!=checksum)
	{
		p_entry++;
	}
	
	if (assert)
	{
		Dbg_MsgAssert(p_entry->mNameChecksum,("No node with name %s found.",FindChecksumName(checksum)));
	}
	
	if (!p_entry->mNameChecksum)
	{
		return -1;
	}
	#endif		
	
	return p_entry->mNodeIndex;
}

int FindNamedNode(const char *p_name)
{
	return FindNamedNode(Crc::GenerateCRCFromString(p_name));
}

// Added for use by the NodeExists script function
bool NodeExists(uint32 checksum)
{
	Dbg_MsgAssert(s_node_name_hash_table_initialised,("Node name hash table not initialised"));

	SNodeNameHashEntry *p_entry=&sp_node_name_hash_table[checksum&((1<<NODE_NAME_HASHBITS)-1)];
	#if	__OLD_HASH_TABLE__
	while (p_entry && p_entry->mNameChecksum!=checksum)
	{
		p_entry=p_entry->mpNext;
	}
	if (p_entry)
	{
		return true;
	}
	#else
	while (p_entry->mNameChecksum && p_entry->mNameChecksum!=checksum)
	{
		p_entry=p_entry++;
	}
	if (p_entry->mNameChecksum)
	{
		return true;
	}
	#endif
	
	return false;
}

#endif		// __PLAT_NGC__

////////////////////////////////////////////////////////////////////
//
// Node name prefix stuff.
//
////////////////////////////////////////////////////////////////////

// Linked list stuff, used when generating the node lists.
// These only exist temporarily in memory.
struct STempNode
{
	int mNodeIndex;
	STempNode *mpNext;
};

// MEMOPT 800K TEMP Size of temporary buffer used when generating prefix info.
#define NUM_TEMP_NODES 120000
static STempNode *sp_temp_nodes=NULL;
static STempNode *sp_free_temp_nodes=NULL;

// The lookup table, which exists in memory for the duration of the level.
struct SPrefixLookup
{
	// Checksum of the prefix string
	uint32 mChecksum;
	
	// Pointer to the list of node indices of the nodes that have this prefix.
	union
	{
		// Points to somewhere inside sp_node_list_buffer
		// pNodeList[0] contains the number of nodes.
		uint16 *mpNodeList;
		
		// Points to a temporary linked list of nodes, which is used for speed when generating the prefix info.
		// (Uses up too much memory to keep in memory all the time)
		STempNode *mpTempNodesHeadPointer;
	};	
};

// MEMOPT 80K PERM An array of SPrefixLookup's, one for each possible prefix.
// This array is kept in order of checksum, smallest checksum first, to enable quick searching
// using a binary search.
#define MAX_PREFIX_LOOKUPS 13000
static SPrefixLookup sp_prefix_lookups[MAX_PREFIX_LOOKUPS];
static uint32 s_num_prefix_lookups=0;

// MEMOPT 200K PERM Big array of uint16's for holding the node lists.
#define NODE_LIST_BUFFER_SIZE 121000
static uint32 s_node_list_buffer_used=0;
static uint16 sp_node_list_buffer[NODE_LIST_BUFFER_SIZE];

// Does a binary search of the lookup table, and returns the index of the entry 
// that has the passed checksum.
// If there is no matching entry, it will return the index of the entry with the next smallest
// checksum.
// If there is no matching entry, and the passed checksum is smaller than the smallest checksum in the
// table, then it will return 0.
static uint32 sFindPrefixLookup(uint32 checksum)
{
	Dbg_MsgAssert(s_num_prefix_lookups,("Zero s_num_prefix_lookups"));
	uint32 bottom=0;
	uint32 top=s_num_prefix_lookups-1;
	uint32 middle=(bottom+top)>>1;
	
	while (bottom!=middle)
	{
		uint32 ch=sp_prefix_lookups[middle].mChecksum;
		
		if (ch==checksum)
		{
			return middle;
		}
		else if (checksum<ch)
		{
			top=middle;
		}
		else
		{
			bottom=middle;
		}
		middle=(bottom+top)>>1;		
	}		
	if (sp_prefix_lookups[top].mChecksum > checksum)
	{
		return bottom;
	}	
	return top;
}

// Creates the big temporary array of nodes.
static void sCreateTempNodes()
{
	Dbg_MsgAssert(sp_temp_nodes==NULL,("sp_temp_nodes not NULL ?"));
	sp_temp_nodes=(STempNode*)Mem::Malloc(NUM_TEMP_NODES*sizeof(STempNode));
	Dbg_MsgAssert(sp_temp_nodes,("Could not allocate sp_temp_nodes"));
	
	// Link them all into a free list.
	for (int i=0; i<NUM_TEMP_NODES-1; ++i)
	{
		sp_temp_nodes[i].mpNext=&sp_temp_nodes[i+1];
	}
	sp_temp_nodes[NUM_TEMP_NODES-1].mpNext=NULL;
	
	sp_free_temp_nodes=sp_temp_nodes;
}

// Deletes the big temporary array of nodes.
static void sDeleteTempNodes()
{
    if (sp_temp_nodes)
	{
		Mem::Free(sp_temp_nodes);
		sp_temp_nodes=NULL;
		sp_free_temp_nodes=NULL;
	}	
}

// Grabs a new node out of the array.
static STempNode *sNewTempNode()
{
	Dbg_MsgAssert(sp_free_temp_nodes,("Ran out of temp nodes when generating prefix info"));
	STempNode *p_new=sp_free_temp_nodes;
	sp_free_temp_nodes=sp_free_temp_nodes->mpNext;
	return p_new;
}

// Converts the linked lists of nodes into simple arrays, kept in the sp_node_list_buffer static array.
// Uses up less memory. Also means the rest of the game code that calls GetPrefixInfo does not have
// to be changed.
static void sConvertTempNodes()
{
	Dbg_MsgAssert(s_node_list_buffer_used==0,("Expected s_node_list_buffer_used to be 0"));
	uint16 *p_buf=sp_node_list_buffer;
	
	// For each prefix in the lookup table ...
	for (uint32 i=0; i<s_num_prefix_lookups; ++i)
	{
		// Get the linked list.
		STempNode *p_first=sp_prefix_lookups[i].mpTempNodesHeadPointer;
		Dbg_MsgAssert(p_first,("NULL p_first"));
		
		// Count how many nodes are in it.
		int count=0;
		STempNode *p_node=p_first;
		while (p_node)
		{
			++count;
			p_node=p_node->mpNext;
		}
		
		// Check there is enough space to copy the node list into.
		Dbg_MsgAssert(s_node_list_buffer_used+1+count<=NODE_LIST_BUFFER_SIZE,("Node list buffer overflow"));

		// Make the prefix entry now point to the simple array.
		sp_prefix_lookups[i].mpNodeList=p_buf;
		
		// First entry is the count.		
		*p_buf++=count;
		// Copy in all the node indices.
		p_node=p_first;
		while (p_node)
		{
			*p_buf++=p_node->mNodeIndex;
			p_node=p_node->mpNext;
		}
		
		// Update the space used. The +1 is for the count.
		s_node_list_buffer_used+=1+count;
	}	
//	Dbg_MsgAssert(0,("buffer = %d\n",s_node_list_buffer_used));	
}

// Adds a new node index to a particular prefix checksum's entry in the lookup table.
static void sAddNewPrefix(uint32 checksum, int nodeIndex)
{
	// sFindPrefixLookup will assert if s_num_prefix_lookups is zero, so do this
	// as a special case for the very first one added.
	if (s_num_prefix_lookups==0)
	{
		STempNode *p_new=sNewTempNode();
		p_new->mNodeIndex=nodeIndex;
		p_new->mpNext=NULL;
		
		// Stick it in at index 0. The array will be maintained as a sorted
		// array from now on.
		sp_prefix_lookups[0].mChecksum=checksum;
		sp_prefix_lookups[0].mpTempNodesHeadPointer=p_new;
		
		++s_num_prefix_lookups;
		// All done.
		return;
	}	
	
	// Look up the checksum, which may not be there.
	int index=sFindPrefixLookup(checksum);
	uint32 ch=sp_prefix_lookups[index].mChecksum;
	// ch may or may not be checksum. If it isn't, it will either be the next smallest one in the
	// array, or it will be the checksum at index 0 in the case of checksum being smaller than all of them.
	
	if (ch==checksum)
	{
		// This prefix is already in the array, so just have to add the new node index to
		// its list.
		
		STempNode *p_new=sNewTempNode();
		p_new->mNodeIndex=nodeIndex;
		p_new->mpNext=sp_prefix_lookups[index].mpTempNodesHeadPointer;
		
		sp_prefix_lookups[index].mpTempNodesHeadPointer=p_new;
	}
	else
	{
		// The prefix is not in the table, so we have to add it.
		
		Dbg_MsgAssert(s_num_prefix_lookups<MAX_PREFIX_LOOKUPS,("Too many prefixes"));
		
		if (checksum>ch)
		{
			// The above if will mostly be true, because usually Ch will be the next smaller checksum
			// in the table. Hence we increment index so that all the checksums above ch get shifted up,
			// so that the new checksum can be inserted at index, maintaining the sort order.
			++index;
		}
		else
		{
			// The above if will be false in the case of checksum being smaller than all the current checksums
			// in the table. In this case, all the checksums need to be shifted up and checksum inserted at the
			// bottom, so index should be zero here.
			Dbg_MsgAssert(index==0,("index not zero ?"));
		}		

		// Shift everything up one so that the new checksum can be inserted.
		for (int i=s_num_prefix_lookups; i>index; --i)
		{
			sp_prefix_lookups[i]=sp_prefix_lookups[i-1];
		}
		
		// Insert the new checksum, and make a new list for it with just one entry at the moment.
		STempNode *p_new=sNewTempNode();
		p_new->mNodeIndex=nodeIndex;
		p_new->mpNext=NULL;
		
		sp_prefix_lookups[index].mChecksum=checksum;
		sp_prefix_lookups[index].mpTempNodesHeadPointer=p_new;
		
		// Increment the count.
		++s_num_prefix_lookups;
	}
}

void DeletePrefixInfo()
{
	// Just to be sure.
	sDeleteTempNodes();
	
	s_node_list_buffer_used=0;
	s_num_prefix_lookups=0;
}	

// This is called from LoadQB if the QB contains a NodeArray.	
void GeneratePrefixInfo()
{
	DeletePrefixInfo();
	
	// Create the temporary array of nodes used to make linked lists in sAddNewPrefix, for speed.
	sCreateTempNodes();

	// Scan through all the nodes, look up their names, and add all their possible
	// prefixes to the lookup table.
	CArray *p_node_array=GetArray(0xc472ecc5/*NodeArray*/);
	Dbg_MsgAssert(p_node_array,("NodeArray not found"));
	for (uint32 i=0; i<p_node_array->GetSize(); ++i)
	{
		CStruct *p_node=p_node_array->GetStructure(i);
		Dbg_MsgAssert(p_node,("NULL p_node"));
		uint32 name_checksum=0;
		p_node->GetChecksum("Name",&name_checksum);
		
		if (name_checksum)
		{
			// Search the hash table for the name.
			const char *p_name=GetChecksumNameFromLastQB(name_checksum);
			if (p_name)
			{
				// Add all the possible prefixes to the lookup table.
				int string_length=strlen(p_name);
				for (int j=1; j<=string_length; ++j)
				{
					sAddNewPrefix(Crc::GenerateCRC(p_name,j),i);
				}		
			}	
		}
	}

	// Move the info in the linked lists into the more memory efficient static array.
	sConvertTempNodes();
	// Remove the temporary stuff.
	sDeleteTempNodes();

	#ifdef __NOPT_ASSERT__
	// Quick check to make sure the lookup table seems right.
	if (s_num_prefix_lookups)
	{
		for (uint32 i=0; i<s_num_prefix_lookups-1; ++i)
		{
			Dbg_MsgAssert(sp_prefix_lookups[i].mChecksum<sp_prefix_lookups[i+1].mChecksum,("Out of order"));
		}	
	}	
	#endif	
}

////////////////////////////////////////////////////////////////////
//
// GetPrefixedNodes function.
// This will return an array of node indices, which are all the nodes
// whose names are prefixed with the passed string.
// For example, if passed "a", it will return all nodes whose names
// begin with 'a'.
// If passed "TRG_" it will return all nodes beginning with TRG_, etc.
// It is not case sensitive.
//
//
// It returns an array of uint16's, which are the node numbers of
// the matching nodes.
// It loads the size of the array into the passed p_numMatches.
//
// Note: It will never return NULL, but p_numMatches may contain zero.
//
////////////////////////////////////////////////////////////////////
const uint16 *GetPrefixedNodes(uint32 checksum, uint16 *p_numMatches)
{
	static uint16 s_dummy=0;
		
	if (s_num_prefix_lookups)
	{
		// Look it up using a binary search.
		uint32 index=sFindPrefixLookup(checksum);
		if (sp_prefix_lookups[index].mChecksum==checksum)
		{
			// It matches, so return the pre-calculated node list for this prefix.
			uint16 *p_node_list=sp_prefix_lookups[index].mpNodeList;
			*p_numMatches=*p_node_list++;		 
			
			return p_node_list;
		}	
	}	
	
	// No matches, so return a node count of zero.
	*p_numMatches=0;
	return &s_dummy;
}


const uint16 *GetPrefixedNodes(const char *p_prefix, uint16 *p_numMatches)
{
	uint32 checksum=Crc::GenerateCRCFromString(p_prefix);
	return GetPrefixedNodes(checksum,p_numMatches);
}


// return the index of the node nearest to this position that
// matches this prefix
int	GetNearestNodeByPrefix(uint32 prefix, const Mth::Vector &pos)
{
	CArray *p_node_array=GetArray(0xc472ecc5/*NodeArray*/);
	Dbg_MsgAssert(p_node_array,("NodeArray not found"));

	int	closest_node = -1;
	float	closest_dist = 1e20f;
	uint16	num_nodes = 0;
	const uint16 *	p_nodes =  GetPrefixedNodes(prefix,&num_nodes);
	while (num_nodes)
	{
		Script::CStruct * p_node = p_node_array->GetStructure(*p_nodes);
		Mth::Vector node_pos;
		p_node->GetVector(CRCD(0x7f261953,"pos"),&node_pos);
		float	node_dist = (node_pos - pos).LengthSqr();
		//printf ("Node %d at (%.02f, %.02f, %.02f)  dist %.02f\n",*p_nodes,node_pos[X],node_pos[Y],node_pos[Z],node_dist);
		if (node_dist < closest_dist)
		{
		  //  printf ("closest node %d\n",*p_nodes);
			closest_dist = node_dist;
			closest_node = *p_nodes;
		}
	
		p_nodes++;
		num_nodes--;
	}
	
	return closest_node;				   
				   
}


// return the index of the node nearest to this position that
// matches this prefix
int	GetNearestNodeByPrefix(const char *p_prefix, const Mth::Vector &pos)
{
	return GetNearestNodeByPrefix(Crc::GenerateCRCFromString(p_prefix),pos);
}


////////////////////////////////////////////////////////////////////
//
// Utility functions for getting nodes, links, etc
//
////////////////////////////////////////////////////////////////////

CStruct *GetNode(int nodeIndex)
{
	CArray *p_node_array=GetArray(0xc472ecc5/*NodeArray*/);
	Dbg_MsgAssert(p_node_array,("NodeArray not found"));
	CStruct *p_node=p_node_array->GetStructure(nodeIndex);
	Dbg_MsgAssert(p_node,("NULL p_node"));
	return p_node;
}


// This is a DEPRECATED function
// only in to support the rather odd manner of access in nodes via index
uint32	GetNodeNameChecksum(int nodeIndex)
{
	CStruct *p_struct = GetNode(nodeIndex);
	uint32 name = 0;
	p_struct->GetChecksum(CRCD(0xa1dc81f9,"name"),&name);
	return name;
}

uint32 GetNumLinks(CStruct *p_node)
{
	Dbg_MsgAssert(p_node,("NULL p_node"));
	CArray *p_links=NULL;
	p_node->GetArray(0x2e7d5ee7/*Links*/,&p_links);
	if (p_links==NULL)
	{
		return 0;
	}
	return p_links->GetSize();
}

uint32 GetNumLinks(int nodeIndex)
{
	CArray *p_node_array=GetArray(0xc472ecc5/*NodeArray*/, NO_ASSERT);
	if (p_node_array)
	{
		CStruct *p_node=p_node_array->GetStructure(nodeIndex);
		Dbg_MsgAssert(p_node,("NULL p_node"));
	
		return GetNumLinks(p_node);
	}	
	return 0;
}

int GetLink(CStruct *p_node, int linkNumber)
{
	Dbg_MsgAssert( p_node, ( "NULL p_node" ) );
	CArray *p_links=NULL;
	p_node->GetArray( CRCD( 0x2e7d5ee7, "Links" ), &p_links);
	Dbg_MsgAssert( p_links, ( "Tried to call GetLink when there are no links" ) );
	Dbg_MsgAssert( p_links->GetSize(), ( "Tried to call GetLink when there are no links (empty Links array)" ) );
	return p_links->GetInteger( linkNumber );
}

int GetLink(int nodeIndex, int linkNumber)
{
	return GetLink(GetNode(nodeIndex),linkNumber);
}

// return true if Node1 is linked to Node2
bool IsLinkedTo(int node1, int node2)
{
	int n = GetNumLinks(node1);
	for (int i = 0; i<n; i++)	 			// will work for zero links 
	{
		if (GetLink(node1,i) == node2) 		// if any link from Node1 is to Node2
		{
			return true;				    // then we are linked
		}
	}
	return false;
}

void GetPosition(CStruct *p_node, Mth::Vector *p_vector)
{
	Dbg_MsgAssert(p_node,("NULL p_node"));
	Dbg_MsgAssert(p_vector,("NULL p_vector"));
	   
	#ifdef	__NOPT_ASSERT__
	if ( p_node->GetVector(CRCD(0xb9d31b0a,"Position"),p_vector,Script::NO_ASSERT) )
	{
		uint32 name = 0;
		p_node->GetChecksum(CRCD(0xa1dc81f9,"name"),&name);
		if (name)
		{
			Dbg_Message( "Warning:  'Position' deprecated! reexport, node %s",Script::FindChecksumName(name) );
		}
		else
		{
			Dbg_Message( "Warning:  'Position' deprecated! reexport" );
		}
	}
	#endif

	if ( p_node->GetVector(CRCD(0x7f261953,"pos"),p_vector,Script::NO_ASSERT) )
	{
		// no need to negate the new style vectors
	}
	else if ( p_node->GetVector(CRCD(0xb9d31b0a,"Position"),p_vector,Script::NO_ASSERT) )
	{
		(*p_vector)[Z]=-(*p_vector)[Z];
	}
	else
	{
		*p_vector = Mth::Vector(0.0f,0.0f,0.0f,1.0f);
//		Script::PrintContents( p_node );
//		Dbg_MsgAssert( 0, ( "No 'pos' vector found in node" ) );
	}
}

void GetPosition(int nodeIndex, Mth::Vector *p_vector)
{
	Dbg_MsgAssert(p_vector,("NULL p_vector"));
	GetPosition(GetNode(nodeIndex),p_vector);
}

void GetOrientation(CStruct *p_node, Mth::Matrix *p_matrix)
{
	Dbg_MsgAssert(p_node,("NULL p_node"));
	Dbg_MsgAssert(p_matrix,("NULL p_matrix"));
	   
	Mth::Vector orientation_vector;
	if ( p_node->GetVector(CRCD(0xc97f3aa9, "orientation"),&orientation_vector,Script::NO_ASSERT) )
	{
		// assumes a positive scalar component
		Mth::Quat orientation_quat;
		orientation_quat.SetVector(orientation_vector);
		orientation_quat.SetScalar(sqrtf(1.0f - orientation_vector.LengthSqr()));
		
		orientation_quat.GetMatrix(*p_matrix);
	}
	else
	{
		p_matrix->Identity();
	}
}

void GetOrientation(int nodeIndex, Mth::Vector *p_matrix)
{
	Dbg_MsgAssert(p_matrix,("NULL p_matrix"));
	GetPosition(GetNode(nodeIndex),p_matrix);
}

void GetAngles(CStruct *p_node, Mth::Vector *p_vector)
{
	Dbg_MsgAssert(p_node,("NULL p_node"));
	Dbg_MsgAssert(p_vector,("NULL p_vector"));
	// Make sure they are initialised to zero, in case there is no Angles component.
	// Angles components of (0,0,0) are ommitted to save memory.
	p_vector->Set(); 
	p_node->GetVector(0x9d2d0915/*Angles*/,p_vector);
}

void GetAngles(int nodeIndex, Mth::Vector *p_vector)
{
	Dbg_MsgAssert(p_vector,("NULL p_vector"));
	GetAngles(GetNode(nodeIndex),p_vector);
}

CArray *GetIgnoredLightArray(CStruct *p_node)
{
	Dbg_MsgAssert(p_node,("NULL p_node"));
	CArray *p_ignored_lights=NULL;
	p_node->GetArray(0xb7b030be/*IgnoredLights*/,&p_ignored_lights);
	return p_ignored_lights;
}

CArray *GetIgnoredLightArray(int nodeIndex)
{
	return GetIgnoredLightArray(GetNode(nodeIndex));
}

// Used by Ryan in the Park Editor.
// This creates an array (called NodeArray) of Size structures, with all the entries initialised to empty structures.
void CreateNodeArray(int size, bool hackUseFrontEndHeap)
{
	// copied from Script::Cleanup()
	//KillStoppedScripts();
	//RemoveOldTriggerScripts();
	//RemoveSymbol(GenerateCRC("TriggerScripts"));
	CleanUpAndRemoveSymbol(CRCD(0xc472ecc5,"NodeArray"));
	DeletePrefixInfo();
	ClearNodeNameHashTable();
	
	// Make sure it doesn't exist already.
	// ParseQB will catch this anyway, but may as well check before getting there.
	Dbg_MsgAssert(GetArray(CRCD(0xc472ecc5,"NodeArray"))==NULL,("Called CreateNodeArray when a NodeArray already exists"));


	// Create a dummy QB file, and parse it the usual way.	
	if (hackUseFrontEndHeap)
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap()); // Use the temporary heap.
	else
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap()); // Use the temporary heap.
	uint8 *p_dummy_qb=(uint8*)Mem::Malloc((1			// ESCRIPTTOKEN_NAME
										 +4			// Checksum of 'NodeArray'
										 +1			// ESCRIPTTOKEN_EQUALS
										 +1			// ESCRIPTTOKEN_STARTARRAY
										 +2*size	// ESCRIPTTOKEN_STARTSTRUCT,ESCRIPTTOKEN_ENDSTRUCT pairs
										 +1			// ESCRIPTTOKEN_ENDARRAY
										 +1			// ESCRIPTTOKEN_ENDOFFILE
										 )*sizeof(uint8));
	uint8 *p_qb=p_dummy_qb;
	
	*p_qb++=ESCRIPTTOKEN_NAME;
	*p_qb++=0xc5; // Checksum of 'NodeArray'
	*p_qb++=0xec;
	*p_qb++=0x72;
	*p_qb++=0xc4;
	*p_qb++=ESCRIPTTOKEN_EQUALS;
	*p_qb++=ESCRIPTTOKEN_STARTARRAY;
	for (int i=0; i<size; ++i)
	{
		*p_qb++=ESCRIPTTOKEN_STARTSTRUCT;
		*p_qb++=ESCRIPTTOKEN_ENDSTRUCT;
	}
	*p_qb++=ESCRIPTTOKEN_ENDARRAY;
	*p_qb=ESCRIPTTOKEN_ENDOFFILE;

	// Parse it the usual way.
	ParseQB("levels\\sk5ed\\sk5ed.qb",p_dummy_qb,ASSERT_IF_DUPLICATE_SYMBOLS);
	// TODO ParseQB no longer does the node name hash table generation etc, so need to copy
	// the calls to the code that does that from SkateScript::LoadQB in sk\scripting\file.cpp
			
	// This next bit is a fix to the assert that was happening when attempting to qbr within the
	// park editor:		
	// Set the mGotReloaded to false for the newly created symbol, otherwise it will be left
	// stuck on causing the next qbr to make it think that the NodeArray got reloaded by that qbr,
	// which will then cause an assert.
	CSymbolTableEntry *p_new_node_array=Script::LookUpSymbol("NodeArray");
	Dbg_MsgAssert(p_new_node_array,("What? No NodeArray?"));
	p_new_node_array->mGotReloaded=false;

	Obj::InsertGoalEditorNodes();
	
	Mem::Free(p_dummy_qb);
	Mem::Manager::sHandle().PopContext();
}

////////////////////////////////////////////////////////////////////
//
// Given a script, and the checksum of a function to look for,
// this will search through the script for any calls to that function.
// It will recursively search through any script calls.
// The callback function is called for each occurrence of the function
// call found, and the parameter list for that call is also passed.
//
// NOTE: Don't store the passed CStruct pointer! 
// It's a local variable in FindReferences.
// To store the parameters create a new CStruct and use the
// AppendStructure member function to copy in the contents of the
// passed one.
//
////////////////////////////////////////////////////////////////////

// Note: p_args contains the set of parameters that were passed to scriptToScanThrough. It is passed so that
// the <,> syntax can be evaluated. This is necessary because sometimes the EndGap command is not directly
// passed the text and score, they might get passed via the <...> syntax, for example see the
// Gap_Gen_End script in sk4_scripts.q
static void FindReferences(uint32 scriptToScanThrough, uint32 functionToScanFor, void (*p_callback)(Script::CStruct *, const uint8 *), Script::CStruct *p_args, int Count)
{
	// Don't recurse too deeply otherwise some levels take ages finding the gaps (eg, the secret level)
	if (Count > 5)
	{
		return;
	}
		
	Dbg_MsgAssert(p_callback,("NULL p_callback"));

	#ifdef __NOPT_ASSERT__
	// Look up the function being scanned for to see if it is a cfunction.
    bool (*p_cfunction)(CStruct *pParams, CScript *pCScript)=NULL;
	CSymbolTableEntry *p_function_entry=LookUpSymbol(functionToScanFor);
	if (p_function_entry && p_function_entry->mType==ESYMBOLTYPE_CFUNCTION)
	{
		p_cfunction=p_function_entry->mpCFunction;
	}	
	Dbg_MsgAssert(p_cfunction==NULL,("Cannot use FindReferences to find cfunction calls yet ..."));
	#endif

	#ifdef NO_SCRIPT_CACHING
	// Look up the script
	CSymbolTableEntry *p_script_entry=LookUpSymbol(scriptToScanThrough);
	if (!p_script_entry)
	{
		Dbg_Warning("Script '%s' not found in call to FindReferences",FindChecksumName(scriptToScanThrough));
		return;
	}
	Dbg_MsgAssert(p_script_entry->mType==ESYMBOLTYPE_QSCRIPT,("'%s' is not a qscript",FindChecksumName(scriptToScanThrough)));
	
	// Get a pointer to it.
	uint8 *p_token=p_script_entry->mpScript;
	Dbg_MsgAssert(p_token,("NULL p_token ???"));
	// Skip over the 4-byte contents checksum.
	p_token+=4;
	#else
	
	Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
	Dbg_MsgAssert(p_script_cache,("NULL p_script_cache"));
	uint8 *p_script=p_script_cache->GetScript(scriptToScanThrough);
	Dbg_MsgAssert(p_script,("Script %s not found in script cache",Script::FindChecksumName(scriptToScanThrough)));
	uint8 *p_token=p_script;
	
	#endif
	
	
	// Create a structure for holding parameters.
	CStruct *p_params=new CStruct;

	// Skip to the first line of the script.	
	p_token=SkipToStartOfNextLine(p_token);

	
	// Now scan through each token of the script in turn, looking for any calls to functionToScanFor.
	
	bool finished=false;
    while (!finished)
    {
        switch (*p_token)
        {
		case ESCRIPTTOKEN_RUNTIME_CFUNCTION:
		{
			// Skip over CFunction calls for now.
			p_token+=5;
			p_token=SkipToStartOfNextLine(p_token);
			break;
		}
		
        case ESCRIPTTOKEN_NAME:
		case ESCRIPTTOKEN_RUNTIME_MEMBERFUNCTION:
        {
			// Remember the location for passing to the callback.
			const uint8 *p_location=p_token;
			
            ++p_token;
            uint32 name_checksum=Read4Bytes(p_token).mChecksum;
            p_token+=4;

			// Skip over lines of script that are setting parameters
			if (*p_token==ESCRIPTTOKEN_EQUALS)
			{
				break;
			}		
			
            CSymbolTableEntry *p_entry=Resolve(name_checksum);
			if (p_entry)
			{
				switch (p_entry->mType)
				{
				case ESYMBOLTYPE_CFUNCTION:
					Dbg_MsgAssert(p_entry->mpCFunction,("\nLine %d of %s\nNULL pCFunction",GetLineNumber(p_token),FindChecksumName(p_entry->mSourceFileNameChecksum)));
				
					if (name_checksum==functionToScanFor)
					{
						// Found the required function call! Get the params and call the callback.
						p_params->Clear();
						// Note: Uses p_args to look up any params enclosed in <,> that are encountered.
						p_token=AddComponentsUntilEndOfLine(p_params,p_token,p_args);
						
						//printf("Callback: %s\n",FindChecksumName(scriptToScanThrough));
						(*p_callback)(p_params,p_location);
					}
					else
					{
						p_token=SkipToStartOfNextLine(p_token);
					}
					break;
					
				case ESYMBOLTYPE_MEMBERFUNCTION:
					if (name_checksum==functionToScanFor)
					{
						// Found the required function call! Get the params and call the callback.
						p_params->Clear();
						// Note: Uses p_args to look up any params enclosed in <,> that are encountered.
						p_token=AddComponentsUntilEndOfLine(p_params,p_token,p_args);
						
						//printf("Callback: %s\n",FindChecksumName(scriptToScanThrough));
						(*p_callback)(p_params,p_location);
						
						if (name_checksum==0xe5399fb2) // EndGap
						{
							uint32 GapScript=0;
							if (p_params->GetChecksum("GapScript",&GapScript))
							{
								// Passing NULL for p_args here because it is not clear what parameters are
								// going to be passed to the GapScript when it is run.
								FindReferences(GapScript,functionToScanFor,p_callback,NULL,Count+1);
							}	
						}	
					}
					else
					{
						p_token=SkipToStartOfNextLine(p_token);
					}
					break;
					
				case ESYMBOLTYPE_QSCRIPT:
					p_params->Clear();
					// Note: Uses p_args to look up any params enclosed in <,> that are encountered.
					p_token=AddComponentsUntilEndOfLine(p_params,p_token,p_args);
					
					if (name_checksum==functionToScanFor)
					{
						// Found the required function call! Get the params and call the callback.
						//printf("Callback: %s\n",FindChecksumName(scriptToScanThrough));
						(*p_callback)(p_params,p_location);
					}
					else
					{
						p_token=SkipToStartOfNextLine(p_token);
					}
					
					// It's a q-script, so recursively search that too.
					// Passing the just calculated p_params so that the <,> syntax within the script
					// can be evaluated.
					FindReferences(name_checksum,functionToScanFor,p_callback,p_params,Count+1);
					break;
					
				default:
					break;
				}    
			}
            break;
        }    

        case ESCRIPTTOKEN_KEYWORD_REPEAT:
            p_token=SkipToken(p_token);
			// Scan over any repeat parameters.
			p_token=SkipToStartOfNextLine(p_token);
            break;

		case ESCRIPTTOKEN_KEYWORD_IF:
            p_token=SkipToken(p_token);

			if (*p_token==ESCRIPTTOKEN_KEYWORD_NOT)
			{
				p_token=SkipToken(p_token);
			}
				
			// If the if keyword is followed by an open parenthesis, then it must be using an
			// expression. Often the expression may contain c-function calls, such as:
			// if ( (GotParam Foo) | (GotParam Blaa) )
			// This was causing a problem because this loop would skip over the open parenthesis,
			// get to the GotParam, and then recognizing that GotParam is a cfunction it would
			// try to skip to the next line, but then that would cause a parenthesis mismatch
			// assert in SkipToStartOfNextLine because the open parenth had been skipped over.
			// So to get around that, skip to the next line as soon as the open parenth is detected.
			if (*p_token==ESCRIPTTOKEN_OPENPARENTH)
			{
				p_token=SkipToStartOfNextLine(p_token);
			}	
			break;
			
        case ESCRIPTTOKEN_KEYWORD_ENDSCRIPT:
			finished=true;
            break;

        default:
            p_token=SkipToken(p_token);
            break;
        }
    }

	#ifndef NO_SCRIPT_CACHING
	p_script_cache->DecrementScriptUsage(scriptToScanThrough);
	#endif
	
	// Delete the temporary p_params.
	delete p_params;	
}

void ScanNodeScripts(uint32 componentName, uint32 functionName, void (*p_callback)(CStruct *, const uint8 *))
{
	Script::DisableExpressionEvaluatorErrorChecking();
	
	CArray *p_node_array=GetArray(0xc472ecc5); // Checksum of NodeArray
	Dbg_MsgAssert(p_node_array,("NodeArray not found"));
	
	for (uint32 i=0; i<p_node_array->GetSize(); ++i)
	{
		CStruct *p_node=p_node_array->GetStructure(i);
		Dbg_MsgAssert(p_node,("NULL p_node"));
		
		uint32 script_checksum=0;
		if (p_node->GetChecksum(componentName,&script_checksum))
		{
			FindReferences(script_checksum,functionName,p_callback,NULL,0);
		}	
	}
	
	Script::EnableExpressionEvaluatorErrorChecking();
}

} // namespace SkateScript

