////

#ifndef	__P_MEMVIEW_H__
#define	__P_MEMVIEW_H__

void MemView_Display();
void MemView_Input(uint buttons, uint makes, uint breaks);
void MemView_Alloc( void *v);
void MemView_Free( void *v);
void MemViewToggle();
void MemView_FindLeaks();
int DumpUnwindStack( int iMaxDepth, int *pDest );
char *MemView_GetFunctionName(int pc, int *p_size);
void MemView_DumpFragments(Mem::Heap *pHeap);
void MemView_AnalyzeBlocks(uint32 mask = 0);
void MemView_MarkBlocks(uint32 flags = 1 );
void MemView_DumpHeap(Mem::Heap *pHeap, uint32 mask =0);
void MemView_AnalyzeHeap(Mem::Heap *pHeap);

#endif

