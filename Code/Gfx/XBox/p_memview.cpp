//////////////////////////////////////////////////////
// p_memview.cpp
//
// code for tracking memory usage, and displaying it in a graphical manner
// keeps extra info about allocated blocks
// including the call stack, so we can print out information
// about specific allocated blocks
// which we will select using the graphical memory browser
//
//
// tried to use a little of the task system as possible
// so we can run the inspector
// without it messing with the heap it inspects
//
//


//extern char _mem_dump_start[];
//extern char _map_file_start[];
//extern char _symbols_start[];
//extern char _callstack_start[];
//extern char _code_end[];
//extern char	_std_mem_end[];
//extern char	_stack_size[];

//extern char __text_org[];
//extern char __data_org[];
//extern char __rodata_org[];
//extern char __bss_org[];

//extern char __rodata_orgend[];
//extern char __bss_objend[];
//extern char __text_objend[];

//extern char _rwheapdebug_start[];

			 
#define	STACKDEPTH  30			 


			 
extern "C"				 
{
extern void* ENTRYPOINT;		
}

#include <gfx/gfxman.h>
#include <gfx/nxviewport.h>
#include <gel/inpman.h>					// needed for buttons
#include <gfx/xbox/p_memview.h>
#include <sys/file/filesys.h> 			// needed for loading map file
#include <gel/scripting/script.h>

#include <stdio.h>
#include <string.h>

#include <sys\mem\memman.h>
#include <sys\mem\heap.h>
#include <sys\mem\region.h>
#include <sys/config/config.h>

// needed for some VerticalMenu specific debugging							
#include <core/support.h>
#include <core/list.h>
#include <core/String/CString.h>


extern volatile int	test_vblanks;


class CCallStack
{
public:
	void Append(CCallStack *p);
	void Remove();
	void InitHead();
	int	used;
	int	size;
	CCallStack *pNext;
	CCallStack *pPrev;
	int	addr[STACKDEPTH];
	uint32	flags;
	Mem::Allocator::BlockHeader * pBlock;	// pointer to block that has this callstack

};

CCallStack		free_list;	// list of created objects
CCallStack		used_list;	// list of created objects

// init a node, so it can act as the head			 
inline void CCallStack::InitHead()
{
	pPrev = this;
	pNext = this;
}
	
// append node p to this node (after it)									 
inline void CCallStack::Append(CCallStack *p)
{

	p->pNext = this->pNext;
	p->pPrev = this;
	this->pNext = p;
	p->pNext->pPrev = p;
}

// simply unlink it from the list			   
inline void CCallStack::Remove()
{
	pPrev->pNext = pNext;
	pNext->pPrev = pPrev;
}


						
//CCallStack * CallStack_FirstFree;
//CCallStack * CallStack_FirstUsed; 

static int MemView_Active = 0;



#define	MAX_CALLSTACK (8192 * 8)		// we got 8 mb, woo woo.


static float step = 128.0f;


static char HexByte(char a)
{
	if (a >= '0' && a <='9')
	{
		return a-'0';
	}
	if (a >= 'A' && a <='F')
	{
		return 10 + a-'A';
	}
	if (a >= 'a' && a <='f')
	{
		return 10 + a-'a';
	}

	// should really be an error, but just ignore it and return 0
	// as this is only used for parsing the map file	
	return 0;
	
	
}


static int doneonce = 0;


char *MemView_GetFunctionName(int pc, int *p_size)
{
	return "UNKNOWN";
	
}

// Modifed version of Jamie's unwind stack function
// ignores the fp, and just goes directly off the sp
// seems to work much better (and faster too)
int DumpUnwindStack( int iMaxDepth, int *pDest )
{
	return 0;
}
				   
				   
//        mD_L2           = nBit( vD_L2 ),
//        mD_R2           = nBit( vD_R2 ),
//        mD_L1           = nBit( vD_L1 ),
//        mD_R1           = nBit( vD_R1 ),
//        mD_TRIANGLE     = nBit( vD_TRIANGLE ),
//	      mD_CIRCLE       = nBit( vD_CIRCLE ),
//        mD_X            = nBit( vD_X ),
//        mD_SQUARE       = nBit( vD_SQUARE ),
//        mD_SELECT       = nBit( vD_SELECT ),
//        mD_L3           = nBit( vD_L3 ),
//        mD_R3           = nBit( vD_R3 ),
//        mD_START        = nBit( vD_START ),
//        mD_UP           = nBit( vD_UP ),
//        mD_RIGHT        = nBit( vD_RIGHT ),
//        mD_DOWN         = nBit( vD_DOWN ),
//        mD_LEFT         = nBit( vD_LEFT ),


void MemViewToggle()
{
	MemView_Active ^=1;
}


	 

void MemView_Alloc( void *v)
{
#ifdef	__LINKED_LIST_HEAP__


#ifdef	__NOPT_CDROM__OLD
   return;
#endif 	

#endif
}

void MemView_Free( void *v)
{
#ifdef	__LINKED_LIST_HEAP__
#endif	
}


Mem::Allocator::BlockHeader *MemView_FindBlock( int addr)
{
#ifdef	__LINKED_LIST_HEAP__

	
	Mem::Allocator::BlockHeader *pSmallestBlock	= NULL;
	uint32 smallest_block_size = 100000000;
	Mem::Manager& mem_man = Mem::Manager::sHandle();
	for (Mem::Heap* heap = mem_man.FirstHeap(); heap != NULL; heap = mem_man.NextHeap(heap))
	{
		Mem::Allocator::BlockHeader *pBlock = (Mem::Allocator::BlockHeader *) heap->find_block((void*)addr);	
		if (pBlock)
		{
			if (pBlock->mSize < smallest_block_size)
			{
				smallest_block_size = pBlock->mSize;
				pSmallestBlock = pBlock;
			}
		}
	}
	return pSmallestBlock;
#else
	return NULL;
#endif
}

char * MemView_GetClassName(CCallStack *c)
{
#ifdef	__LINKED_LIST_HEAP__
	int *ra = (int*)(c->addr[4]);
	int count = STACKDEPTH-4;
	while (count--)
	{
		int instruction = *ra++;
		if (instruction >> 24 == 0x0c)
		{
			int code = (instruction & 0xffffff)<<2;
			int size;
			char *p = MemView_GetFunctionName(code,&size); 
			// to tell if this is class or not
			// we see if the text is of the form  
			//    classname::classname (teminated by a 0)
			// as that indicates that it is a constructor
			// dude... this is where we need a regular expression....
			char *end = p;
			while (*end) end++;	   						// scan to end
			while (end[-1] != ':' && end > p)	end--;	// skip to char after the last :
			char *other = strstr(p,end);				// find fist occurance of end of string
			if (other != end)							// if different, then this is it!!
			{
				return MemView_GetFunctionName(code,&size);
				break;
			}
		}
	}
#endif
	return NULL;
}


void MemView_DumpBlockInfo(int cursor)
{
#	ifdef	__LINKED_LIST_HEAP__
	
	char os[256];
	
	Mem::Allocator::BlockHeader *pBlock = MemView_FindBlock( cursor );
	if( !pBlock )
	{
		// should search free blocks here???
	}

	// find this in the allocators used list
	// and say if it is free, or not	
	if( pBlock == NULL )
	{
/*		if (cursor > (int)__text_org && cursor < (int)__bss_objend)		// check to see if in code/data
		{
			
			if (cursor < (int)__data_org)
				printf("Code: ");
			else if (cursor < (int)__rodata_org)
				printf("Data: ");
			else if (cursor < (int)__bss_org)
				printf("RO-Data: ");
			else 
				printf("BSS: ");
			
		
			int size;
			char *p_name = MemView_GetFunctionName(cursor,&size);
			printf ( "%s, size %d\n",p_name,size);
		}
		else
*/
		{
			OutputDebugString( "Block Not Found\n" );
		}
	}
	else
	{
		void * p_start = (void*)((uint)pBlock + Mem::Allocator::BlockHeader::sSize);
		sprintf( os, "Block found, addr = %p, size = %d (Header = %d)\n",p_start,pBlock->mSize,Mem::Allocator::BlockHeader::sSize);
		OutputDebugString( os );

		CCallStack *c = (CCallStack*)pBlock->mp_debug_data;
		if (!c)
		{
//			OutputDebugString( "Block with No Debug Info!!\n" );
		}
		else
		{
			// assume this is a "new", then the fourth callstack ra will point to the 
			// jal xxxxxx  instruction, where xxxxx is the constructor for the 
			// or it might be sortly thereafter, so check 16 instructions
			char *classname = MemView_GetClassName( c );
			if( classname )
			{
				sprintf( os, "CLASS: %s\n", classname );
				OutputDebugString( os );
			}

			// then list out the call stack (skipping the MemView_Alloc, as that's a given, and irrelevant);			
		
			int *p = c->addr + 1;
			while (p[1])			// also skip the ENTRYPOINT, just go back to main()
			{
				int size;
				sprintf( os, "%p: %s\n", (void*)*p,MemView_GetFunctionName(*p,&size));
				OutputDebugString( os );
				p++;
			}
		}
	}
#endif
}

static	int	blockstart;

static float cursor;

void 		MemView_Display()
{
#	ifdef	__NOPT_CDROM__OLD
	return;
#	endif 	
}


#ifdef	__LINKED_LIST_HEAP__

static int num_used;

static void ScanRegion(uint32 *p_start, uint32 *p_end)
{
#if 0
	printf ("scanning from %p to %p\n",p_start,p_end);
	// scan the whole range of memeory
	while (p_start<p_end)
	{
		// get value that might be a pointer
		uint32 x = *p_start++;
		// check to see if it's not odd, and it lays in the heap area
		if (!(x&3) && x > (uint32)_code_end /*&& x < (uint32)_std_mem_end*/) // don't check for end now, as we have some debug heaps up there we want to include
		{
			// check to see if it points to one of the heap members
			
			uint32 *p_refs = (uint32*)_mem_dump_start;
			
		#if 0
			for (int i=0;i<num_used;i++)
			{
				if (*p_refs == x)	  	
				{
					// got it, increment the reference counter
					p_refs[1]++;
					break;
				}
				p_refs+=2;
			} 
			
		#else
		
			// we want to do it twice, once for x, and once for x+16
			// the reason being, a class this is allocated with
			// the [] operator will actually start 16 bytes before the ref
			// so we need to go back 16 bytes when looking for the block

			int oldx = x;
			for (int i=0;i<2;i++)
			{
			
				// binary search folks.....
				int low = 0;
				int high = num_used-1;
				while (1)
				{
					int mid = (low + high) /2;
					if (p_refs[mid<<1] == x)
					{
						p_refs[(mid<<1)+1] ++;
						break;
					}
					if (high == low)
					{
						break;
					}
					if (p_refs[mid<<1] > x)
					{
						high = mid;
					}
					else
					{
						// if the low point is already the same as the mid point
						// then the only way to go is up!
						// as this will only occur when low + 1 == high
						if (low == mid)
						{
							low = high;
						}
						else
						{
							low = mid;
						}
					}				
				}
				x -= 16;
			}
			x = oldx;
		#endif						  		
		}
	}
#endif
}
#endif

#ifdef	__LINKED_LIST_HEAP__
static uint32 *p_used;		
#endif

int MemView_CountBlocks(Mem::Allocator::BlockHeader *p_header)
{
#ifdef	__LINKED_LIST_HEAP__
	int num_used = 0;
	while ( p_header )
	{
		void * p_start = (void*)((uint)p_header + Mem::Allocator::BlockHeader::sSize);
		
		*p_used++ = (uint32)p_start;	 		// store the start of the block
		*p_used++ = 0;							// store a count
		p_header = p_header->mp_next_used;
		num_used++;
	}
	return num_used;
#else
	return 0;
#endif
}

// Find memory leaks
// the algorithm is quite simple:
// 1) Make a list of all "used" memory blocks, and set their usage count to 0
// 2) Scan all of the heap, and the stack, for each word that looks like a pointer, 
//    check to see if it is in the list of "used", and increment the usage count if so
// 3) Scan the list of used pointers, and check for any with usage == 0

// NEED OT EXTEND FOR TOP_DOWN heap.....		
		
void MemView_FindLeaks()
{
#ifdef	__LINKED_LIST_HEAP__
#if 0
		p_used  =  (uint32*)_mem_dump_start;		
		num_used = 0;
		printf ("Counting blocks....");		
		num_used += MemView_CountBlocks(Mem::Manager::sHandle().BottomUpHeap()->first_block()); 
		num_used += MemView_CountBlocks(Mem::Manager::sHandle().TopDownHeap()->first_block()); 
		num_used += MemView_CountBlocks(Mem::Manager::sHandle().FrontEndHeap()->first_block()); 
		num_used += MemView_CountBlocks(Mem::Manager::sHandle().NetworkHeap()->first_block()); 
		num_used += MemView_CountBlocks(Mem::Manager::sHandle().ScriptHeap()->first_block()); 
		num_used += MemView_CountBlocks(Mem::Manager::sHandle().SkaterHeap(0)->first_block()); 
//		num_used += MemView_CountBlocks(Mem::Manager::sHandle().DebugHeap()->first_block()); 
		printf (" %d\n",num_used);
		printf ("Sorting .....\n");			
		// Now we've done that, let's sort the list, so we can use a binary search later
		uint32 *p_top  =  (uint32*)_mem_dump_start;		
		for	(int i = 0;i<num_used-1;i++)
		{
			uint32 top = *p_top;
			uint32 *p_scan  =  p_top+2;	  
			uint32 *p_best = p_top;  			
			for (int j = i;j<num_used-1;j++)
			{
				uint32 scan = *p_scan;
				if (scan < top)
				{
					top = scan;
					p_best = p_scan;
				}
				p_scan+=2;
			}
			uint64 t = *(uint64*)p_top;
			*(uint64*)p_top = *(uint64*)p_best;
			*(uint64*)p_best = t;		
			p_top +=2;
		}



		// now scan all appropiate regions of memory
		
		// First scan the code, data and regular heap
		ScanRegion((uint32*)_code_end,(uint32*)_std_mem_end);

		// Next scan the alternate area or memory, where the script heap goes
		ScanRegion((uint32*)(_rwheapdebug_start),(uint32*)(_rwheapdebug_start + 0x04970000 - 0x04500000));

		// then scan the stack
//		uint64* sp;											// frame pointer
//		asm ( "daddu %0, $29, $0" : "=r" (sp) );			// get current sp
//		uint32 *stack_start = (uint32*)sp;
//		// stack end is stack start rounded up by the stack size
//		// assumes that things are nice powers of 2
//		uint32 *stack_end = (uint32*)(((int)(stack_start) + (int)_stack_size-4) & ~(int)(_stack_size-1));
//		ScanRegion(stack_start,stack_end);
		
		




		bool	LeaksFound = false;

		// then check for any with zero reference
		uint32 *p_refs = (uint32*)_mem_dump_start;
		for (int i=0;i<num_used;i++)
		{
			if (!p_refs[1])	  	
			{
				uint32 addr = *p_refs;
				if (!LeaksFound)
				{
					printf ("-----------------------------------------------------------------------\n");
					printf ("-----------  LEAKS FOUND !!!!!!!!!!!!!!           ---------------------\n");
					printf ("-----------------------------------------------------------------------\n");
					LeaksFound = true;
				}
				printf ("\nPossible leak, addr %p\n",(void*)addr);				
				MemView_DumpBlockInfo(addr);
			}
			p_refs+=2;
		}			
		
		if (LeaksFound)
		{
			printf ("-----------------------------------------------------------------------\n");
			printf ("-----------  END OF LEAKS                         ---------------------\n");
			printf ("-----------------------------------------------------------------------\n");
		}
		else
		{
			printf ("-----------  NO LEAKS DETECTED!!!  ---------------------\n");
		}
#endif
#endif
}


// Given a block addr, then search all the other blocks to see 
// which block contains a reference to this block
// and recursivly step back through the blocks until
// we can't find another reference, or the address is not in a block
void MemView_DumpRefs(int addr)
{
#ifdef	__LINKED_LIST_HEAP__
#if 0
	printf ("\n\nDumping references for %p\n",(void*)addr);
	MemView_DumpBlockInfo(addr);
	uint32 *p_first = NULL;	
	int last_addr = 0;
	int count = 0 ;
	while (1)
	{
		// now just do a simple search through the heap reagion
		// to find another reference
		uint32 *p_start = (uint32*)_code_end;
		uint32 *p_end = (uint32*)_std_mem_end;
		while (p_start<p_end && count < 10)
		{
			if (*p_start == (uint32)addr && p_start != (uint32*)&blockstart)
			{
				count++;
				printf ("\nReference level %d in %p\n",count,(void*)p_start);
				MemView_DumpBlockInfo((int)p_start);
				addr = (int)p_start;


				Mem::Allocator::BlockHeader *pBlock = MemView_FindBlock(addr);				
				addr = (int)((uint)pBlock + Mem::Allocator::BlockHeader::sSize);
				if (addr == (int) p_first || addr == last_addr)
				{
					printf ("LOOPING .....\n");
					return;
				}
				last_addr = addr;
				if (!p_first)
				{
					p_first = (uint32*)addr;
				}
				break;				
				
			}
			p_start++;
		}
		if (count >= 10)
		{
			printf ("Stopping after %d refs\n",count);
			return;
		}
		if (p_start >= p_end)
		{
			printf ("No more References Found in heap \n");
			return;
		}
	}
#endif
#endif
}

// Find the first block in the free list
// if no free blocks, then return
// scan all used blocks, and print out the info for all the blocks
// that have an address above the first free block
void MemView_DumpFragments(Mem::Heap *pHeap)
{
#	ifdef __LINKED_LIST_HEAP__ 

	char os[256];

	if (!pHeap->mFreeBlocks.m_count)
	{
		OutputDebugString( "NO Fragmentation\n" );
		return;
	}
	
	if( !pHeap->mp_context->mp_free_list )
	{
		sprintf( os, "!!!!!! No free list, but there are %d free blocks???\n",pHeap->mFreeBlocks.m_count);
		OutputDebugString( os );
		return;
	}

	Mem::Allocator::BlockHeader *p_free = pHeap->mp_context->mp_free_list;
	Mem::Allocator::BlockHeader *p_full = pHeap->mp_context->mp_used_list;
	
	sprintf( os, "!!!!!! Free list starts at %p\n",p_free );
	OutputDebugString( os );

	// The first p_free will be the start of fragmentations
	while (p_full)
	{
		if (p_full > p_free)
		{
			OutputDebugString( "Framgented Block: " );
			void * p_start = (void*)((uint)p_full + Mem::Allocator::BlockHeader::sSize);
			MemView_DumpBlockInfo((int)p_start);
		}
		p_full = p_full->mp_next_used;
	}
	#endif
}

void MemView_DumpHeap( Mem::Heap *pHeap )
{
	#ifdef	__LINKED_LIST_HEAP__    
	Mem::Allocator::BlockHeader *p_full = pHeap->mp_context->mp_used_list;
	
	// The first p_free will be the start of fragmentations
	while( p_full )
	{
		printf ("\n");
		void * p_start = (void*)((uint)p_full + Mem::Allocator::BlockHeader::sSize);
		MemView_DumpBlockInfo((int)p_start);
		p_full = p_full->mp_next_used;
	}
	#endif
}



void MemView_DumpBottomFragments()
{

	MemView_DumpFragments(Mem::Manager::sHandle().BottomUpHeap());
}

void MemView_DumpTopFragments()
{

	MemView_DumpFragments(Mem::Manager::sHandle().TopDownHeap());
}



/*
class CCallStack
{
public:
	void Append(CCallStack *p);
	void Remove();
	void InitHead();
	int	used;
	int	size;
	CCallStack *pNext;
	CCallStack *pPrev;
	int	addr[STACKDEPTH];

};
*/

struct SBlockType
{
	int	return_addr; 			// first meaningful return addr 
	
	int	size;					// size of block (if we want to sort by it
	int	total;					// total size of this type
	int	actual;					// actual total size, including headers
	char *p_class;				// points to class node 
	
	int	count;
};

// scan throught the list of "used" blocks
// and sort them into a list, organized by "type"
// the "type" is determined by the first return address after
// a callstack entry that is either "Malloc" or "Spt::Class::operator new"
// the "type" is furthur sorted by either "size" or "Class"
// where "size" is the size of the block (for a Malloc)
// and "Class" is the type of class that constructed this block 

#define	MAX_TYPES 10000


void MemView_DumpAnalysis( SBlockType* blocks, int numBlocksToPrint )
{
#ifdef	__LINKED_LIST_HEAP__
	// Sorts the types, and print out totals

	int temp;

	for (int i = 0; i < numBlocksToPrint; i++)
	{
		for (int j = i+1;j<numBlocksToPrint;j++)
		{
			bool swap = false;
			if (blocks[i].actual < blocks[j].actual)
			{
				swap = true;
			}

			if (swap)
			{
				SBlockType t = blocks[i];
				blocks[i] = blocks[j];
				blocks[j] = t;				
			}			
		}

		// i is sorted, so print it out
		printf ("%7d bytes, (%6d hdrs) %4d blks, avg %6d bytes, class %s, function %s\n", 		
				blocks[i].actual,
				blocks[i].actual-blocks[i].total,
				blocks[i].count,				 
				blocks[i].total/blocks[i].count,
				blocks[i].p_class,
				MemView_GetFunctionName(blocks[i].return_addr,&temp)
			   );	   
		for (int xx=0;xx<2000000;xx++);		// little delay, to allow printfs to work
	}
#endif
}

void MemView_AnalyzeCallStack( CCallStack* pCallStack, SBlockType* pBlocks, int& num )
{
	// for each block we find the three things:
	//  return_addr after Malloc or Spt::Class::operator new
	//  size
	//  class

	int	size = pCallStack->size;	  	// size is the only thing we know for sure
	int	return_addr = 0;				// default unknown return address
	char *p_class = "not a class";
	int latest = 1;
	int i = 0;
	
	for ( i = 1; i < 8; i++ )
	{
		int xsize;

		/*
		// the types of call stack we may encounter:
		// need to 
			0x10be48: Mem::Heap::allocate                                                   
			0x109914: Mem::Manager::New                                                     
			0x1035b0: Spt::Class::operator new                                              
			0x161094: Front::KeyboardControl::sCreateInstance   

			0x10be48: Mem::Heap::allocate                                                   
			0x109914: Mem::Manager::New                                                     
			0x10a150: Malloc                                                                
			0x222df8: _SkyBuildPktForUpLoadAlignedContiguousRectangle  

			0x10be48: Mem::Heap::allocate                                                   
			0x109914: Mem::Manager::New                                                     
			0x10a210: Malloc_FreeList                                                       
			0x257034: _rwFreeListAllocReal
		*/

		char *p_name = MemView_GetFunctionName(pCallStack->addr[i],&xsize);
		if (!strcmp("Malloc",p_name) 
			|| !strcmp("Spt::Class::operator new",p_name)
			|| !strcmp("Mem::Manager::New",p_name)
			|| !strcmp("_rwFreeListAllocReal",p_name))
		{
			latest = i;
		}
	}

	if (latest != 1)
	{
		return_addr = pCallStack->addr[latest+1];
	}

	p_class = MemView_GetClassName(pCallStack);				// get class
	// right, now we have all the info on this block
	// let's see if we've got one just like it

	//		if (!p_class && !MemView_GetFunctionName(return_addr,&temp))
	/*		
			if (!return_addr)
			{
				for (int i = 0;i<STACKDEPTH;i++)
				{
					printf ("%2d: >>%s<<\n",i,MemView_GetFunctionName(p->addr[i],&temp));

				}
				return;

			}
	*/
	
									  
	// check if it is a string, and print it out, if so					
/*	
	int temp;				  
	if (!strcmp("Str::String::copy",MemView_GetFunctionName(return_addr,&temp)))
	{
		printf ("String <%s>\n",(char*)((char*)(pCallStack->pBlock)+32)); 
	}
	
	
	if (!strcmp("Front::VerticalMenu::sCreateInstance",MemView_GetFunctionName(return_addr,&temp)))
	{
		void *p_start  =  (void*)((char*)(pCallStack->pBlock)+32);
		printf ("\nVertical Menu "); 
		
		Front::ScreenElement *pV =  (Front::ScreenElement *)p_start;
		printf (" id = %s\n", Script::FindChecksumName(pV->GetID()));
//		MemView_DumpBlockInfo((int)p_start);
		
	}	
*/	

	// check to see if this block is already included
	for ( i = 0; i < num; i++ )
	{
		if ( pBlocks[i].p_class == p_class
			/*&& pBlocks[i].size == size */
			 && pBlocks[i].return_addr == return_addr )
		{
			pBlocks[i].count++;
			pBlocks[i].total += size;
			pBlocks[i].actual += size + Mem::Allocator::BlockHeader::sSize;
			break;				
		}
	}

	// if not, then add the block
	if ( i == num )
	{
		pBlocks[i].p_class = p_class;
		pBlocks[i].size = size;
		pBlocks[i].total = size;
		pBlocks[i].actual = size + Mem::Allocator::BlockHeader::sSize;
		pBlocks[i].return_addr = return_addr;
		pBlocks[i].count = 1;
		num++;
	}
}

#ifdef	__LINKED_LIST_HEAP__
static int bbb = 0;	   	// compiler patch var, see below
#endif

void MemView_AnalyzeBlocks(uint32 mask)
{
#ifdef	__LINKED_LIST_HEAP__
#if 0
	SBlockType  *pBlocks = (SBlockType  *)_mem_dump_start;	// temp memory
	int	num_blocks = 0;
	int num = 0;

	printf ("\nAnalyzing blocks....\n");
	
	CCallStack *p = used_list.pNext;  
	while (p != &used_list)
	{
		// Get the actualy block we referred to
//		Mem::Allocator::BlockHeader * pBlock = p->pBlock;
//		void * p_start = (void*)((uint)pBlock + Mem::Allocator::BlockHeader::sSize);
		// Otionally check to see if it on the front end heap
//		if (Mem::SameContext(p_start,Mem::Manager::sHandle().FrontEndHeap()))
		{
			if (!mask || !(p->flags & mask))
			{
				MemView_AnalyzeCallStack( p, pBlocks, num );			
				num_blocks++;
			}
		}
		p = p->pNext; 
	}
	
	printf ("%d types, in %d total blocks\n", num, num_blocks); 

	MemView_DumpAnalysis( pBlocks, num );
	if (bbb)
	{
		MemView_DumpBottomFragments();			// just to get it compiling 
		MemView_DumpTopFragments();			// just to get it compiling 
	}		
#endif
#endif
}


void MemView_MarkBlocks(uint32 mask)
{
#ifdef	__LINKED_LIST_HEAP__

	CCallStack *p = used_list.pNext;  
	while (p != &used_list)
	{
		p->flags |= mask;
		
		p = p->pNext;
	}
#endif
}



void MemView_Input(uint buttons, uint makes, uint breaks)
{

	if (Config::CD())
	{
		return;
	}	


//	if (makes & Inp::Data::mD_TRIANGLE)
//	{
//		MemView_Active = !MemView_Active;
//	}


	if (!MemView_Active)
	{
		return;
	}

					  
	float step1 = step;
	
	float zoom = 1.1f;
	
	float scroll = 4.0f;

	



	if (buttons & Inp::Data::mD_LEFT)
	{
		step1 = step * zoom;
	}
	if (buttons & Inp::Data::mD_RIGHT)
	{
		step1 = step / zoom;
	}
	
	if (buttons & Inp::Data::mD_UP)
	{
//		start = start - scroll * 512.0f * 2.0f * step;
		cursor = cursor - scroll * 512.0f * 2.0f * step;
	}
	if (buttons & Inp::Data::mD_DOWN)
	{
//		start = start + scroll * 512.0f * 2.0f * step;
		cursor = cursor + scroll * 512.0f * 2.0f * step;
	}

	if (buttons & Inp::Data::mD_L1)
	{
//		start = start - scroll * 512.0f * 2.0f * step / 256.0f;
		cursor = cursor - scroll * 2.0f * 2.0f * step;
	}
	if (buttons & Inp::Data::mD_L2)
	{
//		start = start + scroll * 512.0f * 2.0f * step / 256.0f;
		cursor = cursor + scroll * 2.0f * 2.0f * step;
	}

#define 	MMMIN 	(0.0078125f)
	 				
	if (step1 <MMMIN)
	{
		step1 = MMMIN;
	}
	
	if (step1>1024.0f)
	{
		step1 = 1024.0f;
	}

//	start = start + (512.0f * 2.0f * 128.0f * (step - step1));

	step = step1;

	if (makes & Inp::Data::mD_CIRCLE)
	{
		if (blockstart)
		{
			MemView_DumpRefs(blockstart);
		}
//		MemView_MarkBlocks(1);
	}

	// We don't look for leaks automatically now, so I'v put it on "SQUARE"	
	if (makes & Inp::Data::mD_SQUARE)
	{
		MemView_FindLeaks();
//	Mem::Manager& mem_man = Mem::Manager::sHandle();		MemView_DumpHeap(1);
//	heap	= mem_man.TopDownHeap();
//	MemView_DumpFragments(heap);
//	MemView_DumpHeap(heap,1);

	}

	if (makes & Inp::Data::mD_X)
	{
		MemView_AnalyzeBlocks();
	}

	// Triangle = Dump Fragmentation
/*	if (makes & Inp::Data::mD_TRIANGLE)
	{
		Mem::Manager& mem_man = Mem::Manager::sHandle();
		Mem::Heap* heap = mem_man.BottomUpHeap();
		Mem::Region* region = heap->ParentRegion();
		printf ("BottomUp Frag %dK, %d Blocks\n",heap->mFreeMem.m_count / 1024, heap->mFreeBlocks.m_count);
		printf ("Region %d/%d K", region->MemAvailable() / 1024, region->TotalSize() / 1024 );
		MemView_DumpFragments(heap);
	}
*/	

}

void MemView_AnalyzeHeap(Mem::Heap *pHeap)
{
	if ( !pHeap )
		return;
	
#ifdef	__LINKED_LIST_HEAP__
#if 0
	SBlockType  *pBlocks = (SBlockType  *)_mem_dump_start;	// temp memory
	int	num_blocks = 0;
	int num = 0;

	Mem::Allocator::BlockHeader *p_full = pHeap->mp_context->mp_used_list;

	while (p_full)
	{
		CCallStack* pCallStack = (CCallStack*)p_full->mp_debug_data;

		if ( pCallStack )
		{
			MemView_AnalyzeCallStack( pCallStack, pBlocks, num );
		}
		else
		{
			printf ("Block with No Debug Info!!\n");
		}

		p_full = p_full->mp_next_used;
	}

	printf ("%d types, in %d total blocks\n", num, num_blocks); 

	MemView_DumpAnalysis( pBlocks, num );
#endif
#endif
}
  
  

