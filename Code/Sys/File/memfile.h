///////////////////////////////////////////////////////////////////////////////
//
// memfile.h
//
// memory mapped file replacement functions
// allow you to load a file to memory, and then use these MemFile:: functions
// instead of File:: functions, to load it exactly the same as before
// so you would replace
// 	File::Read(
// with
//	MEM_Read(
//
// Note that since these are macros, they dont return anything
//
#define MEM_Read( addr, size, count, pFP )	   \
{											   \
	char *p = (char*)(addr);				   \
	char *q = (char*)(pFP);					   \
	int total = (size)*(count);				   \
	for (;total>0;total--)					   \
		*p++ = *q++;						   \
	pFP = (void*) q;						   \
}											   \

#define MEM_Seek(pFP, offset, origin )	   \
{										   \
	pFP = (void*)((char*)(pFP) + (offset));  \
}										   \

